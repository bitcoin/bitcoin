/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
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

#include "relic_dv_low.h"

#include "macro.s"

.global cdecl(fp2_rdcn_low)

cdecl(fp2_rdcn_low):
	push %r12
	push %r13
	push %r14

	FP_RDCN_LOW %rdi, %rsi
	addq $(8*RLC_FP_DIGS), %rdi
	addq $(8*RLC_DV_DIGS), %rsi
	FP_RDCN_LOW %rdi, %rsi

	pop %r14
	pop %r13
	pop %r12
	ret
