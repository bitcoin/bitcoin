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
 * Benchmarks for multiple precision integer arithmetic.
 *
 * @ingroup bench
 */

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	bn_t a[BENCH];

	BENCH_FEW("bn_null", bn_null(a[i]), 1);

	BENCH_FEW("bn_new", bn_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	BENCH_FEW("bn_new_size", bn_new_size(a[i], 2 * RLC_BN_DIGS), 1);
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
		bn_clean(a[i]);
	}
	BENCH_FEW("bn_make", bn_make(a[i], RLC_BN_DIGS), 1);
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
	}
	BENCH_FEW("bn_clean", bn_clean(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
	}
	BENCH_FEW("bn_grow", bn_grow(a[i], 2 * RLC_BN_DIGS), 1);
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
		bn_grow(a[i], 2 * RLC_BN_DIGS);
	}
	BENCH_FEW("bn_trim", bn_trim(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
	}
	BENCH_FEW("bn_free", bn_free(a[i]), 1);

	for (int i = 0; i < BENCH; i++) {
		bn_new_size(a[i], 2 * RLC_BN_DIGS);
	}
	BENCH_FEW("bn_free (size)", bn_free(a[i]), 1);
}

static void util(void) {
	dig_t digit;
	char str[RLC_CEIL(RLC_BN_BITS, 8) * 3 + 1];
	uint8_t bin[RLC_CEIL(RLC_BN_BITS, 8)];
	dig_t raw[RLC_BN_DIGS];
	bn_t a, b;

	bn_null(a);
	bn_null(b);

	bn_new(a);
	bn_new(b);

	bn_rand(b, RLC_POS, RLC_BN_BITS);

	BENCH_RUN("bn_copy") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("bn_abs") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_abs(b, a));
	}
	BENCH_END;

	BENCH_RUN("bn_neg") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("bn_sign") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_sign(a));
	}
	BENCH_END;

	BENCH_RUN("bn_zero") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_zero(b));
	}
	BENCH_END;

	BENCH_RUN("bn_is_zero") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("bn_is_even") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_is_even(a));
	}
	BENCH_END;

	BENCH_RUN("bn_bits") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_bits(a));
	}
	BENCH_END;

	BENCH_RUN("bn_get_bit") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_get_bit(a, RLC_BN_BITS / 2));
	}
	BENCH_END;

	BENCH_RUN("bn_set_bit") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_set_bit(a, RLC_BN_BITS / 2, 1));
	}
	BENCH_END;

	BENCH_RUN("bn_ham") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_ham(a));
	}
	BENCH_END;

	BENCH_RUN("bn_get_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_get_dig(&digit, a));
	}
	BENCH_END;

	BENCH_RUN("bn_set_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("bn_set_2b") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_set_2b(a, RLC_BN_BITS / 2));
	}
	BENCH_END;

	BENCH_RUN("bn_rand") {
		BENCH_ADD(bn_rand(a, RLC_POS, RLC_BN_BITS));
	}
	BENCH_END;

	BENCH_RUN("bn_rand_mod") {
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_rand_mod(a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_size_str") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_size_str(a, 10));
	}
	BENCH_END;

	BENCH_RUN("bn_write_str") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_write_str(str, sizeof(str), a, 10));
	}
	BENCH_END;

	BENCH_RUN("bn_read_str") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_read_str(a, str, sizeof(str), 10));
	}
	BENCH_END;

	BENCH_RUN("bn_size_bin") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_size_bin(a));
	}
	BENCH_END;

	BENCH_RUN("bn_write_bin") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_write_bin(bin, bn_size_bin(a), a));
	}
	BENCH_END;

	BENCH_RUN("bn_read_bin") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_read_bin(a, bin, bn_size_bin(a)));
	}
	BENCH_END;

	BENCH_RUN("bn_size_raw") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_size_raw(a));
	}
	BENCH_END;

	BENCH_RUN("bn_write_raw") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_write_raw(raw, bn_size_raw(a), a));
	}
	BENCH_END;

	BENCH_RUN("bn_read_raw") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_read_raw(a, raw, bn_size_raw(a)));
	}
	BENCH_END;

	BENCH_RUN("bn_cmp_abs") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_cmp_abs(b, a));
	}
	BENCH_END;

	BENCH_RUN("bn_cmp_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	BENCH_RUN("bn_cmp") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_cmp(b, a));
	}
	BENCH_END;

	bn_free(a);
	bn_free(b);
}

static void arith(void) {
	bn_t a, b, c, d, e;
	dig_t f;
	int len;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(e);

	bn_new(a);
	bn_new(b);
	bn_new(c);
	bn_new(d);
	bn_new(e);

	BENCH_RUN("bn_add") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_add_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_add_dig(c, a, f));
	}
	BENCH_END;

	BENCH_RUN("bn_sub") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_sub_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_sub_dig(c, a, f));
	}
	BENCH_END;

	BENCH_RUN("bn_mul") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mul(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_mul_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_mul_dig(c, a, f));
	}
	BENCH_END;

