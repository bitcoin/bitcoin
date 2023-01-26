#pragma once
/**
	@file
	@brief generic function for each N
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mcl/op.hpp>
#include <mcl/util.hpp>
#include <cybozu/bit_operation.hpp>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4127)
#endif

#ifndef MCL_LLVM_BMI2
	#if (CYBOZU_HOST == CYBOZU_HOST_INTEL) && !defined(MCL_STATIC_CODE) && !defined(MCL_USE_VINT)
		#define MCL_LLVM_BMI2 1
	#else
		#define MCL_LLVM_BMI2 0
	#endif
#endif

namespace mcl { namespace fp {

struct Gtag; // GMP
struct Ltag; // LLVM
#if MCL_LLVM_BMI2 == 1
struct LBMI2tag; // LLVM with Intel BMI2 instruction
#endif
struct Atag; // asm

template<class Tag> struct TagToStr { };
template<> struct TagToStr<Gtag> { static const char *f() { return "Gtag"; } };
template<> struct TagToStr<Ltag> { static const char *f() { return "Ltag"; } };
#if MCL_LLVM_BMI2 == 1
template<> struct TagToStr<LBMI2tag> { static const char *f() { return "LBMI2tag"; } };
#endif
template<> struct TagToStr<Atag> { static const char *f() { return "Atag"; } };

template<size_t N>
void clearC(Unit *x)
{
	clearArray(x, 0, N);
}

template<size_t N>
bool isZeroC(const Unit *x)
{
	return isZeroArray(x, N);
}

template<size_t N>
void copyC(Unit *y, const Unit *x)
{
	copyArray(y, x, N);
}

// (carry, z[N]) <- x[N] + y[N]
template<size_t N, class Tag = Gtag>
struct AddPre {
	static inline Unit func(Unit *z, const Unit *x, const Unit *y)
	{
#ifdef MCL_USE_VINT
		return mcl::vint::addN(z, x, y, N);
#else
		return mpn_add_n((mp_limb_t*)z, (const mp_limb_t*)x, (const mp_limb_t*)y, N);
#endif
	}
	static const u3u f;
};
template<size_t N, class Tag>
const u3u AddPre<N, Tag>::f = AddPre<N, Tag>::func;

// (carry, x[N]) <- x[N] + y
template<class Tag = Gtag>
struct AddUnitPre {
	static inline Unit func(Unit *x, Unit n, Unit y)
	{
#if 1
		int ret = 0;
		Unit t = x[0] + y;
		x[0] = t;
		if (t >= y) goto EXIT_0;
		for (size_t i = 1; i < n; i++) {
			t = x[i] + 1;
			x[i] = t;
			if (t != 0) goto EXIT_0;
		}
		ret = 1;
	EXIT_0:
		return ret;
#else
		return mpn_add_1((mp_limb_t*)x, (const mp_limb_t*)x, (int)n, y);
#endif
	}
	static const u1uII f;
};
template<class Tag>
const u1uII AddUnitPre<Tag>::f = AddUnitPre<Tag>::func;

// (carry, z[N]) <- x[N] - y[N]
template<size_t N, class Tag = Gtag>
struct SubPre {
	static inline Unit func(Unit *z, const Unit *x, const Unit *y)
	{
#ifdef MCL_USE_VINT
		return mcl::vint::subN(z, x, y, N);
#else
		return mpn_sub_n((mp_limb_t*)z, (const mp_limb_t*)x, (const mp_limb_t*)y, N);
#endif
	}
	static const u3u f;
};

template<size_t N, class Tag>
const u3u SubPre<N, Tag>::f = SubPre<N, Tag>::func;

// y[N] <- (x[N] >> 1)
template<size_t N, class Tag = Gtag>
struct Shr1 {
	static inline void func(Unit *y, const Unit *x)
	{
#ifdef MCL_USE_VINT
		mcl::vint::shrN(y, x, N, 1);
#else
		mpn_rshift((mp_limb_t*)y, (const mp_limb_t*)x, (int)N, 1);
#endif
	}
	static const void2u f;
};

template<size_t N, class Tag>
const void2u Shr1<N, Tag>::f = Shr1<N, Tag>::func;

// y[N] <- (-x[N]) % p[N]
template<size_t N, class Tag = Gtag>
struct Neg {
	static inline void func(Unit *y, const Unit *x, const Unit *p)
	{
		if (isZeroC<N>(x)) {
			if (x != y) clearC<N>(y);
			return;
		}
		SubPre<N, Tag>::f(y, p, x);
	}
	static const void3u f;
};

template<size_t N, class Tag>
const void3u Neg<N, Tag>::f = Neg<N, Tag>::func;

// z[N * 2] <- x[N] * y[N]
template<size_t N, class Tag = Gtag>
struct MulPreCore {
	static inline void func(Unit *z, const Unit *x, const Unit *y)
	{
#ifdef MCL_USE_VINT
		mcl::vint::mulNM(z, x, N, y, N);
#else
		mpn_mul_n((mp_limb_t*)z, (const mp_limb_t*)x, (const mp_limb_t*)y, (int)N);
#endif
	}
	static const void3u f;
};

template<size_t N, class Tag>
const void3u MulPreCore<N, Tag>::f = MulPreCore<N, Tag>::func;

template<class Tag = Gtag>
struct EnableKaratsuba {
	/* always use mpn* for Gtag */
	static const size_t minMulN = 100;
	static const size_t minSqrN = 100;
};

