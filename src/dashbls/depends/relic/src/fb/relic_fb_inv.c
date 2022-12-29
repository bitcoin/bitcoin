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
 * Implementation of binary field inversion functions.
 *
 * @ingroup fb
 */

#include "relic_core.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"
#include "relic_rand.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FB_INV == BASIC || !defined(STRIP)

void fb_inv_basic(fb_t c, const fb_t a) {
	fb_t t, u, v;
	int i, x;

	fb_null(t);
	fb_null(u);
	fb_null(v);

	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	RLC_TRY {
		fb_new(t);
		fb_new(u);
		fb_new(v);

#if (FB_POLYN % 2) == 0
		fb_sqr(v, a);
		for (i = 2; i < RLC_FB_BITS; i++) {
			fb_sqr(u, a);
			for (int j = 1; j < i; j++) {
				fb_sqr(u, u);
			}
			fb_mul(v, v, u);
		}
		fb_copy(c, v);
#else
		/* u = a^2, v = 1, x = (m - 1)/2. */
		fb_sqr(u, a);
		fb_set_dig(v, 1);
		x = (RLC_FB_BITS - 1) >> 1;

		while (x != 0) {
			/* u = u * a^{2x}. */
			fb_copy(t, u);
			for (i = 0; i < x; i++) {
				fb_sqr(t, t);
			}
			fb_mul(u, u, t);
			if ((x & 0x01) == 0) {
				x = x >> 1;
			} else {
				/* v = v * u, u = u^2, x = (x - 1)/2. */
				fb_mul(v, v, u);
				fb_sqr(u, u);
				x = (x - 1) >> 1;
			}
		}
#endif
		fb_copy(c, v);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fb_free(t);
		fb_free(u);
		fb_free(v);
	}
}

#endif

#if FB_INV == BINAR || !defined(STRIP)

