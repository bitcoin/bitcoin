// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_HKDF_SHA256_32_H
#define BITCOIN_CRYPTO_HKDF_SHA256_32_H

#include <cstddef>
#include <string>

/** A rfc5869 HKDF implementation with HMAC_SHA256 and fixed key output length of 32 bytes (L=32) */
class CHKDF_HMAC_SHA256_L32
{
private:
    unsigned char m_prk[32];
    static const size_t OUTPUT_SIZE = 32;

public:
    CHKDF_HMAC_SHA256_L32(const unsigned char* ikm, size_t ikmlen, const std::string& salt);
    void Expand32(const std::string& info, unsigned char hash[OUTPUT_SIZE]);
};

#endif // BITCOIN_CRYPTO_HKDF_SHA256_32_H
