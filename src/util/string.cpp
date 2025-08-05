// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

#include <regex>
#include <string>

namespace util {
void ReplaceAll(std::string& in_out, const std::string& search, const std::string& substitute, bool regex)
{
    if (search.empty()) return;
    if (regex) {
        in_out = std::regex_replace(in_out, std::regex(search), substitute);
        return;
    }
    size_t pos{0};
    while ((pos = in_out.find(search, pos)) != std::string::npos) {
        in_out.replace(pos, search.size(), substitute);
        ++pos;
    }
}
} // namespace util
