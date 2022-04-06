/*
	This is an internal header
	Do not include this
*/
#define MCLBN_DLL_EXPORT
#include <mcl/bn.h>

#if MCLBN_FP_UNIT_SIZE == 4 && MCLBN_FR_UNIT_SIZE == 4
#include <mcl/bn256.hpp>
#elif MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE == 6
#include <mcl/bn384.hpp>
#elif MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE == 4
#include <mcl/bls12_381.hpp>
#elif MCLBN_FP_UNIT_SIZE == 8 && MCLBN_FR_UNIT_SIZE == 8
#include <mcl/bn512.hpp>
#else
	#error "not supported size"
#endif
#include <mcl/lagrange.hpp>
#include <mcl/ecparam.hpp>
using namespace mcl::bn;

static Fr *cast(mclBnFr *p) { return reinterpret_cast<Fr*>(p); }
static const Fr *cast(const mclBnFr *p) { return reinterpret_cast<const Fr*>(p); }

static G1 *cast(mclBnG1 *p) { return reinterpret_cast<G1*>(p); }
static const G1 *cast(const mclBnG1 *p) { return reinterpret_cast<const G1*>(p); }

static G2 *cast(mclBnG2 *p) { return reinterpret_cast<G2*>(p); }
static const G2 *cast(const mclBnG2 *p) { return reinterpret_cast<const G2*>(p); }

static Fp12 *cast(mclBnGT *p) { return reinterpret_cast<Fp12*>(p); }
static const Fp12 *cast(const mclBnGT *p) { return reinterpret_cast<const Fp12*>(p); }

static Fp6 *cast(uint64_t *p) { return reinterpret_cast<Fp6*>(p); }
static const Fp6 *cast(const uint64_t *p) { return reinterpret_cast<const Fp6*>(p); }

static Fp2 *cast(mclBnFp2 *p) { return reinterpret_cast<Fp2*>(p); }
static const Fp2 *cast(const mclBnFp2 *p) { return reinterpret_cast<const Fp2*>(p); }

static Fp *cast(mclBnFp *p) { return reinterpret_cast<Fp*>(p); }
static const Fp *cast(const mclBnFp *p) { return reinterpret_cast<const Fp*>(p); }

template<class T>
int setStr(T *x, const char *buf, mclSize bufSize, int ioMode)
{
	size_t n = cast(x)->deserialize(buf, bufSize, ioMode);
	return n > 0 ? 0 : -1;
}

#ifdef __EMSCRIPTEN__
// use these functions forcibly
extern "C" MCLBN_DLL_API void *mclBnMalloc(size_t n)
{
	return malloc(n);
}
extern "C" MCLBN_DLL_API void mclBnFree(void *p)
{
	free(p);
}
#endif

int mclBn_getVersion()
{
	return mcl::version;
}

int mclBn_init(int curve, int compiledTimeVar)
{
	if (compiledTimeVar != MCLBN_COMPILED_TIME_VAR) {
		return -(compiledTimeVar | (MCLBN_COMPILED_TIME_VAR * 100));
	}
	if (MCL_EC_BEGIN <= curve && curve < MCL_EC_END) {
		const mcl::EcParam *para = mcl::getEcParam(curve);
		if (para == 0) return -2;
		bool b;
		initG1only(&b, *para);
		return b ? 0 : -1;
	}
	const mcl::CurveParam* cp = mcl::getCurveParam(curve);
	if (cp == 0) return -1;
	bool b;
	initPairing(&b, *cp);
	return b ? 0 : -1;
}

int mclBn_getCurveType()
{
	return mcl::bn::BN::param.cp.curveType;
}

int mclBn_getOpUnitSize()
{
	return (int)Fp::getUnitSize() * sizeof(mcl::fp::Unit) / sizeof(uint64_t);
}

int mclBn_getG1ByteSize()
{
	return mclBn_getFpByteSize();
}

int mclBn_getFrByteSize()
{
	return (int)Fr::getByteSize();
}

int mclBn_getFpByteSize()
{
	return (int)Fp::getByteSize();
}

mclSize mclBn_getCurveOrder(char *buf, mclSize maxBufSize)
{
	return Fr::getModulo(buf, maxBufSize);
}

mclSize mclBn_getFieldOrder(char *buf, mclSize maxBufSize)
{
	return Fp::getModulo(buf, maxBufSize);
}

