/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of the low-level binary field bit multiplication functions.
 *
 * @ingroup fb
 */

#include <stdlib.h>

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"

#include <xmmintrin.h>
#include <tmmintrin.h>
#include "macros.h"

#define INV(A,B,C,D)	D,C,B,A

const relic_align uint32_t tm[] = {
	INV(0x00000000, 0x00000000, 0x00000000, 0x00000000),
	INV(0x0F0E0D0C, 0x0B0A0908, 0x07060504, 0x03020100),
	INV(0x1E1C1A18, 0x16141210, 0x0E0C0A08, 0x06040200),
	INV(0x11121714, 0x1D1E1B18, 0x090A0F0C, 0x05060300),
	INV(0x3C383430, 0x2C282420, 0x1C181410, 0x0C080400),
	INV(0x3336393C, 0x27222D28, 0x1B1E1114, 0x0F0A0500),
	INV(0x22242E28, 0x3A3C3630, 0x12141E18, 0x0A0C0600),
	INV(0x2D2A2324, 0x31363F38, 0x15121B1C, 0x090E0700),
	INV(0x78706860, 0x58504840, 0x38302820, 0x18100800),
	INV(0x777E656C, 0x535A4148, 0x3F362D24, 0x1B120900),
	INV(0x666C7278, 0x4E445A50, 0x363C2228, 0x1E140A00),
	INV(0x69627F74, 0x454E5358, 0x313A272C, 0x1D160B00),
	INV(0x44485C50, 0x74786C60, 0x24283C30, 0x14180C00),
	INV(0x4B46515C, 0x7F726568, 0x232E3934, 0x171A0D00),
	INV(0x5A544648, 0x626C7E70, 0x2A243638, 0x121C0E00),
	INV(0x555A4B44, 0x69667778, 0x2D22333C, 0x111E0F00),
};

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int j, k;
	dig_t b1, b2;

	if (digit == 0) {
		dv_zero(c, FB_DIGS + 1);
		return;
	}
	if (digit == 1) {
		fb_copy(c, a);
		return;
	}
	c[FB_DIGS] = fb_lshb_low(c, a, util_bits_dig(digit) - 1);
	for (int i = util_bits_dig(digit) - 2; i > 0; i--) {
		if (digit & ((dig_t)1 << i)) {
			j = FB_DIGIT - i;
			b1 = a[0];
			c[0] ^= (b1 << i);
			for (k = 1; k < FB_DIGS; k++) {
				b2 = a[k];
				c[k] ^= ((b2 << i) | (b1 >> j));
				b1 = b2;
			}
			c[FB_DIGS] ^= (b1 >> j);
		}
	}
	if (digit & (dig_t)1) {
		fb_add(c, c, a);
	}
}

void fb_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	__m128i rl[FB_DIGS], rh[FB_DIGS], l0, l1, h0, h1;
	__m128i t0, t1, mask, m[FB_DIGS], m0, m1, m2, m3, m8, m9;
	dig_t r0, r1, r2, r3;
	int i, j, k, ta;
	dig_t x[2];

	mask = _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F);

	/* computes BL, BH and BM */
	h0 = _mm_load_si128((__m128i *) & b[0]);
	h1 = _mm_load_si128((__m128i *) & b[2]);

#define LSHIFT4(m)\
  m1=m[3];\
  t1=_mm_srli_epi64(m1,60);\
  for(j=3;j>0;j--){\
    m1=_mm_slli_epi64(m1,4);\
    t0=_mm_xor_si128(m1,_mm_slli_si128(t1,8));\
    m1=m[j-1];\
    t1=_mm_srli_epi64(m1,60);\
    m[j]=_mm_xor_si128(t0,_mm_srli_si128(t1,8));\
   }\
   m[0]=_mm_slli_epi64(m1,4);\
   m[0]=_mm_xor_si128(m[0],_mm_slli_si128(t1,8));

#define LSHIFT8(m4,m3,m2,m1,m0)\
	m3=_mm_alignr_epi8(m3,m2,15);\
	m2=_mm_alignr_epi8(m2,m1,15);\
	m1=_mm_alignr_epi8(m1,m0,15);\
	m0=_mm_slli_si128(m0,1);

