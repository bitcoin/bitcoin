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
 * Tests for the Elliptic Curve Cryptography module.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = STS_ERR;
	ec_t a;

	ec_null(a);

	TRY {
		TEST_BEGIN("memory can be allocated") {
			ec_new(a);
			ec_free(a);
		} TEST_END;
	} CATCH(e) {
		switch (e) {
			case ERR_NO_MEMORY:
				util_print("FATAL ERROR!\n");
				ERROR(end);
				break;
		}
	}
	(void)a;
	code = STS_OK;
  end:
	return code;
}

int util(void) {
	int l, code = STS_ERR;
	ec_t a, b, c;
	uint8_t bin[2 * FC_BYTES + 1];

	ec_null(a);
	ec_null(b);
	ec_null(c);

	TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);

		TEST_BEGIN("comparison is consistent") {
			ec_rand(a);
			ec_rand(b);
			TEST_ASSERT(ec_cmp(a, b) != CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("copy and comparison are consistent") {
			ec_rand(a);
			ec_rand(b);
			ec_rand(c);
			/* Compare points in affine coordinates. */
			if (ec_cmp(a, c) != CMP_EQ) {
				ec_copy(c, a);
				TEST_ASSERT(ec_cmp(c, a) == CMP_EQ, end);
			}
			if (ec_cmp(b, c) != CMP_EQ) {
				ec_copy(c, b);
				TEST_ASSERT(ec_cmp(b, c) == CMP_EQ, end);
			}
			/* Compare with one point in projective. */
			ec_dbl(c, a);
			ec_norm(c, c);
			ec_dbl(a, a);
			TEST_ASSERT(ec_cmp(c, a) == CMP_EQ, end);
			TEST_ASSERT(ec_cmp(a, c) == CMP_EQ, end);
			/* Compare with two points in projective. */
			ec_dbl(c, c);
			ec_dbl(a, a);
			TEST_ASSERT(ec_cmp(c, a) == CMP_EQ, end);
			TEST_ASSERT(ec_cmp(a, c) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("negation and comparison are consistent") {
			ec_rand(a);
			ec_neg(b, a);
			TEST_ASSERT(ec_cmp(a, b) != CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN
				("assignment to random/infinity and comparison are consistent")
		{
			ec_rand(a);
			ec_set_infty(c);
			TEST_ASSERT(ec_cmp(a, c) != CMP_EQ, end);
			TEST_ASSERT(ec_cmp(c, a) != CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("assignment to infinity and infinity test are consistent") {
			ec_set_infty(a);
			TEST_ASSERT(ec_is_infty(a), end);
		}
		TEST_END;

		TEST_BEGIN("validity test is correct") {
			ec_rand(a);
			TEST_ASSERT(ec_is_valid(a), end);
			dv_zero(a->x, FC_DIGS);
			TEST_ASSERT(!ec_is_valid(a), end);
		}
		TEST_END;

		TEST_BEGIN("reading and writing a point are consistent") {
			for (int j = 0; j < 2; j++) {
				ec_set_infty(a);
				l = ec_size_bin(a, j);
				ec_write_bin(bin, l, a, j);
				ec_read_bin(b, bin, l);
				TEST_ASSERT(ec_cmp(a, b) == CMP_EQ, end);
				ec_rand(a);
				l = ec_size_bin(a, j);
				ec_write_bin(bin, l, a, j);
				ec_read_bin(b, bin, l);
				TEST_ASSERT(ec_cmp(a, b) == CMP_EQ, end);
				ec_rand(a);
				ec_dbl(a, a);
				l = ec_size_bin(a, j);
				ec_norm(a, a);
				ec_write_bin(bin, l, a, j);
				ec_read_bin(b, bin, l);
				TEST_ASSERT(ec_cmp(a, b) == CMP_EQ, end);
			}
		}
		TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	return code;
}

int addition(void) {
	int code = STS_ERR;

	ec_t a, b, c, d, e;

	ec_null(a);
	ec_null(b);
	ec_null(c);
	ec_null(d);
	ec_null(e);

	TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);
		ec_new(d);
		ec_new(e);

		TEST_BEGIN("point addition is commutative") {
			ec_rand(a);
			ec_rand(b);
			ec_add(d, a, b);
			ec_add(e, b, a);
			TEST_ASSERT(ec_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("point addition is associative") {
			ec_rand(a);
			ec_rand(b);
			ec_rand(c);
			ec_add(d, a, b);
			ec_add(d, d, c);
			ec_add(e, b, c);
			ec_add(e, e, a);
			TEST_ASSERT(ec_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("point addition has identity") {
			ec_rand(a);
			ec_set_infty(d);
			ec_add(e, a, d);
			TEST_ASSERT(ec_cmp(e, a) == CMP_EQ, end);
			ec_add(e, d, a);
			TEST_ASSERT(ec_cmp(e, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("point addition has inverse") {
			ec_rand(a);
			ec_neg(d, a);
			ec_add(e, a, d);
			TEST_ASSERT(ec_is_infty(e), end);
		} TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	ec_free(d);
	ec_free(e);
	return code;
}

int subtraction(void) {
	int code = STS_ERR;
	ec_t a, b, c, d;

	ec_null(a);
	ec_null(b);
	ec_null(c);
	ec_null(d);

	TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);
		ec_new(d);

		TEST_BEGIN("point subtraction is anti-commutative") {
			ec_rand(a);
			ec_rand(b);
			ec_sub(c, a, b);
			ec_sub(d, b, a);
			ec_neg(d, d);
			TEST_ASSERT(ec_cmp(c, d) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("point subtraction has identity") {
			ec_rand(a);
			ec_set_infty(c);
			ec_sub(d, a, c);
			TEST_ASSERT(ec_cmp(d, a) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("point subtraction has inverse") {
			ec_rand(a);
			ec_sub(c, a, a);
			TEST_ASSERT(ec_is_infty(c), end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	ec_free(d);
	return code;
}

int doubling(void) {
	int code = STS_ERR;
	ec_t a, b, c;

	ec_null(a);
	ec_null(b);
	ec_null(c);

	TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);

		TEST_BEGIN("point doubling is correct") {
			ec_rand(a);
			ec_add(b, a, a);
			ec_dbl(c, a);
			TEST_ASSERT(ec_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	return code;
}

static int multiplication(void) {
	int code = STS_ERR;
	bn_t n, k;
	ec_t p, q, r;

	bn_null(n);
	bn_null(k);
	ec_null(p);
	ec_null(q);
	ec_null(r);

	TRY {
		bn_new(n);
		bn_new(k);
		ec_new(p);
		ec_new(q);
		ec_new(r);

		ec_curve_get_gen(p);
		ec_curve_get_ord(n);

		TEST_BEGIN("generator has the right order") {
			ec_mul(r, p, n);
			TEST_ASSERT(ec_is_infty(r) == 1, end);
		} TEST_END;

		TEST_BEGIN("generator multiplication is correct") {
			bn_zero(k);
			ec_mul_gen(r, k);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_set_dig(k, 1);
			ec_mul_gen(r, k);
			TEST_ASSERT(ec_cmp(p, r) == CMP_EQ, end);
			bn_rand_mod(k, n);
			ec_mul(q, p, k);
			ec_mul_gen(r, k);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_neg(k, k);
			ec_mul_gen(r, k);
			ec_neg(r, r);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("point multiplication is correct") {
			bn_zero(k);
			ec_mul(r, p, k);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_set_dig(k, 1);
			ec_mul(r, p, k);
			TEST_ASSERT(ec_cmp(p, r) == CMP_EQ, end);
			ec_rand(p);
			ec_mul(r, p, n);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_rand_mod(k, n);
			ec_mul(q, p, k);
			bn_neg(k, k);
			ec_mul(r, p, k);
			ec_neg(r, r);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(p);
	ec_free(q);
	ec_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int fixed(void) {
	int code = STS_ERR;
	bn_t n, k;
	ec_t p, q, r, t[RELIC_EC_TABLE];

	bn_null(n);
	bn_null(k);
	ec_null(p);
	ec_null(q);
	ec_null(r);

	for (int i = 0; i < RELIC_EC_TABLE; i++) {
		ec_null(t[i]);
	}

	TRY {
		ec_new(p);
		ec_new(q);
		ec_new(r);
		bn_new(n);
		bn_new(k);

		ec_curve_get_gen(p);
		ec_curve_get_ord(n);

		for (int i = 0; i < RELIC_EC_TABLE; i++) {
			ec_new(t[i]);
		}
		TEST_BEGIN("fixed point multiplication is correct") {
			ec_rand(p);
			ec_mul_pre(t, p);
			bn_zero(k);
			ec_mul_fix(r, (const ec_t *)t, k);
			TEST_ASSERT(ec_is_infty(r), end);
			bn_set_dig(k, 1);
			ec_mul_fix(r, (const ec_t *)t, k);
			TEST_ASSERT(ec_cmp(p, r) == CMP_EQ, end);
			bn_rand_mod(k, n);
			ec_mul(q, p, k);
			ec_mul_fix(q, (const ec_t *)t, k);
			ec_mul(r, p, k);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_neg(k, k);
			ec_mul_fix(r, (const ec_t *)t, k);
			ec_neg(r, r);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
		} TEST_END;
		for (int i = 0; i < RELIC_EC_TABLE; i++) {
			ec_free(t[i]);
		}
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(p);
	ec_free(q);
	ec_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int simultaneous(void) {
	int code = STS_ERR;
	bn_t n, k, l;
	ec_t p, q, r;

	bn_null(n);
	bn_null(k);
	bn_null(l);
	ec_null(p);
	ec_null(q);
	ec_null(r);

	TRY {
		bn_new(n);
		bn_new(k);
		bn_new(l);
		ec_new(p);
		ec_new(q);
		ec_new(r);

		ec_curve_get_gen(p);
		ec_curve_get_gen(q);
		ec_curve_get_ord(n);

		TEST_BEGIN("simultaneous point multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			ec_mul(q, p, l);
			ec_mul_sim(r, p, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			ec_mul(q, p, k);
			ec_mul_sim(r, p, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			ec_mul_sim(r, p, k, q, l);
			ec_mul(p, p, k);
			ec_mul(q, q, l);
			ec_add(q, q, p);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_neg(k, k);
			ec_mul_sim(r, p, k, q, l);
			ec_mul(p, p, k);
			ec_mul(q, q, l);
			ec_add(q, q, p);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_neg(l, l);
			ec_mul_sim(r, p, k, q, l);
			ec_mul(p, p, k);
			ec_mul(q, q, l);
			ec_add(q, q, p);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("simultaneous multiplication with generator is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			ec_mul(q, p, l);
			ec_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			ec_mul_gen(q, k);
			ec_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			ec_mul_sim_gen(r, k, q, l);
			ec_curve_get_gen(p);
			ec_mul_sim(q, p, k, q, l);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_neg(k, k);
			ec_mul_sim_gen(r, k, q, l);
			ec_curve_get_gen(p);
			ec_mul_sim(q, p, k, q, l);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
			bn_neg(l, l);
			ec_mul_sim_gen(r, k, q, l);
			ec_curve_get_gen(p);
			ec_mul_sim(q, p, k, q, l);
			TEST_ASSERT(ec_cmp(q, r) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
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
	int code = STS_ERR;
	ec_t a, b, c;

	ec_null(a);
	ec_null(b);
	ec_null(c);

	TRY {
		ec_new(a);
		ec_new(b);
		ec_new(c);

		TEST_BEGIN("point compression is correct") {
			ec_rand(a);
			ec_pck(b, a);
			ec_upk(c, b);
			TEST_ASSERT(ec_cmp(a, c) == CMP_EQ, end);
		}
		TEST_END;

	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(a);
	ec_free(b);
	ec_free(c);
	return code;
}

static int hashing(void) {
	int code = STS_ERR;
	ec_t a;
	bn_t n;
	uint8_t msg[5];

	ec_null(a);
	bn_null(n);

	TRY {
		ec_new(a);
		bn_new(n);

		ec_curve_get_ord(n);

		TEST_BEGIN("point hashing is correct") {
			rand_bytes(msg, sizeof(msg));
			ec_map(a, msg, sizeof(msg));
			ec_mul(a, a, n);
			TEST_ASSERT(ec_is_infty(a) == 1, end);
		}
		TEST_END;

	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	ec_free(a);
	bn_free(n);
	return code;
}

int test(void) {
	ec_param_print();

	util_banner("Utilities:", 1);

	if (memory() != STS_OK) {
		core_clean();
		return 1;
	}

	if (util() != STS_OK) {
		return STS_ERR;
	}

	util_banner("Arithmetic:", 1);

	if (addition() != STS_OK) {
		return STS_ERR;
	}

	if (subtraction() != STS_OK) {
		return STS_ERR;
	}

	if (doubling() != STS_OK) {
		return STS_ERR;
	}

	if (multiplication() != STS_OK) {
		return STS_ERR;
	}

	if (fixed() != STS_OK) {
		return STS_ERR;
	}

	if (simultaneous() != STS_OK) {
		return STS_ERR;
	}

	if (compression() != STS_OK) {
		return STS_ERR;
	}

	if (hashing() != STS_OK) {
		return STS_ERR;
	}

	return STS_OK;
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the EC module:", 0);

	if (ec_param_set_any() == STS_ERR) {
		THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	if (test() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
