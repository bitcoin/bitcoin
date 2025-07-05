// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OBFUSCATION_H
#define BITCOIN_OBFUSCATION_H

#include <span.h>
#include <tinyformat.h>

#include <stdexcept>

class Obfuscation
{
public:
    static constexpr size_t KEY_SIZE{sizeof(uint64_t)};

    Obfuscation() : m_key{KEY_SIZE, std::byte{0}} {}
    explicit Obfuscation(std::span<const std::byte, KEY_SIZE> key_bytes)
    {
        m_key = {key_bytes.begin(), key_bytes.end()};
    }

    uint64_t Key() const { return ToUint64(); }
    operator bool() const { return Key() != 0; }

    void operator()(std::span<std::byte> write, size_t key_offset = 0) const
    {
        assert(m_key.size() == KEY_SIZE);
        key_offset %= KEY_SIZE;

        for (size_t i = 0, j = key_offset; i != write.size(); i++) {
            write[i] ^= m_key[j++];

            // This potentially acts on very many bytes of data, so it's
            // important that we calculate `j`, i.e. the `key` index in this
            // way instead of doing a %, which would effectively be a division
            // for each byte Xor'd -- much slower than need be.
            if (j == KEY_SIZE)
                j = 0;
        }
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_key;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> m_key;
        if (m_key.size() != KEY_SIZE) throw std::logic_error(strprintf("Obfuscation key size should be exactly %s bytes long", KEY_SIZE));
    }

private:
    std::vector<std::byte> m_key;

    uint64_t ToUint64() const
    {
        uint64_t key{};
        std::memcpy(&key, m_key.data(), KEY_SIZE);
        return key;
    }
};

#endif // BITCOIN_OBFUSCATION_H
