#pragma once
/**
	@file
	@brief finite field extension class
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mcl/fp.hpp>

namespace mcl {

template<class Fp> struct Fp12T;
template<class Fp> class BNT;
template<class Fp> struct Fp2DblT;

template<class Fp>
class FpDblT : public fp::Serializable<FpDblT<Fp> > {
	typedef fp::Unit Unit;
	Unit v_[Fp::maxSize * 2];
	friend struct Fp2DblT<Fp>;
public:
	static size_t getUnitSize() { return Fp::op_.N * 2; }
	const fp::Unit *getUnit() const { return v_; }
	void dump() const
	{
		const size_t n = getUnitSize();
		for (size_t i = 0; i < n; i++) {
			mcl::fp::dumpUnit(v_[n - 1 - i]);
		}
		printf("\n");
	}
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int) const
	{
		char buf[1024];
		size_t n = mcl::fp::arrayToHex(buf, sizeof(buf), v_, getUnitSize());
		if (n == 0) {
			*pb = false;
			return;
		}
		cybozu::write(pb, os, buf + sizeof(buf) - n, sizeof(buf));
	}
	template<class InputStream>
	void load(bool *pb, InputStream& is, int)
	{
		char buf[1024];
		*pb = false;
		size_t n = fp::local::loadWord(buf, sizeof(buf), is);
		if (n == 0) return;
		n = fp::hexToArray(v_, getUnitSize(), buf, n);
		if (n == 0) return;
		for (size_t i = n; i < getUnitSize(); i++) v_[i] = 0;
		*pb = true;
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	template<class OutputStream>
	void save(OutputStream& os, int ioMode = IoSerialize) const
	{
		bool b;
		save(&b, os, ioMode);
		if (!b) throw cybozu::Exception("FpDblT:save") << ioMode;
	}
	template<class InputStream>
	void load(InputStream& is, int ioMode = IoSerialize)
	{
		bool b;
		load(&b, is, ioMode);
		if (!b) throw cybozu::Exception("FpDblT:load") << ioMode;
	}
	void getMpz(mpz_class& x) const
	{
		bool b;
		getMpz(&b, x);
		if (!b) throw cybozu::Exception("FpDblT:getMpz");
	}
	mpz_class getMpz() const
	{
		mpz_class x;
		getMpz(x);
		return x;
	}
#endif
	void clear()
	{
		const size_t n = getUnitSize();
		for (size_t i = 0; i < n; i++) {
			v_[i] = 0;
		}
	}
	FpDblT& operator=(const FpDblT& rhs)
	{
		const size_t n = getUnitSize();
		for (size_t i = 0; i < n; i++) {
			v_[i] = rhs.v_[i];
		}
		return *this;
	}
	// QQQ : does not check range of x strictly(use for debug)
	void setMpz(const mpz_class& x)
	{
		assert(x >= 0);
		const size_t xn = gmp::getUnitSize(x);
		const size_t N2 = getUnitSize();
		if (xn > N2) {
			assert(0);
			return;
		}
		memcpy(v_, gmp::getUnit(x), xn * sizeof(Unit));
		memset(v_ + xn, 0, (N2 - xn) * sizeof(Unit));
	}
	void getMpz(bool *pb, mpz_class& x) const
	{
		gmp::setArray(pb, x, v_, Fp::op_.N * 2);
	}
	static inline void add(FpDblT& z, const FpDblT& x, const FpDblT& y)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fpDbl_addA_(z.v_, x.v_, y.v_);
#else
		Fp::op_.fpDbl_add(z.v_, x.v_, y.v_, Fp::op_.p);
#endif
	}
	static inline void sub(FpDblT& z, const FpDblT& x, const FpDblT& y)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fpDbl_subA_(z.v_, x.v_, y.v_);
#else
		Fp::op_.fpDbl_sub(z.v_, x.v_, y.v_, Fp::op_.p);
#endif
	}
	static inline void mod(Fp& z, const FpDblT& xy)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fpDbl_modA_(z.v_, xy.v_);
#else
		Fp::op_.fpDbl_mod(z.v_, xy.v_, Fp::op_.p);
#endif
	}
#ifdef MCL_XBYAK_DIRECT_CALL
	static void addA(Unit *z, const Unit *x, const Unit *y) { Fp::op_.fpDbl_add(z, x, y, Fp::op_.p); }
	static void subA(Unit *z, const Unit *x, const Unit *y) { Fp::op_.fpDbl_sub(z, x, y, Fp::op_.p); }
	static void modA(Unit *z, const Unit *xy) { Fp::op_.fpDbl_mod(z, xy, Fp::op_.p); }
#endif
	static void addPre(FpDblT& z, const FpDblT& x, const FpDblT& y) { Fp::op_.fpDbl_addPre(z.v_, x.v_, y.v_); }
	static void subPre(FpDblT& z, const FpDblT& x, const FpDblT& y) { Fp::op_.fpDbl_subPre(z.v_, x.v_, y.v_); }
	/*
		mul(z, x, y) = mulPre(xy, x, y) + mod(z, xy)
	*/
	static void mulPre(FpDblT& xy, const Fp& x, const Fp& y) { Fp::op_.fpDbl_mulPre(xy.v_, x.v_, y.v_); }
	static void sqrPre(FpDblT& xx, const Fp& x) { Fp::op_.fpDbl_sqrPre(xx.v_, x.v_); }
	static void mulUnit(FpDblT& z, const FpDblT& x, Unit y)
	{
		if (mulSmallUnit(z, x, y)) return;
		assert(0); // not supported y
	}
	static void init()
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		mcl::fp::Op& op = Fp::op_;
		if (op.fpDbl_addA_ == 0) {
			op.fpDbl_addA_ = addA;
		}
		if (op.fpDbl_subA_ == 0) {
			op.fpDbl_subA_ = subA;
		}
		if (op.fpDbl_modA_ == 0) {
			op.fpDbl_modA_ = modA;
		}
#endif
	}
	void operator+=(const FpDblT& x) { add(*this, *this, x); }
	void operator-=(const FpDblT& x) { sub(*this, *this, x); }
};

