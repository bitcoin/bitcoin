/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2019 RELIC Authors
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
 * Implementation of the low-level prime field addition and subtraction
 * functions.
 *
 * @ingroup fp
 */

#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                         */
/*============================================================================*/

#include "bls12_381_q_64.c"

uint64_t one[] = { 0x760900000002FFFD, 0xEBF4000BC40C0002, 0x5F48985753C758BA,
                   0x77CE585370525745, 0x5C071A97A256EC6D, 0x15F65EC3FA80E493 };
uint64_t pre[] = { 0x58B20FBD4742924F, 0x60DC92E7F4C2F437, 0x3F4FAC6A70C73C60,
                   0xED47D0696C7BF023, 0x2A13C3E0FD09A8CB, 0x1397424C770094FF };

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_invm_low(dig_t *c, const dig_t *a) {
	/* Compute number of iteratios based on modulus size. */
#if FP_PRIME < 46
	int i, iterations = (49 * 381 + 80)/17;
#else
	int i, iterations = (49 * 381 + 57)/17;
#endif

	dig_t out1, out2[6], out3[6], out4[6], out5[6];
	dig_t f[6] = { 0 }, g[6], h[6], v[6] = { 0 }, r[6], d = 1;

	/* v = 0, f = p, r = 1 in Montgomery form */
	dv_copy(r, one, RLC_FP_DIGS);
	dv_copy(f, fp_prime_get(), RLC_FP_DIGS);
	/* Convert a from Montgomery form. */
	fiat_bls12_381_q_from_montgomery(g, a);

	for (i = 0; i < iterations - (iterations % 2); i+=2) {
		fiat_bls12_381_q_divstep(&out1,out2,out3,out4,out5,d,f,g,v,r);
		fiat_bls12_381_q_divstep(&d,f,g,v,r,out1,out2,out3,out4,out5);
    }
    fiat_bls12_381_q_divstep(&out1,out2,out3,out4,out5,d,f,g,v,r);

	fiat_bls12_381_q_opp(h, out4);
	fiat_bls12_381_q_selectznz(v, out2[RLC_FP_DIGS - 1] >> 63, out4, h);
	fiat_bls12_381_q_mul(c, v, pre);
}
