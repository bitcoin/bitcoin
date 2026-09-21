// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bip32.h>

#include <tinyformat.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>

util::Expected<KeyPathElement, std::string> ParseKeyPathElement(std::span<const char> elem)
{
    const std::string_view raw{elem.begin(), elem.end()};
    if (elem.empty()) {
        return util::Unexpected{strprintf("Key path value '%s' is not valid", raw)};
    }

    bool is_hardened = false;
    const char last = elem.back();
    if (last == '\'' || last == 'h') {
        elem = elem.first(elem.size() - 1);
        is_hardened = true;
    }

    const auto number{ToIntegral<uint32_t>(std::string_view{elem.begin(), elem.end()})};
    if (!number) {
        return util::Unexpected{strprintf("Key path value '%s' is not a valid uint32", raw)};
    }
    if (*number >= BIP32_HARDENED_FLAG) {
        return util::Unexpected{strprintf("Key path value %u is out of range", *number)};
    }
    return KeyPathElement{*number, is_hardened};
}

std::optional<KeyPath> ParseHDKeypath(const std::string& keypath_str)
{
    std::stringstream ss(keypath_str);
    std::string item;
    bool first = true;
    KeyPath keypath;
    while (std::getline(ss, item, '/') || std::getline(ss, item, 'h')) {
        if (item.compare("m") == 0) {
            if (first) {
                first = false;
                continue;
            }
            return std::nullopt;
        }
        const auto parsed{ParseKeyPathElement(std::span<const char>{item.data(), item.size()})};
        if (!parsed) return std::nullopt;
        keypath.push_back(*parsed);
        first = false;
    }
    return keypath;
}

std::string FormatHDKeypath(const KeyPath& path, bool apostrophe)
{
    std::string ret;
    for (auto i : path) {
        ret += i.ToString(apostrophe);
    }
    return ret;
}

std::string WriteHDKeypath(const KeyPath& keypath, bool apostrophe)
{
    return "m" + FormatHDKeypath(keypath, apostrophe);
}

bool KeyPath::HasHardenedDerivation() const
{
    return std::any_of(begin(), end(), [](KeyPathElement index) {
        return index.IsHardened();
    });
}

std::string KeyPathElement::ToString(bool apostrophe) const
{
    std::string out = strprintf("/%i", m_index);
    if (m_hardened) out += apostrophe ? '\'' : 'h';
    return out;
}
