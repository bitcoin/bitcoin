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
 * Benchmarks for cryptographic protocols.
 *
 * @version $Id$
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

#if defined(WITH_BN)

static void rsa(void) {
	rsa_t pub, prv;
	uint8_t in[10], new[10], h[RLC_MD_LEN], out[RLC_BN_BITS / 8 + 1];
	int out_len, new_len;

	rsa_null(pub);
	rsa_null(prv);

	rsa_new(pub);
	rsa_new(prv);

	BENCH_ONE("cp_rsa_gen", cp_rsa_gen(pub, prv, RLC_BN_BITS), 1);

	BENCH_RUN("cp_rsa_enc") {
		out_len = RLC_BN_BITS / 8 + 1;
		new_len = out_len;
		rand_bytes(in, sizeof(in));
		BENCH_ADD(cp_rsa_enc(out, &out_len, in, sizeof(in), pub));
		cp_rsa_dec(new, &new_len, out, out_len, prv);
	} BENCH_END;

	BENCH_RUN("cp_rsa_dec") {
		out_len = RLC_BN_BITS / 8 + 1;
		new_len = out_len;
		rand_bytes(in, sizeof(in));
		cp_rsa_enc(out, &out_len, in, sizeof(in), pub);
		BENCH_ADD(cp_rsa_dec(new, &new_len, out, out_len, prv));
	} BENCH_END;

	BENCH_RUN("cp_rsa_sig (h = 0)") {
		out_len = RLC_BN_BITS / 8 + 1;
		new_len = out_len;
		rand_bytes(in, sizeof(in));
		BENCH_ADD(cp_rsa_sig(out, &out_len, in, sizeof(in), 0, prv));
	} BENCH_END;

	BENCH_RUN("cp_rsa_sig (h = 1)") {
		out_len = RLC_BN_BITS / 8 + 1;
		new_len = out_len;
		rand_bytes(in, sizeof(in));
		md_map(h, in, sizeof(in));
		BENCH_ADD(cp_rsa_sig(out, &out_len, h, RLC_MD_LEN, 1, prv));
	} BENCH_END;

	BENCH_RUN("cp_rsa_ver (h = 0)") {
		out_len = RLC_BN_BITS / 8 + 1;
		new_len = out_len;
		rand_bytes(in, sizeof(in));
		cp_rsa_sig(out, &out_len, in, sizeof(in), 0, prv);
		BENCH_ADD(cp_rsa_ver(out, out_len, in, sizeof(in), 0, pub));
	} BENCH_END;

	BENCH_RUN("cp_rsa_ver (h = 1)") {
		out_len = RLC_BN_BITS / 8 + 1;
		new_len = out_len;
		rand_bytes(in, sizeof(in));
		md_map(h, in, sizeof(in));
		cp_rsa_sig(out, &out_len, h, RLC_MD_LEN, 1, prv);
		BENCH_ADD(cp_rsa_ver(out, out_len, h, RLC_MD_LEN, 1, pub));
	} BENCH_END;

	rsa_free(pub);
	rsa_free(prv);
}

static void rabin(void) {
	rabin_t pub, prv;
	uint8_t in[1000], new[1000], out[RLC_BN_BITS / 8 + 1];
	int in_len, out_len, new_len;

	rabin_null(pub);
	rabin_null(prv);

	rabin_new(pub);
	rabin_new(prv);

	BENCH_ONE("cp_rabin_gen", cp_rabin_gen(pub, prv, RLC_BN_BITS), 1);

	BENCH_RUN("cp_rabin_enc") {
		in_len = bn_size_bin(pub->n) - 9;
		out_len = RLC_BN_BITS / 8 + 1;
		rand_bytes(in, in_len);
		BENCH_ADD(cp_rabin_enc(out, &out_len, in, in_len, pub));
		cp_rabin_dec(new, &new_len, out, out_len, prv);
	} BENCH_END;

	BENCH_RUN("cp_rabin_dec") {
		in_len = bn_size_bin(pub->n) - 9;
		new_len = in_len;
		out_len = RLC_BN_BITS / 8 + 1;
		rand_bytes(in, in_len);
		cp_rabin_enc(out, &out_len, in, in_len, pub);
		BENCH_ADD(cp_rabin_dec(new, &new_len, out, out_len, prv));
	} BENCH_END;

	rabin_free(pub);
	rabin_free(prv);
}

static void benaloh(void) {
	bdpe_t pub, prv;
	dig_t in, new;
	uint8_t out[RLC_BN_BITS / 8 + 1];
	int out_len;

	bdpe_null(pub);
	bdpe_null(prv);

	bdpe_new(pub);
	bdpe_new(prv);

	BENCH_ONE("cp_bdpe_gen", cp_bdpe_gen(pub, prv, bn_get_prime(47),
		RLC_BN_BITS), 1);

	BENCH_RUN("cp_bdpe_enc") {
		out_len = RLC_BN_BITS / 8 + 1;
		rand_bytes(out, 1);
		in = out[0] % bn_get_prime(47);
		BENCH_ADD(cp_bdpe_enc(out, &out_len, in, pub));
		cp_bdpe_dec(&new, out, out_len, prv);
	} BENCH_END;

	BENCH_RUN("cp_bdpe_dec") {
		out_len = RLC_BN_BITS / 8 + 1;
		rand_bytes(out, 1);
		in = out[0] % bn_get_prime(47);
		cp_bdpe_enc(out, &out_len, in, pub);
		BENCH_ADD(cp_bdpe_dec(&new, out, out_len, prv));
	} BENCH_END;

	bdpe_free(pub);
	bdpe_free(prv);
}

static void paillier(void) {
	bn_t c, m, pub;
	phpe_t prv;

	bn_null(c);
	bn_null(m);
	bn_null(pub);
	phpe_null(prv);

	bn_new(c);
	bn_new(m);
	bn_new(pub);
	phpe_new(prv);

	BENCH_ONE("cp_phpe_gen", cp_phpe_gen(pub, prv, RLC_BN_BITS / 2), 1);

	BENCH_RUN("cp_phpe_enc") {
		bn_rand_mod(m, pub);
		BENCH_ADD(cp_phpe_enc(c, m, pub));
	} BENCH_END;

	BENCH_RUN("cp_phpe_dec") {
		bn_rand_mod(m, pub);
		cp_phpe_enc(c, m, pub);
		BENCH_ADD(cp_phpe_dec(m, c, prv));
	} BENCH_END;

	BENCH_ONE("cp_ghpe_gen", cp_ghpe_gen(pub, prv->n, RLC_BN_BITS / 2), 1);

	BENCH_RUN("cp_ghpe_enc (1)") {
		bn_rand_mod(m, pub);
		BENCH_ADD(cp_ghpe_enc(c, m, pub, 1));
	} BENCH_END;

	BENCH_RUN("cp_ghpe_dec (1)") {
		bn_rand_mod(m, pub);
		cp_ghpe_enc(m, c, pub, 1);
		BENCH_ADD(cp_ghpe_dec(c, m, pub, prv->n, 1));
	} BENCH_END;

	BENCH_ONE("cp_ghpe_gen", cp_ghpe_gen(pub, prv->n, RLC_BN_BITS / 4), 1);

	BENCH_RUN("cp_ghpe_enc (2)") {
		bn_rand(m, RLC_POS, 2 * bn_bits(pub) - 1);
		BENCH_ADD(cp_ghpe_enc(m, c, pub, 2));
	} BENCH_END;

	BENCH_RUN("cp_ghpe_dec (2)") {
		bn_rand(m, RLC_POS, 2 * bn_bits(pub) - 1);
		cp_ghpe_enc(m, c, pub, 2);
		BENCH_ADD(cp_ghpe_dec(c, m, pub, prv->n, 2));
	} BENCH_END;

	bn_free(c);
	bn_free(m);
	bn_free(pub);
	phpe_free(prv);
}

