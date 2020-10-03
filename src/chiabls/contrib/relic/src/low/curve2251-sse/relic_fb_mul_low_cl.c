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
#include "macros.h"

#include <xmmintrin.h>
#include <tmmintrin.h>
#include <wmmintrin.h>

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_muln_low(dig_t *c, const dig_t *a, const dig_t *b) {
	__m128i ma0, ma1, mb0, mb1, m0, m1, m2, m3, m4, m5, t0, t1, t2, t3;

	ma0 = _mm_load_si128((__m128i *)a);
	mb0 = _mm_load_si128((__m128i *)b);

	MUL(ma0, mb0);
	m0 = t0;
	m1 = t1;

	ma1 = _mm_load_si128((__m128i *)a + 1);
	mb1 = _mm_load_si128((__m128i *)b + 1);
	MUL(ma1, mb1);
	m2 = t0;
	m3 = t1;

	ma0 = XOR(ma0, ma1);
	mb0 = XOR(mb0, mb1);

	MUL(ma0, mb0);
	m4 = t0;
	m5 = t1;

	m4 = _mm_xor_si128(m4, m0);
	m5 = _mm_xor_si128(m5, m1);
	m4 = _mm_xor_si128(m4, m2);
	m5 = _mm_xor_si128(m5, m3);

	m1 = XOR(m1, m4);
	m2 = XOR(m2, m5);

	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) c + 1, m1);
	_mm_store_si128((__m128i *) c + 2, m2);
	_mm_store_si128((__m128i *) c + 3, m3);

}

#if !defined(__INTEL_COMPILER)

void fb_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	__m128i ma0, ma1, mb0, mb1, m0, m1, m2, m3, m4, m5, t0, t1, t2, t3;
	relic_align dig_t t[2*FB_DIGS];

	ma0 = _mm_load_si128((__m128i *)a);
	mb0 = _mm_load_si128((__m128i *)b);

	MUL(ma0, mb0);
	m0 = t0;
	m1 = t1;

	ma1 = _mm_load_si128((__m128i *)a + 1);
	mb1 = _mm_load_si128((__m128i *)b + 1);
	MUL(ma1, mb1);
	m2 = t0;
	m3 = t1;

	ma0 = XOR(ma0, ma1);
	mb0 = XOR(mb0, mb1);

	MUL(ma0, mb0);
	m4 = t0;
	m5 = t1;

	m4 = _mm_xor_si128(m4, m0);
	m5 = _mm_xor_si128(m5, m1);
	m4 = _mm_xor_si128(m4, m2);
	m5 = _mm_xor_si128(m5, m3);

	m1 = XOR(m1, m4);
	m2 = XOR(m2, m5);

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
	__m128i ma0, ma1, mb0, mb1, m0, m1, m2, m3, m4, m5, m8, m9, t0, t1, t2, t3;

	ma0 = _mm_load_si128((__m128i *)a);
	mb0 = _mm_load_si128((__m128i *)b);

	MUL(ma0, mb0);
	m0 = t0;
	m1 = t1;

	ma1 = _mm_load_si128((__m128i *)a + 1);
	mb1 = _mm_load_si128((__m128i *)b + 1);
	MUL(ma1, mb1);
	m2 = t0;
	m3 = t1;

	ma0 = XOR(ma0, ma1);
	mb0 = XOR(mb0, mb1);

	MUL(ma0, mb0);
	m4 = t0;
	m5 = t1;

	m4 = _mm_xor_si128(m4, m0);
	m5 = _mm_xor_si128(m5, m1);
	m4 = _mm_xor_si128(m4, m2);
	m5 = _mm_xor_si128(m5, m3);

	m1 = XOR(m1, m4);
	m2 = XOR(m2, m5);

	relic_align dig_t _x[2];

	REDUCE();
	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) _x, m1);
	c[2] = _x[0];
	c[3] = _x[1] & 0x07FFFFFFFFFFFFFF;
	return;
}

#endif

void fb_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	__m128i ma, mb, m0, m1, m2, t0, t1, t2;

	ma = _mm_load_si128((__m128i *)a);
	mb = _mm_set_epi32(0, 0, digit >> 32, digit & 0xFFFFFFFF);

	MULDXS(ma, mb);
	m0 = t0;
	m1 = t1;

	ma = _mm_load_si128((__m128i *)a + 1);
	MULDXS(ma, mb);
	m1 = XOR(m1, t0);
	m2 = t1;

	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) c + 1, m1);
	_mm_store_si128((__m128i *) c + 2, m2);
}
