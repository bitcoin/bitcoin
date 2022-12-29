/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Implementation of the prime elliptic curve utilities.
 *
 * @version $Id$
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int ep_is_infty(const ep_t p) {
	return (fp_is_zero(p->z) == 1);
}

void ep_set_infty(ep_t p) {
	fp_zero(p->x);
	fp_zero(p->y);
	fp_zero(p->z);
	p->coord = BASIC;
}

void ep_copy(ep_t r, const ep_t p) {
	fp_copy(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_copy(r->z, p->z);
	r->coord = p->coord;
}

void ep_rand(ep_t p) {
	bn_t n, k;

	bn_null(k);
	bn_null(n);

	RLC_TRY {
		bn_new(k);
		bn_new(n);

		ep_curve_get_ord(n);
		bn_rand_mod(k, n);

		ep_mul_gen(p, k);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(k);
		bn_free(n);
	}
}

void ep_blind(ep_t r, const ep_t p) {
	fp_t rand;

	fp_null(rand);

	RLC_TRY {
		fp_new(rand);
		fp_rand(rand);
#if EP_ADD == BASIC
		(void)rand;
		ep_copy(r, p);
#elif EP_ADD == PROJC
		fp_mul(r->x, p->x, rand);
		fp_mul(r->y, p->y, rand);
		fp_mul(r->z, p->z, rand);
		r->coord = PROJC;
#elif EP_ADD == JACOB
		fp_mul(r->z, p->z, rand);
		fp_mul(r->y, p->y, rand);
		fp_sqr(rand, rand);
		fp_mul(r->x, r->x, rand);
		fp_mul(r->y, r->y, rand);
		r->coord = JACOB;
#endif
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(rand);
	}
}

void ep_rhs(fp_t rhs, const ep_t p) {
	fp_t t0;

	fp_null(t0);

	RLC_TRY {
		fp_new(t0);

		/* t0 = x1^2. */
		fp_sqr(t0, p->x);

		/* t0 = x1^2 + a */
		switch (ep_curve_opt_a()) {
			case RLC_ZERO:
				break;
#if FP_RDC != MONTY
			case RLC_MIN3:
				fp_sub_dig(t0, t0, 3);
				break;
			case RLC_ONE:
				fp_add_dig(t0, t0, 1);
				break;
			case RLC_TWO:
				fp_add_dig(t0, t0, 2);
				break;
			case RLC_TINY:
				fp_add_dig(t0, t0, ep_curve_get_a()[0]);
				break;
#endif
			default:
				fp_add(t0, t0, ep_curve_get_a());
				break;
		}

		/* t0 = x1^3 + a * x */
		fp_mul(t0, t0, p->x);

		/* t0 = x1^3 + a * x + b */
		switch (ep_curve_opt_b()) {
			case RLC_ZERO:
				break;
#if FP_RDC != MONTY
			case RLC_MIN3:
				fp_sub_dig(t0, t0, 3);
				break;
			case RLC_ONE:
				fp_add_dig(t0, t0, 1);
				break;
			case RLC_TWO:
				fp_add_dig(t0, t0, 2);
				break;
			case RLC_TINY:
				fp_add_dig(t0, t0, ep_curve_get_b()[0]);
				break;
#endif
			default:
				fp_add(t0, t0, ep_curve_get_b());
				break;
		}

		fp_copy(rhs, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(t0);
	}
}

int ep_on_curve(const ep_t p) {
	ep_t t;
	int r = 0;

	ep_null(t);

	RLC_TRY {
		ep_new(t);

		ep_norm(t, p);
		ep_rhs(t->x, t);
		fp_sqr(t->y, t->y);
		r = (fp_cmp(t->x, t->y) == RLC_EQ) || ep_is_infty(p);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		ep_free(t);
	}
	return r;
}

void ep_tab(ep_t *t, const ep_t p, int w) {
	if (w > 2) {
		ep_dbl(t[0], p);
#if defined(EP_MIXED)
		ep_norm(t[0], t[0]);
#endif
		ep_add(t[1], t[0], p);
		for (int i = 2; i < (1 << (w - 2)); i++) {
			ep_add(t[i], t[i - 1], t[0]);
		}
#if defined(EP_MIXED)
		ep_norm_sim(t + 1, (const ep_t *)t + 1, (1 << (w - 2)) - 1);
#endif
	}
	ep_copy(t[0], p);
}

void ep_print(const ep_t p) {
	fp_print(p->x);
	fp_print(p->y);
	fp_print(p->z);
}

int ep_size_bin(const ep_t a, int pack) {
	int size = 0;

	if (ep_is_infty(a)) {
		return 1;
	}

	size = 1 + RLC_FP_BYTES;
	if (!pack) {
		size += RLC_FP_BYTES;
	}

	return size;
}

void ep_read_bin(ep_t a, const uint8_t *bin, int len) {
	if (len == 1) {
		if (bin[0] == 0) {
			ep_set_infty(a);
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
	fp_read_bin(a->x, bin + 1, RLC_FP_BYTES);
	if (len == RLC_FP_BYTES + 1) {
		switch(bin[0]) {
			case 2:
				fp_zero(a->y);
				break;
			case 3:
				fp_zero(a->y);
				fp_set_bit(a->y, 0, 1);
				break;
			default:
				RLC_THROW(ERR_NO_VALID);
				break;
		}
		ep_upk(a, a);
	}

	if (len == 2 * RLC_FP_BYTES + 1) {
		if (bin[0] == 4) {
			fp_read_bin(a->y, bin + RLC_FP_BYTES + 1, RLC_FP_BYTES);
		} else {
			RLC_THROW(ERR_NO_VALID);
			return;
		}
	}

	if (!ep_on_curve(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}
}

void ep_write_bin(uint8_t *bin, int len, const ep_t a, int pack) {
	ep_t t;

	ep_null(t);

	memset(bin, 0, len);

	if (ep_is_infty(a)) {
		if (len < 1) {
			RLC_THROW(ERR_NO_BUFFER);
			return;
		} else {
			bin[0] = 0;
			return;
		}
	}

	RLC_TRY {
		ep_new(t);

		ep_norm(t, a);

		if (pack) {
			if (len < RLC_FP_BYTES + 1) {
				RLC_THROW(ERR_NO_BUFFER);
			} else {
				ep_pck(t, t);
				bin[0] = 2 | fp_get_bit(t->y, 0);
				fp_write_bin(bin + 1, RLC_FP_BYTES, t->x);
			}
		} else {
			if (len < 2 * RLC_FP_BYTES + 1) {
				RLC_THROW(ERR_NO_BUFFER);
			} else {
				bin[0] = 4;
				fp_write_bin(bin + 1, RLC_FP_BYTES, t->x);
				fp_write_bin(bin + RLC_FP_BYTES + 1, RLC_FP_BYTES, t->y);
			}
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(t);
	}
}