#endif

#if defined(WITH_EC)

static void ecdh(void) {
	bn_t d;
	ec_t p;
	uint8_t key[RLC_MD_LEN];

	bn_null(d);
	ec_null(p);

	bn_new(d);
	ec_new(p);

	BENCH_RUN("cp_ecdh_gen") {
		BENCH_ADD(cp_ecdh_gen(d, p));
	}
	BENCH_END;

	BENCH_RUN("cp_ecdh_key") {
		BENCH_ADD(cp_ecdh_key(key, RLC_MD_LEN, d, p));
	}
	BENCH_END;

	bn_free(d);
	ec_free(p);
}

static void ecmqv(void) {
	bn_t d1, d2;
	ec_t p1, p2;
	uint8_t key[RLC_MD_LEN];

	bn_null(d1);
	bn_null(d2);
	ec_null(p1);
	ec_null(p2);

	bn_new(d1);
	bn_new(d2);
	ec_new(p1);
	ec_new(p2);

	BENCH_RUN("cp_ecmqv_gen") {
		BENCH_ADD(cp_ecmqv_gen(d1, p1));
	}
	BENCH_END;

	cp_ecmqv_gen(d2, p2);

	BENCH_RUN("cp_ecmqv_key") {
		BENCH_ADD(cp_ecmqv_key(key, RLC_MD_LEN, d1, d2, p1, p1, p2));
	}
	BENCH_END;

	bn_free(d1);
	bn_free(d2);
	ec_free(p1);
	ec_free(p2);
}

static void ecies(void) {
	ec_t q, r;
	bn_t d;
	uint8_t in[10], out[16 + RLC_MD_LEN];
	int in_len, out_len;

	bn_null(d);
	ec_null(q);
	ec_null(r);

	ec_new(q);
	ec_new(r);
	bn_new(d);

	BENCH_RUN("cp_ecies_gen") {
		BENCH_ADD(cp_ecies_gen(d, q));
	}
	BENCH_END;

	BENCH_RUN("cp_ecies_enc") {
		in_len = sizeof(in);
		out_len = sizeof(out);
		rand_bytes(in, sizeof(in));
		BENCH_ADD(cp_ecies_enc(r, out, &out_len, in, in_len, q));
		cp_ecies_dec(out, &out_len, r, out, out_len, d);
	}
	BENCH_END;

	BENCH_RUN("cp_ecies_dec") {
		in_len = sizeof(in);
		out_len = sizeof(out);
		rand_bytes(in, sizeof(in));
		cp_ecies_enc(r, out, &out_len, in, in_len, q);
		BENCH_ADD(cp_ecies_dec(in, &in_len, r, out, out_len, d));
	}
	BENCH_END;

	ec_free(q);
	ec_free(r);
	bn_free(d);
}

static void ecdsa(void) {
	uint8_t msg[5] = { 0, 1, 2, 3, 4 }, h[RLC_MD_LEN];
	bn_t r, s, d;
	ec_t p;

	bn_null(r);
	bn_null(s);
	bn_null(d);
	ec_null(p);

	bn_new(r);
	bn_new(s);
	bn_new(d);
	ec_new(p);

	BENCH_RUN("cp_ecdsa_gen") {
		BENCH_ADD(cp_ecdsa_gen(d, p));
	}
	BENCH_END;

	BENCH_RUN("cp_ecdsa_sign (h = 0)") {
		BENCH_ADD(cp_ecdsa_sig(r, s, msg, 5, 0, d));
	}
	BENCH_END;

	BENCH_RUN("cp_ecdsa_sign (h = 1)") {
		md_map(h, msg, 5);
		BENCH_ADD(cp_ecdsa_sig(r, s, h, RLC_MD_LEN, 1, d));
	}
	BENCH_END;

	BENCH_RUN("cp_ecdsa_ver (h = 0)") {
		BENCH_ADD(cp_ecdsa_ver(r, s, msg, 5, 0, p));
	}
	BENCH_END;

	BENCH_RUN("cp_ecdsa_ver (h = 1)") {
		md_map(h, msg, 5);
		BENCH_ADD(cp_ecdsa_ver(r, s, h, RLC_MD_LEN, 1, p));
	}
	BENCH_END;

	bn_free(r);
	bn_free(s);
	bn_free(d);
	ec_free(p);
}

static void ecss(void) {
	uint8_t msg[5] = { 0, 1, 2, 3, 4 };
	bn_t r, s, d;
	ec_t p;

	bn_null(r);
	bn_null(s);
	bn_null(d);
	ec_null(p);

	bn_new(r);
	bn_new(s);
	bn_new(d);
	ec_new(p);

	BENCH_RUN("cp_ecss_gen") {
		BENCH_ADD(cp_ecss_gen(d, p));
	}
	BENCH_END;

	BENCH_RUN("cp_ecss_sign") {
		BENCH_ADD(cp_ecss_sig(r, s, msg, 5, d));
	}
	BENCH_END;

	BENCH_RUN("cp_ecss_ver") {
		BENCH_ADD(cp_ecss_ver(r, s, msg, 5, p));
	}
	BENCH_END;

	bn_free(r);
	bn_free(s);
	bn_free(d);
	ec_free(p);
}

