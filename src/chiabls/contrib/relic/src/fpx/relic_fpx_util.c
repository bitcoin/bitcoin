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
 * Implementation of utilities in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp2_copy(fp2_t c, fp2_t a) {
	fp_copy(c[0], a[0]);
	fp_copy(c[1], a[1]);
}

void fp2_zero(fp2_t a) {
	fp_zero(a[0]);
	fp_zero(a[1]);
}

int fp2_is_zero(fp2_t a) {
	return fp_is_zero(a[0]) && fp_is_zero(a[1]);
}

void fp2_rand(fp2_t a) {
	fp_rand(a[0]);
	fp_rand(a[1]);
}

void fp2_print(fp2_t a) {
	fp_print(a[0]);
	fp_print(a[1]);
}

int fp2_size_bin(fp2_t a, int pack) {
	if (pack) {
		if (fp2_test_uni(a)) {
			return FP_BYTES + 1;
		} else {
			return 2 * FP_BYTES;
		}
	} else {
		return 2 * FP_BYTES;
	}
}

void fp2_read_bin(fp2_t a, uint8_t *bin, int len) {
	if (len != FP_BYTES + 1 && len != 2 * FP_BYTES) {
		THROW(ERR_NO_BUFFER);
	}
	if (len == FP_BYTES + 1) {
		fp_read_bin(a[0], bin, FP_BYTES);
		fp_zero(a[1]);
		fp_set_bit(a[1], 0, bin[FP_BYTES]);
		fp2_upk(a, a);
	}
	if (len == 2 * FP_BYTES) {
		fp_read_bin(a[0], bin, FP_BYTES);
		fp_read_bin(a[1], bin + FP_BYTES, FP_BYTES);
	}
}

