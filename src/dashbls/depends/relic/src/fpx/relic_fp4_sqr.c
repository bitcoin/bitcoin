/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2019 RELIC Authors
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
 * Implementation of squaring in a quartic extension of a prime field.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FPX_RDC == BASIC || !defined(STRIP)

void fp4_sqr_basic(fp4_t c, fp4_t a) {
	fp2_t t0, t1;

	fp2_null(t0);
	fp2_null(t1);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);

		fp2_add(t0, a[0], a[1]);
		fp2_mul_nor(t1, a[1]);
		fp2_add(t1, a[0], t1);
		fp2_mul(t0, t0, t1);
		fp2_mul(c[1], a[0], a[1]);
		fp2_sub(c[0], t0, c[1]);
		fp2_mul_nor(t1, c[1]);
		fp2_sub(c[0], c[0], t1);
		fp2_dbl(c[1], c[1]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
	}
}

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

void fp4_sqr_unr(dv4_t c, fp4_t a) {
	fp2_t t;
	dv2_t u0, u1;

	fp2_null(t);
	dv2_null(u0);
	dv2_null(u1);

	RLC_TRY {
		fp2_new(t);
		dv2_new(u0);
		dv2_new(u1);

		/* t0 = a^2. */
		fp2_sqrn_low(u0, a[0]);
		/* t1 = b^2. */
		fp2_sqrn_low(u1, a[1]);

		fp2_addm_low(t, a[0], a[1]);

		/* c = a^2  + b^2 * E. */
		fp2_norh_low(c[0], u1);
		fp2_addc_low(c[0], c[0], u0);

		/* d = (a + b)^2 - a^2 - b^2 = 2 * a * b. */
		fp2_addc_low(u1, u1, u0);
		fp2_sqrn_low(c[1], t);
		fp2_subc_low(c[1], c[1], u1);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t);
		dv2_free(u0);
		dv2_free(u1);
	}
}

void fp4_sqr_lazyr(fp4_t c, fp4_t a) {
	dv4_t t;

	dv4_null(t);

	RLC_TRY {
		dv4_new(t);
		fp4_sqr_unr(t, a);
		fp2_rdcn_low(c[0], t[0]);
		fp2_rdcn_low(c[1], t[1]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv4_free(t);
	}
}

#endif