static void vbnn(void) {
	uint8_t ida[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint8_t idb[] = { 5, 6, 7, 8, 9, 0, 1, 2, 3, 4 };
	bn_t msk, ska, skb;
	ec_t mpk, pka, pkb;

	uint8_t m[] = "Thrice the brinded cat hath mew'd.";

	ec_t r;
	bn_t z;
	bn_t h;

	bn_null(z);
	bn_null(h);
	bn_null(msk);
	bn_null(ska);
	bn_null(skb);
	ec_null(r);
	ec_null(mpk);
	bn_null(pka);
	bn_null(pkb);

	bn_new(z);
	bn_new(h);
	bn_new(msk);
	bn_new(ska);
	bn_new(skb);
	ec_new(r);
	ec_new(mpk);
	ec_new(pka);
	ec_new(pkb);

	BENCH_RUN("cp_vbnn_gen") {
		BENCH_ADD(cp_vbnn_gen(msk, mpk));
	}
	BENCH_END;

	BENCH_RUN("cp_vbnn_gen_prv") {
		BENCH_ADD(cp_vbnn_gen_prv(ska, pka, msk, ida, sizeof(ida)));
	}
	BENCH_END;

	cp_vbnn_gen_prv(skb, pkb, msk, idb, sizeof(idb));

	BENCH_RUN("cp_vbnn_sig") {
		BENCH_ADD(cp_vbnn_sig(r, z, h, ida, sizeof(ida), m, sizeof(m), ska, pka));
	}
	BENCH_END;

	BENCH_RUN("cp_vbnn_ver") {
		BENCH_ADD(cp_vbnn_ver(r, z, h, ida, sizeof(ida), m, sizeof(m), mpk));
	}
	BENCH_END;

	bn_free(z);
	bn_free(h);
	bn_free(msk);
	bn_free(ska);
	bn_free(skb);
	ec_free(r);
	ec_free(mpk);
	ec_free(pka);
	ec_free(pkb);
}

#define MAX_KEYS	RLC_MAX(BENCH, 16)
#define MIN_KEYS	RLC_MIN(BENCH, 16)

static void ers(void) {
	int size;
	ec_t pp, pk[MAX_KEYS + 1];
	bn_t sk[MAX_KEYS + 1], td;
	ers_t ring[MAX_KEYS + 1];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(td);
	ec_null(pp);

	bn_new(td);
	ec_new(pp);
	for (int i = 0; i <= MAX_KEYS; i++) {
		bn_null(sk[i]);
		bn_new(sk[i]);
		ec_null(pk[i]);
		ec_new(pk[i]);
		ers_null(ring[i]);
		ers_new(ring[i]);
		cp_ers_gen_key(sk[i], pk[i]);
	}

	cp_ers_gen(pp);

	BENCH_RUN("cp_ers_sig") {
		BENCH_ADD(cp_ers_sig(td, ring[0], m, 5, sk[0], pk[0], pp));
	} BENCH_END;

	BENCH_RUN("cp_ers_ver") {
		BENCH_ADD(cp_ers_ver(td, ring, 1, m, 5, pp));
	} BENCH_END;

	size = 1;
	BENCH_FEW("cp_ers_ext", cp_ers_ext(td, ring, &size, m, 5, pk[size], pp), 1);

	size = 1;
	cp_ers_sig(td, ring[0], m, 5, sk[0], pk[0], pp);
	for (int j = 1; j < MAX_KEYS && size < BENCH; j = j << 1) {
		for (int k = 0; k < j && size < BENCH; k++) {
			cp_ers_ext(td, ring, &size, m, 5, pk[size], pp);
		}
		cp_ers_ver(td, ring, size, m, 5, pp);
		util_print("(%2d exts) ", j);
		BENCH_FEW("cp_ers_ver", cp_ers_ver(td, ring, size, m, 5, pp), 1);
	}

	bn_free(td);
	ec_free(pp);
	for (int i = 0; i <= MAX_KEYS; i++) {
		bn_free(sk[i]);
		ec_free(pk[i]);
		ers_free(ring[i])
	}
}

static void etrs(void) {
	int size;
	ec_t pp, pk[MAX_KEYS + 1];
	bn_t sk[MAX_KEYS + 1], td[MAX_KEYS + 1], y[MAX_KEYS + 1];
	etrs_t ring[MAX_KEYS + 1];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	ec_null(pp);
	ec_new(pp);
	for (int i = 0; i <= MAX_KEYS; i++) {
		bn_null(td[i]);
		bn_new(td[i]);
		bn_null(y[i]);
		bn_new(y[i]);
		bn_null(sk[i]);
		bn_new(sk[i]);
		ec_null(pk[i]);
		ec_new(pk[i]);
		etrs_null(ring[i]);
		etrs_new(ring[i]);
		ec_curve_get_ord(sk[i]);
		bn_rand_mod(td[i], sk[i]);
		bn_rand_mod(y[i], sk[i]);
		cp_etrs_gen_key(sk[i], pk[i]);
	}

	cp_etrs_gen(pp);

	BENCH_FEW("cp_etrs_sig", cp_etrs_sig(td, y, MIN_KEYS, ring[0], m, 5, sk[0], pk[0], pp), 1);

	BENCH_FEW("cp_etrs_ver", cp_etrs_ver(1, td, y, MIN_KEYS, ring, 1, m, 5, pp), 1);

	size = 1;
	BENCH_FEW("cp_etrs_ext", (size = 1, cp_etrs_ext(td, y, MIN_KEYS, ring, &size, m, 5, pk[size], pp)), 1);

	size = 1;
	cp_etrs_sig(td, y, MIN_KEYS, ring[0], m, 5, sk[0], pk[0], pp);
	BENCH_FEW("cp_etrs_uni", cp_etrs_uni(1, td, y, MIN_KEYS, ring, &size, m, 5, sk[size], pk[size], pp), 1);

	size = 1;
	cp_etrs_sig(td, y, MIN_KEYS, ring[0], m, 5, sk[0], pk[0], pp);
	for (int j = 1; j < MIN_KEYS && size < MIN_KEYS; j = j << 1) {
		for (int k = 0; k < j && size < MIN_KEYS; k++) {
			cp_etrs_ext(td, y, MIN_KEYS, ring, &size, m, 5, pk[size], pp);
		}
		cp_etrs_ver(1, td+size-1, y+size-1, MIN_KEYS-size+1, ring, size, m, 5, pp);
		util_print("(%2d exts) ", j);
		BENCH_FEW("cp_etrs_ver", cp_etrs_ver(1, td+size-1, y+size-1, MIN_KEYS-size+1, ring, size, m, 5, pp), 1);
	}

	ec_free(pp);
	for (int i = 0; i <= MAX_KEYS; i++) {
		bn_free(td[i]);
		bn_free(y[i]);
		bn_free(sk[i]);
		ec_free(pk[i]);
		etrs_free(ring[i])
	}
}

#endif /* WITH_EC */

#if defined(WITH_PC)

static void pdpub(void) {
	bn_t r1, r2;
	g1_t p, u1, v1;
	g2_t q, u2, v2, w2;
	gt_t e, r, g[3];

	bn_null(r1);
	bn_null(r2);
	g1_null(p);
	g1_null(u1);
	g1_null(v1);
	g2_null(q);
	g2_null(u2);
	g2_null(v2);
	g2_null(w2);
	gt_null(e);
	gt_null(r);
	gt_null(g[0]);
	gt_null(g[1]);
	gt_null(g[2]);

	bn_new(r1);
	bn_new(r2);
	g1_new(p);
	g1_new(u1);
	g1_new(v1);
	g2_new(q);
	g2_new(u2);
	g2_new(v2);
	g2_new(w2);
	gt_new(e);
	gt_new(r);
	gt_new(g[0]);
	gt_new(g[1]);
	gt_new(g[2]);

	BENCH_RUN("cp_pdpub_gen") {
		BENCH_ADD(cp_pdpub_gen(r1, r2, u1, u2, v2, e));
	} BENCH_END;

	BENCH_RUN("cp_pdpub_ask") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_pdpub_ask(v1, w2, p, q, r1, r2, u1, u2, v2));
	} BENCH_END;

	BENCH_RUN("cp_pdpub_ans") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_pdpub_ans(g, p, q, v1, v2, w2));
	} BENCH_END;

	BENCH_RUN("cp_pdpub_ver") {
		g1_rand(p);
		g2_rand(q);
		pc_map(e, p, q);
		BENCH_ADD(cp_pdpub_ver(r, g, r1, e));
	} BENCH_END;

	BENCH_RUN("cp_lvpub_gen") {
		BENCH_ADD(cp_lvpub_gen(r2, u1, u2, v2, e));
	} BENCH_END;

	BENCH_RUN("cp_lvpub_ask") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_lvpub_ask(r1, v1, w2, p, q, r2, u1, u2, v2));
	} BENCH_END;

	BENCH_RUN("cp_lvpub_ans") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_lvpub_ans(g, p, q, v1, v2, w2));
	} BENCH_END;

	BENCH_RUN("cp_lvpub_ver") {
		g1_rand(p);
		g2_rand(q);
		pc_map(e, p, q);
		BENCH_ADD(cp_lvpub_ver(r, g, r1, e));
	} BENCH_END;

	bn_free(r1);
	bn_free(r2);
	g1_free(p);
	g1_free(u1);
	g1_free(v1);
	g2_free(q);
	g2_free(u2);
	g2_free(v2);
	g2_free(w2);
	gt_free(e);
	gt_free(r);
	gt_free(g[0]);
	gt_free(g[1]);
	gt_free(g[2]);
}

