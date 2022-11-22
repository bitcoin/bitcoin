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
 * Implementation of the RSA cryptosystem.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Length of chosen padding scheme.
 */
#if CP_RSAPD == PKCS1
#define RSA_PAD_LEN		(11)
#elif CP_RSAPD == PKCS2
#define RSA_PAD_LEN		(2 * RLC_MD_LEN + 2)
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
 * @return RLC_ERR if errors occurred, RLC_OK otherwise.
 */
static int pad_basic(bn_t m, int *p_len, int m_len, int k_len, int operation) {
	uint8_t pad = 0;
	int result = RLC_ERR;
	bn_t t;

	RLC_TRY {
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
				result = RLC_OK;
				break;
			case RSA_DEC:
			case RSA_VER:
			case RSA_VER_HASH:
				/* EB = 00 | FF | D. */
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (bn_is_zero(t)) {
					*p_len = 1;
					do {
						(*p_len)++;
						m_len--;
						bn_rsh(t, m, 8 * m_len);
						pad = (uint8_t)t->dp[0];
					} while (pad == 0 && m_len > 0);
					if (pad == RSA_PAD) {
						result = RLC_OK;
					}
					bn_mod_2b(m, m, (k_len - *p_len) * 8);
				}
				break;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(t);
	}
	return result;
}

#endif

#if CP_RSAPD == PKCS1

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
			RLC_THROW(ERR_NO_VALID);
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
 * @return RLC_ERR if errors occurred, RLC_OK otherwise.
 */
