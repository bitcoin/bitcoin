#pragma once
/**
	@file
	@brief C interface of bls.hpp
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#define MCLBN_NO_AUTOLINK
#include <mcl/bn.h>

#ifdef BLS_ETH
	#define BLS_COMPILER_TIME_VAR_ADJ 200
	/*
		error if BLS_ETH is inconsistently defined between library and exe
	*/
	#undef MCLBN_COMPILED_TIME_VAR
	#define MCLBN_COMPILED_TIME_VAR ((MCLBN_FR_UNIT_SIZE) * 10 + (MCLBN_FP_UNIT_SIZE) + BLS_COMPILER_TIME_VAR_ADJ)
#endif

#ifdef _MSC_VER
	#ifdef BLS_DONT_EXPORT
		#define BLS_DLL_API
	#else
		#ifdef BLS_DLL_EXPORT
			#define BLS_DLL_API __declspec(dllexport)
		#else
			#define BLS_DLL_API __declspec(dllimport)
		#endif
	#endif
	#ifndef BLS_NO_AUTOLINK
		#if MCLBN_FP_UNIT_SIZE == 4
			#pragma comment(lib, "bls256.lib")
		#elif (MCLBN_FP_UNIT_SIZE == 6) && (MCLBN_FR_UNIT_SIZE == 4)
			#pragma comment(lib, "bls384_256.lib")
		#elif (MCLBN_FP_UNIT_SIZE == 6) && (MCLBN_FR_UNIT_SIZE == 6)
			#pragma comment(lib, "bls384.lib")
		#endif
	#endif
#elif defined(__EMSCRIPTEN__) && !defined(BLS_DONT_EXPORT)
	#define BLS_DLL_API __attribute__((used))
#elif defined(__wasm__) && !defined(BLS_DONT_EXPORT)
	#define BLS_DLL_API __attribute__((visibility("default")))
#else
	#define BLS_DLL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	mclBnFr v;
} blsId;

typedef struct {
	mclBnFr v;
} blsSecretKey;

typedef struct {
#ifdef BLS_ETH
	mclBnG1 v;
#else
	mclBnG2 v;
#endif
} blsPublicKey;

typedef struct {
#ifdef BLS_ETH
	mclBnG2 v;
#else
	mclBnG1 v;
#endif
} blsSignature;

/*
	initialize this library
	call this once before using the other functions
	@param curve [in] enum value defined in mcl/bn.h
	@param compiledTimeVar [in] specify MCLBN_COMPILED_TIME_VAR,
	which macro is used to make sure that the values
	are the same when the library is built and used
	@return 0 if success
	@note blsInit() is not thread safe
*/
BLS_DLL_API int blsInit(int curve, int compiledTimeVar);

/*
	use new eth 2.0 spec
	@return 0 if success
	@remark
	this functions and the spec may change until it is fixed
	the size of message <= 32
*/
#define BLS_ETH_MODE_OLD 0
#define BLS_ETH_MODE_DRAFT_05 1 // 2020/Jan/30
#define BLS_ETH_MODE_DRAFT_06 2 // 2020/Mar/15
#define BLS_ETH_MODE_DRAFT_07 3 // 2020/May/13
#define BLS_ETH_MODE_LATEST 3
BLS_DLL_API int blsSetETHmode(int mode);

/*
	set ETH serialization mode for BLS12-381
	@param ETHserialization [in] 1:enable,  0:disable
	@note ignore the flag if curve is not BLS12-381
	@note set in blsInit if BLS_ETH is defined
*/
BLS_DLL_API void blsSetETHserialization(int ETHserialization);

/*
	set map-to-function to mode
	MCL_MAP_TO_MODE_ORIGINAL ; for backward compatibility
	MCL_MAP_TO_MODE_HASH_TO_CURVE ; irtf-cfrg-hash-to-curve
	return 0 if success else -1
*/
BLS_DLL_API int blsSetMapToMode(int mode);

BLS_DLL_API void blsIdSetInt(blsId *id, int x);

