// Copyright 2014 BitPay Inc.
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#include <univalue.h>
#include <univalue_utffilter.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

/*
 * According to stackexchange, the original json test suite wanted
 * to limit depth to 22.  Widely-deployed PHP bails at depth 512,
 * so we will follow PHP's lead, which should be more than sufficient
 * (further stackexchange comments indicate depth > 32 rarely occurs).
 */
static constexpr size_t MAX_JSON_DEPTH = 512;

static bool json_isdigit(int ch)
{
    return ((ch >= '0') && (ch <= '9'));
}

// convert hexadecimal string to unsigned integer
static const char *hatoui(const char *first, const char *last,
                          unsigned int& out)
{
    unsigned int result = 0;
    for (; first != last; ++first)
    {
        int digit;
        if (json_isdigit(*first))
            digit = *first - '0';

        else if (*first >= 'a' && *first <= 'f')
            digit = *first - 'a' + 10;

        else if (*first >= 'A' && *first <= 'F')
            digit = *first - 'A' + 10;

        else
            break;

        result = 16 * result + digit;
    }
    out = result;

    return first;
}

enum jtokentype getJsonToken(std::string& tokenVal, unsigned int& consumed,
                            const char *raw, const char *end)
{
    tokenVal.clear();
    consumed = 0;

    const char *rawStart = raw;

    while (raw < end && (json_isspace(*raw)))          // skip whitespace
        raw++;

    if (raw >= end)
        return JTOK_NONE;

    switch (*raw) {

    case '{':
        raw++;
        consumed = (raw - rawStart);
        return JTOK_OBJ_OPEN;
    case '}':
        raw++;
        consumed = (raw - rawStart);
        return JTOK_OBJ_CLOSE;
    case '[':
        raw++;
        consumed = (raw - rawStart);
        return JTOK_ARR_OPEN;
    case ']':
        raw++;
        consumed = (raw - rawStart);
        return JTOK_ARR_CLOSE;

    case ':':
        raw++;
        consumed = (raw - rawStart);
        return JTOK_COLON;
    case ',':
        raw++;
        consumed = (raw - rawStart);
        return JTOK_COMMA;

    case 'n':
    case 't':
    case 'f':
        if (!strncmp(raw, "null", 4)) {
            raw += 4;
            consumed = (raw - rawStart);
            return JTOK_KW_NULL;
        } else if (!strncmp(raw, "true", 4)) {
            raw += 4;
            consumed = (raw - rawStart);
            return JTOK_KW_TRUE;
        } else if (!strncmp(raw, "false", 5)) {
            raw += 5;
            consumed = (raw - rawStart);
            return JTOK_KW_FALSE;
        } else
            return JTOK_ERR;

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        // part 1: int
        std::string numStr;

        const char *first = raw;

        const char *firstDigit = first;
        if (!json_isdigit(*firstDigit))
            firstDigit++;
        if ((*firstDigit == '0') && json_isdigit(firstDigit[1]))
            return JTOK_ERR;

        numStr += *raw;                       // copy first char
        raw++;

        if ((*first == '-') && (raw < end) && (!json_isdigit(*raw)))
            return JTOK_ERR;

        while (raw < end && json_isdigit(*raw)) {  // copy digits
            numStr += *raw;
            raw++;
        }

        // part 2: frac
        if (raw < end && *raw == '.') {
            numStr += *raw;                   // copy .
            raw++;

            if (raw >= end || !json_isdigit(*raw))
                return JTOK_ERR;
            while (raw < end && json_isdigit(*raw)) { // copy digits
                numStr += *raw;
                raw++;
            }
        }

        // part 3: exp
        if (raw < end && (*raw == 'e' || *raw == 'E')) {
            numStr += *raw;                   // copy E
            raw++;

            if (raw < end && (*raw == '-' || *raw == '+')) { // copy +/-
                numStr += *raw;
                raw++;
            }

            if (raw >= end || !json_isdigit(*raw))
                return JTOK_ERR;
            while (raw < end && json_isdigit(*raw)) { // copy digits
                numStr += *raw;
                raw++;
            }
        }

        tokenVal = numStr;
        consumed = (raw - rawStart);
        return JTOK_NUMBER;
        }

    case '"': {
        raw++;                                // skip "

        std::string valStr;
        JSONUTF8StringFilter writer(valStr);

        while (true) {
            if (raw >= end || (unsigned char)*raw < 0x20)
                return JTOK_ERR;

            else if (*raw == '\\') {
                raw++;                        // skip backslash

                if (raw >= end)
                    return JTOK_ERR;

                switch (*raw) {
                case '"':  writer.push_back('\"'); break;
                case '\\': writer.push_back('\\'); break;
                case '/':  writer.push_back('/'); break;
                case 'b':  writer.push_back('\b'); break;
                case 'f':  writer.push_back('\f'); break;
                case 'n':  writer.push_back('\n'); break;
                case 'r':  writer.push_back('\r'); break;
                case 't':  writer.push_back('\t'); break;

                case 'u': {
                    unsigned int codepoint;
                    if (raw + 1 + 4 >= end ||
                        hatoui(raw + 1, raw + 1 + 4, codepoint) !=
                               raw + 1 + 4)
                        return JTOK_ERR;
                    writer.push_back_u(codepoint);
                    raw += 4;
                    break;
                    }
                default:
                    return JTOK_ERR;

                }

                raw++;                        // skip esc'd char
            }

            else if (*raw == '"') {
                raw++;                        // skip "
                break;                        // stop scanning
            }

            else {
                writer.push_back(static_cast<unsigned char>(*raw));
                raw++;
            }
        }

        if (!writer.finalize())
            return JTOK_ERR;
        tokenVal = valStr;
        consumed = (raw - rawStart);
        return JTOK_STRING;
        }

    default:
        return JTOK_ERR;
    }
}

