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
 * Tests for manipulating temporary double-precision digit vectors.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = RLC_ERR;
	dv_t a;

	dv_null(a);

	RLC_TRY {
		TEST_CASE("temporary memory can be allocated") {
			dv_new(a);
			dv_free(a);
		} TEST_END;
	} RLC_CATCH(e) {
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

static int copy(void) {
	dv_t a, b;
	int code = RLC_ERR;

	dv_null(a);
	dv_null(b);

	RLC_TRY {
		dv_new(a);
		dv_new(b);

		TEST_CASE("copy and comparison are consistent") {
			rand_bytes((uint8_t *)a, RLC_DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, RLC_DV_DIGS * sizeof(dig_t));
			if (dv_cmp(a, b, RLC_DV_DIGS) != RLC_EQ) {
				if (dv_cmp(a, b, RLC_DV_DIGS) == RLC_GT) {
					TEST_ASSERT(dv_cmp(b, a, RLC_DV_DIGS) == RLC_LT, end);
				} else {
					TEST_ASSERT(dv_cmp(b, a, RLC_DV_DIGS) == RLC_GT, end);
				}
			}
			dv_copy(a, b, RLC_DV_DIGS);
			TEST_ASSERT(dv_cmp_const(a, b, RLC_DV_DIGS) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("conditional copy and comparison are consistent") {
			rand_bytes((uint8_t *)a, RLC_DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, RLC_DV_DIGS * sizeof(dig_t));
			dv_copy_cond(a, b, RLC_DV_DIGS, 0);
			TEST_ASSERT(dv_cmp_const(a, b, RLC_DV_DIGS) == RLC_NE, end);
			dv_copy_cond(a, b, RLC_DV_DIGS, 1);
			TEST_ASSERT(dv_cmp_const(a, b, RLC_DV_DIGS) == RLC_EQ, end);
		}
		TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	dv_free(a);
	dv_free(b);
	return code;
}

static int swap(void) {
	dv_t a, b, c, d;
	int code = RLC_ERR;

	dv_null(a);
	dv_null(b);
	dv_null(c);
	dv_null(d);

	RLC_TRY {
		dv_new(a);
		dv_new(b);
		dv_new(c);
		dv_new(d);

		TEST_CASE("conditional swap and copy are consistent") {
			rand_bytes((uint8_t *)a, RLC_DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, RLC_DV_DIGS * sizeof(dig_t));
			dv_copy(c, a, RLC_DV_DIGS);
			dv_swap_cond(a, b, RLC_DV_DIGS, 1);
			TEST_ASSERT(dv_cmp_const(c, b, RLC_DV_DIGS) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("conditional swap and comparison are consistent") {
			rand_bytes((uint8_t *)a, RLC_DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, RLC_DV_DIGS * sizeof(dig_t));
			dv_copy(c, a, RLC_DV_DIGS);
			dv_copy(d, b, RLC_DV_DIGS);
			dv_swap_cond(a, b, RLC_DV_DIGS, 0);
			TEST_ASSERT(dv_cmp_const(c, a, RLC_DV_DIGS) == RLC_EQ, end);
			TEST_ASSERT(dv_cmp_const(d, b, RLC_DV_DIGS) == RLC_EQ, end);
			TEST_ASSERT(dv_cmp_const(c, b, RLC_DV_DIGS) == RLC_NE, end);
			TEST_ASSERT(dv_cmp_const(d, a, RLC_DV_DIGS) == RLC_NE, end);
			dv_swap_cond(a, b, RLC_DV_DIGS, 1);
			TEST_ASSERT(dv_cmp_const(c, b, RLC_DV_DIGS) == RLC_EQ, end);
			TEST_ASSERT(dv_cmp_const(d, a, RLC_DV_DIGS) == RLC_EQ, end);
			TEST_ASSERT(dv_cmp_const(c, a, RLC_DV_DIGS) == RLC_NE, end);
			TEST_ASSERT(dv_cmp_const(d, b, RLC_DV_DIGS) == RLC_NE, end);
		}
		TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	dv_free(a);
	dv_free(b);
	dv_free(c);
	dv_free(d);
	return code;
}

static int shifting(void) {
	int code = RLC_ERR;
	dv_t a, b, c;

	dv_null(a);
	dv_null(b);
	dv_null(c);

	RLC_TRY {
		dv_new(a);
		dv_new(b);
		dv_new(c);

		TEST_CASE("shifting by  digit is consistent") {
			rand_bytes((uint8_t *)a, RLC_DV_DIGS * sizeof(dig_t));
			a[RLC_DV_DIGS - 1] = 0;
			dv_lshd(b, a, RLC_DV_DIGS, 1);
			dv_rshd(c, b, RLC_DV_DIGS, 1);
			TEST_ASSERT(dv_cmp(c, a, RLC_DV_DIGS) == RLC_EQ, end);
		} TEST_END;

		if (RLC_DV_DIGS > 1) {
			TEST_CASE("shifting by 2 digits is consistent") {
				rand_bytes((uint8_t *)a, RLC_DV_DIGS * sizeof(dig_t));
				a[RLC_DV_DIGS - 1] = 0;
				a[RLC_DV_DIGS - 2] = 0;
				dv_lshd(b, a, RLC_DV_DIGS, 2);
				dv_rshd(c, b, RLC_DV_DIGS, 2);
				TEST_ASSERT(dv_cmp(c, a, RLC_DV_DIGS) == RLC_EQ, end);
			} TEST_END;
		}
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	dv_free(a);
	dv_free(b);
	dv_free(c);
	return code;
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the DV module:\n", 0);

	if (memory() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (copy() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (swap() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (shifting() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