// sec = buf & (1 << bitLen(r)) - 1
// if (sec >= r) sec &= (1 << (bitLen(r) - 1)) - 1
// always return 0
BLS_DLL_API int blsSecretKeySetLittleEndian(blsSecretKey *sec, const void *buf, mclSize bufSize);
// return 0 if success (bufSize <= 64) else -1
// set (buf mod r) to sec
BLS_DLL_API int blsSecretKeySetLittleEndianMod(blsSecretKey *sec, const void *buf, mclSize bufSize);

BLS_DLL_API void blsGetPublicKey(blsPublicKey *pub, const blsSecretKey *sec);

// calculate the has of m and sign the hash
BLS_DLL_API void blsSign(blsSignature *sig, const blsSecretKey *sec, const void *m, mclSize size);

// return 1 if valid else 0
// @remark return 0 if pub is zero for BLS_ETH
BLS_DLL_API int blsVerify(const blsSignature *sig, const blsPublicKey *pub, const void *m, mclSize size);
/*
	return 1 if blsVerify(&sigVec[i], &pubVec[i], &msgVec[i * msgSize]) returns 1 for all i = 0, ..., n-1
	@param randVec [in] non-zero randSize * n byte array
	@note for only BLS_ETH
	sig = sum_i sigVec[i] * randVec[i]
	pubVec[i] *= randVec[i]
	return blsAggregateVerifyNoCheck(sig, pubVec, msgVec, msgSize, n);
	@remark return 0 if some pubVec[i] is zero
*/
BLS_DLL_API int blsMultiVerify(const blsSignature *sigVec, const blsPublicKey *pubVec, const void *msgVec, mclSize msgSize, const void *randVec, mclSize randSize, mclSize n, int threadN);

/*
	subroutine of blsMultiVerify
	e = prod_i millerLoop(pubVec[i] * randVec[i], Hash(msgVec[i]))
	aggSig = sum_i sigVec[i] * randVec[i]
	@remark set *e = 0 if some pubVec[i] is zero
*/
BLS_DLL_API void blsMultiVerifySub(mclBnGT *e, blsSignature *aggSig, const blsSignature *sigVec, const blsPublicKey *pubVec, const char *msg, mclSize msgSize, const char *randVec, mclSize randSize, mclSize n);

/*
	subroutine of blsMultiVerify
	return FE(e * ML(P, -aggSig)) == 1 ? 1 : 0
*/
BLS_DLL_API int blsMultiVerifyFinal(const mclBnGT *e, const blsSignature *aggSig);

// aggSig = sum of sigVec[0..n]
BLS_DLL_API void blsAggregateSignature(blsSignature *aggSig, const blsSignature *sigVec, mclSize n);

// verify(sig, sum of pubVec[0..n], msg)
BLS_DLL_API int blsFastAggregateVerify(const blsSignature *sig, const blsPublicKey *pubVec, mclSize n, const void *msg, mclSize msgSize);

/*
	all msg[i] has the same msgSize byte, so msgVec must have (msgSize * n) byte area
	verify prod e(H(pubVec[i], msgToG2[i]) == e(P, sig)
	@note CHECK that sig has the valid order, all msg are different each other before calling this
*/
BLS_DLL_API int blsAggregateVerifyNoCheck(const blsSignature *sig, const blsPublicKey *pubVec, const void *msgVec, mclSize msgSize, mclSize n);

// return written byte size if success else 0
BLS_DLL_API mclSize blsIdSerialize(void *buf, mclSize maxBufSize, const blsId *id);
BLS_DLL_API mclSize blsSecretKeySerialize(void *buf, mclSize maxBufSize, const blsSecretKey *sec);
BLS_DLL_API mclSize blsPublicKeySerialize(void *buf, mclSize maxBufSize, const blsPublicKey *pub);
BLS_DLL_API mclSize blsSignatureSerialize(void *buf, mclSize maxBufSize, const blsSignature *sig);

// return read byte size if success else 0
BLS_DLL_API mclSize blsIdDeserialize(blsId *id, const void *buf, mclSize bufSize);
BLS_DLL_API mclSize blsSecretKeyDeserialize(blsSecretKey *sec, const void *buf, mclSize bufSize);
BLS_DLL_API mclSize blsPublicKeyDeserialize(blsPublicKey *pub, const void *buf, mclSize bufSize);
BLS_DLL_API mclSize blsSignatureDeserialize(blsSignature *sig, const void *buf, mclSize bufSize);