void mclBn_setETHserialization(int enable)
{
	if (mclBn_getCurveType() != MCL_BLS12_381) return;
	Fp::setETHserialization(enable == 1);
	Fr::setETHserialization(enable == 1);
}

int mclBn_getETHserialization()
{
	return Fp::getETHserialization() ? 1 : 0;
}

int mclBn_setMapToMode(int mode)
{
	return setMapToMode(mode) ? 0 : -1;
}

int mclBnG1_setDst(const char *dst, mclSize dstSize)
{
	return setDstG1(dst, dstSize) ? 0 : -1;
}

int mclBnG2_setDst(const char *dst, mclSize dstSize)
{
	return setDstG2(dst, dstSize) ? 0 : -1;
}

////////////////////////////////////////////////
// set zero
void mclBnFr_clear(mclBnFr *x)
{
	cast(x)->clear();
}

// set x to y
void mclBnFr_setInt(mclBnFr *y, mclInt x)
{
	*cast(y) = x;
}
void mclBnFr_setInt32(mclBnFr *y, int x)
{
	*cast(y) = x;
}

int mclBnFr_setStr(mclBnFr *x, const char *buf, mclSize bufSize, int ioMode)
{
	return setStr(x, buf, bufSize, ioMode);
}
int mclBnFr_setLittleEndian(mclBnFr *x, const void *buf, mclSize bufSize)
{
	cast(x)->setArrayMask((const uint8_t *)buf, bufSize);
	return 0;
}
int mclBnFr_setBigEndianMod(mclBnFr *x, const void *buf, mclSize bufSize)
{
	bool b;
	cast(x)->setBigEndianMod(&b, (const uint8_t*)buf, bufSize);
	return b ? 0 : -1;
}

mclSize mclBnFr_getLittleEndian(void *buf, mclSize maxBufSize, const mclBnFr *x)
{
	return cast(x)->getLittleEndian((uint8_t*)buf, maxBufSize);
}
int mclBnFr_setLittleEndianMod(mclBnFr *x, const void *buf, mclSize bufSize)
{
	bool b;
	cast(x)->setArrayMod(&b, (const uint8_t *)buf, bufSize);
	return b ? 0 : -1;
}
mclSize mclBnFr_deserialize(mclBnFr *x, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(x)->deserialize(buf, bufSize);
}
// return 1 if true
int mclBnFr_isValid(const mclBnFr *x)
{
	return cast(x)->isValid();
}
int mclBnFr_isEqual(const mclBnFr *x, const mclBnFr *y)
{
	return *cast(x) == *cast(y);
}
int mclBnFr_isZero(const mclBnFr *x)
{
	return cast(x)->isZero();
}
int mclBnFr_isOne(const mclBnFr *x)
{
	return cast(x)->isOne();
}
int mclBnFr_isOdd(const mclBnFr *x)
{
	return cast(x)->isOdd();
}
int mclBnFr_isNegative(const mclBnFr *x)
{
	return cast(x)->isNegative();
}

#ifndef MCL_DONT_USE_CSRPNG
int mclBnFr_setByCSPRNG(mclBnFr *x)
{
	bool b;
	cast(x)->setByCSPRNG(&b);
	return b ? 0 : -1;
}
int mclBnFp_setByCSPRNG(mclBnFp *x)
{
	bool b;
	cast(x)->setByCSPRNG(&b);
	return b ? 0 : -1;
}
void mclBn_setRandFunc(void *self, unsigned int (*readFunc)(void *self, void *buf, unsigned int bufSize))
{
	mcl::fp::RandGen::setRandFunc(self, readFunc);
}
#endif

// hash(buf) and set x
int mclBnFr_setHashOf(mclBnFr *x, const void *buf, mclSize bufSize)
{
	cast(x)->setHashOf(buf, bufSize);
	return 0;
}

mclSize mclBnFr_getStr(char *buf, mclSize maxBufSize, const mclBnFr *x, int ioMode)
{
	return cast(x)->getStr(buf, maxBufSize, ioMode);
}
mclSize mclBnFr_serialize(void *buf, mclSize maxBufSize, const mclBnFr *x)
{
	return (mclSize)cast(x)->serialize(buf, maxBufSize);
}

