/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
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
#include "macros.h"

#include <xmmintrin.h>
#include <tmmintrin.h>

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int j, k;
	dig_t b1, b2;

	if (digit == 0) {
		dv_zero(c, RLC_FB_DIGS + 1);
		return;
	}
	if (digit == 1) {
		fb_copy(c, a);
		return;
	}
	c[RLC_FB_DIGS] = fb_lshb_low(c, a, util_bits_dig(digit) - 1);
	for (int i = util_bits_dig(digit) - 2; i > 0; i--) {
		if (digit & ((dig_t)1 << i)) {
			j = RLC_DIG - i;
			b1 = a[0];
			c[0] ^= (b1 << i);
			for (k = 1; k < RLC_FB_DIGS; k++) {
				b2 = a[k];
				c[k] ^= ((b2 << i) | (b1 >> j));
				b1 = b2;
			}
			c[RLC_FB_DIGS] ^= (b1 >> j);
		}
	}
	if (digit & (dig_t)1) {
		fb_add(c, c, a);
	}
}

void fb_muln_low(dig_t *c, const dig_t *a, const dig_t *b) {
	__m128i tab[16][8], tab1[16][8];
	__m128i s0, m0, m1, m2, m3, m4, m8, m9;
	char ta, tb;
	int i, j, k;

#define LOOKUP(i, T)\
	T[0][i] = _mm_setzero_si128();\
	T[1][i] = m0;\
    T[2][i] = m1;\
    T[3][i] = m9=_mm_xor_si128(m0,m1);\
    T[4][i] = m2;\
    T[5][i] = _mm_xor_si128(m2,m0);\
    T[6][i] = _mm_xor_si128(m2,m1);\
    T[7][i] = _mm_xor_si128(m2,m9);\
    T[8][i] =  m3;\
    T[9][i] =  _mm_xor_si128(m3,m0);\
    T[10][i] = _mm_xor_si128(m3,m1);\
    T[11][i] = _mm_xor_si128(m3,m9);\
    T[12][i] = m2=_mm_xor_si128(m3,m2);\
    T[13][i] = _mm_xor_si128(m2,m0);\
    T[14][i] = _mm_xor_si128(m2,m1);\
    T[15][i] = _mm_xor_si128(m2,m9);

	s0 = _mm_setzero_si128();
	for (i = 0; i < 2; i++) {
		m0 = _mm_load_si128((__m128i *) (b + 2 * i));
		m9 = _mm_srli_epi64(m0, 57);
		m8 = _mm_slli_si128(m9, 8);
		m8 = _mm_xor_si128(m8, s0);
		s0 = _mm_srli_si128(m9, 8);
		m1 = _mm_slli_epi64(m0, 1);
		m2 = _mm_slli_epi64(m0, 2);
		m3 = _mm_slli_epi64(m0, 3);
		m3 = _mm_xor_si128(m3, _mm_srli_epi64(m8, 4));
		m2 = _mm_xor_si128(m2, _mm_srli_epi64(m8, 5));
		m1 = _mm_xor_si128(m1, _mm_srli_epi64(m8, 6));
		LOOKUP(i, tab);
		m4 = m0;
		m0 = _mm_slli_epi64(m4, 4);
		m1 = _mm_slli_epi64(m4, 5);
		m2 = _mm_slli_epi64(m4, 6);
		m3 = _mm_slli_epi64(m4, 7);
		m3 = _mm_xor_si128(m3, m8);
		m2 = _mm_xor_si128(m2, _mm_srli_epi64(m8, 1));
		m1 = _mm_xor_si128(m1, _mm_srli_epi64(m8, 2));
		m0 = _mm_xor_si128(m0, _mm_srli_epi64(m8, 3));
		LOOKUP(i, tab1);
	}
	m3 = s0;
	m2 = _mm_srli_epi64(s0, 1);
	tab1[0][i] = tab1[1][i] = tab1[2][i] = tab1[3][i] = _mm_setzero_si128();
    tab1[4][i] = tab1[5][i] = tab1[6][i] = tab1[7][i] = m2;
    tab1[8][i] = tab1[9][i] = tab1[10][i] = tab1[11][i] = m3;
    m2 =_mm_xor_si128(m3,m2);
    tab1[12][i] = tab1[13][i] = tab1[14][i] = tab1[15][i] = m2;
#undef LOOKUP

#define LSHIFT8(m3,m2,m1,m0)\
	m3=_mm_alignr_epi8(m3,m2,15);\
	m2=_mm_alignr_epi8(m2,m1,15);\
	m1=_mm_alignr_epi8(m1,m0,15);\
	m0=_mm_slli_si128(m0,1);

#define M(m2,m1,m0,ta,tb)\
	m0=_mm_xor_si128(m0, tab[ta&0xf][0]);\
	m1=_mm_xor_si128(m1, tab[ta&0xf][1]);\
	m0=_mm_xor_si128(m0,tab1[tb&0xf][0]);\
	m1=_mm_xor_si128(m1,tab1[tb&0xf][1]);\
	m2=_mm_xor_si128(m2,tab1[tb&0xf][2]);\

	// Main computation
	m0 = m1 = m2 = m3 = _mm_setzero_si128();

	for (j = 56; j >= 0; j -= 8) {
		k = j + 4;
		ta = (a[1] >> j);
		tb = (a[1] >> k);
		M(m2, m1, m0, ta, tb);
		ta = (a[3] >> j);
		tb = (a[3] >> k);
		M(m3, m2, m1, ta, tb);
		LSHIFT8(m3, m2, m1, m0);
	}
	for (j = 56; j >= 8; j -= 8) {
		k = j + 4;
		ta = (a[0] >> j);
		tb = (a[0] >> k);
		M(m2, m1, m0, ta, tb);
		ta = (a[2] >> j);
		tb = (a[2] >> k);
		M(m3, m2, m1, ta, tb);
		LSHIFT8(m3, m2, m1, m0);
	}
	ta = a[0];
	tb = (a[0] >> 4);
	M(m2, m1, m0, ta, tb);
	ta = a[2];
	tb = (a[2] >> 4);
	M(m3, m2, m1, ta, tb);

	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) c + 1, m1);
	_mm_store_si128((__m128i *) c + 2, m2);
	_mm_store_si128((__m128i *) c + 3, m3);
