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
 * Implementation of the binary field squaring.
 *
 * @ingroup fb
 */

#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FB_SQR == BASIC || !defined(STRIP)

void fb_sqr_basic(fb_t c, const fb_t a) {
	dv_t t;

	dv_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(t);
		fb_sqrn_low(t, a);
		fb_rdc(c, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif

#if FB_SQR == LUTBL || !defined(STRIP)

void fb_sqr_lutbl(fb_t c, const fb_t a) {
	dv_t t;

	dv_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(t);
		fb_sqrl_low(t, a);
		fb_rdc(c, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif

#if FB_SQR == INTEG || !defined(STRIP)

void fb_sqr_integ(fb_t c, const fb_t a) {
	fb_sqrm_low(c, a);
}

#endif
