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
 * Implementation of the prime field utilities.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_copy(fp_t c, const fp_t a) {
	dv_copy(c, a, RLC_FP_DIGS);
}

void fp_zero(fp_t a) {
	dv_zero(a, RLC_FP_DIGS);
}

int fp_is_zero(const fp_t a) {
	int i;
	dig_t t = 0;

	for (i = 0; i < RLC_FP_DIGS; i++) {
		t |= a[i];
	}

	return !t;
}

int fp_is_even(const fp_t a) {
	if ((a[0] & 0x01) == 0) {
		return 1;
	}
	return 0;
}

int fp_get_bit(const fp_t a, int bit) {
	int d;

	RLC_RIP(bit, d, bit);

	return (a[d] >> bit) & 1;
}

void fp_set_bit(fp_t a, int bit, int value) {
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

int fp_bits(const fp_t a) {
	int i = RLC_FP_DIGS - 1;

	while (i >= 0 && a[i] == 0) {
		i--;
	}

	if (i > 0) {
		return (i << RLC_DIG_LOG) + util_bits_dig(a[i]);
	} else {
		return util_bits_dig(a[0]);
	}
}

void fp_set_dig(fp_t c, dig_t a) {
	fp_prime_conv_dig(c, a);
}

void fp_rand(fp_t a) {
	int bits, digits;

	rand_bytes((uint8_t *)a, RLC_FP_DIGS * sizeof(dig_t));

	RLC_RIP(bits, digits, RLC_FP_BITS);
	if (bits > 0) {
		dig_t mask = ((dig_t)1 << (dig_t)bits) - 1;
		a[RLC_FP_DIGS - 1] &= mask;
	}

	while (dv_cmp(a, fp_prime_get(), RLC_FP_DIGS) != RLC_LT) {
		fp_subm_low(a, a, fp_prime_get());
	}
}

void fp_print(const fp_t a) {
	int i;
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

#if FP_RDC == MONTY
		if (a != fp_prime_get()) {
			fp_prime_back(t, a);
		} else {
			bn_read_raw(t, a, RLC_FP_DIGS);
		}
#else
		bn_read_raw(t, a, RLC_FP_DIGS);
#endif

		for (i = RLC_FP_DIGS - 1; i > 0; i--) {
			if (i >= t->used) {
				util_print_dig(0, 1);
			} else {
				util_print_dig(t->dp[i], 1);
			}
			util_print(" ");
		}
		util_print_dig(t->dp[0], 1);
		util_print("\n");

	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

int fp_size_str(const fp_t a, int radix) {
	bn_t t;
	int digits = 0;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		fp_prime_back(t, a);

		digits = bn_size_str(t, radix);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}

	return digits;
}

void fp_read_str(fp_t a, const char *str, int len, int radix) {
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);
		bn_read_str(t, str, len, radix);
		if (bn_is_zero(t)) {
			fp_zero(a);
		} else {
			if (t->used == 1) {
				fp_prime_conv_dig(a, t->dp[0]);
				if (bn_sign(t) == RLC_NEG) {
					fp_neg(a, a);
				}
			} else {
				fp_prime_conv(a, t);
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

void fp_write_str(char *str, int len, const fp_t a, int radix) {
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		fp_prime_back(t, a);

		bn_write_str(str, len, t, radix);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

void fp_read_bin(fp_t a, const uint8_t *bin, int len) {
	bn_t t;

	bn_null(t);

	if (len != RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	RLC_TRY {
		bn_new(t);
		bn_read_bin(t, bin, len);

		/* Reject values out of bounds. */
		if (bn_sign(t) == RLC_NEG || bn_cmp(t, &core_get()->prime) != RLC_LT) {
			RLC_THROW(ERR_NO_VALID);
		} else {
			if (bn_is_zero(t)) {
				fp_zero(a);
			} else {
				if (t->used == 1) {
					fp_prime_conv_dig(a, t->dp[0]);
				} else {
					fp_prime_conv(a, t);
				}
			}	
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

void fp_write_bin(uint8_t *bin, int len, const fp_t a) {
	bn_t t;

	bn_null(t);

	if (len != RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}

	RLC_TRY {
		bn_new(t);

		fp_prime_back(t, a);

		bn_write_bin(bin, len, t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}
