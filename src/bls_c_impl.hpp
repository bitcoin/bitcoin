#ifndef MCLBN_FORCE_EXPORT
	#define MCLBN_DONT_EXPORT
#endif
#define BLS_DLL_EXPORT

#include <bls/bls.h>

#include "mcl/impl/bn_c_impl.hpp"

#if (CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11) && !defined(__EMSCRIPTEN__) && !defined(__wasm__)
#include <thread>
#define BLS_MULTI_VERIFY_THREAD
#endif


inline void Gmul(G1& z, const G1& x, const Fr& y) { G1::mul(z, x, y); }
inline void Gmul(G2& z, const G2& x, const Fr& y) { G2::mul(z, x, y); }
inline void GmulCT(G1& z, const G1& x, const Fr& y) { G1::mulCT(z, x, y); }
inline void GmulCT(G2& z, const G2& x, const Fr& y) { G2::mulCT(z, x, y); }
inline void Gneg(G1& y, const G1& x) { G1::neg(y, x); }
inline void Gneg(G2& y, const G2& x) { G2::neg(y, x); }
inline void GmulVec(G1& z, const G1* x, const Fr *y, mclSize n) { G1::mulVec(z, x, y, n); }
inline void GmulVec(G2& z, const G2* x, const Fr *y, mclSize n) { G2::mulVec(z, x, y, n); }
inline void hashAndMapToG(G1& z, const void *m, mclSize size) { hashAndMapToG1(z, m, size); }
inline void hashAndMapToG(G2& z, const void *m, mclSize size) { hashAndMapToG2(z, m, size); }

/*
	BLS signature
	e : G1 x G2 -> GT
	Q in G2 ; fixed global parameter
	H : {str} -> G1
	s : secret key
	sQ ; public key
	s H(m) ; signature of m
	verify ; e(sQ, H(m)) = e(Q, s H(m))

	swap G1 and G2 if BLS_ETH is defined
	@note the current implementation does not support precomputed miller loop
*/
/*
	ETH2 spec requires the original G2cofactor
	original cofactor = mulByCofactorBLS12fast * adj
	adj H(m) ; original version
	H(m) ; fast version
	PublicKey = s P
	Signature = s (adj H(m)) ; slow
	          = (s adj) H(m) ; fast
	Verify original ; e(P, s adj H(m)) == e(s P, adj H(m))
	fast version    ; e((1/adj) P, s H(m)) == e(s P, H(m))
*/

static int g_curveType;
static bool g_irtfHashAndMap;
#ifdef BLS_ETH
typedef G2 G;
typedef G1 Gother;
static G1 g_P;
inline const G1& getBasePoint() { return g_P; }
#else
typedef G1 G;
typedef G2 Gother;
static G2 g_Q;
const size_t maxQcoeffN = 128;
static mcl::FixedArray<Fp6, maxQcoeffN> g_Qcoeff; // precomputed Q
inline const G2& getBasePoint() { return g_Q; }
inline const mcl::FixedArray<Fp6, maxQcoeffN>& getQcoeff() { return g_Qcoeff; }
#endif

int blsSetETHmode(int mode)
{
	if (g_curveType != MCL_BLS12_381) return -1;
	switch (mode) {
    case BLS_ETH_MODE_OLD:
        g_irtfHashAndMap = false;
        break;
	case BLS_ETH_MODE_DRAFT_07:
		g_irtfHashAndMap = true;
		break;
	default:
		return -1;
	}
	return 0;
}

int blsSetMapToMode(int mode)
{
	return mclBn_setMapToMode(mode);
}

int blsInit(int curve, int compiledTimeVar)
{
	if (compiledTimeVar != MCLBN_COMPILED_TIME_VAR) {
		return -(compiledTimeVar + (MCLBN_COMPILED_TIME_VAR * 1000));
	}
	const mcl::CurveParam* cp = mcl::getCurveParam(curve);
	if (cp == 0) return -1;
	bool b;
	initPairing(&b, *cp);
#ifdef __wasm__
//	G2::setMulArrayGLV(0);
#endif
	if (!b) return -1;
	g_curveType = curve;

#ifdef BLS_ETH
	if (curve == MCL_BLS12_381) {
		mclBn_setETHserialization(1);
		g_P.setStr(&b, "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569", 10);
		mclBn_setMapToMode(MCL_MAP_TO_MODE_HASH_TO_CURVE_07);
		blsSetETHmode(BLS_ETH_MODE_LATEST);
	} else
	{
		mapToG1(&b, g_P, 1);
	}
#else

	if (curve == MCL_BN254) {
		const char *Qx_BN254 = "11ccb44e77ac2c5dc32a6009594dbe331ec85a61290d6bbac8cc7ebb2dceb128 f204a14bbdac4a05be9a25176de827f2e60085668becdd4fc5fa914c9ee0d9a";
		const char *Qy_BN254 = "7c13d8487903ee3c1c5ea327a3a52b6cc74796b1760d5ba20ed802624ed19c8 8f9642bbaacb73d8c89492528f58932f2de9ac3e80c7b0e41f1a84f1c40182";
		g_Q.x.setStr(&b, Qx_BN254, 16);
		g_Q.y.setStr(&b, Qy_BN254, 16);
		g_Q.z = 1;
	} else {
		mapToG2(&b, g_Q, 1);
	}
	if (!b) return -100;
#if MCL_SIZEOF_UNIT == 8
	if (curve == MCL_BN254) {
		#include "./qcoeff-bn254.hpp"
		g_Qcoeff.resize(BN::param.precomputedQcoeffSize);
		assert(g_Qcoeff.size() == CYBOZU_NUM_OF_ARRAY(QcoeffTblBN254));
		for (size_t i = 0; i < g_Qcoeff.size(); i++) {
			Fp6& x6 = g_Qcoeff[i];
			for (size_t j = 0; j < 6; j++) {
				Fp& x = x6.getFp0()[j];
				mcl::fp::Unit *p = const_cast<mcl::fp::Unit*>(x.getUnit());
				for (size_t k = 0; k < 4; k++) {
					p[k] = QcoeffTblBN254[i][j][k];
				}
			}
		}
	} else
#endif
	{
		precomputeG2(&b, g_Qcoeff, getBasePoint());
	}
#endif
	if (!b) return -101;
	verifyOrderG1(true);
	verifyOrderG2(true);
	return 0;
}