template<size_t N, class Tag = Gtag>
struct MulPre {
	/*
		W = 1 << H
		x = aW + b, y = cW + d
		xy = acW^2 + (ad + bc)W + bd
		ad + bc = (a + b)(c + d) - ac - bd
	*/
	static inline void karatsuba(Unit *z, const Unit *x, const Unit *y)
	{
		const size_t H = N / 2;
		MulPre<H, Tag>::f(z, x, y); // bd
		MulPre<H, Tag>::f(z + N, x + H, y + H); // ac
		Unit a_b[H];
		Unit c_d[H];
		Unit c1 = AddPre<H, Tag>::f(a_b, x, x + H); // a + b
		Unit c2 = AddPre<H, Tag>::f(c_d, y, y + H); // c + d
		Unit tmp[N];
		MulPre<H, Tag>::f(tmp, a_b, c_d);
		Unit c = c1 & c2;
		if (c1) {
			c += AddPre<H, Tag>::f(tmp + H, tmp + H, c_d);
		}
		if (c2) {
			c += AddPre<H, Tag>::f(tmp + H, tmp + H, a_b);
		}
		// c:tmp[N] = (a + b)(c + d)
		c -= SubPre<N, Tag>::f(tmp, tmp, z);
		c -= SubPre<N, Tag>::f(tmp, tmp, z + N);
		// c:tmp[N] = ad + bc
		c += AddPre<N, Tag>::f(z + H, z + H, tmp);
		assert(c <= 2);
		if (c) {
			AddUnitPre<Tag>::f(z + N + H, H, c);
		}
	}
	static inline void func(Unit *z, const Unit *x, const Unit *y)
	{
#if 1
		if (N >= EnableKaratsuba<Tag>::minMulN && (N % 2) == 0) {
			karatsuba(z, x, y);
			return;
		}
#endif
		MulPreCore<N, Tag>::f(z, x, y);
	}
	static const void3u f;
};

template<size_t N, class Tag>
const void3u MulPre<N, Tag>::f = MulPre<N, Tag>::func;

template<class Tag>
struct MulPre<0, Tag> {
	static inline void f(Unit*, const Unit*, const Unit*) {}
};

template<class Tag>
struct MulPre<1, Tag> {
	static inline void f(Unit* z, const Unit* x, const Unit* y)
	{
		MulPreCore<1, Tag>::f(z, x, y);
	}
};

// z[N * 2] <- x[N] * x[N]
template<size_t N, class Tag = Gtag>
struct SqrPreCore {
	static inline void func(Unit *y, const Unit *x)
	{
#ifdef MCL_USE_VINT
		mcl::vint::sqrN(y, x, N);
#else
		mpn_sqr((mp_limb_t*)y, (const mp_limb_t*)x, N);
#endif
	}
	static const void2u f;
};

