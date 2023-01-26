#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iosfwd>
#include <stdint.h>
#include <memory.h>
#include "mcl/impl/bn_c_impl.hpp"
#define MCLSHE_DLL_EXPORT

#include <mcl/she.h>
#include <mcl/she.hpp>

using namespace mcl::she;
using namespace mcl::bn;

static SecretKey *cast(sheSecretKey *p) { return reinterpret_cast<SecretKey*>(p); }
static const SecretKey *cast(const sheSecretKey *p) { return reinterpret_cast<const SecretKey*>(p); }

static PublicKey *cast(shePublicKey *p) { return reinterpret_cast<PublicKey*>(p); }
static const PublicKey *cast(const shePublicKey *p) { return reinterpret_cast<const PublicKey*>(p); }

static PrecomputedPublicKey *cast(shePrecomputedPublicKey *p) { return reinterpret_cast<PrecomputedPublicKey*>(p); }
static const PrecomputedPublicKey *cast(const shePrecomputedPublicKey *p) { return reinterpret_cast<const PrecomputedPublicKey*>(p); }

static CipherTextG1 *cast(sheCipherTextG1 *p) { return reinterpret_cast<CipherTextG1*>(p); }
static const CipherTextG1 *cast(const sheCipherTextG1 *p) { return reinterpret_cast<const CipherTextG1*>(p); }

static CipherTextG2 *cast(sheCipherTextG2 *p) { return reinterpret_cast<CipherTextG2*>(p); }
static const CipherTextG2 *cast(const sheCipherTextG2 *p) { return reinterpret_cast<const CipherTextG2*>(p); }

static CipherTextGT *cast(sheCipherTextGT *p) { return reinterpret_cast<CipherTextGT*>(p); }
static const CipherTextGT *cast(const sheCipherTextGT *p) { return reinterpret_cast<const CipherTextGT*>(p); }

static ZkpBin *cast(sheZkpBin *p) { return reinterpret_cast<ZkpBin*>(p); }
static const ZkpBin *cast(const sheZkpBin *p) { return reinterpret_cast<const ZkpBin*>(p); }

static ZkpEq *cast(sheZkpEq *p) { return reinterpret_cast<ZkpEq*>(p); }
static const ZkpEq *cast(const sheZkpEq *p) { return reinterpret_cast<const ZkpEq*>(p); }

static ZkpBinEq *cast(sheZkpBinEq *p) { return reinterpret_cast<ZkpBinEq*>(p); }
static const ZkpBinEq *cast(const sheZkpBinEq *p) { return reinterpret_cast<const ZkpBinEq*>(p); }

static ZkpDec *cast(sheZkpDec *p) { return reinterpret_cast<ZkpDec*>(p); }
static const ZkpDec *cast(const sheZkpDec *p) { return reinterpret_cast<const ZkpDec*>(p); }

static AuxiliaryForZkpDecGT *cast(sheAuxiliaryForZkpDecGT *p) { return reinterpret_cast<AuxiliaryForZkpDecGT*>(p); }
static const AuxiliaryForZkpDecGT *cast(const sheAuxiliaryForZkpDecGT *p) { return reinterpret_cast<const AuxiliaryForZkpDecGT*>(p); }

static ZkpDecGT *cast(sheZkpDecGT *p) { return reinterpret_cast<ZkpDecGT*>(p); }
static const ZkpDecGT *cast(const sheZkpDecGT *p) { return reinterpret_cast<const ZkpDecGT*>(p); }

static mcl::bn::Fr *cast2(mclBnFr *p) { return reinterpret_cast<mcl::bn::Fr*>(p); }
static const mcl::bn::Fr *cast2(const mclBnFr *p) { return reinterpret_cast<const mcl::bn::Fr*>(p); }

