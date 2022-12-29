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
 * @defgroup cp Cryptographic protocols
 */

/**
 * @file
 *
 * Interface of cryptographic protocols.
 *
 * @ingroup cp
 */

#ifndef RLC_CP_H
#define RLC_CP_H

#include "relic_conf.h"
#include "relic_types.h"
#include "relic_util.h"
#include "relic_bn.h"
#include "relic_ec.h"
#include "relic_pc.h"
#include "relic_mpc.h"

/*============================================================================*/
/* Type definitions.                                                          */
/*============================================================================*/

/**
 * Represents a pair of moduli for using the Chinese Remainder Theorem (CRT).
 */
typedef struct {
	/** The modulus n = pq. */
	bn_t n;
	/** The first prime p. */
	bn_t p;
	/** The second prime q. */
	bn_t q;
	/** The precomputed costant for the first prime. */
	bn_t dp;
	/** The precomputed costant for the second prime. */
	bn_t dq;
	/** The inverse of q modulo p. */
	bn_t qi;
} crt_st;

#if ALLOC == AUTO
typedef crt_st crt_t[1];
#else
typedef crt_st *crt_t;
#endif

/**
 * Represents an RSA key pair.
 */
typedef struct {
	/** The private exponent. */
	bn_t d;
	/** The public exponent. */
	bn_t e;
	/** The pair of moduli. */
	crt_t crt;
} _rsa_st;

/**
 * Pointer to an RSA key pair.
 */
#if ALLOC == AUTO
typedef _rsa_st rsa_t[1];
#else
typedef _rsa_st *rsa_t;
#endif

/**
 * Pointer to a Rabin key pair.
 */
#if ALLOC == AUTO
typedef crt_st rabin_t[1];
#else
typedef crt_st *rabin_t;
#endif

/**
 * Pointer to a Paillier's Homomorphic Probabilistic Encryption key pair.
 */
#if ALLOC == AUTO
typedef crt_st phpe_t[1];
#else
typedef crt_st *phpe_t;
#endif

/**
 * Represents a Benaloh's Dense Probabilistic Encryption key pair.
 */
typedef struct {
	/** The modulus n = pq. */
	bn_t n;
	/** The first prime p. */
	bn_t p;
	/** The second prime q. */
	bn_t q;
	/** The random element in {0, ..., n - 1}. */
	bn_t y;
	/** The divisor of (p-1) such that gcd(t, (p-1)/t) = gcd(t, q-1) = 1. */
	dig_t t;
} bdpe_st;

/**
 * Pointer to a Benaloh's Dense Probabilistic Encryption key pair.
 */
#if ALLOC == AUTO
typedef bdpe_st bdpe_t[1];
#else
typedef bdpe_st *bdpe_t;
#endif

/**
 * Represents a SOKAKA key pair.
 */
typedef struct _sokaka {
	/** The private key in G_1. */
	g1_t s1;
	/** The private key in G_2. */
	g2_t s2;
} sokaka_st;

/**
 * Pointer to SOKAKA key pair.
 */
#if ALLOC == AUTO
typedef sokaka_st sokaka_t[1];
#else
typedef sokaka_st *sokaka_t;
#endif

/**
 * Represents a Boneh-Goh-Nissim cryptosystem key pair.
 */
typedef struct {
	/** The first exponent. */
	bn_t x;
	/** The second exponent. */
	bn_t y;
	/** The third exponent. */
	bn_t z;
	/* The first element from the first group. */
	g1_t gx;
	/* The second element from the first group. */
	g1_t gy;
	/* The thirs element from the first group. */
	g1_t gz;
	/* The first element from the second group. */
	g2_t hx;
	/* The second element from the second group. */
	g2_t hy;
	/* The third element from the second group. */
	g2_t hz;
} bgn_st;

/**
 * Pointer to a Boneh-Goh-Nissim cryptosystem key pair.
 */
#if ALLOC == AUTO
typedef bgn_st bgn_t[1];
#else
typedef bgn_st *bgn_t;
#endif

/**
 * Represents an extendable ring signature.
 */
typedef struct _ers_st {
	/** The ephemeral public key in the signature. */
    ec_t h;
	/** The public key associated to the signature. */
	ec_t pk;
	/** The first component of the signature of knowledge. */
	bn_t c[2];
	/** The second component of the signature of knowledge. */
	bn_t r[2];
} ers_st;

/**
 * Pointer to an extendable ring signature.
 */
/**
 * Pointer to a Boneh-Goh-Nissim cryptosystem key pair.
 */
#if ALLOC == AUTO
typedef ers_st ers_t[1];
#else
typedef ers_st *ers_t;
#endif

/**
 * Represents an extendable threshold ring signature.
 */
typedef struct _etrs_st {
	/** The secret. */
	bn_t y;
	/** The ephemeral public key in the signature. */
    ec_t h;
	/** The public key associated to the signature. */
	ec_t pk;
	/** The first component of the signature of knowledge. */
	bn_t c[2];
	/** The second component of the signature of knowledge. */
	bn_t r[2];
} etrs_st;

/**
 * Pointer to an extendable ring signature.
 */
/**
 * Pointer to a Boneh-Goh-Nissim cryptosystem key pair.
 */
#if ALLOC == AUTO
typedef etrs_st etrs_t[1];
#else
typedef etrs_st *etrs_t;
#endif


/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a CRT moduli set with a null value.
 *
 * @param[out] A			- the moduli to initialize.
 */
#define crt_null(A)			RLC_NULL(A)

/**
 * Calls a function to allocate and initialize a Rabin key pair.
 *
 * @param[out] A			- the new key pair.
 */
#if ALLOC == DYNAMIC
#define crt_new(A)															\
	A = (crt_t)calloc(1, sizeof(crt_st));									\
	if (A == NULL) {														\
		RLC_THROW(ERR_NO_MEMORY);											\
	}																		\
	bn_new((A)->n);															\
	bn_new((A)->dp);														\
	bn_new((A)->dq);														\
	bn_new((A)->p);															\
	bn_new((A)->q);															\
	bn_new((A)->qi);														\

#elif ALLOC == AUTO
#define crt_new(A)															\
	bn_new((A)->n);															\
	bn_new((A)->dp);														\
	bn_new((A)->dq);														\
	bn_new((A)->p);															\
	bn_new((A)->q);															\
	bn_new((A)->qi);														\

#endif

/**
 * Calls a function to clean and free a Rabin key pair.
 *
 * @param[out] A			- the key pair to clean and free.
 */
