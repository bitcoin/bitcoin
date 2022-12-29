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
 * Implementation of the basic functions to manipulate binary field elements.
 *
 * @ingroup fb
 */

#include "relic_core.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_rand.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Checks if a radix is a power of two.
 *
 * @param[in] radix				- the radix to check.
 * @return if radix is a valid radix.
 */
static int valid_radix(int radix) {
	while (radix > 0) {
		if (radix != 1 && radix % 2 == 1)
			return 0;
		radix = radix / 2;
	}
	return 1;
}

/**
 * Computes the logarithm of a valid radix in basis two.
 *
 * @param[in] radix				- the valid radix.
 * @return the logarithm of the radix in basis two.
 */
static int log_radix(int radix) {
	int l = 0;

	while (radix > 0) {
		radix = radix / 2;
		l++;
	}
	return --l;
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_copy(fb_t c, const fb_t a) {
	dv_copy(c, a, RLC_FB_DIGS);
}

void fb_zero(fb_t a) {
	dv_zero(a, RLC_FB_DIGS);
}

int fb_is_zero(const fb_t a) {
	int i;
	dig_t t = 0;

	for (i = 0; i < RLC_FB_DIGS; i++) {
		t |= a[i];
	}

	return !t;
}

int fb_get_bit(const fb_t a, int bit) {
	int d;

	RLC_RIP(bit, d, bit);

	return (a[d] >> bit) & 1;
}

void fb_set_bit(fb_t a, int bit, int value) {
	int d;
	dig_t mask;

	RLC_RIP(bit, d, bit);

	mask = (dig_t)1 << bit;

	if (value == 1) {
		a[d] |= mask;
	} else {
		a[d] &= ~mask;
	}
}

int fb_bits(const fb_t a) {
	int i = RLC_FB_DIGS - 1;

	while (i >= 0 && a[i] == 0) {
		i--;
	}

	if (i > 0) {
		return (i << RLC_DIG_LOG) + util_bits_dig(a[i]);
	} else {
		return util_bits_dig(a[0]);
	}
}

void fb_set_dig(fb_t c, dig_t a) {
	fb_zero(c);
	c[0] = a;
}

void fb_rand(fb_t a) {
	int bits, digits;

	rand_bytes((uint8_t *)a, RLC_FB_DIGS * sizeof(dig_t));

	RLC_RIP(bits, digits, RLC_FB_BITS);
	if (bits > 0) {
		dig_t mask = RLC_MASK(bits);
		a[RLC_FB_DIGS - 1] &= mask;
	}
}

void fb_print(const fb_t a) {
	int i;

	/* Suppress possible unused parameter warning. */
	(void)a;
	for (i = RLC_FB_DIGS - 1; i > 0; i--) {
		util_print_dig(a[i], 1);
		util_print(" ");
	}
	util_print_dig(a[0], 1);
	util_print("\n");
}

int fb_size_str(const fb_t a, int radix) {
	bn_t t;
	int digits = 0;

	bn_null(t);

	if (!valid_radix(radix)) {
		RLC_THROW(ERR_NO_VALID);
		return 0;
	}

	RLC_TRY {
		bn_new(t);

		bn_read_raw(t, a, RLC_FB_DIGS);

		digits = bn_size_str(t, radix);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}

	return digits;
}

void fb_read_str(fb_t a, const char *str, int len, int radix) {
	int i, j, l;
	char c;
	dig_t carry;

	fb_zero(a);

	l = log_radix(radix);
	if (!valid_radix(radix)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	if (RLC_CEIL(l * (len - 1), RLC_DIG) > RLC_FB_DIGS) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	j = 0;
	while (j < len) {
		if (str[j] == 0) {
			break;
		}
		c = (char)((radix < 36) ? RLC_UPP(str[j]) : str[j]);
		for (i = 0; i < 64; i++) {
			if (c == util_conv_char(i)) {
				break;
			}
		}

		if (i < radix) {
			carry = fb_lshb_low(a, a, l);
			if (carry != 0) {
				RLC_THROW(ERR_NO_BUFFER);
				break;
			}
			fb_add_dig(a, a, (dig_t)i);
		} else {
			break;
		}
		j++;
	}
}

void fb_write_str(char *str, int len, const fb_t a, int radix) {
	fb_t t;
	int d, l, i, j;
	char c;

	fb_null(t);

	l = fb_size_str(a, radix);
	if (len < l) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	len = l;

	l = log_radix(radix);
	if (!valid_radix(radix)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	if (fb_is_zero(a) == 1) {
		*str++ = '0';
		*str = '\0';
		return;
	}

	RLC_TRY {
		fb_new(t);
		fb_copy(t, a);

		j = 0;
		while (!fb_is_zero(t)) {
			d = t[0] % radix;
			fb_rshb_low(t, t, l);
			str[j] = util_conv_char(d);
			j++;
		}

		/* Reverse the digits of the string. */
		i = 0;
		j = len - 2;
		while (i < j) {
			c = str[i];
			str[i] = str[j];
			str[j] = c;
			++i;
			--j;
		}

		str[len - 1] = '\0';
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fb_free(t);
	}
}

void fb_read_bin(fb_t a, const uint8_t *bin, int len) {
	bn_t t;

	bn_null(t);

	if (len != RLC_FB_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	RLC_TRY {
		bn_new(t);

		bn_read_bin(t, bin, len);

		fb_copy(a, t->dp);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

void fb_write_bin(uint8_t *bin, int len, const fb_t a) {
	bn_t t;

	bn_null(t);

	if (len != RLC_FB_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	RLC_TRY {
		bn_new(t);

		bn_read_raw(t, a, RLC_FB_DIGS);

		bn_write_bin(bin, len, t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}
