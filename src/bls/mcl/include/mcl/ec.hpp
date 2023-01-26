#pragma once
/**
	@file
	@brief elliptic curve
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdlib.h>
#include <mcl/fp.hpp>
#include <mcl/ecparam.hpp>
#include <mcl/window_method.hpp>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4458)
#endif
#ifdef MCL_USE_OMP
#include <omp.h>
#endif

namespace mcl {

template<class _Fp> class Fp2T;

namespace local {

template<class Ec, class Vec>
void addTbl(Ec& Q, const Ec *tbl, const Vec& naf, size_t i)
{
	if (i >= naf.size()) return;
	int n = naf[i];
	if (n > 0) {
		Q += tbl[(n - 1) >> 1];
	} else if (n < 0) {
		Q -= tbl[(-n - 1) >> 1];
	}
}

} // mcl::local

namespace ec {

enum Mode {
	Jacobi = 0,
	Proj = 1,
	Affine
};

enum ModeCoeffA {
	Zero,
	Minus3,
	GenericA
};

namespace local {

/*
	elliptic class E must have
	member variables of type Fp x, y, z
	static member a_, b_, specialA_
*/
// x is negative <=> x < half(:=(p+1)/2) <=> a = 1
template<class F>
bool get_a_flag(const F& x)
{
	return x.isNegative();
}

// Im(x) is negative <=> Im(x)  < half(:=(p+1)/2) <=> a = 1

template<class F>
bool get_a_flag(const mcl::Fp2T<F>& x)
{
	return get_a_flag(x.b); // x = a + bi
}

/*
	Q = x P
	splitN = 2(G1) or 4(G2)
	w : window size
*/
template<class GLV, class G, class F, int splitN, size_t w>
void mul1CT(G& Q, const G& P, const mpz_class& x)
{
	const mpz_class& r = F::getOp().mp;
	const size_t tblSize = 1 << w;
	G tbl[splitN][tblSize];
	bool negTbl[splitN];
	mpz_class u[splitN];
	mpz_class y;
	F::getOp().modp.modp(y, x);
	if (y < 0) {
		y += r;
	}
	GLV::split(u, y);
	for (int i = 0; i < splitN; i++) {
		if (u[i] < 0) {
			gmp::neg(u[i], u[i]);
			negTbl[i] = true;
		} else {
			negTbl[i] = false;
		}
		tbl[i][0].clear();
	}
	tbl[0][1] = P;
	for (size_t j = 2; j < tblSize; j++) {
		G::add(tbl[0][j], tbl[0][j - 1], P);
	}
	for (int i = 1; i < splitN; i++) {
		for (size_t j = 1; j < tblSize; j++) {
			GLV::mulLambda(tbl[i][j], tbl[i - 1][j]);
		}
	}
	for (int i = 0; i < splitN; i++) {
		if (negTbl[i]) {
			for (size_t j = 0; j < tblSize; j++) {
				G::neg(tbl[i][j], tbl[i][j]);
			}
		}
	}
	mcl::FixedArray<uint8_t, sizeof(F) * 8 / w + 1> vTbl[splitN];
	size_t loopN = 0;
	{
		size_t maxBitSize = 0;
		fp::BitIterator<fp::Unit> itr[splitN];
		for (int i = 0; i < splitN; i++) {
			itr[i].init(gmp::getUnit(u[i]), gmp::getUnitSize(u[i]));
			size_t bitSize = itr[i].getBitSize();
			if (bitSize > maxBitSize) maxBitSize = bitSize;
		}
		loopN = (maxBitSize + w - 1) / w;
		for (int i = 0; i < splitN; i++) {
			bool b = vTbl[i].resize(loopN);
			assert(b);
			(void)b;
			for (size_t j = 0; j < loopN; j++) {
				vTbl[i][loopN - 1 - j] = (uint8_t)itr[i].getNext(w);
			}
		}
	}
	Q.clear();
	for (size_t k = 0; k < loopN; k++) {
		for (size_t i = 0; i < w; i++) {
			G::dbl(Q, Q);
		}
		for (size_t i = 0; i < splitN; i++) {
			uint8_t v = vTbl[i][k];
			G::add(Q, Q, tbl[i][v]);
		}
	}
}

/*
	z += xVec[i] * yVec[i] for i = 0, ..., min(N, n)
	splitN = 2(G1) or 4(G2)
	w : window size
*/
template<class GLV, class G, class F, int splitN, int w, size_t N>
static size_t mulVecNGLVT(G& z, const G *xVec, const mpz_class *yVec, size_t n)
{
	const mpz_class& r = F::getOp().mp;
	const size_t tblSize = 1 << (w - 2);
	typedef mcl::FixedArray<int8_t, sizeof(F) * 8 / splitN + splitN> NafArray;
	NafArray naf[N][splitN];
	G tbl[N][splitN][tblSize];
	bool b;
	mpz_class u[splitN], y;
	size_t maxBit = 0;

	if (n > N) n = N;
	for (size_t i = 0; i < n; i++) {
		y = yVec[i];
		y %= r;
		if (y < 0) {
			y += r;
		}
		GLV::split(u, y);

		for (int j = 0; j < splitN; j++) {
			gmp::getNAFwidth(&b, naf[i][j], u[j], w);
			assert(b); (void)b;
			if (naf[i][j].size() > maxBit) maxBit = naf[i][j].size();
		}

		G P2;
		G::dbl(P2, xVec[i]);
		tbl[i][0][0] = xVec[i];
		for (int k = 1; k < splitN; k++) {
			GLV::mulLambda(tbl[i][k][0], tbl[i][k - 1][0]);
		}
		for (size_t j = 1; j < tblSize; j++) {
			G::add(tbl[i][0][j], tbl[i][0][j - 1], P2);
			for (int k = 1; k < splitN; k++) {
				GLV::mulLambda(tbl[i][k][j], tbl[i][k - 1][j]);
			}
		}
	}
	z.clear();
	for (size_t i = 0; i < maxBit; i++) {
		const size_t bit = maxBit - 1 - i;
		G::dbl(z, z);
		for (size_t j = 0; j < n; j++) {
			for (int k = 0; k < splitN; k++) {
				mcl::local::addTbl(z, tbl[j][k], naf[j][k], bit);
			}
		}
	}
	return n;
}

} // mcl::ec::local

template<class E>
void normalizeJacobi(E& P)
{
	typedef typename E::Fp F;
	if (P.z.isZero()) return;
	F::inv(P.z, P.z);
	F rz2;
	F::sqr(rz2, P.z);
	P.x *= rz2;
	P.y *= rz2;
	P.y *= P.z;
	P.z = 1;
}

template<class E>
void normalizeVecJacobi(E *Q, const E *P, size_t n)
{
	typedef typename E::Fp F;
	F *inv = (F*)CYBOZU_ALLOCA(sizeof(F) * n);
	for (size_t i = 0; i < n; i++) {
		inv[i] = P[i].z;
	}
	F::invVec(inv, inv, n);
	for (size_t i = 0; i < n; i++) {
		if (inv[i].isZero()) {
			Q[i].clear();
		} else {
			F rz2;
			F::sqr(rz2, inv[i]);
			F::mul(Q[i].x, P[i].x, rz2);
			F::mul(Q[i].y, P[i].y, rz2);
			Q[i].y *= inv[i];
			Q[i].z = 1;
		}
	}
}

