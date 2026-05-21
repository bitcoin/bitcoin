// Copyright 2014 BitPay Inc.
// Copyright 2015 Bitcoin Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#include <univalue.h>

#include <cstring>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
static bool ParsePrechecks(const std::string& str)
{
    if (str.empty()) // No empty string allowed
        return false;
    if (str.size() >= 1 && (json_isspace(str[0]) || json_isspace(str[str.size()-1]))) // No padding allowed
        return false;
    if (str.size() != strlen(str.c_str())) // No embedded NUL characters allowed
        return false;
    return true;
}

std::optional<double> ParseDouble(const std::string& str)
{
    if (!ParsePrechecks(str))
        return std::nullopt;
    if (str.size() >= 2 && str[0] == '0' && str[1] == 'x') // No hexadecimal floats allowed
        return std::nullopt;
    std::istringstream text(str);
    text.imbue(std::locale::classic());
    double result;
    text >> result;
    if (!text.eof() || text.fail()) {
        return std::nullopt;
    }
    return result;
}
}

const std::vector<std::string>& UniValue::getKeys() const
{
    checkType(VOBJ);
    return keys;
}

const std::vector<UniValue>& UniValue::getValues() const
{
    if (typ != VOBJ && typ != VARR)
        throw std::runtime_error("JSON value is not an object or array as expected");
    return values;
}

bool UniValue::get_bool() const
{
    checkType(VBOOL);
    return isTrue();
}

const std::string& UniValue::get_str() const
{
    checkType(VSTR);
    return getValStr();
}

double UniValue::get_real() const
{
    checkType(VNUM);
    if (const auto retval{ParseDouble(getValStr())}) {
        return *retval;
    }
    throw std::runtime_error("JSON double out of range");
}

const UniValue& UniValue::get_obj() const
{
    checkType(VOBJ);
    return *this;
}

const UniValue& UniValue::get_array() const
{
    checkType(VARR);
    return *this;
}