/*
	beta = -1
	Fp2 = F[i] / (i^2 + 1)
	x = a + bi
*/
template<class _Fp>
class Fp2T : public fp::Serializable<Fp2T<_Fp>,
	fp::Operator<Fp2T<_Fp> > > {
	typedef _Fp Fp;
	typedef fp::Unit Unit;
	typedef FpDblT<Fp> FpDbl;
	typedef Fp2DblT<Fp> Fp2Dbl;
	static const size_t gN = 5;
	/*
		g = xi^((p - 1) / 6)
		g[] = { g^2, g^4, g^1, g^3, g^5 }
	*/
	static Fp2T g[gN];
	static Fp2T g2[gN];
	static Fp2T g3[gN];
public:
	static const Fp2T *get_gTbl() { return &g[0]; }
	static const Fp2T *get_g2Tbl() { return &g2[0]; }
	static const Fp2T *get_g3Tbl() { return &g3[0]; }
	typedef typename Fp::BaseFp BaseFp;
	static const size_t maxSize = Fp::maxSize * 2;
	static inline size_t getByteSize() { return Fp::getByteSize() * 2; }
	void dump() const
	{
		a.dump();
		b.dump();
	}
	Fp a, b;
	Fp2T() { }
	Fp2T(int64_t a) : a(a), b(0) { }
	Fp2T(const Fp& a, const Fp& b) : a(a), b(b) { }
	Fp2T(int64_t a, int64_t b) : a(a), b(b) { }
	Fp* getFp0() { return &a; }
	const Fp* getFp0() const { return &a; }
	const Unit* getUnit() const { return a.getUnit(); }
	void clear()
	{
		a.clear();
		b.clear();
	}
	void set(const Fp &a_, const Fp &b_)
	{
		a = a_;
		b = b_;
	}
	static void add(Fp2T& z, const Fp2T& x, const Fp2T& y)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fp2_addA_(z.a.v_, x.a.v_, y.a.v_);
#else
		addA(z.a.v_, x.a.v_, y.a.v_);
#endif
	}
	static void sub(Fp2T& z, const Fp2T& x, const Fp2T& y)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fp2_subA_(z.a.v_, x.a.v_, y.a.v_);
#else
		subA(z.a.v_, x.a.v_, y.a.v_);
#endif
	}
	static void neg(Fp2T& y, const Fp2T& x)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fp2_negA_(y.a.v_, x.a.v_);
#else
		negA(y.a.v_, x.a.v_);
#endif
	}
	static void mul(Fp2T& z, const Fp2T& x, const Fp2T& y)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fp2_mulA_(z.a.v_, x.a.v_, y.a.v_);
#else
		mulA(z.a.v_, x.a.v_, y.a.v_);
#endif
	}
	static void sqr(Fp2T& y, const Fp2T& x)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fp2_sqrA_(y.a.v_, x.a.v_);
#else
		sqrA(y.a.v_, x.a.v_);
