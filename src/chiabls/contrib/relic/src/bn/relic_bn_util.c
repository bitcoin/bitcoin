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
 * Implementation of the multiple precision utilities.
 *
 * @ingroup bn
 */

#include <inttypes.h>

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_copy(bn_t c, const bn_t a) {
	int i;

	if (c->dp == a->dp)
		return;

	bn_grow(c, a->used);

	for (i = 0; i < a->used; i++) {
		c->dp[i] = a->dp[i];
	}

	c->used = a->used;
	c->sign = a->sign;
}

void bn_abs(bn_t c, const bn_t a) {
	if (c->dp != a->dp) {
		bn_copy(c, a);
	}
	c->sign = BN_POS;
}

void bn_neg(bn_t c, const bn_t a) {
	if (c->dp != a->dp) {
		bn_copy(c, a);
	}
	if (!bn_is_zero(c)) {
		c->sign = a->sign ^ 1;
	}
}

int bn_sign(const bn_t a) {
	return a->sign;
}

void bn_zero(bn_t a) {
	a->sign = BN_POS;
	a->used = 1;
	dv_zero(a->dp, a->alloc);
}

int bn_is_zero(const bn_t a) {
	if (a->used == 0) {
		return 1;
	}
	if ((a->used == 1) && (a->dp[0] == 0)) {
		return 1;
	}
	return 0;
}

int bn_is_even(const bn_t a) {
	if (bn_is_zero(a)) {
		return 1;
	}
	if ((a->dp[0] & 0x01) == 0) {
		return 1;
	}
	return 0;
}

int bn_bits(const bn_t a) {
	int bits;

	if (a->used == 0) {
		return 0;
	}

	/* Bits in lower digits. */
	bits = (a->used - 1) * BN_DIGIT;

	return bits + util_bits_dig(a->dp[a->used - 1]);
}

int bn_get_bit(const bn_t a, int bit) {
	int d;

	SPLIT(bit, d, bit, BN_DIG_LOG);

	if (d >= a->used) {
		return 0;
	} else {
		return (a->dp[d] >> bit) & (dig_t)1;
	}
}

void bn_set_bit(bn_t a, int bit, int value) {
	int d;

	SPLIT(bit, d, bit, BN_DIG_LOG);

	if (value == 1) {
		a->dp[d] |= ((dig_t)1 << bit);
		if ((d + 1) > a->used) {
			a->used = d + 1;
		}
	} else {
		a->dp[d] &= ~((dig_t)1 << bit);
		bn_trim(a);
	}
}

int bn_ham(const bn_t a) {
	int c = 0;

	for (int i = 0; i < bn_bits(a); i++) {
		c += bn_get_bit(a, i);
	}

	return c;
}

void bn_get_dig(dig_t *c, const bn_t a) {
	*c = a->dp[0];
}

void bn_set_dig(bn_t a, dig_t digit) {
	bn_zero(a);
	a->dp[0] = digit;
	a->used = 1;
	a->sign = BN_POS;
}

void bn_set_2b(bn_t a, int b) {
	int i, d;

	SPLIT(b, d, b, BN_DIG_LOG);

	bn_grow(a, d + 1);
	for (i = 0; i < d; i++)
		a->dp[i] = 0;
	a->used = d + 1;
	a->dp[d] = ((dig_t)1 << b);
	a->sign = BN_POS;
}

void bn_rand(bn_t a, int sign, int bits) {
	int digits;

	SPLIT(bits, digits, bits, BN_DIG_LOG);
	digits += (bits > 0 ? 1 : 0);

	bn_grow(a, digits);

	rand_bytes((uint8_t *)a->dp, digits * sizeof(dig_t));

	a->used = digits;
	a->sign = sign;
	if (bits > 0) {
		dig_t mask = ((dig_t)1 << (dig_t)bits) - 1;
		a->dp[a->used - 1] &= mask;
	}
	bn_trim(a);
}

void bn_rand_mod(bn_t a, bn_t b) {
	do {
		bn_rand(a, bn_sign(b), bn_bits(b));
	} while (bn_is_zero(a) || bn_cmp_abs(a, b) != CMP_LT);
}

void bn_print(const bn_t a) {
	int i;

	if (a->sign == BN_NEG) {
		util_print("-");
	}
	if (a->used == 0) {
		util_print("0\n");
	} else {
#if WSIZE == 64
		util_print_dig(a->dp[a->used - 1], 0);
		for (i = a->used - 2; i >= 0; i--) {
			util_print_dig(a->dp[i], 1);
		}
#else
		util_print_dig(a->dp[a->used - 1], 0);
		for (i = a->used - 2; i >= 0; i--) {
			util_print_dig(a->dp[i], 1);
		}
#endif
		util_print("\n");
	}
}

