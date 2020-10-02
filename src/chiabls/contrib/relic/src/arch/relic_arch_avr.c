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
 * Implementation of AVR-dependent routines.
 *
 * @ingroup arch
 */

#include <avr/pgmspace.h>

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void arch_init(void) {
}

void arch_clean(void) {
}

void arch_copy_rom(char *dest, const char *src, int len) {
	int i = 0;
	char c;

	while ((c = pgm_read_byte(src++)) && (i++ < len - 1)) {
		*dest++ = c;
	}
	*dest = 0;
}
