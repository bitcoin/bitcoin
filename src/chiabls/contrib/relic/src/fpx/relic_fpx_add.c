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

void fp2_neg(fp2_t c, fp2_t a) {
	fp_neg(c[0], a[0]);
	fp_neg(c[1], a[1]);
}

#if FPX_CBC == BASIC || !defined(STRIP)

void fp3_add_basic(fp2_t c, fp2_t a, fp2_t b) {
  fp_add(c[0], a[0], b[0]);
  fp_add(c[1], a[1], b[1]);
  fp_add(c[2], a[2], b[2]);
}

void fp3_sub_basic(fp2_t c, fp2_t a, fp2_t b) {
  fp_sub(c[0], a[0], b[0]);
  fp_sub(c[1], a[1], b[1]);
  fp_sub(c[2], a[2], b[2]);
}

void fp3_dbl_basic(fp2_t c, fp2_t a) {
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

void fp3_add_integ(fp2_t c, fp2_t a, fp2_t b) {
	fp3_addm_low(c, a, b);
}

void fp3_sub_integ(fp2_t c, fp2_t a, fp2_t b) {
	fp3_subm_low(c, a, b);
}

void fp3_dbl_integ(fp2_t c, fp2_t a) {
	fp3_dblm_low(c, a);
}

#endif

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
	/* 2 * (a_0 + a_1 * v + a_2 * v^2) = 2 * a_0 + 2 * a_1 * v + 2 * a_2 * v^2. */
	fp2_dbl(c[0], a[0]);
	fp2_dbl(c[1], a[1]);
	fp2_dbl(c[2], a[2]);
}

void fp6_neg(fp6_t c, fp6_t a) {
	fp2_neg(c[0], a[0]);
	fp2_neg(c[1], a[1]);
	fp2_neg(c[2], a[2]);
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

void fp18_add(fp18_t c, fp18_t a, fp18_t b) {
	fp6_add(c[0], a[0], b[0]);
	fp6_add(c[1], a[1], b[1]);
	fp6_add(c[2], a[2], b[2]);
}

void fp18_sub(fp18_t c, fp18_t a, fp18_t b) {
	fp6_sub(c[0], a[0], b[0]);
	fp6_sub(c[1], a[1], b[1]);
	fp6_sub(c[2], a[2], b[2]);
}

void fp18_neg(fp18_t c, fp18_t a) {
	fp6_neg(c[0], a[0]);
	fp6_neg(c[1], a[1]);
	fp6_neg(c[2], a[2]);
}
