/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2014 RELIC Authors
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
 * Implementation of the low-level extension field modular reduction functions.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp2_rdcn_low(fp2_t c, dv2_t a) {
#if FP_RDC == MONTY
	fp_rdcn_low(c[0], a[0]);
	fp_rdcn_low(c[1], a[1]);
#else
	fp_rdc(c[0], a[0]);
	fp_rdc(c[1], a[1]);
#endif
}

void fp3_rdcn_low(fp3_t c, dv3_t a) {
#if FP_RDC == MONTY
	fp_rdcn_low(c[0], a[0]);
	fp_rdcn_low(c[1], a[1]);
	fp_rdcn_low(c[2], a[2]);
#else
	fp_rdc(c[0], a[0]);
	fp_rdc(c[1], a[1]);
	fp_rdc(c[2], a[2]);
#endif
}