int sheInit(int curve, int compiledTimeVar)
	try
{
	if (compiledTimeVar != MCLBN_COMPILED_TIME_VAR) {
		return -2;
	}
	const mcl::CurveParam *cp = mcl::getCurveParam(curve);
	if (cp == 0) return -3;
	SHE::init(*cp);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheInitG1only(int curve, int compiledTimeVar)
	try
{
	if (compiledTimeVar != MCLBN_COMPILED_TIME_VAR) {
		return -2;
	}
	SHE::initG1only(curve);
	return 0;
} catch (std::exception&) {
	return -1;
}

mclSize sheSecretKeySerialize(void *buf, mclSize maxBufSize, const sheSecretKey *sec)
{
	return (mclSize)cast(sec)->serialize(buf, maxBufSize);
}

mclSize shePublicKeySerialize(void *buf, mclSize maxBufSize, const shePublicKey *pub)
{
	return (mclSize)cast(pub)->serialize(buf, maxBufSize);
}

mclSize sheCipherTextG1Serialize(void *buf, mclSize maxBufSize, const sheCipherTextG1 *c)
{
	return (mclSize)cast(c)->serialize(buf, maxBufSize);
}

mclSize sheCipherTextG2Serialize(void *buf, mclSize maxBufSize, const sheCipherTextG2 *c)
{
	return (mclSize)cast(c)->serialize(buf, maxBufSize);
}

mclSize sheCipherTextGTSerialize(void *buf, mclSize maxBufSize, const sheCipherTextGT *c)
{
	return (mclSize)cast(c)->serialize(buf, maxBufSize);
}

mclSize sheZkpBinSerialize(void *buf, mclSize maxBufSize, const sheZkpBin *zkp)
{
	return (mclSize)cast(zkp)->serialize(buf, maxBufSize);
}

mclSize sheZkpEqSerialize(void *buf, mclSize maxBufSize, const sheZkpEq *zkp)
{
	return (mclSize)cast(zkp)->serialize(buf, maxBufSize);
}

mclSize sheZkpBinEqSerialize(void *buf, mclSize maxBufSize, const sheZkpBinEq *zkp)
{
	return (mclSize)cast(zkp)->serialize(buf, maxBufSize);
}

mclSize sheZkpDecSerialize(void *buf, mclSize maxBufSize, const sheZkpDec *zkp)
{
	return (mclSize)cast(zkp)->serialize(buf, maxBufSize);
}

mclSize sheZkpGTDecSerialize(void *buf, mclSize maxBufSize, const sheZkpDecGT *zkp)
{
	return (mclSize)cast(zkp)->serialize(buf, maxBufSize);
}

mclSize sheSecretKeyDeserialize(sheSecretKey* sec, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(sec)->deserialize(buf, bufSize);
}

mclSize shePublicKeyDeserialize(shePublicKey* pub, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(pub)->deserialize(buf, bufSize);
}

mclSize sheCipherTextG1Deserialize(sheCipherTextG1* c, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(c)->deserialize(buf, bufSize);
}

mclSize sheCipherTextG2Deserialize(sheCipherTextG2* c, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(c)->deserialize(buf, bufSize);
}

mclSize sheCipherTextGTDeserialize(sheCipherTextGT* c, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(c)->deserialize(buf, bufSize);
}

mclSize sheZkpBinDeserialize(sheZkpBin* zkp, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(zkp)->deserialize(buf, bufSize);
}

mclSize sheZkpEqDeserialize(sheZkpEq* zkp, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(zkp)->deserialize(buf, bufSize);
}

mclSize sheZkpBinEqDeserialize(sheZkpBinEq* zkp, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(zkp)->deserialize(buf, bufSize);
}

mclSize sheZkpDecDeserialize(sheZkpDec* zkp, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(zkp)->deserialize(buf, bufSize);
}

mclSize sheZkpDecGTDeserialize(sheZkpDecGT* zkp, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(zkp)->deserialize(buf, bufSize);
}

int sheSecretKeySetByCSPRNG(sheSecretKey *sec)
{
	cast(sec)->setByCSPRNG();
	return 0;
}

void sheGetPublicKey(shePublicKey *pub, const sheSecretKey *sec)
{
	cast(sec)->getPublicKey(*cast(pub));
}

void sheGetAuxiliaryForZkpDecGT(sheAuxiliaryForZkpDecGT *aux, const shePublicKey *pub)
{
	cast(pub)->getAuxiliaryForZkpDecGT(*cast(aux));
}

static int wrapSetRangeForDLP(void f(size_t), mclSize hashSize)
	try
{
	f(hashSize);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheSetRangeForDLP(mclSize hashSize)
{
	return wrapSetRangeForDLP(SHE::setRangeForDLP, hashSize);
}
int sheSetRangeForG1DLP(mclSize hashSize)
{
	return wrapSetRangeForDLP(SHE::setRangeForG1DLP, hashSize);
}
int sheSetRangeForG2DLP(mclSize hashSize)
{
	return wrapSetRangeForDLP(SHE::setRangeForG2DLP, hashSize);
}
int sheSetRangeForGTDLP(mclSize hashSize)
{
	return wrapSetRangeForDLP(SHE::setRangeForGTDLP, hashSize);
}

void sheSetTryNum(mclSize tryNum)
{
	SHE::setTryNum(tryNum);
}
void sheUseDecG1ViaGT(int use)
{
	SHE::useDecG1ViaGT(use != 0);
}
void sheUseDecG2ViaGT(int use)
{
	SHE::useDecG2ViaGT(use != 0);
}

template<class HashTable>
mclSize loadTable(HashTable& table, const void *buf, mclSize bufSize)
	try
{
	return table.load(buf, bufSize);
} catch (std::exception&) {
	return 0;
}

mclSize sheLoadTableForG1DLP(const void *buf, mclSize bufSize)
{
	return loadTable(getHashTableG1(), buf, bufSize);
}
mclSize sheLoadTableForG2DLP(const void *buf, mclSize bufSize)
{
	return loadTable(getHashTableG2(), buf, bufSize);
}
mclSize sheLoadTableForGTDLP(const void *buf, mclSize bufSize)
{
	return loadTable(getHashTableGT(), buf, bufSize);
}

template<class HashTable>
mclSize saveTable(void *buf, mclSize maxBufSize, const HashTable& table)
	try
{
	return table.save(buf, maxBufSize);
} catch (std::exception&) {
	return 0;
}
mclSize sheSaveTableForG1DLP(void *buf, mclSize maxBufSize)
{
	return saveTable(buf, maxBufSize, SHE::PhashTbl_);
}
mclSize sheSaveTableForG2DLP(void *buf, mclSize maxBufSize)
{
	return saveTable(buf, maxBufSize, SHE::QhashTbl_);
}
mclSize sheSaveTableForGTDLP(void *buf, mclSize maxBufSize)
{
	return saveTable(buf, maxBufSize, SHE::ePQhashTbl_);
}

mclSize sheGetTableSizeForG1DLP() { return SHE::PhashTbl_.getTableSize(); }
mclSize sheGetTableSizeForG2DLP() { return SHE::QhashTbl_.getTableSize(); }
mclSize sheGetTableSizeForGTDLP() { return SHE::ePQhashTbl_.getTableSize(); }

template<class CT>
int encT(CT *c, const shePublicKey *pub, mclInt m)
	try
{
	cast(pub)->enc(*cast(c), m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheEncG1(sheCipherTextG1 *c, const shePublicKey *pub, mclInt m)
{
	return encT(c, pub, m);
}

int sheEncG2(sheCipherTextG2 *c, const shePublicKey *pub, mclInt m)
{
	return encT(c, pub, m);
}

int sheEncGT(sheCipherTextGT *c, const shePublicKey *pub, mclInt m)
{
	return encT(c, pub, m);
}

bool setArray(mpz_class& m, const void *buf, mclSize bufSize)
{
	if (bufSize > Fr::getUnitSize() * sizeof(mcl::fp::Unit)) return false;
	bool b;
	mcl::gmp::setArray(&b, m, (const uint8_t*)buf, bufSize);
	return b;
}

template<class CT>
int encIntVecT(CT *c, const shePublicKey *pub, const void *buf, mclSize bufSize)
	try
{
	mpz_class m;
	if (!setArray(m, buf, bufSize)) return -1;
	cast(pub)->enc(*cast(c), m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheEncIntVecG1(sheCipherTextG1 *c, const shePublicKey *pub, const void *buf, mclSize bufSize)
{
	return encIntVecT(c, pub, buf, bufSize);
}

int sheEncIntVecG2(sheCipherTextG2 *c, const shePublicKey *pub, const void *buf, mclSize bufSize)
{
	return encIntVecT(c, pub, buf, bufSize);
}

int sheEncIntVecGT(sheCipherTextGT *c, const shePublicKey *pub, const void *buf, mclSize bufSize)
{
	return encIntVecT(c, pub, buf, bufSize);
}

template<class CT, class PK>
int encWithZkpBinT(CT *c, sheZkpBin *zkp, const PK *pub, int m)
	try
{
	cast(pub)->encWithZkpBin(*cast(c), *cast(zkp), m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheEncWithZkpBinG1(sheCipherTextG1 *c, sheZkpBin *zkp, const shePublicKey *pub, int m)
{
	return encWithZkpBinT(c, zkp, pub, m);
}

int sheEncWithZkpBinG2(sheCipherTextG2 *c, sheZkpBin *zkp, const shePublicKey *pub, int m)
{
	return encWithZkpBinT(c, zkp, pub, m);
}

int shePrecomputedPublicKeyEncWithZkpBinG1(sheCipherTextG1 *c, sheZkpBin *zkp, const shePrecomputedPublicKey *pub, int m)
{
	return encWithZkpBinT(c, zkp, pub, m);
}

int shePrecomputedPublicKeyEncWithZkpBinG2(sheCipherTextG2 *c, sheZkpBin *zkp, const shePrecomputedPublicKey *pub, int m)
{
	return encWithZkpBinT(c, zkp, pub, m);
}

template<class CT, class PK>
int encWithZkpSetT(CT *c, mclBnFr *zkp, const PK *pub, int m, const int *mVec, mclSize mSize)
	try
{
	cast(pub)->encWithZkpSet(*cast(c), cast2(zkp), m, mVec, mSize);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheEncWithZkpSetG1(sheCipherTextG1 *c, mclBnFr *zkp, const shePublicKey *pub, int m, const int *mVec, mclSize mSize)
{
	return encWithZkpSetT(c, zkp, pub, m, mVec, mSize);
}

int shePrecomputedPublicKeyEncWithZkpSetG1(sheCipherTextG1 *c, mclBnFr *zkp, const shePrecomputedPublicKey *ppub, int m, const int *mVec, mclSize mSize)
{
	return encWithZkpSetT(c, zkp, ppub, m, mVec, mSize);
}

template<class PK, class CT>
int verifyT(const PK& pub, const CT& c, const Fr *zkp, const int *mVec, mclSize mSize)
	try
{
	return pub.verify(c, zkp, mVec, mSize);
} catch (std::exception&) {
	return 0;
}

int sheVerifyZkpSetG1(const shePublicKey *pub, const sheCipherTextG1 *c, const mclBnFr *zkp, const int *mVec, mclSize mSize)
{
	return verifyT(*cast(pub), *cast(c), cast2(zkp), mVec, mSize);
}

int shePrecomputedPublicKeyVerifyZkpSetG1(const shePrecomputedPublicKey *ppub, const sheCipherTextG1 *c, const mclBnFr *zkp, const int *mVec, mclSize mSize)
{
	return verifyT(*cast(ppub), *cast(c), cast2(zkp), mVec, mSize);
}

template<class PK>
int encWithZkpEqT(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpEq *zkp, const PK *pub, mclInt m)
	try
{
	cast(pub)->encWithZkpEq(*cast(c1), *cast(c2), *cast(zkp), m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheEncWithZkpEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpEq *zkp, const shePublicKey *pub, mclInt m)
{
	return encWithZkpEqT(c1, c2, zkp, pub, m);
}

int shePrecomputedPublicKeyEncWithZkpEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpEq *zkp, const shePrecomputedPublicKey *ppub, mclInt m)
{
	return encWithZkpEqT(c1, c2, zkp, ppub, m);
}

template<class PK>
int encWithZkpBinEqT(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpBinEq *zkp, const PK *pub, int m)
	try
{
	cast(pub)->encWithZkpBinEq(*cast(c1), *cast(c2), *cast(zkp), m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheEncWithZkpBinEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpBinEq *zkp, const shePublicKey *pub, int m)
{
	return encWithZkpBinEqT(c1, c2, zkp, pub, m);
}

int shePrecomputedPublicKeyEncWithZkpBinEq(sheCipherTextG1 *c1, sheCipherTextG2 *c2, sheZkpBinEq *zkp, const shePrecomputedPublicKey *ppub, int m)
{
	return encWithZkpBinEqT(c1, c2, zkp, ppub, m);
}

template<class CT>
int decT(mclInt *m, const sheSecretKey *sec, const CT *c)
	try
{
	*m = (cast(sec)->dec)(*cast(c));
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheDecG1(mclInt *m, const sheSecretKey *sec, const sheCipherTextG1 *c)
{
	return decT(m, sec, c);
}

int sheDecG2(mclInt *m, const sheSecretKey *sec, const sheCipherTextG2 *c)
{
	return decT(m, sec, c);
}

int sheDecGT(mclInt *m, const sheSecretKey *sec, const sheCipherTextGT *c)
{
	return decT(m, sec, c);
}

template<class CT>
int decViaGTT(mclInt *m, const sheSecretKey *sec, const CT *c)
	try
{
	*m = (cast(sec)->decViaGT)(*cast(c));
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheDecG1ViaGT(mclInt *m, const sheSecretKey *sec, const sheCipherTextG1 *c)
{
	return decViaGTT(m, sec, c);
}

int sheDecG2ViaGT(mclInt *m, const sheSecretKey *sec, const sheCipherTextG2 *c)
{
	return decViaGTT(m, sec, c);
}


template<class CT>
int isZeroT(const sheSecretKey *sec, const CT *c)
	try
{
	return cast(sec)->isZero(*cast(c));
} catch (std::exception&) {
	return 0;
}

int sheIsZeroG1(const sheSecretKey *sec, const sheCipherTextG1 *c)
{
	return isZeroT(sec, c);
}
int sheIsZeroG2(const sheSecretKey *sec, const sheCipherTextG2 *c)
{
	return isZeroT(sec, c);
}
int sheIsZeroGT(const sheSecretKey *sec, const sheCipherTextGT *c)
{
	return isZeroT(sec, c);
}

template<class CT>
int negT(CT& y, const CT& x)
	try
{
	CT::neg(y, x);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheNegG1(sheCipherTextG1 *y, const sheCipherTextG1 *x)
{
	return negT(*cast(y), *cast(x));
}

int sheNegG2(sheCipherTextG2 *y, const sheCipherTextG2 *x)
{
	return negT(*cast(y), *cast(x));
}

int sheNegGT(sheCipherTextGT *y, const sheCipherTextGT *x)
{
	return negT(*cast(y), *cast(x));
}

template<class CT>
int addT(CT& z, const CT& x, const CT& y)
	try
{
	CT::add(z, x, y);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheAddG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, const sheCipherTextG1 *y)
{
	return addT(*cast(z), *cast(x), *cast(y));
}

int sheAddG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, const sheCipherTextG2 *y)
{
	return addT(*cast(z), *cast(x), *cast(y));
}

int sheAddGT(sheCipherTextGT *z, const sheCipherTextGT *x, const sheCipherTextGT *y)
{
	return addT(*cast(z), *cast(x), *cast(y));
}

template<class CT>
int subT(CT& z, const CT& x, const CT& y)
	try
{
	CT::sub(z, x, y);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheSubG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, const sheCipherTextG1 *y)
{
	return subT(*cast(z), *cast(x), *cast(y));
}

int sheSubG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, const sheCipherTextG2 *y)
{
	return subT(*cast(z), *cast(x), *cast(y));
}

int sheSubGT(sheCipherTextGT *z, const sheCipherTextGT *x, const sheCipherTextGT *y)
{
	return subT(*cast(z), *cast(x), *cast(y));
}

template<class CT1, class CT2, class CT3>
int mulT(CT1& z, const CT2& x, const CT3& y)
	try
{
	CT1::mul(z, x, y);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheMulG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, mclInt y)
{
	return mulT(*cast(z), *cast(x), y);
}

int sheMulG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, mclInt y)
{
	return mulT(*cast(z), *cast(x), y);
}

int sheMulGT(sheCipherTextGT *z, const sheCipherTextGT *x, mclInt y)
{
	return mulT(*cast(z), *cast(x), y);
}

template<class CT>
int mulIntVecT(CT& z, const CT& x, const void *buf, mclSize bufSize)
	try
{
	mpz_class m;
	if (!setArray(m, buf, bufSize)) return -1;
	CT::mul(z, x, m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheMulIntVecG1(sheCipherTextG1 *z, const sheCipherTextG1 *x, const void *buf, mclSize bufSize)
{
	return mulIntVecT(*cast(z), *cast(x), buf, bufSize);
}

int sheMulIntVecG2(sheCipherTextG2 *z, const sheCipherTextG2 *x, const void *buf, mclSize bufSize)
{
	return mulIntVecT(*cast(z), *cast(x), buf, bufSize);
}

int sheMulIntVecGT(sheCipherTextGT *z, const sheCipherTextGT *x, const void *buf, mclSize bufSize)
{
	return mulIntVecT(*cast(z), *cast(x), buf, bufSize);
}

int sheMul(sheCipherTextGT *z, const sheCipherTextG1 *x, const sheCipherTextG2 *y)
{
	return mulT(*cast(z), *cast(x), *cast(y));
}

int sheMulML(sheCipherTextGT *z, const sheCipherTextG1 *x, const sheCipherTextG2 *y)
	try
{
	CipherTextGT::mulML(*cast(z), *cast(x), *cast(y));
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheFinalExpGT(sheCipherTextGT *y, const sheCipherTextGT *x)
	try
{
	CipherTextGT::finalExp(*cast(y), *cast(x));
	return 0;
} catch (std::exception&) {
	return -1;
}

template<class CT>
int reRandT(CT& c, const shePublicKey *pub)
	try
{
	cast(pub)->reRand(c);
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheReRandG1(sheCipherTextG1 *c, const shePublicKey *pub)
{
	return reRandT(*cast(c), pub);
}

int sheReRandG2(sheCipherTextG2 *c, const shePublicKey *pub)
{
	return reRandT(*cast(c), pub);
}

int sheReRandGT(sheCipherTextGT *c, const shePublicKey *pub)
{
	return reRandT(*cast(c), pub);
}

template<class CT>
int convert(sheCipherTextGT *y, const shePublicKey *pub, const CT *x)
	try
{
	cast(pub)->convert(*cast(y), *cast(x));
	return 0;
} catch (std::exception&) {
	return -1;
}

int sheConvertG1(sheCipherTextGT *y, const shePublicKey *pub, const sheCipherTextG1 *x)
{
	return convert(y, pub, x);
}

int sheConvertG2(sheCipherTextGT *y, const shePublicKey *pub, const sheCipherTextG2 *x)
{
	return convert(y, pub, x);
}

shePrecomputedPublicKey *shePrecomputedPublicKeyCreate()
	try
{
	return reinterpret_cast<shePrecomputedPublicKey*>(new PrecomputedPublicKey());
} catch (...) {
	return 0;
}

void shePrecomputedPublicKeyDestroy(shePrecomputedPublicKey *ppub)
{
	delete cast(ppub);
}

int shePrecomputedPublicKeyInit(shePrecomputedPublicKey *ppub, const shePublicKey *pub)
	try
{
	cast(ppub)->init(*cast(pub));
	return 0;
} catch (...) {
	return 1;
}

template<class CT>
int pEncT(CT *c, const shePrecomputedPublicKey *pub, mclInt m)
	try
{
	cast(pub)->enc(*cast(c), m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int shePrecomputedPublicKeyEncG1(sheCipherTextG1 *c, const shePrecomputedPublicKey *pub, mclInt m)
{
	return pEncT(c, pub, m);
}

int shePrecomputedPublicKeyEncG2(sheCipherTextG2 *c, const shePrecomputedPublicKey *pub, mclInt m)
{
	return pEncT(c, pub, m);
}

int shePrecomputedPublicKeyEncGT(sheCipherTextGT *c, const shePrecomputedPublicKey *pub, mclInt m)
{
	return pEncT(c, pub, m);
}

template<class CT>
int pEncIntVecT(CT *c, const shePrecomputedPublicKey *pub, const void *buf, mclSize bufSize)
	try
{
	mpz_class m;
	if (!setArray(m, buf, bufSize)) return -1;
	cast(pub)->enc(*cast(c), m);
	return 0;
} catch (std::exception&) {
	return -1;
}

int shePrecomputedPublicKeyEncIntVecG1(sheCipherTextG1 *c, const shePrecomputedPublicKey *pub, const void *buf, mclSize bufSize)
{
	return pEncIntVecT(c, pub, buf, bufSize);
}

int shePrecomputedPublicKeyEncIntVecG2(sheCipherTextG2 *c, const shePrecomputedPublicKey *pub, const void *buf, mclSize bufSize)
{
	return pEncIntVecT(c, pub, buf, bufSize);
}

int shePrecomputedPublicKeyEncIntVecGT(sheCipherTextGT *c, const shePrecomputedPublicKey *pub, const void *buf, mclSize bufSize)
{
	return pEncIntVecT(c, pub, buf, bufSize);
}

template<class PK, class CT>
int verifyT(const PK& pub, const CT& c, const ZkpBin& zkp)
	try
{
	return pub.verify(c, zkp);
} catch (std::exception&) {
	return 0;
}

int sheVerifyZkpBinG1(const shePublicKey *pub, const sheCipherTextG1 *c, const sheZkpBin *zkp)
{
	return verifyT(*cast(pub), *cast(c), *cast(zkp));
}
int sheVerifyZkpBinG2(const shePublicKey *pub, const sheCipherTextG2 *c, const sheZkpBin *zkp)
{
	return verifyT(*cast(pub), *cast(c), *cast(zkp));
}
int shePrecomputedPublicKeyVerifyZkpBinG1(const shePrecomputedPublicKey *pub, const sheCipherTextG1 *c, const sheZkpBin *zkp)
{
	return verifyT(*cast(pub), *cast(c), *cast(zkp));
}
int shePrecomputedPublicKeyVerifyZkpBinG2(const shePrecomputedPublicKey *pub, const sheCipherTextG2 *c, const sheZkpBin *zkp)
{
	return verifyT(*cast(pub), *cast(c), *cast(zkp));
}

template<class PK, class Zkp>
int verifyT(const PK& pub, const CipherTextG1& c1, const CipherTextG2& c2, const Zkp& zkp)
	try
{
	return pub.verify(c1, c2, zkp);
} catch (std::exception&) {
	return 0;
}

int sheVerifyZkpEq(const shePublicKey *pub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpEq *zkp)
{
	return verifyT(*cast(pub), *cast(c1), *cast(c2), *cast(zkp));
}
int sheVerifyZkpBinEq(const shePublicKey *pub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpBinEq *zkp)
{
	return verifyT(*cast(pub), *cast(c1), *cast(c2), *cast(zkp));
}
int shePrecomputedPublicKeyVerifyZkpEq(const shePrecomputedPublicKey *ppub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpEq *zkp)
{
	return verifyT(*cast(ppub), *cast(c1), *cast(c2), *cast(zkp));
}
int shePrecomputedPublicKeyVerifyZkpBinEq(const shePrecomputedPublicKey *ppub, const sheCipherTextG1 *c1, const sheCipherTextG2 *c2, const sheZkpBinEq *zkp)
{
	return verifyT(*cast(ppub), *cast(c1), *cast(c2), *cast(zkp));
}

int sheDecWithZkpDecG1(mclInt *m, sheZkpDec *zkp, const sheSecretKey *sec, const sheCipherTextG1 *c, const shePublicKey *pub)
{
	bool b;
	*m = cast(sec)->decWithZkpDec(&b, *cast(zkp), *cast(c), *cast(pub));
	return b ? 0 : -1;
}

int sheDecWithZkpDecGT(mclInt *m, sheZkpDecGT *zkp, const sheSecretKey *sec, const sheCipherTextGT *c, const sheAuxiliaryForZkpDecGT *aux)
{
	bool b;
	*m = cast(sec)->decWithZkpDec(&b, *cast(zkp), *cast(c), *cast(aux));
	return b ? 0 : -1;
}

int sheVerifyZkpDecG1(const shePublicKey *pub, const sheCipherTextG1 *c1, mclInt m, const sheZkpDec *zkp)
{
	return cast(pub)->verify(*cast(c1), m, *cast(zkp));
}

int sheVerifyZkpDecGT(const sheAuxiliaryForZkpDecGT *aux, const sheCipherTextGT *ct, mclInt m, const sheZkpDecGT *zkp)
{
	return cast(aux)->verify(*cast(ct), m, *cast(zkp));
}

int sheSecretKeyIsEqual(const sheSecretKey *x, const sheSecretKey *y)
{
	return *cast(x) == *cast(y) ? 1 : 0;
}

int shePublicKeyIsEqual(const shePublicKey *x, const shePublicKey *y)
{
	return *cast(x) == *cast(y) ? 1 : 0;
}

int sheCipherTextG1IsEqual(const sheCipherTextG1 *x, const sheCipherTextG1 *y)
{
	return *cast(x) == *cast(y) ? 1 : 0;
}

int sheCipherTextG2IsEqual(const sheCipherTextG2 *x, const sheCipherTextG2 *y)
{
	return *cast(x) == *cast(y) ? 1 : 0;
}

int sheCipherTextGTIsEqual(const sheCipherTextGT *x, const sheCipherTextGT *y)
{
	return *cast(x) == *cast(y) ? 1 : 0;
}
