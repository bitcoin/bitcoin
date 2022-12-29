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
 * Implementation of extendable ring signatures.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_etrs_gen(ec_t pp) {
	ec_rand(pp);
	return RLC_OK;
}

int cp_etrs_gen_key(bn_t sk, ec_t pk) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		ec_curve_get_ord(n);
		bn_rand_mod(sk, n);
		ec_mul_gen(pk, sk);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_etrs_sig(bn_t *td, bn_t *y, int max, etrs_t p, uint8_t *msg, int len,
		bn_t sk, ec_t pk, ec_t pp) {
	bn_t n, l, u, v, z;
	ec_t t, w[2];
	int result = RLC_OK;

	bn_null(n);
	bn_null(l);
	bn_null(u);
	bn_null(v);
	bn_null(z);
	ec_null(t);
	ec_null(w[0]);
	ec_null(w[1]);

	RLC_TRY {
		bn_new(n);
		bn_new(l);
		bn_new(u);
		bn_new(v);
		bn_new(z);
		ec_new(t);
		ec_new(w[0]);
		ec_new(w[1]);

		ec_curve_get_ord(n);

		for(int i = 0; i < max; i++) {
			bn_rand_mod(y[i], n);
			bn_rand_mod(td[i], n);
		}
		bn_rand_mod(p->y, n);

		bn_set_dig(l, 1);
		for(int j = 0; j < max; j++) {
			bn_mod_inv(v, y[j], n);
			bn_sub(u, y[j], p->y);
			bn_mul(u, u, v);
			bn_mod(u, u, n);
			bn_mul(l, l, u);
			bn_mod(l, l, n);
		}
		ec_mul(p->h, pp, l);
		for(int i = 0; i < max; i++) {
			ec_mul_gen(t, td[i]);
			bn_mod_inv(v, y[i], n);
			bn_mul(u, v, p->y);
			bn_mod(l, u, n);
			for(int j = 0; j < max; j++) {
				if (j != i) {
					bn_sub(v, y[j], y[i]);
					bn_mod(v, v, n);
					bn_mod_inv(v, v, n);
					bn_sub(u, y[j], p->y);
					bn_mul(u, u, v);
					bn_mod(u, u, n);
					bn_mul(l, l, u);
					bn_mod(l, l, n);
				}
			}
			ec_mul(t, t, l);
			ec_add(p->h, p->h, t);
		}
		ec_norm(p->h, p->h);

		ec_copy(p->pk, pk);
		ec_copy(w[0], p->h);
		ec_copy(w[1], p->pk);
		cp_sokor_sig(p->c, p->r, msg, len, w, sk, 0);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(l);
		bn_free(u);
		bn_free(v);
		bn_free(z);
		ec_free(t);
		ec_free(w[0]);
		ec_free(w[1]);
	}
	return result;
}

