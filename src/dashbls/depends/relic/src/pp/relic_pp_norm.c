/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2011 RELIC Authors
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
 * Implementation of point normalization for points used in pairing computation.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_md.h"
#include "relic_pp.h"
#include "relic_conf.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_norm_k2(ep_t r, ep_t p) {
	ep_norm(r, p);
}

void pp_norm_k8(ep2_t r, ep2_t p) {
	if (ep2_is_infty(p)) {
		ep2_set_infty(r);
		return;
	}

	if (p->coord) {
		/* If the point is represented in affine coordinates, we just copy it. */
		ep2_copy(r, p);
	}
#if EP_ADD == PROJC || !defined(STRIP)
	fp2_inv(r->z, p->z);
	fp2_mul(r->x, p->x, r->z);
	fp2_mul(r->y, p->y, r->z);
	fp2_mul(r->y, r->y, r->z);
	fp2_set_dig(r->z, 1);
	r->coord = BASIC;
#endif
}

void pp_norm_k12(ep2_t r, ep2_t p) {
	if (ep2_is_infty(p)) {
		ep2_set_infty(r);
		return;
	}

	if (p->coord) {
		/* If the point is represented in affine coordinates, we just copy it. */
		ep2_copy(r, p);
	}
#if EP_ADD == PROJC || !defined(STRIP)
	fp2_inv(r->z, p->z);
	fp2_mul(r->x, p->x, r->z);
	fp2_mul(r->y, p->y, r->z);
	fp2_set_dig(r->z, 1);
	r->coord = BASIC;
#endif
}

void pp_norm_k24(ep4_t r, ep4_t p) {
	if (ep4_is_infty(p)) {
		ep4_set_infty(r);
		return;
	}

	if (p->coord) {
		/* If the point is represented in affine coordinates, we just copy it. */
		ep4_copy(r, p);
	}
#if EP_ADD == PROJC || !defined(STRIP)
	fp4_inv(r->z, p->z);
	fp4_mul(r->x, p->x, r->z);
	fp4_mul(r->y, p->y, r->z);
	fp4_set_dig(r->z, 1);
	r->coord = BASIC;
#endif
}