// (x/z^2, y/z^3)
template<class E>
bool isEqualJacobi(const E& P1, const E& P2)
{
	typedef typename E::Fp F;
	bool zero1 = P1.isZero();
	bool zero2 = P2.isZero();
	if (zero1) {
		return zero2;
	}
	if (zero2) return false;
	F s1, s2, t1, t2;
	F::sqr(s1, P1.z);
	F::sqr(s2, P2.z);
	F::mul(t1, P1.x, s2);
	F::mul(t2, P2.x, s1);
	if (t1 != t2) return false;
	F::mul(t1, P1.y, s2);
	F::mul(t2, P2.y, s1);
	t1 *= P2.z;
	t2 *= P1.z;
	return t1 == t2;
}

// Y^2 == X(X^2 + aZ^4) + bZ^6
template<class E>
bool isValidJacobi(const E& P)
{
	typedef typename E::Fp F;
	F y2, x2, z2, z4, t;
	F::sqr(x2, P.x);
	F::sqr(y2, P.y);
	F::sqr(z2, P.z);
	F::sqr(z4, z2);
	F::mul(t, z4, E::a_);
	t += x2;
	t *= P.x;
	z4 *= z2;
	z4 *= E::b_;
	t += z4;
	return y2 == t;
}

/*
	M > S + A
	a = 0   2M + 5S + 14A
	a = -3  2M + 7S + 15A
	generic 3M + 7S + 15A
	M == S
	a = 0   3M + 4S + 13A
	a = -3  3M + 6S + 14A
	generic 4M + 6S + 14A
*/
template<class E>
void dblJacobi(E& R, const E& P)
{
	typedef typename E::Fp F;
	if (P.isZero()) {
		R.clear();
		return;
	}
	const bool isPzOne = P.z.isOne();
	F x2, y2, xy, t;
	F::sqr(x2, P.x);
	F::sqr(y2, P.y);
	if (sizeof(F) <= 32) { // M == S
		F::mul(xy, P.x, y2);
		xy += xy;
		F::sqr(y2, y2);
	} else { // M > S + A
		F::add(xy, P.x, y2);
		F::sqr(y2, y2);
		F::sqr(xy, xy);
		xy -= x2;
		xy -= y2;
	}
	xy += xy; // 4xy^2
	switch (E::specialA_) {
	case Zero:
		F::mul2(t, x2);
		x2 += t;
		break;
	case Minus3:
		if (isPzOne) {
			x2 -= P.z;
		} else {
			F::sqr(t, P.z);
			F::sqr(t, t);
			x2 -= t;
		}
		F::mul2(t, x2);
		x2 += t;
		break;
	case GenericA:
	default:
		if (isPzOne) {
			t = E::a_;
		} else {
			F::sqr(t, P.z);
			F::sqr(t, t);
			t *= E::a_;
		}
		t += x2;
		F::mul2(x2, x2);
		x2 += t;
		break;
	}
	F::sqr(R.x, x2);
	R.x -= xy;
	R.x -= xy;
	if (isPzOne) {
		R.z = P.y;
	} else {
		F::mul(R.z, P.y, P.z);
	}
	F::mul2(R.z, R.z);
	F::sub(R.y, xy, R.x);
	R.y *= x2;
	F::mul2(y2, y2);
	F::mul2(y2, y2);
	F::mul2(y2, y2);
	R.y -= y2;
}

// 7M + 4S + 7A
template<class E>
void addJacobi(E& R, const E& P, const E& Q)
{
	typedef typename E::Fp F;
	if (P.isZero()) { R = Q; return; }
	if (Q.isZero()) { R = P; return; }
	bool isPzOne = P.z.isOne();
	bool isQzOne = Q.z.isOne();
	F r, U1, S1, H, H3;
	if (isPzOne) {
		// r = 1;
	} else {
		F::sqr(r, P.z);
	}
	if (isQzOne) {
		U1 = P.x;
		if (isPzOne) {
			H = Q.x;
		} else {
			F::mul(H, Q.x, r);
		}
		H -= U1;
		S1 = P.y;
	} else {
		F::sqr(S1, Q.z);
		F::mul(U1, P.x, S1);
		if (isPzOne) {
			H = Q.x;
		} else {
			F::mul(H, Q.x, r);
		}
		H -= U1;
		S1 *= Q.z;
		S1 *= P.y;
	}
	if (isPzOne) {
		r = Q.y;
	} else {
		r *= P.z;
		r *= Q.y;
	}
	r -= S1;
	if (H.isZero()) {
		if (r.isZero()) {
			ec::dblJacobi(R, P);
		} else {
			R.clear();
		}
		return;
	}
	if (isPzOne) {
		if (isQzOne) {
			R.z = H;
		} else {
			F::mul(R.z, H, Q.z);
		}
	} else {
		if (isQzOne) {
			F::mul(R.z, P.z, H);
		} else {
			F::mul(R.z, P.z, Q.z);
			R.z *= H;
		}
	}
	F::sqr(H3, H); // H^2
	F::sqr(R.y, r); // r^2
	U1 *= H3; // U1 H^2
	H3 *= H; // H^3
	R.y -= U1;
	R.y -= U1;
	F::sub(R.x, R.y, H3);
	U1 -= R.x;
	U1 *= r;
	H3 *= S1;
	F::sub(R.y, U1, H3);
}

/*
	accept P == Q
	https://github.com/apache/incubator-milagro-crypto-c/blob/fa0a45a3/src/ecp.c.in#L767-L976
	(x, y, z) is zero <=> x = 0, y = 1, z = 0
*/
template<class E>
void addCTProj(E& R, const E& P, const E& Q)
{
	typedef typename E::Fp F;
	assert(E::a_ == 0);
	F b3;
	F::add(b3, E::b_, E::b_);
	b3 += E::b_;
	F t0, t1, t2, t3, t4, x3, y3, z3;
	F::mul(t0, P.x, Q.x);
	F::mul(t1, P.y, Q.y);
	F::mul(t2, P.z, Q.z);
	F::add(t3, P.x, P.y);
	F::add(t4, Q.x, Q.y);
	F::mul(t3, t3, t4);
	F::add(t4, t0, t1);
	F::sub(t3, t3, t4);
	F::add(t4, P.y, P.z);
	F::add(x3, Q.y, Q.z);
	F::mul(t4, t4, x3);
	F::add(x3, t1, t2);
	F::sub(t4, t4, x3);
	F::add(x3, P.x, P.z);
	F::add(y3, Q.x, Q.z);
	F::mul(x3, x3, y3);
	F::add(y3, t0, t2);
	F::sub(y3, x3, y3);
	F::add(x3, t0, t0);
	F::add(t0, t0, x3);
	t2 *= b3;
	F::add(z3, t1, t2);
	F::sub(t1, t1, t2);
	y3 *= b3;
	F::mul(x3, y3, t4);
	F::mul(t2, t3, t1);
	F::sub(R.x, t2, x3);
	F::mul(y3, y3, t0);
	F::mul(t1, t1, z3);
	F::add(R.y, y3, t1);
	F::mul(t0, t0, t3);
	F::mul(z3, z3, t4);
	F::add(R.z, z3, t0);
}

template<class E>
void normalizeProj(E& P)
{
	typedef typename E::Fp F;
	if (P.z.isZero()) return;
	F::inv(P.z, P.z);
	P.x *= P.z;
	P.y *= P.z;
	P.z = 1;
}

