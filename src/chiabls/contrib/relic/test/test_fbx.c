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
 * Tests for extensions defined over binary fields.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory2(void) {
	err_t e;
	int code = STS_ERR;
	fb2_t a;

	fb2_null(a);

	TRY {
		TEST_BEGIN("memory can be allocated") {
			fb2_new(a);
			fb2_free(a);
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

static int util2(void) {
	int code = STS_ERR;
	fb2_t a, b, c;

	fb2_null(a);
	fb2_null(b);
	fb2_null(c);

	TRY {
		fb2_new(a);
		fb2_new(b);
		fb2_new(c);

		TEST_BEGIN("comparison is consistent") {
			fb2_rand(a);
			fb2_rand(b);
			if (fb2_cmp(a, b) != CMP_EQ) {
				TEST_ASSERT(fb2_cmp(b, a) == CMP_NE, end);
			}
		}
		TEST_END;

		TEST_BEGIN("copy and comparison are consistent") {
			fb2_rand(a);
			fb2_rand(b);
			fb2_rand(c);
			if (fb2_cmp(a, c) != CMP_EQ) {
				fb2_copy(c, a);
				TEST_ASSERT(fb2_cmp(c, a) == CMP_EQ, end);
			}
			if (fb2_cmp(b, c) != CMP_EQ) {
				fb2_copy(c, b);
				TEST_ASSERT(fb2_cmp(b, c) == CMP_EQ, end);
			}
		}
		TEST_END;

		TEST_BEGIN("negation is consistent") {
			fb2_rand(a);
			fb2_neg(b, a);
			if (fb2_cmp(a, b) != CMP_EQ) {
				TEST_ASSERT(fb2_cmp(b, a) == CMP_NE, end);
			}
			fb2_neg(b, b);
			TEST_ASSERT(fb2_cmp(a, b) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("assignment to zero and comparison are consistent") {
			fb2_rand(a);
			fb2_zero(c);
			TEST_ASSERT(fb2_cmp(a, c) == CMP_NE, end);
			TEST_ASSERT(fb2_cmp(c, a) == CMP_NE, end);
		}
		TEST_END;

		TEST_BEGIN("assignment to random and comparison are consistent") {
			fb2_rand(a);
			fb2_zero(c);
			TEST_ASSERT(fb2_cmp(a, c) == CMP_NE, end);
		}
		TEST_END;

		TEST_BEGIN("assignment to zero and zero test are consistent") {
			fb2_zero(a);
			TEST_ASSERT(fb2_is_zero(a), end);
		}
		TEST_END;

	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb2_free(a);
	fb2_free(b);
	fb2_free(c);
	return code;
}

static int addition2(void) {
	int code = STS_ERR;
	fb2_t a, b, c, d, e;

	fb2_null(a);
	fb2_null(b);
	fb2_null(c);
	fb2_null(d);
	fb2_null(e);

	TRY {
		fb2_new(a);
		fb2_new(b);
		fb2_new(c);
		fb2_new(d);
		fb2_new(e);

		TEST_BEGIN("addition is commutative") {
			fb2_rand(a);
			fb2_rand(b);
			fb2_add(d, a, b);
			fb2_add(e, b, a);
			TEST_ASSERT(fb2_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("addition is associative") {
			fb2_rand(a);
			fb2_rand(b);
			fb2_rand(c);
			fb2_add(d, a, b);
			fb2_add(d, d, c);
			fb2_add(e, b, c);
			fb2_add(e, a, e);
			TEST_ASSERT(fb2_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("addition has identity") {
			fb2_rand(a);
			fb2_zero(d);
			fb2_add(e, a, d);
			TEST_ASSERT(fb2_cmp(e, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("addition has inverse") {
			fb2_rand(a);
			fb2_neg(d, a);
			fb2_add(e, a, d);
			TEST_ASSERT(fb2_is_zero(e), end);
		} TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb2_free(a);
	fb2_free(b);
	fb2_free(c);
	fb2_free(d);
	fb2_free(e);
	return code;
}

static int subtraction2(void) {
	int code = STS_ERR;
	fb2_t a, b, c, d;

	fb2_null(a);
	fb2_null(b);
	fb2_null(c);
	fb2_null(d);

	TRY {
		fb2_new(a);
		fb2_new(b);
		fb2_new(c);
		fb2_new(d);

		TEST_BEGIN("subtraction is anti-commutative") {
			fb2_rand(a);
			fb2_rand(b);
			fb2_sub(c, a, b);
			fb2_sub(d, b, a);
			fb2_neg(d, d);
			TEST_ASSERT(fb2_cmp(c, d) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("subtraction has identity") {
			fb2_rand(a);
			fb2_zero(c);
			fb2_sub(d, a, c);
			TEST_ASSERT(fb2_cmp(d, a) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("subtraction has inverse") {
			fb2_rand(a);
			fb2_sub(c, a, a);
			TEST_ASSERT(fb2_is_zero(c), end);
		}
		TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb2_free(a);
	fb2_free(b);
	fb2_free(c);
	fb2_free(d);
	return code;
}

static int multiplication2(void) {
	int code = STS_ERR;
	fb2_t a, b, c, d, e, f;

	fb2_null(a);
	fb2_null(b);
	fb2_null(c);
	fb2_null(d);
	fb2_null(e);
	fb2_null(f);

	TRY {
		fb2_new(a);
		fb2_new(b);
		fb2_new(c);
		fb2_new(d);
		fb2_new(e);
		fb2_new(f);

		TEST_BEGIN("multiplication is commutative") {
			fb2_rand(a);
			fb2_rand(b);
			fb2_mul(d, a, b);
			fb2_mul(e, b, a);
			TEST_ASSERT(fb2_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication is associative") {
			fb2_rand(a);
			fb2_rand(b);
			fb2_rand(c);
			fb2_mul(d, a, b);
			fb2_mul(d, d, c);
			fb2_mul(e, b, c);
			fb2_mul(e, a, e);
			TEST_ASSERT(fb2_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication is distributive") {
			fb2_rand(a);
			fb2_rand(b);
			fb2_rand(c);
			fb2_add(d, a, b);
			fb2_mul(d, c, d);
			fb2_mul(e, c, a);
			fb2_mul(f, c, b);
			fb2_add(e, e, f);
			TEST_ASSERT(fb2_cmp(d, e) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication has identity") {
			fb2_zero(d);
			fb_set_bit(d[0], 0, 1);
			fb2_rand(a);
			fb2_mul(e, a, d);
			TEST_ASSERT(fb2_cmp(e, a) == CMP_EQ, end);
		} TEST_END;

		TEST_BEGIN("multiplication has zero property") {
			fb2_zero(d);
			fb2_rand(a);
			fb2_mul(e, a, d);
			TEST_ASSERT(fb2_is_zero(e), end);
		} TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb2_free(a);
	fb2_free(b);
	fb2_free(c);
	fb2_free(d);
	fb2_free(e);
	fb2_free(f);
	return code;
}

static int squaring2(void) {
	int code = STS_ERR;
	fb2_t a, b, c;

	fb2_null(a);
	fb2_null(b);
	fb2_null(c);

	TRY {
		fb2_new(a);
		fb2_new(b);
		fb2_new(c);

		TEST_BEGIN("squaring is correct") {
			fb2_rand(a);
			fb2_mul(b, a, a);
			fb2_sqr(c, a);
			TEST_ASSERT(fb2_cmp(b, c) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb2_free(a);
	fb2_free(b);
	fb2_free(c);
	return code;
}

static int solve2(void) {
	int code = STS_ERR;
	fb2_t a, b, c;

	fb2_null(a);
	fb2_null(b);
	fb2_null(c);

	TRY {
		fb2_new(a);
		fb2_new(b);
		fb2_new(c);

		TEST_BEGIN("solving a quadratic equation is correct") {
			fb2_rand(a);
			fb2_rand(b);
			/* Make Tr(a_1) = 0. */
			fb_add_dig(a[0], a[0], fb_trc(a[0]));
			fb_add_dig(a[1], a[1], fb_trc(a[1]));
			fb2_slv(b, a);
			/* Verify the solution. */
			fb2_sqr(c, b);
			fb2_add(c, c, b);
			TEST_ASSERT(fb2_cmp(c, a) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb2_free(a);
	fb2_free(b);
	fb2_free(c);
	return code;
}

static int inversion2(void) {
	int code = STS_ERR;
	fb2_t a, b, c;

	fb2_null(a);
	fb2_null(b);
	fb2_null(c);

	TRY {
		fb2_new(a);
		fb2_new(b);
		fb2_new(c);

		TEST_BEGIN("inversion is correct") {
			fb2_rand(a);
			fb2_inv(b, a);
			fb2_mul(c, a, b);
			TEST_ASSERT(fb_cmp_dig(c[0], 1) == CMP_EQ, end);
		} TEST_END;
	}
	CATCH_ANY {
		util_print("FATAL ERROR!\n");
		ERROR(end);
	}
	code = STS_OK;
  end:
	fb2_free(a);
	fb2_free(b);
	fb2_free(c);
	return code;
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the FBX module", 0);

	TRY {
		fb_param_set_any();
	} CATCH_ANY {
		core_clean();
		return 0;
	}

	util_banner("Quadratic extension:", 0);
	util_banner("Utilities:", 1);

	if (memory2() != STS_OK) {
		core_clean();
		return 1;
	}

	if (util2() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Arithmetic:", 1);

	if (addition2() != STS_OK) {
		core_clean();
		return 1;
	}

	if (subtraction2() != STS_OK) {
		core_clean();
		return 1;
	}

	if (multiplication2() != STS_OK) {
		core_clean();
		return 1;
	}

	if (squaring2() != STS_OK) {
		core_clean();
		return 1;
	}

	if (solve2() != STS_OK) {
		core_clean();
		return 1;
	}

	if (inversion2() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
