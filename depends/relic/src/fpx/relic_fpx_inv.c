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

	RLC_TRY {
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
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
	}
}

void fp2_inv_cyc(fp2_t c, fp2_t a) {
	fp_copy(c[0], a[0]);
	fp_neg(c[1], a[1]);
}

void fp2_inv_sim(fp2_t *c, fp2_t *a, int n) {
	int i;
	fp2_t u, *t = RLC_ALLOCA(fp2_t, n);

	for (i = 0; i < n; i++) {
		fp2_null(t[i]);
	}
	fp2_null(u);

	RLC_TRY {
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
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < n; i++) {
			fp2_free(t[i]);
		}
		fp2_free(u);
		RLC_FREE(t);
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

	RLC_TRY {
		fp_new(v0);
		fp_new(v1);
		fp_new(v2);
		fp_new(t0);

		/* v0 = a_0^2 - B * a_1 * a_2. */
		fp_sqr(t0, a[0]);
		fp_mul(v0, a[1], a[2]);
		fp_copy(v2, v0);
		for (int i = 1; i < fp_prime_get_cnr(); i++) {
			fp_add(v2, v2, v0);
		}
		for (int i = 0; i >= fp_prime_get_cnr(); i--) {
			fp_sub(v2, v2, v0);
		}
		fp_sub(v0, t0, v2);

		/* v1 = B * a_2^2 - a_0 * a_1. */
		fp_sqr(t0, a[2]);
		fp_copy(v2, t0);
		for (int i = 1; i < fp_prime_get_cnr(); i++) {
			fp_add(v2, v2, t0);
		}
		for (int i = 0; i >= fp_prime_get_cnr(); i--) {
			fp_sub(v2, v2, t0);
		}
		fp_mul(v1, a[0], a[1]);
		fp_sub(v1, v2, v1);

		/* v2 = a_1^2 - a_0 * a_2. */
		fp_sqr(t0, a[1]);
		fp_mul(v2, a[0], a[2]);
		fp_sub(v2, t0, v2);

		fp_mul(t0, a[1], v2);
		fp_copy(c[1], t0);
		for (int i = 1; i < fp_prime_get_cnr(); i++) {
			fp_add(c[1], c[1], t0);
		}
		for (int i = 0; i >= fp_prime_get_cnr(); i--) {
			fp_sub(c[1], c[1], t0);
		}

		fp_mul(c[0], a[0], v0);

		fp_mul(t0, a[2], v1);
		fp_copy(c[2], t0);
		for (int i = 1; i < fp_prime_get_cnr(); i++) {
			fp_add(c[2], c[2], t0);
		}
		for (int i = 0; i >= fp_prime_get_cnr(); i--) {
			fp_sub(c[2], c[2], t0);
		}

		fp_add(t0, c[0], c[1]);
		fp_add(t0, t0, c[2]);
		fp_inv(t0, t0);

		fp_mul(c[0], v0, t0);
		fp_mul(c[1], v1, t0);
		fp_mul(c[2], v2, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(v0);
		fp_free(v1);
		fp_free(v2);
		fp_free(t0);
	}
}

void fp3_inv_sim(fp3_t * c, fp3_t * a, int n) {
	int i;
	fp3_t u, *t = RLC_ALLOCA(fp3_t, n);

	for (i = 0; i < n; i++) {
		fp3_null(t[i]);
	}
	fp3_null(u);

	RLC_TRY {
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
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < n; i++) {
			fp3_free(t[i]);
		}
		fp3_free(u);
		RLC_FREE(t);
	}
}

void fp4_inv_cyc(fp4_t c, fp4_t a) {
	fp2_copy(c[0], a[0]);
	fp2_neg(c[1], a[1]);
}

