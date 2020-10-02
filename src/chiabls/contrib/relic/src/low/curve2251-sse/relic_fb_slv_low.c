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
 * Implementation of the low-level binary field half-trace.
 *
 * @ingroup fb
 */

#include <tmmintrin.h>
#ifdef __PCLMUL__
#include <smmintrin.h>
#include <wmmintrin.h>
#endif

#include <stdlib.h>

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

void fb_slvn_low(dig_t *c, const dig_t *a) {
	int i;
	dig_t *p, u0, u1, u2, u3;
	void *tab = fb_poly_get_slv();
	__m128i m0, m1, m2, m3, m4, sqrt0, sqrt1, mask0, mask1, mask2, r0, r1, t0, t1, perm;

	perm = _mm_set_epi32(0x0F0D0B09, 0x07050301, 0x0E0C0A08, 0x06040200);
	mask2 = _mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000);
	mask1 = _mm_set_epi32(0xF0F0F0F0, 0xF0F0F0F0, 0xF0F0F0F0, 0xF0F0F0F0);
	mask0 = _mm_set_epi32(0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F);
	sqrt0 = _mm_set_epi32(0x03020302, 0x01000100, 0x03020302, 0x01000100);
	sqrt1 = _mm_set_epi32(0x0c080c08, 0x04000400, 0x0c080c08, 0x04000400);

	t0 = _mm_load_si128((__m128i *)a);
	t1 = _mm_load_si128((__m128i *)(a + 2));
	r0 = r1 = _mm_setzero_si128();

	m0 = _mm_shuffle_epi8(t1, perm);
	m1 = _mm_and_si128(m0, mask0);
	m2 = _mm_and_si128(m0, mask1);
	m2 = _mm_srli_epi64(m2, 4);
	m2 = _mm_shuffle_epi8(sqrt1, m2);
	m1 = _mm_shuffle_epi8(sqrt0, m1);
	m1 = _mm_xor_si128(m1, m2);

	m2 = _mm_slli_si128(m1, 8);
	m1 = _mm_and_si128(m1, mask2);
	m1 = _mm_slli_epi64(m1, 4);
	m1 = _mm_xor_si128(m1, m2);
	t0 = _mm_xor_si128(t0, m1);
	r0 = _mm_xor_si128(r0, m1);

	m0 = _mm_and_si128(t0, mask2);
	m0 = _mm_shuffle_epi8(m0, perm);
	m1 = _mm_and_si128(m0, mask0);
	m2 = _mm_and_si128(m0, mask1);
	m2 = _mm_srli_epi64(m2, 4);
	m2 = _mm_shuffle_epi8(sqrt1, m2);
	m1 = _mm_shuffle_epi8(sqrt0, m1);
	m1 = _mm_xor_si128(m1, m2);

	m2 = _mm_srli_si128(m1, 8);
	m1 = _mm_andnot_si128(mask2, m1);
	m2 = _mm_slli_epi64(m2, 4);
	m1 = _mm_xor_si128(m1, m2);
	t0 = _mm_xor_si128(t0, m1);
	r0 = _mm_xor_si128(r0, m1);

	m1 = _mm_srli_si128(t0, 4);
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0xFFFFFFFF));
	m0 = _mm_shuffle_epi8(m1, perm);
	m1 = _mm_and_si128(m0, mask0);
	m2 = _mm_and_si128(m0, mask1);
	m2 = _mm_srli_epi64(m2, 4);
	m2 = _mm_shuffle_epi8(sqrt1, m2);
	m1 = _mm_shuffle_epi8(sqrt0, m1);
	m1 = _mm_xor_si128(m1, m2);
	m2 = _mm_slli_si128(m1, 8);
	m1 = _mm_slli_epi64(m1, 4);
	m1 = _mm_xor_si128(m1, m2);
	m1 = _mm_srli_si128(m1, 6);
	t0 = _mm_xor_si128(t0, m1);
	r0 = _mm_xor_si128(r0, m1);

	m1 = _mm_srli_si128(t0, 2);
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0xFFFF));
	m0 = _mm_shuffle_epi8(m1, perm);
	m1 = _mm_and_si128(m0, mask0);
	m2 = _mm_and_si128(m0, mask1);
	m2 = _mm_srli_epi64(m2, 4);
	m2 = _mm_shuffle_epi8(sqrt1, m2);
	m1 = _mm_shuffle_epi8(sqrt0, m1);
	m1 = _mm_xor_si128(m1, m2);
	m2 = _mm_slli_si128(m1, 8);
	m1 = _mm_slli_epi64(m1, 4);
	m1 = _mm_xor_si128(m1, m2);
	m1 = _mm_srli_si128(m1, 7);
	t0 = _mm_xor_si128(t0, m1);
	r0 = _mm_xor_si128(r0, m1);

	m1 = _mm_srli_si128(t0, 1);
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0x55));
	m1 = _mm_or_si128(m1, _mm_srli_epi64(m1, 1));
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0x33));
	m1 = _mm_or_si128(m1, _mm_srli_epi64(m1, 2));
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0x0F));
	m1 = _mm_slli_epi64(m1, 4);
	t0 = _mm_xor_si128(t0, m1);
	r0 = _mm_xor_si128(r0, m1);

	m1 = _mm_srli_epi64(t0, 4);
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0x5));
	m1 = _mm_or_si128(m1, _mm_srli_epi64(m1, 1));
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0x3));
	m1 = _mm_slli_epi64(m1, 2);
	t0 = _mm_xor_si128(t0, m1);
	r0 = _mm_xor_si128(r0, m1);

	m1 = _mm_srli_epi64(t0, 2);
	m1 = _mm_and_si128(m1, _mm_set_epi32(0, 0, 0, 0x1));
	m1 = _mm_slli_epi64(m1, 1);
	t0 = _mm_xor_si128(t0, m1);
	r0 = _mm_xor_si128(r0, m1);

	sqrt0 = _mm_set_epi32(0x03030202, 0x03030202, 0x01010000, 0x01010000);
	sqrt1 = _mm_set_epi32(0x0C0C0808, 0x0C0C0808, 0x04040000, 0x04040000);

	m1 = _mm_and_si128(t0, mask0);
	m2 = _mm_and_si128(t0, mask1);
	m3 = _mm_and_si128(t1, mask0);
	m4 = _mm_and_si128(t1, mask1);
	m2 = _mm_srli_epi64(m2, 4);
	m4 = _mm_srli_epi64(m4, 4);
	m2 = _mm_shuffle_epi8(sqrt1, m2);
	m1 = _mm_shuffle_epi8(sqrt0, m1);
	m4 = _mm_shuffle_epi8(sqrt1, m4);
	m3 = _mm_shuffle_epi8(sqrt0, m3);
	m1 = _mm_or_si128(m1, m2);
	m3 = _mm_or_si128(m3, m4);
