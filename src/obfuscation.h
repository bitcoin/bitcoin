// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OBFUSCATION_H
#define BITCOIN_OBFUSCATION_H

#include <span.h>

class Obfuscation
{
public:
    static constexpr size_t SIZE_BYTES{sizeof(uint64_t)};

    void Xor(std::span<std::byte> write, std::span<const std::byte> key, size_t key_offset = 0)
    {
        assert(key.size() == SIZE_BYTES);
        key_offset %= SIZE_BYTES;

        for (size_t i = 0, j = key_offset; i != write.size(); i++) {
            write[i] ^= key[j++];

            // This potentially acts on very many bytes of data, so it's
            // important that we calculate `j`, i.e. the `key` index in this
            // way instead of doing a %, which would effectively be a division
            // for each byte Xor'd -- much slower than need be.
            if (j == SIZE_BYTES)
                j = 0;
        }
    }
};

#endif // BITCOIN_OBFUSCATION_H
