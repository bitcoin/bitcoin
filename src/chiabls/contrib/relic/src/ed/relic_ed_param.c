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
 * Implementation of the prime elliptic curve utilities.
 *
 * @version $Id$
 * @ingroup ed
 */

#include "relic_core.h"

#if FP_PRIME == 255
/**
 * Parameters for the Curve25519 prime elliptic curve.
 */
/** @{ */
#define CURVE_ED25519_A	"7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEC"
#define CURVE_ED25519_D "52036CEE2B6FFE738CC740797779E89800700A4D4141D8AB75EB4DCA135978A3"
#define CURVE_ED25519_Y	"6666666666666666666666666666666666666666666666666666666666666658"
#define CURVE_ED25519_X "216936D3CD6E53FEC0A4E231FDD6DC5C692CC7609525A7B2C9562D608F25D51A"
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
	FETCH(str, CURVE##_A, sizeof(CURVE##_A));								\
	fp_read_str(core_get()->ed_a, str, strlen(str), 16);					\
	FETCH(str, CURVE##_D, sizeof(CURVE##_D));								\
	fp_read_str(core_get()->ed_d, str, strlen(str), 16);					\
	FETCH(str, CURVE##_Y, sizeof(CURVE##_Y));								\
	fp_read_str(g->y, str, strlen(str), 16);								\
	FETCH(str, CURVE##_X, sizeof(CURVE##_X));								\
	fp_read_str(g->x, str, strlen(str), 16);								\
	fp_set_dig(g->z, 1);													\
	FETCH(str, CURVE##_R, sizeof(CURVE##_R));								\
	bn_read_str(r, str, strlen(str), 16);									\
	FETCH(str, CURVE##_H, sizeof(CURVE##_H));								\
	bn_read_str(h, str, strlen(str), 16);

void ed_param_set(int param) {
	ctx_t *ctx = core_get();
	char str[2 * FP_BYTES + 2];

	ed_t g;
	bn_t r;
	bn_t h;

	ed_null(g);
	bn_null(r);
	bn_null(h);

	TRY {
		ed_new(g);
		bn_new(r);
		bn_new(h);

		core_get()->ed_id = 0;

		switch (param) {
#if FP_PRIME == 255
			case CURVE_ED25519:
				ASSIGN_ED(CURVE_ED25519, PRIME_25519);
				break;
#endif
			default:
				(void)str;
				THROW(ERR_NO_VALID);
				break;
		}

		bn_copy(&core_get()->ed_h, h);
		bn_copy(&core_get()->ed_r, r);

#if ED_ADD == PROJC
		ed_copy(&core_get()->ed_g, g);
#elif ED_ADD == EXTND
		ed_projc_to_extnd(&core_get()->ed_g, g->x, g->y, g->z);
#endif

#ifdef ED_PRECO
		for (int i = 0; i < RELIC_ED_TABLE; i++) {
			ctx->ed_ptr[i] = &(ctx->ed_pre[i]);
		}
#endif
#if ALLOC == STATIC
		fp_new(ctx->ed_g.x);
		fp_new(ctx->ed_g.y);
		fp_new(ctx->ed_g.z);
#if ED_ADD == EXTND
		fp_new(ctx->ed_g.t);
#endif
#ifdef ED_PRECO
		for (int i = 0; i < RELIC_ED_TABLE; i++) {
			fp_new(ctx->ed_pre[i].x);
			fp_new(ctx->ed_pre[i].y);
			fp_new(ctx->ed_pre[i].z);
#if ED_ADD == EXTND
			fp_new(ctx->ed_pre[i].t);
#endif
		}
#endif
#endif

#if defined(ED_PRECO)
		ed_mul_pre((ed_t *) ed_curve_get_tab(), &ctx->ed_g);
#endif
		ctx->ed_id = param;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(r);
		bn_free(h);
		ed_free(g);
	}
}

int ed_param_set_any(void) {
	int r = STS_OK;
#if FP_PRIME == 255
	ed_param_set(CURVE_ED25519);
#else
	r = STS_ERR;
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
