#pragma once
/**
	@file
	@brief somewhat homomorphic encryption with one-time multiplication, based on prime-order pairings
	@author MITSUNARI Shigeo(@herumi)
	see https://github.com/herumi/mcl/blob/master/misc/she/she.pdf
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <cmath>
#include <vector>
#include <iosfwd>
#ifndef MCLBN_FP_UNIT_SIZE
	#define MCLBN_FP_UNIT_SIZE 4
#endif
#if MCLBN_FP_UNIT_SIZE == 4
#include <mcl/bn256.hpp>
#elif MCLBN_FP_UNIT_SIZE == 6
#include <mcl/bn384.hpp>
#elif MCLBN_FP_UNIT_SIZE == 8
#include <mcl/bn512.hpp>
#else
#define MCL_MAX_FP_BIT_SIZE (MCLBN_FP_UNIT_SIZE * 64)
#include <mcl/bn.hpp>
#endif

#include <mcl/window_method.hpp>
#include <cybozu/endian.hpp>
#include <cybozu/serializer.hpp>
#include <cybozu/sha2.hpp>
#include <mcl/ecparam.hpp>

namespace mcl { namespace she {

using namespace mcl::bn;

namespace local {

#ifndef MCLSHE_WIN_SIZE
	#define MCLSHE_WIN_SIZE 10
#endif
static const size_t winSize = MCLSHE_WIN_SIZE;
static const size_t defaultHashSize = 1024;
static const size_t defaultTryNum = 1;

struct KeyCount {
	uint32_t key;
	int32_t count; // power
	bool operator<(const KeyCount& rhs) const
	{
		return key < rhs.key;
	}
	bool isSame(const KeyCount& rhs) const
	{
		return key == rhs.key && count == rhs.count;
	}
};

template<class G, bool = true>
struct InterfaceForHashTable : G {
	static G& castG(InterfaceForHashTable& x) { return static_cast<G&>(x); }
	static const G& castG(const InterfaceForHashTable& x) { return static_cast<const G&>(x); }
	void clear() { clear(castG(*this)); }
	void normalize() { normalize(castG(*this)); }
	static bool isOdd(const G& P) { return P.y.isOdd(); }
	static bool isZero(const G& P) { return P.isZero(); }
	static bool isSameX(const G& P, const G& Q) { return P.x == Q.x; }
	static uint32_t getHash(const G& P) { return uint32_t(*P.x.getUnit()); }
	static void clear(G& P) { P.clear(); }
	static void normalize(G& P) { P.normalize(); }
	static void dbl(G& Q, const G& P) { G::dbl(Q, P); }
	static void neg(G& Q, const G& P) { G::neg(Q, P); }
	static void add(G& R, const G& P, const G& Q) { G::add(R, P, Q); }
	template<class INT>
	static void mul(G& Q, const G& P, const INT& x) { G::mul(Q, P, x); }
};

/*
	treat Fp12 as EC
	unitary inverse of (a, b) = (a, -b)
	then b.a.a or -b.a.a is odd
*/
template<class G>
struct InterfaceForHashTable<G, false> : G {
	static G& castG(InterfaceForHashTable& x) { return static_cast<G&>(x); }
	static const G& castG(const InterfaceForHashTable& x) { return static_cast<const G&>(x); }
	void clear() { clear(castG(*this)); }
	void normalize() { normalize(castG(*this)); }
	static bool isOdd(const G& x) { return x.b.a.a.isOdd(); }
	static bool isZero(const G& x) { return x.isOne(); }
	static bool isSameX(const G& x, const G& Q) { return x.a == Q.a; }
	static uint32_t getHash(const G& x) { return uint32_t(*x.getFp0()->getUnit()); }
	static void clear(G& x) { x = 1; }
	static void normalize(G&) { }
	static void dbl(G& y, const G& x) { G::sqr(y, x); }
	static void neg(G& Q, const G& P) { G::unitaryInv(Q, P); }
	static void add(G& z, const G& x, const G& y) { G::mul(z, x, y); }
	template<class INT>
	static void mul(G& z, const G& x, const INT& y) { G::pow(z, x, y); }
};

template<class G>
char GtoChar();
template<>char GtoChar<bn::G1>() { return '1'; }
template<>char GtoChar<bn::G2>() { return '2'; }
template<>char GtoChar<bn::GT>() { return 'T'; }

/*
	HashTable<EC, true> or HashTable<Fp12, false>
*/
template<class G, bool isEC = true>
class HashTable {
	typedef InterfaceForHashTable<G, isEC> I;
	typedef std::vector<KeyCount> KeyCountVec;
	KeyCountVec kcv_;
	G P_;
	mcl::fp::WindowMethod<I> wm_;
	G nextP_;
	G nextNegP_;
	size_t tryNum_;
	void setWindowMethod()
	{
		const size_t bitSize = G::BaseFp::BaseFp::getBitSize();
		wm_.init(static_cast<const I&>(P_), bitSize, local::winSize);
	}
public:
	HashTable() : tryNum_(local::defaultTryNum) {}
	bool operator==(const HashTable& rhs) const
	{
		if (kcv_.size() != rhs.kcv_.size()) return false;
		for (size_t i = 0; i < kcv_.size(); i++) {
			if (!kcv_[i].isSame(rhs.kcv_[i])) return false;
		}
		return P_ == rhs.P_ && nextP_ == rhs.nextP_;
	}
	bool operator!=(const HashTable& rhs) const { return !operator==(rhs); }
	/*
		compute log_P(xP) for |x| <= hashSize * tryNum
	*/
	void init(const G& P, size_t hashSize)
	{
		if (hashSize == 0) {
			kcv_.clear();
			return;
		}
		if (hashSize >= 0x80000000u) throw cybozu::Exception("HashTable:init:hashSize is too large");
		P_ = P;
		kcv_.resize(hashSize);
		G xP;
		I::clear(xP);
		for (int i = 1; i <= (int)kcv_.size(); i++) {
			I::add(xP, xP, P_);
			I::normalize(xP);
			kcv_[i - 1].key = I::getHash(xP);
			kcv_[i - 1].count = I::isOdd(xP) ? i : -i;
		}
		nextP_ = xP;
		I::dbl(nextP_, nextP_);
		I::add(nextP_, nextP_, P_); // nextP = (hasSize * 2 + 1)P
		I::neg(nextNegP_, nextP_); // nextNegP = -nextP
		/*
			ascending order of abs(count) for same key
		*/
		std::stable_sort(kcv_.begin(), kcv_.end());
		setWindowMethod();
	}
	void init(const G& P, size_t hashSize, size_t tryNum)
	{
		init(P, hashSize);
		setTryNum(tryNum);
	}
	void setTryNum(size_t tryNum)
	{
		this->tryNum_ = tryNum;
	}
	/*
		log_P(xP)
		find range which has same hash of xP in kcv_,
		and detect it
	*/
	int basicLog(G xP, bool *pok = 0) const
	{
		if (pok) *pok = true;
		if (I::isZero(xP)) return 0;
		typedef KeyCountVec::const_iterator Iter;
		KeyCount kc;
		I::normalize(xP);
		kc.key = I::getHash(xP);
		kc.count = 0;
		std::pair<Iter, Iter> p = std::equal_range(kcv_.begin(), kcv_.end(), kc);
		G Q;
		I::clear(Q);
		int prev = 0;
		/*
			check range which has same hash
		*/
		while (p.first != p.second) {
			int count = p.first->count;
			int abs_c = std::abs(count);
			assert(abs_c >= prev); // assume ascending order
			bool neg = count < 0;
			G T;
//			I::mul(T, P, abs_c - prev);
			mulByWindowMethod(T, abs_c - prev);
			I::add(Q, Q, T);
			I::normalize(Q);
			if (I::isSameX(Q, xP)) {
				bool QisOdd = I::isOdd(Q);
				bool xPisOdd = I::isOdd(xP);
				if (QisOdd ^ xPisOdd ^ neg) return -count;
				return count;
			}
			prev = abs_c;
			++p.first;
		}
		if (pok) {
			*pok = false;
			return 0;
		}
		throw cybozu::Exception("HashTable:basicLog:not found");
	}
	/*
		compute log_P(xP)
		call basicLog at most 2 * tryNum
	*/
	int64_t log(const G& xP, bool *pok = 0) const
	{
		bool ok;
		int c = basicLog(xP, &ok);
		if (ok) {
			if (pok) *pok = true;
			return c;
		}
		G posP = xP, negP = xP;
		int64_t posCenter = 0;
		int64_t negCenter = 0;
		int64_t next = (int64_t)kcv_.size() * 2 + 1;
		for (size_t i = 1; i < tryNum_; i++) {
			I::add(posP, posP, nextNegP_);
			posCenter += next;
			c = basicLog(posP, &ok);
			if (ok) {
				if (pok) *pok = true;
				return posCenter + c;
			}
			I::add(negP, negP, nextP_);
			negCenter -= next;
			c = basicLog(negP, &ok);
			if (ok) {
				if (pok) *pok = true;
				return negCenter + c;
			}
		}
		if (pok) {
			*pok = false;
			return 0;
		}
		throw cybozu::Exception("HashTable:log:not found:tryNum") << tryNum_;
	}
	/*
		remark
		tryNum is not saved.
	*/
	template<class OutputStream>
	void save(OutputStream& os) const
	{
		cybozu::save(os, BN::param.cp.curveType);
		cybozu::writeChar(os, GtoChar<G>());
		cybozu::save(os, kcv_.size());
		cybozu::write(os, &kcv_[0], sizeof(kcv_[0]) * kcv_.size());
		P_.save(os);
	}
	size_t save(void *buf, size_t maxBufSize) const
	{
		cybozu::MemoryOutputStream os(buf, maxBufSize);
		save(os);
		return os.getPos();
	}
	/*
		remark
		tryNum is not set
	*/
	template<class InputStream>
	void load(InputStream& is)
	{
		int curveType;
		cybozu::load(curveType, is);
		if (curveType != BN::param.cp.curveType) throw cybozu::Exception("HashTable:bad curveType") << curveType;
		char c = 0;
		if (!cybozu::readChar(&c, is) || c != GtoChar<G>()) throw cybozu::Exception("HashTable:bad c") << (int)c;
		size_t kcvSize;
		cybozu::load(kcvSize, is);
		kcv_.resize(kcvSize);
		cybozu::read(&kcv_[0], sizeof(kcv_[0]) * kcvSize, is);
		P_.load(is);
		I::mul(nextP_, P_, (kcvSize * 2) + 1);
		I::neg(nextNegP_, nextP_);
		setWindowMethod();
	}
	size_t load(const void *buf, size_t bufSize)
	{
		cybozu::MemoryInputStream is(buf, bufSize);
		load(is);
		return is.getPos();
	}
	const mcl::fp::WindowMethod<I>& getWM() const { return wm_; }
	/*
		mul(x, P, y);
	*/
	template<class T>
	void mulByWindowMethod(G& x, const T& y) const
	{
		wm_.mul(static_cast<I&>(x), y);
	}
	size_t getTableSize() const { return kcv_.size(); }
};

