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
 * Interface of the memory-management routines for the static memory allocator.
 *
 * @ingroup relic
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef RELIC_POOL_H
#define RELIC_POOL_H

#include "relic_conf.h"
#include "relic_dv.h"
#include "relic_util.h"
#include "relic_label.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

#if ALLOC == STATIC

/**
 * The size of the static pool of digit vectors.
 */
#ifndef POOL_SIZE
#define POOL_SIZE	(MAX(TESTS, MAX(BENCH * BENCH, 10000)))
#endif

/** Indicates that the pool element is already used. */
#define POOL_USED	(1)

/** Indicates that the pool element is free. */
#define POOL_FREE	(0)

/** Indicates that the pool is empty. */
#define POOL_EMPTY (-1)

#endif

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Type that represents a element of a pool of digit vectors.
 */
typedef struct {
	/** Indicates if this pool element is being used. */
	int state;
	/** The pool element. The extra digit stores the pool position. */
	relic_align dig_t elem[DV_DIGS + 1];
} pool_t;

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

#if ALLOC == STATIC

/**
 * Gets a new element from the static pool.
 *
 * @returns the address of a free element in the static pool.
 */
dig_t *pool_get(void);

/**
 * Restores an element to the static pool.
 *
 * @param[in] a			- the address to free.
 */
void pool_put(dig_t *a);

#endif

#endif /* !RELIC_POOL_H */
