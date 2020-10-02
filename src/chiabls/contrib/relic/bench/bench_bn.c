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
 * Benchmarks for multiple precision integer arithmetic.
 *
 * @ingroup bench
 */

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	bn_t a[BENCH];

	BENCH_SMALL("bn_null", bn_null(a[i]));

	BENCH_SMALL("bn_new", bn_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	BENCH_SMALL("bn_new_size", bn_new_size(a[i], 2 * BN_DIGS));
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
		bn_clean(a[i]);
	}
	BENCH_SMALL("bn_init", bn_init(a[i], BN_DIGS));
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
	}
	BENCH_SMALL("bn_clean", bn_clean(a[i]));
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
	}
	BENCH_SMALL("bn_grow", bn_grow(a[i], 2 * BN_DIGS));
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
		bn_grow(a[i], 2 * BN_DIGS);
	}
	BENCH_SMALL("bn_trim", bn_trim(a[i]));
	for (int i = 0; i < BENCH; i++) {
		bn_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		bn_new(a[i]);
	}
	BENCH_SMALL("bn_free", bn_free(a[i]));

	for (int i = 0; i < BENCH; i++) {
		bn_new_size(a[i], 2 * BN_DIGS);
	}
	BENCH_SMALL("bn_free (size)", bn_free(a[i]));
}

static void util(void) {
	dig_t digit;
	char str[RELIC_BN_BYTES * 3 + 1];
	uint8_t bin[RELIC_BN_BYTES];
	dig_t raw[BN_DIGS];
	bn_t a, b;

	bn_null(a);
	bn_null(b);

	bn_new(a);
	bn_new(b);

	bn_rand(b, BN_POS, RELIC_BN_BITS);

	BENCH_BEGIN("bn_copy") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_copy(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_abs") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_abs(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_neg") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_neg(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_sign") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_sign(a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_zero") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_zero(b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_is_zero") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_is_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_is_even") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_is_even(a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_bits") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_bits(a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_get_bit") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_get_bit(a, RELIC_BN_BITS / 2));
	}
	BENCH_END;

	BENCH_BEGIN("bn_set_bit") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_set_bit(a, RELIC_BN_BITS / 2, 1));
	}
	BENCH_END;

	BENCH_BEGIN("bn_ham") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_ham(a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_get_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_get_dig(&digit, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_set_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("bn_set_2b") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_set_2b(a, RELIC_BN_BITS / 2));
	}
	BENCH_END;

	BENCH_BEGIN("bn_rand") {
		BENCH_ADD(bn_rand(a, BN_POS, RELIC_BN_BITS));
	}
	BENCH_END;

	BENCH_BEGIN("bn_rand_mod") {
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_rand_mod(a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_size_str") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_size_str(a, 10));
	}
	BENCH_END;

	BENCH_BEGIN("bn_write_str") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_write_str(str, sizeof(str), a, 10));
	}
	BENCH_END;

	BENCH_BEGIN("bn_read_str") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_read_str(a, str, sizeof(str), 10));
	}
	BENCH_END;

	BENCH_BEGIN("bn_size_bin") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_size_bin(a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_write_bin") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_write_bin(bin, bn_size_bin(a), a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_read_bin") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_read_bin(a, bin, bn_size_bin(a)));
	}
	BENCH_END;

	BENCH_BEGIN("bn_size_raw") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_size_raw(a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_write_raw") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_write_raw(raw, bn_size_raw(a), a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_read_raw") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_read_raw(a, raw, bn_size_raw(a)));
	}
	BENCH_END;

	BENCH_BEGIN("bn_cmp_abs") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_cmp_abs(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_cmp_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	BENCH_BEGIN("bn_cmp") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
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

	BENCH_BEGIN("bn_add") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_add(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_add_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_add_dig(c, a, f));
	}
	BENCH_END;

	BENCH_BEGIN("bn_sub") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_sub(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_sub_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_sub_dig(c, a, f));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mul") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_mul(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mul_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_mul_dig(c, a, f));
	}
	BENCH_END;

#if BN_MUL == BASIC || !defined(STRIP)
	BENCH_BEGIN("bn_mul_basic") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if BN_MUL == COMBA || !defined(STRIP)
	BENCH_BEGIN("bn_mul_comba") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_mul_comba(c, a, b));
	}
	BENCH_END;