template<class G>
int log(const G& P, const G& xP)
{
	if (xP.isZero()) return 0;
	if (xP == P) return 1;
	G negT;
	G::neg(negT, P);
	if (xP == negT) return -1;
	G T = P;
	for (int i = 2; i < 100; i++) {
		T += P;
		if (xP == T) return i;
		G::neg(negT, T);
		if (xP == negT) return -i;
	}
	throw cybozu::Exception("she:log:not found");
}

struct Hash {
	cybozu::Sha256 h_;
	template<class T>
	Hash& operator<<(const T& t)
	{
		char buf[sizeof(T)];
		cybozu::MemoryOutputStream os(buf, sizeof(buf));
		t.save(os);
		h_.update(buf, os.getPos());
		return *this;
	}
	template<class F>
	void get(F& x)
	{
		uint8_t md[32];
		h_.digest(md, sizeof(md), 0, 0);
		x.setArrayMask(md, sizeof(md));
	}
};

} // mcl::she::local

template<size_t dummyInpl = 0>
struct SHET {
	class SecretKey;
	class PublicKey;
	class PrecomputedPublicKey;
	// additive HE
	class CipherTextA; // = CipherTextG1 + CipherTextG2
	class CipherTextGT; // multiplicative HE
	class CipherText; // CipherTextA + CipherTextGT

	static G1 P_;
	static G2 Q_;
	static GT ePQ_; // e(P, Q)
	static std::vector<Fp6> Qcoeff_;
	static local::HashTable<G1> PhashTbl_;
	static local::HashTable<G2> QhashTbl_;
	static mcl::fp::WindowMethod<G2> Qwm_;
	typedef local::InterfaceForHashTable<GT, false> GTasEC;
	static local::HashTable<GT, false> ePQhashTbl_;
	static bool useDecG1ViaGT_;
	static bool useDecG2ViaGT_;
	static bool isG1only_;
private:
	template<class G>
	class CipherTextAT : public fp::Serializable<CipherTextAT<G> > {
		G S_, T_;
		friend class SecretKey;
		friend class PublicKey;
		friend class PrecomputedPublicKey;
		friend class CipherTextA;
		friend class CipherTextGT;
		bool isZero(const Fr& x) const
		{
			G xT;
			G::mul(xT, T_, x);
			return S_ == xT;
		}
	public:
		const G& getS() const { return S_; }
		const G& getT() const { return T_; }
		G& getNonConstRefS() { return S_; }
		G& getNonConstRefT() { return T_; }
		void clear()
		{
			S_.clear();
			T_.clear();
		}
		bool isValid() const
		{
			return S_.isValid() && T_.isValid();
		}
		static void add(CipherTextAT& z, const CipherTextAT& x, const CipherTextAT& y)
		{
			/*
				(S, T) + (S', T') = (S + S', T + T')
			*/
			G::add(z.S_, x.S_, y.S_);
			G::add(z.T_, x.T_, y.T_);
		}
		static void sub(CipherTextAT& z, const CipherTextAT& x, const CipherTextAT& y)
		{
			/*
				(S, T) - (S', T') = (S - S', T - T')
			*/
			G::sub(z.S_, x.S_, y.S_);
			G::sub(z.T_, x.T_, y.T_);
		}
		// INT = int64_t or Fr
		template<class INT>
		static void mul(CipherTextAT& z, const CipherTextAT& x, const INT& y)
		{
			G::mul(z.S_, x.S_, y);
			G::mul(z.T_, x.T_, y);
		}
		static void neg(CipherTextAT& y, const CipherTextAT& x)
		{
			G::neg(y.S_, x.S_);
			G::neg(y.T_, x.T_);
		}
		void add(const CipherTextAT& c) { add(*this, *this, c); }
		void sub(const CipherTextAT& c) { sub(*this, *this, c); }
		template<class InputStream>
		void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
		{
			S_.load(pb, is, ioMode); if (!*pb) return;
			T_.load(pb, is, ioMode);
		}
		template<class OutputStream>
		void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			S_.save(pb, os, ioMode); if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			T_.save(pb, os, ioMode);
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			bool b;
			load(&b, is, ioMode);
			if (!b) throw cybozu::Exception("she:CipherTextA:load");
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			bool b;
			save(&b, os, ioMode);
			if (!b) throw cybozu::Exception("she:CipherTextA:save");
		}
		friend std::istream& operator>>(std::istream& is, CipherTextAT& self)
		{
			self.load(is, fp::detectIoMode(G::getIoMode(), is));
			return is;
		}
		friend std::ostream& operator<<(std::ostream& os, const CipherTextAT& self)
		{
			self.save(os, fp::detectIoMode(G::getIoMode(), os));
			return os;
		}
		bool operator==(const CipherTextAT& rhs) const
		{
			return S_ == rhs.S_ && T_ == rhs.T_;
		}
		bool operator!=(const CipherTextAT& rhs) const { return !operator==(rhs); }
	};
	/*
		g1 = millerLoop(P1, Q)
		g2 = millerLoop(P2, Q)
	*/
	static void doubleMillerLoop(GT& g1, GT& g2, const G1& P1, const G1& P2, const G2& Q)
	{
#if 1
		std::vector<Fp6> Qcoeff;
		precomputeG2(Qcoeff, Q);
		precomputedMillerLoop(g1, P1, Qcoeff);
		precomputedMillerLoop(g2, P2, Qcoeff);
#else
		millerLoop(g1, P1, Q);
		millerLoop(g2, P2, Q);
#endif
	}
	static void finalExp4(GT out[4], const GT in[4])
	{
		for (int i =  0; i < 4; i++) {
			finalExp(out[i], in[i]);
		}
	}
	static void tensorProductML(GT g[4], const G1& S1, const G1& T1, const G2& S2, const G2& T2)
	{
		/*
			(S1, T1) x (S2, T2) = (ML(S1, S2), ML(S1, T2), ML(T1, S2), ML(T1, T2))
		*/
		doubleMillerLoop(g[0], g[2], S1, T1, S2);
		doubleMillerLoop(g[1], g[3], S1, T1, T2);
	}
	static void tensorProduct(GT g[4], const G1& S1, const G1& T1, const G2& S2, const G2& T2)
	{
		/*
			(S1, T1) x (S2, T2) = (e(S1, S2), e(S1, T2), e(T1, S2), e(T1, T2))
		*/
		tensorProductML(g,S1, T1, S2,T2);
		finalExp4(g, g);
	}
	template<class Tag, size_t n>
	struct ZkpT : public fp::Serializable<ZkpT<Tag, n> > {
		Fr d_[n];
		template<class InputStream>
		void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
		{
			for (size_t i = 0; i < n; i++) {
				d_[i].load(pb, is, ioMode); if (!*pb) return;
			}
		}
		template<class OutputStream>
		void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			d_[0].save(pb, os, ioMode); if (!*pb) return;
			for (size_t i = 1; i < n; i++) {
				if (sep) {
					cybozu::writeChar(pb, os, sep);
					if (!*pb) return;
				}
				d_[i].save(pb, os, ioMode);
			}
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			bool b;
			load(&b, is, ioMode);
			if (!b) throw cybozu::Exception("she:ZkpT:load");
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			bool b;
			save(&b, os, ioMode);
			if (!b) throw cybozu::Exception("she:ZkpT:save");
		}
		friend std::istream& operator>>(std::istream& is, ZkpT& self)
		{
			self.load(is, fp::detectIoMode(Fr::getIoMode(), is));
			return is;
		}
		friend std::ostream& operator<<(std::ostream& os, const ZkpT& self)
		{
			self.save(os, fp::detectIoMode(Fr::getIoMode(), os));
			return os;
		}
	};
	struct ZkpBinTag;
	struct ZkpEqTag; // d_[] = { c, sp, ss, sm }
	struct ZkpBinEqTag; // d_[] = { d0, d1, sp0, sp1, ss, sp, sm }
	struct ZkpDecTag; // d_[] = { c, h }
	struct ZkpDecGTTag; // d_[] = { d1, d2, d3, h }
