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
 * Benchmarks for extensions of prime fields
 *
 * @ingroup bench
 */

#include "relic.h"
#include "relic_bench.h"

static void memory2(void) {
	fp2_t a[BENCH];

	BENCH_FEW("fp2_null", fp2_null(a[i]), 1);

	BENCH_FEW("fp2_new", fp2_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp2_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp2_new(a[i]);
	}
	BENCH_FEW("fp2_free", fp2_free(a[i]), 1);

	(void)a;
}

static void util2(void) {
	uint8_t bin[2 * RLC_FP_BYTES];
	fp2_t a, b;

	fp2_null(a);
	fp2_null(b);

	fp2_new(a);
	fp2_new(b);

	BENCH_RUN("fp2_copy") {
		fp2_rand(a);
		BENCH_ADD(fp2_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_neg") {
		fp2_rand(a);
		BENCH_ADD(fp2_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_zero") {
		fp2_rand(a);
		BENCH_ADD(fp2_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp2_is_zero") {
		fp2_rand(a);
		BENCH_ADD((void)fp2_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp2_set_dig (1)") {
		fp2_rand(a);
		BENCH_ADD(fp2_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp2_set_dig") {
		fp2_rand(a);
		BENCH_ADD(fp2_set_dig(a, a[0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp2_rand") {
		BENCH_ADD(fp2_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp2_size_bin (0)") {
		fp2_rand(a);
		BENCH_ADD(fp2_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp2_size_bin (1)") {
		fp2_rand(a);
		fp2_conv_cyc(a, a);
		BENCH_ADD(fp2_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp2_write_bin (0)") {
		fp2_rand(a);
		BENCH_ADD(fp2_write_bin(bin, sizeof(bin), a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp2_write_bin (1)") {
		fp2_rand(a);
		fp2_conv_cyc(a, a);
		BENCH_ADD(fp2_write_bin(bin, sizeof(bin), a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp2_read_bin (0)") {
		fp2_rand(a);
		fp2_write_bin(bin, sizeof(bin), a, 0);
		BENCH_ADD(fp2_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp2_read_bin") {
		fp2_rand(a);
		fp2_conv_cyc(a, a);
		fp2_write_bin(bin, sizeof(bin), a, 1);
		BENCH_ADD(fp2_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp2_cmp") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_cmp_dig") {
		fp2_rand(a);
		BENCH_ADD(fp2_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp2_free(a);
	fp2_free(b);
}

static void arith2(void) {
	fp2_t a, b, c, d[2];
	bn_t e;

	fp2_new(a);
	fp2_new(b);
	fp2_new(c);
	fp2_new(d[0]);
	fp2_new(d[1]);
	bn_new(e);

	BENCH_RUN("fp2_add") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_add(c, a, b));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_RUN("fp2_add_basic") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_add_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_RUN("fp2_add_integ") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_add_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp2_sub") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_sub(c, a, b));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_RUN("fp2_sub_basic") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_sub_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_RUN("fp2_sub_integ") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_sub_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp2_dbl") {
		fp2_rand(a);
		BENCH_ADD(fp2_dbl(c, a));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_RUN("fp2_dbl_basic") {
		fp2_rand(a);
		BENCH_ADD(fp2_dbl_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_RUN("fp2_dbl_integ") {
		fp2_rand(a);
		BENCH_ADD(fp2_dbl_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp2_mul") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_mul(c, a, b));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_RUN("fp2_mul_basic") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_RUN("fp2_mul_integ") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_mul_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp2_mul_art") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_art(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_mul_nor") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_nor(c, a));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_RUN("fp2_mul_nor_basic") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_nor_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_RUN("fp2_mul_nor_integ") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_nor_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp2_sqr") {
		fp2_rand(a);
		BENCH_ADD(fp2_sqr(c, a));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_RUN("fp2_sqr_basic") {
		fp2_rand(a);
		BENCH_ADD(fp2_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_RUN("fp2_sqr_integ") {
		fp2_rand(a);
		BENCH_ADD(fp2_sqr_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp2_test_cyc") {
		fp2_rand(a);
		fp2_conv_cyc(a, a);
		BENCH_ADD(fp2_test_cyc(a));
	}
	BENCH_END;

	BENCH_RUN("fp2_conv_cyc") {
		fp2_rand(a);
		BENCH_ADD(fp2_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_inv") {
		fp2_rand(a);
		BENCH_ADD(fp2_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_inv_cyc") {
		fp2_rand(a);
		BENCH_ADD(fp2_inv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_inv_sim (2)") {
		fp2_rand(d[0]);
		fp2_rand(d[1]);
		BENCH_ADD(fp2_inv_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp2_exp") {
		fp2_rand(a);
		e->used = RLC_FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), RLC_FP_DIGS);
		BENCH_ADD(fp2_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp2_exp_dig") {
		fp2_rand(a);
		bn_rand(e, RLC_POS, RLC_DIG);
		BENCH_ADD(fp2_exp_dig(c, a, e->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("fp2_exp_cyc") {
		fp2_rand(a);
		e->used = RLC_FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), RLC_FP_DIGS);
		BENCH_ADD(fp2_exp_cyc(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp2_frb") {
		fp2_rand(a);
		BENCH_ADD(fp2_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp2_mul_frb") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_frb(c, a, 1, 0));
	}
	BENCH_END;

	BENCH_RUN("fp2_srt") {
		fp2_rand(a);
		fp2_sqr(a, a);
		BENCH_ADD(fp2_srt(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_pck") {
		fp2_rand(a);
		fp2_conv_cyc(a, a);
		BENCH_ADD(fp2_pck(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp2_upk") {
		fp2_rand(a);
		fp2_conv_cyc(a, a);
		fp2_pck(a, a);
		BENCH_ADD(fp2_upk(c, a));
	}
	BENCH_END;

	fp2_free(a);
	fp2_free(b);
	fp2_free(c);
	fp2_free(d[0]);
	fp2_free(d[1]);
	bn_free(e);
}

static void memory3(void) {
	fp3_t a[BENCH];

	BENCH_FEW("fp3_null", fp3_null(a[i]), 1);

	BENCH_FEW("fp3_new", fp3_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp3_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp3_new(a[i]);
	}
	BENCH_FEW("fp3_free", fp3_free(a[i]), 1);

	(void)a;
}

static void util3(void) {
	uint8_t bin[3 * RLC_FP_BYTES];
	fp3_t a, b;

	fp3_null(a);
	fp3_null(b);

	fp3_new(a);
	fp3_new(b);

	BENCH_RUN("fp3_copy") {
		fp3_rand(a);
		BENCH_ADD(fp3_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp3_neg") {
		fp3_rand(a);
		BENCH_ADD(fp3_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp3_zero") {
		fp3_rand(a);
		BENCH_ADD(fp3_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp3_is_zero") {
		fp3_rand(a);
		BENCH_ADD((void)fp3_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp3_set_dig (1)") {
		fp3_rand(a);
		BENCH_ADD(fp3_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp3_set_dig") {
		fp3_rand(a);
		BENCH_ADD(fp3_set_dig(a, a[0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp3_rand") {
		BENCH_ADD(fp3_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp3_size_bin") {
		fp3_rand(a);
		BENCH_ADD(fp3_size_bin(a));
	}
	BENCH_END;

	BENCH_RUN("fp3_write_bin") {
		fp3_rand(a);
		BENCH_ADD(fp3_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_RUN("fp3_read_bin") {
		fp3_rand(a);
		fp3_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp3_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp3_cmp") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp3_cmp_dig") {
		fp3_rand(a);
		BENCH_ADD(fp3_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp3_free(a);
	fp3_free(b);
}

static void arith3(void) {
	fp3_t a, b, c, d[2];
	bn_t e;

	fp3_new(a);
	fp3_new(b);
	fp3_new(c);
	fp3_new(d[0]);
	fp3_new(d[1]);
	bn_new(e);

	BENCH_RUN("fp3_add") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_add(c, a, b));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_RUN("fp3_add_basic") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_add_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_RUN("fp3_add_integ") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_add_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp3_sub") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_sub(c, a, b));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_RUN("fp3_sub_basic") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_sub_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_RUN("fp3_sub_integ") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_sub_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp3_dbl") {
		fp3_rand(a);
		BENCH_ADD(fp3_dbl(c, a));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_RUN("fp3_dbl_basic") {
		fp3_rand(a);
		BENCH_ADD(fp3_dbl_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_RUN("fp3_dbl_integ") {
		fp3_rand(a);
		BENCH_ADD(fp3_dbl_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp3_mul") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_mul(c, a, b));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_RUN("fp3_mul_basic") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_RUN("fp3_mul_integ") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_mul_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp3_mul_nor") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_nor(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp3_sqr") {
		fp3_rand(a);
		BENCH_ADD(fp3_sqr(c, a));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_RUN("fp3_sqr_basic") {
		fp3_rand(a);
		BENCH_ADD(fp3_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_RUN("fp3_sqr_integ") {
		fp3_rand(a);
		BENCH_ADD(fp3_sqr_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp3_inv") {
		fp3_rand(a);
		BENCH_ADD(fp3_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp3_inv_sim (2)") {
		fp3_rand(d[0]);
		fp3_rand(d[1]);
		BENCH_ADD(fp3_inv_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp3_exp") {
		fp3_rand(a);
		e->used = RLC_FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), RLC_FP_DIGS);
		BENCH_ADD(fp3_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp3_frb") {
		fp3_rand(a);
		BENCH_ADD(fp3_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp3_mul_frb (0,1)") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_frb(c, a, 0, 1));
	}
	BENCH_END;

	BENCH_RUN("fp3_mul_frb (1,1)") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_frb(c, a, 1, 1));
	}
	BENCH_END;

	fp3_free(a);
	fp3_free(b);
	fp3_free(c);
	fp3_free(d[0]);
	fp3_free(d[1]);
	bn_free(e);
}

static void memory4(void) {
	fp4_t a[BENCH];

	BENCH_FEW("fp4_null", fp4_null(a[i]), 1);

	BENCH_FEW("fp4_new", fp4_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp4_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp4_new(a[i]);
	}
	BENCH_FEW("fp4_free", fp4_free(a[i]), 1);

	(void)a;
}

static void util4(void) {
	uint8_t bin[4 * RLC_FP_BYTES];
	fp4_t a, b;

	fp4_null(a);
	fp4_null(b);

	fp4_new(a);
	fp4_new(b);

	BENCH_RUN("fp4_copy") {
		fp4_rand(a);
		BENCH_ADD(fp4_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp4_neg") {
		fp4_rand(a);
		BENCH_ADD(fp4_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp4_zero") {
		fp4_rand(a);
		BENCH_ADD(fp4_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp4_is_zero") {
		fp4_rand(a);
		BENCH_ADD((void)fp4_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp4_set_dig (1)") {
		fp4_rand(a);
		BENCH_ADD(fp4_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp4_set_dig") {
		fp4_rand(a);
		BENCH_ADD(fp4_set_dig(a, a[0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp4_rand") {
		BENCH_ADD(fp4_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp4_size_bin") {
		fp4_rand(a);
		BENCH_ADD(fp4_size_bin(a));
	}
	BENCH_END;

	BENCH_RUN("fp4_write_bin") {
		fp4_rand(a);
		BENCH_ADD(fp4_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_RUN("fp4_read_bin") {
		fp4_rand(a);
		fp4_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp4_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp4_cmp") {
		fp4_rand(a);
		fp4_rand(b);
		BENCH_ADD(fp4_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp4_cmp_dig") {
		fp4_rand(a);
		BENCH_ADD(fp4_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp4_free(a);
	fp4_free(b);
}

static void arith4(void) {
	fp4_t a, b, c;
	bn_t d;

	fp4_new(a);
	fp4_new(b);
	fp4_new(c);
	bn_new(d);

	BENCH_RUN("fp4_add") {
		fp4_rand(a);
		fp4_rand(b);
		BENCH_ADD(fp4_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp4_sub") {
		fp4_rand(a);
		fp4_rand(b);
		BENCH_ADD(fp4_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp4_dbl") {
		fp4_rand(a);
		BENCH_ADD(fp4_dbl(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp4_mul") {
		fp4_rand(a);
		fp4_rand(b);
		BENCH_ADD(fp4_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp4_mul_basic") {
		fp4_rand(a);
		fp4_rand(b);
		BENCH_ADD(fp4_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp4_mul_lazyr") {
		fp4_rand(a);
		fp4_rand(b);
		BENCH_ADD(fp4_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp4_mul_art") {
		fp4_rand(a);
		BENCH_ADD(fp4_mul_art(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp4_sqr") {
		fp4_rand(a);
		BENCH_ADD(fp4_sqr(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp4_sqr_basic") {
		fp4_rand(a);
		BENCH_ADD(fp4_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp4_sqr_lazyr") {
		fp4_rand(a);
		BENCH_ADD(fp4_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp4_inv") {
		fp4_rand(a);
		BENCH_ADD(fp4_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp4_exp") {
		fp4_rand(a);
		d->used = RLC_FP_DIGS;
		dv_copy(d->dp, fp_prime_get(), RLC_FP_DIGS);
		BENCH_ADD(fp4_exp(c, a, d));
	}
	BENCH_END;

	BENCH_RUN("fp4_frb") {
		fp4_rand(a);
		BENCH_ADD(fp4_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp4_srt") {
		fp4_rand(a);
		fp4_sqr(a, a);
		BENCH_ADD(fp4_srt(c, a));
	}
	BENCH_END;

	fp4_free(a);
	fp4_free(b);
	fp4_free(c);
	bn_free(d);
}

static void memory6(void) {
	fp6_t a[BENCH];

	BENCH_FEW("fp6_null", fp6_null(a[i]), 1);

	BENCH_FEW("fp6_new", fp6_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp6_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp6_new(a[i]);
	}
	BENCH_FEW("fp6_free", fp6_free(a[i]), 1);

	(void)a;
}

static void util6(void) {
	uint8_t bin[6 * RLC_FP_BYTES];
	fp6_t a, b;

	fp6_null(a);
	fp6_null(b);

	fp6_new(a);
	fp6_new(b);

	BENCH_RUN("fp6_copy") {
		fp6_rand(a);
		BENCH_ADD(fp6_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp6_neg") {
		fp6_rand(a);
		BENCH_ADD(fp6_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp6_zero") {
		fp6_rand(a);
		BENCH_ADD(fp6_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp6_is_zero") {
		fp6_rand(a);
		BENCH_ADD((void)fp6_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp6_set_dig (1)") {
		fp6_rand(a);
		BENCH_ADD(fp6_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp6_set_dig") {
		fp6_rand(a);
		BENCH_ADD(fp6_set_dig(a, a[0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp6_rand") {
		BENCH_ADD(fp6_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp6_size_bin") {
		fp6_rand(a);
		BENCH_ADD(fp6_size_bin(a));
	}
	BENCH_END;

	BENCH_RUN("fp6_write_bin") {
		fp6_rand(a);
		BENCH_ADD(fp6_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_RUN("fp6_read_bin") {
		fp6_rand(a);
		fp6_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp6_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp6_cmp") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp6_cmp_dig") {
		fp6_rand(a);
		BENCH_ADD(fp6_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp6_free(a);
	fp6_free(b);
}

static void arith6(void) {
	fp6_t a, b, c;
	bn_t d;

	fp6_new(a);
	fp6_new(b);
	fp6_new(c);
	bn_new(d);

	BENCH_RUN("fp6_add") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp6_sub") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp6_dbl") {
		fp6_rand(a);
		BENCH_ADD(fp6_dbl(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp6_mul") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp6_mul_basic") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp6_mul_lazyr") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp6_mul_art") {
		fp6_rand(a);
		BENCH_ADD(fp6_mul_art(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp6_sqr") {
		fp6_rand(a);
		BENCH_ADD(fp6_sqr(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp6_sqr_basic") {
		fp6_rand(a);
		BENCH_ADD(fp6_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp6_sqr_lazyr") {
		fp6_rand(a);
		BENCH_ADD(fp6_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp6_inv") {
		fp6_rand(a);
		BENCH_ADD(fp6_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp6_exp") {
		fp6_rand(a);
		d->used = RLC_FP_DIGS;
		dv_copy(d->dp, fp_prime_get(), RLC_FP_DIGS);
		BENCH_ADD(fp6_exp(c, a, d));
	}
	BENCH_END;

	BENCH_RUN("fp6_frb") {
		fp6_rand(a);
		BENCH_ADD(fp6_frb(c, a, 1));
	}
	BENCH_END;

	fp6_free(a);
	fp6_free(b);
	fp6_free(c);
}

static void memory8(void) {
	fp8_t a[BENCH];

	BENCH_FEW("fp8_null", fp8_null(a[i]), 1);

	BENCH_FEW("fp8_new", fp8_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp8_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp8_new(a[i]);
	}
	BENCH_FEW("fp8_free", fp8_free(a[i]), 1);

	(void)a;
}

static void util8(void) {
	fp8_t a, b;
	uint8_t bin[8 * RLC_FP_BYTES];

	fp8_null(a);
	fp8_null(b);

	fp8_new(a);
	fp8_new(b);

	BENCH_RUN("fp8_copy") {
		fp8_rand(a);
		BENCH_ADD(fp8_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp8_neg") {
		fp8_rand(a);
		BENCH_ADD(fp8_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp8_zero") {
		fp8_rand(a);
		BENCH_ADD(fp8_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp8_is_zero") {
		fp8_rand(a);
		BENCH_ADD((void)fp8_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp8_set_dig (1)") {
		fp8_rand(a);
		BENCH_ADD(fp8_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp8_set_dig") {
		fp8_rand(a);
		BENCH_ADD(fp8_set_dig(a, a[0][0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp8_rand") {
		BENCH_ADD(fp8_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp8_size_bin (0)") {
		fp8_rand(a);
		BENCH_ADD(fp8_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp8_size_bin (1)") {
		fp8_rand(a);
		fp8_conv_cyc(a, a);
		BENCH_ADD(fp8_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp8_write_bin") {
		fp8_rand(a);
		BENCH_ADD(fp8_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_RUN("fp8_read_bin") {
		fp8_rand(a);
		fp8_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp8_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp8_cmp") {
		fp8_rand(a);
		fp8_rand(b);
		BENCH_ADD(fp8_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp8_cmp_dig") {
		fp8_rand(a);
		BENCH_ADD(fp8_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp8_free(a);
	fp8_free(b);
}

static void arith8(void) {
	fp8_t a, b, c, d[2];
	bn_t e;

	fp8_new(a);
	fp8_new(b);
	fp8_new(c);
	fp8_new(d[0]);
	fp8_new(d[1]);
	bn_new(e);

	BENCH_RUN("fp8_add") {
		fp8_rand(a);
		fp8_rand(b);
		BENCH_ADD(fp8_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp8_sub") {
		fp8_rand(a);
		fp8_rand(b);
		BENCH_ADD(fp8_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp8_mul") {
		fp8_rand(a);
		fp8_rand(b);
		BENCH_ADD(fp8_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp8_mul_basic") {
		fp8_rand(a);
		fp8_rand(b);
		BENCH_ADD(fp8_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp8_mul_lazyr") {
		fp8_rand(a);
		fp8_rand(b);
		BENCH_ADD(fp8_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp8_mul_dxs") {
		fp8_rand(a);
		fp8_rand(b);
		BENCH_ADD(fp8_mul_dxs(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp8_sqr") {
		fp8_rand(a);
		BENCH_ADD(fp8_sqr(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp8_sqr_basic") {
		fp8_rand(a);
		BENCH_ADD(fp8_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp8_sqr_lazyr") {
		fp8_rand(a);
		BENCH_ADD(fp8_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp8_sqr_cyc") {
		fp8_rand(a);
		BENCH_ADD(fp8_sqr_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp8_test_cyc") {
		fp8_rand(a);
		fp8_conv_cyc(a, a);
		BENCH_ADD(fp8_test_cyc(a));
	}
	BENCH_END;

	BENCH_RUN("fp8_conv_cyc") {
		fp8_rand(a);
		BENCH_ADD(fp8_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp8_inv") {
		fp8_rand(a);
		BENCH_ADD(fp8_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp8_inv_sim (2)") {
		fp8_rand(d[0]);
		fp8_rand(d[1]);
		BENCH_ADD(fp8_inv_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp8_inv_sim (2)") {
		fp8_rand(d[0]);
		fp8_rand(d[1]);
		BENCH_ADD(fp8_inv_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp8_inv_cyc") {
		fp8_rand(a);
		BENCH_ADD(fp8_inv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp8_exp") {
		fp8_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp8_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp8_exp (cyc)") {
		fp8_rand(a);
		fp8_conv_cyc(a, a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp8_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp8_exp_cyc (param or sparse)") {
		fp8_rand(a);
		fp8_conv_cyc(a, a);
		bn_zero(e);
		fp_prime_get_par(e);
		if (bn_is_zero(e)) {
			bn_set_2b(e, RLC_FP_BITS - 1);
			bn_set_bit(e, RLC_FP_BITS / 2, 1);
			bn_set_bit(e, 0, 1);
		}
		BENCH_ADD(fp8_exp_cyc(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp8_frb") {
		fp8_rand(a);
		BENCH_ADD(fp8_frb(c, a, 1));
	}
	BENCH_END;

	fp8_free(a);
	fp8_free(b);
	fp8_free(c);
	fp8_free(d[0]);
	fp8_free(d[1]);
	bn_free(e);
}

static void memory9(void) {
	fp9_t a[BENCH];

	BENCH_FEW("fp9_null", fp9_null(a[i]), 1);

	BENCH_FEW("fp9_new", fp9_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp9_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp9_new(a[i]);
	}
	BENCH_FEW("fp9_free", fp9_free(a[i]), 1);

	(void)a;
}

static void util9(void) {
	uint8_t bin[9 * RLC_FP_BYTES];
	fp9_t a, b;

	fp9_null(a);
	fp9_null(b);

	fp9_new(a);
	fp9_new(b);

	BENCH_RUN("fp9_copy") {
		fp9_rand(a);
		BENCH_ADD(fp9_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp9_neg") {
		fp9_rand(a);
		BENCH_ADD(fp9_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp9_zero") {
		fp9_rand(a);
		BENCH_ADD(fp9_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp9_is_zero") {
		fp9_rand(a);
		BENCH_ADD((void)fp9_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp9_set_dig (1)") {
		fp9_rand(a);
		BENCH_ADD(fp9_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp9_set_dig") {
		fp9_rand(a);
		BENCH_ADD(fp9_set_dig(a, a[0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp9_rand") {
		BENCH_ADD(fp9_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp9_size_bin") {
		fp9_rand(a);
		BENCH_ADD(fp9_size_bin(a));
	}
	BENCH_END;

	BENCH_RUN("fp9_write_bin") {
		fp9_rand(a);
		BENCH_ADD(fp9_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_RUN("fp9_read_bin") {
		fp9_rand(a);
		fp9_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp9_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp9_cmp") {
		fp9_rand(a);
		fp9_rand(b);
		BENCH_ADD(fp9_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp9_cmp_dig") {
		fp9_rand(a);
		BENCH_ADD(fp9_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp9_free(a);
	fp9_free(b);
}

static void arith9(void) {
	fp9_t a, b, c;
	bn_t d;

	fp9_new(a);
	fp9_new(b);
	fp9_new(c);
	bn_new(d);

	BENCH_RUN("fp9_add") {
		fp9_rand(a);
		fp9_rand(b);
		BENCH_ADD(fp9_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp9_sub") {
		fp9_rand(a);
		fp9_rand(b);
		BENCH_ADD(fp9_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp9_dbl") {
		fp9_rand(a);
		BENCH_ADD(fp9_dbl(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp9_mul") {
		fp9_rand(a);
		fp9_rand(b);
		BENCH_ADD(fp9_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp9_mul_basic") {
		fp9_rand(a);
		fp9_rand(b);
		BENCH_ADD(fp9_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp9_mul_lazyr") {
		fp9_rand(a);
		fp9_rand(b);
		BENCH_ADD(fp9_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp9_mul_art") {
		fp9_rand(a);
		BENCH_ADD(fp9_mul_art(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp9_sqr") {
		fp9_rand(a);
		BENCH_ADD(fp9_sqr(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp9_sqr_basic") {
		fp9_rand(a);
		BENCH_ADD(fp9_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp9_sqr_lazyr") {
		fp9_rand(a);
		BENCH_ADD(fp9_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp9_inv") {
		fp9_rand(a);
		BENCH_ADD(fp9_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp9_exp") {
		fp9_rand(a);
		d->used = RLC_FP_DIGS;
		dv_copy(d->dp, fp_prime_get(), RLC_FP_DIGS);
		BENCH_ADD(fp9_exp(c, a, d));
	}
	BENCH_END;

	BENCH_RUN("fp9_frb") {
		fp9_rand(a);
		BENCH_ADD(fp9_frb(c, a, 1));
	}
	BENCH_END;

	fp9_free(a);
	fp9_free(b);
	fp9_free(c);
}

static void memory12(void) {
	fp12_t a[BENCH];

	BENCH_FEW("fp12_null", fp12_null(a[i]), 1);

	BENCH_FEW("fp12_new", fp12_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp12_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp12_new(a[i]);
	}
	BENCH_FEW("fp12_free", fp12_free(a[i]), 1);

	(void)a;
}

static void util12(void) {
	fp12_t a, b;
	uint8_t bin[12 * RLC_FP_BYTES];

	fp12_null(a);
	fp12_null(b);

	fp12_new(a);
	fp12_new(b);

	BENCH_RUN("fp12_copy") {
		fp12_rand(a);
		BENCH_ADD(fp12_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_neg") {
		fp12_rand(a);
		BENCH_ADD(fp12_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_zero") {
		fp12_rand(a);
		BENCH_ADD(fp12_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp12_is_zero") {
		fp12_rand(a);
		BENCH_ADD((void)fp12_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp12_set_dig (1)") {
		fp12_rand(a);
		BENCH_ADD(fp12_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp12_set_dig") {
		fp12_rand(a);
		BENCH_ADD(fp12_set_dig(a, a[0][0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp12_rand") {
		BENCH_ADD(fp12_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp12_size_bin (0)") {
		fp12_rand(a);
		BENCH_ADD(fp12_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp12_size_bin (1)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp12_write_bin (0)") {
		fp12_rand(a);
		BENCH_ADD(fp12_write_bin(bin, sizeof(bin), a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp12_write_bin (1)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_write_bin(bin, 8 * RLC_FP_BYTES, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp12_read_bin (0)") {
		fp12_rand(a);
		fp12_write_bin(bin, sizeof(bin), a, 0);
		BENCH_ADD(fp12_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp12_read_bin (1)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		fp12_write_bin(bin, fp12_size_bin(a, 1), a, 1);
		BENCH_ADD(fp12_read_bin(a, bin, 8 * RLC_FP_BYTES));
	}
	BENCH_END;

	BENCH_RUN("fp12_cmp") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_cmp_dig") {
		fp12_rand(a);
		BENCH_ADD(fp12_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp12_free(a);
	fp12_free(b);
}

static void arith12(void) {
	fp12_t a, b, c, d[2];
	bn_t e;

	fp12_new(a);
	fp12_new(b);
	fp12_new(c);
	fp12_new(d[0]);
	fp12_new(d[1]);
	bn_new(e);

	BENCH_RUN("fp12_add") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp12_sub") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp12_mul") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp12_mul_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp12_mul_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp12_mul_dxs") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_dxs(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp12_mul_dxs_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_dxs_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp12_mul_dxs_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_dxs_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp12_sqr") {
		fp12_rand(a);
		BENCH_ADD(fp12_sqr(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_sqr_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_sqr_cyc(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp12_sqr_cyc_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_cyc_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp12_sqr_cyc_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_cyc_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp12_sqr_pck") {
		fp12_rand(a);
		BENCH_ADD(fp12_sqr_pck(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp12_sqr_pck_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_pck_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp12_sqr_pck_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_pck_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp12_test_cyc") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_test_cyc(a));
	}
	BENCH_END;

	BENCH_RUN("fp12_conv_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_back_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_back_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_back_cyc (2)") {
		fp12_rand(d[0]);
		fp12_rand(d[1]);
		BENCH_ADD(fp12_back_cyc_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp12_conv_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_inv") {
		fp12_rand(a);
		BENCH_ADD(fp12_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_inv_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_inv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_exp") {
		fp12_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp12_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp12_exp (cyc)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp12_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp12_exp_cyc (param or sparse)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		bn_zero(e);
		fp_prime_get_par(e);
		if (bn_is_zero(e)) {
			bn_set_2b(e, RLC_FP_BITS - 1);
			bn_set_bit(e, RLC_FP_BITS / 2, 1);
			bn_set_bit(e, 0, 1);
		}
		BENCH_ADD(fp12_exp_cyc(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp12_exp_cyc_sps (param)") {
		const int *k;
		int l;
		k = fp_prime_get_par_sps(&l);
		fp12_rand(a);
		BENCH_ADD(fp12_exp_cyc_sps(c, a, k, l, RLC_POS));
	}
	BENCH_END;

	BENCH_RUN("fp12_exp_dig") {
		fp12_rand(a);
		bn_rand(e, RLC_POS, RLC_DIG);
		BENCH_ADD(fp12_exp_dig(c, a, e->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("fp12_frb") {
		fp12_rand(a);
		BENCH_ADD(fp12_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp12_pck") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_pck(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_upk") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		fp12_pck(a, a);
		BENCH_ADD(fp12_upk(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_pck_max") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_pck_max(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp12_upk_max") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		fp12_pck_max(a, a);
		BENCH_ADD(fp12_upk_max(c, a));
	}
	BENCH_END;

	fp12_free(a);
	fp12_free(b);
	fp12_free(c);
	fp12_free(d[0]);
	fp12_free(d[1]);
	bn_free(e);
}

static void memory18(void) {
	fp18_t a[BENCH];

	BENCH_FEW("fp18_null", fp18_null(a[i]), 1);

	BENCH_FEW("fp18_new", fp18_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp18_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp18_new(a[i]);
	}
	BENCH_FEW("fp18_free", fp18_free(a[i]), 1);

	(void)a;
}

static void util18(void) {
	fp18_t a, b;

	fp18_null(a);
	fp18_null(b);

	fp18_new(a);
	fp18_new(b);

	BENCH_RUN("fp18_copy") {
		fp18_rand(a);
		BENCH_ADD(fp18_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp18_neg") {
		fp18_rand(a);
		BENCH_ADD(fp18_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp18_zero") {
		fp18_rand(a);
		BENCH_ADD(fp18_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp18_is_zero") {
		fp18_rand(a);
		BENCH_ADD((void)fp18_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp18_set_dig (1)") {
		fp18_rand(a);
		BENCH_ADD(fp18_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp18_set_dig") {
		fp18_rand(a);
		BENCH_ADD(fp18_set_dig(a, a[0][0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp18_rand") {
		BENCH_ADD(fp18_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp18_cmp") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp18_cmp_dig") {
		fp18_rand(a);
		BENCH_ADD(fp18_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp18_free(a);
	fp18_free(b);
}

static void arith18(void) {
	fp18_t a, b, c;
	bn_t e;

	fp18_new(a);
	fp18_new(b);
	fp18_new(c);
	bn_new(e);

	BENCH_RUN("fp18_add") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp18_sub") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp18_mul") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp18_mul_basic") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp18_mul_lazyr") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp18_sqr") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp18_sqr_basic") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp18_sqr_lazyr") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp18_inv") {
		fp18_rand(a);
		BENCH_ADD(fp18_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp18_exp") {
		fp18_rand(a);
		e->used = RLC_FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), RLC_FP_DIGS);
		BENCH_ADD(fp18_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp18_frb") {
		fp18_rand(a);
		BENCH_ADD(fp18_frb(c, a, 1));
	}
	BENCH_END;

	fp18_free(a);
	fp18_free(b);
	fp18_free(c);
	bn_free(e);
}

static void memory24(void) {
	fp24_t a[BENCH];

	BENCH_FEW("fp24_null", fp24_null(a[i]), 1);

	BENCH_FEW("fp24_new", fp24_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp24_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp24_new(a[i]);
	}
	BENCH_FEW("fp24_free", fp24_free(a[i]), 1);

	(void)a;
}

static void util24(void) {
	fp24_t a, b;
	uint8_t bin[24 * RLC_FP_BYTES];

	fp24_null(a);
	fp24_null(b);

	fp24_new(a);
	fp24_new(b);

	BENCH_RUN("fp24_copy") {
		fp24_rand(a);
		BENCH_ADD(fp24_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_neg") {
		fp24_rand(a);
		BENCH_ADD(fp24_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_zero") {
		fp24_rand(a);
		BENCH_ADD(fp24_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp24_is_zero") {
		fp24_rand(a);
		BENCH_ADD((void)fp24_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp24_set_dig (1)") {
		fp24_rand(a);
		BENCH_ADD(fp24_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp24_set_dig") {
		fp24_rand(a);
		BENCH_ADD(fp24_set_dig(a, a[0][0][0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp24_rand") {
		BENCH_ADD(fp24_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp24_size_bin (0)") {
		fp24_rand(a);
		BENCH_ADD(fp24_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp24_size_bin (1)") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		BENCH_ADD(fp24_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp24_write_bin (0)") {
		fp24_rand(a);
		BENCH_ADD(fp24_write_bin(bin, sizeof(bin), a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp24_write_bin (1)") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		BENCH_ADD(fp24_write_bin(bin, 32 * RLC_FP_BYTES, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp24_read_bin (0)") {
		fp24_rand(a);
		fp24_write_bin(bin, sizeof(bin), a, 0);
		BENCH_ADD(fp24_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp24_read_bin (1)") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		fp24_write_bin(bin, fp24_size_bin(a, 1), a, 1);
		BENCH_ADD(fp24_read_bin(a, bin, 32 * RLC_FP_BYTES));
	}
	BENCH_END;

	BENCH_RUN("fp24_cmp") {
		fp24_rand(a);
		fp24_rand(b);
		BENCH_ADD(fp24_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_cmp_dig") {
		fp24_rand(a);
		BENCH_ADD(fp24_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp24_free(a);
	fp24_free(b);
}

static void arith24(void) {
	fp24_t a, b, c, d[2];
	bn_t e;

	fp24_new(a);
	fp24_new(b);
	fp24_new(c);
	fp24_new(d[0]);
	fp24_new(d[1]);
	bn_new(e);

	BENCH_RUN("fp24_add") {
		fp24_rand(a);
		fp24_rand(b);
		BENCH_ADD(fp24_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp24_sub") {
		fp24_rand(a);
		fp24_rand(b);
		BENCH_ADD(fp24_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp24_mul") {
		fp24_rand(a);
		fp24_rand(b);
		BENCH_ADD(fp24_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp24_mul_basic") {
		fp24_rand(a);
		fp24_rand(b);
		BENCH_ADD(fp24_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp24_mul_lazyr") {
		fp24_rand(a);
		fp24_rand(b);
		BENCH_ADD(fp24_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp24_sqr") {
		fp24_rand(a);
		BENCH_ADD(fp24_sqr(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp24_sqr_basic") {
		fp24_rand(a);
		BENCH_ADD(fp24_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp24_sqr_lazyr") {
		fp24_rand(a);
		BENCH_ADD(fp24_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp24_test_cyc") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		BENCH_ADD(fp24_test_cyc(a));
	}
	BENCH_END;

	BENCH_RUN("fp24_conv_cyc") {
		fp24_rand(a);
		BENCH_ADD(fp24_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_back_cyc") {
		fp24_rand(a);
		BENCH_ADD(fp24_back_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_back_cyc (2)") {
		fp24_rand(d[0]);
		fp24_rand(d[1]);
		BENCH_ADD(fp24_back_cyc_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp24_conv_cyc") {
		fp24_rand(a);
		BENCH_ADD(fp24_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_inv") {
		fp24_rand(a);
		BENCH_ADD(fp24_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_exp") {
		fp24_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp24_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp24_exp (cyc)") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp24_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp24_exp_cyc (param or sparse)") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		bn_zero(e);
		fp_prime_get_par(e);
		if (bn_is_zero(e)) {
			bn_set_2b(e, RLC_FP_BITS - 1);
			bn_set_bit(e, RLC_FP_BITS / 2, 1);
			bn_set_bit(e, 0, 1);
		}
		BENCH_ADD(fp24_exp_cyc(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp24_exp_cyc_sps (param)") {
		const int *k;
		int l;
		k = fp_prime_get_par_sps(&l);
		fp24_rand(a);
		BENCH_ADD(fp24_exp_cyc_sps(c, a, k, l, RLC_POS));
	}
	BENCH_END;

	BENCH_RUN("fp24_exp_dig") {
		fp24_rand(a);
		bn_rand(e, RLC_POS, RLC_DIG);
		BENCH_ADD(fp24_exp_dig(c, a, e->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("fp24_frb") {
		fp24_rand(a);
		BENCH_ADD(fp24_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp24_pck") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		BENCH_ADD(fp24_pck(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp24_upk") {
		fp24_rand(a);
		fp24_conv_cyc(a, a);
		fp24_pck(a, a);
		BENCH_ADD(fp24_upk(c, a));
	}
	BENCH_END;

	fp24_free(a);
	fp24_free(b);
	fp24_free(c);
	fp24_free(d[0]);
	fp24_free(d[1]);
	bn_free(e);
}

static void memory48(void) {
	fp48_t a[BENCH];

	BENCH_FEW("fp48_null", fp48_null(a[i]), 1);

	BENCH_FEW("fp48_new", fp48_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp48_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp48_new(a[i]);
	}
	BENCH_FEW("fp48_free", fp48_free(a[i]), 1);

	(void)a;
}

static void util48(void) {
	fp48_t a, b;
	uint8_t bin[48 * RLC_FP_BYTES];

	fp48_null(a);
	fp48_null(b);

	fp48_new(a);
	fp48_new(b);

	BENCH_RUN("fp48_copy") {
		fp48_rand(a);
		BENCH_ADD(fp48_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_neg") {
		fp48_rand(a);
		BENCH_ADD(fp48_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_zero") {
		fp48_rand(a);
		BENCH_ADD(fp48_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp48_is_zero") {
		fp48_rand(a);
		BENCH_ADD((void)fp48_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp48_set_dig (1)") {
		fp48_rand(a);
		BENCH_ADD(fp48_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp48_set_dig") {
		fp48_rand(a);
		BENCH_ADD(fp48_set_dig(a, a[0][0][0][0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp48_rand") {
		BENCH_ADD(fp48_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp48_size_bin (0)") {
		fp48_rand(a);
		BENCH_ADD(fp48_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp48_size_bin (1)") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		BENCH_ADD(fp48_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp48_write_bin (0)") {
		fp48_rand(a);
		BENCH_ADD(fp48_write_bin(bin, sizeof(bin), a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp48_write_bin (1)") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		BENCH_ADD(fp48_write_bin(bin, 32 * RLC_FP_BYTES, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp48_read_bin (0)") {
		fp48_rand(a);
		fp48_write_bin(bin, sizeof(bin), a, 0);
		BENCH_ADD(fp48_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp48_read_bin (1)") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		fp48_write_bin(bin, fp48_size_bin(a, 1), a, 1);
		BENCH_ADD(fp48_read_bin(a, bin, 32 * RLC_FP_BYTES));
	}
	BENCH_END;

	BENCH_RUN("fp48_cmp") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_cmp_dig") {
		fp48_rand(a);
		BENCH_ADD(fp48_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp48_free(a);
	fp48_free(b);
}

static void arith48(void) {
	fp48_t a, b, c, d[2];
	bn_t e;

	fp48_new(a);
	fp48_new(b);
	fp48_new(c);
	fp48_new(d[0]);
	fp48_new(d[1]);
	bn_new(e);

	BENCH_RUN("fp48_add") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp48_sub") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp48_mul") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp48_mul_basic") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp48_mul_lazyr") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp48_mul_dxs") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_mul_dxs(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp48_sqr") {
		fp48_rand(a);
		BENCH_ADD(fp48_sqr(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_sqr_cyc") {
		fp48_rand(a);
		BENCH_ADD(fp48_sqr_cyc(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp48_sqr_cyc_basic") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_sqr_cyc_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp48_sqr_cyc_lazyr") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_sqr_cyc_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp48_sqr_pck") {
		fp48_rand(a);
		BENCH_ADD(fp48_sqr_pck(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp48_sqr_pck_basic") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_sqr_pck_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp48_sqr_pck_lazyr") {
		fp48_rand(a);
		fp48_rand(b);
		BENCH_ADD(fp48_sqr_pck_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp48_test_cyc") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		BENCH_ADD(fp48_test_cyc(a));
	}
	BENCH_END;

	BENCH_RUN("fp48_conv_cyc") {
		fp48_rand(a);
		BENCH_ADD(fp48_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_back_cyc") {
		fp48_rand(a);
		BENCH_ADD(fp48_back_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_back_cyc (2)") {
		fp48_rand(d[0]);
		fp48_rand(d[1]);
		BENCH_ADD(fp48_back_cyc_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp48_conv_cyc") {
		fp48_rand(a);
		BENCH_ADD(fp48_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_inv") {
		fp48_rand(a);
		BENCH_ADD(fp48_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_inv_cyc") {
		fp48_rand(a);
		BENCH_ADD(fp48_inv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_exp") {
		fp48_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp48_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp48_exp (cyc)") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp48_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp48_exp_cyc (param or sparse)") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		bn_zero(e);
		fp_prime_get_par(e);
		if (bn_is_zero(e)) {
			bn_set_2b(e, RLC_FP_BITS - 1);
			bn_set_bit(e, RLC_FP_BITS / 2, 1);
			bn_set_bit(e, 0, 1);
		}
		BENCH_ADD(fp48_exp_cyc(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp48_exp_cyc_sps (param)") {
		const int *k;
		int l;
		k = fp_prime_get_par_sps(&l);
		fp48_rand(a);
		BENCH_ADD(fp48_exp_cyc_sps(c, a, k, l, RLC_POS));
	}
	BENCH_END;

	BENCH_RUN("fp48_exp_dig") {
		fp48_rand(a);
		bn_rand(e, RLC_POS, RLC_DIG);
		BENCH_ADD(fp48_exp_dig(c, a, e->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("fp48_frb") {
		fp48_rand(a);
		BENCH_ADD(fp48_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp48_pck") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		BENCH_ADD(fp48_pck(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp48_upk") {
		fp48_rand(a);
		fp48_conv_cyc(a, a);
		fp48_pck(a, a);
		BENCH_ADD(fp48_upk(c, a));
	}
	BENCH_END;

	fp48_free(a);
	fp48_free(b);
	fp48_free(c);
	fp48_free(d[0]);
	fp48_free(d[1]);
	bn_free(e);
}

static void memory54(void) {
	fp54_t a[BENCH];

	BENCH_FEW("fp54_null", fp54_null(a[i]), 1);

	BENCH_FEW("fp54_new", fp54_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp54_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp54_new(a[i]);
	}
	BENCH_FEW("fp54_free", fp54_free(a[i]), 1);

	(void)a;
}

static void util54(void) {
	fp54_t a, b;
	uint8_t bin[54 * RLC_FP_BYTES];

	fp54_null(a);
	fp54_null(b);

	fp54_new(a);
	fp54_new(b);

	BENCH_RUN("fp54_copy") {
		fp54_rand(a);
		BENCH_ADD(fp54_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_neg") {
		fp54_rand(a);
		BENCH_ADD(fp54_neg(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_zero") {
		fp54_rand(a);
		BENCH_ADD(fp54_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp54_is_zero") {
		fp54_rand(a);
		BENCH_ADD((void)fp54_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp54_set_dig (1)") {
		fp54_rand(a);
		BENCH_ADD(fp54_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp54_set_dig") {
		fp54_rand(a);
		BENCH_ADD(fp54_set_dig(a, a[0][0][0][0][0]));
	}
	BENCH_END;

	BENCH_RUN("fp54_rand") {
		BENCH_ADD(fp54_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp54_size_bin (0)") {
		fp54_rand(a);
		BENCH_ADD(fp54_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp54_size_bin (1)") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		BENCH_ADD(fp54_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp54_write_bin (0)") {
		fp54_rand(a);
		BENCH_ADD(fp54_write_bin(bin, sizeof(bin), a, 0));
	}
	BENCH_END;

	BENCH_RUN("fp54_write_bin (1)") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		BENCH_ADD(fp54_write_bin(bin, 32 * RLC_FP_BYTES, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp54_read_bin (0)") {
		fp54_rand(a);
		fp54_write_bin(bin, sizeof(bin), a, 0);
		BENCH_ADD(fp54_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp54_read_bin (1)") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		fp54_write_bin(bin, fp54_size_bin(a, 1), a, 1);
		BENCH_ADD(fp54_read_bin(a, bin, 32 * RLC_FP_BYTES));
	}
	BENCH_END;

	BENCH_RUN("fp54_cmp") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_cmp_dig") {
		fp54_rand(a);
		BENCH_ADD(fp54_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp54_free(a);
	fp54_free(b);
}

static void arith54(void) {
	fp54_t a, b, c, d[2];
	bn_t e;

	fp54_new(a);
	fp54_new(b);
	fp54_new(c);
	fp54_new(d[0]);
	fp54_new(d[1]);
	bn_new(e);

	BENCH_RUN("fp54_add") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp54_sub") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_sub(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp54_mul") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_mul(c, a, b));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp54_mul_basic") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp54_mul_lazyr") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp54_mul_dxs") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_mul_dxs(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fp54_sqr") {
		fp54_rand(a);
		BENCH_ADD(fp54_sqr(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_sqr_cyc") {
		fp54_rand(a);
		BENCH_ADD(fp54_sqr_cyc(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp54_sqr_cyc_basic") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_sqr_cyc_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp54_sqr_cyc_lazyr") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_sqr_cyc_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp54_sqr_pck") {
		fp54_rand(a);
		BENCH_ADD(fp54_sqr_pck(c, a));
	}
	BENCH_END;

#if FPX_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp54_sqr_pck_basic") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_sqr_pck_basic(c, a));
	}
	BENCH_END;
#endif

#if FPX_RDC == LAZYR || !defined(STRIP)
	BENCH_RUN("fp54_sqr_pck_lazyr") {
		fp54_rand(a);
		fp54_rand(b);
		BENCH_ADD(fp54_sqr_pck_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp54_test_cyc") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		BENCH_ADD(fp54_test_cyc(a));
	}
	BENCH_END;

	BENCH_RUN("fp54_conv_cyc") {
		fp54_rand(a);
		BENCH_ADD(fp54_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_back_cyc") {
		fp54_rand(a);
		BENCH_ADD(fp54_back_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_back_cyc (2)") {
		fp54_rand(d[0]);
		fp54_rand(d[1]);
		BENCH_ADD(fp54_back_cyc_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_RUN("fp54_conv_cyc") {
		fp54_rand(a);
		BENCH_ADD(fp54_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_inv") {
		fp54_rand(a);
		BENCH_ADD(fp54_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_inv_cyc") {
		fp54_rand(a);
		BENCH_ADD(fp54_inv_cyc(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_exp") {
		fp54_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp54_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp54_exp (cyc)") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp54_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp54_exp_cyc (param or sparse)") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		bn_zero(e);
		fp_prime_get_par(e);
		if (bn_is_zero(e)) {
			bn_set_2b(e, RLC_FP_BITS - 1);
			bn_set_bit(e, RLC_FP_BITS / 2, 1);
			bn_set_bit(e, 0, 1);
		}
		BENCH_ADD(fp54_exp_cyc(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("fp54_exp_cyc_sps (param)") {
		const int *k;
		int l;
		k = fp_prime_get_par_sps(&l);
		fp54_rand(a);
		BENCH_ADD(fp54_exp_cyc_sps(c, a, k, l, RLC_POS));
	}
	BENCH_END;

	BENCH_RUN("fp54_exp_dig") {
		fp54_rand(a);
		bn_rand(e, RLC_POS, RLC_DIG);
		BENCH_ADD(fp54_exp_dig(c, a, e->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("fp54_frb") {
		fp54_rand(a);
		BENCH_ADD(fp54_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp54_pck") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		BENCH_ADD(fp54_pck(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp54_upk") {
		fp54_rand(a);
		fp54_conv_cyc(a, a);
		fp54_pck(a, a);
		BENCH_ADD(fp54_upk(c, a));
	}
	BENCH_END;

	fp54_free(a);
	fp54_free(b);
	fp54_free(c);
	fp54_free(d[0]);
	fp54_free(d[1]);
	bn_free(e);
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	util_banner("Benchmarks for the FPX module:", 0);

	/* Try using a pairing-friendly curve for faster exponentiation method. */
	if (pc_param_set_any() != RLC_OK) {
		/* If it does not work, try a tower-friendly field. */
		if (fp_param_set_any_tower() == RLC_ERR) {
			RLC_THROW(ERR_NO_FIELD);
			core_clean();
			return 0;
		}
	}

	fp_param_print();

	if (fp_prime_get_qnr()) {
		util_banner("Quadratic extension:", 0);
		util_banner("Utilities:", 1);
		memory2();
		util2();

		util_banner("Arithmetic:", 1);
		arith2();
	}

	if (fp_prime_get_cnr()) {
		util_banner("Cubic extension:", 0);
		util_banner("Utilities:", 1);
		memory3();
		util3();
		util_banner("Arithmetic:", 1);
		arith3();
	}

	if (fp_prime_get_qnr()) {
		util_banner("Quartic extension:", 0);
		util_banner("Utilities:", 1);
		memory4();
		util4();
		util_banner("Arithmetic:", 1);
		arith4();

		util_banner("Sextic extension:", 0);
		util_banner("Utilities:", 1);
		memory6();
		util6();
		util_banner("Arithmetic:", 1);
		arith6();

		util_banner("Octic extension:", 0);
		util_banner("Utilities:", 1);
		memory8();
		util8();
		util_banner("Arithmetic:", 1);
		arith8();
	}

	if (fp_prime_get_cnr()) {
		util_banner("Nonic extension:", 0);
		util_banner("Utilities:", 1);
		memory9();
		util9();
		util_banner("Arithmetic:", 1);
		arith9();
	}

	if (fp_prime_get_qnr()) {
		util_banner("Dodecic extension:", 0);
		util_banner("Utilities:", 1);
		memory12();
		util12();
		util_banner("Arithmetic:", 1);
		arith12();
	}

	if (fp_prime_get_cnr()) {
		util_banner("Octodecic extension:", 0);
		util_banner("Utilities:", 1);
		memory18();
		util18();

		util_banner("Arithmetic:", 1);
		arith18();
	}

	if (fp_prime_get_qnr()) {
		util_banner("Extension of degree 24:", 0);
		util_banner("Utilities:", 1);
		memory24();
		util24();
		util_banner("Arithmetic:", 1);
		arith24();

		util_banner("Extension of degree 48:", 0);
		util_banner("Utilities:", 1);
		memory48();
		util48();
		util_banner("Arithmetic:", 1);
		arith48();
	}

	if (fp_prime_get_cnr()) {
		util_banner("Extension of degree 54:", 0);
		util_banner("Utilities:", 1);
		memory54();
		util54();
		util_banner("Arithmetic:", 1);
		arith54();
	}

	core_clean();
	return 0;
}