template<size_t N, class Tag>
const void2u SqrPreCore<N, Tag>::f = SqrPreCore<N, Tag>::func;

template<size_t N, class Tag = Gtag>
struct SqrPre {
	/*
		W = 1 << H
		x = aW + b
		x^2 = aaW^2 + 2abW + bb
	*/
	static inline void karatsuba(Unit *z, const Unit *x)
	{
		const size_t H = N / 2;
		SqrPre<H, Tag>::f(z, x); // b^2
		SqrPre<H, Tag>::f(z + N, x + H); // a^2
		Unit ab[N];
		MulPre<H, Tag>::f(ab, x, x + H); // ab
		Unit c = AddPre<N, Tag>::f(ab, ab, ab);
		c += AddPre<N, Tag>::f(z + H, z + H, ab);
		if (c) {
			AddUnitPre<Tag>::f(z + N + H, H, c);
		}
	}
	static inline void func(Unit *y, const Unit *x)
	{
#if 1
		if (N >= EnableKaratsuba<Tag>::minSqrN && (N % 2) == 0) {
			karatsuba(y, x);
			return;
		}
#endif
		SqrPreCore<N, Tag>::f(y, x);
	}
	static const void2u f;
};
template<size_t N, class Tag>
const void2u SqrPre<N, Tag>::f = SqrPre<N, Tag>::func;

template<class Tag>
struct SqrPre<0, Tag> {
	static inline void f(Unit*, const Unit*) {}
};

template<class Tag>
struct SqrPre<1, Tag> {
	static inline void f(Unit* y, const Unit* x)
	{
		SqrPreCore<1, Tag>::f(y, x);
	}
};

// z[N + 1] <- x[N] * y
template<size_t N, class Tag = Gtag>
struct MulUnitPre {
	static inline void func(Unit *z, const Unit *x, Unit y)
	{
#ifdef MCL_USE_VINT
		z[N] = mcl::vint::mulu1(z, x, N, y);
#else
		z[N] = mpn_mul_1((mp_limb_t*)z, (const mp_limb_t*)x, N, y);
#endif
	}
	static const void2uI f;
};

template<size_t N, class Tag>
const void2uI MulUnitPre<N, Tag>::f = MulUnitPre<N, Tag>::func;

// z[N] <- x[N + 1] % p[N]
template<size_t N, class Tag = Gtag>
struct N1_Mod {
	static inline void func(Unit *y, const Unit *x, const Unit *p)
	{
#ifdef MCL_USE_VINT
		mcl::vint::divNM<Unit>(0, 0, y, x, N + 1, p, N);
#else
		mp_limb_t q[2]; // not used
		mpn_tdiv_qr(q, (mp_limb_t*)y, 0, (const mp_limb_t*)x, N + 1, (const mp_limb_t*)p, N);
#endif
	}
	static const void3u f;
};

template<size_t N, class Tag>
const void3u N1_Mod<N, Tag>::f = N1_Mod<N, Tag>::func;

// z[N] <- (x[N] * y) % p[N]
template<size_t N, class Tag = Gtag>
struct MulUnit {
	static inline void func(Unit *z, const Unit *x, Unit y, const Unit *p)
	{
		Unit xy[N + 1];
		MulUnitPre<N, Tag>::f(xy, x, y);
#if 1
		Unit len = UnitBitSize - 1 - cybozu::bsr(p[N - 1]);
		Unit v = xy[N];
		if (N > 1 && len < 3 && v < 0xff) {
			for (;;) {
				if (len == 0) {
					v = xy[N];
				} else {
					v = (xy[N] << len) | (xy[N - 1] >> (UnitBitSize - len));
				}
				if (v == 0) break;
				if (v == 1) {
					xy[N] -= SubPre<N, Tag>::f(xy, xy, p);
				} else {
					Unit t[N + 1];
					MulUnitPre<N, Tag>::f(t, p, v);
					SubPre<N + 1, Tag>::f(xy, xy, t);
				}
			}
			for (;;) {
				if (SubPre<N, Tag>::f(z, xy, p)) {
					copyC<N>(z, xy);
					return;
				}
				if (SubPre<N, Tag>::f(xy, z, p)) {
					return;
				}
			}
		}
#endif
		N1_Mod<N, Tag>::f(z, xy, p);
	}
	static const void2uIu f;
};

