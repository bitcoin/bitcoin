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
 * Implementation of useful configuration routines.
 *
 * @ingroup relic
 */

#include <stdio.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_bn.h"
#include "relic_fp.h"
#include "relic_fb.h"
#include "relic_ep.h"
#include "relic_eb.h"
#include "relic_ed.h"
#include "relic_ec.h"
#include "relic_pc.h"
#include "relic_bench.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Inserts quotation marks in the macro argument.
 */
/** @{ */
#define QUOTE(A)			_QUOTE(A)
#define _QUOTE(A)			#A
/** @} */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void conf_print(void) {
#ifndef QUIET
	util_print("-- RELIC " RLC_VERSION " configuration:\n\n");
#if ALLOC == DYNAMIC
	util_print("** Allocation mode: DYNAMIC\n\n");
#elif ALLOC == AUTO
	util_print("** Allocation mode: AUTO\n\n");
#endif

#if ARITH == EASY
	util_print("** Arithmetic backend: easy\n\n");
#elif ARITH == GMP
	util_print("** Arithmetic backend: gmp\n\n");
#elif ARITH == GMP_SEC
	util_print("** Arithmetic backend: gmp-sec\n\n");
#else
	util_print("** Arithmetic backend: " QUOTE(ARITH) "\n\n");
#endif

#ifdef LABEL
	util_print("** Configured label: " QUOTE(LABEL) "\n\n");
#endif

#if BENCH > 1
	util_print("** Benchmarking options:\n");
	util_print("   Number of times: %d\n", BENCH * BENCH);
#ifdef OVERH
	util_print("   Estimated overhead: ");
	bench_overhead();
#endif
	util_print("\n");
#endif

#ifdef WITH_BN
	util_print("** Multiple precision module options:\n");
	util_print("   Precision: %d bits, %d words\n", RLC_BN_BITS, RLC_BN_DIGS);
	util_print("   Arithmetic method: " BN_METHD "\n\n");
#endif

#ifdef WITH_FP
	util_print("** Prime field module options:\n");
	util_print("   Prime size: %d bits, %d words\n", RLC_FP_BITS, RLC_FP_DIGS);
	util_print("   Arithmetic method: " FP_METHD "\n\n");
#endif

#ifdef WITH_FPX
	util_print("** Prime field extension module options:\n");
	util_print("   Arithmetic method: " FPX_METHD "\n\n");
#endif

#ifdef WITH_EP
	util_print("** Prime elliptic curve module options:\n");
	util_print("   Arithmetic method: " EP_METHD "\n\n");
#endif

#ifdef WITH_PP
	util_print("** Bilinear pairing module options:\n");
	util_print("   Arithmetic method: " PP_METHD "\n\n");
#endif

#ifdef WITH_FB
	util_print("** Binary field module options:\n");
	util_print("   Polynomial size: %d bits, %d words\n", RLC_FB_BITS, RLC_FB_DIGS);
	util_print("   Arithmetic method: " FB_METHD "\n\n");
#endif

#ifdef WITH_EB
	util_print("** Binary elliptic curve module options:\n");
	util_print("   Arithmetic method: " EB_METHD "\n\n");
#endif

#ifdef WITH_EC
	util_print("** Elliptic Curve Cryptography module options:\n");
	util_print("   Arithmetic method: " EC_METHD "\n\n");
#endif

#ifdef WITH_ED
	util_print("** Edwards Curve Cryptography module options:\n");
	util_print("   Arithmetic method: " ED_METHD "\n\n");
#endif

#ifdef WITH_MD
	util_print("** Hash function module options:\n");
	util_print("   Chosen method: " MD_METHD "\n\n");
#endif

#endif
}
