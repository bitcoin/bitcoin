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
 * Implementation of addition in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FPX_QDR == BASIC || !defined(STRIP)

void fp2_add_basic(fp2_t c, fp2_t a, fp2_t b) {
  fp_add(c[0], a[0], b[0]);
  fp_add(c[1], a[1], b[1]);
}

void fp2_sub_basic(fp2_t c, fp2_t a, fp2_t b) {
  fp_sub(c[0], a[0], b[0]);
  fp_sub(c[1], a[1], b[1]);
}

void fp2_dbl_basic(fp2_t c, fp2_t a) {
  /* 2 * (a_0 + a_1 * u) = 2 * a_0 + 2 * a_1 * u. */
  fp_dbl(c[0], a[0]);
  fp_dbl(c[1], a[1]);
}

#endif

#if FPX_QDR == INTEG || !defined(STRIP)

void fp2_add_integ(fp2_t c, fp2_t a, fp2_t b) {
	fp2_addm_low(c, a, b);
}

void fp2_sub_integ(fp2_t c, fp2_t a, fp2_t b) {
	fp2_subm_low(c, a, b);
}

void fp2_dbl_integ(fp2_t c, fp2_t a) {
	fp2_dblm_low(c, a);
}

#endif

void fp2_add_dig(fp2_t c, const fp2_t a, dig_t dig) {
	fp_add_dig(c[0], a[0], dig);
	fp_copy(c[1], a[1]);
}

void fp2_sub_dig(fp2_t c, const fp2_t a, dig_t dig) {
	fp_sub_dig(c[0], a[0], dig);
	fp_copy(c[1], a[1]);
}

void fp2_neg(fp2_t c, fp2_t a) {
	fp_neg(c[0], a[0]);
	fp_neg(c[1], a[1]);
}

#if FPX_CBC == BASIC || !defined(STRIP)

void fp3_add_basic(fp3_t c, fp3_t a, fp3_t b) {
  fp_add(c[0], a[0], b[0]);
  fp_add(c[1], a[1], b[1]);
  fp_add(c[2], a[2], b[2]);
}

void fp3_sub_basic(fp3_t c, fp3_t a, fp3_t b) {
  fp_sub(c[0], a[0], b[0]);
  fp_sub(c[1], a[1], b[1]);
  fp_sub(c[2], a[2], b[2]);
}

void fp3_dbl_basic(fp3_t c, fp3_t a) {
  /* 2 * (a_0 + a_1 * u) = 2 * a_0 + 2 * a_1 * u. */
  fp_dbl(c[0], a[0]);
  fp_dbl(c[1], a[1]);
  fp_dbl(c[2], a[2]);
}

#endif

void fp3_neg(fp3_t c, fp3_t a) {
	fp_neg(c[0], a[0]);
	fp_neg(c[1], a[1]);
	fp_neg(c[2], a[2]);
}

#if FPX_CBC == INTEG || !defined(STRIP)

void fp3_add_integ(fp3_t c, fp3_t a, fp3_t b) {
	fp3_addm_low(c, a, b);
}

void fp3_sub_integ(fp3_t c, fp3_t a, fp3_t b) {
	fp3_subm_low(c, a, b);
}

void fp3_dbl_integ(fp3_t c, fp3_t a) {
	fp3_dblm_low(c, a);
}

#endif

void fp4_add(fp4_t c, fp4_t a, fp4_t b) {
	fp2_add(c[0], a[0], b[0]);
	fp2_add(c[1], a[1], b[1]);
}

void fp4_sub(fp4_t c, fp4_t a, fp4_t b) {
	fp2_sub(c[0], a[0], b[0]);
	fp2_sub(c[1], a[1], b[1]);
}

void fp4_dbl(fp4_t c, fp4_t a) {
	/* 2 * (a_0 + a_1 * v + a_2 * v^2) = 2 * a_0 + 2 * a_1 * v + 2 * a_2 * v^2. */
	fp2_dbl(c[0], a[0]);
	fp2_dbl(c[1], a[1]);
}

void fp4_neg(fp4_t c, fp4_t a) {
	fp2_neg(c[0], a[0]);
	fp2_neg(c[1], a[1]);
}

void fp6_add(fp6_t c, fp6_t a, fp6_t b) {
	fp2_add(c[0], a[0], b[0]);
	fp2_add(c[1], a[1], b[1]);
	fp2_add(c[2], a[2], b[2]);
}

void fp6_sub(fp6_t c, fp6_t a, fp6_t b) {
	fp2_sub(c[0], a[0], b[0]);
	fp2_sub(c[1], a[1], b[1]);
	fp2_sub(c[2], a[2], b[2]);
}

void fp6_dbl(fp6_t c, fp6_t a) {
	fp2_dbl(c[0], a[0]);
	fp2_dbl(c[1], a[1]);
	fp2_dbl(c[2], a[2]);
}

void fp6_neg(fp6_t c, fp6_t a) {
	fp2_neg(c[0], a[0]);
	fp2_neg(c[1], a[1]);
	fp2_neg(c[2], a[2]);
}

void fp8_add(fp8_t c, fp8_t a, fp8_t b) {
	fp4_add(c[0], a[0], b[0]);
	fp4_add(c[1], a[1], b[1]);
}

