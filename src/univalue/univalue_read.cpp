// Copyright 2014 BitPay Inc.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>
#include <vector>
#include <stdio.h>
#include "univalue.h"

using namespace std;

// convert hexadecimal string to unsigned integer
static const char *hatoui(const char *first, const char *last,
                          unsigned int& out)
{
    unsigned int result = 0;
    for (; first != last; ++first)
    {
        int digit;
        if (isdigit(*first))
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

enum jtokentype getJsonToken(string& tokenVal, unsigned int& consumed,
                            const char *raw)
{
    tokenVal.clear();
    consumed = 0;

    const char *rawStart = raw;

    while ((*raw) && (isspace(*raw)))             // skip whitespace
        raw++;

    switch (*raw) {

    case 0:
        return JTOK_NONE;

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
        string numStr;

        const char *first = raw;

        const char *firstDigit = first;
        if (!isdigit(*firstDigit))
            firstDigit++;
        if ((*firstDigit == '0') && isdigit(firstDigit[1]))
            return JTOK_ERR;

        numStr += *raw;                       // copy first char
        raw++;

        if ((*first == '-') && (!isdigit(*raw)))
            return JTOK_ERR;

        while ((*raw) && isdigit(*raw)) {     // copy digits
            numStr += *raw;
            raw++;
        }

        // part 2: frac
        if (*raw == '.') {
            numStr += *raw;                   // copy .
            raw++;

            if (!isdigit(*raw))
                return JTOK_ERR;
            while ((*raw) && isdigit(*raw)) { // copy digits
                numStr += *raw;
                raw++;
            }
        }

        // part 3: exp
        if (*raw == 'e' || *raw == 'E') {
            numStr += *raw;                   // copy E
            raw++;

            if (*raw == '-' || *raw == '+') { // copy +/-
                numStr += *raw;
                raw++;
            }

            if (!isdigit(*raw))
                return JTOK_ERR;
            while ((*raw) && isdigit(*raw)) { // copy digits
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

        string valStr;

        while (*raw) {
            if (*raw < 0x20)
                return JTOK_ERR;

            else if (*raw == '\\') {
                raw++;                        // skip backslash

                switch (*raw) {
                case '"':  valStr += "\""; break;
                case '\\': valStr += "\\"; break;
                case '/':  valStr += "/"; break;
                case 'b':  valStr += "\b"; break;
                case 'f':  valStr += "\f"; break;
                case 'n':  valStr += "\n"; break;
                case 'r':  valStr += "\r"; break;
                case 't':  valStr += "\t"; break;

                case 'u': {
                    char buf[4] = {0,0,0,0};
                    char *last = &buf[0];
                    unsigned int codepoint;
                    if (hatoui(raw + 1, raw + 1 + 4, codepoint) !=
                               raw + 1 + 4)
                        return JTOK_ERR;

                    if (codepoint <= 0x7f)
                         *last = (char)codepoint;
                    else if (codepoint <= 0x7FF) {
                        *last++ = (char)(0xC0 | (codepoint >> 6));
                        *last = (char)(0x80 | (codepoint & 0x3F));
                    } else if (codepoint <= 0xFFFF) {
                        *last++ = (char)(0xE0 | (codepoint >> 12));
                        *last++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                        *last = (char)(0x80 | (codepoint & 0x3F));
                    }

                    valStr += buf;
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
                valStr += *raw;
                raw++;
            }
        }

        tokenVal = valStr;
        consumed = (raw - rawStart);
        return JTOK_STRING;
        }

    default:
        return JTOK_ERR;
    }
}

bool UniValue::read(const char *raw)
{
    clear();

    bool expectName = false;
    bool expectColon = false;
    vector<UniValue*> stack;

    enum jtokentype tok = JTOK_NONE;
    enum jtokentype last_tok = JTOK_NONE;
    while (1) {
        last_tok = tok;

        string tokenVal;
        unsigned int consumed;
        tok = getJsonToken(tokenVal, consumed, raw);
        if (tok == JTOK_NONE || tok == JTOK_ERR)
            break;
        raw += consumed;

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

            if (utyp == VOBJ)
                expectName = true;
            break;
            }

        case JTOK_OBJ_CLOSE:
        case JTOK_ARR_CLOSE: {
            if (!stack.size() || expectColon || (last_tok == JTOK_COMMA))
                return false;

            VType utyp = (tok == JTOK_OBJ_CLOSE ? VOBJ : VARR);
            UniValue *top = stack.back();
            if (utyp != top->getType())
                return false;

            stack.pop_back();
            expectName = false;
            break;
            }

        case JTOK_COLON: {
            if (!stack.size() || expectName || !expectColon)
                return false;

            UniValue *top = stack.back();
            if (top->getType() != VOBJ)
                return false;

            expectColon = false;
            break;
            }

        case JTOK_COMMA: {
            if (!stack.size() || expectName || expectColon ||
                (last_tok == JTOK_COMMA) || (last_tok == JTOK_ARR_OPEN))
                return false;

            UniValue *top = stack.back();
            if (top->getType() == VOBJ)
                expectName = true;
            break;
            }

        case JTOK_KW_NULL:
        case JTOK_KW_TRUE:
        case JTOK_KW_FALSE: {
            if (!stack.size() || expectName || expectColon)
                return false;

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

            UniValue *top = stack.back();
            top->values.push_back(tmpVal);

            break;
            }

        case JTOK_NUMBER: {
            if (!stack.size() || expectName || expectColon)
                return false;

            UniValue tmpVal(VNUM, tokenVal);
            UniValue *top = stack.back();
            top->values.push_back(tmpVal);

            break;
            }

        case JTOK_STRING: {
            if (!stack.size())
                return false;

            UniValue *top = stack.back();

            if (expectName) {
                top->keys.push_back(tokenVal);
                expectName = false;
                expectColon = true;
            } else {
                UniValue tmpVal(VSTR, tokenVal);
                top->values.push_back(tmpVal);
            }

            break;
            }

        default:
            return false;
        }
    }

    if (stack.size() != 0)
        return false;

    return true;
}

