/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2021 RELIC Authors
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
 * Implementation of configuration of prime elliptic curves over quartic
 * extensions.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/* See ep/relic_ep_param.c for discussion of MAP_U parameters. */

#if defined(EP_ENDOM) && FP_PRIME == 509
/** @{ */
#define B24_P509_A0		"0"
#define B24_P509_A1		"0"
#define B24_P509_A2		"0"
#define B24_P509_A3		"0"
#define B24_P509_B0		"0"
#define B24_P509_B1		"0"
#define B24_P509_B2		"AAAAB7FFF9CE54DFE76F95A7CE0767B65C56424AE8C3F4619750081F008485DB13742DFBE0C507867E5AE3038DD69E97731DE83B746C980509E88C6DC5FE956"
#define B24_P509_B3		"AAAAB7FFF9CE54DFE76F95A7CE0767B65C56424AE8C3F4619750081F008485DB13742DFBE0C507867E5AE3038DD69E97731DE83B746C980509E88C6DC5FE955"
#define B24_P509_X0		"4D5AF70D2E605D1691DE7667FF1096AF4537749FD200277E1BC502847F63F4BC2F000FC81571F6E282FD46B96045CD611159ED554AFA95B2B1800A74F6A97C0"
#define B24_P509_X1		"EA0D1E6A2105587ED20E9CA255A777AC78D0A24AEB118B1CE4D1F213BE42F7FFD3F3F5F60F06F902FEE405DF84143D533006D7383C25A7F7C26656440A80A0B"
#define B24_P509_X2		"10E955FAFCA0C3C16D9DA9754859EAF918518C1A3FD0D14F427302CDE750224AD9E337CA12D3824B9E5E1668F94F56A4C2D935C6841C65FF4CA89E62C6A88D34"
#define B24_P509_X3		"A9A981033A3844468D815A18B921C9F7C2B152372F240EAC4848A942FEAA4019B086104AE4F86C929F5B9064B4FE917A327279CEDCE02962FD3E971F9D00CCA"
#define B24_P509_Y0		"C45C4179589584F8D1146550C117E3B452B7789170B7E7C0BEB1E417B5E32845CC1810760585AE0FE07762D94774A311932C100276C7A2EA4304EF7FBAA89F"
#define B24_P509_Y1		"20315B1CE6AEE44F7ADCAE2F0B178DF7574F91380DB5E4A27281D02CC47A24618F995ACC29E611D7BBCE63E8A2CBD783256A1799FFA2A5D061D6872962CBAD"
#define B24_P509_Y2		"20DEE7CA8DF5A616A014D78C0BCC69491116C715DDF5416C52B8A1833F8E4974255FBCCC5C6288DCA9B7CB2F4BB58525AB13D2225590A4A69955A859D36BF67"
#define B24_P509_Y3		"5CD7D6B0C7890DC487A34BE4976767DE0C20BEB43A0EE741A5ED21021EBE5BDC42281008E19C44497E13A38165A36019235BDF7A48E76B6BA79D527024D227C"
#define B24_P509_R		"100000FFFF870FF91CE195DB5B6F3EBD1E08C94C9E193B724ED58B907FF7C311A80D7CABC647746AE3ECB627C943998457FE001"
#define B24_P509_H		"32916E9E0188E2252DA44F42F6DC0E90A66E8C7AFA49D50688A07F362BA18A01F6D9317009D55DAF8CFA9159E35E2736DA6417B31C8550DFC6CD766340D92AB85D629676E78E12D5E76AB9FAC536661EEA6615242264E5F6B46EBA0F95191CD226B0CB144CEC686A846DE323CBE0244A3B6E5FFE49BD01599F13AD869FF3DA2E5551FD9C2D885EF8FB95EB7FFD5460EC84FA36A569BCB5BBBC5A21B025CECEEAD08540C0ACCB87D9136ECB9C2CEDC2D465831D76AC3551EE87BCA06B751C18699A1424FF71E791EB953FA79"
/** @} */
#endif

/**
 * Assigns a set of ordinary elliptic curve parameters.
 *
 * @param[in] CURVE		- the curve parameters to assign.
 */