#endif
	}
	static void mul2(Fp2T& y, const Fp2T& x)
	{
#ifdef MCL_XBYAK_DIRECT_CALL
		Fp::op_.fp2_mul2A_(y.a.v_, x.a.v_);
#else
		mul2A(y.a.v_, x.a.v_);
#endif
	}
	static void mul_xi(Fp2T& y, const Fp2T& x)
	{
		Fp::op_.fp2_mul_xiA_(y.a.v_, x.a.v_);
	}
	/*
		x = a + bi
		1 / x = (a - bi) / (a^2 + b^2)
	*/
	static void inv(Fp2T& y, const Fp2T& x)
	{
		assert(!x.isZero());
		const Fp& a = x.a;
		const Fp& b = x.b;
		Fp r;
		norm(r, x);
		Fp::inv(r, r); // r = 1 / (a^2 + b^2)
		Fp::mul(y.a, a, r);
		Fp::mul(y.b, b, r);
		Fp::neg(y.b, y.b);
	}
	static void addPre(Fp2T& z, const Fp2T& x, const Fp2T& y)
	{
		Fp::addPre(z.a, x.a, y.a);
		Fp::addPre(z.b, x.b, y.b);
	}
	static void divBy2(Fp2T& y, const Fp2T& x)
	{
		Fp::divBy2(y.a, x.a);
		Fp::divBy2(y.b, x.b);
	}
	static void divBy4(Fp2T& y, const Fp2T& x)
	{
		Fp::divBy4(y.a, x.a);
		Fp::divBy4(y.b, x.b);
	}
	static void mulFp(Fp2T& z, const Fp2T& x, const Fp& y)
	{
		Fp::mul(z.a, x.a, y);
		Fp::mul(z.b, x.b, y);
	}
	template<class S>
	void setArray(bool *pb, const S *buf, size_t n)
	{
		assert((n & 1) == 0);
		n /= 2;
		a.setArray(pb, buf, n);
		if (!*pb) return;
		b.setArray(pb, buf + n, n);
	}
	template<class InputStream>
	void load(bool *pb, InputStream& is, int ioMode)
	{
		Fp *ap = &a, *bp = &b;
		if (Fp::isETHserialization_ && ioMode & (IoSerialize | IoSerializeHexStr)) {
			fp::swap_(ap, bp);
		}
		ap->load(pb, is, ioMode);
		if (!*pb) return;
		bp->load(pb, is, ioMode);
	}
	/*
		Fp2T = <a> + ' ' + <b>
	*/
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int ioMode) const
	{
		const Fp *ap = &a, *bp = &b;
		if (Fp::isETHserialization_ && ioMode & (IoSerialize | IoSerializeHexStr)) {
			fp::swap_(ap, bp);
		}
		const char sep = *fp::getIoSeparator(ioMode);
		ap->save(pb, os, ioMode);
		if (!*pb) return;
		if (sep) {
			cybozu::writeChar(pb, os, sep);
			if (!*pb) return;
		}
		bp->save(pb, os, ioMode);
	}
	bool isZero() const { return a.isZero() && b.isZero(); }
	bool isOne() const { return a.isOne() && b.isZero(); }
	bool operator==(const Fp2T& rhs) const { return a == rhs.a && b == rhs.b; }
	bool operator!=(const Fp2T& rhs) const { return !operator==(rhs); }
	/*
		return true is a is odd (do not consider b)
		this function is for only compressed reprezentation of EC
		isOdd() is not good naming. QQQ
	*/
	bool isOdd() const { return a.isOdd(); }
	/*
		(a + bi)^2 = (a^2 - b^2) + 2ab i = c + di
		A = a^2
		B = b^2
		A = (c +/- sqrt(c^2 + d^2))/2
		b = d / 2a
	*/
	static inline bool squareRoot(Fp2T& y, const Fp2T& x)
	{
		Fp t1, t2;
		if (x.b.isZero()) {
			if (Fp::squareRoot(t1, x.a)) {
				y.a = t1;
				y.b.clear();
			} else {
				bool b = Fp::squareRoot(t1, -x.a);
				assert(b); (void)b;
				y.a.clear();
				y.b = t1;
			}
			return true;
		}
		Fp::sqr(t1, x.a);
		Fp::sqr(t2, x.b);
		t1 += t2; // c^2 + d^2
		if (!Fp::squareRoot(t1, t1)) return false;
		Fp::add(t2, x.a, t1);
		Fp::divBy2(t2, t2);
		if (!Fp::squareRoot(t2, t2)) {
			Fp::sub(t2, x.a, t1);
			Fp::divBy2(t2, t2);
			bool b = Fp::squareRoot(t2, t2);
			assert(b); (void)b;
		}
		y.a = t2;
		t2 += t2;
		Fp::inv(t2, t2);
		Fp::mul(y.b, x.b, t2);
		return true;
	}
	// y = a^2 + b^2
	static void inline norm(Fp& y, const Fp2T& x)
	{
		FpDbl AA, BB;
		FpDbl::sqrPre(AA, x.a);
		FpDbl::sqrPre(BB, x.b);
		FpDbl::addPre(AA, AA, BB);
		FpDbl::mod(y, AA);
	}
	/*
		Frobenius
		i^2 = -1
		(a + bi)^p = a + bi^p in Fp
		= a + bi if p = 1 mod 4
		= a - bi if p = 3 mod 4
	*/
	static void Frobenius(Fp2T& y, const Fp2T& x)
	{
		if (Fp::getOp().pmod4 == 1) {
			if (&y != &x) {
				y = x;
			}
		} else {
			if (&y != &x) {
				y.a = x.a;
			}
			Fp::neg(y.b, x.b);
		}
	}

	static uint32_t get_xi_a() { return Fp::getOp().xi_a; }
	static void init(bool *pb)
	{
		mcl::fp::Op& op = Fp::op_;
		assert(op.xi_a);
		// assume p < W/4 where W = 1 << (N * sizeof(Unit) * 8)
		if ((op.p[op.N - 1] >> (sizeof(fp::Unit) * 8 - 2)) != 0) {
			*pb = false;
			return;
		}
#ifdef MCL_XBYAK_DIRECT_CALL
		if (op.fp2_addA_ == 0) {
			op.fp2_addA_ = addA;
		}
		if (op.fp2_subA_ == 0) {
			op.fp2_subA_ = subA;
		}
		if (op.fp2_negA_ == 0) {
			op.fp2_negA_ = negA;
		}
		if (op.fp2_mulA_ == 0) {
			op.fp2_mulA_ = mulA;
		}
		if (op.fp2_sqrA_ == 0) {
			op.fp2_sqrA_ = sqrA;
		}
		if (op.fp2_mul2A_ == 0) {
			op.fp2_mul2A_ = mul2A;
		}
#endif
		if (op.fp2_mul_xiA_ == 0) {
			if (op.xi_a == 1) {
				op.fp2_mul_xiA_ = fp2_mul_xi_1_1iA;
			} else {
				op.fp2_mul_xiA_ = fp2_mul_xiA;
			}
		}
		FpDblT<Fp>::init();
		Fp2DblT<Fp>::init();
		// call init before Fp2::pow because FpDbl is used in Fp2T
		const Fp2T xi(op.xi_a, 1);
		const mpz_class& p = Fp::getOp().mp;
		Fp2T::pow(g[0], xi, (p - 1) / 6); // g = xi^((p-1)/6)
		for (size_t i = 1; i < gN; i++) {
			g[i] = g[i - 1] * g[0];
		}
		/*
			permutate [0, 1, 2, 3, 4] => [1, 3, 0, 2, 4]
			g[0] = g^2
			g[1] = g^4
			g[2] = g^1
			g[3] = g^3
			g[4] = g^5
		*/
		{
			Fp2T t = g[0];
			g[0] = g[1];
			g[1] = g[3];
			g[3] = g[2];
			g[2] = t;
		}
		for (size_t i = 0; i < gN; i++) {
			Fp2T t(g[i].a, g[i].b);
			if (Fp::getOp().pmod4 == 3) Fp::neg(t.b, t.b);
			Fp2T::mul(g2[i], t, g[i]);
			g3[i] = g[i] * g2[i];
		}
		*pb = true;
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	static void init()
	{
		bool b;
		init(&b);
		if (!b) throw cybozu::Exception("Fp2::init");
	}
	template<class InputStream>
	void load(InputStream& is, int ioMode = IoSerialize)
	{
		bool b;
		load(&b, is, ioMode);
		if (!b) throw cybozu::Exception("Fp2T:load");
	}
	template<class OutputStream>
	void save(OutputStream& os, int ioMode = IoSerialize) const
	{
		bool b;
		save(&b, os, ioMode);
		if (!b) throw cybozu::Exception("Fp2T:save");
	}
	template<class S>
	void setArray(const S *buf, size_t n)
	{
		bool b;
		setArray(&b, buf, n);
		if (!b) throw cybozu::Exception("Fp2T:setArray");
	}
#endif
#ifndef CYBOZU_DONT_USE_STRING
	Fp2T(const std::string& a, const std::string& b, int base = 0) : a(a, base), b(b, base) {}
	friend std::istream& operator>>(std::istream& is, Fp2T& self)
	{
		self.load(is, fp::detectIoMode(Fp::BaseFp::getIoMode(), is));
		return is;
	}
	friend std::ostream& operator<<(std::ostream& os, const Fp2T& self)
	{
		self.save(os, fp::detectIoMode(Fp::BaseFp::getIoMode(), os));
		return os;
	}
#endif
private:
	static Fp2T& cast(Unit *x) { return *reinterpret_cast<Fp2T*>(x); }
	static const Fp2T& cast(const Unit *x) { return *reinterpret_cast<const Fp2T*>(x); }
	/*
		default Fp2T operator
		Fp2T = Fp[i]/(i^2 + 1)
	*/
	static void addA(Unit *pz, const Unit *px, const Unit *py)
	{
		Fp2T& z = cast(pz);
		const Fp2T& x = cast(px);
		const Fp2T& y = cast(py);
		Fp::add(z.a, x.a, y.a);
		Fp::add(z.b, x.b, y.b);
	}
	static void subA(Unit *pz, const Unit *px, const Unit *py)
	{
		Fp2T& z = cast(pz);
		const Fp2T& x = cast(px);
		const Fp2T& y = cast(py);
		Fp::sub(z.a, x.a, y.a);
		Fp::sub(z.b, x.b, y.b);
	}
	static void negA(Unit *py, const Unit *px)
	{
		Fp2T& y = cast(py);
		const Fp2T& x = cast(px);
		Fp::neg(y.a, x.a);
		Fp::neg(y.b, x.b);
	}
	static void mulA(Unit *pz, const Unit *px, const Unit *py)
	{
		Fp2T& z = cast(pz);
		const Fp2T& x = cast(px);
		const Fp2T& y = cast(py);
		Fp2Dbl d;
		Fp2Dbl::mulPre(d, x, y);
		FpDbl::mod(z.a, d.a);
		FpDbl::mod(z.b, d.b);
	}
	static void mul2A(Unit *py, const Unit *px)
	{
		Fp2T& y = cast(py);
		const Fp2T& x = cast(px);
		Fp::mul2(y.a, x.a);
		Fp::mul2(y.b, x.b);
	}
	/*
		x = a + bi, i^2 = -1
		y = x^2 = (a + bi)^2 = (a + b)(a - b) + 2abi
	*/
	static void sqrA(Unit *py, const Unit *px)
	{
		Fp2T& y = cast(py);
		const Fp2T& x = cast(px);
		const Fp& a = x.a;
		const Fp& b = x.b;
#if 1 // faster than using FpDbl
		Fp t1, t2, t3;
		Fp::mul2(t1, b);
		t1 *= a; // 2ab
		Fp::add(t2, a, b); // a + b
		Fp::sub(t3, a, b); // a - b
		Fp::mul(y.a, t2, t3); // (a + b)(a - b)
		y.b = t1;
#else
		Fp t1, t2;
		FpDbl d1, d2;
		Fp::addPre(t1, b, b); // 2b
		FpDbl::mulPre(d2, t1, a); // 2ab
		Fp::addPre(t1, a, b); // a + b
		Fp::sub(t2, a, b); // a - b
		FpDbl::mulPre(d1, t1, t2); // (a + b)(a - b)
		FpDbl::mod(py[0], d1);
		FpDbl::mod(py[1], d2);
#endif
	}
	/*
		xi = xi_a + i
		x = a + bi
		y = (a + bi)xi = (a + bi)(xi_a + i)
		=(a * x_ia - b) + (a + b xi_a)i
	*/
	static void fp2_mul_xiA(Unit *py, const Unit *px)
	{
		Fp2T& y = cast(py);
		const Fp2T& x = cast(px);
		const Fp& a = x.a;
		const Fp& b = x.b;
		Fp t;
		Fp::mulUnit(t, a, Fp::getOp().xi_a);
		t -= b;
		Fp::mulUnit(y.b, b, Fp::getOp().xi_a);
		y.b += a;
		y.a = t;
	}
	/*
		xi = 1 + i ; xi_a = 1
		y = (a + bi)xi = (a - b) + (a + b)i
	*/
	static void fp2_mul_xi_1_1iA(Unit *py, const Unit *px)
	{
		Fp2T& y = cast(py);
		const Fp2T& x = cast(px);
		const Fp& a = x.a;
		const Fp& b = x.b;
		Fp t;
		Fp::add(t, a, b);
		Fp::sub(y.a, a, b);
		y.b = t;
	}
};

