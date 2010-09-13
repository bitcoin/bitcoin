// Copyright (c) 2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

// tcatm's 4-way 128-bit SSE2 SHA-256

#ifdef FOURWAYSSE2

#include <string.h>
#include <assert.h>

#include <xmmintrin.h>
#include <stdint.h>
#include <stdio.h>

#define NPAR 32

extern void DoubleBlockSHA256(const void* pin, void* pout, const void* pinit, unsigned int hash[8][NPAR], const void* init2);

static const unsigned int sha256_consts[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, /*  0 */
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, /*  8 */
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, /* 16 */
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, /* 24 */
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, /* 32 */
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, /* 40 */
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, /* 48 */
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, /* 56 */
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


static inline __m128i Ch(const __m128i b, const __m128i c, const __m128i d) {
    return (b & c) ^ (~b & d);
}

static inline __m128i Maj(const __m128i b, const __m128i c, const __m128i d) {
    return (b & c) ^ (b & d) ^ (c & d);
}

static inline __m128i ROTR(__m128i x, const int n) {
    return _mm_srli_epi32(x, n) | _mm_slli_epi32(x, 32 - n);
}

static inline __m128i SHR(__m128i x, const int n) {
    return _mm_srli_epi32(x, n);
}

/* SHA256 Functions */
#define BIGSIGMA0_256(x)    (ROTR((x), 2) ^ ROTR((x), 13) ^ ROTR((x), 22))
#define BIGSIGMA1_256(x)    (ROTR((x), 6) ^ ROTR((x), 11) ^ ROTR((x), 25))
#define SIGMA0_256(x)       (ROTR((x), 7) ^ ROTR((x), 18) ^ SHR((x), 3))
#define SIGMA1_256(x)       (ROTR((x), 17) ^ ROTR((x), 19) ^ SHR((x), 10))

static inline unsigned int store32(const __m128i x, int i) {
    union { unsigned int ret[4]; __m128i x; } box;
    box.x = x;
    return box.ret[i];
}

static inline void store_epi32(const __m128i x, unsigned int *x0, unsigned int *x1, unsigned int *x2, unsigned int *x3) {
    union { unsigned int ret[4]; __m128i x; } box;
    box.x = x;
    *x0 = box.ret[3]; *x1 = box.ret[2]; *x2 = box.ret[1]; *x3 = box.ret[0];
}

#define add4(x0, x1, x2, x3) _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(x0, x1), x2), x3)
#define add5(x0, x1, x2, x3, x4) _mm_add_epi32(add4(x0, x1, x2, x3), x4)

#define SHA256ROUND(a, b, c, d, e, f, g, h, i, w)                       \
    T1 = add5(h, BIGSIGMA1_256(e), Ch(e, f, g), _mm_set1_epi32(sha256_consts[i]), w);   \
d = _mm_add_epi32(d, T1);                                           \
h = _mm_add_epi32(T1, _mm_add_epi32(BIGSIGMA0_256(a), Maj(a, b, c)));

static inline void dumpreg(__m128i x, char *msg) {
    union { unsigned int ret[4]; __m128i x; } box;
    box.x = x ;
    printf("%s %08x %08x %08x %08x\n", msg, box.ret[0], box.ret[1], box.ret[2], box.ret[3]);
}

#if 1
#define dumpstate(i) printf("%s: %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", \
        __func__, store32(w0, i), store32(a, i), store32(b, i), store32(c, i), store32(d, i), store32(e, i), store32(f, i), store32(g, i), store32(h, i));
#else
#define dumpstate()
#endif

// Align by increasing pointer, must have extra space at end of buffer
template <size_t nBytes, typename T>
T* alignup(T* p)
{
    union
    {
        T* ptr;
        size_t n;
    } u;
    u.ptr = p;
    u.n = (u.n + (nBytes-1)) & ~(nBytes-1);
    return u.ptr;
}

static const unsigned int pSHA256InitState[8] =
{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};


unsigned int ScanHash_4WaySSE2(char* pmidstate, char* pblock, char* phash1, char* phash, unsigned int& nHashesDone)
{
    unsigned int& nNonce = *(unsigned int*)(pblock + 12);
    for (;;)
    {
        nNonce += NPAR;
        unsigned int thashbuf[9][NPAR];
        unsigned int (&thash)[9][NPAR] = *alignup<16>(&thashbuf);
        DoubleBlockSHA256(pblock, phash1, pmidstate, thash, pSHA256InitState);

        for (int j = 0; j < NPAR; j++)
        {
            if (thash[7][j] == 0)
            {
                for (int i = 0; i < 32/4; i++)
                    ((unsigned int*)phash)[i] = thash[i][j];
                return nNonce + j;
            }
        }

        if ((nNonce & 0xffff) == 0)
        {
            nHashesDone = 0xffff+1;
            return -1;
        }
    }
}