public:
	/*
		Zkp for m = 0 or 1
	*/
	typedef ZkpT<ZkpBinTag, 4> ZkpBin;
	/*
		Zkp for decG1(c1) == decG2(c2)
	*/
	typedef ZkpT<ZkpEqTag, 4> ZkpEq;
	/*
		Zkp for (m = 0 or 1) and decG1(c1) == decG2(c2)
	*/
	typedef ZkpT<ZkpBinEqTag, 7> ZkpBinEq;
	/*
		Zkp for Dec(c) = m for c in G1
	*/
	typedef ZkpT<ZkpDecTag, 2> ZkpDec;
	/*
		Zkp for Dec(c) = m for c in GT
	*/
	typedef ZkpT<ZkpDecGTTag, 4> ZkpDecGT;

	typedef CipherTextAT<G1> CipherTextG1;
	typedef CipherTextAT<G2> CipherTextG2;
	/*
		auxiliary for ZkpDecGT
		@note GT is multiplicative group though treating GT as additive group in comment
	*/
	struct AuxiliaryForZkpDecGT {
		GT R_[4]; // [R = e(R, Q), xR, yR, xyR]

		// dst = v[1] a[0] + v[0] a[1] - v[2] a[2]
		void f(GT& dst, const GT *v, const Fr *a) const
		{
			GT t;
			GT::pow(dst, v[0], a[1]);
			GT::pow(t, v[1], a[0]);
			dst *= t;
			GT::pow(t, v[2], a[2]);
			GT::unitaryInv(t, t);
			dst *= t;
		}
		bool verify(const CipherTextGT& c, int64_t m, const ZkpDecGT& zkp) const
		{
			const Fr *d = &zkp.d_[0];
			const Fr &h = zkp.d_[3];

			GT A[4];
			GT t;
			GT::pow(t, R_[0], m); // m R
			GT::unitaryInv(t, t);
			GT::mul(A[0], c.g_[0], t);
			A[1] = c.g_[1];
			A[2] = c.g_[2];
			A[3] = c.g_[3];
			GT B[3], X;
			for (int i = 0; i < 3; i++) {
				GT::pow(B[i], R_[0], d[i]);
				GT::pow(t, R_[i+1], h);
				GT::unitaryInv(t, t);
				B[i] *= t;
			}
			f(X, A + 1, zkp.d_);
			GT::pow(t, A[0], h);
			GT::unitaryInv(t, t);
			X *= t;
			local::Hash hash;
			hash << R_[1] << R_[2] << R_[3] << A[0] << A[1] << A[2] << A[3] << B[0] << B[1] << B[2] << X;
			Fr h2;
			hash.get(h2);
			return h == h2;
		}
	};

	static void init(const mcl::CurveParam& cp = mcl::BN254, size_t hashSize = local::defaultHashSize, size_t tryNum = local::defaultTryNum)
	{
		initPairing(cp);
		hashAndMapToG1(P_, "0");
		hashAndMapToG2(Q_, "0");
		pairing(ePQ_, P_, Q_);
		precomputeG2(Qcoeff_, Q_);
		setRangeForDLP(hashSize);
		useDecG1ViaGT_ = false;
		useDecG2ViaGT_ = false;
		isG1only_ = false;
		setTryNum(tryNum);
	}
	/*
		standard lifted ElGamal encryption
	*/
	static void initG1only(int curveType, size_t hashSize = local::defaultHashSize, size_t tryNum = local::defaultTryNum)
	{
		mcl::initCurve<G1, Fr>(curveType, &P_);
		setRangeForG1DLP(hashSize);
		useDecG1ViaGT_ = false;
		useDecG2ViaGT_ = false;
		isG1only_ = true;
		setTryNum(tryNum);
	}
	static void initG1only(const mcl::EcParam& para, size_t hashSize = local::defaultHashSize, size_t tryNum = local::defaultTryNum)
	{
		initG1only(para.curveType, hashSize, tryNum);
	}
	/*
		set range for G1-DLP
	*/
	static void setRangeForG1DLP(size_t hashSize)
	{
		PhashTbl_.init(P_, hashSize);
	}
	/*
		set range for G2-DLP
	*/
	static void setRangeForG2DLP(size_t hashSize)
	{
		QhashTbl_.init(Q_, hashSize);
	}
	/*
		set range for GT-DLP
	*/
	static void setRangeForGTDLP(size_t hashSize)
	{
		ePQhashTbl_.init(ePQ_, hashSize);
	}
	/*
		set range for G1/G2/GT DLP
		decode message m for |m| <= hasSize * tryNum
		decode time = O(log(hasSize) * tryNum)
	*/
	static void setRangeForDLP(size_t hashSize)
	{
		setRangeForG1DLP(hashSize);
		setRangeForG2DLP(hashSize);
		setRangeForGTDLP(hashSize);
	}
	static void setTryNum(size_t tryNum)
	{
		PhashTbl_.setTryNum(tryNum);
		QhashTbl_.setTryNum(tryNum);
		ePQhashTbl_.setTryNum(tryNum);
	}
	static void useDecG1ViaGT(bool use = true)
	{
		useDecG1ViaGT_ = use;
	}
	static void useDecG2ViaGT(bool use = true)
	{
		useDecG2ViaGT_ = use;
	}
	/*
		only one element is necessary for each G1 and G2.
		this is better than David Mandell Freeman's algorithm
	*/
	class SecretKey : public fp::Serializable<SecretKey> {
		Fr x_, y_;
		void getPowOfePQ(GT& v, const CipherTextGT& c) const
		{
			/*
				(s, t, u, v) := (e(S, S'), e(S, T'), e(T, S'), e(T, T'))
				s v^(xy) / (t^y u^x) = s (v^x / t) ^ y / u^x
				= e(P, Q)^(mm')
			*/
			GT t, u;
			GT::unitaryInv(t, c.g_[1]);
			GT::unitaryInv(u, c.g_[2]);
			GT::pow(v, c.g_[3], x_);
			v *= t;
			GT::pow(v, v, y_);
			GT::pow(u, u, x_);
			v *= u;
			v *= c.g_[0];
		}
	public:
		void setByCSPRNG()
		{
			x_.setRand();
			if (!isG1only_) y_.setRand();
		}
		/*
			set xP and yQ
		*/
		void getPublicKey(PublicKey& pub) const
		{
			pub.set(x_, y_);
		}
#if 0
		// log_x(y)
		int log(const GT& x, const GT& y) const
		{
			if (y == 1) return 0;
			if (y == x) return 1;
			GT inv;
			GT::unitaryInv(inv, x);
			if (y == inv) return -1;
			GT t = x;
			for (int i = 2; i < 100; i++) {
				t *= x;
				if (y == t) return i;
				GT::unitaryInv(inv, t);
				if (y == inv) return -i;
			}
			throw cybozu::Exception("she:dec:log:not found");
		}
#endif
		int64_t dec(const CipherTextG1& c, bool *pok = 0) const
		{
			if (useDecG1ViaGT_) return decViaGT(c);
			/*
				S = mP + rxP
				T = rP
				R = S - xT = mP
			*/
			G1 R;
			G1::mul(R, c.T_, x_);
			G1::sub(R, c.S_, R);
			return PhashTbl_.log(R, pok);
		}
		int64_t dec(const CipherTextG2& c, bool *pok = 0) const
		{
			if (useDecG2ViaGT_) return decViaGT(c);
			G2 R;
			G2::mul(R, c.T_, y_);
			G2::sub(R, c.S_, R);
			return QhashTbl_.log(R, pok);
		}
		int64_t dec(const CipherTextA& c, bool *pok = 0) const
		{
			return dec(c.c1_, pok);
		}
		int64_t dec(const CipherTextGT& c, bool *pok = 0) const
		{
			GT v;
			getPowOfePQ(v, c);
			return ePQhashTbl_.log(v, pok);
//			return log(g, v);
		}
		int64_t decViaGT(const CipherTextG1& c, bool *pok = 0) const
		{
			G1 R;
			G1::mul(R, c.T_, x_);
			G1::sub(R, c.S_, R);
			GT v;
			pairing(v, R, Q_);
			return ePQhashTbl_.log(v, pok);
		}
		int64_t decViaGT(const CipherTextG2& c, bool *pok = 0) const
		{
			G2 R;
			G2::mul(R, c.T_, y_);
			G2::sub(R, c.S_, R);
			GT v;
			pairing(v, P_, R);
			return ePQhashTbl_.log(v, pok);
		}
		int64_t dec(const CipherText& c, bool *pok = 0) const
		{
			if (c.isMultiplied()) {
				return dec(c.m_, pok);
			} else {
				return dec(c.a_, pok);
			}
		}
		bool isZero(const CipherTextG1& c) const
		{
			return c.isZero(x_);
		}
		bool isZero(const CipherTextG2& c) const
		{
			return c.isZero(y_);
		}
		bool isZero(const CipherTextA& c) const
		{
			return c.c1_.isZero(x_);
		}
		bool isZero(const CipherTextGT& c) const
		{
			GT v;
			getPowOfePQ(v, c);
			return v.isOne();
		}
		bool isZero(const CipherText& c) const
		{
			if (c.isMultiplied()) {
				return isZero(c.m_);
			} else {
				return isZero(c.a_);
			}
		}
		int64_t decWithZkpDec(bool *pok, ZkpDec& zkp, const CipherTextG1& c, const PublicKey& pub) const
		{
			/*
				c = (S, T)
				S = mP + rxP
				T = rP
				R = S - xT = mP
			*/
			G1 R;
			G1::mul(R, c.T_, x_);
			G1::sub(R, c.S_, R);
			int64_t m = PhashTbl_.log(R, pok);
			if (!*pok) return 0;
			const G1& P1 = P_;
			const G1& P2 = c.T_; // rP
			const G1& A1 = pub.xP_;
			G1 A2;
			G1::sub(A2, c.S_, R); // rxP
			Fr b;
			b.setRand();
			G1 B1, B2;
			G1::mul(B1, P1, b);
			G1::mul(B2, P2, b);
			Fr& d = zkp.d_[0];
			Fr& h = zkp.d_[1];
			local::Hash hash;
			hash << P2 << A1 << A2 << B1 << B2;
			hash.get(h);
			Fr::mul(d, h, x_);
			d += b;
			return m;
		}
		// @note GT is multiplicative group though treating GT as additive group in comment
		int64_t decWithZkpDec(bool *pok, ZkpDecGT& zkp, const CipherTextGT& c, const AuxiliaryForZkpDecGT& aux) const
		{
			int64_t m = dec(c, pok);
			if (!*pok) return 0;
			// A = c - Enc(m; 0, 0, 0) = c - (m R, 0, 0, 0)
			GT A[4];
			GT t;
			GT::pow(t, aux.R_[0], m); // m R
			GT::unitaryInv(t, t);
			GT::mul(A[0], c.g_[0], t);
			A[1] = c.g_[1];
			A[2] = c.g_[2];
			A[3] = c.g_[3];
			// dec(A) = 0

			Fr b[3];
			GT B[3], X;
			for (int i = 0; i < 3; i++) {
				b[i].setByCSPRNG();
				GT::pow(B[i], aux.R_[0], b[i]);
			}
			aux.f(X, A + 1, b);
			local::Hash hash;
			hash << aux.R_[1] << aux.R_[2] << aux.R_[3] << A[0] << A[1] << A[2] << A[3] << B[0] << B[1] << B[2] << X;
			Fr *d = &zkp.d_[0];
			Fr &h = zkp.d_[3];
			hash.get(h);
			Fr::mul(d[0], h, x_); // h x
			Fr::mul(d[1], h, y_); // h y
			Fr::mul(d[2], d[1], x_); // h xy
			for (int i = 0; i < 3; i++) {
				d[i] += b[i];
			}
			return m;
		}
		int64_t decWithZkpDec(ZkpDec& zkp, const CipherTextG1& c, const PublicKey& pub) const
		{
			bool b;
			int64_t ret = decWithZkpDec(&b, zkp, c, pub);
			if (!b) throw cybozu::Exception("she:SecretKey:decWithZkpDec");
			return ret;
		}
		int64_t decWithZkpDec(ZkpDecGT& zkp, const CipherTextGT& c, const AuxiliaryForZkpDecGT& aux) const
		{
			bool b;
			int64_t ret = decWithZkpDec(&b, zkp, c, aux);
			if (!b) throw cybozu::Exception("she:SecretKey:decWithZkpDec");
			return ret;
		}
		template<class InputStream>
		void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
		{
			x_.load(pb, is, ioMode); if (!*pb) return;
			if (!isG1only_) y_.load(pb, is, ioMode);
		}
		template<class OutputStream>
		void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			x_.save(pb, os, ioMode); if (!*pb) return;
			if (isG1only_) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			y_.save(os, ioMode);
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			bool b;
			load(&b, is, ioMode);
			if (!b) throw cybozu::Exception("she:SecretKey:load");
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			bool b;
			save(&b, os, ioMode);
			if (!b) throw cybozu::Exception("she:SecretKey:save");
		}
		friend std::istream& operator>>(std::istream& is, SecretKey& self)
		{
			self.load(is, fp::detectIoMode(Fr::getIoMode(), is));
			return is;
		}
		friend std::ostream& operator<<(std::ostream& os, const SecretKey& self)
		{
			self.save(os, fp::detectIoMode(Fr::getIoMode(), os));
			return os;
		}
		bool operator==(const SecretKey& rhs) const
		{
			return x_ == rhs.x_ && (isG1only_ || y_ == rhs.y_);
		}
		bool operator!=(const SecretKey& rhs) const { return !operator==(rhs); }
	};
