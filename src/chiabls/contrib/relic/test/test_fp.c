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
 * Tests for prime field arithmetic.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"
#include "relic_fp_low.h"

static int memory(void) {
	err_t e;
	int code = STS_ERR;
	fp_t a;

	fp_null(a);

	TRY {
		TEST_BEGIN("memory can be allocated") {
			fp_new(a);
			fp_free(a);
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

static int util(void) {
	int bits, code = STS_ERR;
	char str[FP_BITS + 1];
	uint8_t bin[FP_BYTES];
	fp_t a, b, c;
	bn_t d;
	dig_t e;

	fp_null(a);
	fp_null(b);
	fp_null(c);
	bn_null(d);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);
		bn_new(d);

		TEST_BEGIN("comparison is consistent") {
			fp_rand(a);
			fp_rand(b);
			if (fp_cmp(a, b) != CMP_EQ) {
				if (fp_cmp(a, b) == CMP_GT) {
					TEST_ASSERT(fp_cmp(b, a) == CMP_LT, end);
				} else {
					TEST_ASSERT(fp_cmp(b, a) == CMP_GT, end);
				}
			}
		}
		TEST_END;

		TEST_BEGIN("copy and comparison are consistent") {
			fp_rand(a);
			fp_rand(c);
			fp_rand(b);
			if (fp_cmp(a, c) != CMP_EQ) {
				fp_copy(c, a);
				TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
			}
			if (fp_cmp(b, c) != CMP_EQ) {
				fp_copy(c, b);
				TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
			}
		}
		TEST_END;

		TEST_BEGIN("negation is consistent") {
			fp_rand(a);
			fp_neg(b, a);
			if (fp_cmp(a, b) != CMP_EQ) {
				if (fp_cmp(a, b) == CMP_GT) {
					TEST_ASSERT(fp_cmp(b, a) == CMP_LT, end);
				} else {
					TEST_ASSERT(fp_cmp(b, a) == CMP_GT, end);
				}
			}
			fp_neg(b, b);
			TEST_ASSERT(fp_cmp(a, b) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("assignment to zero and comparison are consistent") {
			do {
				fp_rand(a);
			} while (fp_is_zero(a));
			fp_zero(c);
			TEST_ASSERT(fp_cmp(a, c) == CMP_GT, end);
			TEST_ASSERT(fp_cmp(c, a) == CMP_LT, end);
		}
		TEST_END;

		TEST_BEGIN("assignment to random and comparison are consistent") {
			do {
				fp_rand(a);
				fp_rand(b);
			} while (fp_is_zero(a) || fp_is_zero(b));
			fp_zero(c);
			TEST_ASSERT(fp_cmp(a, c) == CMP_GT, end);
			TEST_ASSERT(fp_cmp(b, c) == CMP_GT, end);
			if (fp_cmp(a, b) != CMP_EQ) {
				if (fp_cmp(a, b) == CMP_GT) {
					TEST_ASSERT(fp_cmp(b, a) == CMP_LT, end);
				} else {
					TEST_ASSERT(fp_cmp(b, a) == CMP_GT, end);
				}
			}
		}
		TEST_END;

		TEST_BEGIN("assignment to zero and zero test are consistent") {
			fp_zero(a);
			TEST_ASSERT(fp_is_zero(a), end);
		}
		TEST_END;

		TEST_BEGIN("assignment to a constant and comparison are consistent") {
			rand_bytes((uint8_t *)&e, (FP_DIGIT / 8));
			fp_set_dig(a, e);
			TEST_ASSERT(fp_cmp_dig(a, e) == CMP_EQ, end);
		}
		TEST_END;

		bits = 0;
		TEST_BEGIN("bit setting and getting are consistent") {
			fp_zero(a);
			fp_set_bit(a, bits, 1);
			TEST_ASSERT(fp_get_bit(a, bits) == 1, end);
			fp_set_bit(a, bits, 0);
			TEST_ASSERT(fp_get_bit(a, bits) == 0, end);
			bits = (bits + 1) % RELIC_BN_BITS;
		}
		TEST_END;

		bits = 0;
		TEST_BEGIN("bit assignment and counting are consistent") {
			fp_zero(a);
			fp_set_bit(a, bits, 1);
			TEST_ASSERT(fp_bits(a) == bits + 1, end);
			bits = (bits + 1) % FP_BITS;
		}
		TEST_END;

		TEST_BEGIN("reading and writing a prime field element are consistent") {
			fp_rand(a);
			for (int j = 2; j <= 64; j++) {
				bits = fp_size_str(a, j);
				fp_write_str(str, bits, a, j);
				fp_read_str(b, str, bits, j);
				TEST_ASSERT(fp_cmp(a, b) == CMP_EQ, end);
			}
			fp_write_bin(bin, sizeof(bin), a);
			fp_read_bin(b, bin, sizeof(bin));
			TEST_ASSERT(fp_cmp(a, b) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("getting the size of a prime field element is correct") {
			fp_rand(a);
			fp_prime_back(d, a);
			TEST_ASSERT((fp_size_str(a, 2) - 1) == bn_bits(d), end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	bn_free(d);
	return code;
}

static int addition(void) {
	int code = STS_ERR;
	fp_t a, b, c, d, e;

	fp_null(a);
	fp_null(b);
	fp_null(c);
	fp_null(d);
	fp_null(e);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);
		fp_new(d);
		fp_new(e);

		TEST_BEGIN("addition is commutative") {
			fp_rand(a);
			fp_rand(b);
			fp_add(d, a, b);
			fp_add(e, b, a);
			TEST_ASSERT(fp_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("addition is associative") {
			fp_rand(a);
			fp_rand(b);
			fp_rand(c);
			fp_add(d, a, b);
			fp_add(d, d, c);
			fp_add(e, b, c);
			fp_add(e, a, e);
			TEST_ASSERT(fp_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("addition has identity") {
			fp_rand(a);
			fp_zero(d);
			fp_add(e, a, d);
			TEST_ASSERT(fp_cmp(e, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("addition has inverse") {
			fp_rand(a);
			fp_neg(d, a);
			fp_add(e, a, d);
			TEST_ASSERT(fp_is_zero(e), end);
		} TEST_END;

#if FP_ADD == BASIC || !defined(STRIP)
		TEST_BEGIN("basic addition is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_add(d, a, b);
			fp_add_basic(e, a, b);
			TEST_ASSERT(fp_cmp(d, e) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
		TEST_BEGIN("integrated addition is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_add(d, a, b);
			fp_add_integ(e, a, b);
			TEST_ASSERT(fp_cmp(d, e) == CMP_EQ, end);
		} TEST_END;
#endif
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	fp_free(d);
	fp_free(e);
	return code;
}

static int subtraction(void) {
	int code = STS_ERR;
	fp_t a, b, c, d;

	fp_null(a);
	fp_null(b);
	fp_null(c);
	fp_null(d);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);
		fp_new(d);

		TEST_BEGIN("subtraction is anti-commutative") {
			fp_rand(a);
			fp_rand(b);
			fp_sub(c, a, b);
			fp_sub(d, b, a);
			fp_neg(d, d);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("subtraction has identity") {
			fp_rand(a);
			fp_zero(c);
			fp_sub(d, a, c);
			TEST_ASSERT(fp_cmp(d, a) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("subtraction has inverse") {
			fp_rand(a);
			fp_sub(c, a, a);
			TEST_ASSERT(fp_is_zero(c), end);
		}
		TEST_END;

#if FP_ADD == BASIC || !defined(STRIP)
		TEST_BEGIN("basic subtraction is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_sub(c, a, b);
			fp_sub_basic(d, a, b);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
		TEST_BEGIN("integrated subtraction is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_sub(c, a, b);
			fp_sub_integ(d, a, b);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_ADD == BASIC || !defined(STRIP)
		TEST_BEGIN("basic negation is correct") {
			fp_rand(a);
			fp_neg(c, a);
			fp_neg_basic(d, a);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
		TEST_BEGIN("integrated negation is correct") {
			fp_rand(a);
			fp_neg(c, a);
			fp_neg_integ(d, a);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		} TEST_END;
#endif
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	fp_free(d);
	return code;
}

static int multiplication(void) {
	int code = STS_ERR;
	fp_t a, b, c, d, e, f;

	fp_null(a);
	fp_null(b);
	fp_null(c);
	fp_null(d);
	fp_null(e);
	fp_null(f);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);
		fp_new(d);
		fp_new(e);
		fp_new(f);

		TEST_BEGIN("multiplication is commutative") {
			fp_rand(a);
			fp_rand(b);
			fp_mul(d, a, b);
			fp_mul(e, b, a);
			TEST_ASSERT(fp_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication is associative") {
			fp_rand(a);
			fp_rand(b);
			fp_rand(c);
			fp_mul(d, a, b);
			fp_mul(d, d, c);
			fp_mul(e, b, c);
			fp_mul(e, a, e);
			TEST_ASSERT(fp_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication is distributive") {
			fp_rand(a);
			fp_rand(b);
			fp_rand(c);
			fp_add(d, a, b);
			fp_mul(d, c, d);
			fp_mul(e, c, a);
			fp_mul(f, c, b);
			fp_add(e, e, f);
			TEST_ASSERT(fp_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication has identity") {
			fp_rand(a);
			fp_set_dig(d, 1);
			fp_mul(e, a, d);
			TEST_ASSERT(fp_cmp(e, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication has zero property") {
			fp_rand(a);
			fp_zero(d);
			fp_mul(e, a, d);
			TEST_ASSERT(fp_is_zero(e), end);
		} TEST_END;

#if FP_MUL == BASIC || !defined(STRIP)
		TEST_BEGIN("basic multiplication is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_mul(c, a, b);
			fp_mul_basic(d, a, b);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		}
		TEST_END;
#endif

#if FP_MUL == INTEG || !defined(STRIP)
		TEST_BEGIN("integrated multiplication is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_mul(c, a, b);
			fp_mul_integ(d, a, b);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		}
		TEST_END;
#endif

#if FP_MUL == COMBA || !defined(STRIP)
		TEST_BEGIN("comba multiplication is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_mul(c, a, b);
			fp_mul_comba(d, a, b);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		}
		TEST_END;
#endif

#if FP_KARAT > 0 || !defined(STRIP)
		TEST_BEGIN("karatsuba multiplication is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_mul(c, a, b);
			fp_mul_karat(d, a, b);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		}
		TEST_END;
#endif
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	fp_free(d);
	fp_free(e);
	fp_free(f);
	return code;
}

static int squaring(void) {
	int code = STS_ERR;
	fp_t a, b, c;

	fp_null(a);
	fp_null(b);
	fp_null(c);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);

		TEST_BEGIN("squaring is correct") {
			fp_rand(a);
			fp_mul(b, a, a);
			fp_sqr(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;

#if FP_SQR == BASIC || !defined(STRIP)
		TEST_BEGIN("basic squaring is correct") {
			fp_rand(a);
			fp_sqr(b, a);
			fp_sqr_basic(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_SQR == INTEG || !defined(STRIP)
		TEST_BEGIN("integrated squaring is correct") {
			fp_rand(a);
			fp_sqr(b, a);
			fp_sqr_integ(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_SQR == COMBA || !defined(STRIP)
		TEST_BEGIN("comba squaring is correct") {
			fp_rand(a);
			fp_sqr(b, a);
			fp_sqr_comba(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_KARAT > 0 || !defined(STRIP)
		TEST_BEGIN("karatsuba squaring is correct") {
			fp_rand(a);
			fp_sqr(b, a);
			fp_sqr_karat(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	return code;
}

static int doubling_halving(void) {
	int code = STS_ERR;
	fp_t a, b, c;

	fp_null(a);
	fp_null(b);
	fp_null(c);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);

		TEST_BEGIN("doubling is consistent") {
			fp_rand(a);
			fp_add(b, a, a);
			fp_dbl(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;

#if FP_ADD == BASIC || !defined(STRIP)
		TEST_BEGIN("basic doubling is correct") {
			fp_rand(a);
			fp_dbl(b, a);
			fp_dbl_basic(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
		TEST_BEGIN("integrated doubling is correct") {
			fp_rand(a);
			fp_dbl(b, a);
			fp_dbl_integ(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif

		TEST_BEGIN("halving is consistent") {
			fp_rand(a);
			fp_dbl(b, a);
			fp_hlv(c, b);
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
		}
		TEST_END;

#if FP_ADD == BASIC || !defined(STRIP)
		TEST_BEGIN("basic halving is correct") {
			fp_rand(a);
			fp_hlv(b, a);
			fp_hlv_basic(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
		TEST_BEGIN("integrated halving is correct") {
			fp_rand(a);
			fp_hlv(b, a);
			fp_hlv_integ(c, a);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
#endif
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	return code;
}

static int shifting(void) {
	int code = STS_ERR;
	fp_t a, b, c;

	fp_null(a);
	fp_null(b);
	fp_null(c);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);

		TEST_BEGIN("shifting by 1 bit is consistent") {
			fp_rand(a);
			a[FP_DIGS - 1] = 0;
			fp_lsh(b, a, 1);
			fp_rsh(c, b, 1);
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("shifting by 2 bits is consistent") {
			fp_rand(a);
			a[FP_DIGS - 1] = 0;
			fp_lsh(b, a, 2);
			fp_rsh(c, b, 2);
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("shifting by half digit is consistent") {
			fp_rand(a);
			a[FP_DIGS - 1] = 0;
			fp_lsh(b, a, FP_DIGIT / 2);
			fp_rsh(c, b, FP_DIGIT / 2);
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("shifting by 1 digit is consistent") {
			fp_rand(a);
			a[FP_DIGS - 1] = 0;
			fp_lsh(b, a, FP_DIGIT);
			fp_rsh(c, b, FP_DIGIT);
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("shifting by 2 digits is consistent") {
			fp_rand(a);
			a[FP_DIGS - 1] = 0;
			a[FP_DIGS - 2] = 0;
			fp_lsh(b, a, 2 * FP_DIGIT);
			fp_rsh(c, b, 2 * FP_DIGIT);
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("shifting by 1 digit and half is consistent") {
			fp_rand(a);
			a[FP_DIGS - 1] = 0;
			a[FP_DIGS - 2] = 0;
			fp_lsh(b, a, FP_DIGIT + FP_DIGIT / 2);
			fp_rsh(c, b, (FP_DIGIT + FP_DIGIT / 2));
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	return code;
}

static int reduction(void) {
	int code = STS_ERR;
	fp_t a, b;
	dv_t t;

	fp_null(a);
	fp_null(b);
	dv_null(t);

	TRY {
		fp_new(a);
		fp_new(b);
		dv_new(t);
		dv_zero(t, 2 * FP_DIGS);

		TEST_BEGIN("modular reduction is correct") {
			fp_rand(a);
			dv_copy(t, fp_prime_get(), FP_DIGS);
			/* Test if a * p mod p == 0. */
			fp_mul(b, a, t);
			TEST_ASSERT(fp_is_zero(b) == 1, end);
		} TEST_END;

#if FP_RDC == BASIC || !defined(STRIP)
		TEST_BEGIN("basic modular reduction is correct") {
			fp_rand(a);
			fp_muln_low(t, a, fp_prime_get());
			fp_rdc_basic(b, t);
			TEST_ASSERT(fp_is_zero(b), end);
		} TEST_END;
#endif

#if FP_RDC == MONTY || !defined(STRIP)
		TEST_BEGIN("montgomery modular reduction is correct") {
			fp_rand(a);
			fp_muln_low(t, a, fp_prime_get());
			fp_rdc_monty(b, t);
			TEST_ASSERT(fp_is_zero(b), end);
		} TEST_END;
#endif

#if FP_RDC == QUICK || !defined(STRIP)
		if (fp_prime_get_sps(NULL) != NULL) {
			TEST_BEGIN("fast modular reduction is correct") {
				fp_rand(a);
				fp_muln_low(t, a, fp_prime_get());
				fp_rdc_quick(b, t);
				TEST_ASSERT(fp_is_zero(b), end);
			}
			TEST_END;
		}
#endif
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb_free(a);
	fb_free(b);
	dv_free(t);
	return code;
}

static int inversion(void) {
	int code = STS_ERR;
	fp_t a, b, c, d[2];

	fp_null(a);
	fp_null(b);
	fp_null(c);
	fp_null(d[0]);
	fp_null(d[1]);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);
		fp_new(d[0]);
		fp_new(d[1]);

		TEST_BEGIN("inversion is correct") {
			fp_rand(a);
			fp_inv(b, a);
			fp_mul(c, a, b);
			fp_set_dig(b, 1);
			TEST_ASSERT(fp_cmp(c, b) == CMP_EQ, end);
		} TEST_END;

#if FP_INV == BASIC || !defined(STRIP)
		TEST_BEGIN("basic inversion is correct") {
			fp_rand(a);
			fp_inv(b, a);
			fp_inv_basic(c, a);
			TEST_ASSERT(fp_cmp(c, b) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_INV == BINAR || !defined(STRIP)
		TEST_BEGIN("binary inversion is correct") {
			fp_rand(a);
			fp_inv(b, a);
			fp_inv_binar(c, a);
			TEST_ASSERT(fp_cmp(c, b) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_INV == MONTY || !defined(STRIP)
		TEST_BEGIN("montgomery inversion is correct") {
			fp_rand(a);
			fp_inv(b, a);
			fp_inv_monty(c, a);
			TEST_ASSERT(fp_cmp(c, b) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_INV == EXGCD || !defined(STRIP)
		TEST_BEGIN("euclidean inversion is correct") {
			fp_rand(a);
			fp_inv(b, a);
			fp_inv_exgcd(c, a);
			TEST_ASSERT(fp_cmp(c, b) == CMP_EQ, end);
		} TEST_END;
#endif

#if FP_INV == LOWER || !defined(STRIP)
		TEST_BEGIN("lower inversion is correct") {
			fp_rand(a);
			fp_inv(b, a);
			fp_inv_lower(c, a);
			TEST_ASSERT(fp_cmp(c, b) == CMP_EQ, end);
		} TEST_END;
#endif

		TEST_BEGIN("simultaneous inversion is correct") {
			fp_rand(a);
			fp_rand(b);
			fp_copy(d[0], a);
			fp_copy(d[1], b);
			fp_inv(a, a);
			fp_inv(b, b);
			fp_inv_sim(d, (const fp_t *)d, 2);
			TEST_ASSERT(fp_cmp(d[0], a) == CMP_EQ &&
					fp_cmp(d[1], b) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	fp_free(d[0]);
	fp_free(d[1]);
	return code;
}

static int exponentiation(void) {
	int code = STS_ERR;
	fp_t a, b, c;
	bn_t d;

	fp_null(a);
	fp_null(b);
	fp_null(c);
	bn_null(d);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);
		bn_new(d);

		TEST_BEGIN("exponentiation is correct") {
			fp_rand(a);
			bn_zero(d);
			fp_exp(c, a, d);
			TEST_ASSERT(fp_cmp_dig(c, 1) == CMP_EQ, end);
			bn_set_dig(d, 1);
			fp_exp(c, a, d);
			TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
			bn_rand(d, BN_POS, FP_BITS);
			fp_exp(b, a, d);
			bn_neg(d, d);
			fp_exp(c, a, d);
			fp_inv(c, c);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
			d->sign = BN_POS;
			d->used = FP_DIGS;
			dv_copy(d->dp, fp_prime_get(), FP_DIGS);
			fp_exp(c, a, d);
			TEST_ASSERT(fp_cmp(a, c) == CMP_EQ, end);
		}
		TEST_END;

#if FP_EXP == BASIC || !defined(STRIP)
		TEST_BEGIN("basic exponentiation is correct") {
			fp_rand(a);
			bn_rand(d, BN_POS, FP_BITS);
			fp_exp(c, a, d);
			fp_exp_basic(b, a, d);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		}
		TEST_END;
#endif

#if FP_EXP == SLIDE || !defined(STRIP)
		TEST_BEGIN("sliding window exponentiation is correct") {
			fp_rand(a);
			bn_rand(d, BN_POS, FP_BITS);
			fp_exp(c, a, d);
			fp_exp_slide(b, a, d);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		}
		TEST_END;
#endif

#if FP_EXP == MONTY || !defined(STRIP)
		TEST_BEGIN("constant-time exponentiation is correct") {
			fp_rand(a);
			bn_rand(d, BN_POS, FP_BITS);
			fp_exp(c, a, d);
			fp_exp_monty(b, a, d);
			TEST_ASSERT(fp_cmp(b, c) == CMP_EQ, end);
		}
		TEST_END;
#endif
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	bn_free(d);
	return code;
}

static int square_root(void) {
	int code = STS_ERR;
	fp_t a, b, c;

	fp_null(a);
	fp_null(b);
	fp_null(c);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);

		TEST_BEGIN("square root extraction is correct") {
			fp_rand(a);
			fp_sqr(c, a);
			TEST_ASSERT(fp_srt(b, c), end);
			fp_neg(c, b);
			TEST_ASSERT(fp_cmp(b, a) == CMP_EQ || fp_cmp(c, a) == CMP_EQ, end);
			fp_rand(a);
			if (fp_srt(b, a)) {
				fp_sqr(c, b);
				TEST_ASSERT(fp_cmp(c, a) == CMP_EQ, end);
			}
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	return code;
}

static int digit(void) {
	int code = STS_ERR;
	fp_t a, b, c, d;
	dig_t g;

	fp_null(a);
	fp_null(b);
	fp_null(c);
	fp_null(d);

	TRY {
		fp_new(a);
		fp_new(b);
		fp_new(c);
		fp_new(d);

		TEST_BEGIN("addition of a single digit is consistent") {
			fp_rand(a);
			fp_rand(b);
			for (int j = 1; j < FP_DIGS; j++)
				b[j] = 0;
			g = b[0];
			fp_set_dig(b, g);
			fp_add(c, a, b);
			fp_add_dig(d, a, g);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("subtraction of a single digit is consistent") {
			fp_rand(a);
			fp_rand(b);
			for (int j = 1; j < FP_DIGS; j++)
				b[j] = 0;
			g = b[0];
			fp_set_dig(b, g);
			fp_sub(c, a, b);
			fp_sub_dig(d, a, g);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication by a single digit is consistent") {
			fp_rand(a);
			fp_rand(b);
			for (int j = 1; j < FP_DIGS; j++)
				b[j] = 0;
			g = b[0];
			fp_set_dig(b, g);
			fp_mul(c, a, b);
			fp_mul_dig(d, a, g);
			TEST_ASSERT(fp_cmp(c, d) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fp_free(a);
	fp_free(b);
	fp_free(c);
	fp_free(d);
	return code;
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the FP module", 0);

	TRY {
		fp_param_set_any();
		fp_param_print();
	} CATCH_ANY {
		core_clean();
		return 0;
	}

	util_banner("Utilities", 1);
	if (memory() != STS_OK) {
		core_clean();
		return 1;
	}

	if (util() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Arithmetic", 1);
	if (addition() != STS_OK) {
		core_clean();
		return 1;
	}

	if (subtraction() != STS_OK) {
		core_clean();
		return 1;
	}

	if (multiplication() != STS_OK) {
		core_clean();
		return 1;
	}

	if (squaring() != STS_OK) {
		core_clean();
		return 1;
	}

	if (doubling_halving() != STS_OK) {
		core_clean();
		return 1;
	}

	if (shifting() != STS_OK) {
		core_clean();
		return 1;
	}

	if (reduction() != STS_OK) {
		core_clean();
		return 1;
	}

	if (inversion() != STS_OK) {
		core_clean();
		return 1;
	}

	if (exponentiation() != STS_OK) {
		core_clean();
		return 1;
	}

	if (square_root() != STS_OK) {
		core_clean();
		return 1;
	}

	if (digit() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
