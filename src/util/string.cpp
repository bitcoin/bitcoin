// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

#include <regex>
#include <string>

namespace util {

// Compile-time sanity checks
static_assert([] {
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("%");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("%%");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("%%%");
    ConstevalFormatString<1>::CheckNumFormatSpecifiers("%_");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("_%");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("%%_");
    ConstevalFormatString<1>::CheckNumFormatSpecifiers("%_%");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("_%%");
    ConstevalFormatString<1>::CheckNumFormatSpecifiers("%%%_");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("%%_%");
    ConstevalFormatString<1>::CheckNumFormatSpecifiers("%_%%");
    ConstevalFormatString<0>::CheckNumFormatSpecifiers("_%%%");
    return true; // All checks above compiled and passed
}());

void ReplaceAll(std::string& in_out, const std::string& search, const std::string& substitute)
{
    if (search.empty()) return;
    in_out = std::regex_replace(in_out, std::regex(search), substitute);
}
} // namespace util
