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
 * Tests for the Elliptic Curve Cryptography module.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = RLC_ERR;
	ec_t a;

	ec_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			ec_new(a);
			ec_free(a);
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

int util(void) {
	int l, code = RLC_ERR;
	ec_t a, b, c;
	uint8_t bin[2 * RLC_FC_BYTES + 1];

	ec_null(a);
	ec_null(b);
	ec_null(c);

	RLC_TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);

		TEST_CASE("comparison is consistent") {
			ec_rand(a);
			ec_rand(b);
			TEST_ASSERT(ec_cmp(a, b) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("copy and comparison are consistent") {
			ec_rand(a);
			ec_rand(b);
			ec_rand(c);
			/* Compare points in affine coordinates. */
			if (ec_cmp(a, c) != RLC_EQ) {
				ec_copy(c, a);
				TEST_ASSERT(ec_cmp(c, a) == RLC_EQ, end);
			}
			if (ec_cmp(b, c) != RLC_EQ) {
				ec_copy(c, b);
				TEST_ASSERT(ec_cmp(b, c) == RLC_EQ, end);
			}
			/* Compare with one point in projective. */
			ec_dbl(c, a);
			ec_norm(c, c);
			ec_dbl(a, a);
			TEST_ASSERT(ec_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(ec_cmp(a, c) == RLC_EQ, end);
			/* Compare with two points in projective. */
			ec_dbl(c, c);
			ec_dbl(a, a);
			TEST_ASSERT(ec_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(ec_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("negation and comparison are consistent") {
			ec_rand(a);
			ec_neg(b, a);
			TEST_ASSERT(ec_cmp(a, b) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE
				("assignment to random/infinity and comparison are consistent")
		{
			ec_rand(a);
			ec_set_infty(c);
			TEST_ASSERT(ec_cmp(a, c) != RLC_EQ, end);
			TEST_ASSERT(ec_cmp(c, a) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to infinity and infinity test are consistent") {
			ec_set_infty(a);
			TEST_ASSERT(ec_is_infty(a), end);
		}
		TEST_END;

		TEST_CASE("validity test is correct") {
			ec_rand(a);
			TEST_ASSERT(ec_on_curve(a), end);
			dv_zero(a->x, RLC_FC_DIGS);
			TEST_ASSERT(!ec_on_curve(a), end);
		}
		TEST_END;

		TEST_CASE("blinding is consistent") {
			ec_rand(a);
			ec_blind(a, a);
			TEST_ASSERT(ec_on_curve(a), end);
		} TEST_END;

		TEST_CASE("reading and writing a point are consistent") {
			for (int j = 0; j < 2; j++) {
				ec_set_infty(a);
				l = ec_size_bin(a, j);
				ec_write_bin(bin, l, a, j);
				ec_read_bin(b, bin, l);
				TEST_ASSERT(ec_cmp(a, b) == RLC_EQ, end);
				ec_rand(a);
				l = ec_size_bin(a, j);
				ec_write_bin(bin, l, a, j);
				ec_read_bin(b, bin, l);
				TEST_ASSERT(ec_cmp(a, b) == RLC_EQ, end);
				ec_rand(a);
				ec_dbl(a, a);
				l = ec_size_bin(a, j);
				ec_norm(a, a);
				ec_write_bin(bin, l, a, j);
				ec_read_bin(b, bin, l);
				TEST_ASSERT(ec_cmp(a, b) == RLC_EQ, end);
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
	ec_free(a);
	ec_free(b);
	ec_free(c);
	return code;
}

int addition(void) {
	int code = RLC_ERR;

	ec_t a, b, c, d, e;

	ec_null(a);
	ec_null(b);
	ec_null(c);
	ec_null(d);
	ec_null(e);

	RLC_TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);
		ec_new(d);
		ec_new(e);

		TEST_CASE("point addition is commutative") {
			ec_rand(a);
			ec_rand(b);
			ec_add(d, a, b);
			ec_add(e, b, a);
			TEST_ASSERT(ec_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition is associative") {
			ec_rand(a);
			ec_rand(b);
			ec_rand(c);
			ec_add(d, a, b);
			ec_add(d, d, c);
			ec_add(e, b, c);
			ec_add(e, e, a);
			TEST_ASSERT(ec_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has identity") {
			ec_rand(a);
			ec_set_infty(d);
			ec_add(e, a, d);
			TEST_ASSERT(ec_cmp(e, a) == RLC_EQ, end);
			ec_add(e, d, a);
			TEST_ASSERT(ec_cmp(e, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has inverse") {
			ec_rand(a);
			ec_neg(d, a);
			ec_add(e, a, d);
			TEST_ASSERT(ec_is_infty(e), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	ec_free(d);
	ec_free(e);
	return code;
}

int subtraction(void) {
	int code = RLC_ERR;
	ec_t a, b, c, d;

	ec_null(a);
	ec_null(b);
	ec_null(c);
	ec_null(d);

	RLC_TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);
		ec_new(d);

		TEST_CASE("point subtraction is anti-commutative") {
			ec_rand(a);
			ec_rand(b);
			ec_sub(c, a, b);
			ec_sub(d, b, a);
			ec_neg(d, d);
			TEST_ASSERT(ec_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has identity") {
			ec_rand(a);
			ec_set_infty(c);
			ec_sub(d, a, c);
			TEST_ASSERT(ec_cmp(d, a) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has inverse") {
			ec_rand(a);
			ec_sub(c, a, a);
			TEST_ASSERT(ec_is_infty(c), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	ec_free(d);
	return code;
}

int doubling(void) {
	int code = RLC_ERR;
	ec_t a, b, c;

	ec_null(a);
	ec_null(b);
	ec_null(c);

	RLC_TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);

		TEST_CASE("point doubling is correct") {
			ec_rand(a);
			ec_add(b, a, a);
			ec_dbl(c, a);
			TEST_ASSERT(ec_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	return code;
}

static int multiplication(void) {
	int code = RLC_ERR;
	bn_t n, k;
	ec_t p, q, r;

	bn_null(n);
	bn_null(k);
	ec_null(p);
	ec_null(q);
	ec_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ec_new(p);
		ec_new(q);
		ec_new(r);

		ec_curve_get_gen(p);
		ec_curve_get_ord(n);

		TEST_ONCE("generator has the right order") {
			ec_mul(r, p, n);
			TEST_ASSERT(ec_is_infty(r) == 1, end);
		} TEST_END;

		TEST_CASE("generator multiplication is correct") {
			bn_zero(k);
			ec_mul_gen(r, k);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_set_dig(k, 1);
			ec_mul_gen(r, k);
			TEST_ASSERT(ec_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ec_mul(q, p, k);
			ec_mul_gen(r, k);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ec_mul_gen(r, k);
			ec_neg(r, r);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point multiplication is correct") {
			bn_zero(k);
			ec_mul(r, p, k);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_set_dig(k, 1);
			ec_mul(r, p, k);
			TEST_ASSERT(ec_cmp(p, r) == RLC_EQ, end);
			ec_rand(p);
			ec_mul(r, p, n);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_rand_mod(k, n);
			ec_mul(q, p, k);
			bn_neg(k, k);
			ec_mul(r, p, k);
			ec_neg(r, r);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

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
	ec_free(p);
	ec_free(q);
	ec_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int fixed(void) {
	int code = RLC_ERR;
	bn_t n, k;
	ec_t p, q, r, t[RLC_EC_TABLE];

	bn_null(n);
	bn_null(k);
	ec_null(p);
	ec_null(q);
	ec_null(r);

	for (int i = 0; i < RLC_EC_TABLE; i++) {
		ec_null(t[i]);
	}

	RLC_TRY {
		ec_new(p);
		ec_new(q);
		ec_new(r);
		bn_new(n);
		bn_new(k);

		ec_curve_get_gen(p);
		ec_curve_get_ord(n);

		for (int i = 0; i < RLC_EC_TABLE; i++) {
			ec_new(t[i]);
		}
		TEST_CASE("fixed point multiplication is correct") {
			ec_rand(p);
			ec_mul_pre(t, p);
			bn_zero(k);
			ec_mul_fix(r, (const ec_t *)t, k);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_set_dig(k, 1);
			ec_mul_fix(r, (const ec_t *)t, k);
			TEST_ASSERT(ec_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			ec_mul(q, p, k);
			ec_mul_fix(q, (const ec_t *)t, k);
			ec_mul(r, p, k);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ec_mul_fix(r, (const ec_t *)t, k);
			ec_neg(r, r);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_EC_TABLE; i++) {
			ec_free(t[i]);
		}
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ec_free(p);
	ec_free(q);
	ec_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int simultaneous(void) {
	int code = RLC_ERR;
	bn_t n, k, l;
	ec_t p, q, r;

	bn_null(n);
	bn_null(k);
	bn_null(l);
	ec_null(p);
	ec_null(q);
	ec_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		bn_new(l);
		ec_new(p);
		ec_new(q);
		ec_new(r);

		ec_curve_get_gen(p);
		ec_curve_get_gen(q);
		ec_curve_get_ord(n);

		TEST_CASE("simultaneous point multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			ec_mul(q, p, l);
			ec_mul_sim(r, p, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			ec_mul(q, p, k);
			ec_mul_sim(r, p, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			ec_mul_sim(r, p, k, q, l);
			ec_mul(p, p, k);
			ec_mul(q, q, l);
			ec_add(q, q, p);
			ec_norm(q, q);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ec_mul_sim(r, p, k, q, l);
			ec_mul(p, p, k);
			ec_mul(q, q, l);
			ec_add(q, q, p);
			ec_norm(q, q);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			ec_mul_sim(r, p, k, q, l);
			ec_mul(p, p, k);
			ec_mul(q, q, l);
			ec_add(q, q, p);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("simultaneous multiplication with generator is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			ec_mul(q, p, l);
			ec_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			ec_mul_gen(q, k);
			ec_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			ec_mul_sim_gen(r, k, q, l);
			ec_curve_get_gen(p);
			ec_mul_sim(q, p, k, q, l);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			ec_mul_sim_gen(r, k, q, l);
			ec_curve_get_gen(p);
			ec_mul_sim(q, p, k, q, l);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			ec_mul_sim_gen(r, k, q, l);
			ec_curve_get_gen(p);
			ec_mul_sim(q, p, k, q, l);
			TEST_ASSERT(ec_cmp(q, r) == RLC_EQ, end);
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
	ec_free(p);
	ec_free(q);
	ec_free(r);
	return code;
}

static int compression(void) {
	int code = RLC_ERR;
	ec_t a, b, c;

	ec_null(a);
	ec_null(b);
	ec_null(c);

	RLC_TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);

		TEST_CASE("point compression is correct") {
			ec_rand(a);
			ec_pck(b, a);
			ec_upk(c, b);
			TEST_ASSERT(ec_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	return code;
}

static int hashing(void) {
	int code = RLC_ERR;
	ec_t a;
	bn_t n;
	uint8_t msg[5];

	ec_null(a);
	bn_null(n);

	RLC_TRY {
		ec_new(a);
		bn_new(n);

		ec_curve_get_ord(n);

		TEST_CASE("point hashing is correct") {
			rand_bytes(msg, sizeof(msg));
			ec_map(a, msg, sizeof(msg));
			ec_mul(a, a, n);
			TEST_ASSERT(ec_is_infty(a) == 1, end);
		}
		TEST_END;

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	ec_free(a);
	bn_free(n);
	return code;
}

int test(void) {
	ec_param_print();

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
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the EC module:", 0);

	if (ec_param_set_any() == RLC_ERR) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	if (test() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
