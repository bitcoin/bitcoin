// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tinyformat.h>
#include <util/bip32.h>
#include <util/strencodings.h>

#include <cstdint>
#include <cstdio>
#include <sstream>

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
        size_t pos = item.find('\'');
        if (pos != std::string::npos) {
            // The hardened tick can only be in the last index of the string
            if (pos != item.size() - 1) {
                return false;
            }
            path |= 0x80000000;
            item = item.substr(0, item.size() - 1); // Drop the last character which is the hardened tick
        }

        // Ensure this is only numbers
        const auto number{ToIntegral<uint32_t>(item)};
        if (!number) {
            return false;
        }
        path |= *number;

        keypath.push_back(path);
        first = false;
    }
    return true;
}

std::string FormatHDKeypath(const std::vector<uint32_t>& path, bool apostrophe)
{
    std::string ret;
    for (auto i : path) {
        ret += strprintf("/%i", (i << 1) >> 1);
        if (i >> 31) ret += apostrophe ? '\'' : 'h';
    }
    return ret;
}

std::string WriteHDKeypath(const std::vector<uint32_t>& keypath, bool apostrophe)
{
    return "m" + FormatHDKeypath(keypath, apostrophe);
}
