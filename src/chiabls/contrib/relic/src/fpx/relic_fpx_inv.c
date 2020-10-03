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
 * Implementation of inversion in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp2_inv(fp2_t c, fp2_t a) {
	fp_t t0, t1;

	fp_null(t0);
	fp_null(t1);

	TRY {
		fp_new(t0);
		fp_new(t1);

		/* t0 = a_0^2, t1 = a_1^2. */
		fp_sqr(t0, a[0]);
		fp_sqr(t1, a[1]);

		/* t1 = 1/(a_0^2 + a_1^2). */
#ifndef FP_QNRES
		if (fp_prime_get_qnr() != -1) {
			if (fp_prime_get_qnr() == -2) {
				fp_dbl(t1, t1);
				fp_add(t0, t0, t1);
			} else {
				if (fp_prime_get_qnr() < 0) {
					fp_mul_dig(t1, t1, -fp_prime_get_qnr());
					fp_add(t0, t0, t1);
				} else {
					fp_mul_dig(t1, t1, fp_prime_get_qnr());
					fp_sub(t0, t0, t1);
				}
			}
		} else {
			fp_add(t0, t0, t1);
		}
#else
		fp_add(t0, t0, t1);
#endif

		fp_inv(t1, t0);

		/* c_0 = a_0/(a_0^2 + a_1^2). */
		fp_mul(c[0], a[0], t1);
		/* c_1 = - a_1/(a_0^2 + a_1^2). */
		fp_mul(c[1], a[1], t1);
		fp_neg(c[1], c[1]);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t0);
		fp_free(t1);
	}
}

void fp2_inv_uni(fp2_t c, fp2_t a) {
	fp_copy(c[0], a[0]);
	fp_neg(c[1], a[1]);
}

void fp2_inv_sim(fp2_t *c, fp2_t *a, int n) {
	int i;
	fp2_t u, t[n];

	for (i = 0; i < n; i++) {
		fp2_null(t[i]);
	}
	fp2_null(u);

	TRY {
		for (i = 0; i < n; i++) {
			fp2_new(t[i]);
		}
		fp2_new(u);

		fp2_copy(c[0], a[0]);
		fp2_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fp2_copy(t[i], a[i]);
			fp2_mul(c[i], c[i - 1], t[i]);
		}

		fp2_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fp2_mul(c[i], c[i - 1], u);
			fp2_mul(u, u, t[i]);
		}
		fp2_copy(c[0], u);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < n; i++) {
			fp2_free(t[i]);
		}
		fp2_free(u);
	}
}

void fp3_inv(fp3_t c, fp3_t a) {
	fp_t v0;
	fp_t v1;
	fp_t v2;
	fp_t t0;

	fp_null(v0);
	fp_null(v1);
	fp_null(v2);
	fp_null(t0);

	TRY {
		fp_new(v0);
		fp_new(v1);
		fp_new(v2);
		fp_new(t0);

		/* v0 = a_0^2 - B * a_1 * a_2. */
		fp_sqr(t0, a[0]);
		fp_mul(v0, a[1], a[2]);
		fp_neg(v2, v0);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(v2, v2, v0);
		}
		fp_sub(v0, t0, v2);

		/* v1 = B * a_2^2 - a_0 * a_1. */
		fp_sqr(t0, a[2]);
		fp_neg(v2, t0);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(v2, v2, t0);
		}
		fp_mul(v1, a[0], a[1]);
		fp_sub(v1, v2, v1);

		/* v2 = a_1^2 - a_0 * a_2. */
		fp_sqr(t0, a[1]);
		fp_mul(v2, a[0], a[2]);
		fp_sub(v2, t0, v2);

		fp_mul(t0, a[1], v2);
		fp_neg(c[1], t0);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(c[1], c[1], t0);
		}

		fp_mul(c[0], a[0], v0);

		fp_mul(t0, a[2], v1);
		fp_neg(c[2], t0);
		for (int i = -1; i > fp_prime_get_cnr(); i--) {
			fp_sub(c[2], c[2], t0);
		}

		fp_add(t0, c[0], c[1]);
		fp_add(t0, t0, c[2]);
		fp_inv(t0, t0);

		fp_mul(c[0], v0, t0);
		fp_mul(c[1], v1, t0);
		fp_mul(c[2], v2, t0);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp_free(v0);
		fp_free(v1);
		fp_free(v2);
		fp_free(t0);
	}
}

void fp3_inv_sim(fp3_t * c, fp3_t * a, int n) {
	int i;
	fp3_t u, t[n];

	for (i = 0; i < n; i++) {
		fp3_null(t[i]);
	}
	fp3_null(u);

	TRY {
		for (i = 0; i < n; i++) {
			fp3_new(t[i]);
		}
		fp3_new(u);

		fp3_copy(c[0], a[0]);
		fp3_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fp3_copy(t[i], a[i]);
			fp3_mul(c[i], c[i - 1], t[i]);
		}

		fp3_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fp3_mul(c[i], c[i - 1], u);
			fp3_mul(u, u, t[i]);
		}
		fp3_copy(c[0], u);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < n; i++) {
			fp3_free(t[i]);
		}
		fp3_free(u);
	}
}

