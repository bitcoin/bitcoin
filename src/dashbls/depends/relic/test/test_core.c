/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
 * Tests for configuration management.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

#if defined(MULTI)
#if MULTI == PTHREAD

void *master(void *ptr) {
	int *code = (int *)ptr;
	core_init();
	RLC_THROW(ERR_NO_MEMORY);
	if (err_get_code() != RLC_ERR) {
		*code = RLC_ERR;
	} else {
		*code = RLC_OK;
	}
	core_clean();
	return NULL;
}

void *tester(void *ptr) {
	int *code = (int *)ptr;
	core_init();
	if (err_get_code() != RLC_OK) {
		*code = RLC_ERR;
	} else {
		*code = RLC_OK;
	}
	core_clean();
	return NULL;
}

#endif
#endif

int main(void) {
	int code = RLC_ERR;

	/* Initialize library with default configuration. */
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the CORE module:\n", 0);

	TEST_ONCE("the library context is consistent") {
		TEST_ASSERT(core_get() != NULL, end);
	} TEST_END;

	TEST_ONCE("switching the library context is correct") {
		ctx_t new_ctx, *old_ctx;
		/* Backup the old context. */
		old_ctx = core_get();
		/* Switch the library context. */
		core_set(&new_ctx);
		/* Reinitialize library with new context. */
		core_init();
		/* Run function to manipulate the library context. */
		RLC_THROW(ERR_NO_MEMORY);
		core_set(old_ctx);
		TEST_ASSERT(err_get_code() == RLC_OK, end);
		core_set(&new_ctx);
		TEST_ASSERT(err_get_code() == RLC_ERR, end);
		/* Now we need to finalize the new context. */
		core_clean();
		/* And restore the original context. */
		core_set(old_ctx);
	} TEST_END;

	code = RLC_OK;

#if defined(MULTI)
#if MULTI == OPENMP
	TEST_ONCE("library context is thread-safe") {
		omp_set_num_threads(CORES);
#pragma omp parallel shared(code)
		{
			if (omp_get_thread_num() == 0) {
				RLC_THROW(ERR_NO_MEMORY);
				if (err_get_code() != RLC_ERR) {
					code = RLC_ERR;
				}
			} else {
				core_init();
				if (err_get_code() != RLC_OK) {
					code = RLC_ERR;
				}
				core_clean();
			}
		}
		TEST_ASSERT(code == RLC_OK, end);

		core_init();
#pragma omp parallel copyin(core_ctx) shared(code)
		{
			if (core_get() == NULL) {
				code = RLC_ERR;
			}
		}
		TEST_ASSERT(code == RLC_OK, end);
		core_clean();
	} TEST_END;
#endif

#if MULTI == PTHREAD
	TEST_ONCE("library context is thread-safe") {
		pthread_t thread[CORES];
		int result[CORES] = { RLC_OK };
		for (int i = 0; i < CORES; i++) {
			if (i == 0) {
				if (pthread_create(&(thread[0]), NULL, master, &(result[0]))) {
					code = RLC_ERR;
				}
			} else {
				if (pthread_create(&(thread[i]), NULL, tester, &(result[i]))) {
					code = RLC_ERR;
				}
			}
			if (result[i] != RLC_OK) {
				code = RLC_ERR;
			}
		}

		for (int i = 0; i < CORES; i++) {
			if (pthread_join(thread[i], NULL)) {
				code = RLC_ERR;
			}
		}
		TEST_ASSERT(code == RLC_OK, end);
	} TEST_END;
#endif
#endif

	util_banner("All tests have passed.\n", 0);
  end:
	core_clean();
	return code;
}
