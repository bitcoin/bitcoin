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

	return ((uint32_t)i1 << 24) | ((uint32_t)i2 << 16) | ((uint32_t)i3 << 8) | i4;
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
#if WSIZE == 8 || WSIZE == 16
	static const uint8_t table[16] = {
		0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
	};
#endif
#if WSIZE == 8
	if (a >> 4 == 0) {
		return table[a & 0xF];
	} else {
		return table[a >> 4] + 4;
	}
	return 0;
#elif WSIZE == 16
	int offset;

	if (a >= ((dig_t)1 << 8)) {
		offset = 8;
	} else {
		offset = 0;
	}
	a = a >> offset;
	if (a >> 4 == 0) {
		return table[a & 0xF] + offset;
	} else {
		return table[a >> 4] + 4 + offset;
	}
	return 0;
#elif WSIZE == 32
	return DIGIT - __builtin_clz(a);
#elif WSIZE == 64
	return DIGIT - __builtin_clzll(a);
#endif
}

int util_cmp_const(const void * a, const void *b, int size) {
	const uint8_t *_a = (const uint8_t *) a;
	const uint8_t *_b = (const uint8_t *) b;
	uint8_t result = 0;
	int i;

	for (i = 0; i < size; i++) {
		result |= _a[i] ^ _b[i];
	}

	return (result == 0 ? CMP_EQ : CMP_NE);
}

void util_printf(const char *format, ...) {
#ifndef QUIET
#if ARCH == AVR && OPSYS == RELIC_NONE
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
#elif ARCH == MSP && OPSYS == RELIC_NONE
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
#endif
}

void util_print_dig(dig_t a, int pad) {
#if DIGIT == 64
	if (pad) {
		util_print("%.16" PRIX64, (uint64_t)a);
	} else {
		util_print("%" PRIX64, (uint64_t)a);
	}
#elif DIGIT == 32
	if (pad) {
		util_print("%.8" PRIX32, (uint32_t)a);
	} else {
		util_print("%" PRIX32, (uint32_t)a);
	}
#elif DIGIT == 16
	if (pad) {
		util_print("%.4" PRIX16, (uint16_t)a);
	} else {
		util_print("%" PRIX16, (uint16_t)a);
	}
#else
	if (pad) {
		util_print("%.2" PRIX8, (uint8_t)a);
	} else {
		util_print("%" PRIX8, (uint8_t)a);
	}
#endif
}