static void pdprv(void) {
	bn_t r1, r2[3];
	g1_t p, u1[2], v1[3];
	g2_t q, u2[2], v2[4], w2[4];
	gt_t e[2], r, g[4];

	bn_null(r1);
	g1_null(p);
	g2_null(q);
	gt_null(r);
	for (int i = 0; i < 2; i++) {
		g1_null(u1[i]);
		g2_null(u2[i]);
		gt_null(e[i]);
	}
	for (int i = 0; i < 3; i++) {
		g1_null(v1[i]);
		bn_null(r2[i]);
	}
	for (int i = 0; i < 4; i++) {
		g2_null(v2[i]);
		g2_null(w2[i]);
		gt_null(g[i]);
	}

	bn_new(r1);
	g1_new(p);
	g2_new(q);
	gt_new(r);
	for (int i = 0; i < 2; i++) {
		g1_new(u1[i]);
		g2_new(u2[i]);
		gt_new(e[i]);
	}
	for (int i = 0; i < 3; i++) {
		g1_new(v1[i]);
		bn_new(r2[i]);
	}
	for (int i = 0; i < 4; i++) {
		g2_new(v2[i]);
		g2_new(w2[i]);
		gt_new(g[i]);
	}

	BENCH_RUN("cp_pdprv_gen") {
		BENCH_ADD(cp_pdprv_gen(r1, r2, u1, u2, v2, e));
	} BENCH_END;

	BENCH_RUN("cp_pdprv_ask") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_pdprv_ask(v1, w2, p, q, r1, r2, u1, u2, v2));
	} BENCH_END;

	BENCH_RUN("cp_pdprv_ans") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_pdprv_ans(g, v1, w2));
	} BENCH_END;

	BENCH_RUN("cp_pdprv_ver") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_pdprv_ver(r, g, r1, e));
	} BENCH_END;

	BENCH_RUN("cp_lvprv_gen") {
		BENCH_ADD(cp_lvprv_gen(r1, r2, u1, u2, v2, e));
	} BENCH_END;

	BENCH_RUN("cp_lvprv_ask") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_lvprv_ask(v1, w2, p, q, r1, r2, u1, u2, v2));
	} BENCH_END;

	BENCH_RUN("cp_lvprv_ans") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_lvprv_ans(g, v1, w2));
	} BENCH_END;

	BENCH_RUN("cp_lvprv_ver") {
		g1_rand(p);
		g2_rand(q);
		BENCH_ADD(cp_lvprv_ver(r, g, r1, e));
	} BENCH_END;

	bn_free(r1);
	g1_free(p);
	g2_free(q);
	gt_free(r);
	for (int i = 0; i < 2; i++) {
		g1_free(u1[i]);
		g2_free(u2[i]);
		gt_free(e[i]);
	}
	for (int i = 0; i < 3; i++) {
		g1_free(v1[i]);
		bn_free(r2[i]);
	}
	for (int i = 0; i < 4; i++) {
		g2_free(v2[i]);
		g2_free(w2[i]);
		gt_free(g[i]);
	}
}

static void sokaka(void) {
	sokaka_t k;
	bn_t s;
	uint8_t key1[RLC_MD_LEN];
	char *id_a = "Alice";
	char *id_b = "Bob";

	sokaka_null(k);

	sokaka_new(k);
	bn_new(s);

	BENCH_RUN("cp_sokaka_gen") {
		BENCH_ADD(cp_sokaka_gen(s));
	}
	BENCH_END;

	BENCH_RUN("cp_sokaka_gen_prv") {
		BENCH_ADD(cp_sokaka_gen_prv(k, id_b, s));
	}
	BENCH_END;

	BENCH_RUN("cp_sokaka_key (g1)") {
		BENCH_ADD(cp_sokaka_key(key1, RLC_MD_LEN, id_b, k, id_a));
	}
	BENCH_END;

	if (pc_map_is_type3()) {
		cp_sokaka_gen_prv(k, id_a, s);

		BENCH_RUN("cp_sokaka_key (g2)") {
			BENCH_ADD(cp_sokaka_key(key1, RLC_MD_LEN, id_a, k, id_b));
		}
		BENCH_END;
	}

	sokaka_free(k);
	bn_free(s);
}

static void ibe(void) {
	bn_t s;
	g1_t pub;
	g2_t prv;
	uint8_t in[10], out[10 + 2 * RLC_FP_BYTES + 1];
	char *id = "Alice";
	int in_len, out_len;

	bn_null(s);
	g1_null(pub);
	g2_null(prv);

	bn_new(s);
	g1_new(pub);
	g2_new(prv);

	rand_bytes(in, sizeof(in));

	BENCH_RUN("cp_ibe_gen") {
		BENCH_ADD(cp_ibe_gen(s, pub));
	}
	BENCH_END;

	BENCH_RUN("cp_ibe_gen_prv") {
		BENCH_ADD(cp_ibe_gen_prv(prv, id, s));
	}
	BENCH_END;

	BENCH_RUN("cp_ibe_enc") {
		in_len = sizeof(in);
		out_len = in_len + 2 * RLC_FP_BYTES + 1;
		rand_bytes(in, sizeof(in));
		BENCH_ADD(cp_ibe_enc(out, &out_len, in, in_len, id, pub));
		cp_ibe_dec(out, &out_len, out, out_len, prv);
	}
	BENCH_END;

	BENCH_RUN("cp_ibe_dec") {
		in_len = sizeof(in);
		out_len = in_len + 2 * RLC_FP_BYTES + 1;
		rand_bytes(in, sizeof(in));
		cp_ibe_enc(out, &out_len, in, in_len, id, pub);
		BENCH_ADD(cp_ibe_dec(out, &out_len, out, out_len, prv));
	}
	BENCH_END;

	bn_free(s);
	g1_free(pub);
	g2_free(prv);
}

