// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OBFUSCATION_H
#define BITCOIN_OBFUSCATION_H

#include <array>
#include <cassert>
#include <cstdint>
#include <random>
#include <span.h>
#include <util/check.h>
#include <cstring>
#include <climits>
#include <serialize.h>

class Obfuscation
{
public:
    static constexpr size_t SIZE_BYTES{sizeof(uint64_t)};

private:
    std::array<uint64_t, SIZE_BYTES> rotations; // Cached key rotations
    void SetRotations(const uint64_t key)
    {
        for (size_t i{0}; i < SIZE_BYTES; ++i) {
            size_t key_rotation_bits{CHAR_BIT * i};
            if constexpr (std::endian::native == std::endian::big) key_rotation_bits *= -1;
            rotations[i] = std::rotr(key, key_rotation_bits);
        }
    }

    static uint64_t ToUint64(const std::span<const std::byte, SIZE_BYTES> key_span)
    {
        uint64_t key{};
        std::memcpy(&key, key_span.data(), SIZE_BYTES);
        return key;
    }

    static void Xor(std::span<std::byte> write, const uint64_t key, const size_t size)
    {
        assert(size <= write.size());
        uint64_t raw{};
        std::memcpy(&raw, write.data(), size);
        raw ^= key;
        std::memcpy(write.data(), &raw, size);
    }

public:
    Obfuscation(const uint64_t key) { SetRotations(key); }
    Obfuscation(const std::span<const std::byte, SIZE_BYTES> key_span) : Obfuscation(ToUint64(key_span)) {}
    Obfuscation(const std::vector<uint8_t>& key_vec) : Obfuscation(MakeByteSpan(key_vec).first<SIZE_BYTES>()) {}
    Obfuscation(const std::vector<std::byte>& key_vec) : Obfuscation(std::span(key_vec).first<SIZE_BYTES>()) {}

    uint64_t Key() const { return rotations[0]; }
    operator bool() const { return Key() != 0; }
    void operator()(std::span<std::byte> write, const size_t key_offset_bytes = 0) const
    {
        if (!*this) return;
        const uint64_t rot_key{rotations[key_offset_bytes % SIZE_BYTES]}; // Continue obfuscation from where we left off
        for (; write.size() >= SIZE_BYTES; write = write.subspan(SIZE_BYTES)) { // Process multiple bytes at a time
            Xor(write, rot_key, SIZE_BYTES);
        }
        Xor(write, rot_key, write.size());
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        std::vector<std::byte> bytes(SIZE_BYTES);
        std::memcpy(bytes.data(), &rotations[0], SIZE_BYTES);
        s << bytes;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        std::vector<std::byte> bytes(SIZE_BYTES);
        s >> bytes;
        SetRotations(ToUint64(MakeByteSpan(bytes).first<SIZE_BYTES>()));
    }
};

#endif // BITCOIN_OBFUSCATION_H
