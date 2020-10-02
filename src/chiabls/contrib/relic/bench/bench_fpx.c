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
 * Benchmarks for extensions of prime fields
 *
 * @ingroup bench
 */

#include "relic.h"
#include "relic_bench.h"

static void memory2(void) {
	fp2_t a[BENCH];

	BENCH_SMALL("fp2_null", fp2_null(a[i]));

	BENCH_SMALL("fp2_new", fp2_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		fp2_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp2_new(a[i]);
	}
	BENCH_SMALL("fp2_free", fp2_free(a[i]));

	(void)a;
}

static void util2(void) {
	uint8_t bin[2 * FP_BYTES];
	fp2_t a, b;
	int l;

	fp2_null(a);
	fp2_null(b);

	fp2_new(a);
	fp2_new(b);

	BENCH_BEGIN("fp2_copy") {
		fp2_rand(a);
		BENCH_ADD(fp2_copy(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_neg") {
		fp2_rand(a);
		BENCH_ADD(fp2_neg(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_zero") {
		fp2_rand(a);
		BENCH_ADD(fp2_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_is_zero") {
		fp2_rand(a);
		BENCH_ADD((void)fp2_is_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_set_dig (1)") {
		fp2_rand(a);
		BENCH_ADD(fp2_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_set_dig") {
		fp2_rand(a);
		BENCH_ADD(fp2_set_dig(a, a[0][0]));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_rand") {
		BENCH_ADD(fp2_rand(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_size_bin (0)") {
		fp2_rand(a);
		BENCH_ADD(fp2_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_size_bin (1)") {
		fp2_rand(a);
		fp2_conv_uni(a, a);
		BENCH_ADD(fp2_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_write_bin (0)") {
		fp2_rand(a);
		l = fp2_size_bin(a, 1);
		BENCH_ADD(fp2_write_bin(bin, l, a, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_write_bin (1)") {
		fp2_rand(a);
		fp2_conv_uni(a, a);
		l = fp2_size_bin(a, 1);
		BENCH_ADD(fp2_write_bin(bin, l, a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_read_bin (0)") {
		fp2_rand(a);
		l = fp2_size_bin(a, 0);
		fp2_write_bin(bin, l, a, 0);
		BENCH_ADD(fp2_read_bin(a, bin, l));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_read_bin") {
		fp2_rand(a);
		fp2_conv_uni(a, a);
		l = fp2_size_bin(a, 1);
		fp2_write_bin(bin, l, a, 1);
		BENCH_ADD(fp2_read_bin(a, bin, l));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_cmp") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_cmp(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_cmp_dig") {
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

	BENCH_BEGIN("fp2_add") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_add(c, a, b));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp2_add_basic") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_add_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp2_add_integ") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_add_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp2_sub") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_sub(c, a, b));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp2_sub_basic") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_sub_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp2_sub_integ") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_sub_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp2_dbl") {
		fp2_rand(a);
		BENCH_ADD(fp2_dbl(c, a));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp2_dbl_basic") {
		fp2_rand(a);
		BENCH_ADD(fp2_dbl_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp2_dbl_integ") {
		fp2_rand(a);
		BENCH_ADD(fp2_dbl_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp2_mul") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_mul(c, a, b));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp2_mul_basic") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp2_mul_integ") {
		fp2_rand(a);
		fp2_rand(b);
		BENCH_ADD(fp2_mul_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp2_mul_art") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_art(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_mul_nor") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_nor(c, a));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp2_mul_nor_basic") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_nor_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp2_mul_nor_integ") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_nor_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp2_sqr") {
		fp2_rand(a);
		BENCH_ADD(fp2_sqr(c, a));
	}
	BENCH_END;

#if PP_QDR == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp2_sqr_basic") {
		fp2_rand(a);
		BENCH_ADD(fp2_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_QDR == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp2_sqr_integ") {
		fp2_rand(a);
		BENCH_ADD(fp2_sqr_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp2_test_uni") {
		fp2_rand(a);
		fp2_conv_uni(a, a);
		BENCH_ADD(fp2_test_uni(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_conv_uni") {
		fp2_rand(a);
		BENCH_ADD(fp2_conv_uni(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_inv") {
		fp2_rand(a);
		BENCH_ADD(fp2_inv(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_inv_uni") {
		fp2_rand(a);
		BENCH_ADD(fp2_inv_uni(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_inv_sim (2)") {
		fp2_rand(d[0]);
		fp2_rand(d[1]);
		BENCH_ADD(fp2_inv_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_exp") {
		fp2_rand(a);
		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		BENCH_ADD(fp2_exp(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_exp_uni") {
		fp2_rand(a);
		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		BENCH_ADD(fp2_exp_uni(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_frb (1)") {
		fp2_rand(a);
		BENCH_ADD(fp2_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_frb (2)") {
		fp2_rand(a);
		BENCH_ADD(fp2_frb(c, a, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_mul_frb (1)") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_frb(c, a, 1, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_mul_frb (2)") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_frb(c, a, 2, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_mul_frb (3)") {
		fp2_rand(a);
		BENCH_ADD(fp2_mul_frb(c, a, 3, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_pck") {
		fp2_rand(a);
		fp2_conv_uni(a, a);
		BENCH_ADD(fp2_pck(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp2_upk") {
		fp2_rand(a);
		fp2_conv_uni(a, a);
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

	BENCH_SMALL("fp3_null", fp3_null(a[i]));

	BENCH_SMALL("fp3_new", fp3_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		fp3_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp3_new(a[i]);
	}
	BENCH_SMALL("fp3_free", fp3_free(a[i]));

	(void)a;
}

static void util3(void) {
	uint8_t bin[3 * FP_BYTES];
	fp3_t a, b;

	fp3_null(a);
	fp3_null(b);

	fp3_new(a);
	fp3_new(b);

	BENCH_BEGIN("fp3_copy") {
		fp3_rand(a);
		BENCH_ADD(fp3_copy(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_neg") {
		fp3_rand(a);
		BENCH_ADD(fp3_neg(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_zero") {
		fp3_rand(a);
		BENCH_ADD(fp3_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_is_zero") {
		fp3_rand(a);
		BENCH_ADD((void)fp3_is_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_set_dig (1)") {
		fp3_rand(a);
		BENCH_ADD(fp3_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_set_dig") {
		fp3_rand(a);
		BENCH_ADD(fp3_set_dig(a, a[0][0]));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_rand") {
		BENCH_ADD(fp3_rand(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_size_bin") {
		fp3_rand(a);
		BENCH_ADD(fp3_size_bin(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_write_bin") {
		fp3_rand(a);
		BENCH_ADD(fp3_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_read_bin") {
		fp3_rand(a);
		fp3_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp3_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_cmp") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_cmp(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_cmp_dig") {
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

	BENCH_BEGIN("fp3_add") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_add(c, a, b));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp3_add_basic") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_add_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp3_add_integ") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_add_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp3_sub") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_sub(c, a, b));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp3_sub_basic") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_sub_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp3_sub_integ") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_sub_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp3_dbl") {
		fp3_rand(a);
		BENCH_ADD(fp3_dbl(c, a));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp3_dbl_basic") {
		fp3_rand(a);
		BENCH_ADD(fp3_dbl_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp3_dbl_integ") {
		fp3_rand(a);
		BENCH_ADD(fp3_dbl_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp3_mul") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_mul(c, a, b));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp3_mul_basic") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp3_mul_integ") {
		fp3_rand(a);
		fp3_rand(b);
		BENCH_ADD(fp3_mul_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp3_mul_art") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_art(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_mul_nor") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_nor(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_sqr") {
		fp3_rand(a);
		BENCH_ADD(fp3_sqr(c, a));
	}
	BENCH_END;

#if PP_CBC == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp3_sqr_basic") {
		fp3_rand(a);
		BENCH_ADD(fp3_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_CBC == INTEG || !defined(STRIP)
	BENCH_BEGIN("fp3_sqr_integ") {
		fp3_rand(a);
		BENCH_ADD(fp3_sqr_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp3_inv") {
		fp3_rand(a);
		BENCH_ADD(fp3_inv(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_inv_sim (2)") {
		fp3_rand(d[0]);
		fp3_rand(d[1]);
		BENCH_ADD(fp3_inv_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_exp") {
		fp3_rand(a);
		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		BENCH_ADD(fp3_exp(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_frb (1)") {
		fp3_rand(a);
		BENCH_ADD(fp3_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_frb (2)") {
		fp3_rand(a);
		BENCH_ADD(fp3_frb(c, a, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_frb (3)") {
		fp3_rand(a);
		BENCH_ADD(fp3_frb(c, a, 3));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_frb (4)") {
		fp3_rand(a);
		BENCH_ADD(fp3_frb(c, a, 4));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_frb (5)") {
		fp3_rand(a);
		BENCH_ADD(fp3_frb(c, a, 5));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_mul_frb (0,1)") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_frb(c, a, 0, 1, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_mul_frb (0,2)") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_frb(c, a, 0, 2, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_mul_frb (1,1)") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_frb(c, a, 1, 1, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_mul_frb (1,2)") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_frb(c, a, 1, 2, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp3_mul_frb (1,3)") {
		fp3_rand(a);
		BENCH_ADD(fp3_mul_frb(c, a, 1, 3, 0));
	}
	BENCH_END;

	fp3_free(a);
	fp3_free(b);
	fp3_free(c);
	fp3_free(d[0]);
	fp3_free(d[1]);
	bn_free(e);
}

static void memory6(void) {
	fp6_t a[BENCH];

	BENCH_SMALL("fp6_null", fp6_null(a[i]));

	BENCH_SMALL("fp6_new", fp6_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		fp6_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp6_new(a[i]);
	}
	BENCH_SMALL("fp6_free", fp6_free(a[i]));

	(void)a;
}

static void util6(void) {
	uint8_t bin[6 * FP_BYTES];
	fp6_t a, b;

	fp6_null(a);
	fp6_null(b);

	fp6_new(a);
	fp6_new(b);

	BENCH_BEGIN("fp6_copy") {
		fp6_rand(a);
		BENCH_ADD(fp6_copy(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_neg") {
		fp6_rand(a);
		BENCH_ADD(fp6_neg(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_zero") {
		fp6_rand(a);
		BENCH_ADD(fp6_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_is_zero") {
		fp6_rand(a);
		BENCH_ADD((void)fp6_is_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_set_dig (1)") {
		fp6_rand(a);
		BENCH_ADD(fp6_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_set_dig") {
		fp6_rand(a);
		BENCH_ADD(fp6_set_dig(a, a[0][0][0]));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_rand") {
		BENCH_ADD(fp6_rand(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_size_bin") {
		fp6_rand(a);
		BENCH_ADD(fp6_size_bin(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_write_bin") {
		fp6_rand(a);
		BENCH_ADD(fp6_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_read_bin") {
		fp6_rand(a);
		fp6_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp6_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_cmp") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_cmp(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_cmp_dig") {
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

	BENCH_BEGIN("fp6_add") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_add(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_sub") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_sub(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_dbl") {
		fp6_rand(a);
		BENCH_ADD(fp6_dbl(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_mul") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_mul(c, a, b));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp6_mul_basic") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp6_mul_lazyr") {
		fp6_rand(a);
		fp6_rand(b);
		BENCH_ADD(fp6_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp6_mul_art") {
		fp6_rand(a);
		BENCH_ADD(fp6_mul_art(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_sqr") {
		fp6_rand(a);
		BENCH_ADD(fp6_sqr(c, a));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp6_sqr_basic") {
		fp6_rand(a);
		BENCH_ADD(fp6_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp6_sqr_lazyr") {
		fp6_rand(a);
		BENCH_ADD(fp6_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp6_inv") {
		fp6_rand(a);
		BENCH_ADD(fp6_inv(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_exp") {
		fp6_rand(a);
		d->used = FP_DIGS;
		dv_copy(d->dp, fp_prime_get(), FP_DIGS);
		BENCH_ADD(fp6_exp(c, a, d));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_frb (1)") {
		fp6_rand(a);
		BENCH_ADD(fp6_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp6_frb (2)") {
		fp6_rand(a);
		BENCH_ADD(fp6_frb(c, a, 2));
	}
	BENCH_END;

	fp6_free(a);
	fp6_free(b);
	fp6_free(c);
}

static void memory12(void) {
	fp12_t a[BENCH];

	BENCH_SMALL("fp12_null", fp12_null(a[i]));

	BENCH_SMALL("fp12_new", fp12_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		fp12_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp12_new(a[i]);
	}
	BENCH_SMALL("fp12_free", fp12_free(a[i]));

	(void)a;
}

static void util12(void) {
	fp12_t a, b;
	uint8_t bin[12 * FP_BYTES];

	fp12_null(a);
	fp12_null(b);

	fp12_new(a);
	fp12_new(b);

	BENCH_BEGIN("fp12_copy") {
		fp12_rand(a);
		BENCH_ADD(fp12_copy(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_neg") {
		fp12_rand(a);
		BENCH_ADD(fp12_neg(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_zero") {
		fp12_rand(a);
		BENCH_ADD(fp12_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_is_zero") {
		fp12_rand(a);
		BENCH_ADD((void)fp12_is_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_set_dig (1)") {
		fp12_rand(a);
		BENCH_ADD(fp12_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_set_dig") {
		fp12_rand(a);
		BENCH_ADD(fp12_set_dig(a, a[0][0][0][0]));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_rand") {
		BENCH_ADD(fp12_rand(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_size_bin (0)") {
		fp12_rand(a);
		BENCH_ADD(fp12_size_bin(a, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_size_bin (1)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_size_bin(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_write_bin (0)") {
		fp12_rand(a);
		BENCH_ADD(fp12_write_bin(bin, sizeof(bin), a, 0));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_write_bin (1)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_write_bin(bin, 8 * FP_BYTES, a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_read_bin (0)") {
		fp12_rand(a);
		fp12_write_bin(bin, sizeof(bin), a, 0);
		BENCH_ADD(fp12_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_read_bin (1)") {
		fp12_rand(a);
		fp12_write_bin(bin, 8 * FP_BYTES, a, 1);
		BENCH_ADD(fp12_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_cmp") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_cmp(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_cmp_dig") {
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

	BENCH_BEGIN("fp12_add") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_add(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_sub") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sub(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_mul") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul(c, a, b));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp12_mul_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp12_mul_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp12_mul_dxs") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_dxs(c, a, b));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp12_mul_dxs_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_dxs_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp12_mul_dxs_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_mul_dxs_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp12_sqr") {
		fp12_rand(a);
		BENCH_ADD(fp12_sqr(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_sqr_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_sqr_cyc(c, a));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp12_sqr_cyc_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_cyc_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp12_sqr_cyc_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_cyc_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp12_sqr_pck") {
		fp12_rand(a);
		BENCH_ADD(fp12_sqr_pck(c, a));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp12_sqr_pck_basic") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_pck_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp12_sqr_pck_lazyr") {
		fp12_rand(a);
		fp12_rand(b);
		BENCH_ADD(fp12_sqr_pck_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp12_test_cyc") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_test_cyc(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_conv_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_back_cyc") {
		fp12_rand(a);
		BENCH_ADD(fp12_back_cyc(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_back_cyc (2)") {
		fp12_rand(d[0]);
		fp12_rand(d[1]);
		BENCH_ADD(fp12_back_cyc_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_conv_uni") {
		fp12_rand(a);
		BENCH_ADD(fp12_conv_uni(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_inv") {
		fp12_rand(a);
		BENCH_ADD(fp12_inv(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_inv_uni") {
		fp12_rand(a);
		BENCH_ADD(fp12_inv_uni(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_exp") {
		fp12_rand(a);
		bn_rand(e, BN_POS, FP_BITS);
		BENCH_ADD(fp12_exp(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_exp (cyc)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		bn_rand(e, BN_POS, FP_BITS);
		BENCH_ADD(fp12_exp(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_exp_cyc (param or sparse)") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		bn_zero(e);
		fp_param_get_var(e);
		if (bn_is_zero(e)) {
			bn_set_2b(e, FP_BITS - 1);
			bn_set_bit(e, FP_BITS / 2, 1);
			bn_set_bit(e, 0, 1);
		}
		BENCH_ADD(fp12_exp_cyc(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_exp_cyc_sps (param)") {
		int l = MAX_TERMS + 1, k[MAX_TERMS + 1];
		fp_param_get_sps(k, &l);
		fp12_rand(a);
		BENCH_ADD(fp12_exp_cyc_sps(c, a, k, l));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_frb (1)") {
		fp12_rand(a);
		BENCH_ADD(fp12_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_frb (2)") {
		fp12_rand(a);
		BENCH_ADD(fp12_frb(c, a, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_frb (3)") {
		fp12_rand(a);
		BENCH_ADD(fp12_frb(c, a, 3));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_pck") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		BENCH_ADD(fp12_pck(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp12_upk") {
		fp12_rand(a);
		fp12_conv_cyc(a, a);
		fp12_pck(a, a);
		BENCH_ADD(fp12_upk(c, a));
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

	BENCH_SMALL("fp18_null", fp18_null(a[i]));

	BENCH_SMALL("fp18_new", fp18_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		fp18_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp18_new(a[i]);
	}
	BENCH_SMALL("fp18_free", fp18_free(a[i]));

	(void)a;
}

static void util18(void) {
	fp18_t a, b;

	fp18_null(a);
	fp18_null(b);

	fp18_new(a);
	fp18_new(b);

	BENCH_BEGIN("fp18_copy") {
		fp18_rand(a);
		BENCH_ADD(fp18_copy(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_neg") {
		fp18_rand(a);
		BENCH_ADD(fp18_neg(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_zero") {
		fp18_rand(a);
		BENCH_ADD(fp18_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_is_zero") {
		fp18_rand(a);
		BENCH_ADD((void)fp18_is_zero(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_set_dig (1)") {
		fp18_rand(a);
		BENCH_ADD(fp18_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_set_dig") {
		fp18_rand(a);
		BENCH_ADD(fp18_set_dig(a, a[0][0][0][0]));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_rand") {
		BENCH_ADD(fp18_rand(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_cmp") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_cmp(b, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_cmp_dig") {
		fp18_rand(a);
		BENCH_ADD(fp18_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp18_free(a);
	fp18_free(b);
}

static void arith18(void) {
	fp18_t a, b, c, d[2];
	bn_t e;

	fp18_new(a);
	fp18_new(b);
	fp18_new(c);
	fp18_new(d[0]);
	fp18_new(d[1]);
	bn_new(e);

	BENCH_BEGIN("fp18_add") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_add(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_sub") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_sub(c, a, b));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_mul") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul(c, a, b));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp18_mul_basic") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp18_mul_lazyr") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp18_mul_dxs") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul_dxs(c, a, b));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp18_mul_dxs_basic") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul_dxs_basic(c, a, b));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp18_mul_dxs_lazyr") {
		fp18_rand(a);
		fp18_rand(b);
		BENCH_ADD(fp18_mul_dxs_lazyr(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp18_sqr") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr(c, a));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("fp18_sqr_basic") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("fp18_sqr_lazyr") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr_lazyr(c, a));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("fp18_sqr_cyc") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr_cyc(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_sqr_pck") {
		fp18_rand(a);
		BENCH_ADD(fp18_sqr_pck(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_test_cyc") {
		fp18_rand(a);
		fp18_conv_cyc(a, a);
		BENCH_ADD(fp18_test_cyc(a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_conv_cyc") {
		fp18_rand(a);
		BENCH_ADD(fp18_conv_cyc(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_back_cyc") {
		fp18_rand(a);
		BENCH_ADD(fp18_back_cyc(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_back_cyc (2)") {
		fp18_rand(d[0]);
		fp18_rand(d[1]);
		BENCH_ADD(fp18_back_cyc_sim(d, d, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_conv_uni") {
		fp18_rand(a);
		BENCH_ADD(fp18_conv_uni(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_inv") {
		fp18_rand(a);
		BENCH_ADD(fp18_inv(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_inv_uni") {
		fp18_rand(a);
		BENCH_ADD(fp18_inv_uni(c, a));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_exp") {
		fp18_rand(a);
		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		BENCH_ADD(fp18_exp(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_exp (cyc)") {
		fp18_rand(a);
		fp18_conv_cyc(a, a);
		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		BENCH_ADD(fp18_exp(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_exp_cyc (param or sparse)") {
		fp18_rand(a);
		fp18_conv_cyc(a, a);
		bn_zero(e);
		fp_param_get_var(e);
		if (bn_is_zero(e)) {
			bn_set_2b(e, FP_BITS - 1);
			bn_set_bit(e, FP_BITS / 2, 1);
			bn_set_bit(e, 0, 1);
		}
		BENCH_ADD(fp18_exp(c, a, e));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_exp_cyc_sps (param)") {
		int l = MAX_TERMS + 1, k[MAX_TERMS + 1];
		fp_param_get_sps(k, &l);
		fp18_rand(a);
		BENCH_ADD(fp18_exp_cyc_sps(c, a, k, l));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_frb (1)") {
		fp18_rand(a);
		BENCH_ADD(fp18_frb(c, a, 1));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_frb (2)") {
		fp18_rand(a);
		BENCH_ADD(fp18_frb(c, a, 2));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_frb (3)") {
		fp18_rand(a);
		BENCH_ADD(fp18_frb(c, a, 3));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_frb (4)") {
		fp18_rand(a);
		BENCH_ADD(fp18_frb(c, a, 4));
	}
	BENCH_END;

	BENCH_BEGIN("fp18_frb (5)") {
		fp18_rand(a);
		BENCH_ADD(fp18_frb(c, a, 5));
	}
	BENCH_END;

	fp18_free(a);
	fp18_free(b);
	fp18_free(c);
	fp18_free(d[0]);
	fp18_free(d[1]);
	bn_free(e);
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	util_banner("Benchmarks for the FPX module:", 0);

	/* Try using a pairing-friendly curve for faster exponentiation method. */
	if (pc_param_set_any() != STS_OK) {
		/* If it does not work, try a tower-friendly field. */
		if (fp_param_set_any_tower() == STS_ERR) {
			THROW(ERR_NO_FIELD);
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
		util_banner("Sextic extension:", 0);
		util_banner("Utilities:", 1);
		memory6();
		util6();

		util_banner("Arithmetic:", 1);
		arith6();

		util_banner("Dodecic extension:", 0);
		util_banner("Utilities:", 1);
		memory12();
		util12();

		util_banner("Arithmetic:", 1);
		arith12();
	}

	if (fp_prime_get_qnr() == fp_prime_get_cnr()) {
		util_banner("Octodecic extension:", 0);
		util_banner("Utilities:", 1);
		memory18();
		util18();

		util_banner("Arithmetic:", 1);
		arith18();
	}

	core_clean();
	return 0;
}