#define LSHIFT8V(m)\
	m[3]=_mm_alignr_epi8(m[3],m[2],15);\
	m[2]=_mm_alignr_epi8(m[2],m[1],15);\
	m[1]=_mm_alignr_epi8(m[1],m[0],15);\
	m[0]=_mm_slli_si128(m[0],1);

#define M(m1,m0,ta,b0,b1)\
	t0 = _mm_load_si128((__m128i *)tm + ta);\
	m0 =  _mm_xor_si128(m0, _mm_shuffle_epi8(t0, b0));\
	m1 =  _mm_xor_si128(m1, _mm_shuffle_epi8(t0, b1));\

#define MultK(al,b0,b1)\
	m0=m1=m2=m3=_mm_setzero_si128();\
	for(k=56;k>=0; k-=8){\
		ta=(r1>>k)&0x0f;\
		M(m1,m0,ta,b0,b1);\
		ta=(r3>>k)&0x0f;\
		M(m2,m1,ta,b0,b1);\
		LSHIFT8(m4,m3,m2,m1,m0);\
	}\
	for(k=56;k>=8; k-=8){\
		ta=(r0>>k)&0x0f;\
		M(m1,m0,ta,b0,b1);\
		ta=(r2>>k)&0x0f;\
		M(m2,m1,ta,b0,b1);\
		LSHIFT8(m4,m3,m2,m1,m0);\
	}\
	ta=(r0)&0xf; M(m1,m0,ta,b0,b1);\
	ta=(r2)&0xf; M(m2,m1,ta,b0,b1);\

	l0 = _mm_and_si128(h0, mask);
	l1 = _mm_and_si128(h1, mask);

	r0 = a[0];
	r1 = a[1];
	r2 = a[2];
	r3 = a[3];

	/* AL * BL */
	MultK(a, l0, l1);

	rl[0] = m0;
	rl[1] = m1;
	rl[2] = m2;
	rl[3] = m3;

	h0 = _mm_and_si128(_mm_srli_epi64(h0, 4), mask);
	h1 = _mm_and_si128(_mm_srli_epi64(h1, 4), mask);

	r0 >>= 4;
	r1 >>= 4;
	r2 >>= 4;
	r3 >>= 4;

	/* AH * BH */
	MultK(ah, h0, h1);

	rh[0] = m0;
	rh[1] = m1;
	rh[2] = m2;
	rh[3] = m3;

	h0 = _mm_xor_si128(h0, l0);
	h1 = _mm_xor_si128(h1, l1);

	r0 ^= a[0];
	r1 ^= a[1];
	r2 ^= a[2];
	r3 ^= a[3];

	/* AM * BM */
	MultK(am, h0, h1);

	m[0] = m0;
	m[1] = m1;
	m[2] = m2;
	m[3] = m3;

	/* m = m + rh + rl */
	for (i = 0; i < FB_DIGS; i++) {
		m[i] = _mm_xor_si128(m[i], rh[i]);
		m[i] = _mm_xor_si128(m[i], rl[i]);
	}

	/* m= m + x^8 rh + x^4 m + rl */

	LSHIFT4(m);
	LSHIFT8V(rh);

	for (i = 0; i < FB_DIGS; i++) {
		m[i] = _mm_xor_si128(m[i], rh[i]);
		m[i] = _mm_xor_si128(m[i], rl[i]);
	}

	m0 = m[0];
	m1 = m[1];
	m2 = m[2];
	m3 = m[3];

	REDUCE();
	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) x, m1);
	c[2] = x[0];
	c[3] = x[1] & 0x07FFFFFFFFFFFFFF;
}

void fb_muln_low(dig_t *c, const dig_t * a, const dig_t * b) {
	__m128i rl[FB_DIGS], rh[FB_DIGS], l0, l1, h0, h1;
	__m128i t0, t1, mask, m[FB_DIGS], m0, m1, m2, m3;
	dig_t r0, r1, r2, r3;
	int i, j, k, ta;

	mask = _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F);

	/* computes BL, BH and BM */
	h0 = _mm_load_si128((__m128i *) & b[0]);
	h1 = _mm_load_si128((__m128i *) & b[2]);