enum expect_bits : unsigned {
    EXP_OBJ_NAME = (1U << 0),
    EXP_COLON = (1U << 1),
    EXP_ARR_VALUE = (1U << 2),
    EXP_VALUE = (1U << 3),
    EXP_NOT_VALUE = (1U << 4),
};

#define expect(bit) (expectMask & (EXP_##bit))
#define setExpect(bit) (expectMask |= EXP_##bit)
#define clearExpect(bit) (expectMask &= ~EXP_##bit)

bool UniValue::read(std::string_view str_in)
{
    clear();

    uint32_t expectMask = 0;
    std::vector<UniValue*> stack;

    std::string tokenVal;
    unsigned int consumed;
    enum jtokentype tok = JTOK_NONE;
    enum jtokentype last_tok = JTOK_NONE;
    const char* raw{str_in.data()};
    const char* end{raw + str_in.size()};
    do {
        last_tok = tok;

        tok = getJsonToken(tokenVal, consumed, raw, end);
        if (tok == JTOK_NONE || tok == JTOK_ERR)
            return false;
        raw += consumed;

        bool isValueOpen = jsonTokenIsValue(tok) ||
            tok == JTOK_OBJ_OPEN || tok == JTOK_ARR_OPEN;

        if (expect(VALUE)) {
            if (!isValueOpen)
                return false;
            clearExpect(VALUE);

        } else if (expect(ARR_VALUE)) {
            bool isArrValue = isValueOpen || (tok == JTOK_ARR_CLOSE);
            if (!isArrValue)
                return false;

            clearExpect(ARR_VALUE);

        } else if (expect(OBJ_NAME)) {
            bool isObjName = (tok == JTOK_OBJ_CLOSE || tok == JTOK_STRING);
            if (!isObjName)
                return false;

        } else if (expect(COLON)) {
            if (tok != JTOK_COLON)
                return false;
            clearExpect(COLON);

        } else if (!expect(COLON) && (tok == JTOK_COLON)) {
            return false;
        }

        if (expect(NOT_VALUE)) {
            if (isValueOpen)
                return false;
            clearExpect(NOT_VALUE);
        }

        switch (tok) {

        case JTOK_OBJ_OPEN:
        case JTOK_ARR_OPEN: {
            VType utyp = (tok == JTOK_OBJ_OPEN ? VOBJ : VARR);
            if (!stack.size()) {
                if (utyp == VOBJ)
                    setObject();
                else
                    setArray();
                stack.push_back(this);
            } else {
                UniValue tmpVal(utyp);
                UniValue *top = stack.back();
                top->values.push_back(tmpVal);

                UniValue *newTop = &(top->values.back());
                stack.push_back(newTop);
            }

            if (stack.size() > MAX_JSON_DEPTH)
                return false;

            if (utyp == VOBJ)
                setExpect(OBJ_NAME);
            else
                setExpect(ARR_VALUE);
            break;
            }

        case JTOK_OBJ_CLOSE:
        case JTOK_ARR_CLOSE: {
            if (!stack.size() || (last_tok == JTOK_COMMA))
                return false;

            VType utyp = (tok == JTOK_OBJ_CLOSE ? VOBJ : VARR);
            UniValue *top = stack.back();
            if (utyp != top->getType())
                return false;

            stack.pop_back();
            clearExpect(OBJ_NAME);
            setExpect(NOT_VALUE);
            break;
            }

        case JTOK_COLON: {
            if (!stack.size())
                return false;

            UniValue *top = stack.back();
            if (top->getType() != VOBJ)
                return false;

            setExpect(VALUE);
            break;
            }

        case JTOK_COMMA: {
            if (!stack.size() ||
                (last_tok == JTOK_COMMA) || (last_tok == JTOK_ARR_OPEN))
                return false;

            UniValue *top = stack.back();
            if (top->getType() == VOBJ)
                setExpect(OBJ_NAME);
            else
                setExpect(ARR_VALUE);
            break;
            }

        case JTOK_KW_NULL:
        case JTOK_KW_TRUE:
        case JTOK_KW_FALSE: {
            UniValue tmpVal;
            switch (tok) {
            case JTOK_KW_NULL:
                // do nothing more
                break;
            case JTOK_KW_TRUE:
                tmpVal.setBool(true);
                break;
            case JTOK_KW_FALSE:
                tmpVal.setBool(false);
                break;
            default: /* impossible */ break;
            }

            if (!stack.size()) {
                *this = tmpVal;
                break;
            }

            UniValue *top = stack.back();
            top->values.push_back(tmpVal);

            setExpect(NOT_VALUE);
            break;
            }

        case JTOK_NUMBER: {
            UniValue tmpVal(VNUM, tokenVal);
            if (!stack.size()) {
                *this = tmpVal;
                break;
            }

            UniValue *top = stack.back();
            top->values.push_back(tmpVal);

            setExpect(NOT_VALUE);
            break;
            }

        case JTOK_STRING: {
            if (expect(OBJ_NAME)) {
                UniValue *top = stack.back();
                top->keys.push_back(tokenVal);
                clearExpect(OBJ_NAME);
                setExpect(COLON);
            } else {
                UniValue tmpVal(VSTR, tokenVal);
                if (!stack.size()) {
                    *this = tmpVal;
                    break;
                }
                UniValue *top = stack.back();
                top->values.push_back(tmpVal);
            }

            setExpect(NOT_VALUE);
            break;
            }

        default:
            return false;
        }
    } while (!stack.empty ());

    /* Check that nothing follows the initial construct (parsed above).  */
    tok = getJsonToken(tokenVal, consumed, raw, end);
    if (tok != JTOK_NONE)
        return false;

    return true;
}

