/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2019 RELIC Authors
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
 * Implementation of context-hiding linearly homomophic signature protocol.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_cmlhs_init(g1_t h) {
	g1_rand(h);
	return RLC_OK;
}

int cp_cmlhs_gen(bn_t x[], gt_t hs[], int len, uint8_t prf[], int plen,
		bn_t sk, g2_t pk, bn_t d, g2_t y) {
	g1_t g1;
	g2_t g2;
	gt_t gt;
	bn_t n;
	int result = RLC_OK;

	g1_null(g1);
	g2_null(g2);
	gt_null(gt);
	bn_null(n);

	RLC_TRY {
		bn_new(n);
		g1_new(g1);
		g2_new(g2);
		gt_new(gt);

		pc_get_ord(n);
		g1_get_gen(g1);
		g2_get_gen(g2);
		pc_map(gt, g1, g2);

		rand_bytes(prf, plen);
		cp_bls_gen(sk, pk);

		pc_get_ord(n);
		/* Generate elements for n tags. */
		for (int i = 0; i < len; i++) {
			bn_rand_mod(x[i], n);
			gt_exp(hs[i], gt, x[i]);
		}

		bn_rand_mod(d, n);
		g2_mul_gen(y, d);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		g1_free(g1);
		g2_free(g2);
		gt_free(gt);
		bn_free(n);
	}
	return result;
}