template<class Fp>
struct Fp2DblT {
	typedef Fp2DblT<Fp> Fp2Dbl;
	typedef FpDblT<Fp> FpDbl;
	typedef Fp2T<Fp> Fp2;
	typedef fp::Unit Unit;
	FpDbl a, b;
	static void add(Fp2DblT& z, const Fp2DblT& x, const Fp2DblT& y)
	{
		FpDbl::add(z.a, x.a, y.a);
		FpDbl::add(z.b, x.b, y.b);
	}
	static void addPre(Fp2DblT& z, const Fp2DblT& x, const Fp2DblT& y)
	{
		FpDbl::addPre(z.a, x.a, y.a);
		FpDbl::addPre(z.b, x.b, y.b);
	}
	static void sub(Fp2DblT& z, const Fp2DblT& x, const Fp2DblT& y)
	{
		FpDbl::sub(z.a, x.a, y.a);
		FpDbl::sub(z.b, x.b, y.b);
	}
	static void subPre(Fp2DblT& z, const Fp2DblT& x, const Fp2DblT& y)
	{
		FpDbl::subPre(z.a, x.a, y.a);
		FpDbl::subPre(z.b, x.b, y.b);
	}
	/*
		imaginary part of Fp2Dbl::mul uses only add,
		so it does not require mod.
	*/
	static void subSpecial(Fp2DblT& y, const Fp2DblT& x)
	{
		FpDbl::sub(y.a, y.a, x.a);
		FpDbl::subPre(y.b, y.b, x.b);
	}
	static void neg(Fp2DblT& y, const Fp2DblT& x)
	{
		FpDbl::neg(y.a, x.a);
		FpDbl::neg(y.b, x.b);
	}
	static void mulPre(Fp2DblT& z, const Fp2& x, const Fp2& y)
	{
		Fp::getOp().fp2Dbl_mulPreA_(z.a.v_, x.getUnit(), y.getUnit());
	}
	static void sqrPre(Fp2DblT& y, const Fp2& x)
	{
		Fp::getOp().fp2Dbl_sqrPreA_(y.a.v_, x.getUnit());
	}
	static void mul_xi(Fp2DblT& y, const Fp2DblT& x)
	{
		Fp::getOp().fp2Dbl_mul_xiA_(y.a.v_, x.a.getUnit());
	}
	static void mod(Fp2& y, const Fp2DblT& x)
	{
		FpDbl::mod(y.a, x.a);
		FpDbl::mod(y.b, x.b);
	}
#ifndef CYBOZU_DONT_USE_STRING
	friend std::ostream& operator<<(std::ostream& os, const Fp2DblT& x)
	{
		return os << x.a << ' ' << x.b;
	}
#endif
	void operator+=(const Fp2DblT& x) { add(*this, *this, x); }
	void operator-=(const Fp2DblT& x) { sub(*this, *this, x); }
	static void init()
 	{
		assert(!Fp::getOp().isFullBit);
		mcl::fp::Op& op = Fp::getOpNonConst();
		if (op.fp2Dbl_mulPreA_ == 0) {
			op.fp2Dbl_mulPreA_ = mulPreA;
		}
		if (op.fp2Dbl_sqrPreA_ == 0) {
			op.fp2Dbl_sqrPreA_ = sqrPreA;
		}
		if (op.fp2Dbl_mul_xiA_ == 0) {
			const uint32_t xi_a = Fp2::get_xi_a();
			if (xi_a == 1) {
				op.fp2Dbl_mul_xiA_ = mul_xi_1A;
			} else {
				op.fp2Dbl_mul_xiA_ = mul_xi_genericA;
			}
		}
	}
private:
	static Fp2 cast(Unit *x) { return *reinterpret_cast<Fp2*>(x); }
	static const Fp2 cast(const Unit *x) { return *reinterpret_cast<const Fp2*>(x); }
	static Fp2Dbl& castD(Unit *x) { return *reinterpret_cast<Fp2Dbl*>(x); }
	static const Fp2Dbl& castD(const Unit *x) { return *reinterpret_cast<const Fp2Dbl*>(x); }
	/*
		Fp2Dbl::mulPre by FpDblT
		@note mod of NIST_P192 is fast
	*/
	static void mulPreA(Unit *pz, const Unit *px, const Unit *py)
	{
		Fp2Dbl& z = castD(pz);
		const Fp2& x = cast(px);
		const Fp2& y = cast(py);
		assert(!Fp::getOp().isFullBit);
		const Fp& a = x.a;
		const Fp& b = x.b;
		const Fp& c = y.a;
		const Fp& d = y.b;
		FpDbl& d0 = z.a;
		FpDbl& d1 = z.b;
		FpDbl d2;
		Fp s, t;
		Fp::addPre(s, a, b);
		Fp::addPre(t, c, d);
		FpDbl::mulPre(d1, s, t); // (a + b)(c + d)
		FpDbl::mulPre(d0, a, c);
		FpDbl::mulPre(d2, b, d);
		FpDbl::subPre(d1, d1, d0);
		FpDbl::subPre(d1, d1, d2);
		FpDbl::sub(d0, d0, d2); // ac - bd
	}
	static void sqrPreA(Unit *py, const Unit *px)
	{
		assert(!Fp::getOp().isFullBit);
		Fp2Dbl& y = castD(py);
		const Fp2& x = cast(px);
		Fp t1, t2;
		Fp::addPre(t1, x.b, x.b); // 2b
		Fp::addPre(t2, x.a, x.b); // a + b
		FpDbl::mulPre(y.b, t1, x.a); // 2ab
		Fp::sub(t1, x.a, x.b); // a - b
		FpDbl::mulPre(y.a, t1, t2); // (a + b)(a - b)
	}
	static void mul_xi_1A(Unit *py, const Unit *px)
	{
		Fp2Dbl& y = castD(py);
		const Fp2Dbl& x = castD(px);
		FpDbl t;
		FpDbl::add(t, x.a, x.b);
		FpDbl::sub(y.a, x.a, x.b);
		y.b = t;
	}
	static void mul_xi_genericA(Unit *py, const Unit *px)
	{
		const uint32_t xi_a = Fp2::get_xi_a();
		Fp2Dbl& y = castD(py);
		const Fp2Dbl& x = castD(px);
		FpDbl t;
		FpDbl::mulUnit(t, x.a, xi_a);
		FpDbl::sub(t, t, x.b);
		FpDbl::mulUnit(y.b, x.b, xi_a);
		FpDbl::add(y.b, y.b, x.a);
		y.a = t;
	}
};

