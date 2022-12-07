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
 * Implementation of the binary field modulus manipulation.
 *
 * @ingroup fb
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_dv.h"
#include "relic_fb.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fb_param_get(void) {
	return core_get()->fb_id;
}

void fb_param_set(int param) {
	switch (param) {
		case PENTA_8:
			fb_poly_set_penta(4, 3, 2);
			break;
		case PENTA_64:
			fb_poly_set_penta(4, 3, 1);
			break;
		case TRINO_113:
			fb_poly_set_trino(9);
			break;
		case TRINO_127:
			fb_poly_set_trino(63);
			break;
		case PENTA_128:
			fb_poly_set_penta(7, 2, 1);
			break;
		case PENTA_131:
			fb_poly_set_penta(13, 2, 1);
			break;
		case SQRT_163:
			fb_poly_set_penta(57, 49, 29);
			break;
		case NIST_163:
			fb_poly_set_penta(7, 6, 3);
			break;
		case TRINO_193:
			fb_poly_set_trino(15);
			break;
		case SQRT_233:
			fb_poly_set_trino(159);
			break;
		case NIST_233:
			fb_poly_set_trino(74);
			break;
		case SQRT_239:
			fb_poly_set_trino(81);
			break;
		case SECG_239:
			fb_poly_set_trino(158);
			break;
		case TRINO_257:
			fb_poly_set_trino(41);
			break;
		case SQRT_251:
			fb_poly_set_penta(89, 81, 3);
			break;
		case PENTA_251:
			fb_poly_set_penta(7, 4, 2);
			break;
		case TRINO_271:
			fb_poly_set_trino(201);
			break;
		case PENTA_271:
			fb_poly_set_penta(207, 175, 111);
			break;
		case SQRT_283:
			fb_poly_set_penta(97, 89, 87);
			break;
		case NIST_283:
			fb_poly_set_penta(12, 7, 5);
			break;
		case TRINO_353:
#if WSIZE == 8
			fb_poly_set_trino(69);
#else
			fb_poly_set_trino(95);
#endif
			break;
		case TRINO_367:
			fb_poly_set_trino(21);
			break;
		case NIST_409:
			fb_poly_set_trino(87);
			break;
		case TRINO_439:
			fb_poly_set_trino(49);
			break;
		case SQRT_571:
			fb_poly_set_penta(193, 185, 5);
			break;
		case NIST_571:
			fb_poly_set_penta(10, 5, 2);
			break;
		case TRINO_1223:
			fb_poly_set_trino(255);
			break;
		default:
			RLC_THROW(ERR_NO_VALID);
			break;
	}
	core_get()->fb_id = param;
}

void fb_param_set_any(void) {
#if FB_POLYN == 8
	fb_param_set(PENTA_8);

#elif FB_POLYN == 64
	fb_param_set(PENTA_64);

#elif FB_POLYN == 113
	fb_param_set(TRINO_113);

#elif FB_POLYN == 127
	fb_param_set(TRINO_127);

#elif FB_POLYN == 128
	fb_param_set(PENTA_128);

#elif FB_POLYN == 131
	fb_param_set(PENTA_131);

#elif FB_POLYN == 163
#ifdef FB_SQRTF
	fb_param_set(SQRT_163);
#else
	fb_param_set(NIST_163);
#endif

#elif FB_POLYN == 193
	fb_param_set(TRINO_193);

#elif FB_POLYN == 233
#ifdef FB_SQRTF
	fb_param_set(SQRT_233);
#else
	fb_param_set(NIST_233);
#endif

#elif FB_POLYN == 239
#ifdef FB_SQRTF
	fb_param_set(SQRT_239);
#else
	fb_param_set(SECG_239);
#endif

#elif FB_POLYN == 251
#ifdef FB_SQRTF
	fb_param_set(SQRT_251);
#else
	fb_param_set(PENTA_251);
#endif

#elif FB_POLYN == 257
	fb_param_set(TRINO_257);

#elif FB_POLYN == 271
#ifdef FB_TRINO
	fb_param_set(TRINO_271);
#else
	fb_param_set(PENTA_271);
#endif

#elif FB_POLYN == 283
#ifdef FB_SQRTF
	fb_param_set(SQRT_283);
#else
	fb_param_set(NIST_283);
#endif

#elif FB_POLYN == 353
	fb_param_set(TRINO_353);

#elif FB_POLYN == 367
	fb_param_set(TRINO_367);

#elif FB_POLYN == 409
	fb_param_set(NIST_409);

#elif FB_POLYN == 439
	fb_param_set(TRINO_439);

#elif FB_POLYN == 571
#ifdef FB_SQRTF
	fb_param_set(SQRT_571);
#else
	fb_param_set(NIST_571);
#endif

#elif FB_POLYN == 1223
	fb_param_set(TRINO_1223);
#else
	RLC_THROW(ERR_NO_FIELD);
#endif
}

void fb_param_print(void) {
	int fa, fb, fc;

	fb_poly_get_rdc(&fa, &fb, &fc);

	if (fb == 0) {
		util_banner("Irreducible trinomial:", 0);
		util_print("   z^%d + z^%d + 1\n", RLC_FB_BITS, fa);
	} else {
		util_banner("Irreducible pentanomial:", 0);
		util_print("   z^%d + z^%d + z^%d + z^%d + 1\n", RLC_FB_BITS, fa, fb, fc);
	}
}