int cp_cmlhs_sig(g1_t sig, g2_t z, g1_t a, g1_t c, g1_t r, g2_t s, bn_t msg,
		char *data, int label, bn_t x, g1_t h, uint8_t prf[], int plen,
		bn_t d, bn_t sk) {
	bn_t k, m, n;
	g1_t t;
	uint8_t mac[RLC_MD_LEN];
	int len, dlen = strlen(data), result = RLC_OK;
	uint8_t *buf = RLC_ALLOCA(uint8_t, 1 + 8 * RLC_PC_BYTES + dlen);

	bn_null(k);
	bn_null(m);
	bn_null(n);
	g1_null(t);

	RLC_TRY {
		bn_new(k);
		bn_new(m);
		bn_new(n);
		g1_new(t);
		if (buf == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		pc_get_ord(n);
		/* Generate r and s. */
		bn_rand_mod(k, n);
		bn_rand_mod(m, n);

		/* Compute S = -g2^s, C = g1^s. */
		g2_mul_gen(s, m);
		g2_neg(s, s);
		g1_mul_gen(c, m);
		/* Compute R = g1^(r - ys). */
		bn_mul(m, d, m);
		bn_mod(m, m, n);
		bn_sub(m, k, m);
		bn_mod(m, m, n);
		g1_mul_gen(r, m);

		/* Compute A = g1^(x + r) * \prod H_j^(y * m_j). */
		bn_add(k, x, k);
		bn_mod(k, k, n);
		g1_mul_gen(a, k);
		bn_mul(k, d, msg);
		bn_mod(k, k, n);
		g1_mul(t, h, k);
		g1_add(a, a, t);
		g1_norm(a, a);
		/* Compute z = F_K(delta), Z = g2^z, A = A^(1/z). */
		md_hmac(mac, (const uint8_t *)data, dlen, prf, plen);
		bn_read_bin(k, mac, RLC_MD_LEN);
		bn_mod(k, k, n);
		g2_mul_gen(z, k);
		bn_mod_inv(k, k, n);
		g1_mul(a, a, k);

		/* Compute C = C * sum H_j^m_j. */
		bn_mod(k, msg, n);
		g1_mul(t, h, k);
		g1_add(c, c, t);
		g1_norm(c, c);

		len = g2_size_bin(z, 0);
		g2_write_bin(buf, len, z, 0);
		memcpy(buf + len, data, dlen);
		cp_bls_sig(sig, buf, len + dlen, sk);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(k);
		bn_free(m);
		bn_free(n);
		g1_free(t);
		RLC_FREE(buf);
	}
	return result;
}

int cp_cmlhs_fun(g1_t a, g1_t c, g1_t as[], g1_t cs[], dig_t f[], int len) {
	int result = RLC_OK;

	g1_mul_sim_dig(a, as, f, len);
	g1_mul_sim_dig(c, cs, f, len);

	return result;
}

int cp_cmlhs_evl(g1_t r, g2_t s, g1_t rs[], g2_t ss[], dig_t f[], int len) {
	int result = RLC_OK;

	g1_mul_sim_dig(r, rs, f, len);
	g2_mul_sim_dig(s, ss, f, len);

	return result;
}

int cp_cmlhs_ver(g1_t r, g2_t s, g1_t sig[], g2_t z[], g1_t a[], g1_t c[],
		bn_t msg, char *data, g1_t h, int label[], gt_t *hs[],
		dig_t *f[], int flen[], g2_t y[], g2_t pk[], int slen) {
	g1_t g1;
	g2_t g2;
	gt_t e, u, v;
	bn_t k, n;
	int len, dlen = strlen(data), result = 1;
	uint8_t *buf = RLC_ALLOCA(uint8_t, 1 + 8 * RLC_PC_BYTES + dlen);

	g1_null(g1);
	g2_null(g2);
	gt_null(e);
	gt_null(u);
	gt_null(v);
	bn_null(k);
	bn_null(n);

	RLC_TRY {
		g1_new(g1);
		g2_new(g2);
		gt_new(e);
		gt_new(u);
		gt_new(v);
		bn_new(k);
		bn_new(n);
		if (buf == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		pc_get_ord(n);
		g1_get_gen(g1);
		g2_get_gen(g2);

		for (int i = 0; i < slen; i++) {
			len = g2_size_bin(z[i], 0);
			g2_write_bin(buf, len, z[i], 0);
			memcpy(buf + len, data, dlen);
			if (cp_bls_ver(sig[i], buf, len + dlen, pk[i]) == 0) {
				result = 0;
			}
		}

		pc_map_sim(e, a, z, slen);
		pc_map_sim(u, c, y, slen);
		pc_map(v, r, g2);
		gt_mul(u, u, v);

		for (int i = 0; i < slen; i++) {
			for (int j = 0; j < flen[i]; j++) {
				gt_exp_dig(v, hs[i][label[j]], f[i][j]);
				gt_mul(u, u, v);
			}
		}
		if (gt_cmp(e, u) != RLC_EQ) {
			result = 0;
		}

		pc_map(e, g1, s);
		g1_set_infty(g1);
		for (int i = 0; i < slen; i++) {
			g1_add(g1, g1, c[i]);
		}
		g1_norm(g1, g1);
		pc_map(u, g1, g2);
		gt_mul(e, e, u);

		g1_mul(g1, h, msg);
		pc_map(v, g1, g2);
		if (gt_cmp(e, v) != RLC_EQ) {
			result = 0;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		g1_free(g1);
		g2_free(g2);
		gt_free(e);
		gt_free(u);
		gt_free(v);
		bn_free(k);
		bn_free(n);
		RLC_FREE(buf);
	}
	return result;
}

void cp_cmlhs_off(gt_t vk, g1_t h, int label[], gt_t *hs[], dig_t *f[],
		int flen[], g2_t y[], g2_t pk[], int slen) {
	gt_t v;

	gt_null(v);

	RLC_TRY {
		gt_new(v);

		gt_set_unity(vk);
		for (int i = 0; i < slen; i++) {
			for (int j = 0; j < flen[i]; j++) {
				gt_exp_dig(v, hs[i][label[j]], f[i][j]);
				gt_mul(vk, vk, v);
			}
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		gt_free(v);
	}
}

int cp_cmlhs_onv(g1_t r, g2_t s, g1_t sig[], g2_t z[], g1_t a[], g1_t c[],
		bn_t msg, char *data, g1_t h, gt_t vk, g2_t y[], g2_t pk[], int slen) {
	g1_t g1;
	g2_t g2;
	gt_t e, u, v;
	bn_t k, n;
	int len, dlen = strlen(data), result = 1;
	uint8_t *buf = RLC_ALLOCA(uint8_t, 1 + 8 * RLC_FP_BYTES + dlen);

	g1_null(g1);
	g2_null(g2);
	gt_null(e);
	gt_null(u);
	gt_null(v);
	bn_null(k);
	bn_null(n);

	RLC_TRY {
		g1_new(g1);
		g2_new(g2);
		gt_new(e);
		gt_new(u);
		gt_new(v);
		bn_new(k);
		bn_new(n);
		if (buf == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		pc_get_ord(n);
		g1_get_gen(g1);
		g2_get_gen(g2);

		for (int i = 0; i < slen; i++) {
			len = g2_size_bin(z[i], 0);
			g2_write_bin(buf, len, z[i], 0);
			memcpy(buf + len, data, dlen);
			if (cp_bls_ver(sig[i], buf, len + dlen, pk[i]) == 0) {
				result = 0;
			}
		}

		pc_map_sim(e, a, z, slen);
		pc_map_sim(u, c, y, slen);
		pc_map(v, r, g2);
		gt_mul(u, u, v);
		gt_mul(u, u, vk);

		if (gt_cmp(e, u) != RLC_EQ) {
			result = 0;
		}

		pc_map(e, g1, s);
		g1_set_infty(g1);
		for (int i = 0; i < slen; i++) {
			g1_add(g1, g1, c[i]);
		}
		g1_norm(g1, g1);
		pc_map(u, g1, g2);
		gt_mul(e, e, u);

		g1_mul(g1, h, msg);
		pc_map(v, g1, g2);
		if (gt_cmp(e, v) != RLC_EQ) {
			result = 0;
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		g1_free(g1);
		g2_free(g2);
		gt_free(e);
		gt_free(u);
		gt_free(v);
		bn_free(k);
		bn_free(n);
		RLC_FREE(buf);
	}
	return result;
}
