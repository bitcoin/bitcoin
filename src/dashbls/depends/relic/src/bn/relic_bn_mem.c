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
 * Implementation of the multiple precision integer memory management routines.
 *
 * @ingroup bn
 */

#include <errno.h>

#if ALLOC != AUTO
#include <malloc.h>
#endif

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_make(bn_t a, int digits) {
	if (digits < 0) {
		RLC_THROW(ERR_NO_VALID);
	}
	/* Allocate at least one digit. */
	digits = RLC_MAX(digits, 1);

#if ALLOC == DYNAMIC
	if (digits % RLC_BN_SIZE != 0) {
		/* Pad the number of digits to a multiple of the block. */
		digits += (RLC_BN_SIZE - digits % RLC_BN_SIZE);
	}

	if (a != NULL) {
		a->dp = NULL;
#if ALIGN == 1
		a->dp = (dig_t *)malloc(digits * sizeof(dig_t));
#elif OPSYS == WINDOWS
		a->dp = _aligned_malloc(digits * sizeof(dig_t), ALIGN);
#else
		int r = posix_memalign((void **)&a->dp, ALIGN, digits * sizeof(dig_t));
		if (r == ENOMEM) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		if (r == EINVAL) {
			RLC_THROW(ERR_NO_VALID);
		}
#endif /* ALIGN */
	}

	if (a->dp == NULL) {
		free((void *)a);
		RLC_THROW(ERR_NO_MEMORY);
	}
#else
	/* Verify if the number of digits is sane. */
	if (digits > RLC_BN_SIZE) {
		RLC_THROW(ERR_NO_PRECI);
		return;
	} else {
		digits = RLC_BN_SIZE;
	}
#endif
	if (a != NULL) {
		a->used = 1;
		a->dp[0] = 0;
		a->alloc = digits;
		a->sign = RLC_POS;
	}
}

void bn_clean(bn_t a) {
#if ALLOC == DYNAMIC
	if (a != NULL) {
		if (a->dp != NULL) {
#if OPSYS == WINDOWS && ALIGN > 1
			_aligned_free(a->dp);
#else
			free(a->dp);
#endif
			a->dp = NULL;
		}
		a->alloc = 0;
	}
#endif
	if (a != NULL) {
		a->used = 0;
		a->sign = RLC_POS;
	}
}

void bn_grow(bn_t a, int digits) {
#if ALLOC == DYNAMIC
	dig_t *t;

	if (a->alloc < digits) {
		/* At least add RLC_BN_SIZE more digits. */
		digits += (RLC_BN_SIZE * 2) - (digits % RLC_BN_SIZE);
		t = (dig_t *)realloc(a->dp, (RLC_DIG / 8) * digits);
		if (t == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
			return;
		}
		a->dp = t;
		/* Set the newly allocated digits to zero. */
		a->alloc = digits;
	}
#elif ALLOC == AUTO
	if (digits > RLC_BN_SIZE) {
		RLC_THROW(ERR_NO_PRECI);
		return;
	}
	(void)a;
#endif
}

void bn_trim(bn_t a) {
	if (a->used <= a->alloc) {
		while (a->used > 0 && a->dp[a->used - 1] == 0) {
			--(a->used);
		}
		/* Zero can't be negative. */
		if (a->used <= 0) {
			a->used = 1;
			a->dp[0] = 0;
			a->sign = RLC_POS;
		}
	}
}
