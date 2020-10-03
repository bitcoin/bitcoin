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
 * Tests for manipulating temporary double-precision digit vectors.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int memory(void) {
	err_t e;
	int code = STS_ERR;

#if ALLOC == STATIC
	dv_t a[POOL_SIZE + 1];

	for (int i = 0; i < POOL_SIZE; i++) {
		dv_null(a[i]);
	}
	dv_null(a[POOL_SIZE]);

	TRY {
		TEST_BEGIN("all memory can be allocated") {
			for (int j = 0; j < POOL_SIZE; j++) {
				dv_new(a[j]);
			}
			dv_new(a[POOL_SIZE]);
		}
	} CATCH(e) {
		switch (e) {
			case ERR_NO_MEMORY:
				for (int j = 0; j < POOL_SIZE; j++) {
					dv_free(a[j]);
				}
				dv_free(a[POOL_SIZE]);
				break;
		}
		code = STS_OK;
	}
#else
	dv_t a;

	dv_null(a);

	TRY {
		TEST_BEGIN("temporary memory can be allocated") {
			dv_new(a);
			dv_free(a);
		} TEST_END;
	} CATCH(e) {
		switch (e) {
			case ERR_NO_MEMORY:
				util_print("FATAL ERROR!\n");
				ERROR(end);
				break;
		}
	}
	code = STS_OK;
  end:
#endif
	return code;
}

static int copy(void) {
	dv_t a, b;
	int code = STS_ERR;

	dv_null(a);
	dv_null(b);

	TRY {
		dv_new(a);
		dv_new(b);

		TEST_BEGIN("copy and comparison are consistent") {
			rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
			dv_copy(a, b, DV_DIGS);
			TEST_ASSERT(dv_cmp_const(a, b, DV_DIGS) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("conditional copy and comparison are consistent") {
			rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
			dv_copy_cond(a, b, DV_DIGS, 0);
			TEST_ASSERT(dv_cmp_const(a, b, DV_DIGS) == CMP_NE, end);
			dv_copy_cond(a, b, DV_DIGS, 1);
			TEST_ASSERT(dv_cmp_const(a, b, DV_DIGS) == CMP_EQ, end);
		}
		TEST_END;
	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	dv_free(a);
	dv_free(b);
	return code;
}

static int swap(void) {
	dv_t a, b, c, d;
	int code = STS_ERR;

	dv_null(a);
	dv_null(b);
	dv_null(c);
	dv_null(d);

	TRY {
		dv_new(a);
		dv_new(b);
		dv_new(c);
		dv_new(d);

		TEST_BEGIN("conditional swap and copy are consistent") {
			rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
			dv_copy(c, a, DV_DIGS);
			dv_swap_cond(a, b, DV_DIGS, 1);
			TEST_ASSERT(dv_cmp_const(c, b, DV_DIGS) == CMP_EQ, end);
		}
		TEST_END;

		TEST_BEGIN("conditional swap and comparison are consistent") {
			rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
			rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
			dv_copy(c, a, DV_DIGS);
			dv_copy(d, b, DV_DIGS);
			dv_swap_cond(a, b, DV_DIGS, 0);
			TEST_ASSERT(dv_cmp_const(c, a, DV_DIGS) == CMP_EQ, end);
			TEST_ASSERT(dv_cmp_const(d, b, DV_DIGS) == CMP_EQ, end);
			TEST_ASSERT(dv_cmp_const(c, b, DV_DIGS) == CMP_NE, end);
			TEST_ASSERT(dv_cmp_const(d, a, DV_DIGS) == CMP_NE, end);
			dv_swap_cond(a, b, DV_DIGS, 1);
			TEST_ASSERT(dv_cmp_const(c, b, DV_DIGS) == CMP_EQ, end);
			TEST_ASSERT(dv_cmp_const(d, a, DV_DIGS) == CMP_EQ, end);
			TEST_ASSERT(dv_cmp_const(c, a, DV_DIGS) == CMP_NE, end);
			TEST_ASSERT(dv_cmp_const(d, b, DV_DIGS) == CMP_NE, end);
		}
		TEST_END;
	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;
  end:
	dv_free(a);
	dv_free(b);
	dv_free(c);
	dv_free(d);
	return code;
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the DV module:\n", 0);

	if (memory() != STS_OK) {
		core_clean();
		return 1;
	}

	if (copy() != STS_OK) {
		core_clean();
		return 1;
	}

	if (swap() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
