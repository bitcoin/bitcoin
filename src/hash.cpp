#include "hash.h"

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash)
{
    // The following is MurmurHash3 (x86_32), see http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    uint32_t h1 = nHashSeed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = vDataToHash.size() / 4;

    //----------
    // body
    const uint32_t * blocks = (const uint32_t *)(&vDataToHash[0] + nblocks*4);

    for(int i = -nblocks; i; i++)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1,13); 
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail
    const uint8_t * tail = (const uint8_t*)(&vDataToHash[0] + nblocks*4);

    uint32_t k1 = 0;

    switch(vDataToHash.size() & 3)
    {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
            k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

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

int HMAC_SHA512_Init(HMAC_SHA512_CTX *pctx, const void *pkey, size_t len)
{
    unsigned char key[128];
    if (len <= 128)
    {
        memcpy(key, pkey, len);
        memset(key + len, 0, 128-len);
    }
    else
    {
        SHA512_CTX ctxKey;
        SHA512_Init(&ctxKey);
        SHA512_Update(&ctxKey, pkey, len);
        SHA512_Final(key, &ctxKey);
        memset(key + 64, 0, 64);
    }

    for (int n=0; n<128; n++)
        key[n] ^= 0x5c;
    SHA512_Init(&pctx->ctxOuter);
    SHA512_Update(&pctx->ctxOuter, key, 128);

    for (int n=0; n<128; n++)
        key[n] ^= 0x5c ^ 0x36;
    SHA512_Init(&pctx->ctxInner);
    return SHA512_Update(&pctx->ctxInner, key, 128);
}

int HMAC_SHA512_Update(HMAC_SHA512_CTX *pctx, const void *pdata, size_t len)
{
    return SHA512_Update(&pctx->ctxInner, pdata, len);
}

int HMAC_SHA512_Final(unsigned char *pmd, HMAC_SHA512_CTX *pctx)
{
    unsigned char buf[64];
    SHA512_Final(buf, &pctx->ctxInner);
    SHA512_Update(&pctx->ctxOuter, buf, 64);
    return SHA512_Final(pmd, &pctx->ctxOuter);
}
