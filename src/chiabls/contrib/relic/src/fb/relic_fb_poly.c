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
 * Implementation of the binary field modulus manipulation.
 *
 * @ingroup fb
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_dv.h"
#include "relic_fb.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if FB_TRC == QUICK || !defined(STRIP)

/**
 * Find non-zero bits for fast trace computation.
 *
 * @throw ERR_NO_MEMORY if there is no available memory.
 * @throw ERR_NO_VALID if the polynomial is invalid.
 */
static void find_trace(void) {
	fb_t t0, t1;
	int counter;
	ctx_t *ctx = core_get();

	fb_null(t0);
	fb_null(t1);

	ctx->fb_ta = ctx->fb_tb = ctx->fb_tc = -1;

	TRY {
		fb_new(t0);
		fb_new(t1);

		counter = 0;
		for (int i = 0; i < FB_BITS; i++) {
			fb_zero(t0);
			fb_set_bit(t0, i, 1);
			fb_copy(t1, t0);
			for (int j = 1; j < FB_BITS; j++) {
				fb_sqr(t1, t1);
				fb_add(t0, t0, t1);
			}
			if (!fb_is_zero(t0)) {
				switch (counter) {
					case 0:
						ctx->fb_ta = i;
						ctx->fb_tb = ctx->fb_tc = -1;
						break;
					case 1:
						ctx->fb_tb = i;
						ctx->fb_tc = -1;
						break;
					case 2:
						ctx->fb_tc = i;
						break;
					default:
						THROW(ERR_NO_VALID);
						break;
				}
				counter++;
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(t0);
		fb_free(t1);
	}
}

#endif

#if FB_SLV == QUICK || !defined(STRIP)

/**
 * Precomputes half-traces for z^i with odd i.
 *
 * @throw ERR_NO_MEMORY if there is no available memory.
 */
static void find_solve(void) {
	int i, j, k, l;
	fb_t t0;
	ctx_t *ctx = core_get();

	fb_null(t0);

	TRY {
		fb_new(t0);

		l = 0;
		for (i = 0; i < FB_BITS; i += 8, l++) {
			for (j = 0; j < 16; j++) {
				fb_zero(t0);
				for (k = 0; k < 4; k++) {
					if (j & (1 << k)) {
						fb_set_bit(t0, i + 2 * k + 1, 1);
					}
				}
				fb_copy(ctx->fb_half[l][j], t0);
				for (k = 0; k < (FB_BITS - 1) / 2; k++) {
					fb_sqr(ctx->fb_half[l][j], ctx->fb_half[l][j]);
					fb_sqr(ctx->fb_half[l][j], ctx->fb_half[l][j]);
					fb_add(ctx->fb_half[l][j], ctx->fb_half[l][j], t0);
				}
			}
			fb_rsh(ctx->fb_half[l][j], ctx->fb_half[l][j], 1);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(t0);
	}
}

#endif

#if FB_SRT == QUICK || !defined(STRIP)

/**
 * Precomputes the square root of z.
 */
static void find_srz(void) {
	ctx_t *ctx = core_get();

	fb_set_dig(ctx->fb_srz, 2);

	for (int i = 1; i < FB_BITS; i++) {
		fb_sqr(ctx->fb_srz, ctx->fb_srz);
	}

#ifdef FB_PRECO
	for (int i = 0; i <= 255; i++) {
		fb_mul_dig(ctx->fb_tab_srz[i], ctx->fb_srz, i);
	}
#endif
}

#endif

#if FB_INV == ITOHT || !defined(STRIP)

/**
 * Finds an addition chain for (FB_BITS - 1).
 */
static void find_chain(void) {
	int i, j, k, l;
	ctx_t *ctx = core_get();

	ctx->chain_len = -1;
	for (int i = 0; i < MAX_TERMS; i++) {
		ctx->chain[i] = (i << 8) + i;
	}
	switch (FB_BITS) {
		case 127:
			ctx->chain[1] = (1 << 8) + 0;
			ctx->chain[4] = (4 << 8) + 2;
			ctx->chain[7] = (7 << 8) + 2;
			ctx->chain_len = 9;
			break;
		case 193:
			ctx->chain[1] = (1 << 8) + 0;
			ctx->chain_len = 8;
			break;
		case 233:
			ctx->chain[1] = (1 << 8) + 0;
			ctx->chain[3] = (3 << 8) + 0;
			ctx->chain[6] = (6 << 8) + 0;
			ctx->chain_len = 10;
			break;
		case 251:
			ctx->chain[1] = (1 << 8) + 0;
			ctx->chain[2] = (2 << 8) + 1;
			ctx->chain[4] = (4 << 8) + 3;
			ctx->chain[5] = (5 << 8) + 4;
			ctx->chain[7] = (7 << 8) + 6;
			ctx->chain[8] = (8 << 8) + 7;
			ctx->chain_len = 10;
			break;
		case 283:
			ctx->chain[4] = (4 << 8) + 0;
			ctx->chain[6] = (6 << 8) + 0;
			ctx->chain[9] = (9 << 8) + 0;
			ctx->chain_len = 11;
			break;
		case 353:
			ctx->chain[2] = (2 << 8) + 0;
			ctx->chain[4] = (4 << 8) + 0;
			ctx->chain_len = 10;
			break;
		case 367:
			ctx->chain[1] = (1 << 8) + 0;
			ctx->chain[2] = (2 << 8) + 1;
			ctx->chain[6] = (6 << 8) + 3;
			ctx->chain[9] = (9 << 8) + 2;
			ctx->chain_len = 11;
			break;
		case 1223:
			ctx->chain[1] = (1 << 8) + 0;
			ctx->chain[2] = (2 << 8) + 0;
			ctx->chain[4] = (4 << 8) + 2;
			ctx->chain[5] = (5 << 8) + 4;
			ctx->chain[10] = (10 << 8) + 2;
			ctx->chain[11] = (11 << 8) + 10;
			ctx->chain_len = 13;
			break;
		default:
			l = 0;
			j = (FB_BITS - 1);
			for (k = 16; k >= 0; k--) {
				if (j & (1 << k)) {
					break;
				}
			}
			for (i = 1; i < k; i++) {
				if (j & (1 << i)) {
					l++;
				}
			}
			i = 0;
			ctx->chain_len = k + l;
			while (j != 1) {
				if ((j & 0x01) != 0) {
					i++;
					ctx->chain[ctx->chain_len - i] = ((ctx->chain_len - i) << 8) + 0;
				}
				i++;
				j = j >> 1;
			}
			break;
	}

	int x, y, u[ctx->chain_len + 1];

	for (i = 0; i < MAX_TERMS; i++) {
		for (j = 0; j < RELIC_FB_TABLE; j++) {
			ctx->fb_tab_ptr[i][j] = &(ctx->fb_tab_sqr[i][j]);
		}
	}

	u[0] = 1;
	u[1] = 2;
	for (i = 2; i <= ctx->chain_len; i++) {
		x = ctx->chain[i - 1] >> 8;
		y = ctx->chain[i - 1] - (x << 8);
		if (x == y) {
			u[i] = 2 * u[i - 1];
		} else {
			u[i] = u[x] + u[y];
		}
	}

	for (i = 0; i <= ctx->chain_len; i++) {
		fb_itr_pre((fb_t *)fb_poly_tab_sqr(i), u[i]);
	}
}

#endif

/**
 * Configures the irreducible polynomial of the binary field.
 *
 * @param[in] f				- the new irreducible polynomial.
 */
static void fb_poly_set(const fb_t f) {
	fb_copy(core_get()->fb_poly, f);
#if FB_TRC == QUICK || !defined(STRIP)
	find_trace();
#endif
#if FB_SLV == QUICK || !defined(STRIP)
	find_solve();
#endif
#if FB_SRT == QUICK || !defined(STRIP)
	find_srz();
#endif
#if FB_INV == ITOHT || !defined(STRIP)
	find_chain();
#endif
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_poly_init(void) {
	ctx_t *ctx = core_get();

	fb_zero(ctx->fb_poly);
	ctx->fb_pa = ctx->fb_pb = ctx->fb_pc = 0;
	ctx->fb_na = ctx->fb_nb = ctx->fb_nc = -1;
}

void fb_poly_clean(void) {
}

dig_t *fb_poly_get(void) {
	return core_get()->fb_poly;
}

void fb_poly_add(fb_t c, const fb_t a) {
	ctx_t *ctx = core_get();

	if (c != a) {
		fb_copy(c, a);
	}

	if (ctx->fb_pa != 0) {
		c[FB_DIGS - 1] ^= ctx->fb_poly[FB_DIGS - 1];
		if (ctx->fb_na != FB_DIGS - 1) {
			c[ctx->fb_na] ^= ctx->fb_poly[ctx->fb_na];
		}
		if (ctx->fb_pb != 0 && ctx->fb_pc != 0) {
			if (ctx->fb_nb != ctx->fb_na) {
				c[ctx->fb_nb] ^= ctx->fb_poly[ctx->fb_nb];
			}
			if (ctx->fb_nc != ctx->fb_na && ctx->fb_nc != ctx->fb_nb) {
				c[ctx->fb_nc] ^= ctx->fb_poly[ctx->fb_nc];
			}
		}
		if (ctx->fb_na != 0 && ctx->fb_nb != 0 && ctx->fb_nc != 0) {
			c[0] ^= 1;
		}
	} else {
		fb_add(c, a, ctx->fb_poly);
	}
}

void fb_poly_sub(fb_t c, const fb_t a) {
	fb_poly_add(c, a);
}

void fb_poly_set_dense(const fb_t f) {
	ctx_t *ctx = core_get();
	fb_poly_set(f);
	ctx->fb_pa = ctx->fb_pb = ctx->fb_pc = 0;
	ctx->fb_na = ctx->fb_nb = ctx->fb_nc = -1;
}

void fb_poly_set_trino(int a) {
	fb_t f;
	ctx_t *ctx = core_get();

	fb_null(f);

	TRY {
		ctx->fb_pa = a;
		ctx->fb_pb = ctx->fb_pc = 0;

		ctx->fb_na = ctx->fb_pa >> FB_DIG_LOG;
		ctx->fb_nb = ctx->fb_nc = -1;

		fb_new(f);
		fb_zero(f);
		fb_set_bit(f, FB_BITS, 1);
		fb_set_bit(f, a, 1);
		fb_set_bit(f, 0, 1);
		fb_poly_set(f);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(f);
	}
}

void fb_poly_set_penta(int a, int b, int c) {
	fb_t f;
	ctx_t *ctx = core_get();

	fb_null(f);

	TRY {
		fb_new(f);

		ctx->fb_pa = a;
		ctx->fb_pb = b;
		ctx->fb_pc = c;

		ctx->fb_na = ctx->fb_pa >> FB_DIG_LOG;
		ctx->fb_nb = ctx->fb_pb >> FB_DIG_LOG;
		ctx->fb_nc = ctx->fb_pc >> FB_DIG_LOG;

		fb_zero(f);
		fb_set_bit(f, FB_BITS, 1);
		fb_set_bit(f, a, 1);
		fb_set_bit(f, b, 1);
		fb_set_bit(f, c, 1);
		fb_set_bit(f, 0, 1);
		fb_poly_set(f);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(f);
	}
}

dig_t *fb_poly_get_srz(void) {
#if FB_SRT == QUICK || !defined(STRIP)
	return core_get()->fb_srz;
#else
	return NULL;
#endif
}

const fb_t *fb_poly_tab_sqr(int i) {
#if FB_INV == ITOHT || !defined(STRIP)
	/* If ITOHT inversion is used and tables are precomputed, return them. */
#if ALLOC == AUTO
	return (const fb_t *)*core_get()->fb_tab_ptr[i];
#else
	return (const fb_t *)core_get()->fb_tab_ptr[i];
#endif

#else
	return NULL;
#endif
}

const dig_t *fb_poly_tab_srz(int i) {
#if FB_SRT == QUICK || !defined(STRIP)

#ifdef FB_PRECO
	return core_get()->fb_tab_srz[i];
#else
	return NULL;
#endif

#else
	return NULL;
#endif
}

void fb_poly_get_trc(int *a, int *b, int *c) {
#if FB_TRC == QUICK || !defined(STRIP)
	ctx_t *ctx = core_get();
	*a = ctx->fb_ta;
	*b = ctx->fb_tb;
	*c = ctx->fb_tc;
#else
	*a = *b = *c = -1;
#endif
}

void fb_poly_get_rdc(int *a, int *b, int *c) {
	ctx_t *ctx = core_get();
	*a = ctx->fb_pa;
	*b = ctx->fb_pb;
	*c = ctx->fb_pc;
}

const dig_t *fb_poly_get_slv(void) {
#if FB_SLV == QUICK || !defined(STRIP)
	return (dig_t *)&(core_get()->fb_half);
#else
	return NULL;
#endif
}

const int *fb_poly_get_chain(int *len) {
#if FB_INV == ITOHT || !defined(STRIP)
	ctx_t *ctx = core_get();
	if (ctx->chain_len > 0 && ctx->chain_len < MAX_TERMS) {
		if (len != NULL) {
			*len = ctx->chain_len;
		}
		return ctx->chain;
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
