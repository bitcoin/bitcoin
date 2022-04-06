#pragma once
/**
	@file
	@brief C interface of 256/384-bit optimal ate pairing over BN curves
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
/*
	the order of an elliptic curve over Fp is Fr
*/
#ifndef MCLBN_FP_UNIT_SIZE
	#error "define MCLBN_FP_UNIT_SIZE 4(, 6 or 8)"
#endif
#ifndef MCLBN_FR_UNIT_SIZE
	#define MCLBN_FR_UNIT_SIZE MCLBN_FP_UNIT_SIZE
#endif
#define MCLBN_COMPILED_TIME_VAR ((MCLBN_FR_UNIT_SIZE) * 10 + (MCLBN_FP_UNIT_SIZE))

#include <stdint.h> // for uint64_t, uint8_t
#include <stdlib.h> // for size_t


#if defined(_MSC_VER)
	#ifdef MCLBN_DONT_EXPORT
		#define MCLBN_DLL_API
	#else
		#ifdef MCLBN_DLL_EXPORT
			#define MCLBN_DLL_API __declspec(dllexport)
		#else
			#define MCLBN_DLL_API __declspec(dllimport)
		#endif
	#endif
	#ifndef MCLBN_NO_AUTOLINK
		#if MCLBN_FP_UNIT_SIZE == 4
			#pragma comment(lib, "mclbn256.lib")
		#elif (MCLBN_FP_UNIT_SIZE == 6) && (MCLBN_FR_UNIT_SIZE == 4)
			#pragma comment(lib, "mclbn384_256.lib")
		#elif (MCLBN_FP_UNIT_SIZE == 6) && (MCLBN_FR_UNIT_SIZE == 6)
			#pragma comment(lib, "mclbn384.lib")
		#elif MCLBN_FP_UNIT_SIZE == 8
			#pragma comment(lib, "mclbn512.lib")
		#endif
	#endif
#elif defined(__EMSCRIPTEN__) && !defined(MCLBN_DONT_EXPORT)
	#define MCLBN_DLL_API __attribute__((used))
#elif defined(__wasm__) && !defined(MCLBN_DONT_EXPORT)
	#define MCLBN_DLL_API __attribute__((visibility("default")))
#else
	#define MCLBN_DLL_API
#endif

#ifdef __EMSCRIPTEN__
	// avoid 64-bit integer
	#define mclSize unsigned int
	#define mclInt int
#else
	// use #define for cgo
	#define mclSize size_t
	#define mclInt int64_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MCLBN_NOT_DEFINE_STRUCT

typedef struct mclBnFr mclBnFr;
typedef struct mclBnG1 mclBnG1;
typedef struct mclBnG2 mclBnG2;
typedef struct mclBnGT mclBnGT;
typedef struct mclBnFp mclBnFp;
typedef struct mclBnFp2 mclBnFp2;

#else

typedef struct {
	uint64_t d[MCLBN_FP_UNIT_SIZE];
} mclBnFp;

/*
	x = d[0] + d[1] i where i^2 = -1
*/
typedef struct {
	mclBnFp d[2];
} mclBnFp2;

/*
	G1 and G2 are isomorphism to Fr
*/
typedef struct {
	uint64_t d[MCLBN_FR_UNIT_SIZE];
} mclBnFr;

/*
	G1 is defined over Fp
*/
typedef struct {
	mclBnFp x, y, z;
} mclBnG1;

typedef struct {
	mclBnFp2 x, y, z;
} mclBnG2;

typedef struct {
	mclBnFp d[12];
} mclBnGT;

#endif

#include <mcl/curve_type.h>

#define MCLBN_IO_EC_AFFINE 0
#define MCLBN_IO_EC_PROJ 1024
#define MCLBN_IO_SERIALIZE_HEX_STR 2048

