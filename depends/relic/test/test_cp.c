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
 * Tests for implementation of cryptographic protocols.
 *
 * @version $Id$
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int rsa(void) {
	int code = RLC_ERR;
	rsa_t pub, prv;
	uint8_t in[10], out[RLC_BN_BITS / 8 + 1], h[RLC_MD_LEN];
	int il, ol;
	int result;

	rsa_null(pub);
	rsa_null(prv);

	RLC_TRY {
		rsa_new(pub);
		rsa_new(prv);

		result = cp_rsa_gen(pub, prv, RLC_BN_BITS);

		TEST_CASE("rsa encryption/decryption is correct") {
			TEST_ASSERT(result == RLC_OK, end);
			il = 10;
			ol = RLC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_enc(out, &ol, in, il, pub) == RLC_OK, end);
			TEST_ASSERT(cp_rsa_dec(out, &ol, out, ol, prv) == RLC_OK, end);
			TEST_ASSERT(memcmp(in, out, ol) == 0, end);
		} TEST_END;

		result = cp_rsa_gen(pub, prv, RLC_BN_BITS);

		TEST_CASE("rsa signature/verification is correct") {
			TEST_ASSERT(result == RLC_OK, end);
			il = 10;
			ol = RLC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_sig(out, &ol, in, il, 0, prv) == RLC_OK, end);
			TEST_ASSERT(cp_rsa_ver(out, ol, in, il, 0, pub) == 1, end);
			md_map(h, in, il);
			TEST_ASSERT(cp_rsa_sig(out, &ol, h, RLC_MD_LEN, 1, prv) == RLC_OK, end);
			TEST_ASSERT(cp_rsa_ver(out, ol, h, RLC_MD_LEN, 1, pub) == 1, end);
		} TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	rsa_free(pub);
	rsa_free(prv);
	return code;
}