void fp2_write_bin(uint8_t *bin, int len, fp2_t a, int pack) {
	fp2_t t;

	fp2_null(t);

	TRY {
		fp2_new(t);

		if (pack && fp2_test_uni(a)) {
			if (len < FP_BYTES + 1) {
				THROW(ERR_NO_BUFFER);
			} else {
				fp2_pck(t, a);
				fp_write_bin(bin, FP_BYTES, t[0]);
				bin[FP_BYTES] = fp_get_bit(t[1], 0);
			}
		} else {
			if (len < 2 * FP_BYTES) {
				THROW(ERR_NO_BUFFER);
			} else {
				fp_write_bin(bin, FP_BYTES, a[0]);
				fp_write_bin(bin + FP_BYTES, FP_BYTES, a[1]);
			}
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(t);
	}
}

void fp2_set_dig(fp2_t a, dig_t b) {
	fp_set_dig(a[0], b);
	fp_zero(a[1]);
}

void fp3_copy(fp3_t c, fp3_t a) {
	fp_copy(c[0], a[0]);
	fp_copy(c[1], a[1]);
	fp_copy(c[2], a[2]);
}

void fp3_zero(fp3_t a) {
	fp_zero(a[0]);
	fp_zero(a[1]);
	fp_zero(a[2]);
}

int fp3_is_zero(fp3_t a) {
	return fp_is_zero(a[0]) && fp_is_zero(a[1]) && fp_is_zero(a[2]);
}

void fp3_rand(fp3_t a) {
	fp_rand(a[0]);
	fp_rand(a[1]);
	fp_rand(a[2]);
}

void fp3_print(fp3_t a) {
	fp_print(a[0]);
	fp_print(a[1]);
	fp_print(a[2]);
}

int fp3_size_bin(fp3_t a) {
	return 3 * FP_BYTES;
}

void fp3_read_bin(fp3_t a, uint8_t *bin, int len) {
	if (len != 3 * FP_BYTES) {
		THROW(ERR_NO_BUFFER);
	}
	fp_read_bin(a[0], bin, FP_BYTES);
	fp_read_bin(a[1], bin + FP_BYTES, FP_BYTES);
	fp_read_bin(a[2], bin + 2 * FP_BYTES, FP_BYTES);
}

void fp3_write_bin(uint8_t *bin, int len, fp3_t a) {
	if (len != 3 * FP_BYTES) {
		THROW(ERR_NO_BUFFER);
	}
	fp_write_bin(bin, FP_BYTES, a[0]);
	fp_write_bin(bin + FP_BYTES, FP_BYTES, a[1]);
	fp_write_bin(bin + 2 * FP_BYTES, FP_BYTES, a[2]);
}

void fp3_set_dig(fp3_t a, dig_t b) {
	fp_set_dig(a[0], b);
	fp_zero(a[1]);
	fp_zero(a[2]);
}

void fp6_copy(fp6_t c, fp6_t a) {
	fp2_copy(c[0], a[0]);
	fp2_copy(c[1], a[1]);
	fp2_copy(c[2], a[2]);
}

void fp6_zero(fp6_t a) {
	fp2_zero(a[0]);
	fp2_zero(a[1]);
	fp2_zero(a[2]);
}

int fp6_is_zero(fp6_t a) {
	return fp2_is_zero(a[0]) && fp2_is_zero(a[1]) && fp2_is_zero(a[2]);
}

void fp6_rand(fp6_t a) {
	fp2_rand(a[0]);
	fp2_rand(a[1]);
	fp2_rand(a[2]);
}

void fp6_print(fp6_t a) {
	fp2_print(a[0]);
	fp2_print(a[1]);
	fp2_print(a[2]);
}

int fp6_size_bin(fp6_t a) {
	return 6 * FP_BYTES;
}

void fp6_read_bin(fp6_t a, uint8_t *bin, int len) {
	if (len != 6 * FP_BYTES) {
		THROW(ERR_NO_BUFFER);
	}
	fp2_read_bin(a[0], bin, 2 * FP_BYTES);
	fp2_read_bin(a[1], bin + 2 * FP_BYTES, 2 * FP_BYTES);
	fp2_read_bin(a[2], bin + 4 * FP_BYTES, 2 * FP_BYTES);
}

void fp6_write_bin(uint8_t *bin, int len, fp6_t a) {
	if (len != 6 * FP_BYTES) {
		THROW(ERR_NO_BUFFER);
	}
	fp2_write_bin(bin, 2 * FP_BYTES, a[0], 0);
	fp2_write_bin(bin + 2 * FP_BYTES, 2 * FP_BYTES, a[1], 0);
	fp2_write_bin(bin + 4 * FP_BYTES, 2 * FP_BYTES, a[2], 0);
}

void fp6_set_dig(fp6_t a, dig_t b) {
	fp2_set_dig(a[0], b);
	fp2_zero(a[1]);
	fp2_zero(a[2]);
}

void fp12_copy(fp12_t c, fp12_t a) {
	fp6_copy(c[0], a[0]);
	fp6_copy(c[1], a[1]);
}

void fp12_zero(fp12_t a) {
	fp6_zero(a[0]);
	fp6_zero(a[1]);
}

int fp12_is_zero(fp12_t a) {
	return (fp6_is_zero(a[0]) && fp6_is_zero(a[1]));
}

void fp12_rand(fp12_t a) {
	fp6_rand(a[0]);
	fp6_rand(a[1]);
}

void fp12_print(fp12_t a) {
	fp6_print(a[0]);
	fp6_print(a[1]);
}

int fp12_size_bin(fp12_t a, int pack) {
	if (pack) {
		if (fp12_test_cyc(a)) {
			return 8 * FP_BYTES;
		} else {
			return 12 * FP_BYTES;
		}
	} else {
		return 12 * FP_BYTES;
	}
}

void fp12_read_bin(fp12_t a, uint8_t *bin, int len) {
	if (len != 8 * FP_BYTES && len != 12 * FP_BYTES) {
		THROW(ERR_NO_BUFFER);
	}
	if (len == 8 * FP_BYTES) {
		fp2_zero(a[0][0]);
		fp2_read_bin(a[0][1], bin, 2 * FP_BYTES);
		fp2_read_bin(a[0][2], bin + 2 * FP_BYTES, 2 * FP_BYTES);
		fp2_read_bin(a[1][0], bin + 4 * FP_BYTES, 2 * FP_BYTES);
		fp2_zero(a[1][1]);
		fp2_read_bin(a[1][2], bin + 6 * FP_BYTES, 2 * FP_BYTES);
		fp12_back_cyc(a, a);
	}
	if (len == 12 * FP_BYTES) {
		fp6_read_bin(a[0], bin, 6 * FP_BYTES);
		fp6_read_bin(a[1], bin + 6 * FP_BYTES, 6 * FP_BYTES);
	}
}

void fp12_write_bin(uint8_t *bin, int len, fp12_t a, int pack) {
	fp12_t t;

	fp12_null(t);

	TRY {
		fp12_new(t);

		if (pack) {
			if (len != 8 * FP_BYTES) {
				THROW(ERR_NO_BUFFER);
			}
			fp12_pck(t, a);
			fp2_write_bin(bin, 2 * FP_BYTES, a[0][1], 0);
			fp2_write_bin(bin + 2 * FP_BYTES, 2 * FP_BYTES, a[0][2], 0);
			fp2_write_bin(bin + 4 * FP_BYTES, 2 * FP_BYTES, a[1][0], 0);
			fp2_write_bin(bin + 6 * FP_BYTES, 2 * FP_BYTES, a[1][2], 0);
		} else {
			if (len != 12 * FP_BYTES) {
				THROW(ERR_NO_BUFFER);
			}
			fp6_write_bin(bin, 6 * FP_BYTES, a[0]);
			fp6_write_bin(bin + 6 * FP_BYTES, 6 * FP_BYTES, a[1]);
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp12_free(t);
	}
}

void fp12_set_dig(fp12_t a, dig_t b) {
	fp6_set_dig(a[0], b);
	fp6_zero(a[1]);
}

void fp18_copy(fp18_t c, fp18_t a) {
	fp6_copy(c[0], a[0]);
	fp6_copy(c[1], a[1]);
	fp6_copy(c[2], a[2]);
}

void fp18_zero(fp18_t a) {
	fp6_zero(a[0]);
	fp6_zero(a[1]);
	fp6_zero(a[2]);
}

int fp18_is_zero(fp18_t a) {
	return (fp6_is_zero(a[0]) && fp6_is_zero(a[1]) && fp6_is_zero(a[2]));
}

void fp18_rand(fp18_t a) {
	fp6_rand(a[0]);
	fp6_rand(a[1]);
	fp6_rand(a[2]);
}

void fp18_print(fp18_t a) {
	fp6_print(a[0]);
	fp6_print(a[1]);
	fp6_print(a[2]);
}

void fp18_set_dig(fp18_t a, dig_t b) {
	fp6_set_dig(a[0], b);
	fp6_zero(a[1]);
	fp6_zero(a[2]);
}