template<class Fp> Fp2T<Fp> Fp2T<Fp>::g[Fp2T<Fp>::gN];
template<class Fp> Fp2T<Fp> Fp2T<Fp>::g2[Fp2T<Fp>::gN];
template<class Fp> Fp2T<Fp> Fp2T<Fp>::g3[Fp2T<Fp>::gN];

template<class Fp>
struct Fp6DblT;
/*
	Fp6T = Fp2[v] / (v^3 - xi)
	x = a + b v + c v^2
*/
template<class _Fp>
struct Fp6T : public fp::Serializable<Fp6T<_Fp>,
	fp::Operator<Fp6T<_Fp> > > {
	typedef _Fp Fp;
	typedef Fp2T<Fp> Fp2;
	typedef Fp2DblT<Fp> Fp2Dbl;
	typedef Fp6DblT<Fp> Fp6Dbl;
	typedef Fp BaseFp;
	Fp2 a, b, c;
	Fp6T() { }
	Fp6T(int64_t a) : a(a) , b(0) , c(0) { }
	Fp6T(const Fp2& a, const Fp2& b, const Fp2& c) : a(a) , b(b) , c(c) { }
	void clear()
	{
		a.clear();
		b.clear();
		c.clear();
	}
	Fp* getFp0() { return a.getFp0(); }
	const Fp* getFp0() const { return a.getFp0(); }
	Fp2* getFp2() { return &a; }
	const Fp2* getFp2() const { return &a; }
	void set(const Fp2 &a_, const Fp2 &b_, const Fp2 &c_)
	{
		a = a_;
		b = b_;
		c = c_;
	}
	bool isZero() const
	{
		return a.isZero() && b.isZero() && c.isZero();
	}
	bool isOne() const
	{
		return a.isOne() && b.isZero() && c.isZero();
	}
	bool operator==(const Fp6T& rhs) const
	{
		return a == rhs.a && b == rhs.b && c == rhs.c;
	}
	bool operator!=(const Fp6T& rhs) const { return !operator==(rhs); }
	template<class InputStream>
	void load(bool *pb, InputStream& is, int ioMode)
	{
		a.load(pb, is, ioMode); if (!*pb) return;
		b.load(pb, is, ioMode); if (!*pb) return;
		c.load(pb, is, ioMode); if (!*pb) return;
	}
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int ioMode) const
	{
		const char sep = *fp::getIoSeparator(ioMode);
		a.save(pb, os, ioMode); if (!*pb) return;
		if (sep) {
			cybozu::writeChar(pb, os, sep);
			if (!*pb) return;
		}
		b.save(pb, os, ioMode); if (!*pb) return;
		if (sep) {
			cybozu::writeChar(pb, os, sep);
			if (!*pb) return;
		}
		c.save(pb, os, ioMode);
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	template<class InputStream>
	void load(InputStream& is, int ioMode = IoSerialize)
	{
		bool b;
		load(&b, is, ioMode);
		if (!b) throw cybozu::Exception("Fp6T:load");
	}
	template<class OutputStream>
	void save(OutputStream& os, int ioMode = IoSerialize) const
	{
		bool b;
		save(&b, os, ioMode);
		if (!b) throw cybozu::Exception("Fp6T:save");
	}
#endif
#ifndef CYBOZU_DONT_USE_STRING
	friend std::istream& operator>>(std::istream& is, Fp6T& self)
	{
		self.load(is, fp::detectIoMode(Fp::BaseFp::getIoMode(), is));
		return is;
	}
	friend std::ostream& operator<<(std::ostream& os, const Fp6T& self)
	{
		self.save(os, fp::detectIoMode(Fp::BaseFp::getIoMode(), os));
		return os;
	}
#endif
	static void add(Fp6T& z, const Fp6T& x, const Fp6T& y)
	{
		Fp2::add(z.a, x.a, y.a);
		Fp2::add(z.b, x.b, y.b);
		Fp2::add(z.c, x.c, y.c);
	}
	static void sub(Fp6T& z, const Fp6T& x, const Fp6T& y)
	{
		Fp2::sub(z.a, x.a, y.a);
		Fp2::sub(z.b, x.b, y.b);
		Fp2::sub(z.c, x.c, y.c);
	}
	static void neg(Fp6T& y, const Fp6T& x)
	{
		Fp2::neg(y.a, x.a);
		Fp2::neg(y.b, x.b);
		Fp2::neg(y.c, x.c);
	}
	static void mul2(Fp6T& y, const Fp6T& x)
	{
		Fp2::mul2(y.a, x.a);
		Fp2::mul2(y.b, x.b);
		Fp2::mul2(y.c, x.c);
	}
	static void sqr(Fp6T& y, const Fp6T& x)
	{
		Fp6Dbl XX;
		Fp6Dbl::sqrPre(XX, x);
		Fp6Dbl::mod(y, XX);
	}
	static inline void mul(Fp6T& z, const Fp6T& x, const Fp6T& y)
	{
		Fp6Dbl XY;
		Fp6Dbl::mulPre(XY, x, y);
		Fp6Dbl::mod(z, XY);
	}
	/*
		x = a + bv + cv^2, v^3 = xi
		y = 1/x = p/q where
		p = (a^2 - bc xi) + (c^2 xi - ab)v + (b^2 - ac)v^2
		q = c^3 xi^2 + b(b^2 - 3ac)xi + a^3
		  = (a^2 - bc xi)a + ((c^2 xi - ab)c + (b^2 - ac)b) xi
	*/
	static void inv(Fp6T& y, const Fp6T& x)
	{
		const Fp2& a = x.a;
		const Fp2& b = x.b;
		const Fp2& c = x.c;
		Fp2Dbl aa, bb, cc, ab, bc, ac;
		Fp2Dbl::sqrPre(aa, a);
		Fp2Dbl::sqrPre(bb, b);
		Fp2Dbl::sqrPre(cc, c);
		Fp2Dbl::mulPre(ab, a, b);
		Fp2Dbl::mulPre(bc, b, c);
		Fp2Dbl::mulPre(ac, c, a);

		Fp6T p;
		Fp2Dbl T;
		Fp2Dbl::mul_xi(T, bc);
		Fp2Dbl::sub(T, aa, T); // a^2 - bc xi
		Fp2Dbl::mod(p.a, T);
		Fp2Dbl::mul_xi(T, cc);
		Fp2Dbl::sub(T, T, ab); // c^2 xi - ab
		Fp2Dbl::mod(p.b, T);
		Fp2Dbl::sub(T, bb, ac); // b^2 - ac
		Fp2Dbl::mod(p.c, T);

		Fp2Dbl T2;
		Fp2Dbl::mulPre(T, p.b, c);
		Fp2Dbl::mulPre(T2, p.c, b);
		Fp2Dbl::add(T, T, T2);
		Fp2Dbl::mul_xi(T, T);
		Fp2Dbl::mulPre(T2, p.a, a);
		Fp2Dbl::addPre(T, T, T2);
		Fp2 q;
		Fp2Dbl::mod(q, T);
		Fp2::inv(q, q);

		Fp2::mul(y.a, p.a, q);
		Fp2::mul(y.b, p.b, q);
		Fp2::mul(y.c, p.c, q);
	}
};