// for backword compatibility
enum {
	mclBn_CurveFp254BNb = 0,
	mclBn_CurveFp382_1 = 1,
	mclBn_CurveFp382_2 = 2,
	mclBn_CurveFp462 = 3,
	mclBn_CurveSNARK1 = 4,
	mclBls12_CurveFp381 = 5
};

// return 0xABC which means A.BC
MCLBN_DLL_API int mclBn_getVersion(void);
/*
	init library
	@param curve [in] type of bn curve
	@param compiledTimeVar [in] specify MCLBN_COMPILED_TIME_VAR,
	which macro is used to make sure that the values
	are the same when the library is built and used
	@return 0 if success
	curve = BN254/BN_SNARK1 is allowed if maxUnitSize = 4
	curve = BN381_1/BN381_2/BLS12_381 are allowed if maxUnitSize = 6
	This parameter is used to detect a library compiled with different MCLBN_FP_UNIT_SIZE for safety.
	@note not threadsafe
	@note BN_init is used in libeay32
*/
MCLBN_DLL_API int mclBn_init(int curve, int compiledTimeVar);

MCLBN_DLL_API int mclBn_getCurveType(void);

/*
	pairing : G1 x G2 -> GT
	#G1 = #G2 = r
	G1 is a curve defined on Fp

	serialized size of elements
	           |Fr| |Fp|
	BN254       32   32
	BN381       48   48
	BLS12_381   32   48
	BN462       58   58
	|G1| = |Fp|
	|G2| = |G1| * 2
	|GT| = |G1| * 12
*/
/*
	return the num of Unit(=uint64_t) to store Fr
*/
MCLBN_DLL_API int mclBn_getOpUnitSize(void);

/*
	return bytes for serialized G1(=Fp)
*/
MCLBN_DLL_API int mclBn_getG1ByteSize(void);
/*
	return bytes for serialized Fr
*/
MCLBN_DLL_API int mclBn_getFrByteSize(void);
/*
	return bytes for serialized Fp
*/
MCLBN_DLL_API int mclBn_getFpByteSize(void);

/*
	return decimal string of the order of the curve(=the characteristic of Fr)
	return str(buf) if success
*/
MCLBN_DLL_API mclSize mclBn_getCurveOrder(char *buf, mclSize maxBufSize);

/*
	return decimal string of the characteristic of Fp
	return str(buf) if success
*/
MCLBN_DLL_API mclSize mclBn_getFieldOrder(char *buf, mclSize maxBufSize);

/*
	set ETH serialization mode for BLS12-381
	@param enable [in] 1:enable,  0:disable
	@note ignore the flag if curve is not BLS12-381
*/
MCLBN_DLL_API void mclBn_setETHserialization(int enable);

// return 1 if ETH serialization mode else 0
MCLBN_DLL_API int mclBn_getETHserialization(void);

/*
	set map-to-function to mode (only support MCL_MAP_TO_MODE_HASH_TO_CURVE_07)
	return 0 if success else -1
*/
MCLBN_DLL_API int mclBn_setMapToMode(int mode);

