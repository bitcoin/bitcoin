/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2021 RELIC Authors
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
 * Implementation of comparison for points on prime elliptic curves over
 * quartic extensions.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int ep4_is_infty(ep4_t p) {
	return (fp4_is_zero(p->z) == 1);
}

void ep4_set_infty(ep4_t p) {
	fp4_zero(p->x);
	fp4_zero(p->y);
	fp4_zero(p->z);
	p->coord = BASIC;
}

void ep4_copy(ep4_t r, ep4_t p) {
	fp4_copy(r->x, p->x);
	fp4_copy(r->y, p->y);
	fp4_copy(r->z, p->z);
	r->coord = p->coord;
}

void ep4_rand(ep4_t p) {
	bn_t n, k;

	bn_null(k);
	bn_null(n);

	RLC_TRY {
		bn_new(k);
		bn_new(n);

		ep4_curve_get_ord(n);
		bn_rand_mod(k, n);

		ep4_mul_gen(p, k);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(k);
		bn_free(n);
	}
}

void ep4_blind(ep4_t r, ep4_t p) {
	fp4_t rand;

	fp4_null(rand);

	RLC_TRY {
		fp4_new(rand);
		fp4_rand(rand);
#if EP_ADD == BASIC
		(void)rand;
		ep4_copy(r, p);
#else
		fp4_mul(r->z, p->z, rand);
		fp4_mul(r->y, p->y, rand);
		fp4_sqr(rand, rand);
		fp4_mul(r->x, r->x, rand);
		fp4_mul(r->y, r->y, rand);
		r->coord = EP_ADD;
#endif
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(rand);
	}
}

void ep4_rhs(fp4_t rhs, ep4_t p) {
	fp4_t t0, t1;

	fp4_null(t0);
	fp4_null(t1);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);

		fp4_sqr(t0, p->x);                  /* x1^2 */

		switch (ep4_curve_opt_a()) {
			case RLC_ZERO:
				break;
#if FP_RDC != MONTY
			case RLC_MIN3:
				fp4_sub_dig(t0, t0, 3);
				break;
			case RLC_ONE:
				fp4_add_dig(t0, t0, 1);
				break;
			case RLC_TWO:
				fp4_add_dig(t0, t0, 2);
				break;
			case RLC_TINY:
				ep4_curve_get_a(t1);
				fp4_mul_dig(t0, t0, t1[0][0]);
				break;
#endif
			default:
				ep4_curve_get_a(t1);
				fp4_add(t0, t0, t1);
				break;
		}

		fp4_mul(t0, t0, p->x);				/* x1^3 + a * x */

		switch (ep4_curve_opt_b()) {
			case RLC_ZERO:
				break;
#if FP_RDC != MONTY
			case RLC_MIN3:
				fp4_sub_dig(t0, t0, 3);
				break;
			case RLC_ONE:
				fp4_add_dig(t0, t0, 1);
				break;
			case RLC_TWO:
				fp4_add_dig(t0, t0, 2);
				break;
			case RLC_TINY:
				ep4_curve_get_b(t1);
				fp4_mul_dig(t0, t0, t1[0][0]);
				break;
#endif
			default:
				ep4_curve_get_b(t1);
				fp4_add(t0, t0, t1);
				break;
		}

		fp4_copy(rhs, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
	}
}


int ep4_on_curve(ep4_t p) {
	ep4_t t;
	int r = 0;

	ep4_null(t);

	RLC_TRY {
		ep4_new(t);

		ep4_norm(t, p);

		ep4_rhs(t->x, t);
		fp4_sqr(t->y, t->y);

		r = (fp4_cmp(t->x, t->y) == RLC_EQ) || ep4_is_infty(p);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep4_free(t);
	}
	return r;
}

void ep4_tab(ep4_t *t, ep4_t p, int w) {
	if (w > 2) {
		ep4_dbl(t[0], p);
#if defined(EP_MIXED)
		ep4_norm(t[0], t[0]);
#endif
		ep4_add(t[1], t[0], p);
		for (int i = 2; i < (1 << (w - 2)); i++) {
			ep4_add(t[i], t[i - 1], t[0]);
		}
#if defined(EP_MIXED)
		ep4_norm_sim(t + 1, t + 1, (1 << (w - 2)) - 1);
#endif
	}
	ep4_copy(t[0], p);
}

void ep4_print(ep4_t p) {
	fp4_print(p->x);
	fp4_print(p->y);
	fp4_print(p->z);
}

int ep4_size_bin(ep4_t a, int pack) {
	ep4_t t;
	int size = 0;

	ep4_null(t);

	if (ep4_is_infty(a)) {
		return 1;
	}

	RLC_TRY {
		ep4_new(t);

		ep4_norm(t, a);

		size = 1 + 8 * RLC_FP_BYTES;
		//TODO: Implement compression.
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep4_free(t);
	}

	return size;
}

void ep4_read_bin(ep4_t a, const uint8_t *bin, int len) {
	if (len == 1) {
		if (bin[0] == 0) {
			ep4_set_infty(a);
			return;
		} else {
			RLC_THROW(ERR_NO_BUFFER);
			return;
		}
	}

	if (len != (8 * RLC_FP_BYTES + 1)) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	a->coord = BASIC;
	fp4_set_dig(a->z, 1);
	fp4_read_bin(a->x, bin + 1, 4 * RLC_FP_BYTES);

	if (len == 8 * RLC_FP_BYTES + 1) {
		if (bin[0] == 4) {
			fp4_read_bin(a->y, bin + 4 * RLC_FP_BYTES + 1, 4 * RLC_FP_BYTES);
		} else {
			RLC_THROW(ERR_NO_VALID);
			return;
		}
	}

	if (!ep4_on_curve(a)) {
		RLC_THROW(ERR_NO_VALID);
	}
}

void ep4_write_bin(uint8_t *bin, int len, ep4_t a, int pack) {
	ep4_t t;

	ep4_null(t);

	memset(bin, 0, len);

	if (ep4_is_infty(a)) {
		if (len < 1) {
			RLC_THROW(ERR_NO_BUFFER);
			return;
		} else {
			return;
		}
	}

	RLC_TRY {
		ep4_new(t);

		ep4_norm(t, a);

		if (len < 8 * RLC_FP_BYTES + 1) {
			RLC_THROW(ERR_NO_BUFFER);
		} else {
			bin[0] = 4;
			fp4_write_bin(bin + 1, 4 * RLC_FP_BYTES, t->x);
			fp4_write_bin(bin + 4 * RLC_FP_BYTES + 1, 4 * RLC_FP_BYTES, t->y);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep4_free(t);
	}
}
