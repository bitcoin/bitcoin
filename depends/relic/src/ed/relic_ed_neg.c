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
 * Implementation of the point negation on Edwards elliptic curves.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ED_ADD == BASIC || !defined(STRIP)

void ed_neg_basic(ed_t r, const ed_t p) {
	if (ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	fp_copy(r->y, p->y);
	fp_neg(r->x, p->x);

	r->coord = BASIC;
}

#endif

#if ED_ADD == PROJC || ED_ADD == EXTND || !defined(STRIP)

void ed_neg_projc(ed_t r, const ed_t p) {
	if (ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	fp_neg(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_copy(r->z, p->z);
#if ED_ADD == EXTND
	fp_neg(r->t, p->t);
#endif

	r->coord = p->coord;
}

#endif