void fp4_inv(fp4_t c, fp4_t a) {
	fp2_t t0;
	fp2_t t1;

	fp2_null(t0);
	fp2_null(t1);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);

		fp2_sqr(t0, a[0]);
		fp2_sqr(t1, a[1]);
		fp2_mul_nor(t1, t1);
		fp2_sub(t0, t0, t1);
		fp2_inv(t0, t0);

		fp2_mul(c[0], a[0], t0);
		fp2_neg(c[1], a[1]);
		fp2_mul(c[1], c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
	}
}

void fp4_inv_sim(fp4_t * c, fp4_t * a, int n) {
	int i;
	fp4_t u, *t = RLC_ALLOCA(fp4_t, n);

	for (i = 0; i < n; i++) {
		fp4_null(t[i]);
	}
	fp4_null(u);

	RLC_TRY {
		for (i = 0; i < n; i++) {
			fp4_new(t[i]);
		}
		fp4_new(u);

		fp4_copy(c[0], a[0]);
		fp4_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fp4_copy(t[i], a[i]);
			fp4_mul(c[i], c[i - 1], t[i]);
		}

		fp4_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fp4_mul(c[i], c[i - 1], u);
			fp4_mul(u, u, t[i]);
		}
		fp4_copy(c[0], u);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < n; i++) {
			fp4_free(t[i]);
		}
		fp4_free(u);
		RLC_FREE(t);
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

	RLC_TRY {
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
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(v0);
		fp2_free(v1);
		fp2_free(v2);
		fp2_free(t0);
	}
}

void fp8_inv_cyc(fp8_t c, fp8_t a) {
	fp4_copy(c[0], a[0]);
	fp4_neg(c[1], a[1]);
}

void fp8_inv(fp8_t c, fp8_t a) {
	fp4_t t0;
	fp4_t t1;

	fp4_null(t0);
	fp4_null(t1);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);

		fp4_sqr(t0, a[0]);
		fp4_sqr(t1, a[1]);
		fp4_mul_art(t1, t1);
		fp4_sub(t0, t0, t1);
		fp4_inv(t0, t0);

		fp4_mul(c[0], a[0], t0);
		fp4_neg(c[1], a[1]);
		fp4_mul(c[1], c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
	}
}

void fp8_inv_sim(fp8_t *c, fp8_t *a, int n) {
	int i;
	fp8_t u, *t = RLC_ALLOCA(fp8_t, n);

	for (i = 0; i < n; i++) {
		fp8_null(t[i]);
	}
	fp8_null(u);

	RLC_TRY {
		for (i = 0; i < n; i++) {
			fp8_new(t[i]);
		}
		fp8_new(u);

		fp8_copy(c[0], a[0]);
		fp8_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fp8_copy(t[i], a[i]);
			fp8_mul(c[i], c[i - 1], t[i]);
		}

		fp8_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fp8_mul(c[i], c[i - 1], u);
			fp8_mul(u, u, t[i]);
		}
		fp8_copy(c[0], u);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < n; i++) {
			fp8_free(t[i]);
		}
		fp8_free(u);
		RLC_FREE(t);
	}
}