static void bgn(void) {
	g1_t c[2];
	g2_t d[2];
	gt_t e[4];
	bgn_t pub, prv;
	dig_t in;

	g1_null(c[0]);
	g1_null(c[1]);
	g2_null(d[0]);
	g2_null(d[1]);
	bgn_null(pub);
	bgn_null(prv);

	g1_new(c[0]);
	g1_new(c[1]);
	g2_new(d[0]);
	g2_new(d[1]);
	bgn_new(pub);
	bgn_new(prv);
	for (int i = 0; i < 4; i++) {
		gt_null(e[i]);
		gt_new(e[i]);
	}

	BENCH_RUN("cp_bgn_gen") {
		BENCH_ADD(cp_bgn_gen(pub, prv));
	} BENCH_END;

	in = 10;

	BENCH_RUN("cp_bgn_enc1") {
		BENCH_ADD(cp_bgn_enc1(c, in, pub));
		cp_bgn_dec1(&in, c, prv);
	} BENCH_END;

	BENCH_RUN("cp_bgn_dec1 (10)") {
		cp_bgn_enc1(c, in, pub);
		BENCH_ADD(cp_bgn_dec1(&in, c, prv));
	} BENCH_END;

	BENCH_RUN("cp_bgn_enc2") {
		BENCH_ADD(cp_bgn_enc2(d, in, pub));
		cp_bgn_dec2(&in, d, prv);
	} BENCH_END;

	BENCH_RUN("cp_bgn_dec2 (10)") {
		cp_bgn_enc2(d, in, pub);
		BENCH_ADD(cp_bgn_dec2(&in, d, prv));
	} BENCH_END;

	BENCH_RUN("cp_bgn_mul") {
		BENCH_ADD(cp_bgn_mul(e, c, d));
	} BENCH_END;

	BENCH_RUN("cp_bgn_dec (100)") {
		BENCH_ADD(cp_bgn_dec(&in, e, prv));
	} BENCH_END;

	BENCH_RUN("cp_bgn_add") {
		BENCH_ADD(cp_bgn_add(e, e, e));
	} BENCH_END;

	g1_free(c[0]);
	g1_free(c[1]);
	g2_free(d[0]);
	g2_free(d[1]);
	bgn_free(pub);
	bgn_free(prv);
	for (int i = 0; i < 4; i++) {
		gt_free(e[i]);
	}
}

static void bls(void) {
	uint8_t msg[5] = { 0, 1, 2, 3, 4 };
	g1_t s;
	g2_t p;
	bn_t d;

	g1_null(s);
	g2_null(p);
	bn_null(d);

	g1_new(s);
	g2_new(p);
	bn_new(d);

	BENCH_RUN("cp_bls_gen") {
		BENCH_ADD(cp_bls_gen(d, p));
	}
	BENCH_END;

	BENCH_RUN("cp_bls_sign") {
		BENCH_ADD(cp_bls_sig(s, msg, 5, d));
	}
	BENCH_END;

	BENCH_RUN("cp_bls_ver") {
		BENCH_ADD(cp_bls_ver(s, msg, 5, p));
	}
	BENCH_END;

	g1_free(s);
	bn_free(d);
	g2_free(p);
}

static void bbs(void) {
	uint8_t msg[5] = { 0, 1, 2, 3, 4 }, h[RLC_MD_LEN];
	g1_t s;
	g2_t p;
	gt_t z;
	bn_t d;

	g1_null(s);
	g2_null(p);
	gt_null(z);
	bn_null(d);

	g1_new(s);
	g2_new(p);
	gt_new(z);
	bn_new(d);

	BENCH_RUN("cp_bbs_gen") {
		BENCH_ADD(cp_bbs_gen(d, p, z));
	}
	BENCH_END;

	BENCH_RUN("cp_bbs_sign (h = 0)") {
		BENCH_ADD(cp_bbs_sig(s, msg, 5, 0, d));
	}
	BENCH_END;

	BENCH_RUN("cp_bbs_sign (h = 1)") {
		md_map(h, msg, 5);
		BENCH_ADD(cp_bbs_sig(s, h, RLC_MD_LEN, 1, d));
	}
	BENCH_END;

	BENCH_RUN("cp_bbs_ver (h = 0)") {
		BENCH_ADD(cp_bbs_ver(s, msg, 5, 0, p, z));
	}
	BENCH_END;

	BENCH_RUN("cp_bbs_ver (h = 1)") {
		md_map(h, msg, 5);
		BENCH_ADD(cp_bbs_ver(s, h, RLC_MD_LEN, 1, p, z));
	}
	BENCH_END;

	g1_free(s);
	bn_free(d);
	g2_free(p);
}

static int cls(void) {
	int i, code = RLC_ERR;
	bn_t r, t, u, v, _v[4];
	g1_t a, A, b, B, c, _A[4], _B[4];
	g2_t x, y, z, _z[4];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };
	uint8_t *msgs[5] = {m, m, m, m, m};
	int lens[5] = {sizeof(m), sizeof(m), sizeof(m), sizeof(m), sizeof(m)};

	bn_null(r);
	bn_null(t);
	bn_null(u);
	bn_null(v);
	g1_null(a);
	g1_null(A);
	g1_null(b);
	g1_null(B);
	g1_null(c);
	g2_null(x);
	g2_null(y);
	g2_null(z);
	for (i = 0; i < 4; i++) {
		bn_null(_v[i]);
		g1_null(_A[i]);
		g1_null(_B[i]);
		g2_null(_z[i]);
	}

	bn_new(r);
	bn_new(t);
	bn_new(u);
	bn_new(v);
	g1_new(a);
	g1_new(A);
	g1_new(b);
	g1_new(B);
	g1_new(c);
	g2_new(x);
	g2_new(y);
	g2_new(z);
	for (i = 0; i < 4; i++) {
		bn_new(_v[i]);
		g1_new(_A[i]);
		g1_new(_B[i]);
		g2_new(_z[i]);
	}

	BENCH_RUN("cp_cls_gen") {
		BENCH_ADD(cp_cls_gen(u, v, x, y));
	} BENCH_END;

	BENCH_RUN("cp_cls_sig") {
		BENCH_ADD(cp_cls_sig(a, b, c, m, sizeof(m), u, v));
	} BENCH_END;

	BENCH_RUN("cp_cls_ver") {
		BENCH_ADD(cp_cls_ver(a, b, c, m, sizeof(m), x, y));
	} BENCH_END;

	BENCH_RUN("cp_cli_gen") {
		BENCH_ADD(cp_cli_gen(t, u, v, x, y, z));
	} BENCH_END;

	bn_rand(r, RLC_POS, 2 * pc_param_level());
	BENCH_RUN("cp_cli_sig") {
		BENCH_ADD(cp_cli_sig(a, A, b, B, c, m, sizeof(m), r, t, u, v));
	} BENCH_END;

	BENCH_RUN("cp_cli_ver") {
		BENCH_ADD(cp_cli_ver(a, A, b, B, c, m, sizeof(m), r, x, y, z));
	} BENCH_END;

	BENCH_RUN("cp_clb_gen (5)") {
		BENCH_ADD(cp_clb_gen(t, u, _v, x, y, _z, 5));
	} BENCH_END;

	BENCH_RUN("cp_clb_sig (5)") {
		BENCH_ADD(cp_clb_sig(a, _A, b, _B, c, msgs, lens, t, u, _v, 5));
	} BENCH_END;

	BENCH_RUN("cp_clb_ver (5)") {
		BENCH_ADD(cp_clb_ver(a, _A, b, _B, c, msgs, lens, x, y, _z, 5));
	} BENCH_END;

	bn_free(r);
	bn_free(t);
	bn_free(u);
	bn_free(v);
	g1_free(a);
	g1_free(A);
	g1_free(b);
	g1_free(B);
	g1_free(c);
	g2_free(x);
	g2_free(y);
	g2_free(z);
	for (i = 0; i < 4; i++) {
		bn_free(_v[i]);
		g1_free(_A[i]);
		g1_free(_B[i]);
		g2_free(_z[i]);
	}
	return code;
}

