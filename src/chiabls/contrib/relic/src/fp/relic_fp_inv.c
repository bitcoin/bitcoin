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
 * Implementation of the prime field inversion functions.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FP_INV == BASIC || !defined(STRIP)

void fp_inv_basic(fp_t c, const fp_t a) {
	bn_t e;

	bn_null(e);

	TRY {
		bn_new(e);

		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		bn_sub_dig(e, e, 2);

		fp_exp(c, a, e);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(e);
	}
}

#endif

#if FP_INV == BINAR || !defined(STRIP)

void fp_inv_binar(fp_t c, const fp_t a) {
	bn_t u, v, g1, g2, p;

	bn_null(u);
	bn_null(v);
	bn_null(g1);
	bn_null(g2);
	bn_null(p);

	TRY {
		bn_new(u);
		bn_new(v);
		bn_new(g1);
		bn_new(g2);
		bn_new(p);

		/* u = a, v = p, g1 = 1, g2 = 0. */
		fp_prime_back(u, a);
		p->used = FP_DIGS;
		dv_copy(p->dp, fp_prime_get(), FP_DIGS);
		bn_copy(v, p);
		bn_set_dig(g1, 1);
		bn_zero(g2);

		/* While (u != 1 && v != 1). */
		while (1) {
			/* While u is even do. */
			while (!(u->dp[0] & 1)) {
				/* u = u/2. */
				fp_rsh1_low(u->dp, u->dp);
				/* If g1 is even then g1 = g1/2; else g1 = (g1 + p)/2. */
				if (g1->dp[0] & 1) {
					bn_add(g1, g1, p);
				}
				bn_hlv(g1, g1);
			}

			while (u->dp[u->used - 1] == 0) {
				u->used--;
			}
			if (u->used == 1 && u->dp[0] == 1)
				break;

			/* While z divides v do. */
			while (!(v->dp[0] & 1)) {
				/* v = v/2. */
				fp_rsh1_low(v->dp, v->dp);
				/* If g2 is even then g2 = g2/2; else (g2 = g2 + p)/2. */
				if (g2->dp[0] & 1) {
					bn_add(g2, g2, p);
				}
				bn_hlv(g2, g2);
			}

			while (v->dp[v->used - 1] == 0) {
				v->used--;
			}
			if (v->used == 1 && v->dp[0] == 1)
				break;

			/* If u > v then u = u - v, g1 = g1 - g2. */
			if (bn_cmp(u, v) == CMP_GT) {
				bn_sub(u, u, v);
				bn_sub(g1, g1, g2);
			} else {
				bn_sub(v, v, u);
				bn_sub(g2, g2, g1);
			}
		}
		/* If u == 1 then return g1; else return g2. */
		if (bn_cmp_dig(u, 1) == CMP_EQ) {
			while (bn_sign(g1) == BN_NEG) {
				bn_add(g1, g1, p);
			}
			while (bn_cmp(g1, p) != CMP_LT) {
				bn_sub(g1, g1, p);
			}
#if FP_RDC == MONTY
			fp_prime_conv(c, g1);
#else
			dv_copy(c, g1->dp, FP_DIGS);
#endif
		} else {
			while (bn_sign(g2) == BN_NEG) {
				bn_add(g2, g2, p);
			}
			while (bn_cmp(g2, p) != CMP_LT) {
				bn_sub(g2, g2, p);
			}
#if FP_RDC == MONTY
			fp_prime_conv(c, g2);
#else
			dv_copy(c, g2->dp, FP_DIGS);
#endif
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(u);
		bn_free(v);
		bn_free(g1);
		bn_free(g2);
		bn_free(p);
	}
}

#endif

#if FP_INV == MONTY || !defined(STRIP)

void fp_inv_monty(fp_t c, const fp_t a) {
	bn_t _a, _p, u, v, x1, x2;
	const dig_t *p = NULL;
	dig_t carry;
	int i, k, flag = 0;

	bn_null(_a);
	bn_null(_p);
	bn_null(u);
	bn_null(v);
	bn_null(x1);
	bn_null(x2);

	TRY {
		bn_new(_a);
		bn_new(_p);
		bn_new(u);
		bn_new(v);
		bn_new(x1);
		bn_new(x2);

		p = fp_prime_get();

		/* u = a, v = p, x1 = 1, x2 = 0, k = 0. */
		k = 0;
		bn_set_dig(x1, 1);
		bn_zero(x2);

#if FP_RDC != MONTY
		bn_read_raw(_a, a, FP_DIGS);
		bn_read_raw(_p, p, FP_DIGS);
		bn_mod_monty_conv(u, _a, _p);
#else
		bn_read_raw(u, a, FP_DIGS);
#endif
		bn_read_raw(v, p, FP_DIGS);

		while (!bn_is_zero(v)) {
			/* If v is even then v = v/2, x1 = 2 * x1. */
			if (!(v->dp[0] & 1)) {
				fp_rsh1_low(v->dp, v->dp);
				bn_dbl(x1, x1);
			} else {
				/* If u is even then u = u/2, x2 = 2 * x2. */
				if (!(u->dp[0] & 1)) {
					fp_rsh1_low(u->dp, u->dp);
					bn_dbl(x2, x2);
					/* If v >= u,then v = (v - u)/2, x2 += x1, x1 = 2 * x1. */
				} else {
					if (bn_cmp(v, u) != CMP_LT) {
						fp_subn_low(v->dp, v->dp, u->dp);
						fp_rsh1_low(v->dp, v->dp);
						bn_add(x2, x2, x1);
						bn_dbl(x1, x1);
					} else {
						/* u = (u - v)/2, x1 += x2, x2 = 2 * x2. */
						fp_subn_low(u->dp, u->dp, v->dp);
						fp_rsh1_low(u->dp, u->dp);
						bn_add(x1, x1, x2);
						bn_dbl(x2, x2);
					}
				}
			}
			bn_trim(u);
			bn_trim(v);
			k++;
		}

		/* If x1 > p then x1 = x1 - p. */
		for (i = x1->used; i < FP_DIGS; i++) {
			x1->dp[i] = 0;
		}

		while (x1->used > FP_DIGS) {
			carry = bn_subn_low(x1->dp, x1->dp, fp_prime_get(), FP_DIGS);
			bn_sub1_low(x1->dp + FP_DIGS, x1->dp + FP_DIGS, carry,
					x1->used - FP_DIGS);
			bn_trim(x1);
		}
		if (fp_cmpn_low(x1->dp, fp_prime_get()) == CMP_GT) {
			fp_subn_low(x1->dp, x1->dp, fp_prime_get());
		}

		dv_copy(x2->dp, fp_prime_get_conv(), FP_DIGS);

		/* If k < Wt then x1 = x1 * R^2 * R^{-1} mod p. */
		if (k <= FP_DIGS * FP_DIGIT) {
			flag = 1;
			fp_mul(x1->dp, x1->dp, x2->dp);
			k = k + FP_DIGS * FP_DIGIT;
		}

		/* x1 = x1 * R^2 * R^{-1} mod p. */
		fp_mul(x1->dp, x1->dp, x2->dp);

		/* c = x1 * 2^(2Wt - k) * R^{-1} mod p. */
		fp_copy(c, x1->dp);
		dv_zero(x1->dp, FP_DIGS);
		bn_set_2b(x1, 2 * FP_DIGS * FP_DIGIT - k);
		fp_mul(c, c, x1->dp);

#if FP_RDC != MONTY
		/*
		 * If we do not use Montgomery reduction, the result of inversion is
		 * a^{-1}R^3 mod p or a^{-1}R^4 mod p, depending on flag.
		 * Hence we must reduce the result three or four times.
		 */
		_a->used = FP_DIGS;
		dv_copy(_a->dp, c, FP_DIGS);
		bn_mod_monty_back(_a, _a, _p);
		bn_mod_monty_back(_a, _a, _p);
		bn_mod_monty_back(_a, _a, _p);

		if (flag) {
			bn_mod_monty_back(_a, _a, _p);
		}
		fp_zero(c);
		dv_copy(c, _a->dp, _a->used);
#endif
		(void)flag;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(_a);
		bn_free(_p);
		bn_free(u);
		bn_free(v);
		bn_free(x1);
		bn_free(x2);
	}
}

#endif

#if FP_INV == EXGCD || !defined(STRIP)

void fp_inv_exgcd(fp_t c, const fp_t a) {
	bn_t u, v, g1, g2, p, q, r;

	bn_null(u);
	bn_null(v);
	bn_null(g1);
	bn_null(g2);
	bn_null(p);
	bn_null(q);
	bn_null(r);

	TRY {
		bn_new(u);
		bn_new(v);
		bn_new(g1);
		bn_new(g2);
		bn_new(p);
		bn_new(q);
		bn_new(r);

		/* u = a, v = p, g1 = 1, g2 = 0. */
		fp_prime_back(u, a);
		p->used = FP_DIGS;
		dv_copy(p->dp, fp_prime_get(), FP_DIGS);
		bn_copy(v, p);
		bn_set_dig(g1, 1);
		bn_zero(g2);

		/* While (u != 1. */
		while (bn_cmp_dig(u, 1) != CMP_EQ) {
			/* q = [v/u], r = v mod u. */
			bn_div_rem(q, r, v, u);
			/* v = u, u = r. */
			bn_copy(v, u);
			bn_copy(u, r);
			/* r = g2 - q * g1. */
			bn_mul(r, q, g1);
			bn_sub(r, g2, r);
			/* g2 = g1, g1 = r. */
			bn_copy(g2, g1);
			bn_copy(g1, r);
		}

		if (bn_sign(g1) == BN_NEG) {
			bn_add(g1, g1, p);
		}
#if FP_RDC == MONTY
		fp_prime_conv(c, g1);
#else
		dv_copy(c, g1->dp, FP_DIGS);
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(u);
		bn_free(v);
		bn_free(g1);
		bn_free(g2);
		bn_free(p);
		bn_free(q);
		bn_free(r);
	}
}

#endif

#if FP_INV == LOWER || !defined(STRIP)

void fp_inv_lower(fp_t c, const fp_t a) {
	fp_invn_low(c, a);
}

#endif

void fp_inv_sim(fp_t *c, const fp_t *a, int n) {
	int i;
	fp_t u, t[n];

	for (i = 0; i < n; i++) {
		fp_null(t[i]);
	}
	fp_null(u);

	TRY {
		for (i = 0; i < n; i++) {
			fp_new(t[i]);
		}
		fp_new(u);

		fp_copy(c[0], a[0]);
		fp_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fp_copy(t[i], a[i]);
			fp_mul(c[i], c[i - 1], a[i]);
		}

		fp_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fp_mul(c[i], u, c[i - 1]);
			fp_mul(u, u, t[i]);
		}
		fp_copy(c[0], u);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < n; i++) {
			fp_free(t[i]);
		}
		fp_free(u);
	}
}


