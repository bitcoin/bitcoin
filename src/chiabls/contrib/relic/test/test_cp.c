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
 * Tests for implementation of cryptographic protocols.
 *
 * @version $Id$
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"

static int rsa(void) {
	int code = STS_ERR;
	rsa_t pub, prv;
	uint8_t in[10], out[RELIC_BN_BITS / 8 + 1], h[MD_LEN];
	int il, ol;
	int result;

	rsa_null(pub);
	rsa_null(prv);

	TRY {
		rsa_new(pub);
		rsa_new(prv);

		result = cp_rsa_gen(pub, prv, RELIC_BN_BITS);

		TEST_BEGIN("rsa encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);
			il = 10;
			ol = RELIC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_enc(out, &ol, in, il, pub) == STS_OK, end);
			TEST_ASSERT(cp_rsa_dec(out, &ol, out, ol, prv) == STS_OK, end);
			TEST_ASSERT(memcmp(in, out, ol) == 0, end);
		} TEST_END;

#if CP_RSA == BASIC || !defined(STRIP)
		result = cp_rsa_gen_basic(pub, prv, RELIC_BN_BITS);

		TEST_BEGIN("basic rsa encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);
			il = 10;
			ol = RELIC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_enc(out, &ol, in, il, pub) == STS_OK, end);
			TEST_ASSERT(cp_rsa_dec_basic(out, &ol, out, ol, prv) == STS_OK,
					end);
			TEST_ASSERT(memcmp(in, out, ol) == 0, end);
		} TEST_END;
#endif

#if CP_RSA == QUICK || !defined(STRIP)
		result = cp_rsa_gen_quick(pub, prv, RELIC_BN_BITS);

		TEST_BEGIN("fast rsa encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);
			il = 10;
			ol = RELIC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_enc(out, &ol, in, il, pub) == STS_OK, end);
			TEST_ASSERT(cp_rsa_dec_quick(out, &ol, out, ol, prv) == STS_OK,
					end);
			TEST_ASSERT(memcmp(in, out, ol) == 0, end);
		} TEST_END;
#endif

		result = cp_rsa_gen(pub, prv, RELIC_BN_BITS);

		TEST_BEGIN("rsa signature/verification is correct") {
			TEST_ASSERT(result == STS_OK, end);
			il = 10;
			ol = RELIC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_sig(out, &ol, in, il, 0, prv) == STS_OK, end);
			TEST_ASSERT(cp_rsa_ver(out, ol, in, il, 0, pub) == 1, end);
			md_map(h, in, il);
			TEST_ASSERT(cp_rsa_sig(out, &ol, h, MD_LEN, 1, prv) == STS_OK, end);
			TEST_ASSERT(cp_rsa_ver(out, ol, h, MD_LEN, 1, pub) == 1, end);
		} TEST_END;

#if CP_RSA == BASIC || !defined(STRIP)
		result = cp_rsa_gen_basic(pub, prv, RELIC_BN_BITS);

		TEST_BEGIN("basic rsa signature/verification is correct") {
			TEST_ASSERT(result == STS_OK, end);
			il = 10;
			ol = RELIC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_sig_basic(out, &ol, in, il, 0, prv) == STS_OK,
					end);
			TEST_ASSERT(cp_rsa_ver(out, ol, in, il, 0, pub) == 1, end);
			md_map(h, in, il);
			TEST_ASSERT(cp_rsa_sig_basic(out, &ol, h, MD_LEN, 1, prv) == STS_OK,
					end);
			TEST_ASSERT(cp_rsa_ver(out, ol, h, MD_LEN, 1, pub) == 1, end);
		} TEST_END;
#endif

#if CP_RSA == QUICK || !defined(STRIP)
		result = cp_rsa_gen_quick(pub, prv, RELIC_BN_BITS);

		TEST_BEGIN("fast rsa signature/verification is correct") {
			TEST_ASSERT(result == STS_OK, end);
			il = 10;
			ol = RELIC_BN_BITS / 8 + 1;
			rand_bytes(in, il);
			TEST_ASSERT(cp_rsa_sig_quick(out, &ol, in, il, 0, prv) == STS_OK,
					end);
			TEST_ASSERT(cp_rsa_ver(out, ol, in, il, 0, pub) == 1, end);
			md_map(h, in, il);
			TEST_ASSERT(cp_rsa_sig_quick(out, &ol, h, MD_LEN, 1, prv) == STS_OK,
					end);
			TEST_ASSERT(cp_rsa_ver(out, ol, h, MD_LEN, 1, pub) == 1, end);
		} TEST_END;
#endif
	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	rsa_free(pub);
	rsa_free(prv);
	return code;
}

