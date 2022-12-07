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
 * Tests for the error-management routines.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_err.h"
#include "relic_test.h"

static void dummy(void);
static void dummy2(void);

int j;

static void dummy(void) {
	j++;
	if (j < 6)
		dummy2();
}

static void dummy2(void) {
	j++;
	if (j < 5)
		dummy();
	else {
		RLC_THROW(ERR_NO_MEMORY);
	}
}

static void dummy3(void) {
	bn_t a;

	bn_null(a);

	RLC_TRY {
		bn_new(a);
		RLC_THROW(ERR_NO_MEMORY);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(a);
	}
}

static void dummy4(void) {
	RLC_TRY {
		dummy3();
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
	}
}


int main(void) {
	err_t e;
	char *msg = NULL;
	int code = RLC_ERR;

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the ERR module:\n", 0);

	TEST_ONCE("not using try-catch is correct") {
		dummy();
		if (err_get_code() == RLC_ERR) {
			err_get_msg(&e, &msg);
			TEST_ASSERT(msg == core_get()->reason[ERR_NO_MEMORY], end);
			TEST_ASSERT(err_get_code() != RLC_ERR, end);
		}
	} TEST_END;

	j = 0;

	TEST_ONCE("try-catch is correct and error message is printed") {
		RLC_TRY {
			dummy();
		}
		RLC_CATCH(e) {
			switch (e) {
				case ERR_NO_MEMORY:
					TEST_END;
					break;
			}
		}
	}

#ifdef WITH_BN
	TEST_ONCE("throw in try-catch is correct and error message is printed") {
		bn_t a;
		bn_null(a);

		RLC_TRY {
			bn_new(a);
			dummy3();
		}
		RLC_CATCH(e) {
			bn_free(a);
			switch (e) {
				case ERR_NO_MEMORY:
					TEST_END;
					break;
			}
		}
	}

	TEST_ONCE("throw in deep try-catch is correct and functions are printed") {
		bn_t a;
		bn_null(a);

		RLC_TRY {
			bn_new(a);
			dummy4();
		}
		RLC_CATCH(e) {
			bn_free(a);
			switch (e) {
				case ERR_NO_MEMORY:
					TEST_END;
					RLC_ERROR(end);
					break;
			}
		}
	}
#endif

	code = RLC_OK;
	
  end:
	core_clean();
	if (code == RLC_ERR) {
		util_banner("All tests have passed.\n", 0);
		return 0;
	} else {
		return 1;
	}
}