#undef M
}

#if !defined(__INTEL_COMPILER)

void fb_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	__m128i tab[16][8], tab1[16][8];
	__m128i s0, m0, m1, m2, m3, m4, m8, m9;
	rlc_align dig_t t[2*RLC_FB_DIGS];
	char ta, tb;
	int i, j, k;

#define LOOKUP(i, T)\
	T[0][i] = _mm_setzero_si128();\
	T[1][i] = m0;\
    T[2][i] = m1;\
    T[3][i] = m9=_mm_xor_si128(m0,m1);\
    T[4][i] = m2;\
    T[5][i] = _mm_xor_si128(m2,m0);\
    T[6][i] = _mm_xor_si128(m2,m1);\
    T[7][i] = _mm_xor_si128(m2,m9);\
    T[8][i] =  m3;\
    T[9][i] =  _mm_xor_si128(m3,m0);\
    T[10][i] = _mm_xor_si128(m3,m1);\
    T[11][i] = _mm_xor_si128(m3,m9);\
    T[12][i] = m2=_mm_xor_si128(m3,m2);\
    T[13][i] = _mm_xor_si128(m2,m0);\
    T[14][i] = _mm_xor_si128(m2,m1);\
    T[15][i] = _mm_xor_si128(m2,m9);

	s0 = _mm_setzero_si128();
	for (i = 0; i < 2; i++) {
		m0 = _mm_load_si128((__m128i *) (b + 2 * i));
		m9 = _mm_srli_epi64(m0, 57);
		m8 = _mm_slli_si128(m9, 8);
		m8 = _mm_xor_si128(m8, s0);
		s0 = _mm_srli_si128(m9, 8);
		m1 = _mm_slli_epi64(m0, 1);
		m2 = _mm_slli_epi64(m0, 2);
		m3 = _mm_slli_epi64(m0, 3);
		m3 = _mm_xor_si128(m3, _mm_srli_epi64(m8, 4));
		m2 = _mm_xor_si128(m2, _mm_srli_epi64(m8, 5));
		m1 = _mm_xor_si128(m1, _mm_srli_epi64(m8, 6));
		LOOKUP(i, tab);
		m4 = m0;
		m0 = _mm_slli_epi64(m4, 4);
		m1 = _mm_slli_epi64(m4, 5);
		m2 = _mm_slli_epi64(m4, 6);
		m3 = _mm_slli_epi64(m4, 7);
		m3 = _mm_xor_si128(m3, m8);
		m2 = _mm_xor_si128(m2, _mm_srli_epi64(m8, 1));
		m1 = _mm_xor_si128(m1, _mm_srli_epi64(m8, 2));
		m0 = _mm_xor_si128(m0, _mm_srli_epi64(m8, 3));
		LOOKUP(i, tab1);
	}
	m3 = s0;
	m2 = _mm_srli_epi64(s0, 1);
	m1 = _mm_setzero_si128();//_mm_srli_epi64(m8, 2);
	m0 = _mm_setzero_si128();//_mm_srli_epi64(m8, 3);
	tab1[0][i] = tab1[1][i] = tab1[2][i] = tab1[3][i] = m9 = _mm_setzero_si128();
    tab1[4][i] = tab1[5][i] = tab1[6][i] = tab1[7][i] = m2;
    tab1[8][i] = tab1[9][i] = tab1[10][i] = tab1[11][i] = m3;
    m2 =_mm_xor_si128(m3,m2);
    tab1[12][i] = tab1[13][i] = tab1[14][i] = tab1[15][i] = m2;
