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
 * @defgroup util Misc utilities
 */

/**
 * @file
 *
 * Interface of misc utilitles.
 *
 * @ingroup util
 */

#ifndef RLC_UTIL_H
#define RLC_UTIL_H

#include "relic_arch.h"
#include "relic_types.h"
#include "relic_label.h"

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Returns the minimum between two numbers.
 *
 * @param[in] A		- the first number.
 * @param[in] B		- the second number.
 */
#define RLC_MIN(A, B)			((A) < (B) ? (A) : (B))

/**
 * Returns the maximum between two numbers.
 *
 * @param[in] A		- the first number.
 * @param[in] B		- the second number.
 */
#define RLC_MAX(A, B)			((A) > (B) ? (A) : (B))

/**
 * Splits a bit count in a digit count and an updated bit count.
 *
 * @param[out] B		- the resulting bit count.
 * @param[out] D		- the resulting digit count.
 * @param[out] V		- the bit count.
 */
#define RLC_RIP(B, D, V)													\
	D = (V) >> (RLC_DIG_LOG); B = (V) - ((D) * (1 << RLC_DIG_LOG));

/**
 * Computes the ceiling function of an integer division.
 *
 * @param[in] A			- the dividend.
 * @param[in] B			- the divisor.
 */
#define RLC_CEIL(A, B)			(((A) - 1) / (B) + 1)

/**
 * Returns a bit mask to isolate the lowest part of a digit.
 *
 * @param[in] B			- the number of bits to isolate.
 */
#define RLC_MASK(B)															\
	((-(dig_t)((B) >= WSIZE)) | (((dig_t)1 << ((B) % WSIZE)) - 1))

/**
 * Returns a bit mask to isolate the lowest half of a digit.
 */
#define RLC_LMASK				(RLC_MASK(RLC_DIG >> 1))

/**
 * Returns a bit mask to isolate the highest half of a digit.
 */
#define RLC_HMASK				(RLC_LMASK << (RLC_DIG >> 1))

/**
 * Bit mask used to return an entire digit.
 */
#define RLC_DMASK				(RLC_HMASK | RLC_LMASK)

/**
 * Returns the lowest half of a digit.
 *
 * @param[in] D			- the digit.
 */
#define RLC_LOW(D)				(D & RLC_LMASK)

/**
 * Returns the highest half of a digit.
 *
 * @param[in] D			- the digit.
 */
#define RLC_HIGH(D)				(D >> (RLC_DIG >> 1))

/**
 * Selects between two values based on the value of a given flag.
 *
 * @param[in] A			- the first argument.
 * @param[in] B			- the second argument.
 * @param[in] C			- the selection flag.
 */
#define RLC_SEL(A, B, C) 		((-(C) & ((A) ^ (B))) ^ (A))

/**
 * Swaps two values.
 *
 * @param[in] A			- the first argument.
 * @param[in] B			- the second argument.
 */
#define RLC_SWAP(A, B) 			((A) ^= (B), (B) ^= (A), (A) ^= (B))

/**
 * Returns the given character in upper case.
 *
 * @param[in] C			- the character.
 */
#define RLC_UPP(C)				((C) - 0x20 * (((C) >= 'a') && ((C) <= 'z')))

/**
  *  Indirection to help some compilers expand symbols.
  */
#define RLC_ECHO(A) 			A

/**
 * Concatenates two tokens.
 */
/** @{ */
#define RLC_CAT(A, B)			_RLC_CAT(A, B)
#define _RLC_CAT(A, B)			A ## B
/** @} */

/**
 * Selects a basic or advanced version of a function by checking if an
 * additional argument was passed.
 */
/** @{ */
#define RLC_OPT(...)			_OPT(__VA_ARGS__, _imp, _basic, _error)
#define _OPT(...)				RLC_ECHO(__OPT(__VA_ARGS__))
#define __OPT(_1, _2, N, ...)	N
/** @} */

