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
 * Benchmarks for binary field arithmetic.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	fb_t a[BENCH];

	BENCH_FEW("fb_null", fb_null(a[i]), 1);

	BENCH_FEW("fb_new", fb_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		fb_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		fb_new(a[i]);
	}
	BENCH_FEW("fb_free", fb_free(a[i]), 1);

	(void)a;
}

static void util(void) {
	char str[2 * RLC_FB_BYTES + 1];
	uint8_t bin[RLC_FB_BYTES];
	fb_t a, b;

	fb_null(a);
	fb_null(b);

	fb_new(a);
	fb_new(b);

	BENCH_RUN("fb_copy") {
		fb_rand(a);
		BENCH_ADD(fb_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("fb_zero") {
		fb_rand(a);
		BENCH_ADD(fb_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fb_is_zero") {
		fb_rand(a);
		BENCH_ADD(fb_is_zero(a));
	}
	BENCH_END;

	BENCH_RUN("fb_get_bit") {
		fb_rand(a);
		BENCH_ADD(fb_get_bit(a, RLC_DIG / 2));
	}
	BENCH_END;

	BENCH_RUN("fb_set_bit") {
		fb_rand(a);
		BENCH_ADD(fb_set_bit(a, RLC_DIG / 2, 1));
	}
	BENCH_END;

	BENCH_RUN("fb_set_dig") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_set_dig(a, b[0]));
	}
	BENCH_END;

	BENCH_RUN("fb_bits") {
		fb_rand(a);
		BENCH_ADD(fb_bits(a));
	}
	BENCH_END;

	BENCH_RUN("fb_rand") {
		BENCH_ADD(fb_rand(a));
	}
	BENCH_END;

	BENCH_RUN("fb_size_str (16)") {
		fb_rand(a);
		BENCH_ADD(fb_size_str(a, 16));
	}
	BENCH_END;

	BENCH_RUN("fb_write_str (16)") {
		fb_rand(a);
		BENCH_ADD(fb_write_str(str, sizeof(str), a, 16));
	}
	BENCH_END;

	BENCH_RUN("fb_read_str (16)") {
		fb_rand(a);
		fb_write_str(str, sizeof(str), a, 16);
		BENCH_ADD(fb_read_str(a, str, sizeof(str), 16));
	}
	BENCH_END;

	BENCH_RUN("fb_write_bin") {
		fb_rand(a);
		BENCH_ADD(fb_write_bin(bin, sizeof(bin), a));
	}
	BENCH_END;

	BENCH_RUN("fb_read_bin") {
		fb_rand(a);
		fb_write_bin(bin, sizeof(bin), a);
		BENCH_ADD(fb_read_bin(a, bin, sizeof(bin)));
	}
	BENCH_END;

	BENCH_RUN("fb_cmp_dig") {
		fb_rand(a);
		BENCH_ADD(fb_cmp_dig(a, (dig_t)0));
	}
	BENCH_END;

	BENCH_RUN("fb_cmp") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_cmp(b, a));
	}
	BENCH_END;

	fb_free(a);
	fb_free(b);
}

static void arith(void) {
	fb_t a, b, c, d[2];
	fb_st t[RLC_FB_TABLE_MAX];
	dv_t e;
	bn_t f;
	int bits;

	fb_null(a);
	fb_null(b);
	fb_null(c);
	fb_null(d[0]);
	fb_null(d[1]);
	dv_null(e);
	bn_null(f);

	fb_new(a);
	fb_new(b);
	fb_new(c);
	fb_new(d[0]);
	fb_new(d[1]);
	dv_new(e);
	dv_zero(e, 2 * RLC_FB_DIGS);
	bn_new(f);

	BENCH_RUN("fb_add") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_add(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fb_add_dig") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_add_dig(c, a, b[0]));
	}
	BENCH_END;

	BENCH_RUN("fb_poly_add") {
		fb_rand(a);
		BENCH_ADD(fb_poly_add(c, a));
	}
	BENCH_END;

	BENCH_RUN("fb_mul") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_mul(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("fb_mul_dig") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_mul_dig(c, a, b[0]));
	}
	BENCH_END;

#if FB_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("fb_mul_basic") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_mul_basic(c, a, b));
	}
	BENCH_END;
#endif

#if FB_MUL == INTEG || !defined(STRIP)
	BENCH_RUN("fb_mul_integ") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_mul_integ(c, a, b));
	}
	BENCH_END;
#endif

#if FB_MUL == LODAH || !defined(STRIP)
	BENCH_RUN("fb_mul_lodah") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_mul_lodah(c, a, b));
	}
	BENCH_END;
#endif

