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
 * Implementation of hashing to a prime elliptic curve over a quadratic
 * extension.
 *
 * @ingroup epx
 */

#include "relic_core.h"
#include "relic_md.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Based on the rust implementation of pairings, zkcrypto/pairing.
 * The algorithm is Shallue–van de Woestijne encoding from
 * Section 3 of "Indifferentiable Hashing to Barreto–Naehrig Curves"
 * from Fouque-Tibouchi: <https://www.di.ens.fr/~fouque/pub/latincrypt12.pdf>
 */
void ep2_sw_encode(ep2_t p, fp2_t t) {
	if (fp2_is_zero(t)) {
		// Maps t=0 to the point at infinity.
		ep2_set_infty(p);
		return;
	}
	fp2_t nt; // Negative t
	fp2_t w;
	fp2_t b;
	bn_t s_n3;
	bn_t s_n3m1o2;
	fp2_t x1;
	fp2_t x2;
	fp2_t x3;
	fp2_t rhs;

	fp2_new(nt);
	fp2_new(w);
	fp2_new(b);
	bn_new(s_n3);
	bn_new(s_n3m1o2);
	fp2_new(x1);
	fp2_new(x2);
	fp2_new(x3);
	fp2_new(rhs);

	// nt = -t
	fp2_neg(nt, t);

	uint8_t buf0[FP_BYTES];  // t[1]
	uint8_t buf1[FP_BYTES];  // -t[1]
	fp_write_bin(buf0, FP_BYTES, t[1]);
	fp_write_bin(buf1, FP_BYTES, nt[1]);

	// Compare bytes since fp_cmp compares montgomery representation
	int parity = memcmp(buf0, buf1, FP_BYTES) > 0;

	// w = t^2 + b + 1
	fp2_mul(w, t, t);
	ep2_curve_get_b(b);
	fp2_add(w, w, b);
	fp_add_dig(w[0], w[0], 1);

	if (fp2_is_zero(w)) {
		ep2_curve_get_gen(p);
		if (parity) {
			ep2_neg(p, p);
		}
		fp2_free(nt);
		fp2_free(w);
		fp2_free(b);
		bn_free(s_n3);
		bn_free(s_n3m1o2);
		fp2_free(x1);
		fp2_free(x2);
		fp2_free(x3);
		fp2_free(rhs);
		return;
	}

	// sqrt(-3)
	ep2_curve_get_s3(s_n3);
	fp2_t s_n3p;
	fp2_null(s_n3p);
	fp2_new(s_n3p);
	fp2_t s_n3m1o2p;
	fp2_null(s_n3m1o2p);
	fp2_new(s_n3m1o2p);

	fp2_zero(s_n3p);
	fp2_zero(s_n3m1o2p);
	fp_prime_conv(s_n3p[0], s_n3);

	// (sqrt(-3) - 1) / 2
	ep2_curve_get_s32(s_n3m1o2);
	fp_prime_conv(s_n3m1o2p[0], s_n3m1o2);

	fp2_inv(w, w);
	fp2_mul(w, w, s_n3p);
	fp2_mul(w, w, t);

	// x1 = -wt + sqrt(-3)
	fp2_neg(x1, w);
	fp2_mul(x1, x1, t);
	fp2_add(x1, x1, s_n3m1o2p);

	// x2 = -x1 - 1
	fp2_neg(x2, x1);
	fp_sub_dig(x2[0], x2[0], 1);

	// x3 = 1/w^2 + 1
	fp2_mul(x3, w, w);
	fp2_inv(x3, x3);
	fp_add_dig(x3[0], x3[0], 1);

	fp2_zero(p->y);
	fp2_set_dig(p->z, 1);

	// if x1 has no y, try x2. if x2 has no y, try x3
	fp2_copy(p->x, x1);

	ep2_rhs(rhs, p);
	int Xx1 = fp2_srt(p->y, rhs) ? 1 : -1;
	fp2_copy(p->x, x2);
	ep2_rhs(rhs, p);
	int Xx2 = fp2_srt(p->y, rhs) ? 1 : -1;

	// This formula computes which index to use, in constant time
	// without conditional branches. It's taken from the paper. 3 is
	// added, because the % operator in c can be negative.
	int index = ((((Xx1 - 1) * Xx2) % 3) + 3) % 3;

	if (index == 0) {
		fp2_copy(p->x, x1);
		ep2_rhs(rhs, p);
		fp2_srt(p->y, rhs);
	} else if (index == 1) {
		fp2_copy(p->x, x2);
		ep2_rhs(rhs, p);
		fp2_srt(p->y, rhs);
	} else if (index == 2) {
		fp2_copy(p->x, x3);
		ep2_rhs(rhs, p);
		fp2_srt(p->y, rhs);
	}

	p->norm = 1;

	// Check for lexicografically greater than the negation
	fp2_t ny;
	fp2_null(ny);
	fp2_new(ny);
	fp2_neg(ny, p->y);
	fp_write_bin(buf0, FP_BYTES, p->y[1]);
	fp_write_bin(buf1, FP_BYTES, ny[1]);

	// Compare bytes since fp_cmp compares montgomery representation
	if ((memcmp(buf0, buf1, FP_BYTES) > 0) != parity) {
		ep2_neg(p, p);
	}

	fp2_free(nt);
	fp2_free(w);
	fp2_free(b);
	bn_free(s_n3);
	bn_free(s_n3m1o2);
	fp2_free(x1);
	fp2_free(x2);
	fp2_free(x3);
	fp2_free(rhs);

	fp2_free(s_n3p);
	fp2_free(s_n3m1o2p);
	fp2_free(ny);
}


