#include "hash.h"

inline unsigned int ROTL32 ( unsigned int x, char r )
{
    return (x << r) | (x >> (32 - r));
}

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash)
{
    // The following is MurmurHash3 (x86_32), see http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    unsigned int h1 = nHashSeed;
    const unsigned int c1 = 0xcc9e2d51;
    const unsigned int c2 = 0x1b873593;

    const int nblocks = vDataToHash.size() / 4;

    //----------
    // body
    const unsigned int * blocks = (const unsigned int *)(&vDataToHash[0] + nblocks*4);

    for(int i = -nblocks; i; i++)
    {
        unsigned int k1 = blocks[i];

        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1,13); 
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail
    const unsigned char * tail = (const unsigned char*)(&vDataToHash[0] + nblocks*4);

    unsigned int k1 = 0;

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