#undef LOOKUP

#define LSHIFT8(m3,m2,m1,m0)\
	m3=_mm_alignr_epi8(m3,m2,15);\
	m2=_mm_alignr_epi8(m2,m1,15);\
	m1=_mm_alignr_epi8(m1,m0,15);\
	m0=_mm_slli_si128(m0,1);

#define M(m2,m1,m0,ta,tb)\
	m0=_mm_xor_si128(m0, tab[ta&0xf][0]);\
	m1=_mm_xor_si128(m1, tab[ta&0xf][1]);\
	m0=_mm_xor_si128(m0,tab1[tb&0xf][0]);\
	m1=_mm_xor_si128(m1,tab1[tb&0xf][1]);\
	m2=_mm_xor_si128(m2,tab1[tb&0xf][2]);\

	// Main computation
	m0 = m1 = m2 = m3 = _mm_setzero_si128();

	for (j = 56; j >= 0; j -= 8) {
		k = j + 4;
		ta = (a[1] >> j);
		tb = (a[1] >> k);
		M(m2, m1, m0, ta, tb);
		ta = (a[3] >> j);
		tb = (a[3] >> k);
		M(m3, m2, m1, ta, tb);
		LSHIFT8(m3, m2, m1, m0);
	}
	for (j = 56; j >= 8; j -= 8) {
		k = j + 4;
		ta = (a[0] >> j);
		tb = (a[0] >> k);
		M(m2, m1, m0, ta, tb);
		ta = (a[2] >> j);
		tb = (a[2] >> k);
		M(m3, m2, m1, ta, tb);
		LSHIFT8(m3, m2, m1, m0);
	}
	ta = a[0];
	tb = (a[0] >> 4);
	M(m2, m1, m0, ta, tb);
	ta = a[2];
	tb = (a[2] >> 4);
	M(m3, m2, m1, ta, tb);

	_mm_store_si128((__m128i *) t + 0, m0);
	_mm_store_si128((__m128i *) t + 1, m1);
	_mm_store_si128((__m128i *) t + 2, m2);
	_mm_store_si128((__m128i *) t + 3, m3);

	const int ra = 52;
	const int rb = 55;
	const int rc = 57;
	const int rh = 59;
	const int lh = 5;
	const int la = 12;
	const int lb = 9;
	const int lc = 7;

	dig_t d = t[7], a0 = t[0], a1 = t[1], a2 = t[2], a3 = t[3], a4 = t[4];

	a4 ^= (d >> rh);
	a4 ^= (d >> ra);
	a4 ^= (d >> rb);
	a4 ^= (d >> rc);

	a3 ^= (d << lh);
	a3 ^= (d << la);
	a3 ^= (d << lb);
	a3 ^= (d << lc);

	d = t[6];
	a3 ^= (d >> rh);
	a3 ^= (d >> ra);
	a3 ^= (d >> rb);
	a3 ^= (d >> rc);

	a2 ^= (d << lh);
	a2 ^= (d << la);
	a2 ^= (d << lb);
	a2 ^= (d << lc);

	d = t[5];
	a2 ^= (d >> rh);
	a2 ^= (d >> ra);
	a2 ^= (d >> rb);
	a2 ^= (d >> rc);

	a1 ^= (d << lh);
	a1 ^= (d << la);
	a1 ^= (d << lb);
	a1 ^= (d << lc);

	d = a4;
	a1 ^= (d >> rh);
	a1 ^= (d >> ra);
	a1 ^= (d >> rb);
	a1 ^= (d >> rc);

	a0 ^= (d << lh);
	a0 ^= (d << la);
	a0 ^= (d << lb);
	a0 ^= (d << lc);

	d = a3 >> rh;
	a0 ^= d;
	d <<= rh;

	a0 ^= (d >> ra);
	a0 ^= (d >> rb);
	a0 ^= (d >> rc);
	a3 ^= d;

	c[3] = a3;
	c[2] = a2;
	c[1] = a1;
	c[0] = a0;

	return;
}

#else

void fb_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	rlc_align __m128i tab[16][8], tab1[16][8];
	__m128i s0, m0, m1, m2, m3, m4, m8, m9, t0, t1, t2, *t;
	rlc_align dig_t x[2];
	char ta, tb;
	int i, j, k;