private:
	/*
		simple ElGamal encryptionfor G1 and G2
		(S, T) = (m P + r xP, rP)
		Pmul.mul(X, a) // X = a P
		xPmul.mul(X, a) // X = a xP
		use *encRand if encRand is not null
	*/
	template<class G, class INT, class MulG, class I>
	static void ElGamalEnc(G& S, G& T, const INT& m, const mcl::fp::WindowMethod<I>& Pmul, const MulG& xPmul, const Fr *encRand = 0)
	{
		Fr r;
		if (encRand) {
			r = *encRand;
		} else {
			r.setRand();
		}
		Pmul.mul(static_cast<I&>(T), r);
		xPmul.mul(S, r); // S = r xP
		if (m == 0) return;
		G C;
		Pmul.mul(static_cast<I&>(C), m);
		S += C;
	}
	/*
		https://github.com/herumi/mcl/blob/master/misc/she/nizkp.pdf

		encRand is a random value used for ElGamalEnc()
		d[1-m] ; rand
		s[1-m] ; rand
		R[0][1-m] = s[1-m] P - d[1-m] T
		R[1][1-m] = s[1-m] xP - d[1-m] (S - (1-m) P)
		r ; rand
		R[0][m] = r P
		R[1][m] = r xP
		c = H(S, T, R[0][0], R[0][1], R[1][0], R[1][1])
		d[m] = c - d[1-m]
		s[m] = r + d[m] encRand
	*/
	template<class G, class I, class MulG>
	static void makeZkpBin(ZkpBin& zkp, const G& S, const G& T, const Fr& encRand, const G& P, int m, const mcl::fp::WindowMethod<I>& Pmul, const MulG& xPmul)
	{
		if (m != 0 && m != 1) throw cybozu::Exception("makeZkpBin:bad m") << m;
		Fr *s = &zkp.d_[0];
		Fr *d = &zkp.d_[2];
		G R[2][2];
		d[1-m].setRand();
		s[1-m].setRand();
		G T1, T2;
		Pmul.mul(static_cast<I&>(T1),  s[1-m]); // T1 = s[1-m] P
		G::mul(T2, T, d[1-m]);
		G::sub(R[0][1-m], T1, T2); // s[1-m] P - d[1-m]T
		xPmul.mul(T1, s[1-m]); // T1 = s[1-m] xP
		if (m == 0) {
			G::sub(T2, S, P);
			G::mul(T2, T2, d[1-m]);
		} else {
			G::mul(T2, S, d[1-m]);
		}
		G::sub(R[1][1-m], T1, T2); // s[1-m] xP - d[1-m](S - (1-m) P)
		Fr r;
		r.setRand();
		Pmul.mul(static_cast<I&>(R[0][m]), r); // R[0][m] = r P
		xPmul.mul(R[1][m], r); // R[1][m] = r xP
		Fr c;
		local::Hash hash;
		hash << S << T << R[0][0] << R[0][1] << R[1][0] << R[1][1];
		hash.get(c);
		d[m] = c - d[1-m];
		s[m] = r + d[m] * encRand;
	}
	/*
		R[0][i] = s[i] P - d[i] T ; i = 0,1
		R[1][0] = s[0] xP - d[0] S
		R[1][1] = s[1] xP - d[1](S - P)
		c = H(S, T, R[0][0], R[0][1], R[1][0], R[1][1])
		c == d[0] + d[1]
	*/
	template<class G, class I, class MulG>
	static bool verifyZkpBin(const G& S, const G& T, const G& P, const ZkpBin& zkp, const mcl::fp::WindowMethod<I>& Pmul, const MulG& xPmul)
	{
		const Fr *s = &zkp.d_[0];
		const Fr *d = &zkp.d_[2];
		G R[2][2];
		G T1, T2;
		for (int i = 0; i < 2; i++) {
			Pmul.mul(static_cast<I&>(T1), s[i]); // T1 = s[i] P
			G::mul(T2, T, d[i]);
			G::sub(R[0][i], T1, T2);
		}
		xPmul.mul(T1, s[0]); // T1 = s[0] xP
		G::mul(T2, S, d[0]);
		G::sub(R[1][0], T1, T2);
		xPmul.mul(T1, s[1]); // T1 = x[1] xP
		G::sub(T2, S, P);
		G::mul(T2, T2, d[1]);
		G::sub(R[1][1], T1, T2);
		Fr c;
		local::Hash hash;
		hash << S << T << R[0][0] << R[0][1] << R[1][0] << R[1][1];
		hash.get(c);
		return c == d[0] + d[1];
	}
	// check m[i] < m[i+1]
	static bool check_mVec(const int *mVec, size_t mSize)
	{
		if (mSize < 1) return false;
		for (size_t i = 0; i < mSize - 1; i++) {
			if (mVec[i] >= mVec[i + 1]) return false;
		}
		return true;
	}
	/*
		Enc(m; encRand) = (S, T)
		make ZKP for m in M := mVec[0, mSize)
		@note zkp has (mSize * 2) elements
		@note M must satisfy the following properties:
		1) m[i] < m[i+1] for all i < mSize - 1
		2) i0 exists such that m[i0] = m
	*/
	template<class G, class I, class MulG>
	static bool makeZkpSet(Fr *zkp, const G& xP, const G& S, const G& T, const Fr& encRand, int m, const int *mVec, size_t mSize, const mcl::fp::WindowMethod<I>& Pmul, const MulG& xPmul)
	{
		if (!check_mVec(mVec, mSize)) return false;
		// find i0 s.t. m[i0] = m
		size_t i0 = mSize;
		for (size_t i = 0; i < mSize; i++) {
			if (mVec[i] == m) {
				i0 = i;
				break;
			}
		}
		if (i0 == mSize) return false;
		Fr *const a = zkp;
		Fr *const t = zkp + mSize;
		for (size_t i = 0; i < mSize; i++) {
			if (i != i0) {
				a[i].setRand();
			}
			t[i].setRand();
		}
		local::Hash hash;
		hash << xP << S << T;
		Fr sum = 0;
		for (size_t i = 0; i < mSize; i++) {
			Fr u;
			if (i == i0) {
				u.clear();
			} else {
				u = a[i];
				u *= (m - mVec[i]);
				sum += a[i];
			}
			G R1, R2;
			ElGamalEnc(R1, R2, u, Pmul, xPmul, &t[i]);
			hash << R1 << R2;
			if (i != i0) {
				// b[i] = t[i] - a[i] r
				t[i] -= a[i] * encRand;
			}
		}
		Fr h;
		hash.get(h); // h = Hash((S, T), {R_i})
		a[i0] = h - sum;
		t[i0] -= a[i0] * encRand;
		return true;
	}
	/*
		verify ZKP with Dec(S, T) in mVec[0, mSize)
		@note zkp has (mSize * 2) elements
		see https://github.com/herumi/mcl/blob/master/misc/she/nizkp.pdf
	*/
	template<class G, class I, class MulG>
	static bool verifyZkpSet(const G& xP, const G& S, const G& T, const Fr *zkp, const int *mVec, size_t mSize, const mcl::fp::WindowMethod<I>& Pmul, const MulG& xPmul)
	{
		if (!check_mVec(mVec, mSize)) return false;
		const Fr *a = zkp;
		const Fr *b = zkp + mSize;
		Fr c;
		local::Hash hash;
		hash << xP << S << T;
		/*
			ai(C - Enc(mi, 0)) - Enc(0, bi)
			= ai(S - mi P, T) - (bi xP, bi P)
		*/
		Fr sum = 0;
		for (size_t i = 0; i < mSize; i++) {
			G1 S1, S2, T1, T2;
			Pmul.mul(static_cast<I&>(S1), mVec[i]);
			xPmul.mul(S2, b[i]);
			Pmul.mul(static_cast<I&>(T2), b[i]);
			S1 = (S - S1) * a[i] + S2;
			T1 = T * a[i] + T2;
			hash << S1 << T1;
			sum += a[i];
		}
		Fr h;
		hash.get(h);
		return h == sum;
	}
	/*
		encRand1, encRand2 are random values use for ElGamalEnc()
	*/
	template<class G1, class G2, class INT, class I1, class I2, class MulG1, class MulG2>
	static void makeZkpEq(ZkpEq& zkp, G1& S1, G1& T1, G2& S2, G2& T2, const INT& m, const mcl::fp::WindowMethod<I1>& Pmul, const MulG1& xPmul, const mcl::fp::WindowMethod<I2>& Qmul, const MulG2& yQmul)
	{
		Fr p, s;
		p.setRand();
		s.setRand();
		ElGamalEnc(S1, T1, m, Pmul, xPmul, &p);
		ElGamalEnc(S2, T2, m, Qmul, yQmul, &s);
		Fr rp, rs, rm;
		rp.setRand();
		rs.setRand();
		rm.setRand();
		G1 R1, R2;
		G2 R3, R4;
		ElGamalEnc(R1, R2, rm, Pmul, xPmul, &rp);
		ElGamalEnc(R3, R4, rm, Qmul, yQmul, &rs);
		Fr& c = zkp.d_[0];
		Fr& sp = zkp.d_[1];
		Fr& ss = zkp.d_[2];
		Fr& sm = zkp.d_[3];
		local::Hash hash;
		hash << S1 << T1 << S2 << T2 << R1 << R2 << R3 << R4;
		hash.get(c);
		Fr::mul(sp, c, p);
		sp += rp;
		Fr::mul(ss, c, s);
		ss += rs;
		Fr::mul(sm, c, m);
		sm += rm;
	}
	template<class G1, class G2, class I1, class I2, class MulG1, class MulG2>
	static bool verifyZkpEq(const ZkpEq& zkp, const G1& S1, const G1& T1, const G2& S2, const G2& T2, const mcl::fp::WindowMethod<I1>& Pmul, const MulG1& xPmul, const mcl::fp::WindowMethod<I2>& Qmul, const MulG2& yQmul)
	{
		const Fr& c = zkp.d_[0];
		const Fr& sp = zkp.d_[1];
		const Fr& ss = zkp.d_[2];
		const Fr& sm = zkp.d_[3];
		G1 R1, R2, X1;
		G2 R3, R4, X2;
		ElGamalEnc(R1, R2, sm, Pmul, xPmul, &sp);
		G1::mul(X1, S1, c);
		R1 -= X1;
		G1::mul(X1, T1, c);
		R2 -= X1;
		ElGamalEnc(R3, R4, sm, Qmul, yQmul, &ss);
		G2::mul(X2, S2, c);
		R3 -= X2;
		G2::mul(X2, T2, c);
		R4 -= X2;
		Fr c2;
		local::Hash hash;
		hash << S1 << T1 << S2 << T2 << R1 << R2 << R3 << R4;
		hash.get(c2);
		return c == c2;
	}
	/*
		encRand1, encRand2 are random values use for ElGamalEnc()
	*/
	template<class G1, class G2, class I1, class I2, class MulG1, class MulG2>
	static void makeZkpBinEq(ZkpBinEq& zkp, G1& S1, G1& T1, G2& S2, G2& T2, int m, const mcl::fp::WindowMethod<I1>& Pmul, const MulG1& xPmul, const mcl::fp::WindowMethod<I2>& Qmul, const MulG2& yQmul)
	{
		if (m != 0 && m != 1) throw cybozu::Exception("makeZkpBinEq:bad m") << m;
		Fr *d = &zkp.d_[0];
		Fr *spm = &zkp.d_[2];
		Fr& ss = zkp.d_[4];
		Fr& sp = zkp.d_[5];
		Fr& sm = zkp.d_[6];
		Fr p, s;
		p.setRand();
		s.setRand();
		ElGamalEnc(S1, T1, m, Pmul, xPmul, &p);
		ElGamalEnc(S2, T2, m, Qmul, yQmul, &s);
		d[1-m].setRand();
		spm[1-m].setRand();
		G1 R1[2], R2[2], X1;
		Pmul.mul(static_cast<I1&>(R1[1-m]), spm[1-m]);
		G1::mul(X1, T1, d[1-m]);
		R1[1-m] -= X1;
		if (m == 0) {
			G1::sub(X1, S1, P_);
			G1::mul(X1, X1, d[1-m]);
		} else {
			G1::mul(X1, S1, d[1-m]);
		}
		xPmul.mul(R2[1-m], spm[1-m]);
		R2[1-m] -= X1;
		Fr rpm, rp, rs, rm;
		rpm.setRand();
		rp.setRand();
		rs.setRand();
		rm.setRand();
		ElGamalEnc(R2[m], R1[m], 0, Pmul, xPmul, &rpm);
		G1 R3, R4;
		G2 R5, R6;
		ElGamalEnc(R4, R3, rm, Pmul, xPmul, &rp);
		ElGamalEnc(R6, R5, rm, Qmul, yQmul, &rs);
		Fr c;
		local::Hash hash;
		hash << S1 << T1 << R1[0] << R1[1] << R2[0] << R2[1] << R3 << R4 << R5 << R6;
		hash.get(c);
		Fr::sub(d[m], c, d[1-m]);
		Fr::mul(spm[m], d[m], p);
		spm[m] += rpm;
		Fr::mul(sp, c, p);
		sp += rp;
		Fr::mul(ss, c, s);
		ss += rs;
		Fr::mul(sm, c, m);
		sm += rm;
	}
	template<class G1, class G2, class I1, class I2, class MulG1, class MulG2>
	static bool verifyZkpBinEq(const ZkpBinEq& zkp, const G1& S1, const G1& T1, const G2& S2, const G2& T2, const mcl::fp::WindowMethod<I1>& Pmul, const MulG1& xPmul, const mcl::fp::WindowMethod<I2>& Qmul, const MulG2& yQmul)
	{
		const Fr *d = &zkp.d_[0];
		const Fr *spm = &zkp.d_[2];
		const Fr& ss = zkp.d_[4];
		const Fr& sp = zkp.d_[5];
		const Fr& sm = zkp.d_[6];
		G1 R1[2], R2[2], X1;
		for (int i = 0; i < 2; i++) {
			Pmul.mul(static_cast<I1&>(R1[i]), spm[i]);
			G1::mul(X1, T1, d[i]);
			R1[i] -= X1;
		}
		xPmul.mul(R2[0], spm[0]);
		G1::mul(X1, S1, d[0]);
		R2[0] -= X1;
		xPmul.mul(R2[1], spm[1]);
		G1::sub(X1, S1, P_);
		G1::mul(X1, X1, d[1]);
		R2[1] -= X1;
		Fr c;
		Fr::add(c, d[0], d[1]);
		G1 R3, R4;
		G2 R5, R6;
		ElGamalEnc(R4, R3, sm, Pmul, xPmul, &sp);
		G1::mul(X1, T1, c);
		R3 -= X1;
		G1::mul(X1, S1, c);
		R4 -= X1;
		ElGamalEnc(R6, R5, sm, Qmul, yQmul, &ss);
		G2 X2;
		G2::mul(X2, T2, c);
		R5 -= X2;
		G2::mul(X2, S2, c);
		R6 -= X2;
		Fr c2;
		local::Hash hash;
		hash << S1 << T1 << R1[0] << R1[1] << R2[0] << R2[1] << R3 << R4 << R5 << R6;
		hash.get(c2);
		return c == c2;
	}
	/*
		common method for PublicKey and PrecomputedPublicKey
	*/
	template<class T>
	struct PublicKeyMethod {
		/*
			you can use INT as int64_t and Fr,
			but the return type of dec() is int64_t.
		*/
		template<class INT>
		void enc(CipherTextG1& c, const INT& m) const
		{
			static_cast<const T&>(*this).encG1(c, m);
		}
		template<class INT>
		void enc(CipherTextG2& c, const INT& m) const
		{
			static_cast<const T&>(*this).encG2(c, m);
		}
		template<class INT>
		void enc(CipherTextA& c, const INT& m) const
		{
			enc(c.c1_, m);
			enc(c.c2_, m);
		}
		template<class INT>
		void enc(CipherTextGT& c, const INT& m) const
		{
			static_cast<const T&>(*this).encGT(c, m);
		}
		template<class INT>
		void enc(CipherText& c, const INT& m, bool multiplied = false) const
		{
			c.isMultiplied_ = multiplied;
			if (multiplied) {
				enc(c.m_, m);
			} else {
				enc(c.a_, m);
			}
		}
		/*
			reRand method is for circuit privacy
		*/
		template<class CT>
		void reRandT(CT& c) const
		{
			CT c0;
			static_cast<const T&>(*this).enc(c0, 0);
			CT::add(c, c, c0);
		}
		void reRand(CipherTextG1& c) const { reRandT(c); }
		void reRand(CipherTextG2& c) const { reRandT(c); }
		void reRand(CipherTextGT& c) const { reRandT(c); }
		void reRand(CipherText& c) const
		{
			if (c.isMultiplied()) {
				reRandT(c.m_);
			} else {
				reRandT(c.a_);
			}
		}
		/*
			convert from CipherTextG1 to CipherTextGT
		*/
		void convert(CipherTextGT& cm, const CipherTextG1& c1) const
		{
			/*
				Enc(1) = (S, T) = (Q + r yQ, rQ) = (Q, 0) if r = 0
				cm = c1 * (Q, 0) = (S, T) * (Q, 0) = (e(S, Q), 1, e(T, Q), 1)
			*/
			precomputedMillerLoop(cm.g_[0], c1.getS(), Qcoeff_);
			finalExp(cm.g_[0], cm.g_[0]);
			precomputedMillerLoop(cm.g_[2], c1.getT(), Qcoeff_);
			finalExp(cm.g_[2], cm.g_[2]);

			cm.g_[1] = cm.g_[3] = 1;
		}
		/*
			convert from CipherTextG2 to CipherTextGT
		*/
		void convert(CipherTextGT& cm, const CipherTextG2& c2) const
		{
			/*
				Enc(1) = (S, T) = (P + r xP, rP) = (P, 0) if r = 0
				cm = (P, 0) * c2 = (e(P, S), e(P, T), 1, 1)
			*/
			pairing(cm.g_[0], P_, c2.getS());
			pairing(cm.g_[1], P_, c2.getT());
			cm.g_[2] = cm.g_[3] = 1;
		}
		void convert(CipherTextGT& cm, const CipherTextA& ca) const
		{
			convert(cm, ca.c1_);
		}
		void convert(CipherText& cm, const CipherText& ca) const
		{
			if (ca.isMultiplied()) throw cybozu::Exception("she:PublicKey:convertCipherText:already isMultiplied");
			cm.isMultiplied_ = true;
			convert(cm.m_, ca.a_);
		}
	};
