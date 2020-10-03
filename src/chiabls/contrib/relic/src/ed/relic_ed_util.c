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
 * Implementation of the prime elliptic Edwards curve utilities.
 *
 * @version $Id$
 * @ingroup ed
 */

#include <assert.h>

#include "relic_core.h"
#include "relic_label.h"
#include "relic_md.h"

/*============================================================================*/
/* Private definitions                                                         */
/*============================================================================*/

static int ed_affine_is_valid(const fp_t x, const fp_t y) {
	fp_t tmpFP0;
	fp_t tmpFP1;
	fp_t tmpFP2;

	fp_null(tmpFP0);
	fp_null(tmpFP1);
	fp_null(tmpFP2);

	int r = 0;

	TRY {
		fp_new(tmpFP0);
		fp_new(tmpFP1);
		fp_new(tmpFP2);

		// a * X^2 + Y^2 - 1 - d * X^2 * Y^2 =?= 0
		fp_sqr(tmpFP0, x);
		fp_mul(tmpFP0, core_get()->ed_a, tmpFP0);
		fp_sqr(tmpFP1, y);
		fp_add(tmpFP1, tmpFP0, tmpFP1);
		fp_sub_dig(tmpFP1, tmpFP1, 1);
		fp_sqr(tmpFP0, x);
		fp_mul(tmpFP0, core_get()->ed_d, tmpFP0);
		fp_sqr(tmpFP2, y);
		fp_mul(tmpFP2, tmpFP0, tmpFP2);
		fp_sub(tmpFP0, tmpFP1, tmpFP2);

		r = fp_is_zero(tmpFP0);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp_free(tmpFP0);
		fp_free(tmpFP1);
		fp_free(tmpFP2);
	}
	return r;
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ed_rand(ed_t p) {
	bn_t n, k;

	bn_null(k);
	bn_null(n);

	TRY {
		bn_new(k);
		bn_new(n);

		ed_curve_get_ord(n);

		bn_rand_mod(k, n);

		ed_mul_gen(p, k);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(k);
		bn_free(n);
	}
}

void ed_curve_get_gen(ed_t g) {
	ed_copy(g, &core_get()->ed_g);
}

void ed_curve_get_ord(bn_t n) {
	bn_copy(n, &core_get()->ed_r);
}

void ed_curve_get_cof(bn_t h) {
	bn_copy(h, &core_get()->ed_h);
}

const ed_t *ed_curve_get_tab(void) {
#if defined(ED_PRECO)

	/* Return a meaningful pointer. */
#if ALLOC == AUTO
	return (const ed_t *)*core_get()->ed_ptr;
#else
	return (const ed_t *)core_get()->ed_ptr;
#endif

#else
	/* Return a null pointer. */
	return NULL;
#endif
}

#if ED_ADD == EXTND
void ed_projc_to_extnd(ed_t r, const fp_t x, const fp_t y, const fp_t z) {
	fp_mul(r->t, x, y);
	fp_mul(r->x, x, z);
	fp_mul(r->y, y, z);
	fp_sqr(r->z, z);
}
#endif

void ed_copy(ed_t r, const ed_t p) {
#if ED_ADD == PROJC || ED_ADD == EXTND
	fp_copy(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_copy(r->z, p->z);
#endif
#if ED_ADD == EXTND
	fp_copy(r->t, p->t);
#endif

	r->norm = p->norm;
}

int ed_cmp(const ed_t p, const ed_t q) {
	int ret = CMP_NE;

	if (fp_cmp(p->x, q->x) != CMP_EQ) {
		ret = CMP_NE;
	} else if (fp_cmp(p->y, q->y) != CMP_EQ) {
		ret = CMP_NE;
	} else if (fp_cmp(p->z, q->z) != CMP_EQ) {
		ret = CMP_NE;
#if ED_ADD == EXTND
	} else if (fp_cmp(p->t, q->t) != CMP_EQ) {
		ret = CMP_NE;
#endif
	} else {
		ret = CMP_EQ;
	}

	return ret;
}

void ed_set_infty(ed_t p) {
#if ED_ADD == PROJC
	fp_zero(p->x);
	fp_set_dig(p->y, 1);
	fp_set_dig(p->z, 1);
#elif ED_ADD == EXTND
	fp_zero(p->x);
	fp_set_dig(p->y, 1);
	fp_zero(p->t);
	fp_set_dig(p->z, 1);
#endif
	p->norm = 0;
}

int ed_is_infty(const ed_t p) {
	assert(!fp_is_zero(p->z));
	int ret = 0;
	fp_t norm_y;

	fp_null(norm_y);
	fp_new(norm_y);

	fp_inv(norm_y, p->z);
	fp_mul(norm_y, p->y, norm_y);

	if (fp_cmp_dig(norm_y, 1) == CMP_EQ && fp_is_zero(p->x)) {
		ret = 1;
	}

	fp_free(norm_y);
	return ret;
}

void ed_neg(ed_t r, const ed_t p) {
#if ED_ADD == PROJC
	fp_neg(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_copy(r->z, p->z);
#elif ED_ADD == EXTND
	fp_neg(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_neg(r->t, p->t);
	fp_copy(r->z, p->z);
#endif
}

/**
* Normalizes a point represented in projective coordinates.
*
* @param r			- the result.
* @param p			- the point to normalize.
*/
void ed_norm(ed_t r, const ed_t p) {
	if (ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	if (fp_cmp_dig(p->z, 1) == CMP_EQ) {
		/* If the point is represented in affine coordinates, we just copy it. */
		ed_copy(r, p);
	} else {
		fp_t z_inv;

		fp_null(z_inv);
		fp_new(z_inv);

		fp_inv(z_inv, p->z);

		fp_mul(r->x, p->x, z_inv);
		fp_mul(r->y, p->y, z_inv);
#if ED_ADD == EXTND
		fp_mul(r->t, p->t, z_inv);
#endif

		fp_set_dig(r->z, 1);

		fp_free(z_inv);
	}
}

void ed_norm_sim(ed_t * r, const ed_t * t, int n) {
	int i;
	fp_t a[n];

	for (i = 0; i < n; i++) {
		fp_null(a[i]);
	}

	TRY {
		for (i = 0; i < n; i++) {
			fp_new(a[i]);
			fp_copy(a[i], t[i]->z);
		}

		fp_inv_sim(a, (const fp_t *)a, n);

		for (i = 0; i < n; i++) {
			fp_mul(r[i]->x, t[i]->x, a[i]);
			fp_mul(r[i]->y, t[i]->y, a[i]);
#if ED_ADD == EXTND
			fp_mul(r[i]->t, t[i]->t, a[i]);
#endif
			fp_set_dig(r[i]->z, 1);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < n; i++) {
			fp_free(a[i]);
		}
	}
}

void ed_print(const ed_t p) {
	fp_print(p->x);
	fp_print(p->y);
#if ED_ADD == EXTND
	fp_print(p->t);
#endif
	fp_print(p->z);
}

int ed_is_valid(const ed_t p) {
	ed_t t;
#if ED_ADD == EXTND
	fp_t x_times_y;
#endif
	int r = 0;

	ed_null(t);
#if ED_ADD == EXTND
	fp_null(x_times_y);
#endif

	if (fp_is_zero(p->z)) {
		r = 0;
	} else {
		TRY {
#if ED_ADD == EXTND
			fp_new(x_times_y);
#endif
			ed_new(t);
			ed_norm(t, p);

			// check t coordinate
#if ED_ADD == PROJC
			r = ed_affine_is_valid(t->x, t->y);
#elif ED_ADD == EXTND
			fp_mul(x_times_y, t->x, t->y);
			if (fp_cmp(x_times_y, t->t) != CMP_EQ) {
				r = 0;
			} else {
				r = ed_affine_is_valid(t->x, t->y);
			}
#endif
		}
		CATCH_ANY {
			THROW(ERR_CAUGHT);
		}
		FINALLY {
#if ED_ADD == EXTND
			fp_free(x_times_y);
#endif
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

int ed_size_bin(const ed_t a, int pack) {
	int size = 0;

	if (ed_is_infty(a)) {
		return 1;
	}

	size = 1 + FP_BYTES;
	if (!pack) {
		size += FP_BYTES;
	}

	return size;
}

void ed_write_bin(uint8_t *bin, int len, const ed_t a, int pack) {
	ed_t t;

	ed_null(t);

	if (ed_is_infty(a)) {
		if (len != 1) {
			THROW(ERR_NO_BUFFER);
		} else {
			bin[0] = 0;
			return;
		}
	}

	TRY {
		ed_new(t);

		ed_norm(t, a);

		if (pack) {
			if (len != FP_BYTES + 1) {
				THROW(ERR_NO_BUFFER);
			} else {
				ed_pck(t, t);
				bin[0] = 2 | fp_get_bit(t->x, 0);
				fp_write_bin(bin + 1, FP_BYTES, t->y);
			}
		} else {
			if (len != 2 * FP_BYTES + 1) {
				THROW(ERR_NO_BUFFER);
			} else {
				bin[0] = 4;
				fp_write_bin(bin + 1, FP_BYTES, t->y);
				fp_write_bin(bin + FP_BYTES + 1, FP_BYTES, t->x);
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ed_free(t);
	}
}

void ed_read_bin(ed_t a, const uint8_t *bin, int len) {
	if (len == 1) {
		if (bin[0] == 0) {
			ed_set_infty(a);
			return;
		} else {
			THROW(ERR_NO_BUFFER);
			return;
		}
	}

	if (len != (FP_BYTES + 1) && len != (2 * FP_BYTES + 1)) {
		THROW(ERR_NO_BUFFER);
		return;
	}

	a->norm = 1;
	fp_set_dig(a->z, 1);
	fp_read_bin(a->y, bin + 1, FP_BYTES);
	if (len == FP_BYTES + 1) {
		switch (bin[0]) {
			case 2:
				fp_zero(a->x);
				break;
			case 3:
				fp_zero(a->x);
				fp_set_bit(a->x, 0, 1);
				break;
			default:
				THROW(ERR_NO_VALID);
				break;
		}
		ed_upk(a, a);
	}

	if (len == 2 * FP_BYTES + 1) {
		if (bin[0] == 4) {
			fp_read_bin(a->x, bin + FP_BYTES + 1, FP_BYTES);
		} else {
			THROW(ERR_NO_VALID);
		}
	}
#if ED_ADD == EXTND
	ed_projc_to_extnd(a, a->x, a->y, a->z);
#endif
}