#define LOOKUP(i, T)\
	T[0][i] = _mm_setzero_si128();\
	T[1][i] = m0;\
    T[2][i] = m1;\
    T[3][i] = m9=_mm_xor_si128(m0,m1);\
    T[4][i] = m2;\
    T[5][i] = _mm_xor_si128(m2,m0);\
    T[6][i] = _mm_xor_si128(m2,m1);\
    T[7][i] = _mm_xor_si128(m2,m9);\
    T[8][i] =  m3;\
    T[9][i] =  _mm_xor_si128(m3,m0);\
    T[10][i] = _mm_xor_si128(m3,m1);\
    T[11][i] = _mm_xor_si128(m3,m9);\
    T[12][i] = m2=_mm_xor_si128(m3,m2);\
    T[13][i] = _mm_xor_si128(m2,m0);\
    T[14][i] = _mm_xor_si128(m2,m1);\
    T[15][i] = _mm_xor_si128(m2,m9);

	s0 = _mm_setzero_si128();
	for (i = 0; i < 2; i++) {
		m0 = _mm_load_si128((__m128i *) (b + 2 * i));
		m9 = _mm_srli_epi64(m0, 57);
		m8 = _mm_slli_si128(m9, 8);
		m8 = _mm_xor_si128(m8, s0);
		s0 = _mm_srli_si128(m9, 8);
		m1 = _mm_slli_epi64(m0, 1);
		m2 = _mm_slli_epi64(m0, 2);
		m3 = _mm_slli_epi64(m0, 3);
		m3 = _mm_xor_si128(m3, _mm_srli_epi64(m8, 4));
		m2 = _mm_xor_si128(m2, _mm_srli_epi64(m8, 5));
		m1 = _mm_xor_si128(m1, _mm_srli_epi64(m8, 6));
		LOOKUP(i, tab);
		m4 = m0;
		m0 = _mm_slli_epi64(m4, 4);
		m1 = _mm_slli_epi64(m4, 5);
		m2 = _mm_slli_epi64(m4, 6);
		m3 = _mm_slli_epi64(m4, 7);
		m3 = _mm_xor_si128(m3, m8);
		m2 = _mm_xor_si128(m2, _mm_srli_epi64(m8, 1));
		m1 = _mm_xor_si128(m1, _mm_srli_epi64(m8, 2));
		m0 = _mm_xor_si128(m0, _mm_srli_epi64(m8, 3));
		LOOKUP(i, tab1);
	}
	m3 = s0;
	m2 = _mm_srli_epi64(s0, 1);
	m1 = _mm_setzero_si128();
	m0 = _mm_setzero_si128();
	tab1[0][i] = tab1[1][i] = tab1[2][i] = tab1[3][i] = m9 = _mm_setzero_si128();
    tab1[4][i] = tab1[5][i] = tab1[6][i] = tab1[7][i] = m2;
    tab1[8][i] = tab1[9][i] = tab1[10][i] = tab1[11][i] = m3;
    m2 =_mm_xor_si128(m3,m2);
    tab1[12][i] = tab1[13][i] = tab1[14][i] = tab1[15][i] = m2;
#undef LOOKUP

#define LSHIFT8(m3,m2,m1,m0)\
	m3=_mm_alignr_epi8(m3,m2,15);\
	m2=_mm_alignr_epi8(m2,m1,15);\
	m1=_mm_alignr_epi8(m1,m0,15);\
	m0=_mm_slli_si128(m0,1);

#define M(m2,m1,m0,ta,tb)\
	ta &= 0x0f; tb &= 0x0f;\
	m0=_mm_xor_si128(m0, tab[ta][0]);\
	m1=_mm_xor_si128(m1, tab[ta][1]);\
	m0=_mm_xor_si128(m0,tab1[tb][0]);\
	m1=_mm_xor_si128(m1,tab1[tb][1]);\
	m2=_mm_xor_si128(m2,tab1[tb][2]);\

	// Main computation
	m0 = m1 = m2 = m3 = _mm_setzero_si128();

	for (j = 56; j >= 0; j -= 8) {
		k = j + 4;
		ta = (a[1] >> j);
		tb = (a[1] >> k);
		M(m2, m1, m0, ta, tb);
		ta = (a[3] >> j);
		tb = (a[3] >> k);
		M(m3, m2, m1, ta, tb);
		LSHIFT8(m3, m2, m1, m0);
	}
	for (j = 56; j >= 8; j -= 8) {
		k = j + 4;
		ta = (a[0] >> j);
		tb = (a[0] >> k);
		M(m2, m1, m0, ta, tb);
		ta = (a[2] >> j);
		tb = (a[2] >> k);
		M(m3, m2, m1, ta, tb);
		LSHIFT8(m3, m2, m1, m0);
	}
	ta = a[0];
	tb = (a[0] >> 4);
	M(m2, m1, m0, ta, tb);
	ta = a[2];
	tb = (a[2] >> 4);
	M(m3, m2, m1, ta, tb);

#undef M

	REDUCE();
	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) x, m1);
	c[2] = x[0];
	c[3] = x[1] & 0x07FFFFFFFFFFFFFF;
#undef M
}

#endif