template<size_t N, class Tag>
const void2uIu MulUnit<N, Tag>::f = MulUnit<N, Tag>::func;

// z[N] <- x[N * 2] % p[N]
template<size_t N, class Tag = Gtag>
struct Dbl_Mod {
	static inline void func(Unit *y, const Unit *x, const Unit *p)
	{
#ifdef MCL_USE_VINT
		mcl::vint::divNM<Unit>(0, 0, y, x, N * 2, p, N);
#else
		mp_limb_t q[N + 1]; // not used
		mpn_tdiv_qr(q, (mp_limb_t*)y, 0, (const mp_limb_t*)x, N * 2, (const mp_limb_t*)p, N);
#endif
	}
	static const void3u f;
};

template<size_t N, class Tag>
const void3u Dbl_Mod<N, Tag>::f = Dbl_Mod<N, Tag>::func;

template<size_t N, class Tag>
struct SubIfPossible {
	static inline void f(Unit *z, const Unit *p)
	{
		Unit tmp[N - 1];
		if (SubPre<N - 1, Tag>::f(tmp, z, p) == 0) {
			copyC<N - 1>(z, tmp);
			z[N - 1] = 0;
		}
	}
};
template<class Tag>
struct SubIfPossible<1, Tag> {
	static inline void f(Unit *, const Unit *)
	{
	}
};


// z[N] <- (x[N] + y[N]) % p[N]
template<size_t N, bool isFullBit, class Tag = Gtag>
struct Add {
	static inline void func(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		if (isFullBit) {
			if (AddPre<N, Tag>::f(z, x, y)) {
				SubPre<N, Tag>::f(z, z, p);
				return;
			}
			Unit tmp[N];
			if (SubPre<N, Tag>::f(tmp, z, p) == 0) {
				copyC<N>(z, tmp);
			}
		} else {
			AddPre<N, Tag>::f(z, x, y);
			Unit a = z[N - 1];
			Unit b = p[N - 1];
			if (a < b) return;
			if (a > b) {
				SubPre<N, Tag>::f(z, z, p);
				return;
			}
			/* the top of z and p are same */
			SubIfPossible<N, Tag>::f(z, p);
		}
	}
	static const void4u f;
};

template<size_t N, bool isFullBit, class Tag>
const void4u Add<N, isFullBit, Tag>::f = Add<N, isFullBit, Tag>::func;

// z[N] <- (x[N] - y[N]) % p[N]
template<size_t N, bool isFullBit, class Tag = Gtag>
struct Sub {
	static inline void func(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		if (SubPre<N, Tag>::f(z, x, y)) {
			AddPre<N, Tag>::f(z, z, p);
		}
	}
	static const void4u f;
};

template<size_t N, bool isFullBit, class Tag>
const void4u Sub<N, isFullBit, Tag>::f = Sub<N, isFullBit, Tag>::func;

//	z[N * 2] <- (x[N * 2] + y[N * 2]) mod p[N] << (N * UnitBitSize)
template<size_t N, class Tag = Gtag>
struct DblAdd {
	static inline void func(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		if (AddPre<N * 2, Tag>::f(z, x, y)) {
			SubPre<N, Tag>::f(z + N, z + N, p);
			return;
		}
		Unit tmp[N];
		if (SubPre<N, Tag>::f(tmp, z + N, p) == 0) {
			memcpy(z + N, tmp, sizeof(tmp));
		}
	}
	static const void4u f;
};

template<size_t N, class Tag>
const void4u DblAdd<N, Tag>::f = DblAdd<N, Tag>::func;

//	z[N * 2] <- (x[N * 2] - y[N * 2]) mod p[N] << (N * UnitBitSize)
template<size_t N, class Tag = Gtag>
struct DblSub {
	static inline void func(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		if (SubPre<N * 2, Tag>::f(z, x, y)) {
			AddPre<N, Tag>::f(z + N, z + N, p);
		}
	}
	static const void4u f;
};