template<class E>
void normalizeVecProj(E *Q, const E *P, size_t n)
{
	typedef typename E::Fp F;
	F *inv = (F*)CYBOZU_ALLOCA(sizeof(F) * n);
	for (size_t i = 0; i < n; i++) {
		inv[i] = P[i].z;
	}
	F::invVec(inv, inv, n);
	for (size_t i = 0; i < n; i++) {
		if (inv[i].isZero()) {
			Q[i].clear();
		} else {
			F::mul(Q[i].x, P[i].x, inv[i]);
			F::mul(Q[i].y, P[i].y, inv[i]);
			Q[i].z = 1;
		}
	}
}

// (Y^2 - bZ^2)Z = X(X^2 + aZ^2)
template<class E>
bool isValidProj(const E& P)
{
	typedef typename E::Fp F;
	F y2, x2, z2, t;
	F::sqr(x2, P.x);
	F::sqr(y2, P.y);
	F::sqr(z2, P.z);
	F::mul(t, E::a_, z2);
	t += x2;
	t *= P.x;
	z2 *= E::b_;
	y2 -= z2;
	y2 *= P.z;
	return y2 == t;
}

// (x/z, y/z)
template<class E>
bool isEqualProj(const E& P1, const E& P2)
{
	typedef typename E::Fp F;
	bool zero1 = P1.isZero();
	bool zero2 = P2.isZero();
	if (zero1) {
		return zero2;
	}
	if (zero2) return false;
	F t1, t2;
	F::mul(t1, P1.x, P2.z);
	F::mul(t2, P2.x, P1.z);
	if (t1 != t2) return false;
	F::mul(t1, P1.y, P2.z);
	F::mul(t2, P2.y, P1.z);
	return t1 == t2;
}

/*
	   |a=0|-3| generic
	mul|  8| 8| 9
	sqr|  4| 5| 5
	add| 11|12|12
*/
template<class E>
void dblProj(E& R, const E& P)
{
	typedef typename E::Fp F;
	if (P.isZero()) {
		R.clear();
		return;
	}
	const bool isPzOne = P.z.isOne();
	F w, t, h;
	switch (E::specialA_) {
	case Zero:
		F::sqr(w, P.x);
		F::add(t, w, w);
		w += t;
		break;
	case Minus3:
		F::sqr(w, P.x);
		if (isPzOne) {
			w -= P.z;
		} else {
			F::sqr(t, P.z);
			w -= t;
		}
		F::add(t, w, w);
		w += t;
		break;
	case GenericA:
	default:
		if (isPzOne) {
			w = E::a_;
		} else {
			F::sqr(w, P.z);
			w *= E::a_;
		}
		F::sqr(t, P.x);
		w += t;
		w += t;
		w += t; // w = a z^2 + 3x^2
		break;
	}
	if (isPzOne) {
		R.z = P.y;
	} else {
		F::mul(R.z, P.y, P.z); // s = yz
	}
	F::mul(t, R.z, P.x);
	t *= P.y; // xys
	t += t;
	t += t; // 4(xys) ; 4B
	F::sqr(h, w);
	h -= t;
	h -= t; // w^2 - 8B
	F::mul(R.x, h, R.z);
	t -= h; // h is free
	t *= w;
	F::sqr(w, P.y);
	R.x += R.x;
	R.z += R.z;
	F::sqr(h, R.z);
	w *= h;
	R.z *= h;
	F::sub(R.y, t, w);
	R.y -= w;
}

/*
	mul| 12
	sqr|  2
	add|  7
*/
template<class E>
void addProj(E& R, const E& P, const E& Q)
{
	typedef typename E::Fp F;
	if (P.isZero()) { R = Q; return; }
	if (Q.isZero()) { R = P; return; }
	bool isPzOne = P.z.isOne();
	bool isQzOne = Q.z.isOne();
	F r, PyQz, v, A, vv;
	if (isQzOne) {
		r = P.x;
		PyQz = P.y;
	} else {
		F::mul(r, P.x, Q.z);
		F::mul(PyQz, P.y, Q.z);
	}
	if (isPzOne) {
		A = Q.y;
		v = Q.x;
	} else {
		F::mul(A, Q.y, P.z);
		F::mul(v, Q.x, P.z);
	}
	v -= r;
	if (v.isZero()) {
		if (A == PyQz) {
			dblProj(R, P);
		} else {
			R.clear();
		}
		return;
	}
	F::sub(R.y, A, PyQz);
	F::sqr(A, R.y);
	F::sqr(vv, v);
	r *= vv;
	vv *= v;
	if (isQzOne) {
		R.z = P.z;
	} else {
		if (isPzOne) {
			R.z = Q.z;
		} else {
			F::mul(R.z, P.z, Q.z);
		}
	}
	// R.z = 1 if isPzOne && isQzOne
	if (isPzOne && isQzOne) {
		R.z = vv;
	} else {
		A *= R.z;
		R.z *= vv;
	}
	A -= vv;
	vv *= PyQz;
	A -= r;
	A -= r;
	F::mul(R.x, v, A);
	r -= A;
	R.y *= r;
	R.y -= vv;
}

// y^2 == (x^2 + a)x + b
template<class E>
bool isValidAffine(const E& P)
{
	typedef typename E::Fp F;
	assert(!P.z.isZero());
	F y2, t;
	F::sqr(y2, P.y);
	F::sqr(t, P.x);
	t += E::a_;
	t *= P.x;
	t += E::b_;
	return y2 == t;
}

// y^2 = x^3 + ax + b
template<class E>
static inline void dblAffine(E& R, const E& P)
{
	typedef typename E::Fp F;
	if (P.isZero()) {
		R.clear();
		return;
	}
	if (P.y.isZero()) {
		R.clear();
		return;
	}
	F t, s;
	F::sqr(t, P.x);
	F::add(s, t, t);
	t += s;
	t += E::a_;
	F::add(s, P.y, P.y);
	t /= s;
	F::sqr(s, t);
	s -= P.x;
	F x3;
	F::sub(x3, s, P.x);
	F::sub(s, P.x, x3);
	s *= t;
	F::sub(R.y, s, P.y);
	R.x = x3;
	R.z = 1;
}

template<class E>
void addAffine(E& R, const E& P, const E& Q)
{
	typedef typename E::Fp F;
	if (P.isZero()) { R = Q; return; }
	if (Q.isZero()) { R = P; return; }
	F t;
	F::sub(t, Q.x, P.x);
	if (t.isZero()) {
		if (P.y == Q.y) {
			dblAffine(R, P);
		} else {
			R.clear();
		}
		return;
	}
	F s;
	F::sub(s, Q.y, P.y);
	F::div(t, s, t);
	R.z = 1;
	F x3;
	F::sqr(x3, t);
	x3 -= P.x;
	x3 -= Q.x;
	F::sub(s, P.x, x3);
	s *= t;
	F::sub(R.y, s, P.y);
	R.x = x3;
}

template<class E>
void tryAndIncMapTo(E& P, const typename E::Fp& t)
{
	typedef typename E::Fp F;
	F x = t;
	for (;;) {
		F y;
		E::getWeierstrass(y, x);
		if (F::squareRoot(y, y)) {
			bool b;
			P.set(&b, x, y, false);
			assert(b);
			return;
		}
		*x.getFp0() += F::BaseFp::one();
	}
}

} // mcl::ec