// return 1 if same else 0
BLS_DLL_API int blsIdIsEqual(const blsId *lhs, const blsId *rhs);
BLS_DLL_API int blsSecretKeyIsEqual(const blsSecretKey *lhs, const blsSecretKey *rhs);
BLS_DLL_API int blsPublicKeyIsEqual(const blsPublicKey *lhs, const blsPublicKey *rhs);
BLS_DLL_API int blsSignatureIsEqual(const blsSignature *lhs, const blsSignature *rhs);

// return 1 if zero else 0
BLS_DLL_API int blsIdIsZero(const blsId *x);
BLS_DLL_API int blsSecretKeyIsZero(const blsSecretKey *x);
BLS_DLL_API int blsPublicKeyIsZero(const blsPublicKey *x);
BLS_DLL_API int blsSignatureIsZero(const blsSignature *x);

// return 0 if success
// make sec corresponding to id from {msk[0], ..., msk[k-1]}
BLS_DLL_API int blsSecretKeyShare(blsSecretKey *sec, const blsSecretKey *msk, mclSize k, const blsId *id);
// make pub corresponding to id from {mpk[0], ..., mpk[k-1]}
BLS_DLL_API int blsPublicKeyShare(blsPublicKey *pub, const blsPublicKey *mpk, mclSize k, const blsId *id);

// return 0 if success
// recover sec from {(secVec[i], idVec[i]) for i = 0, ..., n-1}
BLS_DLL_API int blsSecretKeyRecover(blsSecretKey *sec, const blsSecretKey *secVec, const blsId *idVec, mclSize n);
// recover pub from {(pubVec[i], idVec[i]) for i = 0, ..., n-1}
BLS_DLL_API int blsPublicKeyRecover(blsPublicKey *pub, const blsPublicKey *pubVec, const blsId *idVec, mclSize n);
// recover sig from {(sigVec[i], idVec[i]) for i = 0, ..., n-1}
BLS_DLL_API int blsSignatureRecover(blsSignature *sig, const blsSignature *sigVec, const blsId *idVec, mclSize n);

// sec += rhs
BLS_DLL_API void blsSecretKeyAdd(blsSecretKey *sec, const blsSecretKey *rhs);
// pub += rhs
BLS_DLL_API void blsPublicKeyAdd(blsPublicKey *pub, const blsPublicKey *rhs);
// sig += rhs
BLS_DLL_API void blsSignatureAdd(blsSignature *sig, const blsSignature *rhs);

/*
	verify whether a point of an elliptic curve has order r
	This api affetcs setStr(), deserialize() for G2 on BN or G1/G2 on BLS12
	@param doVerify [in] does not verify if zero(default 1)
	Signature = G1, PublicKey = G2
*/
BLS_DLL_API void blsSignatureVerifyOrder(int doVerify);
BLS_DLL_API void blsPublicKeyVerifyOrder(int doVerify);
//	deserialize under VerifyOrder(true) = deserialize under VerifyOrder(false) + IsValidOrder
BLS_DLL_API int blsSignatureIsValidOrder(const blsSignature *sig);
BLS_DLL_API int blsPublicKeyIsValidOrder(const blsPublicKey *pub);

#ifndef BLS_MINIMUM_API

/*
	verify X == sY by checking e(X, sQ) = e(Y, Q)
	@param X [in]
	@param Y [in]
	@param pub [in] pub = sQ
	@return 1 if e(X, pub) = e(Y, Q) else 0
*/
BLS_DLL_API int blsVerifyPairing(const blsSignature *X, const blsSignature *Y, const blsPublicKey *pub);

/*
	sign the hash
	use the low (bitSize of r) - 1 bit of h
	return 0 if success else -1
	NOTE : return false if h is zero or c1 or -c1 value for BN254. see hashTest() in test/bls_test.hpp
*/
BLS_DLL_API int blsSignHash(blsSignature *sig, const blsSecretKey *sec, const void *h, mclSize size);
// return 1 if valid
BLS_DLL_API int blsVerifyHash(const blsSignature *sig, const blsPublicKey *pub, const void *h, mclSize size);