void blsSetETHserialization(int ETHserialization)
{
	mclBn_setETHserialization(ETHserialization);
}

#ifdef __EMSCRIPTEN__
extern "C" BLS_DLL_API void *blsMalloc(size_t n)
{
	return malloc(n);
}
extern "C" BLS_DLL_API void blsFree(void *p)
{
	free(p);
}
#endif

static inline const mclBnG1 *cast(const G1* x) { return (const mclBnG1*)x; }
static inline const mclBnG2 *cast(const G2* x) { return (const mclBnG2*)x; }

void blsIdSetInt(blsId *id, int x)
{
	*cast(&id->v) = x;
}

int blsSecretKeySetLittleEndian(blsSecretKey *sec, const void *buf, mclSize bufSize)
{
	cast(&sec->v)->setArrayMask((const uint8_t *)buf, bufSize);
	return 0;
}
int blsSecretKeySetLittleEndianMod(blsSecretKey *sec, const void *buf, mclSize bufSize)
{
	bool b;
	cast(&sec->v)->setArrayMod(&b, (const uint8_t *)buf, bufSize);
	return b ? 0 : -1;
}

void blsGetPublicKey(blsPublicKey *pub, const blsSecretKey *sec)
{
	Gmul(*cast(&pub->v), getBasePoint(), *cast(&sec->v));
}

int blsHashToSignature(blsSignature *sig, const void *buf, mclSize bufSize)
{
	hashAndMapToG(*cast(&sig->v), buf, bufSize);
	return 0;
}

void blsSign(blsSignature *sig, const blsSecretKey *sec, const void *m, mclSize size)
{
	blsHashToSignature(sig, m, size);
	Fr s = *cast(&sec->v);
	GmulCT(*cast(&sig->v), *cast(&sig->v), s);
}

#ifdef BLS_ETH
/*
	e(P, sHm) == e(sP, Hm)
	<=> finalExp(ML(P, sHm) * e(-sP, Hm)) == 1
*/
bool isEqualTwoPairings(const G2& sHm, const G1& sP, const G2& Hm)
{
	GT e;
	G1 v1[2];
	G2 v2[2] = { sHm, Hm };
	v1[0] = getBasePoint();
	G1::neg(v1[1], sP);
	millerLoopVec(e, v1, v2, 2);
	finalExp(e, e);
	return e.isOne();
}
#else
/*
	e(P1, Q1) == e(P2, Q2)
	<=> finalExp(ML(P1, Q1)) == finalExp(ML(P2, Q2))
	<=> finalExp(ML(P1, Q1) / ML(P2, Q2)) == 1
	<=> finalExp(ML(P1, Q1) * ML(-P2, Q2)) == 1
	Q1 is precomputed
*/
bool isEqualTwoPairings(const G1& P1, const Fp6* Q1coeff, const G1& P2, const G2& Q2)
{
	GT e;
	precomputedMillerLoop2mixed(e, P2, Q2, -P1, Q1coeff);
	finalExp(e, e);
	return e.isOne();
}
#endif

int blsVerify(const blsSignature *sig, const blsPublicKey *pub, const void *m, mclSize size)
{
	if (cast(&pub->v)->isZero()) return 0;
	G Hm;
	hashAndMapToG(Hm, m, size);
#ifdef BLS_ETH
	return isEqualTwoPairings(*cast(&sig->v), *cast(&pub->v), Hm);
#else
	/*
		e(sHm, Q) = e(Hm, sQ)
		e(sig, Q) = e(Hm, pub)
	*/
	return isEqualTwoPairings(*cast(&sig->v), getQcoeff().data(), Hm, *cast(&pub->v));
#endif
}

