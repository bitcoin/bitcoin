/*
32bit raspi
N=6
mulPre 511.30nsec
sqrPre 598.33nsec
mod    769.64nsec
mont     1.283usec
N=8
mulPre   1.463usec
sqrPre   1.422usec
mod      1.972usec
mont     2.962usec
N=12
mulPre   2.229usec
sqrPre   2.056usec
mod      3.811usec
mont     6.802usec
N=16
mulPre   4.955usec
sqrPre   4.706usec
mod      6.817usec
mont    12.916usec
*/
#include <stdio.h>
#include <stdint.h>
#include <cybozu/inttype.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/xorshift.hpp>

typedef size_t Unit;

template<size_t N>
void mulPre(Unit*, const Unit*, const Unit*);

template<size_t N>
void sqrPre(Unit*, const Unit*);

template<size_t N>
void mod(Unit*, const Unit*, const Unit *);

template<size_t N>
void mont(Unit*, const Unit*, const Unit*, const Unit *);

#define MCL_FP_DEF_FUNC_SUB(n, suf) \
extern "C" { \
void mcl_fp_add ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
void mcl_fp_addNF ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
void mcl_fp_sub ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
void mcl_fp_subNF ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
void mcl_fp_shr1_ ## n ## suf(Unit*y, const Unit* x); \
Unit mcl_fp_addPre ## n ## suf(Unit* z, const Unit* x, const Unit* y); \
Unit mcl_fp_subPre ## n ## suf(Unit* z, const Unit* x, const Unit* y); \
void mcl_fp_mulUnitPre ## n ## suf(Unit* z, const Unit* x, Unit y); \
void mcl_fpDbl_mulPre ## n ## suf(Unit* z, const Unit* x, const Unit* y); \
void mcl_fpDbl_sqrPre ## n ## suf(Unit* y, const Unit* x); \
void mcl_fp_mont ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
void mcl_fp_montNF ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
void mcl_fp_montRed ## n ## suf(Unit* z, const Unit* xy, const Unit* p); \
void mcl_fp_montRedNF ## n ## suf(Unit* z, const Unit* xy, const Unit* p); \
void mcl_fpDbl_add ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
void mcl_fpDbl_sub ## n ## suf(Unit* z, const Unit* x, const Unit* y, const Unit* p); \
} \
template<>void mulPre<n>(Unit *z, const Unit *x, const Unit *y) { mcl_fpDbl_mulPre ## n ## suf(z, x, y); } \
template<>void sqrPre<n>(Unit *z, const Unit *x) { mcl_fpDbl_sqrPre ## n ## suf(z, x); } \
template<>void mod<n>(Unit *z, const Unit *x, const Unit *p) { mcl_fp_montRedNF ## n ## suf(z, x, p); } \
template<>void mont<n>(Unit *z, const Unit *x, const Unit *y, const Unit *p) { mcl_fp_montNF ## n ## suf(z, x, y, p); }

#if MCL_SIZEOF_UNIT == 8
MCL_FP_DEF_FUNC_SUB(4, L)
MCL_FP_DEF_FUNC_SUB(5, L)
#endif
MCL_FP_DEF_FUNC_SUB(6, L)
//MCL_FP_DEF_FUNC_SUB(7, L)
MCL_FP_DEF_FUNC_SUB(8, L)
#if MCL_SIZEOF_UNIT == 4
MCL_FP_DEF_FUNC_SUB(12, L)
MCL_FP_DEF_FUNC_SUB(16, L)
#endif


template<class RG, class T>
void setRand(T *x, size_t n, RG& rg)
{
	for (size_t i = 0; i < n; i++) {
		if (sizeof(T) == 4) {
			x[i] = rg.get32();
		} else {
			x[i] = rg.get64();
		}
	}
}

template<size_t N>
void bench(Unit *x, Unit *y, const Unit *p)
{
	printf("N=%zd\n", N);
	Unit xx[N * 2], yy[N * 2];
#if MCL_SIZEOF_UNIT == 8
	const int C = 10000;
#else
	const int C = 1000;
#endif
	CYBOZU_BENCH_C("mulPre", C, mulPre<N>, xx, x, y);
	CYBOZU_BENCH_C("sqrPre", C, sqrPre<N>, yy, x);
	CYBOZU_BENCH_C("mod   ", C, mod<N>, yy, xx, p);
	CYBOZU_BENCH_C("mont  ", C, mont<N>, yy, x, y, p);
}

int main()
{
	printf("sizeof(Unit)=%zd\n", sizeof(Unit));
	const size_t maxN = 16;
	Unit x[maxN], y[maxN], p[maxN + 1];
	cybozu::XorShift rg;
	setRand(x, maxN, rg);
	setRand(y, maxN, rg);
	setRand(p, maxN + 1, rg);
#if MCL_SIZEOF_UNIT == 8
	bench<4>(x, y, p + 1);
	bench<5>(x, y, p + 1);
#endif
	bench<6>(x, y, p + 1);
//	bench<7>(x, y, p + 1);
	bench<8>(x, y, p + 1);
#if MCL_SIZEOF_UNIT == 4
	bench<12>(x, y, p + 1);
	bench<16>(x, y, p + 1);
#endif
}