static void pss(void) {
	bn_t ms[10], n, u, v, _v[10];
	g1_t a, b;
	g2_t g, x, y, _y[10];

	bn_null(n);
	bn_null(u);
	bn_null(v);
	g1_null(a);
	g1_null(b);
	g2_null(g);
	g2_null(x);
	g2_null(y);
	bn_new(n);
	bn_new(u);
	bn_new(v);
	g1_new(a);
	g1_new(b);
	g2_new(g);
	g2_new(x);
	g2_new(y);

	g1_get_ord(n);
	for (int i = 0; i < 10; i++) {
		bn_null(ms[i]);
		bn_null(_v[i]);
		g2_null(_y[i]);
		bn_new(ms[i]);
		bn_rand_mod(ms[i], n);
		bn_new(_v[i]);
		g2_new(_y[i]);
	}

	BENCH_RUN("cp_pss_gen") {
		BENCH_ADD(cp_pss_gen(u, v, g, x, y));
	} BENCH_END;

	BENCH_RUN("cp_pss_sig") {
		BENCH_ADD(cp_pss_sig(a, b, ms[0], u, v));
	} BENCH_END;

	BENCH_RUN("cp_pss_ver") {
		BENCH_ADD(cp_pss_ver(a, b, ms[0], g, x, y));
	} BENCH_END;

	BENCH_RUN("cp_psb_gen (10)") {
		BENCH_ADD(cp_psb_gen(u, _v, g, x, _y, 10));
	} BENCH_END;

	BENCH_RUN("cp_psb_sig (10)") {
		BENCH_ADD(cp_psb_sig(a, b, ms, u, _v, 10));
	} BENCH_END;

	BENCH_RUN("cp_psb_ver (10)") {
		BENCH_ADD(cp_psb_ver(a, b, ms, g, x, _y, 10));
	} BENCH_END;

	bn_free(u);
	bn_free(v);
	g1_free(a);
	g1_free(b);
	g2_free(g);
	g2_free(x);
	g2_free(y);
	for (int i = 0; i < 10; i++) {
		bn_free(ms[i]);
		bn_free(_v[i]);
		g1_free(_y[i]);
	}
}

#ifdef WITH_MPC

static void mpss(void) {
	bn_t m[2], n, u[2], v[2], ms[10][2], _v[10][2];
	g1_t g, s[2];
	g2_t h, x[2], y[2], _y[10][2];
	gt_t r[2];
	mt_t tri[3][2];
	pt_t t[2];

	bn_null(n);
	g1_null(g);
	g2_null(h);

	bn_new(n);
	g1_new(g);
	g2_new(h);
	for (int i = 0; i < 2; i++) {
		bn_null(m[i]);
		bn_null(u[i]);
		bn_null(v[i]);
		g1_null(s[i]);
		g2_null(x[i]);
		g2_null(y[i]);
		gt_null(r[i]);
		mt_null(tri[0][i]);
		mt_null(tri[1][i]);
		mt_null(tri[2][i]);
		pt_null(t[i]);
		bn_new(m[i]);
		bn_new(u[i]);
		bn_new(v[i]);
		g1_new(s[i]);
		g2_new(x[i]);
		g2_new(y[i]);
		gt_new(r[i]);
		mt_new(tri[0][i]);
		mt_new(tri[1][i]);
		mt_new(tri[2][i]);
		pt_new(t[i]);

		g1_get_ord(n);
		for (int j = 0; j < 10; j++) {
			bn_null(ms[j][i]);
			bn_null(_v[j][i]);
			g2_null(_y[j][i]);
			bn_new(ms[j][i]);
			bn_rand_mod(ms[j][i], n);
			bn_new(_v[j][i]);
			g2_new(_y[j][i]);
		}
	}

	pc_map_tri(t);
	mt_gen(tri[0], n);
	mt_gen(tri[1], n);
	mt_gen(tri[2], n);

	bn_rand_mod(m[0], n);
	bn_rand_mod(m[1], n);
	bn_sub(m[0], m[1], m[0]);
	if (bn_sign(m[0]) == RLC_NEG) {
		bn_add(m[0], m[0], n);
	}
	gt_exp_gen(r[0], tri[2][0]->c);
	gt_exp_gen(r[1], tri[2][1]->c);
	tri[2][0]->bt = &r[0];
	tri[2][1]->bt = &r[1];
	tri[2][0]->ct = &r[0];
	tri[2][1]->ct = &r[1];

	BENCH_RUN("cp_mpss_gen") {
		BENCH_ADD(cp_mpss_gen(u, v, h, x, y));
	} BENCH_END;

	BENCH_RUN("cp_mpss_bct") {
		BENCH_ADD(cp_mpss_bct(x, y));
	} BENCH_END;

	BENCH_RUN("cp_mpss_sig") {
		BENCH_ADD(cp_mpss_sig(g, s, m, u, v, tri[0], tri[1]));
	} BENCH_DIV(2);

	BENCH_RUN("cp_mpss_ver") {
		BENCH_ADD(cp_mpss_ver(r[0], g, s, m, h, x[0], y[0], tri[2], t));
	} BENCH_DIV(2);

	g1_get_ord(n);
	pc_map_tri(t);
	mt_gen(tri[0], n);
	mt_gen(tri[1], n);
	mt_gen(tri[2], n);

	BENCH_RUN("cp_mpsb_gen (10)") {
		BENCH_ADD(cp_mpsb_gen(u, _v, h, x, _y, 10));
	} BENCH_END;

	BENCH_RUN("cp_mpsb_bct (10)") {
		BENCH_ADD(cp_mpsb_bct(x, _y, 10));
	} BENCH_END;

	BENCH_RUN("cp_mpsb_sig (10)") {
		BENCH_ADD(cp_mpsb_sig(g, s, ms, u, _v, tri[0], tri[1], 10));
	} BENCH_DIV(2);

	BENCH_RUN("cp_mpsb_ver (10)") {
		BENCH_ADD(cp_mpsb_ver(r[1], g, s, ms, h, x[0], _y, NULL, tri[2], t, 10));
	} BENCH_DIV(2);

	BENCH_RUN("cp_mpsb_ver (10,sk)") {
		BENCH_ADD(cp_mpsb_ver(r[1], g, s, ms, h, x[0], _y, _v, tri[2], t, 10));
	} BENCH_DIV(2);

  	bn_free(n);
	g1_free(g);
	g2_free(h);
	for (int i = 0; i < 2; i++) {
		bn_free(m[i]);
		bn_free(u[i]);
		bn_free(v[i]);
		g1_free(s[i]);
		g2_free(x[i]);
		g2_free(y[i]);
		gt_null(r[i]);
		mt_free(tri[0][i]);
		mt_free(tri[1][i]);
		mt_free(tri[2][i]);
		pt_free(t[i]);
		for (int j = 0; j < 10; j++) {
			bn_free(ms[j][i]);
			bn_free(_v[j][i]);
			g2_free(_y[j][i]);
		}
	}
}

#endif