void blsMultiVerifySub(mclBnGT *e, blsSignature *aggSig, const blsSignature *sigVec, const blsPublicKey *pubVec, const char *msg, mclSize msgSize, const char *randVec, mclSize randSize, mclSize n)
{
#ifdef BLS_ETH
	const size_t N = 16;
	Fr rand[N];
	G1 g1Vec[N];
	G2 g2Vec[N];
	bool initE = true;
	while (n > 0) {
		size_t m = mcl::fp::min_<size_t>(n, N);
		for (size_t i = 0; i < m; i++) {
			bool b;
			rand[i].setArray(&b, (const uint8_t *)&randVec[i * randSize], randSize);
			(void)b;
			const G1& pub = *cast(&pubVec[i].v);
			if (pub.isZero()) {
				cast(e)->clear();
				return;
			}
			G1::mul(g1Vec[i], pub, rand[i]);
			hashAndMapToG(g2Vec[i], &msg[i * msgSize], msgSize);
		}
		if (initE) {
			G2::mulVec(*cast(&aggSig->v), cast(&sigVec->v), rand, m);
		} else {
			G2 t;
			G2::mulVec(t, cast(&sigVec->v), rand, m);
			*cast(&aggSig->v) += t;
		}
		sigVec += m;
		pubVec += m;
		msg += m * msgSize;
		randVec += m * randSize;
		n -= m;
		millerLoopVec(*cast(e), g1Vec, g2Vec, m, initE);
		initE = false;
	}
#else
	(void)e;
	(void)aggSig;
	(void)sigVec;
	(void)pubVec;
	(void)msg;
	(void)msgSize;
	(void)randVec;
	(void)randSize;;
	(void)n;
#endif
}

int blsMultiVerifyFinal(const mclBnGT *e, const blsSignature *aggSig)
{
#ifdef BLS_ETH
	if (cast(e)->isZero()) return false;
	GT e2;
	millerLoop(e2, -getBasePoint(), *cast(&aggSig->v));
	e2 *= *cast(e);
	finalExp(e2, e2);
	return e2.isOne();
#else
	(void)e;
	(void)aggSig;
	return 0;
#endif
}
/*
	sig = sum_i sigVec[i] * randVec[i]
	pubVec[i] *= randVec[i]
	verify prod e(H(pubVec[i], msgToG2[i]) == e(P, sig)
	@remark return 0 if some pubVec[i] is zero
*/
int blsMultiVerify(const blsSignature *sigVec, const blsPublicKey *pubVec, const void *msgVec, mclSize msgSize, const void *randVec, mclSize randSize, mclSize n, int threadN)
{
#ifdef BLS_ETH
	if (n == 0) return 0;
	const char *msg = (const char*)msgVec;
	const char *rp = (const char*)randVec;
	GT e;
	G2 aggSig;
	const int maxThreadNum = 32;
	if (threadN < 1) threadN = 1;
	if (threadN > maxThreadNum) threadN = maxThreadNum;
#ifdef BLS_MULTI_VERIFY_THREAD
	const size_t minN = 16;
	if (threadN > 1 && n >= minN) {
		GT et[maxThreadNum];
		G2 aggSigt[maxThreadNum];
		std::thread th[maxThreadNum];
		size_t blockN = n / minN;
		assert(blockN > 0);
		size_t q = blockN / threadN;
		size_t r = blockN % threadN;

		for (int i = 0; i < threadN; i++) {
			size_t m = q;
			if (r > 0) {
				m++;
				r--;
			}
			if (m == 0) {
				threadN = i; // n is too small for threadN
				break;
			}
			m *= minN;
			if (i == threadN - 1) {
				m = n;
			}
			th[i] = std::thread(blsMultiVerifySub, (mclBnGT*)&et[i], (blsSignature*)&aggSigt[i], sigVec, pubVec, msg, msgSize, rp, randSize, m);
			sigVec += m;
			pubVec += m;
			msg += msgSize * m;
			rp += randSize * m;
			n -= m;
		}
		for (int i = 0; i < threadN; i++) {
			th[i].join();
		}
		e = et[0];
		aggSig = aggSigt[0];
		for (int i = 1; i < threadN; i++) {
			e *= et[i];
			aggSig += aggSigt[i];
		}
	} else
#endif
	{
		blsMultiVerifySub((mclBnGT*)&e, (blsSignature*)&aggSig, sigVec, pubVec, msg, msgSize, rp, randSize, n);
	}
	return blsMultiVerifyFinal((const mclBnGT*)&e, (const blsSignature*)&aggSig);
#else
	(void)sigVec;
	(void)pubVec;
	(void)msgVec;
	(void)msgSize;
	(void)randVec;
	(void)randSize;
	(void)n;
	(void)threadN;
	return 0;
#endif
}

void blsAggregateSignature(blsSignature *aggSig, const blsSignature *sigVec, mclSize n)
{
	if (n == 0) {
		memset(aggSig, 0, sizeof(*aggSig));
		return;
	}
	*aggSig = sigVec[0];
	for (mclSize i = 1; i < n; i++) {
		blsSignatureAdd(aggSig, &sigVec[i]);
	}
}

// return -1 if some pubVec[i] is zero else 0
int blsAggregatePublicKey(blsPublicKey *aggPub, const blsPublicKey *pubVec, mclSize n)
{
	if (n == 0) {
		memset(aggPub, 0, sizeof(*aggPub));
		return 0;
	}
	int ret = 0;
	*aggPub = pubVec[0];
	if (cast(&pubVec[0].v)->isZero()) ret = -1;
	for (mclSize i = 1; i < n; i++) {
		if (cast(&pubVec[i].v)->isZero()) ret = -1;
		blsPublicKeyAdd(aggPub, &pubVec[i]);
	}
	return ret;
}