#if BN_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("bn_mul_basic") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if BN_MUL == COMBA || !defined(STRIP)
	BENCH_RUN("bn_mul_comba") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mul_comba(c, a, b));
	}
	BENCH_END;
#endif

#if BN_KARAT > 0 || !defined(STRIP)
	BENCH_RUN("bn_mul_karat") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mul_karat(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("bn_sqr") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_sqr(c, a));
	}
	BENCH_END;

#if BN_SQR == BASIC || !defined(STRIP)
	BENCH_RUN("bn_sqr_basic") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if BN_SQR == COMBA || !defined(STRIP)
	BENCH_RUN("bn_sqr_comba") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_sqr_comba(c, a));
	}
	BENCH_END;
#endif

#if BN_KARAT > 0 || !defined(STRIP)
	BENCH_RUN("bn_sqr_karat") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_sqr_karat(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("bn_dbl") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_dbl(c, a));
	}
	BENCH_END;

	BENCH_RUN("bn_hlv") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_hlv(c, a));
	}
	BENCH_END;

	BENCH_RUN("bn_lsh") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_lsh(c, a, RLC_BN_BITS / 2 + RLC_DIG / 2));
	}
	BENCH_END;

	BENCH_RUN("bn_rsh") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_rsh(c, a, RLC_BN_BITS / 2 + RLC_DIG / 2));
	}
	BENCH_END;

	BENCH_RUN("bn_div") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_div(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_div_rem") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_div_rem(c, d, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_div_dig") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		do {
			bn_rand(b, RLC_POS, RLC_DIG);
		} while (bn_is_zero(b));
		BENCH_ADD(bn_div_dig(c, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("bn_div_rem_dig") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		do {
			bn_rand(b, RLC_POS, RLC_DIG);
		} while (bn_is_zero(b));
		BENCH_ADD(bn_div_rem_dig(c, &f, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("bn_mod_2b") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mod_2b(c, a, RLC_BN_BITS / 2));
	}
	BENCH_END;

	BENCH_RUN("bn_mod_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		do {
			bn_rand(b, RLC_POS, RLC_DIG);
		} while (bn_is_zero(b));
		BENCH_ADD(bn_mod_dig(&f, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("bn_mod") {
#if BN_MOD == PMERS
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_set_2b(b, RLC_BN_BITS);
		bn_rand(c, RLC_POS, RLC_DIG);
		bn_sub(b, b, c);
		bn_mod_pre(d, b);
#else
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod_pre(d, b);
#endif
		BENCH_ADD(bn_mod(c, a, b, d));
	}
	BENCH_END;

#if BN_MOD == BASIC || !defined(STRIP)
	BENCH_RUN("bn_mod_basic") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mod_basic(c, a, b));
	}
	BENCH_END;
#endif

#if BN_MOD == BARRT || !defined(STRIP)
	BENCH_RUN("bn_mod_pre_barrt") {
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mod_pre_barrt(d, b));
	}
	BENCH_END;
#endif

#if BN_MOD == BARRT || !defined(STRIP)
	BENCH_RUN("bn_mod_barrt") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		bn_mod_pre_barrt(d, b);
		BENCH_ADD(bn_mod_barrt(c, a, b, d));
	}
	BENCH_END;
#endif

#if BN_MOD == MONTY || !defined(STRIP)
	BENCH_RUN("bn_mod_pre_monty") {
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		BENCH_ADD(bn_mod_pre_monty(d, b));
	}
	BENCH_END;

	BENCH_RUN("bn_mod_monty_conv") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		BENCH_ADD(bn_mod_monty_conv(a, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_mod_monty") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		bn_mod_pre_monty(d, b);
		BENCH_ADD(bn_mod_monty(c, a, b, d));
	}
	BENCH_END;

#if BN_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("bn_mod_monty_basic") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		bn_mod_pre_monty(d, b);
		BENCH_ADD(bn_mod_monty_basic(c, a, b, d));
	}
	BENCH_END;
#endif

#if BN_MUL == COMBA || !defined(STRIP)
	BENCH_RUN("bn_mod_monty_comba") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		bn_mod_pre_monty(d, b);
		BENCH_ADD(bn_mod_monty_comba(c, a, b, d));
	}
	BENCH_END;
#endif

	BENCH_RUN("bn_mod_monty_back") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		bn_mod_pre_monty(d, b);
		BENCH_ADD(bn_mod_monty_back(c, c, b));
	}
	BENCH_END;
#endif