static void zss(void) {
	uint8_t msg[5] = { 0, 1, 2, 3, 4 }, h[RLC_MD_LEN];
	g1_t p;
	g2_t s;
	gt_t z;
	bn_t d;

	bn_null(d);
	g1_null(p);
	g2_null(s);
	gt_null(z);

	g1_new(p);
	g2_new(s);
	gt_new(z);
	bn_new(d);

	BENCH_RUN("cp_zss_gen") {
		BENCH_ADD(cp_zss_gen(d, p, z));
	}
	BENCH_END;

	BENCH_RUN("cp_zss_sign (h = 0)") {
		BENCH_ADD(cp_zss_sig(s, msg, 5, 0, d));
	}
	BENCH_END;

	BENCH_RUN("cp_zss_sign (h = 1)") {
		md_map(h, msg, 5);
		BENCH_ADD(cp_zss_sig(s, h, RLC_MD_LEN, 1, d));
	}
	BENCH_END;

	BENCH_RUN("cp_zss_ver (h = 0)") {
		BENCH_ADD(cp_zss_ver(s, msg, 5, 0, p, z));
	}
	BENCH_END;

	BENCH_RUN("cp_zss_ver (h = 1)") {
		md_map(h, msg, 5);
		BENCH_ADD(cp_zss_ver(s, h, RLC_MD_LEN, 1, p, z));
	}
	BENCH_END;

	bn_free(d);
	g1_free(p);
	g2_free(s);
}

/* Size of the dataset for benchmarking. */
#define S	10			/* Number of signers. */
#define L	16			/* Number of labels, must be <= RLC_TERMS. */
#define K	RLC_MD_LEN	/* Size of PRF key. */
//#define BENCH_LHS		/* Uncomment for fine-grained benchmarking. */