/*
	elliptic curve
	y^2 = x^3 + ax + b (affine)
	y^2 = x^3 + az^4 + bz^6 (Jacobi) x = X/Z^2, y = Y/Z^3
*/
template<class _Fp>
class EcT : public fp::Serializable<EcT<_Fp> > {
public:
	typedef _Fp Fp;
	typedef _Fp BaseFp;
	Fp x, y, z;
	static int mode_;
	static Fp a_;
	static Fp b_;
	static int specialA_;
	static int ioMode_;
	/*
		order_ is the order of G2 which is the subgroup of EcT<Fp2>.
		check the order of the elements if verifyOrder_ is true
	*/
	static bool verifyOrder_;
	static mpz_class order_;
	static void (*mulArrayGLV)(EcT& z, const EcT& x, const fp::Unit *y, size_t yn, bool isNegative, bool constTime);
	static size_t (*mulVecNGLV)(EcT& z, const EcT *xVec, const mpz_class *yVec, size_t yn);
	static bool (*isValidOrderFast)(const EcT& x);
	/* default constructor is undefined value */
	EcT() {}
	EcT(const Fp& _x, const Fp& _y)
	{
		set(_x, _y);
	}
	bool isNormalized() const
	{
		return isZero() || z.isOne();
	}
private:
	bool isValidAffine() const
	{
		return ec::isValidAffine(*this);
	}
public:
	void normalize()
	{
		switch (mode_) {
		case ec::Jacobi:
			ec::normalizeJacobi(*this);
			break;
		case ec::Proj:
			ec::normalizeProj(*this);
			break;
		}
	}
	static void normalize(EcT& y, const EcT& x)
	{
		y = x;
		y.normalize();
	}
	static void normalizeVec(EcT *y, const EcT *x, size_t n)
	{
		switch (mode_) {
		case ec::Jacobi:
			ec::normalizeVecJacobi(y, x, n);
			break;
		case ec::Proj:
			ec::normalizeVecProj(y, x, n);
			break;
		case ec::Affine:
			if (y == x) return;
			for (size_t i = 0; i < n; i++) {
				y[i] = x[i];
			}
			break;
		}
	}
	static inline void init(const Fp& a, const Fp& b, int mode = ec::Jacobi)
	{
		a_ = a;
		b_ = b;
		if (a_.isZero()) {
			specialA_ = ec::Zero;
		} else if (a_ == -3) {
			specialA_ = ec::Minus3;
		} else {
			specialA_ = ec::GenericA;
		}
		ioMode_ = 0;
		verifyOrder_ = false;
		order_ = 0;
		mulArrayGLV = 0;
		mulVecNGLV = 0;
		isValidOrderFast = 0;
		mode_ = mode;
	}
	static inline int getMode() { return mode_; }
	/*
		verify the order of *this is equal to order if order != 0
		in constructor, set, setStr, operator<<().
	*/
	static void setOrder(const mpz_class& order)
	{
		if (order != 0) {
			verifyOrder_ = true;
			order_ = order;
		} else {
			verifyOrder_ = false;
			// don't clear order_ because it is used for isValidOrder()
		}
	}
	static void setVerifyOrderFunc(bool f(const EcT&))
	{
		isValidOrderFast = f;
	}
	static void setMulArrayGLV(void f(EcT& z, const EcT& x, const fp::Unit *y, size_t yn, bool isNegative, bool constTime), size_t g(EcT& z, const EcT *xVec, const mpz_class *yVec, size_t yn) = 0)
	{
		mulArrayGLV = f;
		mulVecNGLV = g;
	}
	static inline void init(bool *pb, const char *astr, const char *bstr, int mode = ec::Jacobi)
	{
		Fp a, b;
		a.setStr(pb, astr);
		if (!*pb) return;
		b.setStr(pb, bstr);
		if (!*pb) return;
		init(a, b, mode);
	}
	// verify the order
	bool isValidOrder() const
	{
		if (isValidOrderFast) {
			return isValidOrderFast(*this);
		}
		EcT Q;
		EcT::mulGeneric(Q, *this, order_);
		return Q.isZero();
	}
	bool isValid() const
	{
		switch (mode_) {
		case ec::Jacobi:
			if (!ec::isValidJacobi(*this)) return false;
			break;
		case ec::Proj:
			if (!ec::isValidProj(*this)) return false;
			break;
		case ec::Affine:
			if (z.isZero()) return true;
			if (!isValidAffine()) return false;
			break;
		}
		if (verifyOrder_) return isValidOrder();
		return true;
	}
	void set(bool *pb, const Fp& x, const Fp& y, bool verify = true)
	{
		this->x = x;
		this->y = y;
		z = 1;
		if (!verify || (isValidAffine() && (!verifyOrder_ || isValidOrder()))) {
			*pb = true;
			return;
		}
		*pb = false;
		clear();
	}
	void clear()
	{
		x.clear();
		y.clear();
		z.clear();
	}
	static inline void dbl(EcT& R, const EcT& P)
	{
		switch (mode_) {
		case ec::Jacobi:
			ec::dblJacobi(R, P);
			break;
		case ec::Proj:
			ec::dblProj(R, P);
			break;
		case ec::Affine:
			ec::dblAffine(R, P);
			break;
		}
	}
	static inline void add(EcT& R, const EcT& P, const EcT& Q)
	{
		switch (mode_) {
		case ec::Jacobi:
			ec::addJacobi(R, P, Q);
			break;
		case ec::Proj:
			ec::addProj(R, P, Q);
			break;
		case ec::Affine:
			ec::addAffine(R, P, Q);
			break;
		}
	}
	static inline void sub(EcT& R, const EcT& P, const EcT& Q)
	{
		EcT nQ;
		neg(nQ, Q);
		add(R, P, nQ);
	}
	static inline void neg(EcT& R, const EcT& P)
	{
		if (P.isZero()) {
			R.clear();
			return;
		}
		R.x = P.x;
		Fp::neg(R.y, P.y);
		R.z = P.z;
	}
	template<class tag, size_t maxBitSize, template<class _tag, size_t _maxBitSize>class FpT>
	static inline void mul(EcT& z, const EcT& x, const FpT<tag, maxBitSize>& y)
	{
		fp::Block b;
		y.getBlock(b);
		mulArray(z, x, b.p, b.n, false);
	}
	static inline void mul(EcT& z, const EcT& x, int64_t y)
	{
		const uint64_t u = fp::abs_(y);
#if MCL_SIZEOF_UNIT == 8
		mulArray(z, x, &u, 1, y < 0);
#else
		uint32_t ua[2] = { uint32_t(u), uint32_t(u >> 32) };
		size_t un = ua[1] ? 2 : 1;
		mulArray(z, x, ua, un, y < 0);
#endif
	}
	static inline void mul(EcT& z, const EcT& x, const mpz_class& y)
	{
		mulArray(z, x, gmp::getUnit(y), gmp::getUnitSize(y), y < 0);
	}
	template<class tag, size_t maxBitSize, template<class _tag, size_t _maxBitSize>class FpT>
	static inline void mulCT(EcT& z, const EcT& x, const FpT<tag, maxBitSize>& y)
	{
		fp::Block b;
		y.getBlock(b);
		mulArray(z, x, b.p, b.n, false, true);
	}
	static inline void mulCT(EcT& z, const EcT& x, const mpz_class& y)
	{
		mulArray(z, x, gmp::getUnit(y), gmp::getUnitSize(y), y < 0, true);
	}
	/*
		0 <= P for any P
		(Px, Py) <= (P'x, P'y) iff Px < P'x or Px == P'x and Py <= P'y
		@note compare function calls normalize()
	*/
	template<class F>
	static inline int compareFunc(const EcT& P_, const EcT& Q_, F comp)
	{
		const bool QisZero = Q_.isZero();
		if (P_.isZero()) {
			if (QisZero) return 0;
			return -1;
		}
		if (QisZero) return 1;
		EcT P(P_), Q(Q_);
		P.normalize();
		Q.normalize();
		int c = comp(P.x, Q.x);
		if (c > 0) return 1;
		if (c < 0) return -1;
		return comp(P.y, Q.y);
	}
	static inline int compare(const EcT& P, const EcT& Q)
	{
		return compareFunc(P, Q, Fp::compare);
	}
	static inline int compareRaw(const EcT& P, const EcT& Q)
	{
		return compareFunc(P, Q, Fp::compareRaw);
	}
	bool isZero() const
	{
		return z.isZero();
	}
	static inline bool isMSBserialize()
	{
		return !b_.isZero() && (Fp::BaseFp::getBitSize() & 7) != 0;
	}
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int ioMode) const
	{
		const char sep = *fp::getIoSeparator(ioMode);
		if (ioMode & IoEcProj) {
			cybozu::writeChar(pb, os, '4'); if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			x.save(pb, os, ioMode); if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			y.save(pb, os, ioMode); if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			z.save(pb, os, ioMode);
			return;
		}
		EcT P(*this);
		P.normalize();
		if (ioMode & IoEcAffineSerialize) {
			if (b_ == 0) { // assume Zero if x = y = 0
				*pb = false;
				return;
			}
			if (isZero()) {
				// all zero
				P.z.save(pb, os, IoSerialize);
				if (!*pb) return;
				P.z.save(pb, os, IoSerialize);
				return;
			}
			P.x.save(pb, os, IoSerialize);
			if (!*pb) return;
			P.y.save(pb, os, IoSerialize);
			return;
		}
		if (ioMode & (IoSerialize | IoSerializeHexStr)) {
			const size_t n = Fp::getByteSize();
			const size_t adj = isMSBserialize() ? 0 : 1;
			uint8_t buf[sizeof(Fp) + 1];
			if (Fp::BaseFp::isETHserialization()) {
				const uint8_t c_flag = 0x80;
				const uint8_t b_flag = 0x40;
				const uint8_t a_flag = 0x20;
				if (P.isZero()) {
					buf[0] = c_flag | b_flag;
					memset(buf + 1, 0, n - 1);
				} else {
					cybozu::MemoryOutputStream mos(buf, n);
					P.x.save(pb, mos, IoSerialize); if (!*pb) return;
					uint8_t cba = c_flag;
					if (ec::local::get_a_flag(P.y)) cba |= a_flag;
					buf[0] |= cba;
				}
			} else {
				/*
					if (isMSBserialize()) {
					  // n bytes
					  x | (y.isOdd ? 0x80 : 0)
					} else {
					  // n + 1 bytes
					  (y.isOdd ? 3 : 2), x
					}
				*/
				if (isZero()) {
					memset(buf, 0, n + adj);
				} else {
					cybozu::MemoryOutputStream mos(buf + adj, n);
					P.x.save(pb, mos, IoSerialize); if (!*pb) return;
					if (adj) {
						buf[0] = P.y.isOdd() ? 3 : 2;
					} else {
						if (P.y.isOdd()) {
							buf[n - 1] |= 0x80;
						}
					}
				}
			}
			if (ioMode & IoSerializeHexStr) {
				mcl::fp::writeHexStr(pb, os, buf, n + adj);
			} else {
				cybozu::write(pb, os, buf, n + adj);
			}
			return;
		}
		if (isZero()) {
			cybozu::writeChar(pb, os, '0');
			return;
		}
		if (ioMode & IoEcCompY) {
			cybozu::writeChar(pb, os, P.y.isOdd() ? '3' : '2');
			if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			P.x.save(pb, os, ioMode);
		} else {
			cybozu::writeChar(pb, os, '1'); if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			P.x.save(pb, os, ioMode); if (!*pb) return;
			if (sep) {
				cybozu::writeChar(pb, os, sep);
				if (!*pb) return;
			}
			P.y.save(pb, os, ioMode);
		}
	}
	template<class InputStream>
	void load(bool *pb, InputStream& is, int ioMode)
	{
		z = 1;
		if (ioMode & IoEcAffineSerialize) {
			if (b_ == 0) { // assume Zero if x = y = 0
				*pb = false;
				return;
			}
			x.load(pb, is, IoSerialize);
			if (!*pb) return;
			y.load(pb, is, IoSerialize);
			if (!*pb) return;
			if (x.isZero() && y.isZero()) {
				z.clear();
				return;
			}
			goto verifyValidAffine;
		}
		if (ioMode & (IoSerialize | IoSerializeHexStr)) {
			const size_t n = Fp::getByteSize();
			const size_t adj = isMSBserialize() ? 0 : 1;
			const size_t n1 = n + adj;
			uint8_t buf[sizeof(Fp) + 1];
			size_t readSize;
			if (ioMode & IoSerializeHexStr) {
				readSize = mcl::fp::readHexStr(buf, n1, is);
			} else {
				readSize = cybozu::readSome(buf, n1, is);
			}
			if (readSize != n1) {
				*pb = false;
				return;
			}
			if (Fp::BaseFp::isETHserialization()) {
				const uint8_t c_flag = 0x80;
				const uint8_t b_flag = 0x40;
				const uint8_t a_flag = 0x20;
				*pb = false;
				if ((buf[0] & c_flag) == 0) { // assume compressed
					return;
				}
				if (buf[0] & b_flag) { // infinity
					if (buf[0] != (c_flag | b_flag)) return;
					for (size_t i = 1; i < n - 1; i++) {
						if (buf[i]) return;
					}
					clear();
					*pb = true;
					return;
				}
				bool a = (buf[0] & a_flag) != 0;
				buf[0] &= ~(c_flag | b_flag | a_flag);
				mcl::fp::local::byteSwap(buf, n);
				x.setArray(pb, buf, n);
				if (!*pb) return;
				getWeierstrass(y, x);
				if (!Fp::squareRoot(y, y)) {
					*pb = false;
					return;
				}
				if (ec::local::get_a_flag(y) ^ a) {
					Fp::neg(y, y);
				}
				goto verifyOrder;
			}
			if (fp::isZeroArray(buf, n1)) {
				clear();
				*pb = true;
				return;
			}
			bool isYodd;
			if (adj) {
				char c = buf[0];
				if (c != 2 && c != 3) {
					*pb = false;
					return;
				}
				isYodd = c == 3;
			} else {
				isYodd = (buf[n - 1] >> 7) != 0;
				buf[n - 1] &= 0x7f;
			}
			x.setArray(pb, buf + adj, n);
			if (!*pb) return;
			*pb = getYfromX(y, x, isYodd);
			if (!*pb) return;
		} else {
			char c = 0;
			if (!fp::local::skipSpace(&c, is)) {
				*pb = false;
				return;
			}
			if (c == '0') {
				clear();
				*pb = true;
				return;
			}
			x.load(pb, is, ioMode); if (!*pb) return;
			if (c == '1') {
				y.load(pb, is, ioMode); if (!*pb) return;
				goto verifyValidAffine;
			} else if (c == '2' || c == '3') {
				bool isYodd = c == '3';
				*pb = getYfromX(y, x, isYodd);
				if (!*pb) return;
			} else if (c == '4') {
				y.load(pb, is, ioMode); if (!*pb) return;
				z.load(pb, is, ioMode); if (!*pb) return;
				if (mode_ == ec::Affine) {
					if (!z.isZero() && !z.isOne()) {
						*pb = false;
						return;
					}
				}
				*pb = isValid();
				return;
			} else {
				*pb = false;
				return;
			}
		}
	verifyOrder:
		if (verifyOrder_ && !isValidOrder()) {
			*pb = false;
		} else {
			*pb = true;
		}
		return;
	verifyValidAffine:
		if (!isValidAffine()) {
			*pb = false;
			return;
		}
		goto verifyOrder;
	}
	// deplicated
	static void setCompressedExpression(bool compressedExpression = true)
	{
		if (compressedExpression) {
			ioMode_ |= IoEcCompY;
		} else {
			ioMode_ &= ~IoEcCompY;
		}
	}
	/*
		set IoMode for operator<<(), or operator>>()
	*/
	static void setIoMode(int ioMode)
	{
		assert(!(ioMode & 0xff));
		ioMode_ = ioMode;
	}
	static inline int getIoMode() { return Fp::BaseFp::getIoMode() | ioMode_; }
	static inline void getWeierstrass(Fp& yy, const Fp& x)
	{
		Fp t;
		Fp::sqr(t, x);
		t += a_;
		t *= x;
		Fp::add(yy, t, b_);
	}
	static inline bool getYfromX(Fp& y, const Fp& x, bool isYodd)
	{
		getWeierstrass(y, x);
		if (!Fp::squareRoot(y, y)) {
			return false;
		}
		if (y.isOdd() ^ isYodd) {
			Fp::neg(y, y);
		}
		return true;
	}
	inline friend EcT operator+(const EcT& x, const EcT& y) { EcT z; add(z, x, y); return z; }
	inline friend EcT operator-(const EcT& x, const EcT& y) { EcT z; sub(z, x, y); return z; }
	template<class INT>
	inline friend EcT operator*(const EcT& x, const INT& y) { EcT z; mul(z, x, y); return z; }
	EcT& operator+=(const EcT& x) { add(*this, *this, x); return *this; }
	EcT& operator-=(const EcT& x) { sub(*this, *this, x); return *this; }
	template<class INT>
	EcT& operator*=(const INT& x) { mul(*this, *this, x); return *this; }
	EcT operator-() const { EcT x; neg(x, *this); return x; }
	bool operator==(const EcT& rhs) const
	{
		switch (mode_) {
		case ec::Jacobi:
			return ec::isEqualJacobi(*this, rhs);
		case ec::Proj:
			return ec::isEqualProj(*this, rhs);
		case ec::Affine:
		default:
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}
	}
	bool operator!=(const EcT& rhs) const { return !operator==(rhs); }
	bool operator<(const EcT& rhs) const
	{
		return compare(*this, rhs) < 0;
	}
	bool operator>=(const EcT& rhs) const { return !operator<(rhs); }
	bool operator>(const EcT& rhs) const { return rhs < *this; }
	bool operator<=(const EcT& rhs) const { return !operator>(rhs); }
	static inline void mulArray(EcT& z, const EcT& x, const fp::Unit *y, size_t yn, bool isNegative, bool constTime = false, bool useGLV = true)
	{
		if (!constTime) {
			if (yn == 0) {
				z.clear();
				return;
			}
			yn = fp::getNonZeroArraySize(y, yn);
			if (yn <= 1 && mulSmallInt(z, x, *y, isNegative)) return;
		}
		if (useGLV && mulArrayGLV && (yn * sizeof(fp::Unit) > 8)) {
			mulArrayGLV(z, x, y, yn, isNegative, constTime);
			return;
		}
		mulArrayBase(z, x, y, yn, isNegative, constTime);
	}
	static inline bool mulSmallInt(EcT& z, const EcT& x, fp::Unit y, bool isNegative)
	{
		switch (y) {
		case 0: z.clear(); return true;
		case 1: z = x; break;
		case 2: dbl(z, x); break;
		case 3: {
			EcT t;
			dbl(t, x);
			add(z, t, x);
			break;
		}
		case 4: {
			dbl(z, x);
			dbl(z, z);
			break;
		}
		case 5: {
			EcT t;
			dbl(t, x);
			dbl(t, t);
			add(z, t, x);
			break;
		}
		case 6: {
			EcT t;
			dbl(t, x);
			add(z, t, x);
			dbl(z, z);
			break;
		}
		case 7: {
			EcT t;
			dbl(t, x);
			dbl(t, t);
			dbl(t, t);
			sub(z, t, x);
			break;
		}
		case 8: {
			dbl(z, x);
			dbl(z, z);
			dbl(z, z);
			break;
		}
		case 9: {
			EcT t;
			dbl(t, x);
			dbl(t, t);
			dbl(t, t);
			add(z, t, x);
			break;
		}
		case 10: {
			EcT t;
			dbl(t, x);
			dbl(t, t);
			add(z, t, x);
			dbl(z, z);
			break;
		}
		case 11: {
			EcT t1, t2;
			dbl(t1, x); // 2x
			dbl(t2, t1);
			dbl(t2, t2); // 8x
			add(t2, t2, t1);
			add(z, t2, x);
			break;
		}
		case 12: {
			EcT t1, t2;
			dbl(t1, x);
			dbl(t1, t1); // 4x
			dbl(t2, t1); // 8x
			add(z, t1, t2);
			break;
		}
		case 13: {
			EcT t1, t2;
			dbl(t1, x);
			dbl(t1, t1); // 4x
			dbl(t2, t1); // 8x
			add(t1, t1, t2); // 12x
			add(z, t1, x);
			break;
		}
		case 14: {
			EcT t;
			// (8 - 1) * 2
			dbl(t, x);
			dbl(t, t);
			dbl(t, t);
			sub(t, t, x);
			dbl(z, t);
			break;
		}
		case 15: {
			EcT t;
			dbl(t, x);
			dbl(t, t);
			dbl(t, t);
			dbl(t, t);
			sub(z, t, x);
			break;
		}
		case 16: {
			dbl(z, x);
			dbl(z, z);
			dbl(z, z);
			dbl(z, z);
			break;
		}
		default:
			return false;
		}
		if (isNegative) {
			neg(z, z);
		}
		return true;
	}
	static inline void mulArrayBase(EcT& z, const EcT& x, const fp::Unit *y, size_t yn, bool isNegative, bool constTime)
	{
		(void)constTime;
		mpz_class v;
		bool b;
		gmp::setArray(&b, v, y, yn);
		assert(b); (void)b;
		if (isNegative) v = -v;
		const int maxW = 5;
		const int maxTblSize = 1 << (maxW - 2);
		/*
			L = log2(y), w = (L <= 32) ? 3 : (L <= 128) ? 4 : 5;
		*/
		const int w = (yn == 1 && *y <= (1ull << 32)) ? 3 : (yn * sizeof(fp::Unit) > 16) ? 5 : 4;
		const size_t tblSize = size_t(1) << (w - 2);
		typedef mcl::FixedArray<int8_t, sizeof(EcT::Fp) * 8 + 1> NafArray;
		NafArray naf;
		EcT tbl[maxTblSize];
		gmp::getNAFwidth(&b, naf, v, w);
		assert(b); (void)b;
		EcT P2;
		dbl(P2, x);
		tbl[0] = x;
		for (size_t i = 1; i < tblSize; i++) {
			add(tbl[i], tbl[i - 1], P2);
		}
		z.clear();
		for (size_t i = 0; i < naf.size(); i++) {
			EcT::dbl(z, z);
			local::addTbl(z, tbl, naf, naf.size() - 1 - i);
		}
	}
	/*
		generic mul
		GLV can't be applied in Fp12 - GT
	*/
	static inline void mulGeneric(EcT& z, const EcT& x, const mpz_class& y, bool constTime = false)
	{
		mulArray(z, x, gmp::getUnit(y), gmp::getUnitSize(y), y < 0, constTime, false);
	}
	/*
		z = sum_{i=0}^{n-1} xVec[i] * yVec[i]
		return min(N, n)
		@note &z != xVec[i]
	*/
