// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <util/expected.h>
#include <vector>

/** BIP32 unhardened derivation index (no high bit set) */
static constexpr uint32_t BIP32_UNHARDENED_FLAG = 0x0;
/** BIP32 hardened derivation flag (2^31) */
static constexpr uint32_t BIP32_HARDENED_FLAG = 0x80000000;

class SingleKeyPathElement {
private:
    /** Derivation index, without the hardened flag */
    uint32_t m_index;
    std::optional<char> m_hardened;

public:
    SingleKeyPathElement() = default;
    SingleKeyPathElement(uint32_t index, std::optional<char> hardened) : m_index(index), m_hardened(hardened) {}
    SingleKeyPathElement(uint32_t num) : m_index(num & ~BIP32_HARDENED_FLAG), m_hardened(bool(num & BIP32_HARDENED_FLAG) ? std::optional{'h'} : std::nullopt) {}

    /** Derivation index with the hardened flag applied */
    uint32_t ChildNumber() const { return m_index | (m_hardened.has_value() ? BIP32_HARDENED_FLAG : BIP32_UNHARDENED_FLAG); }

    bool IsHardened() const { return m_hardened.has_value(); }
    void SetHardenedChar(char hardened)
    {
        if (m_hardened) m_hardened = hardened;
    }

    std::string ToString(const std::optional<char>& hardened_char = std::nullopt) const;

    bool operator<(const SingleKeyPathElement& other) const { return ChildNumber() < other.ChildNumber(); }
    bool operator==(const SingleKeyPathElement& other) const { return ChildNumber() == other.ChildNumber(); }

    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        s << ChildNumber();
    }

    template <typename Stream>
    inline void Unserialize(Stream& s)
    {
        uint32_t num;
        s >> num;
        m_index = num & ~BIP32_HARDENED_FLAG;
        m_hardened = bool(num & BIP32_HARDENED_FLAG);
    }
};

class KeyPathElement {
private:
    std::vector<SingleKeyPathElement> m_indexes;

public:
    KeyPathElement() = default;
    KeyPathElement(uint32_t index, std::optional<char> hardened) : m_indexes({{index, hardened}}) {}
    KeyPathElement(uint32_t num) : m_indexes({{num}}) {}
    KeyPathElement(SingleKeyPathElement index) : m_indexes({index}) {}
    KeyPathElement(std::vector<SingleKeyPathElement> indexes) : m_indexes(indexes) {}

    const std::vector<SingleKeyPathElement>& Indexes() const { return m_indexes; }
    size_t MultipathLen() const { return m_indexes.size(); }
    bool IsMultipath() const { return m_indexes.size() > 1; }
    const SingleKeyPathElement& Index(size_t multipath_pos) const { return m_indexes.at(multipath_pos); }

    /** Derivation index with the hardened flag applied */
    uint32_t ChildNumber(size_t multipath_pos = 0) const { return m_indexes.at(multipath_pos).ChildNumber(); }

    bool IsHardened(size_t multipath_pos = 0) const { return m_indexes.at(multipath_pos).IsHardened(); }
    bool HasHardened() const { return std::any_of(m_indexes.begin(), m_indexes.end(), [](const SingleKeyPathElement& elem) { return elem.IsHardened(); }); }
    void SetHardenedChar(char hardened)
    {
        for (SingleKeyPathElement& i : m_indexes) i.SetHardenedChar(hardened);
    }

    std::string ToString(const std::optional<char>& hardened_char = std::nullopt) const;

    bool operator<(const KeyPathElement& other) const { return m_indexes < other.m_indexes; }
    bool operator==(const KeyPathElement& other) const { return m_indexes == other.m_indexes; }

    /** Serialize and Unserialize are for backwards compatibility and only serialize the first index.
     *  There is no serialization of multipath indexes
     */
    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        s << m_indexes.at(0);
    }

    template <typename Stream>
    inline void Unserialize(Stream& s)
    {
        m_indexes.clear();
        m_indexes.emplace_back();
        s >> m_indexes.back();
    }
};


class KeyPath : public std::vector<KeyPathElement>
{
    using vector::vector;

public:
    /** Whether a parsed HD keypath contains at least one hardened derivation step. */
    bool HasHardenedDerivation() const;

    void SetHardenedChar(char hardened);
};

/** Parse a single key path element like "0", "0'", or "0h".
 *  Returns the derivation index and hardened status, or an error message. */
util::Expected<SingleKeyPathElement, std::string> ParseSingleKeyPathElement(std::span<const char> elem);
util::Expected<KeyPathElement, std::string> ParseKeyPathElement(std::span<const char> elem);

/** Parse an HD keypaths like "m/7/0'/2000". */
std::optional<KeyPath> ParseHDKeypath(const std::string& keypath_str, bool allow_multipath = false);
util::Expected<KeyPath, std::string> ParseHDKeypath(const std::vector<std::span<const char>>& split_keypath, bool allow_multipath);

/** Write HD keypaths as strings */
std::string WriteHDKeypath(const KeyPath& keypath, const std::optional<char>& hardened_char = 'h');
std::string FormatHDKeypath(const KeyPath& path, const std::optional<char>& hardened_char = 'h');

#endif // BITCOIN_UTIL_BIP32_H