#if BN_MOD == PMERS || !defined(STRIP)
	BENCH_RUN("bn_mod_pre_pmers") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_set_2b(b, RLC_BN_BITS);
		bn_rand(c, RLC_POS, RLC_DIG);
		bn_sub(b, b, c);
		BENCH_ADD(bn_mod_pre_pmers(d, b));
	}
	BENCH_END;

	BENCH_RUN("bn_mod_pmers") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_set_2b(b, RLC_BN_BITS);
		bn_rand(c, RLC_POS, RLC_DIG);
		bn_sub(b, b, c);
		bn_mod_pre_pmers(d, b);
		BENCH_ADD(bn_mod_pmers(c, a, b, d));
	}
	BENCH_END;
#endif

	BENCH_RUN("bn_mxp") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
#if BN_MOD != PMERS
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
#else
		bn_set_2b(b, RLC_BN_BITS);
		bn_rand(c, RLC_POS, RLC_DIG);
		bn_sub(b, b, c);
#endif
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp(c, a, b, b));
	}
	BENCH_END;

#if BN_MXP == BASIC || !defined(STRIP)
	BENCH_RUN("bn_mxp_basic") {
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp_basic(c, a, b, b));
	}
	BENCH_END;
#endif

#if BN_MXP == SLIDE || !defined(STRIP)
	BENCH_RUN("bn_mxp_slide") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp_slide(c, a, b, b));
	}
	BENCH_END;
#endif

#if BN_MXP == CONST || !defined(STRIP)
	BENCH_RUN("bn_mxp_monty") {
		bn_rand(a, RLC_POS, 2 * RLC_BN_BITS - RLC_DIG / 2);
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp_monty(c, a, b, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("bn_mxp_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(d, RLC_POS, RLC_DIG);
		bn_get_dig(&f, d);
		BENCH_ADD(bn_mxp_dig(c, a, f, b));
	}
	BENCH_END;

	BENCH_RUN("bn_srt") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_srt(b, a));
	}
	BENCH_END;

	BENCH_RUN("bn_gcd") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd(c, a, b));
	}
	BENCH_END;