#define ASSIGN(CURVE)														\
	RLC_GET(str, CURVE##_A0, sizeof(CURVE##_A0));							\
	fp_read_str(a[0][0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_A1, sizeof(CURVE##_A1));							\
	fp_read_str(a[0][1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_A2, sizeof(CURVE##_A2));							\
	fp_read_str(a[1][0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_A3, sizeof(CURVE##_A3));							\
	fp_read_str(a[1][1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B0, sizeof(CURVE##_B0));							\
	fp_read_str(b[0][0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B1, sizeof(CURVE##_B1));							\
	fp_read_str(b[0][1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B2, sizeof(CURVE##_B2));							\
	fp_read_str(b[1][0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B3, sizeof(CURVE##_B3));							\
	fp_read_str(b[1][1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_X0, sizeof(CURVE##_X0));							\
	fp_read_str(g->x[0][0], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_X1, sizeof(CURVE##_X1));							\
	fp_read_str(g->x[0][1], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_X2, sizeof(CURVE##_X2));							\
	fp_read_str(g->x[1][0], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_X3, sizeof(CURVE##_X3));							\
	fp_read_str(g->x[1][1], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_Y0, sizeof(CURVE##_Y0));							\
	fp_read_str(g->y[0][0], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_Y1, sizeof(CURVE##_Y1));							\
	fp_read_str(g->y[0][1], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_Y2, sizeof(CURVE##_Y2));							\
	fp_read_str(g->y[1][0], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_Y3, sizeof(CURVE##_Y3));							\
	fp_read_str(g->y[1][1], str, strlen(str), 16);							\
	RLC_GET(str, CURVE##_R, sizeof(CURVE##_R));								\
	bn_read_str(r, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_H, sizeof(CURVE##_H));								\
	bn_read_str(h, str, strlen(str), 16);									\

/**
 * Detects an optimization based on the curve coefficients.
 */
static void detect_opt(int *opt, fp4_t a) {
	fp4_t t;
	fp4_null(t);

	RLC_TRY {
		fp4_new(t);
		fp4_set_dig(t, 3);
		fp4_neg(t, t);

		if (fp4_cmp(a, t) == RLC_EQ) {
			*opt = RLC_MIN3;
		} else if (fp4_is_zero(a)) {
			*opt = RLC_ZERO;
		} else if (fp4_cmp_dig(a, 1) == RLC_EQ) {
			*opt = RLC_ONE;
		} else if (fp4_cmp_dig(a, 2) == RLC_EQ) {
			*opt = RLC_TWO;
		} else if ((fp_bits(a[0][0]) <= RLC_DIG) && fp_is_zero(a[0][1]) &&
				fp2_is_zero(a[1])) {
			*opt = RLC_TINY;
		} else {
			*opt = RLC_HUGE;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp4_free(t);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep4_curve_init(void) {
	ctx_t *ctx = core_get();

#ifdef EP_PRECO
	for (int i = 0; i < RLC_EP_TABLE; i++) {
		ctx->ep4_ptr[i] = &(ctx->ep4_pre[i]);
	}
#endif

#if ALLOC == DYNAMIC
	ep4_new(ctx->ep4_g);
	fp4_new(ctx->ep4_a);
	fp4_new(ctx->ep4_b);
#endif

#ifdef EP_PRECO
#if ALLOC == DYNAMIC
	for (int i = 0; i < RLC_EP_TABLE; i++) {
		fp4_new(ctx->ep4_pre[i].x);
		fp4_new(ctx->ep4_pre[i].y);
		fp4_new(ctx->ep4_pre[i].z);
	}
#endif
#endif
	ep4_set_infty(ctx->ep4_g);
	bn_make(&(ctx->ep4_r), RLC_FP_DIGS);
	bn_make(&(ctx->ep4_h), RLC_FP_DIGS);
}

void ep4_curve_clean(void) {
	ctx_t *ctx = core_get();
	if (ctx != NULL) {
#ifdef EP_PRECO
		for (int i = 0; i < RLC_EP_TABLE; i++) {
			fp4_free(ctx->ep4_pre[i].x);
			fp4_free(ctx->ep4_pre[i].y);
			fp4_free(ctx->ep4_pre[i].z);
		}
#endif
		bn_clean(&(ctx->ep4_r));
		bn_clean(&(ctx->ep4_h));
		ep4_free(ctx->ep4_g);
		fp4_free(ctx->ep4_a);
		fp4_free(ctx->ep4_b);
	}
}

int ep4_curve_opt_a(void) {
	return core_get()->ep4_opt_a;
}

int ep4_curve_opt_b(void) {
	return core_get()->ep4_opt_b;
}

int ep4_curve_is_twist(void) {
	return core_get()->ep4_is_twist;
}

void ep4_curve_get_gen(ep4_t g) {
	ep4_copy(g, core_get()->ep4_g);
}

void ep4_curve_get_a(fp4_t a) {
	fp4_copy(a, core_get()->ep4_a);
}

void ep4_curve_get_b(fp4_t b) {
	fp4_copy(b, core_get()->ep4_b);
}

void ep4_curve_get_vs(bn_t *v) {
	bn_t x, t;

	bn_null(x);
	bn_null(t);

	RLC_TRY {
		bn_new(x);
		bn_new(t);

		fp_prime_get_par(x);
		bn_copy(v[1], x);
		bn_copy(v[2], x);
		bn_copy(v[3], x);

		/* t = 2x^2. */
		bn_sqr(t, x);
		bn_dbl(t, t);

		/* v0 = 2x^2 + 3x + 1. */
		bn_mul_dig(v[0], x, 3);
		bn_add_dig(v[0], v[0], 1);
		bn_add(v[0], v[0], t);

		/* v3 = -(2x^2 + x). */
		bn_add(v[3], v[3], t);
		bn_neg(v[3], v[3]);

		/* v1 = 12x^3 + 8x^2 + x, v2 = 6x^3 + 4x^2 + x. */
		bn_dbl(t, t);
		bn_add(v[2], v[2], t);
		bn_dbl(t, t);
		bn_add(v[1], v[1], t);
		bn_rsh(t, t, 2);
		bn_mul(t, t, x);
		bn_mul_dig(t, t, 3);
		bn_add(v[2], v[2], t);
		bn_dbl(t, t);
		bn_add(v[1], v[1], t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(x);
		bn_free(t);
	}
}

void ep4_curve_get_ord(bn_t n) {
	ctx_t *ctx = core_get();
	if (ctx->ep4_is_twist) {
		ep_curve_get_ord(n);
	} else {
		bn_copy(n, &(ctx->ep4_r));
	}
}

void ep4_curve_get_cof(bn_t h) {
	bn_copy(h, &(core_get()->ep4_h));
}

#if defined(EP_PRECO)

ep4_t *ep4_curve_get_tab(void) {
#if ALLOC == AUTO
	return (ep4_t *)*(core_get()->ep4_ptr);
#else
	return core_get()->ep4_ptr;
#endif
}

#endif

void ep4_curve_set_twist(int type) {
	char str[8 * RLC_FP_BYTES + 1];
	ctx_t *ctx = core_get();
	ep4_t g;
	fp4_t a, b;
	bn_t r, h;

	ep4_null(g);
	fp4_null(a);
	fp4_null(b);
	bn_null(r);
	bn_null(h);

	ctx->ep4_is_twist = 0;
	if (type == RLC_EP_MTYPE || type == RLC_EP_DTYPE) {
		ctx->ep4_is_twist = type;
	} else {
		return;
	}

	RLC_TRY {
		ep4_new(g);
		fp4_new(a);
		fp4_new(b);
		bn_new(r);
		bn_new(h);

		switch (ep_param_get()) {
#if FP_PRIME == 509
			case B24_P509:
				ASSIGN(B24_P509);
				break;
#endif
			default:
				(void)str;
				RLC_THROW(ERR_NO_VALID);
				break;
		}

		fp4_zero(g->z);
		fp4_set_dig(g->z, 1);
		g->coord = BASIC;

		ep4_copy(ctx->ep4_g, g);
		fp4_copy(ctx->ep4_a, a);
		fp4_copy(ctx->ep4_b, b);

		detect_opt(&(ctx->ep4_opt_a), ctx->ep4_a);
		detect_opt(&(ctx->ep4_opt_b), ctx->ep4_b);

		bn_copy(&(ctx->ep4_r), r);
		bn_copy(&(ctx->ep4_h), h);

#if defined(WITH_PC)
		/* Compute pairing generator. */
		pc_core_calc();
#endif

#if defined(EP_PRECO)
		ep4_mul_pre((ep4_t *)ep4_curve_get_tab(), ctx->ep4_g);
#endif
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep4_free(g);
		fp4_free(a);
		fp4_free(b);
		bn_free(r);
		bn_free(h);
	}
}

void ep4_curve_set(fp4_t a, fp4_t b, ep4_t g, bn_t r, bn_t h) {
	ctx_t *ctx = core_get();
	ctx->ep4_is_twist = 0;

	fp4_copy(ctx->ep4_a, a);
	fp4_copy(ctx->ep4_b, b);

	ep4_norm(ctx->ep4_g, g);
	bn_copy(&(ctx->ep4_r), r);
	bn_copy(&(ctx->ep4_h), h);

#if defined(EP_PRECO)
	ep4_mul_pre((ep4_t *)ep4_curve_get_tab(), ctx->ep4_g);
#endif
}
