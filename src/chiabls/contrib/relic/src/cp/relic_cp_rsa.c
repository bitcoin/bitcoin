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
 * Implementation of the RSA cryptosystem.
 *
 * @ingroup cp
 */

#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_rand.h"
#include "relic_bn.h"
#include "relic_util.h"
#include "relic_cp.h"
#include "relic_md.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Length of chosen padding scheme.
 */
#if CP_RSAPD == PKCS1
#define RSA_PAD_LEN		(11)
#elif CP_RSAPD == PKCS2
#define RSA_PAD_LEN		(2 * MD_LEN + 2)
#else
#define RSA_PAD_LEN		(2)
#endif

/**
 * Identifier for encrypted messages.
 */
#define RSA_PUB			(02)

/**
 * Identifier for signed messages.
 */
#define RSA_PRV			(01)

/**
 * Byte used as padding unit.
 */
#define RSA_PAD			(0xFF)

/**
 * Byte used as padding unit in PSS signatures.
 */
#define RSA_PSS			(0xBC)

/**
 * Identifier for encryption.
 */
#define RSA_ENC				1

/**
 * Identifier for decryption.
 */
#define RSA_DEC				2

/**
 * Identifier for signature.
 */
#define RSA_SIG				3

/**
 * Identifier for verification.
 */
#define RSA_VER				4

/**
 * Identifier for second encryption step.
 */
#define RSA_ENC_FIN			5

/**
 * Identifier for second sining step.
 */
#define RSA_SIG_FIN			6

/**
 * Identifier for signature of a precomputed hash.
 */
#define RSA_SIG_HASH		7

/**
 * Identifier for verification of a precomputed hash.
 */
#define RSA_VER_HASH		8

#if CP_RSAPD == BASIC

/**
 * Applies or removes simple encryption padding.
 *
 * @param[out] m		- the buffer to pad.
 * @param[out] p_len	- the number of added pad bytes.
 * @param[in] m_len		- the message length in bytes.
 * @param[in] k_len		- the key length in bytes.
 * @param[in] operation	- flag to indicate the operation type.
 * @return STS_ERR if errors occurred, STS_OK otherwise.
 */
static int pad_basic(bn_t m, int *p_len, int m_len, int k_len, int operation) {
	uint8_t pad = 0;
	int result = STS_OK;
	bn_t t;

	TRY {
		bn_null(t);
		bn_new(t);

		switch (operation) {
			case RSA_ENC:
			case RSA_SIG:
			case RSA_SIG_HASH:
				/* EB = 00 | FF | D. */
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PAD);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_DEC:
			case RSA_VER:
			case RSA_VER_HASH:
				/* EB = 00 | FF | D. */
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = STS_ERR;
				}
				*p_len = 1;
				do {
					(*p_len)++;
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad == 0);
				if (pad != RSA_PAD) {
					result = STS_ERR;
				}
				bn_mod_2b(m, m, (k_len - *p_len) * 8);
				break;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(t);
	}
	return result;
}

#endif

#if CP_RSAPD == PKCS1

/**
 * ASN.1 identifier of the hash function SHA-1.
 */
static const uint8_t shone_id[] =
		{ 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a,
			0x05, 0x00, 0x04, 0x14 };

/**
 * ASN.1 identifier of the hash function SHA-224.
 */
static const uint8_t sh224_id[] =
		{ 0x30, 0x2d, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65,
			0x03, 0x04, 0x02, 0x04, 0x05, 0x00, 0x04, 0x1c };

/**
 * ASN.1 identifier of the hash function SHA-256.
 */
static const uint8_t sh256_id[] =
		{ 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65,
			0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20 };

/**
 * ASN.1 identifier of the hash function SHA-384.
 */
static const uint8_t sh384_id[] =
		{ 0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65,
			0x03, 0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30 };

/**
 * ASN.1 identifier of the hash function SHA-512.
 */
static const uint8_t sh512_id[] =
		{ 0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65,
			0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40 };

/**
 * Returns a pointer to the ASN.1 identifier of a hash function according to the
 * PKCS#1 v1.5 padding standard.
 *
 * @param[in] md			- the hash function.
 * @param[in, out] len		- the length of the identifier.
 * @return The pointer to the hash function identifier.
 */
