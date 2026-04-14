// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bip32.h>

#include <tinyformat.h>
#include <util/strencodings.h>

#include <cstdint>
#include <optional>
#include <sstream>
#include <string_view>

std::optional<uint32_t> ParseKeyPathElement(std::span<const char> elem, bool& is_hardened, std::string& error)
{
    is_hardened = false;
    const std::string_view raw{elem.begin(), elem.end()};
    if (elem.empty()) {
        error = strprintf("Key path value '%s' is not valid", raw);
        return std::nullopt;
    }

    const char last = elem.back();
    if (last == '\'' || last == 'h') {
        elem = elem.first(elem.size() - 1);
        is_hardened = true;
    }

    const auto number{ToIntegral<uint32_t>(std::string_view{elem.begin(), elem.end()})};
    if (!number) {
        error = strprintf("Key path value '%s' is not a valid uint32", raw);
        return std::nullopt;
    }
    if (*number >= BIP32_HARDENED_FLAG) {
        error = strprintf("Key path value %u is out of range", *number);
        return std::nullopt;
    }
    return *number | (is_hardened ? BIP32_HARDENED_FLAG : BIP32_UNHARDENED_FLAG);
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
        bool hardened = false;
        std::string error;
        const auto index{ParseKeyPathElement(std::span<const char>{item.data(), item.size()}, hardened, error)};
        if (!index.has_value()) return false;
        keypath.push_back(*index);
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