/**
 * Multiplies a point by the cofactor in a Barreto-Naehrig curve.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_cof_bn(ep2_t r, ep2_t p) {
	bn_t x;
	ep2_t t0, t1, t2;

	ep2_null(t0);
	ep2_null(t1);
	ep2_null(t2);
	bn_null(x);

	TRY {
		ep2_new(t0);
		ep2_new(t1);
		ep2_new(t2);
		bn_new(x);

		fp_param_get_var(x);

		/* Compute t0 = xP. */
		ep2_mul_basic(t0, p, x);

		/* Compute t1 = \psi(3xP). */
		ep2_dbl(t1, t0);
		ep2_add(t1, t1, t0);
		ep2_norm(t1, t1);
		ep2_frb(t1, t1, 1);

		/* Compute t2 = \psi^3(P) + t0 + t1 + \psi^2(xP). */
		ep2_frb(t2, p, 2);
		ep2_frb(t2, t2, 1);
		ep2_add(t2, t2, t0);
		ep2_add(t2, t2, t1);
		ep2_frb(t1, t0, 2);
		ep2_add(t2, t2, t1);

		ep2_norm(r, t2);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(t0);
		ep2_free(t1);
		ep2_free(t2);
		bn_free(x);
	}
}

/**
 * Multiplies a point by the cofactor in a Barreto-Lynn-Soctt.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 */
