/*
 * Copyright 2007-2009 RELIC Project
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file.
 *
 * RELIC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of low-level prime field squaring functions.
 *
 * @ingroup fp
 */

#include "relic_conf.h"
#include "relic_fp_low.h"

#ifdef MSP430X
#include <msp430.h>
#else

/* For some reason these are not defined in asm */
#define MPY                0x0130  /* Multiply Unsigned/Operand 1 */
#define MPYS               0x0132  /* Multiply Signed/Operand 1 */
#define MAC                0x0134  /* Multiply Unsigned and Accumulate/Operand 1 */
#define MACS               0x0136  /* Multiply Signed and Accumulate/Operand 1 */
#define OP2                0x0138  /* Operand 2 */
#define RESLO              0x013A  /* Result Low Word */
#define RESHI              0x013C  /* Result High Word */
#define SUMEXT             0x013E  /* Sum Extend */

#endif

.text
.align 2

.global fp_sqrn_low


#if FP_PRIME <= 160 && FP_PRIME > (160-16)

#include "relic_fp_sqr_low_160.s"

#elif FP_PRIME <= 256 && FP_PRIME > (256 - 16)

#include "relic_fp_sqr_low_256.s"

#else

#error Unsupported field size

#endif