static int rabin(void) {
	int code = STS_ERR;
	rabin_t pub, prv;
	uint8_t in[10];
	uint8_t out[RELIC_BN_BITS / 8 + 1];
	int in_len, out_len;
	int result;

	rabin_null(pub);
	rabin_null(prv);

	TRY {
		rabin_new(pub);
		rabin_new(prv);

		result = cp_rabin_gen(pub, prv, RELIC_BN_BITS);

		TEST_BEGIN("rabin encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);
			in_len = 10;
			out_len = RELIC_BN_BITS / 8 + 1;
			rand_bytes(in, in_len);
			TEST_ASSERT(cp_rabin_enc(out, &out_len, in, in_len, pub) == STS_OK,
					end);
			TEST_ASSERT(cp_rabin_dec(out, &out_len, out, out_len,
							prv) == STS_OK, end);
			TEST_ASSERT(memcmp(in, out, out_len) == 0, end);
		} TEST_END;
	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	rabin_free(pub);
	rabin_free(prv);
	return code;
}

static int benaloh(void) {
	int code = STS_ERR;
	bdpe_t pub, prv;
	bn_t a, b;
	dig_t in, out;
	uint8_t buf[RELIC_BN_BITS / 8 + 1];
	int len;
	int result;

	bn_null(a);
	bn_null(b);
	bdpe_null(pub);
	bdpe_null(prv);

	TRY {
		bn_new(a);
		bn_new(b);
		bdpe_new(pub);
		bdpe_new(prv);

		result = cp_bdpe_gen(pub, prv, bn_get_prime(47), RELIC_BN_BITS);

		TEST_BEGIN("benaloh encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);
			len = RELIC_BN_BITS / 8 + 1;
			rand_bytes(buf, 1);
			in = buf[0] % bn_get_prime(47);
			TEST_ASSERT(cp_bdpe_enc(buf, &len, in, pub) == STS_OK, end);
			TEST_ASSERT(cp_bdpe_dec(&out, buf, len, prv) == STS_OK, end);
			TEST_ASSERT(in == out, end);
		} TEST_END;

		TEST_BEGIN("benaloh encryption/decryption is homomorphic") {
			TEST_ASSERT(result == STS_OK, end);
			len = RELIC_BN_BITS / 8 + 1;
			rand_bytes(buf, 1);
			in = buf[0] % bn_get_prime(47);
			TEST_ASSERT(cp_bdpe_enc(buf, &len, in, pub) == STS_OK, end);
			bn_read_bin(a, buf, len);
			rand_bytes(buf, 1);
			out = (buf[0] % bn_get_prime(47));
			in = (in + out) % bn_get_prime(47);
			TEST_ASSERT(cp_bdpe_enc(buf, &len, out, pub) == STS_OK, end);
			bn_read_bin(b, buf, len);
			bn_mul(a, a, b);
			bn_mod(a, a, pub->n);
			len = bn_size_bin(pub->n);
			bn_write_bin(buf, len, a);
			TEST_ASSERT(cp_bdpe_dec(&out, buf, len, prv) == STS_OK, end);
			TEST_ASSERT(in == out, end);
		} TEST_END;
	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(a);
	bn_free(b);
	bdpe_free(pub);
	bdpe_free(prv);
	return code;
}

static int paillier(void) {
	int code = STS_ERR;
	bn_t a, b, c, d, n, l, s;
	uint8_t in[RELIC_BN_BITS / 8 + 1], out[RELIC_BN_BITS / 8 + 1];
	int in_len, out_len;
	int result;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);
	bn_null(n);
	bn_null(l);
	bn_null(s);

	TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);
		bn_new(n);
		bn_new(l);
		bn_new(s);

		result = cp_phpe_gen(n, l, RELIC_BN_BITS / 2);

		TEST_BEGIN("paillier encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);
			in_len = bn_size_bin(n);
			out_len = RELIC_BN_BITS / 8 + 1;
			memset(in, 0, sizeof(in));
			rand_bytes(in + (in_len - 10), 10);
			TEST_ASSERT(cp_phpe_enc(out, &out_len, in, in_len, n) == STS_OK,
					end);
			TEST_ASSERT(cp_phpe_dec(out, in_len, out, out_len, n, l) == STS_OK,
					end);
			TEST_ASSERT(memcmp(in, out, in_len) == 0, end);
		}
		TEST_END;

		TEST_BEGIN("paillier encryption/decryption is homomorphic") {
			TEST_ASSERT(result == STS_OK, end);
			in_len = bn_size_bin(n);
			out_len = RELIC_BN_BITS / 8 + 1;
			memset(in, 0, sizeof(in));
			rand_bytes(in + (in_len - 10), 10);
			bn_read_bin(a, in, in_len);
			TEST_ASSERT(cp_phpe_enc(out, &out_len, in, in_len, n) == STS_OK,
					end);
			bn_read_bin(b, out, out_len);
			memset(in, 0, sizeof(in));
			rand_bytes(in + (in_len - 10), 10);
			bn_read_bin(c, in, in_len);
			out_len = RELIC_BN_BITS / 8 + 1;
			TEST_ASSERT(cp_phpe_enc(out, &out_len, in, in_len, n) == STS_OK,
					end);
			bn_read_bin(d, out, out_len);
			bn_mul(b, b, d);
			bn_sqr(s, n);
			bn_mod(b, b, s);
			bn_write_bin(out, out_len, b);
			TEST_ASSERT(cp_phpe_dec(out, in_len, out, out_len, n, l) == STS_OK,
					end);
			bn_add(a, a, c);
			bn_write_bin(in, in_len, a);
			bn_read_bin(a, out, in_len);
			TEST_ASSERT(memcmp(in, out, in_len) == 0, end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);
	bn_free(n);
	bn_free(l);
	bn_free(s);
	return code;
}

#if defined(WITH_EC)

#if defined(EP_PLAIN) && FP_PRIME == 160

/* Test vectors taken from SECG GEC 2. */

#define SECG_P160_A		"AA374FFC3CE144E6B073307972CB6D57B2A4E982"
#define SECG_P160_B		"45FB58A92A17AD4B15101C66E74F277E2B460866"
#define SECG_P160_A_X	"51B4496FECC406ED0E75A24A3C03206251419DC0"
#define SECG_P160_A_Y	"C28DCB4B73A514B468D793894F381CCC1756AA6C"
#define SECG_P160_B_X	"49B41E0E9C0369C2328739D90F63D56707C6E5BC"
#define SECG_P160_B_Y	"26E008B567015ED96D232A03111C3EDC0E9C8F83"

uint8_t resultp[] = {
	0x74, 0x4A, 0xB7, 0x03, 0xF5, 0xBC, 0x08, 0x2E, 0x59, 0x18, 0x5F, 0x6D,
	0x04, 0x9D, 0x2D, 0x36, 0x7D, 0xB2, 0x45, 0xC2
};

#endif

#if defined(EB_KBLTZ) && FB_POLYN == 163

/* Test vectors taken from SECG GEC 2. */

#define NIST_K163_A		"3A41434AA99C2EF40C8495B2ED9739CB2155A1E0D"
#define NIST_K163_B		"057E8A78E842BF4ACD5C315AA0569DB1703541D96"
#define NIST_K163_A_X	"37D529FA37E42195F10111127FFB2BB38644806BC"
#define NIST_K163_A_Y	"447026EEE8B34157F3EB51BE5185D2BE0249ED776"
#define NIST_K163_B_X	"72783FAAB9549002B4F13140B88132D1C75B3886C"
#define NIST_K163_B_Y	"5A976794EA79A4DE26E2E19418F097942C08641C7"

uint8_t resultk[] = {
	0x59, 0x79, 0x85, 0x28, 0x08, 0x3F, 0x50, 0xB0, 0x75, 0x28, 0x35, 0x3C,
	0xDA, 0x99, 0xD0, 0xE4, 0x60, 0xA7, 0x22, 0x9D
};

#endif

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
	FETCH(str, CURVE##_A, sizeof(CURVE##_A));								\
	bn_read_str(d_a, str, strlen(str), 16);									\
	FETCH(str, CURVE##_A_X, sizeof(CURVE##_A_X));							\
	fp_read_str(q_a->x, str, strlen(str), 16);									\
	FETCH(str, CURVE##_A_Y, sizeof(CURVE##_A_Y));							\
	fp_read_str(q_a->y, str, strlen(str), 16);									\
	fp_set_dig(q_a->z, 1);													\
	FETCH(str, CURVE##_B, sizeof(CURVE##_B));								\
	bn_read_str(d_b, str, strlen(str), 16);									\
	FETCH(str, CURVE##_B_X, sizeof(CURVE##_B_X));							\
	fp_read_str(q_b->x, str, strlen(str), 16);									\
	FETCH(str, CURVE##_B_Y, sizeof(CURVE##_B_Y));							\
	fp_read_str(q_b->y, str, strlen(str), 16);									\
	fp_set_dig(q_b->z, 1);													\
	q_a->norm = q_b->norm = 1;												\

#define ASSIGNK(CURVE)														\
	FETCH(str, CURVE##_A, sizeof(CURVE##_A));								\
	bn_read_str(d_a, str, strlen(str), 16);									\
	FETCH(str, CURVE##_A_X, sizeof(CURVE##_A_X));							\
	fb_read_str(q_a->x, str, strlen(str), 16);									\
	FETCH(str, CURVE##_A_Y, sizeof(CURVE##_A_Y));							\
	fb_read_str(q_a->y, str, strlen(str), 16);									\
	fb_set_dig(q_a->z, 1);													\
	FETCH(str, CURVE##_B, sizeof(CURVE##_B));								\
	bn_read_str(d_b, str, strlen(str), 16);									\
	FETCH(str, CURVE##_B_X, sizeof(CURVE##_B_X));							\
	fb_read_str(q_b->x, str, strlen(str), 16);									\
	FETCH(str, CURVE##_B_Y, sizeof(CURVE##_B_Y));							\
	fb_read_str(q_b->y, str, strlen(str), 16);									\
	fb_set_dig(q_b->z, 1);													\
	q_a->norm = q_b->norm = 1;												\

static int ecdh(void) {
	int code = STS_ERR;
	char str[2 * FC_BYTES + 1];
	bn_t d_a, d_b;
	ec_t q_a, q_b;
	uint8_t key[MD_LEN], k1[MD_LEN], k2[MD_LEN];

	bn_null(d_a);
	bn_null(d_b);
	ec_null(q_a);
	ec_null(q_b);

	TRY {
		bn_new(d_a);
		bn_new(d_b);
		ec_new(q_a);
		ec_new(q_b);

		TEST_BEGIN("ecdh key agreement is correct") {
			TEST_ASSERT(cp_ecdh_gen(d_a, q_a) == STS_OK, end);
			TEST_ASSERT(cp_ecdh_gen(d_b, q_b) == STS_OK, end);
			TEST_ASSERT(cp_ecdh_key(k1, MD_LEN, d_b, q_a) == STS_OK, end);
			TEST_ASSERT(cp_ecdh_key(k2, MD_LEN, d_a, q_b) == STS_OK, end);
			TEST_ASSERT(memcmp(k1, k2, MD_LEN) == 0, end);
		} TEST_END;

#if MD_MAP == SHONE

		switch (ec_param_get()) {

#if EC_CUR == PRIME

#if defined(EP_PLAIN) && FP_PRIME == 160
			case SECG_P160:
				ASSIGNP(SECG_P160);
				memcpy(key, resultp, MD_LEN);
				break;
#endif

#else /* EC_CUR == CHAR2 */

#if defined(EB_KBLTZ) && FB_POLYN == 163
			case NIST_K163:
				ASSIGNK(NIST_K163);
				memcpy(key, resultk, MD_LEN);
				break;
#endif

#endif
			default:
				code = STS_OK;
				break;
		}

		if (code != STS_OK) {
			TEST_ONCE("ecdh satisfies test vectors") {
				TEST_ASSERT(ec_is_valid(q_a) == 1, end);
				TEST_ASSERT(ec_is_valid(q_b) == 1, end);
				TEST_ASSERT(cp_ecdh_key(k1, MD_LEN, d_b, q_a) == STS_OK, end);
				TEST_ASSERT(cp_ecdh_key(k2, MD_LEN, d_a, q_b) == STS_OK, end);
				TEST_ASSERT(memcmp(k1, key, MD_LEN) == 0, end);
				TEST_ASSERT(memcmp(k2, key, MD_LEN) == 0, end);
			}
			TEST_END;
		}
#endif
		(void)str;
		(void)key;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(d_a);
	bn_free(d_b);
	ec_free(q_a);
	ec_free(q_b);
	return code;
}

static int ecmqv(void) {
	int code = STS_ERR;
	bn_t d1_a, d1_b;
	bn_t d2_a, d2_b;
	ec_t q1_a, q1_b;
	ec_t q2_a, q2_b;
	uint8_t key1[MD_LEN], key2[MD_LEN];

	bn_null(d1_a);
	bn_null(d1_b);
	ec_null(q1_a);
	ec_null(q1_b);
	bn_null(d2_a);
	bn_null(d2_b);
	ec_null(q2_a);
	ec_null(q2_b);

	TRY {
		bn_new(d1_a);
		bn_new(d1_b);
		ec_new(q1_a);
		ec_new(q1_b);
		bn_new(d2_a);
		bn_new(d2_b);
		ec_new(q2_a);
		ec_new(q2_b);

		TEST_BEGIN("ecmqv authenticated key agreement is correct") {
			TEST_ASSERT(cp_ecmqv_gen(d1_a, q1_a) == STS_OK, end);
			TEST_ASSERT(cp_ecmqv_gen(d2_a, q2_a) == STS_OK, end);
			TEST_ASSERT(cp_ecmqv_gen(d1_b, q1_b) == STS_OK, end);
			TEST_ASSERT(cp_ecmqv_gen(d2_b, q2_b) == STS_OK, end);
			TEST_ASSERT(cp_ecmqv_key(key1, MD_LEN, d1_b, d2_b, q2_b, q1_a,
							q2_a) == STS_OK, end);
			TEST_ASSERT(cp_ecmqv_key(key2, MD_LEN, d1_a, d2_a, q2_a, q1_b,
							q2_b) == STS_OK, end);
			TEST_ASSERT(memcmp(key1, key2, MD_LEN) == 0, end);
		} TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(d1_a);
	bn_free(d1_b);
	ec_free(q1_a);
	ec_free(q1_b);
	bn_free(d2_a);
	bn_free(d2_b);
	ec_free(q2_a);
	ec_free(q2_b);
	return code;
}

static int ecies(void) {
	int code = STS_ERR;
	ec_t r;
	bn_t d_a, d_b;
	ec_t q_a, q_b;
	int l, in_len, out_len;
	uint8_t in[BC_LEN - 1], out[BC_LEN + MD_LEN];

	ec_null(r);
	bn_null(d_a);
	bn_null(d_b);
	ec_null(q_a);
	ec_null(q_b);

	TRY {
		ec_new(r);
		bn_new(d_a);
		bn_new(d_b);
		ec_new(q_a);
		ec_new(q_b);

		l = ec_param_level();
		if (l == 128 || l == 192 || l == 256) {
			TEST_BEGIN("ecies encryption/decryption is correct") {
				TEST_ASSERT(cp_ecies_gen(d_a, q_a) == STS_OK, end);
				in_len = BC_LEN - 1;
				out_len = BC_LEN + MD_LEN;
				rand_bytes(in, in_len);
				TEST_ASSERT(cp_ecies_enc(r, out, &out_len, in, in_len, q_a)
						== STS_OK, end);
				TEST_ASSERT(cp_ecies_dec(out, &out_len, r, out, out_len, d_a)
						== STS_OK, end);
				TEST_ASSERT(memcmp(in, out, out_len) == 0, end);
			}
			TEST_END;
		}
#if MD_MAP == SH256
		uint8_t msg[BC_LEN + MD_LEN];
		char str[2 * FC_BYTES + 1];

		switch (ec_param_get()) {

#if defined(EP_PLAIN) && FP_PRIME == 256
			case NIST_P256:
				ASSIGNP(NIST_P256);
				memcpy(msg, result, sizeof(result));
				break;
#endif
			default:
				(void)str;
				code = STS_OK;
				break;
		}

		if (code != STS_OK) {

			TEST_ONCE("ecies satisfies test vectors") {
				uint8_t in[] = {
					0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF
				};
				TEST_ASSERT(ec_is_valid(q_a) == 1, end);
				TEST_ASSERT(ec_is_valid(q_b) == 1, end);
				out_len = 16;
				TEST_ASSERT(cp_ecies_dec(out, &out_len, q_b, msg, sizeof(msg),
								d_a) == STS_OK, end);
				TEST_ASSERT(out_len == sizeof(in), end);
				TEST_ASSERT(memcmp(out, in, sizeof(in)) == STS_OK, end);
				out_len = 16;
				TEST_ASSERT(cp_ecies_dec(out, &out_len, q_a, msg, sizeof(msg),
								d_b) == STS_OK, end);
				TEST_ASSERT(out_len == sizeof(in), end);
				TEST_ASSERT(memcmp(out, in, sizeof(in)) == STS_OK, end);
			}
			TEST_END;
		}
#endif
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	ec_free(r);
	bn_free(d_a);
	bn_free(d_b);
	ec_free(q_a);
	ec_free(q_b);
	return code;
}

static int ecdsa(void) {
	int code = STS_ERR;
	bn_t d, r, s;
	ec_t q;
	uint8_t m[5] = { 0, 1, 2, 3, 4 }, h[MD_LEN];

	bn_null(d);
	bn_null(r);
	bn_null(s);
	ec_null(q);

	TRY {
		bn_new(d);
		bn_new(r);
		bn_new(s);
		ec_new(q);

		TEST_BEGIN("ecdsa signature is correct") {
			TEST_ASSERT(cp_ecdsa_gen(d, q) == STS_OK, end);
			TEST_ASSERT(cp_ecdsa_sig(r, s, m, sizeof(m), 0, d) == STS_OK, end);
			TEST_ASSERT(cp_ecdsa_ver(r, s, m, sizeof(m), 0, q) == 1, end);
			md_map(h, m, sizeof(m));
			TEST_ASSERT(cp_ecdsa_sig(r, s, h, MD_LEN, 1, d) == STS_OK, end);
			TEST_ASSERT(cp_ecdsa_ver(r, s, h, MD_LEN, 1, q) == 1, end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(d);
	bn_free(r);
	bn_free(s);
	ec_free(q);
	return code;
}

static int ecss(void) {
	int code = STS_ERR;
	bn_t d, r;
	ec_t q;
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(d);
	bn_null(r);
	ec_null(q);

	TRY {
		bn_new(d);
		bn_new(r);
		ec_new(q);

		TEST_BEGIN("ecss signature is correct") {
			TEST_ASSERT(cp_ecss_gen(d, q) == STS_OK, end);
			TEST_ASSERT(cp_ecss_sig(r, d, m, sizeof(m), d) == STS_OK, end);
			TEST_ASSERT(cp_ecss_ver(r, d, m, sizeof(m), q) == 1, end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(d);
	bn_free(r);
	ec_free(q);
	return code;
}

static int vbnn_ibs(void) {
	int code = STS_ERR;

	vbnn_ibs_kgc_t kgc;

	uint8_t userA_id[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	vbnn_ibs_user_t userA;

	uint8_t userB_id[] = { 5, 6, 7, 8, 9, 0, 1, 2, 3, 4 };
	vbnn_ibs_user_t userB;

	uint8_t message[] = "Thrice the brinded cat hath mew'd.";

	ec_t sig_R;
	bn_t sig_z;
	bn_t sig_h;

	vbnn_ibs_kgc_null(kgc);

	vbnn_ibs_user_null(userA);
	vbnn_ibs_user_null(userB);

	ec_null(sig_R);
	bn_null(sig_z);
	bn_null(sig_h);

	TRY {
		vbnn_ibs_kgc_new(kgc);

		vbnn_ibs_user_new(userA);
		vbnn_ibs_user_new(userB);

		ec_new(sig_R);
		bn_new(sig_z);
		bn_new(sig_h);

		TEST_BEGIN("vbnn_ibs is correct") {
			TEST_ASSERT(cp_vbnn_ibs_kgc_gen(kgc) == STS_OK, end);
			TEST_ASSERT(cp_vbnn_ibs_kgc_extract_key(userA, kgc, userA_id, sizeof(userA_id)) == STS_OK, end);
			TEST_ASSERT(cp_vbnn_ibs_kgc_extract_key(userB, kgc, userB_id, sizeof(userB_id)) == STS_OK, end);
			TEST_ASSERT(cp_vbnn_ibs_user_sign(sig_R, sig_z, sig_h, userA_id, sizeof(userA_id), message, sizeof(message), userA) == STS_OK, end);
			TEST_ASSERT(cp_vbnn_ibs_user_verify(sig_R, sig_z, sig_h, userA_id, sizeof(userA_id), message, sizeof(message), kgc->mpk) == 1, end);
			TEST_ASSERT(cp_vbnn_ibs_user_verify(sig_R, sig_z, sig_h, userB_id, sizeof(userB_id), message, sizeof(message), kgc->mpk) == 0, end);
			TEST_ASSERT(cp_vbnn_ibs_user_sign(sig_R, sig_z, sig_h, userA_id, sizeof(userA_id), message, sizeof(message), userB) == STS_OK, end);
			TEST_ASSERT(cp_vbnn_ibs_user_verify(sig_R, sig_z, sig_h, userA_id, sizeof(userA_id), message, sizeof(message), kgc->mpk) == 0, end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

end:
	ec_free(sig_R);
	bn_free(sig_z);
	bn_free(sig_h);

	vbnn_ibs_kgc_free(kgc);
	vbnn_ibs_user_free(userA);
	vbnn_ibs_user_free(userB);
	return code;
}

#endif

#if defined(WITH_PC)

static int sokaka(void) {
	int code = STS_ERR, l = MD_LEN;
	sokaka_t k;
	bn_t s;
	uint8_t k1[MD_LEN], k2[MD_LEN];
	char i_a[5] = { 'A', 'l', 'i', 'c', 'e' };
	char i_b[3] = { 'B', 'o', 'b' };

	sokaka_null(k);
	bn_null(s);

	TRY {
		sokaka_new(k);
		bn_new(s);

		cp_sokaka_gen(s);

		TEST_BEGIN
				("sakai-ohgishi-kasahara authenticated key agreement is correct")
		{
			TEST_ASSERT(cp_sokaka_gen_prv(k, i_a, 5, s) == STS_OK, end);
			TEST_ASSERT(cp_sokaka_key(k1, l, i_a, 5, k, i_b, 3) == STS_OK, end);
			TEST_ASSERT(cp_sokaka_gen_prv(k, i_b, 3, s) == STS_OK, end);
			TEST_ASSERT(cp_sokaka_key(k2, l, i_b, 3, k, i_a, 5) == STS_OK, end);
			TEST_ASSERT(memcmp(k1, k2, l) == 0, end);
		} TEST_END;

	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	sokaka_free(k);
	bn_free(s);
	return code;
}

static int ibe(void) {
	int code = STS_ERR;
	bn_t s;
	g1_t pub;
	g2_t prv;
	uint8_t in[10], out[10 + 2 * FP_BYTES + 1];
	char id[5] = { 'A', 'l', 'i', 'c', 'e' };
	int il, ol;
	int result;

	bn_null(s);
	g1_null(pub);
	g2_null(prv);

	TRY {
		bn_new(s);
		g1_new(pub);
		g2_new(prv);

		result = cp_ibe_gen(s, pub);

		TEST_BEGIN("boneh-franklin identity-based encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);
			il = ol = 10;
			ol += 1 + 2 * FP_BYTES;
			rand_bytes(in, il);
			TEST_ASSERT(cp_ibe_gen_prv(prv, id, 5, s) == STS_OK, end);
			TEST_ASSERT(cp_ibe_enc(out, &ol, in, il, id, 5, pub) == STS_OK, end);
			TEST_ASSERT(cp_ibe_dec(out, &il, out, ol, prv) == STS_OK, end);
			TEST_ASSERT(memcmp(in, out, il) == 0, end);
		} TEST_END;
	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(s);
	g1_free(pub);
	g2_free(prv);
	return code;
}

static int bgn(void) {
	int result, code = STS_ERR;
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

	TRY {
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

		TEST_BEGIN("boneh-go-nissim encryption/decryption is correct") {
			TEST_ASSERT(result == STS_OK, end);

			rand_bytes((unsigned char *)&in, sizeof(dig_t));
			in = in % 11;

			TEST_ASSERT(cp_bgn_enc1(c, in, pub) == STS_OK, end);
			TEST_ASSERT(cp_bgn_dec1(&out, c, prv) == STS_OK, end);
			TEST_ASSERT(in == out, end);
			TEST_ASSERT(cp_bgn_enc2(e, in, pub) == STS_OK, end);
			TEST_ASSERT(cp_bgn_dec2(&out, e, prv) == STS_OK, end);
			TEST_ASSERT(in == out, end);
		} TEST_END;

		TEST_BEGIN("boneh-go-nissim encryption is additively homomorphic") {
			rand_bytes((unsigned char *)&in, sizeof(dig_t));
			in = in % 11;
			out = in % 7;
			TEST_ASSERT(cp_bgn_enc1(c, in, pub) == STS_OK, end);
			TEST_ASSERT(cp_bgn_enc1(d, out, pub) == STS_OK, end);
			g1_add(c[0], c[0], d[0]);
			g1_add(c[1], c[1], d[1]);
			g1_norm(c[0], c[0]);
			g1_norm(c[1], c[1]);
			TEST_ASSERT(cp_bgn_dec1(&t, c, prv) == STS_OK, end);
			TEST_ASSERT(in + out == t, end);
			TEST_ASSERT(cp_bgn_enc2(e, in, pub) == STS_OK, end);
			TEST_ASSERT(cp_bgn_enc2(f, out, pub) == STS_OK, end);
			g2_add(e[0], e[0], f[0]);
			g2_add(e[1], e[1], f[1]);
			g2_norm(e[0], e[0]);
			g2_norm(e[1], e[1]);
			TEST_ASSERT(cp_bgn_dec2(&t, e, prv) == STS_OK, end);
			TEST_ASSERT(in + out == t, end);
		} TEST_END;

		TEST_BEGIN("boneh-go-nissim encryption is multiplicatively homomorphic") {
			rand_bytes((unsigned char *)&in, sizeof(dig_t));
			in = in % 11;
			out = in % 17;
			TEST_ASSERT(cp_bgn_enc1(c, in, pub) == STS_OK, end);
			TEST_ASSERT(cp_bgn_enc2(e, out, pub) == STS_OK, end);
			in = in * out;
			TEST_ASSERT(cp_bgn_mul(g, c, e) == STS_OK, end);
			TEST_ASSERT(cp_bgn_dec(&t, g, prv) == STS_OK, end);
			TEST_ASSERT(in == t, end);
			TEST_ASSERT(cp_bgn_add(g, g, g) == STS_OK, end);
			TEST_ASSERT(cp_bgn_dec(&t, g, prv) == STS_OK, end);
			TEST_ASSERT(in + in == t, end);
		} TEST_END;

	} CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

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
	int code = STS_ERR;
	bn_t d;
	g1_t s;
	g2_t q;
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(d);
	g1_null(s);
	g2_null(q);

	TRY {
		bn_new(d);
		g1_new(s);
		g2_new(q);

		TEST_BEGIN("boneh-lynn-schacham short signature is correct") {
			TEST_ASSERT(cp_bls_gen(d, q) == STS_OK, end);
			TEST_ASSERT(cp_bls_sig(s, m, sizeof(m), d) == STS_OK, end);
			TEST_ASSERT(cp_bls_ver(s, m, sizeof(m), q) == 1, end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(d);
	g1_free(s);
	g2_free(q);
	return code;
}

static int bbs(void) {
	int code = STS_ERR;
	bn_t d;
	g1_t s;
	g2_t q;
	gt_t z;
	uint8_t m[5] = { 0, 1, 2, 3, 4 }, h[MD_LEN];

	bn_null(d);
	g1_null(s);
	g2_null(q);
	gt_null(z);

	TRY {
		bn_new(d);
		g1_new(s);
		g2_new(q);
		gt_new(z);

		TEST_BEGIN("boneh-boyen short signature is correct") {
			TEST_ASSERT(cp_bbs_gen(d, q, z) == STS_OK, end);
			TEST_ASSERT(cp_bbs_sig(s, m, sizeof(m), 0, d) == STS_OK, end);
			TEST_ASSERT(cp_bbs_ver(s, m, sizeof(m), 0, q, z) == 1, end);
			md_map(h, m, sizeof(m));
			TEST_ASSERT(cp_bbs_sig(s, m, sizeof(m), 1, d) == STS_OK, end);
			TEST_ASSERT(cp_bbs_ver(s, m, sizeof(m), 1, q, z) == 1, end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(d);
	g1_free(s);
	g2_free(q);
	gt_free(z);
	return code;
}

static int zss(void) {
	int code = STS_ERR;
	bn_t d;
	g1_t q;
	g2_t s;
	gt_t z;
	uint8_t m[5] = { 0, 1, 2, 3, 4 }, h[MD_LEN];

	bn_null(d);
	g1_null(q);
	g2_null(s);
	gt_null(z);

	TRY {
		bn_new(d);
		g1_new(q);
		g2_new(s);
		gt_new(z);

		TEST_BEGIN("zhang-safavi-naini-susilo signature is correct") {
			TEST_ASSERT(cp_zss_gen(d, q, z) == STS_OK, end);
			TEST_ASSERT(cp_zss_sig(s, m, sizeof(m), 0, d) == STS_OK, end);
			TEST_ASSERT(cp_zss_ver(s, m, sizeof(m), 0, q, z) == 1, end);
			md_map(h, m, sizeof(m));
			TEST_ASSERT(cp_zss_sig(s, m, sizeof(m), 1, d) == STS_OK, end);
			TEST_ASSERT(cp_zss_ver(s, m, sizeof(m), 1, q, z) == 1, end);
		}
		TEST_END;
	}
	CATCH_ANY {
		ERROR(end);
	}
	code = STS_OK;

  end:
	bn_free(d);
	g1_free(q);
	g2_free(s);
	gt_free(z);
	return code;
}

#endif

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the CP module", 0);

#if defined(WITH_BN)
	util_banner("Protocols based on integer factorization:\n", 0);

	if (rsa() != STS_OK) {
		core_clean();
		return 1;
	}

	if (rabin() != STS_OK) {
		core_clean();
		return 1;
	}

	if (benaloh() != STS_OK) {
		core_clean();
		return 1;
	}

	if (paillier() != STS_OK) {
		core_clean();
		return 1;
	}

#endif

#if defined(WITH_EC)
	util_banner("Protocols based on elliptic curves:\n", 0);
	if (ec_param_set_any() == STS_OK) {

		if (ecdh() != STS_OK) {
			core_clean();
			return 1;
		}

		if (ecmqv() != STS_OK) {
			core_clean();
			return 1;
		}
#if defined(WITH_BC)
		if (ecies() != STS_OK) {
			core_clean();
			return 1;
		}
#endif

		if (ecdsa() != STS_OK) {
			core_clean();
			return 1;
		}

		if (ecss() != STS_OK) {
			core_clean();
			return 1;
		}

		if (vbnn_ibs() != STS_OK) {
			core_clean();
			return 1;
		}

	} else {
		THROW(ERR_NO_CURVE);
	}
#endif

#if defined(WITH_PC)
	util_banner("Protocols based on pairings:\n", 0);
	if (pc_param_set_any() == STS_OK) {

		if (sokaka() != STS_OK) {
			core_clean();
			return 1;
		}

		if (ibe() != STS_OK) {
			core_clean();
			return 1;
		}

		if (bgn() != STS_OK) {
			core_clean();
			return 1;
		}


		if (bls() != STS_OK) {
			core_clean();
			return 1;
		}

		if (bbs() != STS_OK) {
			core_clean();
			return 1;
		}

		if (zss() != STS_OK) {
			core_clean();
			return 1;
		}

	} else {
		THROW(ERR_NO_CURVE);
	}
#endif

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
