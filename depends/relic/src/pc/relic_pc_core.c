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
 * Implementation core routines for pairing-based protocols.
 *
 * @ingroup pc
 */

#include "relic_pc.h"
#include "relic_core.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pc_core_init(void) {
	gt_new(core_get()->gt_g);
}

void pc_core_calc(void) {
	g1_t g1;
	g2_t g2;
	gt_t gt;

	g1_null(g1);
	g2_null(g2);
	gt_null(gt);

	RLC_TRY {
		g1_new(g1);
		g2_new(g2);
		gt_new(gt);

		g1_get_gen(g1);
		g2_get_gen(g2);

		pc_map(gt, g1, g2);
		gt_copy(core_get()->gt_g, gt);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		g1_free(g1);
		g2_free(g2);
		gt_free(gt);
	}
}

void pc_core_clean(void) {
	ctx_t *ctx = core_get();
	if (ctx != NULL) {
		gt_free(core_get()->gt_g);
	}
}