/**
 * Generic macro to initialize an object to NULL.
 *
 * @param[out] A			- the object to initialize.
 */
#if ALLOC == AUTO
#define RLC_NULL(A)			/* empty */
#else
#define RLC_NULL(A)			A = NULL;
#endif


/**
 * Accumulates a double precision digit in a triple register variable.
 *
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to multiply.
 * @param[in] B				- the second digit to multiply.
 */
#define RLC_COMBA_STEP_MUL(R2, R1, R0, A, B)								\
	dig_t _r, _r0, _r1;														\
	RLC_MUL_DIG(_r1, _r0, A, B);											\
	RLC_COMBA_ADD(_r, R2, R1, R0, _r0);										\
	(R1) += _r1;															\
	(R2) += (R1) < _r1;														\

/**
 * Computes the step of a Comba squaring.
 *
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to multiply.
 * @param[in] B				- the second digit to multiply.
 */
#define RLC_COMBA_STEP_SQR(R2, R1, R0, A, B)								\
	dig_t _r, _r0, _r1;														\
	RLC_MUL_DIG(_r1, _r0, A, B);											\
	dig_t _s0 = _r0 + _r0;													\
	dig_t _s1 = _r1 + _r1 + (_s0 < _r0);									\
	RLC_COMBA_ADD(_r, R2, R1, R0, _s0);										\
	(R1) += _s1;															\
	(R2) += (R1) < _s1;														\
	(R2) += (_s1 < _r1);													\

/**
 * Accumulates a single precision digit in a triple register variable.
 *
 * @param[in,out] T			- the temporary variable.
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to accumulate.
 */
#define RLC_COMBA_ADD(T, R2, R1, R0, A)										\
	(T) = (R1);																\
	(R0) += (A);															\
	(R1) += (R0) < (A);														\
	(R2) += (R1) < (T);														\

/**
 * Selects a real or dummy printing function depending on library flags.
 *
 * @param[in] F			- the format string.
 */
#ifndef QUIET
#define util_print(F, ...)		util_printf(RLC_STR(F), ##__VA_ARGS__)
#else
#define util_print(F, ...)		/* empty */
#endif

/**
 * Prints a standard label.
 *
 * @param[in] L			- the label of the banner.
 * @param[in] I			- if the banner is inside an hierarchy.
 */
#define util_banner(L, I)													\
	if (!I) {																\
		util_print("\n-- " L "\n");											\
	} else {																\
		util_print("\n** " L "\n\n");										\
	}																		\

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Toggle endianess of a digit.
 */
uint32_t util_conv_endian(uint32_t i);

/**
 * Convert a digit to big-endian.
 */
uint32_t util_conv_big(uint32_t i);

/**
 * Convert a digit to little-endian.
 */
uint32_t util_conv_little(uint32_t i);

/**
 * Converts a small digit to a character.
 */
char util_conv_char(dig_t i);

/**
 * Returns the highest bit set on a digit.
 *
 * @param[in] a				- the digit.
 * @return the position of the highest bit set.
 */
int util_bits_dig(dig_t a);

/**
 * Compares two buffers in constant time.
 *
 * @param[in] a				- the first buffer.
 * @param[in] b				- the second buffer.
 * @param[in] n				- the length in bytes of the buffers.
 * @return RLC_EQ if they are equal and RLC_NE otherwise.
 */
int util_cmp_const(const void *a, const void *b, int n);

/**
 * Formats and prints data following a printf-like syntax.
 *
 * @param[in] format		- the format.
 * @param[in] ...			- the list of arguments matching the format.
 */
void util_printf(const char *format, ...);

/**
 * Prints a digit.
 *
 * @param[in] a 			- the digit to print.
 * @param[in] pad 			- the flag to indicate if the digit must be padded
 * 							with zeroes.
 */
void util_print_dig(dig_t a, int pad);

#endif /* !RLC_UTIL_H */