#if FB_KARAT > 0 || !defined(STRIP)
	BENCH_RUN("fb_mul_karat") {
		fb_rand(a);
		fb_rand(b);
		BENCH_ADD(fb_mul_karat(c, a, b));
	}
	BENCH_END;
#endif

	BENCH_RUN("fb_sqr") {
		fb_rand(a);
		BENCH_ADD(fb_sqr(c, a));
	}
	BENCH_END;

#if FB_SQR == BASIC || !defined(STRIP)
	BENCH_RUN("fb_sqr_basic") {
		fb_rand(a);
		BENCH_ADD(fb_sqr_basic(c, a));
	}
	BENCH_END;
#endif

#if FB_SQR == INTEG || !defined(STRIP)
	BENCH_RUN("fb_sqr_integ") {
		fb_rand(a);
		BENCH_ADD(fb_sqr_integ(c, a));
	}
	BENCH_END;
#endif

#if FB_SQR == QUICK || !defined(STRIP)
	BENCH_RUN("fb_sqr_quick") {
		fb_rand(a);
		BENCH_ADD(fb_sqr_quick(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fb_lsh") {
		fb_rand(a);
		a[RLC_FB_DIGS - 1] = 0;
		bits = a[0] & RLC_MASK(RLC_DIG_LOG);
		BENCH_ADD(fb_lsh(c, a, bits));
	}
	BENCH_END;

	BENCH_RUN("fb_rsh") {
		fb_rand(a);
		a[RLC_FB_DIGS - 1] = 0;
		bits = a[0] & RLC_MASK(RLC_DIG_LOG);
		BENCH_ADD(fb_rsh(c, a, bits));

	}
	BENCH_END;

	BENCH_RUN("fb_rdc") {
		fb_rand(a);
		fb_lsh(e, a, RLC_FB_BITS);
		fb_rand(e);
		BENCH_ADD(fb_rdc(c, e));
	}
	BENCH_END;

#if FB_RDC == BASIC || !defined(STRIP)
	BENCH_RUN("fb_rdc_basic") {
		fb_rand(a);
		fb_lsh(e, a, RLC_FB_BITS);
		fb_rand(e);
		BENCH_ADD(fb_rdc_basic(c, e));
	}
	BENCH_END;
#endif

#if FB_RDC == QUICK || !defined(STRIP)
	BENCH_RUN("fb_rdc_quick") {
		fb_rand(a);
		fb_lsh(e, a, RLC_FB_BITS);
		fb_rand(e);
		BENCH_ADD(fb_rdc_quick(c, e));
	}
	BENCH_END;
#endif

	BENCH_RUN("fb_srt") {
		fb_rand(a);
		fb_sqr(e, a);
		BENCH_ADD(fb_srt(c, e));
	}
	BENCH_END;

#if FB_SRT == BASIC || !defined(STRIP)
	BENCH_RUN("fb_srt_basic") {
		fb_rand(a);
		fb_sqr(e, a);
		BENCH_ADD(fb_srt_basic(c, e));
	}
	BENCH_END;
#endif

#if FB_SRT == QUICK || !defined(STRIP)
	BENCH_RUN("fb_srt_quick") {
		fb_rand(a);
		fb_sqr(e, a);
		BENCH_ADD(fb_srt_quick(c, e));
	}
	BENCH_END;
#endif

	BENCH_RUN("fb_trc") {
		fb_rand(a);
		BENCH_ADD(fb_trc(a));
	}
	BENCH_END;

#if FB_TRC == BASIC || !defined(STRIP)
	BENCH_RUN("fb_trc_basic") {
		fb_rand(a);
		BENCH_ADD(fb_trc_basic(a));
	}
	BENCH_END;
#endif

#if FB_TRC == QUICK || !defined(STRIP)
	BENCH_RUN("fb_trc_quick") {
		fb_rand(a);
		BENCH_ADD(fb_trc_quick(a));
	}
	BENCH_END;
#endif

	if (RLC_FB_BITS % 2 != 0) {
		BENCH_RUN("fb_slv") {
			fb_rand(a);
			BENCH_ADD(fb_slv(c, a));
		}
		BENCH_END;

#if FB_SLV == BASIC || !defined(STRIP)
		BENCH_RUN("fb_slv_basic") {
			fb_rand(a);
			BENCH_ADD(fb_slv_basic(c, a));
		}
		BENCH_END;
#endif

#if FB_SLV == QUICK || !defined(STRIP)
		BENCH_RUN("fb_slv_quick") {
			fb_rand(a);
			BENCH_ADD(fb_slv_quick(c, a));
		}
		BENCH_END;
#endif
	}

	BENCH_RUN("fb_inv") {
		fb_rand(a);
		BENCH_ADD(fb_inv(c, a));
	}
	BENCH_END;

#if FB_INV == BASIC || !defined(STRIP)
	BENCH_RUN("fb_inv_basic") {
		fb_rand(a);
		BENCH_ADD(fb_inv_basic(c, a));
	}
	BENCH_END;
#endif

#if FB_INV == BINAR || !defined(STRIP)
	BENCH_RUN("fb_inv_binar") {
		fb_rand(a);
		BENCH_ADD(fb_inv_binar(c, a));
	}
	BENCH_END;
#endif

#if FB_INV == ALMOS || !defined(STRIP)
	BENCH_RUN("fb_inv_almos") {
		fb_rand(a);
		BENCH_ADD(fb_inv_almos(c, a));
	}
	BENCH_END;
#endif

#if FB_INV == EXGCD || !defined(STRIP)
	BENCH_RUN("fb_inv_exgcd") {
		fb_rand(a);
		BENCH_ADD(fb_inv_exgcd(c, a));
	}
	BENCH_END;
#endif

#if FB_INV == BRUCH || !defined(STRIP)
	BENCH_RUN("fb_inv_bruch") {
		fb_rand(a);
		BENCH_ADD(fb_inv_bruch(c, a));
	}
	BENCH_END;
#endif

#if FB_INV == ITOHT || !defined(STRIP)
	BENCH_RUN("fb_inv_itoht") {
		fb_rand(a);
		BENCH_ADD(fb_inv_itoht(c, a));
	}
	BENCH_END;
#endif

#if FB_INV == CTAIA || !defined(STRIP)
	BENCH_RUN("fb_inv_ctaia") {
		fb_rand(a);
		BENCH_ADD(fb_inv_ctaia(c, a));
	}
	BENCH_END;
#endif

#if FB_INV == LOWER || !defined(STRIP)
	BENCH_RUN("fb_inv_lower") {
		fb_rand(a);
		BENCH_ADD(fb_inv_lower(c, a));
	}
	BENCH_END;
#endif

	BENCH_RUN("fb_inv_sim (2)") {
		fb_rand(d[0]);
		fb_rand(d[1]);
		BENCH_ADD(fb_inv_sim(d, (const fb_t *)d, 2));
	}
	BENCH_END;

	BENCH_RUN("fb_exp") {
		fb_rand(a);
		bn_rand(f, RLC_POS, RLC_FB_BITS);
		BENCH_ADD(fb_exp(c, a, f));
	}
	BENCH_END;

#if FB_EXP == BASIC || !defined(STRIP)
	BENCH_RUN("fb_exp_basic") {
		fb_rand(a);
		bn_rand(f, RLC_POS, RLC_FB_BITS);
		BENCH_ADD(fb_exp_basic(c, a, f));
	}
	BENCH_END;
#endif

#if FB_EXP == SLIDE || !defined(STRIP)
	BENCH_RUN("fb_exp_slide") {
		fb_rand(a);
		bn_rand(f, RLC_POS, RLC_FB_BITS);
		BENCH_ADD(fb_exp_slide(c, a, f));
	}
	BENCH_END;
#endif

#if FB_EXP == MONTY || !defined(STRIP)
	BENCH_RUN("fb_exp_monty") {
		fb_rand(a);
		bn_rand(f, RLC_POS, RLC_FB_BITS);
		BENCH_ADD(fb_exp_monty(c, a, f));
	}
	BENCH_END;
#endif

	BENCH_RUN("fb_itr") {
		fb_rand(a);
		bn_rand(f, RLC_POS, 8);
		fb_itr_pre(t, f->dp[0]);
		BENCH_ADD(fb_itr(c, a, f->dp[0], t));
	}
	BENCH_END;

#if FB_ITR == BASIC || !defined(STRIP)
	BENCH_RUN("fb_itr_basic") {
		fb_rand(a);
		bn_rand(f, RLC_POS, 8);
		BENCH_ADD(fb_itr_basic(c, a, f->dp[0]));
	}
	BENCH_END;
#endif

#if FB_ITR == QUICK || !defined(STRIP)
	BENCH_RUN("fb_itr_quick") {
		fb_rand(a);
		bn_rand(f, RLC_POS, 8);
		fb_itr_pre_quick(t, f->dp[0]);
		BENCH_ADD(fb_itr_quick(c, a, t));
	}
	BENCH_END;
#endif

	fb_free(a);
	fb_free(b);
	fb_free(c);
	fb_free(d[0]);
	fb_free(d[1]);
	dv_free(e);
	bn_free(f);
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the FB module:", 0);

	fb_param_set_any();
	fb_param_print();
	util_banner("Utilities:\n", 0);
	memory();
	util();
	util_banner("Arithmetic:\n", 0);
	arith();

	core_clean();
	return 0;
}
