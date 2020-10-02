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
 * Implementation of the prime field prime manipulation functions.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fpx.h"
#include "relic_bn_low.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Assigns the prime field modulus.
 *
 * @param[in] p			- the new prime field modulus.
 */
static void fp_prime_set(const bn_t p) {
	dv_t s, q;
	bn_t t;
	ctx_t *ctx = core_get();

	if (p->used != FP_DIGS) {
		THROW(ERR_NO_VALID);
	}

	dv_null(s);
	bn_null(t);
	dv_null(q);

	TRY {
		dv_new(s);
		bn_new(t);
		dv_new(q);

		bn_copy(&(ctx->prime), p);

		bn_mod_dig(&(ctx->mod8), &(ctx->prime), 8);

		switch (ctx->mod8) {
			case 3:
			case 7:
				ctx->qnr = -1;
				/* The current code for extensions of Fp^3 relies on qnr being
				 * also a cubic non-residue. */
				ctx->cnr = 0;
				break;
			case 1:
			case 5:
				ctx->qnr = ctx->cnr = -2;
				break;
			default:
				ctx->qnr = ctx->cnr = 0;
				THROW(ERR_NO_VALID);
				break;
		}
#ifdef FP_QNRES
		if (ctx->mod8 != 3) {
			THROW(ERR_NO_VALID);
		}
#endif

#if FP_RDC == MONTY || !defined(STRIP)
		bn_mod_pre_monty(t, &(ctx->prime));
		ctx->u = t->dp[0];
		dv_zero(s, 2 * FP_DIGS);
		s[2 * FP_DIGS] = 1;
		dv_zero(q, 2 * FP_DIGS + 1);
		dv_copy(q, ctx->prime.dp, FP_DIGS);
		bn_divn_low(t->dp, ctx->conv.dp, s, 2 * FP_DIGS + 1, q, FP_DIGS);
		ctx->conv.used = FP_DIGS;
		bn_trim(&(ctx->conv));
		bn_set_dig(&(ctx->one), 1);
		bn_lsh(&(ctx->one), &(ctx->one), ctx->prime.used * BN_DIGIT);
		bn_mod(&(ctx->one), &(ctx->one), &(ctx->prime));
#endif
		fp_prime_calc();
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(t);
		dv_free(s);
		dv_free(q);
	}
}

#ifdef WITH_FPX

/**
 * Computes the constantes required for evaluating Frobenius maps.
 */