void fp8_sub(fp8_t c, fp8_t a, fp8_t b) {
	fp4_sub(c[0], a[0], b[0]);
	fp4_sub(c[1], a[1], b[1]);
}

void fp8_dbl(fp8_t c, fp8_t a) {
	fp4_dbl(c[0], a[0]);
	fp4_dbl(c[1], a[1]);
}

void fp8_neg(fp8_t c, fp8_t a) {
	fp4_neg(c[0], a[0]);
	fp4_neg(c[1], a[1]);
}

void fp9_add(fp9_t c, fp9_t a, fp9_t b) {
	fp3_add(c[0], a[0], b[0]);
	fp3_add(c[1], a[1], b[1]);
	fp3_add(c[2], a[2], b[2]);
}

void fp9_sub(fp9_t c, fp9_t a, fp9_t b) {
	fp3_sub(c[0], a[0], b[0]);
	fp3_sub(c[1], a[1], b[1]);
	fp3_sub(c[2], a[2], b[2]);
}

void fp9_dbl(fp9_t c, fp9_t a) {
	fp3_dbl(c[0], a[0]);
	fp3_dbl(c[1], a[1]);
	fp3_dbl(c[2], a[2]);
}

void fp9_neg(fp9_t c, fp9_t a) {
	fp3_neg(c[0], a[0]);
	fp3_neg(c[1], a[1]);
	fp3_neg(c[2], a[2]);
}

void fp12_add(fp12_t c, fp12_t a, fp12_t b) {
	fp6_add(c[0], a[0], b[0]);
	fp6_add(c[1], a[1], b[1]);
}

void fp12_sub(fp12_t c, fp12_t a, fp12_t b) {
	fp6_sub(c[0], a[0], b[0]);
	fp6_sub(c[1], a[1], b[1]);
}

void fp12_neg(fp12_t c, fp12_t a) {
	fp6_neg(c[0], a[0]);
	fp6_neg(c[1], a[1]);
}

void fp12_dbl(fp12_t c, fp12_t a) {
	fp6_dbl(c[0], a[0]);
	fp6_dbl(c[1], a[1]);
}

void fp18_add(fp18_t c, fp18_t a, fp18_t b) {
	fp9_add(c[0], a[0], b[0]);
	fp9_add(c[1], a[1], b[1]);
}

void fp18_sub(fp18_t c, fp18_t a, fp18_t b) {
	fp9_sub(c[0], a[0], b[0]);
	fp9_sub(c[1], a[1], b[1]);
}

void fp18_dbl(fp18_t c, fp18_t a) {
	fp9_dbl(c[0], a[0]);
	fp9_dbl(c[1], a[1]);
}

void fp18_neg(fp18_t c, fp18_t a) {
	fp9_neg(c[0], a[0]);
	fp9_neg(c[1], a[1]);
}

void fp24_add(fp24_t c, fp24_t a, fp24_t b) {
	fp8_add(c[0], a[0], b[0]);
	fp8_add(c[1], a[1], b[1]);
    fp8_add(c[2], a[2], b[2]);
}

void fp24_sub(fp24_t c, fp24_t a, fp24_t b) {
	fp8_sub(c[0], a[0], b[0]);
	fp8_sub(c[1], a[1], b[1]);
    fp8_sub(c[2], a[2], b[2]);
}

void fp24_neg(fp24_t c, fp24_t a) {
	fp8_neg(c[0], a[0]);
	fp8_neg(c[1], a[1]);
    fp8_neg(c[2], a[2]);
}

void fp24_dbl(fp24_t c, fp24_t a) {
	fp8_dbl(c[0], a[0]);
	fp8_dbl(c[1], a[1]);
    fp8_dbl(c[2], a[2]);
}

void fp48_add(fp48_t c, fp48_t a, fp48_t b) {
	fp24_add(c[0], a[0], b[0]);
	fp24_add(c[1], a[1], b[1]);
}

void fp48_sub(fp48_t c, fp48_t a, fp48_t b) {
	fp24_sub(c[0], a[0], b[0]);
	fp24_sub(c[1], a[1], b[1]);
}

void fp48_neg(fp48_t c, fp48_t a) {
	fp24_neg(c[0], a[0]);
	fp24_neg(c[1], a[1]);
}

void fp48_dbl(fp48_t c, fp48_t a) {
	fp24_dbl(c[0], a[0]);
	fp24_dbl(c[1], a[1]);
}

void fp54_add(fp54_t c, fp54_t a, fp54_t b) {
	fp18_add(c[0], a[0], b[0]);
	fp18_add(c[1], a[1], b[1]);
    fp18_add(c[2], a[2], b[2]);
}

void fp54_sub(fp54_t c, fp54_t a, fp54_t b) {
	fp18_sub(c[0], a[0], b[0]);
	fp18_sub(c[1], a[1], b[1]);
    fp18_sub(c[2], a[2], b[2]);
}

void fp54_neg(fp54_t c, fp54_t a) {
	fp18_neg(c[0], a[0]);
	fp18_neg(c[1], a[1]);
    fp18_neg(c[2], a[2]);
}

void fp54_dbl(fp54_t c, fp54_t a) {
	fp18_dbl(c[0], a[0]);
	fp18_dbl(c[1], a[1]);
    fp18_dbl(c[2], a[2]);
}
