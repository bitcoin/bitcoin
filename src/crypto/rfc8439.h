// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_RFC8439_H
#define BITCOIN_CRYPTO_RFC8439_H

#include <crypto/poly1305.h>
#include <span.h>

#include <array>
#include <cstddef>
#include <vector>

constexpr static size_t RFC8439_KEYLEN = 32;
constexpr static size_t RFC8439_EXPANSION = POLY1305_TAGLEN;

void RFC8439Encrypt(const Span<const std::byte> aad, const Span<const std::byte> key, const std::array<std::byte, 12>& nonce, const Span<const std::byte> plaintext, Span<std::byte> output);

// returns false if authentication fails
bool RFC8439Decrypt(const Span<const std::byte> aad, const Span<const std::byte> key, const std::array<std::byte, 12>& nonce, const Span<const std::byte> input, Span<std::byte> plaintext);
#endif // BITCOIN_CRYPTO_RFC8439_H
