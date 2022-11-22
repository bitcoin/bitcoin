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
 * Implementation of the Edwards elliptic curve parameters.
 *
 * @version $Id$
 * @ingroup ed
 */

#include "relic_core.h"

#if FP_PRIME == 255
/**
 * Parameters for the Ed25519 prime elliptic curve.
 */
/** @{ */
#define CURVE_ED25519_A	"7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEC"
#define CURVE_ED25519_D "52036CEE2B6FFE738CC740797779E89800700A4D4141D8AB75EB4DCA135978A3"
#define CURVE_ED25519_X "216936D3CD6E53FEC0A4E231FDD6DC5C692CC7609525A7B2C9562D608F25D51A"
#define CURVE_ED25519_Y	"6666666666666666666666666666666666666666666666666666666666666658"
#define CURVE_ED25519_R "1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED"
#define CURVE_ED25519_H "0000000000000000000000000000000000000000000000000000000000000008"
/** @} */
#endif

/**
 * Assigns a set of ordinary elliptic curve parameters.
 *
 * @param[in] CURVE		- the curve parameters to assign.
 * @param[in] FIELD		- the finite field identifier.
 */
#define ASSIGN_ED(CURVE, FIELD)												\
	fp_param_set(FIELD);													\
	RLC_GET(str, CURVE##_A, sizeof(CURVE##_A));								\
	fp_read_str(core_get()->ed_a, str, strlen(str), 16);					\
	RLC_GET(str, CURVE##_D, sizeof(CURVE##_D));								\
	fp_read_str(core_get()->ed_d, str, strlen(str), 16);					\
	RLC_GET(str, CURVE##_X, sizeof(CURVE##_X));								\
	fp_read_str(g->x, str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_Y, sizeof(CURVE##_Y));								\
	fp_read_str(g->y, str, strlen(str), 16);								\
	fp_set_dig(g->z, 1);													\
	RLC_GET(str, CURVE##_R, sizeof(CURVE##_R));								\
	bn_read_str(r, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_H, sizeof(CURVE##_H));								\
	bn_read_str(h, str, strlen(str), 16);

void ed_param_set(int param) {
	ctx_t *ctx = core_get();
	char str[2 * RLC_FP_BYTES + 2];

	ed_t g;
	bn_t r;
	bn_t h;

	ed_null(g);
	bn_null(r);
	bn_null(h);

	RLC_TRY {
		ed_new(g);
		bn_new(r);
		bn_new(h);

		ctx->ed_id = 0;

		switch (param) {
#if FP_PRIME == 255
			case CURVE_ED25519:
				ASSIGN_ED(CURVE_ED25519, PRIME_25519);
				break;
#endif
			default:
				(void)str;
				RLC_THROW(ERR_NO_VALID);
				break;
		}
		fp_set_dig(g->z, 1);
		fp_neg(g->z, g->z);
		fp_neg(g->z, g->z);
#if ED_ADD == EXTND
		fp_mul(g->t, g->x, g->y);
#endif
		g->coord = BASIC;

		bn_copy(&ctx->ed_h, h);
		bn_copy(&ctx->ed_r, r);
		ed_copy(&ctx->ed_g, g);

#ifdef ED_PRECO
		for (int i = 0; i < RLC_ED_TABLE; i++) {
			ctx->ed_ptr[i] = &(ctx->ed_pre[i]);
		}
		ed_mul_pre((ed_t *)ed_curve_get_tab(), &ctx->ed_g);
#endif
		ctx->ed_id = param;

		/* compute constants for p == 5 mod 8 */
		if (fp_prime_get_2ad() == 2) {
			h->used = RLC_FP_DIGS;
			h->sign = RLC_POS;
			dv_copy(h->dp, fp_prime_get(), RLC_FP_DIGS); /* p */
			bn_add_dig(h, h, 3);                         /* p + 3 */
			bn_rsh(h, h, 3);                             /* (p + 3) / 8 */
			/* compute 2^((p+3)/8) */
			fp_set_dig(ctx->ed_map_c[0], 2);
			fp_exp(ctx->ed_map_c[0], ctx->ed_map_c[0], h);

			/* compute sqrt(-1) */
			fp_set_dig(ctx->ed_map_c[1], 1);
			fp_neg(ctx->ed_map_c[1], ctx->ed_map_c[1]);
			if (!fp_srt(ctx->ed_map_c[1], ctx->ed_map_c[1])) {
				/* should never happen because 2adicity > 1 */
				RLC_THROW(ERR_NO_VALID);
			}

			if (param == CURVE_ED25519) {
				/* compute sqrt(-486664) for mapping curve25519 -> edwards25519 */
				fp_read_str(ctx->ed_map_c[3], "486662", 6, 10);
				fp_add_dig(ctx->ed_map_c[2], ctx->ed_map_c[3], 2);
				fp_neg(ctx->ed_map_c[2], ctx->ed_map_c[2]);
				if (!fp_srt(ctx->ed_map_c[2], ctx->ed_map_c[2])) {
					RLC_THROW(ERR_NO_VALID);
				}
				fp_prime_back(h, ctx->ed_map_c[2]);
				/* make sure sgn0(ctx->ed_map_c[2]) == 0 */
				if (bn_get_bit(h, 0) != 0) {
					fp_neg(ctx->ed_map_c[2], ctx->ed_map_c[2]);
				}
			} else {
				/* don't know how to compute remaining constants for other curves */
				RLC_THROW(ERR_NO_VALID);
			}
		} else {
			/* map impl only supports p = 5 mod 8! */
			RLC_THROW(ERR_NO_VALID);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(r);
		bn_free(h);
		ed_free(g);
	}
}

int ed_param_set_any(void) {
	int r = RLC_OK;
#if FP_PRIME == 255
	ed_param_set(CURVE_ED25519);
#else
	r = RLC_ERR;
#endif
	return r;
}

int ed_param_get(void) {
	return core_get()->ed_id;
}

int ed_param_level(void) {
	switch (ed_param_get()) {
		case CURVE_ED25519:
			return 128;
	}
	return 0;
}

void ed_param_print(void) {
	switch (ed_param_get()) {
		case CURVE_ED25519:
			util_banner("Curve ED25519:", 0);
			break;
	}
}
