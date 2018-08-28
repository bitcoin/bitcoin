// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <utilstrencodings.h>

#include <tinyformat.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <limits>

static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const std::string SAFE_CHARS[] =
{
    CHARS_ALPHA_NUM + " .,;-_/:?@()", // SAFE_CHARS_DEFAULT
    CHARS_ALPHA_NUM + " .,;-_?@", // SAFE_CHARS_UA_COMMENT
    CHARS_ALPHA_NUM + ".-_", // SAFE_CHARS_FILENAME
};

std::string SanitizeString(const std::string& str, int rule)
{
    std::string strResult;
    for (std::string::size_type i = 0; i < str.size(); i++)
    {
        if (SAFE_CHARS[rule].find(str[i]) != std::string::npos)
            strResult.push_back(str[i]);
    }
    return strResult;
}

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

signed char HexDigit(char c)
{
    return p_util_hexdigit[(unsigned char)c];
}

bool IsHex(const std::string& str)
{
    for(std::string::const_iterator it(str.begin()); it != str.end(); ++it)
    {
        if (HexDigit(*it) < 0)
            return false;
    }
    return (str.size() > 0) && (str.size()%2 == 0);
}

bool IsHexNumber(const std::string& str)
{
    size_t starting_location = 0;
    if (str.size() > 2 && *str.begin() == '0' && *(str.begin()+1) == 'x') {
        starting_location = 2;
    }
    for (auto c : str.substr(starting_location)) {
        if (HexDigit(c) < 0) return false;
    }
    // Return false for empty string or "0x".
    return (str.size() > starting_location);
}

std::vector<unsigned char> ParseHex(const char* psz)
{
    // convert hex dump to vector
    std::vector<unsigned char> vch;
    while (true)
    {
        while (isspace(*psz))
            psz++;
        signed char c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        unsigned char n = (c << 4);
        c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        n |= c;
        vch.push_back(n);
    }
    return vch;
}

std::vector<unsigned char> ParseHex(const std::string& str)
{
    return ParseHex(str.c_str());
}

void SplitHostPort(std::string in, int &portOut, std::string &hostOut) {
    size_t colon = in.find_last_of(':');
    // if a : is found, and it either follows a [...], or no other : is in the string, treat it as port separator
    bool fHaveColon = colon != in.npos;
    bool fBracketed = fHaveColon && (in[0]=='[' && in[colon-1]==']'); // if there is a colon, and in[0]=='[', colon is not 0, so in[colon-1] is safe
    bool fMultiColon = fHaveColon && (in.find_last_of(':',colon-1) != in.npos);
    if (fHaveColon && (colon==0 || fBracketed || !fMultiColon)) {
        int32_t n;
        if (ParseInt32(in.substr(colon + 1), &n) && n > 0 && n < 0x10000) {
            in = in.substr(0, colon);
            portOut = n;
        }
    }
    if (in.size()>0 && in[0] == '[' && in[in.size()-1] == ']')
        hostOut = in.substr(1, in.size()-2);
    else
        hostOut = in;
}

std::string EncodeBase64(const unsigned char* pch, size_t len)
{
    static const char *pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string str;
    str.reserve(((len + 2) / 3) * 4);
    ConvertBits<8, 6, true>([&](int v) { str += pbase64[v]; }, pch, pch + len);
    while (str.size() % 4) str += '=';
    return str;
}

