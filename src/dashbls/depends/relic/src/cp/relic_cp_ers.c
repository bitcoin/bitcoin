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

int cp_ers_gen(ec_t pp) {
	ec_rand(pp);
	return RLC_OK;
}

int cp_ers_gen_key(bn_t sk, ec_t pk) {
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

int cp_ers_sig(bn_t td, ers_t p, uint8_t *msg, int len, bn_t sk, ec_t pk,
		ec_t pp) {
	bn_t n;
	ec_t t, y[2];
	int result = RLC_OK;

	bn_null(n);
	ec_null(t);
	ec_null(y[0]);
	ec_null(y[1]);

	RLC_TRY {
		bn_new(n);
		ec_new(t);
		ec_new(y[0]);
		ec_new(y[1]);

		ec_curve_get_ord(n);
		bn_rand_mod(td, n);
		ec_mul_gen(t, td);
		ec_sub(t, pp, t);
		ec_norm(p->h, t);

		ec_copy(p->pk, pk);
		ec_copy(y[0], p->h);
		ec_copy(y[1], p->pk);
		cp_sokor_sig(p->c, p->r, msg, len, y, sk, 0);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		ec_free(t);
		ec_free(y[0]);
		ec_free(y[1]);
	}
	return result;
}

int cp_ers_ver(bn_t td, ers_t *s, int size, uint8_t *msg, int len, ec_t pp) {
	bn_t n;
	ec_t t, y[2];
	int flag = 0, result = 0;

	bn_null(n);
	ec_null(t);
	ec_null(y[0]);
	ec_null(y[1]);

	RLC_TRY {
		bn_new(n);
		ec_new(t);
		ec_new(y[0]);
		ec_new(y[1]);

		ec_curve_get_ord(n);
		ec_mul_gen(t, td);

		for (int i = 0; i < size; i++) {
            ec_add(t, t, s[i]->h);
        }
		if (ec_cmp(pp, t) == RLC_EQ) {
			flag = 1;
			for (int i = 0; i < size; i++) {
				ec_copy(y[0], s[i]->h);
				ec_copy(y[1], s[i]->pk);
				flag &= cp_sokor_ver(s[i]->c, s[i]->r, msg, len, y);
	        }
		}
		result = flag;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		ec_free(t);
		ec_free(y[0]);
		ec_free(y[1]);
	}
	return result;
}

int cp_ers_ext(bn_t td, ers_t *p, int *size, uint8_t *msg, int len, ec_t pk,
		ec_t pp) {
	bn_t n, r;
	ec_t y[2];
	int result = RLC_OK;

	bn_null(n);
	bn_null(r);
	ec_null(y[0]);
	ec_null(y[1]);

	for (int i = 0; i < *size; i++) {
		if (ec_cmp(pk, p[i]->pk) == RLC_EQ) {
			return RLC_ERR;
		}
	}

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		ec_new(y[0]);
		ec_new(y[1]);

		ec_curve_get_ord(n);
		bn_rand_mod(r, n);
		bn_sub(td, td, r);
		bn_mod(td, td, n);
		ec_mul_gen(p[*size]->h, r);

		ec_copy(p[*size]->pk, pk);
		ec_copy(y[0], p[*size]->h);
		ec_copy(y[1], p[*size]->pk);
		cp_sokor_sig(p[*size]->c, p[*size]->r, msg, len, y, r, 1);
		(*size)++;
		result = RLC_OK;
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		ec_free(y[0]);
		ec_free(y[1]);
	}
	return result;
}
