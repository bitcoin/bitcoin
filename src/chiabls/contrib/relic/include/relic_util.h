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
 * @defgroup util Misc utilities
 */

/**
 * @file
 *
 * Interface of misc utilitles.
 *
 * @ingroup util
 */

#ifndef RELIC_UTIL_H
#define RELIC_UTIL_H

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
 * @returns			- the smaller number.
 */
#define MIN(A, B)			((A) < (B) ? (A) : (B))

/**
 * Returns the maximum between two numbers.
 *
 * @param[in] A		- the first number.
 * @param[in] B		- the second number.
 * @returns			- the bigger number.
 */
#define MAX(A, B)			((A) > (B) ? (A) : (B))

/**
 * Splits a bit count in a digit count and an updated bit count.
 *
 * @param[out] B		- the resulting bit count.
 * @param[out] D		- the resulting digit count.
 * @param[out] V		- the bit count.
 * @param[in] L			- the logarithm of the digit size.
 */
#define SPLIT(B, D, V, L)	D = (V) >> (L); B = (V) - ((D) << (L));

/**
 * Computes the ceiling function of an integer division.
 *
 * @param[in] A			- the dividend.
 * @param[in] B			- the divisor.
 */
#define CEIL(A, B)			(((A) - 1) / (B) + 1)

/**
 * Returns a bit mask to isolate the lowest part of a digit.
 *
 * @param[in] B			- the number of bits to isolate.
 */
#define MASK(B)				(((dig_t)1 << (B)) - 1)

/**
 * Returns a bit mask to isolate the lowest half of a digit.
 */
#define LMASK				(MASK(DIGIT >> 1))

/**
 * Returns a bit mask to isolate the highest half of a digit.
 */
#define HMASK				(LMASK << (DIGIT >> 1))

/**
 * Bit mask used to return an entire digit.
 */
#define DMASK				(HMASK | LMASK)

/**
 * Returns the lowest half of a digit.
 *
 * @param[in] D			- the digit.
 */
#define LOW(D)				(D & LMASK)

/**
 * Returns the highest half of a digit.
 *
 * @param[in] D			- the digit.
 */
#define HIGH(D)				(D >> (DIGIT >> 1))

/**
 * Returns the given character in upper case.
 *
 * @param[in] C			- the character.
 */
#define TOUPPER(C)			((C) - 0x20 * (((C) >= 'a') && ((C) <= 'z')))

/**
 * Concatenates two tokens.
 */
/** @{ */
#define CAT(A, B)			_CAT(A, B)
#define _CAT(A, B)			A ## B
/** @} */

 /**
  * Inserts quotation marks in the macro argument.
  */
 /** @{ */
#define QUOTE(A)			_QUOTE(A)
#define _QUOTE(A)			#A
/** @} */

/**
 * Selects a basic or advanced version of a function by checking if an
 * additional argument was passed.
 */
/** @{ */
#define OPT(...)				_OPT(__VA_ARGS__, _imp, _basic, _error)
#define _OPT(...)				__OPT(__VA_ARGS__)
#define __OPT(_1, _2, N, ...)	N
/** @} */

/**
 * Selects a real or dummy printing function depending on library flags.
 *
 * @param[in] F			- the format string.
 */
#ifndef QUIET
#define util_print(F, ...)		util_printf(STRING(F), ##__VA_ARGS__)
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
 * @return CMP_EQ if they are equal and CMP_NE otherwise.
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

#endif /* !RELIC_UTIL_H */