void fp_inv_exgcd_bn(bn_t c, const bn_t u, const bn_t p) {
	bn_t v, g1, g2, q, r;

	bn_null(v);
	bn_null(g1);
	bn_null(g2);
	bn_null(q);
	bn_null(r);

	TRY {
		bn_new(v);
		bn_new(g1);
		bn_new(g2);
		bn_new(q);
		bn_new(r);

		/* u = a, v = p, g1 = 1, g2 = 0. */
		bn_copy(v, p);
		bn_set_dig(g1, 1);
		bn_zero(g2);

		/* While (u != 1. */
		while (bn_cmp_dig(u, 1) != CMP_EQ) {
			/* q = [v/u], r = v mod u. */
			bn_div_rem(q, r, v, u);
			/* v = u, u = r. */
			bn_copy(v, u);
			bn_copy(u, r);
			/* r = g2 - q * g1. */
			bn_mul(r, q, g1);
			bn_sub(r, g2, r);
			/* g2 = g1, g1 = r. */
			bn_copy(g2, g1);
			bn_copy(g1, r);
		}

		if (bn_sign(g1) == BN_NEG) {
			bn_add(g1, g1, p);
		}
		bn_copy(c, g1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(v);
		bn_free(g1);
		bn_free(g2);
		bn_free(q);
		bn_free(r);
	};
}