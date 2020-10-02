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
	p->norm = 1;
}

void ep_copy(ep_t r, const ep_t p) {
	fp_copy(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_copy(r->z, p->z);
	r->norm = p->norm;
}

int ep_cmp(const ep_t p, const ep_t q) {
    ep_t r, s;
    int result = CMP_EQ;

    ep_null(r);
    ep_null(s);

    TRY {
        ep_new(r);
        ep_new(s);

        if ((!p->norm) && (!q->norm)) {
            /* If the two points are not normalized, it is faster to compare
             * x1 * z2^2 == x2 * z1^2 and y1 * z2^3 == y2 * z1^3. */
            fp_sqr(r->z, p->z);
            fp_sqr(s->z, q->z);
            fp_mul(r->x, p->x, s->z);
            fp_mul(s->x, q->x, r->z);
            fp_mul(r->z, r->z, p->z);
            fp_mul(s->z, s->z, q->z);
            fp_mul(r->y, p->y, s->z);
            fp_mul(s->y, q->y, r->z);
        } else {
            if (!p->norm) {
                ep_norm(r, p);
            } else {
                ep_copy(r, p);
            }

            if (!q->norm) {
                ep_norm(s, q);
            } else {
                ep_copy(s, q);
            }
        }

        if (fp_cmp(r->x, s->x) != CMP_EQ) {
            result = CMP_NE;
        }

        if (fp_cmp(r->y, s->y) != CMP_EQ) {
            result = CMP_NE;
        }
    } CATCH_ANY {
        THROW(ERR_CAUGHT);
    } FINALLY {
        ep_free(r);
        ep_free(s);
    }

    return result;
}

void ep_rand(ep_t p) {
	bn_t n, k;

	bn_null(k);
	bn_null(n);

	TRY {
		bn_new(k);
		bn_new(n);

		ep_curve_get_ord(n);
		bn_rand_mod(k, n);

		ep_mul_gen(p, k);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(k);
		bn_free(n);
	}
}

void ep_rhs(fp_t rhs, const ep_t p) {
	fp_t t0;
	fp_t t1;

	fp_null(t0);
	fp_null(t1);

	TRY {
		fp_new(t0);
		fp_new(t1);

		/* t0 = x1^2. */
		fp_sqr(t0, p->x);
		/* t1 = x1^3. */
		fp_mul(t1, t0, p->x);

		/* t1 = x1^3 + a * x1 + b. */
		switch (ep_curve_opt_a()) {
			case OPT_ZERO:
				break;
			case OPT_ONE:
				fp_add(t1, t1, p->x);
				break;
#if FP_RDC != MONTY
			case OPT_DIGIT:
				fp_mul_dig(t0, p->x, ep_curve_get_a()[0]);
				fp_add(t1, t1, t0);
				break;
#endif
			default:
				fp_mul(t0, p->x, ep_curve_get_a());
				fp_add(t1, t1, t0);
				break;
		}

		switch (ep_curve_opt_b()) {
			case OPT_ZERO:
				break;
			case OPT_ONE:
				fp_add_dig(t1, t1, 1);
				break;
#if FP_RDC != MONTY
			case OPT_DIGIT:
				fp_add_dig(t1, t1, ep_curve_get_b()[0]);
				break;
#endif
			default:
				fp_add(t1, t1, ep_curve_get_b());
				break;
		}

		fp_copy(rhs, t1);

	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp_free(t0);
		fp_free(t1);
	}
}

int ep_is_valid(const ep_t p) {
	ep_t t;
	int r = 0;

	ep_null(t);

	TRY {
		ep_new(t);

		ep_norm(t, p);

		ep_rhs(t->x, t);
		fp_sqr(t->y, t->y);
		r = (fp_cmp(t->x, t->y) == CMP_EQ) || ep_is_infty(p);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
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
	ep_t t;
	int size = 0;

	ep_null(t);

	if (ep_is_infty(a)) {
		return 1;
	}

	TRY {
		ep_new(t);

		ep_norm(t, a);

		size = 1 + FP_BYTES;
		if (!pack) {
			size += FP_BYTES;
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		ep_free(t);
	}

	return size;
}

void ep_read_bin(ep_t a, const uint8_t *bin, int len) {
	if (len == 1) {
		if (bin[0] == 0) {
			ep_set_infty(a);
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
	fp_read_bin(a->x, bin + 1, FP_BYTES);
	if (len == FP_BYTES + 1) {
		switch(bin[0]) {
			case 2:
				fp_zero(a->y);
				break;
			case 3:
				fp_zero(a->y);
				fp_set_bit(a->y, 0, 1);
				break;
			default:
				THROW(ERR_NO_VALID);
				break;
		}
		if (!ep_upk(a, a)) {
			THROW(ERR_NO_VALID);
		}
	}

	if (len == 2 * FP_BYTES + 1) {
		if (bin[0] == 4) {
			fp_read_bin(a->y, bin + FP_BYTES + 1, FP_BYTES);
		} else {
			THROW(ERR_NO_VALID);
		}
	}
}

void ep_write_bin(uint8_t *bin, int len, const ep_t a, int pack) {
	ep_t t;

	ep_null(t);

	if (ep_is_infty(a)) {
		if (len != 1) {
			THROW(ERR_NO_BUFFER);
		} else {
			bin[0] = 0;
			return;
		}
	}

	TRY {
		ep_new(t);

		ep_norm(t, a);

		if (pack) {
			if (len < FP_BYTES + 1) {
				THROW(ERR_NO_BUFFER);
			} else {
				ep_pck(t, t);
				bin[0] = 2 | fp_get_bit(t->y, 0);
				fp_write_bin(bin + 1, FP_BYTES, t->x);
			}
		} else {
			if (len < 2 * FP_BYTES + 1) {
				THROW(ERR_NO_BUFFER);
			} else {
				bin[0] = 4;
				fp_write_bin(bin + 1, FP_BYTES, t->x);
				fp_write_bin(bin + FP_BYTES + 1, FP_BYTES, t->y);
			}
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(t);
	}
}
