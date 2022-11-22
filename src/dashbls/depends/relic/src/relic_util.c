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
#include <stdarg.h>
#include <ctype.h>
#include <inttypes.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_util.h"
#include "relic_types.h"

#if ARCH == ARM && OPSYS == DROID
#include <android/log.h>
#endif

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Buffer to hold printed messages.
 */
#if ARCH == AVR

#ifndef QUIET
volatile char print_buf[128 + 1];
volatile char *util_print_ptr;

#if OPSYS == DUINO
/**
 * Send byte to serial port.
 */
void uart_putchar(char c, FILE *stream) {
	if (c == '\n') {
		uart_putchar('\r', stream);
	}
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}

/**
 * Stream for serial port.
 */
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

#endif
#endif

#endif /* QUIET */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

uint32_t util_conv_endian(uint32_t i) {
	uint32_t i1, i2, i3, i4;
	i1 = i & 0xFF;
	i2 = (i >> 8) & 0xFF;
	i3 = (i >> 16) & 0xFF;
	i4 = (i >> 24) & 0xFF;

	return ((uint32_t) i1 << 24) | ((uint32_t) i2 << 16) | ((uint32_t) i3 << 8)
			| i4;
}

uint32_t util_conv_big(uint32_t i) {
#ifdef BIGED
	return i;
#else
	return util_conv_endian(i);
#endif
}

uint32_t util_conv_little(uint32_t i) {
#ifndef BIGED
	return util_conv_endian(i);
#else
	return i;
#endif
}

char util_conv_char(dig_t i) {
#if WSIZE == 8 || WSIZE == 16
	/* Avoid tables to save up some memory. This is not performance-critical. */
	if (i < 10) {
		return i + '0';
	}
	if (i < 36) {
		return (i - 10) + 'A';
	}
	if (i < 62) {
		return (i - 36) + 'a';
	}
	if (i == 62) {
		return '+';
	} else {
		return '/';
	}
#else
	/* Use a table. */
	static const char conv_table[] =
			"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
	return conv_table[i];
#endif
}

int util_bits_dig(dig_t a) {
    return RLC_DIG - arch_lzcnt(a);
}

int util_cmp_const(const void *a, const void *b, int size) {
	const uint8_t *_a = (const uint8_t *)a;
	const uint8_t *_b = (const uint8_t *)b;
	uint8_t result = 0;
	int i;

	for (i = 0; i < size; i++) {
		result |= _a[i] ^ _b[i];
	}

	return (result == 0 ? RLC_EQ : RLC_NE);
}

#ifndef QUIET
void util_print(const char *format, ...) {
#if ARCH == AVR && !defined(OPSYS)
	util_print_ptr = print_buf + 1;
	va_list list;
	va_start(list, format);
	vsnprintf_P((char *)util_print_ptr, sizeof(print_buf) - 1, format, list);
	va_end(list);
	print_buf[0] = (uint8_t)2;
#elif ARCH == AVR && OPSYS == DUINO
	stdout = &uart_output;
	va_list list;
	va_start(list, format);
	vsnprintf_P((char *)print_buf, sizeof(print_buf), format, list);
	printf("%s", (char *)print_buf);
	va_end(list);
#elif ARCH == MSP && !defined(OPSYS)
	va_list list;
	va_start(list, format);
	vprintf(format, list);
	va_end(list);
#elif ARCH == ARM && OPSYS == DROID
	va_list list;
	va_start(list, format);
	__android_log_vprint(ANDROID_LOG_INFO, "relic-toolkit", format, list);
	va_end(list);
#else
	va_list list;
	va_start(list, format);
	vprintf(format, list);
	fflush(stdout);
	va_end(list);
#endif
}
#endif

void util_print_dig(dig_t a, int pad) {
#if RLC_DIG == 64
	if (pad) {
		util_print("%.16" PRIX64, (uint64_t) a);
	} else {
		util_print("%" PRIX64, (uint64_t) a);
	}
#elif RLC_DIG == 32
	if (pad) {
		util_print("%.8" PRIX32, (uint32_t) a);
	} else {
		util_print("%" PRIX32, (uint32_t) a);
	}
#elif RLC_DIG == 16
	if (pad) {
		util_print("%.4" PRIX16, (uint16_t) a);
	} else {
		util_print("%" PRIX16, (uint16_t) a);
	}
#else
	if (pad) {
		util_print("%.2" PRIX8, (uint8_t)a);
	} else {
		util_print("%" PRIX8, (uint8_t)a);
	}
#endif
}