int blsFastAggregateVerify(const blsSignature *sig, const blsPublicKey *pubVec, mclSize n, const void *msg, mclSize msgSize)
{
	if (n == 0) return 0;
	blsPublicKey aggPub;
	int ret = blsAggregatePublicKey(&aggPub, pubVec, n);
	if (ret < 0) return 0;
	return blsVerify(sig, &aggPub, msg, msgSize);
}

int blsAggregateVerifyNoCheck(const blsSignature *sig, const blsPublicKey *pubVec, const void *msgVec, mclSize msgSize, mclSize n)
{
#ifdef BLS_ETH
	if (n == 0) return 0;
#if 1 // 1.1 times faster
	GT e;
	const char *msg = (const char*)msgVec;
	const size_t N = 16;
	G1 g1Vec[N+1];
	G2 g2Vec[N+1];
	bool initE = true;

	while (n > 0) {
		size_t m = mcl::fp::min_<size_t>(n, N);
		for (size_t i = 0; i < m; i++) {
			g1Vec[i] = *cast(&pubVec[i].v);
			if (g1Vec[i].isZero()) return 0;
			hashAndMapToG(g2Vec[i], &msg[i * msgSize], msgSize);
		}
		pubVec += m;
		msg += m * msgSize;
		n -= m;
		if (n == 0) {
			g1Vec[m] = getBasePoint();
			G2::neg(g2Vec[m], *cast(&sig->v));
			m++;
		}
		millerLoopVec(e, g1Vec, g2Vec, m, initE);
		initE = false;
	}
	BN::finalExp(e, e);
	return e.isOne();
#else
	const char *p = (const char *)msgVec;
	GT s(1), t;
	for (mclSize i = 0; i < n; i++) {
		G2 Q;
		hashAndMapToG(Q, &p[msgSize * i], msgSize);
		if (cast(&pubVec[i].v)->isZero()) return 0;
		millerLoop(t, *cast(&pubVec[i].v), Q);
		s *= t;
	}
	millerLoop(t, -getBasePoint(), *cast(&sig->v));
	s *= t;
	finalExp(s, s);
	return s.isOne() ? 1 : 0;
#endif
#else
	(void)sig;
	(void)pubVec;
	(void)msgVec;
	(void)msgSize;
	(void)n;
	return 0;
#endif
}

mclSize blsIdSerialize(void *buf, mclSize maxBufSize, const blsId *id)
{
	return cast(&id->v)->serialize(buf, maxBufSize);
}

mclSize blsSecretKeySerialize(void *buf, mclSize maxBufSize, const blsSecretKey *sec)
{
	return cast(&sec->v)->serialize(buf, maxBufSize);
}

mclSize blsPublicKeySerialize(void *buf, mclSize maxBufSize, const blsPublicKey *pub)
{
	return cast(&pub->v)->serialize(buf, maxBufSize);
}

mclSize blsSignatureSerialize(void *buf, mclSize maxBufSize, const blsSignature *sig)
{
	return cast(&sig->v)->serialize(buf, maxBufSize);
}

mclSize blsIdDeserialize(blsId *id, const void *buf, mclSize bufSize)
{
	return cast(&id->v)->deserialize(buf, bufSize);
}

mclSize blsSecretKeyDeserialize(blsSecretKey *sec, const void *buf, mclSize bufSize)
{
	return cast(&sec->v)->deserialize(buf, bufSize);
}

mclSize blsPublicKeyDeserialize(blsPublicKey *pub, const void *buf, mclSize bufSize)
{
	return cast(&pub->v)->deserialize(buf, bufSize);
}

mclSize blsSignatureDeserialize(blsSignature *sig, const void *buf, mclSize bufSize)
{
	return cast(&sig->v)->deserialize(buf, bufSize);
}

int blsIdIsEqual(const blsId *lhs, const blsId *rhs)
{
	return *cast(&lhs->v) == *cast(&rhs->v);
}

int blsSecretKeyIsEqual(const blsSecretKey *lhs, const blsSecretKey *rhs)
{
	return *cast(&lhs->v) == *cast(&rhs->v);
}

int blsPublicKeyIsEqual(const blsPublicKey *lhs, const blsPublicKey *rhs)
{
	return *cast(&lhs->v) == *cast(&rhs->v);
}

int blsSignatureIsEqual(const blsSignature *lhs, const blsSignature *rhs)
{
	return *cast(&lhs->v) == *cast(&rhs->v);
}

int blsIdIsZero(const blsId *x)
{
	return cast(&x->v)->isZero() ? 1 : 0;
}

int blsSecretKeyIsZero(const blsSecretKey *x)
{
	return cast(&x->v)->isZero() ? 1 : 0;
}

int blsPublicKeyIsZero(const blsPublicKey *x)
{
	return cast(&x->v)->isZero() ? 1 : 0;
}

int blsSignatureIsZero(const blsSignature *x)
{
	return cast(&x->v)->isZero() ? 1 : 0;
}

int blsSecretKeyShare(blsSecretKey *sec, const blsSecretKey* msk, mclSize k, const blsId *id)
{
	bool b;
	mcl::evaluatePolynomial(&b, *cast(&sec->v), cast(&msk->v), k, *cast(&id->v));
	return b ? 0 : -1;
}

