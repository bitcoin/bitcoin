/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
		if (fp2_test_cyc(a)) {
			return RLC_FP_BYTES + 1;
		} else {
			return 2 * RLC_FP_BYTES;
		}
	} else {
		return 2 * RLC_FP_BYTES;
	}
}

void fp2_read_bin(fp2_t a, const uint8_t *bin, int len) {
	if (len != RLC_FP_BYTES + 1 && len != 2 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	if (len == RLC_FP_BYTES + 1) {
		fp_read_bin(a[0], bin, RLC_FP_BYTES);
		fp_zero(a[1]);
		fp_set_bit(a[1], 0, bin[RLC_FP_BYTES]);
		fp2_upk(a, a);
	}
	if (len == 2 * RLC_FP_BYTES) {
		fp_read_bin(a[0], bin, RLC_FP_BYTES);
		fp_read_bin(a[1], bin + RLC_FP_BYTES, RLC_FP_BYTES);
	}
}

void fp2_write_bin(uint8_t *bin, int len, fp2_t a, int pack) {
	fp2_t t;

	fp2_null(t);

	RLC_TRY {
		fp2_new(t);

		if (pack && fp2_test_cyc(a)) {
			if (len < RLC_FP_BYTES + 1) {
				RLC_THROW(ERR_NO_BUFFER);
				return;
			} else {
				fp2_pck(t, a);
				fp_write_bin(bin, RLC_FP_BYTES, t[0]);
				bin[RLC_FP_BYTES] = fp_get_bit(t[1], 0);
			}
		} else {
			if (len < 2 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
				return;
			} else {
				fp_write_bin(bin, RLC_FP_BYTES, a[0]);
				fp_write_bin(bin + RLC_FP_BYTES, RLC_FP_BYTES, a[1]);
			}
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
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
	return 3 * RLC_FP_BYTES;
}

void fp3_read_bin(fp3_t a, const uint8_t *bin, int len) {
	if (len != 3 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp_read_bin(a[0], bin, RLC_FP_BYTES);
	fp_read_bin(a[1], bin + RLC_FP_BYTES, RLC_FP_BYTES);
	fp_read_bin(a[2], bin + 2 * RLC_FP_BYTES, RLC_FP_BYTES);
}

void fp3_write_bin(uint8_t *bin, int len, fp3_t a) {
	if (len != 3 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp_write_bin(bin, RLC_FP_BYTES, a[0]);
	fp_write_bin(bin + RLC_FP_BYTES, RLC_FP_BYTES, a[1]);
	fp_write_bin(bin + 2 * RLC_FP_BYTES, RLC_FP_BYTES, a[2]);
}

void fp3_set_dig(fp3_t a, dig_t b) {
	fp_set_dig(a[0], b);
	fp_zero(a[1]);
	fp_zero(a[2]);
}

void fp4_copy(fp4_t c, fp4_t a) {
	fp2_copy(c[0], a[0]);
	fp2_copy(c[1], a[1]);
}

void fp4_zero(fp4_t a) {
	fp2_zero(a[0]);
	fp2_zero(a[1]);
}

int fp4_is_zero(fp4_t a) {
	return fp2_is_zero(a[0]) && fp2_is_zero(a[1]);
}

void fp4_rand(fp4_t a) {
	fp2_rand(a[0]);
	fp2_rand(a[1]);
}

void fp4_print(fp4_t a) {
	fp2_print(a[0]);
	fp2_print(a[1]);
}

int fp4_size_bin(fp4_t a) {
	return 4 * RLC_FP_BYTES;
}

void fp4_read_bin(fp4_t a, const uint8_t *bin, int len) {
	if (len != 4 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp2_read_bin(a[0], bin, 2 * RLC_FP_BYTES);
	fp2_read_bin(a[1], bin + 2 * RLC_FP_BYTES, 2 * RLC_FP_BYTES);
}

void fp4_write_bin(uint8_t *bin, int len, fp4_t a) {
	if (len != 4 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp2_write_bin(bin, 2 * RLC_FP_BYTES, a[0], 0);
	fp2_write_bin(bin + 2 * RLC_FP_BYTES, 2 * RLC_FP_BYTES, a[1], 0);
}

void fp4_set_dig(fp4_t a, dig_t b) {
	fp2_set_dig(a[0], b);
	fp2_zero(a[1]);
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
	return 6 * RLC_FP_BYTES;
}

void fp6_read_bin(fp6_t a, const uint8_t *bin, int len) {
	if (len != 6 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp2_read_bin(a[0], bin, 2 * RLC_FP_BYTES);
	fp2_read_bin(a[1], bin + 2 * RLC_FP_BYTES, 2 * RLC_FP_BYTES);
	fp2_read_bin(a[2], bin + 4 * RLC_FP_BYTES, 2 * RLC_FP_BYTES);
}

void fp6_write_bin(uint8_t *bin, int len, fp6_t a) {
	if (len != 6 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp2_write_bin(bin, 2 * RLC_FP_BYTES, a[0], 0);
	fp2_write_bin(bin + 2 * RLC_FP_BYTES, 2 * RLC_FP_BYTES, a[1], 0);
	fp2_write_bin(bin + 4 * RLC_FP_BYTES, 2 * RLC_FP_BYTES, a[2], 0);
}

void fp6_set_dig(fp6_t a, dig_t b) {
	fp2_set_dig(a[0], b);
	fp2_zero(a[1]);
	fp2_zero(a[2]);
}

void fp8_copy(fp8_t c, fp8_t a) {
	fp4_copy(c[0], a[0]);
	fp4_copy(c[1], a[1]);
}

void fp8_zero(fp8_t a) {
	fp4_zero(a[0]);
	fp4_zero(a[1]);
}

int fp8_is_zero(fp8_t a) {
	return fp4_is_zero(a[0]) && fp4_is_zero(a[1]);
}

void fp8_rand(fp8_t a) {
	fp4_rand(a[0]);
	fp4_rand(a[1]);
}

void fp8_print(fp8_t a) {
	fp4_print(a[0]);
	fp4_print(a[1]);
}

int fp8_size_bin(fp8_t a, int pack) {
	if (pack) {
		if (fp8_test_cyc(a)) {
			return 4 * RLC_FP_BYTES;
		} else {
			return 8 * RLC_FP_BYTES;
		}
	} else {
		return 8 * RLC_FP_BYTES;
	}
}

void fp8_read_bin(fp8_t a, const uint8_t *bin, int len) {
	if (len != 8 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp4_read_bin(a[0], bin, 4 * RLC_FP_BYTES);
	fp4_read_bin(a[1], bin + 4 * RLC_FP_BYTES, 4 * RLC_FP_BYTES);
}

void fp8_write_bin(uint8_t *bin, int len, fp8_t a) {
	if (len != 8 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp4_write_bin(bin, 4 * RLC_FP_BYTES, a[0]);
	fp4_write_bin(bin + 4 * RLC_FP_BYTES, 4 * RLC_FP_BYTES, a[1]);
}

void fp8_set_dig(fp8_t a, dig_t b) {
	fp4_set_dig(a[0], b);
	fp4_zero(a[1]);
}

void fp9_copy(fp9_t c, fp9_t a) {
	fp3_copy(c[0], a[0]);
	fp3_copy(c[1], a[1]);
	fp3_copy(c[2], a[2]);
}

void fp9_zero(fp9_t a) {
	fp3_zero(a[0]);
	fp3_zero(a[1]);
	fp3_zero(a[2]);
}

int fp9_is_zero(fp9_t a) {
	return fp3_is_zero(a[0]) && fp3_is_zero(a[1]) && fp3_is_zero(a[2]);
}

void fp9_rand(fp9_t a) {
	fp3_rand(a[0]);
	fp3_rand(a[1]);
	fp3_rand(a[2]);
}

void fp9_print(fp9_t a) {
	fp3_print(a[0]);
	fp3_print(a[1]);
	fp3_print(a[2]);
}

int fp9_size_bin(fp9_t a) {
	return 9 * RLC_FP_BYTES;
}

void fp9_read_bin(fp9_t a, const uint8_t *bin, int len) {
	if (len != 9 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp3_read_bin(a[0], bin, 3 * RLC_FP_BYTES);
	fp3_read_bin(a[1], bin + 3 * RLC_FP_BYTES, 3 * RLC_FP_BYTES);
	fp3_read_bin(a[2], bin + 6 * RLC_FP_BYTES, 3 * RLC_FP_BYTES);
}

void fp9_write_bin(uint8_t *bin, int len, fp9_t a) {
	if (len != 9 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp3_write_bin(bin, 3 * RLC_FP_BYTES, a[0]);
	fp3_write_bin(bin + 3 * RLC_FP_BYTES, 3 * RLC_FP_BYTES, a[1]);
	fp3_write_bin(bin + 6 * RLC_FP_BYTES, 3 * RLC_FP_BYTES, a[2]);
}

void fp9_set_dig(fp9_t a, dig_t b) {
	fp3_set_dig(a[0], b);
	fp3_zero(a[1]);
	fp3_zero(a[2]);
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
			return 8 * RLC_FP_BYTES;
		} else {
			return 12 * RLC_FP_BYTES;
		}
	} else {
		return 12 * RLC_FP_BYTES;
	}
}

void fp12_read_bin(fp12_t a, const uint8_t *bin, int len) {
	if (len != 8 * RLC_FP_BYTES && len != 12 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	if (len == 8 * RLC_FP_BYTES) {
		fp2_zero(a[0][0]);
		fp2_read_bin(a[0][1], bin, 2 * RLC_FP_BYTES);
		fp2_read_bin(a[0][2], bin + 2 * RLC_FP_BYTES, 2 * RLC_FP_BYTES);
		fp2_read_bin(a[1][0], bin + 4 * RLC_FP_BYTES, 2 * RLC_FP_BYTES);
		fp2_zero(a[1][1]);
		fp2_read_bin(a[1][2], bin + 6 * RLC_FP_BYTES, 2 * RLC_FP_BYTES);
		fp12_back_cyc(a, a);
	}
	if (len == 12 * RLC_FP_BYTES) {
		fp6_read_bin(a[0], bin, 6 * RLC_FP_BYTES);
		fp6_read_bin(a[1], bin + 6 * RLC_FP_BYTES, 6 * RLC_FP_BYTES);
	}
}

void fp12_write_bin(uint8_t *bin, int len, fp12_t a, int pack) {
	fp12_t t;

	fp12_null(t);

	RLC_TRY {
		fp12_new(t);

		if (pack) {
			if (len != 8 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp12_pck(t, a);
			fp2_write_bin(bin, 2 * RLC_FP_BYTES, a[0][1], 0);
			fp2_write_bin(bin + 2 * RLC_FP_BYTES, 2 * RLC_FP_BYTES, a[0][2], 0);
			fp2_write_bin(bin + 4 * RLC_FP_BYTES, 2 * RLC_FP_BYTES, a[1][0], 0);
			fp2_write_bin(bin + 6 * RLC_FP_BYTES, 2 * RLC_FP_BYTES, a[1][2], 0);
		} else {
			if (len != 12 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp6_write_bin(bin, 6 * RLC_FP_BYTES, a[0]);
			fp6_write_bin(bin + 6 * RLC_FP_BYTES, 6 * RLC_FP_BYTES, a[1]);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp12_free(t);
	}
}

void fp12_set_dig(fp12_t a, dig_t b) {
	fp6_set_dig(a[0], b);
	fp6_zero(a[1]);
}

void fp18_copy(fp18_t c, fp18_t a) {
	fp9_copy(c[0], a[0]);
	fp9_copy(c[1], a[1]);
}

void fp18_zero(fp18_t a) {
	fp9_zero(a[0]);
	fp9_zero(a[1]);
}

int fp18_is_zero(fp18_t a) {
	return (fp9_is_zero(a[0]) && fp9_is_zero(a[1]));
}

void fp18_rand(fp18_t a) {
	fp9_rand(a[0]);
	fp9_rand(a[1]);
}

void fp18_print(fp18_t a) {
	fp9_print(a[0]);
	fp9_print(a[1]);
}

int fp18_size_bin(fp18_t a) {
	return 18 * RLC_FP_BYTES;
}

void fp18_read_bin(fp18_t a, const uint8_t *bin, int len) {
	if (len != 18 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp9_read_bin(a[0], bin, 9 * RLC_FP_BYTES);
	fp9_read_bin(a[1], bin + 9 * RLC_FP_BYTES, 9 * RLC_FP_BYTES);
}

void fp18_write_bin(uint8_t *bin, int len, fp18_t a) {
	if (len != 18 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	fp9_write_bin(bin, 9 * RLC_FP_BYTES, a[0]);
	fp9_write_bin(bin + 9 * RLC_FP_BYTES, 9 * RLC_FP_BYTES, a[1]);
}

void fp18_set_dig(fp18_t a, dig_t b) {
	fp9_set_dig(a[0], b);
	fp9_zero(a[1]);
}

void fp24_copy(fp24_t c, fp24_t a) {
	fp8_copy(c[0], a[0]);
	fp8_copy(c[1], a[1]);
	fp8_copy(c[2], a[2]);
}

void fp24_zero(fp24_t a) {
	fp8_zero(a[0]);
	fp8_zero(a[1]);
	fp8_zero(a[2]);
}

int fp24_is_zero(fp24_t a) {
	return fp8_is_zero(a[0]) && fp8_is_zero(a[1]) && fp8_is_zero(a[2]);
}

void fp24_rand(fp24_t a) {
	fp8_rand(a[0]);
	fp8_rand(a[1]);
	fp8_rand(a[2]);
}

void fp24_print(fp24_t a) {
	fp8_print(a[0]);
	fp8_print(a[1]);
	fp8_print(a[2]);
}

int fp24_size_bin(fp24_t a, int pack) {
	if (pack) {
		if (fp24_test_cyc(a)) {
			return 16 * RLC_FP_BYTES;
		} else {
			return 24 * RLC_FP_BYTES;
		}
	} else {
		return 24 * RLC_FP_BYTES;
	}
}

void fp24_read_bin(fp24_t a, const uint8_t *bin, int len) {
	if (len != 16 * RLC_FP_BYTES && len != 24 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	if (len == 16 * RLC_FP_BYTES) {
		fp4_zero(a[0][0]);
		fp4_zero(a[0][1]);
		fp4_read_bin(a[1][0], bin, 4 * RLC_FP_BYTES);
		fp4_read_bin(a[1][1], bin + 4 * RLC_FP_BYTES, 4 * RLC_FP_BYTES);
		fp4_read_bin(a[2][0], bin + 8 * RLC_FP_BYTES, 4 * RLC_FP_BYTES);
		fp4_read_bin(a[2][1], bin + 12 * RLC_FP_BYTES, 4 * RLC_FP_BYTES);
		fp24_back_cyc(a, a);
	}
	if (len == 24 * RLC_FP_BYTES) {
		fp8_read_bin(a[0], bin, 8 * RLC_FP_BYTES);
		fp8_read_bin(a[1], bin + 8 * RLC_FP_BYTES, 8 * RLC_FP_BYTES);
		fp8_read_bin(a[2], bin + 16 * RLC_FP_BYTES, 8 * RLC_FP_BYTES);
	}
}

void fp24_write_bin(uint8_t *bin, int len, fp24_t a, int pack) {
	fp24_t t;

	fp24_null(t);

	RLC_TRY {
		fp24_new(t);

		if (pack) {
			if (len != 16 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp24_pck(t, a);
			fp4_write_bin(bin, 4 * RLC_FP_BYTES, a[1][0]);
			fp4_write_bin(bin + 4 * RLC_FP_BYTES, 4 * RLC_FP_BYTES, a[1][1]);
			fp4_write_bin(bin + 8 * RLC_FP_BYTES, 4 * RLC_FP_BYTES, a[2][0]);
			fp4_write_bin(bin + 12 * RLC_FP_BYTES, 4 * RLC_FP_BYTES, a[2][1]);
		} else {
			if (len != 24 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp8_write_bin(bin, 8 * RLC_FP_BYTES, a[0]);
			fp8_write_bin(bin + 8 * RLC_FP_BYTES, 8 * RLC_FP_BYTES, a[1]);
			fp8_write_bin(bin + 16 * RLC_FP_BYTES, 8 * RLC_FP_BYTES, a[2]);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp24_free(t);
	}
}

void fp24_set_dig(fp24_t a, dig_t b) {
	fp8_set_dig(a[0], b);
	fp8_zero(a[1]);
	fp8_zero(a[2]);
}

void fp48_copy(fp48_t c, fp48_t a) {
	fp24_copy(c[0], a[0]);
	fp24_copy(c[1], a[1]);
}

void fp48_zero(fp48_t a) {
	fp24_zero(a[0]);
	fp24_zero(a[1]);
}

int fp48_is_zero(fp48_t a) {
	return (fp24_is_zero(a[0]) && fp24_is_zero(a[1]));
}

void fp48_rand(fp48_t a) {
	fp24_rand(a[0]);
	fp24_rand(a[1]);
}

void fp48_print(fp48_t a) {
	fp24_print(a[0]);
	fp24_print(a[1]);
}

int fp48_size_bin(fp48_t a, int pack) {
	if (pack) {
		if (fp48_test_cyc(a)) {
			return 32 * RLC_FP_BYTES;
		} else {
			return 48 * RLC_FP_BYTES;
		}
	} else {
		return 48 * RLC_FP_BYTES;
	}
}

void fp48_read_bin(fp48_t a, const uint8_t *bin, int len) {
	if (len != 32 * RLC_FP_BYTES && len != 48 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	if (len == 32 * RLC_FP_BYTES) {
		fp8_zero(a[0][0]);
		fp8_read_bin(a[0][1], bin, 8 * RLC_FP_BYTES);
		fp8_read_bin(a[0][2], bin + 8 * RLC_FP_BYTES, 8 * RLC_FP_BYTES);
		fp8_read_bin(a[1][0], bin + 16 * RLC_FP_BYTES, 8 * RLC_FP_BYTES);
		fp8_zero(a[1][1]);
		fp8_read_bin(a[1][2], bin + 24 * RLC_FP_BYTES, 8 * RLC_FP_BYTES);
		fp48_back_cyc(a, a);
	}
	if (len == 48 * RLC_FP_BYTES) {
		fp24_read_bin(a[0], bin, 24 * RLC_FP_BYTES);
		fp24_read_bin(a[1], bin + 24 * RLC_FP_BYTES, 24 * RLC_FP_BYTES);
	}
}

void fp48_write_bin(uint8_t *bin, int len, fp48_t a, int pack) {
	fp48_t t;

	fp48_null(t);

	RLC_TRY {
		fp48_new(t);

		if (pack) {
			if (len != 32 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp48_pck(t, a);
			fp8_write_bin(bin, 8 * RLC_FP_BYTES, a[0][1]);
			fp8_write_bin(bin + 8 * RLC_FP_BYTES, 8 * RLC_FP_BYTES, a[0][2]);
			fp8_write_bin(bin + 16 * RLC_FP_BYTES, 8 * RLC_FP_BYTES, a[1][0]);
			fp8_write_bin(bin + 24 * RLC_FP_BYTES, 8 * RLC_FP_BYTES, a[1][2]);
		} else {
			if (len != 48 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp24_write_bin(bin, 24 * RLC_FP_BYTES, a[0], 0);
			fp24_write_bin(bin + 24 * RLC_FP_BYTES, 24 * RLC_FP_BYTES, a[1], 0);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp48_free(t);
	}
}

void fp48_set_dig(fp48_t a, dig_t b) {
	fp24_set_dig(a[0], b);
	fp24_zero(a[1]);
}

void fp54_copy(fp54_t c, fp54_t a) {
	fp18_copy(c[0], a[0]);
	fp18_copy(c[1], a[1]);
	fp18_copy(c[2], a[2]);
}

void fp54_zero(fp54_t a) {
	fp18_zero(a[0]);
	fp18_zero(a[1]);
	fp18_zero(a[2]);
}

int fp54_is_zero(fp54_t a) {
	return (fp18_is_zero(a[0]) && fp18_is_zero(a[1]) && fp18_is_zero(a[2]));
}

void fp54_rand(fp54_t a) {
	fp18_rand(a[0]);
	fp18_rand(a[1]);
	fp18_rand(a[2]);
}

void fp54_print(fp54_t a) {
	fp18_print(a[0]);
	fp18_print(a[1]);
	fp18_print(a[2]);
}

int fp54_size_bin(fp54_t a, int pack) {
	if (pack) {
		if (fp54_test_cyc(a)) {
			return 36 * RLC_FP_BYTES;
		} else {
			return 54 * RLC_FP_BYTES;
		}
	} else {
		return 54 * RLC_FP_BYTES;
	}
}

void fp54_read_bin(fp54_t a, const uint8_t *bin, int len) {
	if (len != 36 * RLC_FP_BYTES && len != 54 * RLC_FP_BYTES) {
		RLC_THROW(ERR_NO_BUFFER);
		return;
	}
	if (len == 36 * RLC_FP_BYTES) {
		fp9_zero(a[0][0]);
		fp9_zero(a[0][1]);
		fp9_read_bin(a[1][0], bin, 9 * RLC_FP_BYTES);
		fp9_read_bin(a[1][1], bin + 9 * RLC_FP_BYTES, 9 * RLC_FP_BYTES);
		fp9_read_bin(a[2][0], bin + 18 * RLC_FP_BYTES, 9 * RLC_FP_BYTES);
		fp9_read_bin(a[2][1], bin + 27 * RLC_FP_BYTES, 9 * RLC_FP_BYTES);
		fp54_back_cyc(a, a);
	}
	if (len == 54 * RLC_FP_BYTES) {
		fp18_read_bin(a[0], bin, 18 * RLC_FP_BYTES);
		fp18_read_bin(a[1], bin + 18 * RLC_FP_BYTES, 18 * RLC_FP_BYTES);
		fp18_read_bin(a[2], bin + 36 * RLC_FP_BYTES, 18 * RLC_FP_BYTES);
	}
}

void fp54_write_bin(uint8_t *bin, int len, fp54_t a, int pack) {
	fp54_t t;

	fp54_null(t);

	RLC_TRY {
		fp54_new(t);

		if (pack) {
			if (len != 36 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp54_pck(t, a);
			fp9_write_bin(bin, 9 * RLC_FP_BYTES, a[1][0]);
			fp9_write_bin(bin + 9 * RLC_FP_BYTES, 9 * RLC_FP_BYTES, a[1][1]);
			fp9_write_bin(bin + 18 * RLC_FP_BYTES, 9 * RLC_FP_BYTES, a[2][0]);
			fp9_write_bin(bin + 27 * RLC_FP_BYTES, 9 * RLC_FP_BYTES, a[2][1]);
		} else {
			if (len != 54 * RLC_FP_BYTES) {
				RLC_THROW(ERR_NO_BUFFER);
			}
			fp18_write_bin(bin, 18 * RLC_FP_BYTES, a[0]);
			fp18_write_bin(bin + 18 * RLC_FP_BYTES, 18 * RLC_FP_BYTES, a[1]);
			fp18_write_bin(bin + 36 * RLC_FP_BYTES, 18 * RLC_FP_BYTES, a[2]);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp54_free(t);
	}
}

void fp54_set_dig(fp54_t a, dig_t b) {
	fp18_set_dig(a[0], b);
	fp18_zero(a[1]);
	fp18_zero(a[2]);
}
