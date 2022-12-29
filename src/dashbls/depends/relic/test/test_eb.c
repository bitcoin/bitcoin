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
 * Tests for arithmetic on binary elliptic curves.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = RLC_ERR;
	eb_t a;

	eb_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			eb_new(a);
			eb_free(a);
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
	eb_t a, b, c;
	uint8_t bin[2 * RLC_FB_BYTES + 1];

	eb_null(a);
	eb_null(b);
	eb_null(c);

	RLC_TRY {
		eb_new(a);
		eb_new(b);
		eb_new(c);

		TEST_CASE("copy and comparison are consistent") {
			eb_rand(a);
			eb_rand(b);
			eb_rand(c);
			/* Compare points in affine coordinates. */
			if (eb_cmp(a, c) != RLC_EQ) {
				eb_copy(c, a);
				TEST_ASSERT(eb_cmp(c, a) == RLC_EQ, end);
			}
			if (eb_cmp(b, c) != RLC_EQ) {
				eb_copy(c, b);
				TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
			}
			/* Compare with one point in projective. */
			eb_dbl(c, a);
			eb_norm(c, c);
			eb_dbl(a, a);
			TEST_ASSERT(eb_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(eb_cmp(a, c) == RLC_EQ, end);
			/* Compare with two points in projective. */
			eb_dbl(c, c);
			eb_dbl(a, a);
			TEST_ASSERT(eb_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(eb_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("negation and comparison are consistent") {
			eb_rand(a);
			eb_neg(b, a);
			TEST_ASSERT(eb_cmp(a, b) != RLC_EQ, end);
			eb_neg(b, b);
			TEST_ASSERT(eb_cmp(a, b) == RLC_EQ, end);
			eb_neg(b, a);
			eb_add(a, a, b);
			eb_set_infty(b);
			TEST_ASSERT(eb_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to random and comparison are consistent") {
			eb_rand(a);
			eb_set_infty(c);
			TEST_ASSERT(eb_cmp(a, c) != RLC_EQ, end);
			TEST_ASSERT(eb_cmp(c, a) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to infinity and infinity test are consistent") {
			eb_set_infty(a);
			TEST_ASSERT(eb_is_infty(a), end);
		}
		TEST_END;

		TEST_CASE("validity test is correct") {
			eb_set_infty(a);
			TEST_ASSERT(eb_on_curve(a), end);
			eb_rand(a);
			TEST_ASSERT(eb_on_curve(a), end);
			fb_rand(a->x);
			TEST_ASSERT(!eb_on_curve(a), end);
		}
		TEST_END;

		TEST_CASE("blinding is consistent") {
			eb_rand(a);
			eb_blind(a, a);
			TEST_ASSERT(eb_on_curve(a), end);
		} TEST_END;

		TEST_CASE("reading and writing a point are consistent") {
			for (int j = 0; j < 2; j++) {
				eb_set_infty(a);
				l = eb_size_bin(a, j);
				eb_write_bin(bin, l, a, j);
				eb_read_bin(b, bin, l);
				TEST_ASSERT(eb_cmp(a, b) == RLC_EQ, end);
				eb_rand(a);
				l = eb_size_bin(a, j);
				eb_write_bin(bin, l, a, j);
				eb_read_bin(b, bin, l);
				TEST_ASSERT(eb_cmp(a, b) == RLC_EQ, end);
				eb_rand(a);
				eb_dbl(a, a);
				l = eb_size_bin(a, j);
				eb_norm(a, a);
				eb_write_bin(bin, l, a, j);
				eb_read_bin(b, bin, l);
				TEST_ASSERT(eb_cmp(a, b) == RLC_EQ, end);
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
	eb_free(a);
	eb_free(b);
	eb_free(c);
	return code;
}

static int addition(void) {
	int code = RLC_ERR;
	eb_t a, b, c, d, e;

	eb_null(a);
	eb_null(b);
	eb_null(c);
	eb_null(d);
	eb_null(e);

	RLC_TRY {
		eb_new(a);
		eb_new(b);
		eb_new(c);
		eb_new(d);
		eb_new(e);

		TEST_CASE("point addition is commutative") {
			eb_rand(a);
			eb_rand(b);
			eb_add(d, a, b);
			eb_add(e, b, a);
			TEST_ASSERT(eb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition is associative") {
			eb_rand(a);
			eb_rand(b);
			eb_rand(c);
			eb_add(d, a, b);
			eb_add(d, d, c);
			eb_add(e, b, c);
			eb_add(e, e, a);
			TEST_ASSERT(eb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has identity") {
			eb_rand(a);
			eb_set_infty(d);
			eb_add(e, a, d);
			TEST_ASSERT(eb_cmp(e, a) == RLC_EQ, end);
			eb_add(e, d, a);
			TEST_ASSERT(eb_cmp(e, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has inverse") {
			eb_rand(a);
			eb_neg(d, a);
			eb_add(e, a, d);
			TEST_ASSERT(eb_is_infty(e), end);
		} TEST_END;

#if EB_ADD == BASIC || !defined(STRIP)
		TEST_CASE("point addition in affine coordinates is correct") {
			eb_rand(a);
			eb_rand(b);
			eb_add(d, a, b);
			eb_add_basic(e, a, b);
			TEST_ASSERT(eb_cmp(e, d) == RLC_EQ, end);
		} TEST_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
#if !defined(EB_MIXED) || !defined(STRIP)
		TEST_CASE("point addition in projective coordinates is correct") {
			eb_rand(a);
			eb_rand(b);
			eb_rand(c);
			eb_add_projc(a, a, b);
			eb_add_projc(b, b, c);
			/* a and b in projective coordinates. */
			eb_add_projc(d, a, b);
			/* normalize before mixing coordinates. */
			eb_norm(a, a);
			eb_norm(b, b);
			eb_add(e, a, b);
			TEST_ASSERT(eb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("point addition in mixed coordinates (z2 = 1) is correct") {
			eb_rand(a);
			eb_rand(b);
			/* a in projective, b in affine coordinates. */
			eb_add_projc(a, a, b);
			eb_add_projc(d, a, b);
			/* a in affine coordinates. */
			eb_norm(a, a);
			eb_add(e, a, b);
			TEST_ASSERT(eb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition in mixed coordinates (z1,z2 = 1) is correct") {
			eb_rand(a);
			eb_rand(b);
			/* a and b in affine coordinates. */
			eb_add(d, a, b);
			eb_add_projc(e, a, b);
			TEST_ASSERT(eb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	eb_free(a);
	eb_free(b);
	eb_free(c);
	eb_free(d);
	eb_free(e);
	return code;
}

static int subtraction(void) {
	int code = RLC_ERR;
	eb_t a, b, c, d;

	eb_null(a);
	eb_null(b);
	eb_null(c);
	eb_null(d);

	RLC_TRY {
		eb_new(a);
		eb_new(b);
		eb_new(c);
		eb_new(d);

		TEST_CASE("point subtraction is anti-commutative") {
			eb_rand(a);
			eb_rand(b);
			eb_sub(c, a, b);
			eb_sub(d, b, a);
			eb_neg(d, d);
			TEST_ASSERT(eb_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has identity") {
			eb_rand(a);
			eb_set_infty(c);
			eb_sub(d, a, c);
			TEST_ASSERT(eb_cmp(d, a) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has inverse") {
			eb_rand(a);
			eb_sub(c, a, a);
			TEST_ASSERT(eb_is_infty(c), end);
		}
		TEST_END;

#if EB_ADD == BASIC || !defined(STRIP)
		TEST_CASE("point subtraction in affine coordinates is correct") {
			eb_rand(a);
			eb_rand(b);
			eb_sub(c, a, b);
			eb_norm(c, c);
			eb_sub_basic(d, a, b);
			TEST_ASSERT(eb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
#if !defined(EB_MIXED) || !defined(STRIP)
		TEST_CASE("point subtraction in projective coordinates is correct") {
			eb_rand(a);
			eb_rand(b);
			eb_add_projc(a, a, b);
			eb_rand(b);
			eb_rand(c);
			eb_add_projc(b, b, c);
			/* a and b in projective coordinates. */
			eb_sub_projc(c, a, b);
			eb_norm(c, c);
			eb_norm(a, a);
			eb_norm(b, b);
			eb_sub(d, a, b);
			eb_norm(d, d);
			TEST_ASSERT(eb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("point subtraction in mixed coordinates (z2 = 1) is correct") {
			eb_rand(a);
			eb_rand(b);
			eb_add_projc(a, a, b);
			eb_rand(b);
			/* a and b in projective coordinates. */
			eb_sub_projc(c, a, b);
			eb_norm(c, c);
			/* a in affine coordinates. */
			eb_norm(a, a);
			eb_sub(d, a, b);
			eb_norm(d, d);
			TEST_ASSERT(eb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE
				("point subtraction in mixed coordinates (z1,z2 = 1) is correct")
		{
			eb_rand(a);
			eb_rand(b);
			eb_norm(a, a);
			eb_norm(b, b);
			/* a and b in affine coordinates. */
			eb_sub(c, a, b);
			eb_norm(c, c);
			eb_sub_projc(d, a, b);
			eb_norm(d, d);
			TEST_ASSERT(eb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	eb_free(a);
	eb_free(b);
	eb_free(c);
	eb_free(d);
	return code;
}

static int doubling(void) {
	int code = RLC_ERR;
	eb_t a, b, c;

	eb_null(a);
	eb_null(b);
	eb_null(c);

	RLC_TRY {
		eb_new(a);
		eb_new(b);
		eb_new(c);

		TEST_CASE("point doubling is correct") {
			eb_rand(a);
			eb_add(b, a, a);
			eb_norm(b, b);
			eb_dbl(c, a);
			TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

#if EB_ADD == BASIC || !defined(STRIP)
		TEST_CASE("point doubling in affine coordinates is correct") {
			eb_rand(a);
			eb_dbl(b, a);
			eb_dbl_basic(c, a);
			TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
		TEST_CASE("point doubling in projective coordinates is correct") {
			eb_rand(a);
			/* a in projective coordinates. */
			eb_dbl_projc(a, a);
			eb_dbl_projc(b, a);
			eb_norm(a, a);
			eb_dbl(c, a);
			TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point doubling in mixed coordinates (z1 = 1) is correct") {
			eb_rand(a);
			eb_dbl_projc(b, a);
			eb_norm(b, b);
			eb_dbl(c, a);
			TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	eb_free(a);
	eb_free(b);
	eb_free(c);
	return code;
}

static int halving(void) {
	int code = RLC_ERR;
	eb_t a, b, c;

	eb_null(a);
	eb_null(b);
	eb_null(c);

	RLC_TRY {
		eb_new(a);
		eb_new(b);
		eb_new(c);

		TEST_CASE("point halving is correct") {
			eb_rand(a);
			eb_hlv(b, a);
			eb_norm(b, b);
			eb_dbl(c, b);
			TEST_ASSERT(eb_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	eb_free(a);
	eb_free(b);
	eb_free(c);
	return code;
}

static int frobenius(void) {
	int code = RLC_ERR;
	eb_t a, b, c;

	eb_null(a);
	eb_null(b);
	eb_null(c);

	RLC_TRY {
		eb_new(a);
		eb_new(b);
		eb_new(c);

#if defined(EB_KBLTZ)
		if (eb_curve_is_kbltz()) {
			TEST_CASE("frobenius map is correct") {
				/* Test if (t^2 + 2)P = utP. */
				eb_rand(a);
				eb_frb(b, a);
				eb_frb(b, b);
				eb_dbl(c, a);
				eb_add(b, c, b);
				eb_frb(c, a);
				if (eb_curve_opt_a() == RLC_ZERO) {
					eb_neg(c, c);
				}
				TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
			}
			TEST_END;

#if EB_ADD == BASIC || !defined(STRIP)
			TEST_CASE("frobenius in affine coordinates is correct") {
				/* Test if (t^2 + 2)P = utP. */
				eb_rand(a);
				eb_frb(b, a);
				eb_frb(b, b);
				eb_dbl(c, a);
				eb_add(b, c, b);
				/* Convert to affine. */
				eb_norm(b, b);
				eb_frb(c, a);
				if (eb_curve_opt_a() == RLC_ZERO) {
					eb_neg(c, c);
				}
				TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
			}
			TEST_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
			TEST_CASE("frobenius in projective coordinates is correct") {
				eb_rand(a);
				eb_dbl_projc(a, a);
				/* a in projective coordinates. */
				eb_frb(b, a);
				eb_norm(a, a);
				eb_frb(c, a);
				TEST_ASSERT(eb_cmp(b, c) == RLC_EQ, end);
			}
			TEST_END;
#endif
		}
#endif
	(void)a;
	(void)b;
	(void)c;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	eb_free(a);
	eb_free(b);
	eb_free(c);
	return code;
}

static int multiplication(void) {
	int code = RLC_ERR;
	bn_t n, k;
	eb_t p, q, r;

	bn_null(n);
	bn_null(k);
	eb_null(p);
	eb_null(q);
	eb_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		eb_new(p);
		eb_new(q);
		eb_new(r);

		eb_curve_get_gen(p);
		eb_curve_get_ord(n);

		TEST_ONCE("generator has the right order") {
			TEST_ASSERT(eb_on_curve(p), end);
			eb_mul(r, p, n);
			TEST_ASSERT(eb_is_infty(r) == 1, end);
		} TEST_END;

		TEST_CASE("generator multiplication is correct") {
			bn_zero(k);
			eb_mul_gen(r, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_gen(r, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			eb_mul(q, p, k);
			eb_mul_gen(r, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_gen(r, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

#if EB_MUL == BASIC || !defined(STRIP)
		TEST_CASE("binary point multiplication is correct") {
			bn_zero(k);
			eb_mul_basic(r, p, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_basic(r, p, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			eb_rand(p);
			eb_mul_basic(r, p, n);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_rand_mod(k, n);
			eb_mul(q, p, k);
			eb_mul_basic(r, p, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_basic(r, p, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EB_MUL == LODAH || !defined(STRIP)
		TEST_CASE("lopez-dahab point multiplication is correct") {
			bn_zero(k);
			eb_mul_lodah(r, p, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_lodah(r, p, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			eb_rand(p);
			eb_mul_lodah(r, p, n);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_rand_mod(k, n);
			eb_mul(q, p, k);
			eb_mul_lodah(r, p, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_lodah(r, p, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if EB_MUL == LWNAF || !defined(STRIP)
		TEST_CASE("left-to-right w-(t)naf point multiplication is correct") {
			bn_zero(k);
			eb_mul_lwnaf(r, p, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_lwnaf(r, p, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			eb_rand(p);
			eb_mul_lwnaf(r, p, n);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_rand_mod(k, n);
			eb_mul(q, p, k);
			eb_mul_lwnaf(r, p, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_lwnaf(r, p, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if EB_MUL == RWNAF || !defined(STRIP)
		TEST_CASE("right-to-left w-(t)naf point multiplication is correct") {
			bn_zero(k);
			eb_mul_rwnaf(r, p, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_rwnaf(r, p, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			eb_rand(p);
			eb_mul_rwnaf(r, p, n);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_rand_mod(k, n);
			eb_mul(q, p, k);
			eb_mul_rwnaf(r, p, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_rwnaf(r, p, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if EB_MUL == HALVE || !defined(STRIP)
		TEST_CASE("point multiplication by halving is correct") {
			bn_zero(k);
			eb_mul_halve(r, p, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_halve(r, p, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			eb_rand(p);
			eb_mul_halve(r, p, n);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_rand_mod(k, n);
			eb_mul(q, p, k);
			eb_mul_halve(r, p, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_halve(r, p, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
#endif

		TEST_CASE("point multiplication by digit is correct") {
			eb_mul_dig(r, p, 0);
			TEST_ASSERT(eb_is_infty(r), end);
			eb_mul_dig(r, p, 1);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			bn_rand(k, RLC_POS, RLC_DIG);
			eb_mul(q, p, k);
			eb_mul_dig(r, p, k->dp[0]);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
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
	eb_free(p);
	eb_free(q);
	eb_free(r);
	return code;
}

static int fixed(void) {
	int code = RLC_ERR;
	bn_t n, k;
	eb_t p, q, r, t[RLC_EB_TABLE_MAX];

	bn_null(n);
	bn_null(k);
	eb_null(p);
	eb_null(q);
	eb_null(r);

	for (int i = 0; i < RLC_EB_TABLE_MAX; i++) {
		eb_null(t[i]);
	}

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		eb_new(p);
		eb_new(q);
		eb_new(r);

		eb_curve_get_ord(n);

		for (int i = 0; i < RLC_EB_TABLE; i++) {
			eb_new(t[i]);
		}
		TEST_CASE("fixed point multiplication is correct") {
			eb_rand(p);
			eb_mul_pre(t, p);
			bn_zero(k);
			eb_mul_fix(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_fix(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			eb_mul(q, p, k);
			eb_mul_fix(q, (const eb_t *)t, k);
			eb_mul(r, p, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_fix(r, (const eb_t *)t, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EB_TABLE; i++) {
			eb_free(t[i]);
		}

#if EB_FIX == BASIC || !defined(STRIP)
		for (int i = 0; i < RLC_EB_TABLE_BASIC; i++) {
			eb_new(t[i]);
		}
		TEST_CASE("binary fixed point multiplication is correct") {
			eb_rand(p);
			eb_mul_pre_basic(t, p);
			bn_zero(k);
			eb_mul_fix_basic(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_fix_basic(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			eb_mul(r, p, k);
			eb_mul_fix_basic(q, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_fix_basic(r, (const eb_t *)t, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EB_TABLE_BASIC; i++) {
			eb_free(t[i]);
		}
#endif

#if EB_FIX == COMBS || !defined(STRIP)
		for (int i = 0; i < RLC_EB_TABLE_COMBS; i++) {
			eb_new(t[i]);
		}
		TEST_CASE("single-table comb fixed point multiplication is correct") {
			eb_rand(p);
			eb_mul_pre_combs(t, p);
			bn_zero(k);
			eb_mul_fix_combs(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_fix_combs(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			eb_mul(r, p, k);
			eb_mul_fix_combs(q, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_fix_combs(r, (const eb_t *)t, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EB_TABLE_COMBS; i++) {
			eb_free(t[i]);
		}
#endif

#if EB_FIX == COMBD || !defined(STRIP)
		for (int i = 0; i < RLC_EB_TABLE_COMBD; i++) {
			eb_new(t[i]);
		}
		TEST_CASE("double-table comb fixed point multiplication is correct") {
			eb_rand(p);
			eb_mul_pre_combd(t, p);
			bn_zero(k);
			eb_mul_fix_combd(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_fix_combd(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			eb_mul(r, p, k);
			eb_mul_fix_combd(q, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_fix_combd(r, (const eb_t *)t, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EB_TABLE_COMBD; i++) {
			eb_free(t[i]);
		}
#endif

#if EB_FIX == LWNAF || !defined(STRIP)
		for (int i = 0; i < RLC_EB_TABLE_LWNAF; i++) {
			eb_new(t[i]);
		}
		TEST_CASE("left-to-right w(t)naf fixed point multiplication is correct") {
			eb_rand(p);
			eb_mul_pre_lwnaf(t, p);
			bn_zero(k);
			eb_mul_fix_lwnaf(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_is_infty(r), end);
			bn_set_dig(k, 1);
			eb_mul_fix_lwnaf(r, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			eb_mul(r, p, k);
			eb_mul_fix_lwnaf(q, (const eb_t *)t, k);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_fix_lwnaf(r, (const eb_t *)t, k);
			eb_neg(r, r);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EB_TABLE_LWNAF; i++) {
			eb_free(t[i]);
		}
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	eb_free(p);
	eb_free(q);
	eb_free(r);
	return code;
}

static int simultaneous(void) {
	int code = RLC_ERR;
	bn_t n, k, l;
	eb_t p, q, r;

	bn_null(n);
	bn_null(k);
	bn_null(l);
	eb_null(p);
	eb_null(q);
	eb_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		bn_new(l);
		eb_new(p);
		eb_new(q);
		eb_new(r);

		eb_curve_get_gen(p);
		eb_curve_get_gen(q);
		eb_curve_get_ord(n);

		TEST_CASE("simultaneous point multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			eb_mul(q, p, l);
			eb_mul_sim(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			eb_mul(q, p, k);
			eb_mul_sim(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			eb_mul_sim(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_sim(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			eb_mul_sim(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

#if EB_SIM == BASIC || !defined(STRIP)
		TEST_CASE("basic simultaneous point multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			eb_mul(q, p, l);
			eb_mul_sim_basic(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			eb_mul(q, p, k);
			eb_mul_sim_basic(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			eb_mul_sim_basic(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_sim_basic(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			eb_mul_sim_basic(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EB_SIM == TRICK || !defined(STRIP)
		TEST_CASE("shamir's trick for simultaneous multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			eb_mul(q, p, l);
			eb_mul_sim_trick(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			eb_mul(q, p, k);
			eb_mul_sim_trick(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			eb_mul_sim_trick(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_sim_trick(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			eb_mul_sim_trick(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EB_SIM == INTER || !defined(STRIP)
		TEST_CASE("interleaving for simultaneous multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			eb_mul(q, p, l);
			eb_mul_sim_inter(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			eb_mul(q, p, k);
			eb_mul_sim_inter(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			eb_mul_sim_inter(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_sim_inter(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			eb_mul_sim_inter(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
#endif

#if EB_SIM == JOINT || !defined(STRIP)
		TEST_CASE("jsf for simultaneous multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			eb_mul(q, p, l);
			eb_mul_sim_joint(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			eb_mul(q, p, k);
			eb_mul_sim_joint(r, p, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			eb_mul_sim_joint(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_sim_joint(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			eb_mul_sim_joint(r, p, k, q, l);
			eb_mul(p, p, k);
			eb_mul(q, q, l);
			eb_add(q, q, p);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("simultaneous multiplication with generator is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			eb_mul(q, p, l);
			eb_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			eb_mul_gen(q, k);
			eb_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			eb_mul_sim_gen(r, k, q, l);
			eb_curve_get_gen(p);
			eb_mul_sim(q, p, k, q, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			eb_mul_sim_gen(r, k, q, l);
			eb_curve_get_gen(p);
			eb_mul_sim(q, p, k, q, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			eb_mul_sim_gen(r, k, q, l);
			eb_curve_get_gen(p);
			eb_mul_sim(q, p, k, q, l);
			TEST_ASSERT(eb_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	bn_free(l);
	eb_free(p);
	eb_free(q);
	eb_free(r);
	return code;
}

static int compression(void) {
	int code = RLC_ERR;
	eb_t a, b, c;

	eb_null(a);
	eb_null(b);
	eb_null(c);

	RLC_TRY {
		eb_new(a);
		eb_new(b);
		eb_new(c);

		TEST_CASE("point compression is correct") {
			eb_rand(a);
			eb_pck(b, a);
			TEST_ASSERT(eb_upk(c, b) == 1, end);
			TEST_ASSERT(eb_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	eb_free(a);
	eb_free(b);
	eb_free(c);
	return code;
}

static int hashing(void) {
	int code = RLC_ERR;
	eb_t a;
	bn_t n;
	uint8_t msg[5];

	eb_null(a);
	bn_null(n);

	RLC_TRY {
		eb_new(a);
		bn_new(n);

		eb_curve_get_ord(n);

		TEST_CASE("point hashing is correct") {
			rand_bytes(msg, sizeof(msg));
			eb_map(a, msg, sizeof(msg));
			eb_mul(a, a, n);
			TEST_ASSERT(eb_is_infty(a) == 1, end);
		}
		TEST_END;

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	eb_free(a);
	bn_free(n);
	return code;
}

static int test(void) {
	eb_param_print();

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

	if (halving() != RLC_OK) {
		return RLC_ERR;
	}

	if (frobenius() != RLC_OK) {
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
	int r0 = RLC_ERR, r1 = RLC_ERR;

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the EB module:", 0);

	r0 = eb_param_set_any_plain();
	if (r0 == RLC_OK) {
		if (test() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

#if defined(EB_KBLTZ)
	r1 = eb_param_set_any_kbltz();
	if (r1 == RLC_OK) {
		if (test() != RLC_OK) {
			core_clean();
			return 1;
		}
	}
#endif

	if (r0 == RLC_ERR && r1 == RLC_ERR) {
		if (eb_param_set_any() == RLC_ERR) {
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