////////////////////////////////////////////////
/*
	deserialize
	return read size if success else 0
*/
MCLBN_DLL_API mclSize mclBnFr_deserialize(mclBnFr *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API mclSize mclBnG1_deserialize(mclBnG1 *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API mclSize mclBnG2_deserialize(mclBnG2 *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API mclSize mclBnGT_deserialize(mclBnGT *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API mclSize mclBnFp_deserialize(mclBnFp *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API mclSize mclBnFp2_deserialize(mclBnFp2 *x, const void *buf, mclSize bufSize);

/*
	serialize
	return written byte if sucess else 0
*/
MCLBN_DLL_API mclSize mclBnFr_serialize(void *buf, mclSize maxBufSize, const mclBnFr *x);
MCLBN_DLL_API mclSize mclBnG1_serialize(void *buf, mclSize maxBufSize, const mclBnG1 *x);
MCLBN_DLL_API mclSize mclBnG2_serialize(void *buf, mclSize maxBufSize, const mclBnG2 *x);
MCLBN_DLL_API mclSize mclBnGT_serialize(void *buf, mclSize maxBufSize, const mclBnGT *x);
MCLBN_DLL_API mclSize mclBnFp_serialize(void *buf, mclSize maxBufSize, const mclBnFp *x);
MCLBN_DLL_API mclSize mclBnFp2_serialize(void *buf, mclSize maxBufSize, const mclBnFp2 *x);

/*
	set string
	ioMode
	10 : decimal number
	16 : hexadecimal number
	MCLBN_IO_SERIALIZE_HEX_STR : hex string of serialized data
	return 0 if success else -1
*/
MCLBN_DLL_API int mclBnFr_setStr(mclBnFr *x, const char *buf, mclSize bufSize, int ioMode);
MCLBN_DLL_API int mclBnG1_setStr(mclBnG1 *x, const char *buf, mclSize bufSize, int ioMode);
MCLBN_DLL_API int mclBnG2_setStr(mclBnG2 *x, const char *buf, mclSize bufSize, int ioMode);
MCLBN_DLL_API int mclBnGT_setStr(mclBnGT *x, const char *buf, mclSize bufSize, int ioMode);
MCLBN_DLL_API int mclBnFp_setStr(mclBnFp *x, const char *buf, mclSize bufSize, int ioMode);

/*
	buf is terminated by '\0'
	return strlen(buf) if sucess else 0
*/
MCLBN_DLL_API mclSize mclBnFr_getStr(char *buf, mclSize maxBufSize, const mclBnFr *x, int ioMode);
MCLBN_DLL_API mclSize mclBnG1_getStr(char *buf, mclSize maxBufSize, const mclBnG1 *x, int ioMode);
MCLBN_DLL_API mclSize mclBnG2_getStr(char *buf, mclSize maxBufSize, const mclBnG2 *x, int ioMode);
MCLBN_DLL_API mclSize mclBnGT_getStr(char *buf, mclSize maxBufSize, const mclBnGT *x, int ioMode);
MCLBN_DLL_API mclSize mclBnFp_getStr(char *buf, mclSize maxBufSize, const mclBnFp *x, int ioMode);

// set zero
MCLBN_DLL_API void mclBnFr_clear(mclBnFr *x);
MCLBN_DLL_API void mclBnFp_clear(mclBnFp *x);
MCLBN_DLL_API void mclBnFp2_clear(mclBnFp2 *x);

// set x to y
MCLBN_DLL_API void mclBnFr_setInt(mclBnFr *y, mclInt x);
MCLBN_DLL_API void mclBnFr_setInt32(mclBnFr *y, int x);
MCLBN_DLL_API void mclBnFp_setInt(mclBnFp *y, mclInt x);
MCLBN_DLL_API void mclBnFp_setInt32(mclBnFp *y, int x);

// x = buf & (1 << bitLen(r)) - 1
// if (x >= r) x &= (1 << (bitLen(r) - 1)) - 1
// always return 0
MCLBN_DLL_API int mclBnFr_setLittleEndian(mclBnFr *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API int mclBnFp_setLittleEndian(mclBnFp *x, const void *buf, mclSize bufSize);

/*
	write a value as little endian
	return written size if success else 0
	@note buf[0] = 0 and return 1 if the value is zero
*/
MCLBN_DLL_API mclSize mclBnFr_getLittleEndian(void *buf, mclSize maxBufSize, const mclBnFr *x);
MCLBN_DLL_API mclSize mclBnFp_getLittleEndian(void *buf, mclSize maxBufSize, const mclBnFp *x);

// set (buf mod r) to x
// return 0 if bufSize <= (byte size of Fr * 2) else -1
MCLBN_DLL_API int mclBnFr_setLittleEndianMod(mclBnFr *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API int mclBnFr_setBigEndianMod(mclBnFr *x, const void *buf, mclSize bufSize);
// set (buf mod p) to x
// return 0 if bufSize <= (byte size of Fp * 2) else -1
MCLBN_DLL_API int mclBnFp_setLittleEndianMod(mclBnFp *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API int mclBnFp_setBigEndianMod(mclBnFp *x, const void *buf, mclSize bufSize);

// return 1 if true and 0 otherwise
MCLBN_DLL_API int mclBnFr_isValid(const mclBnFr *x);
MCLBN_DLL_API int mclBnFr_isEqual(const mclBnFr *x, const mclBnFr *y);
MCLBN_DLL_API int mclBnFr_isZero(const mclBnFr *x);
MCLBN_DLL_API int mclBnFr_isOne(const mclBnFr *x);
MCLBN_DLL_API int mclBnFr_isOdd(const mclBnFr *x);
// return 1 if half <= x < r, where half = (r + 1) / 2 else 0
MCLBN_DLL_API int mclBnFr_isNegative(const mclBnFr *x);

MCLBN_DLL_API int mclBnFp_isValid(const mclBnFp *x);
MCLBN_DLL_API int mclBnFp_isEqual(const mclBnFp *x, const mclBnFp *y);
MCLBN_DLL_API int mclBnFp_isZero(const mclBnFp *x);
MCLBN_DLL_API int mclBnFp_isOne(const mclBnFp *x);
MCLBN_DLL_API int mclBnFp_isOdd(const mclBnFp *x);
// return 1 if half <= x < p, where half = (p + 1) / 2 else 0
MCLBN_DLL_API int mclBnFp_isNegative(const mclBnFp *x);

MCLBN_DLL_API int mclBnFp2_isEqual(const mclBnFp2 *x, const mclBnFp2 *y);
MCLBN_DLL_API int mclBnFp2_isZero(const mclBnFp2 *x);
MCLBN_DLL_API int mclBnFp2_isOne(const mclBnFp2 *x);


#ifndef MCL_DONT_USE_CSRPNG
// return 0 if success
MCLBN_DLL_API int mclBnFr_setByCSPRNG(mclBnFr *x);
MCLBN_DLL_API int mclBnFp_setByCSPRNG(mclBnFp *x);

/*
	set user-defined random function for setByCSPRNG
	@param self [in] user-defined pointer
	@param readFunc [in] user-defined function,
	which writes random bufSize bytes to buf and returns bufSize if success else returns 0
	@note if self == 0 and readFunc == 0 then set default random function
	@note not threadsafe
*/
MCLBN_DLL_API void mclBn_setRandFunc(void *self, unsigned int (*readFunc)(void *self, void *buf, unsigned int bufSize));
#endif

// hash(s) and set x
// return 0 if success
MCLBN_DLL_API int mclBnFr_setHashOf(mclBnFr *x, const void *buf, mclSize bufSize);
MCLBN_DLL_API int mclBnFp_setHashOf(mclBnFp *x, const void *buf, mclSize bufSize);

// map x to y
// return 0 if success else -1
MCLBN_DLL_API int mclBnFp_mapToG1(mclBnG1 *y, const mclBnFp *x);
MCLBN_DLL_API int mclBnFp2_mapToG2(mclBnG2 *y, const mclBnFp2 *x);

MCLBN_DLL_API void mclBnFr_neg(mclBnFr *y, const mclBnFr *x);
MCLBN_DLL_API void mclBnFr_inv(mclBnFr *y, const mclBnFr *x);
MCLBN_DLL_API void mclBnFr_sqr(mclBnFr *y, const mclBnFr *x);
MCLBN_DLL_API void mclBnFr_add(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);
MCLBN_DLL_API void mclBnFr_sub(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);
MCLBN_DLL_API void mclBnFr_mul(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);
MCLBN_DLL_API void mclBnFr_div(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);

MCLBN_DLL_API void mclBnFp_neg(mclBnFp *y, const mclBnFp *x);
MCLBN_DLL_API void mclBnFp_inv(mclBnFp *y, const mclBnFp *x);
MCLBN_DLL_API void mclBnFp_sqr(mclBnFp *y, const mclBnFp *x);
MCLBN_DLL_API void mclBnFp_add(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);
MCLBN_DLL_API void mclBnFp_sub(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);
MCLBN_DLL_API void mclBnFp_mul(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);
MCLBN_DLL_API void mclBnFp_div(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);

MCLBN_DLL_API void mclBnFp2_neg(mclBnFp2 *y, const mclBnFp2 *x);
MCLBN_DLL_API void mclBnFp2_inv(mclBnFp2 *y, const mclBnFp2 *x);
MCLBN_DLL_API void mclBnFp2_sqr(mclBnFp2 *y, const mclBnFp2 *x);
MCLBN_DLL_API void mclBnFp2_add(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);
MCLBN_DLL_API void mclBnFp2_sub(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);
MCLBN_DLL_API void mclBnFp2_mul(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);
MCLBN_DLL_API void mclBnFp2_div(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);

// y is one of square root of x
// return 0 if success else -1
MCLBN_DLL_API int mclBnFr_squareRoot(mclBnFr *y, const mclBnFr *x);
MCLBN_DLL_API int mclBnFp_squareRoot(mclBnFp *y, const mclBnFp *x);
MCLBN_DLL_API int mclBnFp2_squareRoot(mclBnFp2 *y, const mclBnFp2 *x);

////////////////////////////////////////////////
// set zero
MCLBN_DLL_API void mclBnG1_clear(mclBnG1 *x);


// return 1 if true and 0 otherwise
MCLBN_DLL_API int mclBnG1_isValid(const mclBnG1 *x);
MCLBN_DLL_API int mclBnG1_isEqual(const mclBnG1 *x, const mclBnG1 *y);
MCLBN_DLL_API int mclBnG1_isZero(const mclBnG1 *x);
/*
	return 1 if x has a correct order
	x is valid point of G1 if and only if
	mclBnG1_isValid() is true, which contains mclBnG1_isValidOrder() if mclBn_verifyOrderG1(true)
	mclBnG1_isValid() && mclBnG1_isValidOrder() is true if mclBn_verifyOrderG1(false)
*/
MCLBN_DLL_API int mclBnG1_isValidOrder(const mclBnG1 *x);

MCLBN_DLL_API int mclBnG1_hashAndMapTo(mclBnG1 *x, const void *buf, mclSize bufSize);
// user-defined dst
MCLBN_DLL_API int mclBnG1_hashAndMapToWithDst(mclBnG1 *x, const void *buf, mclSize bufSize, const char *dst, mclSize dstSize);
// set default dst
MCLBN_DLL_API int mclBnG1_setDst(const char *dst, mclSize dstSize);


MCLBN_DLL_API void mclBnG1_neg(mclBnG1 *y, const mclBnG1 *x);
MCLBN_DLL_API void mclBnG1_dbl(mclBnG1 *y, const mclBnG1 *x);
MCLBN_DLL_API void mclBnG1_normalize(mclBnG1 *y, const mclBnG1 *x);
MCLBN_DLL_API void mclBnG1_add(mclBnG1 *z, const mclBnG1 *x, const mclBnG1 *y);
MCLBN_DLL_API void mclBnG1_sub(mclBnG1 *z, const mclBnG1 *x, const mclBnG1 *y);
MCLBN_DLL_API void mclBnG1_mul(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y);

/*
	constant time mul
*/
MCLBN_DLL_API void mclBnG1_mulCT(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y);

////////////////////////////////////////////////
// set zero
MCLBN_DLL_API void mclBnG2_clear(mclBnG2 *x);

// return 1 if true and 0 otherwise
MCLBN_DLL_API int mclBnG2_isValid(const mclBnG2 *x);
MCLBN_DLL_API int mclBnG2_isEqual(const mclBnG2 *x, const mclBnG2 *y);
MCLBN_DLL_API int mclBnG2_isZero(const mclBnG2 *x);
// return 1 if x has a correct order
MCLBN_DLL_API int mclBnG2_isValidOrder(const mclBnG2 *x);

MCLBN_DLL_API int mclBnG2_hashAndMapTo(mclBnG2 *x, const void *buf, mclSize bufSize);
// user-defined dst
MCLBN_DLL_API int mclBnG2_hashAndMapToWithDst(mclBnG2 *x, const void *buf, mclSize bufSize, const char *dst, mclSize dstSize);
// set default dst
MCLBN_DLL_API int mclBnG2_setDst(const char *dst, mclSize dstSize);

// return written size if sucess else 0

MCLBN_DLL_API void mclBnG2_neg(mclBnG2 *y, const mclBnG2 *x);
MCLBN_DLL_API void mclBnG2_dbl(mclBnG2 *y, const mclBnG2 *x);
MCLBN_DLL_API void mclBnG2_normalize(mclBnG2 *y, const mclBnG2 *x);
MCLBN_DLL_API void mclBnG2_add(mclBnG2 *z, const mclBnG2 *x, const mclBnG2 *y);
MCLBN_DLL_API void mclBnG2_sub(mclBnG2 *z, const mclBnG2 *x, const mclBnG2 *y);
MCLBN_DLL_API void mclBnG2_mul(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y);
/*
	constant time mul
*/
MCLBN_DLL_API void mclBnG2_mulCT(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y);

////////////////////////////////////////////////
// set zero
MCLBN_DLL_API void mclBnGT_clear(mclBnGT *x);
// set x to y
MCLBN_DLL_API void mclBnGT_setInt(mclBnGT *y, mclInt x);
MCLBN_DLL_API void mclBnGT_setInt32(mclBnGT *y, int x);

// return 1 if true and 0 otherwise
MCLBN_DLL_API int mclBnGT_isEqual(const mclBnGT *x, const mclBnGT *y);
MCLBN_DLL_API int mclBnGT_isZero(const mclBnGT *x);
MCLBN_DLL_API int mclBnGT_isOne(const mclBnGT *x);

MCLBN_DLL_API void mclBnGT_neg(mclBnGT *y, const mclBnGT *x);
MCLBN_DLL_API void mclBnGT_sqr(mclBnGT *y, const mclBnGT *x);
MCLBN_DLL_API void mclBnGT_add(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);
MCLBN_DLL_API void mclBnGT_sub(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);
MCLBN_DLL_API void mclBnGT_mul(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);
MCLBN_DLL_API void mclBnGT_div(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);

// y = conjugate of x in Fp12, which is equal to the inverse of x if |x|^r = 1
MCLBN_DLL_API void mclBnGT_inv(mclBnGT *y, const mclBnGT *x);
// use invGeneric when x in Fp12 is not in GT
MCLBN_DLL_API void mclBnGT_invGeneric(mclBnGT *y, const mclBnGT *x);

/*
	pow for all elements of Fp12
*/
MCLBN_DLL_API void mclBnGT_powGeneric(mclBnGT *z, const mclBnGT *x, const mclBnFr *y);
/*
	pow for only {x|x^r = 1} in GT by GLV method
	the value generated by pairing satisfies the condition
*/
MCLBN_DLL_API void mclBnGT_pow(mclBnGT *z, const mclBnGT *x, const mclBnFr *y);

// z = sum_{i=0}^{n-1} x[i] y[i]
MCLBN_DLL_API void mclBnG1_mulVec(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y, mclSize n);
MCLBN_DLL_API void mclBnG2_mulVec(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y, mclSize n);
MCLBN_DLL_API void mclBnGT_powVec(mclBnGT *z, const mclBnGT *x, const mclBnFr *y, mclSize n);

MCLBN_DLL_API void mclBn_pairing(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y);
MCLBN_DLL_API void mclBn_finalExp(mclBnGT *y, const mclBnGT *x);
MCLBN_DLL_API void mclBn_millerLoop(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y);
// z = prod_{i=0}^{n-1} millerLoop(x[i], y[i])
MCLBN_DLL_API void mclBn_millerLoopVec(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y, mclSize n);
// multi thread version of millerLoopVec/mclBnG1_mulVec/mclBnG2_mulVec (enabled if the library built with MCL_USE_OMP=1)
// the num of thread is automatically detected if cpuN = 0
MCLBN_DLL_API void mclBn_millerLoopVecMT(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y, mclSize n, mclSize cpuN);
MCLBN_DLL_API void mclBnG1_mulVecMT(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y, mclSize n, mclSize cpuN);
MCLBN_DLL_API void mclBnG2_mulVecMT(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y, mclSize n, mclSize cpuN);

// return precomputedQcoeffSize * sizeof(Fp6) / sizeof(uint64_t)
MCLBN_DLL_API int mclBn_getUint64NumToPrecompute(void);

// allocate Qbuf[MCLBN_getUint64NumToPrecompute()] before calling this
MCLBN_DLL_API void mclBn_precomputeG2(uint64_t *Qbuf, const mclBnG2 *Q);

MCLBN_DLL_API void mclBn_precomputedMillerLoop(mclBnGT *f, const mclBnG1 *P, const uint64_t *Qbuf);
MCLBN_DLL_API void mclBn_precomputedMillerLoop2(mclBnGT *f, const mclBnG1 *P1, const uint64_t *Q1buf, const mclBnG1 *P2, const uint64_t *Q2buf);
MCLBN_DLL_API void mclBn_precomputedMillerLoop2mixed(mclBnGT *f, const mclBnG1 *P1, const mclBnG2 *Q1, const mclBnG1 *P2, const uint64_t *Q2buf);

/*
	Lagrange interpolation
	recover out = y(0) by { (xVec[i], yVec[i]) }
	return 0 if success else -1
	@note *out = yVec[0] if k = 1
	@note k >= 2, xVec[i] != 0, xVec[i] != xVec[j] for i != j
*/
MCLBN_DLL_API int mclBn_FrLagrangeInterpolation(mclBnFr *out, const mclBnFr *xVec, const mclBnFr *yVec, mclSize k);
MCLBN_DLL_API int mclBn_G1LagrangeInterpolation(mclBnG1 *out, const mclBnFr *xVec, const mclBnG1 *yVec, mclSize k);
MCLBN_DLL_API int mclBn_G2LagrangeInterpolation(mclBnG2 *out, const mclBnFr *xVec, const mclBnG2 *yVec, mclSize k);

/*
	evaluate polynomial
	out = f(x) = c[0] + c[1] * x + c[2] * x^2 + ... + c[cSize - 1] * x^(cSize - 1)
	return 0 if success else -1
	@note cSize >= 1
*/
MCLBN_DLL_API int mclBn_FrEvaluatePolynomial(mclBnFr *out, const mclBnFr *cVec, mclSize cSize, const mclBnFr *x);
MCLBN_DLL_API int mclBn_G1EvaluatePolynomial(mclBnG1 *out, const mclBnG1 *cVec, mclSize cSize, const mclBnFr *x);
MCLBN_DLL_API int mclBn_G2EvaluatePolynomial(mclBnG2 *out, const mclBnG2 *cVec, mclSize cSize, const mclBnFr *x);

/*
	verify whether a point of an elliptic curve has order r
	This api affetcs setStr(), deserialize() for G2 on BN or G1/G2 on BLS12
	@param doVerify [in] does not verify if zero(default 1)
*/
MCLBN_DLL_API void mclBn_verifyOrderG1(int doVerify);
MCLBN_DLL_API void mclBn_verifyOrderG2(int doVerify);

/*
	EXPERIMENTAL
	only for curve = MCL_SECP* or MCL_NIST*
	return standard base point of the current elliptic curve
*/
MCLBN_DLL_API int mclBnG1_getBasePoint(mclBnG1 *x);

#ifdef __cplusplus
}
#endif