int blsPublicKeyShare(blsPublicKey *pub, const blsPublicKey *mpk, mclSize k, const blsId *id)
{
	bool b;
	mcl::evaluatePolynomial(&b, *cast(&pub->v), cast(&mpk->v), k, *cast(&id->v));
	return b ? 0 : -1;
}

int blsSecretKeyRecover(blsSecretKey *sec, const blsSecretKey *secVec, const blsId *idVec, mclSize n)
{
	bool b;
	mcl::LagrangeInterpolation(&b, *cast(&sec->v), cast(&idVec->v), cast(&secVec->v), n);
	return b ? 0 : -1;
}

int blsPublicKeyRecover(blsPublicKey *pub, const blsPublicKey *pubVec, const blsId *idVec, mclSize n)
{
	bool b;
	mcl::LagrangeInterpolation(&b, *cast(&pub->v), cast(&idVec->v), cast(&pubVec->v), n);
	return b ? 0 : -1;
}

int blsSignatureRecover(blsSignature *sig, const blsSignature *sigVec, const blsId *idVec, mclSize n)
{
	bool b;
	mcl::LagrangeInterpolation(&b, *cast(&sig->v), cast(&idVec->v), cast(&sigVec->v), n);
	return b ? 0 : -1;
}

void blsSecretKeyAdd(blsSecretKey *sec, const blsSecretKey *rhs)
{
	*cast(&sec->v) += *cast(&rhs->v);
}

void blsPublicKeyAdd(blsPublicKey *pub, const blsPublicKey *rhs)
{
	*cast(&pub->v) += *cast(&rhs->v);
}

void blsSignatureAdd(blsSignature *sig, const blsSignature *rhs)
{
	*cast(&sig->v) += *cast(&rhs->v);
}

void blsSignatureVerifyOrder(int doVerify)
{
#ifdef BLS_ETH
	verifyOrderG2(doVerify != 0);
#else
	verifyOrderG1(doVerify != 0);
#endif
}
void blsPublicKeyVerifyOrder(int doVerify)
{
#ifdef BLS_ETH
	verifyOrderG1(doVerify != 0);
#else
	verifyOrderG2(doVerify != 0);
#endif
}
int blsSignatureIsValidOrder(const blsSignature *sig)
{
	return cast(&sig->v)->isValidOrder();
}
int blsPublicKeyIsValidOrder(const blsPublicKey *pub)
{
	return cast(&pub->v)->isValidOrder();
}

#ifndef BLS_MINIMUM_API
template<class G>
inline bool toG(G& Hm, const void *h, mclSize size)
{
	if (g_irtfHashAndMap) {
		hashAndMapToG(Hm, h, size);
		return true;
	}
	// backward compatibility
	Fp t;
	t.setArrayMask((const uint8_t *)h, size);
	bool b;
#ifdef BLS_ETH
	BN::mapToG2(&b, Hm, Fp2(t, 0));
#else
	BN::mapToG1(&b, Hm, t);
#endif
	return b;
}

int blsVerifyAggregatedHashes(const blsSignature *aggSig, const blsPublicKey *pubVec, const void *hVec, size_t sizeofHash, mclSize n)
{
	if (n == 0) return 0;
	GT e;
	const char *ph = (const char*)hVec;
	const size_t N = 16;
	G1 g1Vec[N+1]; // +1 is for the last appending element
	G2 g2Vec[N+1];
#ifdef BLS_ETH
	bool initE = true;
	while (n > 0) {
		size_t m = mcl::fp::min_<size_t>(n, N);
		for (size_t i = 0; i < m; i++) {
			g1Vec[i] = *cast(&pubVec[i].v);
			if (g1Vec[i].isZero()) return 0;
			if (!toG(g2Vec[i], &ph[i * sizeofHash], sizeofHash)) return 0;
		}
		pubVec += m;
		ph += m * sizeofHash;
		n -= m;
		if (n == 0) {
			g1Vec[m] = getBasePoint();
			G2::neg(g2Vec[m], *cast(&aggSig->v));
			m++;
		}
		millerLoopVec(e, g1Vec, g2Vec, m, initE);
		initE = false;
	}
#else
	/*
		e(aggSig, Q) = prod_i e(hVec[i], pubVec[i])
		<=> finalExp(ML(-aggSig, Q) * prod_i ML(hVec[i], pubVec[i])) == 1
	*/
	BN::precomputedMillerLoop(e, -*cast(&aggSig->v), g_Qcoeff.data());
	while (n > 0) {
		size_t m = N;
		if (n < m) m = n;
		for (size_t i = 0; i < m; i++) {
			if (!toG(g1Vec[i], &ph[i * sizeofHash], sizeofHash)) return 0;
			g2Vec[i] = *cast(&pubVec[i].v);
			if (g2Vec[i].isZero()) return 0;
		}
		millerLoopVec(e, g1Vec, g2Vec, m, false);
		pubVec += m;
		ph += m * sizeofHash;
		n -= m;
	}
#endif
	BN::finalExp(e, e);
	return e.isOne();
}