public:
	class PublicKey : public fp::Serializable<PublicKey,
		PublicKeyMethod<PublicKey> > {
		G1 xP_;
		G2 yQ_;
		friend class SecretKey;
		friend class PrecomputedPublicKey;
		template<class T>
		friend struct PublicKeyMethod;
		template<class G>
		struct MulG {
			const G& base;
			MulG(const G& base) : base(base) {}
			template<class INT>
			void mul(G& out, const INT& m) const
			{
				G::mul(out, base, m);
			}
		};
		void set(const Fr& x, const Fr& y)
		{
			G1::mul(xP_, P_, x);
			if (!isG1only_) G2::mul(yQ_, Q_, y);
		}
		template<class INT>
		void encG1(CipherTextG1& c, const INT& m) const
		{
			const MulG<G1> xPmul(xP_);
			ElGamalEnc(c.S_, c.T_, m, PhashTbl_.getWM(), xPmul);
		}
		template<class INT>
		void encG2(CipherTextG2& c, const INT& m) const
		{
			const MulG<G2> yQmul(yQ_);
			ElGamalEnc(c.S_, c.T_, m, QhashTbl_.getWM(), yQmul);
		}
public:
		void getAuxiliaryForZkpDecGT(AuxiliaryForZkpDecGT& aux) const
		{
			aux.R_[0] = ePQ_;
			pairing(aux.R_[1], xP_, Q_);
			pairing(aux.R_[2], P_, yQ_);
			pairing(aux.R_[3], xP_, yQ_);
		}
		void encWithZkpBin(CipherTextG1& c, ZkpBin& zkp, int m) const
		{
			Fr encRand;
			encRand.setRand();
			const MulG<G1> xPmul(xP_);
			ElGamalEnc(c.S_, c.T_, m, PhashTbl_.getWM(), xPmul, &encRand);
			makeZkpBin(zkp, c.S_, c.T_, encRand, P_, m,  PhashTbl_.getWM(), xPmul);
		}
		void encWithZkpBin(CipherTextG2& c, ZkpBin& zkp, int m) const
		{
			Fr encRand;
			encRand.setRand();
			const MulG<G2> yQmul(yQ_);
			ElGamalEnc(c.S_, c.T_, m, QhashTbl_.getWM(), yQmul, &encRand);
			makeZkpBin(zkp, c.S_, c.T_, encRand, Q_, m,  QhashTbl_.getWM(), yQmul);
		}
		void encWithZkpSet(CipherTextG1& c, Fr *zkp, int m, const int *mVec, size_t mSize) const
		{
			Fr encRand;
			encRand.setRand();
			const MulG<G1> xPmul(xP_);
			ElGamalEnc(c.S_, c.T_, m, PhashTbl_.getWM(), xPmul, &encRand);
			if (!makeZkpSet(zkp, P_, c.S_, c.T_, encRand, m,  mVec, mSize, PhashTbl_.getWM(), xPmul)) {
				throw cybozu::Exception("encWithZkpSet:bad mVec") << mSize;
			}
		}
		bool verify(const CipherTextG1& c, const ZkpBin& zkp) const
		{
			const MulG<G1> xPmul(xP_);
			return verifyZkpBin(c.S_, c.T_, P_, zkp, PhashTbl_.getWM(), xPmul);
		}
		bool verify(const CipherTextG1& c, int64_t m, const ZkpDec& zkp) const
		{
			/*
				Enc(m;r) - Enc(m;0) = (S, T) - (mP, 0) = (S - mP, T)
			*/
			const Fr& d = zkp.d_[0];
			const Fr& h = zkp.d_[1];
			const G1& P1 = P_;
			const G1& P2 = c.T_; // rP
			const G1& A1 = xP_;
			G1 A2;
			G1::mul(A2, P_, m);
//			PhashTbl_.getWM().mul(A2, m);
			G1::sub(A2, c.S_, A2); // S - mP = xrP
			G1 B1, B2, T;
			G1::mul(B1, P1, d);
			G1::mul(B2, P2, d);
			G1::mul(T, A1, h);
			B1 -= T;
			G1::mul(T, A2, h);
			B2 -= T;
			Fr h2;
			local::Hash hash;
			hash << P2 << A1 << A2 << B1 << B2;
			hash.get(h2);
			return h == h2;
		}
		bool verify(const CipherTextG2& c, const ZkpBin& zkp) const
		{
			const MulG<G2> yQmul(yQ_);
			return verifyZkpBin(c.S_, c.T_, Q_, zkp, QhashTbl_.getWM(), yQmul);
		}
		bool verify(const CipherTextG1& c, const Fr *zkp, const int *mVec, size_t mSize) const
		{
			const MulG<G1> xPmul(xP_);
			return verifyZkpSet(P_, c.S_, c.T_, zkp, mVec, mSize, PhashTbl_.getWM(), xPmul);
		}
		template<class INT>
		void encWithZkpEq(CipherTextG1& c1, CipherTextG2& c2, ZkpEq& zkp, const INT& m) const
		{
			const MulG<G1> xPmul(xP_);
			const MulG<G2> yQmul(yQ_);
			makeZkpEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, m, PhashTbl_.getWM(), xPmul, QhashTbl_.getWM(), yQmul);
		}
		bool verify(const CipherTextG1& c1, const CipherTextG2& c2, const ZkpEq& zkp) const
		{
			const MulG<G1> xPmul(xP_);
			const MulG<G2> yQmul(yQ_);
			return verifyZkpEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, PhashTbl_.getWM(), xPmul, QhashTbl_.getWM(), yQmul);
		}
		void encWithZkpBinEq(CipherTextG1& c1, CipherTextG2& c2, ZkpBinEq& zkp, int m) const
		{
			const MulG<G1> xPmul(xP_);
			const MulG<G2> yQmul(yQ_);
			makeZkpBinEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, m, PhashTbl_.getWM(), xPmul, QhashTbl_.getWM(), yQmul);
		}
		bool verify(const CipherTextG1& c1, const CipherTextG2& c2, const ZkpBinEq& zkp) const
		{
			const MulG<G1> xPmul(xP_);
			const MulG<G2> yQmul(yQ_);
			return verifyZkpBinEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, PhashTbl_.getWM(), xPmul, QhashTbl_.getWM(), yQmul);
		}
		template<class INT>
		void encGT(CipherTextGT& c, const INT& m) const
		{
			/*
				(s, t, u, v) = ((e^x)^a (e^y)^b (e^-xy)^c e^m, e^b, e^a, e^c)
				s = e(a xP + m P, Q)e(b P - c xP, yQ)
			*/
			Fr ra, rb, rc;
			ra.setRand();
			rb.setRand();
			rc.setRand();
			GT e;

			G1 P1, P2;
			G1::mul(P1, xP_, ra);
			if (m != 0) {
//				G1::mul(P2, P, m);
				PhashTbl_.mulByWindowMethod(P2, m);
				P1 += P2;
			}
//			millerLoop(c.g[0], P1, Q);
			precomputedMillerLoop(c.g_[0], P1, Qcoeff_);
//			G1::mul(P1, P, rb);
			PhashTbl_.mulByWindowMethod(P1, rb);
			G1::mul(P2, xP_, rc);
			P1 -= P2;
			millerLoop(e, P1, yQ_);
			c.g_[0] *= e;
			finalExp(c.g_[0], c.g_[0]);
#if 1
			ePQhashTbl_.mulByWindowMethod(c.g_[1], rb);
			ePQhashTbl_.mulByWindowMethod(c.g_[2], ra);
			ePQhashTbl_.mulByWindowMethod(c.g_[3], rc);
#else
			GT::pow(c.g_[1], ePQ_, rb);
			GT::pow(c.g_[2], ePQ_, ra);
			GT::pow(c.g_[3], ePQ_, rc);
#endif
		}
	public:
		template<class InputStream>
		void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
		{
			xP_.load(pb, is, ioMode); if (!*pb) return;
			if (!isG1only_) yQ_.load(pb, is, ioMode);
		}
		template<class OutputStream>
		void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			xP_.save(pb, os, ioMode); if (!*pb) return;
			if (isG1only_) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			yQ_.save(pb, os, ioMode);
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			bool b;
			load(&b, is, ioMode);
			if (!b) throw cybozu::Exception("she:PublicKey:load");
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			bool b;
			save(&b, os, ioMode);
			if (!b) throw cybozu::Exception("she:PublicKey:save");
		}
		friend std::istream& operator>>(std::istream& is, PublicKey& self)
		{
			self.load(is, fp::detectIoMode(G1::getIoMode(), is));
			return is;
		}
		friend std::ostream& operator<<(std::ostream& os, const PublicKey& self)
		{
			self.save(os, fp::detectIoMode(G1::getIoMode(), os));
			return os;
		}
		bool operator==(const PublicKey& rhs) const
		{
			return xP_ == rhs.xP_ && (isG1only_ || yQ_ == rhs.yQ_);
		}
		bool operator!=(const PublicKey& rhs) const { return !operator==(rhs); }
	};

	class PrecomputedPublicKey : public fp::Serializable<PrecomputedPublicKey,
		PublicKeyMethod<PrecomputedPublicKey> > {
		typedef local::InterfaceForHashTable<GT, false> GTasEC;
		typedef mcl::fp::WindowMethod<GTasEC> GTwin;
		template<class T>
		friend struct PublicKeyMethod;
		GT exPQ_;
		GT eyPQ_;
		GT exyPQ_;
		GTwin exPQwm_;
		GTwin eyPQwm_;
		GTwin exyPQwm_;
		mcl::fp::WindowMethod<G1> xPwm_;
		mcl::fp::WindowMethod<G2> yQwm_;
		template<class T>
		void mulByWindowMethod(GT& x, const GTwin& wm, const T& y) const
		{
			wm.mul(static_cast<GTasEC&>(x), y);
		}
		template<class INT>
		void encG1(CipherTextG1& c, const INT& m) const
		{
			ElGamalEnc(c.S_, c.T_, m, PhashTbl_.getWM(), xPwm_);
		}
		template<class INT>
		void encG2(CipherTextG2& c, const INT& m) const
		{
			ElGamalEnc(c.S_, c.T_, m, QhashTbl_.getWM(), yQwm_);
		}
		template<class INT>
		void encGT(CipherTextGT& c, const INT& m) const
		{
			/*
				(s, t, u, v) = (e^m e^(xya), (e^x)^b, (e^y)^c, e^(b + c - a))
			*/
			Fr ra, rb, rc;
			ra.setRand();
			rb.setRand();
			rc.setRand();
			GT t;
			ePQhashTbl_.mulByWindowMethod(c.g_[0], m); // e^m
			mulByWindowMethod(t, exyPQwm_, ra); // (e^xy)^a
			c.g_[0] *= t;
			mulByWindowMethod(c.g_[1], exPQwm_, rb); // (e^x)^b
			mulByWindowMethod(c.g_[2], eyPQwm_, rc); // (e^y)^c
			rb += rc;
			rb -= ra;
			ePQhashTbl_.mulByWindowMethod(c.g_[3], rb);
		}
	public:
		void init(const PublicKey& pub)
		{
			const size_t bitSize = Fr::getBitSize();
			xPwm_.init(pub.xP_, bitSize, local::winSize);
			if (isG1only_) return;
			yQwm_.init(pub.yQ_, bitSize, local::winSize);
			pairing(exPQ_, pub.xP_, Q_);
			pairing(eyPQ_, P_, pub.yQ_);
			pairing(exyPQ_, pub.xP_, pub.yQ_);
			exPQwm_.init(static_cast<const GTasEC&>(exPQ_), bitSize, local::winSize);
			eyPQwm_.init(static_cast<const GTasEC&>(eyPQ_), bitSize, local::winSize);
			exyPQwm_.init(static_cast<const GTasEC&>(exyPQ_), bitSize, local::winSize);
		}
		void encWithZkpBin(CipherTextG1& c, ZkpBin& zkp, int m) const
		{
			Fr encRand;
			encRand.setRand();
			ElGamalEnc(c.S_, c.T_, m, PhashTbl_.getWM(), xPwm_, &encRand);
			makeZkpBin(zkp, c.S_, c.T_, encRand, P_, m,  PhashTbl_.getWM(), xPwm_);
		}
		void encWithZkpBin(CipherTextG2& c, ZkpBin& zkp, int m) const
		{
			Fr encRand;
			encRand.setRand();
			ElGamalEnc(c.S_, c.T_, m, QhashTbl_.getWM(), yQwm_, &encRand);
			makeZkpBin(zkp, c.S_, c.T_, encRand, Q_, m,  QhashTbl_.getWM(), yQwm_);
		}
		void encWithZkpSet(CipherTextG1& c, Fr *zkp, int m, const int *mVec, size_t mSize) const
		{
			Fr encRand;
			encRand.setRand();
			ElGamalEnc(c.S_, c.T_, m, PhashTbl_.getWM(), xPwm_, &encRand);
			if (!makeZkpSet(zkp, xPwm_.tbl_[1], c.S_, c.T_, encRand, m,  mVec, mSize, PhashTbl_.getWM(), xPwm_)) {
				throw cybozu::Exception("encWithZkpSet:bad mVec") << mSize;
			}
		}
		bool verify(const CipherTextG1& c, const ZkpBin& zkp) const
		{
			return verifyZkpBin(c.S_, c.T_, P_, zkp, PhashTbl_.getWM(), xPwm_);
		}
		bool verify(const CipherTextG2& c, const ZkpBin& zkp) const
		{
			return verifyZkpBin(c.S_, c.T_, Q_, zkp, QhashTbl_.getWM(), yQwm_);
		}
		bool verify(const CipherTextG1& c, const Fr *zkp, const int *mVec, size_t mSize) const
		{
			return verifyZkpSet(xPwm_.tbl_[1], c.S_, c.T_, zkp, mVec, mSize, PhashTbl_.getWM(), xPwm_);
		}
		template<class INT>
		void encWithZkpEq(CipherTextG1& c1, CipherTextG2& c2, ZkpEq& zkp, const INT& m) const
		{
			makeZkpEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, m, PhashTbl_.getWM(), xPwm_, QhashTbl_.getWM(), yQwm_);
		}
		bool verify(const CipherTextG1& c1, const CipherTextG2& c2, const ZkpEq& zkp) const
		{
			return verifyZkpEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, PhashTbl_.getWM(), xPwm_, QhashTbl_.getWM(), yQwm_);
		}
		void encWithZkpBinEq(CipherTextG1& c1, CipherTextG2& c2, ZkpBinEq& zkp, int m) const
		{
			makeZkpBinEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, m, PhashTbl_.getWM(), xPwm_, QhashTbl_.getWM(), yQwm_);
		}
		bool verify(const CipherTextG1& c1, const CipherTextG2& c2, const ZkpBinEq& zkp) const
		{
			return verifyZkpBinEq(zkp, c1.S_, c1.T_, c2.S_, c2.T_, PhashTbl_.getWM(), xPwm_, QhashTbl_.getWM(), yQwm_);
		}
	};
	class CipherTextA {
		CipherTextG1 c1_;
		CipherTextG2 c2_;
		friend class SecretKey;
		friend class PublicKey;
		friend class CipherTextGT;
		template<class T>
		friend struct PublicKeyMethod;
	public:
		void clear()
		{
			c1_.clear();
			c2_.clear();
		}
		static void add(CipherTextA& z, const CipherTextA& x, const CipherTextA& y)
		{
			CipherTextG1::add(z.c1_, x.c1_, y.c1_);
			CipherTextG2::add(z.c2_, x.c2_, y.c2_);
		}
		static void sub(CipherTextA& z, const CipherTextA& x, const CipherTextA& y)
		{
			CipherTextG1::sub(z.c1_, x.c1_, y.c1_);
			CipherTextG2::sub(z.c2_, x.c2_, y.c2_);
		}
		static void mul(CipherTextA& z, const CipherTextA& x, int64_t y)
		{
			CipherTextG1::mul(z.c1_, x.c1_, y);
			CipherTextG2::mul(z.c2_, x.c2_, y);
		}
		static void neg(CipherTextA& y, const CipherTextA& x)
		{
			CipherTextG1::neg(y.c1_, x.c1_);
			CipherTextG2::neg(y.c2_, x.c2_);
		}
		void add(const CipherTextA& c) { add(*this, *this, c); }
		void sub(const CipherTextA& c) { sub(*this, *this, c); }
		template<class InputStream>
		void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
		{
			c1_.load(pb, is, ioMode); if (!*pb) return;
			c2_.load(pb, is, ioMode);
		}
		template<class OutputStream>
		void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			c1_.save(pb, os, ioMode); if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			c2_.save(pb, os, ioMode);
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			bool b;
			load(&b, is, ioMode);
			if (!b) throw cybozu::Exception("she:CipherTextA:load");
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			bool b;
			save(&b, os, ioMode);
			if (!b) throw cybozu::Exception("she:CipherTextA:save");
		}
		friend std::istream& operator>>(std::istream& is, CipherTextA& self)
		{
			self.load(is, fp::detectIoMode(G1::getIoMode(), is));
			return is;
		}
		friend std::ostream& operator<<(std::ostream& os, const CipherTextA& self)
		{
			self.save(os, fp::detectIoMode(G1::getIoMode(), os));
			return os;
		}
		bool operator==(const CipherTextA& rhs) const
		{
			return c1_ == rhs.c1_ && c2_ == rhs.c2_;
		}
		bool operator!=(const CipherTextA& rhs) const { return !operator==(rhs); }
	};

	class CipherTextGT : public fp::Serializable<CipherTextGT> {
		GT g_[4];
		friend class SecretKey;
		friend class PublicKey;
		friend class PrecomputedPublicKey;
		friend class CipherTextA;
		friend struct AuxiliaryForZkpDecGT;
		template<class T>
		friend struct PublicKeyMethod;
	public:
		void clear()
		{
			for (int i = 0; i < 4; i++) {
				g_[i].setOne();
			}
		}
		static void neg(CipherTextGT& y, const CipherTextGT& x)
		{
			for (int i = 0; i < 4; i++) {
				GT::unitaryInv(y.g_[i], x.g_[i]);
			}
		}
		static void add(CipherTextGT& z, const CipherTextGT& x, const CipherTextGT& y)
		{
			/*
				(g[i]) + (g'[i]) = (g[i] * g'[i])
			*/
			for (int i = 0; i < 4; i++) {
				GT::mul(z.g_[i], x.g_[i], y.g_[i]);
			}
		}
		static void sub(CipherTextGT& z, const CipherTextGT& x, const CipherTextGT& y)
		{
			/*
				(g[i]) - (g'[i]) = (g[i] / g'[i])
			*/
			GT t;
			for (size_t i = 0; i < 4; i++) {
				GT::unitaryInv(t, y.g_[i]);
				GT::mul(z.g_[i], x.g_[i], t);
			}
		}
		static void mulML(CipherTextGT& z, const CipherTextG1& x, const CipherTextG2& y)
		{
			/*
				(S1, T1) * (S2, T2) = (ML(S1, S2), ML(S1, T2), ML(T1, S2), ML(T1, T2))
			*/
			tensorProductML(z.g_, x.S_, x.T_, y.S_, y.T_);
		}
		static void finalExp(CipherTextGT& y, const CipherTextGT& x)
		{
			finalExp4(y.g_, x.g_);
		}
		/*
			mul(x, y) = mulML(x, y) + finalExp
			mul(c11, c12) + mul(c21, c22)
			= finalExp(mulML(c11, c12) + mulML(c21, c22)),
			then one finalExp can be reduced
		*/
		static void mul(CipherTextGT& z, const CipherTextG1& x, const CipherTextG2& y)
		{
			/*
				(S1, T1) * (S2, T2) = (e(S1, S2), e(S1, T2), e(T1, S2), e(T1, T2))
			*/
			mulML(z, x, y);
			finalExp(z, z);
		}
		static void mul(CipherTextGT& z, const CipherTextA& x, const CipherTextA& y)
		{
			mul(z, x.c1_, y.c2_);
		}
		template<class INT>
		static void mul(CipherTextGT& z, const CipherTextGT& x, const INT& y)
		{
			for (int i = 0; i < 4; i++) {
				GT::pow(z.g_[i], x.g_[i], y);
			}
		}
		void add(const CipherTextGT& c) { add(*this, *this, c); }
		void sub(const CipherTextGT& c) { sub(*this, *this, c); }
		template<class InputStream>
		void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
		{
			for (int i = 0; i < 4; i++) {
				g_[i].load(pb, is, ioMode); if (!*pb) return;
			}
		}
		template<class OutputStream>
		void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			g_[0].save(pb, os, ioMode); if (!*pb) return;
			for (int i = 1; i < 4; i++) {
				if (sep) {
					cybozu::writeChar(pb, os, sep);
					if (!*pb) return;
				}
				g_[i].save(pb, os, ioMode); if (!*pb) return;
			}
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			bool b;
			load(&b, is, ioMode);
			if (!b) throw cybozu::Exception("she:CipherTextGT:load");
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			bool b;
			save(&b, os, ioMode);
			if (!b) throw cybozu::Exception("she:CipherTextGT:save");
		}
		friend std::istream& operator>>(std::istream& is, CipherTextGT& self)
		{
			self.load(is, fp::detectIoMode(G1::getIoMode(), is));
			return is;
		}
		friend std::ostream& operator<<(std::ostream& os, const CipherTextGT& self)
		{
			self.save(os, fp::detectIoMode(G1::getIoMode(), os));
			return os;
		}
		bool operator==(const CipherTextGT& rhs) const
		{
			for (int i = 0; i < 4; i++) {
				if (g_[i] != rhs.g_[i]) return false;
			}
			return true;
		}
		bool operator!=(const CipherTextGT& rhs) const { return !operator==(rhs); }
	};

	class CipherText : public fp::Serializable<CipherText> {
		bool isMultiplied_;
		CipherTextA a_;
		CipherTextGT m_;
		friend class SecretKey;
		friend class PublicKey;
		template<class T>
		friend struct PublicKeyMethod;
	public:
		CipherText() : isMultiplied_(false) {}
		void clearAsAdded()
		{
			isMultiplied_ = false;
			a_.clear();
		}
		void clearAsMultiplied()
		{
			isMultiplied_ = true;
			m_.clear();
		}
		bool isMultiplied() const { return isMultiplied_; }
		static void add(CipherText& z, const CipherText& x, const CipherText& y)
		{
			if (x.isMultiplied() && y.isMultiplied()) {
				z.isMultiplied_ = true;
				CipherTextGT::add(z.m_, x.m_, y.m_);
				return;
			}
			if (!x.isMultiplied() && !y.isMultiplied()) {
				z.isMultiplied_ = false;
				CipherTextA::add(z.a_, x.a_, y.a_);
				return;
			}
			throw cybozu::Exception("she:CipherText:add:mixed CipherText");
		}
		static void sub(CipherText& z, const CipherText& x, const CipherText& y)
		{
			if (x.isMultiplied() && y.isMultiplied()) {
				z.isMultiplied_ = true;
				CipherTextGT::sub(z.m_, x.m_, y.m_);
				return;
			}
			if (!x.isMultiplied() && !y.isMultiplied()) {
				z.isMultiplied_ = false;
				CipherTextA::sub(z.a_, x.a_, y.a_);
				return;
			}
			throw cybozu::Exception("she:CipherText:sub:mixed CipherText");
		}
		static void neg(CipherText& y, const CipherText& x)
		{
			if (x.isMultiplied()) {
				y.isMultiplied_ = true;
				CipherTextGT::neg(y.m_, x.m_);
				return;
			} else {
				y.isMultiplied_ = false;
				CipherTextA::neg(y.a_, x.a_);
				return;
			}
		}
		static void mul(CipherText& z, const CipherText& x, const CipherText& y)
		{
			if (x.isMultiplied() || y.isMultiplied()) {
				throw cybozu::Exception("she:CipherText:mul:mixed CipherText");
			}
			z.isMultiplied_ = true;
			CipherTextGT::mul(z.m_, x.a_, y.a_);
		}
		static void mul(CipherText& z, const CipherText& x, int64_t y)
		{
			if (x.isMultiplied()) {
				CipherTextGT::mul(z.m_, x.m_, y);
			} else {
				CipherTextA::mul(z.a_, x.a_, y);
			}
		}
		void add(const CipherText& c) { add(*this, *this, c); }
		void sub(const CipherText& c) { sub(*this, *this, c); }
		void mul(const CipherText& c) { mul(*this, *this, c); }
		template<class InputStream>
		void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
		{
			char c;
			if (!cybozu::readChar(&c, is)) return;
			if (c == '0' || c == '1') {
				isMultiplied_ = c == '0';
			} else {
				*pb = false;
				return;
			}
			if (isMultiplied()) {
				m_.load(pb, is, ioMode);
			} else {
				a_.load(pb, is, ioMode);
			}
		}
		template<class OutputStream>
		void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
		{
			cybozu::writeChar(pb, os, isMultiplied_ ? '0' : '1'); if (!*pb) return;
			if (isMultiplied()) {
				m_.save(pb, os, ioMode);
			} else {
				a_.save(pb, os, ioMode);
			}
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			bool b;
			load(&b, is, ioMode);
			if (!b) throw cybozu::Exception("she:CipherText:load");
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			bool b;
			save(&b, os, ioMode);
			if (!b) throw cybozu::Exception("she:CipherText:save");
		}
		friend std::istream& operator>>(std::istream& is, CipherText& self)
		{
			self.load(is, fp::detectIoMode(G1::getIoMode(), is));
			return is;
		}
		friend std::ostream& operator<<(std::ostream& os, const CipherText& self)
		{
			self.save(os, fp::detectIoMode(G1::getIoMode(), os));
			return os;
		}
		bool operator==(const CipherTextGT& rhs) const
		{
			if (isMultiplied() != rhs.isMultiplied()) return false;
			if (isMultiplied()) {
				return m_ == rhs.m_;
			}
			return a_ == rhs.a_;
		}
		bool operator!=(const CipherTextGT& rhs) const { return !operator==(rhs); }
	};
};
typedef local::HashTable<G1> HashTableG1;
typedef local::HashTable<G2> HashTableG2;
typedef local::HashTable<Fp12, false> HashTableGT;