template<class Fp>
struct Fp6DblT {
	typedef Fp2T<Fp> Fp2;
	typedef Fp6T<Fp> Fp6;
	typedef FpDblT<Fp> FpDbl;
	typedef Fp2DblT<Fp> Fp2Dbl;
	typedef Fp6DblT<Fp> Fp6Dbl;
	typedef fp::Unit Unit;
	Fp2Dbl a, b, c;
	static void add(Fp6Dbl& z, const Fp6Dbl& x, const Fp6Dbl& y)
	{
		Fp2Dbl::add(z.a, x.a, y.a);
		Fp2Dbl::add(z.b, x.b, y.b);
		Fp2Dbl::add(z.c, x.c, y.c);
	}
	static void sub(Fp6Dbl& z, const Fp6Dbl& x, const Fp6Dbl& y)
	{
		Fp2Dbl::sub(z.a, x.a, y.a);
		Fp2Dbl::sub(z.b, x.b, y.b);
		Fp2Dbl::sub(z.c, x.c, y.c);
	}
	/*
		x = a + bv + cv^2, y = d + ev + fv^2, v^3 = xi
		xy = (ad + (bf + ce)xi) + ((ae + bd) + cf xi)v + ((af + cd) + be)v^2
		bf + ce = (b + c)(e + f) - be - cf
		ae + bd = (a + b)(e + d) - ad - be
		af + cd = (a + c)(d + f) - ad - cf
		assum p < W/4 where W = 1 << (sizeof(Unit) * 8 * N)
		then (b + c)(e + f) < 4p^2 < pW
	*/
	static void mulPre(Fp6DblT& z, const Fp6& x, const Fp6& y)
	{
		const Fp2& a = x.a;
		const Fp2& b = x.b;
		const Fp2& c = x.c;
		const Fp2& d = y.a;
		const Fp2& e = y.b;
		const Fp2& f = y.c;
		Fp2Dbl& ZA = z.a;
		Fp2Dbl& ZB = z.b;
		Fp2Dbl& ZC = z.c;
		Fp2 t1, t2;
		Fp2Dbl BE, CF, AD;
		Fp2::addPre(t1, b, c);
		Fp2::addPre(t2, e, f);
		Fp2Dbl::mulPre(ZA, t1, t2);
		Fp2::addPre(t1, a, b);
		Fp2::addPre(t2, e, d);
		Fp2Dbl::mulPre(ZB, t1, t2);
		Fp2::addPre(t1, a, c);
		Fp2::addPre(t2, d, f);
		Fp2Dbl::mulPre(ZC, t1, t2);
		Fp2Dbl::mulPre(BE, b, e);
		Fp2Dbl::mulPre(CF, c, f);
		Fp2Dbl::mulPre(AD, a, d);
		Fp2Dbl::subSpecial(ZA, BE);
		Fp2Dbl::subSpecial(ZA, CF);
		Fp2Dbl::subSpecial(ZB, AD);
		Fp2Dbl::subSpecial(ZB, BE);
		Fp2Dbl::subSpecial(ZC, AD);
		Fp2Dbl::subSpecial(ZC, CF);
		Fp2Dbl::mul_xi(ZA, ZA);
		Fp2Dbl::add(ZA, ZA, AD);
		Fp2Dbl::mul_xi(CF, CF);
		Fp2Dbl::add(ZB, ZB, CF);
		Fp2Dbl::add(ZC, ZC, BE);
	}
	/*
		x = a + bv + cv^2, v^3 = xi
		x^2 = (a^2 + 2bc xi) + (c^2 xi + 2ab)v + (b^2 + 2ac)v^2

		b^2 + 2ac = (a + b + c)^2 - a^2 - 2bc - c^2 - 2ab
	*/
	static void sqrPre(Fp6DblT& y, const Fp6& x)
	{
		const Fp2& a = x.a;
		const Fp2& b = x.b;
		const Fp2& c = x.c;
		Fp2 t;
		Fp2Dbl BC2, AB2, AA, CC, T;
		Fp2::mul2(t, b);
		Fp2Dbl::mulPre(BC2, t, c); // 2bc
		Fp2Dbl::mulPre(AB2, t, a); // 2ab
		Fp2Dbl::sqrPre(AA, a);
		Fp2Dbl::sqrPre(CC, c);
		Fp2::add(t, a, b);
		Fp2::add(t, t, c);
		Fp2Dbl::sqrPre(T, t); // (a + b + c)^2
		Fp2Dbl::sub(T, T, AA);
		Fp2Dbl::sub(T, T, BC2);
		Fp2Dbl::sub(T, T, CC);
		Fp2Dbl::sub(y.c, T, AB2);
		Fp2Dbl::mul_xi(BC2, BC2);
		Fp2Dbl::add(y.a, AA, BC2);
		Fp2Dbl::mul_xi(CC, CC);
		Fp2Dbl::add(y.b, CC, AB2);
	}
	static void mod(Fp6& y, const Fp6Dbl& x)
	{
		Fp2Dbl::mod(y.a, x.a);
		Fp2Dbl::mod(y.b, x.b);
		Fp2Dbl::mod(y.c, x.c);
	}
};