static void fp2_calc(void) {
	bn_t e;
	fp2_t t0, t1;
	ctx_t *ctx = core_get();

	bn_null(e);
	fp2_null(t0);
	fp2_null(t1);

	TRY {
		bn_new(e);
		fp2_new(t0);
		fp2_new(t1);

		fp2_set_dig(t1, 1);
		fp2_mul_nor(t0, t1);
		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		bn_sub_dig(e, e, 1);
		bn_div_dig(e, e, 6);
		fp2_exp(t0, t0, e);
#if ALLOC == AUTO
		fp2_copy(ctx->fp2_p[0], t0);
		fp2_sqr(ctx->fp2_p[1], ctx->fp2_p[0]);
		fp2_mul(ctx->fp2_p[2], ctx->fp2_p[1], ctx->fp2_p[0]);
		fp2_sqr(ctx->fp2_p[3], ctx->fp2_p[1]);
		fp2_mul(ctx->fp2_p[4], ctx->fp2_p[3], ctx->fp2_p[0]);
#else
		fp_copy(ctx->fp2_p[0][0], t0[0]);
		fp_copy(ctx->fp2_p[0][1], t0[1]);
		fp2_sqr(t1, t0);
		fp_copy(ctx->fp2_p[1][0], t1[0]);
		fp_copy(ctx->fp2_p[1][1], t1[1]);
		fp2_mul(t1, t1, t0);
		fp_copy(ctx->fp2_p[2][0], t1[0]);
		fp_copy(ctx->fp2_p[2][1], t1[1]);
		fp2_sqr(t1, t0);
		fp2_sqr(t1, t1);
		fp_copy(ctx->fp2_p[3][0], t1[0]);
		fp_copy(ctx->fp2_p[3][1], t1[1]);
		fp2_mul(t1, t1, t0);
		fp_copy(ctx->fp2_p[4][0], t1[0]);
		fp_copy(ctx->fp2_p[4][1], t1[1]);
#endif
		fp2_frb(t1, t0, 1);
		fp2_mul(t0, t1, t0);
		fp_copy(ctx->fp2_p2[0], t0[0]);
		fp_sqr(ctx->fp2_p2[1], ctx->fp2_p2[0]);
		fp_mul(ctx->fp2_p2[2], ctx->fp2_p2[1], ctx->fp2_p2[0]);
		fp_sqr(ctx->fp2_p2[3], ctx->fp2_p2[1]);

		for (int i = 0; i < 5; i++) {
			fp_mul(ctx->fp2_p3[i][0], ctx->fp2_p2[i % 3], ctx->fp2_p[i][0]);
			fp_mul(ctx->fp2_p3[i][1], ctx->fp2_p2[i % 3], ctx->fp2_p[i][1]);
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(e);
		fp2_free(t0);
		fp2_free(t1);
	}
}

/**
 * Computes the constants required for evaluating Frobenius maps.
 */
static void fp3_calc(void) {
	bn_t e;
	fp3_t t0, t1, t2;
	ctx_t *ctx = core_get();

	bn_null(e);
	fp3_null(t0);
	fp3_null(t1);
	fp3_null(t2);

	TRY {
		bn_new(e);
		fp3_new(t0);
		fp3_new(t1);
		fp3_new(t2);

		fp_set_dig(ctx->fp3_base[0], -fp_prime_get_cnr());
		fp_neg(ctx->fp3_base[0], ctx->fp3_base[0]);
		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		bn_sub_dig(e, e, 1);
		bn_div_dig(e, e, 3);
		fp_exp(ctx->fp3_base[0], ctx->fp3_base[0], e);
		fp_sqr(ctx->fp3_base[1], ctx->fp3_base[0]);

		fp3_zero(t0);
		fp_set_dig(t0[1], 1);
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		bn_sub_dig(e, e, 1);
		bn_div_dig(e, e, 6);

		/* t0 = u^((p-1)/6). */
		fp3_exp(t0, t0, e);
		fp_copy(ctx->fp3_p[0], t0[2]);
		fp3_sqr(t1, t0);
		fp_copy(ctx->fp3_p[1], t1[1]);
		fp3_mul(t2, t1, t0);
		fp_copy(ctx->fp3_p[2], t2[0]);
		fp3_sqr(t2, t1);
		fp_copy(ctx->fp3_p[3], t2[2]);
		fp3_mul(t2, t2, t0);
		fp_copy(ctx->fp3_p[4], t2[1]);

		fp_mul(ctx->fp3_p2[0], ctx->fp3_p[0], ctx->fp3_base[1]);
		fp_mul(t0[0], ctx->fp3_p2[0], ctx->fp3_p[0]);
		fp_neg(ctx->fp3_p2[0], t0[0]);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(ctx->fp3_p2[0], ctx->fp3_p2[0], t0[0]);
		}
		fp_mul(ctx->fp3_p2[1], ctx->fp3_p[1], ctx->fp3_base[0]);
		fp_mul(ctx->fp3_p2[1], ctx->fp3_p2[1], ctx->fp3_p[1]);
		fp_sqr(ctx->fp3_p2[2], ctx->fp3_p[2]);
		fp_mul(ctx->fp3_p2[3], ctx->fp3_p[3], ctx->fp3_base[1]);
		fp_mul(t0[0], ctx->fp3_p2[3], ctx->fp3_p[3]);
		fp_neg(ctx->fp3_p2[3], t0[0]);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(ctx->fp3_p2[3], ctx->fp3_p2[3], t0[0]);
		}
		fp_mul(ctx->fp3_p2[4], ctx->fp3_p[4], ctx->fp3_base[0]);
		fp_mul(ctx->fp3_p2[4], ctx->fp3_p2[4], ctx->fp3_p[4]);

		fp_mul(ctx->fp3_p3[0], ctx->fp3_p[0], ctx->fp3_base[0]);
		fp_mul(t0[0], ctx->fp3_p3[0], ctx->fp3_p2[0]);
		fp_neg(ctx->fp3_p3[0], t0[0]);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(ctx->fp3_p3[0], ctx->fp3_p3[0], t0[0]);
		}
		fp_mul(ctx->fp3_p3[1], ctx->fp3_p[1], ctx->fp3_base[1]);
		fp_mul(t0[0], ctx->fp3_p3[1], ctx->fp3_p2[1]);
		fp_neg(ctx->fp3_p3[1], t0[0]);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(ctx->fp3_p3[1], ctx->fp3_p3[1], t0[0]);
		}
		fp_mul(ctx->fp3_p3[2], ctx->fp3_p[2], ctx->fp3_p2[2]);
		fp_mul(ctx->fp3_p3[3], ctx->fp3_p[3], ctx->fp3_base[0]);
		fp_mul(t0[0], ctx->fp3_p3[3], ctx->fp3_p2[3]);
		fp_neg(ctx->fp3_p3[3], t0[0]);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(ctx->fp3_p3[3], ctx->fp3_p3[3], t0[0]);
		}
		fp_mul(ctx->fp3_p3[4], ctx->fp3_p[4], ctx->fp3_base[1]);
		fp_mul(t0[0], ctx->fp3_p3[4], ctx->fp3_p2[4]);
		fp_neg(ctx->fp3_p3[4], t0[0]);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(ctx->fp3_p3[4], ctx->fp3_p3[4], t0[0]);
		}
		for (int i = 0; i < 5; i++) {
			fp_mul(ctx->fp3_p4[i], ctx->fp3_p[i], ctx->fp3_p3[i]);
			fp_mul(ctx->fp3_p5[i], ctx->fp3_p2[i], ctx->fp3_p3[i]);
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(e);
		fp3_free(t0);
		fp3_free(t1);
		fp3_free(t2);
	}
}