#if BN_GCD == BASIC || !defined(STRIP)
	BENCH_RUN("bn_gcd_basic") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_basic(c, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == LEHME || !defined(STRIP)
	BENCH_RUN("bn_gcd_lehme") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_lehme(c, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == STEIN || !defined(STRIP)
	BENCH_RUN("bn_gcd_stein") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_stein(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("bn_gcd_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_DIG);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_gcd_dig(c, a, f));
	}
	BENCH_END;

	BENCH_RUN("bn_gcd_ext") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_ext(c, d, e, a, b));
	}
	BENCH_END;

#if BN_GCD == BASIC || !defined(STRIP)
	BENCH_RUN("bn_gcd_ext_basic") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_basic(c, d, e, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == LEHME || !defined(STRIP)
	BENCH_RUN("bn_gcd_ext_lehme") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_lehme(c, d, e, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == STEIN || !defined(STRIP)
	BENCH_RUN("bn_gcd_ext_stein") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_stein(c, d, e, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("bn_gcd_ext_mid") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_mid(c, c, d, d, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_gcd_ext_dig") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_DIG);
		BENCH_ADD(bn_gcd_ext_dig(c, d, e, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("bn_lcm") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_lcm(c, a, b));
	}
	BENCH_END;

	bn_gen_prime(b, RLC_BN_BITS);

	BENCH_RUN("bn_smb_leg") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_smb_leg(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("bn_smb_jac") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		BENCH_ADD(bn_smb_jac(c, a, b));
	}
	BENCH_END;

	BENCH_ONE("bn_gen_prime", bn_gen_prime(a, RLC_BN_BITS), 1);

#if BN_GEN == BASIC || !defined(STRIP)
	BENCH_ONE("bn_gen_prime_basic", bn_gen_prime_basic(a, RLC_BN_BITS), 1);
#endif

#if BN_GEN == SAFEP || !defined(STRIP)
	BENCH_ONE("bn_gen_prime_safep", bn_gen_prime_safep(a, RLC_BN_BITS), 1);
#endif

#if BN_GEN == STRON || !defined(STRIP)
	BENCH_ONE("bn_gen_prime_stron", bn_gen_prime_stron(a, RLC_BN_BITS), 1);
#endif

	BENCH_ONE("bn_is_prime", bn_is_prime(a), 1);

	BENCH_ONE("bn_is_prime_basic", bn_is_prime_basic(a), 1);

	BENCH_ONE("bn_is_prime_rabin", bn_is_prime_rabin(a), 1);

	BENCH_ONE("bn_is_prime_solov", bn_is_prime_solov(a), 1);

	/* It should be the case that a is prime here. */
	BENCH_RUN("bn_mod_inv") {
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_mod_inv(c, b, a));
	}
	BENCH_END;

	bn_rand(a, RLC_POS, RLC_BN_BITS);

	BENCH_ONE("bn_factor", bn_factor(c, a), 1);

	BENCH_RUN("bn_is_factor") {
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD(bn_is_factor(b, a));
	}
	BENCH_END;

	BENCH_RUN("bn_rec_win") {
		uint8_t win[RLC_BN_BITS + 1];
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD((len = RLC_BN_BITS + 1, bn_rec_win(win, &len, a, 4)));
	}
	BENCH_END;

	BENCH_RUN("bn_rec_slw") {
		uint8_t win[RLC_BN_BITS + 1];
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD((len = RLC_BN_BITS + 1, bn_rec_slw(win, &len, a, 4)));
	}
	BENCH_END;

	BENCH_RUN("bn_rec_naf") {
		int8_t naf[RLC_BN_BITS + 1];
		int len;
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD((len = RLC_BN_BITS + 1, bn_rec_naf(naf, &len, a, 4)));
	}
	BENCH_END;

#if defined(WITH_EB) && defined(EB_KBLTZ) && (EB_MUL == LWNAF || EB_MUL == RWNAF || EB_FIX == LWNAF || EB_SIM == INTER || !defined(STRIP))
	if (eb_param_set_any_kbltz() == RLC_OK) {
		BENCH_RUN("bn_rec_tnaf") {
			int8_t tnaf[RLC_FB_BITS + 8];
			int len = RLC_BN_BITS + 1;
			eb_curve_get_ord(e);
			bn_rand_mod(a, e);
			if (eb_curve_opt_a() == RLC_ZERO) {
				BENCH_ADD((len = RLC_FB_BITS + 8, bn_rec_tnaf(tnaf, &len, a, -1, RLC_FB_BITS, 4)));
			} else {
				BENCH_ADD((len = RLC_FB_BITS + 8, bn_rec_tnaf(tnaf, &len, a, 1, RLC_FB_BITS, 4)));
			}
		}
		BENCH_END;

		BENCH_RUN("bn_rec_rtnaf") {
			int8_t tnaf[RLC_FB_BITS + 8];
			eb_curve_get_ord(e);
			bn_rand_mod(a, e);
			if (eb_curve_opt_a() == RLC_ZERO) {
				BENCH_ADD((len = RLC_FB_BITS + 8, bn_rec_rtnaf(tnaf, &len, a, -1, RLC_FB_BITS, 4)));
			} else {
				BENCH_ADD((len = RLC_FB_BITS + 8, bn_rec_rtnaf(tnaf, &len, a, 1, RLC_FB_BITS, 4)));
			}
		}
		BENCH_END;
	}
#endif

	BENCH_RUN("bn_rec_reg") {
		int8_t naf[RLC_BN_BITS + 1];
		int len = RLC_BN_BITS + 1;
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		BENCH_ADD((len = RLC_BN_BITS + 1, bn_rec_reg(naf, &len, a, RLC_BN_BITS, 4)));
	}
	BENCH_END;

	BENCH_RUN("bn_rec_jsf") {
		int8_t jsf[2 * (RLC_BN_BITS + 1)];
		bn_rand(a, RLC_POS, RLC_BN_BITS);
		bn_rand(b, RLC_POS, RLC_BN_BITS);
		BENCH_ADD((len = 2 * (RLC_BN_BITS + 1), bn_rec_jsf(jsf, &len, a, b)));
	}
	BENCH_END;

#if defined(WITH_EP) && defined(EP_ENDOM) && (EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || EP_SIM == INTER || !defined(STRIP))
	if (ep_param_set_any_endom() == RLC_OK) {
		bn_t v1[3], v2[3];

		for (int j = 0; j < 3; j++) {
			bn_new(v1[j]);
			bn_new(v2[j]);
		}

		BENCH_RUN("bn_rec_glv") {
			bn_rand(a, RLC_POS, RLC_FP_BITS);
			ep_curve_get_v1(v1);
			ep_curve_get_v2(v2);
			ep_curve_get_ord(e);
			bn_rand_mod(a, e);
			BENCH_ADD(bn_rec_glv(b, c, a, e, (const bn_t *)v1,
							(const bn_t *)v2));
		}
		BENCH_END;

		for (int j = 0; j < 3; j++) {
			bn_free(v1[j]);
			bn_free(v2[j]);
		}
	}
#endif /* WITH_EP && EP_KBLTZ */

	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(e);
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the BN module:", 0);
	util_banner("Utilities:", 1);
	memory();
	util();
	util_banner("Arithmetic:", 1);
	arith();

	core_clean();
	return 0;
}