#endif

#if BN_KARAT > 0 || !defined(STRIP)
	BENCH_BEGIN("bn_mul_karat") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_mul_karat(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("bn_sqr") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_sqr(c, a));
	}
	BENCH_END;

#if BN_SQR == BASIC || !defined(STRIP)
	BENCH_BEGIN("bn_sqr_basic") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if BN_SQR == COMBA || !defined(STRIP)
	BENCH_BEGIN("bn_sqr_comba") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_sqr_comba(c, a));
	}
	BENCH_END;
#endif

#if BN_KARAT > 0 || !defined(STRIP)
	BENCH_BEGIN("bn_sqr_karat") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_sqr_karat(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("bn_dbl") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_dbl(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_hlv") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_hlv(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_lsh") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_lsh(c, a, RELIC_BN_BITS / 2 + BN_DIGIT / 2));
	}
	BENCH_END;

	BENCH_BEGIN("bn_rsh") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_rsh(c, a, RELIC_BN_BITS / 2 + BN_DIGIT / 2));
	}
	BENCH_END;

	BENCH_BEGIN("bn_div") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_div(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_div_rem") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_div_rem(c, d, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_div_dig") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		do {
			bn_rand(b, BN_POS, BN_DIGIT);
		} while (bn_is_zero(b));
		BENCH_ADD(bn_div_dig(c, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_BEGIN("bn_div_rem_dig") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		do {
			bn_rand(b, BN_POS, BN_DIGIT);
		} while (bn_is_zero(b));
		BENCH_ADD(bn_div_rem_dig(c, &f, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mod_2b") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_mod_2b(c, a, RELIC_BN_BITS / 2));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mod_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		do {
			bn_rand(b, BN_POS, BN_DIGIT);
		} while (bn_is_zero(b));
		BENCH_ADD(bn_mod_dig(&f, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mod") {
#if BN_MOD == PMERS
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_set_2b(b, RELIC_BN_BITS);
		bn_rand(c, BN_POS, BN_DIGIT);
		bn_sub(b, b, c);
		bn_mod_pre(d, b);
#else
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod_pre(d, b);
#endif
		BENCH_ADD(bn_mod(c, a, b, d));
	}
	BENCH_END;

#if BN_MOD == BASIC || !defined(STRIP)
	BENCH_BEGIN("bn_mod_basic") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_mod_basic(c, a, b));
	}
	BENCH_END;
#endif

#if BN_MOD == BARRT || !defined(STRIP)
	BENCH_BEGIN("bn_mod_pre_barrt") {
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_mod_pre_barrt(d, b));
	}
	BENCH_END;
#endif

#if BN_MOD == BARRT || !defined(STRIP)
	BENCH_BEGIN("bn_mod_barrt") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		bn_mod_pre_barrt(d, b);
		BENCH_ADD(bn_mod_barrt(c, a, b, d));
	}
	BENCH_END;
#endif

#if BN_MOD == MONTY || !defined(STRIP)
	BENCH_BEGIN("bn_mod_pre_monty") {
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		BENCH_ADD(bn_mod_pre_monty(d, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mod_monty_conv") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		BENCH_ADD(bn_mod_monty_conv(a, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mod_monty") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		bn_mod_pre_monty(d, b);
		BENCH_ADD(bn_mod_monty(c, a, b, d));
	}
	BENCH_END;

#if BN_MUL == BASIC || !defined(STRIP)
	BENCH_BEGIN("bn_mod_monty_basic") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
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
	BENCH_BEGIN("bn_mod_monty_comba") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		bn_mod(a, a, b);
		bn_mod_pre_monty(d, b);
		BENCH_ADD(bn_mod_monty_comba(c, a, b, d));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("bn_mod_monty_back") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
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
	BENCH_BEGIN("bn_mod_pre_pmers") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_set_2b(b, RELIC_BN_BITS);
		bn_rand(c, BN_POS, BN_DIGIT);
		bn_sub(b, b, c);
		BENCH_ADD(bn_mod_pre_pmers(d, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_mod_pmers") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_set_2b(b, RELIC_BN_BITS);
		bn_rand(c, BN_POS, BN_DIGIT);
		bn_sub(b, b, c);
		bn_mod_pre_pmers(d, b);
		BENCH_ADD(bn_mod_pmers(c, a, b, d));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("bn_mxp") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
#if BN_MOD != PMERS
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
#else
		bn_set_2b(b, RELIC_BN_BITS);
		bn_rand(c, BN_POS, BN_DIGIT);
		bn_sub(b, b, c);
#endif
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp(c, a, b, b));
	}
	BENCH_END;

#if BN_MXP == BASIC || !defined(STRIP)
	BENCH_BEGIN("bn_mxp_basic") {
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp_basic(c, a, b, b));
	}
	BENCH_END;
