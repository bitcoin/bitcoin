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
 * Implementation of point multiplication of a prime elliptic curve over a
 * quadratic extension by the curve cofactor.
 *
 * @ingroup epx
 */

#include "relic_core.h"
#include "relic_md.h"
#include "relic_tmpl_map.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep4_mul_cof(ep4_t r, ep4_t p) {
	bn_t z;
	ep4_t t0, t1, t2, t3;

	ep4_null(t0);
	ep4_null(t1);
	ep4_null(t2);
	ep4_null(t3);
	bn_null(z);

	RLC_TRY {
		ep4_new(t0);
		ep4_new(t1);
		ep4_new(t2);
		ep4_new(t3);

		fp_prime_get_par(z);

		ep4_mul_basic(t0, p, z);
		ep4_mul_basic(t1, t0, z);
		ep4_mul_basic(t2, t1, z);
		ep4_mul_basic(t3, t2, z);

		ep4_sub(t3, t3, t2);
		ep4_sub(t3, t3, p);
		ep4_sub(t2, t2, t1);
		ep4_frb(t2, t2, 1);

		ep4_sub(t1, t1, t0);
		ep4_frb(t1, t1, 2);

		ep4_sub(t0, t0, p);
		ep4_frb(t0, t0, 3);

		ep4_dbl(r, p);
		ep4_frb(r, r, 4);
		ep4_add(r, r, t0);
		ep4_add(r, r, t1);
		ep4_add(r, r, t2);
		ep4_add(r, r, t3);

		ep4_norm(r, r);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep4_free(t0);
		ep4_free(t1);
		ep4_free(t2);
		ep4_free(t3);
		bn_free(z);

	}
}