static int pad_pkcs1(bn_t m, int *p_len, int m_len, int k_len, int operation) {
	uint8_t *id, pad = 0;
	int len, result = RLC_ERR;
	bn_t t;

	bn_null(t);

	RLC_TRY {
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
				/* Make room for the zero and real message. */
				bn_lsh(m, m, (m_len + 1) * 8);
				result = RLC_OK;
				break;
			case RSA_DEC:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (bn_is_zero(t)) {
					*p_len = m_len;
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
					if (pad == RSA_PUB) {
						do {
							m_len--;
							bn_rsh(t, m, 8 * m_len);
							pad = (uint8_t)t->dp[0];
						} while (pad != 0 && m_len > 0);
						/* Remove padding and trailing zero. */
						*p_len -= (m_len - 1);
						bn_mod_2b(m, m, (k_len - *p_len) * 8);
						result = (m_len > 0 ? RLC_OK : RLC_ERR);
					}
				}
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
				/* Make room for the zero and hash id. */
				bn_lsh(m, m, 8 * (len + 1));
				bn_read_bin(t, id, len);
				bn_add(m, m, t);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				result = RLC_OK;
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
				/* Make room for the zero and hash. */
				bn_lsh(m, m, 8 * (m_len + 1));
				result = RLC_OK;
				break;
			case RSA_VER:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (bn_is_zero(t)) {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
					if (pad == RSA_PRV) {
						int counter = 0;
						do {
							counter++;
							m_len--;
							bn_rsh(t, m, 8 * m_len);
							pad = (uint8_t)t->dp[0];
						} while (pad == RSA_PAD && m_len > 0);
						/* Remove padding and trailing zero. */
						id = hash_id(MD_MAP, &len);
						bn_rsh(t, m, 8 * m_len);
						bn_mod_2b(t, t, 8);
						if (bn_is_zero(t)) {
							m_len -= len;
							bn_rsh(t, m, 8 * m_len);
							int r = 0;
							for (int i = 0; i < len; i++) {
								pad = (uint8_t)t->dp[0];
								r |= pad ^ id[len - i - 1];
								bn_rsh(t, t, 8);
							}
							*p_len = k_len - m_len;
							bn_mod_2b(m, m, m_len * 8);
							if (r == 0 && m_len == RLC_MD_LEN && counter >= 8) {
								result = RLC_OK;
							}
						}
					}
				}
				break;
			case RSA_VER_HASH:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (bn_is_zero(t)) {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
					if (pad == RSA_PRV) {
						int counter = 0;
						do {
							counter++;
							m_len--;
							bn_rsh(t, m, 8 * m_len);
							pad = (uint8_t)t->dp[0];
						} while (pad == RSA_PAD && m_len > 0);
						/* Remove padding and trailing zero. */
						*p_len = k_len - m_len;
						bn_rsh(t, m, 8 * m_len);
						bn_mod_2b(t, t, 8);
						if (bn_is_zero(t)) {
							bn_mod_2b(m, m, m_len * 8);
							if (m_len == RLC_MD_LEN && counter >= 8) {
								result = RLC_OK;
							}
						}
					}
				}
				break;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
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
 * @return RLC_ERR if errors occurred, RLC_OK otherwise.
 */
static int pad_pkcs2(bn_t m, int *p_len, int m_len, int k_len, int operation) {
        uint8_t pad, h1[RLC_MD_LEN], h2[RLC_MD_LEN];
        /* MSVC does not allow dynamic stack arrays */
        uint8_t *mask = RLC_ALLOCA(uint8_t, k_len);
	int result = RLC_ERR;
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		switch (operation) {
			case RSA_ENC:
				/* DB = lHash | PS | 01 | D. */
				md_map(h1, NULL, 0);
				bn_read_bin(m, h1, RLC_MD_LEN);
				*p_len = k_len - 2 * RLC_MD_LEN - 2 - m_len;
				bn_lsh(m, m, *p_len * 8);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0x01);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				result = RLC_OK;
				break;
			case RSA_ENC_FIN:
				/* EB = 00 | maskedSeed | maskedDB. */
				rand_bytes(h1, RLC_MD_LEN);
				md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
				bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
				for (int i = 0; i < t->used; i++) {
					m->dp[i] ^= t->dp[i];
				}
				bn_write_bin(mask, k_len - RLC_MD_LEN - 1, m);
				md_mgf(h2, RLC_MD_LEN, mask, k_len - RLC_MD_LEN - 1);
				for (int i = 0; i < RLC_MD_LEN; i++) {
					h1[i] ^= h2[i];
				}
				bn_read_bin(t, h1, RLC_MD_LEN);
				bn_lsh(t, t, 8 * (k_len - RLC_MD_LEN - 1));
				bn_add(t, t, m);
				bn_copy(m, t);
				result = RLC_OK;
				break;
			case RSA_DEC:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (bn_is_zero(t)) {
					m_len -= RLC_MD_LEN;
					bn_rsh(t, m, 8 * m_len);
					bn_write_bin(h1, RLC_MD_LEN, t);
					bn_mod_2b(m, m, 8 * m_len);
					bn_write_bin(mask, m_len, m);
					md_mgf(h2, RLC_MD_LEN, mask, m_len);
					for (int i = 0; i < RLC_MD_LEN; i++) {
						h1[i] ^= h2[i];
					}
					md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
					bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
					for (int i = 0; i < t->used; i++) {
						m->dp[i] ^= t->dp[i];
					}
					m_len -= RLC_MD_LEN;
					bn_rsh(t, m, 8 * m_len);
					bn_write_bin(h2, RLC_MD_LEN, t);
					md_map(h1, NULL, 0);
					pad = 0;
					for (int i = 0; i < RLC_MD_LEN; i++) {
						pad |= h1[i] ^ h2[i];
					}
					bn_mod_2b(m, m, 8 * m_len);
					*p_len = bn_size_bin(m);
					(*p_len)--;
					bn_rsh(t, m, *p_len * 8);
					if (pad == 0 && bn_cmp_dig(t, 1) == RLC_EQ) {
						result = RLC_OK;
					}
					bn_mod_2b(m, m, *p_len * 8);
					*p_len = k_len - *p_len;
				}
				break;
			case RSA_SIG:
			case RSA_SIG_HASH:
				/* M' = 00 00 00 00 00 00 00 00 | H(M). */
				bn_zero(m);
				bn_lsh(m, m, 64);
				/* Make room for the real message. */
				bn_lsh(m, m, RLC_MD_LEN * 8);
				result = RLC_OK;
				break;
			case RSA_SIG_FIN:
				memset(mask, 0, 8);
				bn_write_bin(mask + 8, RLC_MD_LEN, m);
				md_map(h1, mask, RLC_MD_LEN + 8);
				bn_read_bin(m, h1, RLC_MD_LEN);
				md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
				bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
				t->dp[0] ^= 0x01;
				/* m_len is now the size in bits of the modulus. */
				bn_lsh(t, t, 8 * RLC_MD_LEN);
				bn_add(m, t, m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PSS);
				for (int i = m_len - 1; i < 8 * k_len; i++) {
					bn_set_bit(m, i, 0);
				}
				result = RLC_OK;
				break;
			case RSA_VER:
			case RSA_VER_HASH:
				bn_mod_2b(t, m, 8);
				pad = (uint8_t)t->dp[0];
				if (pad == RSA_PSS) {
					int r = 1;
					for (int i = m_len; i < 8 * k_len; i++) {
						if (bn_get_bit(m, i) != 0) {
							r = 0;
						}
					}
					bn_rsh(m, m, 8);
					bn_mod_2b(t, m, 8 * RLC_MD_LEN);
					bn_write_bin(h2, RLC_MD_LEN, t);
					bn_rsh(m, m, 8 * RLC_MD_LEN);
					bn_write_bin(h1, RLC_MD_LEN, t);
					md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
					bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
					for (int i = 0; i < t->used; i++) {
						m->dp[i] ^= t->dp[i];
					}
					m->dp[0] ^= 0x01;
					for (int i = m_len - 1; i < 8 * k_len; i++) {
						bn_set_bit(m, i - ((RLC_MD_LEN + 1) * 8), 0);
					}
					if (r == 1 && bn_is_zero(m)) {
						result = RLC_OK;
					}
					bn_read_bin(m, h2, RLC_MD_LEN);
					*p_len = k_len - RLC_MD_LEN;
				}
				break;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(t);
	}

        RLC_FREE(mask);

	return result;
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_rsa_gen(rsa_t pub, rsa_t prv, int bits) {
	bn_t t, r;
	int result = RLC_OK;

	if (pub == NULL || prv == NULL || bits == 0) {
		return RLC_ERR;
	}

	bn_null(t);
	bn_null(r);

	RLC_TRY {
		bn_new(t);
		bn_new(r);

		/* Generate different primes p and q. */
		do {
			bn_gen_prime(prv->crt->p, bits / 2);
			bn_gen_prime(prv->crt->q, bits / 2);
		} while (bn_cmp(prv->crt->p, prv->crt->q) == RLC_EQ);

		/* Swap p and q so that p is smaller. */
		if (bn_cmp(prv->crt->p, prv->crt->q) != RLC_LT) {
			bn_copy(t, prv->crt->p);
			bn_copy(prv->crt->p, prv->crt->q);
			bn_copy(prv->crt->q, t);
		}

		/* n = pq. */
		bn_mul(pub->crt->n, prv->crt->p, prv->crt->q);
		bn_copy(prv->crt->n, pub->crt->n);
		bn_sub_dig(prv->crt->p, prv->crt->p, 1);
		bn_sub_dig(prv->crt->q, prv->crt->q, 1);

		/* phi(n) = (p - 1)(q - 1). */
		bn_mul(t, prv->crt->p, prv->crt->q);

		bn_set_2b(pub->e, 16);
		bn_add_dig(pub->e, pub->e, 1);

#if !defined(CP_CRT)
		/* d = e^(-1) mod phi(n). */
		bn_gcd_ext(r, prv->d, NULL, pub->e, t);
		if (bn_sign(prv->d) == RLC_NEG) {
			bn_add(prv->d, prv->d, t);
		}
		if (bn_cmp_dig(r, 1) == RLC_EQ) {
			/* Restore p and q. */
			bn_add_dig(prv->crt->p, prv->crt->p, 1);
			bn_add_dig(prv->crt->q, prv->crt->q, 1);
			result = RLC_OK;
		}
#else
		/* d = e^(-1) mod phi(n). */
		bn_gcd_ext(r, prv->d, NULL, pub->e, t);
		if (bn_sign(prv->d) == RLC_NEG) {
			bn_add(prv->d, prv->d, t);
		}

		if (bn_cmp_dig(r, 1) == RLC_EQ) {
			/* dP = d mod (p - 1). */
			bn_mod(prv->crt->dp, prv->d, prv->crt->p);
			/* dQ = d mod (q - 1). */
			bn_mod(prv->crt->dq, prv->d, prv->crt->q);
			/* Restore p and q. */
			bn_add_dig(prv->crt->p, prv->crt->p, 1);
			bn_add_dig(prv->crt->q, prv->crt->q, 1);
			/* qInv = q^(-1) mod p. */
			bn_mod_inv(prv->crt->qi, prv->crt->q, prv->crt->p);

			result = RLC_OK;
		}
#endif /* CP_CRT */
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(t);
		bn_free(r);
	}

	return result;
}

int cp_rsa_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len, rsa_t pub) {
	bn_t m, eb;
	int size, pad_len, result = RLC_OK;

	bn_null(m);
	bn_null(eb);

	size = bn_size_bin(pub->crt->n);

	if (pub == NULL || in_len <= 0 || in_len > (size - RSA_PAD_LEN)) {
		return RLC_ERR;
	}

	RLC_TRY {
		bn_new(m);
		bn_new(eb);

		bn_zero(m);
		bn_zero(eb);

#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, in_len, size, RSA_ENC) == RLC_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, in_len, size, RSA_ENC) == RLC_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, in_len, size, RSA_ENC) == RLC_OK) {
#endif
			bn_read_bin(m, in, in_len);
			bn_add(eb, eb, m);

#if CP_RSAPD == PKCS2
			pad_pkcs2(eb, &pad_len, in_len, size, RSA_ENC_FIN);
#endif
			bn_mxp(eb, eb, pub->e, pub->crt->n);

			if (size <= *out_len) {
				*out_len = size;
				memset(out, 0, *out_len);
				bn_write_bin(out, size, eb);
			} else {
				result = RLC_ERR;
			}
		} else {
			result = RLC_ERR;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

int cp_rsa_dec(uint8_t *out, int *out_len, uint8_t *in, int in_len, rsa_t prv) {
	bn_t m, eb;
	int size, pad_len, result = RLC_OK;

	bn_null(m);
	bn_null(eb);

	size = bn_size_bin(prv->crt->n);

	if (prv == NULL || in_len != size || in_len < RSA_PAD_LEN) {
		return RLC_ERR;
	}

	RLC_TRY {
		bn_new(m);
		bn_new(eb);

		bn_read_bin(eb, in, in_len);
#if !defined(CP_CRT)
		bn_mxp(eb, eb, prv->d, prv->crt->n);
#else

		bn_copy(m, eb);

#if MULTI == OPENMP
		omp_set_num_threads(CORES);
		#pragma omp parallel copyin(core_ctx) firstprivate(prv)
		{
			#pragma omp sections
			{
				#pragma omp section
				{
#endif
					/* m1 = c^dP mod p. */
					bn_mxp(eb, eb, prv->crt->dp, prv->crt->p);

#if MULTI == OPENMP
				}
				#pragma omp section
				{
#endif
					/* m2 = c^dQ mod q. */
					bn_mxp(m, m, prv->crt->dq, prv->crt->q);

#if MULTI == OPENMP
				}
			}
		}
#endif
		/* m1 = m1 - m2 mod p. */
		bn_sub(eb, eb, m);
		while (bn_sign(eb) == RLC_NEG) {
			bn_add(eb, eb, prv->crt->p);
		}
		bn_mod(eb, eb, prv->crt->p);
		/* m1 = qInv(m1 - m2) mod p. */
		bn_mul(eb, eb, prv->crt->qi);
		bn_mod(eb, eb, prv->crt->p);
		/* m = m2 + m1 * q. */
		bn_mul(eb, eb, prv->crt->q);
		bn_add(eb, eb, m);

#endif /* CP_CRT */

		if (bn_cmp(eb, prv->crt->n) != RLC_LT) {
			result = RLC_ERR;
		}
#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, in_len, size, RSA_DEC) == RLC_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, in_len, size, RSA_DEC) == RLC_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, in_len, size, RSA_DEC) == RLC_OK) {
#endif
			size = size - pad_len;

			if (size <= *out_len) {
				memset(out, 0, size);
				bn_write_bin(out, size, eb);
				*out_len = size;
			} else {
				result = RLC_ERR;
			}
		} else {
			result = RLC_ERR;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

int cp_rsa_sig(uint8_t *sig, int *sig_len, uint8_t *msg, int msg_len, int hash, rsa_t prv) {
	bn_t m, eb;
	int pad_len, size, result = RLC_OK;
	uint8_t h[RLC_MD_LEN];

	if (prv == NULL || msg_len < 0) {
		return RLC_ERR;
	}

	pad_len = (!hash ? RLC_MD_LEN : msg_len);

#if CP_RSAPD == PKCS2
	size = bn_bits(prv->crt->n) - 1;
	size = (size / 8) + (size % 8 > 0);
	if (pad_len > (size - 2)) {
		return RLC_ERR;
	}
#else
	size = bn_size_bin(prv->crt->n);
	if (pad_len > (size - RSA_PAD_LEN)) {
		return RLC_ERR;
	}
#endif

	bn_null(m);
	bn_null(eb);

	RLC_TRY {
		bn_new(m);
		bn_new(eb);

		bn_zero(m);
		bn_zero(eb);

		int operation = (!hash ? RSA_SIG : RSA_SIG_HASH);

#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, pad_len, size, operation) == RLC_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, pad_len, size, operation) == RLC_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, pad_len, size, operation) == RLC_OK) {
#endif
			if (!hash) {
				md_map(h, msg, msg_len);
				bn_read_bin(m, h, RLC_MD_LEN);
				bn_add(eb, eb, m);
			} else {
				bn_read_bin(m, msg, msg_len);
				bn_add(eb, eb, m);
			}

#if CP_RSAPD == PKCS2
			pad_pkcs2(eb, &pad_len, bn_bits(prv->crt->n), size, RSA_SIG_FIN);
#endif

			bn_copy(m, eb);

#if !defined(CP_CRT)
			bn_mxp(eb, eb, prv->d, prv->crt->n);
#else  /* CP_CRT */

#if MULTI == OPENMP
			omp_set_num_threads(CORES);
			#pragma omp parallel copyin(core_ctx) firstprivate(prv)
			{
				#pragma omp sections
				{
					#pragma omp section
					{
#endif
						/* m1 = c^dP mod p. */
						bn_mxp(eb, eb, prv->crt->dp, prv->crt->p);
#if MULTI == OPENMP
					}
					#pragma omp section
					{
#endif
						/* m2 = c^dQ mod q. */
						bn_mxp(m, m, prv->crt->dq, prv->crt->q);
#if MULTI == OPENMP
					}
				}
			}
#endif
			/* m1 = m1 - m2 mod p. */
			bn_sub(eb, eb, m);
			while (bn_sign(eb) == RLC_NEG) {
				bn_add(eb, eb, prv->crt->p);
			}
			bn_mod(eb, eb, prv->crt->p);
			/* m1 = qInv(m1 - m2) mod p. */
			bn_mul(eb, eb, prv->crt->qi);
			bn_mod(eb, eb, prv->crt->p);
			/* m = m2 + m1 * q. */
			bn_mul(eb, eb, prv->crt->q);
			bn_add(eb, eb, m);
			bn_mod(eb, eb, prv->crt->n);

#endif /* CP_CRT */

			size = bn_size_bin(prv->crt->n);

			if (size <= *sig_len) {
				memset(sig, 0, size);
				bn_write_bin(sig, size, eb);
				*sig_len = size;
			} else {
				result = RLC_ERR;
			}
		} else {
			result = RLC_ERR;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(eb);
	}

	return result;
}

int cp_rsa_ver(uint8_t *sig, int sig_len, uint8_t *msg, int msg_len, int hash, rsa_t pub) {
	bn_t m, eb;
	int size, pad_len, result;
	uint8_t *h1 = RLC_ALLOCA(uint8_t, RLC_MAX(msg_len, RLC_MD_LEN) + 8);
	uint8_t *h2 = RLC_ALLOCA(uint8_t, RLC_MAX(msg_len, RLC_MD_LEN));

	/* We suppose that the signature is invalid. */
	result = 0;

	if (h1 == NULL || h2 == NULL) {
		RLC_FREE(h1);
		RLC_FREE(h2);
		return 0;
	}

	if (pub == NULL || msg_len < 0) {
		return 0;
	}

	pad_len = (!hash ? RLC_MD_LEN : msg_len);

#if CP_RSAPD == PKCS2
	size = bn_bits(pub->crt->n) - 1;
	if (size % 8 == 0) {
		size = size / 8 - 1;
	} else {
		size = bn_size_bin(pub->crt->n);
	}
	if (pad_len > (size - 2)) {
		return 0;
	}
#else
	size = bn_size_bin(pub->crt->n);
	if (pad_len > (size - RSA_PAD_LEN)) {
		return 0;
	}
#endif

	bn_null(m);
	bn_null(eb);

	RLC_TRY {
		bn_new(m);
		bn_new(eb);

		bn_read_bin(eb, sig, sig_len);

		bn_mxp(eb, eb, pub->e, pub->crt->n);

		int operation = (!hash ? RSA_VER : RSA_VER_HASH);

#if CP_RSAPD == BASIC
		if (pad_basic(eb, &pad_len, RLC_MD_LEN, size, operation) == RLC_OK) {
#elif CP_RSAPD == PKCS1
		if (pad_pkcs1(eb, &pad_len, RLC_MD_LEN, size, operation) == RLC_OK) {
#elif CP_RSAPD == PKCS2
		if (pad_pkcs2(eb, &pad_len, bn_bits(pub->crt->n), size, operation) == RLC_OK) {
#endif

#if CP_RSAPD == PKCS2
			memset(h1, 0, 8);

			if (!hash) {
				md_map(h1 + 8, msg, msg_len);
				md_map(h2, h1, RLC_MD_LEN + 8);

				memset(h1, 0, RLC_MD_LEN);
				bn_write_bin(h1, size - pad_len, eb);
				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, h2, RLC_MD_LEN);
			} else {
				memcpy(h1 + 8, msg, msg_len);
				md_map(h2, h1, RLC_MD_LEN + 8);

				memset(h1, 0, msg_len);
				bn_write_bin(h1, size - pad_len, eb);

				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, h2, msg_len);
			}
#else
			memset(h1, 0, RLC_MAX(msg_len, RLC_MD_LEN));
			bn_write_bin(h1, size - pad_len, eb);

			if (!hash) {
				md_map(h2, msg, msg_len);
				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, h2, RLC_MD_LEN);
			} else {
				/* Everything went ok, so signature status is changed. */
				result = util_cmp_const(h1, msg, msg_len);
			}
#endif
			result = (result == RLC_EQ ? 1 : 0);
		} else {
			result = 0;
		}
	}
	RLC_CATCH_ANY {
		result = 0;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(eb);
		RLC_FREE(h1);
		RLC_FREE(h2);
	}

	return result;
}
