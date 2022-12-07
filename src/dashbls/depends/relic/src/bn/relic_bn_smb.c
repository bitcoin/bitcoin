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
 * Implementation of Legendre and Jacobi symbols.
 *
 * @ingroup bn
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_smb_leg(bn_t c, const bn_t a, const bn_t b) {
	bn_t t;

	bn_null(t);

	if (bn_sign(b) == RLC_NEG) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	if (bn_cmp(a, b) == RLC_EQ) {
		bn_zero(c);
		return;
	}

	RLC_TRY {
		bn_new(t);

		/* t = (b - 1)/2. */
		bn_sub_dig(t, b, 1);
		bn_rsh(t, t, 1);
		bn_mxp(c, a, t, b);
		bn_sub_dig(t, b, 1);
		if (bn_cmp(c, t) == RLC_EQ) {
			bn_set_dig(c, 1);
			bn_neg(c, c);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

void bn_smb_jac(bn_t c, const bn_t a, const bn_t b) {
	bn_t t0, t1, r;
	int t, h;

	bn_null(t0);
	bn_null(t1);
	bn_null(r);

	/* Argument b must be odd. */
	if (bn_is_even(b) || bn_sign(b) == RLC_NEG) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	RLC_TRY {
		bn_new(t0);
		bn_new(t1);
		bn_new(r);
		t = 1;

		if (bn_sign(a) == RLC_NEG) {
			bn_add(t0, a, b);
		} else {
			bn_copy(t0, a);
		}
		bn_copy(t1, b);

		while (1) {
			/* t0 = a mod b. */
			bn_mod(t0, t0, t1);
			/* If a = 0 then if n = 1 return t else return 0. */
			if (bn_is_zero(t0)) {
				if (bn_cmp_dig(t1, 1) == RLC_EQ) {
					bn_set_dig(c, 1);
					if (t == -1) {
						bn_neg(c, c);
					}
					break;
				} else {
					bn_zero(c);
					break;
				}
			}
			/* Write t0 as 2^h * t0. */
			h = 0;
			while (bn_is_even(t0)) {
				h++;
				bn_rsh(t0, t0, 1);
			}
			/* If h != 0 (mod 2) and n != +-1 (mod 8) then t = -t. */
			bn_mod_2b(r, t1, 3);
			if ((h % 2 != 0) && (bn_cmp_dig(r, 1) != RLC_EQ) &&
					(bn_cmp_dig(r, 7) != RLC_EQ)) {
				t = -t;
			}
			/* If t0 != 1 (mod 4) and n != 1 (mod 4) then t = -t. */
			bn_mod_2b(r, t0, 2);
			if (bn_cmp_dig(r, 1) != RLC_EQ) {
				bn_mod_2b(r, t1, 2);
				if (bn_cmp_dig(r, 1) != RLC_EQ) {
					t = -t;
				}
			}
			bn_copy(r, t0);
			bn_copy(t0, t1);
			bn_copy(t1, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t0);
		bn_free(t1);
		bn_free(r);
	}
}
