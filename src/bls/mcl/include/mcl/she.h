#pragma once
/**
	@file
	@brief C api of somewhat homomorphic encryption with one-time multiplication, based on prime-order pairings
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mcl/bn.h>

#ifdef _MSC_VER
#ifdef MCLSHE_DLL_EXPORT
#define MCLSHE_DLL_API __declspec(dllexport)
#else
#define MCLSHE_DLL_API __declspec(dllimport)
#ifndef MCLSHE_NO_AUTOLINK
	#if MCLBN_FP_UNIT_SIZE == 4
		#pragma comment(lib, "mclshe256.lib")
	#elif MCLBN_FP_UNIT_SIZE == 6
		#pragma comment(lib, "mclshe384.lib")
	#else
		#pragma comment(lib, "mclshe512.lib")
	#endif
#endif
#endif
#else
#ifdef __EMSCRIPTEN__
	#define MCLSHE_DLL_API __attribute__((used))
#elif defined(__wasm__)
	#define MCLSHE_DLL_API __attribute__((visibility("default")))
#else
	#define MCLSHE_DLL_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	mclBnFr x;
	mclBnFr y;
} sheSecretKey;

typedef struct {
	mclBnG1 xP;
	mclBnG2 yQ;
} shePublicKey;

struct shePrecomputedPublicKey;

typedef struct {
	mclBnG1 S;
	mclBnG1 T;
} sheCipherTextG1;

typedef struct {
	mclBnG2 S;
	mclBnG2 T;
} sheCipherTextG2;

typedef struct {
	mclBnGT g[4];
} sheCipherTextGT;

typedef struct {
	mclBnFr d[4];
} sheZkpBin;

typedef struct {
	mclBnFr d[4];
} sheZkpEq;

typedef struct {
	mclBnFr d[7];
} sheZkpBinEq;

typedef struct {
	mclBnFr d[2];
} sheZkpDec;

typedef struct {
	mclBnGT d[4];
} sheAuxiliaryForZkpDecGT;

typedef struct {
	mclBnFr d[4];
} sheZkpDecGT;

/*
	initialize this library
	call this once before using the other functions
	@param curve [in] enum value defined in mcl/bn.h
	@param compiledTimeVar [in] specify MCLBN_COMPILED_TIME_VAR,
	which macro is used to make sure that the values
	are the same when the library is built and used
	@return 0 if success
	@note sheInit() is thread safe and serialized if it is called simultaneously
	but don't call it while using other functions.
*/
MCLSHE_DLL_API int sheInit(int curve, int compiledTimeVar);

// initialize this library for only G1 (such as MCL_SECP256K1)
MCLSHE_DLL_API int sheInitG1only(int curve, int compiledTimeVar);

// return written byte size if success else 0
MCLSHE_DLL_API mclSize sheSecretKeySerialize(void *buf, mclSize maxBufSize, const sheSecretKey *sec);
MCLSHE_DLL_API mclSize shePublicKeySerialize(void *buf, mclSize maxBufSize, const shePublicKey *pub);
MCLSHE_DLL_API mclSize sheCipherTextG1Serialize(void *buf, mclSize maxBufSize, const sheCipherTextG1 *c);
MCLSHE_DLL_API mclSize sheCipherTextG2Serialize(void *buf, mclSize maxBufSize, const sheCipherTextG2 *c);
MCLSHE_DLL_API mclSize sheCipherTextGTSerialize(void *buf, mclSize maxBufSize, const sheCipherTextGT *c);
MCLSHE_DLL_API mclSize sheZkpBinSerialize(void *buf, mclSize maxBufSize, const sheZkpBin *zkp);
MCLSHE_DLL_API mclSize sheZkpEqSerialize(void *buf, mclSize maxBufSize, const sheZkpEq *zkp);
MCLSHE_DLL_API mclSize sheZkpBinEqSerialize(void *buf, mclSize maxBufSize, const sheZkpBinEq *zkp);
MCLSHE_DLL_API mclSize sheZkpDecSerialize(void *buf, mclSize maxBufSize, const sheZkpDec *zkp);
MCLSHE_DLL_API mclSize sheZkpDecGTSerialize(void *buf, mclSize maxBufSize, const sheZkpDecGT *zkp);

