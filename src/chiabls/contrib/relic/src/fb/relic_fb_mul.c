/*f
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
 * Implementation of the binary field multiplication functions.
 *
 * @ingroup fb
 */

#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if FB_MUL == BASIC
/**
 * Multiplies two binary field elements using shift-and-add multiplication.
 *
 * @param c					- the result.
 * @param a					- the first binary field element.
 * @param b					- the second binary field element.
 * @param size				- the number of digits to multiply.
 */
static void fb_mul_basic_imp(dig_t *c, const dig_t *a, const dig_t *b, int size) {
	int i;
	dv_t s;

	dv_null(s);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(s);
		dv_zero(s, 2 * FB_DIGS);

		dv_copy(s, b, size);
		dv_zero(c, 2 * size);

		if (a[0] & 1) {
			dv_copy(c, b, size);
		}
		for (i = 1; i <= (FB_DIGIT * size) - 1; i++) {
			fb_lsh1_low(s, s);
			fb_rdc(s, s);
			if (fb_get_bit(a, i)) {
				fb_add(c, c, s);
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(s);
	}
}
#endif /* FB_MUL == BASIC */

#if FB_MUL == LCOMB
/**
 * Multiplies two binary field elements using left-to-right comb multiplication.
 *
 * @param c					- the result.
 * @param a					- the first binary field element.
 * @param b					- the second binary field element.
 * @param size				- the number of digits to multiply.
 */
static void fb_mul_lcomb_imp(dig_t *c, const dig_t *a, const dig_t *b, int size) {
	dv_zero(c, 2 * size);

	for (int i = FB_DIGIT - 1; i >= 0; i--) {
		for (int j = 0; j < size; j++) {
			if (a[j] & ((dig_t)1 << i)) {
				fb_addd_low(c + j, c + j, b, size);
			}
		}
		if (i != 0) {
			bn_lsh1_low(c, c, 2 * size);
		}
	}
}
#endif /* FB_MUL == LCOMB */

#if FB_MUL == RCOMB
/**
 * Multiplies two binary field elements using right-to-left comb multiplication.
 *
 * @param c					- the result.
 * @param a					- the first binary field element.
 * @param b					- the second binary field element.
 * @param size				- the number of digits to multiply.
 */
static void fb_mul_rcomb_imp(dig_t *c, const dig_t *a, const dig_t *b, int size) {
	dv_t _b;

	dv_null(_b);

	TRY {
		dv_new(_b);
		dv_zero(c, 2 * size);

		for (int i = 0; i < size; i++)
			_b[i] = b[i];
		_b[size] = 0;

		for (int i = 0; i < FB_DIGIT; i++) {
			for (int j = 0; j < size; j++) {
				if (a[j] & ((dig_t)1 << i)) {
					fb_addd_low(c + j, c + j, _b, size + 1);
				}
			}
			if (i != FB_DIGIT - 1) {
				bn_lsh1_low(_b, _b, size + 1);
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(_b);
	}
}
#endif /* FB_MUL == RCOMB */

#if FB_KARAT > 0 || !defined(STRIP)
/**
 * Multiplies two binary field elements using recursive Karatsuba
 * multiplication.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first binary field element.
 * @param[in] b				- the second binary field element.
 * @param[in] size			- the number of digits to multiply.
 * @param[in] level			- the number of Karatsuba steps to apply.
 */
static void fb_mul_karat_imp(dv_t c, const fb_t a, const fb_t b, int size,
		int level) {
	int i, h, h1;
	dv_t a1, b1, ab;
	dig_t *a0b0, *a1b1;

	dv_null(a1);
	dv_null(b1);
	dv_null(ab);

	/* Compute half the digits of a or b. */
	h = size >> 1;
	h1 = size - h;

	TRY {
		/* Allocate the temp variables. */
		dv_new(a1);
		dv_new(b1);
		dv_new(ab);
		a0b0 = ab;
		a1b1 = ab + 2 * h;

		/* a0b0 = a0 * b0 and a1b1 = a1 * b1 */
		if (level <= 1) {
#if FB_MUL == BASIC
			fb_mul_basic_imp(a0b0, a, b, h);
			fb_mul_basic_imp(a1b1, a + h, b + h, h1);
#elif FB_MUL == LCOMB
			fb_mul_lcomb_imp(a0b0, a, b, h);
			fb_mul_lcomb_imp(a1b1, a + h, b + h, h1);
#elif FB_MUL == RCOMB
			fb_mul_rcomb_imp(a0b0, a, b, h);
			fb_mul_rcomb_imp(a1b1, a + h, b + h, h1);
#elif FB_MUL == INTEG || FB_MUL == LODAH
			fb_muld_low(a0b0, a, b, h);
			fb_muld_low(a1b1, a + h, b + h, h1);
#endif
		} else {
			fb_mul_karat_imp(a0b0, a, b, h, level - 1);
			fb_mul_karat_imp(a1b1, a + h, b + h, h1, level - 1);
		}

		for (i = 0; i < 2 * size; i++) {
			c[i] = ab[i];
		}

		/* c = c - (a0*b0 << h digits) */
		fb_addd_low(c + h, c + h, a0b0, 2 * h);

		/* c = c - (a1*b1 << h digits) */
		fb_addd_low(c + h, c + h, a1b1, 2 * h1);

		/* a1 = (a1 + a0) */
		fb_addd_low(a1, a, a + h, h);

		/* b1 = (b1 + b0) */
		fb_addd_low(b1, b, b + h, h);

		if (h1 > h) {
			a1[h1 - 1] = a[h + h1 - 1];
			b1[h1 - 1] = b[h + h1 - 1];
		}

		if (level <= 1) {
			/* a1b1 = (a1 + a0)*(b1 + b0) */
#if FB_MUL == BASIC
			fb_mul_basic_imp(a1b1, a1, b1, h1);
#elif FB_MUL == LCOMB
			fb_mul_lcomb_imp(a1b1, a1, b1, h1);
#elif FB_MUL == RCOMB
			fb_mul_rcomb_imp(a1b1, a1, b1, h1);
#elif FB_MUL == INTEG || FB_MUL == LODAH
			fb_muld_low(a1b1, a1, b1, h1);
#endif
		} else {
			fb_mul_karat_imp(a1b1, a1, b1, h1, level - 1);
		}

		/* c = c + [(a1 + a0)*(b1 + b0) << digits] */
		fb_addd_low(c + h, c + h, a1b1, 2 * h1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(a1);
		dv_free(b1);
		dv_free(ab);
	}
}

#endif /* FB_KARAT > 0 || !defined(STRIP) */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FB_MUL == BASIC || !defined(STRIP)

void fb_mul_basic(fb_t c, const fb_t a, const fb_t b) {
	int i;
	dv_t s;
	fb_t t;

	dv_null(s);
	fb_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		fb_new(t);
		dv_new(s);
		fb_zero(t);
		dv_zero(s + FB_DIGS, FB_DIGS);
		fb_copy(s, b);

		if (a[0] & 1) {
			fb_copy(t, b);
		}
		for (i = 1; i < FB_BITS; i++) {
			/* We are already shifting a temporary value, so this is more efficient
			 * than calling fb_lsh(). */
			s[FB_DIGS] = fb_lsh1_low(s, s);
			fb_rdc(s, s);
			if (fb_get_bit(a, i)) {
				fb_add(t, t, s);
			}
		}

		if (fb_bits(t) > FB_BITS) {
			fb_poly_add(c, t);
		} else {
			fb_copy(c, t);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(t);
		fb_free(s);
	}
}

#endif

#if FB_MUL == INTEG || !defined(STRIP)

void fb_mul_integ(fb_t c, const fb_t a, const fb_t b) {
	fb_mulm_low(c, a, b);
}

#endif

#if FB_MUL == LCOMB || !defined(STRIP)

void fb_mul_lcomb(fb_t c, const fb_t a, const fb_t b) {
	dv_t t;
	dig_t carry;

	dv_null(t);

	TRY {
		dv_new(t);
		dv_zero(t, 2 * FB_DIGS);

		for (int i = FB_DIGIT - 1; i >= 0; i--) {
			for (int j = 0; j < FB_DIGS; j++) {
				if (a[j] & ((dig_t)1 << i)) {
					/* This cannot use fb_addn_low() because there is no
					 * guarantee that operands will be aligned. */
					fb_addd_low(t + j, t + j, b, FB_DIGS);
				}
			}
			if (i != 0) {
				carry = fb_lsh1_low(t, t);
				fb_lsh1_low(t + FB_DIGS, t + FB_DIGS);
				t[FB_DIGS] |= carry;
			}
		}

		fb_rdc(c, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif

#if FB_MUL == RCOMB || !defined(STRIP)

void fb_mul_rcomb(fb_t c, const fb_t a, const fb_t b) {
	dv_t t, _b;
	dig_t carry;

	dv_null(t);
	dv_null(_b);

	TRY {
		dv_new(t);
		dv_new(_b);
		dv_zero(t, 2 * FB_DIGS);
		dv_zero(_b, FB_DIGS + 1);

		fb_copy(_b, b);

		for (int i = 0; i < FB_DIGIT; i++) {
			for (int j = 0; j < FB_DIGS; j++) {
				if (a[j] & ((dig_t)1 << i)) {
					fb_addd_low(t + j, t + j, _b, FB_DIGS + 1);
				}
			}
			if (i != FB_DIGIT - 1) {
				carry = fb_lsh1_low(_b, _b);
				_b[FB_DIGS] = (_b[FB_DIGS] << 1) | carry;
			}
		}

		fb_rdc(c, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
		dv_free(_b);
	}
}

#endif

#if FB_MUL == LODAH || !defined(STRIP)

void fb_mul_lodah(fb_t c, const fb_t a, const fb_t b) {
	dv_t t;

	dv_null(t);

	TRY {
		dv_new(t);

		fb_muln_low(t, a, b);

		fb_rdc(c, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif

#if FB_KARAT > 0 || !defined(STRIP)

void fb_mul_karat(fb_t c, const fb_t a, const fb_t b) {
	dv_t t;

	dv_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(t);
		dv_zero(t, 2 * FB_DIGS);

		fb_mul_karat_imp(t, a, b, FB_DIGS, FB_KARAT);

		fb_rdc(c, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif

void fb_mul_dig(fb_t c, const fb_t a, dig_t b) {
	dv_t t;

	dv_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(t);

		fb_mul1_low(t, a, b);

		fb_rdc1_low(c, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}
