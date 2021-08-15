// Copyright (c) 2013-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <crypto/common.h>
#include <crypto/curve25519.h>
#include <crypto/hmac_sha512.h>


inline uint32_t ROTL32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash)
{
    // The following is MurmurHash3 (x86_32), see http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    uint32_t h1 = nHashSeed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = vDataToHash.size() / 4;

    //----------
    // body
    const uint8_t* blocks = vDataToHash.data();

    for (int i = 0; i < nblocks; ++i) {
        uint32_t k1 = ReadLE32(blocks + i*4);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail
    const uint8_t* tail = vDataToHash.data() + nblocks * 4;

    uint32_t k1 = 0;

    switch (vDataToHash.size() & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    }

    //----------
    // finalization
    h1 ^= vDataToHash.size();
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64])
{
    unsigned char num[4];
    num[0] = (nChild >> 24) & 0xFF;
    num[1] = (nChild >> 16) & 0xFF;
    num[2] = (nChild >>  8) & 0xFF;
    num[3] = (nChild >>  0) & 0xFF;
    CHMAC_SHA512(chainCode.begin(), chainCode.size()).Write(&header, 1).Write(data, 32).Write(num, 4).Finalize(output);
}

uint64_t PocLegacy::GeneratePlotterId(const std::string &passphrase)
{
    // 1.passphraseHash = sha256(passphrase)
    // 2.<signingKey,publicKey> = Curve25519(passphraseHash)
    // 3.publicKeyHash = sha256(publicKey)
    // 4.unsigned int64 id = unsigned int64(publicKeyHash[0~7])
    uint8_t privateKey[32] = {0}, publicKey[32] = {0};
    CSHA256().Write((const unsigned char*)passphrase.data(), (size_t)passphrase.length()).Finalize(privateKey);
    crypto::curve25519_kengen(publicKey, nullptr, privateKey);
    return ToPlotterId(publicKey);
}

uint64_t PocLegacy::ToPlotterId(const unsigned char publicKey[32])
{
    uint8_t publicKeyHash[32] = {0};
    CSHA256().Write((const unsigned char*)publicKey, 32).Finalize(publicKeyHash);
    return ((uint64_t)publicKeyHash[24]) | \
        ((uint64_t)publicKeyHash[25]) << 8 | \
        ((uint64_t)publicKeyHash[26]) << 16 | \
        ((uint64_t)publicKeyHash[27]) << 24 | \
        ((uint64_t)publicKeyHash[28]) << 32 | \
        ((uint64_t)publicKeyHash[29]) << 40 | \
        ((uint64_t)publicKeyHash[30]) << 48 | \
        ((uint64_t)publicKeyHash[31]) << 56;
}

bool PocLegacy::Sign(const std::string &passphrase, const unsigned char data[32], unsigned char signature[64], unsigned char publicKey[32])
{
    uint8_t privateKey[32] = {0}, signingKey[32] = {0};
    CSHA256().Write((const unsigned char*)passphrase.data(), (size_t)passphrase.length()).Finalize(privateKey);
    crypto::curve25519_kengen(publicKey, signingKey, privateKey);

    unsigned char x[32], Y[32], h[32], v[32];
    CSHA256().Write(data, 32).Write(signingKey, 32).Finalize(x); // digest(m + s) => x
    crypto::curve25519_kengen(Y, NULL, x); // keygen(Y, NULL, x) => Y
    CSHA256().Write(data, 32).Write(Y, 32).Finalize(h); // digest(m + Y) => h
    int r = crypto::curve25519_sign(v, h, x, signingKey); // sign(v, h, x, s)
    if (r == 1) {
        memcpy(signature, v, 32);
        memcpy(signature + 32, h, 32);
        return true;
    } else
        return false;
}

bool PocLegacy::Verify(const unsigned char publicKey[32], const unsigned char data[32], const unsigned char signature[64])
{
    unsigned char Y[32], h[32];
    crypto::curve25519_verify(Y, signature, signature + 32, publicKey); // verify25519(Y, signature, signature + 32, P) => Y
    CSHA256().Write(data, 32).Write(Y, 32).Finalize(h); // digest(m + Y) => h
    return memcmp(h, signature + 32, 32) == 0;
}