int cp_etrs_ver(int thres, bn_t *td, bn_t *y, int max, etrs_t *s, int size,
		uint8_t *msg, int len, ec_t pp) {
	int i, flag = 0, result = 0;
	bn_t l, n, u, v;
	ec_t t, w[2];

	int d = max + size - thres;
	bn_t *_y = RLC_ALLOCA(bn_t, d);
	ec_t *_t = RLC_ALLOCA(ec_t, d);

	bn_null(l);
	bn_null(n);
	bn_null(u);
	bn_null(v);
	ec_null(t);
	ec_null(w[0]);
	ec_null(w[1]);

	RLC_TRY {
		bn_new(l);
		bn_new(n);
		bn_new(u);
		bn_new(v);
		ec_new(t);
		ec_new(w[0]);
		ec_new(w[1]);
		if (_y == NULL || _t == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (i = 0; i < d; i++) {
			bn_new(_y[i]);
			ec_new(_t[i]);
		}

		for (i = 0; i < max; i++) {
			bn_copy(_y[i], y[i]);
			ec_mul_gen(_t[i], td[i]);
		}
		for (; i < d; i++) {
			bn_copy(_y[i], s[i - max]->y);
			ec_copy(_t[i], s[i - max]->h);
		}

		ec_curve_get_ord(n);

		flag = 1;
		ec_set_infty(w[0]);
		for(i = 0; i < d; i++) {
			bn_set_dig(l, 1);
			for(int j = 0; j < d; j++) {
				if (j != i) {
					bn_sub(v, _y[j], _y[i]);
					bn_mod(v, v, n);
					bn_mod_inv(v, v, n);
					bn_mul(u, _y[j], v);
					bn_mod(u, u, n);
					bn_mul(l, l, u);
					bn_mod(l, l, n);
				}
			}

			ec_mul(t, _t[i], l);
			ec_add(w[0], w[0], t);
		}
		ec_norm(w[0], w[0]);
		flag &= ec_cmp(w[0], pp) != RLC_EQ;

		for (int i = 0; i < size; i++) {
			ec_copy(w[0], s[i]->h);
			ec_copy(w[1], s[i]->pk);
			flag &= cp_sokor_ver(s[i]->c, s[i]->r, msg, len, w);
        }
		result = flag;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(l);
		bn_free(n);
		bn_free(u);
		bn_free(v);
		ec_free(t);
		ec_free(w[0]);
		ec_free(w[1]);
		for (int i = 0; i < d; i++) {
			bn_free(_y[i]);
			ec_free(_t[i]);
		}
		RLC_FREE(_y);
		RLC_FREE(_t);
	}
	return result;
}

int cp_etrs_ext(bn_t *td, bn_t *y, int max, etrs_t *p, int *size, uint8_t *msg, int len,
		ec_t pk, ec_t pp) {
	bn_t n, r;
	ec_t w[2];
	int i, result = RLC_OK;

	bn_null(n);
	bn_null(r);
	ec_null(w[0]);
	ec_null(w[1]);

	for (int i = 0; i < *size; i++) {
		if (ec_cmp(pk, p[i]->pk) == RLC_EQ) {
			return RLC_ERR;
		}
	}

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		ec_new(w[0]);
		ec_new(w[1]);

		for (i = 0; i < max; i++) {
			if (!bn_is_zero(td[i])) {
				break;
			}
		}
		bn_copy(r, td[i]);

		ec_curve_get_ord(n);
		ec_mul_gen(p[*size]->h, td[i]);
		bn_copy(p[*size]->y, y[i]);

		bn_zero(td[i]);
		bn_zero(y[i]);

		ec_copy(p[*size]->pk, pk);
		ec_copy(w[0], p[*size]->h);
		ec_copy(w[1], p[*size]->pk);
		cp_sokor_sig(p[*size]->c, p[*size]->r, msg, len, w, r, 1);
		(*size)++;
		result = RLC_OK;
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		ec_free(w[0]);
		ec_free(w[1]);
	}
	return result;
}

int cp_etrs_uni(int thres, bn_t *td, bn_t *y, int max, etrs_t *p, int *size,
		uint8_t *msg, int len, bn_t sk, ec_t pk, ec_t pp) {
	int i, result = 0;
	bn_t l, n, u, v;
	ec_t t, w[2];

	int d = max + *size;
	bn_t *_y = RLC_ALLOCA(bn_t, d);
	ec_t *_t = RLC_ALLOCA(ec_t, d);

	bn_null(l);
	bn_null(n);
	bn_null(u);
	bn_null(v);
	ec_null(t);
	ec_null(w[0]);
	ec_null(w[1]);

	RLC_TRY {
		bn_new(l);
		bn_new(n);
		bn_new(u);
		bn_new(v);
		ec_new(t);
		ec_new(w[0]);
		ec_new(w[1]);
		if (_y == NULL || _t == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (i = 0; i < d; i++) {
			bn_new(_y[i]);
			ec_new(_t[i]);
		}

		for (i = 0; i < max; i++) {
			bn_copy(_y[i], y[i]);
			ec_mul_gen(_t[i], td[i]);
		}
		for (; i < d; i++) {
			bn_copy(_y[i], p[i - max]->y);
			ec_copy(_t[i], p[i - max]->h);
		}

		ec_curve_get_ord(n);
		bn_rand_mod(p[*size]->y, n);

		ec_set_infty(p[*size]->h);
		for(i = 0; i < d; i++) {
			bn_set_dig(l, 1);
			for(int j = 0; j < d; j++) {
				if (j != i) {
					bn_sub(v, _y[j], _y[i]);
					bn_mod(v, v, n);
					bn_mod_inv(v, v, n);
					bn_sub(u, _y[j], p[*size]->y);
					bn_mul(u, u, v);
					bn_mod(u, u, n);
					bn_mul(l, l, u);
					bn_mod(l, l, n);
				}
			}
			ec_mul(t, _t[i], l);
			ec_add(p[*size]->h, p[*size]->h, t);
		}
		ec_norm(p[*size]->h, p[*size]->h);

		ec_copy(p[*size]->pk, pk);
		ec_copy(w[0], p[*size]->h);
		ec_copy(w[1], p[*size]->pk);
		cp_sokor_sig(p[*size]->c, p[*size]->r, msg, len, w, sk, 0);
		(*size)++;
		result = RLC_OK;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(l);
		bn_free(n);
		bn_free(u);
		bn_free(v);
		ec_free(t);
		ec_free(w[0]);
		ec_free(w[1]);
		for (int i = 0; i < d; i++) {
			bn_free(_y[i]);
			ec_free(_t[i]);
		}
		RLC_FREE(_y);
		RLC_FREE(_t);
	}
	return result;
}