void mclBnFr_neg(mclBnFr *y, const mclBnFr *x)
{
	Fr::neg(*cast(y), *cast(x));
}
void mclBnFr_inv(mclBnFr *y, const mclBnFr *x)
{
	Fr::inv(*cast(y), *cast(x));
}
void mclBnFr_sqr(mclBnFr *y, const mclBnFr *x)
{
	Fr::sqr(*cast(y), *cast(x));
}
void mclBnFr_add(mclBnFr *z, const mclBnFr *x, const mclBnFr *y)
{
	Fr::add(*cast(z),*cast(x), *cast(y));
}
void mclBnFr_sub(mclBnFr *z, const mclBnFr *x, const mclBnFr *y)
{
	Fr::sub(*cast(z),*cast(x), *cast(y));
}
void mclBnFr_mul(mclBnFr *z, const mclBnFr *x, const mclBnFr *y)
{
	Fr::mul(*cast(z),*cast(x), *cast(y));
}
void mclBnFr_div(mclBnFr *z, const mclBnFr *x, const mclBnFr *y)
{
	Fr::div(*cast(z),*cast(x), *cast(y));
}

void mclBnFp_neg(mclBnFp *y, const mclBnFp *x)
{
	Fp::neg(*cast(y), *cast(x));
}
void mclBnFp_inv(mclBnFp *y, const mclBnFp *x)
{
	Fp::inv(*cast(y), *cast(x));
}
void mclBnFp_sqr(mclBnFp *y, const mclBnFp *x)
{
	Fp::sqr(*cast(y), *cast(x));
}
void mclBnFp_add(mclBnFp *z, const mclBnFp *x, const mclBnFp *y)
{
	Fp::add(*cast(z),*cast(x), *cast(y));
}
void mclBnFp_sub(mclBnFp *z, const mclBnFp *x, const mclBnFp *y)
{
	Fp::sub(*cast(z),*cast(x), *cast(y));
}
void mclBnFp_mul(mclBnFp *z, const mclBnFp *x, const mclBnFp *y)
{
	Fp::mul(*cast(z),*cast(x), *cast(y));
}
void mclBnFp_div(mclBnFp *z, const mclBnFp *x, const mclBnFp *y)
{
	Fp::div(*cast(z),*cast(x), *cast(y));
}

void mclBnFp2_neg(mclBnFp2 *y, const mclBnFp2 *x)
{
	Fp2::neg(*cast(y), *cast(x));
}
void mclBnFp2_inv(mclBnFp2 *y, const mclBnFp2 *x)
{
	Fp2::inv(*cast(y), *cast(x));
}
void mclBnFp2_sqr(mclBnFp2 *y, const mclBnFp2 *x)
{
	Fp2::sqr(*cast(y), *cast(x));
}
void mclBnFp2_add(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y)
{
	Fp2::add(*cast(z),*cast(x), *cast(y));
}
void mclBnFp2_sub(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y)
{
	Fp2::sub(*cast(z),*cast(x), *cast(y));
}
void mclBnFp2_mul(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y)
{
	Fp2::mul(*cast(z),*cast(x), *cast(y));
}
void mclBnFp2_div(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y)
{
	Fp2::div(*cast(z),*cast(x), *cast(y));
}

int mclBnFr_squareRoot(mclBnFr *y, const mclBnFr *x)
{
	return Fr::squareRoot(*cast(y), *cast(x)) ? 0 : -1;
}
int mclBnFp_squareRoot(mclBnFp *y, const mclBnFp *x)
{
	return Fp::squareRoot(*cast(y), *cast(x)) ? 0 : -1;
}
int mclBnFp2_squareRoot(mclBnFp2 *y, const mclBnFp2 *x)
{
	return Fp2::squareRoot(*cast(y), *cast(x)) ? 0 : -1;
}

////////////////////////////////////////////////
// set zero
void mclBnG1_clear(mclBnG1 *x)
{
	cast(x)->clear();
}

int mclBnG1_setStr(mclBnG1 *x, const char *buf, mclSize bufSize, int ioMode)
{
	return setStr(x, buf, bufSize, ioMode);
}
mclSize mclBnG1_deserialize(mclBnG1 *x, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(x)->deserialize(buf, bufSize);
}