#ifndef __PCLMUL__
	align dig_t x[2];
	_mm_store_si128((__m128i *)x, m1);
	u0 = x[0];
	u1 = x[1];
	_mm_store_si128((__m128i *)x, m3);
	u2 = x[0];
	u3 = x[1];
#else
	u0 = _mm_extract_epi64(m1, 0);
	u1 = _mm_extract_epi64(m1, 1);
	u2 = _mm_extract_epi64(m3, 0);
	u3 = _mm_extract_epi64(m3, 1);
#endif

	for (i = 0; i < 8; i++) {
		p = (dig_t *)(tab + (16 * i + (u0 & 0x0F)) * sizeof(fb_st));
		r0 = _mm_xor_si128(r0, *(__m128i *)(p));
		r1 = _mm_xor_si128(r1, *(__m128i *)(p + 2));
		u0 >>= 8;
		p = (dig_t *)(tab + (16 * (i + 8) + (u1 & 0x0F)) * sizeof(fb_st));
		r0 = _mm_xor_si128(r0, *(__m128i *)(p));
		r1 = _mm_xor_si128(r1, *(__m128i *)(p + 2));
		u1 >>= 8;
		p = (dig_t *)(tab + (16 * (i + 16) + (u2 & 0x0F)) * sizeof(fb_st));
		r0 = _mm_xor_si128(r0, *(__m128i *)(p));
		r1 = _mm_xor_si128(r1, *(__m128i *)(p + 2));
		u2 >>= 8;
		p = (dig_t *)(tab + (16 * (i + 24) + (u3 & 0xF)) * sizeof(fb_st));
		r0 = _mm_xor_si128(r0, *(__m128i *)(p));
		r1 = _mm_xor_si128(r1, *(__m128i *)(p + 2));
		u3 >>= 8;
	}

	_mm_store_si128((__m128i *)c, r0);
	_mm_store_si128((__m128i *)(c + 2), r1);
}