static uint8_t *hash_id(int md, int *len) {
	switch (md) {
		case SHONE:
			*len = sizeof(shone_id);
			return (uint8_t *)shone_id;
		case SH224:
			*len = sizeof(sh224_id);
			return (uint8_t *)sh224_id;
		case SH256:
			*len = sizeof(sh256_id);
			return (uint8_t *)sh256_id;
		case SH384:
			*len = sizeof(sh384_id);
			return (uint8_t *)sh384_id;
		case SH512:
			*len = sizeof(sh512_id);
			return (uint8_t *)sh512_id;
		default:
			THROW(ERR_NO_VALID);
			return NULL;
	}
}

/**
 * Applies or removes a PKCS#1 v1.5 encryption padding.
 *
 * @param[out] m		- the buffer to pad.
 * @param[out] p_len	- the number of added pad bytes.
 * @param[in] m_len		- the message length in bytes.
 * @param[in] k_len		- the key length in bytes.
 * @param[in] operation	- flag to indicate the operation type.
 * @return STS_ERR if errors occurred, STS_OK otherwise.
 */
static int pad_pkcs1(bn_t m, int *p_len, int m_len, int k_len, int operation) {
	uint8_t *id, pad = 0;
	int len, result = STS_OK;
	bn_t t;

	bn_null(t);

	TRY {
		bn_new(t);

		switch (operation) {
			case RSA_ENC:
				/* EB = 00 | 02 | PS | 00 | D. */
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PUB);

				*p_len = k_len - 3 - m_len;
				for (int i = 0; i < *p_len; i++) {
					bn_lsh(m, m, 8);
					do {
						rand_bytes(&pad, 1);
					} while (pad == 0);
					bn_add_dig(m, m, pad);
				}
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_DEC:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = STS_ERR;
				}

				*p_len = m_len;
				m_len--;
				bn_rsh(t, m, 8 * m_len);
				pad = (uint8_t)t->dp[0];
				if (pad != RSA_PUB) {
					result = STS_ERR;
				}
				do {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad != 0);
				/* Remove padding and trailing zero. */
				*p_len -= (m_len - 1);
				bn_mod_2b(m, m, (k_len - *p_len) * 8);
				break;
			case RSA_SIG:
				/* EB = 00 | 01 | PS | 00 | D. */
				id = hash_id(MD_MAP, &len);
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PRV);

				*p_len = k_len - 3 - m_len - len;
				for (int i = 0; i < *p_len; i++) {
					bn_lsh(m, m, 8);
					bn_add_dig(m, m, RSA_PAD);
				}
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0);
				bn_lsh(m, m, 8 * len);
				bn_read_bin(t, id, len);
				bn_add(m, m, t);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_SIG_HASH:
				/* EB = 00 | 01 | PS | 00 | D. */
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PRV);

				*p_len = k_len - 3 - m_len;
				for (int i = 0; i < *p_len; i++) {
					bn_lsh(m, m, 8);
					bn_add_dig(m, m, RSA_PAD);
				}
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_VER:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = STS_ERR;
				}
				m_len--;
				bn_rsh(t, m, 8 * m_len);
				pad = (uint8_t)t->dp[0];
				if (pad != RSA_PRV) {
					result = STS_ERR;
				}
				do {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad != 0 && m_len > 0);
				if (m_len == 0) {
					result = STS_ERR;
				}
				/* Remove padding and trailing zero. */
				id = hash_id(MD_MAP, &len);
				m_len -= len;

				bn_rsh(t, m, m_len * 8);
				int r = 0;
				for (int i = 0; i < len; i++) {
					pad = (uint8_t)t->dp[0];
					r |= pad - id[len - i - 1];
					bn_rsh(t, t, 8);
				}
				*p_len = k_len - m_len;
				bn_mod_2b(m, m, m_len * 8);
				result = (r == 0 ? STS_OK : STS_ERR);
				break;
			case RSA_VER_HASH:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = STS_ERR;
				}
				m_len--;
				bn_rsh(t, m, 8 * m_len);
				pad = (uint8_t)t->dp[0];
				if (pad != RSA_PRV) {
					result = STS_ERR;
				}
				do {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad != 0 && m_len > 0);
				if (m_len == 0) {
					result = STS_ERR;
				}
				/* Remove padding and trailing zero. */
				*p_len = k_len - m_len;
				bn_mod_2b(m, m, m_len * 8);
				break;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(t);
	}
	return result;
}