// return 1 if true
int mclBnG1_isValid(const mclBnG1 *x)
{
	return cast(x)->isValid();
}
int mclBnG1_isEqual(const mclBnG1 *x, const mclBnG1 *y)
{
	return *cast(x) == *cast(y);
}
int mclBnG1_isZero(const mclBnG1 *x)
{
	return cast(x)->isZero();
}
int mclBnG1_isValidOrder(const mclBnG1 *x)
{
	return cast(x)->isValidOrder();
}

int mclBnG1_hashAndMapTo(mclBnG1 *x, const void *buf, mclSize bufSize)
{
	hashAndMapToG1(*cast(x), buf, bufSize);
	return 0;
}
int mclBnG1_hashAndMapToWithDst(mclBnG1 *x, const void *buf, mclSize bufSize, const char *dst, mclSize dstSize)
{
	hashAndMapToG1(*cast(x), buf, bufSize, dst, dstSize);
	return 0;
}

mclSize mclBnG1_getStr(char *buf, mclSize maxBufSize, const mclBnG1 *x, int ioMode)
{
	return cast(x)->getStr(buf, maxBufSize, ioMode);
}

mclSize mclBnG1_serialize(void *buf, mclSize maxBufSize, const mclBnG1 *x)
{
	return (mclSize)cast(x)->serialize(buf, maxBufSize);
}

void mclBnG1_neg(mclBnG1 *y, const mclBnG1 *x)
{
	G1::neg(*cast(y), *cast(x));
}
void mclBnG1_dbl(mclBnG1 *y, const mclBnG1 *x)
{
	G1::dbl(*cast(y), *cast(x));
}
void mclBnG1_normalize(mclBnG1 *y, const mclBnG1 *x)
{
	G1::normalize(*cast(y), *cast(x));
}
void mclBnG1_add(mclBnG1 *z, const mclBnG1 *x, const mclBnG1 *y)
{
	G1::add(*cast(z),*cast(x), *cast(y));
}
void mclBnG1_sub(mclBnG1 *z, const mclBnG1 *x, const mclBnG1 *y)
{
	G1::sub(*cast(z),*cast(x), *cast(y));
}
void mclBnG1_mul(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y)
{
	G1::mul(*cast(z),*cast(x), *cast(y));
}
void mclBnG1_mulCT(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y)
{
	G1::mulCT(*cast(z),*cast(x), *cast(y));
}

////////////////////////////////////////////////
// set zero
void mclBnG2_clear(mclBnG2 *x)
{
	cast(x)->clear();
}

int mclBnG2_setStr(mclBnG2 *x, const char *buf, mclSize bufSize, int ioMode)
{
	return setStr(x, buf, bufSize, ioMode);
}
mclSize mclBnG2_deserialize(mclBnG2 *x, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(x)->deserialize(buf, bufSize);
}

// return 1 if true
int mclBnG2_isValid(const mclBnG2 *x)
{
	return cast(x)->isValid();
}
int mclBnG2_isEqual(const mclBnG2 *x, const mclBnG2 *y)
{
	return *cast(x) == *cast(y);
}
int mclBnG2_isZero(const mclBnG2 *x)
{
	return cast(x)->isZero();
}
int mclBnG2_isValidOrder(const mclBnG2 *x)
{
	return cast(x)->isValidOrder();
}

int mclBnG2_hashAndMapTo(mclBnG2 *x, const void *buf, mclSize bufSize)
{
	hashAndMapToG2(*cast(x), buf, bufSize);
	return 0;
}
int mclBnG2_hashAndMapToWithDst(mclBnG2 *x, const void *buf, mclSize bufSize, const char *dst, mclSize dstSize)
{
	hashAndMapToG2(*cast(x), buf, bufSize, dst, dstSize);
	return 0;
}

mclSize mclBnG2_getStr(char *buf, mclSize maxBufSize, const mclBnG2 *x, int ioMode)
{
	return cast(x)->getStr(buf, maxBufSize, ioMode);
}

mclSize mclBnG2_serialize(void *buf, mclSize maxBufSize, const mclBnG2 *x)
{
	return (mclSize)cast(x)->serialize(buf, maxBufSize);
}

