/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2014 RELIC Authors
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
 * Implementation of hashing to a prime elliptic curve.
 *
 * @version $Id$
 * @ingroup ed
 */

#include "relic_core.h"
#include "relic_md.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

void ed_map_ell2_5mod8(ed_t p, fp_t t) {
	bn_t h;
	fp_t tv1, tv2, tv3, tv4, tv5;
	bn_null(h);
	fp_null(tv1);
	fp_null(tv2);
	fp_null(tv3);
	fp_null(tv4);
	fp_null(tv5);

	/* aliases to make code easier to read */
	ctx_t *ctx = core_get();
	dig_t *c_2exp = ctx->ed_map_c[0];
	dig_t *sqrt_M1 = ctx->ed_map_c[1];
	dig_t *sqrt_M486664 = ctx->ed_map_c[2];
	dig_t *c_486662 = ctx->ed_map_c[3];

	RLC_TRY {
		bn_new(h);
		fp_new(tv1);
		fp_new(tv2);
		fp_new(tv3);
		fp_new(tv4);
		fp_new(tv5);

		/* set h = (p - 5) / 8 */
		h->used = RLC_FP_DIGS;
		h->sign = RLC_POS;
		dv_copy(h->dp, fp_prime_get(), RLC_FP_DIGS); /* p */
		bn_sub_dig(h, h, 5);                         /* p - 5 */
		bn_rsh(h, h, 3);                             /* (p - 5) / 8 */

		/* start evaluating map */
		fp_sqr(tv1, t);
		fp_dbl(tv1, tv1);
		fp_add_dig(tv3, tv1, 1);
		fp_sqr(tv2, tv3);
		fp_mul(p->z, tv2, tv3);

		/* compute numerator of g(x1) */
		fp_sqr(tv4, c_486662);
		fp_mul(tv4, tv4, tv1);
		fp_sub(tv4, tv4, tv2);
		fp_mul(tv4, tv4, c_486662);

		/* compute divsrqt */
		fp_sqr(tv3, p->z);
		fp_sqr(tv2, tv3);
		fp_mul(tv3, tv3, p->z);
		fp_mul(tv3, tv3, tv4);
		fp_mul(tv2, tv2, tv3);
		fp_exp(tv2, tv2, h);
		fp_mul(tv2, tv2, tv3);

		/* figure out which sqrt we should keep */
		fp_mul(p->y, tv2, sqrt_M1);
		fp_sqr(p->x, tv2);
		fp_mul(p->x, p->x, p->z);
		{
			const int e1 = fp_cmp(p->x, tv4);
			dv_copy_cond(p->y, tv2, RLC_FP_DIGS, e1 == RLC_EQ);
		} /* e1 goes out of scope */

		/* compute numerator of g(x2) */
		fp_mul(tv3, tv2, t);
		fp_mul(tv3, tv3, c_2exp);
		fp_mul(tv5, tv3, sqrt_M1);

		/* figure out which sqrt we should keep */
		fp_mul(p->x, tv4, tv1);
		fp_sqr(tv2, tv3);
		fp_mul(tv2, tv2, p->z);
		{
			const int e2 = fp_cmp(p->x, tv2);
			dv_copy_cond(tv5, tv3, RLC_FP_DIGS, e2 == RLC_EQ);
		} /* e2 goes out of scope */

		/* figure out whether we wanted y1 or y2 and x1 or x2 */
		fp_sqr(tv2, p->y);
		fp_mul(tv2, tv2, p->z);
		{
			const int e3 = fp_cmp(tv2, tv4);
			fp_set_dig(p->x, 1);
			dv_copy_cond(p->x, tv1, RLC_FP_DIGS, e3 != RLC_EQ);
			fp_mul(p->x, p->x, c_486662);
			fp_neg(p->x, p->x);
			dv_copy_cond(p->y, tv5, RLC_FP_DIGS, e3 != RLC_EQ);

			/* fix sign of y */
			fp_prime_back(h, p->y);
			const int e4 = bn_get_bit(h, 0);
			fp_neg(tv2, p->y);
			dv_copy_cond(p->y, tv2, RLC_FP_DIGS, (e3 == RLC_EQ) ^ (e4 == 1));
		} /* e3 and e4 go out of scope */
		fp_add_dig(p->z, tv1, 1);

		/* convert to an Edwards point */
		/* tmp1 = xnumerator = sqrt_M486664 * x */
		fp_mul(tv1, p->x, sqrt_M486664); /* xn = sqrt(-486664) * x */
		fp_mul(tv2, p->y, p->z);         /* xd = y * z */
		fp_sub(tv3, p->x, p->z);         /* yn = x - z */
		fp_add(tv4, p->x, p->z);         /* yd = x + z */

		fp_mul(p->z, tv2, tv4);
		fp_mul(p->x, tv1, tv4);
		fp_mul(p->y, tv2, tv3);
		{
			/* exceptional case: either denominator == 0 */
			const int e4 = fp_is_zero(p->z);
			fp_set_dig(tv5, 1);
			dv_copy_cond(p->x, p->z, RLC_FP_DIGS, e4); /* set x to 0 */
			dv_copy_cond(p->y, tv5, RLC_FP_DIGS, e4);
			dv_copy_cond(p->z, tv5, RLC_FP_DIGS, e4);
		} /* e4 goes out of scope */

		/* clear denominator / compute extended coordinates if necessary */
#if ED_ADD == EXTND || ED_ADD == PROJC
		p->coord = PROJC;
#if ED_ADD == EXTND
		/* extended coordinates: T * Z == X * Y */
		fp_mul(p->t, p->x, p->y);
		fp_mul(p->x, p->x, p->z);
		fp_mul(p->y, p->y, p->z);
		fp_sqr(p->z, p->z);
#endif /* ED_ADD == EXTND */
#else  /* ED_ADD == BASIC */
		fp_inv(tv1, p->z);
		fp_mul(p->x, p->x, tv1);
		fp_mul(p->y, p->y, tv1);
		fp_set_dig(p->z, 1);
		p->coord = BASIC;
#endif /* ED_ADD */
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT)
	}
	RLC_FINALLY {
		bn_free(h);
		fp_free(tv1);
		fp_free(tv2);
		fp_free(tv3);
		fp_free(tv4);
		fp_free(tv5);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ed_map_dst(ed_t p, const uint8_t *msg, int len, const uint8_t *dst, int dst_len) {
	bn_t k;
	fp_t t;
	ed_t q;
	/* enough space for two field elements plus extra bytes for uniformity */
	const int len_per_elm = (FP_PRIME + ed_param_level() + 7) / 8;
	uint8_t *pseudo_random_bytes = RLC_ALLOCA(uint8_t, 2 * len_per_elm);

	bn_null(k);
	fp_null(t);
	ed_null(q);

	RLC_TRY {
		bn_new(k);
		fp_new(t);
		ed_new(q);

		/* pseudorandom string */
		md_xmd(pseudo_random_bytes, 2 * len_per_elm, msg, len, dst, dst_len);

#define ED_MAP_CONVERT_BYTES(IDX)                                                        \
	do {                                                                                 \
		bn_read_bin(k, pseudo_random_bytes + IDX * len_per_elm, len_per_elm);            \
		fp_prime_conv(t, k);                                                             \
	} while (0)

		/* first map invocation */
		ED_MAP_CONVERT_BYTES(0);
		ed_map_ell2_5mod8(p, t);

		/* second map invocation */
		ED_MAP_CONVERT_BYTES(1);
		ed_map_ell2_5mod8(q, t);

#undef ED_MAP_CONVERT_BYTES

		ed_add(p, p, q);

		/* clear cofactor */
		switch (ed_param_get()) {
			case CURVE_ED25519:
				ed_dbl(p, p);
				ed_dbl(p, p);
				ed_dbl(p, p);
				break;
			default:
				RLC_THROW(ERR_NO_VALID);
				break;
		}

		ed_norm(p, p);
#if ED_ADD == EXTND
		fp_mul(p->t, p->x, p->y);
#endif
		p->coord = BASIC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(k);
		fp_free(t);
		ed_free(q);
		RLC_FREE(pseudo_random_bytes);
	}
}

void ed_map(ed_t p, const uint8_t *msg, int len) {
	ed_map_dst(p, msg, len, (const uint8_t *)"RELIC", 5);
}
