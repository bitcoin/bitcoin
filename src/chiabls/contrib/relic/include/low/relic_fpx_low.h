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
 * Interface of the low-level prime extension field arithmetic module.
 *
 * @ingroup fpx
 */

#ifndef RELIC_FPX_LOW_H
#define RELIC_FPX_LOW_H

#include "relic_fpx.h"

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Adds two quadratic extension field elements of the same size.
 * Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp2_addn_low(fp2_t c, fp2_t a, fp2_t b);

/**
 * Adds two quadratic extension field elements of the same size with integrated
 * modular reduction. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp2_addm_low(fp2_t c, fp2_t a, fp2_t b);

/**
 * Adds two double-precision quadratic extension field elements of the same
 * size. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp2_addd_low(dv2_t c, dv2_t a, dv2_t b);

/**
 * Adds two double-precision quadratic extension field elements of the same size
 * and corrects the result by conditionally adding 2^(FP_DIGS * WSIZE) * p.
 * Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp2_addc_low(dv2_t c, dv2_t a, dv2_t b);

/**
 * Subtracts a quadratic extension field element from another of the same size.
 * Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element.
 * @param[in] b				- the field element to subtract.
 */
void fp2_subn_low(fp2_t c, fp2_t a, fp2_t b);

/**
 * Subtracts a quadratic extension field element from another of the same size
 * with integrated modular reduction. Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element.
 * @param[in] b				- the field element to subtract.
 */
void fp2_subm_low(fp2_t c, fp2_t a, fp2_t b);

/**
 * Subtracts a double-precision quadratic extension field element from another
 * of the same size. Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp2_subd_low(dv2_t c, dv2_t a, dv2_t b);

/**
 * Subtracts a double-precision quadratic extension field element from another
 * of the same size and  corrects the result by conditionally adding
 * 2^(FP_DIGS * WSIZE) * p. Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp2_subc_low(dv2_t c, dv2_t a, dv2_t b);

/**
 * Doubles a quadratic extension field element. Computes c = a + a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to multiply.
 */
void fp2_dbln_low(fp2_t c, fp2_t a);

/**
 * Doubles a quadratic extension field element with integrated modular
 * reduction. Computes c = a + a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to double.
 */
void fp2_dblm_low(fp2_t c, fp2_t a);

/**
 * Multiplies a quadratic extension field element by the quadratic/cubic
 * non-residue. Computes c = a * E.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to multiply.
 */
void fp2_norm_low(fp2_t c, fp2_t a);

/**
 * Multiplies a double-precision quadratic extension field element by the
 * quadratic/cubic non-residue, reducing only half of the result. Computes
 * c = a * E.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to multiply.
 */
void fp2_norh_low(dv2_t c, dv2_t a);

/**
 * Multiplies a double-precision quadratic extension field element by the
 * quadratic/cubic non-residue. Computes c = a * E.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to multiply.
 */
void fp2_nord_low(dv2_t c, dv2_t a);

/**
 * Multiplies two quadratic extension field elements of the same size.
 * Computes c = a * b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to multiply.
 * @param[in] b				- the second field element to multiply.
 */
void fp2_muln_low(dv2_t c, fp2_t a, fp2_t b);

/**
 * Multiplies two quadratic extension elements of the same size and corrects
 * the result by adding (2^(FP_DIGS * WSIZE) * p)/4. This function should only
 * be used when the FP_SPACE optimization is detected. Computes c = a * b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to multiply.
 * @param[in] b				- the second field element to multiply.
 */
void fp2_mulc_low(dv2_t c, fp2_t a, fp2_t b);

/**
 * Multiplies two quadratic extension field elements of the same size with
 * embedded modular reduction. Computes c = (a * b) mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to multiply.
 * @param[in] b				- the second field element to multiply.
 */
void fp2_mulm_low(fp2_t c, fp2_t a, fp2_t b);

/**
 * Squares a quadratic extension element. Computes c = a * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to square.
 */
void fp2_sqrn_low(dv2_t c, fp2_t a);

/**
 * Squares a quadratic extension field element with integrated modular
 * reduction. Computes c = (a * a) mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to square.
 */
void fp2_sqrm_low(fp2_t c, fp2_t a);

/**
 * Reduces a quadratic extension element modulo the configured prime p.
 * Computes c = a mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to reduce.
 */
void fp2_rdcn_low(fp2_t c, dv2_t a);

/**
 * Adds two cubic extension field elements of the same size.
 * Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp3_addn_low(fp3_t c, fp3_t a, fp3_t b);

/**
 * Adds two cubic extension field elements of the same size with integrated
 * modular reduction. Computes c = a + b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp3_addm_low(fp3_t c, fp3_t a, fp3_t b);

/**
 * Subtracts a cubic extension field element from another of the same size.
 * Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element.
 * @param[in] b				- the field element to subtract.
 */
void fp3_subn_low(fp3_t c, fp3_t a, fp3_t b);

/**
 * Subtracts a cubic extension field element from another of the same size
 * with integrated modular reduction. Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element.
 * @param[in] b				- the field element to subtract.
 */
void fp3_subm_low(fp3_t c, fp3_t a, fp3_t b);

/**
 * Subtracts a double-precision cubic extension field element from another
 * of the same size and corrects the result by conditionally adding
 * 2^(FP_DIGS * WSIZE) * p. Computes c = a - b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to add.
 * @param[in] b				- the second field element to add.
 */
void fp3_subc_low(dv3_t c, dv3_t a, dv3_t b);

/**
 * Doubles a cubic extension field element. Computes c = a + a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to multiply.
 */
void fp3_dbln_low(fp3_t c, fp3_t a);

/**
 * Doubles a cubic extension field element with integrated modular
 * reduction. Computes c = a + a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to double.
 */
void fp3_dblm_low(fp3_t c, fp3_t a);

/**
 * Multiplies two cubic extension field elements of the same size.
 * Computes c = a * b.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to multiply.
 * @param[in] b				- the second field element to multiply.
 */
void fp3_muln_low(dv3_t c, fp3_t a, fp3_t b);

/**
 * Multiplies two cubic extension field elements of the same size with
 * embedded modular reduction. Computes c = (a * b) mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first field element to multiply.
 * @param[in] b				- the second field element to multiply.
 */
void fp3_mulm_low(fp3_t c, fp3_t a, fp3_t b);

/**
 * Squares a cubic extension element. Computes c = a * a.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to square.
 */
void fp3_sqrn_low(dv2_t c, fp3_t a);

/**
 * Squares a cubic extension field element with integrated modular
 * reduction. Computes c = (a * a) mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the field element to square.
 */
void fp3_sqrm_low(fp3_t c, fp3_t a);

/**
 * Reduces a cubic extension element modulo the configured prime p.
 * Computes c = a mod p.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the digit vector to reduce.
 */
void fp3_rdcn_low(fp3_t c, dv3_t a);

#endif /* !RELIC_FPX_LOW_H */