static int rabin(void) {
	int code = RLC_ERR;
	rabin_t pub, prv;
	uint8_t in[10];
	uint8_t out[RLC_BN_BITS / 8 + 1];
	int in_len, out_len;
	int result;

	rabin_null(pub);
	rabin_null(prv);

	RLC_TRY {
		rabin_new(pub);
		rabin_new(prv);

		result = cp_rabin_gen(pub, prv, RLC_BN_BITS);

		TEST_CASE("rabin encryption/decryption is correct") {
			TEST_ASSERT(result == RLC_OK, end);
			in_len = 10;
			out_len = RLC_BN_BITS / 8 + 1;
			rand_bytes(in, in_len);
			TEST_ASSERT(cp_rabin_enc(out, &out_len, in, in_len, pub) == RLC_OK,
					end);
			TEST_ASSERT(cp_rabin_dec(out, &out_len, out, out_len,
							prv) == RLC_OK, end);
			TEST_ASSERT(memcmp(in, out, out_len) == 0, end);
		} TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	rabin_free(pub);
	rabin_free(prv);
	return code;
}

static int benaloh(void) {
	int code = RLC_ERR;
	bdpe_t pub, prv;
	bn_t a, b;
	dig_t in, out;
	uint8_t buf[RLC_BN_BITS / 8 + 1];
	int len;
	int result;

	bn_null(a);
	bn_null(b);
	bdpe_null(pub);
	bdpe_null(prv);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bdpe_new(pub);
		bdpe_new(prv);

		result = cp_bdpe_gen(pub, prv, bn_get_prime(47), RLC_BN_BITS);

		TEST_CASE("benaloh encryption/decryption is correct") {
			TEST_ASSERT(result == RLC_OK, end);
			len = RLC_BN_BITS / 8 + 1;
			rand_bytes(buf, 1);
			in = buf[0] % bn_get_prime(47);
			TEST_ASSERT(cp_bdpe_enc(buf, &len, in, pub) == RLC_OK, end);
			TEST_ASSERT(cp_bdpe_dec(&out, buf, len, prv) == RLC_OK, end);
			TEST_ASSERT(in == out, end);
		} TEST_END;

		TEST_CASE("benaloh encryption/decryption is homomorphic") {
			TEST_ASSERT(result == RLC_OK, end);
			len = RLC_BN_BITS / 8 + 1;
			rand_bytes(buf, 1);
			in = buf[0] % bn_get_prime(47);
			TEST_ASSERT(cp_bdpe_enc(buf, &len, in, pub) == RLC_OK, end);
			bn_read_bin(a, buf, len);
			rand_bytes(buf, 1);
			out = (buf[0] % bn_get_prime(47));
			in = (in + out) % bn_get_prime(47);
			TEST_ASSERT(cp_bdpe_enc(buf, &len, out, pub) == RLC_OK, end);
			bn_read_bin(b, buf, len);
			bn_mul(a, a, b);
			bn_mod(a, a, pub->n);
			len = bn_size_bin(pub->n);
			bn_write_bin(buf, len, a);
			TEST_ASSERT(cp_bdpe_dec(&out, buf, len, prv) == RLC_OK, end);
			TEST_ASSERT(in == out, end);
		} TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(a);
	bn_free(b);
	bdpe_free(pub);
	bdpe_free(prv);
	return code;
}

static int paillier(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, s, pub;
	phpe_t prv;
	int result;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(s);
	bn_null(pub);
	phpe_null(prv);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(s);
		bn_new(pub);
		phpe_new(prv);

		result = cp_phpe_gen(pub, prv, RLC_BN_BITS / 2);

		TEST_CASE("paillier encryption/decryption is correct") {
			TEST_ASSERT(result == RLC_OK, end);
			bn_rand_mod(a, pub);
			TEST_ASSERT(cp_phpe_enc(c, a, pub) == RLC_OK, end);
			TEST_ASSERT(cp_phpe_dec(b, c, prv) == RLC_OK, end);
			TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
		}
		TEST_END;

		TEST_CASE("paillier encryption/decryption is homomorphic") {
			TEST_ASSERT(result == RLC_OK, end);
			bn_rand_mod(a, pub);
			bn_rand_mod(b, pub);
			TEST_ASSERT(cp_phpe_enc(c, a, pub) == RLC_OK, end);
			TEST_ASSERT(cp_phpe_enc(d, b, pub) == RLC_OK, end);
			bn_mul(c, c, d);
			bn_sqr(d, pub);
			bn_mod(c, c, d);
			TEST_ASSERT(cp_phpe_dec(d, c, prv) == RLC_OK, end);
			bn_add(a, a, b);
			bn_mod(a, a, pub);
			TEST_ASSERT(bn_cmp(a, d) == RLC_EQ, end);
		}
		TEST_END;

		for (int k = 1; k <= 2; k++) {
			result = cp_ghpe_gen(pub, s, RLC_BN_BITS / (2 * k));
			util_print("(s = %d) ", k);
			TEST_CASE("general paillier encryption/decryption is correct") {
				TEST_ASSERT(result == RLC_OK, end);
				bn_rand(a, RLC_POS, k * (bn_bits(pub) - 1));
				TEST_ASSERT(cp_ghpe_enc(c, a, pub, k) == RLC_OK, end);
				TEST_ASSERT(cp_ghpe_dec(b, c, pub, s, k) == RLC_OK, end);
				TEST_ASSERT(bn_cmp(a, b) == RLC_EQ, end);
			}  TEST_END;

			util_print("(s = %d) ", k);
			TEST_CASE("general paillier encryption/decryption is homomorphic") {
				TEST_ASSERT(result == RLC_OK, end);
				bn_rand(a, RLC_POS, k * (bn_bits(pub) - 1));
				bn_rand(b, RLC_POS, k * (bn_bits(pub) - 1));
				TEST_ASSERT(cp_ghpe_enc(c, a, pub, k) == RLC_OK, end);
				TEST_ASSERT(cp_ghpe_enc(d, b, pub, k) == RLC_OK, end);
				bn_mul(c, c, d);
				bn_sqr(d, pub);
				if (k == 2) {
					bn_mul(d, d, pub);
				}
				bn_mod(c, c, d);
				TEST_ASSERT(cp_ghpe_dec(c, c, pub, s, k) == RLC_OK, end);
				bn_add(a, a, b);
				bn_copy(d, pub);
				if (k == 2) {
					bn_mul(d, d, pub);
				}
				bn_mod(a, a, d);
				TEST_ASSERT(bn_cmp(a, c) == RLC_EQ, end);
			}
			TEST_END;
		}
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(s);
	bn_free(pub);
	phpe_free(prv);
	return code;
}

#if defined(WITH_EC)

/* Test vectors generated by BouncyCastle. */

#if defined(EP_PLAIN) && FP_PRIME == 256

#define NIST_P256_A		"DA818E65859F3997D4CD287945363B14A0030665B8ABD19719D57952E3A2BEAD"
#define NIST_P256_B		"66BF67EDF1ABDC8178C8A07644FDD5C88EFD4954FD6D2691933B5F0EA0AE2153"
#define NIST_P256_A_X	"9A2E9583CCBDD502933709D3ED1764E79D1C2EE601DF75A40C486BE3DAB3CDCA"
#define NIST_P256_A_Y	"D025EA9D9BDA94C0DC7F3813ECA72B369F52CA87E92948BCD76984F44D319F8F"
#define NIST_P256_B_X	"B8F245FC8A1C7E933D5CAD6E77102C72B0C1F393F779F3F504DA1CA776434B10"
#define NIST_P256_B_Y	"5373FA01BC13FF5843D4A31E40833785C598C0BBC2F6AF7317C327BE09883799"

uint8_t result[] = {
	0xC0, 0xEC, 0x2B, 0xAC, 0xEB, 0x3C, 0x6E, 0xE3, 0x21, 0x96, 0xD5, 0x43,
	0x0E, 0xE6, 0xDA, 0xBB, 0x50, 0xAE, 0xEE, 0xBE, 0xBA, 0xCE, 0x6B, 0x86,
	0x09, 0xD7, 0xEB, 0x07, 0xD6, 0x45, 0xF6, 0x34, 0xD4, 0xE0, 0xD1, 0x9A,
	0xAB, 0xA0, 0xD2, 0x90, 0x2F, 0x4A, 0xDC, 0x20, 0x1B, 0x0F, 0x35, 0x8D
};

#endif

#define ASSIGNP(CURVE)														\
	RLC_GET(str, CURVE##_A, sizeof(CURVE##_A));								\
	bn_read_str(da, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_A_X, sizeof(CURVE##_A_X));							\
	fp_read_str(qa->x, str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_A_Y, sizeof(CURVE##_A_Y));							\
	fp_read_str(qa->y, str, strlen(str), 16);								\
	fp_set_dig(qa->z, 1);													\
	RLC_GET(str, CURVE##_B, sizeof(CURVE##_B));								\
	bn_read_str(d_b, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_B_X, sizeof(CURVE##_B_X));							\
	fp_read_str(q_b->x, str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B_Y, sizeof(CURVE##_B_Y));							\
	fp_read_str(q_b->y, str, strlen(str), 16);								\
	fp_set_dig(q_b->z, 1);													\
	qa->coord = q_b->coord = BASIC;											\

#define ASSIGNK(CURVE)														\
	RLC_GET(str, CURVE##_A, sizeof(CURVE##_A));								\
	bn_read_str(da, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_A_X, sizeof(CURVE##_A_X));							\
	fb_read_str(qa->x, str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_A_Y, sizeof(CURVE##_A_Y));							\
	fb_read_str(qa->y, str, strlen(str), 16);								\
	fb_set_dig(qa->z, 1);													\
	RLC_GET(str, CURVE##_B, sizeof(CURVE##_B));								\
	bn_read_str(d_b, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_B_X, sizeof(CURVE##_B_X));							\
	fb_read_str(q_b->x, str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B_Y, sizeof(CURVE##_B_Y));							\
	fb_read_str(q_b->y, str, strlen(str), 16);								\
	fb_set_dig(q_b->z, 1);													\
	qa->coord = q_b->coord = BASIC;											\

static int ecdh(void) {
	int code = RLC_ERR;
	bn_t da, d_b;
	ec_t qa, q_b;
	uint8_t k1[RLC_MD_LEN], k2[RLC_MD_LEN];

	bn_null(da);
	bn_null(d_b);
	ec_null(qa);
	ec_null(q_b);

	RLC_TRY {
		bn_new(da);
		bn_new(d_b);
		ec_new(qa);
		ec_new(q_b);

		TEST_CASE("ecdh key agreement is correct") {
			TEST_ASSERT(cp_ecdh_gen(da, qa) == RLC_OK, end);
			TEST_ASSERT(cp_ecdh_gen(d_b, q_b) == RLC_OK, end);
			TEST_ASSERT(cp_ecdh_key(k1, RLC_MD_LEN, d_b, qa) == RLC_OK, end);
			TEST_ASSERT(cp_ecdh_key(k2, RLC_MD_LEN, da, q_b) == RLC_OK, end);
			TEST_ASSERT(memcmp(k1, k2, RLC_MD_LEN) == 0, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(da);
	bn_free(d_b);
	ec_free(qa);
	ec_free(q_b);
	return code;
}

static int ecmqv(void) {
	int code = RLC_ERR;
	bn_t d1a, d1_b;
	bn_t d2a, d2_b;
	ec_t q1a, q1_b;
	ec_t q2a, q2_b;
	uint8_t key1[RLC_MD_LEN], key2[RLC_MD_LEN];

	bn_null(d1a);
	bn_null(d1_b);
	ec_null(q1a);
	ec_null(q1_b);
	bn_null(d2a);
	bn_null(d2_b);
	ec_null(q2a);
	ec_null(q2_b);

	RLC_TRY {
		bn_new(d1a);
		bn_new(d1_b);
		ec_new(q1a);
		ec_new(q1_b);
		bn_new(d2a);
		bn_new(d2_b);
		ec_new(q2a);
		ec_new(q2_b);

		TEST_CASE("ecmqv authenticated key agreement is correct") {
			TEST_ASSERT(cp_ecmqv_gen(d1a, q1a) == RLC_OK, end);
			TEST_ASSERT(cp_ecmqv_gen(d2a, q2a) == RLC_OK, end);
			TEST_ASSERT(cp_ecmqv_gen(d1_b, q1_b) == RLC_OK, end);
			TEST_ASSERT(cp_ecmqv_gen(d2_b, q2_b) == RLC_OK, end);
			TEST_ASSERT(cp_ecmqv_key(key1, RLC_MD_LEN, d1_b, d2_b, q2_b, q1a,
							q2a) == RLC_OK, end);
			TEST_ASSERT(cp_ecmqv_key(key2, RLC_MD_LEN, d1a, d2a, q2a, q1_b,
							q2_b) == RLC_OK, end);
			TEST_ASSERT(memcmp(key1, key2, RLC_MD_LEN) == 0, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(d1a);
	bn_free(d1_b);
	ec_free(q1a);
	ec_free(q1_b);
	bn_free(d2a);
	bn_free(d2_b);
	ec_free(q2a);
	ec_free(q2_b);
	return code;
}

static int ecies(void) {
	int code = RLC_ERR;
	ec_t r;
	bn_t da, d_b;
	ec_t qa, q_b;
	int l, in_len, out_len;
	uint8_t in[RLC_BC_LEN - 1], out[RLC_BC_LEN + RLC_MD_LEN];

	ec_null(r);
	bn_null(da);
	bn_null(d_b);
	ec_null(qa);
	ec_null(q_b);

	RLC_TRY {
		ec_new(r);
		bn_new(da);
		bn_new(d_b);
		ec_new(qa);
		ec_new(q_b);

		l = ec_param_level();
		if (l == 80 || l == 128 || l == 192 || l == 256) {
			TEST_CASE("ecies encryption/decryption is correct") {
				TEST_ASSERT(cp_ecies_gen(da, qa) == RLC_OK, end);
				in_len = RLC_BC_LEN - 1;
				out_len = RLC_BC_LEN + RLC_MD_LEN;
				rand_bytes(in, in_len);
				TEST_ASSERT(cp_ecies_enc(r, out, &out_len, in, in_len, qa)
						== RLC_OK, end);
				TEST_ASSERT(cp_ecies_dec(out, &out_len, r, out, out_len, da)
						== RLC_OK, end);
				TEST_ASSERT(memcmp(in, out, out_len) == 0, end);
			}
			TEST_END;
		}
#if MD_MAP == SH256
		uint8_t msg[RLC_BC_LEN + RLC_MD_LEN];
		char str[2 * RLC_FC_BYTES + 1];

		switch (ec_param_get()) {

#if defined(EP_PLAIN) && FP_PRIME == 256
			case NIST_P256:
				ASSIGNP(NIST_P256);
				memcpy(msg, result, sizeof(result));
				break;
#endif
			default:
				(void)str;
				code = RLC_OK;
				break;
		}

		if (code != RLC_OK) {

			TEST_ONCE("ecies satisfies test vectors") {
				uint8_t in[] = {
					0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF
				};
				TEST_ASSERT(ec_on_curve(qa) == 1, end);
				TEST_ASSERT(ec_on_curve(q_b) == 1, end);
				out_len = 16;
				TEST_ASSERT(cp_ecies_dec(out, &out_len, q_b, msg, sizeof(msg),
								da) == RLC_OK, end);
				TEST_ASSERT(out_len == sizeof(in), end);
				TEST_ASSERT(memcmp(out, in, sizeof(in)) == RLC_OK, end);
				out_len = 16;
				TEST_ASSERT(cp_ecies_dec(out, &out_len, qa, msg, sizeof(msg),
								d_b) == RLC_OK, end);
				TEST_ASSERT(out_len == sizeof(in), end);
				TEST_ASSERT(memcmp(out, in, sizeof(in)) == RLC_OK, end);
			}
			TEST_END;
		}
#endif
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	ec_free(r);
	bn_free(da);
	bn_free(d_b);
	ec_free(qa);
	ec_free(q_b);
	return code;
}

static int ecdsa(void) {
	int code = RLC_ERR;
	bn_t d, r, s;
	ec_t q;
	uint8_t m[5] = { 0, 1, 2, 3, 4 }, h[RLC_MD_LEN];

	bn_null(d);
	bn_null(r);
	bn_null(s);
	ec_null(q);

	RLC_TRY {
		bn_new(d);
		bn_new(r);
		bn_new(s);
		ec_new(q);

		TEST_CASE("ecdsa signature is correct") {
			TEST_ASSERT(cp_ecdsa_gen(d, q) == RLC_OK, end);
			TEST_ASSERT(cp_ecdsa_sig(r, s, m, sizeof(m), 0, d) == RLC_OK, end);
			TEST_ASSERT(cp_ecdsa_ver(r, s, m, sizeof(m), 0, q) == 1, end);
			m[0] ^= 1;
			TEST_ASSERT(cp_ecdsa_ver(r, s, m, sizeof(m), 0, q) == 0, end);
			md_map(h, m, sizeof(m));
			TEST_ASSERT(cp_ecdsa_sig(r, s, h, RLC_MD_LEN, 1, d) == RLC_OK, end);
			TEST_ASSERT(cp_ecdsa_ver(r, s, h, RLC_MD_LEN, 1, q) == 1, end);
			h[0] ^= 1;
			TEST_ASSERT(cp_ecdsa_ver(r, s, h, RLC_MD_LEN, 1, q) == 0, end);
			memset(h, 0, RLC_MD_LEN);
			TEST_ASSERT(cp_ecdsa_ver(r, s, h, RLC_MD_LEN, 1, q) == 0, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(d);
	bn_free(r);
	bn_free(s);
	ec_free(q);
	return code;
}

static int ecss(void) {
	int code = RLC_ERR;
	bn_t d, r;
	ec_t q;
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(d);
	bn_null(r);
	ec_null(q);

	RLC_TRY {
		bn_new(d);
		bn_new(r);
		ec_new(q);

		TEST_CASE("ecss signature is correct") {
			TEST_ASSERT(cp_ecss_gen(d, q) == RLC_OK, end);
			TEST_ASSERT(cp_ecss_sig(r, d, m, sizeof(m), d) == RLC_OK, end);
			TEST_ASSERT(cp_ecss_ver(r, d, m, sizeof(m), q) == 1, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(d);
	bn_free(r);
	ec_free(q);
	return code;
}

static int vbnn(void) {
	int code = RLC_ERR;
	uint8_t ida[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint8_t idb[] = { 5, 6, 7, 8, 9, 0, 1, 2, 3, 4 };
	bn_t ska, skb;
	ec_t pka, pkb;
	bn_t msk, z, h;
	ec_t r, mpk;

	uint8_t m[] = "Thrice the brinded cat hath mew'd.";

	bn_null(z);
	bn_null(h);
	bn_null(msk);
	bn_null(ska);
	bn_null(skb);
	ec_null(r);
	ec_null(mpk);
	bn_null(pka);
	bn_null(pkb);

	RLC_TRY {
		bn_new(z);
		bn_new(h);
		bn_new(msk);
		bn_new(ska);
		bn_new(skb);
		ec_new(r);
		ec_new(mpk);
		ec_new(pka);
		ec_new(pkb);

		TEST_CASE("vbnn signature is correct") {
			TEST_ASSERT(cp_vbnn_gen(msk, mpk) == RLC_OK, end);
			TEST_ASSERT(cp_vbnn_gen_prv(ska, pka, msk, ida, sizeof(ida)) == RLC_OK, end);
			TEST_ASSERT(cp_vbnn_gen_prv(skb, pkb, msk, idb, sizeof(idb)) == RLC_OK, end);
			TEST_ASSERT(cp_vbnn_sig(r, z, h, ida, sizeof(ida), m, sizeof(m), ska, pka) == RLC_OK, end);
			TEST_ASSERT(cp_vbnn_ver(r, z, h, ida, sizeof(ida), m, sizeof(m), mpk) == 1, end);
			TEST_ASSERT(cp_vbnn_ver(r, z, h, idb, sizeof(idb), m, sizeof(m), mpk) == 0, end);
			TEST_ASSERT(cp_vbnn_sig(r, z, h, ida, sizeof(ida), m, sizeof(m), skb, pkb) == RLC_OK, end);
			TEST_ASSERT(cp_vbnn_ver(r, z, h, ida, sizeof(ida), m, sizeof(m), mpk) == 0, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

end:
	bn_free(z);
	bn_free(h);
	bn_free(msk);
	bn_free(ska);
	bn_free(skb);
	ec_free(r);
	ec_free(mpk);
	ec_free(pka);
	ec_free(pkb);
	return code;
}

static int pok(void) {
	int code = RLC_ERR;
	bn_t c[2], n, r[2], x;
	ec_t y[2];

	bn_null(n);
	bn_null(x);

	RLC_TRY {
		bn_new(n);
		bn_new(x);
		for (int i = 0; i < 2; i++) {
			bn_null(c[i]);
			bn_null(r[i]);
			ec_null(y[i]);
			bn_new(c[i]);
			bn_new(r[i]);
			ec_new(y[i]);
		}
		ec_curve_get_ord(n);

		TEST_CASE("proof of knowledge of discrete logarithm is correct") {
			bn_rand_mod(x, n);
			ec_mul_gen(y[0], x);
			TEST_ASSERT(cp_pokdl_prv(c[0], r[0], y[0], x) == RLC_OK, end);
			TEST_ASSERT(cp_pokdl_ver(c[0], r[0], y[0]) == 1, end);
			ec_dbl(y[0], y[0]);
			ec_norm(y[0], y[0]);
			TEST_ASSERT(cp_pokdl_ver(c[0], r[0], y[0]) == 0, end);
		} TEST_END;

		TEST_CASE("proof of knowledge of disjunction is correct") {
			bn_rand_mod(x, n);
			do {
				ec_rand(y[0]);
				ec_mul_gen(y[1], x);
			} while (ec_cmp(y[0], y[1]) == RLC_EQ);
			TEST_ASSERT(cp_pokor_prv(c, r, y, x) == RLC_OK, end);
			TEST_ASSERT(cp_pokor_ver(c, r, y) == 1, end);
			ec_dbl(y[1], y[1]);
			ec_norm(y[1], y[1]);
			TEST_ASSERT(cp_pokor_ver(c, r, y) == 0, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

end:
	bn_free(n);
	bn_free(x);
	for (int i = 0; i < 2; i++) {
		bn_free(c[i]);
		bn_free(r[i]);
		ec_free(y[i]);
	}
	return code;
}

static int sok(void) {
	int code = RLC_ERR;
	bn_t c[2], n, r[2], x;
	ec_t y[2];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(n);
	bn_null(x);

	RLC_TRY {
		bn_new(n);
		bn_new(x);
		for (int i = 0; i < 2; i++) {
			bn_null(c[i]);
			bn_null(r[i]);
			ec_null(y[i]);
			bn_new(c[i]);
			bn_new(r[i]);
			ec_new(y[i]);
		}
		ec_curve_get_ord(n);

		TEST_CASE("signature of knowledge of discrete logarithm is correct") {
			bn_rand_mod(x, n);
			ec_mul_gen(y[0], x);
			TEST_ASSERT(cp_sokdl_sig(c[0], r[0], m, 5, y[0], x) == RLC_OK, end);
			TEST_ASSERT(cp_sokdl_ver(c[0], r[0], m, 5, y[0]) == 1, end);
			ec_dbl(y[0], y[0]);
			ec_norm(y[0], y[0]);
			TEST_ASSERT(cp_sokdl_ver(c[0], r[0], m, 5, y[0]) == 0, end);
		} TEST_END;

		TEST_CASE("signature of knowledge of disjunction is correct") {
			bn_rand_mod(x, n);
			do {
				ec_rand(y[0]);
				ec_mul_gen(y[1], x);
			} while (ec_cmp(y[0], y[1]) == RLC_EQ);
			TEST_ASSERT(cp_sokor_sig(c, r,  m, 5, y, x, 0) == RLC_OK, end);
			TEST_ASSERT(cp_sokor_ver(c, r,  m, 5, y) == 1, end);
			ec_dbl(y[1], y[1]);
			ec_norm(y[1], y[1]);
			TEST_ASSERT(cp_sokor_ver(c, r,  m, 5, y) == 0, end);
			do {
				ec_mul_gen(y[0], x);
				ec_rand(y[1]);
			} while (ec_cmp(y[0], y[1]) == RLC_EQ);
			TEST_ASSERT(cp_sokor_sig(c, r,  m, 5, y, x, 1) == RLC_OK, end);
			TEST_ASSERT(cp_sokor_ver(c, r,  m, 5, y) == 1, end);
			ec_dbl(y[0], y[0]);
			ec_norm(y[0], y[0]);
			TEST_ASSERT(cp_sokor_ver(c, r,  m, 5, y) == 0, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

end:
	bn_free(n);
	bn_free(x);
	for (int i = 0; i < 2; i++) {
		bn_free(c[i]);
		bn_free(r[i]);
		ec_free(y[i]);
	}
	return code;
}

static int ers(void) {
	int size, code = RLC_ERR;
	ec_t pp, pk[4];
	bn_t sk[4], td;
	ers_t ring[4];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(td);
	ec_null(pp);

	RLC_TRY {
		bn_new(td);
		ec_new(pp);
		for (int i = 0; i < 4; i++) {
			bn_null(sk[i]);
			bn_new(sk[i]);
			ec_null(pk[i]);
			ec_new(pk[i]);
			ers_null(ring[i]);
			ers_new(ring[i]);
			cp_ers_gen_key(sk[i], pk[i]);
		}

		cp_ers_gen(pp);

		TEST_CASE("extendable ring signature scheme is correct") {
			TEST_ASSERT(cp_ers_sig(td, ring[0], m, 5, sk[0], pk[0], pp) == RLC_OK, end);
			TEST_ASSERT(cp_ers_ver(td, ring, 1, m, 5, pp) == 1, end);
			TEST_ASSERT(cp_ers_ver(td, ring, 1, m, 0, pp) == 0, end);
			size = 1;
			for (int j = 1; j < 4; j++) {
				TEST_ASSERT(cp_ers_ext(td, ring, &size, m, 5, pk[j], pp) == RLC_OK, end);
				TEST_ASSERT(cp_ers_ver(td, ring, size, m, 5, pp) == 1, end);
				TEST_ASSERT(cp_ers_ver(td, ring, size, m, 0, pp) == 0, end);
			}
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

end:
	bn_free(td);
	ec_free(pp);
	for (int i = 0; i < 4; i++) {
		bn_free(sk[i]);
		ec_free(pk[i]);
		ers_free(ring[i])
	}
	return code;
}

static int etrs(void) {
	int size, code = RLC_ERR;
	ec_t pp, pk[4];
	bn_t sk[4], td[4], y[4];
	etrs_t ring[4];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };


	ec_null(pp);

	RLC_TRY {
		ec_new(pp);
		for (int i = 0; i < 4; i++) {
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
			cp_etrs_gen_key(sk[i], pk[i]);
		}

		cp_etrs_gen(pp);

		TEST_CASE("extendable threshold ring signature scheme is correct") {
			TEST_ASSERT(cp_etrs_sig(td, y, 4, ring[0], m, 5, sk[0], pk[0], pp) == RLC_OK, end);
			TEST_ASSERT(cp_etrs_ver(0, td, y, 4, ring, 1, m, 5, pp) == 0, end);
			TEST_ASSERT(cp_etrs_ver(1, td, y, 4, ring, 1, m, 5, pp) == 1, end);
			TEST_ASSERT(cp_etrs_ver(1, td, y, 4, ring, 1, m, 0, pp) == 0, end);
			size = 1;
			for (int j = 1; j < 4; j++) {
				TEST_ASSERT(cp_etrs_ext(td, y, 4, ring, &size, m, 5, pk[j], pp) == RLC_OK, end);
				TEST_ASSERT(cp_etrs_ver(0, td+j, y+j, 4-j, ring, size, m, 5, pp) == 0, end);
				TEST_ASSERT(cp_etrs_ver(1, td+j, y+j, 4-j, ring, size, m, 5, pp) == 1, end);
				TEST_ASSERT(cp_etrs_ver(1, td+j, y+j, 4-j, ring, size, m, 0, pp) == 0, end);
			}

			TEST_ASSERT(cp_etrs_sig(td, y, 4, ring[0], m, 5, sk[0], pk[0], pp) == RLC_OK, end);
			size = 1;
			TEST_ASSERT(cp_etrs_uni(1, td, y, 4, ring, &size, m, 5, sk[1], pk[1], pp) == RLC_OK, end);
			TEST_ASSERT(cp_etrs_ver(1, td, y, 4, ring, size, m, 5, pp) == 0, end);
			TEST_ASSERT(cp_etrs_ver(2, td, y, 4, ring, size, m, 5, pp) == 1, end);
			TEST_ASSERT(cp_etrs_ver(2, td, y, 4, ring, size, m, 0, pp) == 0, end);
			for (int j = 2; j < 4; j++) {
				TEST_ASSERT(cp_etrs_ext(td, y, 4, ring, &size, m, 5, pk[j], pp) == RLC_OK, end);
				TEST_ASSERT(cp_etrs_ver(1, td+j-1, y+j-1, 4-j+1, ring, size, m, 5, pp) == 0, end);
				TEST_ASSERT(cp_etrs_ver(2, td+j-1, y+j-1, 4-j+1, ring, size, m, 5, pp) == 1, end);
				TEST_ASSERT(cp_etrs_ver(2, td+j-1, y+j-1, 4-j+1, ring, size, m, 0, pp) == 0, end);
			}
		} TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

end:
	ec_free(pp);
	for (int i = 0; i < 4; i++) {
		bn_free(td[i]);
		bn_free(y[i]);
		bn_free(sk[i]);
		ec_free(pk[i]);
		etrs_free(ring[i])
	}
	return code;
}

#endif /* WITH_EC */

#if defined(WITH_PC)

static int pdpub(void) {
	int code = RLC_ERR;
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

	RLC_TRY {
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

		TEST_CASE("delegated pairing computation with public inputs is correct") {
			TEST_ASSERT(cp_pdpub_gen(r1, r2, u1, u2, v2, e) == RLC_OK, end);
			g1_rand(p);
			g2_rand(q);
			TEST_ASSERT(cp_pdpub_ask(v1, w2, p, q, r1, r2, u1, u2, v2) == RLC_OK, end);
			TEST_ASSERT(cp_pdpub_ans(g, p, q, v1, v2, w2) == RLC_OK, end);
			TEST_ASSERT(cp_pdpub_ver(r, g, r1, e) == 1, end);
			pc_map(e, p, q);
			TEST_ASSERT(gt_cmp(r, e) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("faster delegated pairing with public inputs is correct") {
			TEST_ASSERT(cp_lvpub_gen(r2, u1, u2, v2, e) == RLC_OK, end);
			g1_rand(p);
			g2_rand(q);
			TEST_ASSERT(cp_lvpub_ask(r1, v1, w2, p, q, r2, u1, u2, v2) == RLC_OK, end);
			TEST_ASSERT(cp_lvpub_ans(g, p, q, v1, v2, w2) == RLC_OK, end);
			TEST_ASSERT(cp_lvpub_ver(r, g, r1, e) == 1, end);
			pc_map(e, p, q);
			TEST_ASSERT(gt_cmp(r, e) == RLC_EQ, end);
		} TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
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
  	return code;
}

static int pdprv(void) {
	int code = RLC_ERR;
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

	RLC_TRY {
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

		TEST_CASE("delegated pairing computation with private inputs is correct") {
			TEST_ASSERT(cp_pdprv_gen(r1, r2, u1, u2, v2, e) == RLC_OK, end);
			g1_rand(p);
			g2_rand(q);
			TEST_ASSERT(cp_pdprv_ask(v1, w2, p, q, r1, r2, u1, u2, v2) == RLC_OK, end);
			TEST_ASSERT(cp_pdprv_ans(g, v1, w2) == RLC_OK, end);
			TEST_ASSERT(cp_pdprv_ver(r, g, r1, e) == 1, end);
			pc_map(e[0], p, q);
			TEST_ASSERT(gt_cmp(r, e[0]) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("faster delegated pairing with private inputs is correct") {
			TEST_ASSERT(cp_pdprv_gen(r1, r2, u1, u2, v2, e) == RLC_OK, end);
			g1_rand(p);
			g2_rand(q);
			TEST_ASSERT(cp_lvprv_ask(v1, w2, p, q, r1, r2, u1, u2, v2) == RLC_OK, end);
			TEST_ASSERT(cp_lvprv_ans(g, v1, w2) == RLC_OK, end);
			TEST_ASSERT(cp_lvprv_ver(r, g, r1, e) == 1, end);
			pc_map(e[0], p, q);
			TEST_ASSERT(gt_cmp(r, e[0]) == RLC_EQ, end);
		} TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
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
  	return code;
}

static int sokaka(void) {
	int code = RLC_ERR, l = RLC_MD_LEN;
	sokaka_t k;
	bn_t s;
	uint8_t k1[RLC_MD_LEN], k2[RLC_MD_LEN];
	char *ia = "Alice";
	char *ib = "Bob";

	sokaka_null(k);
	bn_null(s);

	RLC_TRY {
		sokaka_new(k);
		bn_new(s);

		cp_sokaka_gen(s);

		TEST_CASE
				("sakai-ohgishi-kasahara authenticated key agreement is correct")
		{
			TEST_ASSERT(cp_sokaka_gen_prv(k, ia, s) == RLC_OK, end);
			TEST_ASSERT(cp_sokaka_key(k1, l, ia, k, ib) == RLC_OK, end);
			TEST_ASSERT(cp_sokaka_gen_prv(k, ib, s) == RLC_OK, end);
			TEST_ASSERT(cp_sokaka_key(k2, l, ib, k, ia) == RLC_OK, end);
			TEST_ASSERT(memcmp(k1, k2, l) == 0, end);
		} TEST_END;

	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	sokaka_free(k);
	bn_free(s);
	return code;
}

static int ibe(void) {
	int code = RLC_ERR;
	bn_t s;
	g1_t pub;
	g2_t prv;
	uint8_t in[10], out[10 + 2 * RLC_FP_BYTES + 1];
	char *id = "Alice";
	int il, ol;
	int result;

	bn_null(s);
	g1_null(pub);
	g2_null(prv);

	RLC_TRY {
		bn_new(s);
		g1_new(pub);
		g2_new(prv);

		result = cp_ibe_gen(s, pub);

		TEST_CASE("boneh-franklin identity-based encryption/decryption is correct") {
			TEST_ASSERT(result == RLC_OK, end);
			il = 10;
			ol = il + 2 * RLC_FP_BYTES + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_ibe_gen_prv(prv, id, s) == RLC_OK, end);
			TEST_ASSERT(cp_ibe_enc(out, &ol, in, il, id, pub) == RLC_OK, end);
			TEST_ASSERT(cp_ibe_dec(out, &il, out, ol, prv) == RLC_OK, end);
			TEST_ASSERT(memcmp(in, out, il) == 0, end);
		} TEST_END;
	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(s);
	g1_free(pub);
	g2_free(prv);
	return code;
}

static int bgn(void) {
	int result, code = RLC_ERR;
	g1_t c[2], d[2];
	g2_t e[2], f[2];
	gt_t g[4];
	bgn_t pub, prv;
	dig_t in, out, t;

	g1_null(c[0]);
	g1_null(c[1]);
	g1_null(d[0]);
	g1_null(d[1]);
	g2_null(e[0]);
	g2_null(e[1]);
	g2_null(f[0]);
	g2_null(f[1]);
	bgn_null(pub);
	bgn_null(prv);

	RLC_TRY {
		g1_new(c[0]);
		g1_new(c[1]);
		g1_new(d[0]);
		g1_new(d[1]);
		g2_new(e[0]);
		g2_new(e[1]);
		g2_new(f[0]);
		g2_new(f[1]);
		bgn_new(pub);
		bgn_new(prv);
		for (int i = 0; i < 4; i++) {
			gt_null(g[i]);
			gt_new(g[i]);
		}

		result = cp_bgn_gen(pub, prv);

		TEST_CASE("boneh-go-nissim encryption/decryption is correct") {
			TEST_ASSERT(result == RLC_OK, end);

			rand_bytes((unsigned char *)&in, sizeof(dig_t));
			in = in % 11;

			TEST_ASSERT(cp_bgn_enc1(c, in, pub) == RLC_OK, end);
			TEST_ASSERT(cp_bgn_dec1(&out, c, prv) == RLC_OK, end);
			TEST_ASSERT(in == out, end);
			TEST_ASSERT(cp_bgn_enc2(e, in, pub) == RLC_OK, end);
			TEST_ASSERT(cp_bgn_dec2(&out, e, prv) == RLC_OK, end);
			TEST_ASSERT(in == out, end);
		} TEST_END;

		TEST_CASE("boneh-go-nissim encryption is additively homomorphic") {
			rand_bytes((unsigned char *)&in, sizeof(dig_t));
			in = in % 11;
			out = in % 7;
			TEST_ASSERT(cp_bgn_enc1(c, in, pub) == RLC_OK, end);
			TEST_ASSERT(cp_bgn_enc1(d, out, pub) == RLC_OK, end);
			g1_add(c[0], c[0], d[0]);
			g1_add(c[1], c[1], d[1]);
			g1_norm(c[0], c[0]);
			g1_norm(c[1], c[1]);
			TEST_ASSERT(cp_bgn_dec1(&t, c, prv) == RLC_OK, end);
			TEST_ASSERT(in + out == t, end);
			TEST_ASSERT(cp_bgn_enc2(e, in, pub) == RLC_OK, end);
			TEST_ASSERT(cp_bgn_enc2(f, out, pub) == RLC_OK, end);
			g2_add(e[0], e[0], f[0]);
			g2_add(e[1], e[1], f[1]);
			g2_norm(e[0], e[0]);
			g2_norm(e[1], e[1]);
			TEST_ASSERT(cp_bgn_dec2(&t, e, prv) == RLC_OK, end);
			TEST_ASSERT(in + out == t, end);
		} TEST_END;

		TEST_CASE("boneh-go-nissim encryption is multiplicatively homomorphic") {
			rand_bytes((unsigned char *)&in, sizeof(dig_t));
			in = in % 11;
			out = in % 17;
			TEST_ASSERT(cp_bgn_enc1(c, in, pub) == RLC_OK, end);
			TEST_ASSERT(cp_bgn_enc2(e, out, pub) == RLC_OK, end);
			in = in * out;
			TEST_ASSERT(cp_bgn_mul(g, c, e) == RLC_OK, end);
			TEST_ASSERT(cp_bgn_dec(&t, g, prv) == RLC_OK, end);
			TEST_ASSERT(in == t, end);
			TEST_ASSERT(cp_bgn_add(g, g, g) == RLC_OK, end);
			TEST_ASSERT(cp_bgn_dec(&t, g, prv) == RLC_OK, end);
			TEST_ASSERT(in + in == t, end);
		} TEST_END;

	} RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	g1_free(c[0]);
	g1_free(c[1]);
	g1_free(d[0]);
	g1_free(d[1]);
	g2_free(e[0]);
	g2_free(e[1]);
	g2_free(f[0]);
	g2_free(f[1]);
	bgn_free(pub);
	bgn_free(prv);
	for (int i = 0; i < 4; i++) {
		gt_free(g[i]);
	}
	return code;
}

static int bls(void) {
	int code = RLC_ERR;
	bn_t d;
	g1_t s;
	g2_t q;
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(d);
	g1_null(s);
	g2_null(q);

	RLC_TRY {
		bn_new(d);
		g1_new(s);
		g2_new(q);

		TEST_CASE("boneh-lynn-schacham short signature is correct") {
			TEST_ASSERT(cp_bls_gen(d, q) == RLC_OK, end);
			TEST_ASSERT(cp_bls_sig(s, m, sizeof(m), d) == RLC_OK, end);
			TEST_ASSERT(cp_bls_ver(s, m, sizeof(m), q) == 1, end);
			/* Check adversarial signature. */
			memset(m, 0, sizeof(m));
			g2_set_infty(q);
			TEST_ASSERT(cp_bls_ver(s, m, sizeof(m), q) == 0, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(d);
	g1_free(s);
	g2_free(q);
	return code;
}

static int bbs(void) {
	int code = RLC_ERR;
	bn_t d;
	g1_t s;
	g2_t q;
	gt_t z;
	uint8_t m[5] = { 0, 1, 2, 3, 4 }, h[RLC_MD_LEN];

	bn_null(d);
	g1_null(s);
	g2_null(q);
	gt_null(z);

	RLC_TRY {
		bn_new(d);
		g1_new(s);
		g2_new(q);
		gt_new(z);

		TEST_CASE("boneh-boyen short signature is correct") {
			TEST_ASSERT(cp_bbs_gen(d, q, z) == RLC_OK, end);
			TEST_ASSERT(cp_bbs_sig(s, m, sizeof(m), 0, d) == RLC_OK, end);
			TEST_ASSERT(cp_bbs_ver(s, m, sizeof(m), 0, q, z) == 1, end);
			md_map(h, m, sizeof(m));
			TEST_ASSERT(cp_bbs_sig(s, m, sizeof(m), 1, d) == RLC_OK, end);
			TEST_ASSERT(cp_bbs_ver(s, m, sizeof(m), 1, q, z) == 1, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(d);
	g1_free(s);
	g2_free(q);
	gt_free(z);
	return code;
}

static int cls(void) {
	int i, code = RLC_ERR;
	bn_t r, t, u, v, vs[4];
	g1_t a, A, b, B, c, As[4], Bs[4];
	g2_t x, y, z, zs[4];
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
		bn_null(vs[i]);
		g1_null(As[i]);
		g1_null(Bs[i]);
		g2_null(zs[i]);
	}

	RLC_TRY {
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
			bn_new(vs[i]);
			g1_new(As[i]);
			g1_new(Bs[i]);
			g2_new(zs[i]);
		}

		TEST_CASE("camenisch-lysyanskaya simple signature is correct") {
			TEST_ASSERT(cp_cls_gen(u, v, x, y) == RLC_OK, end);
			TEST_ASSERT(cp_cls_sig(a, b, c, m, sizeof(m), u, v) == RLC_OK, end);
			TEST_ASSERT(cp_cls_ver(a, b, c, m, sizeof(m), x, y) == 1, end);
			/* Check adversarial signature. */
			g1_set_infty(a);
			g1_set_infty(b);
			g1_set_infty(c);
			TEST_ASSERT(cp_cls_ver(a, b, c, m, sizeof(m), x, y) == 0, end);
		}
		TEST_END;

		TEST_CASE("camenisch-lysyanskaya message-independent signature is correct") {
			bn_rand(r, RLC_POS, 2 * pc_param_level());
			TEST_ASSERT(cp_cli_gen(t, u, v, x, y, z) == RLC_OK, end);
			TEST_ASSERT(cp_cli_sig(a, A, b, B, c, m, sizeof(m), r, t, u, v) == RLC_OK, end);
			TEST_ASSERT(cp_cli_ver(a, A, b, B, c, m, sizeof(m), r, x, y, z) == 1, end);
			/* Check adversarial signature. */
			g1_set_infty(a);
			g1_set_infty(A);
			g1_set_infty(b);
			g1_set_infty(B);
			g1_set_infty(c);
			TEST_ASSERT(cp_cli_ver(a, A, b, B, c, m, sizeof(m), r, x, y, z) == 0, end);
		}
		TEST_END;

		TEST_CASE("camenisch-lysyanskaya message-block signature is correct") {
			TEST_ASSERT(cp_clb_gen(t, u, vs, x, y, zs, 5) == RLC_OK, end);
			TEST_ASSERT(cp_clb_sig(a, As, b, Bs, c, msgs, lens, t, u, vs, 5) == RLC_OK, end);
			TEST_ASSERT(cp_clb_ver(a, As, b, Bs, c, msgs, lens, x, y, zs, 5) == 1, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
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
		bn_free(vs[i]);
		g1_free(As[i]);
		g1_free(Bs[i]);
		g2_free(zs[i]);
	}
  	return code;
}

static int pss(void) {
	int i, code = RLC_ERR;
	bn_t ms[5], n, u, v, _v[5];
	g1_t a, b;
	g2_t g, x, y, _y[5];

	bn_null(n);
	bn_null(u);
	bn_null(v);
	g1_null(a);
	g1_null(b);
	g2_null(g);
	g2_null(x);
	g2_null(y);

	RLC_TRY {
		bn_new(n);
		bn_new(u);
		bn_new(v);
		g1_new(a);
		g1_new(b);
		g2_new(g);
		g2_new(x);
		g2_new(y);

		g1_get_ord(n);

		for (i = 0; i < 5; i++) {
			bn_null(ms[i]);
			bn_null(_v[i]);
			g2_null(_y[i]);
			bn_new(ms[i]);
			bn_rand_mod(ms[i], n);
			bn_new(_v[i]);
			g2_new(_y[i]);
		}

		TEST_CASE("pointcheval-sanders simple signature is correct") {
			TEST_ASSERT(cp_pss_gen(u, v, g, x, y) == RLC_OK, end);
			TEST_ASSERT(cp_pss_sig(a, b, ms[0], u, v) == RLC_OK, end);
			TEST_ASSERT(cp_pss_ver(a, b, ms[0], g, x, y) == 1, end);
			/* Check adversarial signature. */
			g1_set_infty(a);
			g1_set_infty(b);
			TEST_ASSERT(cp_pss_ver(a, b, ms[0], g, x, y) == 0, end);
		}
		TEST_END;

		TEST_CASE("pointcheval-sanders block signature is correct") {
			TEST_ASSERT(cp_psb_gen(u, _v, g, x, _y, 5) == RLC_OK, end);
			TEST_ASSERT(cp_psb_sig(a, b, ms, u, _v, 5) == RLC_OK, end);
			TEST_ASSERT(cp_psb_ver(a, b, ms, g, x, _y, 5) == 1, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(n);
	bn_free(u);
	bn_free(v);
	g1_free(a);
	g1_free(b);
	g2_free(g);
	g2_free(x);
	g2_free(y);
	for (i = 0; i < 5; i++) {
		bn_free(ms[i]);
		bn_free(_v[i]);
		g2_free(_y[i]);
	}
  	return code;
}

#if defined(WITH_MPC)

static int mpss(void) {
	int i, j, code = RLC_ERR;
	bn_t m[2], n, u[2], v[2], ms[5][2], _v[5][2];
	g1_t g, s[2];
	g2_t h, x[2], y[2], _y[5][2];
	gt_t e[2], f[2];
	mt_t tri[3][2];
	pt_t t[2];

	bn_null(n);
	g1_null(g);
	g2_null(h);

	RLC_TRY {
		bn_new(n);
		g1_new(g);
		g2_new(h);
		g1_get_ord(n);
		for (i = 0; i < 2; i++) {
			bn_null(m[i]);
			bn_null(u[i]);
			bn_null(v[i]);
			g1_null(s[i]);
			g2_null(x[i]);
			g2_null(y[i]);
			gt_null(e[i]);
			gt_null(f[i]);
			mt_null(tri[0][i]);
			mt_null(tri[1][i]);
			mt_null(tri[2][i]);
			pt_null(t[i]);
			bn_new(m[i]);
			bn_rand_mod(m[i], n);
			bn_new(u[i]);
			bn_new(v[i]);
			g1_new(s[i]);
			g2_new(x[i]);
			g2_new(y[i]);
			gt_new(e[i]);
			gt_new(f[i]);
			mt_new(tri[0][i]);
			mt_new(tri[1][i]);
			mt_new(tri[2][i]);
			pt_new(t[i]);
			for (j = 0; j < 5; j++) {
				bn_null(ms[j][i]);
				bn_null(_v[j][i]);
				g2_null(_y[j][i]);
				bn_new(ms[j][i]);
				bn_rand_mod(ms[j][i], n);
				bn_new(_v[j][i]);
				g2_new(_y[j][i]);
			}
		}

		TEST_CASE("multi-party pointcheval-sanders simple signature is correct") {
			pc_map_tri(t);
			mt_gen(tri[0], n);
			mt_gen(tri[1], n);
			mt_gen(tri[2], n);
			gt_exp_gen(e[0], tri[2][0]->b);
			gt_exp_gen(e[1], tri[2][1]->b);
			gt_exp_gen(f[0], tri[2][0]->c);
			gt_exp_gen(f[1], tri[2][1]->c);
			tri[2][0]->bt = &e[0];
			tri[2][1]->bt = &e[1];
			tri[2][0]->ct = &f[0];
			tri[2][1]->ct = &f[1];
			TEST_ASSERT(cp_mpss_gen(u, v, h, x, y) == RLC_OK, end);
			TEST_ASSERT(cp_mpss_bct(x, y) == RLC_OK, end);
			/* Compute signature in MPC. */
			TEST_ASSERT(cp_mpss_sig(g, s, m, u, v, tri[0], tri[1]) == RLC_OK, end);
			/* Verify signature in MPC. */
			cp_mpss_ver(e[0], g, s, m, h, x[0], y[0], tri[2], t);
			TEST_ASSERT(gt_is_unity(e[0]) == 1, end);
			/* Check that signature is also valid for conventional scheme. */
			bn_add(m[0], m[0], m[1]);
			bn_mod(m[0], m[0], n);
			g1_add(s[0], s[0], s[1]);
			g1_norm(s[0], s[0]);
			TEST_ASSERT(cp_pss_ver(g, s[0], m[0], h, x[0], y[0]) == 1, end);
		}
		TEST_END;

		TEST_CASE("multi-party pointcheval-sanders block signature is correct") {
			g1_get_ord(n);
			pc_map_tri(t);
			mt_gen(tri[0], n);
			mt_gen(tri[1], n);
			mt_gen(tri[2], n);
			gt_exp_gen(e[0], tri[2][0]->b);
			gt_exp_gen(e[1], tri[2][1]->b);
			gt_exp_gen(f[0], tri[2][0]->c);
			gt_exp_gen(f[1], tri[2][1]->c);
			tri[2][0]->bt = &e[0];
			tri[2][1]->bt = &e[1];
			tri[2][0]->ct = &f[0];
			tri[2][1]->ct = &f[1];
			TEST_ASSERT(cp_mpsb_gen(u, _v, h, x, _y, 5) == RLC_OK, end);
			TEST_ASSERT(cp_mpsb_bct(x, _y, 5) == RLC_OK, end);
			/* Compute signature in MPC. */
			TEST_ASSERT(cp_mpsb_sig(g, s, ms, u, _v, tri[0], tri[1], 5) == RLC_OK, end);
			/* Verify signature in MPC. */
			cp_mpsb_ver(e[1], g, s, ms, h, x[0], _y, NULL, tri[2], t, 5);
			TEST_ASSERT(gt_is_unity(e[1]) == 1, end);
			gt_exp_gen(e[0], tri[2][0]->b);
			gt_exp_gen(e[1], tri[2][1]->b);
			cp_mpsb_ver(e[1], g, s, ms, h, x[0], _y, _v, tri[2], t, 5);
			TEST_ASSERT(gt_is_unity(e[1]) == 1, end);
			bn_sub_dig(ms[0][0], ms[0][0], 1);
			cp_mpsb_ver(e[1], g, s, ms, h, x[0], _y, _v, tri[2], t, 5);
			TEST_ASSERT(gt_is_unity(e[1]) == 0, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
  	bn_free(n);
	g1_free(g);
	g2_free(h);
	for (i = 0; i < 2; i++) {
		bn_free(m[i]);
		bn_free(u[i]);
		bn_free(v[i]);
		g1_free(s[i]);
		g2_free(x[i]);
		g2_free(y[i]);
		gt_free(e[i]);
		gt_free(f[i]);
		mt_free(tri[0][i]);
		mt_free(tri[1][i]);
		mt_free(tri[2][i]);
		pt_free(t[i]);
		for (j = 0; j < 5; j++) {
			bn_free(ms[j][i]);
			bn_free(_v[j][i]);
			g2_free(_y[j][i]);
		}
	}
  	return code;
}

#endif

static int zss(void) {
	int code = RLC_ERR;
	bn_t d;
	g1_t q;
	g2_t s;
	gt_t z;
	uint8_t m[5] = { 0, 1, 2, 3, 4 }, h[RLC_MD_LEN];

	bn_null(d);
	g1_null(q);
	g2_null(s);
	gt_null(z);

	RLC_TRY {
		bn_new(d);
		g1_new(q);
		g2_new(s);
		gt_new(z);

		TEST_CASE("zhang-safavi-naini-susilo signature is correct") {
			TEST_ASSERT(cp_zss_gen(d, q, z) == RLC_OK, end);
			TEST_ASSERT(cp_zss_sig(s, m, sizeof(m), 0, d) == RLC_OK, end);
			TEST_ASSERT(cp_zss_ver(s, m, sizeof(m), 0, q, z) == 1, end);
			md_map(h, m, sizeof(m));
			TEST_ASSERT(cp_zss_sig(s, m, sizeof(m), 1, d) == RLC_OK, end);
			TEST_ASSERT(cp_zss_ver(s, m, sizeof(m), 1, q, z) == 1, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(d);
	g1_free(q);
	g2_free(s);
	gt_free(z);
	return code;
}

#define S	2			/* Number of signers. */
#define L	4			/* Number of labels. */
#define K	RLC_MD_LEN	/* Size of PRF key. */

static int lhs(void) {
	int code = RLC_ERR;
	uint8_t k[S][K];
	bn_t m, n, msg[S][L], sk[S], d[S], x[S][L];
	g1_t _r, h, as[S], cs[S], sig[S];
	g1_t a[S][L], c[S][L], r[S][L];
	g2_t _s, s[S][L], pk[S], y[S], z[S];
	gt_t *hs[S], vk;
	char *data = "database-identifier";
	char *id[S] = { "Alice", "Bob" };
	dig_t *f[S] = { NULL };
	int flen[S];

	bn_null(m);
	bn_null(n);
	g1_null(h);
	g1_null(_r);
	g2_null(_s);
	gt_null(vk);

	RLC_TRY {
		bn_new(m);
		bn_new(n);
		g1_new(h);
		g1_new(_r);
		g2_new(_s);
		gt_new(vk);

		for (int i = 0; i < S; i++) {
			hs[i] = RLC_ALLOCA(gt_t, RLC_TERMS);
			for (int j = 0; j < RLC_TERMS; j++) {
				gt_null(hs[i][j]);
				gt_new(hs[i][j]);
			}
			for (int j = 0; j < L; j++) {
				bn_null(x[i][j]);
				bn_null(msg[i][j]);
				g1_null(a[i][j]);
				g1_null(c[i][j]);
				g1_null(r[i][j]);
				g2_null(s[i][j]);
				bn_new(x[i][j]);
				bn_new(msg[i][j]);
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
				dig_t t;
				rand_bytes((uint8_t *)&t, sizeof(dig_t));
				f[i][j] = t & RLC_MASK(RLC_DIG / 2);
			}
			flen[i] = L;
		}

		/* Initialize scheme for messages of single components. */
		g1_get_ord(n);
		cp_cmlhs_init(h);
		for (int j = 0; j < S; j++) {
			cp_cmlhs_gen(x[j], hs[j], L, k[j], K, sk[j], pk[j], d[j], y[j]);
		}

		TEST_CASE("context-hiding linear homomorphic signature is correct") {
			int label[L];
			/* Compute all signatures. */
			for (int j = 0; j < S; j++) {
				for (int l = 0; l < L; l++) {
					label[l] = l;
					bn_rand_mod(msg[j][l], n);
					cp_cmlhs_sig(sig[j], z[j], a[j][l], c[j][l], r[j][l], s[j][l],
						msg[j][l], data, label[l], x[j][l], h, k[j], K,
						d[j], sk[j]);
				}
			}
			/* Apply linear function over signatures. */
			for (int j = 0; j < S; j++) {
				cp_cmlhs_fun(as[j], cs[j], a[j], c[j], f[j], flen[j]);
			}

			cp_cmlhs_evl(_r, _s, r[0], s[0], f[0], flen[0]);
			for (int j = 1; j < S; j++) {
				cp_cmlhs_evl(r[0][0], s[0][0], r[j], s[j], f[j], flen[j]);
				g1_add(_r, _r, r[0][0]);
				g2_add(_s, _s, s[0][0]);
			}
			g1_norm(_r, _r);
			g2_norm(_s, _s);
			/* We share messages between users to simplify tests. */
			bn_zero(m);
			for (int j = 0; j < S; j++) {
				for (int l = 0; l < L; l++) {
					bn_mul_dig(msg[j][l], msg[j][l], f[j][l]);
					bn_add(m, m, msg[j][l]);
					bn_mod(m, m, n);
				}
			}

			TEST_ASSERT(cp_cmlhs_ver(_r, _s, sig, z, as, cs, m, data, h, label,
				hs, f, flen, y, pk, S) == 1, end);

			cp_cmlhs_off(vk, h, label, hs, f, flen, y, pk, S);
			TEST_ASSERT(cp_cmlhs_onv(_r, _s, sig, z, as, cs, m, data, h, vk,
				y, pk, S) == 1, end);
		}
		TEST_END;

		char *ls[L] = { NULL };
		dig_t ft[S];

		TEST_CASE("simple linear multi-key homomorphic signature is correct") {
			for (int j = 0; j < S; j++) {
				cp_mklhs_gen(sk[j], pk[j]);
				for (int l = 0; l < L; l++) {
					ls[l] = "l";
					bn_rand_mod(msg[j][l], n);
					cp_mklhs_sig(a[j][l], msg[j][l], data, id[j], ls[l], sk[j]);
				}
			}

			for (int j = 0; j < S; j++) {
				cp_mklhs_fun(d[j], msg[j], f[j], L);
			}

			g1_set_infty(_r);
			for (int j = 0; j < S; j++) {
				cp_mklhs_evl(r[0][j], a[j], f[j], L);
				g1_add(_r, _r, r[0][j]);
			}
			g1_norm(_r, _r);

			bn_zero(m);
			for (int j = 0; j < S; j++) {
				for (int l = 0; l < L; l++) {
					bn_mul_dig(msg[j][l], msg[j][l], f[j][l]);
					bn_add(m, m, msg[j][l]);
					bn_mod(m, m, n);
				}
			}

			TEST_ASSERT(cp_mklhs_ver(_r, m, d, data, id, ls, f, flen, pk, S), end);

			cp_mklhs_off(as, ft, id, ls, f, flen, S);
			TEST_ASSERT(cp_mklhs_onv(_r, m, d, data, id, as, ft, pk, S), end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	  bn_free(n);
	  bn_free(m);
	  g1_free(h);
	  g1_free(_r);
	  g2_free(_s);
	  gt_free(vk);

	  for (int i = 0; i < S; i++) {
		  RLC_FREE(f[i]);
		  for (int j = 0; j < RLC_TERMS; j++) {
			  gt_free(hs[i][j]);
		  }
		  RLC_FREE(hs[i]);
		  for (int j = 0; j < L; j++) {
			  bn_free(x[i][j]);
			  bn_free(msg[i][j]);
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
	return code;
}

#endif /* WITH_PC */

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the CP module", 0);

#if defined(WITH_BN)
	util_banner("Protocols based on integer factorization:\n", 0);

	if (rsa() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (rabin() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (benaloh() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (paillier() != RLC_OK) {
		core_clean();
		return 1;
	}

#endif

#if defined(WITH_EC)
	util_banner("Protocols based on elliptic curves:\n", 0);
	if (ec_param_set_any() == RLC_OK) {

		if (ecdh() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (ecmqv() != RLC_OK) {
			core_clean();
			return 1;
		}
#if defined(WITH_BC)
		if (ecies() != RLC_OK) {
			core_clean();
			return 1;
		}
#endif

		if (ecdsa() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (ecss() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (vbnn() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pok() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (sok() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (ers() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (etrs() != RLC_OK) {
			core_clean();
			return 1;
		}
	}
#endif

#if defined(WITH_PC)
	util_banner("Protocols based on pairings:\n", 0);
	if (pc_param_set_any() == RLC_OK) {

		if (pdpub() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pdprv() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (sokaka() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (ibe() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (bgn() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (bls() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (bbs() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (cls() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pss() != RLC_OK) {
			core_clean();
			return 1;
		}

#if defined(WITH_MPC)
		if (mpss() != RLC_OK) {
			core_clean();
			return 1;
		}
#endif

		if (zss() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (lhs() != RLC_OK) {
			core_clean();
			return 1;
		}
	}
#endif

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