void mclBnG2_neg(mclBnG2 *y, const mclBnG2 *x)
{
	G2::neg(*cast(y), *cast(x));
}
void mclBnG2_dbl(mclBnG2 *y, const mclBnG2 *x)
{
	G2::dbl(*cast(y), *cast(x));
}
void mclBnG2_normalize(mclBnG2 *y, const mclBnG2 *x)
{
	G2::normalize(*cast(y), *cast(x));
}
void mclBnG2_add(mclBnG2 *z, const mclBnG2 *x, const mclBnG2 *y)
{
	G2::add(*cast(z),*cast(x), *cast(y));
}
void mclBnG2_sub(mclBnG2 *z, const mclBnG2 *x, const mclBnG2 *y)
{
	G2::sub(*cast(z),*cast(x), *cast(y));
}
void mclBnG2_mul(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y)
{
	G2::mul(*cast(z),*cast(x), *cast(y));
}
void mclBnG2_mulCT(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y)
{
	G2::mulCT(*cast(z),*cast(x), *cast(y));
}

////////////////////////////////////////////////
// set zero
void mclBnGT_clear(mclBnGT *x)
{
	cast(x)->clear();
}
void mclBnGT_setInt(mclBnGT *y, mclInt x)
{
	cast(y)->clear();
	*(cast(y)->getFp0()) = x;
}
void mclBnGT_setInt32(mclBnGT *y, int x)
{
	cast(y)->clear();
	*(cast(y)->getFp0()) = x;
}

int mclBnGT_setStr(mclBnGT *x, const char *buf, mclSize bufSize, int ioMode)
{
	return setStr(x, buf, bufSize, ioMode);
}
mclSize mclBnGT_deserialize(mclBnGT *x, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(x)->deserialize(buf, bufSize);
}

// return 1 if true
int mclBnGT_isEqual(const mclBnGT *x, const mclBnGT *y)
{
	return *cast(x) == *cast(y);
}
int mclBnGT_isZero(const mclBnGT *x)
{
	return cast(x)->isZero();
}
int mclBnGT_isOne(const mclBnGT *x)
{
	return cast(x)->isOne();
}

mclSize mclBnGT_getStr(char *buf, mclSize maxBufSize, const mclBnGT *x, int ioMode)
{
	return cast(x)->getStr(buf, maxBufSize, ioMode);
}

mclSize mclBnGT_serialize(void *buf, mclSize maxBufSize, const mclBnGT *x)
{
	return (mclSize)cast(x)->serialize(buf, maxBufSize);
}

void mclBnGT_neg(mclBnGT *y, const mclBnGT *x)
{
	Fp12::neg(*cast(y), *cast(x));
}
void mclBnGT_inv(mclBnGT *y, const mclBnGT *x)
{
	Fp12::unitaryInv(*cast(y), *cast(x));
}
void mclBnGT_invGeneric(mclBnGT *y, const mclBnGT *x)
{
	Fp12::inv(*cast(y), *cast(x));
}
void mclBnGT_sqr(mclBnGT *y, const mclBnGT *x)
{
	Fp12::sqr(*cast(y), *cast(x));
}
void mclBnGT_add(mclBnGT *z, const mclBnGT *x, const mclBnGT *y)
{
	Fp12::add(*cast(z),*cast(x), *cast(y));
}
void mclBnGT_sub(mclBnGT *z, const mclBnGT *x, const mclBnGT *y)
{
	Fp12::sub(*cast(z),*cast(x), *cast(y));
}
void mclBnGT_mul(mclBnGT *z, const mclBnGT *x, const mclBnGT *y)
{
	Fp12::mul(*cast(z),*cast(x), *cast(y));
}
void mclBnGT_div(mclBnGT *z, const mclBnGT *x, const mclBnGT *y)
{
	Fp12::div(*cast(z),*cast(x), *cast(y));
}

void mclBnGT_pow(mclBnGT *z, const mclBnGT *x, const mclBnFr *y)
{
	Fp12::pow(*cast(z), *cast(x), *cast(y));
}
void mclBnGT_powGeneric(mclBnGT *z, const mclBnGT *x, const mclBnFr *y)
{
	Fp12::powGeneric(*cast(z), *cast(x), *cast(y));
}

void mclBnG1_mulVec(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y, mclSize n)
{
	G1::mulVec(*cast(z), cast(x), cast(y), n);
}
void mclBnG2_mulVec(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y, mclSize n)
{
	G2::mulVec(*cast(z), cast(x), cast(y), n);
}
void mclBnGT_powVec(mclBnGT *z, const mclBnGT *x, const mclBnFr *y, mclSize n)
{
	GT::powVec(*cast(z), cast(x), cast(y), n);
}

