#pragma once
/**
	@file
	@brief C interface of ECDSA
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdint.h> // for uint64_t, uint8_t
#include <stdlib.h> // for size_t

#if defined(_MSC_VER)
	#ifdef ECDSA_DLL_EXPORT
		#define ECDSA_DLL_API __declspec(dllexport)
	#else
		#define ECDSA_DLL_API __declspec(dllimport)
		#ifndef ECDSA_NO_AUTOLINK
			#pragma comment(lib, "mclecdsa.lib")
		#endif
	#endif
#elif defined(__EMSCRIPTEN__)
	#define ECDSA_DLL_API __attribute__((used))
#else
	#define ECDSA_DLL_API
#endif

#ifndef mclSize
	#ifdef __EMSCRIPTEN__
		// avoid 64-bit integer
		#define mclSize unsigned int
		#define mclInt int
	#else
		// use #define for cgo
		#define mclSize size_t
		#define mclInt int64_t
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ECDSA_NOT_DEFINE_STRUCT

typedef struct ecdsaSecretKey ecdsaSecretKey;
typedef struct ecdsaPublicKey ecdsaPublicKey;
typedef struct ecdsaSignature ecdsaSignature;

#else

typedef struct {
	uint64_t d[4];
} ecdsaSecretKey;

typedef struct {
	uint64_t d[4 * 3];
} ecdsaPublicKey;

typedef struct {
	uint64_t d[4 * 2];
} ecdsaSignature;

#endif

struct ecdsaPrecomputedPublicKey;

/*
	init library
	return 0 if success
	@note not threadsafe
*/
ECDSA_DLL_API int ecdsaInit(void);

/*
	change serializeMode
	0 : old
	1 : compatible with BitCoin (default)
	return 0 if success
*/
ECDSA_DLL_API int ecdsaSetSerializeMode(int mode);

// return written byte size if success else 0
ECDSA_DLL_API mclSize ecdsaSecretKeySerialize(void *buf, mclSize maxBufSize, const ecdsaSecretKey *sec);
ECDSA_DLL_API mclSize ecdsaPublicKeySerialize(void *buf, mclSize maxBufSize, const ecdsaPublicKey *pub);
ECDSA_DLL_API mclSize ecdsaSignatureSerialize(void *buf, mclSize maxBufSize, const ecdsaSignature *sig);
// return 0x02 + bigEndian(x) if y is even
// return 0x03 + bigEndian(x) if y is odd
ECDSA_DLL_API mclSize ecdsaPublicKeySerializeCompressed(void *buf, mclSize maxBufSize, const ecdsaPublicKey *pub);

// return read byte size if sucess else 0
ECDSA_DLL_API mclSize ecdsaSecretKeyDeserialize(ecdsaSecretKey* sec, const void *buf, mclSize bufSize);
ECDSA_DLL_API mclSize ecdsaPublicKeyDeserialize(ecdsaPublicKey* pub, const void *buf, mclSize bufSize);
ECDSA_DLL_API mclSize ecdsaSignatureDeserialize(ecdsaSignature* sig, const void *buf, mclSize bufSize);

//	return 0 if success
ECDSA_DLL_API int ecdsaSecretKeySetByCSPRNG(ecdsaSecretKey *sec);

ECDSA_DLL_API void ecdsaGetPublicKey(ecdsaPublicKey *pub, const ecdsaSecretKey *sec);

ECDSA_DLL_API void ecdsaSign(ecdsaSignature *sig, const ecdsaSecretKey *sec, const void *m, mclSize size);

// normalize sig to lower S (r, s) such that s < half
ECDSA_DLL_API void ecdsaNormalizeSignature(ecdsaSignature *sig);

// return 1 if valid
// accept only lower S signature
ECDSA_DLL_API int ecdsaVerify(const ecdsaSignature *sig, const ecdsaPublicKey *pub, const void *m, mclSize size);
ECDSA_DLL_API int ecdsaVerifyPrecomputed(const ecdsaSignature *sig, const ecdsaPrecomputedPublicKey *pub, const void *m, mclSize size);

// return nonzero if success
ECDSA_DLL_API ecdsaPrecomputedPublicKey *ecdsaPrecomputedPublicKeyCreate();
// call this function to avoid memory leak
ECDSA_DLL_API void ecdsaPrecomputedPublicKeyDestroy(ecdsaPrecomputedPublicKey *ppub);
// return 0 if success
ECDSA_DLL_API int ecdsaPrecomputedPublicKeyInit(ecdsaPrecomputedPublicKey *ppub, const ecdsaPublicKey *pub);

#ifdef __cplusplus
}
#endif