#endif

#if CP_RSAPD == PKCS2

/**
 * Applies or removes a PKCS#1 v2.1 encryption padding.
 *
 * @param[out] m		- the buffer to pad.
 * @param[out] p_len	- the number of added pad bytes.
 * @param[in] m_len		- the message length in bytes.
 * @param[in] k_len		- the key length in bytes.
 * @param[in] operation	- flag to indicate the operation type.
 * @return STS_ERR if errors occurred, STS_OK otherwise.
 */
static int pad_pkcs2(bn_t m, int *p_len, int m_len, int k_len, int operation) {
	uint8_t pad, h1[MD_LEN], h2[MD_LEN], mask[k_len];
	int result = STS_OK;
	bn_t t;

	bn_null(t);

	TRY {
		bn_new(t);

		switch (operation) {
			case RSA_ENC:
				/* DB = lHash | PS | 01 | D. */
				md_map(h1, NULL, 0);
				bn_read_bin(m, h1, MD_LEN);
				*p_len = k_len - 2 * MD_LEN - 2 - m_len;
				bn_lsh(m, m, *p_len * 8);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0x01);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_ENC_FIN:
				/* EB = 00 | maskedSeed | maskedDB. */
				rand_bytes(h1, MD_LEN);
				md_mgf1(mask, k_len - MD_LEN - 1, h1, MD_LEN);
				bn_read_bin(t, mask, k_len - MD_LEN - 1);
				for (int i = 0; i < t->used; i++) {
					m->dp[i] ^= t->dp[i];
				}
				bn_write_bin(mask, k_len - MD_LEN - 1, m);
				md_mgf1(h2, MD_LEN, mask, k_len - MD_LEN - 1);
				for (int i = 0; i < MD_LEN; i++) {
					h1[i] ^= h2[i];
				}
				bn_read_bin(t, h1, MD_LEN);
				bn_lsh(t, t, 8 * (k_len - MD_LEN - 1));
				bn_add(t, t, m);
				bn_copy(m, t);
				break;
			case RSA_DEC:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = STS_ERR;
				}
				m_len -= MD_LEN;
				bn_rsh(t, m, 8 * m_len);
				bn_write_bin(h1, MD_LEN, t);
				bn_mod_2b(m, m, 8 * m_len);
				bn_write_bin(mask, m_len, m);
				md_mgf1(h2, MD_LEN, mask, m_len);
				for (int i = 0; i < MD_LEN; i++) {
					h1[i] ^= h2[i];
				}
				md_mgf1(mask, k_len - MD_LEN - 1, h1, MD_LEN);
				bn_read_bin(t, mask, k_len - MD_LEN - 1);
				for (int i = 0; i < t->used; i++) {
					m->dp[i] ^= t->dp[i];
				}
				m_len -= MD_LEN;
				bn_rsh(t, m, 8 * m_len);
				bn_write_bin(h2, MD_LEN, t);
				md_map(h1, NULL, 0);
				pad = 0;
				for (int i = 0; i < MD_LEN; i++) {
					pad |= h1[i] - h2[i];
				}
				if (result == STS_OK) {
					result = (pad ? STS_ERR : STS_OK);
				}
				bn_mod_2b(m, m, 8 * m_len);
				*p_len = bn_size_bin(m);
				(*p_len)--;
				bn_rsh(t, m, *p_len * 8);
				if (bn_cmp_dig(t, 1) != CMP_EQ) {
					result = STS_ERR;
				}
				bn_mod_2b(m, m, *p_len * 8);
				*p_len = k_len - *p_len;
				break;
			case RSA_SIG:
			case RSA_SIG_HASH:
				/* M' = 00 00 00 00 00 00 00 00 | H(M). */
				bn_zero(m);
				bn_lsh(m, m, 64);
				/* Make room for the real message. */
				bn_lsh(m, m, MD_LEN * 8);
				break;
			case RSA_SIG_FIN:
				memset(mask, 0, 8);
				bn_write_bin(mask + 8, MD_LEN, m);
				md_map(h1, mask, MD_LEN + 8);
				bn_read_bin(m, h1, MD_LEN);
				md_mgf1(mask, k_len - MD_LEN - 1, h1, MD_LEN);
				bn_read_bin(t, mask, k_len - MD_LEN - 1);
				t->dp[0] ^= 0x01;
				/* m_len is now the size in bits of the modulus. */
				bn_lsh(t, t, 8 * MD_LEN);
				bn_add(m, t, m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PSS);
				for (int i = m_len - 1; i < 8 * k_len; i++) {
					bn_set_bit(m, i, 0);
				}
				break;
			case RSA_VER:
			case RSA_VER_HASH:
				bn_mod_2b(t, m, 8);
				if (bn_cmp_dig(t, RSA_PSS) != CMP_EQ) {
					result = STS_ERR;
				} else {
					for (int i = m_len; i < 8 * k_len; i++) {
						if (bn_get_bit(m, i) != 0) {
							result = STS_ERR;
						}
					}
					bn_rsh(m, m, 8);
					bn_mod_2b(t, m, 8 * MD_LEN);
					bn_write_bin(h2, MD_LEN, t);
					bn_rsh(m, m, 8 * MD_LEN);
					bn_write_bin(h1, MD_LEN, t);
					md_mgf1(mask, k_len - MD_LEN - 1, h1, MD_LEN);
					bn_read_bin(t, mask, k_len - MD_LEN - 1);
					for (int i = 0; i < t->used; i++) {
						m->dp[i] ^= t->dp[i];
					}
					m->dp[0] ^= 0x01;
					for (int i = m_len - 1; i < 8 * k_len; i++) {
						bn_set_bit(m, i - ((MD_LEN + 1) * 8), 0);
					}
					if (!bn_is_zero(m)) {
						result = STS_ERR;
					}
					bn_read_bin(m, h2, MD_LEN);
					*p_len = k_len - MD_LEN;
				}
				break;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(t);
	}

	return result;
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if CP_RSA == BASIC || !defined(STRIP)

