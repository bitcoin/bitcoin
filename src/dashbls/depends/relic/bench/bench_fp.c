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
 * Benchmarks for prime field arithmetic.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	fp_t a[BENCH];

	BENCH_FEW("fp_null", fp_null(a[i]), 1);

	BENCH_FEW("fp_new", fp_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fp_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fp_new(a[i]);
	}
	BENCH_FEW("fp_free", fp_free(a[i]), 1);

	(void)a;
}

static void util(void) {
	char str[2 * RLC_FP_BYTES + 1];
	uint8_t bin[RLC_FP_BYTES];
	fp_t a, b;

	fp_null(a);
	fp_null(b);

	fp_new(a);
	fp_new(b);

	BENCH_RUN("fp_copy") {
		fp_rand(a);
		BENCH_ADD(fp_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp_zero") {
		fp_rand(a);
		BENCH_ADD(fp_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp_is_zero") {
		fp_rand(a);
		BENCH_ADD(fp_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fp_get_bit") {
		fp_rand(a);
		BENCH_ADD(fp_get_bit(a, RLC_DIG / 2));
	}
	BENCH_END;

	BENCH_RUN("fp_set_bit") {
		fp_rand(a);
		BENCH_ADD(fp_set_bit(a, RLC_DIG / 2, 1));
	}
	BENCH_END;

	BENCH_RUN("fp_set_dig (1)") {
		fp_rand(a);
		BENCH_ADD(fp_set_dig(a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp_set_dig") {
		fp_rand(a);
		BENCH_ADD(fp_set_dig(a, a[0]));
	}
	BENCH_END;

	BENCH_RUN("fp_bits") {
		fp_rand(a);
		BENCH_ADD(fp_bits(a));
	}
	BENCH_END;

	BENCH_RUN("fp_rand") {
		BENCH_ADD(fp_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fp_size_str (16)") {
		fp_rand(a);
		BENCH_ADD(fp_size_str(a, 16));
	}
	BENCH_END;

	BENCH_RUN("fp_write_str (16)") {
		fp_rand(a);
		BENCH_ADD(fp_write_str(str, sizeof(str), a, 16));
	}
	BENCH_END;

	BENCH_RUN("fp_read_str (16)") {
		fp_rand(a);
		fp_write_str(str, sizeof(str), a, 16);
		BENCH_ADD(fp_read_str(a, str, sizeof(str), 16));
	}
	BENCH_END;

	BENCH_RUN("fp_write_bin") {
		fp_rand(a);
		BENCH_ADD(fp_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_RUN("fp_read_bin") {
		fp_rand(a);
		fp_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fp_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fp_cmp") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("fp_cmp_dig") {
		fp_rand(a);
		BENCH_ADD(fp_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	fp_free(a);
	fp_free(b);
}

static void arith(void) {
	fp_t a, b, c, f[2];
	dv_t d;
	bn_t e;

	fp_null(a);
	fp_null(b);
	fp_null(c);
	dv_null(d);
	bn_null(e);
	fp_null(f[0]);
	fp_null(f[1]);

	fp_new(a);
	fp_new(b);
	fp_new(c);
	dv_new(d);
	bn_new(e);
	fp_new(f[0]);
	fp_new(f[1]);

	dv_zero(d, RLC_DV_DIGS);

	BENCH_RUN("fp_add") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_add(c, a, b));
	}
	BENCH_END;

#if FP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("fp_add_basic") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_add_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
	BENCH_RUN("fp_add_integ") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_add_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_add_dig (1)") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_add_dig(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp_add_dig") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_add_dig(c, a, b[0]));
	}
	BENCH_END;

	BENCH_RUN("fp_sub") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_sub(c, a, b));
	}
	BENCH_END;

#if FP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("fp_sub_basic") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_sub_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
	BENCH_RUN("fp_sub_integ") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_sub_integ(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_sub_dig (1)") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_sub_dig(c, a, 1));
	}
	BENCH_END;

	BENCH_RUN("fp_sub_dig") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_sub_dig(c, a, b[0]));
	}
	BENCH_END;

	BENCH_RUN("fp_neg") {
		fp_rand(a);
		BENCH_ADD(fp_neg(b, a));
	}
	BENCH_END;

#if FP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("fp_neg_basic") {
		fp_rand(a);
		BENCH_ADD(fp_neg_basic(c, a));
	}
	BENCH_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
	BENCH_RUN("fp_neg_integ") {
		fp_rand(a);
		BENCH_ADD(fp_neg_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_mul") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_mul(c, a, b));
	}
	BENCH_END;

#if FP_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("fp_mul_basic") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FP_MUL == INTEG || !defined(STRIP)
	BENCH_RUN("fp_mul_integ") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_mul_integ(c, a, b));
	}
	BENCH_END;