template<size_t dummyInpl> G1 SHET<dummyInpl>::P_;
template<size_t dummyInpl> G2 SHET<dummyInpl>::Q_;
template<size_t dummyInpl> Fp12 SHET<dummyInpl>::ePQ_;
template<size_t dummyInpl> std::vector<Fp6> SHET<dummyInpl>::Qcoeff_;
template<size_t dummyInpl> HashTableG1 SHET<dummyInpl>::PhashTbl_;
template<size_t dummyInpl> HashTableG2 SHET<dummyInpl>::QhashTbl_;
template<size_t dummyInpl> HashTableGT SHET<dummyInpl>::ePQhashTbl_;
template<size_t dummyInpl> bool SHET<dummyInpl>::useDecG1ViaGT_;
template<size_t dummyInpl> bool SHET<dummyInpl>::useDecG2ViaGT_;
template<size_t dummyInpl> bool SHET<dummyInpl>::isG1only_;
typedef mcl::she::SHET<> SHE;
typedef SHE::SecretKey SecretKey;
typedef SHE::PublicKey PublicKey;
typedef SHE::PrecomputedPublicKey PrecomputedPublicKey;
typedef SHE::CipherTextG1 CipherTextG1;
typedef SHE::CipherTextG2 CipherTextG2;
typedef SHE::CipherTextGT CipherTextGT;
typedef SHE::CipherTextA CipherTextA;
typedef CipherTextGT CipherTextGM; // old class
typedef SHE::CipherText CipherText;
typedef SHE::ZkpBin ZkpBin;
typedef SHE::ZkpEq ZkpEq;
typedef SHE::ZkpBinEq ZkpBinEq;
typedef SHE::ZkpDec ZkpDec;
typedef SHE::AuxiliaryForZkpDecGT AuxiliaryForZkpDecGT;
typedef SHE::ZkpDecGT ZkpDecGT;

