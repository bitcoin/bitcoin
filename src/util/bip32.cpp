// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bip32.h>

#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>
#include <unordered_set>

util::Expected<SingleKeyPathElement, std::string> ParseSingleKeyPathElement(std::span<const char> elem)
{
    const std::string_view raw{elem.begin(), elem.end()};
    if (elem.empty()) {
        return util::Unexpected{strprintf("Key path value '%s' is not valid", raw)};
    }

    std::optional<char> last = elem.back();
    if (last == '\'' || last == 'h') {
        elem = elem.first(elem.size() - 1);
    } else {
        last = std::nullopt;
    }

    const auto number{ToIntegral<uint32_t>(std::string_view{elem.begin(), elem.end()})};
    if (!number) {
        return util::Unexpected{strprintf("Key path value '%s' is not a valid uint32", raw)};
    }
    if (*number >= BIP32_HARDENED_FLAG) {
        return util::Unexpected{strprintf("Key path value %u is out of range", *number)};
    }
    return SingleKeyPathElement{*number, last};
}

util::Expected<KeyPathElement, std::string> ParseKeyPathElement(std::span<const char> elem)
{
    const std::string_view raw{elem.begin(), elem.end()};
    if (elem.empty()) {
        return util::Unexpected{strprintf("Key path value '%s' is not valid", raw)};
    }

    if (elem.front() == '<' && elem.back() == '>') {

        // Parse each possible value
        std::vector<std::span<const char>> nums = util::Split(std::span(elem.begin()+1, elem.end()-1), ";");
        if (nums.size() < 2) {
            return util::Unexpected{"Multipath key path specifiers must have at least two items"};
        }

        std::unordered_set<uint32_t> seen;
        std::vector<SingleKeyPathElement> indexes;
        for (const auto& num : nums) {
            const auto& op_num = ParseSingleKeyPathElement(num);
            if (!op_num) return util::Unexpected{op_num.error()};
            auto [_, inserted] = seen.insert(op_num->ChildNumber());
            if (!inserted) {
                return util::Unexpected(strprintf("Duplicated key path value %u in multipath specifier", op_num->ChildNumber()));
            }
            indexes.push_back(*op_num);
        }
        return KeyPathElement{indexes};
    }

    const auto& op_num = ParseSingleKeyPathElement(elem);
    if (!op_num) return util::Unexpected{op_num.error()};
    return KeyPathElement{*op_num};
}


std::optional<KeyPath> ParseHDKeypath(const std::string& keypath_str, bool allow_multipath)
{
    if (keypath_str == "m") return KeyPath{};

    std::span<const char> sp = keypath_str;
    if (keypath_str.starts_with("m/")) {
        sp = sp.subspan(2);
    }
    if (keypath_str.ends_with("/")) {
        sp = sp.subspan(0, sp.size() - 1);
    }
    if (sp.empty()) return KeyPath{};

    const auto split = util::Split(sp, "/");

    util::Expected<KeyPath, std::string> parsed = ParseHDKeypath(split, allow_multipath);
    if (!parsed) return std::nullopt;
    return *parsed;
}

util::Expected<KeyPath, std::string> ParseHDKeypath(const std::vector<std::span<const char>>& split_keypath, bool allow_multipath)
{
    KeyPath keypath;
    bool seen_multipath = false;
    for (size_t i = 0; i < split_keypath.size(); ++i) {
        const std::span<const char>& item = split_keypath[i];

        const auto parsed{ParseKeyPathElement(std::span<const char>{item.data(), item.size()})};
        if (!parsed) return util::Unexpected{parsed.error()};
        if (parsed->IsMultipath()) {
            if (!allow_multipath) {
                return util::Unexpected{strprintf("Key path value '%s' specifies multipath in a section where multipath is not allowed", std::string(item.begin(), item.end()))};
            } else if (seen_multipath) {
                return util::Unexpected{strprintf("Multiple multipath key path specifiers found")};
            }
            seen_multipath = true;
        }

        keypath.push_back(*parsed);
    }
    return keypath;
}

std::string FormatHDKeypath(const KeyPath& path, const std::optional<char>& hardened_char)
{
    std::string ret;
    for (auto i : path) {
        ret += i.ToString(hardened_char);
    }
    return ret;
}

std::string WriteHDKeypath(const KeyPath& keypath, const std::optional<char>& hardened_char)
{
    return "m" + FormatHDKeypath(keypath, hardened_char);
}

bool KeyPath::HasHardenedDerivation() const
{
    return std::any_of(begin(), end(), [](KeyPathElement index) {
        return index.IsHardened();
    });
}

void KeyPath::SetHardenedChar(char hardened)
{
    for (KeyPathElement& elem : *this) {
        elem.SetHardenedChar(hardened);
    }
}

size_t KeyPath::MultipathLen() const
{
    size_t len = 0;
    for (const auto& e : *this) {
        len = std::max(len, e.MultipathLen());
    }
    return len;
}

KeyPath KeyPath::ChooseMultipath(size_t pos) const
{
    KeyPath new_path;
    new_path.reserve(size());
    std::transform(begin(), end(), std::back_inserter(new_path),
                   [&pos](const KeyPathElement& e) {
                       if (e.IsMultipath()) return e.ChildNumber(pos);
                       return e.ChildNumber();
                   });
    return new_path;
}

std::string SingleKeyPathElement::ToString(const std::optional<char>& hardened_char) const
{
    std::string out = strprintf("%i", m_index);
    if (m_hardened) {
        if (hardened_char) {
            out += hardened_char.value();
        } else {
            out += m_hardened.value();
        }
    }
    return out;
}

std::string KeyPathElement::ToString(const std::optional<char>& hardened_char) const
{
    if (!IsMultipath()) {
        return "/" + m_indexes.at(0).ToString(hardened_char);
    }

    std::string out = "/<";
    size_t pos = 0;
    for (const auto& i : m_indexes) {
        if (pos++) out += ';';
        out += i.ToString(hardened_char);
    }
    out += '>';
    return out;
}
