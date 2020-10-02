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
 * Implementation of hashing to a prime elliptic curve.
 *
 * @ingroup ep
 */

#include "relic_core.h"
#include "relic_md.h"

/**
 * Based on the rust implementation of pairings, zkcrypto/pairing.
 * The algorithm is Shallue–van de Woestijne encoding from
 * Section 3 of "Indifferentiable Hashing to Barreto–Naehrig Curves"
 * from Fouque-Tibouchi: <https://www.di.ens.fr/~fouque/pub/latincrypt12.pdf>
 */
void ep_sw_encode(ep_t p, fp_t t) {
	if (fp_is_zero(t)) {
		ep_set_infty(p);
		return;
	}
	fp_t nt; // Negative t
	fp_t w;
	fp_t s_n3;
	fp_t s_n3m1o2;
	fp_t two_inv;
	fp_t x1;
	fp_t x2;
	fp_t x3;
	fp_t rhs;
	fp_t ny;

	fp_new(nt);
	fp_new(w);
	fp_new(s_n3);
	fp_new(s_n3m1o2);
	fp_new(two_inv);
	fp_new(x1);
	fp_new(x2);
	fp_new(x3);
	fp_new(rhs);
	fp_new(ny);

	fp_neg(nt, t);

	uint8_t buf0[FP_BYTES];
	uint8_t buf1[FP_BYTES];
	fp_write_bin(buf0, FP_BYTES, t);
	fp_write_bin(buf1, FP_BYTES, nt);
	int parity = (memcmp(buf0, buf1, FP_BYTES) > 0);

	// w = t^2 + b + 1
	fp_mul(w, t, t);
	fp_add(w, w, ep_curve_get_b());
	fp_add_dig(w, w, 1);

	if (fp_is_zero(w)) {
		// Returns generator
		ep_curve_get_gen(p);
		return;
	}

	// sqrt(-3)
	fp_set_dig(s_n3, 3);
	fp_neg(s_n3, s_n3);
	fp_srt(s_n3, s_n3);

	// (sqrt(-3) - 1) / 2
	fp_copy(s_n3m1o2, s_n3);
	fp_sub_dig(s_n3m1o2, s_n3m1o2, 1);
	fp_set_dig(two_inv, 2);
	fp_inv(two_inv, two_inv);
	fp_mul(s_n3m1o2, s_n3m1o2, two_inv);

	fp_inv(w, w);
	fp_mul(w, w, s_n3);
	fp_mul(w, w, t);

	// x1 = -wt + sqrt(-3)
	fp_neg(x1, w);
	fp_mul(x1, x1, t);
	fp_add(x1, x1, s_n3m1o2);

	// x2 = -x1 - 1
	fp_neg(x2, x1);
	fp_sub_dig(x2, x2, 1);

	// x3 = 1/w^2 + 1
	fp_mul(x3, w, w);
	fp_inv(x3, x3);
	fp_add_dig(x3, x3, 1);

	fp_zero(p->y);
	fp_set_dig(p->z, 1);

	fp_copy(p->x, x1);
	ep_rhs(rhs, p);
	int Xx1 = fp_srt(p->y, rhs) ? 1 : -1;
	fp_copy(p->x, x2);
	ep_rhs(rhs, p);
	int Xx2 = fp_srt(p->y, rhs) ? 1 : -1;
	int index = ((((Xx1 - 1) * Xx2) % 3) + 3) % 3;

	if (index == 0) {
		fp_copy(p->x, x1);
	} else if (index == 1) {
		fp_copy(p->x, x2);
	} else if (index == 2) {
		fp_copy(p->x, x3);
	}
	ep_rhs(rhs, p);
	fp_srt(p->y, rhs);

	p->norm = 1;
	fp_neg(ny, p->y);

	fp_write_bin(buf0, FP_BYTES, p->y);
	fp_write_bin(buf1, FP_BYTES, ny);
	if ((memcmp(buf0, buf1, FP_BYTES) > 0) != parity) {
		ep_neg(p, p);
	}
	fp_free(nt);
	fp_free(w);
	fp_free(s_n3);
	fp_free(s_n3m1o2);
	fp_free(two_inv);
	fp_free(x1);
	fp_free(x2);
	fp_free(x3);
	fp_free(rhs);
	fp_free(ny);
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep_map(ep_t p, const uint8_t *msg, int len) {
	bn_t t0;
	bn_t t1;
	fp_t t0p;
	fp_t t1p;
	ep_t p0;
	ep_t p1;
	bn_t k;

	TRY {
		uint8_t input[MD_LEN + 5];
		md_map(input, msg, len);

		// b"G1_0"
		input[MD_LEN + 0] = 0x47;
		input[MD_LEN + 1] = 0x31;
		input[MD_LEN + 2] = 0x5f;
		input[MD_LEN + 3] = 0x30;

		fp_new(t0p);
		fp_new(t1p);
		bn_new(t0);
		bn_new(t1);
		ep_new(p0);
		ep_new(p1);
		bn_new(k);

		uint8_t t0Bytes[MD_LEN * 2];
		uint8_t t1Bytes[MD_LEN * 2];

		input[MD_LEN + 4] = 0;
		md_map(t0Bytes, input, MD_LEN + 5);
		input[MD_LEN + 4] = 1;
		md_map(t0Bytes + MD_LEN, input, MD_LEN + 5);

		// b"G1_1"
		input[MD_LEN + 3] = 0x31;

		input[MD_LEN + 4] = 0;
		md_map(t1Bytes, input, MD_LEN + 5);
		input[MD_LEN + 4] = 1;
		md_map(t1Bytes + MD_LEN, input, MD_LEN + 5);

		bn_read_bin(t0, t0Bytes, MD_LEN * 2);
		bn_read_bin(t1, t1Bytes, MD_LEN * 2);

		fp_prime_conv(t0p, t0);
		fp_prime_conv(t1p, t1);

		ep_sw_encode(p0, t0p);
		ep_sw_encode(p1, t1p);
		ep_add(p0, p0, p1);

		/* Now, multiply by cofactor to get the correct group. */
		ep_curve_get_cof(k);
		if (bn_bits(k) < BN_DIGIT) {
			ep_mul_dig(p, p0, k->dp[0]);
		} else {
			ep_mul(p, p0, k);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(k);
		ep_free(p0);
		ep_free(p1);
		fp_free(t0p);
		fp_free(t1p);
		bn_free(t0);
		bn_free(t1);
	}
}