/*
	verify aggSig with pubVec[0, n) and hVec[0, n)
	e(aggSig, Q) = prod_i e(hVec[i], pubVec[i])
	return 1 if valid
	@note do not check duplication of hVec
*/
BLS_DLL_API int blsVerifyAggregatedHashes(const blsSignature *aggSig, const blsPublicKey *pubVec, const void *hVec, size_t sizeofHash, mclSize n);

/*
	Uncompressed version of Serialize/Deserialize
	the buffer size is twice of Serialize/Deserialize
*/
BLS_DLL_API mclSize blsPublicKeySerializeUncompressed(void *buf, mclSize maxBufSize, const blsPublicKey *pub);
BLS_DLL_API mclSize blsSignatureSerializeUncompressed(void *buf, mclSize maxBufSize, const blsSignature *sig);
BLS_DLL_API mclSize blsPublicKeyDeserializeUncompressed(blsPublicKey *pub, const void *buf, mclSize bufSize);
BLS_DLL_API mclSize blsSignatureDeserializeUncompressed(blsSignature *sig, const void *buf, mclSize bufSize);

///// to here only for BLS12-381 with BLS_ETH

// sub
BLS_DLL_API void blsSecretKeySub(blsSecretKey *sec, const blsSecretKey *rhs);
BLS_DLL_API void blsPublicKeySub(blsPublicKey *pub, const blsPublicKey *rhs);
BLS_DLL_API void blsSignatureSub(blsSignature *sig, const blsSignature *rhs);

// neg
BLS_DLL_API void blsSecretKeyNeg(blsSecretKey *x);
BLS_DLL_API void blsPublicKeyNeg(blsPublicKey *x);
BLS_DLL_API void blsSignatureNeg(blsSignature *x);

// mul y *= x
BLS_DLL_API void blsSecretKeyMul(blsSecretKey *y, const blsSecretKey *x);
BLS_DLL_API void blsPublicKeyMul(blsPublicKey *y, const blsSecretKey *x);
BLS_DLL_API void blsSignatureMul(blsSignature *y, const blsSecretKey *x);

BLS_DLL_API void blsPublicKeyMulVec(blsPublicKey *z, const blsPublicKey *x, const blsSecretKey *y, mclSize n);
BLS_DLL_API void blsSignatureMulVec(blsSignature *z, const blsSignature *x, const blsSecretKey *y, mclSize n);

// not thread safe version (old blsInit)
BLS_DLL_API int blsInitNotThreadSafe(int curve, int compiledTimeVar);

BLS_DLL_API mclSize blsGetOpUnitSize(void);
// return strlen(buf) if success else 0
BLS_DLL_API int blsGetCurveOrder(char *buf, mclSize maxBufSize);
BLS_DLL_API int blsGetFieldOrder(char *buf, mclSize maxBufSize);

// return serialized secretKey size
BLS_DLL_API int blsGetSerializedSecretKeyByteSize(void);
// return serialized publicKey size
BLS_DLL_API int blsGetSerializedPublicKeyByteSize(void);
// return serialized signature size
BLS_DLL_API int blsGetSerializedSignatureByteSize(void);

// return bytes for serialized G1(=Fp)
BLS_DLL_API int blsGetG1ByteSize(void);

// return bytes for serialized Fr
BLS_DLL_API int blsGetFrByteSize(void);

// get a generator of PublicKey
BLS_DLL_API void blsGetGeneratorOfPublicKey(blsPublicKey *pub);
// set a generator of PublicKey
BLS_DLL_API int blsSetGeneratorOfPublicKey(const blsPublicKey *pub);

// return 0 if success
BLS_DLL_API int blsIdSetDecStr(blsId *id, const char *buf, mclSize bufSize);
BLS_DLL_API int blsIdSetHexStr(blsId *id, const char *buf, mclSize bufSize);

/*
	return strlen(buf) if success else 0
	buf is '\0' terminated
*/
BLS_DLL_API mclSize blsIdGetDecStr(char *buf, mclSize maxBufSize, const blsId *id);
BLS_DLL_API mclSize blsIdGetHexStr(char *buf, mclSize maxBufSize, const blsId *id);