void fb_inv_binar(fb_t c, const fb_t a) {
	int lu, lv;
	dv_t u, v, g1, g2;

	dv_null(u);
	dv_null(v);
	dv_null(g1);
	dv_null(g2);

	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	RLC_TRY {
		dv_new(u);
		dv_new(v);
		dv_new(g1);
		dv_new(g2);

		/* u = a, v = f, g1 = 1, g2 = 0. */
		fb_copy(u, a);
		fb_copy(v, fb_poly_get());
		if (RLC_FB_BITS % RLC_DIG == 0) {
			v[RLC_FB_DIGS] = 1;
		}
		dv_zero(g1, 2 * RLC_FB_DIGS);
		g1[0] = 1;
		dv_zero(g2, 2 * RLC_FB_DIGS);

		lu = RLC_FB_DIGS;
		lv = RLC_FB_DIGS + (RLC_FB_BITS % RLC_DIG == 0);

		/* While (u != 1 && v != 1. */
		while (1) {
			/* While z divides u do. */
			while ((u[0] & 0x01) == 0) {
				/* u = u/z. */
				bn_rsh1_low(u, u, lu);
				/* If z divides g1 then g1 = g1/z; else g1 = (g1 + f)/z. */
				if ((g1[0] & 0x01) == 1) {
					fb_poly_add(g1, g1);
					if (RLC_FB_BITS % RLC_DIG == 0) {
						g1[RLC_FB_DIGS] ^= 1;
					}
				}
				bn_rsh1_low(g1, g1, RLC_FB_DIGS + 1);
			}

			while (u[lu - 1] == 0)
				lu--;
			if (lu == 1 && u[0] == 1)
				break;

			/* While z divides v do. */
			while ((v[0] & 0x01) == 0) {
				/* v = v/z. */
				bn_rsh1_low(v, v, lv);
				/* If z divides g2 then g2 = g2/z; else (g2 = g2 + f)/z. */
				if ((g2[0] & 0x01) == 1) {
					fb_poly_add(g2, g2);
					if (RLC_FB_BITS % RLC_DIG == 0) {
						g2[RLC_FB_DIGS] ^= 1;
					}
				}
				bn_rsh1_low(g2, g2, RLC_FB_DIGS + 1);
			}

			while (v[lv - 1] == 0)
				lv--;
			if (lv == 1 && v[0] == 1)
				break;

			/* If deg(u) > deg(v) then u = u + v, g1 = g1 + g2. */
			if (lu > lv || (lu == lv && u[lu - 1] > v[lv - 1])) {
				fb_addd_low(u, u, v, lv);
				fb_add(g1, g1, g2);
			} else {
				/* Else v = v + u, g2 = g2 + g1. */
				fb_addd_low(v, v, u, lu);
				fb_add(g2, g2, g1);
			}
		}

		/* If u == 1 then return g1; else return g2. */
		if (lu == 1 && u[0] == 1) {
			fb_copy(c, g1);
		} else {
			fb_copy(c, g2);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		dv_free(u);
		dv_free(v);
		dv_free(g1);
		dv_free(g2);
	}
}

#endif

#if FB_INV == EXGCD || !defined(STRIP)

void fb_inv_exgcd(fb_t c, const fb_t a) {
	int j, d, lu, lv, lt, l1, l2, bu, bv;
	dv_t _u, _v, _g1, _g2;
	dig_t *t = NULL, *u = NULL, *v = NULL, *g1 = NULL, *g2 = NULL, carry;

	fb_null(_u);
	fb_null(_v);
	fb_null(_g1);
	fb_null(_g2);

	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	RLC_TRY {
		dv_new(_u);
		dv_new(_v);
		dv_new(_g1);
		dv_new(_g2);

		dv_zero(_g1, RLC_FB_DIGS + 1);
		dv_zero(_g2, RLC_FB_DIGS + 1);

		u = _u;
		v = _v;
		g1 = _g1;
		g2 = _g2;

		/* u = a, v = f, g1 = 1, g2 = 0. */
		fb_copy(u, a);
		fb_copy(v, fb_poly_get());
		g1[0] = 1;

		lu = lv = RLC_FB_DIGS;
		l1 = l2 = 1;

		bu = fb_bits(u);
		bv = RLC_FB_BITS + 1;
		j = bu - bv;

		/* While (u != 1). */
		while (1) {
			/* If j < 0 then swap(u, v), swap(g1, g2), j = -j. */
			if (j < 0) {
				t = u;
				u = v;
				v = t;

				lt = lu;
				lu = lv;
				lv = lt;

				t = g1;
				g1 = g2;
				g2 = t;

				lt = l1;
				l1 = l2;
				l2 = lt;

				j = -j;
			}

			RLC_RIP(j, d, j);

			/* u = u + v * z^j. */
			if (j > 0) {
				carry = fb_lsha_low(u + d, v, j, lv);
				u[d + lv] ^= carry;
			} else {
				fb_addd_low(u + d, u + d, v, lv);
			}

			/* g1 = g1 + g2 * z^j. */
			if (j > 0) {
				carry = fb_lsha_low(g1 + d, g2, j, l2);
				l1 = (l2 + d >= l1 ? l2 + d : l1);
				if (carry) {
					g1[d + l2] ^= carry;
					l1 = (l2 + d >= l1 ? l1 + 1 : l1);
				}
			} else {
				fb_addd_low(g1 + d, g1 + d, g2, l2);
				l1 = (l2 + d > l1 ? l2 + d : l1);
			}

			while (u[lu - 1] == 0)
				lu--;
			while (v[lv - 1] == 0)
				lv--;

			if (lu == 1 && u[0] == 1)
				break;

			/* j = deg(u) - deg(v). */
			lt = util_bits_dig(u[lu - 1]) - util_bits_dig(v[lv - 1]);
			if (lv > lu) {
				j = -((lv - lu) << RLC_DIG_LOG) + lt;
			} else {
				j = ((lu - lv) << RLC_DIG_LOG) + lt;
			}
		}
		/* Return g1. */
		fb_copy(c, g1);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		dv_free(_u);
		dv_free(_v);
		dv_free(_g1);
		dv_free(_g2);
	}
}

#endif

#if FB_INV == ALMOS || !defined(STRIP)

void fb_inv_almos(fb_t c, const fb_t a) {
	int lu, lv, lt;
	dv_t _b, _d, _u, _v;
	dig_t *t = NULL, *u = NULL, *v = NULL, *b = NULL, *d = NULL;

	dv_null(_b);
	dv_null(_d);
	dv_null(_u);
	dv_null(_v);

	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	/* This is actually the binary version of the Extended Euclidean algorithm
 	 * discussed in the Almost Inverse paper, so a rename is needed. */

	RLC_TRY {
		dv_new(_b);
		dv_new(_d);
		dv_new(_u);
		dv_new(_v);

		b = _b;
		d = _d;
		u = _u;
		v = _v;

		/* b = 1, d = 0, u = a, v = f. */
		dv_zero(b, 2 * RLC_FB_DIGS);
		fb_set_dig(b, 1);
		dv_zero(d, 2 * RLC_FB_DIGS);
		fb_copy(u, a);
		fb_copy(v, fb_poly_get());

		if (RLC_FB_BITS % RLC_DIG == 0) {
			v[RLC_FB_DIGS] = 1;
		}

		lu = RLC_FB_DIGS;
		lv = RLC_FB_DIGS + (RLC_FB_BITS % RLC_DIG == 0);

		while (1) {
			/* While z divides u do. */
			while ((u[0] & 0x01) == 0) {
				/* u = u/z. */
				bn_rsh1_low(u, u, lu);
				/* If z divide v then b = b/z; else b = (b + f)/z. */
				if ((b[0] & 0x01) == 1) {
					fb_poly_add(b, b);
					if (RLC_FB_BITS % RLC_DIG == 0) {
						b[RLC_FB_DIGS] ^= 1;
					}
				}
				/* b often has RLC_FB_DIGS digits. */
				bn_rsh1_low(b, b, RLC_FB_DIGS + 1);
			}
			/* If u = 1, return b. */
			while (u[lu - 1] == 0)
				lu--;
			if (lu == 1 && u[0] == 1) {
				break;
			}
			/* If deg(u) < deg(v) then swap(u, v), swap(b, d). */
			if ((lu < lv) || ((lu == lv) && (u[lu - 1] < v[lv - 1]))) {
				t = u;
				u = v;
				v = t;

				/* Swap lu and lv too. */
				lt = lu;
				lu = lv;
				lv = lt;

				t = b;
				b = d;
				d = t;
			}
			/* u = u + v, b = b + d. */
			fb_addd_low(u, u, v, lv);
			fb_addn_low(b, b, d);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fb_copy(c, b);
		dv_free(_b);
		dv_free(_d);
		dv_free(_u);
		dv_free(_v);
	}
}

#endif

#if FB_INV == ITOHT || !defined(STRIP)

void fb_inv_itoht(fb_t c, const fb_t a) {
	int i, x, y, len;
	const int *chain = fb_poly_get_chain(&len);
	int *u = RLC_ALLOCA(int, len + 1);
	fb_t *table = RLC_ALLOCA(fb_t, len + 1);

	for (i = 0; i <= len; i++) {
		fb_null(table[i]);
	}

	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	RLC_TRY {
		if (u == NULL || table == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (i = 0; i <= len; i++) {
			fb_new(table[i]);
		}

#if (FB_POLYN % 2) == 0
		fb_sqr(table[0], a);
		for (i = 2; i < RLC_FB_BITS; i++) {
			fb_sqr(table[1], a);
			for (int j = 1; j < i; j++) {
				fb_sqr(table[1], table[1]);
			}
			fb_mul(table[0], table[0], table[1]);
		}
		fb_copy(c, table[0]);
#else
		u[0] = 1;
		u[1] = 2;
		fb_copy(table[0], a);
		fb_sqr(table[1], table[0]);
		fb_mul(table[1], table[1], table[0]);
		for (i = 2; i <= len; i++) {
			x = chain[i - 1] >> 8;
			y = chain[i - 1] - (x << 8);
			if (x == y) {
				u[i] = 2 * u[i - 1];
			} else {
				u[i] = u[x] + u[y];
			}
			fb_itr(table[i], table[x], u[y], fb_poly_tab_sqr(y));
			fb_mul(table[i], table[i], table[y]);
		}
		fb_sqr(c, table[len]);
#endif
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i <= len; i++) {
			fb_free(table[i]);
		}
		RLC_FREE(u);
		RLC_FREE(table);
	}
}

#endif

#if FB_INV == BRUCH || !defined(STRIP)

void fb_inv_bruch(fb_t c, const fb_t a) {
	fb_t _r, _s, _u, _v;
	dig_t *r = NULL, *s = NULL, *t = NULL, *u = NULL, *v = NULL;
	int delta = 0;

	fb_null(_r);
	fb_null(_s);
	fb_null(_u);
	fb_null(_v);

	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	RLC_TRY {
		fb_new(_r);
		fb_new(_s);
		fb_new(_u);
		fb_new(_v);

		fb_copy(_r, a);
		fb_copy(_s, fb_poly_get());
		fb_zero(_v);
		fb_set_dig(_u, 1);

		r = _r;
		s = _s;
		u = _u;
		v = _v;

		for (int i = 1; i <= 2 * RLC_FB_BITS; i++) {
			if ((r[RLC_FB_DIGS - 1] & ((dig_t)1 << (RLC_FB_BITS % RLC_DIG))) == 0) {
				fb_lsh(r, r, 1);
				fb_lsh(u, u, 1);
				delta++;
			} else {
				if ((s[RLC_FB_DIGS - 1] & ((dig_t)1 << (RLC_FB_BITS % RLC_DIG)))) {
					fb_add(s, s, r);
					fb_add(v, v, u);
				}
				fb_lsh(s, s, 1);
				if (delta == 0) {
					t = r;
					r = s;
					s = t;
					t = u;
					u = v;
					v = t;
					fb_lsh(u, u, 1);
					delta++;
				} else {
					fb_rsh(u, u, 1);
					delta--;
				}
			}
		}
		fb_copy(c, u);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fb_free(_r);
		fb_free(_s);
		fb_free(_u);
		fb_free(_v);
	}
}

#endif

#if FB_INV == CTAIA || !defined(STRIP)

void fb_inv_ctaia(fb_t c, const fb_t a) {
	fb_t r, s, t, u, v;
	int i, k, d, r0, d0;

	fb_null(r);
	fb_null(s);
	fb_null(t);
	fb_null(u);
	fb_null(v);

	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	RLC_TRY {
		fb_new(r);
		fb_new(s);
		fb_new(t);
		fb_new(u);
		fb_new(v);

		fb_copy(r, a);
		fb_copy(t, fb_poly_get());
		fb_copy(s, t);
		fb_set_dig(u, 1);
		fb_zero(v);
		d = -1;

		for (k = 1; k < 2 * RLC_FB_BITS; k++) {
			r0 = r[0] & 1;
			d0 = d >> (8 * sizeof(int) - 1);

			for (i = 0; i < RLC_FB_DIGS; i++) {
				r[i] ^= (s[i] & -r0);
				u[i] ^= (v[i] & -r0);
				s[i] ^= (r[i] & d0);
				v[i] ^= (u[i] & d0);
			}

			d = RLC_SEL(d, -d, r0 & -d0);

			fb_rsh(r, r, 1);

			r0 = u[0] & 1;
			fb_poly_add(t, u);
			for (i = 0; i < RLC_FB_DIGS; i++) {
				u[i] = RLC_SEL(u[i], t[i], r0);
			}
			fb_rsh(u, u, 1);
			d--;
		}

		fb_copy(c, v);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fb_free(r);
		fb_free(s);
		fb_free(t);
		fb_free(u);
		fb_free(v);
	}
}

#endif

#if FB_INV == LOWER || !defined(STRIP)

void fb_inv_lower(fb_t c, const fb_t a) {
	if (fb_is_zero(a)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	fb_invn_low(c, a);
}
#endif

void fb_inv_sim(fb_t *c, const fb_t *a, int n) {
	int i;
	fb_t u, *t = RLC_ALLOCA(fb_t, n);

	if (t == NULL) {
		RLC_THROW(ERR_NO_MEMORY);
	}

	fb_null(u);

	RLC_TRY {
		for (i = 0; i < n; i++) {
			fb_null(t[i]);
			fb_new(t[i]);
		}
		fb_new(u);

		fb_copy(c[0], a[0]);
		fb_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fb_copy(t[i], a[i]);
			fb_mul(c[i], c[i - 1], a[i]);
		}

		fb_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fb_mul(c[i], u, c[i - 1]);
			fb_mul(u, u, t[i]);
		}
		fb_copy(c[0], u);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < n; i++) {
			fb_free(t[i]);
		}
		fb_free(u);
		RLC_FREE(t);
	}
}