private:
	template<class tag, size_t maxBitSize, template<class _tag, size_t _maxBitSize>class FpT>
	static inline size_t mulVecN(EcT& z, const EcT *xVec, const FpT<tag, maxBitSize> *yVec, size_t n)
	{
		const size_t N = mcl::fp::maxMulVecN;
		if (n > N) n = N;
		const int w = 5;
		const size_t tblSize = 1 << (w - 2);
		typedef mcl::FixedArray<int8_t, sizeof(EcT::Fp) * 8 + 1> NafArray;
		NafArray naf[N];
		EcT tbl[N][tblSize];
		size_t maxBit = 0;
		mpz_class y;
		for (size_t i = 0; i < n; i++) {
			bool b;
			yVec[i].getMpz(&b, y);
			assert(b); (void)b;
			gmp::getNAFwidth(&b, naf[i], y, w);
			assert(b); (void)b;
			if (naf[i].size() > maxBit) maxBit = naf[i].size();
			EcT P2;
			EcT::dbl(P2, xVec[i]);
			tbl[i][0] = xVec[i];
			for (size_t j = 1; j < tblSize; j++) {
				EcT::add(tbl[i][j], tbl[i][j - 1], P2);
			}
		}
		z.clear();
		for (size_t i = 0; i < maxBit; i++) {
			EcT::dbl(z, z);
			for (size_t j = 0; j < n; j++) {
				local::addTbl(z, tbl[j], naf[j], maxBit - 1 - i);
			}
		}
		return n;
	}