void fp6_inv(fp6_t c, fp6_t a) {
	fp2_t v0;
	fp2_t v1;
	fp2_t v2;
	fp2_t t0;

	fp2_null(v0);
	fp2_null(v1);
	fp2_null(v2);
	fp2_null(t0);

	TRY {
		fp2_new(v0);
		fp2_new(v1);
		fp2_new(v2);
		fp2_new(t0);

		/* v0 = a_0^2 - E * a_1 * a_2. */
		fp2_sqr(t0, a[0]);
		fp2_mul(v0, a[1], a[2]);
		fp2_mul_nor(v2, v0);
		fp2_sub(v0, t0, v2);

		/* v1 = E * a_2^2 - a_0 * a_1. */
		fp2_sqr(t0, a[2]);
		fp2_mul_nor(v2, t0);
		fp2_mul(v1, a[0], a[1]);
		fp2_sub(v1, v2, v1);

		/* v2 = a_1^2 - a_0 * a_2. */
		fp2_sqr(t0, a[1]);
		fp2_mul(v2, a[0], a[2]);
		fp2_sub(v2, t0, v2);

		fp2_mul(t0, a[1], v2);
		fp2_mul_nor(c[1], t0);

		fp2_mul(c[0], a[0], v0);

		fp2_mul(t0, a[2], v1);
		fp2_mul_nor(c[2], t0);

		fp2_add(t0, c[0], c[1]);
		fp2_add(t0, t0, c[2]);
		fp2_inv(t0, t0);

		fp2_mul(c[0], v0, t0);
		fp2_mul(c[1], v1, t0);
		fp2_mul(c[2], v2, t0);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp2_free(v0);
		fp2_free(v1);
		fp2_free(v2);
		fp2_free(t0);
	}
}

void fp12_inv(fp12_t c, fp12_t a) {
	fp6_t t0;
	fp6_t t1;

	fp6_null(t0);
	fp6_null(t1);

	TRY {
		fp6_new(t0);
		fp6_new(t1);

		fp6_sqr(t0, a[0]);
		fp6_sqr(t1, a[1]);
		fp6_mul_art(t1, t1);
		fp6_sub(t0, t0, t1);
		fp6_inv(t0, t0);

		fp6_mul(c[0], a[0], t0);
		fp6_neg(c[1], a[1]);
		fp6_mul(c[1], c[1], t0);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
	}
}

void fp12_inv_uni(fp12_t c, fp12_t a) {
	/* In this case, it's a simple conjugate. */
	fp6_copy(c[0], a[0]);
	fp6_neg(c[1], a[1]);
}

void fp18_inv(fp18_t c, fp18_t a) {
	fp6_t v0;
	fp6_t v1;
	fp6_t v2;
	fp6_t t0;

	fp6_null(v0);
	fp6_null(v1);
	fp6_null(v2);
	fp6_null(t0);

	TRY {
		fp6_new(v0);
		fp6_new(v1);
		fp6_new(v2);
		fp6_new(t0);

		/* v0 = a_0^2 - E * a_1 * a_2. */
		fp6_sqr(t0, a[0]);
		fp6_mul(v0, a[1], a[2]);
		fp6_mul_art(v2, v0);
		fp6_sub(v0, t0, v2);

		/* v1 = E * a_2^2 - a_0 * a_1. */
		fp6_sqr(t0, a[2]);
		fp6_mul_art(v2, t0);
		fp6_mul(v1, a[0], a[1]);
		fp6_sub(v1, v2, v1);

		/* v2 = a_1^2 - a_0 * a_2. */
		fp6_sqr(t0, a[1]);
		fp6_mul(v2, a[0], a[2]);
		fp6_sub(v2, t0, v2);

		fp6_mul(t0, a[1], v2);
		fp6_mul_art(c[1], t0);

		fp6_mul(c[0], a[0], v0);

		fp6_mul(t0, a[2], v1);
		fp6_mul_art(c[2], t0);

		fp6_add(t0, c[0], c[1]);
		fp6_add(t0, t0, c[2]);
		fp6_inv(t0, t0);

		fp6_mul(c[0], v0, t0);
		fp6_mul(c[1], v1, t0);
		fp6_mul(c[2], v2, t0);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(v0);
		fp6_free(v1);
		fp6_free(v2);
		fp6_free(t0);
	}
}

void fp18_inv_uni(fp18_t c, fp18_t a) {
	fp_copy(c[0][0][0], a[0][0][0]);
	fp_copy(c[0][2][0], a[0][2][0]);
	fp_copy(c[0][1][1], a[0][1][1]);
	fp_neg(c[0][1][0], a[0][1][0]);
	fp_neg(c[0][0][1], a[0][0][1]);
	fp_neg(c[0][2][1], a[0][2][1]);

	fp_neg(c[1][0][0], a[1][0][0]);
	fp_neg(c[1][2][0], a[1][2][0]);
	fp_neg(c[1][1][1], a[1][1][1]);
	fp_copy(c[1][1][0], a[1][1][0]);
	fp_copy(c[1][0][1], a[1][0][1]);
	fp_copy(c[1][2][1], a[1][2][1]);

	fp_copy(c[2][0][0], a[2][0][0]);
	fp_copy(c[2][2][0], a[2][2][0]);
	fp_copy(c[2][1][1], a[2][1][1]);
	fp_neg(c[2][1][0], a[2][1][0]);
	fp_neg(c[2][0][1], a[2][0][1]);
	fp_neg(c[2][2][1], a[2][2][1]);
}
