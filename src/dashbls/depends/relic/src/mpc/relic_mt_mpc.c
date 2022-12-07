/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
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
 * Implementation of multiplication tri generation.
 *
 * @ingroup mpc
 */

#include "relic_core.h"
#include "relic_bn.h"
#include "relic_mpc.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void mt_gen(mt_t tri[2], bn_t order) {
	for (int i = 0; i < 2; i++) {
		bn_rand_mod(tri[i]->a, order);
		bn_rand_mod(tri[i]->b, order);
	}
	bn_add(tri[0]->c, tri[0]->a, tri[1]->a);
	bn_mod(tri[0]->c, tri[0]->c, order);
	bn_add(tri[1]->c, tri[0]->b, tri[1]->b);
	bn_mod(tri[1]->c, tri[1]->c, order);

	bn_mul(tri[0]->c, tri[0]->c, tri[1]->c);
	bn_mod(tri[0]->c, tri[0]->c, order);

	bn_rand_mod(tri[1]->c, order);
	bn_sub(tri[0]->c, tri[0]->c, tri[1]->c);
	if (bn_sign(tri[0]->c)) {
		bn_add(tri[0]->c, tri[0]->c, order);
	}
}

void mt_mul_lcl(bn_t d, bn_t e, bn_t x, bn_t y, bn_t n, mt_t tri) {
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		/* Compute public values for transmission. */

		/* [d] = [x] - [a]. */
		bn_sub(d, x, tri->a);
		if (bn_sign(d) == RLC_NEG) {
			bn_add(d, d, n);
		}
		bn_mod(d, d, n);

		/* [e] = [y] - [b]. */
		bn_sub(e, y, tri->b);
		if (bn_sign(e) == RLC_NEG) {
			bn_add(e, e, n);
		}
		bn_mod(e, e, n);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(t);
	}
}

void mt_mul_bct(bn_t d[2], bn_t e[2], bn_t n) {
	/* Open values d and e. */
	bn_add(d[0], d[0], d[1]);
	bn_mod(d[0], d[0], n);
	bn_copy(d[1], d[0]);
	bn_add(e[0], e[0], e[1]);
	bn_mod(e[0], e[0], n);
	bn_copy(e[1], e[0]);
}

void mt_mul_mpc(bn_t r, bn_t d, bn_t e, bn_t n, mt_t tri, int party) {
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		/* One party computes public value d*([b] + e), the other just d[b]. */
		if (party == 0) {
			bn_add(t, tri->b, e);
			if (bn_sign(t) == RLC_NEG) {
				bn_add(t, t, n);
			}
			bn_mod(t, t, n);
		} else {
			bn_copy(t, tri->b);
		}
		bn_mul(t, t, d);
		bn_mod(t, t, n);
		/* Compute r = [a]*e*/
		bn_mul(r, tri->a, e);
		bn_mod(r, r, n);
		bn_add(r, r, t);
		/* r = [a][e] + d[b] + de + c = [xy] */
		bn_add(r, r, tri->c);
		bn_mod(r, r, n);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(t);
	}
}
