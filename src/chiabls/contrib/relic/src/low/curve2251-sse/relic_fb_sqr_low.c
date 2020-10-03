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
 * Implementation of the low-level binary field squaring.
 *
 * @ingroup bn
 */

#include <tmmintrin.h>

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"
#include "macros.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#include <xmmintrin.h>

void fb_sqrn_low(dig_t *c, const dig_t *a) {
	__m128i m0, t;
	int j = 0;

	for (int i = 0; i < FB_DIGS; i++) {
		m0 = _mm_set_epi32(0x00000000, a[i] >> 32, 0x00000000, a[i] & 0xFFFFFFFFFFFFFFFF);
		t = _mm_slli_epi64(m0, 16);
		t = _mm_or_si128(t, m0);
		t = _mm_and_si128(t, _mm_set_epi32(0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF));
		m0 = t;
		t = _mm_slli_epi64(m0, 8);
		t = _mm_or_si128(t, m0);
		t = _mm_and_si128(t, _mm_set_epi32(0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF));
		m0 = t;
		t = _mm_slli_epi64(m0, 4);
		t = _mm_or_si128(t, m0);
		t = _mm_and_si128(t, _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F));
		m0 = t;
		t = _mm_slli_epi64(m0, 2);
		t = _mm_or_si128(t, m0);
		t = _mm_and_si128(t, _mm_set_epi32(0x33333333, 0x33333333, 0x33333333, 0x33333333));
		m0 = t;
		t = _mm_slli_epi64(m0, 1);
		t = _mm_or_si128(t, m0);
		t = _mm_and_si128(t, _mm_set_epi32(0x55555555, 0x55555555, 0x55555555, 0x55555555));
		_mm_store_si128((__m128i *)(c + j), t);
		j += 2;
	}
}

void fb_sqrl_low(dig_t *c, const dig_t *a) {
	__m128i m0, m1, m2, m3, m4, m5, m6, mask;
	__m128i t0;

	t0 = _mm_set_epi32(0x55545150, 0x45444140, 0x15141110, 0x05040100);
	mask = _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F);

	m0 = _mm_load_si128((__m128i *)(a));
	m1 = _mm_and_si128(m0, mask);
	m1 = _mm_shuffle_epi8(t0, m1);
	m2 = _mm_srli_epi64(m0, 4);
	m2 = _mm_and_si128(m2, mask);
	m2 = _mm_shuffle_epi8(t0, m2);
	m3 = _mm_unpacklo_epi8(m1, m2);
	m4 = _mm_unpackhi_epi8(m1, m2);

	m0 = _mm_load_si128((__m128i *)(a+2));
	m1 = _mm_and_si128(m0, mask);
	m1 = _mm_shuffle_epi8(t0, m1);
	m2 = _mm_srli_epi64(m0, 4);
	m2 = _mm_and_si128(m2, mask);
	m2 = _mm_shuffle_epi8(t0, m2);
	m5 = _mm_unpacklo_epi8(m1, m2);
	m6 = _mm_unpackhi_epi8(m1, m2);

	_mm_store_si128((__m128i *)(c + 0), m3);
	_mm_store_si128((__m128i *)(c + 2), m4);
	_mm_store_si128((__m128i *)(c + 4), m5);
	_mm_store_si128((__m128i *)(c + 6), m6);
}

#if defined(__INTEL_COMPILER)

void fb_sqrm_low(dig_t *c, const dig_t *a) {
	__m128i t0, t1, m0, m1, m2, m3, m4, m5, m6, m8, m9, mask;
	relic_align dig_t x[2];

	t0 = _mm_set_epi32(0x55545150, 0x45444140, 0x15141110, 0x05040100);
	mask = _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F);

	m0 = _mm_load_si128((__m128i *)(a));
	m1 = _mm_and_si128(m0, mask);
	m1 = _mm_shuffle_epi8(t0, m1);
	m2 = _mm_srli_epi64(m0, 4);
	m2 = _mm_and_si128(m2, mask);
	m2 = _mm_shuffle_epi8(t0, m2);
	m3 = _mm_unpacklo_epi8(m1, m2);
	m4 = _mm_unpackhi_epi8(m1, m2);

	m0 = _mm_load_si128((__m128i *)(a+2));
	m1 = _mm_and_si128(m0, mask);
	m1 = _mm_shuffle_epi8(t0, m1);
	m2 = _mm_srli_epi64(m0, 4);
	m2 = _mm_and_si128(m2, mask);
	m2 = _mm_shuffle_epi8(t0, m2);
	m5 = _mm_unpacklo_epi8(m1, m2);
	m6 = _mm_unpackhi_epi8(m1, m2);

	m0 = m3;
	m1 = m4;
	m2 = m5;
	m3 = m6;

	REDUCE();
	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) x, m1);
	c[2] = x[0];
	c[3] = x[1] & 0x07FFFFFFFFFFFFFF;
}

#else

void fb_sqrm_low(dig_t *c, const dig_t *a) {
	__m128i t0, m0, m1, m2, m3, m4, m5, m6, mask;
	relic_align dig_t t[2*FB_DIGS];

	t0 = _mm_set_epi32(0x55545150, 0x45444140, 0x15141110, 0x05040100);
	mask = _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F);

	m0 = _mm_load_si128((__m128i *)(a));
	m1 = _mm_and_si128(m0, mask);
	m1 = _mm_shuffle_epi8(t0, m1);
	m2 = _mm_srli_epi64(m0, 4);
	m2 = _mm_and_si128(m2, mask);
	m2 = _mm_shuffle_epi8(t0, m2);
	m3 = _mm_unpacklo_epi8(m1, m2);
	m4 = _mm_unpackhi_epi8(m1, m2);

	m0 = _mm_load_si128((__m128i *)(a+2));
	m1 = _mm_and_si128(m0, mask);
	m1 = _mm_shuffle_epi8(t0, m1);
	m2 = _mm_srli_epi64(m0, 4);
	m2 = _mm_and_si128(m2, mask);
	m2 = _mm_shuffle_epi8(t0, m2);
	m5 = _mm_unpacklo_epi8(m1, m2);
	m6 = _mm_unpackhi_epi8(m1, m2);

	m0 = m3;
	m1 = m4;
	m2 = m5;
	m3 = m6;

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

#endif