int cp_rsa_gen_basic(rsa_t pub, rsa_t prv, int bits) {
	bn_t t, r;
	int result = STS_OK;

	if (pub == NULL || prv == NULL || bits == 0) {
		return STS_ERR;
	}

	bn_null(t);
	bn_null(r);

	TRY {
		bn_new(t);
		bn_new(r);

		/* Generate different primes p and q. */
		do {
			bn_gen_prime(prv->p, bits / 2);
			bn_gen_prime(prv->q, bits / 2);
		} while (bn_cmp(prv->p, prv->q) == CMP_EQ);

		/* Swap p and q so that p is smaller. */
		if (bn_cmp(prv->p, prv->q) == CMP_LT) {
			bn_copy(t, prv->p);
			bn_copy(prv->p, prv->q);
			bn_copy(prv->q, t);
		}

		bn_mul(pub->n, prv->p, prv->q);
		bn_copy(prv->n, pub->n);
		bn_sub_dig(prv->p, prv->p, 1);
		bn_sub_dig(prv->q, prv->q, 1);

		bn_mul(t, prv->p, prv->q);

		bn_set_2b(pub->e, 16);
		bn_add_dig(pub->e, pub->e, 1);

		bn_gcd_ext(r, prv->d, NULL, pub->e, t);
		if (bn_sign(prv->d) == BN_NEG) {
			bn_add(prv->d, prv->d, t);
		}

		if (bn_cmp_dig(r, 1) == CMP_EQ) {
			bn_add_dig(prv->p, prv->p, 1);
			bn_add_dig(prv->q, prv->q, 1);
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(t);
		bn_free(r);
	}

	return result;
}

#endif

#if CP_RSA == QUICK || !defined(STRIP)

int cp_rsa_gen_quick(rsa_t pub, rsa_t prv, int bits) {
	bn_t t, r;
	int result = STS_OK;

	if (pub == NULL || prv == NULL || bits == 0) {
		return STS_ERR;
	}

	bn_null(t);
	bn_null(r);

	TRY {
		bn_new(t);
		bn_new(r);

		/* Generate different primes p and q. */
		do {
			bn_gen_prime(prv->p, bits / 2);
			bn_gen_prime(prv->q, bits / 2);
		} while (bn_cmp(prv->p, prv->q) == CMP_EQ);

		/* Swap p and q so that p is smaller. */
		if (bn_cmp(prv->p, prv->q) == CMP_LT) {
			bn_copy(t, prv->p);
			bn_copy(prv->p, prv->q);
			bn_copy(prv->q, t);
		}

		/* n = pq. */
		bn_mul(pub->n, prv->p, prv->q);
		bn_copy(prv->n, pub->n);
		bn_sub_dig(prv->p, prv->p, 1);
		bn_sub_dig(prv->q, prv->q, 1);

		/* phi(n) = (p - 1)(q - 1). */
		bn_mul(t, prv->p, prv->q);

		bn_set_2b(pub->e, 16);
		bn_add_dig(pub->e, pub->e, 1);

		/* d = e^(-1) mod phi(n). */
		bn_gcd_ext(r, prv->d, NULL, pub->e, t);
		if (bn_sign(prv->d) == BN_NEG) {
			bn_add(prv->d, prv->d, t);
		}

		if (bn_cmp_dig(r, 1) == CMP_EQ) {
			/* dP = d mod (p - 1). */
			bn_mod(prv->dp, prv->d, prv->p);
			/* dQ = d mod (q - 1). */
			bn_mod(prv->dq, prv->d, prv->q);

			bn_add_dig(prv->p, prv->p, 1);
			bn_add_dig(prv->q, prv->q, 1);

			/* qInv = q^(-1) mod p. */
			bn_gcd_ext(r, prv->qi, NULL, prv->q, prv->p);
			if (bn_sign(prv->qi) == BN_NEG) {
				bn_add(prv->qi, prv->qi, prv->p);
			}

			result = STS_OK;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(t);
		bn_free(r);
	}

	return result;
}

#endif

int cp_rsa_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len, rsa_t pub) {
	bn_t m, eb;
	int size, pad_len, result = STS_OK;

	bn_null(m);
	bn_null(eb);

	size = bn_size_bin(pub->n);

	if (pub == NULL || in_len <= 0 || in_len > (size - RSA_PAD_LEN)) {
		return STS_ERR;
	}

	TRY {
		bn_new(m);
		bn_new(eb);

		bn_zero(m);
		bn_zero(eb);

#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, in_len, size, RSA_ENC) == STS_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, in_len, size, RSA_ENC) == STS_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, in_len, size, RSA_ENC) == STS_OK) {
#endif
			bn_read_bin(m, in, in_len);
			bn_add(eb, eb, m);

#if CP_RSAPD == PKCS2
			pad_pkcs2(eb, &pad_len, in_len, size, RSA_ENC_FIN);
#endif
			bn_mxp(eb, eb, pub->e, pub->n);

			if (size <= *out_len) {
				*out_len = size;
				memset(out, 0, *out_len);
				bn_write_bin(out, size, eb);
			} else {
				result = STS_ERR;
			}
		} else {
			result = STS_ERR;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

#if CP_RSA == BASIC || !defined(STRIP)

int cp_rsa_dec_basic(uint8_t *out, int *out_len, uint8_t *in, int in_len, rsa_t prv) {
	bn_t m, eb;
	int size, pad_len, result = STS_OK;

	size = bn_size_bin(prv->n);

	if (prv == NULL || in_len != size || in_len < RSA_PAD_LEN) {
		return STS_ERR;
	}

	bn_null(m);
	bn_null(eb);

	TRY {
		bn_new(m);
		bn_new(eb);

		bn_read_bin(eb, in, in_len);
		bn_mxp(eb, eb, prv->d, prv->n);

		if (bn_cmp(eb, prv->n) != CMP_LT) {
			result = STS_ERR;
		}
#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, in_len, size, RSA_DEC) == STS_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, in_len, size, RSA_DEC) == STS_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, in_len, size, RSA_DEC) == STS_OK) {
#endif
			size = size - pad_len;

			if (size <= *out_len) {
				memset(out, 0, size);
				bn_write_bin(out, size, eb);
				*out_len = size;
			} else {
				result = STS_ERR;
			}
		} else {
			result = STS_ERR;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

#endif

#if CP_RSA == QUICK || !defined(STRIP)

int cp_rsa_dec_quick(uint8_t *out, int *out_len, uint8_t *in, int in_len, rsa_t prv) {
	bn_t m, eb;
	int size, pad_len, result = STS_OK;

	bn_null(m);
	bn_null(eb);

	size = bn_size_bin(prv->n);

	if (prv == NULL || in_len != size || in_len < RSA_PAD_LEN) {
		return STS_ERR;
	}

	TRY {
		bn_new(m);
		bn_new(eb);

		bn_read_bin(eb, in, in_len);

		bn_copy(m, eb);

		/* m1 = c^dP mod p. */
		bn_mxp(eb, eb, prv->dp, prv->p);

		/* m2 = c^dQ mod q. */
		bn_mxp(m, m, prv->dq, prv->q);

		/* m1 = m1 - m2 mod p. */
		bn_sub(eb, eb, m);
		while (bn_sign(eb) == BN_NEG) {
			bn_add(eb, eb, prv->p);
		}
		bn_mod(eb, eb, prv->p);
		/* m1 = qInv(m1 - m2) mod p. */
		bn_mul(eb, eb, prv->qi);
		bn_mod(eb, eb, prv->p);
		/* m = m2 + m1 * q. */
		bn_mul(eb, eb, prv->q);
		bn_add(eb, eb, m);

		if (bn_cmp(eb, prv->n) != CMP_LT) {
			result = STS_ERR;
		}
#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, in_len, size, RSA_DEC) == STS_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, in_len, size, RSA_DEC) == STS_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, in_len, size, RSA_DEC) == STS_OK) {
#endif
			size = size - pad_len;

			if (size <= *out_len) {
				memset(out, 0, size);
				bn_write_bin(out, size, eb);
				*out_len = size;
			} else {
				result = STS_ERR;
			}
		} else {
			result = STS_ERR;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

#endif

#if CP_RSA == BASIC || !defined(STRIP)

int cp_rsa_sig_basic(uint8_t *sig, int *sig_len, uint8_t *msg, int msg_len, int hash, rsa_t prv) {
	bn_t m, eb;
	int size, pad_len, result = STS_OK;
	uint8_t h[MD_LEN];

	if (prv == NULL || msg_len < 0) {
		return STS_ERR;
	}

	pad_len = (!hash ? MD_LEN : msg_len);

#if CP_RSAPD == PKCS2
	size = bn_bits(prv->n) - 1;
	size = (size / 8) + (size % 8 > 0);
	if (pad_len > (size - 2)) {
		return STS_ERR;
	}
#else
	size = bn_size_bin(prv->n);
	if (pad_len > (size - RSA_PAD_LEN)) {
		return STS_ERR;
	}
#endif

	bn_null(m);
	bn_null(eb);

	TRY {
		bn_new(m);
		bn_new(eb);

		bn_zero(m);
		bn_zero(eb);

		int operation = (!hash ? RSA_SIG : RSA_SIG_HASH);

#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, pad_len, size, operation) == STS_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, pad_len, size, operation) == STS_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, pad_len, size, operation) == STS_OK) {
#endif
			if (!hash) {
				md_map(h, msg, msg_len);
				bn_read_bin(m, h, MD_LEN);
				bn_add(eb, eb, m);
			} else {
				bn_read_bin(m, msg, msg_len);
				bn_add(eb, eb, m);
			}

#if CP_RSAPD == PKCS2
			pad_pkcs2(eb, &pad_len, bn_bits(prv->n), size, RSA_SIG_FIN);
#endif

			bn_mxp(eb, eb, prv->d, prv->n);

			size = bn_size_bin(prv->n);

			if (size <= *sig_len) {
				memset(sig, 0, size);
				bn_write_bin(sig, size, eb);
				*sig_len = size;
			} else {
				result = STS_ERR;
			}
		} else {
			result = STS_ERR;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

#endif

#if CP_RSA == QUICK || !defined(STRIP)

int cp_rsa_sig_quick(uint8_t *sig, int *sig_len, uint8_t *msg, int msg_len, int hash, rsa_t prv) {
	bn_t m, eb;
	int pad_len, size, result = STS_OK;
	uint8_t h[MD_LEN];

	if (prv == NULL || msg_len < 0) {
		return STS_ERR;
	}

	pad_len = (!hash ? MD_LEN : msg_len);

#if CP_RSAPD == PKCS2
	size = bn_bits(prv->n) - 1;
	size = (size / 8) + (size % 8 > 0);
	if (pad_len > (size - 2)) {
		return STS_ERR;
	}
#else
	size = bn_size_bin(prv->n);
	if (pad_len > (size - RSA_PAD_LEN)) {
		return STS_ERR;
	}
#endif

	bn_null(m);
	bn_null(eb);

	TRY {
		bn_new(m);
		bn_new(eb);

		bn_zero(m);
		bn_zero(eb);

		int operation = (!hash ? RSA_SIG : RSA_SIG_HASH);

#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, pad_len, size, operation) == STS_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, pad_len, size, operation) == STS_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, pad_len, size, operation) == STS_OK) {
#endif
			if (!hash) {
				md_map(h, msg, msg_len);
				bn_read_bin(m, h, MD_LEN);
				bn_add(eb, eb, m);
			} else {
				bn_read_bin(m, msg, msg_len);
				bn_add(eb, eb, m);
			}

#if CP_RSAPD == PKCS2
			pad_pkcs2(eb, &pad_len, bn_bits(prv->n), size, RSA_SIG_FIN);
#endif

			bn_copy(m, eb);

			/* m1 = c^dP mod p. */
			bn_mxp(eb, eb, prv->dp, prv->p);

			/* m2 = c^dQ mod q. */
			bn_mxp(m, m, prv->dq, prv->q);

			/* m1 = m1 - m2 mod p. */
			bn_sub(eb, eb, m);
			while (bn_sign(eb) == BN_NEG) {
				bn_add(eb, eb, prv->p);
			}
			bn_mod(eb, eb, prv->p);
			/* m1 = qInv(m1 - m2) mod p. */
			bn_mul(eb, eb, prv->qi);
			bn_mod(eb, eb, prv->p);
			/* m = m2 + m1 * q. */
			bn_mul(eb, eb, prv->q);
			bn_add(eb, eb, m);
			bn_mod(eb, eb, prv->n);

			size = bn_size_bin(prv->n);

			if (size <= *sig_len) {
				memset(sig, 0, size);
				bn_write_bin(sig, size, eb);
				*sig_len = size;
			} else {
				result = STS_ERR;
			}
		} else {
			result = STS_ERR;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

#endif

int cp_rsa_ver(uint8_t *sig, int sig_len, uint8_t *msg, int msg_len, int hash, rsa_t pub) {
	bn_t m, eb;
	int size, pad_len, result;
	uint8_t h1[MAX(msg_len, MD_LEN) + 8], h2[MAX(msg_len, MD_LEN)];

	/* We suppose that the signature is invalid. */
	result = 0;

	if (pub == NULL || msg_len < 0) {
		return 0;
	}

	pad_len = (!hash ? MD_LEN : msg_len);

#if CP_RSAPD == PKCS2
	size = bn_bits(pub->n) - 1;
	if (size % 8 == 0) {
		size = size / 8 - 1;
	} else {
		size = bn_size_bin(pub->n);
	}
	if (pad_len > (size - 2)) {
		return 0;
	}
#else
	size = bn_size_bin(pub->n);
	if (pad_len > (size - RSA_PAD_LEN)) {
		return 0;
	}
#endif

	bn_null(m);
	bn_null(eb);

	TRY {
		bn_new(m);
		bn_new(eb);

		bn_read_bin(eb, sig, sig_len);

		bn_mxp(eb, eb, pub->e, pub->n);

		int operation = (!hash ? RSA_VER : RSA_VER_HASH);

#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, MD_LEN, size, operation) == STS_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, MD_LEN, size, operation) == STS_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, bn_bits(pub->n), size, operation) == STS_OK) {
#endif

#if CP_RSAPD == PKCS2
			memset(h1, 0, 8);

			if (!hash) {
				md_map(h1 + 8, msg, msg_len);
				md_map(h2, h1, MD_LEN + 8);

				memset(h1, 0, MD_LEN);
				bn_write_bin(h1, size - pad_len, eb);
				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, h2, MD_LEN);
			} else {
				memcpy(h1 + 8, msg, msg_len);
				md_map(h2, h1, MD_LEN + 8);

				memset(h1, 0, msg_len);
				bn_write_bin(h1, size - pad_len, eb);

				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, h2, msg_len);
			}
#else
			memset(h1, 0, MAX(msg_len, MD_LEN));
			bn_write_bin(h1, size - pad_len, eb);

			if (!hash) {
				md_map(h2, msg, msg_len);
				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, h2, MD_LEN);
			} else {
				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, msg, msg_len);
			}
#endif
			result = (result == CMP_EQ ? 1 : 0);
		} else {
			result = 0;
		}
	}
	CATCH_ANY {
		result = 0;
	}
	FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}
