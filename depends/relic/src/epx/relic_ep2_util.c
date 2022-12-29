/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
 * quadratic extensions.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int ep2_is_infty(ep2_t p) {
	return (fp2_is_zero(p->z) == 1);
}

void ep2_set_infty(ep2_t p) {
	fp2_zero(p->x);
	fp2_zero(p->y);
	fp2_zero(p->z);
	p->coord = BASIC;
}

void ep2_copy(ep2_t r, ep2_t p) {
	fp2_copy(r->x, p->x);
	fp2_copy(r->y, p->y);
	fp2_copy(r->z, p->z);
	r->coord = p->coord;
}

void ep2_rand(ep2_t p) {
	bn_t n, k;

	bn_null(k);
	bn_null(n);

	RLC_TRY {
		bn_new(k);
		bn_new(n);

		ep2_curve_get_ord(n);
		bn_rand_mod(k, n);

		ep2_mul_gen(p, k);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(k);
		bn_free(n);
	}
}

void ep2_blind(ep2_t r, ep2_t p) {
	fp2_t rand;

	fp2_null(rand);

	RLC_TRY {
		fp2_new(rand);
		fp2_rand(rand);
#if EP_ADD == BASIC
		(void)rand;
		ep2_copy(r, p);
#else
		fp2_mul(r->z, p->z, rand);
		fp2_mul(r->y, p->y, rand);
		fp2_sqr(rand, rand);
		fp2_mul(r->x, r->x, rand);
		fp2_mul(r->y, r->y, rand);
		r->coord = EP_ADD;
#endif
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(rand);
	}
}

void ep2_rhs(fp2_t rhs, ep2_t p) {
	fp2_t t0;

	fp2_null(t0);

	RLC_TRY {
		fp2_new(t0);

		fp2_sqr(t0, p->x);                  /* x1^2 */

		switch (ep2_curve_opt_a()) {
			case RLC_ZERO:
				break;
#if FP_RDC != MONTY
			case RLC_MIN3:
				fp2_sub_dig(t0, t0, 3);
				break;
			case RLC_ONE:
				fp2_add_dig(t0, t0, 1);
				break;
			case RLC_TWO:
				fp2_add_dig(t0, t0, 2);
				break;
			case RLC_TINY:
				fp2_mul_dig(t0, t0, ep2_curve_get_a()[0][0]);
				break;
#endif
			default:
				fp2_add(t0, t0, ep2_curve_get_a());
				break;
		}

		fp2_mul(t0, t0, p->x);				/* x1^3 + a * x */

		switch (ep2_curve_opt_b()) {
			case RLC_ZERO:
				break;
#if FP_RDC != MONTY
			case RLC_MIN3:
				fp2_sub_dig(t0, t0, 3);
				break;
			case RLC_ONE:
				fp2_add_dig(t0, t0, 1);
				break;
			case RLC_TWO:
				fp2_add_dig(t0, t0, 2);
				break;
			case RLC_TINY:
				fp2_mul_dig(t0, t0, ep2_curve_get_b()[0][0]);
				break;
#endif
			default:
				fp2_add(t0, t0, ep2_curve_get_b());
				break;
		}

		fp2_copy(rhs, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
	}
}


int ep2_on_curve(ep2_t p) {
	ep2_t t;
	int r = 0;

	ep2_null(t);

	RLC_TRY {
		ep2_new(t);

		ep2_norm(t, p);

		ep2_rhs(t->x, t);
		fp2_sqr(t->y, t->y);

		r = (fp2_cmp(t->x, t->y) == RLC_EQ) || ep2_is_infty(p);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep2_free(t);
	}
	return r;
}

void ep2_tab(ep2_t *t, ep2_t p, int w) {
	if (w > 2) {
		ep2_dbl(t[0], p);
#if defined(EP_MIXED)
		ep2_norm(t[0], t[0]);
#endif
		ep2_add(t[1], t[0], p);
		for (int i = 2; i < (1 << (w - 2)); i++) {
			ep2_add(t[i], t[i - 1], t[0]);
		}
#if defined(EP_MIXED)
		ep2_norm_sim(t + 1, t + 1, (1 << (w - 2)) - 1);
#endif
	}
	ep2_copy(t[0], p);
}

void ep2_print(ep2_t p) {
	fp2_print(p->x);
	fp2_print(p->y);
	fp2_print(p->z);
}

int ep2_size_bin(ep2_t a, int pack) {
	ep2_t t;
	int size = 0;

	ep2_null(t);

	if (ep2_is_infty(a)) {
		return 1;
	}

	RLC_TRY {
		ep2_new(t);

		ep2_norm(t, a);

		size = 1 + 2 * RLC_FP_BYTES;
		if (!pack) {
			size += 2 * RLC_FP_BYTES;
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep2_free(t);
	}

	return size;
}

void ep2_read_bin(ep2_t a, const uint8_t *bin, int len) {
	if (len == 1) {
		if (bin[0] == 0) {
			ep2_set_infty(a);
			return;
		} else {
			RLC_THROW(ERR_NO_BUFFER);
			return;
		}
	}

	if (len != (2 * RLC_FP_BYTES + 1) && len != (4 * RLC_FP_BYTES + 1)) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	a->coord = BASIC;
	fp2_set_dig(a->z, 1);
	fp2_read_bin(a->x, bin + 1, 2 * RLC_FP_BYTES);
	if (len == 2 * RLC_FP_BYTES + 1) {
		switch(bin[0]) {
			case 2:
				fp2_zero(a->y);
				break;
			case 3:
				fp2_zero(a->y);
				fp_set_bit(a->y[0], 0, 1);
				fp_zero(a->y[1]);
				break;
			default:
				RLC_THROW(ERR_NO_VALID);
				break;
		}
		ep2_upk(a, a);
	}

	if (len == 4 * RLC_FP_BYTES + 1) {
		if (bin[0] == 4) {
			fp2_read_bin(a->y, bin + 2 * RLC_FP_BYTES + 1, 2 * RLC_FP_BYTES);
		} else {
			RLC_THROW(ERR_NO_VALID);
			return;
		}
	}

	if (!ep2_on_curve(a)) {
		RLC_THROW(ERR_NO_VALID);
	}
}

void ep2_write_bin(uint8_t *bin, int len, ep2_t a, int pack) {
	ep2_t t;

	ep2_null(t);

	memset(bin, 0, len);

	if (ep2_is_infty(a)) {
		if (len < 1) {
			RLC_THROW(ERR_NO_BUFFER);
			return;
		} else {
			return;
		}
	}

	RLC_TRY {
		ep2_new(t);

		ep2_norm(t, a);

		if (pack) {
			if (len < 2 * RLC_FP_BYTES + 1) {
				RLC_THROW(ERR_NO_BUFFER);
			} else {
				ep2_pck(t, t);
				bin[0] = 2 | fp_get_bit(t->y[0], 0);
				fp2_write_bin(bin + 1, 2 * RLC_FP_BYTES, t->x, 0);
			}
		} else {
			if (len < 4 * RLC_FP_BYTES + 1) {
				RLC_THROW(ERR_NO_BUFFER);
			} else {
				bin[0] = 4;
				fp2_write_bin(bin + 1, 2 * RLC_FP_BYTES, t->x, 0);
				fp2_write_bin(bin + 2 * RLC_FP_BYTES + 1, 2 * RLC_FP_BYTES, t->y, 0);
			}
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep2_free(t);
	}
}
