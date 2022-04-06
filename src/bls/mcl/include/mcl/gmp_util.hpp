#pragma once
/**
	@file
	@brief util function for gmp
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <cybozu/bit_operation.hpp>
#ifndef CYBOZU_DONT_USE_EXCEPTION
#include <cybozu/exception.hpp>
#endif
#include <mcl/randgen.hpp>
#include <mcl/config.hpp>
#include <mcl/conversion.hpp>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4616)
	#pragma warning(disable : 4800)
	#pragma warning(disable : 4244)
	#pragma warning(disable : 4127)
	#pragma warning(disable : 4512)
	#pragma warning(disable : 4146)
#endif
#if defined(__EMSCRIPTEN__) || defined(__wasm__)
	#define MCL_USE_VINT
#endif
#ifdef MCL_USE_VINT
#include <mcl/vint.hpp>
typedef mcl::Vint mpz_class;
#else
#include <gmpxx.h>
#ifdef _MSC_VER
	#pragma warning(pop)
	#include <cybozu/link_mpir.hpp>
#endif
#endif

namespace mcl {

namespace gmp {

typedef mpz_class ImplType;

// z = [buf[n-1]:..:buf[1]:buf[0]]
// eg. buf[] = {0x12345678, 0xaabbccdd}; => z = 0xaabbccdd12345678;
template<class T>
void setArray(bool *pb, mpz_class& z, const T *buf, size_t n)
{
#ifdef MCL_USE_VINT
	z.setArray(pb, buf, n);
#else
	mpz_import(z.get_mpz_t(), n, -1, sizeof(*buf), 0, 0, buf);
	*pb = true;
#endif
}
/*
	buf[0, size) = x
	buf[size, maxSize) with zero
*/
template<class T>
void getArray(bool *pb, T *buf, size_t maxSize, const mpz_class& x)
{
#ifdef MCL_USE_VINT
	const fp::Unit *src = x.getUnit();
	const size_t n = x.getUnitSize();
	*pb = fp::convertArrayAsLE(buf, maxSize, src, n);
#else
	int n = x.get_mpz_t()->_mp_size;
	if (n < 0) {
		*pb = false;
		return;
	}
	*pb = fp::convertArrayAsLE(buf, maxSize, x.get_mpz_t()->_mp_d, n);
#endif
}
inline void set(mpz_class& z, uint64_t x)
{
	bool b;
	setArray(&b, z, &x, 1);
	assert(b);
	(void)b;
}
inline void setStr(bool *pb, mpz_class& z, const char *str, int base = 0)
{
#ifdef MCL_USE_VINT
	z.setStr(pb, str, base);
#else
	*pb = z.set_str(str, base) == 0;
#endif
}

/*
	set buf with string terminated by '\0'
	return strlen(buf) if success else 0
*/
inline size_t getStr(char *buf, size_t bufSize, const mpz_class& z, int base = 10)
{
#ifdef MCL_USE_VINT
	return z.getStr(buf, bufSize, base);
#else
	__gmp_alloc_cstring tmp(mpz_get_str(0, base, z.get_mpz_t()));
	size_t n = strlen(tmp.str);
	if (n + 1 > bufSize) return 0;
	memcpy(buf, tmp.str, n + 1);
	return n;
#endif
}

#ifndef CYBOZU_DONT_USE_STRING
inline void getStr(std::string& str, const mpz_class& z, int base = 10)
{
#ifdef MCL_USE_VINT
	z.getStr(str, base);
#else
	str = z.get_str(base);
#endif
}
inline std::string getStr(const mpz_class& z, int base = 10)
{
	std::string s;
	gmp::getStr(s, z, base);
	return s;
}
#endif

