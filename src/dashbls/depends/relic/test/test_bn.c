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
 * Tests for multiple precision integer arithmetic.
 *
 * @ingroup test
 */

#include <stdlib.h>
#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = RLC_ERR;
	bn_t a;

	bn_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			bn_new(a);
			bn_free(a);
		}
		TEST_END;
	}
	RLC_CATCH(e) {
		switch (e) {
			case ERR_NO_MEMORY:
				util_print("FATAL ERROR!\n");
				RLC_ERROR(end);
				break;
		}
	}
	code = RLC_OK;
  end:
	return code;
}

static int util(void) {
	int bits, code = RLC_ERR;
	char str[RLC_BN_BITS + 2];
	dig_t digit, raw[RLC_BN_DIGS];
	uint8_t bin[RLC_CEIL(RLC_BN_BITS, 8)];
	bn_t a, b, c;

	bn_null(a);
	bn_null(b);
	bn_null(c);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);

		TEST_CASE("comparison is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			if (bn_cmp(a, b) != RLC_EQ) {
				if (bn_cmp(a, b) == RLC_GT) {
					TEST_ASSERT(bn_cmp(b, a) == RLC_LT, end);
				} else {
					TEST_ASSERT(bn_cmp(b, a) == RLC_GT, end);
				}
			}
		}
		TEST_END;

		TEST_CASE("copy and comparison are consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_rand(c, RLC_POS, RLC_BN_BITS);
			if (bn_cmp(a, c) != RLC_EQ) {
				bn_copy(c, a);
				TEST_ASSERT(bn_cmp(c, a) == RLC_EQ, end);
			}
			if (bn_cmp(b, c) != RLC_EQ) {
				bn_copy(c, b);
				TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
			}
		}
		TEST_END;

		TEST_CASE("absolute, negation and comparison are consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_neg(b, a);
			bn_abs(a, b);
			TEST_ASSERT(bn_cmp(a, b) == RLC_GT, end);
			TEST_ASSERT(bn_cmp(b, a) == RLC_LT, end);
			TEST_ASSERT(bn_cmp_abs(a, b) == RLC_EQ, end);
			TEST_ASSERT(bn_cmp_dig(a, (dig_t)0) == RLC_GT, end);
			TEST_ASSERT(bn_cmp_dig(b, (dig_t)0) == RLC_LT, end);
		} TEST_END;

		TEST_CASE("signal test is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_NEG, RLC_BN_BITS);
			TEST_ASSERT(bn_sign(a) == RLC_POS, end);
			TEST_ASSERT(bn_sign(b) == RLC_NEG, end);
		} TEST_END;

		TEST_CASE("assignment to zero and comparison are consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_NEG, RLC_BN_BITS);
			bn_zero(c);
			TEST_ASSERT(bn_cmp(a, c) == RLC_GT, end);
			TEST_ASSERT(bn_cmp(c, a) == RLC_LT, end);
			TEST_ASSERT(bn_cmp(b, c) == RLC_LT, end);
			TEST_ASSERT(bn_cmp(c, b) == RLC_GT, end);
			TEST_ASSERT(bn_cmp_dig(a, (dig_t)0) == RLC_GT, end);
			TEST_ASSERT(bn_cmp_dig(b, (dig_t)0) == RLC_LT, end);
			TEST_ASSERT(bn_cmp_dig(c, (dig_t)0) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("assignment to zero and zero test are consistent") {
			bn_zero(c);
			TEST_ASSERT(bn_is_zero(c), end);
			TEST_ASSERT(bn_cmp_dig(c, (dig_t)0) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("oddness test is correct") {
			bn_set_dig(a, 2);
			bn_set_dig(b, 1);
			TEST_ASSERT(bn_is_even(a) == 1, end);
			TEST_ASSERT(bn_is_even(b) == 0, end);
		} TEST_END;

		bits = 0;
		TEST_CASE("assignment and bit counting are consistent") {
			bn_set_2b(a, bits);
			TEST_ASSERT(bits + 1 == bn_bits(a), end);
			bits = (bits + 1) % RLC_BN_BITS;
		} TEST_END;

		bits = 0;
		TEST_CASE("bit setting and getting are consistent") {
			bn_zero(a);
			bn_set_bit(a, bits, 1);
			TEST_ASSERT(bn_get_bit(a, bits) == 1, end);
			bn_set_bit(a, bits, 0);
			TEST_ASSERT(bn_get_bit(a, bits) == 0, end);
			bits = (bits + 1) % RLC_BN_BITS;
		}
		TEST_END;

		bits = 0;
		TEST_CASE("hamming weight is correct") {
			bn_zero(a);
			for (int j = 0; j < bits; j++) {
				bn_set_bit(a, j, 1);
			}
			TEST_ASSERT(bn_ham(a) == bits, end);
			bits = (bits + 1) % RLC_BN_BITS;
		}
		TEST_END;

		TEST_CASE("generating a random integer is consistent") {
			do {
				bn_rand(b, RLC_POS, RLC_BN_BITS);
			} while (bn_is_zero(b));
			bn_rand_mod(a, b);
			TEST_ASSERT(bn_sign(a) == bn_sign(b), end);
			TEST_ASSERT(bn_is_zero(a) == 0, end);
			TEST_ASSERT(bn_cmp(a, b) == RLC_LT, end);
			do {
				bn_rand(b, RLC_NEG, RLC_DIG);
			} while (bn_bits(b) <= 1);
			bn_rand_mod(a, b);
			TEST_ASSERT(bn_sign(a) == bn_sign(b), end);
			TEST_ASSERT(bn_is_zero(a) == 0, end);
			TEST_ASSERT(bn_cmp(a, b) == RLC_GT, end);
		}
		TEST_END;

		TEST_CASE("reading and writing the first digit are consistent") {
			bn_rand(a, RLC_POS, RLC_DIG);
			bn_rand(b, RLC_POS, RLC_DIG);
			bn_get_dig(&digit, a);
			bn_set_dig(b, digit);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("assignment to a constant and comparison are consistent") {
			bn_set_dig(a, 2);
			bn_set_dig(b, 1);
			TEST_ASSERT(bn_cmp(a, b) == RLC_GT, end);
			TEST_ASSERT(bn_cmp(b, a) == RLC_LT, end);
			TEST_ASSERT(bn_cmp_dig(a, (dig_t)0) == RLC_GT, end);
			TEST_ASSERT(bn_cmp_dig(b, (dig_t)0) == RLC_GT, end);
		} TEST_END;

		TEST_CASE("assignment to random and comparison are consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_NEG, RLC_BN_BITS);
			bn_zero(c);
			TEST_ASSERT(bn_cmp(a, c) == RLC_GT, end);
			TEST_ASSERT(bn_cmp(b, c) == RLC_LT, end);
			TEST_ASSERT(bn_cmp_dig(a, (dig_t)0) == RLC_GT, end);
			TEST_ASSERT(bn_cmp_dig(b, (dig_t)0) == RLC_LT, end);
		} TEST_END;

		bits = 0;
		TEST_CASE("different forms of assignment are consistent") {
			bn_set_dig(a, (dig_t)(1) << (dig_t)bits);
			bn_set_2b(b, bits);
			bits++;
			bits %= (RLC_DIG);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("reading and writing a positive number are consistent") {
			int len = RLC_CEIL(RLC_BN_BITS, 8);
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			for (int j = 2; j <= 64; j++) {
				bits = bn_size_str(a, j);
				bn_write_str(str, bits, a, j);
				bn_read_str(b, str, bits, j);
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
			bn_write_bin(bin, len, a);
			bn_read_bin(b, bin, len);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			len = RLC_BN_DIGS;
			bn_write_raw(raw, len, a);
			bn_read_raw(b, raw, len);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("getting the size of a positive number is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			TEST_ASSERT((bn_size_str(a, 2) - 1) == bn_bits(a), end);
			bits = (bn_bits(a) % 8 == 0 ? bn_bits(a) / 8 : bn_bits(a) / 8 + 1);
			TEST_ASSERT(bn_size_bin(a) == bits, end);
			TEST_ASSERT(bn_size_raw(a) == a->used, end);
		}
		TEST_END;

		TEST_CASE("reading and writing a negative number are consistent") {
			int len = RLC_CEIL(RLC_BN_BITS, 8);
			bn_rand(a, RLC_NEG, RLC_BN_BITS);
			for (int j = 2; j <= 64; j++) {
				bits = bn_size_str(a, j);
				bn_write_str(str, bits, a, j);
				bn_read_str(b, str, bits, j);
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
			bn_write_bin(bin, len, a);
			bn_read_bin(b, bin, len);
			bn_neg(b, b);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			len = RLC_BN_DIGS;
			bn_write_raw(raw, len, a);
			bn_read_raw(b, raw, len);
			bn_neg(b, b);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("getting the size of a negative number is correct") {
			bn_rand(a, RLC_NEG, RLC_BN_BITS);
			TEST_ASSERT((bn_size_str(a, 2) - 2) == bn_bits(a), end);
			bits = (bn_bits(a) % 8 == 0 ? bn_bits(a) / 8 : bn_bits(a) / 8 + 1);
			TEST_ASSERT(bn_size_bin(a) == bits, end);
			TEST_ASSERT(bn_size_raw(a) == a->used, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	return code;
}

static int addition(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, e;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(e);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(e);

		TEST_CASE("addition is commutative") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_add(d, a, b);
			bn_add(e, b, a);
			TEST_ASSERT(bn_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("addition is associative") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_rand(c, RLC_POS, RLC_BN_BITS);
			bn_add(d, a, b);
			bn_add(d, d, c);
			bn_add(e, b, c);
			bn_add(e, a, e);
			TEST_ASSERT(bn_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("addition has identity") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_zero(d);
			bn_add(e, a, d);
			TEST_ASSERT(bn_cmp(e, a) == RLC_EQ, end);
			bn_add(e, d, a);
			TEST_ASSERT(bn_cmp(e, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("addition has inverse") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_neg(d, a);
			bn_add(e, a, d);
			TEST_ASSERT(bn_is_zero(e), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(e);
	return code;
}

static int subtraction(void) {
	int code = RLC_ERR;
	int s;
	bn_t a, b, c, d;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);

		TEST_CASE("subtraction is anti-commutative") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_sub(c, a, b);
			bn_sub(d, b, a);
			TEST_ASSERT(bn_cmp_abs(c, d) == RLC_EQ, end);
			if (!bn_is_zero(c)) {
				s = bn_sign(d);
				TEST_ASSERT(bn_sign(c) != s, end);
			}
		}
		TEST_END;

		TEST_CASE("subtraction has identity") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_zero(c);
			bn_sub(d, a, c);
			TEST_ASSERT(bn_cmp(d, a) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("subtraction has inverse") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_sub(c, a, a);
			TEST_ASSERT(bn_is_zero(c), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	return code;
}

static int multiplication(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, e, f;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(e);
	bn_null(f);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(e);
		bn_new(f);

		TEST_CASE("multiplication is commutative") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_mul(d, a, b);
			bn_mul(e, b, a);
			TEST_ASSERT(bn_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication is associative") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 3);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 3);
			bn_rand(c, RLC_POS, RLC_BN_BITS / 3);
			bn_mul(d, a, b);
			bn_mul(d, d, c);
			bn_mul(e, b, c);
			bn_mul(e, a, e);
			TEST_ASSERT(bn_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication is distributive") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(c, RLC_POS, RLC_BN_BITS / 2);
			bn_add(d, a, b);
			bn_mul(d, c, d);
			bn_mul(e, c, a);
			bn_mul(f, c, b);
			bn_add(e, e, f);
			TEST_ASSERT(bn_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication has identity") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_set_dig(d, (dig_t)1);
			bn_mul(e, a, d);
			TEST_ASSERT(bn_cmp(e, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication has zero property") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_zero(d);
			bn_mul(e, a, d);
			TEST_ASSERT(bn_is_zero(e), end);
		} TEST_END;

		TEST_CASE("multiplication has negation property") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_set_dig(d, 1);
			bn_neg(d, d);
			bn_mul(e, a, d);
			TEST_ASSERT(bn_cmp_abs(e, a) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(e) == RLC_NEG, end);
		} TEST_END;

		TEST_CASE("multiplication by a positive number preserves order") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(c, RLC_POS, RLC_BN_BITS / 2);
			int t = bn_cmp(a, b);
			bn_mul(d, c, a);
			bn_mul(e, c, b);
			TEST_ASSERT(bn_cmp(d, e) == t, end);
		} TEST_END;

		TEST_CASE("multiplication by a negative number reverses order") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(c, RLC_POS, RLC_BN_BITS / 2);
			int t = bn_cmp(a, b);
			bn_neg(d, c);
			bn_mul(e, d, a);
			bn_mul(d, d, b);
			if (t != RLC_EQ) {
				TEST_ASSERT(bn_cmp(e, d) != t, end);
			}
		}
		TEST_END;

#if BN_MUL == BASIC || !defined(STRIP)
		TEST_CASE("basic multiplication is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_mul(c, a, b);
			bn_mul_basic(d, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if BN_MUL == COMBA || !defined(STRIP)
		TEST_CASE("comba multiplication is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_mul(c, a, b);
			bn_mul_comba(d, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if BN_KARAT > 0 || !defined(STRIP)
		TEST_CASE("karatsuba multiplication is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_mul(c, a, b);
			bn_mul_karat(d, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(e);
	bn_free(f);
	return code;
}

static int squaring(void) {
	int code = RLC_ERR;
	bn_t a, b, c;

	bn_null(a);
	bn_null(b);
	bn_null(c);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);

		TEST_CASE("squaring is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_mul(b, a, a);
			bn_sqr(c, a);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

#if BN_SQR == BASIC || !defined(STRIP)
		TEST_CASE("basic squaring is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_sqr(b, a);
			bn_sqr_basic(c, a);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if BN_SQR == COMBA || !defined(STRIP)
		TEST_CASE("comba squaring is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_sqr(b, a);
			bn_sqr_comba(c, a);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if BN_KARAT > 0 || !defined(STRIP)
		TEST_CASE("karatsuba squaring is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_sqr(b, a);
			bn_sqr_karat(c, a);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	return code;
}

static int doubling_halving(void) {
	int code = RLC_ERR;
	bn_t a, b, c;

	bn_null(a);
	bn_null(b);
	bn_null(c);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);

		TEST_CASE("doubling is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - 1);
			bn_add(b, a, a);
			bn_dbl(c, a);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("halving is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - 1);
			bn_dbl(b, a);
			bn_hlv(c, b);
			TEST_ASSERT(bn_cmp(c, a) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	return code;
}

static int shifting(void) {
	int code = RLC_ERR;
	bn_t a, b, c;

	bn_null(a);
	bn_null(b);
	bn_null(c);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);

		TEST_CASE("shifting by 1 bit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - 1);
			bn_lsh(b, a, 1);
			bn_dbl(c, a);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
			bn_rsh(b, a, 1);
			bn_hlv(c, a);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
			bn_lsh(b, a, 1);
			bn_rsh(c, b, 1);
			TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("shifting by 2 bits is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - 2);
			bn_lsh(b, a, 2);
			bn_rsh(c, b, 2);
			TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("shifting by half digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - RLC_DIG / 2);
			bn_lsh(b, a, RLC_DIG / 2);
			bn_rsh(c, b, RLC_DIG / 2);
			TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("shifting by 1 digit is consistent") {
			bn_rand(a, RLC_POS, RLC_DIG);
			bn_lsh(b, a, RLC_DIG);
			bn_rsh(c, b, RLC_DIG);
			TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("shifting by 2 digits is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - 2 * RLC_DIG);
			bn_lsh(b, a, 2 * RLC_DIG);
			bn_rsh(c, b, 2 * RLC_DIG);
			TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("shifting by 1 digit and half is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - RLC_DIG - RLC_DIG / 2);
			bn_lsh(b, a, RLC_DIG + RLC_DIG / 2);
			bn_copy(c, a);
			for (int j = 0; j < (int)(RLC_DIG + RLC_DIG / 2); j++)
				bn_dbl(c, c);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
			bn_rsh(b, a, (RLC_DIG + RLC_DIG / 2));
			bn_copy(c, a);
			for (int j = 0; j < (int)(RLC_DIG + RLC_DIG / 2); j++)
				bn_hlv(c, c);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	return code;
}

static int division(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, e;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(e);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(e);

		TEST_CASE("trivial division is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) == bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == RLC_POS, end);
		} TEST_END;

		TEST_CASE("trivial negative division is correct") {
			bn_rand(a, RLC_NEG, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) == bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == bn_sign(b), end);
		} TEST_END;

		TEST_CASE("trivial division by negative is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_NEG, RLC_BN_BITS);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) != bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == bn_sign(b), end);
		} TEST_END;

		TEST_CASE("negative trivial division by negative is correct") {
			bn_rand(a, RLC_NEG, RLC_BN_BITS / 2);
			bn_rand(b, RLC_NEG, RLC_BN_BITS);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) != bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == bn_sign(b), end);
		} TEST_END;

		TEST_CASE("division is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) == bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == bn_sign(d), end);
		} TEST_END;

		TEST_CASE("negative division is correct") {
			bn_rand(a, RLC_NEG, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) == bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == RLC_POS, end);
		} TEST_END;

		TEST_CASE("division by negative is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_NEG, RLC_BN_BITS / 2);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) != bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == bn_sign(b), end);
		} TEST_END;

		TEST_CASE("negative division by negative is correct") {
			bn_rand(a, RLC_NEG, RLC_BN_BITS);
			bn_rand(b, RLC_NEG, RLC_BN_BITS / 2);
			bn_div(e, a, b);
			bn_div_rem(c, d, a, b);
			TEST_ASSERT(bn_cmp(e, c) == RLC_EQ, end);
			bn_mul(e, c, b);
			bn_add(e, e, d);
			TEST_ASSERT(bn_cmp(a, e) == RLC_EQ, end);
			TEST_ASSERT(bn_sign(a) != bn_sign(c), end);
			TEST_ASSERT(bn_sign(d) == bn_sign(b), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(e);
	return code;
}

static int reduction(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, e;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(e);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(e);

#if BN_MOD == BASIC || !defined(STRIP)
		TEST_CASE("basic reduction is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - RLC_DIG / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_div_rem(c, d, a, b);
			bn_sqr(c, b);
			if (bn_cmp(a, c) == RLC_LT) {
				bn_mod_basic(e, a, b);
				TEST_ASSERT(bn_cmp(e, d) == RLC_EQ, end);
			}
		}
		TEST_END;
#endif

#if BN_MOD == BARRT || !defined(STRIP)
		TEST_CASE("barrett reduction is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - RLC_DIG / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_div_rem(c, d, a, b);
			bn_sqr(c, b);
			if (bn_cmp(a, c) == RLC_LT) {
				bn_mod_pre_barrt(e, b);
				bn_mod_barrt(e, a, b, e);
				TEST_ASSERT(bn_cmp(e, d) == RLC_EQ, end);
			}
		}
		TEST_END;
#endif

#if (BN_MOD == MONTY && BN_MUL == BASIC) || !defined(STRIP)
		TEST_CASE("basic montgomery reduction is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - RLC_DIG / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			if (bn_is_even(b)) {
				bn_add_dig(b, b, 1);
			}
			bn_mod(a, a, b);
			bn_mod_monty_conv(c, a, b);
			bn_mod_pre_monty(e, b);
			bn_mod_monty_basic(d, c, b, e);
			TEST_ASSERT(bn_cmp(a, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if (BN_MOD == MONTY && BN_MUL == COMBA) || !defined(STRIP)
		TEST_CASE("comba montgomery reduction is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS - RLC_DIG / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			if (bn_is_even(b)) {
				bn_add_dig(b, b, 1);
			}
			bn_mod(a, a, b);
			bn_mod_monty_conv(c, a, b);
			bn_mod_pre_monty(e, b);
			bn_mod_monty_comba(d, c, b, e);
			TEST_ASSERT(bn_cmp(a, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if BN_MOD == PMERS || !defined(STRIP)
		TEST_CASE("pseudo-mersenne reduction is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(c, RLC_POS, RLC_BN_BITS / 4);
			if (bn_is_zero(c))
				bn_set_dig(c, 1);
			bn_set_2b(b, RLC_BN_BITS / 2);
			bn_sub(b, b, c);
			bn_mod(c, a, b);
			bn_mod_pre_pmers(e, b);
			bn_mod_pmers(d, a, b, e);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(e);
	return code;
}

static int exponentiation(void) {
	int code = RLC_ERR;
	bn_t a, b, c, p;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(p);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(p);

#if BN_MOD != PMERS
		bn_gen_prime(p, RLC_BN_BITS);
#elif BN_PRECI >= 128
		/* Let's try a Mersenne prime. */
		bn_zero(p);
		bn_set_bit(p, 127, 1);
		bn_sub_dig(p, p, 1);
#endif

		TEST_CASE("modular exponentiation is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_mod(a, a, p);
			bn_mxp(b, a, p, p);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("modular exponentiation with zero power is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_zero(b);
			bn_mxp(a, a, b, p);
			TEST_ASSERT(bn_cmp_dig(a, 1) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("modular exponentiation with negative power is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_mod(a, a, p);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			/* Compute c = a^b mod p. */
			bn_mxp(c, a, b, p);
			/* Compute b = a^-b mod p. */
			bn_neg(b, b);
			bn_mxp(b, a, b, p);
			/* Check that c * b = 1 mod p. */
			bn_mul(c, c, b);
			bn_mod(c, c, p);
			TEST_ASSERT(bn_cmp_dig(c, 1) == RLC_EQ, end);
		}
		TEST_END;

#if BN_MXP == BASIC || !defined(STRIP)
		TEST_CASE("basic modular exponentiation is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_mod(a, a, p);
			bn_mxp_basic(b, a, p, p);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if BN_MXP == SLIDE || !defined(STRIP)
		TEST_CASE("sliding window modular exponentiation is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_mod(a, a, p);
			bn_mxp_slide(b, a, p, p);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if BN_MXP == CONST || !defined(STRIP)
		TEST_CASE("powering ladder modular exponentiation is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_mod(a, a, p);
			bn_mxp_monty(b, a, p, p);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(p);
	return code;
}

static int square_root(void) {
	int bits, code = RLC_ERR;
	bn_t a, b, c;

	bn_null(a);
	bn_null(b);
	bn_null(c);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);

		TEST_ONCE("square root extraction is correct") {
			for (bits = 0; bits < RLC_BN_BITS / 2; bits++) {
				bn_rand(a, RLC_POS, bits);
				bn_sqr(c, a);
				bn_srt(b, c);
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
			for (bits = 0; bits < RLC_BN_BITS; bits++) {
				bn_rand(a, RLC_POS, bits);
				bn_srt(b, a);
				bn_sqr(c, b);
				TEST_ASSERT(bn_cmp(c, a) != RLC_GT, end);
			}
		}
		TEST_END;

		TEST_ONCE("square root of powers of 2 is correct") {
			for (bits = 0; bits < RLC_BN_BITS / 2; bits++) {
				bn_set_2b(a, bits);
				bn_sqr(c, a);
				bn_srt(b, c);
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	return code;
}

static int gcd(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, e, f;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(e);
	bn_null(f);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(e);
		bn_new(f);

		TEST_CASE("greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd(c, a, b);
			bn_mod(d, a, c);
			bn_mod(e, b, c);
			TEST_ASSERT(bn_is_zero(d) && bn_is_zero(e), end);
			bn_div(a, a, c);
			bn_div(b, b, c);
			bn_gcd(c, a, b);
			TEST_ASSERT(bn_cmp_dig(c, 1) == RLC_EQ, end);
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_zero(b);
			bn_gcd(c, a, b);
			TEST_ASSERT(bn_cmp_abs(c, a) == RLC_EQ, end);
			bn_rand(a, RLC_NEG, RLC_BN_BITS);
			bn_zero(b);
			bn_gcd(c, a, b);
			TEST_ASSERT(bn_cmp_abs(c, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("extended greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd_ext(c, d, e, a, b);
			bn_mul(d, d, a);
			bn_mul(e, e, b);
			bn_add(d, d, e);
			bn_gcd(f, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ && bn_cmp(c, f) == RLC_EQ, end);
		} TEST_END;

#if BN_GCD == BASIC || !defined(STRIP)
		TEST_CASE("basic greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd(c, a, b);
			bn_gcd_basic(d, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("basic extended greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd_ext_basic(c, d, e, a, b);
			bn_mul(d, d, a);
			bn_mul(e, e, b);
			bn_add(d, d, e);
			bn_gcd_basic(f, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ && bn_cmp(c, f) == RLC_EQ, end);
		} TEST_END;
#endif

#if BN_GCD == LEHME || !defined(STRIP)
		TEST_CASE("lehmer greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd(c, a, b);
			bn_gcd_lehme(d, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("lehmer extended greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd_ext_lehme(c, d, e, a, b);
			bn_mul(d, d, a);
			bn_mul(e, e, b);
			bn_add(d, d, e);
			bn_gcd_lehme(f, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ && bn_cmp(c, f) == RLC_EQ, end);
		} TEST_END;
#endif

#if BN_GCD == STEIN || !defined(STRIP)
		TEST_CASE("stein greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd(c, a, b);
			bn_gcd_stein(d, a, b);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("stein extended greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd_ext_stein(c, d, e, a, b);
			bn_mul(d, d, a);
			bn_mul(e, e, b);
			bn_add(d, d, e);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("midway extended greatest common divisor is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_gcd_ext_mid(c, d, e, f, a, b);
			bn_abs(d, d);
			bn_abs(f, f);
			bn_mul(c, c, f);
			bn_mul(e, e, d);
			bn_add(c, c, e);
			TEST_ASSERT(bn_cmp(b, c) == RLC_EQ || bn_cmp(a, c) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(e);
	bn_free(f);
	return code;
}

static int lcm(void) {
	int code = RLC_ERR;
	bn_t a, b, c;

	bn_null(a);
	bn_null(b);
	bn_null(c);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);

		TEST_CASE("least common multiple is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			bn_lcm(c, a, b);
			bn_mod(a, c, a);
			bn_mod(b, c, b);
			TEST_ASSERT(bn_is_zero(a) && bn_is_zero(b), end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	return code;
}

static int symbol(void) {
	int code = RLC_ERR;
	bn_t a, b, c, p, q;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(p);
	bn_null(q);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(p);
		bn_new(q);

		do {
			bn_gen_prime(p, RLC_BN_BITS);
			bn_gen_prime(q, RLC_BN_BITS);
		} while (bn_is_even(p) || bn_is_even(q));

		TEST_CASE("legendre symbol is correct") {
			bn_smb_leg(c, p, p);
			TEST_ASSERT(bn_is_zero(c) == 1, end);
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_sqr(a, a);
			bn_mod(a, a, p);
			bn_smb_leg(c, a, p);
			TEST_ASSERT(bn_cmp_dig(c, 1) == RLC_EQ, end);
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_smb_leg(c, a, p);
			bn_set_dig(a, 1);
			TEST_ASSERT(bn_cmp_abs(c, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("legendre symbol is a homomorphism") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_mul(c, a, b);
			bn_smb_leg(a, a, p);
			bn_smb_leg(b, b, p);
			bn_smb_leg(c, c, p);
			bn_mul(a, a, b);
			TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
		} TEST_END;

		TEST_ONCE("legendre symbol satisfies quadratic reciprocity") {
			/* Check the first supplement: (-1|p) = (-1)^(p-1)/2. */
			bn_set_dig(a, 1);
			bn_neg(a, a);
			bn_smb_leg(b, a, p);
			bn_sub_dig(c, p, 1);
			bn_rsh(c, c, 1);
			if (bn_is_even(c)) {
				bn_neg(a, a);
			}
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			/* Check second supplement: (2|p) = (-1)^(p^2-1)/8. */
			bn_set_dig(a, 2);
			bn_smb_leg(b, a, p);
			bn_sqr(c, p);
			bn_sub_dig(c, c, 1);
			bn_rsh(c, c, 3);
			bn_set_dig(a, 1);
			if (!bn_is_even(c)) {
				bn_neg(a, a);
			}
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			/* Check quadratic reciprocity law. */
			bn_smb_leg(a, q, p);
			bn_smb_leg(b, p, q);
			bn_sub_dig(c, p, 1);
			bn_rsh(c, c, 1);
			if (!bn_is_even(c)) {
				bn_sub_dig(c, q, 1);
				bn_rsh(c, c, 1);
				if (!bn_is_even(c)) {
					bn_neg(b, b);
				}
			}
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("jacobi symbol is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_smb_leg(c, a, p);
			bn_smb_jac(b, a, p);
			TEST_ASSERT(bn_cmp_abs(c, b) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("jacobi symbol is a homomorphism") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(b, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(p, RLC_POS, RLC_BN_BITS / 2);
			if (bn_is_even(p)) {
				bn_add_dig(p, p, 1);
			}
			bn_mul(c, a, b);
			bn_smb_jac(a, a, p);
			bn_smb_jac(b, b, p);
			bn_smb_jac(c, c, p);
			bn_mul(a, a, b);
			TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("jacobi symbol is consistent with gcd") {
			bn_rand(a, RLC_POS, RLC_BN_BITS / 2);
			bn_rand(p, RLC_POS, RLC_BN_BITS / 2);
			if (bn_is_even(p)) {
				bn_add_dig(p, p, 1);
			}
			bn_smb_jac(c, a, p);
			bn_gcd(b, a, p);
			if (bn_cmp_dig(b, 1) != RLC_EQ) {
				TEST_ASSERT(bn_is_zero(c), end);
			} else {
				bn_set_dig(a, 1);
				TEST_ASSERT(bn_cmp_abs(c, a) == RLC_EQ, end);
			}
		} TEST_END;

		TEST_CASE("jacobi symbol satisfies quadratic reciprocity") {
			bn_rand(p, RLC_POS, RLC_BN_BITS / 2);
			if (bn_is_even(p)) {
				bn_add_dig(p, p, 1);
			}
			bn_rand(q, RLC_POS, RLC_BN_BITS / 2);
			if (bn_is_even(q)) {
				bn_add_dig(q, q, 1);
			}
			/* Check the first supplement: (-1|n) = (-1)^(n-1)/2. */
			bn_set_dig(a, 1);
			bn_neg(a, a);
			bn_smb_jac(b, a, p);
			bn_sub_dig(c, p, 1);
			bn_rsh(c, c, 1);
			if (bn_is_even(c)) {
				bn_neg(a, a);
			}
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			/* Check second supplement: (2|p) = (-1)^(p^2-1)/8. */
			bn_set_dig(a, 2);
			bn_smb_jac(b, a, p);
			bn_sqr(c, p);
			bn_sub_dig(c, c, 1);
			bn_rsh(c, c, 3);
			bn_set_dig(a, 1);
			if (!bn_is_even(c)) {
				bn_neg(a, a);
			}
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			/* Check quadratic reciprocity law. */
			bn_smb_jac(a, p, q);
			bn_smb_jac(b, q, p);
			bn_sub_dig(c, p, 1);
			bn_rsh(c, c, 1);
			if (!bn_is_even(c)) {
				bn_sub_dig(c, q, 1);
				bn_rsh(c, c, 1);
				if (!bn_is_even(c)) {
					bn_neg(b, b);
				}
			}
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(p);
	bn_free(q);
	return code;
}

static int digit(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, e, f;
	dig_t g;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(e);
	bn_null(f);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(e);
		bn_new(f);

		TEST_CASE("addition of a single digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			bn_get_dig(&g, b);
			bn_add(c, a, b);
			bn_add_dig(d, a, g);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("subtraction of a single digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			bn_get_dig(&g, b);
			bn_sub(c, a, b);
			bn_sub_dig(d, a, g);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication by a single digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			bn_get_dig(&g, b);
			bn_mul(c, a, b);
			bn_mul_dig(d, a, g);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("division by a single digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			if (bn_is_zero(b)) {
				continue;
			}
			bn_div(d, a, b);
			bn_div_dig(e, a, b->dp[0]);
			TEST_ASSERT(bn_cmp(d, e) == RLC_EQ, end);
			bn_div_rem(d, c, a, b);
			bn_div_rem_dig(e, &g, a, b->dp[0]);
			TEST_ASSERT(bn_cmp(d, e) == RLC_EQ, end);
			TEST_ASSERT(bn_cmp_dig(c, g) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("modular reduction modulo a digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			if (b->dp[0] == 0)
				continue;
			bn_div_rem(d, c, a, b);
			bn_mod_dig(&g, a, b->dp[0]);
			TEST_ASSERT(bn_cmp_dig(c, g) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("greatest common divisor with a digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			bn_get_dig(&g, b);
			bn_gcd(c, a, b);
			bn_gcd_dig(e, a, g);
			TEST_ASSERT(bn_cmp(c, e) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE
				("extended greatest common divisor with a digit is consistent")
		{
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			bn_get_dig(&g, b);
			bn_gcd_ext_dig(c, d, e, a, g);
			bn_mul(d, d, a);
			bn_mul(e, e, b);
			bn_add(d, d, e);
			TEST_ASSERT(bn_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;

#if BN_MOD != PMERS
		bn_gen_prime(d, RLC_BN_BITS);
#elif BN_PRECI >= 128
		/* Let's try a Mersenne prime. */
		bn_zero(d);
		bn_set_bit(d, 127, 1);
		bn_sub_dig(d, d, 1);
#endif

		TEST_CASE("modular exponentiation with a digit is consistent") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_DIG);
			bn_get_dig(&g, b);
			bn_mxp(c, a, b, d);
			bn_mxp_dig(e, a, g, d);
			TEST_ASSERT(bn_cmp(c, e) == RLC_EQ, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(e);
	bn_free(f);
	return code;
}

static int prime(void) {
	int code = RLC_ERR;
	bn_t p;

	bn_null(p);

	RLC_TRY {
		bn_new(p);

		TEST_ONCE("prime generation is consistent") {
			bn_gen_prime(p, RLC_BN_BITS);
			TEST_ASSERT(bn_is_prime(p) == 1, end);
		} TEST_END;

#if BN_GEN == BASIC || !defined(STRIP)
		TEST_ONCE("basic prime generation is consistent") {
			bn_gen_prime_basic(p, RLC_BN_BITS);
			TEST_ASSERT(bn_is_prime(p) == 1, end);
		} TEST_END;
#endif

#if BN_GEN == SAFEP || !defined(STRIP)
		TEST_ONCE("safe prime generation is consistent") {
			bn_gen_prime_safep(p, RLC_BN_BITS);
			TEST_ASSERT(bn_is_prime(p) == 1, end);
			bn_sub_dig(p, p, 1);
			bn_hlv(p, p);
			TEST_ASSERT(bn_is_prime(p) == 1, end);
		} TEST_END;
#endif

#if BN_GEN == STRON || !defined(STRIP)
		TEST_ONCE("strong prime generation is consistent") {
			bn_gen_prime_stron(p, RLC_BN_BITS);
			TEST_ASSERT(bn_is_prime(p) == 1, end);
		} TEST_END;
#endif

		bn_gen_prime(p, RLC_BN_BITS);

		TEST_ONCE("basic prime testing is correct") {
			TEST_ASSERT(bn_is_prime_basic(p) == 1, end);
		} TEST_END;

		TEST_ONCE("miller-rabin prime testing is correct") {
			TEST_ASSERT(bn_is_prime_rabin(p) == 1, end);
		} TEST_END;

		TEST_ONCE("solovay-strassen prime testing is correct") {
			TEST_ASSERT(bn_is_prime_solov(p) == 1, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(p);
	return code;
}

static int small_primes(void) {
	int code = RLC_ERR;

	int i;
	const int nr_tests = 50;

	dig_t primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
			47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103,
			107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163,
			167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
			229};

	dig_t non_primes[] = {1, 4, 6, 8, 9, 10, 12, 14, 15, 16, 18, 20, 21, 22,
			24, 25, 26, 27, 28, 30, 32, 33, 34, 35, 36, 38, 39, 40,
			42, 44, 45, 46, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58,
			60, 62, 63, 64, 65, 66, 68, 69};

	bn_t p;
	bn_null(p);

	RLC_TRY {
		bn_new(p);

		TEST_ONCE("prime testing of small primes is correct") {
			for(i = 0; i < nr_tests; i++) {
				bn_set_dig(p, primes[i]);
				TEST_ASSERT(bn_is_prime(p) == 1, end);
			}
		} TEST_END;

		TEST_ONCE("miller-rabin testing of small primes is correct") {
			for(i = 0; i < nr_tests; i++) {
				bn_set_dig(p, primes[i]);
				TEST_ASSERT(bn_is_prime_rabin(p) == 1, end);
			}
		} TEST_END;

		TEST_ONCE("prime testing of small non-primes is correct") {
			for(i = 0; i < nr_tests; i++) {
				bn_set_dig(p, non_primes[i]);
				TEST_ASSERT(bn_is_prime(p) == 0, end);
			}
		} TEST_END;

		TEST_ONCE("miller-rabin testing of small non-primes is correct") {
			for(i = 0; i < nr_tests; i++) {
				bn_set_dig(p, non_primes[i]);
				TEST_ASSERT(bn_is_prime_rabin(p) == 0, end);
			}
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(p);
	return code;
}

static int inversion(void) {
	int code = RLC_ERR;
	bn_t a, b, c;

	bn_null(a);
	bn_null(b);
	bn_null(c);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);

		bn_gen_prime(a, RLC_BN_BITS);

		TEST_CASE("modular inversion is correct") {
			bn_rand_mod(b, a);
			bn_mod_inv(c, b, a);
			TEST_ASSERT(bn_cmp_dig(c, 1) != RLC_EQ, end);
			bn_mul(c, b, c);
			bn_mod(c, c, a);
			TEST_ASSERT(bn_cmp_dig(c, 1) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	return code;
}

static int factor(void) {
	int code = RLC_ERR;
	bn_t p, q, n;

	bn_null(p);
	bn_null(q);
	bn_null(n);

	RLC_TRY {
		bn_new(p);
		bn_new(q);
		bn_new(n);

		TEST_ONCE("integer factorization is consistent") {
			bn_gen_prime(p, 16);
			bn_rand(n, RLC_POS, RLC_BN_BITS - 16);
			bn_mul(n, n, p);
			if (bn_factor(q, n) == 1) {
				TEST_ASSERT(bn_is_factor(q, n) == 1, end);
			} else {
				TEST_ASSERT(bn_is_factor(p, n) == 1, end);
			}
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(p);
	bn_free(q);
	bn_free(n);
	return code;
}

static int recoding(void) {
	int code = RLC_ERR;
	bn_t a, b, c, v1[3], v2[3];
	int w, k, l;
	uint8_t d[RLC_BN_BITS + 1];
	int8_t e[2 * (RLC_BN_BITS + 1)];

	bn_null(a);
	bn_null(b);
	bn_null(c);
	for (k = 0; k < 3; k++) {
		bn_null(v1[k]);
		bn_null(v2[k]);
	}

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		for (k = 0; k < 3; k++) {
			bn_new(v1[k]);
			bn_new(v2[k]);
		}

		TEST_CASE("window recoding is correct") {
			for (w = 2; w <= 8; w++) {
				bn_rand(a, RLC_POS, RLC_BN_BITS);
				l = RLC_BN_BITS + 1;
				bn_rec_win(d, &l, a, w);
				bn_zero(b);
				for (k = l - 1; k >= 0; k--) {
					bn_lsh(b, b, w);
					bn_add_dig(b, b, d[k]);
				}
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
		} TEST_END;

		TEST_CASE("sliding window recoding is correct") {
			for (w = 2; w <= 8; w++) {
				bn_rand(a, RLC_POS, RLC_BN_BITS);
				l = RLC_BN_BITS + 1;
				bn_rec_slw(d, &l, a, w);
				bn_zero(b);
				for (k = 0; k < l; k++) {
					if (d[k] == 0) {
						bn_dbl(b, b);
					} else {
						bn_lsh(b, b, util_bits_dig(d[k]));
						bn_add_dig(b, b, d[k]);
					}
				}
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
		} TEST_END;

		TEST_CASE("naf recoding is correct") {
			for (w = 2; w <= 8; w++) {
				bn_rand(a, RLC_POS, RLC_BN_BITS);
				l = RLC_BN_BITS + 1;
				bn_rec_naf(e, &l, a, w);
				bn_zero(b);
				for (k = l - 1; k >= 0; k--) {
					bn_dbl(b, b);
					if (e[k] >= 0) {
						bn_add_dig(b, b, e[k]);
					} else {
						bn_sub_dig(b, b, -e[k]);
					}
				}
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
		} TEST_END;

#if defined(WITH_EB) && defined(EB_KBLTZ) && (EB_MUL == LWNAF || EB_MUL == RWNAF || EB_FIX == LWNAF || EB_SIM == INTER || !defined(STRIP))
		if (eb_param_set_any_kbltz() == RLC_OK) {
			eb_curve_get_ord(v1[2]);
			TEST_CASE("tnaf recoding is correct") {
				for (w = 2; w <= 8; w++) {
					uint8_t t_w;
					int8_t beta[64], gama[64];
					int8_t tnaf[RLC_FB_BITS + 8];
					int8_t u = (eb_curve_opt_a() == RLC_ZERO ? -1 : 1);
					bn_rand_mod(a, v1[2]);
					l = RLC_FB_BITS + 1;
					bn_rec_tnaf_mod(v1[0], v1[1], a, u, RLC_FB_BITS);
					bn_rec_tnaf_get(&t_w, beta, gama, u, w);
					bn_rec_tnaf(tnaf, &l, a, u, RLC_FB_BITS, w);
					bn_zero(a);
					bn_zero(b);
					for (k = l - 1; k >= 0; k--) {
						bn_copy(c, b);
						if (u == -1) {
							bn_neg(c, c);
						}
						bn_add(c, c, a);
						bn_dbl(a, b);
						bn_neg(a, a);
						bn_copy(b, c);
						if (w == 2) {
							if (tnaf[k] >= 0) {
								bn_add_dig(a, a, tnaf[k]);
							} else {
								bn_sub_dig(a, a, -tnaf[k]);
							}
						} else {
							if (tnaf[k] > 0) {
								if (beta[tnaf[k] / 2] >= 0) {
									bn_add_dig(a, a, beta[tnaf[k] / 2]);
								} else {
									bn_sub_dig(a, a, -beta[tnaf[k] / 2]);
								}
								if (gama[tnaf[k] / 2] >= 0) {
									bn_add_dig(b, b, gama[tnaf[k] / 2]);
								} else {
									bn_sub_dig(b, b, -gama[tnaf[k] / 2]);
								}
							}
							if (tnaf[k] < 0) {
								if (beta[-tnaf[k] / 2] >= 0) {
									bn_sub_dig(a, a, beta[-tnaf[k] / 2]);
								} else {
									bn_add_dig(a, a, -beta[-tnaf[k] / 2]);
								}
								if (gama[-tnaf[k] / 2] >= 0) {
									bn_sub_dig(b, b, gama[-tnaf[k] / 2]);
								} else {
									bn_add_dig(b, b, -gama[-tnaf[k] / 2]);
								}
							}
						}
					}
					TEST_ASSERT(bn_cmp(a, v1[0]) == RLC_EQ, end);
					TEST_ASSERT(bn_cmp(b, v1[1]) == RLC_EQ, end);
				}
			}
			TEST_END;

			TEST_CASE("regular tnaf recoding is correct") {
				for (w = 2; w <= 8; w++) {
					uint8_t t_w;
					int8_t beta[64], gama[64];
					int8_t tnaf[RLC_FB_BITS + 8];
					int8_t u = (eb_curve_opt_a() == RLC_ZERO ? -1 : 1);
					int n;
					do {
						bn_rand_mod(a, v1[2]);
						l = RLC_FB_BITS + 1;
						bn_rec_tnaf_mod(v1[0], v1[1], a, u, RLC_FB_BITS);
					} while (bn_is_even(v1[0]) || bn_is_even(v1[1]));
					bn_rec_tnaf_get(&t_w, beta, gama, u, w);
					bn_rec_rtnaf(tnaf, &l, a, u, RLC_FB_BITS, w);
					bn_zero(a);
					bn_zero(b);
					n = 0;
					for (k = l - 1; k >= 0; k--) {
						for (int m = 0; m < w - 1; m++) {
							bn_copy(c, b);
							if (u == -1) {
								bn_neg(c, c);
							}
							bn_add(c, c, a);
							bn_dbl(a, b);
							bn_neg(a, a);
							bn_copy(b, c);
						}
						if (tnaf[k] != 0) {
							n++;
						}
						if (w == 2) {
							if (tnaf[k] >= 0) {
								bn_add_dig(a, a, tnaf[k]);
							} else {
								bn_sub_dig(a, a, -tnaf[k]);
							}
						} else {
							if (tnaf[k] > 0) {
								if (beta[tnaf[k] / 2] >= 0) {
									bn_add_dig(a, a, beta[tnaf[k] / 2]);
								} else {
									bn_sub_dig(a, a, -beta[tnaf[k] / 2]);
								}
								if (gama[tnaf[k] / 2] >= 0) {
									bn_add_dig(b, b, gama[tnaf[k] / 2]);
								} else {
									bn_sub_dig(b, b, -gama[tnaf[k] / 2]);
								}
							}
							if (tnaf[k] < 0) {
								if (beta[-tnaf[k] / 2] >= 0) {
									bn_sub_dig(a, a, beta[-tnaf[k] / 2]);
								} else {
									bn_add_dig(a, a, -beta[-tnaf[k] / 2]);
								}
								if (gama[-tnaf[k] / 2] >= 0) {
									bn_sub_dig(b, b, gama[-tnaf[k] / 2]);
								} else {
									bn_add_dig(b, b, -gama[-tnaf[k] / 2]);
								}
							}
						}
					}
					TEST_ASSERT(bn_cmp(a, v1[0]) == RLC_EQ, end);
					TEST_ASSERT(bn_cmp(b, v1[1]) == RLC_EQ, end);
				}
			} TEST_END;
		}
#endif

		TEST_CASE("regular recoding is correct") {
			/* Recode same scalar with different widths. */
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			if (bn_is_even(a)) {
				bn_add_dig(a, a, 1);
			}
			for (w = 2; w <= 8; w++) {
				l = RLC_BN_BITS + 1;
				bn_rec_reg(e, &l, a, RLC_BN_BITS, w);
				bn_zero(b);
				for (k = l - 1; k >= 0; k--) {
					bn_lsh(b, b, w - 1);
					if (e[k] > 0) {
						bn_add_dig(b, b, e[k]);
					} else {
						bn_sub_dig(b, b, -e[k]);
					}
				}
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
		} TEST_END;

		TEST_CASE("jsf recoding is correct") {
			bn_rand(a, RLC_POS, RLC_BN_BITS);
			bn_rand(b, RLC_POS, RLC_BN_BITS);
			l = 2 * (RLC_BN_BITS + 1);
			bn_rec_jsf(e, &l, a, b);
			w = RLC_MAX(bn_bits(a), bn_bits(b)) + 1;
			bn_add(a, a, b);
			bn_zero(b);
			for (k = l - 1; k >= 0; k--) {
				bn_dbl(b, b);
				if (e[k] >= 0) {
					bn_add_dig(b, b, e[k]);
				} else {
					bn_sub_dig(b, b, -e[k]);
				}
				if (e[k + w] >= 0) {
					bn_add_dig(b, b, e[k + w]);
				} else {
					bn_sub_dig(b, b, -e[k + w]);
				}
			}
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

#if defined(WITH_EP) && defined(EP_ENDOM) && (EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || EP_SIM == INTER || !defined(STRIP))
		TEST_CASE("glv recoding is correct") {
			if (ep_param_set_any_endom() == RLC_OK) {
				ep_curve_get_v1(v1);
				ep_curve_get_v2(v2);
				ep_curve_get_ord(b);
				bn_rand_mod(a, b);
				bn_rec_glv(b, c, a, b, (const bn_t *)v1, (const bn_t *)v2);
				ep_curve_get_ord(v2[0]);
				/* Check that subscalars have the right length. */
				TEST_ASSERT(bn_bits(b) <= 1 + (bn_bits(v2[0]) >> 1), end);
				TEST_ASSERT(bn_bits(c) <= 1 + (bn_bits(v2[0]) >> 1), end);
				/* Recover lambda parameter. */
				if (bn_cmp_dig(v1[2], 1) == RLC_EQ) {
					bn_gcd_ext(v1[0], v2[1], NULL, v1[1], v2[0]);
				} else {
					bn_gcd_ext(v1[0], v2[1], NULL, v1[2], v2[0]);
				}
				if (bn_sign(v2[1]) == RLC_NEG) {
					/* Negate modulo r. */
					bn_add(v2[1], v2[0], v2[1]);
				}
				if (bn_cmp_dig(v1[2], 1) == RLC_EQ) {
					bn_sub(v1[0], v2[1], v1[2]);
				} else {
					bn_mul(v1[0], v2[1], v1[1]);
				}
				bn_mod(v1[0], v1[0], v2[0]);
				bn_sub(v1[1], v2[0], v1[0]);
				if (bn_cmp(v1[1], v1[0]) == RLC_LT) {
					bn_copy(v1[0], v1[1]);
				}
				/* Check if b + c * lambda = k (mod n). */
				bn_mul(c, c, v1[0]);
				bn_add(b, b, c);
				bn_mod(b, b, v2[0]);
				if (bn_sign(b) == RLC_NEG) {
					bn_add(b, b, v2[0]);
				}
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}
		} TEST_END;
#endif /* WITH_EP && EP_ENDOM */
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	for (k = 0; k < 3; k++) {
		bn_free(v1[k]);
		bn_free(v2[k]);
	}
	return code;
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the BN module", 0);
	util_banner("Utilities:", 1);

	if (memory() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (util() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Arithmetic:", 1);

	if (addition() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (subtraction() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (multiplication() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (squaring() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (doubling_halving() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (shifting() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (division() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (reduction() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (square_root() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (gcd() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (lcm() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (symbol() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (digit() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (recoding() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (exponentiation() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (prime() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (small_primes() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (inversion() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (factor() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