#endif /* WITH_FPX */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_prime_init(void) {
	ctx_t *ctx = core_get();
	ctx->fp_id = 0;
	bn_init(&(ctx->prime), FP_DIGS);
#if FP_RDC == QUICK || !defined(STRIP)
	ctx->sps_len = 0;
	memset(ctx->sps, 0, sizeof(ctx->sps));
#endif
#if FP_RDC == MONTY || !defined(STRIP)
	bn_init(&(ctx->conv), FP_DIGS);
	bn_init(&(ctx->one), FP_DIGS);
#endif
}

void fp_prime_clean(void) {
	ctx_t *ctx = core_get();
	ctx->fp_id = 0;
#if FP_RDC == QUICK || !defined(STRIP)
	ctx->sps_len = 0;
	memset(ctx->sps, 0, sizeof(ctx->sps));
#endif
#if FP_RDC == MONTY || !defined(STRIP)
	bn_clean(&(ctx->one));
	bn_clean(&(ctx->conv));
#endif
	bn_clean(&(ctx->prime));
}

const dig_t *fp_prime_get(void) {
	return core_get()->prime.dp;
}

const dig_t *fp_prime_get_rdc(void) {
	return &(core_get()->u);
}

const int *fp_prime_get_sps(int *len) {
#if FP_RDC == QUICK || !defined(STRIP)
	ctx_t *ctx = core_get();
	if (ctx->sps_len > 0 && ctx->sps_len < MAX_TERMS) {
		if (len != NULL) {
			*len = ctx->sps_len;
		}
		return ctx->sps;
	} else {
		if (len != NULL) {
			*len = 0;
		}
		return NULL;
	}
#else
	return NULL;
#endif
}

const dig_t *fp_prime_get_conv(void) {
#if FP_RDC == MONTY || !defined(STRIP)
	return core_get()->conv.dp;
#else
	return NULL;
#endif
}