int blsSignHash(blsSignature *sig, const blsSecretKey *sec, const void *h, mclSize size)
{
	G Hm;
	if (!toG(Hm, h, size)) return -1;
	Fr s = *cast(&sec->v);
	GmulCT(*cast(&sig->v), Hm, s);
	return 0;
}

#ifdef BLS_ETH
const uint8_t ZERO_HEADER = 1 << 6;
const mclSize serializedPublicKeySize = 48;
const mclSize serializedSignatureSize = 48 * 2;
inline bool isZeroFormat(const uint8_t *buf, mclSize n)
{
	if (buf[0] != ZERO_HEADER) return false;
	for (mclSize i = 1; i < n; i++) {
		if (buf[i] != 0) return false;
	}
	return true;
}
#endif

mclSize blsPublicKeySerializeUncompressed(void *buf, mclSize maxBufSize, const blsPublicKey *pub)
{
#ifdef BLS_ETH
	if (g_curveType != MCL_BLS12_381) return 0;
	const mclSize retSize = serializedPublicKeySize * 2;
	if (maxBufSize < retSize) return 0;
	uint8_t *dst = (uint8_t*)buf;
	if (cast(&pub->v)->isZero()) {
		dst[0] = ZERO_HEADER;
		memset(dst + 1, 0, retSize - 1);
	} else {
		G1 x;
		G1::normalize(x, *cast(&pub->v));
		if (x.x.serialize(dst, serializedPublicKeySize) == 0) return 0;
		if (x.y.serialize(dst + serializedPublicKeySize, serializedPublicKeySize) == 0) return 0;
	}
	return retSize;
#else
	(void)buf;
	(void)maxBufSize;
	(void)pub;
	return 0;
#endif
}

mclSize blsSignatureSerializeUncompressed(void *buf, mclSize maxBufSize, const blsSignature *sig)
{
#ifdef BLS_ETH
	if (g_curveType != MCL_BLS12_381) return 0;
	const mclSize retSize = serializedSignatureSize * 2;
	if (maxBufSize < retSize) return 0;
	uint8_t *dst = (uint8_t*)buf;
	if (cast(&sig->v)->isZero()) {
		dst[0] = ZERO_HEADER;
		memset(dst + 1, 0, retSize - 1);
	} else {
		G2 x;
		G2::normalize(x, *cast(&sig->v));
		if (x.x.serialize(dst, serializedSignatureSize) == 0) return 0;
		if (x.y.serialize(dst + serializedSignatureSize, serializedSignatureSize) == 0) return 0;
	}
	return retSize;
#else
	(void)buf;
	(void)maxBufSize;
	(void)sig;
	return 0;
#endif
}

mclSize blsPublicKeyDeserializeUncompressed(blsPublicKey *pub, const void *buf, mclSize bufSize)
{
#ifdef BLS_ETH
	if (g_curveType != MCL_BLS12_381) return 0;
	const mclSize retSize = serializedPublicKeySize * 2;
	if (bufSize < retSize) return 0;
	const uint8_t *src = (const uint8_t*)buf;
	G1& x = *cast(&pub->v);
	if (isZeroFormat(src, retSize)) {
		x.clear();
	} else {
		if (x.x.deserialize(src, serializedPublicKeySize) == 0) return 0;
		if (x.y.deserialize(src + serializedPublicKeySize, serializedPublicKeySize) == 0) return 0;
		x.z = 1;
	}
	if (!x.isValid()) return 0;
	return retSize;
#else
	(void)pub;
	(void)buf;
	(void)bufSize;
	return 0;
#endif
}

mclSize blsSignatureDeserializeUncompressed(blsSignature *sig, const void *buf, mclSize bufSize)
{
#ifdef BLS_ETH
	if (g_curveType != MCL_BLS12_381) return 0;
	const mclSize retSize = serializedSignatureSize * 2;
	if (bufSize < retSize) return 0;
	const uint8_t *src = (const uint8_t*)buf;
	G2& x = *cast(&sig->v);
	if (isZeroFormat(src, retSize)) {
		x.clear();
	} else {
		if (x.x.deserialize(src, serializedSignatureSize) == 0) return 0;
		if (x.y.deserialize(src + serializedSignatureSize, serializedSignatureSize) == 0) return 0;
		x.z = 1;
	}
	if (!x.isValid()) return 0;
	return retSize;
#else
	(void)sig;
	(void)buf;
	(void)bufSize;
	return 0;
#endif
}

int blsVerifyPairing(const blsSignature *X, const blsSignature *Y, const blsPublicKey *pub)
{
#ifdef BLS_ETH
	return isEqualTwoPairings(*cast(&X->v), *cast(&pub->v), *cast(&Y->v));
#else
	return isEqualTwoPairings(*cast(&X->v), getQcoeff().data(), *cast(&Y->v), *cast(&pub->v));
#endif
}

int blsVerifyHash(const blsSignature *sig, const blsPublicKey *pub, const void *h, mclSize size)
{
	blsSignature Hm;
	if (!toG(*cast(&Hm.v), h, size)) return 0;
	if (cast(&pub->v)->isZero()) return 0;
	return blsVerifyPairing(sig, &Hm, pub);
}

void blsSecretKeySub(blsSecretKey *sec, const blsSecretKey *rhs)
{
	*cast(&sec->v) -= *cast(&rhs->v);
}

