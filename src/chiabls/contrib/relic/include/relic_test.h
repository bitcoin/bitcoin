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
 * @defgroup tests Automated tests
 */

/**
 * @file
 *
 * Interface of useful routines for testing.
 *
 * @ingroup test
 */

#ifndef RELIC_TEST_H
#define RELIC_TEST_H

#include <string.h>

#include "relic_conf.h"
#include "relic_label.h"
#include "relic_util.h"

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Runs a new benchmark once.
 *
 * @param[in] P				- the property description.
 */
#define TEST_ONCE(P)														\
	util_print("Testing if " P "...%*c", (64 - strlen(P)), ' ');			\

/**
 * Tests a sequence of commands to see if they respect some property.
 *
 * @param[in] P				- the property description.
 */
#define TEST_BEGIN(P)														\
	util_print("Testing if " P "...%*c", (64 - strlen(P)), ' ');			\
	for (int i = 0; i < TESTS; i++)											\

/**
 * Asserts a condition.
 *
 * If the condition is not satisfied, a unconditional jump is made to the passed
 * label.
 *
 * @param[in] C				- the condition to assert.
 * @param[in] LABEL			- the label to jump if the condition is no satisfied.
 */
#define TEST_ASSERT(C, LABEL)												\
	if (!(C)) {																\
		test_fail();														\
		util_print("(at ");													\
		util_print(__FILE__);												\
		util_print(":%d)\n", __LINE__);										\
		ERROR(LABEL);														\
	}																		\

/**
 * Finalizes a test printing the test result.
 */
#define TEST_END															\
	test_pass()																\

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Prints a string indicating that the test failed.
 */
void test_fail(void);

/**
 * Prints a string indicating that the test passed.
 */
void test_pass(void);

#endif /* !RELIC_TEST_H */
