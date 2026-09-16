// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

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

class KeyPathElement {
private:
    /** Derivation index, without the hardened flag */
    uint32_t m_index;
    bool m_hardened;

public:
    KeyPathElement() = default;
    KeyPathElement(uint32_t index, bool hardened) : m_index(index), m_hardened(hardened) {}
    KeyPathElement(uint32_t num) : m_index(num & ~BIP32_HARDENED_FLAG), m_hardened(num & BIP32_HARDENED_FLAG) {}

    /** Derivation index with the hardened flag applied */
    uint32_t ChildNumber() const { return m_index | (m_hardened ? BIP32_HARDENED_FLAG : BIP32_UNHARDENED_FLAG); }

    bool IsHardened() const { return m_hardened; }

    std::string ToString(bool apostrophe = false) const;

    bool operator<(const KeyPathElement& other) const { return ChildNumber() < other.ChildNumber(); }
    bool operator==(const KeyPathElement& other) const { return ChildNumber() == other.ChildNumber(); }

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

using KeyPath = std::vector<KeyPathElement>;

/** Parse a single key path element like "0", "0'", or "0h".
 *  Returns the derivation index and hardened status, or an error message. */
util::Expected<KeyPathElement, std::string> ParseKeyPathElement(std::span<const char> elem);

/** Parse an HD keypaths like "m/7/0'/2000". */
std::optional<KeyPath> ParseHDKeypath(const std::string& keypath_str);

/** Write HD keypaths as strings */
std::string WriteHDKeypath(const KeyPath& keypath, bool apostrophe = false);
std::string FormatHDKeypath(const KeyPath& path, bool apostrophe = false);

/** Whether a parsed HD keypath contains at least one hardened derivation step. */
bool HasHardenedDerivation(const KeyPath& keypath);

#endif // BITCOIN_UTIL_BIP32_H
