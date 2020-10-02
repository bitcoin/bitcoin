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
 * Implementation of prime field exponentiation functions.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FP_EXP == BASIC || !defined(STRIP)

void fp_exp_basic(fp_t c, const fp_t a, const bn_t b) {
	int i, l;
	fp_t r;

	fp_null(r);

	if (bn_is_zero(b)) {
		fp_set_dig(c, 1);
		return;
	}

	TRY {
		fp_new(r);

		l = bn_bits(b);

		fp_copy(r, a);

		for (i = l - 2; i >= 0; i--) {
			fp_sqr(r, r);
			if (bn_get_bit(b, i)) {
				fp_mul(r, r, a);
			}
		}

		if (bn_sign(b) == BN_NEG) {
			fp_inv(c, r);
		} else {
			fp_copy(c, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(r);
	}
}

#endif

#if FP_EXP == SLIDE || !defined(STRIP)

void fp_exp_slide(fp_t c, const fp_t a, const bn_t b) {
	fp_t t[1 << (FP_WIDTH - 1)], r;
	int i, j, l;
	uint8_t win[FP_BITS + 1];

	fp_null(r);

	if (bn_is_zero(b)) {
		fp_set_dig(c, 1);
		return;
	}


	/* Initialize table. */
	for (i = 0; i < (1 << (FP_WIDTH - 1)); i++) {
		fp_null(t[i]);
	}

	TRY {
		for (i = 0; i < (1 << (FP_WIDTH - 1)); i ++) {
			fp_new(t[i]);
		}
		fp_new(r);

		fp_copy(t[0], a);
		fp_sqr(r, a);

		/* Create table. */
		for (i = 1; i < 1 << (FP_WIDTH - 1); i++) {
			fp_mul(t[i], t[i - 1], r);
		}

		fp_set_dig(r, 1);
		l = FP_BITS + 1;
		bn_rec_slw(win, &l, b, FP_WIDTH);
		for (i = 0; i < l; i++) {
			if (win[i] == 0) {
				fp_sqr(r, r);
			} else {
				for (j = 0; j < util_bits_dig(win[i]); j++) {
					fp_sqr(r, r);
				}
				fp_mul(r, r, t[win[i] >> 1]);
			}
		}

		if (bn_sign(b) == BN_NEG) {
			fp_inv(c, r);
		} else {
			fp_copy(c, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < (1 << (FP_WIDTH - 1)); i++) {
			fp_free(t[i]);
		}
		fp_free(r);
	}
}

#endif

#if FP_EXP == MONTY || !defined(STRIP)

void fp_exp_monty(fp_t c, const fp_t a, const bn_t b) {
	fp_t t[2];

	fp_null(t[0]);
	fp_null(t[1]);

	if (bn_is_zero(b)) {
		fp_set_dig(c, 1);
		return;
	}

	TRY {
		fp_new(t[0]);
		fp_new(t[1]);

		fp_set_dig(t[0], 1);
		fp_copy(t[1], a);

		for (int i = bn_bits(b) - 1; i >= 0; i--) {
			int j = bn_get_bit(b, i);
			dv_swap_cond(t[0], t[1], FP_DIGS, j ^ 1);
			fp_mul(t[0], t[0], t[1]);
			fp_sqr(t[1], t[1]);
			dv_swap_cond(t[0], t[1], FP_DIGS, j ^ 1);
		}

		if (bn_sign(b) == BN_NEG) {
			fp_inv(c, t[0]);
		} else {
			fp_copy(c, t[0]);
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t[1]);
		fp_free(t[0]);
	}
}

#endif
