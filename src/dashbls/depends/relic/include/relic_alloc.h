/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2018 RELIC Authors
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
 * Implementation of the auxiliary memory allocation functions.
 *
 * @ingroup utils
 */

#include "relic_conf.h"

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#include <malloc.h>

/*
 * Dynamiclly allocates an array of "Type" with the specified size on the stack.
 * This memory will be automaticlly deallocated from the stack when the function
 * frame is returned from.
 * Note: This is the Windows specific implementation.
 *
 * @param[in] T                 - the type of each object.
 * @param[in] S                 - the number of obecs to allocate.
 */
#if ALLOC == DYNAMIC
#define RLC_ALLOCA(T, S)		(T*) calloc((S), sizeof(T))
#else
#define RLC_ALLOCA(T, S)		(T*) _alloca((S) * sizeof(T))
#endif

#else /* _MSC_VER */

#if OPSYS == FREEBSD || OPSYS == NETBSD
#include <stdlib.h>
#else
#include <alloca.h>
#endif

/*
 * Dynamiclly allocates an array of "Type" with the specified size on the stack.
 * This memory will be automaticlly deallocated from the stack when the function
 * frame is returned from.
 * Note: This is the POSIX specific implementation.
 *
 * @param[in] T                 - the type of each object.
 * @param[in] S                 - the number of obecs to allocate.
 */
#if ALLOC == DYNAMIC
#define RLC_ALLOCA(T, S)		(T*) malloc((S) * sizeof(T))
#else
#define RLC_ALLOCA(T, S)		(T*) alloca((S) * sizeof(T))
#endif

#endif

/*
 * Free memory allocated with RLC_ALLOCA.
 *
 * @param[in] A					- the variable to free.
 */
#if ALLOC == DYNAMIC
#define RLC_FREE(A)															\
	if (A != NULL) {														\
		free((void *)A);													\
		A = NULL;															\
	}
#else
#define RLC_FREE(A)         	(void)A;
#endif