#endif

#if FP_MUL == COMBA || !defined(STRIP)
	BENCH_RUN("fp_mul_comba") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_mul_comba(c, a, b));
	}
	BENCH_END;
#endif

#if FP_KARAT > 0 || !defined(STRIP)
	BENCH_RUN("fp_mul_karat") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_mul_karat(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_mul_dig") {
		fp_rand(a);
		fp_rand(b);
		BENCH_ADD(fp_mul_dig(c, a, b[0]));
	}
	BENCH_END;

	BENCH_RUN("fp_sqr") {
		fp_rand(a);
		BENCH_ADD(fp_sqr(c, a));
	}
	BENCH_END;

#if FP_SQR == BASIC || !defined(STRIP)
	BENCH_RUN("fp_sqr_basic") {
		fp_rand(a);
		BENCH_ADD(fp_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FP_SQR == INTEG || !defined(STRIP)
	BENCH_RUN("fp_sqr_integ") {
		fp_rand(a);
		BENCH_ADD(fp_sqr_integ(c, a));
	}
	BENCH_END;
#endif

#if FP_SQR == COMBA || !defined(STRIP)
	BENCH_RUN("fp_sqr_comba") {
		fp_rand(a);
		BENCH_ADD(fp_sqr_comba(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_dbl") {
		fp_rand(a);
		BENCH_ADD(fp_dbl(c, a));
	}
	BENCH_END;

#if FP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("fp_dbl_basic") {
		fp_rand(a);
		BENCH_ADD(fp_dbl_basic(c, a));
	}
	BENCH_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
	BENCH_RUN("fp_dbl_integ") {
		fp_rand(a);
		BENCH_ADD(fp_dbl_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_hlv") {
		fp_rand(a);
		BENCH_ADD(fp_hlv(c, a));
	}
	BENCH_END;

#if FP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("fp_hlv_basic") {
		fp_rand(a);
		BENCH_ADD(fp_hlv_basic(c, a));
	}
	BENCH_END;
#endif

#if FP_ADD == INTEG || !defined(STRIP)
	BENCH_RUN("fp_hlv_integ") {
		fp_rand(a);
		BENCH_ADD(fp_hlv_integ(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_lsh") {
		fp_rand(a);
		a[RLC_FP_DIGS - 1] = 0;
		BENCH_ADD(fp_lsh(c, a, RLC_DIG / 2));
	}
	BENCH_END;

	BENCH_RUN("fp_rsh") {
		fp_rand(a);
		a[RLC_FP_DIGS - 1] = 0;
		BENCH_ADD(fp_rsh(c, a, RLC_FP_BITS / 2));
	}
	BENCH_END;

	BENCH_RUN("fp_rdc") {
		fp_rand(a);
		fp_lsh(d, a, RLC_FP_BITS);
		BENCH_ADD(fp_rdc(c, d));
	}
	BENCH_END;

#if FP_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fp_rdc_basic") {
		fp_rand(a);
		fp_lsh(d, a, RLC_FP_BITS);
		BENCH_ADD(fp_rdc_basic(c, d));
	}
	BENCH_END;
#endif

#if FP_RDC == MONTY || !defined(STRIP)
	BENCH_RUN("fp_rdc_monty") {
		fp_rand(a);
		fp_lsh(d, a, RLC_FP_BITS);
		BENCH_ADD(fp_rdc_monty(c, d));
	}
	BENCH_END;

#if FP_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("fp_rdc_monty_basic") {
		fp_rand(a);
		fp_lsh(d, a, RLC_FP_BITS);
		BENCH_ADD(fp_rdc_monty_basic(c, d));
	}
	BENCH_END;
#endif

#if FP_MUL == COMBA || !defined(STRIP)
	BENCH_RUN("fp_rdc_monty_comba") {
		fp_rand(a);
		fp_lsh(d, a, RLC_FP_BITS);
		BENCH_ADD(fp_rdc_monty_comba(c, d));
	}
	BENCH_END;
#endif
#endif

#if FP_RDC == QICK || !defined(STRIP)
	if (fp_prime_get_sps(NULL) != NULL) {
		BENCH_RUN("fp_rdc_quick") {
			fp_rand(a);
			fp_lsh(d, a, RLC_FP_BITS);
			BENCH_ADD(fp_rdc_quick(c, d));
		}
		BENCH_END;
	}
#endif

	BENCH_RUN("fp_inv") {
		fp_rand(a);
		BENCH_ADD(fp_inv(c, a));
	}
	BENCH_END;

#if FP_INV == BASIC || !defined(STRIP)
	BENCH_RUN("fp_inv_basic") {
		fp_rand(a);
		BENCH_ADD(fp_inv_basic(c, a));
	}
	BENCH_END;
#endif

#if FP_INV == BINAR || !defined(STRIP)
	BENCH_RUN("fp_inv_binar") {
		fp_rand(a);
		BENCH_ADD(fp_inv_binar(c, a));
	}
	BENCH_END;
#endif

#if FP_INV == MONTY || !defined(STRIP)
	BENCH_RUN("fp_inv_monty") {
		fp_rand(a);
		BENCH_ADD(fp_inv_monty(c, a));
	}
	BENCH_END;
#endif

#if FP_INV == EXGCD || !defined(STRIP)
	BENCH_RUN("fp_inv_exgcd") {
		fp_rand(a);
		BENCH_ADD(fp_inv_exgcd(c, a));
	}
	BENCH_END;
#endif

#if FP_INV == DIVST || !defined(STRIP)
	BENCH_RUN("fp_inv_divst") {
		fp_rand(a);
		BENCH_ADD(fp_inv_divst(c, a));
	}
	BENCH_END;
#endif

#if FP_INV == LOWER || !defined(STRIP)
	BENCH_RUN("fp_inv_lower") {
		fp_rand(a);
		BENCH_ADD(fp_inv_lower(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_inv_sim (2)") {
		fp_rand(f[0]);
		fp_rand(f[1]);
		BENCH_ADD(fp_inv_sim(f, (const fp_t *)f, 2));
	}
	BENCH_END;

	BENCH_RUN("fp_exp") {
		fp_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp_exp(c, a, e));
	}
	BENCH_END;

#if FP_EXP == BASIC || !defined(STRIP)
	BENCH_RUN("fp_exp_basic") {
		fp_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp_exp_basic(c, a, e));
	}
	BENCH_END;
#endif

#if FP_EXP == SLIDE || !defined(STRIP)
	BENCH_RUN("fp_exp_slide") {
		fp_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp_exp_slide(c, a, e));
	}
	BENCH_END;
#endif

#if FP_EXP == MONTY || !defined(STRIP)
	BENCH_RUN("fp_exp_monty") {
		fp_rand(a);
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp_exp_monty(c, a, e));
	}
	BENCH_END;
#endif

	BENCH_RUN("fp_srt") {
		fp_rand(a);
		fp_sqr(a, a);
		BENCH_ADD(fp_srt(c, a));
	}
	BENCH_END;

	BENCH_RUN("fp_prime_conv") {
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp_prime_conv(a, e));
	}
	BENCH_END;

	BENCH_RUN("fp_prime_conv_dig") {
		bn_rand(e, RLC_POS, RLC_FP_BITS);
		BENCH_ADD(fp_prime_conv_dig(a, e->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("fp_prime_back") {
		fp_rand(c);
		BENCH_ADD(fp_prime_back(e, c));
	}
	BENCH_END;

	fp_free(a);
	fp_free(b);
	fp_free(c);
	dv_free(d);
	bn_free(e);
	fp_free(f[0]);
	fp_free(f[1]);
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the FP module:", 0);

	fp_param_set_any();
	fp_param_print();

	util_banner("Utilities:\n", 0);
	memory();
	util();
	util_banner("Arithmetic:\n", 0);
	arith();

	core_clean();
	return 0;
}