static void lhs(void) {
	uint8_t k[S][K];
	bn_t m, n, msg[L], sk[S], d[S], x[S][L];
	g1_t _r, h, as[S], cs[S], sig[S];
	g1_t a[S][L], c[S][L], r[S][L];
	g2_t _s, s[S][L], pk[S], y[S], z[S];
	gt_t *hs[S], vk;
	char *data = "id";
	char *id[S] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
	dig_t ft[S];
	dig_t *f[S];
	int flen[S];

	bn_null(m);
	bn_null(n);
	g1_null(h);
	g1_null(_r);
	g2_null(_s);
	gt_null(vk);

	bn_new(m);
	bn_new(n);
	g1_new(h);
	g1_new(_r);
	g2_new(_s);
	gt_new(vk);

	pc_get_ord(n);
	for (int i = 0; i < L; i++) {
		bn_null(msg[i]);
		bn_new(msg[i]);
		bn_rand_mod(msg[i], n);
	}
	for (int i = 0; i < S; i++) {
		hs[i] = RLC_ALLOCA(gt_t, RLC_TERMS);
		for (int j = 0; j < RLC_TERMS; j++) {
			gt_null(hs[i][j]);
			gt_new(hs[i][j]);
		}
		for (int j = 0; j < L; j++) {
			bn_null(x[i][j]);
			g1_null(a[i][j]);
			g1_null(c[i][j]);
			g1_null(r[i][j]);
			g2_null(s[i][j]);
			bn_new(x[i][j]);
			g1_new(a[i][j]);
			g1_new(c[i][j]);
			g1_new(r[i][j]);
			g2_new(s[i][j]);
		}
		bn_null(sk[i]);
		bn_null(d[i]);
		g1_null(sig[i]);
		g1_null(as[i]);
		g1_null(cs[i]);
		g2_null(y[i]);
		g2_null(z[i]);
		g2_null(pk[i]);

		bn_new(sk[i]);
		bn_new(d[i]);
		g1_new(sig[i]);
		g1_new(as[i]);
		g1_new(cs[i]);
		g2_new(y[i]);
		g2_new(z[i]);
		g2_new(pk[i]);
	}

	/* Define linear function. */
	for (int i = 0; i < S; i++) {
		f[i] = RLC_ALLOCA(dig_t, RLC_TERMS);
		for (int j = 0; j < RLC_TERMS; j++) {
			uint32_t t;
			rand_bytes((uint8_t *)&t, sizeof(uint32_t));
			f[i][j] = t;
		}
		flen[i] = L;
	}

	/* Initialize scheme for messages of single components. */
	cp_cmlhs_init(h);

	BENCH_ONE("cp_cmlhs_gen",
		for (int j = 0; j < S; j++) {
			BENCH_ADD(cp_cmlhs_gen(x[j], hs[j], L, k[j], K, sk[j], pk[j], d[j], y[j]));
		},
	S);

	int label[L];

	BENCH_FEW("cp_cmlhs_sig",
		/* Compute all signatures. */
		for (int j = 0; j < S; j++) {
			for (int l = 0; l < L; l++) {
				label[l] = l;
				bn_mod(msg[l], msg[l], n);
				BENCH_ADD(cp_cmlhs_sig(sig[j], z[j], a[j][l], c[j][l], r[j][l],
					s[j][l], msg[l], data, label[l], x[j][l], h, k[j], K, d[j], sk[j]));
			}
		},
	S * L);

	BENCH_RUN("cp_cmlhs_fun") {
		for (int j = 0; j < S; j++) {
			BENCH_ADD(cp_cmlhs_fun(as[j], cs[j], a[j], c[j], f[j], L));
		}
	} BENCH_DIV(S);

	BENCH_RUN("cp_cmlhs_evl") {
		cp_cmlhs_evl(_r, _s, r[0], s[0], f[0], L);
		for (int j = 1; j < S; j++) {
			BENCH_ADD(cp_cmlhs_evl(r[0][0], s[0][0], r[j], s[j], f[j], L));
			g1_add(_r, _r, r[0][0]);
			g2_add(_s, _s, s[0][0]);
		}
		g1_norm(_r, _r);
		g2_norm(_s, _s);
	} BENCH_DIV(S);

	bn_zero(m);
	for (int j = 0; j < L; j++) {
		dig_t sum = 0;
		for (int l = 0; l < S; l++) {
			sum += f[l][j];
		}
		bn_mul_dig(msg[j], msg[j], sum);
		bn_add(m, m, msg[j]);
		bn_mod(m, m, n);
	}

	BENCH_RUN("cp_cmlhs_ver") {
		BENCH_ADD(cp_cmlhs_ver(_r, _s, sig, z, as, cs, m, data, h, label, hs,
			f, flen, y, pk, S));
	} BENCH_DIV(S);

	BENCH_RUN("cp_cmlhs_off") {
		BENCH_ADD(cp_cmlhs_off(vk, h, label, hs, f, flen, y, pk, S));
	} BENCH_DIV(S);

	BENCH_RUN("cp_cmlhs_onv") {
		BENCH_ADD(cp_cmlhs_onv(_r, _s, sig, z, as, cs, m, data, h, vk, y,
			pk, S));
	} BENCH_DIV(S);

#ifdef BENCH_LHS
	for (int t = 1; t <= S; t++) {
		util_print("(%2d ids) ", t);
		BENCH_RUN("cp_cmlhs_ver") {
			BENCH_ADD(cp_cmlhs_ver(_r, _s, sig, z, as, cs, m, data, h, label,
				hs, f, flen, y, pk, t));
		} BENCH_END;

		util_print("(%2d ids) ", t);
		BENCH_RUN("cp_cmlhs_off") {
			BENCH_ADD(cp_cmlhs_off(vk, h, label, hs, f, flen, y, pk, t));
		} BENCH_END;

		util_print("(%2d ids) ", t);
		BENCH_RUN("cp_cmlhs_onv") {
			BENCH_ADD(cp_cmlhs_onv(_r, _s, sig, z, as, cs, m, data, h, vk, y,
				pk, t));
		} BENCH_END;
	}

	for (int t = 1; t <= L; t++) {
		util_print("(%2d lbs) ", t);
		for (int u = 0; u < S; u++) {
			flen[u] = t;
		}
		BENCH_RUN("cp_cmlhs_ver") {
			BENCH_ADD(cp_cmlhs_ver(_r, _s, sig, z, as, cs, m, data, h, label,
				hs,	f, flen, y, pk, S));
		} BENCH_END;

		util_print("(%2d lbs) ", t);
		BENCH_RUN("cp_cmlhs_off") {
			BENCH_ADD(cp_cmlhs_off(vk, h, label, hs, f, flen, y, pk, t));
		} BENCH_END;

		util_print("(%2d lbs) ", t);
		BENCH_RUN("cp_cmlhs_onv") {
			BENCH_ADD(cp_cmlhs_onv(_r, _s, sig, z, as, cs, m, data, h, vk, y,
				pk, t));
		} BENCH_END;
	}
#endif  /* BENCH_LHS */

	char *ls[L];

	BENCH_RUN("cp_mklhs_gen") {
		for (int j = 0; j < S; j++) {
			BENCH_ADD(cp_mklhs_gen(sk[j], pk[j]));
		}
	} BENCH_DIV(S);

	BENCH_RUN("cp_mklhs_sig") {
		for (int j = 0; j < S; j++) {
			for (int l = 0; l < L; l++) {
				ls[l] = "l";
				bn_mod(msg[l], msg[l], n);
				BENCH_ADD(cp_mklhs_sig(a[j][l], msg[l], data,
					id[j], ls[l], sk[j]));
			}
		}
	} BENCH_DIV(S * L);

	BENCH_RUN("cp_mklhs_fun") {
		for (int j = 0; j < S; j++) {
			bn_zero(d[j]);
			BENCH_ADD(cp_mklhs_fun(d[j], msg, f[j], L));
		}
	}
	BENCH_DIV(S);

	BENCH_RUN("cp_mklhs_evl") {
		g1_set_infty(_r);
		for (int j = 0; j < S; j++) {
			BENCH_ADD(cp_mklhs_evl(r[0][j], a[j], f[j], L));
			g1_add(_r, _r, r[0][j]);
		}
		g1_norm(_r, _r);
	}
	BENCH_DIV(S);

	bn_zero(m);
	for (int j = 0; j < L; j++) {
		dig_t sum = 0;
		for (int l = 0; l < S; l++) {
			sum += f[l][j];
		}
		bn_mul_dig(msg[j], msg[j], sum);
		bn_add(m, m, msg[j]);
		bn_mod(m, m, n);
	}

	BENCH_RUN("cp_mklhs_ver") {
		BENCH_ADD(cp_mklhs_ver(_r, m, d, data, id, ls, f, flen, pk, S));
	} BENCH_DIV(S);

	BENCH_RUN("cp_mklhs_off") {
		BENCH_ADD(cp_mklhs_off(cs, ft, id, ls, f, flen, S));
	} BENCH_DIV(S);

	BENCH_RUN("cp_mklhs_onv") {
		BENCH_ADD(cp_mklhs_onv(_r, m, d, data, id, cs, ft, pk, S));
	} BENCH_DIV(S);

#ifdef BENCH_LHS
	for (int t = 1; t <= S; t++) {
		util_print("(%2d ids) ", t);
		BENCH_RUN("cp_mklhs_ver") {
			BENCH_ADD(cp_mklhs_ver(_r, m, d, data, id, ls, f, flen, pk, t));
		} BENCH_END;

		util_print("(%2d ids) ", t);
		BENCH_RUN("cp_mklhs_off") {
			BENCH_ADD(cp_mklhs_off(cs, ft, id, ls, f, flen, t));
		} BENCH_END;

		util_print("(%2d ids) ", t);
		BENCH_RUN("cp_mklhs_onv") {
			BENCH_ADD(cp_mklhs_onv(_r, m, d, data, id, cs, ft, pk, t));
		} BENCH_END;
	}

	for (int t = 1; t <= L; t++) {
		util_print("(%2d lbs) ", t);
		for (int u = 0; u < S; u++) {
			flen[u] = t;
		}
		BENCH_RUN("cp_mklhs_ver") {
			BENCH_ADD(cp_mklhs_ver(_r, m, d, data, id, ls, f, flen, pk, S));
		} BENCH_END;

		util_print("(%2d lbs) ", t);
		BENCH_RUN("cp_mklhs_off") {
			BENCH_ADD(cp_mklhs_off(cs, ft, id, ls, f, flen, S));
		} BENCH_END;

		util_print("(%2d lbs) ", t);
		BENCH_RUN("cp_mklhs_onv") {
			BENCH_ADD(cp_mklhs_onv(_r, m, d, data, id, cs, ft, pk, S));
		} BENCH_END;
	}
#endif /* BENCH_LHS */

	bn_free(n);
	bn_free(m);
	g1_free(h);
	g1_free(_r);
	g2_free(_s);
	gt_free(vk);

	for (int i = 0; i < L; i++) {
		bn_free(msg[i]);
	}
	for (int i = 0; i < S; i++) {
		RLC_FREE(f[i]);
		for (int j = 0; j < RLC_TERMS; j++) {
			gt_free(hs[i][j]);
		}
		RLC_FREE(hs[i]);
		for (int j = 0; j < L; j++) {
			bn_free(x[i][j]);
			g1_free(a[i][j]);
			g1_free(c[i][j]);
			g1_free(r[i][j]);
			g2_free(s[i][j]);
		}
		bn_free(sk[i]);
		bn_free(d[i]);
		g1_free(sig[i]);
		g1_free(as[i]);
		g1_free(cs[i]);
		g2_free(y[i]);
		g2_free(z[i]);
		g2_free(pk[i]);
	}
}

#endif /* WITH_PC */

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	util_banner("Benchmarks for the CP module:", 0);

#if defined(WITH_BN)
	util_banner("Protocols based on integer factorization:\n", 0);
	rsa();
	rabin();
	benaloh();
	paillier();
#endif

#if defined(WITH_EC)
	if (ec_param_set_any() == RLC_OK) {
		util_banner("Protocols based on elliptic curves:\n", 0);
		ecdh();
		ecmqv();
		ecies();
		ecdsa();
		ecss();
		vbnn();
		ers();
		etrs();
	}
#endif

#if defined(WITH_PC)
	if (pc_param_set_any() == RLC_OK) {
		util_banner("Protocols based on pairings:\n", 0);
		pdpub();
		pdprv();
		sokaka();
		ibe();
		bgn();
		bls();
		bbs();
		cls();
		pss();
#if defined(WITH_MPC)
		mpss();
#endif
		zss();
		lhs();
	}
#endif

	core_clean();
	return 0;
}