#endif

#if BN_MXP == SLIDE || !defined(STRIP)
	BENCH_BEGIN("bn_mxp_slide") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp_slide(c, a, b, b));
	}
	BENCH_END;
#endif

#if BN_MXP == CONST || !defined(STRIP)
	BENCH_BEGIN("bn_mxp_monty") {
		bn_rand(a, BN_POS, 2 * RELIC_BN_BITS - BN_DIGIT / 2);
		bn_mod(a, a, b);
		BENCH_ADD(bn_mxp_monty(c, a, b, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("bn_mxp_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(d, BN_POS, BN_DIGIT);
		bn_get_dig(&f, d);
		BENCH_ADD(bn_mxp_dig(c, a, f, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_srt") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_srt(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_gcd") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd(c, a, b));
	}
	BENCH_END;

#if BN_GCD == BASIC || !defined(STRIP)
	BENCH_BEGIN("bn_gcd_basic") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_basic(c, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == LEHME || !defined(STRIP)
	BENCH_BEGIN("bn_gcd_lehme") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_lehme(c, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == STEIN || !defined(STRIP)
	BENCH_BEGIN("bn_gcd_stein") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_stein(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("bn_gcd_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, BN_DIGIT);
		bn_get_dig(&f, b);
		BENCH_ADD(bn_gcd_dig(c, a, f));
	}
	BENCH_END;

	BENCH_BEGIN("bn_gcd_ext") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_ext(c, d, e, a, b));
	}
	BENCH_END;

#if BN_GCD == BASIC || !defined(STRIP)
	BENCH_BEGIN("bn_gcd_ext_basic") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_basic(c, d, e, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == LEHME || !defined(STRIP)
	BENCH_BEGIN("bn_gcd_ext_lehme") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_lehme(c, d, e, a, b));
	}
	BENCH_END;
#endif

#if BN_GCD == STEIN || !defined(STRIP)
	BENCH_BEGIN("bn_gcd_ext_stein") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_stein(c, d, e, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("bn_gcd_ext_mid") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_gcd_ext_mid(c, c, d, d, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_gcd_ext_dig") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, BN_DIGIT);
		BENCH_ADD(bn_gcd_ext_dig(c, d, e, a, b->dp[0]));
	}
	BENCH_END;

	BENCH_BEGIN("bn_lcm") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_lcm(c, a, b));
	}
	BENCH_END;

	bn_gen_prime(b, RELIC_BN_BITS);

	BENCH_BEGIN("bn_smb_leg") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_smb_leg(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("bn_smb_jac") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		if (bn_is_even(b)) {
			bn_add_dig(b, b, 1);
		}
		BENCH_ADD(bn_smb_jac(c, a, b));
	}
	BENCH_END;

	BENCH_ONCE("bn_gen_prime", bn_gen_prime(a, RELIC_BN_BITS));

#if BN_GEN == BASIC || !defined(STRIP)
	BENCH_ONCE("bn_gen_prime_basic", bn_gen_prime_basic(a, RELIC_BN_BITS));
#endif

#if BN_GEN == SAFEP || !defined(STRIP)
	BENCH_ONCE("bn_gen_prime_safep", bn_gen_prime_safep(a, RELIC_BN_BITS));
#endif

#if BN_GEN == STRON || !defined(STRIP)
	BENCH_ONCE("bn_gen_prime_stron", bn_gen_prime_stron(a, RELIC_BN_BITS));
#endif

	BENCH_ONCE("bn_is_prime", bn_is_prime(a));

	BENCH_ONCE("bn_is_prime_basic", bn_is_prime_basic(a));

	BENCH_ONCE("bn_is_prime_rabin", bn_is_prime_rabin(a));

	BENCH_ONCE("bn_is_prime_solov", bn_is_prime_solov(a));

	bn_rand(a, BN_POS, RELIC_BN_BITS);

	BENCH_ONCE("bn_factor", bn_factor(c, a));

	BENCH_BEGIN("bn_is_factor") {
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD(bn_is_factor(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("bn_rec_win") {
		uint8_t win[RELIC_BN_BITS + 1];
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD((len = RELIC_BN_BITS + 1, bn_rec_win(win, &len, a, 4)));
	}
	BENCH_END;

	BENCH_BEGIN("bn_rec_slw") {
		uint8_t win[RELIC_BN_BITS + 1];
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD((len = RELIC_BN_BITS + 1, bn_rec_slw(win, &len, a, 4)));
	}
	BENCH_END;

	BENCH_BEGIN("bn_rec_naf") {
		signed char naf[RELIC_BN_BITS + 1];
		int len;
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD((len = RELIC_BN_BITS + 1, bn_rec_naf(naf, &len, a, 4)));
	}
	BENCH_END;

#if defined(WITH_EB) && defined(EB_KBLTZ) && (EB_MUL == LWNAF || EB_MUL == RWNAF || EB_FIX == LWNAF || EB_SIM == INTER || !defined(STRIP))
	if (eb_param_set_any_kbltz() == STS_OK) {
		BENCH_BEGIN("bn_rec_tnaf") {
			signed char tnaf[FB_BITS + 8];
			int len = RELIC_BN_BITS + 1;
			eb_curve_get_ord(e);
			bn_rand_mod(a, e);
			if (eb_curve_opt_a() == OPT_ZERO) {
				BENCH_ADD((len = FB_BITS + 8, bn_rec_tnaf(tnaf, &len, a, -1, FB_BITS, 4)));
			} else {
				BENCH_ADD((len = FB_BITS + 8, bn_rec_tnaf(tnaf, &len, a, 1, FB_BITS, 4)));
			}
		}
		BENCH_END;

		BENCH_BEGIN("bn_rec_rtnaf") {
			signed char tnaf[FB_BITS + 8];
			eb_curve_get_ord(e);
			bn_rand_mod(a, e);
			if (eb_curve_opt_a() == OPT_ZERO) {
				BENCH_ADD((len = FB_BITS + 8, bn_rec_rtnaf(tnaf, &len, a, -1, FB_BITS, 4)));
			} else {
				BENCH_ADD((len = FB_BITS + 8, bn_rec_rtnaf(tnaf, &len, a, 1, FB_BITS, 4)));
			}
		}
		BENCH_END;
	}
#endif

	BENCH_BEGIN("bn_rec_reg") {
		signed char naf[RELIC_BN_BITS + 1];
		int len = RELIC_BN_BITS + 1;
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		BENCH_ADD((len = RELIC_BN_BITS + 1, bn_rec_reg(naf, &len, a, RELIC_BN_BITS, 4)));
	}
	BENCH_END;

	BENCH_BEGIN("bn_rec_jsf") {
		signed char jsf[2 * (RELIC_BN_BITS + 1)];
		bn_rand(a, BN_POS, RELIC_BN_BITS);
		bn_rand(b, BN_POS, RELIC_BN_BITS);
		BENCH_ADD((len = 2 * (RELIC_BN_BITS + 1), bn_rec_jsf(jsf, &len, a, b)));
	}
	BENCH_END;

#if defined(WITH_EP) && defined(EP_ENDOM) && (EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || EP_SIM == INTER || !defined(STRIP))
	if (ep_param_set_any_endom() == STS_OK) {
		bn_t v1[3], v2[3];

		for (int j = 0; j < 3; j++) {
			bn_new(v1[j]);
			bn_new(v2[j]);
		}

		BENCH_BEGIN("bn_rec_glv") {
			bn_rand(a, BN_POS, FP_BITS);
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
	if (core_init() != STS_OK) {
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
