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
		THROW(ERR_NO_MEMORY);
	}
}

int main(void) {
	err_t e;
	char *msg = NULL;
	int code = STS_ERR;

	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the ERR module:\n", 0);

	TEST_ONCE("not using try-catch is correct") {
		dummy();
		if (err_get_code() == STS_ERR) {
			err_get_msg(&e, &msg);
			TEST_ASSERT(msg == core_get()->reason[ERR_NO_MEMORY], end);
			TEST_ASSERT(err_get_code() != STS_ERR, end);
		}
	} TEST_END;

	j = 0;

	TEST_ONCE("try-catch is correct and error message is printed");
	TRY {
		dummy();
	}
	CATCH(e) {
		switch (e) {
			case ERR_NO_MEMORY:
				TEST_END;
				ERROR(end);
				break;
		}
	}

	util_banner("All tests have passed.\n", 0);

	code = STS_OK;
  end:
	core_clean();
	if (code == STS_ERR)
		return 0;
	else {
		return 1;
	}
}