inline void init(const mcl::CurveParam& cp = mcl::BN254, size_t hashSize = local::defaultHashSize, size_t tryNum = local::defaultTryNum)
{
	SHE::init(cp, hashSize, tryNum);
}
inline void initG1only(int curveType, size_t hashSize = local::defaultHashSize, size_t tryNum = local::defaultTryNum)
{
	SHE::initG1only(curveType, hashSize, tryNum);
}
inline void initG1only(const mcl::EcParam& para, size_t hashSize = local::defaultHashSize, size_t tryNum = local::defaultTryNum)
{
	initG1only(para.curveType, hashSize, tryNum);
}
inline void setRangeForG1DLP(size_t hashSize) { SHE::setRangeForG1DLP(hashSize); }
inline void setRangeForG2DLP(size_t hashSize) { SHE::setRangeForG2DLP(hashSize); }
inline void setRangeForGTDLP(size_t hashSize) { SHE::setRangeForGTDLP(hashSize); }
inline void setRangeForDLP(size_t hashSize) { SHE::setRangeForDLP(hashSize); }
inline void setTryNum(size_t tryNum) { SHE::setTryNum(tryNum); }
inline void useDecG1ViaGT(bool use = true) { SHE::useDecG1ViaGT(use); }
inline void useDecG2ViaGT(bool use = true) { SHE::useDecG2ViaGT(use); }
inline HashTableG1& getHashTableG1() { return SHE::PhashTbl_; }
inline HashTableG2& getHashTableG2() { return SHE::QhashTbl_; }
inline HashTableGT& getHashTableGT() { return SHE::ePQhashTbl_; }

