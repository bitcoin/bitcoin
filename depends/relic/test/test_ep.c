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
 * Tests for arithmetic on prime elliptic curves.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = RLC_ERR;
	ep_t a;

	ep_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			ep_new(a);
			ep_free(a);
		} TEST_END;
	} RLC_CATCH(e) {
		switch (e) {
			case ERR_NO_MEMORY:
				util_print("FATAL ERROR!\n");
				RLC_ERROR(end);
				break;
		}
	}
	(void)a;
	code = RLC_OK;
  end:
	return code;
}

static int util(void) {
	int l, code = RLC_ERR;
	ep_t a, b, c;
	uint8_t bin[2 * RLC_FP_BYTES + 1];

	ep_null(a);
	ep_null(b);
	ep_null(c);

	RLC_TRY {
		ep_new(a);
		ep_new(b);
		ep_new(c);

		TEST_CASE("copy and comparison are consistent") {
			ep_rand(a);
			ep_rand(b);
			ep_rand(c);
			/* Compare points in affine coordinates. */
			if (ep_cmp(a, c) != RLC_EQ) {
				ep_copy(c, a);
				TEST_ASSERT(ep_cmp(c, a) == RLC_EQ, end);
			}
			if (ep_cmp(b, c) != RLC_EQ) {
				ep_copy(c, b);
				TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
			}
			/* Compare with one point in projective. */
			ep_dbl(c, a);
			ep_norm(c, c);
			ep_dbl(a, a);
			TEST_ASSERT(ep_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(ep_cmp(a, c) == RLC_EQ, end);
			/* Compare with two points in projective. */
			ep_dbl(c, c);
			ep_dbl(a, a);
			TEST_ASSERT(ep_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(ep_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("negation and comparison are consistent") {
			ep_rand(a);
			ep_neg(b, a);
			TEST_ASSERT(ep_cmp(a, b) != RLC_EQ, end);
			ep_neg(b, b);
			TEST_ASSERT(ep_cmp(a, b) == RLC_EQ, end);
			/* Compare with infinity. */
			ep_neg(b, a);
			ep_add(a, a, b);
			ep_set_infty(b);
			TEST_ASSERT(ep_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to random and comparison are consistent") {
			ep_rand(a);
			ep_set_infty(c);
			TEST_ASSERT(ep_cmp(a, c) != RLC_EQ, end);
			TEST_ASSERT(ep_cmp(c, a) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to infinity and infinity test are consistent") {
			ep_set_infty(a);
			TEST_ASSERT(ep_is_infty(a), end);
		}
		TEST_END;

		TEST_CASE("validity test is correct") {
			ep_set_infty(a);
			TEST_ASSERT(ep_on_curve(a), end);
			ep_rand(a);
			TEST_ASSERT(ep_on_curve(a), end);
			fp_rand(a->x);
			TEST_ASSERT(!ep_on_curve(a), end);
		}
		TEST_END;

		TEST_CASE("blinding is consistent") {
			ep_rand(a);
			ep_blind(a, a);
			TEST_ASSERT(ep_on_curve(a), end);
		} TEST_END;

		TEST_CASE("reading and writing a point are consistent") {
			for (int j = 0; j < 2; j++) {
				ep_set_infty(a);
				l = ep_size_bin(a, j);
				ep_write_bin(bin, l, a, j);
				ep_read_bin(b, bin, l);
				TEST_ASSERT(ep_cmp(a, b) == RLC_EQ, end);
				ep_rand(a);
				l = ep_size_bin(a, j);
				ep_write_bin(bin, l, a, j);
				ep_read_bin(b, bin, l);
				TEST_ASSERT(ep_cmp(a, b) == RLC_EQ, end);
				ep_rand(a);
				ep_dbl(a, a);
				l = ep_size_bin(a, j);
				ep_norm(a, a);
				ep_write_bin(bin, l, a, j);
				ep_read_bin(b, bin, l);
				TEST_ASSERT(ep_cmp(a, b) == RLC_EQ, end);
			}
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ep_free(a);
	ep_free(b);
	ep_free(c);
	return code;
}

static int addition(void) {
	int code = RLC_ERR;
	ep_t a, b, c, d, e;

	ep_null(a);
	ep_null(b);
	ep_null(c);
	ep_null(d);
	ep_null(e);

	RLC_TRY {
		ep_new(a);
		ep_new(b);
		ep_new(c);
		ep_new(d);
		ep_new(e);

		TEST_CASE("point addition is commutative") {
			ep_rand(a);
			ep_rand(b);
			ep_add(d, a, b);
			ep_add(e, b, a);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition is associative") {
			ep_rand(a);
			ep_rand(b);
			ep_rand(c);
			ep_add(d, a, b);
			ep_add(d, d, c);
			ep_add(e, b, c);
			ep_add(e, e, a);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has identity") {
			ep_rand(a);
			ep_set_infty(d);
			ep_add(e, a, d);
			TEST_ASSERT(ep_cmp(a, e) == RLC_EQ, end);
			ep_add(e, d, a);
			TEST_ASSERT(ep_cmp(a, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has inverse") {
			ep_rand(a);
			ep_neg(d, a);
			ep_add(e, a, d);
			TEST_ASSERT(ep_is_infty(e), end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("point addition in affine coordinates is correct") {
			ep_rand(a);
			ep_rand(b);
			ep_add(d, a, b);
			ep_add_basic(e, a, b);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
#if !defined(EP_MIXED) || !defined(STRIP)
		TEST_CASE("point addition in projective coordinates is correct") {
			ep_rand(a);
			ep_rand(b);
			ep_rand(c);
			ep_add_projc(a, a, b);
			ep_add_projc(b, b, c);
			/* a and b in projective coordinates. */
			ep_add_projc(d, a, b);
			/* normalize before mixing coordinates. */
			ep_norm(a, a);
			ep_norm(b, b);
			ep_add(e, a, b);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("point addition in mixed coordinates (z2 = 1) is correct") {
			ep_rand(a);
			ep_rand(b);
			/* a in projective, b in affine coordinates. */
			ep_add_projc(a, a, b);
			ep_add_projc(d, a, b);
			/* a in affine coordinates. */
			ep_norm(a, a);
			ep_add(e, a, b);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition in mixed coordinates (z1,z2 = 1) is correct") {
			ep_rand(a);
			ep_rand(b);
			/* a and b in affine coordinates. */
			ep_add(d, a, b);
			ep_add_projc(e, a, b);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == JACOB || !defined(STRIP)
#if !defined(EP_MIXED) || !defined(STRIP)
		TEST_CASE("point addition in jacobian coordinates is correct") {
			ep_rand(a);
			ep_rand(b);
			ep_rand(c);
			ep_add_jacob(a, a, b);
			ep_add_jacob(b, b, c);
			/* a and b in projective coordinates. */
			ep_add_jacob(d, a, b);
			ep_norm(a, a);
			ep_norm(b, b);
			ep_add(e, a, b);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("point addition in mixed coordinates (z2 = 1) is correct") {
			ep_rand(a);
			ep_rand(b);
			/* a in projective, b in affine coordinates. */
			ep_add_jacob(a, a, b);
			ep_add_jacob(d, a, b);
			/* a in affine coordinates. */
			ep_norm(a, a);
			ep_add(e, a, b);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition in mixed coordinates (z1,z2 = 1) is correct") {
			ep_rand(a);
			ep_rand(b);
			ep_norm(a, a);
			ep_norm(b, b);
			/* a and b in affine coordinates. */
			ep_add(d, a, b);
			ep_add_jacob(e, a, b);
			TEST_ASSERT(ep_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ep_free(a);
	ep_free(b);
	ep_free(c);
	ep_free(d);
	ep_free(e);
	return code;
}

static int subtraction(void) {
	int code = RLC_ERR;
	ep_t a, b, c, d;

	ep_null(a);
	ep_null(b);
	ep_null(c);
	ep_null(d);

	RLC_TRY {
		ep_new(a);
		ep_new(b);
		ep_new(c);
		ep_new(d);

		TEST_CASE("point subtraction is anti-commutative") {
			ep_rand(a);
			ep_rand(b);
			ep_sub(c, a, b);
			ep_sub(d, b, a);
			ep_neg(d, d);
			TEST_ASSERT(ep_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has identity") {
			ep_rand(a);
			ep_set_infty(c);
			ep_sub(d, a, c);
			TEST_ASSERT(ep_cmp(d, a) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has inverse") {
			ep_rand(a);
			ep_sub(c, a, a);
			TEST_ASSERT(ep_is_infty(c), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ep_free(a);
	ep_free(b);
	ep_free(c);
	ep_free(d);
	return code;
}

static int doubling(void) {
	int code = RLC_ERR;
	ep_t a, b, c;

	ep_null(a);
	ep_null(b);
	ep_null(c);

	RLC_TRY {
		ep_new(a);
		ep_new(b);
		ep_new(c);

		TEST_CASE("point doubling is correct") {
			ep_rand(a);
			ep_add(b, a, a);
			ep_dbl(c, a);
			TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("point doubling in affine coordinates is correct") {
			ep_rand(a);
			ep_dbl(b, a);
			ep_dbl_basic(c, a);
			TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
		TEST_CASE("point doubling in projective coordinates is correct") {
			ep_rand(a);
			/* a in projective coordinates. */
			ep_dbl_projc(a, a);
			ep_dbl_projc(b, a);
			ep_norm(a, a);
			ep_dbl(c, a);
			TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point doubling in mixed coordinates (z1 = 1) is correct") {
			ep_rand(a);
			ep_dbl_projc(b, a);
			ep_norm(b, b);
			ep_dbl(c, a);
			TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
		TEST_CASE("point doubling in jacobian coordinates is correct") {
			ep_rand(a);
			/* a in projective coordinates. */
			ep_dbl_jacob(a, a);
			ep_dbl_jacob(b, a);
			ep_norm(a, a);
			ep_dbl(c, a);
			TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point doubling in mixed coordinates (z1 = 1) is correct") {
			ep_rand(a);
			ep_dbl_jacob(b, a);
			ep_norm(b, b);
			ep_dbl(c, a);
			TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ep_free(a);
	ep_free(b);
	ep_free(c);
	return code;
}

static int endomorphism(void) {
	int code = RLC_ERR;
	bn_t l, n, v1[3], v2[3];
	ep_t a, b, c;

	bn_null(l);
	bn_null(n);
	ep_null(a);
	ep_null(b);
	ep_null(c);

	for (int k = 0; k < 3; k++) {
		bn_null(v1[k]);
		bn_null(v2[k]);
	}

	RLC_TRY {
		bn_new(l);
		bn_new(n);
		ep_new(a);
		ep_new(b);
		ep_new(c);

		for (int k = 0; k < 3; k++) {
			bn_new(v1[k]);
			bn_new(v2[k]);
		}

#if defined(EP_ENDOM)
		if (ep_curve_is_endom()) {
			/* Recover lambda parameter. */
			ep_curve_get_v1(v1);
			ep_curve_get_v2(v2);
			ep_curve_get_ord(v2[0]);
			if (bn_cmp_dig(v1[2], 1) == RLC_EQ) {
				bn_gcd_ext(v1[0], v2[1], NULL, v1[1], v2[0]);
			} else {
				bn_gcd_ext(v1[0], v2[1], NULL, v1[2], v2[0]);
			}
			if (bn_sign(v2[1]) == RLC_NEG) {
				/* Negate modulo r. */
				bn_add(v2[1], v2[0], v2[1]);
			}
			bn_mul(v1[0], v2[1], v1[1]);
			bn_mod(l, v1[0], v2[0]);
			bn_sub(v1[1], v2[0], l);
			if (bn_cmp(v1[1], l) == RLC_LT) {
				bn_copy(l, v1[1]);
			}

			TEST_CASE("endomorphism is correct") {
				/* Test if \psi(P) = [l]P. */
				ep_rand(a);
				ep_psi(b, a);
				ep_mul(c, a, l);
				TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
			}
			TEST_END;

#if EB_ADD == BASIC || !defined(STRIP)
			TEST_CASE("endomorphism in affine coordinates is correct") {
				ep_rand(a);
				ep_psi(b, a);
				ep_mul(c, a, l);
				TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
			}
			TEST_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
			TEST_CASE("endomorphism in projective coordinates is correct") {
				ep_rand(a);
				ep_dbl_projc(a, a);
				ep_norm(a, a);
				ep_psi(b, a);
				ep_mul(c, a, l);
				TEST_ASSERT(ep_cmp(b, c) == RLC_EQ, end);
			}
			TEST_END;
#endif
		}
#endif /* EP_ENDOM */
	(void)a;
	(void)b;
	(void)c;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(l);
	bn_free(n);
	ep_free(a);
	ep_free(b);
	ep_free(c);
	for (int k = 0; k < 3; k++) {
		bn_free(v1[k]);
		bn_free(v2[k]);
	}

	return code;
}

static int multiplication(void) {
	int code = RLC_ERR;
	bn_t n, k;
	ep_t p, q, r;

	bn_null(n);
	bn_null(k);
	ep_null(p);
	ep_null(q);
	ep_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep_new(q);
		ep_new(r);

		ep_curve_get_gen(p);
		ep_curve_get_ord(n);

		TEST_ONCE("generator has the right order") {
			TEST_ASSERT(ep_on_curve(p), end);
			ep_mul(r, p, n);
			TEST_ASSERT(ep_is_infty(r) == 1, end);
		} TEST_END;

		TEST_CASE("generator multiplication is correct") {
			bn_zero(k);
			ep_mul_gen(r, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_gen(r, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul(q, p, k);
			ep_mul_gen(r, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_gen(r, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_gen(q, k);
			bn_add(k, k, n);
			ep_mul_gen(r, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

#if EP_MUL == BASIC || !defined(STRIP)
		TEST_CASE("binary point multiplication is correct") {
			bn_zero(k);
			ep_mul_basic(r, p, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_basic(r, p, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			ep_rand(p);
			ep_mul_basic(r, p, n);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_rand_mod(k, n);
			ep_mul(q, p, k);
			ep_mul_basic(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_basic(r, p, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_basic(q, p, k);
			bn_add(k, k, n);
			ep_mul_basic(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_MUL == MONTY || !defined(STRIP)
		TEST_CASE("sliding window point multiplication is correct") {
			bn_zero(k);
			ep_mul_slide(r, p, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_slide(r, p, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			ep_rand(p);
			ep_mul_slide(r, p, n);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_rand_mod(k, n);
			ep_mul(q, p, k);
			ep_mul_slide(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_slide(r, p, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_slide(q, p, k);
			bn_add(k, k, n);
			ep_mul_slide(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if EP_MUL == MONTY || !defined(STRIP)
		TEST_CASE("montgomery ladder point multiplication is correct") {
			bn_zero(k);
			ep_mul_monty(r, p, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_monty(r, p, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			ep_rand(p);
			ep_mul_monty(r, p, n);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_rand_mod(k, n);
			ep_mul(q, p, k);
			ep_mul_monty(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_monty(r, p, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_monty(q, p, k);
			bn_add(k, k, n);
			ep_mul_monty(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if EP_MUL == LWNAF || !defined(STRIP)
		TEST_CASE("left-to-right w-naf point multiplication is correct") {
			bn_zero(k);
			ep_mul_lwnaf(r, p, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_lwnaf(r, p, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			ep_rand(p);
			ep_mul_lwnaf(r, p, n);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_rand_mod(k, n);
			ep_mul(q, p, k);
			ep_mul_lwnaf(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_lwnaf(r, p, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_lwnaf(q, p, k);
			bn_add(k, k, n);
			ep_mul_lwnaf(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if EP_MUL == LWREG || !defined(STRIP)
		TEST_CASE("left-to-right regular point multiplication is correct") {
			bn_zero(k);
			ep_mul_lwreg(r, p, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_lwreg(r, p, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			ep_rand(p);
			ep_mul_lwreg(r, p, n);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_rand_mod(k, n);
			ep_mul(q, p, k);
			ep_mul_lwreg(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_lwreg(r, p, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_lwreg(q, p, k);
			bn_add(k, k, n);
			ep_mul_lwreg(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

		TEST_CASE("point multiplication by digit is correct") {
			ep_mul_dig(r, p, 0);
			TEST_ASSERT(ep_is_infty(r), end);
			ep_mul_dig(r, p, 1);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			bn_rand(k, RLC_POS, RLC_DIG);
			ep_mul(q, p, k);
			ep_mul_dig(r, p, k->dp[0]);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep_free(q);
	ep_free(r);
	return code;
}

static int fixed(void) {
	int code = RLC_ERR;
	bn_t n, k;
	ep_t p, q, r, t[RLC_EP_TABLE_MAX];

	bn_null(n);
	bn_null(k);
	ep_null(p);
	ep_null(q);
	ep_null(r);

	for (int i = 0; i < RLC_EP_TABLE_MAX; i++) {
		ep_null(t[i]);
	}

	RLC_TRY {
		ep_new(p);
		ep_new(q);
		ep_new(r);
		bn_new(n);
		bn_new(k);

		ep_curve_get_gen(p);
		ep_curve_get_ord(n);

		for (int i = 0; i < RLC_EP_TABLE; i++) {
			ep_new(t[i]);
		}
		TEST_CASE("fixed point multiplication is correct") {
			ep_rand(p);
			ep_mul_pre(t, p);
			bn_zero(k);
			ep_mul_fix(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_fix(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul(q, p, k);
			ep_mul_fix(q, (const ep_t *)t, k);
			ep_mul(r, p, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_fix(r, (const ep_t *)t, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EP_TABLE; i++) {
			ep_free(t[i]);
		}

#if EP_FIX == BASIC || !defined(STRIP)
		for (int i = 0; i < RLC_EP_TABLE_BASIC; i++) {
			ep_new(t[i]);
		}
		TEST_CASE("binary fixed point multiplication is correct") {
			ep_rand(p);
			ep_mul_pre_basic(t, p);
			bn_zero(k);
			ep_mul_fix_basic(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_fix_basic(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul(r, p, k);
			ep_mul_fix_basic(q, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_fix_basic(r, (const ep_t *)t, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_fix_basic(q, (const ep_t *)t, k);
			bn_add(k, k, n);
			ep_mul_fix_basic(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EP_TABLE_BASIC; i++) {
			ep_free(t[i]);
		}
#endif

#if EP_FIX == COMBS || !defined(STRIP)
		for (int i = 0; i < RLC_EP_TABLE_COMBS; i++) {
			ep_new(t[i]);
		}
		TEST_CASE("single-table comb fixed point multiplication is correct") {
			ep_rand(p);
			ep_mul_pre_combs(t, p);
			bn_zero(k);
			ep_mul_fix_combs(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_fix_combs(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul(r, p, k);
			ep_mul_fix_combs(q, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_fix_combs(r, (const ep_t *)t, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_fix_combs(q, (const ep_t *)t, k);
			bn_add(k, k, n);
			ep_mul_fix_combs(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EP_TABLE_COMBS; i++) {
			ep_free(t[i]);
		}
#endif

#if EP_FIX == COMBD || !defined(STRIP)
		for (int i = 0; i < RLC_EP_TABLE_COMBD; i++) {
			ep_new(t[i]);
		}
		TEST_CASE("double-table comb fixed point multiplication is correct") {
			ep_rand(p);
			ep_mul_pre_combd(t, p);
			bn_zero(k);
			ep_mul_fix_combd(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_fix_combd(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul(r, p, k);
			ep_mul_fix_combd(q, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_fix_combd(r, (const ep_t *)t, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_fix_combd(q, (const ep_t *)t, k);
			bn_add(k, k, n);
			ep_mul_fix_combd(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EP_TABLE_COMBD; i++) {
			ep_free(t[i]);
		}
#endif

#if EP_FIX == LWNAF || !defined(STRIP)
		for (int i = 0; i < RLC_EP_TABLE_LWNAF; i++) {
			ep_new(t[i]);
		}
		TEST_CASE("left-to-right w-naf fixed point multiplication is correct") {
			ep_rand(p);
			ep_mul_pre_lwnaf(t, p);
			bn_zero(k);
			ep_mul_fix_lwnaf(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_is_infty(r), end);
			bn_set_dig(k, 1);
			ep_mul_fix_lwnaf(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul(r, p, k);
			ep_mul_fix_lwnaf(q, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ep_mul_fix_lwnaf(r, (const ep_t *)t, k);
			ep_neg(r, r);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ep_mul_fix_lwnaf(q, (const ep_t *)t, k);
			bn_add(k, k, n);
			ep_mul_fix_lwnaf(r, (const ep_t *)t, k);
			TEST_ASSERT(ep_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EP_TABLE_LWNAF; i++) {
			ep_free(t[i]);
		}
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ep_free(p);
	ep_free(q);
	ep_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int simultaneous(void) {
	int code = RLC_ERR;
	bn_t n, k[2];
	ep_t p[2], r;

	bn_null(n);
	bn_null(k[0]);
	bn_null(k[1]);
	ep_null(p[0]);
	ep_null(p[1]);
	ep_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k[0]);
		bn_new(k[1]);
		ep_new(p[0]);
		ep_new(p[1]);
		ep_new(r);

		ep_curve_get_gen(p[0]);
		ep_curve_get_ord(n);

		TEST_CASE("simultaneous point multiplication is correct") {
			bn_zero(k[0]);
			bn_rand_mod(k[1], n);
			ep_mul(p[1], p[0], k[1]);
			ep_mul_sim(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_zero(k[1]);
			ep_mul(p[1], p[0], k[0]);
			ep_mul_sim(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			ep_mul_sim(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[0], k[0]);
			ep_mul_sim(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[1], k[1]);
			ep_mul_sim(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			ep_mul_sim(r, p[0], k[0], p[1], k[1]);
			ep_mul_sim_lot(p[1], p, k, 2);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			bn_add(k[0], k[0], n);
			bn_add(k[1], k[1], n);
			ep_mul_sim(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
		} TEST_END;

#if EP_SIM == BASIC || !defined(STRIP)
		TEST_CASE("basic simultaneous point multiplication is correct") {
			bn_zero(k[0]);
			bn_rand_mod(k[1], n);
			ep_mul(p[1], p[0], k[1]);
			ep_mul_sim_basic(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_zero(k[1]);
			ep_mul(p[1], p[0], k[0]);
			ep_mul_sim_basic(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			ep_mul_sim_basic(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[0], k[0]);
			ep_mul_sim_basic(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[1], k[1]);
			ep_mul_sim_basic(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			bn_add(k[0], k[0], n);
			bn_add(k[1], k[1], n);
			ep_mul_sim_basic(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_SIM == TRICK || !defined(STRIP)
		TEST_CASE("shamir's trick for simultaneous multiplication is correct") {
			bn_zero(k[0]);
			bn_rand_mod(k[1], n);
			ep_mul(p[1], p[0], k[1]);
			ep_mul_sim_trick(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_zero(k[1]);
			ep_mul(p[1], p[0], k[0]);
			ep_mul_sim_trick(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			ep_mul_sim_trick(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[0], k[0]);
			ep_mul_sim_trick(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[1], k[1]);
			ep_mul_sim_trick(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			bn_add(k[0], k[0], n);
			bn_add(k[1], k[1], n);
			ep_mul_sim_trick(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_SIM == INTER || !defined(STRIP)
		TEST_CASE("interleaving for simultaneous multiplication is correct") {
			bn_zero(k[0]);
			bn_rand_mod(k[1], n);
			ep_mul(p[1], p[0], k[1]);
			ep_mul_sim_inter(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_zero(k[1]);
			ep_mul(p[1], p[0], k[0]);
			ep_mul_sim_inter(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			ep_mul_sim_inter(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[0], k[0]);
			ep_mul_sim_inter(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[1], k[1]);
			ep_mul_sim_inter(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			bn_add(k[0], k[0], n);
			bn_add(k[1], k[1], n);
			ep_mul_sim_inter(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_SIM == JOINT || !defined(STRIP)
		TEST_CASE("jsf for simultaneous multiplication is correct") {
			bn_zero(k[0]);
			bn_rand_mod(k[1], n);
			ep_mul(p[1], p[0], k[1]);
			ep_mul_sim_joint(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_zero(k[1]);
			ep_mul(p[1], p[0], k[0]);
			ep_mul_sim_joint(r, p[0], k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			ep_mul_sim_joint(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[0], k[0]);
			ep_mul_sim_joint(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[1], k[1]);
			ep_mul_sim_joint(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			bn_add(k[0], k[0], n);
			bn_add(k[1], k[1], n);
			ep_mul_sim_joint(r, p[0], k[0], p[1], k[1]);
			ep_mul(p[0], p[0], k[0]);
			ep_mul(p[1], p[1], k[1]);
			ep_add(p[1], p[1], p[0]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("simultaneous multiplication with generator is correct") {
			bn_zero(k[0]);
			bn_rand_mod(k[1], n);
			ep_mul(p[1], p[0], k[1]);
			ep_mul_sim_gen(r, k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_zero(k[1]);
			ep_mul_gen(p[1], k[0]);
			ep_mul_sim_gen(r, k[0], p[0], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			ep_mul_sim_gen(r, k[0], p[1], k[1]);
			ep_curve_get_gen(p[0]);
			ep_mul_sim(p[1], p[0], k[0], p[1], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[0], k[0]);
			ep_mul_sim_gen(r, k[0], p[1], k[1]);
			ep_curve_get_gen(p[0]);
			ep_mul_sim(p[1], p[0], k[0], p[1], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_neg(k[1], k[1]);
			ep_mul_sim_gen(r, k[0], p[1], k[1]);
			ep_curve_get_gen(p[0]);
			ep_mul_sim(p[1], p[0], k[0], p[1], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
			bn_rand_mod(k[0], n);
			bn_rand_mod(k[1], n);
			bn_add(k[0], k[0], n);
			bn_add(k[1], k[1], n);
			ep_mul_sim_gen(r, k[0], p[1], k[1]);
			ep_curve_get_gen(p[0]);
			ep_mul_sim(p[1], p[0], k[0], p[1], k[1]);
			TEST_ASSERT(ep_cmp(p[1], r) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k[0]);
	bn_free(k[1]);
	ep_free(p[0]);
	ep_free(p[1]);
	ep_free(r);
	return code;
}

static int compression(void) {
	int code = RLC_ERR;
	ep_t a, b, c;

	ep_null(a);
	ep_null(b);
	ep_null(c);

	RLC_TRY {
		ep_new(a);
		ep_new(b);
		ep_new(c);

		TEST_CASE("point compression is correct") {
			ep_rand(a);
			ep_pck(b, a);
			TEST_ASSERT(ep_upk(c, b) == 1, end);
			TEST_ASSERT(ep_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ep_free(a);
	ep_free(b);
	ep_free(c);
	return code;
}

static int hashing(void) {
	int code = RLC_ERR;
	ep_t a;
	ep_t b;
	bn_t n;
	uint8_t msg[5];

	ep_null(a);
	ep_null(b);
	bn_null(n);

	RLC_TRY {
		ep_new(a);
		ep_new(b);
		bn_new(n);

		ep_curve_get_ord(n);

		TEST_CASE("point hashing is correct") {
			rand_bytes(msg, sizeof(msg));
			ep_map(a, msg, sizeof(msg));
			TEST_ASSERT(ep_is_infty(a) == 0, end);
			ep_map_dst(b, msg, sizeof(msg), (const uint8_t *)"RELIC", 5);
			TEST_ASSERT(ep_cmp(a, b) == RLC_EQ, end);
			ep_mul(a, a, n);
			TEST_ASSERT(ep_is_infty(a) == 1, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ep_free(a);
	ep_free(b);
	bn_free(n);
	return code;
}

int test(void) {
	ep_param_print();

	util_banner("Utilities:", 1);

	if (memory() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (util() != RLC_OK) {
		return RLC_ERR;
	}

	util_banner("Arithmetic:", 1);

	if (addition() != RLC_OK) {
		return RLC_ERR;
	}

	if (subtraction() != RLC_OK) {
		return RLC_ERR;
	}

	if (doubling() != RLC_OK) {
		return RLC_ERR;
	}

	if (endomorphism() != RLC_OK) {
		return RLC_ERR;
	}

	if (multiplication() != RLC_OK) {
		return RLC_ERR;
	}

	if (fixed() != RLC_OK) {
		return RLC_ERR;
	}

	if (simultaneous() != RLC_OK) {
		return RLC_ERR;
	}

	if (compression() != RLC_OK) {
		return RLC_ERR;
	}

	if (hashing() != RLC_OK) {
		return RLC_ERR;
	}

	return RLC_OK;
}

int main(void) {
	int r0 = RLC_ERR, r1 = RLC_ERR, r2 = RLC_ERR, r3 = RLC_ERR;

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the EP module:", 0);

#if defined(EP_PLAIN)
	r0 = ep_param_set_any_plain();
	if (r0 == RLC_OK) {
		if (test() != RLC_OK) {
			core_clean();
			return 1;
		}
	}
#endif

#if defined(EP_ENDOM)
	r1 = ep_param_set_any_endom();
	if (r1 == RLC_OK) {
		if (test() != RLC_OK) {
			core_clean();
			return 1;
		}
	}
#endif

	r2 = ep_param_set_any_pairf();
	if (r2 == RLC_OK) {
		if (test() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

#if defined(EP_SUPER)
	r3 = ep_param_set_any_super();
	if (r3 == RLC_OK) {
		if (test() != RLC_OK) {
			core_clean();
			return 1;
		}
	}
#endif

	if (r0 == RLC_ERR && r1 == RLC_ERR && r2 == RLC_ERR && r3 == RLC_ERR) {
		if (ep_param_set_any() == RLC_ERR) {
			RLC_THROW(ERR_NO_CURVE);
			core_clean();
			return 0;
		} else {
			if (test() != RLC_OK) {
				core_clean();
				return 1;
			}
		}
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