void blsPublicKeySub(blsPublicKey *pub, const blsPublicKey *rhs)
{
	*cast(&pub->v) -= *cast(&rhs->v);
}

void blsSignatureSub(blsSignature *sig, const blsSignature *rhs)
{
	*cast(&sig->v) -= *cast(&rhs->v);
}

void blsSecretKeyNeg(blsSecretKey *x)
{
	Fr::neg(*cast(&x->v), *cast(&x->v));
}

void blsPublicKeyNeg(blsPublicKey *x)
{
	Gneg(*cast(&x->v), *cast(&x->v));
}

void blsSignatureNeg(blsSignature *x)
{
	Gneg(*cast(&x->v), *cast(&x->v));
}

void blsSecretKeyMul(blsSecretKey *y, const blsSecretKey *x)
{
	*cast(&y->v) *= *cast(&x->v);
}

void blsPublicKeyMul(blsPublicKey *y, const blsSecretKey *x)
{
	*cast(&y->v) *= *cast(&x->v);
}

void blsSignatureMul(blsSignature *y, const blsSecretKey *x)
{
	*cast(&y->v) *= *cast(&x->v);
}

void blsPublicKeyMulVec(blsPublicKey *z, const blsPublicKey *x, const blsSecretKey *y, mclSize n)
{
	GmulVec(*cast(&z->v), cast(&x->v), cast(&y->v), n);
}

void blsSignatureMulVec(blsSignature *z, const blsSignature *x, const blsSecretKey *y, mclSize n)
{
	GmulVec(*cast(&z->v), cast(&x->v), cast(&y->v), n);
}

mclSize blsGetOpUnitSize() // FpUint64Size
{
	return Fp::getUnitSize() * sizeof(mcl::fp::Unit) / sizeof(uint64_t);
}

int blsGetCurveOrder(char *buf, mclSize maxBufSize)
{
	return (int)Fr::getModulo(buf, maxBufSize);
}

int blsGetFieldOrder(char *buf, mclSize maxBufSize)
{
	return (int)Fp::getModulo(buf, maxBufSize);
}

int blsGetSerializedSecretKeyByteSize()
{
	return blsGetFrByteSize();
}

int blsGetSerializedPublicKeyByteSize()
{
#ifdef BLS_ETH
	return blsGetG1ByteSize();
#else
	return blsGetG1ByteSize() * 2;
#endif
}

int blsGetSerializedSignatureByteSize()
{
#ifdef BLS_ETH
	return blsGetG1ByteSize() * 2;
#else
	return blsGetG1ByteSize();
#endif
}

int blsGetG1ByteSize()
{
	return (int)Fp::getByteSize();
}

int blsGetFrByteSize()
{
	return (int)Fr::getByteSize();
}

void blsGetGeneratorOfPublicKey(blsPublicKey *pub)
{
	*cast(&pub->v) = getBasePoint();
}

int blsSetGeneratorOfPublicKey(const blsPublicKey *pub)
{
#ifdef BLS_ETH
	g_P = *cast(&pub->v);
	return 0;
#else
	g_Q = *cast(&pub->v);
	bool b;
	precomputeG2(&b, g_Qcoeff, getBasePoint());
	return b ? 0 : -1;
#endif
}

int blsIdSetDecStr(blsId *id, const char *buf, mclSize bufSize)
{
	return cast(&id->v)->deserialize(buf, bufSize, 10) > 0 ? 0 : -1;
}
int blsIdSetHexStr(blsId *id, const char *buf, mclSize bufSize)
{
	return cast(&id->v)->deserialize(buf, bufSize, 16) > 0 ? 0 : -1;
}

int blsIdSetLittleEndian(blsId *id, const void *buf, mclSize bufSize)
{
	cast(&id->v)->setArrayMask((const uint8_t *)buf, bufSize);
	return 0;
}

mclSize blsIdGetDecStr(char *buf, mclSize maxBufSize, const blsId *id)
{
	return cast(&id->v)->getStr(buf, maxBufSize, 10);
}

mclSize blsIdGetHexStr(char *buf, mclSize maxBufSize, const blsId *id)
{
	return cast(&id->v)->getStr(buf, maxBufSize, 16);
}

int blsHashToSecretKey(blsSecretKey *sec, const void *buf, mclSize bufSize)
{
	cast(&sec->v)->setHashOf(buf, bufSize);
	return 0;
}

#ifndef MCL_DONT_USE_CSPRNG
int blsSecretKeySetByCSPRNG(blsSecretKey *sec)
{
	bool b;
	cast(&sec->v)->setByCSPRNG(&b);
	return b ? 0 : -1;
}
void blsSetRandFunc(void *self, unsigned int (*readFunc)(void *self, void *buf, unsigned int bufSize))
{
	mcl::fp::RandGen::setRandFunc(self, readFunc);
}
#endif

void blsGetPop(blsSignature *sig, const blsSecretKey *sec)
{
	blsPublicKey pub;
	blsGetPublicKey(&pub, sec);
	char buf[1024];
	mclSize n = cast(&pub.v)->serialize(buf, sizeof(buf));
	assert(n);
	blsSign(sig, sec, buf, n);
}

