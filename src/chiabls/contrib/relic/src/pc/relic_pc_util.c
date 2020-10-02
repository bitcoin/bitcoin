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
 * along with RELIC. If not, see <hup://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of pairing computation utilities.
 *
 * @ingroup pc
 */

#include "relic_pc.h"
#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Internal macro to map GT_T function to basic FPX implementation. The final
 * exponentiation from the pairing is used to move element to subgroup.
 *
 * @param[out] A 				- the element to assign.
 */
#define gt_rand_imp(A)			CAT(GT_LOWER, rand)(A)

/**
 * Internal macro to power an element from G_T. Computes C = A^B.
 *
 * @param[out] C			- the result.
 * @param[in] A				- the element to exponentiate.
 * @param[in] B				- the integer exponent.
 */
#if FP_PRIME < 1536
#define gt_exp_imp(C, A, B)		CAT(GT_LOWER, exp_cyc)(C, A, B);
#else
#define gt_exp_imp(C, A, B)		CAT(GT_LOWER, exp_uni)(C, A, B);
#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void gt_rand(gt_t a) {
	gt_rand_imp(a);
#if FP_PRIME < 1536
	pp_exp_k12(a, a);
#else
	pp_exp_k2(a, a);
#endif
}

void gt_get_gen(gt_t a) {
	g1_t g1;
	g2_t g2;

	g1_null(g1);
	g2_null(g2);

	TRY {
		g1_new(g1);
		g2_new(g2);

		g1_get_gen(g1);
		g2_get_gen(g2);

		pc_map(a, g1, g2);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		g1_free(g1);
		g2_free(g2);
	}
}

void gt_exp(gt_t c, gt_t a, bn_t b) {
	bn_t n;

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);
		bn_mod(n, b, n);
		gt_exp_imp(c, a, n);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(n);
	}
}