// return read byte size if sucess else 0
MCLSHE_DLL_API mclSize sheSecretKeyDeserialize(sheSecretKey* sec, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize shePublicKeyDeserialize(shePublicKey* pub, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheCipherTextG1Deserialize(sheCipherTextG1* c, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheCipherTextG2Deserialize(sheCipherTextG2* c, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheCipherTextGTDeserialize(sheCipherTextGT* c, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheZkpBinDeserialize(sheZkpBin* zkp, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheZkpEqDeserialize(sheZkpEq* zkp, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheZkpBinEqDeserialize(sheZkpBinEq* zkp, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheZkpDecDeserialize(sheZkpDec* zkp, const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheZkpDecGTDeserialize(sheZkpDecGT* zkp, const void *buf, mclSize bufSize);

/*
	set secretKey if system has /dev/urandom or CryptGenRandom
	return 0 if success
*/
MCLSHE_DLL_API int sheSecretKeySetByCSPRNG(sheSecretKey *sec);

MCLSHE_DLL_API void sheGetPublicKey(shePublicKey *pub, const sheSecretKey *sec);

MCLSHE_DLL_API void sheGetAuxiliaryForZkpDecGT(sheAuxiliaryForZkpDecGT *aux, const shePublicKey *pub);

/*
	make table to decode DLP
	return 0 if success
*/
MCLSHE_DLL_API int sheSetRangeForDLP(mclSize hashSize);
MCLSHE_DLL_API int sheSetRangeForG1DLP(mclSize hashSize);
MCLSHE_DLL_API int sheSetRangeForG2DLP(mclSize hashSize);
MCLSHE_DLL_API int sheSetRangeForGTDLP(mclSize hashSize);

/*
	set tryNum to decode DLP
*/
MCLSHE_DLL_API void sheSetTryNum(mclSize tryNum);

/*
	decode G1 via GT if use != 0
	@note faster if tryNum >= 300
*/
MCLSHE_DLL_API void sheUseDecG1ViaGT(int use);
/*
	decode G2 via GT if use != 0
	@note faster if tryNum >= 100
*/
MCLSHE_DLL_API void sheUseDecG2ViaGT(int use);
/*
	load table for DLP
	return read size if success else 0
*/
MCLSHE_DLL_API mclSize sheLoadTableForG1DLP(const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheLoadTableForG2DLP(const void *buf, mclSize bufSize);
MCLSHE_DLL_API mclSize sheLoadTableForGTDLP(const void *buf, mclSize bufSize);

/*
	get table size for DLP
*/
MCLSHE_DLL_API mclSize sheGetTableSizeForG1DLP();
MCLSHE_DLL_API mclSize sheGetTableSizeForG2DLP();
MCLSHE_DLL_API mclSize sheGetTableSizeForGTDLP();

/*
	save table for DLP
	return written size if success else 0
*/
MCLSHE_DLL_API mclSize sheSaveTableForG1DLP(void *buf, mclSize maxBufSize);
MCLSHE_DLL_API mclSize sheSaveTableForG2DLP(void *buf, mclSize maxBufSize);
MCLSHE_DLL_API mclSize sheSaveTableForGTDLP(void *buf, mclSize maxBufSize);

// return 0 if success
MCLSHE_DLL_API int sheEncG1(sheCipherTextG1 *c, const shePublicKey *pub, mclInt m);
MCLSHE_DLL_API int sheEncG2(sheCipherTextG2 *c, const shePublicKey *pub, mclInt m);
MCLSHE_DLL_API int sheEncGT(sheCipherTextGT *c, const shePublicKey *pub, mclInt m);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncG1(sheCipherTextG1 *c, const shePrecomputedPublicKey *ppub, mclInt m);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncG2(sheCipherTextG2 *c, const shePrecomputedPublicKey *ppub, mclInt m);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncGT(sheCipherTextGT *c, const shePrecomputedPublicKey *ppub, mclInt m);

/*
	enc large integer
	buf[bufSize] is little endian
	bufSize <= (FrBitSize + 63) & ~63
	return 0 if success
*/
MCLSHE_DLL_API int sheEncIntVecG1(sheCipherTextG1 *c, const shePublicKey *pub, const void *buf, mclSize bufSize);
MCLSHE_DLL_API int sheEncIntVecG2(sheCipherTextG2 *c, const shePublicKey *pub, const void *buf, mclSize bufSize);
MCLSHE_DLL_API int sheEncIntVecGT(sheCipherTextGT *c, const shePublicKey *pub, const void *buf, mclSize bufSize);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncIntVecG1(sheCipherTextG1 *c, const shePrecomputedPublicKey *ppub, const void *buf, mclSize bufSize);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncIntVecG2(sheCipherTextG2 *c, const shePrecomputedPublicKey *ppub, const void *buf, mclSize bufSize);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncIntVecGT(sheCipherTextGT *c, const shePrecomputedPublicKey *ppub, const void *buf, mclSize bufSize);

/*
	m must be 0 or 1
*/
MCLSHE_DLL_API int sheEncWithZkpBinG1(sheCipherTextG1 *c, sheZkpBin *zkp, const shePublicKey *pub, int m);
MCLSHE_DLL_API int sheEncWithZkpBinG2(sheCipherTextG2 *c, sheZkpBin *zkp, const shePublicKey *pub, int m);
MCLSHE_DLL_API int sheEncWithZkpBinEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpBinEq *zkp, const shePublicKey *pub, int m);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncWithZkpBinG1(sheCipherTextG1 *c, sheZkpBin *zkp, const shePrecomputedPublicKey *ppub, int m);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncWithZkpBinG2(sheCipherTextG2 *c, sheZkpBin *zkp, const shePrecomputedPublicKey *ppub, int m);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncWithZkpBinEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpBinEq *zkp, const shePrecomputedPublicKey *ppub, int m);

/*
	m in mVec[0, mSize)
	output c and zkp[0, mSize * 2)
*/
MCLSHE_DLL_API int sheEncWithZkpSetG1(sheCipherTextG1 *c, mclBnFr *zkp, const shePublicKey *ppub, int m, const int *mVec, mclSize mSize);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncWithZkpSetG1(sheCipherTextG1 *c, mclBnFr *zkp, const shePrecomputedPublicKey *ppub, int m, const int *mVec, mclSize mSize);

/*
	return 1 if Dec(c) in mVec[0, mSize) else 0
*/
MCLSHE_DLL_API int sheVerifyZkpSetG1(const shePublicKey *ppub, const sheCipherTextG1 *c, const mclBnFr *zkp, const int *mVec, mclSize mSize);
MCLSHE_DLL_API int shePrecomputedPublicKeyVerifyZkpSetG1(const shePrecomputedPublicKey *ppub, const sheCipherTextG1 *c, const mclBnFr *zkp, const int *mVec, mclSize mSize);

/*
	arbitary m
*/
MCLSHE_DLL_API int sheEncWithZkpEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpEq *zkp, const shePublicKey *pub, mclInt m);
MCLSHE_DLL_API int shePrecomputedPublicKeyEncWithZkpEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpEq *zkp, const shePrecomputedPublicKey *ppub, mclInt m);

/*
	Zkp s.t. Dec(c) = m
	return 0 if success
*/
MCLSHE_DLL_API int sheDecWithZkpDecG1(mclInt *m, sheZkpDec *zkp, const sheSecretKey *sec, const sheCipherTextG1 *c, const shePublicKey *pub);
MCLSHE_DLL_API int sheDecWithZkpDecGT(mclInt *m, sheZkpDecGT *zkp, const sheSecretKey *sec, const sheCipherTextGT *c, const sheAuxiliaryForZkpDecGT *aux);

/*
	decode c and set m
	return 0 if success
*/
MCLSHE_DLL_API int sheDecG1(mclInt *m, const sheSecretKey *sec, const sheCipherTextG1 *c);
MCLSHE_DLL_API int sheDecG2(mclInt *m, const sheSecretKey *sec, const sheCipherTextG2 *c);
MCLSHE_DLL_API int sheDecGT(mclInt *m, const sheSecretKey *sec, const sheCipherTextGT *c);
/*
	verify zkp
	return 1 if valid
*/
MCLSHE_DLL_API int sheVerifyZkpBinG1(const shePublicKey *pub, const sheCipherTextG1 *c, const sheZkpBin *zkp);
MCLSHE_DLL_API int sheVerifyZkpBinG2(const shePublicKey *pub, const sheCipherTextG2 *c, const sheZkpBin *zkp);
MCLSHE_DLL_API int sheVerifyZkpEq(const shePublicKey *pub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpEq *zkp);
MCLSHE_DLL_API int sheVerifyZkpBinEq(const shePublicKey *pub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpBinEq *zkp);
MCLSHE_DLL_API int shePrecomputedPublicKeyVerifyZkpBinG1(const shePrecomputedPublicKey *ppub, const sheCipherTextG1 *c, const sheZkpBin *zkp);
MCLSHE_DLL_API int shePrecomputedPublicKeyVerifyZkpBinG2(const shePrecomputedPublicKey *ppub, const sheCipherTextG2 *c, const sheZkpBin *zkp);
MCLSHE_DLL_API int shePrecomputedPublicKeyVerifyZkpEq(const shePrecomputedPublicKey *ppub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpEq *zkp);
MCLSHE_DLL_API int shePrecomputedPublicKeyVerifyZkpBinEq(const shePrecomputedPublicKey *ppub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpBinEq *zkp);
MCLSHE_DLL_API int sheVerifyZkpDecG1(const shePublicKey *pub, const sheCipherTextG1 *c1, mclInt m, const sheZkpDec *zkp);
MCLSHE_DLL_API int sheVerifyZkpDecGT(const sheAuxiliaryForZkpDecGT *aux, const sheCipherTextGT *ct, mclInt m, const sheZkpDecGT *zkp);
/*
	decode c via GT and set m
	return 0 if success
*/
MCLSHE_DLL_API int sheDecG1ViaGT(mclInt *m, const sheSecretKey *sec, const sheCipherTextG1 *c);
MCLSHE_DLL_API int sheDecG2ViaGT(mclInt *m, const sheSecretKey *sec, const sheCipherTextG2 *c);

/*
	return 1 if dec(c) == 0
*/
MCLSHE_DLL_API int sheIsZeroG1(const sheSecretKey *sec, const sheCipherTextG1 *c);
MCLSHE_DLL_API int sheIsZeroG2(const sheSecretKey *sec, const sheCipherTextG2 *c);
MCLSHE_DLL_API int sheIsZeroGT(const sheSecretKey *sec, const sheCipherTextGT *c);

// return 0 if success
// y = -x
MCLSHE_DLL_API int sheNegG1(sheCipherTextG1 *y, const sheCipherTextG1 *x);
MCLSHE_DLL_API int sheNegG2(sheCipherTextG2 *y, const sheCipherTextG2 *x);
MCLSHE_DLL_API int sheNegGT(sheCipherTextGT *y, const sheCipherTextGT *x);

// return 0 if success
// z = x + y
MCLSHE_DLL_API int sheAddG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, const sheCipherTextG1 *y);
MCLSHE_DLL_API int sheAddG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, const sheCipherTextG2 *y);
MCLSHE_DLL_API int sheAddGT(sheCipherTextGT *z, const sheCipherTextGT *x, const sheCipherTextGT *y);

// return 0 if success
// z = x - y
MCLSHE_DLL_API int sheSubG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, const sheCipherTextG1 *y);
MCLSHE_DLL_API int sheSubG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, const sheCipherTextG2 *y);
MCLSHE_DLL_API int sheSubGT(sheCipherTextGT *z, const sheCipherTextGT *x, const sheCipherTextGT *y);

// return 0 if success
// z = x * y
MCLSHE_DLL_API int sheMulG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, mclInt y);
MCLSHE_DLL_API int sheMulG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, mclInt y);
MCLSHE_DLL_API int sheMulGT(sheCipherTextGT *z, const sheCipherTextGT *x, mclInt y);
/*
	mul large integer
	buf[bufSize] is little endian
	bufSize <= (FrBitSize + 63) & ~63
	return 0 if success
*/
MCLSHE_DLL_API int sheMulIntVecG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, const void *buf, mclSize bufSize);
MCLSHE_DLL_API int sheMulIntVecG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, const void *buf, mclSize bufSize);
MCLSHE_DLL_API int sheMulIntVecGT(sheCipherTextGT *z, const sheCipherTextGT *x, const void *buf, mclSize bufSize);

