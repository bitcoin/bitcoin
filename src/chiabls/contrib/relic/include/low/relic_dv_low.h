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
 * Interface of the low-level digit vector module.
 *
 * @ingroup dv
 */

#ifndef RELIC_DV_LOW_H
#define RELIC_DV_LOW_H

#include "relic_bn_low.h"
#include "relic_fb_low.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

#ifdef ASM

/**
 * Size in digits of a squaring result in a prime field.
 */
#ifdef WITH_FP
#define DV_FP	(2 * FP_DIGS + 1)
#else
#define DV_FP	(0)
#endif

/**
 * Size in digits of a squaring result in a binary field.
 */
#ifdef WITH_FB
#define DV_FB	(2 * FB_DIGS)
#else
#define DV_FB	(0)
#endif

/**
 * Size in digits of a temporary vector.
 *
 * A temporary vector has enough size to store a multiplication/squaring/cubing
 * result in any finite field.
 */
#if DV_FB > DV_FP
#define DV_DIGS	DV_FB
#else
#define DV_DIGS	DV_FP
#endif

#if BN_SIZE > DV_DIGS
#undef DV_DIGS
#define DV_DIGS BN_SIZE
#endif

#endif /* ASM */

#endif /* !RELIC_DV_LOW_H */