#if ALLOC == DYNAMIC
#define crt_free(A)															\
	if (A != NULL) {														\
		bn_free((A)->n);													\
		bn_free((A)->dp);													\
		bn_free((A)->dq);													\
		bn_free((A)->p);													\
		bn_free((A)->q);													\
		bn_free((A)->qi);													\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == AUTO
#define crt_free(A)				/* empty */

#endif

/**
 * Initializes an RSA key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define rsa_null(A)			RLC_NULL(A)

/**
 * Calls a function to allocate and initialize an RSA key pair.
 *
 * @param[out] A			- the new key pair.
 */
#if ALLOC == DYNAMIC
#define rsa_new(A)															\
	A = (rsa_t)calloc(1, sizeof(_rsa_st));									\
	if (A == NULL) {														\
		RLC_THROW(ERR_NO_MEMORY);											\
	}																		\
	bn_null((A)->d);														\
	bn_null((A)->e);														\
	bn_new((A)->d);															\
	bn_new((A)->e);															\
	crt_new((A)->crt);														\

#elif ALLOC == AUTO
#define rsa_new(A)															\
	bn_new((A)->d);															\
	bn_new((A)->e);															\
	crt_new((A)->crt);														\

#endif

/**
 * Calls a function to clean and free an RSA key pair.
 *
 * @param[out] A			- the key pair to clean and free.
 */
#if ALLOC == DYNAMIC
#define rsa_free(A)															\
	if (A != NULL) {														\
		bn_free((A)->d);													\
		bn_free((A)->e);													\
		crt_free((A)->crt);													\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == AUTO
#define rsa_free(A)				/* empty */

#endif

/**
 * Initializes a Rabin key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define rabin_null(A)		RLC_NULL(A)

/**
 * Calls a function to allocate and initialize a Rabin key pair.
 *
 * @param[out] A			- the new key pair.
 */
#define rabin_new(A)		crt_new(A)

/**
 * Calls a function to clean and free a Rabin key pair.
 *
 * @param[out] A			- the key pair to clean and free.
 */
#define rabin_free(A)		crt_free(A)

/**
 * Initializes a Paillier key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define phpe_null(A)		RLC_NULL(A)

/**
 * Calls a function to allocate and initialize a Paillier key pair.
 *
 * @param[out] A			- the new key pair.
 */
#define phpe_new(A)			crt_new(A)

/**
 * Calls a function to clean and free a Paillier key pair.
 *
 * @param[out] A			- the key pair to clean and free.
 */
#define phpe_free(A)		crt_free(A)

/**
 * Initializes a Benaloh's key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define bdpe_null(A)		RLC_NULL(A)

/**
 * Calls a function to allocate and initialize a Benaloh's key pair.
 *
 * @param[out] A			- the new key pair.
 */
#if ALLOC == DYNAMIC
#define bdpe_new(A)															\
	A = (bdpe_t)calloc(1, sizeof(bdpe_st));									\
	if (A == NULL) {														\
		RLC_THROW(ERR_NO_MEMORY);											\
	}																		\
	bn_new((A)->n);															\
	bn_new((A)->y);															\
	bn_new((A)->p);															\
	bn_new((A)->q);															\
	(A)->t = 0;																\

#elif ALLOC == AUTO
#define bdpe_new(A)															\
	bn_new((A)->n);															\
	bn_new((A)->y);															\
	bn_new((A)->p);															\
	bn_new((A)->q);															\
	(A)->t = 0;																\

#endif

/**
 * Calls a function to clean and free a Benaloh's key pair.
 *
 * @param[out] A			- the key pair to clean and free.
 */
#if ALLOC == DYNAMIC
#define bdpe_free(A)														\
	if (A != NULL) {														\
		bn_free((A)->n);													\
		bn_free((A)->y);													\
		bn_free((A)->p);													\
		bn_free((A)->q);													\
		(A)->t = 0;															\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == AUTO
#define bdpe_free(A)			/* empty */
#endif

/**
 * Initializes a SOKAKA key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define sokaka_null(A)		RLC_NULL(A)

/**
 * Calls a function to allocate and initialize a SOKAKA key pair.
 *
 * @param[out] A			- the new key pair.
 */
#if ALLOC == DYNAMIC
#define sokaka_new(A)														\
	A = (sokaka_t)calloc(1, sizeof(sokaka_st));								\
	if (A == NULL) {														\
		RLC_THROW(ERR_NO_MEMORY);											\
	}																		\
	g1_new((A)->s1);														\
	g2_new((A)->s2);														\

#elif ALLOC == AUTO
#define sokaka_new(A)			/* empty */
#endif

/**
 * Calls a function to clean and free a SOKAKA key pair.
 *
 * @param[out] A			- the key pair to clean and free.
 */
#if ALLOC == DYNAMIC
#define sokaka_free(A)														\
	if (A != NULL) {														\
		g1_free((A)->s1);													\
		g2_free((A)->s2);													\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == AUTO
#define sokaka_free(A)			/* empty */
#endif

/**
 * Initializes a BGN key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define bgn_null(A)			RLC_NULL(A)

/**
 * Calls a function to allocate and initialize a BGN key pair.
 *
 * @param[out] A			- the new key pair.
 */
#if ALLOC == DYNAMIC
#define bgn_new(A)															\
	A = (bgn_t)calloc(1, sizeof(bgn_st));									\
	if (A == NULL) {														\
		RLC_THROW(ERR_NO_MEMORY);											\
	}																		\
	bn_new((A)->x);															\
	bn_new((A)->y);															\
	bn_new((A)->z);															\
	g1_new((A)->gx);														\
	g1_new((A)->gy);														\
	g1_new((A)->gz);														\
	g2_new((A)->hx);														\
	g2_new((A)->hy);														\
	g2_new((A)->hz);														\

#elif ALLOC == AUTO
#define bgn_new(A)				/* empty */
#endif

/**
 * Calls a function to clean and free a BGN key pair.
 *
 * @param[out] A			- the key pair to clean and free.
 */
#if ALLOC == DYNAMIC
#define bgn_free(A)															\
	if (A != NULL) {														\
		bn_free((A)->x);													\
		bn_free((A)->y);													\
		bn_free((A)->z);													\
		g1_free((A)->gx);													\
		g1_free((A)->gy);													\
		g1_free((A)->gz);													\
		g2_free((A)->hx);													\
		g2_free((A)->hy);													\
		g2_free((A)->hz);													\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == AUTO
#define bgn_free(A)				/* empty */
#endif

/**
 * Initializes a BGN key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define ers_null(A)			RLC_NULL(A)

/**
 * Calls a function to allocate and initialize an extendable signature ring.
 *
 * @param[out] A			- the new signature ring.
 */
#if ALLOC == DYNAMIC
#define ers_new(A)															\
	A = (ers_t)calloc(1, sizeof(ers_st));									\
	if (A == NULL) {														\
		RLC_THROW(ERR_NO_MEMORY);											\
	}																		\
	ec_new((A)->h);															\
	ec_new((A)->pk);														\
	bn_new((A)->c[0]);														\
	bn_new((A)->c[1]);														\
	bn_new((A)->r[0]);														\
	bn_new((A)->r[1]);														\

#elif ALLOC == AUTO
#define ers_new(A)				/* empty */
#endif

/**
 * Calls a function to clean and free an extendable signature ring.
 *
 * @param[out] A			- the signature ring to clean and free.
 */
#if ALLOC == DYNAMIC
#define ers_free(A)															\
	if (A != NULL) {														\
		ec_free((A)->h);													\
		ec_free((A)->pk);													\
		bn_free((A)->c[0]);													\
		bn_free((A)->c[1]);													\
		bn_free((A)->r[0]);													\
		bn_free((A)->r[1]);													\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == AUTO
#define ers_free(A)				/* empty */
#endif

/**
 * Initializes a BGN key pair with a null value.
 *
 * @param[out] A			- the key pair to initialize.
 */
#define etrs_null(A)			RLC_NULL(A)

/**
 * Calls a function to allocate and initialize an extendable signature ring.
 *
 * @param[out] A			- the new signature ring.
 */
#if ALLOC == DYNAMIC
#define etrs_new(A)															\
	A = (etrs_t)calloc(1, sizeof(etrs_st));									\
	if (A == NULL) {														\
		RLC_THROW(ERR_NO_MEMORY);											\
	}																		\
	bn_new((A)->y);															\
	ec_new((A)->h);															\
	ec_new((A)->pk);														\
	bn_new((A)->c[0]);														\
	bn_new((A)->c[1]);														\
	bn_new((A)->r[0]);														\
	bn_new((A)->r[1]);														\

#elif ALLOC == AUTO
#define etrs_new(A)				/* empty */

#endif

/**
 * Calls a function to clean and free an extendable signature ring.
 *
 * @param[out] A			- the signature ring to clean and free.
 */
#if ALLOC == DYNAMIC
#define etrs_free(A)														\
	if (A != NULL) {														\
		bn_free((A)->y);													\
		ec_free((A)->h);													\
		ec_free((A)->pk);													\
		bn_free((A)->c[0]);													\
		bn_free((A)->c[1]);													\
		bn_free((A)->r[0]);													\
		bn_free((A)->r[1]);													\
		free(A);															\
		A = NULL;															\
	}

#elif ALLOC == AUTO
#define etrs_free(A)				/* empty */
#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Generates a key pair for the RSA cryptosystem. Generates additional values
 * for the CRT optimization if CP_CRT is on.
 *
 * @param[out] pub			- the public key.
 * @param[out] prv			- the private key.
 * @param[in] bits			- the key length in bits.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_rsa_gen(rsa_t pub, rsa_t prv, int bits);

/**
 * Encrypts using the RSA cryptosystem.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to encrypt.
 * @param[in] pub			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_rsa_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len, rsa_t pub);

/**
 * Decrypts using the RSA cryptosystem. Uses the CRT optimization if
 * CP_CRT is on.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to decrypt.
 * @param[in] prv			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_rsa_dec(uint8_t *out, int *out_len, uint8_t *in, int in_len, rsa_t prv);

/**
 * Signs using the basic RSA signature algorithm. The flag must be non-zero if
 * the message being signed is already a hash value. Uses the CRT optimization
 * if CP_CRT is on.
 *
 * @param[out] sig			- the signature
 * @param[out] sig_len		- the number of bytes written in the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] msg_len		- the number of bytes to sign.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[in] prv			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_rsa_sig(uint8_t *sig, int *sig_len, uint8_t *msg, int msg_len,
		int hash, rsa_t prv);

/**
 * Verifies an RSA signature. The flag must be non-zero if the message being
 * signed is already a hash value.
 *
 * @param[in] sig			- the signature to verify.
 * @param[in] sig_len		- the signature length in bytes.
 * @param[in] msg			- the signed message.
 * @param[in] msg_len		- the message length in bytes.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[in] pub			- the public key.
 * @return a boolean value indicating if the signature is valid.
 */
int cp_rsa_ver(uint8_t *sig, int sig_len, uint8_t *msg, int msg_len, int hash,
		rsa_t pub);

/**
 * Generates a key pair for the Rabin cryptosystem.
 *
 * @param[out] pub			- the public key.
 * @param[out] prv			- the private key,
 * @param[in] bits			- the key length in bits.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_rabin_gen(rabin_t pub, rabin_t prv, int bits);

/**
 * Encrypts using the Rabin cryptosystem.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to encrypt.
 * @param[in] pub			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_rabin_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len,
		rabin_t pub);

/**
 * Decrypts using the Rabin cryptosystem.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to decrypt.
 * @param[in] prv			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_rabin_dec(uint8_t *out, int *out_len, uint8_t *in, int in_len,
		rabin_t prv);

/**
 * Generates a key pair for Benaloh's Dense Probabilistic Encryption.
 *
 * @param[out] pub			- the public key.
 * @param[out] prv			- the private key.
 * @param[in] block			- the block size.
 * @param[in] bits			- the key length in bits.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bdpe_gen(bdpe_t pub, bdpe_t prv, dig_t block, int bits);

/**
 * Encrypts using Benaloh's cryptosystem.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the plaintext as a small integer.
 * @param[in] pub			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bdpe_enc(uint8_t *out, int *out_len, dig_t in, bdpe_t pub);

/**
 * Decrypts using Benaloh's cryptosystem.
 *
 * @param[out] out			- the decrypted small integer.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to encrypt.
 * @param[in] prv			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bdpe_dec(dig_t *out, uint8_t *in, int in_len, bdpe_t prv);

/**
 * Generates a key pair for Paillier's Homomorphic Probabilistic Encryption.
 *
 * @param[out] pub			- the public key.
 * @param[out] prv			- the private key.
 * @param[in] bits			- the key length in bits.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_phpe_gen(bn_t pub, phpe_t prv, int bits);

/**
 * Encrypts using the Paillier cryptosystem.
 *
 * @param[out] c			- the ciphertex, represented as an integer.
 * @param[in] m				- the plaintext as an integer.
 * @param[in] pub			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_phpe_enc(bn_t c, bn_t m, bn_t pub);

/**
 * Decrypts using the Paillier cryptosystem.
 *
 * @param[out] m			- the plaintext, represented as an integer.
 * @param[in] c				- the ciphertex as an integer.
 * @param[in] prv			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_phpe_dec(bn_t m, bn_t c, phpe_t prv);

/**
 * Generates a key pair for Genealized Homomorphic Probabilistic Encryption.
 *
 * @param[out] pub			- the public key.
 * @param[out] prv			- the private key.
 * @param[in] bits			- the key length in bits.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ghpe_gen(bn_t pub, bn_t prv, int bits);

/**
 * Encrypts using the Generalized Paillier cryptosystem.
 *
 * @param[out] c			- the ciphertext.
 * @param[in] m				- the plaintext.
 * @param[in] pub			- the public key.
 * @param[in] s 			- the block length parameter.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ghpe_enc(bn_t c, bn_t m, bn_t pub, int s);

/**
 * Decrypts using the Generalized Paillier cryptosystem.
 *
 * @param[out] m			- the plaintext.
 * @param[in] c				- the ciphertext.
 * @param[in] pub			- the public key.
 * @param[in] s 			- the block length parameter.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ghpe_dec(bn_t m, bn_t c, bn_t pub, bn_t prv, int s);

/**
 * Generates an ECDH key pair.
 *
 * @param[out] d			- the private key.
 * @param[in] q				- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecdh_gen(bn_t d, ec_t q);

/**
 * Derives a shared secret using ECDH.
 *
 * @param[out] key			- the shared key.
 * @param[int] key_len		- the intended shared key length in bytes.
 * @param[in] d				- the private key.
 * @param[in] q				- the point received from the other party.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecdh_key(uint8_t *key, int key_len, bn_t d, ec_t q);

/**
 * Generate an ECMQV key pair.
 *
 * Should also be used to generate the ephemeral key pair.
 *
 * @param[out] d			- the private key.
 * @param[out] q			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecmqv_gen(bn_t d, ec_t q);

/**
 * Derives a shared secret using ECMQV.
 *
 * @param[out] key			- the shared key.
 * @param[int] key_len		- the intended shared key length in bytes.
 * @param[in] d1			- the private key.
 * @param[in] d2			- the ephemeral private key.
 * @param[in] q2u			- the ephemeral public key.
 * @param[in] q1v			- the point received from the other party.
 * @param[in] q2v			- the ephemeral point received from the party.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecmqv_key(uint8_t *key, int key_len, bn_t d1, bn_t d2, ec_t q2u,
		ec_t q1v, ec_t q2v);

/**
 * Generates an ECIES key pair.
 *
 * @param[out] d			- the private key.
 * @param[in] q				- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecies_gen(bn_t d, ec_t q);

/**
 * Encrypts using the ECIES cryptosystem.
 *
 * @param[out] r 			- the resulting elliptic curve point.
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to encrypt.
 * @param[in] iv 			- the block cipher initialization vector.
 * @param[in] q				- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecies_enc(ec_t r, uint8_t *out, int *out_len, uint8_t *in, int in_len,
		ec_t q);

/**
 * Decrypts using the ECIES cryptosystem.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to encrypt.
 * @param[in] iv 			- the block cipher initialization vector.
 * @param[in] d				- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecies_dec(uint8_t *out, int *out_len, ec_t r, uint8_t *in, int in_len,
		bn_t d);

/**
 * Generates an ECDSA key pair.
 *
 * @param[out] d			- the private key.
 * @param[in] q				- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecdsa_gen(bn_t d, ec_t q);

/**
 * Signs a message using ECDSA.
 *
 * @param[out] r			- the first component of the signature.
 * @param[out] s			- the second component of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[in] d				- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecdsa_sig(bn_t r, bn_t s, uint8_t *msg, int len, int hash, bn_t d);

/**
 * Verifies a message signed with ECDSA using the basic method.
 *
 * @param[out] r			- the first component of the signature.
 * @param[out] s			- the second component of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[in] q				- the public key.
 * @return a boolean value indicating if the signature is valid.
 */
int cp_ecdsa_ver(bn_t r, bn_t s, uint8_t *msg, int len, int hash, ec_t q);

/**
 * Generates an Elliptic Curve Schnorr Signature key pair.
 *
 * @param[out] d			- the private key.
 * @param[in] q				- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecss_gen(bn_t d, ec_t q);

/**
 * Signs a message using the Elliptic Curve Schnorr Signature.
 *
 * @param[out] r			- the first component of the signature.
 * @param[out] s			- the second component of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] d				- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ecss_sig(bn_t e, bn_t s, uint8_t *msg, int len, bn_t d);

/**
 * Verifies a message signed with the Elliptic Curve Schnorr Signature using the
 * basic method.
 *
 * @param[out] r			- the first component of the signature.
 * @param[out] s			- the second component of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] q				- the public key.
 * @return a boolean value indicating if the signature is valid.
 */
int cp_ecss_ver(bn_t e, bn_t s, uint8_t *msg, int len, ec_t q);

/**
 * Generate parameters for the DCKKS pairing delegation protocol described at
 * "Secure and Efficient Delegationof Pairings with Online Inputs" (CARDIS 2020)
 *
 * @param[out] c 			- the challenge.
 * @param[out] r 			- the randomness.
 * @param[out] u1			- the U1 precomputed value in G_1.
 * @param[out] u2			- the U2 precomputed value in G_2.
 * @param[out] v2			- the image of the randomness in G_2.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pdpub_gen(bn_t c, bn_t r, g1_t u1, g2_t u2, g2_t v2, gt_t e);

/**
 * Execute the client-side request for the DCKKS pairing delegation protocol.
 *
 * @param[out] v1			- the blinded element in G_1.
 * @param[out] w2			- the blinded element in G_2.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] c 			- the challenge.
 * @param[in] r 			- the randomness.
 * @param[in] u1			- the U1 precomputed value in G_1.
 * @param[in] u2			- the U2 precomputed value in G_2.
 * @param[in] v2			- the image of the randomness in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pdpub_ask(g1_t v1, g2_t w2, g1_t p, g2_t q, bn_t c, bn_t r, g1_t u1,
		g2_t u2, g2_t v2);

/**
 * Execute the server-side response for the DCKKS pairing delegation protocol.
 *
 * @param[out] g			- the group elements computed by the server.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] v1			- the blinded element in G_1.
 * @param[in] v2			- the image of the randomness in G_2.
 * @param[in] w2			- the blinded element in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pdpub_ans(gt_t g[3], g1_t p, g2_t q, g1_t v1, g2_t v2, g2_t w2);

/**
 * Verifies the result of the DCKKS pairing delegation protocol.
 *
 * @param[out] r			- the result of the computation.
 * @param[in] g				- the group elements returned by the server.
 * @param[in] c 			- the challenge.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return a boolean value indicating if the computation is correct.
 */
int cp_pdpub_ver(gt_t r, gt_t g[3], bn_t c, gt_t e);

/**
 * Generate parameters for the DCKKS pairing delegation protocol described at
 * "Secure and Efficient Delegationof Pairings with Online Inputs" (CARDIS 2020)
 *
 * @param[out] c 			- the challenge.
 * @param[out] r 			- the randomness.
 * @param[out] u1			- the U1 precomputed value in G_1.
 * @param[out] u2			- the U2 precomputed value in G_2.
 * @param[out] v2			- the image of the randomness in G_2.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pdprv_gen(bn_t c, bn_t r[3], g1_t u1[2], g2_t u2[2], g2_t v2[4],
		gt_t e[2]);

/**
 * Execute the client-side request for the DCKKS pairing delegation protocol.
 *
 * @param[out] v1			- the blinded element in G_1.
 * @param[out] w2			- the blinded element in G_2.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] c 			- the challenge.
 * @param[in] r 			- the randomness.
 * @param[in] u1			- the U1 precomputed value in G_1.
 * @param[in] u2			- the U2 precomputed value in G_2.
 * @param[in] v2			- the image of the randomness in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pdprv_ask(g1_t v1[3], g2_t w2[4], g1_t p, g2_t q, bn_t c, bn_t r[3],
		g1_t u1[2], g2_t u2[2], g2_t v2[4]);

/**
 * Execute the server-side response for the DCKKS pairing delegation protocol.
 *
 * @param[out] g			- the group elements computed by the server.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] v1			- the blinded element in G_1.
 * @param[in] v2			- the image of the randomness in G_2.
 * @param[in] w2			- the blinded element in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pdprv_ans(gt_t g[4], g1_t v1[3], g2_t w2[4]);

/**
 * Verifies the result of the DCKKS pairing delegation protocol.
 *
 * @param[out] r			- the result of the computation.
 * @param[in] g				- the group elements returned by the server.
 * @param[in] c 			- the challenge.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return a boolean value indicating if the computation is correct.
 */
int cp_pdprv_ver(gt_t r, gt_t g[4], bn_t c, gt_t e[2]);

/**
 * Generate parameters for the AMORE pairing delegation protocol with public
 * inputs.
 *
 * @param[out] r 			- the randomness.
 * @param[out] u1			- the U1 precomputed value in G_1.
 * @param[out] u2			- the U2 precomputed value in G_2.
 * @param[out] v2			- the image of the randomness in G_2.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_lvpub_gen(bn_t r, g1_t u1, g2_t u2, g2_t v2, gt_t e);

/**
 * Execute the client-side request for the AMORE pairing delegation protocol.
 *
 * @param[out] c 			- the challenge.
 * @param[out] v1			- the blinded element in G_1.
 * @param[out] w2			- the blinded element in G_2.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] c 			- the challenge.
 * @param[in] r 			- the randomness.
 * @param[in] u1			- the U1 precomputed value in G_1.
 * @param[in] u2			- the U2 precomputed value in G_2.
 * @param[in] v2			- the image of the randomness in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_lvpub_ask(bn_t c, g1_t v1, g2_t w2, g1_t p, g2_t q, bn_t r, g1_t u1,
		g2_t u2, g2_t v2);

/**
 * Execute the server-side response for the AMORE pairing delegation protocol.
 *
 * @param[out] g			- the group elements computed by the server.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] v1			- the blinded element in G_1.
 * @param[in] v2			- the image of the randomness in G_2.
 * @param[in] w2			- the blinded element in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_lvpub_ans(gt_t g[2], g1_t p, g2_t q, g1_t v1, g2_t v2, g2_t w2);

/**
 * Verifies the result of the AMORE pairing delegation protocol.
 *
 * @param[out] r			- the result of the computation.
 * @param[in] g				- the group elements returned by the server.
 * @param[in] c 			- the challenge.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return a boolean value indicating if the computation is correct.
 */
int cp_lvpub_ver(gt_t r, gt_t g[2], bn_t c, gt_t e);

/**
 * Generate parameters for the AMORE pairing delegation protocol with private
 * inputs.
 *
 * @param[out] c 			- the challenge.
 * @param[out] r 			- the randomness.
 * @param[out] u1			- the U1 precomputed value in G_1.
 * @param[out] u2			- the U2 precomputed value in G_2.
 * @param[out] v2			- the image of the randomness in G_2.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_lvprv_gen(bn_t c, bn_t r[3], g1_t u1[2], g2_t u2[2], g2_t v2[4],
		gt_t e[2]);

/**
 * Execute the client-side request for the AMORE pairing delegation protocol.
 *
 * @param[out] v1			- the blinded element in G_1.
 * @param[out] w2			- the blinded element in G_2.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] c 			- the challenge.
 * @param[in] r 			- the randomness.
 * @param[in] u1			- the U1 precomputed value in G_1.
 * @param[in] u2			- the U2 precomputed value in G_2.
 * @param[in] v2			- the image of the randomness in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_lvprv_ask(g1_t v1[3], g2_t w2[4], g1_t p, g2_t q, bn_t c, bn_t r[3],
		g1_t u1[2], g2_t u2[2], g2_t v2[4]);

/**
 * Execute the server-side response for the AMORE pairing delegation protocol.
 *
 * @param[out] g			- the group elements computed by the server.
 * @param[in] p				- the first argument of the pairing.
 * @param[in] q				- the second argument of the pairing.
 * @param[in] v1			- the blinded element in G_1.
 * @param[in] v2			- the image of the randomness in G_2.
 * @param[in] w2			- the blinded element in G_2.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_lvprv_ans(gt_t g[4], g1_t v1[3], g2_t w2[4]);

/**
 * Verifies the result of the AMORE pairing delegation protocol.
 *
 * @param[out] r			- the result of the computation.
 * @param[in] g				- the group elements returned by the server.
 * @param[in] c 			- the challenge.
 * @param[out] e			- the precomputed values e(U1, U2).
 * @return a boolean value indicating if the computation is correct.
 */
int cp_lvprv_ver(gt_t r, gt_t g[4], bn_t c, gt_t e[2]);

/**
 * Generates a master key for the SOKAKA identity-based non-interactive
 * authenticated key agreement protocol.
 *
 * @param[out] master			- the master key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_sokaka_gen(bn_t master);

/**
 * Generates a private key for the SOKAKA protocol.
 *
 * @param[out] k			- the private key.
 * @param[in] id			- the identity.
 * @param[in] master		- the master key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_sokaka_gen_prv(sokaka_t k, char *id, bn_t master);

/**
 * Computes a shared key between two entities.
 *
 * @param[out] key			- the shared key.
 * @param[int] key_len		- the intended shared key length in bytes.
 * @param[in] id1			- the first identity.
 * @param[in] k				- the private key of the first identity.
 * @param[in] id2			- the second identity.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_sokaka_key(uint8_t *key, unsigned int key_len, char *id1,
		sokaka_t k, char *id2);

/**
 * Generates a key pair for the Boneh-Go-Nissim (BGN) cryptosystem.
 *
 * @param[out] pub 			- the public key.
 * @param[out] prv 			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_gen(bgn_t pub, bgn_t prv);

/**
 * Encrypts in G_1 using the BGN cryptosystem.
 *
 * @param[out] out 			- the ciphertext.
 * @param[in] in 			- the plaintext as a small integer.
 * @param[in] pub 			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_enc1(g1_t out[2], dig_t in, bgn_t pub);

/**
 * Decrypts in G_1 using the BGN cryptosystem.
 *
 * @param[out] out 			- the decrypted small integer.
 * @param[in] in 			- the ciphertext.
 * @param[in] prv 			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_dec1(dig_t *out, g1_t in[2], bgn_t prv);

/**
 * Encrypts in G_2 using the BGN cryptosystem.
 *
 * @param[out] c 			- the ciphertext.
 * @param[in] m 			- the plaintext as a small integer.
 * @param[in] pub 			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_enc2(g2_t out[2], dig_t in, bgn_t pub);

/**
 * Decrypts in G_2 using the BGN cryptosystem.
 *
 * @param[out] out 			- the decrypted small integer.
 * @param[in] c 			- the ciphertext.
 * @param[in] prv 			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_dec2(dig_t *out, g2_t in[2], bgn_t prv);

/**
 * Adds homomorphically two BGN ciphertexts in G_T.
 *
 * @param[out] e 			- the resulting ciphertext.
 * @param[in] c 			- the first ciphertext to add.
 * @param[in] d 			- the second ciphertext to add.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_add(gt_t e[4], gt_t c[4], gt_t d[4]);

/**
 * Multiplies homomorphically two BGN ciphertexts in G_T.
 *
 * @param[out] e 			- the resulting ciphertext.
 * @param[in] c 			- the first ciphertext to add.
 * @param[in] d 			- the second ciphertext to add.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_mul(gt_t e[4], g1_t c[2], g2_t d[2]);

/**
 * Decrypts in G_T using the BGN cryptosystem.
 *
 * @param[out] out 			- the decrypted small integer.
 * @param[in] c 			- the ciphertext.
 * @param[in] prv 			- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bgn_dec(dig_t *out, gt_t in[4], bgn_t prv);

/**
 * Generates a master key for a Private Key Generator (PKG) in the
 * Boneh-Franklin Identity-Based Encryption (BF-IBE).
 *
 * @param[out] master		- the master key.
 * @param[out] pub 			- the public key of the private key generator.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ibe_gen(bn_t master, g1_t pub);

/**
 * Generates a private key for a user in the BF-IBE protocol.
 *
 * @param[out] prv			- the private key.
 * @param[in] id			- the identity.
 * @param[in] s				- the master key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ibe_gen_prv(g2_t prv, char *id, bn_t master);

/**
 * Encrypts a message using the BF-IBE protocol.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to encrypt.
 * @param[in] id			- the identity.
 * @param[in] pub			- the public key of the PKG.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ibe_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len, char *id,
		g1_t pub);

/**
 * Decrypts a message using the BF-IBE protocol.
 *
 * @param[out] out			- the output buffer.
 * @param[in, out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the input buffer.
 * @param[in] in_len		- the number of bytes to decrypt.
 * @param[in] pub			- the private key of the user.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_ibe_dec(uint8_t *out, int *out_len, uint8_t *in, int in_len, g2_t prv);

/**
 * Generates a key pair for the Boneh-Lynn-Schacham (BLS) signature protocol.
 *
 * @param[out] d			- the private key.
 * @param[out] q			- the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bls_gen(bn_t d, g2_t q);

/**
 * Signs a message using the BLS protocol.
 *
 * @param[out] s			- the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] d				- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bls_sig(g1_t s, uint8_t *msg, int len, bn_t d);

/**
 * Verifies a message signed with BLS protocol.
 *
 * @param[out] s			- the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] q				- the public key.
 * @return a boolean value indicating if the signature is valid.
 */
int cp_bls_ver(g1_t s, uint8_t *msg, int len, g2_t q);

/**
 * Generates a key pair for the Boneh-Boyen (BB) signature protocol.
 *
 * @param[out] d			- the private key.
 * @param[out] q			- the first component of the public key.
 * @param[out] z			- the second component of the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bbs_gen(bn_t d, g2_t q, gt_t z);

/**
 * Signs a message using the BB protocol.
 *
 * @param[out] s			- the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[in] d				- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_bbs_sig(g1_t s, uint8_t *msg, int len, int hash, bn_t d);

/**
 * Verifies a message signed with the BB protocol.
 *
 * @param[in] s				- the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[out] q			- the first component of the public key.
 * @param[out] z			- the second component of the public key.
 * @return a boolean value indicating the verification result.
 */
int cp_bbs_ver(g1_t s, uint8_t *msg, int len, int hash, g2_t q, gt_t z);

/**
 * Generates a key pair for the Camenisch-Lysyanskaya simple signature (CLS)
 * protocol.
 *
 * @param[out] u			- the first part of the private key.
 * @param[out] v			- the second part of the private key.
 * @param[out] x			- the first part of the public key.
 * @param[out] y			- the second part of the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cls_gen(bn_t u, bn_t v, g2_t x, g2_t y);

/**
 * Signs a message using the CLS protocol.
 *
 * @param[out] a			- the first part of the signature.
 * @param[out] b			- the second part of the signature.
 * @param[out] c			- the third part of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] u				- the first part of the private key.
 * @param[in] v				- the second part of the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cls_sig(g1_t a, g1_t b, g1_t c, uint8_t *msg, int len, bn_t u, bn_t v);

/**
 ** Verifies a signature using the CLS protocol.
 *
 * @param[in] a				- the first part of the signature.
 * @param[in] b				- the second part of the signature.
 * @param[in] c				- the third part of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] u				- the first part of the public key.
 * @param[in] v				- the second part of the public key.
 * @return a boolean value indicating the verification result.
 */
int cp_cls_ver(g1_t a, g1_t b, g1_t c, uint8_t *msg, int len, g2_t x, g2_t y);

/**
 * Generates a key pair for the Camenisch-Lysyanskaya message-independent (CLI)
 * signature protocol.
 *
 * @param[out] t			- the first part of the private key.
 * @param[out] u			- the second part of the private key.
 * @param[out] v			- the third part of the private key.
 * @param[out] x			- the first part of the public key.
 * @param[out] y			- the second part of the public key.
 * @param[out] z			- the third part of the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cli_gen(bn_t t, bn_t u, bn_t v, g2_t x, g2_t y, g2_t z);

/**
 * Signs a message using the CLI protocol.
 *
 * @param[out] a			- one of the components of the signature.
 * @param[out] A			- one of the components of the signature.
 * @param[out] b			- one of the components of the signature.
 * @param[out] B			- one of the components of the signature.
 * @param[out] c			- one of the components of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] r				- the per-message randomness.
 * @param[in] t				- the first part of the private key.
 * @param[in] u				- the second part of the private key.
 * @param[in] v				- the third part of the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cli_sig(g1_t a, g1_t A, g1_t b, g1_t B, g1_t c, uint8_t *msg, int len,
		bn_t r, bn_t t, bn_t u, bn_t v);

/**
 * Verifies a message signed using the CLI protocol.
 *
 * @param[in] a				- one of the components of the signature.
 * @param[in] A				- one of the components of the signature.
 * @param[in] b				- one of the components of the signature.
 * @param[in] B				- one of the components of the signature.
 * @param[in] c				- one of the components of the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] r				- the per-message randomness.
 * @param[in] x				- the first part of the public key.
 * @param[in] y				- the second part of the public key.
 * @param[in] z				- the third part of the public key.
 * @return a boolean value indicating the verification result.
 */
int cp_cli_ver(g1_t a, g1_t A, g1_t b, g1_t B, g1_t c, uint8_t *msg, int len,
		bn_t r, g2_t x, g2_t y, g2_t z);

/**
 * Generates a key pair for the Camenisch-Lysyanskaya message-block (CLB)
 * signature protocol.
 *
 * @param[out] t			- the first part of the private key.
 * @param[out] u			- the second part of the private key.
 * @param[out] v			- the remaining (l - 1) parts of the private key.
 * @param[out] x			- the first part of the public key.
 * @param[out] y			- the second part of the public key.
 * @param[out] z			- the remaining (l - 1) parts of the public key.
 * @param[in] l 			- the number of messages to sign.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_clb_gen(bn_t t, bn_t u, bn_t v[], g2_t x, g2_t y, g2_t z[], int l);

/**
 * Signs a block of messages using the CLB protocol.
 *
 * @param[out] a			- the first component of the signature.
 * @param[out] A			- the (l - 1) next components of the signature.
 * @param[out] b			- the next component of the signature.
 * @param[out] B			- the (l - 1) next components of the signature.
 * @param[out] c			- the last component of the signature.
 * @param[in] msgs			- the l messages to sign.
 * @param[in] lens			- the l message lengths in bytes.
 * @param[in] t				- the first part of the private key.
 * @param[in] u				- the second part of the private key.
 * @param[in] v				- the remaining (l - 1) parts of the private key.
 * @param[in] l 			- the number of messages to sign.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_clb_sig(g1_t a, g1_t A[], g1_t b, g1_t B[], g1_t c, uint8_t *msgs[],
		int lens[], bn_t t, bn_t u, bn_t v[], int l);

/**
 * Verifies a block of messages signed using the CLB protocol.
 *
 * @param[out] a			- the first component of the signature.
 * @param[out] A			- the (l - 1) next components of the signature.
 * @param[out] b			- the next component of the signature.
 * @param[out] B			- the (l - 1) next components of the signature.
 * @param[out] c			- the last component of the signature.
 * @param[in] msgs			- the l messages to sign.
 * @param[in] lens			- the l message lengths in bytes.
 * @param[in] x				- the first part of the public key.
 * @param[in] y				- the second part of the public key.
 * @param[in] z			- the remaining (l - 1) parts of the public key.
 * @param[in] l 			- the number of messages to sign.
 * @return a boolean value indicating the verification result.
 */
int cp_clb_ver(g1_t a, g1_t A[], g1_t b, g1_t B[], g1_t c, uint8_t *msgs[],
		int lens[], g2_t x, g2_t y, g2_t z[], int l);

/**
 * Generates a key pair for the Pointcheval-Sanders simple signature (PSS)
 * protocol.
 *
 * @param[out] u			- the first part of the private key.
 * @param[out] v			- the second part of the private key.
 * @param[out] g			- the first part of the public key.
 * @param[out] x			- the secpmd part of the public key.
 * @param[out] y			- the third part of the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pss_gen(bn_t u, bn_t v, g2_t g, g2_t x, g2_t y);

/**
 * Signs a message using the PSS protocol.
 *
 * @param[out] a			- the first part of the signature.
 * @param[out] b			- the second part of the signature.
 * @param[in] m			- the message to sign.
 * @param[in] u				- the first part of the private key.
 * @param[in] v				- the second part of the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pss_sig(g1_t a, g1_t b, bn_t m, bn_t u, bn_t v);

/**
 ** Verifies a signature using the PSS protocol.
 *
 * @param[in] a				- the first part of the signature.
 * @param[in] b				- the second part of the signature.
 * @param[in] m				- the message to sign.
 * @param[in] g				- the first part of the public key.
 * @param[in] u				- the second part of the public key.
 * @param[in] v				- the third part of the public key.
 * @return a boolean value indicating the verification result.
 */
int cp_pss_ver(g1_t a, g1_t b, bn_t m, g2_t g, g2_t x, g2_t y);

/**
 * Generates a key pair for the multi-part version of the Pointcheval-Sanders
 * simple signature (MPSS) protocol.
 *
 * @param[out] r			- the first part of the private key.
 * @param[out] s			- the second part of the private key.
 * @param[out] g			- the first part of the public key.
 * @param[out] x			- the second part of the public key.
 * @param[out] y			- the third part of the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mpss_gen(bn_t r[2], bn_t s[2], g2_t g, g2_t x[2], g2_t y[2]);

/**
 * Signs a message using the MPSS protocol operating over shares using triples.
 *
 * @param[out] a			- the first part of the signature.
 * @param[out] b			- the second part of the signature.
 * @param[in] m				- the message to sign.
 * @param[in] r				- the first part of the private key.
 * @param[in] s				- the second part of the private key.
 * @param[in] mul_tri 		- the triple for the multiplication.
 * @param[in] sm_tri 		- the triple for the scalar multiplication.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mpss_sig(g1_t a, g1_t b[2], bn_t m[2], bn_t r[2], bn_t s[2],
		mt_t mul_tri[2], mt_t sm_tri[2]);

/**
 * Opens public values in the MPSS protocols, in this case public keys.
 *
 * @param[in,out] x			- the shares of the second part of the public key.
 * @param[in,out] y			- the shares of the third part of the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mpss_bct(g2_t x[2], g2_t y[2]);

/**
 * Verifies a signature using the MPSS protocol operating over shares using
 * triples.
 *
 * @param[out] e			- the result of the verification.
 * @param[in] a				- the first part of the signature.
 * @param[in] b				- the second part of the signature.
 * @param[in] m				- the message to sign.
 * @param[in] g				- the first part of the public key.
 * @param[in] x				- the second part of the public key.
 * @param[in] y				- the third part of the public key.
 * @param[in] sm_tri 		- the triple for the scalar multiplication.
 * @param[in] pc_tri 		- the triple for the pairing computation.
 * @return a boolean value indicating the verification result.
 */
int cp_mpss_ver(gt_t e, g1_t a, g1_t b[2], bn_t m[2], g2_t h, g2_t x, g2_t y,
		mt_t sm_tri[2], pt_t pc_tri[2]);

/**
 * Generates a key pair for the Pointcheval-Sanders block signature (PSB)
 * protocol.
 *
 * @param[out] r			- the first part of the private key.
 * @param[out] s			- the second part of the private key.
 * @param[out] g			- the first part of the public key.
 * @param[out] x			- the secpmd part of the public key.
 * @param[out] y			- the third part of the public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_psb_gen(bn_t r, bn_t s[], g2_t g, g2_t x, g2_t y[], int l);

/**
 * Signs a block of messages using the PSB protocol.
 *
 * @param[out] a			- the first component of the signature.
 * @param[out] b			- the second component of the signature.
 * @param[in] ms			- the l messages to sign.
 * @param[in] r				- the first part of the private key.
 * @param[in] s				- the remaining l part of the private key.
 * @param[in] l 			- the number of messages to sign.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_psb_sig(g1_t a, g1_t b, bn_t ms[], bn_t r, bn_t s[], int l);

/**
 * Verifies a block of messages signed using the PSB protocol.
 *
 * @param[out] a			- the first component of the signature.
 * @param[out] b			- the seconed component of the signature.
 * @param[in] ms			- the l messages to sign.
 * @param[in] g				- the first part of the public key.
 * @param[in] x				- the second part of the public key.
 * @param[in] y				- the remaining l parts of the public key.
 * @param[in] l 			- the number of messages to sign.
 * @return a boolean value indicating the verification result.
 */
int cp_psb_ver(g1_t a, g1_t b, bn_t ms[], g2_t g, g2_t x, g2_t y[], int l);

/**
 * Generates a key pair for the multi-part version of the Pointcheval-Sanders
 * simple signature (MPSS) protocol.
 *
 * @param[out] r			- the first part of the private key.
 * @param[out] s			- the second part of the private key.
 * @param[out] g			- the first part of the public key.
 * @param[out] x			- the second part of the public key.
 * @param[out] y			- the third part of the public key.
 * @param[in] l 			- the number of messages to sign.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mpsb_gen(bn_t r[2], bn_t s[][2], g2_t h, g2_t x[2], g2_t y[][2], int l);

/**
 * Signs a message using the MPSS protocol operating over shares using triples.
 *
 * @param[out] a			- the first part of the signature.
 * @param[out] b			- the second part of the signature.
 * @param[in] m				- the messages to sign.
 * @param[in] r				- the first part of the private key.
 * @param[in] s				- the second parts of the private key.
 * @param[in] mul_tri 		- the triple for the multiplication.
 * @param[in] sm_tri 		- the triple for the scalar multiplication.
 * @param[in] l 			- the number of messages to sign.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mpsb_sig(g1_t a, g1_t b[2], bn_t m[][2], bn_t r[2], bn_t s[][2],
		mt_t mul_tri[2], mt_t sm_tri[2], int l);

/**
 * Opens public values in the MPSS protocols, in this case public keys.
 *
 * @param[in,out] x			- the shares of the second part of the public key.
 * @param[in,out] y			- the shares of the third part of the public key.
 * @param[in] l 			- the number of messages to sign.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mpsb_bct(g2_t x[2], g2_t y[][2], int l);

/**
 * Verifies a signature using the MPSS protocol operating over shares using
 * triples.
 *
 * @param[out] e			- the result of the verification.
 * @param[in] a				- the first part of the signature.
 * @param[in] b				- the second part of the signature.
 * @param[in] m				- the messages to sign.
 * @param[in] g				- the first part of the public key.
 * @param[in] x				- the second part of the public key.
 * @param[in] y				- the third parts of the public key.
 * @param[in] sm_tri 		- the triple for the scalar multiplication.
 * @param[in] pc_tri 		- the triple for the pairing computation.
 * @param[in] v 			- the private keys, can be NULL.
 * @param[in] l 			- the number of messages to sign.
 * @return a boolean value indicating the verification result.
 */
int cp_mpsb_ver(gt_t e, g1_t a, g1_t b[2], bn_t m[][2], g2_t h, g2_t x,
		g2_t y[][2], bn_t v[][2], mt_t sm_tri[2], pt_t pc_tri[2], int l);

/**
 * Generates a Zhang-Safavi-Naini-Susilo (ZSS) key pair.
 *
 * @param[out] d			- the private key.
 * @param[out] q			- the first component of the public key.
 * @param[out] z			- the second component of the public key.
 */
int cp_zss_gen(bn_t d, g1_t q, gt_t z);

/**
 * Signs a message using ZSS scheme.
 *
 * @param[out] s			- the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[in] d				- the private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_zss_sig(g2_t s, uint8_t *msg, int len, int hash, bn_t d);

/**
 * Verifies a message signed with ZSS scheme.
 *
 * @param[in] s				- the signature.
 * @param[in] msg			- the message to sign.
 * @param[in] len			- the message length in bytes.
 * @param[in] hash			- the flag to indicate the message format.
 * @param[out] q			- the first component of the public key.
 * @param[out] z			- the second component of the public key.
 * @return a boolean value indicating the verification result.
 */
int cp_zss_ver(g2_t s, uint8_t *msg, int len, int hash, g1_t q, gt_t z);

/**
 * Generates a vBNN-IBS key generation center (KGC).
 *
 * @param[out] msk 			- the KGC master key.
 * @param[out] mpk 			- the KGC public key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_vbnn_gen(bn_t msk, ec_t mpk);

/**
 * Extract a user key from an identity and a vBNN-IBS key generation center.
 *
 * @param[out] sk 			- the extracted vBNN-IBS user private key.
 * @param[out] pk 			- the extracted vBNN-IBS user public key.
 * @param[in] msk 			- the KGC master key.
 * @param[in] id			- the identity used for extraction.
 * @param[in] id_len		- the identity length in bytes.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_vbnn_gen_prv(bn_t sk, ec_t pk, bn_t msk, uint8_t *id, int id_len);

/**
 * Signs a message using the vBNN-IBS scheme.
 *
 * @param[out] r			- the R value of the signature.
 * @param[out] z 			- the z value of the signature.
 * @param[out] h 			- the h value of the signature.
 * @param[in] id 			- the identity buffer.
 * @param[in] id_len 		- the size of identity buffer.
 * @param[in] msg 			- the message buffer to sign.
 * @param[in] msg_len 		- the size of message buffer.
 * @param[in] sk 			- the signer private key.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_vbnn_sig(ec_t r, bn_t z, bn_t h, uint8_t *id, int id_len, uint8_t *msg,
		int msg_len, bn_t sk, ec_t pk);

/**
 * Verifies a signature and message using the vBNN-IBS scheme.
 *
 * @param[in] r				- the R value of the signature.
 * @param[in] z 			- the z value of the signature.
 * @param[in] h 			- the h value of the signature.
 * @param[in] id 			- the identity buffer.
 * @param[in] id_len 		- the size of identity buffer.
 * @param[in] msg 			- the message buffer to sign.
 * @param[in] msg_len 		- the size of message buffer.
 * @param[in] mpk			- the master public key of the generation center.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_vbnn_ver(ec_t r, bn_t z, bn_t h, uint8_t *id, int id_len, uint8_t *msg,
		int msg_len, ec_t mpk);

/**
 * Computes the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Proves that y = [x]G.
 *
 * @param[out] c 			- the challenge.
 * @param[out] r 			- the response.
 * @param[in] y 			- the elliptic curve point
 * @param[in] x 			- the discrete logarithm to prove.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pokdl_prv(bn_t c, bn_t r, ec_t y, bn_t x);

/**
 * Verifies the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Verifies that y = [x]G.
 *
 * @param[in] c 			- the challenge.
 * @param[in] r 			- the response.
 * @param[in] y 			- the elliptic curve point.
 * @return a boolean value indicating the verification result.
 */
int cp_pokdl_ver(bn_t c, bn_t r, ec_t y);

/**
 * Computes the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Proves that y0 = [x]G or y[1] = [x]G.
 *
 * @param[out] c 			- the challenges.
 * @param[out] r 			- the responses.
 * @param[in] y 			- the elliptic curve points.
 * @param[in] x 			- the discrete logarithm to prove.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_pokor_prv(bn_t c[2], bn_t r[2], ec_t y[2], bn_t x);

/**
 * Verifies the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Verifies that y = [x]G.
 *
 * @param[in] c 			- the challenges.
 * @param[in] r 			- the responses.
 * @param[in] y 			- the elliptic curve points.
 * @return a boolean value indicating the verification result.
 */
int cp_pokor_ver(bn_t c[2], bn_t r[2], ec_t y[2]);

/**
 * Computes the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Proves that y = [x]G.
 *
 * @param[out] c 			- the challenge.
 * @param[out] r 			- the response.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the length of the message.
 * @param[in] y 			- the elliptic curve point
 * @param[in] x 			- the discrete logarithm to prove.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_sokdl_sig(bn_t c, bn_t r, uint8_t *msg, int len, ec_t y, bn_t x);

/**
 * Verifies the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Verifies that y = [x]G.
 *
 * @param[in] c 			- the challenge.
 * @param[in] r 			- the response.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the length of the message.
 * @param[in] y 			- the elliptic curve point.
 * @return a boolean value indicating the verification result.
 */
int cp_sokdl_ver(bn_t c, bn_t r, uint8_t *msg, int len, ec_t y);

/**
 * Computes the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Proves that y0 = [x]G or y1 = [x]G.
 *
 * @param[out] c 			- the challenges.
 * @param[out] r 			- the responses.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the length of the message.
 * @param[in] y 			- the elliptic curve points.
 * @param[in] x 			- the discrete logarithm to prove.
 * @param[in] first 		- the flag to indicate the point fort which the
 *							  discrete logarithm is known.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_sokor_sig(bn_t c[2], bn_t r[2], uint8_t *msg, int len, ec_t y[2],
	bn_t x, int first);

/**
 * Verifies the proof of knowledge of a discrete logarithm of an elliptic curve
 * point to a generator. Verifies that y = [x]G.
 *
 * @param[in] c 			- the challenges.
 * @param[in] r 			- the responses.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the length of the message.
 * @param[in] y 			- the elliptic curve points.
 * @return a boolean value indicating the verification result.
 */
int cp_sokor_ver(bn_t c[2], bn_t r[2], uint8_t *msg, int len, ec_t y[2]);

/**
 * Generates the public parameters of the extendable ring signature.
 *
 * @åaram[out] pp 			- the public parameters.
 */
int cp_ers_gen(ec_t pp);

/**
 * Generates a key pair for the extendable ring signature.
 *
 * @åaram[out] sk 			- the private key.
 * @param[out] pk 			- the public key.
 */
int cp_ers_gen_key(bn_t sk, ec_t pk);

/**
 * Signs a message using the extendable ring signature scheme.
 *
 * @param[out] td 			- the signature trapdoor.
 * @param[out] p 			- the resulting signature.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the message length.
 * @param[in] sk			- the signer's private key.
 * @param[in] pk			- the singer's public key.
 * @param[in] pp			- the public parameters.
 */
int cp_ers_sig(bn_t td, ers_t p, uint8_t *msg, int len, bn_t sk, ec_t pk,
		ec_t pp);

/**
 * Verifies an extendable ring signature scheme over some messages.
 *
 * @param[in] td 			- the signature trapdoor.
 * @param[in] s 			- the ring of signatures.
 * @param[in] size 			- the number of signatures in the ring.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the message length.
 * @param[in] pp			- the public parameters.
 */
int cp_ers_ver(bn_t td, ers_t *s, int size, uint8_t *msg, int len, ec_t pp);

/**
 * Extends an extendable ring signature with a new signature.
 *
 * @param[in] td 			- the signature trapdoor.
 * @param[in] p 			- the ring of signatures.
 * @param[in] size 			- the number of signatures in the ring.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the message length.
 * @param[in] pk			- the singer's public key.
 * @param[in] pp			- the public parameters.
 */
int cp_ers_ext(bn_t td, ers_t *p, int *size, uint8_t *msg, int len, ec_t pk,
		ec_t pp);

/**
 * Generates the public parameters of the extendable threshold ring signature.
 *
 * @åaram[out] pp 			- the public parameters.
 */
int cp_etrs_gen(ec_t pp);

/**
 * Generates a key pair for the extendable threshold ring signature.
 *
 * @åaram[out] sk 			- the private key.
 * @param[out] pk 			- the public key.
 */
int cp_etrs_gen_key(bn_t sk, ec_t pk);

/**
 * Signs a message using the extendable threshold ring signature scheme.
 *
 * @param[out] td 			- the signature trapdoors.
 * @param[out] y 			- the signature randomness.
 * @param[out] max	 		- the maximum number of extensions.
 * @param[out] p 			- the resulting signature.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the message length.
 * @param[in] sk			- the signer's private key.
 * @param[in] pk			- the singer's public key.
 * @param[in] pp			- the public parametetrs.
 */
int cp_etrs_sig(bn_t *td, bn_t *y, int max, etrs_t p, uint8_t *msg, int len,
		bn_t sk, ec_t pk, ec_t pp);

/**
 * Verifies an extendable threshold ring signature scheme over some messages.
 *
 * @param[in] thres 		- the specified threshold.
 * @param[in] td 			- the signature trapdoors.
 * @param[in] y 			- the signature randomness.
 * @param[in] max	 		- the maximum number of extensions.
 * @param[in] s 			- the ring of signatures.
 * @param[in] size 			- the number of signatures in the ring.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the message length.
 * @param[in] pp			- the public parametetrs.
 */
int cp_etrs_ver(int thres, bn_t *td, bn_t *y, int max, etrs_t *s, int size,
 		uint8_t *msg, int len, ec_t pp);

/**
 * Extends an extendable threshold ring signature with a new signature.
 *
 * @param[in] td 			- the signature trapdoors.
 * @param[in] y 			- the signature randomness.
 * @param[in] max	 		- the maximum number of extensions.
 * @param[in] p 			- the ring of signatures.
 * @param[in] size 			- the number of signatures in the ring.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the message length.
 * @param[in] pk			- the singer's public key.
 * @param[in] pp			- the public parametetrs.
 */
int cp_etrs_ext(bn_t *td, bn_t *y, int max, etrs_t *p, int *size, uint8_t *msg,
		int len, ec_t pk, ec_t pp);
/**
 * Joins an extendable threshold ring signature with a new signature.
 *
 * @param[in] thres 		- the specified threshold.
 * @param[in] td 			- the signature trapdoors.
 * @param[in] y 			- the signature randomness.
 * @param[in] max	 		- the maximum number of extensions.
 * @param[in] p 			- the ring of signatures.
 * @param[in] size 			- the number of signatures in the ring.
 * @param[in] msg 			- the message to sign.
 * @param[in] len 			- the message length.
 * @param[in] sk			- the signer's private key.
 * @param[in] pk			- the singer's public key.
 * @param[in] pp			- the public parametetrs.
 */
int cp_etrs_uni(int thres, bn_t *td, bn_t *y, int max, etrs_t *p, int *size,
		uint8_t *msg, int len, bn_t sk, ec_t pk, ec_t pp);
/**
 * Initialize the Context-hiding Multi-key Homomorphic Signature scheme (CMLHS).
 * The scheme due to Schabhuser et al. signs a vector of messages.
 *
 * @param[out] h			- the random element (message as one component).
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cmlhs_init(g1_t h);

/**
 * Generates a key pair for the CMLHS scheme using BLS as underlying signature.
 *
 * @param[out] x 			- the exponent values, one per label.
 * @param[out] hs 			- the hash values, one per label.
 * @param[in] len			- the number of possible labels.
 * @param[out] prf 			- the key for the pseudo-random function (PRF).
 * @param[out] plen 		- the PRF key length.
 * @param[out] sk 			- the private key for the BLS signature scheme.
 * @param[out] pk 			- the public key for the BLS signature scheme.
 * @param[out] d 			- the secret exponent.
 * @param[out] y 			- the corresponding public element.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cmlhs_gen(bn_t x[], gt_t hs[], int len, uint8_t prf[], int plen,
		bn_t sk, g2_t pk, bn_t d, g2_t y);

/**
 * Signs a message vector using the CMLHS.
 *
 * @param[out] sig 			- the resulting BLS signature.
 * @param[out] z 			- the power of the output of the PRF.
 * @param[out] a 			- the first component of the signature.
 * @param[out] c 			- the second component of the signature.
 * @param[out] r 			- the third component of the signature.
 * @param[out] s 			- the fourth component of the signature.
 * @param[in] msg 			- the message vector to sign (one component).
 * @param[in] data 			- the dataset identifier.
 * @param[in] label 		- the integer label.
 * @param[in] x 			- the exponent value for the label.
 * @param[in] h 			- the random value (message has one component).
 * @param[in] prf 			- the key for the pseudo-random function (PRF).
 * @param[in] plen 			- the PRF key length.
 * @param[in] sk 			- the private key for the BLS signature scheme.
 * @param[out] d 			- the secret exponent.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cmlhs_sig(g1_t sig, g2_t z, g1_t a, g1_t c, g1_t r, g2_t s, bn_t msg,
		char *data, int label, bn_t x, g1_t h, uint8_t prf[], int plen,
		bn_t sk, bn_t d);

/**
 * Applies a function over a set of CMLHS signatures from the same user.
 *
 * @param[out] a			- the resulting first component of the signature.
 * @param[out] c			- the resulting second component of the signature.
 * @param[in] as			- the vector of first components of the signatures.
 * @param[in] cs			- the vector of second components of the signatures.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] len			- the number of coefficients.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cmlhs_fun(g1_t a, g1_t c, g1_t as[], g1_t cs[], dig_t f[], int len);

/**
 * Evaluates a function over a set of CMLHS signatures.
 *
 * @param[out] r			- the resulting third component of the signature.
 * @param[out] s			- the resulting fourth component of the signature.
 * @param[in] rs			- the vector of third components of the signatures.
 * @param[in] ss			- the vector of fourth components of the signatures.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] len			- the number of coefficients.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_cmlhs_evl(g1_t r, g2_t s, g1_t rs[], g2_t ss[], dig_t f[], int len);

/**
 * Verifies a CMLHS signature over a set of messages.
 *
 * @param[in] r 			- the first component of the homomorphic signature.
 * @param[in] s 			- the second component of the homomorphic signature.
 * @param[in] sig 			- the BLS signatures.
 * @param[in] z 			- the powers of the outputs of the PRF.
 * @param[in] a				- the vector of first components of the signatures.
 * @param[in] c				- the vector of second components of the signatures.
 * @param[in] msg 			- the combined message.
 * @param[in] data 			- the dataset identifier.
 * @param[in] h				- the random element (message has one component).
 * @param[in] label 		- the integer labels.
 * @param[in] hs 			- the hash values, one per label.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] flen			- the number of coefficients.
 * @param[in] y 			- the public elements of the users.
 * @param[in] pk 			- the public keys of the users.
 * @param[in] slen 			- the number of signatures.
 * @return a boolean value indicating the verification result.
 */
int cp_cmlhs_ver(g1_t r, g2_t s, g1_t sig[], g2_t z[], g1_t a[], g1_t c[],
		bn_t m, char *data, g1_t h, int label[], gt_t * hs[],
		dig_t *f[], int flen[], g2_t y[], g2_t pk[], int slen);

/**
 * Perform the offline verification of a CMLHS signature over a set of messages.
 *
 * @param[out] vk			- the verification key.
 * @param[in] h				- the random element (message has one component).
 * @param[in] label 		- the integer labels.
 * @param[in] hs 			- the hash values, one per label.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] flen			- the number of coefficients.
 * @param[in] y 			- the public elements of the users.
 * @param[in] pk 			- the public keys of the users.
 * @param[in] slen 			- the number of signatures.
 * @return a boolean value indicating the verification result.
 */
void cp_cmlhs_off(gt_t vk, g1_t h, int label[], gt_t * hs[], dig_t *f[],
		int flen[], g2_t y[], g2_t pk[], int slen);

/**
 * Perform the online verification of a CMLHS signature over a set of messages.
 *
 * @param[in] r 			- the first component of the homomorphic signature.
 * @param[in] s 			- the second component of the homomorphic signature.
 * @param[in] sig 			- the BLS signatures.
 * @param[in] z 			- the powers of the outputs of the PRF.
 * @param[in] a				- the vector of first components of the signatures.
 * @param[in] c				- the vector of second components of the signatures.
 * @param[in] msg 			- the combined message.
 * @param[in] data 			- the dataset identifier.
 * @param[in] h				- the random element (message has one component).
 * @param[in] vk			- the verification key.
 * @return a boolean value indicating the verification result.
 */
int cp_cmlhs_onv(g1_t r, g2_t s, g1_t sig[], g2_t z[], g1_t a[], g1_t c[],
		bn_t msg, char *data, g1_t h, gt_t vk, g2_t y[], g2_t pk[], int slen);
/**
 * Generates a key pair for the Multi-Key Homomorphic Signature (MKLHS) scheme.
 *
 * @param[out] sk 			- the private key for the signature scheme.
 * @param[out] pk 			- the public key for the signature scheme.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mklhs_gen(bn_t sk, g2_t pk);

/**
 * Signs a message using the MKLHS.
 *
 * @param[out] s 			- the resulting signature.
 * @param[in] m 			- the message to sign.
 * @param[in] data 			- the dataset identifier.
 * @param[in] id 			- the identity.
 * @param[in] tag 			- the tag.
 * @param[in] sk 			- the private key for the signature scheme.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mklhs_sig(g1_t s, bn_t m, char *data, char *id, char *tag, bn_t sk);

/**
 * Applies a function over a set of messages from the same user.
 *
 * @param[out] mu			- the combined message.
 * @param[in] m				- the vector of individual messages.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] len			- the number of coefficients.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mklhs_fun(bn_t mu, bn_t m[], dig_t f[], int len);

/**
 * Evaluates a function over a set of MKLHS signatures.
 *
 * @param[out] sig			- the resulting signature.
 * @param[in] s				- the set of signatures.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] len			- the number of coefficients.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mklhs_evl(g1_t sig, g1_t s[], dig_t f[], int len);

/**
 * Verifies a MKLHS signature over a set of messages.
 *
 * @param[in] sig 			- the homomorphic signature to verify.
 * @param[in] m 			- the signed message.
 * @param[in] mu			- the vector of signed messages per user.
 * @param[in] data 			- the dataset identifier.
 * @param[in] id 			- the vector of identities.
 * @param[in] tag	 		- the vector of tags.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] flen			- the number of coefficients.
 * @param[in] pk 			- the public keys of the users.
 * @param[in] slen 			- the number of signatures.
 * @return a boolean value indicating the verification result.
 */
int cp_mklhs_ver(g1_t sig, bn_t m, bn_t mu[], char *data, char *id[],
		char *tag[], dig_t *f[], int flen[], g2_t pk[], int slen);

/**
 * Computes the offline part of veryfying a MKLHS signature over a set of
 * messages.
 *
 * @param[out] h 			- the hashes of labels
 * @param[out] ft 			- the precomputed linear coefficients.
 * @param[in] id 			- the vector of identities.
 * @param[in] tag 			- the vector of tags.
 * @param[in] f 			- the linear coefficients in the function.
 * @param[in] flen			- the number of coefficients.
 * @param[in] slen 			- the number of signatures.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int cp_mklhs_off(g1_t h[], dig_t ft[], char *id[], char *tag[], dig_t *f[],
		int flen[], int slen);

/**
 * Computes the online part of veryfying a MKLHS signature over a set of
 * messages.
 *
 * @param[in] sig 			- the homomorphic signature to verify.
 * @param[in] m 			- the signed message.
 * @param[in] mu			- the vector of signed messages per user.
 * @param[in] data 			- the dataset identifier.
 * @param[in] id 			- the vector of identities.
 * @param[in] d 			- the hashes of labels.
 * @param[in] ft 			- the precomputed linear coefficients.
 * @param[in] pk 			- the public keys of the users.
 * @param[in] slen 			- the number of signatures.
 * @return a boolean value indicating the verification result.
 */
int cp_mklhs_onv(g1_t sig, bn_t m, bn_t mu[], char *data, char *id[], g1_t h[],
		dig_t ft[], g2_t pk[], int slen);

#endif /* !RLC_CP_H */
