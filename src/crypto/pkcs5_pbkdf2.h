// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_PKCS5_PBKDF2_H
#define BITCOIN_CRYPTO_PKCS5_PBKDF2_H

#include <cstdint>
#include <cstdio>
#include <vector>

std::vector<uint8_t> pkcs5_pbkdf2_hmacsha512(const std::vector<uint8_t>, const std::vector<uint8_t>, const int);

#endif // BITCOIN_CRYPTO_PKCS5_PBKDF2_H