template<size_t N, class Tag>
const void4u DblSub<N, Tag>::f = DblSub<N, Tag>::func;

/*
	z[N] <- montRed(xy[N * 2], p[N])
	REMARK : assume p[-1] = rp
*/
template<size_t N, bool isFullBit, class Tag = Gtag>
struct MontRed {
	static inline void func(Unit *z, const Unit *xy, const Unit *p)
	{
		const Unit rp = p[-1];
		Unit pq[N + 1];
		Unit buf[N * 2 + 1];
		copyC<N - 1>(buf + N + 1, xy + N + 1);
		buf[N * 2] = 0;
		Unit q = xy[0] * rp;
		MulUnitPre<N, Tag>::f(pq, p, q);
		Unit up = AddPre<N + 1, Tag>::f(buf, xy, pq);
		if (up) {
			buf[N * 2] = AddUnitPre<Tag>::f(buf + N + 1, N - 1, 1);
		}
		Unit *c = buf + 1;
		for (size_t i = 1; i < N; i++) {
			q = c[0] * rp;
			MulUnitPre<N, Tag>::f(pq, p, q);
			up = AddPre<N + 1, Tag>::f(c, c, pq);
			if (up) {
				AddUnitPre<Tag>::f(c + N + 1, N - i, 1);
			}
			c++;
		}
		if (c[N]) {
			SubPre<N, Tag>::f(z, c, p);
		} else {
			if (SubPre<N, Tag>::f(z, c, p)) {
				memcpy(z, c, N * sizeof(Unit));
			}
		}
	}
	static const void3u f;
};

template<size_t N, bool isFullBit, class Tag>
const void3u MontRed<N, isFullBit, Tag>::f = MontRed<N, isFullBit, Tag>::func;

/*
	z[N] <- Montgomery(x[N], y[N], p[N])
	REMARK : assume p[-1] = rp
*/
template<size_t N, bool isFullBit, class Tag = Gtag>
struct Mont {
	static inline void func(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
#if MCL_MAX_BIT_SIZE == 1024 || MCL_SIZEOF_UNIT == 4 // check speed
		Unit xy[N * 2];
		MulPre<N, Tag>::f(xy, x, y);
		MontRed<N, isFullBit, Tag>::f(z, xy, p);
#else
		const Unit rp = p[-1];
		if (isFullBit) {
			Unit buf[N * 2 + 2];
			Unit *c = buf;
			MulUnitPre<N, Tag>::f(c, x, y[0]); // x * y[0]
			Unit q = c[0] * rp;
			Unit t[N + 2];
			MulUnitPre<N, Tag>::f(t, p, q); // p * q
			t[N + 1] = 0; // always zero
			c[N + 1] = AddPre<N + 1, Tag>::f(c, c, t);
			c++;
			for (size_t i = 1; i < N; i++) {
				MulUnitPre<N, Tag>::f(t, x, y[i]);
				c[N + 1] = AddPre<N + 1, Tag>::f(c, c, t);
				q = c[0] * rp;
				MulUnitPre<N, Tag>::f(t, p, q);
				AddPre<N + 2, Tag>::f(c, c, t);
				c++;
			}
			if (c[N]) {
				SubPre<N, Tag>::f(z, c, p);
			} else {
				if (SubPre<N, Tag>::f(z, c, p)) {
					memcpy(z, c, N * sizeof(Unit));
				}
			}
		} else {
			/*
				R = 1 << 64
				L % 64 = 63 ; not full bit
				F = 1 << (L + 1)
				max p = (1 << L) - 1
				x, y <= p - 1
				max x * y[0], p * q <= ((1 << L) - 1)(R - 1)
				t = x * y[i] + p * q <= 2((1 << L) - 1)(R - 1) = (F - 2)(R - 1)
				t >> 64 <= (F - 2)(R - 1)/R = (F - 2) - (F - 2)/R
				t + (t >> 64) = (F - 2)R - (F - 2)/R < FR
			*/
			Unit carry;
			(void)carry;
			Unit buf[N * 2 + 1];
			Unit *c = buf;
			MulUnitPre<N, Tag>::f(c, x, y[0]); // x * y[0]
			Unit q = c[0] * rp;
			Unit t[N + 1];
			MulUnitPre<N, Tag>::f(t, p, q); // p * q
			carry = AddPre<N + 1, Tag>::f(c, c, t);
			assert(carry == 0);
			c++;
			c[N] = 0;
			for (size_t i = 1; i < N; i++) {
				c[N + 1] = 0;
				MulUnitPre<N, Tag>::f(t, x, y[i]);
				carry = AddPre<N + 1, Tag>::f(c, c, t);
				assert(carry == 0);
				q = c[0] * rp;
				MulUnitPre<N, Tag>::f(t, p, q);
				carry = AddPre<N + 1, Tag>::f(c, c, t);
				assert(carry == 0);
				c++;
			}
			assert(c[N] == 0);
			if (SubPre<N, Tag>::f(z, c, p)) {
				memcpy(z, c, N * sizeof(Unit));
			}
		}
#endif
	}
	static const void4u f;
};