void fp9_inv(fp9_t c, fp9_t a) {
	fp3_t v0;
	fp3_t v1;
	fp3_t v2;
	fp3_t t0;

	fp3_null(v0);
	fp3_null(v1);
	fp3_null(v2);
	fp3_null(t0);

	RLC_TRY {
		fp3_new(v0);
		fp3_new(v1);
		fp3_new(v2);
		fp3_new(t0);

		/* v0 = a_0^2 - E * a_1 * a_2. */
		fp3_sqr(t0, a[0]);
		fp3_mul(v0, a[1], a[2]);
		fp3_mul_nor(v2, v0);
		fp3_sub(v0, t0, v2);

		/* v1 = E * a_2^2 - a_0 * a_1. */
		fp3_sqr(t0, a[2]);
		fp3_mul_nor(v2, t0);
		fp3_mul(v1, a[0], a[1]);
		fp3_sub(v1, v2, v1);

		/* v2 = a_1^2 - a_0 * a_2. */
		fp3_sqr(t0, a[1]);
		fp3_mul(v2, a[0], a[2]);
		fp3_sub(v2, t0, v2);

		fp3_mul(t0, a[1], v2);
		fp3_mul_nor(c[1], t0);

		fp3_mul(c[0], a[0], v0);

		fp3_mul(t0, a[2], v1);
		fp3_mul_nor(c[2], t0);

		fp3_add(t0, c[0], c[1]);
		fp3_add(t0, t0, c[2]);
		fp3_inv(t0, t0);

		fp3_mul(c[0], v0, t0);
		fp3_mul(c[1], v1, t0);
		fp3_mul(c[2], v2, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp3_free(v0);
		fp3_free(v1);
		fp3_free(v2);
		fp3_free(t0);
	}
}

void fp9_inv_sim(fp9_t * c, fp9_t * a, int n) {
	int i;
	fp9_t u, *t = RLC_ALLOCA(fp9_t, n);

	for (i = 0; i < n; i++) {
		fp9_null(t[i]);
	}
	fp9_null(u);

	RLC_TRY {
		for (i = 0; i < n; i++) {
			fp9_new(t[i]);
		}
		fp9_new(u);

		fp9_copy(c[0], a[0]);
		fp9_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fp9_copy(t[i], a[i]);
			fp9_mul(c[i], c[i - 1], t[i]);
		}

		fp9_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fp9_mul(c[i], c[i - 1], u);
			fp9_mul(u, u, t[i]);
		}
		fp9_copy(c[0], u);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < n; i++) {
			fp9_free(t[i]);
		}
		fp9_free(u);
		RLC_FREE(t);
	}
}

void fp12_inv(fp12_t c, fp12_t a) {
	fp6_t t0;
	fp6_t t1;

	fp6_null(t0);
	fp6_null(t1);

	RLC_TRY {
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
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp6_free(t0);
		fp6_free(t1);
	}
}

void fp12_inv_cyc(fp12_t c, fp12_t a) {
	fp6_copy(c[0], a[0]);
	fp6_neg(c[1], a[1]);
}