inline void add(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
#ifdef MCL_USE_VINT
	Vint::add(z, x, y);
#else
	mpz_add(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
#endif
}
#ifndef MCL_USE_VINT
inline void add(mpz_class& z, const mpz_class& x, unsigned int y)
{
	mpz_add_ui(z.get_mpz_t(), x.get_mpz_t(), y);
}
inline void sub(mpz_class& z, const mpz_class& x, unsigned int y)
{
	mpz_sub_ui(z.get_mpz_t(), x.get_mpz_t(), y);
}
inline void mul(mpz_class& z, const mpz_class& x, unsigned int y)
{
	mpz_mul_ui(z.get_mpz_t(), x.get_mpz_t(), y);
}
inline void div(mpz_class& q, const mpz_class& x, unsigned int y)
{
	mpz_div_ui(q.get_mpz_t(), x.get_mpz_t(), y);
}
inline void mod(mpz_class& r, const mpz_class& x, unsigned int m)
{
	mpz_mod_ui(r.get_mpz_t(), x.get_mpz_t(), m);
}
inline int compare(const mpz_class& x, int y)
{
	return mpz_cmp_si(x.get_mpz_t(), y);
}
#endif
inline void sub(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
#ifdef MCL_USE_VINT
	Vint::sub(z, x, y);
#else
	mpz_sub(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
#endif
}
inline void mul(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
#ifdef MCL_USE_VINT
	Vint::mul(z, x, y);
#else
	mpz_mul(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
#endif
}
inline void sqr(mpz_class& z, const mpz_class& x)
{
#ifdef MCL_USE_VINT
	Vint::mul(z, x, x);
#else
	mpz_mul(z.get_mpz_t(), x.get_mpz_t(), x.get_mpz_t());
#endif
}
inline void divmod(mpz_class& q, mpz_class& r, const mpz_class& x, const mpz_class& y)
{
#ifdef MCL_USE_VINT
	Vint::divMod(&q, r, x, y);
#else
	mpz_divmod(q.get_mpz_t(), r.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
#endif
}
inline void div(mpz_class& q, const mpz_class& x, const mpz_class& y)
{
#ifdef MCL_USE_VINT
	Vint::div(q, x, y);
#else
	mpz_div(q.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
#endif
}
inline void mod(mpz_class& r, const mpz_class& x, const mpz_class& m)
{
#ifdef MCL_USE_VINT
	Vint::mod(r, x, m);
#else
	mpz_mod(r.get_mpz_t(), x.get_mpz_t(), m.get_mpz_t());
#endif
}
inline void clear(mpz_class& z)
{
#ifdef MCL_USE_VINT
	z.clear();
#else
	mpz_set_ui(z.get_mpz_t(), 0);
#endif
}
inline bool isZero(const mpz_class& z)
{
#ifdef MCL_USE_VINT
	return z.isZero();
#else
	return mpz_sgn(z.get_mpz_t()) == 0;
#endif
}
inline bool isNegative(const mpz_class& z)
{
#ifdef MCL_USE_VINT
	return z.isNegative();
#else
	return mpz_sgn(z.get_mpz_t()) < 0;
#endif
}
inline void neg(mpz_class& z, const mpz_class& x)
{
#ifdef MCL_USE_VINT
	Vint::neg(z, x);
#else
	mpz_neg(z.get_mpz_t(), x.get_mpz_t());
#endif
}
inline int compare(const mpz_class& x, const mpz_class & y)
{
#ifdef MCL_USE_VINT
	return Vint::compare(x, y);
#else
	return mpz_cmp(x.get_mpz_t(), y.get_mpz_t());
#endif
}
template<class T>
void addMod(mpz_class& z, const mpz_class& x, const T& y, const mpz_class& m)
{
	add(z, x, y);
	if (compare(z, m) >= 0) {
		sub(z, z, m);
	}
}
template<class T>
void subMod(mpz_class& z, const mpz_class& x, const T& y, const mpz_class& m)
{
	sub(z, x, y);
	if (!isNegative(z)) return;
	add(z, z, m);
}
template<class T>
void mulMod(mpz_class& z, const mpz_class& x, const T& y, const mpz_class& m)
{
	mul(z, x, y);
	mod(z, z, m);
}
inline void sqrMod(mpz_class& z, const mpz_class& x, const mpz_class& m)
{
	sqr(z, x);
	mod(z, z, m);
}
// z = x^y (y >= 0)
inline void pow(mpz_class& z, const mpz_class& x, unsigned int y)
{
#ifdef MCL_USE_VINT
	Vint::pow(z, x, y);
#else
	mpz_pow_ui(z.get_mpz_t(), x.get_mpz_t(), y);
#endif
}
// z = x^y mod m (y >=0)
inline void powMod(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& m)
{
#ifdef MCL_USE_VINT
	Vint::powMod(z, x, y, m);
#else
	mpz_powm(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t(), m.get_mpz_t());
#endif
}
// z = 1/x mod m
inline void invMod(mpz_class& z, const mpz_class& x, const mpz_class& m)
{
#ifdef MCL_USE_VINT
	Vint::invMod(z, x, m);
#else
	mpz_invert(z.get_mpz_t(), x.get_mpz_t(), m.get_mpz_t());
#endif
}
// z = lcm(x, y)
inline void lcm(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
#ifdef MCL_USE_VINT
	Vint::lcm(z, x, y);
#else
	mpz_lcm(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
#endif
}
inline mpz_class lcm(const mpz_class& x, const mpz_class& y)
{
	mpz_class z;
	lcm(z, x, y);
	return z;
}
// z = gcd(x, y)
inline void gcd(mpz_class& z, const mpz_class& x, const mpz_class& y)
{
#ifdef MCL_USE_VINT
	Vint::gcd(z, x, y);
#else
	mpz_gcd(z.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
#endif
}
inline mpz_class gcd(const mpz_class& x, const mpz_class& y)
{
	mpz_class z;
	gcd(z, x, y);
	return z;
}
/*
	assume p : odd prime
	return  1 if x^2 = a mod p for some x
	return -1 if x^2 != a mod p for any x
*/
inline int legendre(const mpz_class& a, const mpz_class& p)
{
#ifdef MCL_USE_VINT
	return Vint::jacobi(a, p);
#else
	return mpz_legendre(a.get_mpz_t(), p.get_mpz_t());
#endif
}
inline bool isPrime(bool *pb, const mpz_class& x)
{
#ifdef MCL_USE_VINT
	return x.isPrime(pb, 32);
#else
	*pb = true;
	return mpz_probab_prime_p(x.get_mpz_t(), 32) != 0;
#endif
}
inline size_t getBitSize(const mpz_class& x)
{
#ifdef MCL_USE_VINT
	return x.getBitSize();
#else
	return mpz_sizeinbase(x.get_mpz_t(), 2);
#endif
}
inline bool testBit(const mpz_class& x, size_t pos)
{
#ifdef MCL_USE_VINT
	return x.testBit(pos);
#else
	return mpz_tstbit(x.get_mpz_t(), pos) != 0;
#endif
}
inline void resetBit(mpz_class& x, size_t pos)
{
#ifdef MCL_USE_VINT
	x.setBit(pos, false);
#else
	mpz_clrbit(x.get_mpz_t(), pos);
#endif
}
inline void setBit(mpz_class& x, size_t pos, bool v = true)
{
#ifdef MCL_USE_VINT
	x.setBit(pos, v);
#else
	if (v) {
		mpz_setbit(x.get_mpz_t(), pos);
	} else {
		resetBit(x, pos);
	}
#endif
}
inline const fp::Unit *getUnit(const mpz_class& x)
{
#ifdef MCL_USE_VINT
	return x.getUnit();
#else
	return reinterpret_cast<const fp::Unit*>(x.get_mpz_t()->_mp_d);
#endif
}
inline fp::Unit getUnit(const mpz_class& x, size_t i)
{
	return getUnit(x)[i];
}
inline size_t getUnitSize(const mpz_class& x)
{
#ifdef MCL_USE_VINT
	return x.getUnitSize();
#else
	return std::abs(x.get_mpz_t()->_mp_size);
#endif
}

/*
	get the number of lower zeros
*/
template<class T>
size_t getLowerZeroBitNum(const T *x, size_t n)
{
	size_t ret = 0;
	for (size_t i = 0; i < n; i++) {
		T v = x[i];
		if (v == 0) {
			ret += sizeof(T) * 8;
		} else {
			ret += cybozu::bsf<T>(v);
			break;
		}
	}
	return ret;
}

/*
	get the number of lower zero
	@note x != 0
*/
inline size_t getLowerZeroBitNum(const mpz_class& x)
{
	assert(!isZero(x));
	return getLowerZeroBitNum(getUnit(x), getUnitSize(x));
}

inline mpz_class abs(const mpz_class& x)
{
#ifdef MCL_USE_VINT
	return Vint::abs(x);
#else
	return ::abs(x);
#endif
}

inline void getRand(bool *pb, mpz_class& z, size_t bitSize, fp::RandGen rg = fp::RandGen())
{
	if (rg.isZero()) rg = fp::RandGen::get();
	assert(bitSize > 1);
	const size_t rem = bitSize & 31;
	const size_t n = (bitSize + 31) / 32;
	uint32_t buf[128];
	assert(n <= CYBOZU_NUM_OF_ARRAY(buf));
	if (n > CYBOZU_NUM_OF_ARRAY(buf)) {
		*pb = false;
		return;
	}
	rg.read(pb, buf, n * sizeof(buf[0]));
	if (!*pb) return;
	uint32_t v = buf[n - 1];
	if (rem == 0) {
		v |= 1U << 31;
	} else {
		v &= (1U << rem) - 1;
		v |= 1U << (rem - 1);
	}
	buf[n - 1] = v;
	setArray(pb, z, buf, n);
}

inline void getRandPrime(bool *pb, mpz_class& z, size_t bitSize, fp::RandGen rg = fp::RandGen(), bool setSecondBit = false, bool mustBe3mod4 = false)
{
	if (rg.isZero()) rg = fp::RandGen::get();
	assert(bitSize > 2);
	for (;;) {
		getRand(pb, z, bitSize, rg);
		if (!*pb) return;
		z |= 1; // odd
		if (setSecondBit) {
			z |= mpz_class(1) << (bitSize - 2);
		}
		if (mustBe3mod4) {
			z |= 3;
		}
		bool ret = isPrime(pb, z);
		if (!*pb) return;
		if (ret) return;
	}
}
inline mpz_class getQuadraticNonResidue(const mpz_class& p)
{
	mpz_class g = 2;
	while (legendre(g, p) > 0) {
		++g;
	}
	return g;
}

namespace impl {

template<class Vec>
void convertToBinary(Vec& v, const mpz_class& x)
{
	const size_t len = gmp::getBitSize(x);
	v.resize(len);
	for (size_t i = 0; i < len; i++) {
		v[i] = gmp::testBit(x, len - 1 - i) ? 1 : 0;
	}
}

template<class Vec>
size_t getContinuousVal(const Vec& v, size_t pos, int val)
{
	while (pos >= 2) {
		if (v[pos] != val) break;
		pos--;
	}
	return pos;
}

template<class Vec>
void convertToNAF(Vec& v, const Vec& in)
{
	v.copy(in);
	size_t pos = v.size() - 1;
	for (;;) {
		size_t p = getContinuousVal(v, pos, 0);
		if (p == 1) return;
		assert(v[p] == 1);
		size_t q = getContinuousVal(v, p, 1);
		if (q == 1) return;
		assert(v[q] == 0);
		if (p - q <= 1) {
			pos = p - 1;
			continue;
		}
		v[q] = 1;
		for (size_t i = q + 1; i < p; i++) {
			v[i] = 0;
		}
		v[p] = -1;
		pos = q;
	}
}

template<class Vec>
size_t getNumOfNonZeroElement(const Vec& v)
{
	size_t w = 0;
	for (size_t i = 0; i < v.size(); i++) {
		if (v[i]) w++;
	}
	return w;
}

} // impl

/*
	compute a repl of x which has smaller Hamming weights.
	return true if naf is selected
*/
template<class Vec>
bool getNAF(Vec& v, const mpz_class& x)
{
	Vec bin;
	impl::convertToBinary(bin, x);
	Vec naf;
	impl::convertToNAF(naf, bin);
	const size_t binW = impl::getNumOfNonZeroElement(bin);
	const size_t nafW = impl::getNumOfNonZeroElement(naf);
	if (nafW < binW) {
		v.swap(naf);
		return true;
	} else {
		v.swap(bin);
		return false;
	}
}

/*
	v = naf[i]
	v = 0 or (|v| <= 2^(w-1) - 1 and odd)
*/
template<class Vec>
void getNAFwidth(bool *pb, Vec& naf, mpz_class x, size_t w)
{
	assert(w > 0);
	*pb = true;
	naf.clear();
	bool negative = false;
	if (x < 0) {
		negative = true;
		x = -x;
	}
	size_t zeroNum = 0;
	const int signedMaxW = 1 << (w - 1);
	const int maxW = signedMaxW * 2;
	const int maskW = maxW - 1;
	while (!isZero(x)) {
		size_t z = gmp::getLowerZeroBitNum(x);
		if (z) {
			x >>= z;
			zeroNum += z;
		}
		for (size_t i = 0; i < zeroNum; i++) {
			naf.push(pb, 0);
			if (!*pb) return;
		}
		assert(!isZero(x));
		int v = getUnit(x)[0] & maskW;
		x >>= w;
		if (v & signedMaxW) {
			x++;
			v -= maxW;
		}
		naf.push(pb, typename Vec::value_type(v));
		if (!*pb) return;
		zeroNum = w - 1;
	}
	if (negative) {
		for (size_t i = 0; i < naf.size(); i++) {
			naf[i] = -naf[i];
		}
	}
}

#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void setStr(mpz_class& z, const std::string& str, int base = 0)
{
	bool b;
	setStr(&b, z, str.c_str(), base);
	if (!b) throw cybozu::Exception("gmp:setStr");
}
template<class T>
void setArray(mpz_class& z, const T *buf, size_t n)
{
	bool b;
	setArray(&b, z, buf, n);
	if (!b) throw cybozu::Exception("gmp:setArray");
}
template<class T>
void getArray(T *buf, size_t maxSize, const mpz_class& x)
{
	bool b;
	getArray(&b, buf, maxSize, x);
	if (!b) throw cybozu::Exception("gmp:getArray");
}
inline bool isPrime(const mpz_class& x)
{
	bool b;
	bool ret = isPrime(&b, x);
	if (!b) throw cybozu::Exception("gmp:isPrime");
	return ret;
}
inline void getRand(mpz_class& z, size_t bitSize, fp::RandGen rg = fp::RandGen())
{
	bool b;
	getRand(&b, z, bitSize, rg);
	if (!b) throw cybozu::Exception("gmp:getRand");
}
inline void getRandPrime(mpz_class& z, size_t bitSize, fp::RandGen rg = fp::RandGen(), bool setSecondBit = false, bool mustBe3mod4 = false)
{
	bool b;
	getRandPrime(&b, z, bitSize, rg, setSecondBit, mustBe3mod4);
	if (!b) throw cybozu::Exception("gmp:getRandPrime");
}
#endif


} // mcl::gmp

/*
	Tonelli-Shanks
*/
class SquareRoot {
	bool isPrecomputed_;
	bool isPrime;
	mpz_class p;
	mpz_class g;
	int r;
	mpz_class q; // p - 1 = 2^r q
	mpz_class s; // s = g^q
	mpz_class q_add_1_div_2;
	struct Tbl {
		const char *p;
		const char *g;
		int r;
		const char *q;
		const char *s;
		const char *q_add_1_div_2;
	};
	bool setIfPrecomputed(const mpz_class& p_)
	{
		static const Tbl tbl[] = {
			{ // BN254.p
				"2523648240000001ba344d80000000086121000000000013a700000000000013",
				"2",
				1,
				"1291b24120000000dd1a26c0000000043090800000000009d380000000000009",
				"2523648240000001ba344d80000000086121000000000013a700000000000012",
				"948d920900000006e8d1360000000021848400000000004e9c0000000000005",
			},
			{ // BN254.r
				"2523648240000001ba344d8000000007ff9f800000000010a10000000000000d",
				"2",
				2,
				"948d920900000006e8d136000000001ffe7e000000000042840000000000003",
				"9366c4800000000555150000000000122400000000000015",
				"4a46c9048000000374689b000000000fff3f000000000021420000000000002",
			},
			{ // BLS12_381,p
				"1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
				"2",
				1,
				"d0088f51cbff34d258dd3db21a5d66bb23ba5c279c2895fb39869507b587b120f55ffff58a9ffffdcff7fffffffd555",
				"1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
				"680447a8e5ff9a692c6e9ed90d2eb35d91dd2e13ce144afd9cc34a83dac3d8907aaffffac54ffffee7fbfffffffeaab",
			},
			{ // BLS12_381.r
				"73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001",
				"5",
				32,
				"73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff",
				"212d79e5b416b6f0fd56dc8d168d6c0c4024ff270b3e0941b788f500b912f1f",
				"39f6d3a994cebea4199cec0404d0ec02a9ded2017fff2dff80000000",
			},
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			mpz_class targetPrime;
			bool b;
			mcl::gmp::setStr(&b, targetPrime, tbl[i].p, 16);
			if (!b) continue;
			if (targetPrime != p_) continue;
			isPrime = true;
			p = p_;
			mcl::gmp::setStr(&b, g, tbl[i].g, 16);
			if (!b) continue;
			r = tbl[i].r;
			mcl::gmp::setStr(&b, q, tbl[i].q, 16);
			if (!b) continue;
			mcl::gmp::setStr(&b, s, tbl[i].s, 16);
			if (!b) continue;
			mcl::gmp::setStr(&b, q_add_1_div_2, tbl[i].q_add_1_div_2, 16);
			if (!b) continue;
			isPrecomputed_ = true;
			return true;
		}
		return false;
	}
public:
	SquareRoot() { clear(); }
	bool isPrecomputed() const { return isPrecomputed_; }
	void clear()
	{
		isPrecomputed_ = false;
		isPrime = false;
		p = 0;
		g = 0;
		r = 0;
		q = 0;
		s = 0;
		q_add_1_div_2 = 0;
	}
#if !defined(CYBOZU_DONT_USE_USE_STRING) && !defined(CYBOZU_DONT_USE_EXCEPTION)
	void dump() const
	{
		printf("\"%s\",\n", mcl::gmp::getStr(p, 16).c_str());
		printf("\"%s\",\n", mcl::gmp::getStr(g, 16).c_str());
		printf("%d,\n", r);
		printf("\"%s\",\n", mcl::gmp::getStr(q, 16).c_str());
		printf("\"%s\",\n", mcl::gmp::getStr(s, 16).c_str());
		printf("\"%s\",\n", mcl::gmp::getStr(q_add_1_div_2, 16).c_str());
	}
#endif
	void set(bool *pb, const mpz_class& _p, bool usePrecomputedTable = true)
	{
		if (usePrecomputedTable && setIfPrecomputed(_p)) {
			*pb = true;
			return;
		}
		p = _p;
		if (p <= 2) {
			*pb = false;
			return;
		}
		isPrime = gmp::isPrime(pb, p);
		if (!*pb) return;
		if (!isPrime) {
			*pb = false;
			return;
		}
		g = gmp::getQuadraticNonResidue(p);
		// p - 1 = 2^r q, q is odd
		r = 0;
		q = p - 1;
		while ((q & 1) == 0) {
			r++;
			q /= 2;
		}
		gmp::powMod(s, g, q, p);
		q_add_1_div_2 = (q + 1) / 2;
		*pb = true;
	}
	/*
		solve x^2 = a mod p
	*/
	bool get(mpz_class& x, const mpz_class& a) const
	{
		if (!isPrime) {
			return false;
		}
		if (a == 0) {
			x = 0;
			return true;
		}
		if (gmp::legendre(a, p) < 0) return false;
		if (r == 1) {
			// (p + 1) / 4 = (q + 1) / 2
			gmp::powMod(x, a, q_add_1_div_2, p);
			return true;
		}
		mpz_class c = s, d;
		int e = r;
		gmp::powMod(d, a, q, p);
		gmp::powMod(x, a, q_add_1_div_2, p); // destroy a if &x == &a
		mpz_class dd;
		mpz_class b;
		while (d != 1) {
			int i = 1;
			dd = d * d; dd %= p;
			while (dd != 1) {
				dd *= dd; dd %= p;
				i++;
			}
			b = 1;
			b <<= e - i - 1;
			gmp::powMod(b, c, b, p);
			x *= b; x %= p;
			c = b * b; c %= p;
			d *= c; d %= p;
			e = i;
		}
		return true;
	}
	/*
		solve x^2 = a in Fp
	*/
	template<class Fp>
	bool get(Fp& x, const Fp& a) const
	{
		assert(Fp::getOp().mp == p);
		if (a == 0) {
			x = 0;
			return true;
		}
		{
			bool b;
			mpz_class aa;
			a.getMpz(&b, aa);
			assert(b);
			if (gmp::legendre(aa, p) < 0) return false;
		}
		if (r == 1) {
			// (p + 1) / 4 = (q + 1) / 2
			Fp::pow(x, a, q_add_1_div_2);
			return true;
		}
		Fp c, d;
		{
			bool b;
			c.setMpz(&b, s);
			assert(b);
		}
		int e = r;
		Fp::pow(d, a, q);
		Fp::pow(x, a, q_add_1_div_2); // destroy a if &x == &a
		Fp dd;
		Fp b;
		while (!d.isOne()) {
			int i = 1;
			Fp::sqr(dd, d);
			while (!dd.isOne()) {
				dd *= dd;
				i++;
			}
			b = 1;
//			b <<= e - i - 1;
			for (int j = 0; j < e - i - 1; j++) {
				b += b;
			}
			Fp::pow(b, c, b);
			x *= b;
			Fp::sqr(c, b);
			d *= c;
			e = i;
		}
		return true;
	}
	bool operator==(const SquareRoot& rhs) const
	{
		return isPrime == rhs.isPrime && p == rhs.p && g == rhs.g && r == rhs.r
			&& q == rhs.q && s == rhs.s && q_add_1_div_2 == rhs.q_add_1_div_2;
	}
	bool operator!=(const SquareRoot& rhs) const { return !operator==(rhs); }
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void set(const mpz_class& _p)
	{
		bool b;
		set(&b, _p);
		if (!b) throw cybozu::Exception("gmp:SquareRoot:set");
	}
#endif
};

/*
	x mod p for a small value x < (pMulTblN * p).
*/
struct SmallModp {
	typedef mcl::fp::Unit Unit;
	static const size_t unitBitSize = sizeof(Unit) * 8;
	static const size_t maxTblSize = (MCL_MAX_BIT_SIZE + unitBitSize - 1) / unitBitSize + 1;
	static const size_t maxMulN = 9;
	static const size_t pMulTblN = maxMulN + 1;
	uint32_t N_;
	uint32_t shiftL_;
	uint32_t shiftR_;
	uint32_t maxIdx_;
	// pMulTbl_[i] = (p * i) >> (pBitSize_ - 1)
	Unit pMulTbl_[pMulTblN][maxTblSize];
	// idxTbl_[x] = (x << (pBitSize_ - 1)) / p
	uint8_t idxTbl_[pMulTblN * 2];
	// return x >> (pBitSize_ - 1)
	SmallModp()
		: N_(0)
		, shiftL_(0)
		, shiftR_(0)
		, maxIdx_(0)
		, pMulTbl_()
		, idxTbl_()
	{
	}
	// return argmax { i : x > i * p }
	uint32_t approxMul(const Unit *x) const
	{
		uint32_t top = getTop(x);
		assert(top <= maxIdx_);
		return idxTbl_[top];
	}
	const Unit *getPmul(size_t v) const
	{
		assert(v < pMulTblN);
		return pMulTbl_[v];
	}
	uint32_t getTop(const Unit *x) const
	{
		if (shiftR_ == 0) return x[N_ - 1];
		return (x[N_ - 1] >> shiftR_) | (x[N_] << shiftL_);
	}
	uint32_t cvtInt(const mpz_class& x) const
	{
		assert(mcl::gmp::getUnitSize(x) <= 1);
		if (x == 0) {
			return 0;
		} else {
			return uint32_t(mcl::gmp::getUnit(x)[0]);
		}
	}
	void init(const mpz_class& p)
	{
		size_t pBitSize = mcl::gmp::getBitSize(p);
		N_ = uint32_t((pBitSize + unitBitSize - 1) / unitBitSize);
		shiftR_ = (pBitSize - 1) % unitBitSize;
		shiftL_ = unitBitSize - shiftR_;
		mpz_class t = 0;
		for (size_t i = 0; i < pMulTblN; i++) {
			bool b;
			mcl::gmp::getArray(&b, pMulTbl_[i], maxTblSize, t);
			assert(b);
			(void)b;
			if (i == pMulTblN - 1) {
				maxIdx_ = getTop(pMulTbl_[i]);
				assert(maxIdx_ < CYBOZU_NUM_OF_ARRAY(idxTbl_));
				break;
			}
			t += p;
		}

		for (uint32_t i = 0; i <= maxIdx_; i++) {
			idxTbl_[i] = cvtInt((mpz_class(int(i)) << (pBitSize - 1)) / p);
		}
	}
};


/*
	Barrett Reduction
	for non GMP version
	mod of GMP is faster than Modp
*/
struct Modp {
	static const size_t unitBitSize = sizeof(mcl::fp::Unit) * 8;
	mpz_class p_;
	mpz_class u_;
	mpz_class a_;
	size_t pBitSize_;
	size_t N_;
	bool initU_; // Is u_ initialized?
	Modp()
		: pBitSize_(0)
		, N_(0)
		, initU_(false)
	{
	}
	// x &= 1 << (unitBitSize * unitSize)
	void shrinkSize(mpz_class &x, size_t unitSize) const
	{
		size_t u = gmp::getUnitSize(x);
		if (u < unitSize) return;
		bool b;
		gmp::setArray(&b, x, gmp::getUnit(x), unitSize);
		(void)b;
		assert(b);
	}
	// p_ is set by p and compute (u_, a_) if possible
	void init(const mpz_class& p)
	{
		p_ = p;
		pBitSize_ = gmp::getBitSize(p);
		N_ = (pBitSize_ + unitBitSize - 1) / unitBitSize;
		initU_ = false;
#if 0
		u_ = (mpz_class(1) << (unitBitSize * 2 * N_)) / p_;
#else
		/*
			1 << (unitBitSize * 2 * N_) may be overflow,
			so use (1 << (unitBitSize * 2 * N_)) - 1 because u_ is same.
		*/
		uint8_t buf[48 * 2];
		const size_t byteSize = unitBitSize / 8 * 2 * N_;
		if (byteSize > sizeof(buf)) return;
		memset(buf, 0xff, byteSize);
		bool b;
		gmp::setArray(&b, u_, buf, byteSize);
		if (!b) return;
#endif
		u_ /= p_;
		a_ = mpz_class(1) << (unitBitSize * (N_ + 1));
		initU_ = true;
	}
	void modp(mpz_class& r, const mpz_class& t) const
	{
		assert(p_ > 0);
		const size_t tBitSize = gmp::getBitSize(t);
		// use gmp::mod if init() fails or t is too large
		if (tBitSize > unitBitSize * 2 * N_ || !initU_) {
			gmp::mod(r, t, p_);
			return;
		}
		if (tBitSize < pBitSize_) {
			r = t;
			return;
		}
		// mod is faster than modp if t is small
		if (tBitSize <= unitBitSize * N_) {
			gmp::mod(r, t, p_);
			return;
		}
		mpz_class q;
		q = t;
		q >>= unitBitSize * (N_ - 1);
		q *= u_;
		q >>= unitBitSize * (N_ + 1);
		q *= p_;
		shrinkSize(q, N_ + 1);
		r = t;
		shrinkSize(r, N_ + 1);
		r -= q;
		if (r < 0) {
			r += a_;
		}
		if (r >= p_) {
			r -= p_;
		}
	}
};

} // mcl
