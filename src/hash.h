// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_HASH_H
#define BITCOIN_HASH_H

#include "uint256.h"
#include "serialize.h"

#include <openssl/sha.h>
#include <openssl/ripemd.h>

template<typename T1>
inline uint256 Hash(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256((pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]), (unsigned char*)&hash1);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

class CHashWriter
{
private:
    SHA256_CTX ctx;

public:
    int nType;
    int nVersion;

    void Init()
    {
        SHA256_Init(&ctx);
    }

    CHashWriter(int nTypeIn, int nVersionIn) : nType(nTypeIn), nVersion(nVersionIn)
    {
        Init();
    }

    CHashWriter& write(const char *pch, size_t size)
    {
        SHA256_Update(&ctx, pch, size);
        return (*this);
    }

    // invalidates the object
    uint256 GetHash()
    {
        uint256 hash1;
        SHA256_Final((unsigned char*)&hash1, &ctx);
        uint256 hash2;
        SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
        return hash2;
    }

    template<typename T>
    CHashWriter& operator<<(const T& obj) {
        // Serialize to this stream
        ::Serialize(*this, obj, nType, nVersion);
        return (*this);
    }
};


template<typename T1, typename T2>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Final((unsigned char*)&hash1, &ctx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

template<typename T1, typename T2, typename T3>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end,
                    const T3 p3begin, const T3 p3end)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Update(&ctx, (p3begin == p3end ? pblank : (unsigned char*)&p3begin[0]), (p3end - p3begin) * sizeof(p3begin[0]));
    SHA256_Final((unsigned char*)&hash1, &ctx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

template<typename T>
uint256 SerializeHash(const T& obj, int nType=SER_GETHASH, int nVersion=PROTOCOL_VERSION)
{
    CHashWriter ss(nType, nVersion);
    ss << obj;
    return ss.GetHash();
}

template<typename T1>
inline uint160 Hash160(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256((pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]), (unsigned char*)&hash1);
    uint160 hash2;
    RIPEMD160((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

inline uint160 Hash160(const std::vector<unsigned char>& vch)
{
    return Hash160(vch.begin(), vch.end());
}

inline uint32_t ROTL32 ( uint32_t x, int8_t r )
{
    return (x << r) | (x >> (32 - r));
}

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash);


typedef struct
{
    SHA512_CTX ctxInner;
    SHA512_CTX ctxOuter;
} HMAC_SHA512_CTX;

int HMAC_SHA512_Init(HMAC_SHA512_CTX *pctx, const void *pkey, size_t len);
int HMAC_SHA512_Update(HMAC_SHA512_CTX *pctx, const void *pdata, size_t len);
int HMAC_SHA512_Final(unsigned char *pmd, HMAC_SHA512_CTX *pctx);

#endif