void fp18_inv(fp18_t c, fp18_t a) {
	fp9_t t0;
	fp9_t t1;

	fp9_null(t0);
	fp9_null(t1);

	RLC_TRY {
		fp9_new(t0);
		fp9_new(t1);

		fp9_sqr(t0, a[0]);
		fp9_sqr(t1, a[1]);
		fp9_mul_art(t1, t1);
		fp9_sub(t0, t0, t1);
		fp9_inv(t0, t0);

		fp9_mul(c[0], a[0], t0);
		fp9_neg(c[1], a[1]);
		fp9_mul(c[1], c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp9_free(t0);
		fp9_free(t1);
	}
}

void fp18_inv_cyc(fp18_t c, fp18_t a) {
	fp9_copy(c[0], a[0]);
	fp9_neg(c[1], a[1]);
}

void fp24_inv(fp24_t c, fp24_t a) {
	fp8_t v0;
	fp8_t v1;
	fp8_t v2;
	fp8_t t0;

	fp8_null(v0);
	fp8_null(v1);
	fp8_null(v2);
	fp8_null(t0);

	RLC_TRY {
		fp8_new(v0);
		fp8_new(v1);
		fp8_new(v2);
		fp8_new(t0);

		/* v0 = a_0^2 - E * a_1 * a_2. */
		fp8_sqr(t0, a[0]);
		fp8_mul(v0, a[1], a[2]);
		fp8_mul_art(v2, v0);
		fp8_sub(v0, t0, v2);

		/* v1 = E * a_2^2 - a_0 * a_1. */
		fp8_sqr(t0, a[2]);
		fp8_mul_art(v2, t0);
		fp8_mul(v1, a[0], a[1]);
		fp8_sub(v1, v2, v1);

		/* v2 = a_1^2 - a_0 * a_2. */
		fp8_sqr(t0, a[1]);
		fp8_mul(v2, a[0], a[2]);
		fp8_sub(v2, t0, v2);

		fp8_mul(t0, a[1], v2);
		fp8_mul_art(c[1], t0);

		fp8_mul(c[0], a[0], v0);

		fp8_mul(t0, a[2], v1);
		fp8_mul_art(c[2], t0);

		fp8_add(t0, c[0], c[1]);
		fp8_add(t0, t0, c[2]);
		fp8_inv(t0, t0);

		fp8_mul(c[0], v0, t0);
		fp8_mul(c[1], v1, t0);
		fp8_mul(c[2], v2, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(v0);
		fp8_free(v1);
		fp8_free(v2);
		fp8_free(t0);
	}
}

void fp24_inv_cyc(fp24_t c, fp24_t a) {
	fp8_inv_cyc(c[0], a[0]);
	fp8_inv_cyc(c[1], a[1]);
	fp8_neg(c[1], c[1]);
	fp8_inv_cyc(c[2], a[2]);
}

void fp48_inv(fp48_t c, fp48_t a) {
	fp24_t t0;
	fp24_t t1;

	fp24_null(t0);
	fp24_null(t1);

	RLC_TRY {
		fp24_new(t0);
		fp24_new(t1);

		fp24_sqr(t0, a[0]);
		fp24_sqr(t1, a[1]);
		fp24_mul_art(t1, t1);
		fp24_sub(t0, t0, t1);
		fp24_inv(t0, t0);

		fp24_mul(c[0], a[0], t0);
		fp24_neg(c[1], a[1]);
		fp24_mul(c[1], c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp24_free(t0);
		fp24_free(t1);
	}
}

void fp48_inv_cyc(fp48_t c, fp48_t a) {
	fp24_copy(c[0], a[0]);
	fp24_neg(c[1], a[1]);
}

void fp54_inv(fp54_t c, fp54_t a) {
	fp18_t v0;
	fp18_t v1;
	fp18_t v2;
	fp18_t t0;

	fp18_null(v0);
	fp18_null(v1);
	fp18_null(v2);
	fp18_null(t0);

	RLC_TRY {
		fp18_new(v0);
		fp18_new(v1);
		fp18_new(v2);
		fp18_new(t0);

		/* v0 = a_0^2 - E * a_1 * a_2. */
		fp18_sqr(t0, a[0]);
		fp18_mul(v0, a[1], a[2]);
		fp18_mul_art(v2, v0);
		fp18_sub(v0, t0, v2);

		/* v1 = E * a_2^2 - a_0 * a_1. */
		fp18_sqr(t0, a[2]);
		fp18_mul_art(v2, t0);
		fp18_mul(v1, a[0], a[1]);
		fp18_sub(v1, v2, v1);

		/* v2 = a_1^2 - a_0 * a_2. */
		fp18_sqr(t0, a[1]);
		fp18_mul(v2, a[0], a[2]);
		fp18_sub(v2, t0, v2);

		fp18_mul(t0, a[1], v2);
		fp18_mul_art(c[1], t0);

		fp18_mul(c[0], a[0], v0);

		fp18_mul(t0, a[2], v1);
		fp18_mul_art(c[2], t0);

		fp18_add(t0, c[0], c[1]);
		fp18_add(t0, t0, c[2]);
		fp18_inv(t0, t0);

		fp18_mul(c[0], v0, t0);
		fp18_mul(c[1], v1, t0);
		fp18_mul(c[2], v2, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp18_free(v0);
		fp18_free(v1);
		fp18_free(v2);
		fp18_free(t0);
	}
}

void fp54_inv_cyc(fp54_t c, fp54_t a) {
	fp18_inv_cyc(c[0], a[0]);
	fp18_inv_cyc(c[1], a[1]);
	fp18_neg(c[1], c[1]);
	fp18_inv_cyc(c[2], a[2]);
}
