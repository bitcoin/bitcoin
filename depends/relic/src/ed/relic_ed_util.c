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
 * Implementation of the Edwards elliptic curve utilities.
 *
 * @version $Id$
 * @ingroup ed
 */

#include <assert.h>

#include "relic_core.h"
#include "relic_label.h"
#include "relic_md.h"


/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int ed_is_infty(const ed_t p) {
	fp_t t;
	int r = 0;

	fp_null(t);

	if (p->coord == BASIC) {
		return (fp_is_zero(p->x) && (fp_cmp_dig(p->y, 1) == RLC_EQ));
	}

	if (fp_is_zero(p->z)) {
		RLC_THROW(ERR_NO_VALID);
		return 0;
	}

	RLC_TRY {
		fp_new(t);
		fp_inv(t, p->z);
		fp_mul(t, p->y, t);
		if (fp_is_zero(p->x) && (fp_cmp_dig(t, 1) == RLC_EQ)) {
			r = 1;
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(t);
	}

	return r;
}

void ed_set_infty(ed_t p) {
	fp_zero(p->x);
	fp_set_dig(p->y, 1);
	fp_set_dig(p->z, 1);
#if ED_ADD == EXTND
	fp_zero(p->t);
#endif
	p->coord = PROJC;
}

void ed_copy(ed_t r, const ed_t p) {
	fp_copy(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_copy(r->z, p->z);
#if ED_ADD == EXTND
	fp_copy(r->t, p->t);
#endif

	r->coord = p->coord;
}

void ed_rand(ed_t p) {
	bn_t n, k;

	bn_null(k);
	bn_null(n);

	RLC_TRY {
		bn_new(k);
		bn_new(n);

		ed_curve_get_ord(n);
		bn_rand_mod(k, n);

		ed_mul_gen(p, k);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(k);
		bn_free(n);
	}
}

void ed_blind(ed_t r, const ed_t p) {
	fp_t rand;

	fp_null(rand);

	RLC_TRY {
		fp_new(rand);

		fp_rand(rand);
		fp_mul(r->x, p->x, rand);
		fp_mul(r->y, p->y, rand);
		fp_mul(r->z, p->z, rand);
		r->coord = PROJC;
#if ED_ADD == EXTND
		fp_mul(r->t, p->t, rand);
#endif
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(rand);
	}
}

void ed_rhs(fp_t rhs, const ed_t p) {
	fp_t t0, t1;

	fp_null(t0);
	fp_null(t1);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);

		// 1 = a * X^2 + Y^2 - d * X^2 * Y^2
		fp_sqr(t0, p->x);
		fp_mul(t0, t0, core_get()->ed_a);
		fp_sqr(t1, p->y);
		fp_add(t1, t1, t0);
		fp_mul(t0, p->x, p->y);
		fp_sqr(t0, t0);
		fp_mul(t0, t0, core_get()->ed_d);
		fp_sub(rhs, t1, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
	}
}

int ed_on_curve(const ed_t p) {
	ed_t t;
	int r = 0;

	ed_null(t);

	if (fp_is_zero(p->z)) {
		r = 0;
	} else {
		RLC_TRY {
			ed_new(t);
			ed_norm(t, p);

			ed_rhs(t->z, t);
#if ED_ADD == EXTND
			fp_mul(t->y, t->x, t->y);
			r = ((fp_cmp_dig(t->z, 1) == RLC_EQ) &&
					(fp_cmp(t->y, t->t) == RLC_EQ)) || ed_is_infty(p);
#else
			r = (fp_cmp_dig(t->z, 1) == RLC_EQ) || ed_is_infty(p);
#endif
		}
		RLC_CATCH_ANY {
			RLC_THROW(ERR_CAUGHT);
		}
		RLC_FINALLY {
			ed_free(t);
		}
	}
	return r;
}

void ed_tab(ed_t * t, const ed_t p, int w) {
	if (w > 2) {
		ed_dbl(t[0], p);
#if defined(ED_MIXED)
		ed_norm(t[0], t[0]);
#endif
		ed_add(t[1], t[0], p);
		for (int i = 2; i < (1 << (w - 2)); i++) {
			ed_add(t[i], t[i - 1], t[0]);
		}
#if defined(ED_MIXED)
		ed_norm_sim(t + 1, (const ed_t *)t + 1, (1 << (w - 2)) - 1);
#endif
	}
	ed_copy(t[0], p);
}

void ed_print(const ed_t p) {
	fp_print(p->x);
	fp_print(p->y);
#if ED_ADD == EXTND
	fp_print(p->t);
#endif
	fp_print(p->z);
}

int ed_size_bin(const ed_t a, int pack) {
	int size = 0;

	if (ed_is_infty(a)) {
		return 1;
	}

	size = 1 + RLC_FP_BYTES;
	if (!pack) {
		size += RLC_FP_BYTES;
	}

	return size;
}

void ed_read_bin(ed_t a, const uint8_t *bin, int len) {
	if (len == 1) {
		if (bin[0] == 0) {
			ed_set_infty(a);
			return;
		} else {
			RLC_THROW(ERR_NO_BUFFER);
			return;
		}
	}

	if (len != (RLC_FP_BYTES + 1) && len != (2 * RLC_FP_BYTES + 1)) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	a->coord = BASIC;
	fp_set_dig(a->z, 1);
	fp_read_bin(a->y, bin + 1, RLC_FP_BYTES);
	if (len == RLC_FP_BYTES + 1) {
		switch (bin[0]) {
			case 2:
				fp_zero(a->x);
				break;
			case 3:
				fp_zero(a->x);
				fp_set_bit(a->x, 0, 1);
				break;
			default:
				RLC_THROW(ERR_NO_VALID);
				break;
		}
		ed_upk(a, a);
	}

	if (len == 2 * RLC_FP_BYTES + 1) {
		if (bin[0] == 4) {
			fp_read_bin(a->x, bin + RLC_FP_BYTES + 1, RLC_FP_BYTES);
		} else {
			RLC_THROW(ERR_NO_VALID);
			return;
		}
	}
#if ED_ADD == EXTND
	fp_mul(a->t, a->x, a->y);
	fp_mul(a->x, a->x, a->z);
	fp_mul(a->y, a->y, a->z);
	fp_sqr(a->z, a->z);
#endif

	if (!ed_on_curve(a)) {
		RLC_THROW(ERR_NO_VALID);
	}
}

void ed_write_bin(uint8_t *bin, int len, const ed_t a, int pack) {
	ed_t t;

	ed_null(t);

	memset(bin, 0, len);

	if (ed_is_infty(a)) {
		if (len < 1) {
			RLC_THROW(ERR_NO_BUFFER);
			return;
		} else {
			return;
		}
	}

	RLC_TRY {
		ed_new(t);

		ed_norm(t, a);

		if (pack) {
			if (len != RLC_FP_BYTES + 1) {
				RLC_THROW(ERR_NO_BUFFER);
			} else {
				ed_pck(t, t);
				bin[0] = 2 | fp_get_bit(t->x, 0);
				fp_write_bin(bin + 1, RLC_FP_BYTES, t->y);
			}
		} else {
			if (len != 2 * RLC_FP_BYTES + 1) {
				RLC_THROW(ERR_NO_BUFFER);
			} else {
				bin[0] = 4;
				fp_write_bin(bin + 1, RLC_FP_BYTES, t->y);
				fp_write_bin(bin + RLC_FP_BYTES + 1, RLC_FP_BYTES, t->x);
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t);
	}
}