public:
	template<class tag, size_t maxBitSize, template<class _tag, size_t _maxBitSize>class FpT>
	static inline void mulVec(EcT& z, const EcT *xVec, const FpT<tag, maxBitSize> *yVec, size_t n)
	{
		/*
			mulVecNGLV is a little slow for large n
		*/
#if 1
		if (mulVecNGLV && n <= mcl::fp::maxMulVecNGLV) {
			mpz_class myVec[mcl::fp::maxMulVecNGLV];
			for (size_t i = 0; i < n; i++) {
				bool b;
				yVec[i].getMpz(&b, myVec[i]);
				assert(b); (void)b;
			}
			size_t done = mulVecNGLV(z, xVec, myVec, n);
			assert(done == n); (void)done;
			return;
		}
#else
		if (mulVecNGLV) {
			EcT r;
			r.clear();
			mpz_class myVec[mcl::fp::maxMulVecNGLV];
			while (n > 0) {
				size_t nn = mcl::fp::min_(mcl::fp::maxMulVecNGLV, n);
				for (size_t i = 0; i < nn; i++) {
					bool b;
					yVec[i].getMpz(&b, myVec[i]);
					assert(b); (void)b;
				}
				EcT t;
				size_t done = mulVecNGLV(t, xVec, myVec, nn);
				assert(nn == done);
				r += t;
				xVec += done;
				yVec += done;
				n -= done;
			}
			z = r;
			return;
		}
#endif
		EcT r;
		r.clear();
		while (n > 0) {
			EcT t;
			size_t done = mulVecN(t, xVec, yVec, n);
			r += t;
			xVec += done;
			yVec += done;
			n -= done;
		}
		z = r;
	}
	// multi thread version of mulVec
	// the num of thread is automatically detected if cpuN = 0
	template<class tag, size_t maxBitSize, template<class _tag, size_t _maxBitSize>class FpT>
	static inline void mulVecMT(EcT& z, const EcT *xVec, const FpT<tag, maxBitSize> *yVec, size_t n, size_t cpuN = 0)
	{
#ifdef MCL_USE_OMP
	const size_t minN = mcl::fp::maxMulVecN;
	if (cpuN == 0) {
		cpuN = omp_get_num_procs();
		if (n < minN * cpuN) {
			cpuN = (n + minN - 1) / minN;
		}
	}
	if (cpuN <= 1 || n <= cpuN) {
		mulVec(z, xVec, yVec, n);
		return;
	}
	EcT *zs = (EcT*)CYBOZU_ALLOCA(sizeof(EcT) * cpuN);
	size_t q = n / cpuN;
	size_t r = n % cpuN;
	#pragma omp parallel for
	for (size_t i = 0; i < cpuN; i++) {
		size_t adj = q * i + fp::min_(i, r);
		mulVec(zs[i], xVec + adj, yVec + adj, q + (i < r));
	}
	z.clear();
//	#pragma omp declare reduction(red:EcT:omp_out *= omp_in) initializer(omp_priv = omp_orig)
//	#pragma omp parallel for reduction(red:z)
	for (size_t i = 0; i < cpuN; i++) {
		z += zs[i];
	}
#else
		(void)cpuN;
		mulVec(z, xVec, yVec, n);
#endif
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	static inline void init(const std::string& astr, const std::string& bstr, int mode = ec::Jacobi)
	{
		bool b;
		init(&b, astr.c_str(), bstr.c_str(), mode);
		if (!b) throw cybozu::Exception("mcl:EcT:init");
	}
	void set(const Fp& _x, const Fp& _y, bool verify = true)
	{
		bool b;
		set(&b, _x, _y, verify);
		if (!b) throw cybozu::Exception("ec:EcT:set") << _x << _y;
	}
	template<class OutputStream>
	void save(OutputStream& os, int ioMode = IoSerialize) const
	{
		bool b;
		save(&b, os, ioMode);
		if (!b) throw cybozu::Exception("EcT:save");
	}
	template<class InputStream>
	void load(InputStream& is, int ioMode = IoSerialize)
	{
		bool b;
		load(&b, is, ioMode);
		if (!b) throw cybozu::Exception("EcT:load");
	}
#endif
#ifndef CYBOZU_DONT_USE_STRING
	// backward compatilibity
	static inline void setParam(const std::string& astr, const std::string& bstr, int mode = ec::Jacobi)
	{
		init(astr, bstr, mode);
	}
	friend inline std::istream& operator>>(std::istream& is, EcT& self)
	{
		self.load(is, fp::detectIoMode(getIoMode(), is));
		return is;
	}
	friend inline std::ostream& operator<<(std::ostream& os, const EcT& self)
	{
		self.save(os, fp::detectIoMode(getIoMode(), os));
		return os;
	}
#endif
};