/*
	Fp12T = Fp6[w] / (w^2 - v)
	x = a + b w
*/
template<class Fp>
struct Fp12T : public fp::Serializable<Fp12T<Fp>,
	fp::Operator<Fp12T<Fp> > > {
	typedef Fp2T<Fp> Fp2;
	typedef Fp6T<Fp> Fp6;
	typedef Fp2DblT<Fp> Fp2Dbl;
	typedef Fp6DblT<Fp> Fp6Dbl;
	typedef Fp BaseFp;
	Fp6 a, b;
	Fp12T() {}
	Fp12T(int64_t a) : a(a), b(0) {}
	Fp12T(const Fp6& a, const Fp6& b) : a(a), b(b) {}
	void clear()
	{
		a.clear();
		b.clear();
	}
	void setOne()
	{
		clear();
		a.a.a = 1;
	}

	Fp* getFp0() { return a.getFp0(); }
	const Fp* getFp0() const { return a.getFp0(); }
	Fp2* getFp2() { return a.getFp2(); }
	const Fp2* getFp2() const { return a.getFp2(); }
	void set(const Fp2& v0, const Fp2& v1, const Fp2& v2, const Fp2& v3, const Fp2& v4, const Fp2& v5)
	{
		a.set(v0, v1, v2);
		b.set(v3, v4, v5);
	}

	bool isZero() const
	{
		return a.isZero() && b.isZero();
	}
	bool isOne() const
	{
		return a.isOne() && b.isZero();
	}
	bool operator==(const Fp12T& rhs) const
	{
		return a == rhs.a && b == rhs.b;
	}
	bool operator!=(const Fp12T& rhs) const { return !operator==(rhs); }
	static void add(Fp12T& z, const Fp12T& x, const Fp12T& y)
	{
		Fp6::add(z.a, x.a, y.a);
		Fp6::add(z.b, x.b, y.b);
	}
	static void sub(Fp12T& z, const Fp12T& x, const Fp12T& y)
	{
		Fp6::sub(z.a, x.a, y.a);
		Fp6::sub(z.b, x.b, y.b);
	}
	static void neg(Fp12T& z, const Fp12T& x)
	{
		Fp6::neg(z.a, x.a);
		Fp6::neg(z.b, x.b);
	}
	/*
		z = x v + y
		in Fp6 : (a + bv + cv^2)v = cv^3 + av + bv^2 = cxi + av + bv^2
	*/
	static void mulVadd(Fp6& z, const Fp6& x, const Fp6& y)
	{
		Fp2 t;
		Fp2::mul_xi(t, x.c);
		Fp2::add(z.c, x.b, y.c);
		Fp2::add(z.b, x.a, y.b);
		Fp2::add(z.a, t, y.a);
	}
	static void mulVadd(Fp6Dbl& z, const Fp6Dbl& x, const Fp6Dbl& y)
	{
		Fp2Dbl t;
		Fp2Dbl::mul_xi(t, x.c);
		Fp2Dbl::add(z.c, x.b, y.c);
		Fp2Dbl::add(z.b, x.a, y.b);
		Fp2Dbl::add(z.a, t, y.a);
	}
	/*
		x = a + bw, y = c + dw, w^2 = v
		z = xy = (a + bw)(c + dw) = (ac + bdv) + (ad + bc)w
		ad+bc = (a + b)(c + d) - ac - bd

		in Fp6 : (a + bv + cv^2)v = cv^3 + av + bv^2 = cxi + av + bv^2
	*/
	static void mul(Fp12T& z, const Fp12T& x, const Fp12T& y)
	{
		// 4.7Kclk -> 4.55Kclk
		const Fp6& a = x.a;
		const Fp6& b = x.b;
		const Fp6& c = y.a;
		const Fp6& d = y.b;
		Fp6 t1, t2;
		Fp6::add(t1, a, b);
		Fp6::add(t2, c, d);
		Fp6Dbl T, AC, BD;
		Fp6Dbl::mulPre(AC, a, c);
		Fp6Dbl::mulPre(BD, b, d);
		mulVadd(T, BD, AC);
		Fp6Dbl::mod(z.a, T);
		Fp6Dbl::mulPre(T, t1, t2); // (a + b)(c + d)
		Fp6Dbl::sub(T, T, AC);
		Fp6Dbl::sub(T, T, BD);
		Fp6Dbl::mod(z.b, T);
	}
	/*
		x = a + bw, w^2 = v
		y = x^2 = (a + bw)^2 = (a^2 + b^2v) + 2abw
		a^2 + b^2v = (a + b)(bv + a) - (abv + ab)
	*/
	static void sqr(Fp12T& y, const Fp12T& x)
	{
		const Fp6& a = x.a;
		const Fp6& b = x.b;
		Fp6 t0, t1;
		Fp6::add(t0, a, b); // a + b
		mulVadd(t1, b, a); // bv + a
		t0 *= t1; // (a + b)(bv + a)
		Fp6::mul(t1, a, b); // ab
		Fp6::mul2(y.b, t1); // 2ab
		mulVadd(y.a, t1, t1); // abv + ab
		Fp6::sub(y.a, t0, y.a);
	}
	/*
		x = a + bw, w^2 = v
		y = 1/x = (a - bw) / (a^2 - b^2v)
	*/
	static void inv(Fp12T& y, const Fp12T& x)
	{
		const Fp6& a = x.a;
		const Fp6& b = x.b;
		Fp6Dbl AA, BB;
		Fp6Dbl::sqrPre(AA, a);
		Fp6Dbl::sqrPre(BB, b);
		Fp2Dbl::mul_xi(BB.c, BB.c);
		Fp2Dbl::sub(AA.a, AA.a, BB.c);
		Fp2Dbl::sub(AA.b, AA.b, BB.a);
		Fp2Dbl::sub(AA.c, AA.c, BB.b); // a^2 - b^2 v
		Fp6 t;
		Fp6Dbl::mod(t, AA);
		Fp6::inv(t, t);
		Fp6::mul(y.a, x.a, t);
		Fp6::mul(y.b, x.b, t);
		Fp6::neg(y.b, y.b);
	}
	/*
		y = 1 / x = conjugate of x if |x| = 1
	*/
	static void unitaryInv(Fp12T& y, const Fp12T& x)
	{
		if (&y != &x) y.a = x.a;
		Fp6::neg(y.b, x.b);
	}
	/*
		Frobenius
		i^2 = -1
		(a + bi)^p = a + bi^p in Fp
		= a + bi if p = 1 mod 4
		= a - bi if p = 3 mod 4

		g = xi^(p - 1) / 6
		v^3 = xi in Fp2
		v^p = ((v^6) ^ (p-1)/6) v = g^2 v
		v^2p = g^4 v^2
		(a + bv + cv^2)^p in Fp6
		= F(a) + F(b)g^2 v + F(c) g^4 v^2

		w^p = ((w^6) ^ (p-1)/6) w = g w
		((a + bv + cv^2)w)^p in Fp12T
		= (F(a) g + F(b) g^3 v + F(c) g^5 v^2)w
	*/
	static void Frobenius(Fp12T& y, const Fp12T& x)
	{
		for (int i = 0; i < 6; i++) {
			Fp2::Frobenius(y.getFp2()[i], x.getFp2()[i]);
		}
		for (int i = 1; i < 6; i++) {
			y.getFp2()[i] *= Fp2::get_gTbl()[i - 1];
		}
	}
	static void Frobenius2(Fp12T& y, const Fp12T& x)
	{
#if 0
		Frobenius(y, x);
		Frobenius(y, y);
#else
		y.getFp2()[0] = x.getFp2()[0];
		if (Fp::getOp().pmod4 == 1) {
			for (int i = 1; i < 6; i++) {
				Fp2::mul(y.getFp2()[i], x.getFp2()[i], Fp2::get_g2Tbl()[i]);
			}
		} else {
			for (int i = 1; i < 6; i++) {
				Fp2::mulFp(y.getFp2()[i], x.getFp2()[i], Fp2::get_g2Tbl()[i - 1].a);
			}
		}
#endif
	}
	static void Frobenius3(Fp12T& y, const Fp12T& x)
	{
#if 0
		Frobenius(y, x);
		Frobenius(y, y);
		Frobenius(y, y);
#else
		Fp2::Frobenius(y.getFp2()[0], x.getFp2()[0]);
		for (int i = 1; i < 6; i++) {
			Fp2::Frobenius(y.getFp2()[i], x.getFp2()[i]);
			y.getFp2()[i] *= Fp2::get_g3Tbl()[i - 1];
		}
#endif
	}
	template<class InputStream>
	void load(bool *pb, InputStream& is, int ioMode)
	{
		a.load(pb, is, ioMode); if (!*pb) return;
		b.load(pb, is, ioMode);
	}
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int ioMode) const
	{
		const char sep = *fp::getIoSeparator(ioMode);
		a.save(pb, os, ioMode); if (!*pb) return;
		if (sep) {
			cybozu::writeChar(pb, os, sep);
			if (!*pb) return;
		}
		b.save(pb, os, ioMode);
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	template<class InputStream>
	void load(InputStream& is, int ioMode = IoSerialize)
	{
		bool b;
		load(&b, is, ioMode);
		if (!b) throw cybozu::Exception("Fp12T:load");
	}
	template<class OutputStream>
	void save(OutputStream& os, int ioMode = IoSerialize) const
	{
		bool b;
		save(&b, os, ioMode);
		if (!b) throw cybozu::Exception("Fp12T:save");
	}
#endif
#ifndef CYBOZU_DONT_USE_STRING
	friend std::istream& operator>>(std::istream& is, Fp12T& self)
	{
		self.load(is, fp::detectIoMode(Fp::BaseFp::getIoMode(), is));
		return is;
	}
	friend std::ostream& operator<<(std::ostream& os, const Fp12T& self)
	{
		self.save(os, fp::detectIoMode(Fp::BaseFp::getIoMode(), os));
		return os;
	}
#endif
};

