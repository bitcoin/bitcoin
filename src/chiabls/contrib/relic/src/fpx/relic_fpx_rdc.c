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
 * Implementation of modular reduction in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if PP_QDR == BASIC || !defined(STRIP)

void fp2_rdc_basic(fp2_t c, dv2_t a) {
	fp_rdc(c[0], a[0]);
	fp_rdc(c[1], a[1]);
}

#endif

#if PP_QDR == INTEG || !defined(STRIP)

void fp2_rdc_integ(fp2_t c, dv2_t a) {
	fp2_rdcn_low(c, a);
}

#endif

#if PP_CBC == BASIC || !defined(STRIP)

void fp3_rdc_basic(fp3_t c, dv3_t a) {
	fp_rdc(c[0], a[0]);
	fp_rdc(c[1], a[1]);
	fp_rdc(c[2], a[2]);
}

#endif

#if PP_CBC == INTEG || !defined(STRIP)

void fp3_rdc_integ(fp3_t c, dv3_t a) {
	fp3_rdcn_low(c, a);
}

#endif