void mclBn_pairing(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y)
{
	pairing(*cast(z), *cast(x), *cast(y));
}
void mclBn_finalExp(mclBnGT *y, const mclBnGT *x)
{
	finalExp(*cast(y), *cast(x));
}
void mclBn_millerLoop(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y)
{
	millerLoop(*cast(z), *cast(x), *cast(y));
}
void mclBn_millerLoopVec(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y, mclSize n)
{
	millerLoopVec(*cast(z), cast(x), cast(y), n);
}
void mclBn_millerLoopVecMT(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y, mclSize n, mclSize cpuN)
{
	millerLoopVecMT(*cast(z), cast(x), cast(y), n, cpuN);
}
void mclBnG1_mulVecMT(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y, mclSize n, mclSize cpuN)
{
	G1::mulVecMT(*cast(z), cast(x), cast(y), n, cpuN);
}
void mclBnG2_mulVecMT(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y, mclSize n, mclSize cpuN)
{
	G2::mulVecMT(*cast(z), cast(x), cast(y), n, cpuN);
}
int mclBn_getUint64NumToPrecompute(void)
{
	return int(BN::param.precomputedQcoeffSize * sizeof(Fp6) / sizeof(uint64_t));
}

void mclBn_precomputeG2(uint64_t *Qbuf, const mclBnG2 *Q)
{
	precomputeG2(cast(Qbuf), *cast(Q));
}

void mclBn_precomputedMillerLoop(mclBnGT *f, const mclBnG1 *P, const uint64_t *Qbuf)
{
	precomputedMillerLoop(*cast(f), *cast(P), cast(Qbuf));
}

void mclBn_precomputedMillerLoop2(mclBnGT *f, const mclBnG1 *P1, const uint64_t  *Q1buf, const mclBnG1 *P2, const uint64_t *Q2buf)
{
	precomputedMillerLoop2(*cast(f), *cast(P1), cast(Q1buf), *cast(P2), cast(Q2buf));
}

void mclBn_precomputedMillerLoop2mixed(mclBnGT *f, const mclBnG1 *P1, const mclBnG2 *Q1, const mclBnG1 *P2, const uint64_t *Q2buf)
{
	precomputedMillerLoop2mixed(*cast(f), *cast(P1), *cast(Q1), *cast(P2), cast(Q2buf));
}

int mclBn_FrLagrangeInterpolation(mclBnFr *out, const mclBnFr *xVec, const mclBnFr *yVec, mclSize k)
{
	bool b;
	mcl::LagrangeInterpolation(&b, *cast(out), cast(xVec), cast(yVec), k);
	return b ? 0 : -1;
}
int mclBn_G1LagrangeInterpolation(mclBnG1 *out, const mclBnFr *xVec, const mclBnG1 *yVec, mclSize k)
{
	bool b;
	mcl::LagrangeInterpolation(&b, *cast(out), cast(xVec), cast(yVec), k);
	return b ? 0 : -1;
}
int mclBn_G2LagrangeInterpolation(mclBnG2 *out, const mclBnFr *xVec, const mclBnG2 *yVec, mclSize k)
{
	bool b;
	mcl::LagrangeInterpolation(&b, *cast(out), cast(xVec), cast(yVec), k);
	return b ? 0 : -1;
}
int mclBn_FrEvaluatePolynomial(mclBnFr *out, const mclBnFr *cVec, mclSize cSize, const mclBnFr *x)
{
	bool b;
	mcl::evaluatePolynomial(&b, *cast(out), cast(cVec), cSize, *cast(x));
	return b ? 0 : -1;
}
int mclBn_G1EvaluatePolynomial(mclBnG1 *out, const mclBnG1 *cVec, mclSize cSize, const mclBnFr *x)
{
	bool b;
	mcl::evaluatePolynomial(&b, *cast(out), cast(cVec), cSize, *cast(x));
	return b ? 0 : -1;
}
int mclBn_G2EvaluatePolynomial(mclBnG2 *out, const mclBnG2 *cVec, mclSize cSize, const mclBnFr *x)
{
	bool b;
	mcl::evaluatePolynomial(&b, *cast(out), cast(cVec), cSize, *cast(x));
	return b ? 0 : -1;
}