void ep2_mul_cof_b12(ep2_t r, ep2_t p) {
	bn_t x;
	ep2_t t0, t1, t2, t3;

	ep2_null(t0);
	ep2_null(t1);
	ep2_null(t2);
	ep2_null(t3);
	bn_null(x);

	TRY {
		ep2_new(t0);
		ep2_new(t1);
		ep2_new(t2);
		ep2_new(t3);
		bn_new(x);

		fp_param_get_var(x);

		/* Compute t0 = xP. */
		ep2_mul_basic(t0, p, x);
		/* Compute t1 = [x^2]P. */
		ep2_mul_basic(t1, t0, x);

		/* t2 = (x^2 - x - 1)P = x^2P - x*P - P. */
		ep2_sub(t2, t1, t0);
		ep2_sub(t2, t2, p);
		/* t3 = \psi(x - 1)P. */
		ep2_sub(t3, t0, p);
		ep2_norm(t3, t3);
		ep2_frb(t3, t3, 1);
		ep2_add(t2, t2, t3);
		/* t3 = \psi^2(2P). */
		ep2_dbl(t3, p);
		ep2_norm(t3, t3);
		ep2_frb(t3, t3, 2);
		ep2_add(t2, t2, t3);
		ep2_norm(r, t2);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(t0);
		ep2_free(t1);
		ep2_free(t2);
		ep2_free(t3);
		bn_free(x);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep2_map(ep2_t p, const uint8_t *msg, int len, int performHash) {
	bn_t t00;
	bn_t t01;
	bn_t t10;
	bn_t t11;
	fp_t t00p;
	fp_t t01p;
	fp_t t10p;
	fp_t t11p;
	fp2_t t0p;
	fp2_t t1p;
	ep2_t p0;
	ep2_t p1;

	bn_null(t00);
	bn_null(t01);
	bn_null(t10);
	bn_null(t11);
	fp_null(t00p);
	fp_null(t01p);
	fp_null(t10p);
	fp_null(t11p);
	fp2_null(t0p);
	fp2_null(t1p);
	ep2_null(p1);
	ep2_null(p0);

	TRY {
		uint8_t input[MD_LEN + 8];
		if (performHash) {
			md_map(input, msg, len);
		} else {
			if (len != MD_LEN) {
				THROW(ERR_CAUGHT);
			}
			memcpy(input, msg, len);
		}
		// b"G2_0_c0"
		input[MD_LEN + 0] = 0x47; // G
		input[MD_LEN + 1] = 0x32; // 2
		input[MD_LEN + 2] = 0x5f; // _
		input[MD_LEN + 3] = 0x30; // 0
		input[MD_LEN + 4] = 0x5f; // _
		input[MD_LEN + 5] = 0x63; // c
		input[MD_LEN + 6] = 0x30; // 0

		bn_new(t00);
		bn_new(t01);
		bn_new(t10);
		bn_new(t11);
		fp_new(t00p);
		fp_new(t01p);
		fp_new(t10p);
		fp_new(t11p);
		fp2_new(t0p);
		fp2_new(t1p);
        ep2_new(p1);
		ep2_new(p0);

		uint8_t t00Bytes[MD_LEN * 2];
		uint8_t t01Bytes[MD_LEN * 2];
		uint8_t t10Bytes[MD_LEN * 2];
		uint8_t t11Bytes[MD_LEN * 2];

		// b"G2_0_c0"
		input[MD_LEN + 7] = 0x00; // 0
		md_map(t00Bytes, input, MD_LEN + 8);
		input[MD_LEN + 7] = 1;    // 1
		md_map(t00Bytes + MD_LEN, input, MD_LEN + 8);

		// b"G2_0_c1"
		input[MD_LEN + 6] = 0x31; // b"1"
		input[MD_LEN + 7] = 0;    // 0

		md_map(t01Bytes, input, MD_LEN + 8);
		input[MD_LEN + 7] = 1;    // 1
		md_map(t01Bytes + MD_LEN, input, MD_LEN + 8);

		// b"G2_1_c0"
		input[MD_LEN + 3] = 0x31;  // b"1"
		input[MD_LEN + 6] = 0x30;  // b"0"
		input[MD_LEN + 7] = 0;     // 0
		md_map(t10Bytes, input, MD_LEN + 8);
		input[MD_LEN + 7] = 1;     // 1
		md_map(t10Bytes + MD_LEN, input, MD_LEN + 8);

		// b"G2_1_c1"
		input[MD_LEN + 6] = 0x31;     // b"1"
		input[MD_LEN + 7] = 0;     // 0
		md_map(t11Bytes, input, MD_LEN + 8);
		input[MD_LEN + 7] = 1;     // 1
		md_map(t11Bytes + MD_LEN, input, MD_LEN + 8);

		bn_read_bin(t00, t00Bytes, MD_LEN * 2);
		bn_read_bin(t01, t01Bytes, MD_LEN * 2);
		bn_read_bin(t10, t10Bytes, MD_LEN * 2);
		bn_read_bin(t11, t11Bytes, MD_LEN * 2);

		fp_prime_conv(t00p, t00);
		fp_prime_conv(t01p, t01);
		fp_prime_conv(t10p, t10);
		fp_prime_conv(t11p, t11);

		fp_copy(t0p[0], t00p);
		fp_copy(t0p[1], t01p);
		fp_copy(t1p[0], t10p);
		fp_copy(t1p[1], t11p);

		ep2_sw_encode(p0, t0p);
		ep2_sw_encode(p1, t1p);
		ep2_add(p0, p0, p1);

		/* Now, multiply by cofactor to get the correct group. */
		switch (ep_param_get()) {
			case BN_P158:
			case BN_P254:
			case BN_P256:
			case BN_P382:
			case BN_P638:
				ep2_mul_cof_bn(p, p0);
				break;
			case B12_P381:
			case B12_P455:
			case B12_P638:
				ep2_mul_cof_b12(p, p0);
				break;
			default: {
				bn_t x;
				bn_new(x);
				ep2_curve_get_cof(x);
				if (bn_bits(x) < BN_DIGIT) {
					ep2_mul_dig(p, p0, x->dp[0]);
					if (bn_sign(x) == BN_NEG) {
						ep2_neg(p, p);
					}
				} else {
					ep2_mul(p, p0, x);
				}
				bn_free(x);
				break;
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(t0p);
		fp2_free(t1p);
		fp_free(t00p);
		fp_free(t01p);
		fp_free(t10p);
		fp_free(t11p);
		bn_free(t00);
		bn_free(t01);
		bn_free(t10);
		bn_free(t11);
		ep2_free(p0);
		ep2_free(p1);
	}
}
