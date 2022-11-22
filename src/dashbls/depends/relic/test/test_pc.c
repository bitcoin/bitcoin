/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2010 RELIC Authors
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
 * Tests for the Pairing-Based Cryptography module.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory1(void) {
	err_t e;
	int code = RLC_ERR;
	g1_t a;

	g1_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			g1_new(a);
			g1_free(a);
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

int util1(void) {
	int l, code = RLC_ERR;
	g1_t a, b, c;
	uint8_t bin[2 * RLC_PC_BYTES + 1];

	g1_null(a);
	g1_null(b);
	g1_null(c);

	RLC_TRY {
		g1_new(a);
		g1_new(b);
		g1_new(c);

		TEST_CASE("comparison is consistent") {
			g1_rand(a);
			g1_rand(b);
			TEST_ASSERT(g1_cmp(a, b) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("copy and comparison are consistent") {
			g1_rand(a);
			g1_rand(b);
			g1_rand(c);
			/* Compare points in affine coordinates. */
			if (g1_cmp(a, c) != RLC_EQ) {
				g1_copy(c, a);
				TEST_ASSERT(g1_cmp(c, a) == RLC_EQ, end);
			}
			if (g1_cmp(b, c) != RLC_EQ) {
				g1_copy(c, b);
				TEST_ASSERT(g1_cmp(b, c) == RLC_EQ, end);
			}
			/* Compare with one point in projective. */
			g1_dbl(c, a);
			g1_norm(c, c);
			g1_dbl(a, a);
			TEST_ASSERT(g1_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(g1_cmp(a, c) == RLC_EQ, end);
			/* Compare with two points in projective. */
			g1_dbl(c, c);
			g1_dbl(a, a);
			TEST_ASSERT(g1_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(g1_cmp(a, c) == RLC_EQ, end);
			g1_neg(b, a);
			g1_add(a, a, b);
			g1_set_infty(b);
			TEST_ASSERT(g1_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("inversion and comparison are consistent") {
			g1_rand(a);
			g1_neg(b, a);
			TEST_ASSERT(g1_cmp(a, b) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE
				("assignment to random/infinity and comparison are consistent")
		{
			g1_rand(a);
			g1_set_infty(c);
			TEST_ASSERT(g1_cmp(a, c) != RLC_EQ, end);
			TEST_ASSERT(g1_cmp(c, a) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to infinity and infinity test are consistent") {
			g1_set_infty(a);
			TEST_ASSERT(g1_is_infty(a), end);
		}
		TEST_END;

		TEST_CASE("reading and writing a point are consistent") {
			for (int j = 0; j < 2; j++) {
				g1_set_infty(a);
				l = g1_size_bin(a, j);
				g1_write_bin(bin, l, a, j);
				g1_read_bin(b, bin, l);
				TEST_ASSERT(g1_cmp(a, b) == RLC_EQ, end);
				g1_rand(a);
				l = g1_size_bin(a, j);
				g1_write_bin(bin, l, a, j);
				g1_read_bin(b, bin, l);
				TEST_ASSERT(g1_cmp(a, b) == RLC_EQ, end);
				g1_rand(a);
				g1_dbl(a, a);
				l = g1_size_bin(a, j);
				g1_norm(a, a);
				g1_write_bin(bin, l, a, j);
				g1_read_bin(b, bin, l);
				TEST_ASSERT(g1_cmp(a, b) == RLC_EQ, end);
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
	g1_free(a);
	g1_free(b);
	g1_free(c);
	return code;
}

int addition1(void) {
	int code = RLC_ERR;

	g1_t a, b, c, d, e;

	g1_null(a);
	g1_null(b);
	g1_null(c);
	g1_null(d);
	g1_null(e);

	RLC_TRY {
		g1_new(a);
		g1_new(b);
		g1_new(c);
		g1_new(d);
		g1_new(e);

		TEST_CASE("point addition is commutative") {
			g1_rand(a);
			g1_rand(b);
			g1_add(d, a, b);
			g1_add(e, b, a);
			TEST_ASSERT(g1_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition is associative") {
			g1_rand(a);
			g1_rand(b);
			g1_rand(c);
			g1_add(d, a, b);
			g1_add(d, d, c);
			g1_add(e, b, c);
			g1_add(e, e, a);
			TEST_ASSERT(g1_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has identity") {
			g1_rand(a);
			g1_set_infty(d);
			g1_add(e, a, d);
			TEST_ASSERT(g1_cmp(e, a) == RLC_EQ, end);
			g1_add(e, d, a);
			TEST_ASSERT(g1_cmp(e, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has inverse") {
			g1_rand(a);
			g1_neg(d, a);
			g1_add(e, a, d);
			TEST_ASSERT(g1_is_infty(e), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(a);
	g1_free(b);
	g1_free(c);
	g1_free(d);
	g1_free(e);
	return code;
}

int subtraction1(void) {
	int code = RLC_ERR;
	g1_t a, b, c, d;

	g1_null(a);
	g1_null(b);
	g1_null(c);
	g1_null(d);

	RLC_TRY {
		g1_new(a);
		g1_new(b);
		g1_new(c);
		g1_new(d);

		TEST_CASE("point subtraction is anti-commutative") {
			g1_rand(a);
			g1_rand(b);
			g1_sub(c, a, b);
			g1_sub(d, b, a);
			g1_neg(d, d);
			TEST_ASSERT(g1_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has identity") {
			g1_rand(a);
			g1_set_infty(c);
			g1_sub(d, a, c);
			TEST_ASSERT(g1_cmp(d, a) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has inverse") {
			g1_rand(a);
			g1_sub(c, a, a);
			TEST_ASSERT(g1_is_infty(c), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(a);
	g1_free(b);
	g1_free(c);
	g1_free(d);
	return code;
}

int doubling1(void) {
	int code = RLC_ERR;
	g1_t a, b, c;

	g1_null(a);
	g1_null(b);
	g1_null(c);

	RLC_TRY {
		g1_new(a);
		g1_new(b);
		g1_new(c);

		TEST_CASE("point doubling is correct") {
			g1_rand(a);
			g1_add(b, a, a);
			g1_dbl(c, a);
			TEST_ASSERT(g1_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(a);
	g1_free(b);
	g1_free(c);
	return code;
}

static int multiplication1(void) {
	int code = RLC_ERR;
	g1_t p, q, r;
	bn_t n, k;

	bn_null(n);
	bn_null(k);
	g1_null(p);
	g1_null(q);
	g1_null(r);

	RLC_TRY {
		g1_new(p);
		g1_new(q);
		g1_new(r);
		bn_new(n);
		bn_new(k);

		g1_get_gen(p);
		pc_get_ord(n);

		TEST_CASE("generator has the right order") {
			g1_mul(r, p, n);
			TEST_ASSERT(g1_is_infty(r) == 1, end);
		} TEST_END;

		TEST_CASE("generator multiplication is correct") {
			bn_zero(k);
			g1_mul_gen(r, k);
			TEST_ASSERT(g1_is_infty(r), end);
			bn_set_dig(k, 1);
			g1_mul_gen(r, k);
			TEST_ASSERT(g1_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			g1_mul(q, p, k);
			g1_mul_gen(r, k);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g1_mul_gen(r, k);
			g1_neg(r, r);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("random element has the right order") {
			g1_rand(p);
			g1_mul(r, p, n);
			TEST_ASSERT(g1_is_infty(r) == 1, end);
		} TEST_END;

		TEST_CASE("point multiplication by digit is correct") {
			g1_mul_dig(r, p, 0);
			TEST_ASSERT(g1_is_infty(r), end);
			g1_mul_dig(r, p, 1);
			TEST_ASSERT(g1_cmp(p, r) == RLC_EQ, end);
			bn_rand(k, RLC_POS, RLC_DIG);
			g1_mul(q, p, k);
			g1_mul_dig(r, p, k->dp[0]);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(p);
	g1_free(q);
	g1_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int fixed1(void) {
	int code = RLC_ERR;
	g1_t p, q, r;
	g1_t t[RLC_G1_TABLE];
	bn_t n, k;

	bn_null(n);
	bn_null(k);
	g1_null(p);
	g1_null(q);
	g1_null(r);

	for (int i = 0; i < RLC_G1_TABLE; i++) {
		g1_null(t[i]);
	}

	RLC_TRY {
		g1_new(p);
		g1_new(q);
		g1_new(r);
		bn_new(n);
		bn_new(k);

		g1_get_gen(p);
		pc_get_ord(n);

		for (int i = 0; i < RLC_G1_TABLE; i++) {
			g1_new(t[i]);
		}
		TEST_CASE("fixed point multiplication is correct") {
			g1_rand(p);
			g1_mul_pre(t, p);
			bn_zero(k);
			g1_mul_fix(r, (const g1_t *)t, k);
			TEST_ASSERT(g1_is_infty(r), end);
			bn_set_dig(k, 1);
			g1_mul_fix(r, (const g1_t *)t, k);
			TEST_ASSERT(g1_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			g1_mul(q, p, k);
			g1_mul_fix(q, (const g1_t *)t, k);
			g1_mul(r, p, k);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g1_mul_fix(r, (const g1_t *)t, k);
			g1_neg(r, r);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_G1_TABLE; i++) {
			g1_free(t[i]);
		}
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(p);
	g1_free(q);
	g1_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int simultaneous1(void) {
	int code = RLC_ERR;
	g1_t p, q, r;
	bn_t n, k, l;

	bn_null(n);
	bn_null(k);
	bn_null(l);
	g1_null(p);
	g1_null(q);
	g1_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		bn_new(l);
		g1_new(p);
		g1_new(q);
		g1_new(r);

		g1_get_gen(p);
		pc_get_ord(n);

		TEST_CASE("simultaneous point multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			g1_mul(q, p, l);
			g1_mul_sim(r, p, k, p, l);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			g1_mul(q, p, k);
			g1_mul_sim(r, p, k, p, l);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			g1_mul_sim(r, p, k, q, l);
			g1_mul(p, p, k);
			g1_mul(q, q, l);
			g1_add(q, q, p);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g1_mul_sim(r, p, k, q, l);
			g1_mul(p, p, k);
			g1_mul(q, q, l);
			g1_add(q, q, p);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			g1_mul_sim(r, p, k, q, l);
			g1_mul(p, p, k);
			g1_mul(q, q, l);
			g1_add(q, q, p);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("simultaneous multiplication with generator is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			g1_mul(q, p, l);
			g1_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			g1_mul_gen(q, k);
			g1_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			g1_mul_sim_gen(r, k, q, l);
			g1_get_gen(p);
			g1_mul_sim(q, p, k, q, l);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g1_mul_sim_gen(r, k, q, l);
			g1_get_gen(p);
			g1_mul_sim(q, p, k, q, l);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			g1_mul_sim_gen(r, k, q, l);
			g1_get_gen(p);
			g1_mul_sim(q, p, k, q, l);
			TEST_ASSERT(g1_cmp(q, r) == RLC_EQ, end);
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
	g1_free(p);
	g1_free(q);
	g1_free(r);
	return code;
}

static int validity1(void) {
	int code = RLC_ERR;
	g1_t a;

	g1_null(a);

	RLC_TRY {
		g1_new(a);

		TEST_CASE("validity test is correct") {
			g1_set_infty(a);
			TEST_ASSERT(!g1_is_valid(a), end);
			g1_rand(a);
			TEST_ASSERT(g1_is_valid(a), end);
		}
		TEST_END;

		TEST_CASE("blinding is consistent") {
			g1_rand(a);
			g1_blind(a, a);
			TEST_ASSERT(g1_is_valid(a), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(a);
	return code;
}

static int hashing1(void) {
	int code = RLC_ERR;
	g1_t a;
	bn_t n;
	uint8_t msg[5];

	g1_null(a);
	bn_null(n);

	RLC_TRY {
		g1_new(a);
		bn_new(n);

		pc_get_ord(n);

		TEST_CASE("point hashing is correct") {
			rand_bytes(msg, sizeof(msg));
			g1_map(a, msg, sizeof(msg));
			TEST_ASSERT(g1_is_valid(a), end);
		}
		TEST_END;

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(a);
	bn_free(n);
	return code;
}

static int memory2(void) {
	err_t e;
	int code = RLC_ERR;
	g2_t a;

	g2_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			g2_new(a);
			g2_free(a);
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

int util2(void) {
	int l, code = RLC_ERR;
	g2_t a, b, c;
	uint8_t bin[8 * RLC_PC_BYTES + 1];

	g2_null(a);
	g2_null(b);
	g2_null(c);

	RLC_TRY {
		g2_new(a);
		g2_new(b);
		g2_new(c);

		TEST_CASE("comparison is consistent") {
			g2_rand(a);
			g2_rand(b);
			TEST_ASSERT(g2_cmp(a, b) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("copy and comparison are consistent") {
			g2_rand(a);
			g2_rand(b);
			g2_rand(c);
			/* Compare points in affine coordinates. */
			if (g2_cmp(a, c) != RLC_EQ) {
				g2_copy(c, a);
				TEST_ASSERT(g2_cmp(c, a) == RLC_EQ, end);
			}
			if (g2_cmp(b, c) != RLC_EQ) {
				g2_copy(c, b);
				TEST_ASSERT(g2_cmp(b, c) == RLC_EQ, end);
			}
			/* Compare with one point in projective. */
			g2_dbl(c, a);
			g2_norm(c, c);
			g2_dbl(a, a);
			TEST_ASSERT(g2_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(g2_cmp(a, c) == RLC_EQ, end);
			/* Compare with two points in projective. */
			g2_dbl(c, c);
			g2_dbl(a, a);
			TEST_ASSERT(g2_cmp(c, a) == RLC_EQ, end);
			TEST_ASSERT(g2_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("negation and comparison are consistent") {
			g2_rand(a);
			g2_neg(b, a);
			TEST_ASSERT(g2_cmp(a, b) != RLC_EQ, end);
			g2_neg(b, a);
			g2_add(a, a, b);
			g2_set_infty(b);
			TEST_ASSERT(g2_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE
				("assignment to random/infinity and comparison are consistent")
		{
			g2_rand(a);
			g2_set_infty(c);
			TEST_ASSERT(g2_cmp(a, c) != RLC_EQ, end);
			TEST_ASSERT(g2_cmp(c, a) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to infinity and infinity test are consistent") {
			g2_set_infty(a);
			TEST_ASSERT(g2_is_infty(a), end);
		}
		TEST_END;

		TEST_CASE("reading and writing a point are consistent") {
			for (int j = 0; j < 2; j++) {
				g2_set_infty(a);
				l = g2_size_bin(a, j);
				g2_write_bin(bin, l, a, j);
				g2_read_bin(b, bin, l);
				TEST_ASSERT(g2_cmp(a, b) == RLC_EQ, end);
				g2_rand(a);
				l = g2_size_bin(a, j);
				g2_write_bin(bin, l, a, j);
				g2_read_bin(b, bin, l);
				TEST_ASSERT(g2_cmp(a, b) == RLC_EQ, end);
				g2_rand(a);
				g2_dbl(a, a);
				l = g2_size_bin(a, j);
				g2_norm(a, a);
				g2_write_bin(bin, l, a, j);
				g2_read_bin(b, bin, l);
				TEST_ASSERT(g2_cmp(a, b) == RLC_EQ, end);
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
	g2_free(a);
	g2_free(b);
	g2_free(c);
	return code;
}

int addition2(void) {
	int code = RLC_ERR;

	g2_t a, b, c, d, e;

	g2_null(a);
	g2_null(b);
	g2_null(c);
	g2_null(d);
	g2_null(e);

	RLC_TRY {
		g2_new(a);
		g2_new(b);
		g2_new(c);
		g2_new(d);
		g2_new(e);

		TEST_CASE("point addition is commutative") {
			g2_rand(a);
			g2_rand(b);
			g2_add(d, a, b);
			g2_add(e, b, a);
			TEST_ASSERT(g2_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition is associative") {
			g2_rand(a);
			g2_rand(b);
			g2_rand(c);
			g2_add(d, a, b);
			g2_add(d, d, c);
			g2_add(e, b, c);
			g2_add(e, e, a);
			TEST_ASSERT(g2_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has identity") {
			g2_rand(a);
			g2_set_infty(d);
			g2_add(e, a, d);
			TEST_ASSERT(g2_cmp(e, a) == RLC_EQ, end);
			g2_add(e, d, a);
			TEST_ASSERT(g2_cmp(e, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("point addition has inverse") {
			g2_rand(a);
			g2_neg(d, a);
			g2_add(e, a, d);
			TEST_ASSERT(g2_is_infty(e), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g2_free(a);
	g2_free(b);
	g2_free(c);
	g2_free(d);
	g2_free(e);
	return code;
}

int subtraction2(void) {
	int code = RLC_ERR;
	g2_t a, b, c, d;

	g2_null(a);
	g2_null(b);
	g2_null(c);
	g2_null(d);

	RLC_TRY {
		g2_new(a);
		g2_new(b);
		g2_new(c);
		g2_new(d);

		TEST_CASE("point subtraction is anti-commutative") {
			g2_rand(a);
			g2_rand(b);
			g2_sub(c, a, b);
			g2_sub(d, b, a);
			g2_neg(d, d);
			TEST_ASSERT(g2_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has identity") {
			g2_rand(a);
			g2_set_infty(c);
			g2_sub(d, a, c);
			TEST_ASSERT(g2_cmp(d, a) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("point subtraction has inverse") {
			g2_rand(a);
			g2_sub(c, a, a);
			TEST_ASSERT(g2_is_infty(c), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g2_free(a);
	g2_free(b);
	g2_free(c);
	g2_free(d);
	return code;
}

int doubling2(void) {
	int code = RLC_ERR;
	g2_t a, b, c;

	g2_null(a);
	g2_null(b);
	g2_null(c);

	RLC_TRY {
		g2_new(a);
		g2_new(b);
		g2_new(c);

		TEST_CASE("point doubling is correct") {
			g2_rand(a);
			g2_add(b, a, a);
			g2_dbl(c, a);
			TEST_ASSERT(g2_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g2_free(a);
	g2_free(b);
	g2_free(c);
	return code;
}

static int multiplication2(void) {
	int code = RLC_ERR;
	g2_t p, q, r;
	bn_t n, k;

	bn_null(n);
	bn_null(k);
	g2_null(p);
	g2_null(q);
	g2_null(r);

	RLC_TRY {
		g2_new(p);
		g2_new(q);
		g2_new(r);
		bn_new(n);
		bn_new(k);

		g2_get_gen(p);
		pc_get_ord(n);

		TEST_CASE("generator has the right order") {
			g2_mul(r, p, n);
			TEST_ASSERT(g2_is_infty(r) == 1, end);
		} TEST_END;

		TEST_CASE("generator multiplication is correct") {
			bn_zero(k);
			g2_mul_gen(r, k);
			TEST_ASSERT(g2_is_infty(r), end);
			bn_set_dig(k, 1);
			g2_mul_gen(r, k);
			TEST_ASSERT(g2_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			g2_mul(q, p, k);
			g2_mul_gen(r, k);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g2_mul_gen(r, k);
			g2_neg(r, r);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("random element has the right order") {
			g2_rand(p);
			g2_mul(r, p, n);
			TEST_ASSERT(g2_is_infty(r) == 1, end);
		} TEST_END;

		TEST_CASE("point multiplication by digit is correct") {
			g2_mul_dig(r, p, 0);
			TEST_ASSERT(g2_is_infty(r), end);
			g2_mul_dig(r, p, 1);
			TEST_ASSERT(g2_cmp(p, r) == RLC_EQ, end);
			bn_rand(k, RLC_POS, RLC_DIG);
			g2_mul(q, p, k);
			g2_mul_dig(r, p, k->dp[0]);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g2_free(p);
	g2_free(q);
	g2_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int fixed2(void) {
	int code = RLC_ERR;
	g2_t p, q, r;
	g2_t t[RLC_G2_TABLE];
	bn_t n, k;

	bn_null(n);
	bn_null(k);
	g2_null(p);
	g2_null(q);
	g2_null(r);

	for (int i = 0; i < RLC_G2_TABLE; i++) {
		g2_null(t[i]);
	}

	RLC_TRY {
		g2_new(p);
		g2_new(q);
		g2_new(r);
		bn_new(n);
		bn_new(k);

		g2_get_gen(p);
		pc_get_ord(n);

		for (int i = 0; i < RLC_G2_TABLE; i++) {
			g2_new(t[i]);
		}
		TEST_CASE("fixed point multiplication is correct") {
			g2_rand(p);
			g2_mul_pre(t, p);
			bn_zero(k);
			g2_mul_fix(r, t, k);
			TEST_ASSERT(g2_is_infty(r), end);
			bn_set_dig(k, 1);
			g2_mul_fix(r, t, k);
			TEST_ASSERT(g2_cmp(p, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			g2_mul(q, p, k);
			g2_mul_fix(q, t, k);
			g2_mul(r, p, k);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g2_mul_fix(r, t, k);
			g2_neg(r, r);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
		} TEST_END;
		for (int i = 0; i < RLC_G2_TABLE; i++) {
			g2_free(t[i]);
		}
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g2_free(p);
	g2_free(q);
	g2_free(r);
	bn_free(n);
	bn_free(k);
	return code;
}

static int simultaneous2(void) {
	int code = RLC_ERR;
	g2_t p, q, r;
	bn_t n, k, l;

	bn_null(n);
	bn_null(k);
	bn_null(l);
	g2_null(p);
	g2_null(q);
	g2_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		bn_new(l);
		g2_new(p);
		g2_new(q);
		g2_new(r);

		g2_get_gen(p);
		pc_get_ord(n);

		TEST_CASE("simultaneous point multiplication is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			g2_mul(q, p, l);
			g2_mul_sim(r, p, k, p, l);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			g2_mul(q, p, k);
			g2_mul_sim(r, p, k, p, l);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			g2_mul_sim(r, p, k, q, l);
			g2_mul(p, p, k);
			g2_mul(q, q, l);
			g2_add(q, q, p);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g2_mul_sim(r, p, k, q, l);
			g2_mul(p, p, k);
			g2_mul(q, q, l);
			g2_add(q, q, p);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			g2_mul_sim(r, p, k, q, l);
			g2_mul(p, p, k);
			g2_mul(q, q, l);
			g2_add(q, q, p);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("simultaneous multiplication with generator is correct") {
			bn_zero(k);
			bn_rand_mod(l, n);
			g2_mul(q, p, l);
			g2_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_zero(l);
			g2_mul_gen(q, k);
			g2_mul_sim_gen(r, k, p, l);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_rand_mod(k, n);
			bn_rand_mod(l, n);
			g2_mul_sim_gen(r, k, q, l);
			g2_get_gen(p);
			g2_mul_sim(q, p, k, q, l);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_neg(k, k);
			g2_mul_sim_gen(r, k, q, l);
			g2_get_gen(p);
			g2_mul_sim(q, p, k, q, l);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
			bn_neg(l, l);
			g2_mul_sim_gen(r, k, q, l);
			g2_get_gen(p);
			g2_mul_sim(q, p, k, q, l);
			TEST_ASSERT(g2_cmp(q, r) == RLC_EQ, end);
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
	g2_free(p);
	g2_free(q);
	g2_free(r);
	return code;
}

static int validity2(void) {
	int code = RLC_ERR;
	g2_t a;

	g2_null(a);

	RLC_TRY {
		g2_new(a);

		TEST_CASE("validity test is correct") {
			g2_set_infty(a);
			TEST_ASSERT(!g2_is_valid(a), end);
			g2_rand(a);
			TEST_ASSERT(g2_is_valid(a), end);
		}
		TEST_END;

		TEST_CASE("blinding is consistent") {
			g2_rand(a);
			g2_blind(a, a);
			TEST_ASSERT(g2_is_valid(a), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g2_free(a);
	return code;
}

#if FP_PRIME != 509

static int hashing2(void) {
	int code = RLC_ERR;
	g2_t a;
	bn_t n;
	uint8_t msg[5];

	g2_null(a);
	bn_null(n);

	RLC_TRY {
		g2_new(a);
		bn_new(n);

		pc_get_ord(n);

		TEST_CASE("point hashing is correct") {
			rand_bytes(msg, sizeof(msg));
			g2_map(a, msg, sizeof(msg));
			TEST_ASSERT(g2_is_valid(a), end);
		}
		TEST_END;

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g2_free(a);
	bn_free(n);
	return code;
}

#endif

static int memory(void) {
	err_t e;
	int code = RLC_ERR;
	gt_t a;

	gt_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			gt_new(a);
			gt_free(a);
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
	int code = RLC_ERR;
	gt_t a, b, c;

	gt_null(a);
	gt_null(b);
	gt_null(c);

	RLC_TRY {
		gt_new(a);
		gt_new(b);
		gt_new(c);

		TEST_CASE("comparison is consistent") {
			gt_rand(a);
			gt_rand(b);
			TEST_ASSERT(gt_cmp(a, b) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("copy and comparison are consistent") {
			gt_rand(a);
			gt_rand(b);
			gt_rand(c);
			if (gt_cmp(a, c) != RLC_EQ) {
				gt_copy(c, a);
				TEST_ASSERT(gt_cmp(c, a) == RLC_EQ, end);
			}
			if (gt_cmp(b, c) != RLC_EQ) {
				gt_copy(c, b);
				TEST_ASSERT(gt_cmp(b, c) == RLC_EQ, end);
			}
		}
		TEST_END;

		TEST_CASE("inversion and comparison are consistent") {
			gt_rand(a);
			gt_inv(b, a);
			TEST_ASSERT(gt_cmp(a, b) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE
				("assignment to random/infinity and comparison are consistent")
		{
			gt_rand(a);
			gt_set_unity(c);
			TEST_ASSERT(gt_cmp(a, c) != RLC_EQ, end);
			TEST_ASSERT(gt_cmp(c, a) != RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("assignment to unity and unity test are consistent") {
			gt_set_unity(a);
			TEST_ASSERT(gt_is_unity(a), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	gt_free(a);
	gt_free(b);
	gt_free(c);
	return code;
}

int multiplication(void) {
	int code = RLC_ERR;

	gt_t a, b, c, d, e;

	gt_null(a);
	gt_null(b);
	gt_null(c);
	gt_null(d);
	gt_null(e);

	RLC_TRY {
		gt_new(a);
		gt_new(b);
		gt_new(c);
		gt_new(d);
		gt_new(e);

		TEST_CASE("multiplication is commutative") {
			gt_rand(a);
			gt_rand(b);
			gt_mul(d, a, b);
			gt_mul(e, b, a);
			TEST_ASSERT(gt_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication is associative") {
			gt_rand(a);
			gt_rand(b);
			gt_rand(c);
			gt_mul(d, a, b);
			gt_mul(d, d, c);
			gt_mul(e, b, c);
			gt_mul(e, e, a);
			TEST_ASSERT(gt_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication has identity") {
			gt_rand(a);
			gt_set_unity(d);
			gt_mul(e, a, d);
			TEST_ASSERT(gt_cmp(e, a) == RLC_EQ, end);
			gt_mul(e, d, a);
			TEST_ASSERT(gt_cmp(e, a) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	gt_free(a);
	gt_free(b);
	gt_free(c);
	gt_free(d);
	gt_free(e);
	return code;
}

int squaring(void) {
	int code = RLC_ERR;
	gt_t a, b, c;

	gt_null(a);
	gt_null(b);
	gt_null(c);

	RLC_TRY {
		gt_new(a);
		gt_new(b);
		gt_new(c);

		TEST_CASE("squaring is correct") {
			gt_rand(a);
			gt_mul(b, a, a);
			gt_sqr(c, a);
			TEST_ASSERT(gt_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	gt_free(a);
	gt_free(b);
	gt_free(c);
	return code;
}

static int inversion(void) {
	int code = RLC_ERR;
	gt_t a, b, c;

	RLC_TRY {
		gt_new(a);
		gt_new(b);
		gt_new(c);

		TEST_CASE("inversion is correct") {
			gt_rand(a);
			gt_inv(b, a);
			gt_mul(c, a, b);
			gt_set_unity(b);
			TEST_ASSERT(gt_cmp(c, b) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	gt_free(a);
	gt_free(b);
	gt_free(c);
	return code;
}

int exponentiation(void) {
	int code = RLC_ERR;
	gt_t a, b, c;
	bn_t n, d, e;

	gt_null(a);
	gt_null(b);
	gt_null(c);
	bn_null(d);
	bn_null(e);
	bn_null(n);

	RLC_TRY {
		gt_new(a);
		gt_new(b);
		gt_new(c);
		bn_new(d);
		bn_new(e);
		bn_new(n);

		gt_get_gen(a);
		pc_get_ord(n);

		TEST_CASE("generator has the right order") {
			gt_exp(c, a, n);
			TEST_ASSERT(gt_is_unity(c), end);
		} TEST_END;

		TEST_CASE("generator exponentiation is correct") {
			bn_zero(d);
			gt_exp_gen(c, d);
			TEST_ASSERT(gt_is_unity(c), end);
			bn_set_dig(d, 1);
			gt_exp_gen(c, d);
			TEST_ASSERT(gt_cmp(c, a) == RLC_EQ, end);
			bn_add_dig(d, n, 1);
			gt_exp_gen(c, d);
			TEST_ASSERT(gt_cmp(c, a) == RLC_EQ, end);
			gt_exp_gen(c, n);
			TEST_ASSERT(gt_is_unity(c), end);
			bn_rand_mod(d, n);
			gt_exp_gen(b, d);
			bn_neg(d, d);
			gt_exp_gen(c, d);
			gt_inv(c, c);
			TEST_ASSERT(gt_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("random element has the right order") {
			gt_rand(a);
			gt_exp(c, a, n);
			TEST_ASSERT(gt_is_unity(c) == 1, end);
		} TEST_END;

		TEST_CASE("exponentiation is correct") {
			gt_rand(a);
			gt_rand(b);
			bn_rand_mod(d, n);
			bn_rand_mod(e, n);
			gt_exp_sim(c, a, d, b, e);
			gt_exp(a, a, d);
			gt_exp(b, b, e);
			gt_mul(b, a, b);
			TEST_ASSERT(gt_cmp(b, c) == RLC_EQ, end);
			gt_exp_dig(b, a, 0);
			TEST_ASSERT(gt_is_unity(b), end);
			gt_exp_dig(b, a, 1);
			TEST_ASSERT(gt_cmp(a, b) == RLC_EQ, end);
			bn_rand(d, RLC_POS, RLC_DIG);
			gt_exp(b, a, d);
			gt_exp_dig(c, a, d->dp[0]);
			TEST_ASSERT(gt_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	gt_free(a);
	gt_free(b);
	gt_free(c);
	bn_free(d);
	bn_free(e);
	bn_free(n);
	return code;
}

static int validity(void) {
	int code = RLC_ERR;
	gt_t a;

	gt_null(a);

	RLC_TRY {
		gt_new(a);

		TEST_CASE("validity check is correct") {
			gt_set_unity(a);
			TEST_ASSERT(!gt_is_valid(a), end);
			gt_rand(a);
			TEST_ASSERT(gt_is_valid(a), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	gt_free(a);
	return code;
}

static int pairing(void) {
	int j, code = RLC_ERR;
	g1_t p[2];
	g2_t q[2];
	gt_t e1, e2;
	bn_t k, n;

	gt_null(e1);
	gt_null(e2);
	bn_null(k);
	bn_null(n);

	RLC_TRY {
		gt_new(e1);
		gt_new(e2);
		bn_new(k);
		bn_new(n);

		for (j = 0; j < 2; j++) {
			g1_null(p[j]);
			g2_null(q[j]);
			g1_new(p[j]);
			g2_new(q[j]);
		}

		pc_get_ord(n);

		TEST_CASE("pairing non-degeneracy is correct") {
			g1_rand(p[0]);
			g2_rand(q[0]);
			pc_map(e1, p[0], q[0]);
			TEST_ASSERT(gt_cmp_dig(e1, 1) != RLC_EQ, end);
			g1_set_infty(p[0]);
			pc_map(e1, p[0], q[0]);
			TEST_ASSERT(gt_cmp_dig(e1, 1) == RLC_EQ, end);
			g1_rand(p[0]);
			g2_set_infty(q[0]);
			pc_map(e1, p[0], q[0]);
			TEST_ASSERT(gt_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing is bilinear") {
			g1_rand(p[0]);
			g2_rand(q[0]);
			bn_rand_mod(k, n);
			g2_mul(q[1], q[0], k);
			pc_map(e1, p[0], q[1]);
			pc_map(e2, p[0], q[0]);
			gt_exp(e2, e2, k);
			TEST_ASSERT(gt_cmp(e1, e2) == RLC_EQ, end);
			g1_mul(p[0], p[0], k);
			pc_map(e2, p[0], q[0]);
			TEST_ASSERT(gt_cmp(e1, e2) == RLC_EQ, end);
			g1_dbl(p[0], p[0]);
			pc_map(e2, p[0], q[0]);
			gt_sqr(e1, e1);
			TEST_ASSERT(gt_cmp(e1, e2) == RLC_EQ, end);
			g2_dbl(q[0], q[0]);
			pc_map(e2, p[0], q[0]);
			gt_sqr(e1, e1);
			TEST_ASSERT(gt_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multi-pairing is correct") {
			g1_rand(p[i % 2]);
			g2_rand(q[i % 2]);
			pc_map(e1, p[i % 2], q[i % 2]);
			g1_rand(p[1 - (i % 2)]);
			g2_set_infty(q[1 - (i % 2)]);
			pc_map_sim(e2, p, q, 2);
			TEST_ASSERT(gt_cmp(e1, e2) == RLC_EQ, end);
			g1_set_infty(p[1 - (i % 2)]);
			g2_rand(q[1 - (i % 2)]);
			pc_map_sim(e2, p, q, 2);
			TEST_ASSERT(gt_cmp(e1, e2) == RLC_EQ, end);
			g2_set_infty(q[i % 2]);
			pc_map_sim(e2, p, q, 2);
			TEST_ASSERT(gt_cmp_dig(e2, 1) == RLC_EQ, end);
			g1_rand(p[0]);
			g2_rand(q[0]);
			pc_map(e1, p[0], q[0]);
			g1_rand(p[1]);
			g2_rand(q[1]);
			pc_map(e2, p[1], q[1]);
			gt_mul(e1, e1, e2);
			pc_map_sim(e2, p, q, 2);
			TEST_ASSERT(gt_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	gt_free(e1);
	gt_free(e2);
	bn_free(k);
	bn_free(n);
	for (j = 0; j < 2; j++) {
		g1_free(p[j]);
		g2_free(q[j]);
	}
	return code;
}

int test1(void) {
	util_banner("Utilities:", 1);

	if (memory1() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (util1() != RLC_OK) {
		return RLC_ERR;
	}

	util_banner("Arithmetic:", 1);

	if (addition1() != RLC_OK) {
		return RLC_ERR;
	}

	if (subtraction1() != RLC_OK) {
		return RLC_ERR;
	}

	if (doubling1() != RLC_OK) {
		return RLC_ERR;
	}

	if (multiplication1() != RLC_OK) {
		return RLC_ERR;
	}

	if (fixed1() != RLC_OK) {
		return RLC_ERR;
	}

	if (simultaneous1() != RLC_OK) {
		return RLC_ERR;
	}

	if (validity1() != RLC_OK) {
		return RLC_ERR;
	}

	if (hashing1() != RLC_OK) {
		return RLC_ERR;
	}

	return RLC_OK;
}

int test2(void) {
	util_banner("Utilities:", 1);

	if (memory2() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (util2() != RLC_OK) {
		return RLC_ERR;
	}

	util_banner("Arithmetic:", 1);

	if (addition2() != RLC_OK) {
		return RLC_ERR;
	}

	if (subtraction2() != RLC_OK) {
		return RLC_ERR;
	}

	if (doubling2() != RLC_OK) {
		return RLC_ERR;
	}

	if (multiplication2() != RLC_OK) {
		return RLC_ERR;
	}

	if (fixed2() != RLC_OK) {
		return RLC_ERR;
	}

	if (simultaneous2() != RLC_OK) {
		return RLC_ERR;
	}

	if (validity2() != RLC_OK) {
		return RLC_ERR;
	}

#if FP_PRIME != 509
	if (hashing2() != RLC_OK) {
		return RLC_ERR;
	}
#endif

	return RLC_OK;
}

int test(void) {
	util_banner("Utilities:", 1);

	if (memory() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (util() != RLC_OK) {
		return RLC_ERR;
	}

	util_banner("Arithmetic:", 1);

	if (multiplication() != RLC_OK) {
		return RLC_ERR;
	}

	if (squaring() != RLC_OK) {
		return RLC_ERR;
	}

	if (inversion() != RLC_OK) {
		return RLC_ERR;
	}

	if (exponentiation() != RLC_OK) {
		return RLC_ERR;
	}

	if (validity() != RLC_OK) {
		return RLC_ERR;
	}

	if (pairing() != RLC_OK) {
		return RLC_ERR;
	}

	return RLC_OK;
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the PC module:", 0);

	if (pc_param_set_any() != RLC_OK) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	pc_param_print();

	util_banner("Group G_1:", 0);
	if (test1() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Group G_2:", 0);
	if (test2() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Group G_T:", 0);
	if (test() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