/*
	convert multiplicative group to additive group
*/
template<class T>
struct GroupMtoA : public T {
	static T& castT(GroupMtoA& x) { return static_cast<T&>(x); }
	static const T& castT(const GroupMtoA& x) { return static_cast<const T&>(x); }
	void clear()
	{
		castT(*this) = 1;
	}
	bool isZero() const { return castT(*this).isOne(); }
	static void add(GroupMtoA& z, const GroupMtoA& x, const GroupMtoA& y)
	{
		T::mul(castT(z), castT(x), castT(y));
	}
	static void sub(GroupMtoA& z, const GroupMtoA& x, const GroupMtoA& y)
	{
		T r;
		T::unitaryInv(r, castT(y));
		T::mul(castT(z), castT(x), r);
	}
	static void dbl(GroupMtoA& y, const GroupMtoA& x)
	{
		T::sqr(castT(y), castT(x));
	}
	static void neg(GroupMtoA& y, const GroupMtoA& x)
	{
		// assume Fp12
		T::unitaryInv(castT(y), castT(x));
	}
	static void Frobenus(GroupMtoA& y, const GroupMtoA& x)
	{
		T::Frobenius(castT(y), castT(x));
	}
	template<class INT>
	static void mul(GroupMtoA& z, const GroupMtoA& x, const INT& y)
	{
		T::pow(castT(z), castT(x), y);
	}
	template<class INT>
	static void mulGeneric(GroupMtoA& z, const GroupMtoA& x, const INT& y)
	{
		T::powGeneric(castT(z), castT(x), y);
	}
	void operator+=(const GroupMtoA& rhs)
	{
		add(*this, *this, rhs);
	}
	void operator-=(const GroupMtoA& rhs)
	{
		sub(*this, *this, rhs);
	}
	void normalize() {}
private:
	bool isOne() const;
};

} // mcl