template<class Fp> Fp EcT<Fp>::a_;
template<class Fp> Fp EcT<Fp>::b_;
template<class Fp> int EcT<Fp>::specialA_;
template<class Fp> int EcT<Fp>::ioMode_;
template<class Fp> bool EcT<Fp>::verifyOrder_;
template<class Fp> mpz_class EcT<Fp>::order_;
template<class Fp> void (*EcT<Fp>::mulArrayGLV)(EcT& z, const EcT& x, const fp::Unit *y, size_t yn, bool isNegative, bool constTime);
template<class Fp> size_t (*EcT<Fp>::mulVecNGLV)(EcT& z, const EcT *xVec, const mpz_class *yVec, size_t yn);
template<class Fp> bool (*EcT<Fp>::isValidOrderFast)(const EcT& x);
template<class Fp> int EcT<Fp>::mode_;

// r = the order of Ec
template<class Ec, class _Fr>
struct GLV1T {
	typedef GLV1T<Ec, _Fr> GLV1;
	typedef typename Ec::Fp Fp;
	typedef _Fr Fr;
	static Fp rw; // rw = 1 / w = (-1 - sqrt(-3)) / 2
	static size_t rBitSize;
	static mpz_class v0, v1;
	static mpz_class B[2][2];
public:
#ifndef CYBOZU_DONT_USE_STRING
	static void dump(const mpz_class& x)
	{
		printf("\"%s\",\n", mcl::gmp::getStr(x, 16).c_str());
	}
	static void dump()
	{
		printf("\"%s\",\n", rw.getStr(16).c_str());
		printf("%d,\n", (int)rBitSize);
		dump(v0);
		dump(v1);
		dump(B[0][0]); dump(B[0][1]); dump(B[1][0]); dump(B[1][1]);
	}
#endif
	/*
		L (x, y) = (rw x, y)
	*/
	static void mulLambda(Ec& Q, const Ec& P)
	{
		Fp::mul(Q.x, P.x, rw);
		Q.y = P.y;
		Q.z = P.z;
	}
	/*
		x = u[0] + u[1] * lambda mod r
	*/
	static void split(mpz_class u[2], const mpz_class& x)
	{
		mpz_class& a = u[0];
		mpz_class& b = u[1];
		mpz_class t;
		t = (x * v0) >> rBitSize;
		b = (x * v1) >> rBitSize;
		a = x - (t * B[0][0] + b * B[1][0]);
		b = - (t * B[0][1] + b * B[1][1]);
	}
	static void mul(Ec& Q, const Ec& P, const mpz_class& x, bool constTime = false)
	{
		if (constTime) {
			ec::local::mul1CT<GLV1, Ec, _Fr, 2, 4>(Q, P, x);
		} else {
			ec::local::mulVecNGLVT<GLV1, Ec, _Fr, 2, 5, 1>(Q, &P, &x, 1);
		}
	}
	static inline size_t mulVecNGLV(Ec& z, const Ec *xVec, const mpz_class *yVec, size_t n)
	{
		return ec::local::mulVecNGLVT<GLV1, Ec, _Fr, 2, 5, mcl::fp::maxMulVecNGLV>(z, xVec, yVec, n);
	}
	static void mulArrayGLV(Ec& z, const Ec& x, const mcl::fp::Unit *y, size_t yn, bool isNegative, bool constTime)
	{
		mpz_class s;
		bool b;
		mcl::gmp::setArray(&b, s, y, yn);
		assert(b);
		if (isNegative) s = -s;
		mul(z, x, s, constTime);
	}
	/*
		initForBN() is defined in bn.hpp
	*/
	static void initForSecp256k1()
	{
		bool b = Fp::squareRoot(rw, -3);
		assert(b);
		(void)b;
		rw = -(rw + 1) / 2;
		rBitSize = Fr::getOp().bitSize;
		rBitSize = (rBitSize + fp::UnitBitSize - 1) & ~(fp::UnitBitSize - 1);
		gmp::setStr(&b, B[0][0], "0x3086d221a7d46bcde86c90e49284eb15");
		assert(b); (void)b;
		gmp::setStr(&b, B[0][1], "-0xe4437ed6010e88286f547fa90abfe4c3");
		assert(b); (void)b;
		gmp::setStr(&b, B[1][0], "0x114ca50f7a8e2f3f657c1108d9d44cfd8");
		assert(b); (void)b;
		B[1][1] = B[0][0];
		const mpz_class& r = Fr::getOp().mp;
		v0 = ((B[1][1]) << rBitSize) / r;
		v1 = ((-B[0][1]) << rBitSize) / r;
	}
};

