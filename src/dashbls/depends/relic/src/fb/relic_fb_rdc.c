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
 * Implementation of the binary field modular reduction.
 *
 * @ingroup fb
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_bn_low.h"
#include "relic_dv.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FB_RDC == BASIC || !defined(STRIP)

void fb_rdc_basic(fb_t c, dv_t a) {
	int j, k;
	dig_t *tmpa;
	dv_t r;

	dv_null(r);

	RLC_TRY {
		dv_new(r);

		tmpa = a + RLC_FB_DIGS;

		/* First reduce the high part. */
		for (int i = fb_bits(tmpa) - 1; i >= 0; i--) {
			if (fb_get_bit(tmpa, i)) {
				RLC_RIP(k, j, i - RLC_FB_BITS);
				if (k <= 0) {
					fb_addd_low(tmpa + j, tmpa + j, fb_poly_get(), RLC_FB_DIGS);
				} else {
					r[RLC_FB_DIGS] = fb_lshb_low(r, fb_poly_get(), k);
					fb_addd_low(tmpa + j, tmpa + j, r, RLC_FB_DIGS + 1);
				}
			}
		}
		for (int i = fb_bits(a) - 1; i >= RLC_FB_BITS; i--) {
			if (fb_get_bit(a, i)) {
				RLC_RIP(k, j, i - RLC_FB_BITS);
				if (k == 0) {
					fb_addd_low(a + j, a + j, fb_poly_get(), RLC_FB_DIGS);
				} else {
					r[RLC_FB_DIGS] = fb_lshb_low(r, fb_poly_get(), k);
					fb_addd_low(a + j, a + j, r, RLC_FB_DIGS + 1);
				}
			}
		}

		fb_copy(c, a);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fb_free(r);
	}
}

#endif

#if FB_RDC == QUICK || !defined(STRIP)

void fb_rdc_quick(fb_t c, dv_t a) {
	fb_rdcn_low(c, a);
}

#endif