template<size_t N, bool isFullBit, class Tag>
const void4u Mont<N, isFullBit, Tag>::f = Mont<N, isFullBit, Tag>::func;

// z[N] <- Montgomery(x[N], x[N], p[N])
template<size_t N, bool isFullBit, class Tag = Gtag>
struct SqrMont {
	static inline void func(Unit *y, const Unit *x, const Unit *p)
	{
#if 0 // #if MCL_MAX_BIT_SIZE == 1024 || MCL_SIZEOF_UNIT == 4 // check speed
		Unit xx[N * 2];
		SqrPre<N, Tag>::f(xx, x);
		MontRed<N, isFullBit, Tag>::f(y, xx, p);
#else
		Mont<N, isFullBit, Tag>::f(y, x, x, p);
#endif
	}
	static const void3u f;
};
template<size_t N, bool isFullBit, class Tag>
const void3u SqrMont<N, isFullBit, Tag>::f = SqrMont<N, isFullBit, Tag>::func;

// z[N] <- (x[N] * y[N]) % p[N]
template<size_t N, class Tag = Gtag>
struct Mul {
	static inline void func(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		Unit xy[N * 2];
		MulPre<N, Tag>::f(xy, x, y);
		Dbl_Mod<N, Tag>::f(z, xy, p);
	}
	static const void4u f;
};
template<size_t N, class Tag>
const void4u Mul<N, Tag>::f = Mul<N, Tag>::func;

// y[N] <- (x[N] * x[N]) % p[N]
template<size_t N, class Tag = Gtag>
struct Sqr {
	static inline void func(Unit *y, const Unit *x, const Unit *p)
	{
		Unit xx[N * 2];
		SqrPre<N, Tag>::f(xx, x);
		Dbl_Mod<N, Tag>::f(y, xx, p);
	}
	static const void3u f;
};
template<size_t N, class Tag>
const void3u Sqr<N, Tag>::f = Sqr<N, Tag>::func;

template<size_t N, class Tag = Gtag>
struct Fp2MulNF {
	static inline void func(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		const Unit *const a = x;
		const Unit *const b = x + N;
		const Unit *const c = y;
		const Unit *const d = y + N;
		Unit d0[N * 2];
		Unit d1[N * 2];
		Unit d2[N * 2];
		Unit s[N];
		Unit t[N];
		AddPre<N, Tag>::f(s, a, b);
		AddPre<N, Tag>::f(t, c, d);
		MulPre<N, Tag>::f(d0, s, t);
		MulPre<N, Tag>::f(d1, a, c);
		MulPre<N, Tag>::f(d2, b, d);
		SubPre<N * 2, Tag>::f(d0, d0, d1);
		SubPre<N * 2, Tag>::f(d0, d0, d2);
		MontRed<N, false, Tag>::f(z + N, d0, p);
		DblSub<N, Tag>::f(d1, d1, d2, p);
		MontRed<N, false, Tag>::f(z, d1, p);
	}
	static const void4u f;
};
template<size_t N, class Tag>
const void4u Fp2MulNF<N, Tag>::f = Fp2MulNF<N, Tag>::func;

} } // mcl::fp

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
