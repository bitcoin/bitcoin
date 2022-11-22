/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
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
 * @defgroup relic Core functions
 */

/**
 * @file
 *
 * Multithreading support.
 *
 * @ingroup relic
 */

#ifndef RLC_MULTI_H
#define RLC_MULTI_H

#if defined(MULTI)
#include <math.h>
#if MULTI == OPENMP
#include <omp.h>
#elif MULTI == PTHREAD
#include <pthread.h>
#endif /* OPENMP */
#endif /* MULTI */

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

#if defined(MULTI)

/**
 * If multi-threading is enabled, assigns each thread a local copy of the data.
 */
#if MULTI == PTHREAD
#define rlc_thread 	__thread
#else
#define rlc_thread /* */
#endif

/**
 * Make library context private to each thread.
 */
#if MULTI == OPENMP
/**
 * Active library context, only visible inside the library.
 */
extern ctx_t first_ctx;

/**
 * Pointer to active library context, only visible inside the library.
 */
extern ctx_t *core_ctx;

#pragma omp threadprivate(first_ctx, core_ctx)
#endif

#endif /* MULTI */

#endif /* !RLC_MULTI_H */
