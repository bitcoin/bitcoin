// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_OBFUSCATION_H
#define BITCOIN_UTIL_OBFUSCATION_H

#include <cstdint>
#include <span.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <array>
#include <bit>
#include <climits>
#include <ios>
#include <memory>

class Obfuscation
{
public:
    using KeyType = uint64_t;
    static constexpr size_t KEY_SIZE{sizeof(KeyType)};

    Obfuscation() { SetRotations(0); }
    explicit Obfuscation(std::span<const std::byte, KEY_SIZE> key_bytes)
    {
        SetRotations(ToKey(key_bytes));
    }

    operator bool() const { return m_rotations[0] != 0; }

    void operator()(std::span<std::byte> target, size_t key_offset = 0) const
    {
        if (!*this) return;

        KeyType rot_key{m_rotations[key_offset % KEY_SIZE]}; // Continue obfuscation from where we left off
        if (target.size() > KEY_SIZE) {
            // Obfuscate until KEY_SIZE alignment boundary
            if (const auto misalign{reinterpret_cast<uintptr_t>(target.data()) % KEY_SIZE}) {
                const size_t alignment{KEY_SIZE - misalign};
                XorWord(target.first(alignment), rot_key);

                target = {std::assume_aligned<KEY_SIZE>(target.data() + alignment), target.size() - alignment};
                rot_key = m_rotations[(key_offset + alignment) % KEY_SIZE];
            }
            // Aligned obfuscation in 8*KEY_SIZE chunks
            for (constexpr auto unroll{8}; target.size() >= KEY_SIZE * unroll; target = target.subspan(KEY_SIZE * unroll)) {
                for (size_t i{0}; i < unroll; ++i) {
                    XorWord(target.subspan(i * KEY_SIZE, KEY_SIZE), rot_key);
                }
            }
            // Aligned obfuscation in KEY_SIZE chunks
            for (; target.size() >= KEY_SIZE; target = target.subspan(KEY_SIZE)) {
                XorWord(target.first<KEY_SIZE>(), rot_key);
            }
        }
        XorWord(target, rot_key);
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        // Use vector serialization for convenient compact size prefix.
        std::vector<std::byte> bytes{KEY_SIZE};
        std::memcpy(bytes.data(), &m_rotations[0], KEY_SIZE);
        s << bytes;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        std::vector<std::byte> bytes{KEY_SIZE};
        s >> bytes;
        if (bytes.size() != KEY_SIZE) throw std::ios_base::failure(strprintf("Obfuscation key size should be exactly %s bytes long", KEY_SIZE));
        SetRotations(ToKey(std::span<std::byte, KEY_SIZE>(bytes)));
    }

    std::string HexKey() const
    {
        return HexStr(std::as_bytes(std::span{&m_rotations[0], 1}));
    }

private:
    // Cached key rotations for different offsets.
    std::array<KeyType, KEY_SIZE> m_rotations;

    void SetRotations(KeyType key)
    {
        for (size_t i{0}; i < KEY_SIZE; ++i) {
            int key_rotation_bits{int(CHAR_BIT * i)};
            if constexpr (std::endian::native == std::endian::big) key_rotation_bits *= -1;
            m_rotations[i] = std::rotr(key, key_rotation_bits);
        }
    }

    static KeyType ToKey(std::span<const std::byte, KEY_SIZE> key_span)
    {
        KeyType key{};
        std::memcpy(&key, key_span.data(), KEY_SIZE);
        return key;
    }

    static void XorWord(std::span<std::byte> target, KeyType key)
    {
        assert(target.size() <= KEY_SIZE);
        if (target.empty()) return;
        KeyType raw{};
        std::memcpy(&raw, target.data(), target.size());
        raw ^= key;
        std::memcpy(target.data(), &raw, target.size());
    }
};

#endif // BITCOIN_UTIL_OBFUSCATION_H