dig_t fp_prime_get_mod8(void) {
	return core_get()->mod8;
}

int fp_prime_get_qnr(void) {
	return core_get()->qnr;
}

int fp_prime_get_cnr(void) {
	return core_get()->cnr;
}

void fp_prime_set_dense(const bn_t p) {
	fp_prime_set(p);
#if FP_RDC == QUICK
	THROW(ERR_NO_CONFIG);
#endif
}

void fp_prime_set_pmers(const int *f, int len) {
	bn_t p, t;

	bn_null(p);
	bn_null(t);

	TRY {
		bn_new(p);
		bn_new(t);

		if (len >= MAX_TERMS) {
			THROW(ERR_NO_VALID);
		}

		bn_set_2b(p, f[len - 1]);
		for (int i = len - 2; i > 0; i--) {
			if (f[i] > 0) {
				bn_set_2b(t, f[i]);
				bn_add(p, p, t);
			} else {
				bn_set_2b(t, -f[i]);
				bn_sub(p, p, t);
			}
		}
		if (f[0] > 0) {
			bn_add_dig(p, p, f[0]);
		} else {
			bn_sub_dig(p, p, -f[0]);
		}

#if FP_RDC == QUICK || !defined(STRIP)
		ctx_t *ctx = core_get();
		for (int i = 0; i < len; i++) {
			ctx->sps[i] = f[i];
		}
		ctx->sps[len] = 0;
		ctx->sps_len = len;
#endif /* FP_RDC == QUICK */

		fp_prime_set(p);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(p);
		bn_free(t);
	}
}

void fp_prime_calc(void) {
#ifdef WITH_FPX
	if (fp_prime_get_qnr() != 0) {
		fp2_calc();
	}
	if (fp_prime_get_cnr() != 0) {
		fp3_calc();
	}
#endif
}

void fp_prime_conv(fp_t c, const bn_t a) {
	bn_t t;

	bn_null(t);

	TRY {
		bn_new(t);

#if FP_RDC == MONTY
		bn_mod(t, a, &(core_get()->prime));
		bn_lsh(t, t, FP_DIGS * FP_DIGIT);
		bn_mod(t, t, &(core_get()->prime));
		dv_copy(c, t->dp, FP_DIGS);
#else
		if (a->used > FP_DIGS) {
			THROW(ERR_NO_PRECI);
		}

		bn_mod(t, a, &(core_get()->prime));

		if (bn_is_zero(t)) {
			fp_zero(c);
		} else {
			int i;
			for (i = 0; i < t->used; i++) {
				c[i] = t->dp[i];
			}
			for (; i < FP_DIGS; i++) {
				c[i] = 0;
			}
		}
		(void)t;
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(t);
	}
}

void fp_prime_conv_dig(fp_t c, dig_t a) {
	dv_t t;
	ctx_t *ctx = core_get();

	bn_null(t);

	TRY {
		dv_new(t);

#if FP_RDC == MONTY
		if (a != 1) {
			dv_zero(t, 2 * FP_DIGS + 1);
			t[FP_DIGS] = fp_mul1_low(t, ctx->conv.dp, a);
			fp_rdc(c, t);
		} else {
			dv_copy(c, ctx->one.dp, FP_DIGS);
		}
#else
		(void)ctx;
		fp_zero(c);
		c[0] = a;
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

void fp_prime_back(bn_t c, const fp_t a) {
	dv_t t;
	int i;

	dv_null(t);

	TRY {
		dv_new(t);

		bn_grow(c, FP_DIGS);
		for (i = 0; i < FP_DIGS; i++) {
			c->dp[i] = a[i];
		}
#if FP_RDC == MONTY
		dv_zero(t, 2 * FP_DIGS + 1);
		dv_copy(t, a, FP_DIGS);
		fp_rdc(c->dp, t);
#endif
		c->used = FP_DIGS;
		c->sign = BN_POS;
		bn_trim(c);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}
