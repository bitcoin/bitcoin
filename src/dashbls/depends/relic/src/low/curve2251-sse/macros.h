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
 * Useful macros for binary field arithmetic.
 *
 * @version $Id$
 * @ingroup fb
 */

#define PSHUFB(A, B)		_mm_shuffle_epi8(A, B);
#define SHL64(A, B)			_mm_slli_epi64(A, B)
#define SHR64(A, B)			_mm_srli_epi64(A, B)
#define XOR(A, B)			_mm_xor_si128(A, B)
#define SHL8(A, B)			_mm_slli_si128(A, B)
#define SHR8(A, B)			_mm_srli_si128(A, B)
#define AND(A, B)			_mm_and_si128(A, B)

#define MUL(ma, mb) \
	t0 = _mm_clmulepi64_si128(ma, mb, 0x00);\
	t1 = _mm_clmulepi64_si128(ma, mb, 0x11);\
	t2 = XOR(SHR8(ma, 8), ma);\
	t3 = XOR(SHR8(mb, 8), mb);\
	t2 = _mm_clmulepi64_si128(t2, t3, 0x00);\
	t2 = XOR(t2, t0);\
	t2 = XOR(t2, t1);\
	t3 = SHR8(t2, 8);\
	t2 = SHL8(t2, 8);\
	t0 = XOR(t0, t2);\
	t1 = XOR(t1, t3);\

#define MULDXS(ma, mb)	\
	t0 = _mm_clmulepi64_si128(ma, mb, 0x00);\
	t2 = _mm_clmulepi64_si128(ma, mb, 0x01);\
	t1 = SHR8(t2, 8);\
	t2 = SHL8(t2, 8);\
	t0 = XOR(t0, t2);\

#define MULSXD(ma, mb)	\
	MULDXS(mb, ma)

#define RED251(t,m1,m0)\
	t0 = _mm_slli_si128(t,8);\
	t1 = _mm_srli_si128(t,8);\
	m1 = _mm_xor_si128(m1,_mm_srli_epi64(t1,59));\
	m1 = _mm_xor_si128(m1,_mm_srli_epi64(t1,57));\
	m1 = _mm_xor_si128(m1,_mm_srli_epi64(t1,55));\
	m1 = _mm_xor_si128(m1,_mm_srli_epi64(t1,52));\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(t0,59));\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(t0,57));\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(t0,55));\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(t0,52));\
	t0 = _mm_srli_si128(t0,8);\
	t1 = _mm_slli_si128(t1,8);\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t0,5));\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t0,7));\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t0,9));\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t0,12));\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t1,5));\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t1,7));\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t1,9));\
	m0 = _mm_xor_si128(m0,_mm_slli_epi64(t1,12));

#define REDUCE()														\
	RED251(m3,m2,m1);													\
	RED251(m2,m1,m0);													\
	m8 = _mm_srli_si128(m1,8);											\
	m9 = _mm_srli_epi64(m8,59);											\
	m9 = _mm_slli_epi64(m9,59);											\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,59));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,57));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,55));						\
	m0 = _mm_xor_si128(m0,_mm_srli_epi64(m9,52));						\

