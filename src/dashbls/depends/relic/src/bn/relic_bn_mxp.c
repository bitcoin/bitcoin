/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Implementation of the multiple precision exponentiation functions.
 *
 * @ingroup bn
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Size of precomputation table.
 */
#define RLC_TABLE_SIZE			64

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if BN_MXP == BASIC || !defined(STRIP)

void bn_mxp_basic(bn_t c, const bn_t a, const bn_t b, const bn_t m) {
	int i, l;
	bn_t t, u, r;

	if (bn_cmp_dig(m, 1) == RLC_EQ) {
		bn_zero(c);
		return;
	}

	if (bn_is_zero(b)) {
		bn_set_dig(c, 1);
		return;
	}

	bn_null(t);
	bn_null(u);
	bn_null(r);

	RLC_TRY {
		bn_new(t);
		bn_new(u);
		bn_new(r);

		bn_mod_pre(u, m);

		l = bn_bits(b);

#if BN_MOD == MONTY
		bn_mod_monty_conv(t, a, m);
#else
		bn_copy(t, a);
#endif

		bn_copy(r, t);

		for (i = l - 2; i >= 0; i--) {
			bn_sqr(r, r);
			bn_mod(r, r, m, u);
			if (bn_get_bit(b, i)) {
				bn_mul(r, r, t);
				bn_mod(r, r, m, u);
			}
		}

#if BN_MOD == MONTY
		bn_mod_monty_back(r, r, m);
#endif

		if (bn_sign(b) == RLC_NEG) {
			bn_mod_inv(c, r, m);
		} else {
			bn_copy(c, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
		bn_free(u);
		bn_free(r);
	}
}

#endif

#if BN_MXP == SLIDE || !defined(STRIP)

void bn_mxp_slide(bn_t c, const bn_t a, const bn_t b, const bn_t m) {
	bn_t tab[RLC_TABLE_SIZE], t, u, r;
	int i, j, l, w = 1;
	uint8_t *win = RLC_ALLOCA(uint8_t, bn_bits(b));

	if (win == NULL) {
		RLC_THROW(ERR_NO_MEMORY);
		return;
	}

	if (bn_cmp_dig(m, 1) == RLC_EQ) {
		RLC_FREE(win);
		bn_zero(c);
		return;
	}

	if (bn_is_zero(b)) {
		RLC_FREE(win);
		bn_set_dig(c, 1);
		return;
	}

	bn_null(t);
	bn_null(u);
	bn_null(r);
	/* Initialize table. */
	for (i = 0; i < RLC_TABLE_SIZE; i++) {
		bn_null(tab[i]);
	}

	/* Find window size. */
	i = bn_bits(b);
	if (i <= 21) {
		w = 2;
	} else if (i <= 32) {
		w = 3;
	} else if (i <= 128) {
		w = 4;
	} else if (i <= 256) {
		w = 5;
	} else if (i <= 512) {
		w = 6;
	} else {
		w = 7;
	}

	RLC_TRY {
		for (i = 0; i < (1 << (w - 1)); i++) {
			bn_new(tab[i]);
		}

		bn_new(t);
		bn_new(u);
		bn_new(r);
		bn_mod_pre(u, m);

#if BN_MOD == MONTY
		bn_set_dig(r, 1);
		bn_mod_monty_conv(r, r, m);
		bn_mod_monty_conv(t, a, m);
#else /* BN_MOD == BARRT || BN_MOD == RADIX */
		bn_set_dig(r, 1);
		bn_copy(t, a);
#endif

		bn_copy(tab[0], t);
		bn_sqr(t, tab[0]);
		bn_mod(t, t, m, u);
		/* Create table. */
		for (i = 1; i < 1 << (w - 1); i++) {
			bn_mul(tab[i], tab[i - 1], t);
			bn_mod(tab[i], tab[i], m, u);
		}

		l = bn_bits(b);
		bn_rec_slw(win, &l, b, w);
		for (i = 0; i < l; i++) {
			if (win[i] == 0) {
				bn_sqr(r, r);
				bn_mod(r, r, m, u);
			} else {
				for (j = 0; j < util_bits_dig(win[i]); j++) {
					bn_sqr(r, r);
					bn_mod(r, r, m, u);
				}
				bn_mul(r, r, tab[win[i] >> 1]);
				bn_mod(r, r, m, u);
			}
		}
		bn_trim(r);
#if BN_MOD == MONTY
		bn_mod_monty_back(r, r, m);
#endif

		if (bn_sign(b) == RLC_NEG) {
			bn_mod_inv(c, r, m);
		} else {
			bn_copy(c, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < (1 << (w - 1)); i++) {
			bn_free(tab[i]);
		}
		bn_free(u);
		bn_free(t);
		bn_free(r);
		RLC_FREE(win);
	}
}

#endif

#if BN_MXP == MONTY || !defined(STRIP)

void bn_mxp_monty(bn_t c, const bn_t a, const bn_t b, const bn_t m) {
	bn_t tab[2], u;
	dig_t mask;
	int i, j, t;

	if (bn_cmp_dig(m, 1) == RLC_EQ) {
		bn_zero(c);
		return;
	}

	if (bn_is_zero(b)) {
		bn_set_dig(c, 1);
		return;
	}

	bn_null(tab[0]);
	bn_null(tab[1]);
	bn_null(u);

	RLC_TRY {
		bn_new(u);
		bn_mod_pre(u, m);

		bn_new(tab[0]);
		bn_new(tab[1]);

#if BN_MOD == MONTY
		bn_set_dig(tab[0], 1);
		bn_mod_monty_conv(tab[0], tab[0], m);
		bn_mod_monty_conv(tab[1], a, m);
#else
		bn_set_dig(tab[0], 1);
		bn_mod(tab[1], a, m);
#endif

		bn_grow(tab[0], m->alloc);
		bn_grow(tab[1], m->alloc);
		for (i = bn_bits(b) - 1; i >= 0; i--) {
			j = bn_get_bit(b, i);
			dv_swap_cond(tab[0]->dp, tab[1]->dp, m->alloc, j ^ 1);
			mask = -(j ^ 1);
			t = (tab[0]->used ^ tab[1]->used) & mask;
			tab[0]->used ^= t;
			tab[1]->used ^= t;
			t = (tab[0]->sign ^ tab[1]->sign) & mask;
			tab[0]->sign ^= t;
			tab[1]->sign ^= t;
			bn_mul(tab[0], tab[0], tab[1]);
			bn_mod(tab[0], tab[0], m, u);
			bn_sqr(tab[1], tab[1]);
			bn_mod(tab[1], tab[1], m, u);
			dv_swap_cond(tab[0]->dp, tab[1]->dp, m->alloc, j ^ 1);
			mask = -(j ^ 1);
			t = (tab[0]->used ^ tab[1]->used) & mask;
			tab[0]->used ^= t;
			tab[1]->used ^= t;
			t = (tab[0]->sign ^ tab[1]->sign) & mask;
			tab[0]->sign ^= t;
			tab[1]->sign ^= t;
		}

#if BN_MOD == MONTY
		bn_mod_monty_back(u, tab[0], m);
#else
		bn_copy(u, tab[0]);
#endif

		/* Silly branchless code, since called functions not constant-time. */
		bn_mod_inv(tab[0], u, m);
		dv_swap_cond(u->dp, tab[0]->dp, RLC_BN_DIGS, bn_sign(b) == RLC_NEG);
		if (bn_sign(b) == RLC_NEG) {
			u->sign = tab[0]->sign;
			if (bn_cmp_dig(tab[1], 1) != RLC_EQ) {
				bn_zero(c);
				RLC_THROW(ERR_NO_VALID);
			}
		}
		bn_add(tab[1], u, m);
		dv_swap_cond(u->dp, tab[1]->dp, RLC_BN_DIGS, bn_sign(b) == RLC_NEG && bn_sign(u) == RLC_NEG);
		u->sign = RLC_POS;
		bn_copy(c, u);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(tab[1]);
		bn_free(tab[0]);
		bn_free(u);
	}
}

#endif

void bn_mxp_dig(bn_t c, const bn_t a, dig_t b, const bn_t m) {
	int i, l;
	bn_t t, u, r;

	if (bn_cmp_dig(m, 1) == RLC_EQ) {
		bn_zero(c);
		return;
	}

	if (b == 0) {
		bn_set_dig(c, 1);
		return;
	}

	bn_null(t);
	bn_null(u);
	bn_null(r);

	RLC_TRY {
		bn_new(t);
		bn_new(u);
		bn_new(r);

		bn_mod_pre(u, m);

		l = util_bits_dig(b);

#if BN_MOD == MONTY
		bn_mod_monty_conv(t, a, m);
#else
		bn_copy(t, a);
#endif

		bn_copy(r, t);

		for (i = l - 2; i >= 0; i--) {
			bn_sqr(r, r);
			bn_mod(r, r, m, u);
			if (b & ((dig_t)1 << i)) {
				bn_mul(r, r, t);
				bn_mod(r, r, m, u);
			}
		}

#if BN_MOD == MONTY
		bn_mod_monty_back(c, r, m);
#else
		bn_copy(c, r);
#endif
		/* Exponent is unsigned, so no need to invert if negative. */
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
		bn_free(u);
		bn_free(r);
	}
}