void DoubleBlockSHA256(const void* pin, void* pad, const void *pre, unsigned int thash[9][NPAR], const void *init)
{
    unsigned int* In = (unsigned int*)pin;
    unsigned int* Pad = (unsigned int*)pad;
    unsigned int* hPre = (unsigned int*)pre;
    unsigned int* hInit = (unsigned int*)init;
    unsigned int i, j, k;

    /* vectors used in calculation */
    __m128i w0, w1, w2, w3, w4, w5, w6, w7;
    __m128i w8, w9, w10, w11, w12, w13, w14, w15;
    __m128i T1;
    __m128i a, b, c, d, e, f, g, h;
    __m128i nonce;

    /* nonce offset for vector */
    __m128i offset = _mm_set_epi32(0x00000003, 0x00000002, 0x00000001, 0x00000000);


    for(k = 0; k<NPAR; k+=4) {
        w0 = _mm_set1_epi32(In[0]);
        w1 = _mm_set1_epi32(In[1]);
        w2 = _mm_set1_epi32(In[2]);
        //w3 = _mm_set1_epi32(In[3]); nonce will be later hacked into the hash
        w4 = _mm_set1_epi32(In[4]);
        w5 = _mm_set1_epi32(In[5]);
        w6 = _mm_set1_epi32(In[6]);
        w7 = _mm_set1_epi32(In[7]);
        w8 = _mm_set1_epi32(In[8]);
        w9 = _mm_set1_epi32(In[9]);
        w10 = _mm_set1_epi32(In[10]);
        w11 = _mm_set1_epi32(In[11]);
        w12 = _mm_set1_epi32(In[12]);
        w13 = _mm_set1_epi32(In[13]);
        w14 = _mm_set1_epi32(In[14]);
        w15 = _mm_set1_epi32(In[15]);

        /* hack nonce into lowest byte of w3 */
        nonce = _mm_set1_epi32(In[3]);
        nonce = _mm_add_epi32(nonce, offset);
        nonce = _mm_add_epi32(nonce, _mm_set1_epi32(k));
        w3 = nonce;

        a = _mm_set1_epi32(hPre[0]);
        b = _mm_set1_epi32(hPre[1]);
        c = _mm_set1_epi32(hPre[2]);
        d = _mm_set1_epi32(hPre[3]);
        e = _mm_set1_epi32(hPre[4]);
        f = _mm_set1_epi32(hPre[5]);
        g = _mm_set1_epi32(hPre[6]);
        h = _mm_set1_epi32(hPre[7]);

        SHA256ROUND(a, b, c, d, e, f, g, h, 0, w0);
        SHA256ROUND(h, a, b, c, d, e, f, g, 1, w1);
        SHA256ROUND(g, h, a, b, c, d, e, f, 2, w2);
        SHA256ROUND(f, g, h, a, b, c, d, e, 3, w3);
        SHA256ROUND(e, f, g, h, a, b, c, d, 4, w4);
        SHA256ROUND(d, e, f, g, h, a, b, c, 5, w5);
        SHA256ROUND(c, d, e, f, g, h, a, b, 6, w6);
        SHA256ROUND(b, c, d, e, f, g, h, a, 7, w7);
        SHA256ROUND(a, b, c, d, e, f, g, h, 8, w8);
        SHA256ROUND(h, a, b, c, d, e, f, g, 9, w9);
        SHA256ROUND(g, h, a, b, c, d, e, f, 10, w10);
        SHA256ROUND(f, g, h, a, b, c, d, e, 11, w11);
        SHA256ROUND(e, f, g, h, a, b, c, d, 12, w12);
        SHA256ROUND(d, e, f, g, h, a, b, c, 13, w13);
        SHA256ROUND(c, d, e, f, g, h, a, b, 14, w14);
        SHA256ROUND(b, c, d, e, f, g, h, a, 15, w15);

        w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
        SHA256ROUND(a, b, c, d, e, f, g, h, 16, w0);
        w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
        SHA256ROUND(h, a, b, c, d, e, f, g, 17, w1);
        w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
        SHA256ROUND(g, h, a, b, c, d, e, f, 18, w2);
        w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
        SHA256ROUND(f, g, h, a, b, c, d, e, 19, w3);
        w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
        SHA256ROUND(e, f, g, h, a, b, c, d, 20, w4);
        w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
        SHA256ROUND(d, e, f, g, h, a, b, c, 21, w5);
        w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
        SHA256ROUND(c, d, e, f, g, h, a, b, 22, w6);
        w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
        SHA256ROUND(b, c, d, e, f, g, h, a, 23, w7);
        w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
        SHA256ROUND(a, b, c, d, e, f, g, h, 24, w8);
        w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
        SHA256ROUND(h, a, b, c, d, e, f, g, 25, w9);
        w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
        SHA256ROUND(g, h, a, b, c, d, e, f, 26, w10);
        w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
        SHA256ROUND(f, g, h, a, b, c, d, e, 27, w11);
        w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
        SHA256ROUND(e, f, g, h, a, b, c, d, 28, w12);
        w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
        SHA256ROUND(d, e, f, g, h, a, b, c, 29, w13);
        w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
        SHA256ROUND(c, d, e, f, g, h, a, b, 30, w14);
        w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
        SHA256ROUND(b, c, d, e, f, g, h, a, 31, w15);

        w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
        SHA256ROUND(a, b, c, d, e, f, g, h, 32, w0);
        w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
        SHA256ROUND(h, a, b, c, d, e, f, g, 33, w1);
        w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
        SHA256ROUND(g, h, a, b, c, d, e, f, 34, w2);
        w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
        SHA256ROUND(f, g, h, a, b, c, d, e, 35, w3);
        w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
        SHA256ROUND(e, f, g, h, a, b, c, d, 36, w4);
        w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
        SHA256ROUND(d, e, f, g, h, a, b, c, 37, w5);
        w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
        SHA256ROUND(c, d, e, f, g, h, a, b, 38, w6);
        w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
        SHA256ROUND(b, c, d, e, f, g, h, a, 39, w7);
        w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
        SHA256ROUND(a, b, c, d, e, f, g, h, 40, w8);
        w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
        SHA256ROUND(h, a, b, c, d, e, f, g, 41, w9);
        w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
        SHA256ROUND(g, h, a, b, c, d, e, f, 42, w10);
        w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
        SHA256ROUND(f, g, h, a, b, c, d, e, 43, w11);
        w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
        SHA256ROUND(e, f, g, h, a, b, c, d, 44, w12);
        w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
        SHA256ROUND(d, e, f, g, h, a, b, c, 45, w13);
        w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
        SHA256ROUND(c, d, e, f, g, h, a, b, 46, w14);
        w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
        SHA256ROUND(b, c, d, e, f, g, h, a, 47, w15);

        w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
        SHA256ROUND(a, b, c, d, e, f, g, h, 48, w0);
        w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
        SHA256ROUND(h, a, b, c, d, e, f, g, 49, w1);
        w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
        SHA256ROUND(g, h, a, b, c, d, e, f, 50, w2);
        w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
        SHA256ROUND(f, g, h, a, b, c, d, e, 51, w3);
        w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
        SHA256ROUND(e, f, g, h, a, b, c, d, 52, w4);
        w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
        SHA256ROUND(d, e, f, g, h, a, b, c, 53, w5);
        w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
        SHA256ROUND(c, d, e, f, g, h, a, b, 54, w6);
        w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
        SHA256ROUND(b, c, d, e, f, g, h, a, 55, w7);
        w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
        SHA256ROUND(a, b, c, d, e, f, g, h, 56, w8);
        w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
        SHA256ROUND(h, a, b, c, d, e, f, g, 57, w9);
        w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
        SHA256ROUND(g, h, a, b, c, d, e, f, 58, w10);
        w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
        SHA256ROUND(f, g, h, a, b, c, d, e, 59, w11);
        w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
        SHA256ROUND(e, f, g, h, a, b, c, d, 60, w12);
        w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
        SHA256ROUND(d, e, f, g, h, a, b, c, 61, w13);
        w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
        SHA256ROUND(c, d, e, f, g, h, a, b, 62, w14);
        w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
        SHA256ROUND(b, c, d, e, f, g, h, a, 63, w15);

#define store_load(x, i, dest) \
        T1 = _mm_set1_epi32((hPre)[i]); \
        dest = _mm_add_epi32(T1, x);

        store_load(a, 0, w0);
        store_load(b, 1, w1);
        store_load(c, 2, w2);
        store_load(d, 3, w3);
        store_load(e, 4, w4);
        store_load(f, 5, w5);
        store_load(g, 6, w6);
        store_load(h, 7, w7);

        w8 = _mm_set1_epi32(Pad[8]);
        w9 = _mm_set1_epi32(Pad[9]);
        w10 = _mm_set1_epi32(Pad[10]);
        w11 = _mm_set1_epi32(Pad[11]);
        w12 = _mm_set1_epi32(Pad[12]);
        w13 = _mm_set1_epi32(Pad[13]);
        w14 = _mm_set1_epi32(Pad[14]);
        w15 = _mm_set1_epi32(Pad[15]);

        a = _mm_set1_epi32(hInit[0]);
        b = _mm_set1_epi32(hInit[1]);
        c = _mm_set1_epi32(hInit[2]);
        d = _mm_set1_epi32(hInit[3]);
        e = _mm_set1_epi32(hInit[4]);
        f = _mm_set1_epi32(hInit[5]);
        g = _mm_set1_epi32(hInit[6]);
        h = _mm_set1_epi32(hInit[7]);

        SHA256ROUND(a, b, c, d, e, f, g, h, 0, w0);
        SHA256ROUND(h, a, b, c, d, e, f, g, 1, w1);
        SHA256ROUND(g, h, a, b, c, d, e, f, 2, w2);
        SHA256ROUND(f, g, h, a, b, c, d, e, 3, w3);
        SHA256ROUND(e, f, g, h, a, b, c, d, 4, w4);
        SHA256ROUND(d, e, f, g, h, a, b, c, 5, w5);
        SHA256ROUND(c, d, e, f, g, h, a, b, 6, w6);
        SHA256ROUND(b, c, d, e, f, g, h, a, 7, w7);
        SHA256ROUND(a, b, c, d, e, f, g, h, 8, w8);
        SHA256ROUND(h, a, b, c, d, e, f, g, 9, w9);
        SHA256ROUND(g, h, a, b, c, d, e, f, 10, w10);
        SHA256ROUND(f, g, h, a, b, c, d, e, 11, w11);
        SHA256ROUND(e, f, g, h, a, b, c, d, 12, w12);
        SHA256ROUND(d, e, f, g, h, a, b, c, 13, w13);
        SHA256ROUND(c, d, e, f, g, h, a, b, 14, w14);
        SHA256ROUND(b, c, d, e, f, g, h, a, 15, w15);

        w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
        SHA256ROUND(a, b, c, d, e, f, g, h, 16, w0);
        w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
        SHA256ROUND(h, a, b, c, d, e, f, g, 17, w1);
        w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
        SHA256ROUND(g, h, a, b, c, d, e, f, 18, w2);
        w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
        SHA256ROUND(f, g, h, a, b, c, d, e, 19, w3);
        w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
        SHA256ROUND(e, f, g, h, a, b, c, d, 20, w4);
        w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
        SHA256ROUND(d, e, f, g, h, a, b, c, 21, w5);
        w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
        SHA256ROUND(c, d, e, f, g, h, a, b, 22, w6);
        w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
        SHA256ROUND(b, c, d, e, f, g, h, a, 23, w7);
        w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
        SHA256ROUND(a, b, c, d, e, f, g, h, 24, w8);
        w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
        SHA256ROUND(h, a, b, c, d, e, f, g, 25, w9);
        w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
        SHA256ROUND(g, h, a, b, c, d, e, f, 26, w10);
        w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
        SHA256ROUND(f, g, h, a, b, c, d, e, 27, w11);
        w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
        SHA256ROUND(e, f, g, h, a, b, c, d, 28, w12);
        w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
        SHA256ROUND(d, e, f, g, h, a, b, c, 29, w13);
        w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
        SHA256ROUND(c, d, e, f, g, h, a, b, 30, w14);
        w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
        SHA256ROUND(b, c, d, e, f, g, h, a, 31, w15);

        w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
        SHA256ROUND(a, b, c, d, e, f, g, h, 32, w0);
        w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
        SHA256ROUND(h, a, b, c, d, e, f, g, 33, w1);
        w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
        SHA256ROUND(g, h, a, b, c, d, e, f, 34, w2);
        w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
        SHA256ROUND(f, g, h, a, b, c, d, e, 35, w3);
        w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
        SHA256ROUND(e, f, g, h, a, b, c, d, 36, w4);
        w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
        SHA256ROUND(d, e, f, g, h, a, b, c, 37, w5);
        w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
        SHA256ROUND(c, d, e, f, g, h, a, b, 38, w6);
        w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
        SHA256ROUND(b, c, d, e, f, g, h, a, 39, w7);
        w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
        SHA256ROUND(a, b, c, d, e, f, g, h, 40, w8);
        w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
        SHA256ROUND(h, a, b, c, d, e, f, g, 41, w9);
        w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
        SHA256ROUND(g, h, a, b, c, d, e, f, 42, w10);
        w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
        SHA256ROUND(f, g, h, a, b, c, d, e, 43, w11);
        w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
        SHA256ROUND(e, f, g, h, a, b, c, d, 44, w12);
        w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
        SHA256ROUND(d, e, f, g, h, a, b, c, 45, w13);
        w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
        SHA256ROUND(c, d, e, f, g, h, a, b, 46, w14);
        w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
        SHA256ROUND(b, c, d, e, f, g, h, a, 47, w15);

        w0 = add4(SIGMA1_256(w14), w9, SIGMA0_256(w1), w0);
        SHA256ROUND(a, b, c, d, e, f, g, h, 48, w0);
        w1 = add4(SIGMA1_256(w15), w10, SIGMA0_256(w2), w1);
        SHA256ROUND(h, a, b, c, d, e, f, g, 49, w1);
        w2 = add4(SIGMA1_256(w0), w11, SIGMA0_256(w3), w2);
        SHA256ROUND(g, h, a, b, c, d, e, f, 50, w2);
        w3 = add4(SIGMA1_256(w1), w12, SIGMA0_256(w4), w3);
        SHA256ROUND(f, g, h, a, b, c, d, e, 51, w3);
        w4 = add4(SIGMA1_256(w2), w13, SIGMA0_256(w5), w4);
        SHA256ROUND(e, f, g, h, a, b, c, d, 52, w4);
        w5 = add4(SIGMA1_256(w3), w14, SIGMA0_256(w6), w5);
        SHA256ROUND(d, e, f, g, h, a, b, c, 53, w5);
        w6 = add4(SIGMA1_256(w4), w15, SIGMA0_256(w7), w6);
        SHA256ROUND(c, d, e, f, g, h, a, b, 54, w6);
        w7 = add4(SIGMA1_256(w5), w0, SIGMA0_256(w8), w7);
        SHA256ROUND(b, c, d, e, f, g, h, a, 55, w7);
        w8 = add4(SIGMA1_256(w6), w1, SIGMA0_256(w9), w8);
        SHA256ROUND(a, b, c, d, e, f, g, h, 56, w8);
        w9 = add4(SIGMA1_256(w7), w2, SIGMA0_256(w10), w9);
        SHA256ROUND(h, a, b, c, d, e, f, g, 57, w9);
        w10 = add4(SIGMA1_256(w8), w3, SIGMA0_256(w11), w10);
        SHA256ROUND(g, h, a, b, c, d, e, f, 58, w10);
        w11 = add4(SIGMA1_256(w9), w4, SIGMA0_256(w12), w11);
        SHA256ROUND(f, g, h, a, b, c, d, e, 59, w11);
        w12 = add4(SIGMA1_256(w10), w5, SIGMA0_256(w13), w12);
        SHA256ROUND(e, f, g, h, a, b, c, d, 60, w12);
        w13 = add4(SIGMA1_256(w11), w6, SIGMA0_256(w14), w13);
        SHA256ROUND(d, e, f, g, h, a, b, c, 61, w13);
        w14 = add4(SIGMA1_256(w12), w7, SIGMA0_256(w15), w14);
        SHA256ROUND(c, d, e, f, g, h, a, b, 62, w14);
        w15 = add4(SIGMA1_256(w13), w8, SIGMA0_256(w0), w15);
        SHA256ROUND(b, c, d, e, f, g, h, a, 63, w15);

        /* store resulsts directly in thash */
#define store_2(x,i)  \
        w0 = _mm_set1_epi32(hInit[i]); \
        *(__m128i *)&(thash)[i][0+k] = _mm_add_epi32(w0, x);

        store_2(a, 0);
        store_2(b, 1);
        store_2(c, 2);
        store_2(d, 3);
        store_2(e, 4);
        store_2(f, 5);
        store_2(g, 6);
        store_2(h, 7);
        *(__m128i *)&(thash)[8][0+k] = nonce;
    }

}

#endif // FOURWAYSSE2