int bn_size_str(const bn_t a, int radix) {
	int digits = 0;
	bn_t t;

	bn_null(t);

	/* Binary case requires the bits, a sign and the null terminator. */
	if (radix == 2) {
		return bn_bits(a) + (a->sign == BN_NEG ? 1 : 0) + 1;
	}

	/* Check the radix. */
	if (radix < 2 || radix > 64) {
		THROW(ERR_NO_VALID);
	}

	if (bn_is_zero(a)) {
		return 2;
	}

	if (a->sign == BN_NEG) {
		digits++;
	}

	TRY {
		bn_new(t);
		bn_copy(t, a);

		t->sign = BN_POS;

		while (!bn_is_zero(t)) {
			bn_div_dig(t, t, (dig_t)radix);
			digits++;
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(t);
	}

	return digits + 1;
}

void bn_read_str(bn_t a, const char *str, int len, int radix) {
	int sign, i, j;
	char c;

	bn_zero(a);

	if (radix < 2 || radix > 64) {
		THROW(ERR_NO_VALID)
	}

	j = 0;
	if (str[0] == '-') {
		j++;
		sign = BN_NEG;
	} else {
		sign = BN_POS;
	}

	while (str[j] && j < len) {
		c = (char)((radix < 36) ? TOUPPER(str[j]) : str[j]);
		for (i = 0; i < 64; i++) {
			if (c == util_conv_char(i)) {
				break;
			}
		}

		if (i < radix) {
			bn_mul_dig(a, a, (dig_t)radix);
			bn_add_dig(a, a, (dig_t)i);
		} else {
			break;
		}
		j++;
	}

	a->sign = sign;
}

void bn_write_str(char *str, int len, const bn_t a, int radix) {
	bn_t t;
	dig_t d;
	int digits, l, i, j;
	char c;

	bn_null(t);

	l = bn_size_str(a, radix);
	if (len < l) {
		THROW(ERR_NO_BUFFER);
	}

	if (radix < 2 || radix > 64) {
		THROW(ERR_NO_VALID)
	}

	if (bn_is_zero(a) == 1) {
		*str++ = '0';
		*str = '\0';
		return;
	}

	TRY {
		bn_new(t);
		bn_copy(t, a);

		j = 0;
		if (t->sign == BN_NEG) {
			str[j] = '-';
			j++;
			t->sign = BN_POS;
		}

		digits = 0;
		while (!bn_is_zero(t)) {
			bn_div_rem_dig(t, &d, t, (dig_t)radix);
			str[j] = util_conv_char(d);
			digits++;
			j++;
		}

		/* Reverse the digits of the string. */
		i = 0;
		if (str[0] == '-') {
			i = 1;
		}

		j = l - 2;
		while (i < j) {
			c = str[i];
			str[i] = str[j];
			str[j] = c;
			++i;
			--j;
		}

		str[l - 1] = '\0';
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(t);
	}
}

int bn_size_bin(const bn_t a) {
	dig_t d;
	int digits;

	digits = (a->used - 1) * (BN_DIGIT / 8);
	d = a->dp[a->used - 1];

	while (d != 0) {
		d = d >> 8;
		digits++;
	}
	return digits;
}

void bn_read_bin(bn_t a, const uint8_t *bin, int len) {
	int i, j;
	dig_t d = (BN_DIGIT / 8);
	int digs = (len % d == 0 ? len / d : len / d + 1);

	bn_grow(a, digs);
	bn_zero(a);
	a->used = digs;

	for (i = 0; i < digs - 1; i++) {
		d = 0;
		for (j = (BN_DIGIT / 8) - 1; j >= 0; j--) {
			d = d << 8;
			d |= bin[len - 1 - (i * (BN_DIGIT / 8) + j)];
		}
		a->dp[i] = d;
	}
	d = 0;
	for (j = (BN_DIGIT / 8) - 1; j >= 0; j--) {
		if ((int)(i * (BN_DIGIT / 8) + j) < len) {
			d = d << 8;
			d |= bin[len - 1 - (i * (BN_DIGIT / 8) + j)];
		}
	}
	a->dp[i] = d;

	a->sign = BN_POS;
	bn_trim(a);
}

void bn_write_bin(uint8_t *bin, int len, const bn_t a) {
	int size, k;
	dig_t d;

	size = bn_size_bin(a);

	if (len < size) {
		THROW(ERR_NO_BUFFER);
	}

	k = 0;
	for (int i = 0; i < a->used - 1; i++) {
		d = a->dp[i];
		for (int j = 0; j < (int)(BN_DIGIT / 8); j++) {
			bin[len - 1 - k++] = d & 0xFF;
			d = d >> 8;
		}
	}

	d = a->dp[a->used - 1];
	while (d != 0) {
		bin[len - 1 - k++] = d & 0xFF;
		d = d >> 8;
	}

	while (k < len) {
		bin[len - 1 - k++] = 0;
	}
}

int bn_size_raw(const bn_t a) {
	return a->used;
}

void bn_read_raw(bn_t a, const dig_t *raw, int len) {
	TRY {
		bn_grow(a, len);
		a->used = len;
		a->sign = BN_POS;
		dv_copy(a->dp, raw, len);
		bn_trim(a);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
}

void bn_write_raw(dig_t *raw, int len, const bn_t a) {
	int i, size;

	size = a->used;

	if (len < size) {
		THROW(ERR_NO_BUFFER);
	}

	for (i = 0; i < size; i++) {
		raw[i] = a->dp[i];
	}
	for (; i < len; i++) {
		raw[i] = 0;
	}
}