int blsVerifyPop(const blsSignature *sig, const blsPublicKey *pub)
{
	char buf[1024];
	mclSize n = cast(&pub->v)->serialize(buf, sizeof(buf));
	if (n == 0) return 0;
	return blsVerify(sig, pub, buf, n);
}

mclSize blsIdGetLittleEndian(void *buf, mclSize maxBufSize, const blsId *id)
{
	return cast(&id->v)->serialize(buf, maxBufSize);
}
int blsSecretKeySetDecStr(blsSecretKey *sec, const char *buf, mclSize bufSize)
{
	return cast(&sec->v)->deserialize(buf, bufSize, 10) > 0 ? 0 : -1;
}
int blsSecretKeySetHexStr(blsSecretKey *sec, const char *buf, mclSize bufSize)
{
	return cast(&sec->v)->deserialize(buf, bufSize, 16) > 0 ? 0 : -1;
}
mclSize blsSecretKeyGetLittleEndian(void *buf, mclSize maxBufSize, const blsSecretKey *sec)
{
	return cast(&sec->v)->serialize(buf, maxBufSize);
}
mclSize blsSecretKeyGetDecStr(char *buf, mclSize maxBufSize, const blsSecretKey *sec)
{
	return cast(&sec->v)->getStr(buf, maxBufSize, 10);
}
mclSize blsSecretKeyGetHexStr(char *buf, mclSize maxBufSize, const blsSecretKey *sec)
{
	return cast(&sec->v)->getStr(buf, maxBufSize, 16);
}
int blsPublicKeySetHexStr(blsPublicKey *pub, const char *buf, mclSize bufSize)
{
	return cast(&pub->v)->deserialize(buf, bufSize, 16) > 0 ? 0 : -1;
}
mclSize blsPublicKeyGetHexStr(char *buf, mclSize maxBufSize, const blsPublicKey *pub)
{
	return cast(&pub->v)->getStr(buf, maxBufSize, 16);
}
int blsSignatureSetHexStr(blsSignature *sig, const char *buf, mclSize bufSize)
{
	return cast(&sig->v)->deserialize(buf, bufSize, 16) > 0 ? 0 : -1;
}
mclSize blsSignatureGetHexStr(char *buf, mclSize maxBufSize, const blsSignature *sig)
{
	return cast(&sig->v)->getStr(buf, maxBufSize, 16);
}
void blsDHKeyExchange(blsPublicKey *out, const blsSecretKey *sec, const blsPublicKey *pub)
{
	GmulCT(*cast(&out->v), *cast(&pub->v), *cast(&sec->v));
}

void normalizePubVec(blsPublicKey *pubVec, mclSize n)
{
	for (size_t i = 0; i < n; i++) {
		cast(&pubVec[i].v)->normalize();
	}
}

#define CYBOZU_DONT_USE_OPENSSL
#include <cybozu/sha2.hpp>
#include <cybozu/endian.hpp>
void hashPublicKey(cybozu::Sha256& h, const blsPublicKey *pubVec, mclSize n)
{
	for (size_t i = 0; i < n; i++) {
		const Gother& v = *cast(&pubVec[i].v);
		char buf[sizeof(Gother) / 3];
		size_t m = v.x.serialize(buf, sizeof(buf));
		assert(m > 0);
		h.update(buf, m);
		m = v.y.serialize(buf, sizeof(buf));
		assert(m > 0);
		h.update(buf, m);
	}
}

void hashToFr(Fr *out, const cybozu::Sha256& h0, mclSize begin, mclSize n)
{
	for (size_t i = 0; i < n; i++) {
		cybozu::Sha256 h = h0;
		uint8_t md[32];
		char buf[4];
		cybozu::Set32bitAsLE(buf, uint32_t(begin + i));
		h.digest(md, sizeof(md), buf, sizeof(buf));
		out[i].setArrayMask(md, sizeof(md));
	}
}

template<class T, class U>
void aggregate(T& out, const cybozu::Sha256& h0, const U *vec, mclSize n)
{
	out.clear();
	const size_t N = 16;
	Fr t[N];
	size_t pos = 0;
	while (pos < n) {
		size_t m = n - pos;
		if (m > N) m = N;
		hashToFr(t, h0, pos, m);
		T sub;
		T::mulVec(sub, cast(&vec[pos].v), t, m);
		out += sub;
		pos += m;
	}
}

// aggSig = sum sigVec[i] t_i where (t_1, ..., t_n) = H({pubVec})
void blsMultiAggregateSignature(blsSignature *aggSig, blsSignature *sigVec, blsPublicKey *pubVec, mclSize n)
{
	normalizePubVec(pubVec, n);
	cybozu::Sha256 h0;
	hashPublicKey(h0, pubVec, n);
	G out;
	aggregate(out, h0, sigVec, n);
	*cast(&aggSig->v) = out;
}
// aggPub = sum pubVec[i] t_i where (t_1, ..., t_n) = H({pubVec})
void blsMultiAggregatePublicKey(blsPublicKey *aggPub, blsPublicKey *pubVec, mclSize n)
{
	normalizePubVec(pubVec, n);
	cybozu::Sha256 h0;
	hashPublicKey(h0, pubVec, n);
	Gother out;
	aggregate(out, h0, pubVec, n);
	*cast(&aggPub->v) = out;
}

#endif

