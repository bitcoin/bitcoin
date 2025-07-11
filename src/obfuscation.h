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
#include <memory>
#include <stdexcept>

class Obfuscation
{
public:
    static constexpr size_t KEY_SIZE{sizeof(uint64_t)};

    Obfuscation() { SetRotations(0); }
    explicit Obfuscation(std::span<const std::byte, KEY_SIZE> key_bytes)
    {
        SetRotations(ToUint64(key_bytes));
    }

    uint64_t Key() const { return m_rotations[0]; }
    operator bool() const { return Key() != 0; }

    void operator()(std::span<std::byte> target, size_t key_offset = 0) const
    {
        if (!*this) return;

        uint64_t rot_key{m_rotations[key_offset % KEY_SIZE]}; // Continue obfuscation from where we left off
        if (target.size() > KEY_SIZE) {
            // Obfuscate until 64-bit alignment boundary
            if (const auto misalign{std::bit_cast<uintptr_t>(target.data()) % KEY_SIZE}) {
                const size_t alignment{std::min(KEY_SIZE - misalign, target.size())};
                Xor(target, rot_key, alignment);

                target = {std::assume_aligned<KEY_SIZE>(target.data() + alignment), target.size() - alignment};
                rot_key = m_rotations[(key_offset + alignment) % KEY_SIZE];
            }
            // Aligned obfuscation in 64-byte chunks
            for (constexpr auto unroll{8}; target.size() >= KEY_SIZE * unroll; target = target.subspan(KEY_SIZE * unroll)) {
                for (size_t i{0}; i < unroll; ++i) {
                    Xor(target.subspan(i * KEY_SIZE, KEY_SIZE), rot_key, KEY_SIZE);
                }
            }
            // Aligned obfuscation in 64-bit chunks
            for (; target.size() >= KEY_SIZE; target = target.subspan(KEY_SIZE)) {
                Xor(target, rot_key, KEY_SIZE);
            }
        }
        Xor(target, rot_key, target.size());
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
        if (bytes.size() != KEY_SIZE) throw std::logic_error(strprintf("Obfuscation key size should be exactly %s bytes long", KEY_SIZE));
        SetRotations(ToUint64(std::span<std::byte, KEY_SIZE>(bytes)));
    }

private:
    // Cached key rotations for different offsets.
    std::array<uint64_t, KEY_SIZE> m_rotations;

    void SetRotations(const uint64_t key)
    {
        for (size_t i{0}; i < KEY_SIZE; ++i) {
            size_t key_rotation_bits{CHAR_BIT * i};
            if constexpr (std::endian::native == std::endian::big) key_rotation_bits *= -1;
            m_rotations[i] = std::rotr(key, key_rotation_bits);
        }
    }

    static uint64_t ToUint64(const std::span<const std::byte, KEY_SIZE> key_span)
    {
        uint64_t key{};
        std::memcpy(&key, key_span.data(), KEY_SIZE);
        return key;
    }

    static void Xor(std::span<std::byte> target, const uint64_t key, const size_t size)
    {
        if (target.empty() || !size) return;
        assert(size <= target.size());
        uint64_t raw{};
        std::memcpy(&raw, target.data(), size);
        raw ^= key;
        std::memcpy(target.data(), &raw, size);
    }
};

#endif // BITCOIN_OBFUSCATION_H
