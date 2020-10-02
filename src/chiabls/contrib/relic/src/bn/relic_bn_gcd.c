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
 * Implementation of the multiple precision addition and subtraction functions.
 *
 * @ingroup bn
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if BN_GCD == BASIC || !defined(STRIP)

void bn_gcd_basic(bn_t c, const bn_t a, const bn_t b) {
	bn_t u, v;

	if (bn_is_zero(a)) {
		bn_abs(c, b);
		return;
	}

	if (bn_is_zero(b)) {
		bn_abs(c, a);
		return;
	}

	bn_null(u);
	bn_null(v);

	TRY {
		bn_new(u);
		bn_new(v);

		bn_abs(u, a);
		bn_abs(v, b);
		while (!bn_is_zero(v)) {
			bn_copy(c, v);
			bn_mod(v, u, v);
			bn_copy(u, c);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(u);
		bn_free(v);
	}
}

void bn_gcd_ext_basic(bn_t c, bn_t d, bn_t e, const bn_t a, const bn_t b) {
	bn_t u, v, x_1, y_1, q, r;

	if (bn_is_zero(a)) {
		bn_abs(c, b);
		bn_zero(d);
		if (e != NULL) {
			bn_set_dig(e, 1);
		}
		return;
	}

	if (bn_is_zero(b)) {
		bn_abs(c, a);
		bn_set_dig(d, 1);
		if (e != NULL) {
			bn_zero(e);
		}
		return;
	}

	bn_null(u);
	bn_null(v);
	bn_null(x_1);
	bn_null(y_1);
	bn_null(q);
	bn_null(r);

	TRY {
		bn_new(u);
		bn_new(v);
		bn_new(x_1);
		bn_new(y_1);
		bn_new(q);
		bn_new(r);

		bn_abs(u, a);
		bn_abs(v, b);

		bn_zero(x_1);
		bn_set_dig(y_1, 1);

		if (e != NULL) {
			bn_set_dig(d, 1);
			bn_zero(e);

			while (!bn_is_zero(v)) {
				bn_div_rem(q, r, u, v);

				bn_copy(u, v);
				bn_copy(v, r);

				bn_mul(c, q, x_1);
				bn_sub(r, d, c);
				bn_copy(d, x_1);
				bn_copy(x_1, r);

				bn_mul(c, q, y_1);
				bn_sub(r, e, c);
				bn_copy(e, y_1);
				bn_copy(y_1, r);
			}
		} else {
			bn_set_dig(d, 1);

			while (!bn_is_zero(v)) {
				bn_div_rem(q, r, u, v);

				bn_copy(u, v);
				bn_copy(v, r);

				bn_mul(c, q, x_1);
				bn_sub(r, d, c);
				bn_copy(d, x_1);
				bn_copy(x_1, r);
			}
		}
		bn_copy(c, u);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(u);
		bn_free(v);
		bn_free(x_1);
		bn_free(y_1);
		bn_free(q);
		bn_free(r);
	}
}

#endif

#if BN_GCD == LEHME || !defined(STRIP)

void bn_gcd_lehme(bn_t c, const bn_t a, const bn_t b) {
	bn_t x, y, u, v, t0, t1, t2, t3;
	dig_t _x, _y, q, _q, t, _t;
	dis_t _a, _b, _c, _d;

	if (bn_is_zero(a)) {
		bn_abs(c, b);
		return;
	}

	if (bn_is_zero(b)) {
		bn_abs(c, a);
		return;
	}

	bn_null(x);
	bn_null(y);
	bn_null(u);
	bn_null(v);
	bn_null(t0);
	bn_null(t1);
	bn_null(t2);
	bn_null(t3);

	/*
	 * Taken from Handbook of Hyperelliptic and Elliptic Cryptography.
	 */
	TRY {
		bn_new(x);
		bn_new(y);
		bn_new(u);
		bn_new(v);
		bn_new(t0);
		bn_new(t1);
		bn_new(t2);
		bn_new(t3);

		if (bn_cmp(a, b) == CMP_GT) {
			bn_abs(x, a);
			bn_abs(y, b);
		} else {
			bn_abs(x, b);
			bn_abs(y, a);
		}
		while (y->used > 1) {
			bn_rsh(u, x, bn_bits(x) - BN_DIGIT);
			_x = u->dp[0];
			bn_rsh(v, y, bn_bits(x) - BN_DIGIT);
			_y = v->dp[0];
			_a = _d = 1;
			_b = _c = 0;
			t = 0;
			if (_y != 0) {
				q = _x / _y;
				t = _x % _y;
			}
			if (t >= ((dig_t)1 << (BN_DIGIT / 2))) {
				while (1) {
					_q = _y / t;
					_t = _y % t;
					if (_t < ((dig_t)1 << (BN_DIGIT / 2))) {
						break;
					}
					_x = _y;
					_y = t;
					t = _a - q * _c;
					_a = _c;
					_c = t;
					t = _b - q * _d;
					_b = _d;
					_d = t;
					t = _t;
					q = _q;
				}
			}
			if (_b == 0) {
				bn_mod(t0, x, y);
				bn_copy(x, y);
				bn_copy(y, t0);
			} else {
				bn_rsh(u, x, bn_bits(x) - 2 * BN_DIGIT);
				bn_rsh(v, y, bn_bits(x) - 2 * BN_DIGIT);
				if (_a < 0) {
					bn_mul_dig(t0, u, -_a);
					bn_neg(t0, t0);
				} else {
					bn_mul_dig(t0, u, _a);
				}
				if (_b < 0) {
					bn_mul_dig(t1, v, -_b);
					bn_neg(t1, t1);
				} else {
					bn_mul_dig(t1, v, _b);
				}
				if (_c < 0) {
					bn_mul_dig(t2, u, -_c);
					bn_neg(t2, t2);
				} else {
					bn_mul_dig(t2, u, _c);
				}
				if (_d < 0) {
					bn_mul_dig(t3, v, -_d);
					bn_neg(t3, t3);
				} else {
					bn_mul_dig(t3, v, _d);
				}
				bn_add(u, t0, t1);
				bn_add(v, t2, t3);
				bn_rsh(t0, u, bn_bits(u) - BN_DIGIT);
				_x = t0->dp[0];
				bn_rsh(t1, v, bn_bits(u) - BN_DIGIT);
				_y = t1->dp[0];
				t = 0;
				if (_y != 0) {
					q = _x / _y;
					t = _x % _y;
				}
				if (t >= ((dig_t)1 << BN_DIGIT / 2)) {
					while (1) {
						_q = _y / t;
						_t = _y % t;
						if (_t < ((dig_t)1 << BN_DIGIT / 2)) {
							break;
						}
						_x = _y;
						_y = t;
						t = _a - q * _c;
						_a = _c;
						_c = t;
						t = _b - q * _d;
						_b = _d;
						_d = t;
						t = _t;
						q = _q;
					}
				}
				if (_a < 0) {
					bn_mul_dig(t0, x, -_a);
					bn_neg(t0, t0);
				} else {
					bn_mul_dig(t0, x, _a);
				}
				if (_b < 0) {
					bn_mul_dig(t1, y, -_b);
					bn_neg(t1, t1);
				} else {
					bn_mul_dig(t1, y, _b);
				}
				if (_c < 0) {
					bn_mul_dig(t2, x, -_c);
					bn_neg(t2, t2);
				} else {
					bn_mul_dig(t2, x, _c);
				}
				if (_d < 0) {
					bn_mul_dig(t3, y, -_d);
					bn_neg(t3, t3);
				} else {
					bn_mul_dig(t3, y, _d);
				}
				bn_add(x, t0, t1);
				bn_add(y, t2, t3);
			}
		}
		bn_gcd_ext_dig(c, u, v, x, y->dp[0]);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(x);
		bn_free(y);
		bn_free(u);
		bn_free(v);
		bn_free(t0);
		bn_free(t1);
		bn_free(t2);
		bn_free(t3);
	}
}

void bn_gcd_ext_lehme(bn_t c, bn_t d, bn_t e, const bn_t a, const bn_t b) {
	bn_t x, y, u, v, t0, t1, t2, t3, t4;
	dig_t _x, _y, q, _q, t, _t;
	dis_t _a, _b, _c, _d;
	int swap;

	if (bn_is_zero(a)) {
		bn_abs(c, b);
		bn_zero(d);
		if (e != NULL) {
			bn_set_dig(e, 1);
		}
		return;
	}

	if (bn_is_zero(b)) {
		bn_abs(c, a);
		bn_set_dig(d, 1);
		if (e != NULL) {
			bn_zero(e);
		}
		return;
	}

	bn_null(x);
	bn_null(y);
	bn_null(u);
	bn_null(v);
	bn_null(t0);
	bn_null(t1);
	bn_null(t2);
	bn_null(t3);
	bn_null(t4);

	/*
	 * Taken from Handbook of Hyperelliptic and Elliptic Cryptography.
	 */
	TRY {
		bn_new(x);
		bn_new(y);
		bn_new(u);
		bn_new(v);
		bn_new(t0);
		bn_new(t1);
		bn_new(t2);
		bn_new(t3);
		bn_new(t4);

		if (bn_cmp(a, b) != CMP_LT) {
			bn_abs(x, a);
			bn_abs(y, b);
			swap = 0;
		} else {
			bn_abs(x, b);
			bn_abs(y, a);
			swap = 1;
		}

		bn_zero(t4);
		bn_set_dig(d, 1);

		while (y->used > 1) {
			bn_rsh(u, x, bn_bits(x) - BN_DIGIT);
			_x = u->dp[0];
			bn_rsh(v, y, bn_bits(x) - BN_DIGIT);
			_y = v->dp[0];
			_a = _d = 1;
			_b = _c = 0;
			t = 0;
			if (_y != 0) {
				q = _x / _y;
				t = _x % _y;
			}
			if (t >= ((dig_t)1 << (BN_DIGIT / 2))) {
				while (1) {
					_q = _y / t;
					_t = _y % t;
					if (_t < ((dig_t)1 << (BN_DIGIT / 2))) {
						break;
					}
					_x = _y;
					_y = t;
					t = _a - q * _c;
					_a = _c;
					_c = t;
					t = _b - q * _d;
					_b = _d;
					_d = t;
					t = _t;
					q = _q;
				}
			}
			if (_b == 0) {
				bn_div_rem(t1, t0, x, y);
				bn_copy(x, y);
				bn_copy(y, t0);
				bn_mul(t1, t1, d);
				bn_sub(t1, t4, t1);
				bn_copy(t4, d);
				bn_copy(d, t1);
			} else {
				bn_rsh(u, x, bn_bits(x) - 2 * BN_DIGIT);
				bn_rsh(v, y, bn_bits(x) - 2 * BN_DIGIT);
				if (_a < 0) {
					bn_mul_dig(t0, u, -_a);
					bn_neg(t0, t0);
				} else {
					bn_mul_dig(t0, u, _a);
				}
				if (_b < 0) {
					bn_mul_dig(t1, v, -_b);
					bn_neg(t1, t1);
				} else {
					bn_mul_dig(t1, v, _b);
				}
				if (_c < 0) {
					bn_mul_dig(t2, u, -_c);
					bn_neg(t2, t2);
				} else {
					bn_mul_dig(t2, u, _c);
				}
				if (_d < 0) {
					bn_mul_dig(t3, v, -_d);
					bn_neg(t3, t3);
				} else {
					bn_mul_dig(t3, v, _d);
				}
				bn_add(u, t0, t1);
				bn_add(v, t2, t3);
				bn_rsh(t0, u, bn_bits(u) - BN_DIGIT);
				_x = t0->dp[0];
				bn_rsh(t1, v, bn_bits(u) - BN_DIGIT);
				_y = t1->dp[0];
				t = 0;
				if (_y != 0) {
					q = _x / _y;
					t = _x % _y;
				}
				if (t >= ((dig_t)1 << BN_DIGIT / 2)) {
					while (1) {
						_q = _y / t;
						_t = _y % t;
						if (_t < ((dig_t)1 << BN_DIGIT / 2)) {
							break;
						}
						_x = _y;
						_y = t;
						t = _a - q * _c;
						_a = _c;
						_c = t;
						t = _b - q * _d;
						_b = _d;
						_d = t;
						t = _t;
						q = _q;
					}
				}
				if (_a < 0) {
					bn_mul_dig(t0, x, -_a);
					bn_neg(t0, t0);
				} else {
					bn_mul_dig(t0, x, _a);
				}
				if (_b < 0) {
					bn_mul_dig(t1, y, -_b);
					bn_neg(t1, t1);
				} else {
					bn_mul_dig(t1, y, _b);
				}
				if (_c < 0) {
					bn_mul_dig(t2, x, -_c);
					bn_neg(t2, t2);
				} else {
					bn_mul_dig(t2, x, _c);
				}
				if (_d < 0) {
					bn_mul_dig(t3, y, -_d);
					bn_neg(t3, t3);
				} else {
					bn_mul_dig(t3, y, _d);
				}
				bn_add(x, t0, t1);
				bn_add(y, t2, t3);

				if (_a < 0) {
					bn_mul_dig(t0, t4, -_a);
					bn_neg(t0, t0);
				} else {
					bn_mul_dig(t0, t4, _a);
				}
				if (_b < 0) {
					bn_mul_dig(t1, d, -_b);
					bn_neg(t1, t1);
				} else {
					bn_mul_dig(t1, d, _b);
				}
				if (_c < 0) {
					bn_mul_dig(t2, t4, -_c);
					bn_neg(t2, t2);
				} else {
					bn_mul_dig(t2, t4, _c);
				}
				if (_d < 0) {
					bn_mul_dig(t3, d, -_d);
					bn_neg(t3, t3);
				} else {
					bn_mul_dig(t3, d, _d);
				}
				bn_add(t4, t0, t1);
				bn_add(d, t2, t3);
			}
		}
		bn_gcd_ext_dig(c, u, v, x, y->dp[0]);
		if (!swap) {
			bn_mul(t0, t4, u);
			bn_mul(t1, d, v);
			bn_add(t4, t0, t1);
			bn_mul(x, b, t4);
			bn_sub(x, c, x);
			bn_div(d, x, a);
			if (bn_sign(x) != bn_sign(a)) {
				bn_add_dig(d, d, 1);
			}
		} else {
			bn_mul(t0, t4, u);
			bn_mul(t1, d, v);
			bn_add(d, t0, t1);
			bn_mul(x, a, d);
			bn_sub(x, c, x);
			bn_div(t4, x, b);
			if (bn_sign(x) != bn_sign(b)) {
				bn_add_dig(t4, t4, 1);
			}
		}
		if (e != NULL) {
			bn_copy(e, t4);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(x);
		bn_free(y);
		bn_free(u);
		bn_free(v);
		bn_free(t0);
		bn_free(t1);
		bn_free(t2);
		bn_free(t3);
		bn_free(t4);
	}
}

#endif

#if BN_GCD == STEIN || !defined(STRIP)

void bn_gcd_stein(bn_t c, const bn_t a, const bn_t b) {
	bn_t u, v, t;
	int shift;

	if (bn_is_zero(a)) {
		bn_abs(c, b);
		return;
	}

	if (bn_is_zero(b)) {
		bn_abs(c, a);
		return;
	}

	bn_null(u);
	bn_null(v);
	bn_null(t);

	TRY {
		bn_new(u);
		bn_new(v);
		bn_new(t);

		bn_abs(u, a);
		bn_abs(v, b);

		shift = 0;
		while (bn_is_even(u) && bn_is_even(v)) {
			bn_hlv(u, u);
			bn_hlv(v, v);
			shift++;
		}
		while (!bn_is_zero(u)) {
			while (bn_is_even(u)) {
				bn_hlv(u, u);
			}
			while (bn_is_even(v)) {
				bn_hlv(v, v);
			}
			bn_sub(t, u, v);
			bn_abs(t, t);
			bn_hlv(t, t);
			if (bn_cmp(u, v) != CMP_LT) {
				bn_copy(u, t);
			} else {
				bn_copy(v, t);
			}
		}
		bn_lsh(c, v, shift);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(u);
		bn_free(v);
		bn_free(t);
	}
}

void bn_gcd_ext_stein(bn_t c, bn_t d, bn_t e, const bn_t a, const bn_t b) {
	bn_t x, y, u, v, _a, _b, _e;
	int shift, found;

	if (bn_is_zero(a)) {
		bn_abs(c, b);
		bn_zero(d);
		if (e != NULL) {
			bn_set_dig(e, 1);
		}
		return;
	}

	if (bn_is_zero(b)) {
		bn_abs(c, a);
		bn_set_dig(d, 1);
		if (e != NULL) {
			bn_zero(e);
		}
		return;
	}

	bn_null(x);
	bn_null(y);
	bn_null(u);
	bn_null(v);
	bn_null(_a);
	bn_null(_b);
	bn_null(_e);

	TRY {
		bn_new(x);
		bn_new(y);
		bn_new(u);
		bn_new(v);
		bn_new(_a);
		bn_new(_b);
		bn_new(_e);

		bn_abs(x, a);
		bn_abs(y, b);

		/* g = 1. */
		shift = 0;
		/* While x and y are both even, x = x/2 and y = y/2, g = 2g. */
		while (bn_is_even(x) && bn_is_even(y)) {
			bn_hlv(x, x);
			bn_hlv(y, y);
			shift++;
		}

		bn_copy(u, x);
		bn_copy(v, y);

		/* u = x, y = v, A = 1, B = 0, C = 0, D = 1. */
		bn_set_dig(_a, 1);
		bn_zero(_b);
		bn_zero(d);
		bn_set_dig(_e, 1);

		found = 0;
		while (!found) {
			/* While u is even, u = u/2. */
			while ((u->dp[0] & 0x01) == 0) {
				bn_hlv(u, u);
				/* If A = B = 0 (mod 2) then A = A/2, B = B/2. */
				if ((_a->dp[0] & 0x01) == 0 && (_b->dp[0] & 0x01) == 0) {
					bn_hlv(_a, _a);
					bn_hlv(_b, _b);
				} else {
					/* Otherwise A = (A + y)/2, B = (B - x)/2. */
					bn_add(_a, _a, y);
					bn_hlv(_a, _a);
					bn_sub(_b, _b, x);
					bn_hlv(_b, _b);
				}
			}
			/* While v is even, v = v/2. */
			while ((v->dp[0] & 0x01) == 0) {
				bn_hlv(v, v);
				/* If C = D = 0 (mod 2) then C = C/2, D = D/2. */
				if ((d->dp[0] & 0x01) == 0 && (_e->dp[0] & 0x01) == 0) {
					bn_hlv(d, d);
					bn_hlv(_e, _e);
				} else {
					/* Otherwise C = (C + y)/2, D = (D - x)/2. */
					bn_add(d, d, y);
					bn_hlv(d, d);
					bn_sub(_e, _e, x);
					bn_hlv(_e, _e);
				}
			}
			/* If u >= v then u = u - v, A = A - C, B = B - D. */
			if (bn_cmp(u, v) != CMP_LT) {
				bn_sub(u, u, v);
				bn_sub(_a, _a, d);
				bn_sub(_b, _b, _e);
			} else {
				/* Otherwise, v = v - u, C = C - a, D = D - B. */
				bn_sub(v, v, u);
				bn_sub(d, d, _a);
				bn_sub(_e, _e, _b);
			}
			/* If u = 0 then d = C, e = D and return (d, e, g * v). */
			if (bn_is_zero(u)) {
				bn_lsh(c, v, shift);
				found = 1;
			}
		}
		if (e != NULL) {
			bn_copy(e, _e);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(x);
		bn_free(y);
		bn_free(u);
		bn_free(v);
		bn_free(_a);
		bn_free(_b);
		bn_free(_e);
	}
}

#endif

void bn_gcd_ext_mid(bn_t c, bn_t d, bn_t e, bn_t f, const bn_t a, const bn_t b) {
	bn_t p, q, r, s, t, u, v, x, w, y, z;

	if (bn_is_zero(a)) {
		bn_abs(c, b);
		bn_zero(d);
		bn_zero(e);
		return;
	}

	if (bn_is_zero(b)) {
		bn_abs(c, a);
		bn_set_dig(d, 1);
		bn_set_dig(e, 1);
		return;
	}

	bn_null(q);
	bn_null(r);
	bn_null(s);
	bn_null(t);
	bn_null(u);
	bn_null(v);
	bn_null(x);
	bn_null(w);
	bn_null(y);
	bn_null(z);

	TRY {
		bn_new(p);
		bn_new(q);
		bn_new(r);
		bn_new(s);
		bn_new(t);
		bn_new(u);
		bn_new(v);
		bn_new(x);
		bn_new(w);
		bn_new(y);
		bn_new(z);

		if (bn_cmp_abs(a, b) == CMP_GT) {
			bn_abs(u, a);
			bn_abs(v, b);
		} else {
			bn_abs(u, b);
			bn_abs(v, a);
		}

		bn_srt(p, u);

		bn_set_dig(x, 1);
		bn_zero(t);

		int wait = 0;
		while (!bn_is_zero(v)) {
			bn_div_rem(q, r, u, v);

			bn_copy(u, v);
			bn_copy(v, r);

			bn_mul(s, q, x);
			bn_sub(s, t, s);
			bn_copy(t, x);
			bn_copy(x, s);

			if (wait) {
				bn_copy(e, r);
				bn_neg(f, x);
				wait = 0;
			}
			if (bn_cmp(u, p) == CMP_GT) {
				bn_copy(c, r);
				bn_neg(d, x);
				bn_copy(w, u);
				bn_neg(y, t);
				wait = 1;
			}
		}
		/* Compute r as the norm of vector (w, y). */
		bn_sqr(s, w);
		bn_sqr(t, y);
		bn_add(t, t, s);

		/* Compute q as the norm of vector (e, f). */
		bn_sqr(r, e);
		bn_sqr(q, f);
		bn_add(q, q, r);

		/* Output (e, f) as the vector of smaller norm. */
		if (bn_cmp(t, q) == CMP_LT) {
			bn_copy(e, w);
			bn_copy(f, y);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(p);
		bn_free(q);
		bn_free(r);
		bn_free(s);
		bn_free(t);
		bn_free(u);
		bn_free(v);
		bn_free(x);
		bn_free(w);
		bn_free(y);
		bn_free(z);
	}
}

void bn_gcd_dig(bn_t c, const bn_t a, dig_t b) {
	dig_t _u, _v, _t = 0;

	if (bn_is_zero(a)) {
		bn_set_dig(c, b);
		return;
	}

	if (b == 0) {
		bn_abs(c, a);
		return;
	}

	bn_mod_dig(&(c->dp[0]), a, b);
	_v = c->dp[0];
	_u = b;
	while (_v != 0) {
		_t = _v;
		_v = _u % _v;
		_u = _t;
	}
	bn_set_dig(c, _u);
}

void bn_gcd_ext_dig(bn_t c, bn_t d, bn_t e, const bn_t a, const dig_t b) {
	bn_t u, v, x1, y1, q, r;
	dig_t _v, _q, _t, _u;

	if (d == NULL && e == NULL) {
		bn_gcd_dig(c, a, b);
		return;
	}

	if (bn_is_zero(a)) {
		bn_set_dig(c, b);
		bn_zero(d);
		if (e != NULL) {
			bn_set_dig(e, 1);
		}
		return;
	}

	if (b == 0) {
		bn_abs(c, a);
		bn_set_dig(d, 1);
		if (e != NULL) {
			bn_zero(e);
		}
		return;
	}

	bn_null(u);
	bn_null(v);
	bn_null(x1);
	bn_null(y1);
	bn_null(q);
	bn_null(r);

	TRY {
		bn_new(u);
		bn_new(v);
		bn_new(x1);
		bn_new(y1);
		bn_new(q);
		bn_new(r);

		bn_abs(u, a);
		bn_set_dig(v, b);

		bn_zero(x1);
		bn_set_dig(y1, 1);
		bn_set_dig(d, 1);

		if (e != NULL) {
			bn_zero(e);
		}

		bn_div_rem(q, r, u, v);

		bn_copy(u, v);
		bn_copy(v, r);

		bn_mul(c, q, x1);
		bn_sub(r, d, c);
		bn_copy(d, x1);
		bn_copy(x1, r);

		if (e != NULL) {
			bn_mul(c, q, y1);
			bn_sub(r, e, c);
			bn_copy(e, y1);
			bn_copy(y1, r);
		}

		_v = v->dp[0];
		_u = u->dp[0];
		while (_v != 0) {
			_q = _u / _v;
			_t = _u % _v;

			_u = _v;
			_v = _t;

			bn_mul_dig(c, x1, _q);
			bn_sub(r, d, c);
			bn_copy(d, x1);
			bn_copy(x1, r);

			if (e != NULL) {
				bn_mul_dig(c, y1, _q);
				bn_sub(r, e, c);
				bn_copy(e, y1);
				bn_copy(y1, r);
			}
		}
		bn_set_dig(c, _u);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(u);
		bn_free(v);
		bn_free(x1);
		bn_free(y1);
		bn_free(q);
		bn_free(r);
	}
}
