// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OBFUSCATION_H
#define BITCOIN_OBFUSCATION_H

#include <span.h>
#include <tinyformat.h>

#include <array>
#include <bit>
#include <climits>
#include <stdexcept>

class Obfuscation
{
public:
    static constexpr size_t SIZE_BYTES{sizeof(uint64_t)};

    Obfuscation(const uint64_t key) { SetRotations(key); }
    Obfuscation(const std::span<const std::byte, SIZE_BYTES> key_span) : Obfuscation(ToUint64(key_span)) {}

    uint64_t Key() const { return m_rotations[0]; }
    operator bool() const { return Key() != 0; }
    void operator()(std::span<std::byte> target, const size_t key_offset_bytes = 0) const
    {
        if (!*this) return;
        const uint64_t rot_key{m_rotations[key_offset_bytes % SIZE_BYTES]}; // Continue obfuscation from where we left off
        for (; target.size() >= SIZE_BYTES; target = target.subspan(SIZE_BYTES)) { // Process multiple bytes at a time
            Xor(target, rot_key, SIZE_BYTES);
        }
        Xor(target, rot_key, target.size());
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        // Use vector serialization for convenient compact size prefix.
        std::vector<std::byte> bytes{SIZE_BYTES};
        std::memcpy(bytes.data(), &m_rotations[0], SIZE_BYTES);
        s << bytes;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        std::vector<std::byte> bytes{SIZE_BYTES};
        s >> bytes;
        if (bytes.size() != SIZE_BYTES) throw std::logic_error(strprintf("Obfuscation key size should be exactly %s bytes long", SIZE_BYTES));
        SetRotations(ToUint64(std::span<std::byte, SIZE_BYTES>(bytes)));
    }

private:
    // Cached key rotations for different offsets.
    std::array<uint64_t, SIZE_BYTES> m_rotations;

    void SetRotations(const uint64_t key)
    {
        for (size_t i{0}; i < SIZE_BYTES; ++i) {
            size_t key_rotation_bits{CHAR_BIT * i};
            if constexpr (std::endian::native == std::endian::big) key_rotation_bits *= -1;
            m_rotations[i] = std::rotr(key, key_rotation_bits);
        }
    }

    static uint64_t ToUint64(const std::span<const std::byte, SIZE_BYTES> key_span)
    {
        uint64_t key{};
        std::memcpy(&key, key_span.data(), SIZE_BYTES);
        return key;
    }

    static void Xor(std::span<std::byte> target, const uint64_t key, const size_t size)
    {
        assert(size <= target.size());
        uint64_t raw{};
        std::memcpy(&raw, target.data(), size);
        raw ^= key;
        std::memcpy(target.data(), &raw, size);
    }
};

#endif // BITCOIN_OBFUSCATION_H