inline void add(CipherTextG1& z, const CipherTextG1& x, const CipherTextG1& y) { CipherTextG1::add(z, x, y); }
inline void add(CipherTextG2& z, const CipherTextG2& x, const CipherTextG2& y) { CipherTextG2::add(z, x, y); }
inline void add(CipherTextGT& z, const CipherTextGT& x, const CipherTextGT& y) { CipherTextGT::add(z, x, y); }
inline void add(CipherText& z, const CipherText& x, const CipherText& y) { CipherText::add(z, x, y); }

inline void sub(CipherTextG1& z, const CipherTextG1& x, const CipherTextG1& y) { CipherTextG1::sub(z, x, y); }
inline void sub(CipherTextG2& z, const CipherTextG2& x, const CipherTextG2& y) { CipherTextG2::sub(z, x, y); }
inline void sub(CipherTextGT& z, const CipherTextGT& x, const CipherTextGT& y) { CipherTextGT::sub(z, x, y); }
inline void sub(CipherText& z, const CipherText& x, const CipherText& y) { CipherText::sub(z, x, y); }

inline void neg(CipherTextG1& y, const CipherTextG1& x) { CipherTextG1::neg(y, x); }
inline void neg(CipherTextG2& y, const CipherTextG2& x) { CipherTextG2::neg(y, x); }
inline void neg(CipherTextGT& y, const CipherTextGT& x) { CipherTextGT::neg(y, x); }
inline void neg(CipherText& y, const CipherText& x) { CipherText::neg(y, x); }

template<class INT>
inline void mul(CipherTextG1& z, const CipherTextG1& x, const INT& y) { CipherTextG1::mul(z, x, y); }
template<class INT>
inline void mul(CipherTextG2& z, const CipherTextG2& x, const INT& y) { CipherTextG2::mul(z, x, y); }
template<class INT>
inline void mul(CipherTextGT& z, const CipherTextGT& x, const INT& y) { CipherTextGT::mul(z, x, y); }
template<class INT>
inline void mul(CipherText& z, const CipherText& x, const INT& y) { CipherText::mul(z, x, y); }

inline void mul(CipherTextGT& z, const CipherTextG1& x, const CipherTextG2& y) { CipherTextGT::mul(z, x, y); }
inline void mul(CipherText& z, const CipherText& x, const CipherText& y) { CipherText::mul(z, x, y); }

} } // mcl::she

