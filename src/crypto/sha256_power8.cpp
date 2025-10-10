// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// This is a translation to GCC extended asm syntax from YASM code by Intel

#include <stdint.h>

#include <altivec.h>

namespace sha256_power8
{

typedef __vector uint32_t uint32x4_p8;
typedef __vector uint8_t uint8x16_p8;

//! Gets the first uin32_t from a, b, c, d, converts from BE to host endian, and returns them concatenated
template<uint8_t OFFS> static inline uint32x4_p8 pack_bytes
        (const uint8x16_p8 a, const uint8x16_p8 b, const uint8x16_p8 c, const uint8x16_p8 d) {
    uint8x16_p8 perm1 = {0+OFFS,1+OFFS,2+OFFS,3+OFFS, 16+OFFS,17+OFFS,18+OFFS,19+OFFS, 0,0,0,0, 0,0,0,0};
#ifdef WORDS_BIGENDIAN
    uint8x16_p8 perm2 = {0,1,2,3, 4,5,6,7, 16,17,18,19, 20,21,22,23};
#else
    uint8x16_p8 perm2 = {3,2,1,0, 7,6,5,4, 19,18,17,16, 23,22,21,20};
#endif
    return (uint32x4_p8)vec_perm(vec_perm((uint8x16_p8)a, (uint8x16_p8)b, perm1), vec_perm((uint8x16_p8)c, (uint8x16_p8)d, perm1), perm2);
}

static const __attribute__((aligned(16))) uint32_t K[] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

#define Ch(x, y, z) vec_sel((z), (y), (x))
#define Maj(x, y, z) vec_sel((y), (z), vec_xor((x), (y)))

#define KRound(a, b, c, d, e, f, g, h, k, w) \
    do { \
        uint32x4_p8 t1 = h + Ch(e, f, g) + __builtin_crypto_vshasigmaw(e, 1, 0xf) + k + w; \
        uint32x4_p8 t2 = Maj(a, b, c) + __builtin_crypto_vshasigmaw(a, 1, 0); \
        d += t1; \
        h = t1 + t2; \
    } while(false);

#define Round(a, b, c, d, e, f, g, h, k, w) \
    do { \
        uint32x4_p8 kay = {k, k, k, k}; \
        uint32x4_p8 t1 = h + Ch(e, f, g) + __builtin_crypto_vshasigmaw(e, 1, 0xf) + kay + w; \
        uint32x4_p8 t2 = Maj(a, b, c) + __builtin_crypto_vshasigmaw(a, 1, 0); \
        d += t1; \
        h = t1 + t2; \
    } while(false);

#define KRound2(a, b, c, d, e, f, g, h, k) \
    do { \
        uint32x4_p8 t1 = h + Ch(e, f, g) + __builtin_crypto_vshasigmaw(e, 1, 0xf) + k; \
        uint32x4_p8 t2 = Maj(a, b, c) + __builtin_crypto_vshasigmaw(a, 1, 0); \
        d += t1; \
        h = t1 + t2; \
    } while(false);

#define Round2(a, b, c, d, e, f, g, h, k) \
    do { \
        uint32x4_p8 kay = {k, k, k, k}; \
        uint32x4_p8 t1 = h + Ch(e, f, g) + __builtin_crypto_vshasigmaw(e, 1, 0xf) + kay; \
        uint32x4_p8 t2 = Maj(a, b, c) + __builtin_crypto_vshasigmaw(a, 1, 0); \
        d += t1; \
        h = t1 + t2; \
    } while(false);

#define sigma0(w) __builtin_crypto_vshasigmaw(w, 0, 0)
#define sigma1(w) __builtin_crypto_vshasigmaw(w, 0, 0xf)

/** Perform a 4 double-SHA-256 64-byte updates at once*/
void Transform_4way(unsigned char* out, const unsigned char* in)
{
    uint32x4_p8 a = {0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul};
    uint32x4_p8 b = {0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul};
    uint32x4_p8 c = {0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul};
    uint32x4_p8 d = {0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul};
    uint32x4_p8 e = {0x510e527ful, 0x510e527ful, 0x510e527ful, 0x510e527ful};
    uint32x4_p8 f = {0x9b05688cul, 0x9b05688cul, 0x9b05688cul, 0x9b05688cul};
    uint32x4_p8 g = {0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul};
    uint32x4_p8 h = {0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul};

    uint8x16_p8 w0123_0 = vec_vsx_ld(0 *16 + 0  , in);
    uint8x16_p8 w4567_0 = vec_vsx_ld(1 *16 + 0  , in);
    uint8x16_p8 w8901_0 = vec_vsx_ld(2 *16 + 0  , in);
    uint8x16_p8 w2345_0 = vec_vsx_ld(3 *16 + 0  , in);

    uint8x16_p8 w0123_1 = vec_vsx_ld(0 *16 + 64 , in);
    uint8x16_p8 w4567_1 = vec_vsx_ld(1 *16 + 64 , in);
    uint8x16_p8 w8901_1 = vec_vsx_ld(2 *16 + 64 , in);
    uint8x16_p8 w2345_1 = vec_vsx_ld(3 *16 + 64 , in);

    uint8x16_p8 w0123_2 = vec_vsx_ld(0 *16 + 128, in);
    uint8x16_p8 w4567_2 = vec_vsx_ld(1 *16 + 128, in);
    uint8x16_p8 w8901_2 = vec_vsx_ld(2 *16 + 128, in);
    uint8x16_p8 w2345_2 = vec_vsx_ld(3 *16 + 128, in);

    uint8x16_p8 w0123_3 = vec_vsx_ld(0 *16 + 192, in);
    uint8x16_p8 w4567_3 = vec_vsx_ld(1 *16 + 192, in);
    uint8x16_p8 w8901_3 = vec_vsx_ld(2 *16 + 192, in);
    uint8x16_p8 w2345_3 = vec_vsx_ld(3 *16 + 192, in);

    uint32x4_p8 w0  = pack_bytes<0 >(w0123_0, w0123_1, w0123_2, w0123_3);
    uint32x4_p8 w1  = pack_bytes<4 >(w0123_0, w0123_1, w0123_2, w0123_3);
    uint32x4_p8 w2  = pack_bytes<8 >(w0123_0, w0123_1, w0123_2, w0123_3);
    uint32x4_p8 w3  = pack_bytes<12>(w0123_0, w0123_1, w0123_2, w0123_3);

    uint32x4_p8 w4  = pack_bytes<0 >(w4567_0, w4567_1, w4567_2, w4567_3);
    uint32x4_p8 w5  = pack_bytes<4 >(w4567_0, w4567_1, w4567_2, w4567_3);
    uint32x4_p8 w6  = pack_bytes<8 >(w4567_0, w4567_1, w4567_2, w4567_3);
    uint32x4_p8 w7  = pack_bytes<12>(w4567_0, w4567_1, w4567_2, w4567_3);

    uint32x4_p8 w8  = pack_bytes<0 >(w8901_0, w8901_1, w8901_2, w8901_3);
    uint32x4_p8 w9  = pack_bytes<4 >(w8901_0, w8901_1, w8901_2, w8901_3);
    uint32x4_p8 w10 = pack_bytes<8 >(w8901_0, w8901_1, w8901_2, w8901_3);
    uint32x4_p8 w11 = pack_bytes<12>(w8901_0, w8901_1, w8901_2, w8901_3);

    uint32x4_p8 w12 = pack_bytes<0 >(w2345_0, w2345_1, w2345_2, w2345_3);
    uint32x4_p8 w13 = pack_bytes<4 >(w2345_0, w2345_1, w2345_2, w2345_3);
    uint32x4_p8 w14 = pack_bytes<8 >(w2345_0, w2345_1, w2345_2, w2345_3);
    uint32x4_p8 w15 = pack_bytes<12>(w2345_0, w2345_1, w2345_2, w2345_3);

    uint32x4_p8 k = (uint32x4_p8)vec_ld(0, K);
    KRound(a, b, c, d, e, f, g, h, vec_splat(k, 0), w0);
    KRound(h, a, b, c, d, e, f, g, vec_splat(k, 1), w1);
    KRound(g, h, a, b, c, d, e, f, vec_splat(k, 2), w2);
    KRound(f, g, h, a, b, c, d, e, vec_splat(k, 3), w3);
    k = (uint32x4_p8)vec_ld(1*16, K);
    KRound(e, f, g, h, a, b, c, d, vec_splat(k, 0), w4);
    KRound(d, e, f, g, h, a, b, c, vec_splat(k, 1), w5);
    KRound(c, d, e, f, g, h, a, b, vec_splat(k, 2), w6);
    KRound(b, c, d, e, f, g, h, a, vec_splat(k, 3), w7);
    k = (uint32x4_p8)vec_ld(2*16, K);
    KRound(a, b, c, d, e, f, g, h, vec_splat(k, 0), w8);
    KRound(h, a, b, c, d, e, f, g, vec_splat(k, 1), w9);
    KRound(g, h, a, b, c, d, e, f, vec_splat(k, 2), w10);
    KRound(f, g, h, a, b, c, d, e, vec_splat(k, 3), w11);
    k = (uint32x4_p8)vec_ld(3*16, K);
    KRound(e, f, g, h, a, b, c, d, vec_splat(k, 0), w12);
    KRound(d, e, f, g, h, a, b, c, vec_splat(k, 1), w13);
    KRound(c, d, e, f, g, h, a, b, vec_splat(k, 2), w14);
    KRound(b, c, d, e, f, g, h, a, vec_splat(k, 3), w15);

    for (int i = 0; i < 3; i++) {
        k = (uint32x4_p8)vec_ld((4+4*i)*16, K);
        KRound(a, b, c, d, e, f, g, h, vec_splat(k, 0), (w0 += sigma1(w14) + w9 + sigma0(w1)));
        KRound(h, a, b, c, d, e, f, g, vec_splat(k, 1), (w1 += sigma1(w15) + w10 + sigma0(w2)));
        KRound(g, h, a, b, c, d, e, f, vec_splat(k, 2), (w2 += sigma1(w0) + w11 + sigma0(w3)));
        KRound(f, g, h, a, b, c, d, e, vec_splat(k, 3), (w3 += sigma1(w1) + w12 + sigma0(w4)));
        k = (uint32x4_p8)vec_ld((5+4*i)*16, K);
        KRound(e, f, g, h, a, b, c, d, vec_splat(k, 0), (w4 += sigma1(w2) + w13 + sigma0(w5)));
        KRound(d, e, f, g, h, a, b, c, vec_splat(k, 1), (w5 += sigma1(w3) + w14 + sigma0(w6)));
        KRound(c, d, e, f, g, h, a, b, vec_splat(k, 2), (w6 += sigma1(w4) + w15 + sigma0(w7)));
        KRound(b, c, d, e, f, g, h, a, vec_splat(k, 3), (w7 += sigma1(w5) + w0 + sigma0(w8)));
        k = (uint32x4_p8)vec_ld((6+4*i)*16, K);
        KRound(a, b, c, d, e, f, g, h, vec_splat(k, 0), (w8 += sigma1(w6) + w1 + sigma0(w9)));
        KRound(h, a, b, c, d, e, f, g, vec_splat(k, 1), (w9 += sigma1(w7) + w2 + sigma0(w10)));
        KRound(g, h, a, b, c, d, e, f, vec_splat(k, 2), (w10 += sigma1(w8) + w3 + sigma0(w11)));
        KRound(f, g, h, a, b, c, d, e, vec_splat(k, 3), (w11 += sigma1(w9) + w4 + sigma0(w12)));
        k = (uint32x4_p8)vec_ld((7+4*i)*16, K);
        KRound(e, f, g, h, a, b, c, d, vec_splat(k, 0), (w12 += sigma1(w10) + w5 + sigma0(w13)));
        KRound(d, e, f, g, h, a, b, c, vec_splat(k, 1), (w13 += sigma1(w11) + w6 + sigma0(w14)));
        KRound(c, d, e, f, g, h, a, b, vec_splat(k, 2), (w14 += sigma1(w12) + w7 + sigma0(w15)));
        KRound(b, c, d, e, f, g, h, a, vec_splat(k, 3), (w15 += sigma1(w13) + w8 + sigma0(w0)));
    }

    a += uint32x4_p8{0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul};
    b += uint32x4_p8{0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul};
    c += uint32x4_p8{0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul};
    d += uint32x4_p8{0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul};
    e += uint32x4_p8{0x510e527ful, 0x510e527ful, 0x510e527ful, 0x510e527ful};
    f += uint32x4_p8{0x9b05688cul, 0x9b05688cul, 0x9b05688cul, 0x9b05688cul};
    g += uint32x4_p8{0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul};
    h += uint32x4_p8{0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul};

    uint32x4_p8 t0 = a;
    uint32x4_p8 t1 = b;
    uint32x4_p8 t2 = c;
    uint32x4_p8 t3 = d;
    uint32x4_p8 t4 = e;
    uint32x4_p8 t5 = f;
    uint32x4_p8 t6 = g;
    uint32x4_p8 t7 = h;

    KRound2(a, b, c, d, e, f, g, h, 0xc28a2f98);
    KRound2(h, a, b, c, d, e, f, g, 0x71374491);
    KRound2(g, h, a, b, c, d, e, f, 0xb5c0fbcf);
    KRound2(f, g, h, a, b, c, d, e, 0xe9b5dba5);
    KRound2(e, f, g, h, a, b, c, d, 0x3956c25b);
    KRound2(d, e, f, g, h, a, b, c, 0x59f111f1);
    KRound2(c, d, e, f, g, h, a, b, 0x923f82a4);
    KRound2(b, c, d, e, f, g, h, a, 0xab1c5ed5);
    KRound2(a, b, c, d, e, f, g, h, 0xd807aa98);
    KRound2(h, a, b, c, d, e, f, g, 0x12835b01);
    KRound2(g, h, a, b, c, d, e, f, 0x243185be);
    KRound2(f, g, h, a, b, c, d, e, 0x550c7dc3);
    KRound2(e, f, g, h, a, b, c, d, 0x72be5d74);
    KRound2(d, e, f, g, h, a, b, c, 0x80deb1fe);
    KRound2(c, d, e, f, g, h, a, b, 0x9bdc06a7);
    KRound2(b, c, d, e, f, g, h, a, 0xc19bf374);
    KRound2(a, b, c, d, e, f, g, h, 0x649b69c1);
    KRound2(h, a, b, c, d, e, f, g, 0xf0fe4786);
    KRound2(g, h, a, b, c, d, e, f, 0x0fe1edc6);
    KRound2(f, g, h, a, b, c, d, e, 0x240cf254);
    KRound2(e, f, g, h, a, b, c, d, 0x4fe9346f);
    KRound2(d, e, f, g, h, a, b, c, 0x6cc984be);
    KRound2(c, d, e, f, g, h, a, b, 0x61b9411e);
    KRound2(b, c, d, e, f, g, h, a, 0x16f988fa);
    KRound2(a, b, c, d, e, f, g, h, 0xf2c65152);
    KRound2(h, a, b, c, d, e, f, g, 0xa88e5a6d);
    KRound2(g, h, a, b, c, d, e, f, 0xb019fc65);
    KRound2(f, g, h, a, b, c, d, e, 0xb9d99ec7);
    KRound2(e, f, g, h, a, b, c, d, 0x9a1231c3);
    KRound2(d, e, f, g, h, a, b, c, 0xe70eeaa0);
    KRound2(c, d, e, f, g, h, a, b, 0xfdb1232b);
    KRound2(b, c, d, e, f, g, h, a, 0xc7353eb0);
    KRound2(a, b, c, d, e, f, g, h, 0x3069bad5);
    KRound2(h, a, b, c, d, e, f, g, 0xcb976d5f);
    KRound2(g, h, a, b, c, d, e, f, 0x5a0f118f);
    KRound2(f, g, h, a, b, c, d, e, 0xdc1eeefd);
    KRound2(e, f, g, h, a, b, c, d, 0x0a35b689);
    KRound2(d, e, f, g, h, a, b, c, 0xde0b7a04);
    KRound2(c, d, e, f, g, h, a, b, 0x58f4ca9d);
    KRound2(b, c, d, e, f, g, h, a, 0xe15d5b16);
    KRound2(a, b, c, d, e, f, g, h, 0x007f3e86);
    KRound2(h, a, b, c, d, e, f, g, 0x37088980);
    KRound2(g, h, a, b, c, d, e, f, 0xa507ea32);
    KRound2(f, g, h, a, b, c, d, e, 0x6fab9537);
    KRound2(e, f, g, h, a, b, c, d, 0x17406110);
    KRound2(d, e, f, g, h, a, b, c, 0x0d8cd6f1);
    KRound2(c, d, e, f, g, h, a, b, 0xcdaa3b6d);
    KRound2(b, c, d, e, f, g, h, a, 0xc0bbbe37);
    KRound2(a, b, c, d, e, f, g, h, 0x83613bda);
    KRound2(h, a, b, c, d, e, f, g, 0xdb48a363);
    KRound2(g, h, a, b, c, d, e, f, 0x0b02e931);
    KRound2(f, g, h, a, b, c, d, e, 0x6fd15ca7);
    KRound2(e, f, g, h, a, b, c, d, 0x521afaca);
    KRound2(d, e, f, g, h, a, b, c, 0x31338431);
    KRound2(c, d, e, f, g, h, a, b, 0x6ed41a95);
    KRound2(b, c, d, e, f, g, h, a, 0x6d437890);
    KRound2(a, b, c, d, e, f, g, h, 0xc39c91f2);
    KRound2(h, a, b, c, d, e, f, g, 0x9eccabbd);
    KRound2(g, h, a, b, c, d, e, f, 0xb5c9a0e6);
    KRound2(f, g, h, a, b, c, d, e, 0x532fb63c);
    KRound2(e, f, g, h, a, b, c, d, 0xd2c741c6);
    KRound2(d, e, f, g, h, a, b, c, 0x07237ea3);
    KRound2(c, d, e, f, g, h, a, b, 0xa4954b68);
    KRound2(b, c, d, e, f, g, h, a, 0x4c191d76);


    w0 = t0 + a;
    w1 = t1 + b;
    w2 = t2 + c;
    w3 = t3 + d;
    w4 = t4 + e;
    w5 = t5 + f;
    w6 = t6 + g;
    w7 = t7 + h;

    a = uint32x4_p8{0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul};
    b = uint32x4_p8{0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul};
    c = uint32x4_p8{0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul};
    d = uint32x4_p8{0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul};
    e = uint32x4_p8{0x510e527ful, 0x510e527ful, 0x510e527ful, 0x510e527ful};
    f = uint32x4_p8{0x9b05688cul, 0x9b05688cul, 0x9b05688cul, 0x9b05688cul};
    g = uint32x4_p8{0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul};
    h = uint32x4_p8{0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul};

    Round(a, b, c, d, e, f, g, h, 0x428a2f98, w0);
    Round(h, a, b, c, d, e, f, g, 0x71374491, w1);
    Round(g, h, a, b, c, d, e, f, 0xb5c0fbcf, w2);
    Round(f, g, h, a, b, c, d, e, 0xe9b5dba5, w3);
    Round(e, f, g, h, a, b, c, d, 0x3956c25b, w4);
    Round(d, e, f, g, h, a, b, c, 0x59f111f1, w5);
    Round(c, d, e, f, g, h, a, b, 0x923f82a4, w6);
    Round(b, c, d, e, f, g, h, a, 0xab1c5ed5, w7);
    Round2(a, b, c, d, e, f, g, h, 0x5807aa98);
    Round2(h, a, b, c, d, e, f, g, 0x12835b01);
    Round2(g, h, a, b, c, d, e, f, 0x243185be);
    Round2(f, g, h, a, b, c, d, e, 0x550c7dc3);
    Round2(e, f, g, h, a, b, c, d, 0x72be5d74);
    Round2(d, e, f, g, h, a, b, c, 0x80deb1fe);
    Round2(c, d, e, f, g, h, a, b, 0x9bdc06a7);
    Round2(b, c, d, e, f, g, h, a, 0xc19bf274);
    Round(a, b, c, d, e, f, g, h, 0xe49b69c1, (w0 += sigma0(w1)));
    w1 += uint32x4_p8{0xa00000, 0xa00000, 0xa00000, 0xa00000};
    Round(h, a, b, c, d, e, f, g, 0xefbe4786, (w1 += sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x0fc19dc6, (w2 += sigma1(w0) + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x240ca1cc, (w3 += sigma1(w1) + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x2de92c6f, (w4 += sigma1(w2) + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x4a7484aa, (w5 += sigma1(w3) + sigma0(w6)));
    w6 += uint32x4_p8{0x100, 0x100, 0x100, 0x100};
    Round(c, d, e, f, g, h, a, b, 0x5cb0a9dc, (w6 += sigma1(w4) + sigma0(w7)));
    w7 += uint32x4_p8{0x11002000, 0x11002000, 0x11002000, 0x11002000};
    Round(b, c, d, e, f, g, h, a, 0x76f988da, (w7 += sigma1(w5) + w0));
    w8 = uint32x4_p8{0x80000000, 0x80000000, 0x80000000, 0x80000000};
    Round(a, b, c, d, e, f, g, h, 0x983e5152, (w8 += sigma1(w6) + w1));
    Round(h, a, b, c, d, e, f, g, 0xa831c66d, (w9 = sigma1(w7) + w2));
    Round(g, h, a, b, c, d, e, f, 0xb00327c8, (w10 = sigma1(w8) + w3));
    Round(f, g, h, a, b, c, d, e, 0xbf597fc7, (w11 = sigma1(w9) + w4));
    Round(e, f, g, h, a, b, c, d, 0xc6e00bf3, (w12 = sigma1(w10) + w5));
    Round(d, e, f, g, h, a, b, c, 0xd5a79147, (w13 = sigma1(w11) + w6));
    w14 = uint32x4_p8{0x400022, 0x400022, 0x400022, 0x400022};
    Round(c, d, e, f, g, h, a, b, 0x06ca6351, (w14 += sigma1(w12) + w7));
    w15 = uint32x4_p8{0x100, 0x100, 0x100, 0x100};
    Round(b, c, d, e, f, g, h, a, 0x14292967, (w15 += sigma1(w13) + w8 + sigma0(w0)));
    Round(a, b, c, d, e, f, g, h, 0x27b70a85, (w0 += sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0x2e1b2138, (w1 += sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x4d2c6dfc, (w2 += sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x53380d13, (w3 += sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x650a7354, (w4 += sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x766a0abb, (w5 += sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x81c2c92e, (w6 += sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x92722c85, (w7 += sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, d, e, f, g, h, 0xa2bfe8a1, (w8 += sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, c, d, e, f, g, 0xa81a664b, (w9 += sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, b, c, d, e, f, 0xc24b8b70, (w10 += sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, a, b, c, d, e, 0xc76c51a3, (w11 += sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, h, a, b, c, d, 0xd192e819, (w12 += sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, g, h, a, b, c, 0xd6990624, (w13 += sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, f, g, h, a, b, 0xf40e3585, (w14 += sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, 0x106aa070, (w15 += sigma1(w13) + w8 + sigma0(w0)));
    Round(a, b, c, d, e, f, g, h, 0x19a4c116, (w0 += sigma1(w14) + w9 + sigma0(w1)));
    Round(h, a, b, c, d, e, f, g, 0x1e376c08, (w1 += sigma1(w15) + w10 + sigma0(w2)));
    Round(g, h, a, b, c, d, e, f, 0x2748774c, (w2 += sigma1(w0) + w11 + sigma0(w3)));
    Round(f, g, h, a, b, c, d, e, 0x34b0bcb5, (w3 += sigma1(w1) + w12 + sigma0(w4)));
    Round(e, f, g, h, a, b, c, d, 0x391c0cb3, (w4 += sigma1(w2) + w13 + sigma0(w5)));
    Round(d, e, f, g, h, a, b, c, 0x4ed8aa4a, (w5 += sigma1(w3) + w14 + sigma0(w6)));
    Round(c, d, e, f, g, h, a, b, 0x5b9cca4f, (w6 += sigma1(w4) + w15 + sigma0(w7)));
    Round(b, c, d, e, f, g, h, a, 0x682e6ff3, (w7 += sigma1(w5) + w0 + sigma0(w8)));
    Round(a, b, c, d, e, f, g, h, 0x748f82ee, (w8 += sigma1(w6) + w1 + sigma0(w9)));
    Round(h, a, b, c, d, e, f, g, 0x78a5636f, (w9 += sigma1(w7) + w2 + sigma0(w10)));
    Round(g, h, a, b, c, d, e, f, 0x84c87814, (w10 += sigma1(w8) + w3 + sigma0(w11)));
    Round(f, g, h, a, b, c, d, e, 0x8cc70208, (w11 += sigma1(w9) + w4 + sigma0(w12)));
    Round(e, f, g, h, a, b, c, d, 0x90befffa, (w12 += sigma1(w10) + w5 + sigma0(w13)));
    Round(d, e, f, g, h, a, b, c, 0xa4506ceb, (w13 += sigma1(w11) + w6 + sigma0(w14)));
    Round(c, d, e, f, g, h, a, b, 0xbef9a3f7, (w14 + sigma1(w12) + w7 + sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, 0xc67178f2, (w15 + sigma1(w13) + w8 + sigma0(w0)));

    a += uint32x4_p8{0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul, 0x6a09e667ul};
    b += uint32x4_p8{0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul, 0xbb67ae85ul};
    c += uint32x4_p8{0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul, 0x3c6ef372ul};
    d += uint32x4_p8{0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul, 0xa54ff53aul};
    e += uint32x4_p8{0x510e527ful, 0x510e527ful, 0x510e527ful, 0x510e527ful};
    f += uint32x4_p8{0x9b05688cul, 0x9b05688cul, 0x9b05688cul, 0x9b05688cul};
    g += uint32x4_p8{0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul, 0x1f83d9abul};
    h += uint32x4_p8{0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul, 0x5be0cd19ul};

    w0123_0 = (uint8x16_p8)pack_bytes<0 >((uint8x16_p8)a, (uint8x16_p8)b, (uint8x16_p8)c, (uint8x16_p8)d);
    w4567_0 = (uint8x16_p8)pack_bytes<0 >((uint8x16_p8)e, (uint8x16_p8)f, (uint8x16_p8)g, (uint8x16_p8)h);

    w0123_1 = (uint8x16_p8)pack_bytes<4 >((uint8x16_p8)a, (uint8x16_p8)b, (uint8x16_p8)c, (uint8x16_p8)d);
    w4567_1 = (uint8x16_p8)pack_bytes<4 >((uint8x16_p8)e, (uint8x16_p8)f, (uint8x16_p8)g, (uint8x16_p8)h);

    w0123_2 = (uint8x16_p8)pack_bytes<8 >((uint8x16_p8)a, (uint8x16_p8)b, (uint8x16_p8)c, (uint8x16_p8)d);
    w4567_2 = (uint8x16_p8)pack_bytes<8 >((uint8x16_p8)e, (uint8x16_p8)f, (uint8x16_p8)g, (uint8x16_p8)h);

    w0123_3 = (uint8x16_p8)pack_bytes<12>((uint8x16_p8)a, (uint8x16_p8)b, (uint8x16_p8)c, (uint8x16_p8)d);
    w4567_3 = (uint8x16_p8)pack_bytes<12>((uint8x16_p8)e, (uint8x16_p8)f, (uint8x16_p8)g, (uint8x16_p8)h);

    vec_vsx_st(w0123_0, 0 *16 + 0 , out);
    vec_vsx_st(w4567_0, 1 *16 + 0 , out);

    vec_vsx_st(w0123_1, 0 *16 + 32, out);
    vec_vsx_st(w4567_1, 1 *16 + 32, out);

    vec_vsx_st(w0123_2, 0 *16 + 64, out);
    vec_vsx_st(w4567_2, 1 *16 + 64, out);

    vec_vsx_st(w0123_3, 0 *16 + 96, out);
    vec_vsx_st(w4567_3, 1 *16 + 96, out);
}
}