// hash buf and set SecretKey
BLS_DLL_API int blsHashToSecretKey(blsSecretKey *sec, const void *buf, mclSize bufSize);
// hash buf and set Signature
BLS_DLL_API int blsHashToSignature(blsSignature *sig, const void *buf, mclSize bufSize);
#ifndef MCL_DONT_USE_CSPRNG
/*
	set secretKey if system has /dev/urandom or CryptGenRandom
	return 0 if success else -1
*/
BLS_DLL_API int blsSecretKeySetByCSPRNG(blsSecretKey *sec);
/*
	set user-defined random function for setByCSPRNG
	@param self [in] user-defined pointer
	@param readFunc [in] user-defined function,
	which writes random bufSize bytes to buf and returns bufSize if success else returns 0
	@note if self == 0 and readFunc == 0 then set default random function
	@note not threadsafe
*/
BLS_DLL_API void blsSetRandFunc(void *self, unsigned int (*readFunc)(void *self, void *buf, unsigned int bufSize));
#endif

BLS_DLL_API void blsGetPop(blsSignature *sig, const blsSecretKey *sec);

BLS_DLL_API int blsVerifyPop(const blsSignature *sig, const blsPublicKey *pub);
//////////////////////////////////////////////////////////////////////////
// the following apis will be removed

// mask buf with (1 << (bitLen(r) - 1)) - 1 if buf >= r
BLS_DLL_API int blsIdSetLittleEndian(blsId *id, const void *buf, mclSize bufSize);
/*
	return written byte size if success else 0
*/
BLS_DLL_API mclSize blsIdGetLittleEndian(void *buf, mclSize maxBufSize, const blsId *id);

// return 0 if success
BLS_DLL_API int blsSecretKeySetDecStr(blsSecretKey *sec, const char *buf, mclSize bufSize);
BLS_DLL_API int blsSecretKeySetHexStr(blsSecretKey *sec, const char *buf, mclSize bufSize);
BLS_DLL_API int blsPublicKeySetHexStr(blsPublicKey *pub, const char *buf, mclSize bufSize);
BLS_DLL_API int blsSignatureSetHexStr(blsSignature *sig, const char *buf, mclSize bufSize);
/*
	return written byte size if success else 0
*/
BLS_DLL_API mclSize blsSecretKeyGetLittleEndian(void *buf, mclSize maxBufSize, const blsSecretKey *sec);
/*
	return strlen(buf) if success else 0
	buf is '\0' terminated
*/
BLS_DLL_API mclSize blsSecretKeyGetDecStr(char *buf, mclSize maxBufSize, const blsSecretKey *sec);
BLS_DLL_API mclSize blsSecretKeyGetHexStr(char *buf, mclSize maxBufSize, const blsSecretKey *sec);
BLS_DLL_API mclSize blsPublicKeyGetHexStr(char *buf, mclSize maxBufSize, const blsPublicKey *pub);
BLS_DLL_API mclSize blsSignatureGetHexStr(char *buf, mclSize maxBufSize, const blsSignature *sig);

/*
	Diffie Hellman key exchange
	out = sec * pub
*/
BLS_DLL_API void blsDHKeyExchange(blsPublicKey *out, const blsSecretKey *sec, const blsPublicKey *pub);

/*
	BLS Multi-Signatures With Public-Key Aggregation
	https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html
	H(pubVec)_i := SHA-256(pubVec[0], ..., pubVec[n-1], 4-byte little endian(i))
	@note
	1. this hash function will be modifed in the future
	2. sigVec and pubVec are not const because they may be normalized (the value are not changed)
*/
// aggSig = sum sigVec[i] t_i where (t_1, ..., t_n) = H({pubVec})
BLS_DLL_API void blsMultiAggregateSignature(blsSignature *aggSig, blsSignature *sigVec, blsPublicKey *pubVec, mclSize n);
// aggPub = sum pubVec[i] t_i where (t_1, ..., t_n) = H({pubVec})
BLS_DLL_API void blsMultiAggregatePublicKey(blsPublicKey *aggPub, blsPublicKey *pubVec, mclSize n);
#endif // BLS_MINIMUM_API

#ifdef __cplusplus
}
#endif
