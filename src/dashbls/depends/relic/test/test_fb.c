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
 * Tests for binary field arithmetic.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_fb_low.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = RLC_ERR;
	fb_t a;

	fb_null(a);

	RLC_TRY {
		TEST_CASE("memory can be allocated") {
			fb_new(a);
			fb_free(a);
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
	int bits, code = RLC_ERR;
	fb_t a, b;
	char str[RLC_FB_BITS + 1];
	uint8_t bin[RLC_FB_BYTES];
	dig_t d;

	fb_null(a);
	fb_null(b);

	RLC_TRY {
		fb_new(a);
		fb_new(b);

		TEST_CASE("copy and comparison are consistent") {
			fb_rand(a);
			fb_rand(b);
			if (fb_cmp(a, b) != RLC_EQ) {
				fb_copy(b, a);
				TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
			}
		}
		TEST_END;

		TEST_CASE("assignment and comparison are consistent") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_zero(b);
			TEST_ASSERT(fb_cmp(a, b) == RLC_NE, end);
			TEST_ASSERT(fb_cmp(b, a) == RLC_NE, end);
			TEST_ASSERT(fb_is_zero(b), end);
			rand_bytes((uint8_t *)&d, (RLC_DIG / 8));
			fb_set_dig(a, d);
			TEST_ASSERT(fb_cmp_dig(a, d) == RLC_EQ, end);
		}
		TEST_END;

		bits = 0;
		TEST_CASE("bit setting and getting are consistent") {
			fb_zero(a);
			fb_set_bit(a, bits, 1);
			TEST_ASSERT(fb_get_bit(a, bits) == 1, end);
			fb_set_bit(a, bits, 0);
			TEST_ASSERT(fb_get_bit(a, bits) == 0, end);
			bits = (bits + 1) % RLC_FB_BITS;
		}
		TEST_END;

		bits = 0;
		TEST_CASE("bit assignment and counting are consistent") {
			fb_zero(a);
			fb_set_bit(a, bits, 1);
			TEST_ASSERT(fb_bits(a) == bits + 1, end);
			bits = (bits + 1) % RLC_FB_BITS;
		}
		TEST_END;

		TEST_CASE("reading and writing a binary field element are consistent") {
			fb_rand(a);
			for (int j = 1; j < 7; j++) {
				bits = fb_size_str(a, 1 << j);
				fb_write_str(str, bits, a, 1 << j);
				fb_read_str(b, str, bits, 1 << j);
				TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
			}
			fb_write_bin(bin, sizeof(bin), a);
			fb_read_bin(b, bin, sizeof(bin));
			TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("getting the size of a binary field element is correct") {
			fb_rand(a);
			TEST_ASSERT(fb_size_str(a, 2) == (1 + fb_bits(a)), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	return code;
}

static int addition(void) {
	int code = RLC_ERR;
	fb_t a, b, c, d, e;

	fb_null(a);
	fb_null(b);
	fb_null(c);
	fb_null(d);
	fb_null(e);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);
		fb_new(d);
		fb_new(e);

		TEST_CASE("addition is commutative") {
			fb_rand(a);
			fb_rand(b);
			fb_add(d, a, b);
			fb_add(e, b, a);
			TEST_ASSERT(fb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("addition is associative") {
			fb_rand(a);
			fb_rand(b);
			fb_rand(c);
			fb_add(d, a, b);
			fb_add(d, d, c);
			fb_add(e, b, c);
			fb_add(e, a, e);
			TEST_ASSERT(fb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("addition has identity") {
			fb_rand(a);
			fb_zero(d);
			fb_add(e, a, d);
			TEST_ASSERT(fb_cmp(e, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("addition has inverse") {
			fb_rand(a);
			fb_copy(d, a);
			fb_add(e, a, d);
			TEST_ASSERT(fb_is_zero(e), end);
		} TEST_END;

		TEST_CASE("addition of the modulo f(z) is correct") {
			fb_rand(a);
			fb_poly_add(d, a);
			fb_add(e, a, fb_poly_get());
			TEST_ASSERT(fb_cmp(d, e) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	fb_free(d);
	fb_free(e);
	return code;
}

static int multiplication(void) {
	int code = RLC_ERR;
	fb_t a, b, c, d;

	fb_null(a);
	fb_null(b);
	fb_null(c);
	fb_null(d);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);
		fb_new(d);

		TEST_CASE("multiplication is commutative") {
			fb_rand(a);
			fb_rand(b);
			fb_mul(c, a, b);
			fb_mul(d, b, a);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication is associative") {
			fb_rand(a);
			fb_rand(b);
			fb_rand(c);
			fb_mul(d, a, b);
			fb_mul(d, d, c);
			fb_mul(b, b, c);
			fb_mul(c, a, b);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication is distributive") {
			fb_rand(a);
			fb_rand(b);
			fb_rand(c);
			fb_add(d, a, b);
			fb_mul(d, c, d);
			fb_mul(a, c, a);
			fb_mul(b, c, b);
			fb_add(c, a, b);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication has identity") {
			fb_rand(a);
			fb_zero(c);
			fb_set_bit(c, 0, 1);
			fb_mul(d, a, c);
			TEST_ASSERT(fb_cmp(d, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication has zero property") {
			fb_rand(a);
			fb_zero(b);
			fb_mul(c, a, b);
			TEST_ASSERT(fb_is_zero(c), end);
		} TEST_END;

#if FB_MUL == BASIC || !defined(STRIP)
		TEST_CASE("basic multiplication is correct") {
			fb_rand(a);
			fb_rand(b);
			fb_mul(c, a, b);
			fb_mul_basic(d, a, b);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if FB_MUL == LODAH || !defined(STRIP)
		TEST_CASE("lopez-dahab multiplication is correct") {
			fb_rand(a);
			fb_rand(b);
			fb_mul(c, a, b);
			fb_mul_lodah(d, a, b);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if FB_MUL == INTEG || !defined(STRIP)
		TEST_CASE("integrated multiplication is correct") {
			fb_rand(a);
			fb_rand(b);
			fb_mul(c, a, b);
			fb_mul_integ(d, a, b);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if FB_KARAT > 0 || !defined(STRIP)
		TEST_CASE("karatsuba multiplication is correct") {
			fb_rand(a);
			fb_rand(b);
			fb_mul(c, a, b);
			fb_mul_karat(d, a, b);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		}
		TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	fb_free(d);
	return code;
}

static int squaring(void) {
	int code = RLC_ERR;
	fb_t a, b, c;

	fb_null(a);
	fb_null(b);
	fb_null(c);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);

		TEST_CASE("squaring is correct") {
			fb_rand(a);
			fb_mul(b, a, a);
			fb_sqr(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;

#if FB_SQR == BASIC || !defined(STRIP)
		TEST_CASE("basic squaring is correct") {
			fb_rand(a);
			fb_sqr(b, a);
			fb_sqr_basic(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_SQR == QUICK || !defined(STRIP)
		TEST_CASE("table squaring is correct") {
			fb_rand(a);
			fb_sqr(b, a);
			fb_sqr_quick(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_SQR == INTEG || !defined(STRIP)
		TEST_CASE("integrated squaring is correct") {
			fb_rand(a);
			fb_sqr(b, a);
			fb_sqr_integ(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	return code;
}

static int square_root(void) {
	int code = RLC_ERR;
	fb_t a, b, c;

	fb_null(a);
	fb_null(b);
	fb_null(c);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);

		TEST_CASE("square root extraction is correct") {
			fb_rand(a);
			fb_sqr(c, a);
			fb_srt(b, c);
			TEST_ASSERT(fb_cmp(b, a) == RLC_EQ, end);
		} TEST_END;

#if FB_SRT == BASIC || !defined(STRIP)
		TEST_CASE("basic square root extraction is correct") {
			fb_rand(a);
			fb_srt(b, a);
			fb_srt_basic(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_SRT == QUICK || !defined(STRIP)
		TEST_CASE("fast square root extraction is correct") {
			fb_rand(a);
			fb_srt(b, a);
			fb_srt_quick(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		}
		TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	return code;
}

static int shifting(void) {
	int code = RLC_ERR;
	fb_t a, b, c;

	fb_null(a);
	fb_null(b);
	fb_null(c);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);

		TEST_CASE("shifting by 1 bit is consistent") {
			fb_rand(a);
			a[RLC_FB_DIGS - 1] = 0;
			fb_lsh(b, a, 1);
			fb_rsh(c, b, 1);
			TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("shifting by 2 bits is consistent") {
			fb_rand(a);
			a[RLC_FB_DIGS - 1] = 0;
			fb_lsh(b, a, 2);
			fb_rsh(c, b, 2);
			TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("shifting by half digit is consistent") {
			fb_rand(a);
			a[RLC_FB_DIGS - 1] = 0;
			fb_lsh(b, a, RLC_DIG / 2);
			fb_rsh(c, b, RLC_DIG / 2);
			TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("shifting by 1 digit is consistent") {
			fb_rand(a);
			a[RLC_FB_DIGS - 1] = 0;
			fb_lsh(b, a, RLC_DIG);
			fb_rsh(c, b, RLC_DIG);
			TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
		} TEST_END;

		if (RLC_FB_DIGS > 1) {
			TEST_CASE("shifting by 2 digits is consistent") {
				fb_rand(a);
				a[RLC_FB_DIGS - 1] = 0;
				a[RLC_FB_DIGS - 2] = 0;
				fb_lsh(b, a, 2 * RLC_DIG);
				fb_rsh(c, b, 2 * RLC_DIG);
				TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
			} TEST_END;

			TEST_CASE("shifting by 1 digit and half is consistent") {
				fb_rand(a);
				a[RLC_FB_DIGS - 1] = 0;
				a[RLC_FB_DIGS - 2] = 0;
				fb_lsh(b, a, RLC_DIG + RLC_DIG / 2);
				fb_rsh(c, b, (RLC_DIG + RLC_DIG / 2));
				TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
			} TEST_END;
		}
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	return code;
}

static int reduction(void) {
	int code = RLC_ERR;
	fb_t a, b, c;
	dv_t t0, t1;
	dig_t carry;

	fb_null(a);
	fb_null(b);
	fb_null(c);
	dv_null(t0);
	dv_null(t1);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);
		dv_new(t0);
		dv_new(t1);

		TEST_CASE("modular reduction is correct") {
			if (FB_POLYN % RLC_DIG == 0) {
				/* Test if a * f(z) mod f(z) == 0. */
				fb_rand(a);
				fb_mul(b, a, fb_poly_get());
				fb_copy(t0, b);
				fb_copy(t0 + RLC_FB_DIGS, a);
				fb_rdc(a, t0);
			} else {
				/* Test if f(z) * z^(m-1) mod f(z) == 0. */
				dv_zero(t0, RLC_FB_DIGS);
				carry = fb_lshb_low(t0 + RLC_FB_DIGS - 1, fb_poly_get(),
						FB_POLYN % RLC_DIG - 1);
				t0[2 * RLC_FB_DIGS - 1] = carry;
				fb_rdc(a, t0);
			}
			TEST_ASSERT(fb_is_zero(a) == 1, end);
		}
		TEST_END;

#if FB_RDC == BASIC || !defined(STRIP)
		TEST_CASE("basic modular reduction is correct") {
			dv_zero(t0, 2 * RLC_FB_DIGS);
			dv_zero(t1, 2 * RLC_FB_DIGS);
			fb_rand(a);
			fb_copy(t0 + RLC_FB_DIGS - 1, a);
			fb_copy(t1 + RLC_FB_DIGS - 1, a);
			fb_rdc(b, t0);
			fb_rdc_basic(c, t1);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		}
		TEST_END;
#endif

#if FB_RDC == QUICK || !defined(STRIP)
		TEST_CASE("fast modular reduction is correct") {
			dv_zero(t0, 2 * RLC_FB_DIGS);
			dv_zero(t1, 2 * RLC_FB_DIGS);
			fb_rand(a);
			fb_copy(t0 + RLC_FB_DIGS - 1, a);
			fb_copy(t1 + RLC_FB_DIGS - 1, a);
			fb_rdc(b, t0);
			fb_rdc_quick(c, t1);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		}
		TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	dv_free(t0);
	dv_free(t1);
	return code;
}

static int trace(void) {
	int code = RLC_ERR;
	fb_t a, b, c;

	fb_null(a);
	fb_null(b);
	fb_null(c);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);

		TEST_CASE("trace is linear") {
			fb_rand(a);
			fb_rand(b);
			fb_add(c, a, b);
			TEST_ASSERT(fb_trc(c) == (fb_trc(a) ^ fb_trc(b)), end);
		} TEST_END;

#if FB_TRC == BASIC || !defined(STRIP)
		TEST_CASE("basic trace is correct") {
			fb_rand(a);
			TEST_ASSERT(fb_trc(a) == fb_trc_basic(a), end);
		} TEST_END;
#endif

#if FB_TRC == QUICK || !defined(STRIP)
		TEST_CASE("fast trace is correct") {
			fb_rand(a);
			TEST_ASSERT(fb_trc(a) == fb_trc_quick(a), end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	return code;
}

static int solve(void) {
	int code = RLC_ERR;
	fb_t a, b, c;

	fb_null(a);
	fb_null(b);
	fb_null(c);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);

		TEST_CASE("solving a quadratic equation is correct") {
			fb_rand(a);
			fb_rand(b);
			/* Make Tr(a) = 0. */
			fb_add_dig(a, a, fb_trc(a));
			fb_slv(b, a);
			/* Verify the solution. */
			fb_sqr(c, b);
			fb_add(c, c, b);
			TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
		} TEST_END;

#if FB_SLV == BASIC || !defined(STRIP)
		TEST_CASE("basic solve is correct") {
			fb_rand(a);
			/* Make Tr(a) = 0. */
			fb_add_dig(a, a, fb_trc(a));
			fb_slv(c, a);
			fb_slv_basic(b, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_SLV == QUICK || !defined(STRIP)
		TEST_CASE("fast solve is correct") {
			fb_rand(a);
			/* Make Tr(a) = 0. */
			fb_add_dig(a, a, fb_trc(a));
			fb_slv(b, a);
			fb_slv_quick(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	return code;
}

static int inversion(void) {
	int code = RLC_ERR;
	fb_t a, b, c, d[2];

	fb_null(a);
	fb_null(b);
	fb_null(c);
	fb_null(d[0]);
	fb_null(d[1]);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);
		fb_new(d[0]);
		fb_new(d[1]);

		TEST_CASE("inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_mul(c, a, b);
			TEST_ASSERT(fb_cmp_dig(c, 1) == RLC_EQ, end);
		} TEST_END;

#if FB_INV == BASIC || !defined(STRIP)
		TEST_CASE("basic inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_basic(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_INV == BINAR || !defined(STRIP)
		TEST_CASE("binary inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_binar(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_INV == ALMOS || !defined(STRIP)
		TEST_CASE("almost inverse is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_almos(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_INV == EXGCD || !defined(STRIP)
		TEST_CASE("euclidean inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_exgcd(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_INV == BRUCH || !defined(STRIP)
		TEST_CASE("brunner inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_bruch(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_INV == ITOHT || !defined(STRIP)
		TEST_CASE("itoh-tsuji inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_itoht(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_INV == CTAIA || !defined(STRIP)
		TEST_CASE("constant-time almost inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_ctaia(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_INV == LOWER || !defined(STRIP)
		TEST_CASE("lower inversion is correct") {
			do {
				fb_rand(a);
			} while (fb_is_zero(a));
			fb_inv(b, a);
			fb_inv_lower(c, a);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("simultaneous inversion is correct") {
			do {
				fb_rand(a);
				fb_rand(b);
			} while (fb_is_zero(a) || fb_is_zero(b));
			fb_copy(d[0], a);
			fb_copy(d[1], b);
			fb_inv(a, a);
			fb_inv(b, b);
			fb_inv_sim(d, (const fb_t *)d, 2);
			TEST_ASSERT(fb_cmp(d[0], a) == RLC_EQ &&
					fb_cmp(d[1], b) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	fb_free(d[0]);
	fb_free(d[1]);
	return code;
}

static int exponentiation(void) {
	int code = RLC_ERR;
	fb_t a, b, c;
	fb_st t[RLC_FB_TABLE_MAX];
	bn_t d;

	fb_null(a);
	fb_null(b);
	fb_null(c);
	bn_null(d);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);
		bn_new(d);

		TEST_CASE("exponentiation is correct") {
			fb_rand(a);
			bn_zero(d);
			fb_exp(c, a, d);
			TEST_ASSERT(fb_cmp_dig(c, 1) == RLC_EQ, end);
			bn_set_dig(d, 1);
			fb_exp(c, a, d);
			TEST_ASSERT(fb_cmp(c, a) == RLC_EQ, end);
			bn_rand(d, RLC_POS, RLC_FB_BITS);
			fb_exp(b, a, d);
			bn_neg(d, d);
			fb_exp(c, a, d);
			fb_inv(c, c);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
			bn_set_2b(d, RLC_FB_BITS);
			fb_exp(c, a, d);
			TEST_ASSERT(fb_cmp(a, c) == RLC_EQ, end);
		} TEST_END;

#if FB_EXP == BASIC || !defined(STRIP)
		TEST_CASE("basic exponentiation is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, RLC_FB_BITS);
			fb_exp(c, a, d);
			fb_exp_basic(b, a, d);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_EXP == SLIDE || !defined(STRIP)
		TEST_CASE("sliding window exponentiation is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, RLC_FB_BITS);
			fb_exp(c, a, d);
			fb_exp_slide(b, a, d);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_EXP == MONTY || !defined(STRIP)
		TEST_CASE("constant-time exponentiation is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, RLC_FB_BITS);
			fb_exp(c, a, d);
			fb_exp_monty(b, a, d);
			TEST_ASSERT(fb_cmp(b, c) == RLC_EQ, end);
		} TEST_END;
#endif

		TEST_CASE("iterated squaring is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, 4);
			fb_itr_pre(t, d->dp[0]);
			fb_itr(b, a, d->dp[0], (const fb_st *)t);
			for (int j = 0; j < d->dp[0]; j++) {
				fb_sqr(a, a);
			}
			TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("iterated square-root is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, 4);
			fb_itr_pre(t, -d->dp[0]);
			fb_itr(b, a, -d->dp[0], (const fb_st *)t);
			for (int j = 0; j < d->dp[0]; j++) {
				fb_srt(a, a);
			}
			TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

#if FB_ITR == BASIC || !defined(STRIP)
		TEST_CASE("basic iterated squaring is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, 4);
			fb_itr_basic(b, a, d->dp[0]);
			for (int j = 0; j < d->dp[0]; j++) {
				fb_sqr(a, a);
			}
			TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("basic iterated square-root is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, 4);
			fb_itr_basic(b, a, -d->dp[0]);
			for (int j = 0; j < d->dp[0]; j++) {
				fb_srt(a, a);
			}
			TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
		} TEST_END;
#endif

#if FB_ITR == QUICK || !defined(STRIP)
		TEST_CASE("fast iterated squaring is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, 4);
			fb_itr_pre_quick(t, d->dp[0]);
			fb_itr_quick(b, a, (const fb_st *)t);
			for (int j = 0; j < d->dp[0]; j++) {
				fb_sqr(a, a);
			}
			TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("fast iterated square-root is correct") {
			fb_rand(a);
			bn_rand(d, RLC_POS, 4);
			fb_itr_pre_quick(t, -d->dp[0]);
			fb_itr_quick(b, a, (const fb_st *)t);
			for (int j = 0; j < d->dp[0]; j++) {
				fb_srt(a, a);
			}
			TEST_ASSERT(fb_cmp(a, b) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	bn_free(d);
	return code;
}

static int digit(void) {
	int code = RLC_ERR;
	fb_t a, b, c, d;
	dig_t g;

	fb_null(a);
	fb_null(b);
	fb_null(c);
	fb_null(d);

	RLC_TRY {
		fb_new(a);
		fb_new(b);
		fb_new(c);
		fb_new(d);

		TEST_CASE("addition of a single digit is consistent") {
			fb_rand(a);
			fb_rand(b);
			for (int j = 1; j < RLC_FB_DIGS; j++)
				b[j] = 0;
			g = b[0];
			fb_add(c, a, b);
			fb_add_dig(d, a, g);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication by a single digit is consistent") {
			fb_rand(a);
			fb_rand(b);
			for (int j = 1; j < RLC_FB_DIGS; j++) {
				b[j] = 0;
			}
			g = b[0];
			fb_mul(c, a, b);
			fb_mul_dig(d, a, g);
			TEST_ASSERT(fb_cmp(c, d) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	fb_free(a);
	fb_free(b);
	fb_free(c);
	fb_free(d);
	return code;
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the FB module", 0);

	RLC_TRY {
		fb_param_set_any();
		fb_param_print();
	}
	RLC_CATCH_ANY {
		core_clean();
		return 0;
	}

	util_banner("Utilities", 1);
	if (memory() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (util() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Arithmetic", 1);

	if (addition() != RLC_OK) {
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

	if (square_root() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (shifting() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (reduction() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (trace() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (RLC_FB_BITS % 2 != 0) {
		if (solve() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

	if (inversion() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (exponentiation() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (digit() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