void mclBn_verifyOrderG1(int doVerify)
{
	verifyOrderG1(doVerify != 0);
}

void mclBn_verifyOrderG2(int doVerify)
{
	verifyOrderG2(doVerify != 0);
}

void mclBnFp_setInt(mclBnFp *y, mclInt x)
{
	*cast(y) = x;
}
void mclBnFp_setInt32(mclBnFp *y, int x)
{
	*cast(y) = x;
}

mclSize mclBnFp_getStr(char *buf, mclSize maxBufSize, const mclBnFp *x, int ioMode)
{
	return cast(x)->getStr(buf, maxBufSize, ioMode);
}
int mclBnFp_setStr(mclBnFp *x, const char *buf, mclSize bufSize, int ioMode)
{
	return setStr(x, buf, bufSize, ioMode);
}
mclSize mclBnFp_deserialize(mclBnFp *x, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(x)->deserialize(buf, bufSize);
}

mclSize mclBnFp_serialize(void *buf, mclSize maxBufSize, const mclBnFp *x)
{
	return (mclSize)cast(x)->serialize(buf, maxBufSize);
}

void mclBnFp_clear(mclBnFp *x)
{
	cast(x)->clear();
}

int mclBnFp_setLittleEndian(mclBnFp *x, const void *buf, mclSize bufSize)
{
	cast(x)->setArrayMask((const uint8_t *)buf, bufSize);
	return 0;
}

int mclBnFp_setLittleEndianMod(mclBnFp *x, const void *buf, mclSize bufSize)
{
	bool b;
	cast(x)->setLittleEndianMod(&b, (const uint8_t*)buf, bufSize);
	return b ? 0 : -1;
}

int mclBnFp_setBigEndianMod(mclBnFp *x, const void *buf, mclSize bufSize)
{
	bool b;
	cast(x)->setBigEndianMod(&b, (const uint8_t*)buf, bufSize);
	return b ? 0 : -1;
}

mclSize mclBnFp_getLittleEndian(void *buf, mclSize maxBufSize, const mclBnFp *x)
{
	return cast(x)->getLittleEndian((uint8_t*)buf, maxBufSize);
}
int mclBnFp_isValid(const mclBnFp *x)
{
	return cast(x)->isValid();
}
int mclBnFp_isEqual(const mclBnFp *x, const mclBnFp *y)
{
	return *cast(x) == *cast(y);
}
int mclBnFp_isZero(const mclBnFp *x)
{
	return cast(x)->isZero();
}
int mclBnFp_isOne(const mclBnFp *x)
{
	return cast(x)->isOne();
}
int mclBnFp_isOdd(const mclBnFp *x)
{
	return cast(x)->isOdd();
}
int mclBnFp_isNegative(const mclBnFp *x)
{
	return cast(x)->isNegative();
}

int mclBnFp_setHashOf(mclBnFp *x, const void *buf, mclSize bufSize)
{
	cast(x)->setHashOf(buf, bufSize);
	return 0;
}

int mclBnFp_mapToG1(mclBnG1 *y, const mclBnFp *x)
{
	bool b;
	mapToG1(&b, *cast(y), *cast(x));
	return b ? 0 : -1;
}

mclSize mclBnFp2_deserialize(mclBnFp2 *x, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(x)->deserialize(buf, bufSize);
}

mclSize mclBnFp2_serialize(void *buf, mclSize maxBufSize, const mclBnFp2 *x)
{
	return (mclSize)cast(x)->serialize(buf, maxBufSize);
}

void mclBnFp2_clear(mclBnFp2 *x)
{
	cast(x)->clear();
}

int mclBnFp2_isEqual(const mclBnFp2 *x, const mclBnFp2 *y)
{
	return *cast(x) == *cast(y);
}
int mclBnFp2_isZero(const mclBnFp2 *x)
{
	return cast(x)->isZero();
}
int mclBnFp2_isOne(const mclBnFp2 *x)
{
	return cast(x)->isOne();
}

int mclBnFp2_mapToG2(mclBnG2 *y, const mclBnFp2 *x)
{
	bool b;
	mapToG2(&b, *cast(y), *cast(x));
	return b ? 0 : -1;
}

int mclBnG1_getBasePoint(mclBnG1 *x)
{
	*cast(x) = mcl::bn::getG1basePoint();
	return 0;
}