std::string EncodeBase64(const std::string& str)
{
    return EncodeBase64((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase64(const char* p, bool* pfInvalid)
{
    static const int decode64_table[256] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    const char* e = p;
    std::vector<uint8_t> val;
    val.reserve(strlen(p));
    while (*p != 0) {
        int x = decode64_table[(unsigned char)*p];
        if (x == -1) break;
        val.push_back(x);
        ++p;
    }

    std::vector<unsigned char> ret;
    ret.reserve((val.size() * 3) / 4);
    bool valid = ConvertBits<6, 8, false>([&](unsigned char c) { ret.push_back(c); }, val.begin(), val.end());

    const char* q = p;
    while (valid && *p != 0) {
        if (*p != '=') {
            valid = false;
            break;
        }
        ++p;
    }
    valid = valid && (p - e) % 4 == 0 && p - q < 4;
    if (pfInvalid) *pfInvalid = !valid;

    return ret;
}

std::string DecodeBase64(const std::string& str)
{
    std::vector<unsigned char> vchRet = DecodeBase64(str.c_str());
    return std::string((const char*)vchRet.data(), vchRet.size());
}

std::string EncodeBase32(const unsigned char* pch, size_t len)
{
    static const char *pbase32 = "abcdefghijklmnopqrstuvwxyz234567";

    std::string str;
    str.reserve(((len + 4) / 5) * 8);
    ConvertBits<8, 5, true>([&](int v) { str += pbase32[v]; }, pch, pch + len);
    while (str.size() % 8) str += '=';
    return str;
}

std::string EncodeBase32(const std::string& str)
{
    return EncodeBase32((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid)
{
    static const int decode32_table[256] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1,  0,  1,  2,
         3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    const char* e = p;
    std::vector<uint8_t> val;
    val.reserve(strlen(p));
    while (*p != 0) {
        int x = decode32_table[(unsigned char)*p];
        if (x == -1) break;
        val.push_back(x);
        ++p;
    }

    std::vector<unsigned char> ret;
    ret.reserve((val.size() * 5) / 8);
    bool valid = ConvertBits<5, 8, false>([&](unsigned char c) { ret.push_back(c); }, val.begin(), val.end());

    const char* q = p;
    while (valid && *p != 0) {
        if (*p != '=') {
            valid = false;
            break;
        }
        ++p;
    }
    valid = valid && (p - e) % 8 == 0 && p - q < 8;
    if (pfInvalid) *pfInvalid = !valid;

    return ret;
}

std::string DecodeBase32(const std::string& str)
{
    std::vector<unsigned char> vchRet = DecodeBase32(str.c_str());
    return std::string((const char*)vchRet.data(), vchRet.size());
}

static bool ParsePrechecks(const std::string& str)
{
    if (str.empty()) // No empty string allowed
        return false;
    if (str.size() >= 1 && (isspace(str[0]) || isspace(str[str.size()-1]))) // No padding allowed
        return false;
    if (str.size() != strlen(str.c_str())) // No embedded NUL characters allowed
        return false;
    return true;
}

bool ParseInt32(const std::string& str, int32_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    char *endp = nullptr;
    errno = 0; // strtol will not set errno if valid
    long int n = strtol(str.c_str(), &endp, 10);
    if(out) *out = (int32_t)n;
    // Note that strtol returns a *long int*, so even if strtol doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *int32_t*. On 64-bit
    // platforms the size of these types may be different.
    return endp && *endp == 0 && !errno &&
        n >= std::numeric_limits<int32_t>::min() &&
        n <= std::numeric_limits<int32_t>::max();
}

bool ParseInt64(const std::string& str, int64_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    char *endp = nullptr;
    errno = 0; // strtoll will not set errno if valid
    long long int n = strtoll(str.c_str(), &endp, 10);
    if(out) *out = (int64_t)n;
    // Note that strtoll returns a *long long int*, so even if strtol doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *int64_t*.
    return endp && *endp == 0 && !errno &&
        n >= std::numeric_limits<int64_t>::min() &&
        n <= std::numeric_limits<int64_t>::max();
}

bool ParseUInt32(const std::string& str, uint32_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    if (str.size() >= 1 && str[0] == '-') // Reject negative values, unfortunately strtoul accepts these by default if they fit in the range
        return false;
    char *endp = nullptr;
    errno = 0; // strtoul will not set errno if valid
    unsigned long int n = strtoul(str.c_str(), &endp, 10);
    if(out) *out = (uint32_t)n;
    // Note that strtoul returns a *unsigned long int*, so even if it doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *uint32_t*. On 64-bit
    // platforms the size of these types may be different.
    return endp && *endp == 0 && !errno &&
        n <= std::numeric_limits<uint32_t>::max();
}

bool ParseUInt64(const std::string& str, uint64_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    if (str.size() >= 1 && str[0] == '-') // Reject negative values, unfortunately strtoull accepts these by default if they fit in the range
        return false;
    char *endp = nullptr;
    errno = 0; // strtoull will not set errno if valid
    unsigned long long int n = strtoull(str.c_str(), &endp, 10);
    if(out) *out = (uint64_t)n;
    // Note that strtoull returns a *unsigned long long int*, so even if it doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *uint64_t*.
    return endp && *endp == 0 && !errno &&
        n <= std::numeric_limits<uint64_t>::max();
}


bool ParseDouble(const std::string& str, double *out)
{
    if (!ParsePrechecks(str))
        return false;
    if (str.size() >= 2 && str[0] == '0' && str[1] == 'x') // No hexadecimal floats allowed
        return false;
    std::istringstream text(str);
    text.imbue(std::locale::classic());
    double result;
    text >> result;
    if(out) *out = result;
    return text.eof() && !text.fail();
}

std::string FormatParagraph(const std::string& in, size_t width, size_t indent)
{
    std::stringstream out;
    size_t ptr = 0;
    size_t indented = 0;
    while (ptr < in.size())
    {
        size_t lineend = in.find_first_of('\n', ptr);
        if (lineend == std::string::npos) {
            lineend = in.size();
        }
        const size_t linelen = lineend - ptr;
        const size_t rem_width = width - indented;
        if (linelen <= rem_width) {
            out << in.substr(ptr, linelen + 1);
            ptr = lineend + 1;
            indented = 0;
        } else {
            size_t finalspace = in.find_last_of(" \n", ptr + rem_width);
            if (finalspace == std::string::npos || finalspace < ptr) {
                // No place to break; just include the entire word and move on
                finalspace = in.find_first_of("\n ", ptr);
                if (finalspace == std::string::npos) {
                    // End of the string, just add it and break
                    out << in.substr(ptr);
                    break;
                }
            }
            out << in.substr(ptr, finalspace - ptr) << "\n";
            if (in[finalspace] == '\n') {
                indented = 0;
            } else if (indent) {
                out << std::string(indent, ' ');
                indented = indent;
            }
            ptr = finalspace + 1;
        }
    }
    return out.str();
}

std::string i64tostr(int64_t n)
{
    return strprintf("%d", n);
}

std::string itostr(int n)
{
    return strprintf("%d", n);
}

int64_t atoi64(const char* psz)
{
#ifdef _MSC_VER
    return _atoi64(psz);
#else
    return strtoll(psz, nullptr, 10);
#endif
}

int64_t atoi64(const std::string& str)
{
#ifdef _MSC_VER
    return _atoi64(str.c_str());
#else
    return strtoll(str.c_str(), nullptr, 10);
#endif
}

int atoi(const std::string& str)
{
    return atoi(str.c_str());
}

/** Upper bound for mantissa.
 * 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
 * Larger integers cannot consist of arbitrary combinations of 0-9:
 *
 *   999999999999999999  1^18-1
 *  9223372036854775807  (1<<63)-1  (max int64_t)
 *  9999999999999999999  1^19-1     (would overflow)
 */
static const int64_t UPPER_BOUND = 1000000000000000000LL - 1LL;

/** Helper function for ParseFixedPoint */
static inline bool ProcessMantissaDigit(char ch, int64_t &mantissa, int &mantissa_tzeros)
{
    if(ch == '0')
        ++mantissa_tzeros;
    else {
        for (int i=0; i<=mantissa_tzeros; ++i) {
            if (mantissa > (UPPER_BOUND / 10LL))
                return false; /* overflow */
            mantissa *= 10;
        }
        mantissa += ch - '0';
        mantissa_tzeros = 0;
    }
    return true;
}

bool ParseFixedPoint(const std::string &val, int decimals, int64_t *amount_out)
{
    int64_t mantissa = 0;
    int64_t exponent = 0;
    int mantissa_tzeros = 0;
    bool mantissa_sign = false;
    bool exponent_sign = false;
    int ptr = 0;
    int end = val.size();
    int point_ofs = 0;

    if (ptr < end && val[ptr] == '-') {
        mantissa_sign = true;
        ++ptr;
    }
    if (ptr < end)
    {
        if (val[ptr] == '0') {
            /* pass single 0 */
            ++ptr;
        } else if (val[ptr] >= '1' && val[ptr] <= '9') {
            while (ptr < end && IsDigit(val[ptr])) {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
            }
        } else return false; /* missing expected digit */
    } else return false; /* empty string or loose '-' */
    if (ptr < end && val[ptr] == '.')
    {
        ++ptr;
        if (ptr < end && IsDigit(val[ptr]))
        {
            while (ptr < end && IsDigit(val[ptr])) {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
                ++point_ofs;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr < end && (val[ptr] == 'e' || val[ptr] == 'E'))
    {
        ++ptr;
        if (ptr < end && val[ptr] == '+')
            ++ptr;
        else if (ptr < end && val[ptr] == '-') {
            exponent_sign = true;
            ++ptr;
        }
        if (ptr < end && IsDigit(val[ptr])) {
            while (ptr < end && IsDigit(val[ptr])) {
                if (exponent > (UPPER_BOUND / 10LL))
                    return false; /* overflow */
                exponent = exponent * 10 + val[ptr] - '0';
                ++ptr;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr != end)
        return false; /* trailing garbage */

    /* finalize exponent */
    if (exponent_sign)
        exponent = -exponent;
    exponent = exponent - point_ofs + mantissa_tzeros;

    /* finalize mantissa */
    if (mantissa_sign)
        mantissa = -mantissa;

    /* convert to one 64-bit fixed-point value */
    exponent += decimals;
    if (exponent < 0)
        return false; /* cannot represent values smaller than 10^-decimals */
    if (exponent >= 18)
        return false; /* cannot represent values larger than or equal to 10^(18-decimals) */

    for (int i=0; i < exponent; ++i) {
        if (mantissa > (UPPER_BOUND / 10LL) || mantissa < -(UPPER_BOUND / 10LL))
            return false; /* overflow */
        mantissa *= 10;
    }
    if (mantissa > UPPER_BOUND || mantissa < -UPPER_BOUND)
        return false; /* overflow */

    if (amount_out)
        *amount_out = mantissa;

    return true;
}

bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath)
{
    std::stringstream ss(keypath_str);
    std::string item;
    bool first = true;
    while (std::getline(ss, item, '/')) {
        if (item.compare("m") == 0) {
            if (first) {
                first = false;
                continue;
            }
            return false;
        }
        // Finds whether it is hardened
        uint32_t path = 0;
        size_t pos = item.find("'");
        if (pos != std::string::npos) {
            // The hardened tick can only be in the last index of the string
            if (pos != item.size() - 1) {
                return false;
            }
            path |= 0x80000000;
            item = item.substr(0, item.size() - 1); // Drop the last character which is the hardened tick
        }

        // Ensure this is only numbers
        if (item.find_first_not_of( "0123456789" ) != std::string::npos) {
            return false;
        }
        uint32_t number;
        if (!ParseUInt32(item, &number)) {
            return false;
        }
        path |= number;

        keypath.push_back(path);
        first = false;
    }
    return true;
}

void Downcase(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){return ToLower(c);});
}

std::string Capitalize(std::string str)
{
    if (str.empty()) return str;
    str[0] = ToUpper(str.front());
    return str;
}