// rw = 1 / w = (-1 - sqrt(-3)) / 2
template<class Ec, class Fr> typename Ec::Fp GLV1T<Ec, Fr>::rw;
template<class Ec, class Fr> size_t GLV1T<Ec, Fr>::rBitSize;
template<class Ec, class Fr> mpz_class GLV1T<Ec, Fr>::v0;
template<class Ec, class Fr> mpz_class GLV1T<Ec, Fr>::v1;
template<class Ec, class Fr> mpz_class GLV1T<Ec, Fr>::B[2][2];

/*
	Ec : elliptic curve
	Zn : cyclic group of the order |Ec|
	set P the generator of Ec if P != 0
*/
template<class Ec, class Zn>
void initCurve(bool *pb, int curveType, Ec *P = 0, mcl::fp::Mode mode = fp::FP_AUTO, mcl::ec::Mode ecMode = ec::Jacobi)
{
	typedef typename Ec::Fp Fp;
	*pb = false;
	const EcParam *ecParam = getEcParam(curveType);
	if (ecParam == 0) return;

	Zn::init(pb, ecParam->n, mode);
	if (!*pb) return;
	Fp::init(pb, ecParam->p, mode);
	if (!*pb) return;
	Ec::init(pb, ecParam->a, ecParam->b, ecMode);
	if (!*pb) return;
	if (P) {
		Fp x, y;
		x.setStr(pb, ecParam->gx);
		if (!*pb) return;
		y.setStr(pb, ecParam->gy);
		if (!*pb) return;
		P->set(pb, x, y);
		if (!*pb) return;
	}
	if (curveType == MCL_SECP256K1) {
		GLV1T<Ec, Zn>::initForSecp256k1();
		Ec::setMulArrayGLV(GLV1T<Ec, Zn>::mulArrayGLV, GLV1T<Ec, Zn>::mulVecNGLV);
	} else {
		Ec::setMulArrayGLV(0);
	}
}

#ifndef CYBOZU_DONT_USE_EXCEPTION
template<class Ec, class Zn>
void initCurve(int curveType, Ec *P = 0, mcl::fp::Mode mode = fp::FP_AUTO, mcl::ec::Mode ecMode = ec::Jacobi)
{
	bool b;
	initCurve<Ec, Zn>(&b, curveType, P, mode, ecMode);
	if (!b) throw cybozu::Exception("mcl:initCurve") << curveType << mode << ecMode;
}
#endif

} // mcl

#ifndef CYBOZU_DONT_USE_EXCEPTION
#ifdef CYBOZU_USE_BOOST
namespace mcl {
template<class Fp>
size_t hash_value(const mcl::EcT<Fp>& P_)
{
	if (P_.isZero()) return 0;
	mcl::EcT<Fp> P(P_); P.normalize();
	return mcl::hash_value(P.y, mcl::hash_value(P.x));
}

}
#else
namespace std { CYBOZU_NAMESPACE_TR1_BEGIN

template<class Fp>
struct hash<mcl::EcT<Fp> > {
	size_t operator()(const mcl::EcT<Fp>& P_) const
	{
		if (P_.isZero()) return 0;
		mcl::EcT<Fp> P(P_); P.normalize();
		return hash<Fp>()(P.y, hash<Fp>()(P.x));
	}
};

CYBOZU_NAMESPACE_TR1_END } // std
#endif
#endif

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