// return 0 if success
// z = x * y
MCLSHE_DLL_API int sheMul(sheCipherTextGT *z, const sheCipherTextG1 *x, const sheCipherTextG2 *y);
/*
	sheMul(z, x, y) = sheMulML(z, x, y) + sheFinalExpGT(z)
	@note
	Mul(x1, y1) + ... + Mul(xn, yn) = finalExp(MulML(x1, y1) + ... + MulML(xn, yn))
*/
MCLSHE_DLL_API int sheMulML(sheCipherTextGT *z, const sheCipherTextG1 *x, const sheCipherTextG2 *y);
MCLSHE_DLL_API int sheFinalExpGT(sheCipherTextGT *y, const sheCipherTextGT *x);

// return 0 if success
// rerandomize(c)
MCLSHE_DLL_API int sheReRandG1(sheCipherTextG1 *c, const shePublicKey *pub);
MCLSHE_DLL_API int sheReRandG2(sheCipherTextG2 *c, const shePublicKey *pub);
MCLSHE_DLL_API int sheReRandGT(sheCipherTextGT *c, const shePublicKey *pub);

// return 0 if success
// y = convert(x)
MCLSHE_DLL_API int sheConvertG1(sheCipherTextGT *y, const shePublicKey *pub, const sheCipherTextG1 *x);
MCLSHE_DLL_API int sheConvertG2(sheCipherTextGT *y, const shePublicKey *pub, const sheCipherTextG2 *x);

// return nonzero if success
MCLSHE_DLL_API shePrecomputedPublicKey *shePrecomputedPublicKeyCreate();
// call this function to avoid memory leak
MCLSHE_DLL_API void shePrecomputedPublicKeyDestroy(shePrecomputedPublicKey *ppub);
// return 0 if success
MCLSHE_DLL_API int shePrecomputedPublicKeyInit(shePrecomputedPublicKey *ppub, const shePublicKey *pub);

// return 1 if x == y else 0
MCLSHE_DLL_API int sheSecretKeyIsEqual(const sheSecretKey *x, const sheSecretKey *y);
MCLSHE_DLL_API int shePublicKeyIsEqual(const shePublicKey *x, const shePublicKey *y);
MCLSHE_DLL_API int sheCipherTextG1IsEqual(const sheCipherTextG1 *x, const sheCipherTextG1 *y);
MCLSHE_DLL_API int sheCipherTextG2IsEqual(const sheCipherTextG2 *x, const sheCipherTextG2 *y);
MCLSHE_DLL_API int sheCipherTextGTIsEqual(const sheCipherTextGT *x, const sheCipherTextGT *y);
#ifdef __cplusplus
}
#endif
