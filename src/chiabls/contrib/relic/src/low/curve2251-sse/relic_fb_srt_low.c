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
 * Implementation of the low-level binary field square root.
 *
 * @ingroup fb
 */

#include <xmmintrin.h>
#include <tmmintrin.h>
#ifdef __PCLMUL__
#include <wmmintrin.h>
#endif

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"
#include "macros.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#define HALF ((int)((FB_BITS / 2)/(FB_DIGIT) + ((FB_BITS / 2) % FB_DIGIT > 0)))

#ifndef __PCLMUL__

void fb_mulh_low(dig_t *c, const dig_t *a) {
	__m128i m0, m1, m2, m3, m8, m9, t0, t1;
	uint8_t ta;
	int j;
	relic_align dig_t x[2];
	dig_t *tab;

#define LSHIFT8(m2,m1,m0)\
	m2=_mm_alignr_epi8(m2,m1,15);\
	m1=_mm_alignr_epi8(m1,m0,15);\
	m0=_mm_slli_si128(m0,1);

#define M(m1,m0,ta)\
	tab = fb_poly_tab_srz(ta);\
	m0=_mm_xor_si128(m0, ((__m128i *)tab)[0]);\
	m1=_mm_xor_si128(m1, ((__m128i *)tab)[1]);\

	// Main computation
	m0 = m1 = m2 = m3 = _mm_setzero_si128();

	for (j = 56; j >= 0; j -= 8) {
		ta = (a[1] >> j) & 0xFF;
		M(m1, m0, ta);
		LSHIFT8(m2, m1, m0);
	}
	for (j = 56; j >= 8; j -= 8) {
		ta = (a[0] >> j) & 0xFF;
		M(m1, m0, ta);
		LSHIFT8(m2, m1, m0);
	}
	ta = a[0] & 0xFF;
	M(m1, m0, ta);

	RED251(m2,m1,m0);													\
	m8 = _mm_srli_si128(m1,8);											\
	m9 = _mm_srli_epi64(m8,59);											\
	m9 = _mm_slli_epi64(m9,59);											\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,59));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,57));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,55));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,52));						\

	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) x, m1);
	c[2] = x[0];
	c[3] = x[1] & 0x07FFFFFFFFFFFFFF;
#undef M
}

#else

void fb_mulh_low(dig_t *c, const dig_t *a) {
	__m128i ma0, mb0, mb1, m0, m1, m2, m4, m8, m9, t0, t1, t2, t3;
	dig_t *b = fb_poly_get_srz();

	ma0 = _mm_load_si128((__m128i *)a);
	mb0 = _mm_load_si128((__m128i *)b);
	mb1 = _mm_load_si128((__m128i *)b + 1);

	MUL(ma0, mb0);
	m0 = t0;
	m1 = t1;

	mb0 = XOR(mb0, mb1);

	MUL(ma0, mb0);
	m4 = _mm_xor_si128(t0, m0);
	m2 = _mm_xor_si128(t1, m1);

	m1 = XOR(m1, m4);

	relic_align dig_t _x[2];

	RED251(m2,m1,m0);													\
	m8 = _mm_srli_si128(m1,8);											\
	m9 = _mm_srli_epi64(m8,59);											\
	m9 = _mm_slli_epi64(m9,59);											\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,59));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,57));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,55));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,52));						\
	_mm_store_si128((__m128i *) c + 0, m0);
	_mm_store_si128((__m128i *) _x, m1);
	c[2] = _x[0];
	c[3] = _x[1] & 0x07FFFFFFFFFFFFFF;
	return;
#undef M
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_srtn_low(dig_t *c, const dig_t *a) {
	__m128i m0, m1, m2, perm, mask0, mask1, sqrt0, sqrt1;
	relic_align dig_t x[2], d0, d1;
	relic_align dig_t t_e[FB_DIGS] = {0}, t_o[FB_DIGS] = {0};
	int i, n;

	//sqrt1 = sqrt0<<2
	sqrt0 = _mm_set_epi32(0x33322322, 0x31302120, 0x13120302, 0x11100100);
	sqrt1 = _mm_set_epi32(0xccc88c88, 0xc4c08480, 0x4c480c08, 0x44400400);
	perm = _mm_set_epi32(0x0F0D0B09, 0x07050301, 0x0E0C0A08, 0x06040200);
	mask1 = _mm_set_epi32(0xF0F0F0F0, 0xF0F0F0F0, 0xF0F0F0F0, 0xF0F0F0F0);
	mask0 = _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F);

	n = 0;
	for (i = 0; i < FB_DIGS; i += 2) {
		m1 = _mm_load_si128((__m128i *) & a[i]);
		m0 = _mm_shuffle_epi8(m1, perm);
		m1 = _mm_and_si128(m0, mask0);
		m2 = _mm_and_si128(m0, mask1);
		m2 = _mm_srli_epi64(m2, 4);
		m2 = _mm_shuffle_epi8(sqrt1, m2);
		m1 = _mm_shuffle_epi8(sqrt0, m1);
		m2 = _mm_or_si128(m1, m2);
		m0 = _mm_and_si128(m2, mask0);
		m1 = _mm_and_si128(m2, mask1);
		_mm_store_si128((__m128i *) x, m0);
		d0 = x[0] | (x[1] << 4);
		_mm_store_si128((__m128i *) x, m1);
		d1 = x[1] | (x[0] >> 4);
		t_e[n] = d0;
		t_o[n] = d1;
		n++;
	}

	fb_mulh_low(c, t_o);
	for (i = 0; i < HALF; i++) {
		c[i] ^= t_e[i];
	}
}