#define LSHIFT4(m)\
  m1=m[3];\
  t1=_mm_srli_epi64(m1,60);\
  for(j=3;j>0;j--){\
    m1=_mm_slli_epi64(m1,4);\
    t0=_mm_xor_si128(m1,_mm_slli_si128(t1,8));\
    m1=m[j-1];\
    t1=_mm_srli_epi64(m1,60);\
    m[j]=_mm_xor_si128(t0,_mm_srli_si128(t1,8));\
   }\
   m[0]=_mm_slli_epi64(m1,4);\
   m[0]=_mm_xor_si128(m[0],_mm_slli_si128(t1,8));

#define LSHIFT8(m4,m3,m2,m1,m0)\
	m3=_mm_alignr_epi8(m3,m2,15);\
	m2=_mm_alignr_epi8(m2,m1,15);\
	m1=_mm_alignr_epi8(m1,m0,15);\
	m0=_mm_slli_si128(m0,1);

#define LSHIFT8V(m)\
	m[3]=_mm_alignr_epi8(m[3],m[2],15);\
	m[2]=_mm_alignr_epi8(m[2],m[1],15);\
	m[1]=_mm_alignr_epi8(m[1],m[0],15);\
	m[0]=_mm_slli_si128(m[0],1);

#define M(m1,m0,ta,b0,b1)\
	t0 = _mm_load_si128((__m128i *)tm + ta);\
	m0 =  _mm_xor_si128(m0, _mm_shuffle_epi8(t0, b0));\
	m1 =  _mm_xor_si128(m1, _mm_shuffle_epi8(t0, b1));\

#define MultK(al,b0,b1)\
	m0=m1=m2=m3=_mm_setzero_si128();\
	for(k=56;k>=0; k-=8){\
		ta=(r1>>k)&0x0f;\
		M(m1,m0,ta,b0,b1);\
		ta=(r3>>k)&0x0f;\
		M(m2,m1,ta,b0,b1);\
		LSHIFT8(m4,m3,m2,m1,m0);\
	}\
	for(k=56;k>=8; k-=8){\
		ta=(r0>>k)&0x0f;\
		M(m1,m0,ta,b0,b1);\
		ta=(r2>>k)&0x0f;\
		M(m2,m1,ta,b0,b1);\
		LSHIFT8(m4,m3,m2,m1,m0);\
	}\
	ta=(r0)&0xf; M(m1,m0,ta,b0,b1);\
	ta=(r2)&0xf; M(m2,m1,ta,b0,b1);\

	l0 = _mm_and_si128(h0, mask);
	l1 = _mm_and_si128(h1, mask);

	r0 = a[0];
	r1 = a[1];
	r2 = a[2];
	r3 = a[3];

	/* AL * BL */
	MultK(a, l0, l1);

	rl[0] = m0;
	rl[1] = m1;
	rl[2] = m2;
	rl[3] = m3;

	h0 = _mm_and_si128(_mm_srli_epi64(h0, 4), mask);
	h1 = _mm_and_si128(_mm_srli_epi64(h1, 4), mask);

	r0 >>= 4;
	r1 >>= 4;
	r2 >>= 4;
	r3 >>= 4;

	/* AH * BH */
	MultK(ah, h0, h1);

	rh[0] = m0;
	rh[1] = m1;
	rh[2] = m2;
	rh[3] = m3;

	h0 = _mm_xor_si128(h0, l0);
	h1 = _mm_xor_si128(h1, l1);

	r0 ^= a[0];
	r1 ^= a[1];
	r2 ^= a[2];
	r3 ^= a[3];

	/* AM * BM */
	MultK(am, h0, h1);

	m[0] = m0;
	m[1] = m1;
	m[2] = m2;
	m[3] = m3;

	/* m = m + rh + rl */
	for (i = 0; i < FB_DIGS; i++) {
		m[i] = _mm_xor_si128(m[i], rh[i]);
		m[i] = _mm_xor_si128(m[i], rl[i]);
	}

	/* m= m + x^8 rh + x^4 m + rl */

	LSHIFT4(m);
	LSHIFT8V(rh);

	for (i = 0; i < FB_DIGS; i++) {
		m[i] = _mm_xor_si128(m[i], rh[i]);
		m[i] = _mm_xor_si128(m[i], rl[i]);
	}

	_mm_store_si128((__m128i *) c + 0, m[0]);
	_mm_store_si128((__m128i *) c + 1, m[1]);
	_mm_store_si128((__m128i *) c + 2, m[2]);
	_mm_store_si128((__m128i *) c + 3, m[3]);
}
