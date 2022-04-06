/*
	sudo cpufreq-set -c 0 -g performance
	mycl karatsuba.cpp -DMCL_USE_LLVM=1 ../lib/libmcl.a && ./a.out
*/
#include <stdio.h>
#include <mcl/fp.hpp>
#include <cybozu/xorshift.hpp>
#include "../src/proto.hpp"
#include "../src/low_func.hpp"
#ifdef MCL_USE_LLVM
#include "../src/low_func_llvm.hpp"
#endif
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>

typedef mcl::FpT<> Fp;

using namespace mcl::fp;

void dump(const Unit *x, size_t N)
{
	for (size_t i = 0; i < N; i++) {
		printf("%016llx ", (long long)x[N - 1 - i]);
	}
	printf("\n");
}

void gggKara(uint64_t *z, const uint64_t *x, const uint64_t *)
{
	SqrPre<8, Gtag>::f(z, x);
}
void gggLLVM(uint64_t *z, const uint64_t *x, const uint64_t *y)
{
	MulPre<8, Ltag>::f(z, x, y);
}

template<size_t N>
void benchKaratsuba()
{
	cybozu::XorShift rg;
	printf("N=%d\n", (int)N);
	Unit z[N * 2];
	rg.read(z, N);
	CYBOZU_BENCH("g:mulPre ", (MulPreCore<N, Gtag>::f), z, z, z);
//	CYBOZU_BENCH("g:mulKara", (MulPre<N, Gtag>::karatsuba), z, z, z);
	CYBOZU_BENCH("g:sqrPre ", (SqrPreCore<N, Gtag>::f), z, z);
//	CYBOZU_BENCH("g:sqrKara", (SqrPre<N, Gtag>::karatsuba), z, z);

#ifdef MCL_USE_LLVM
	CYBOZU_BENCH("l:mulPre ", (MulPreCore<N, Ltag>::f), z, z, z);
	CYBOZU_BENCH("l:sqrPre ", (SqrPreCore<N, Ltag>::f), z, z);
	CYBOZU_BENCH("l:mulKara", (MulPre<N, Ltag>::karatsuba), z, z, z);
	CYBOZU_BENCH("l:sqrKara", (SqrPre<N, Ltag>::karatsuba), z, z);
#endif
}

CYBOZU_TEST_AUTO(karatsuba)
{
	benchKaratsuba<4>();
	benchKaratsuba<6>();
	benchKaratsuba<8>();
#if MCL_MAX_BIT_SIZE >= 640
	benchKaratsuba<10>();
#endif
#if MCL_MAX_BIT_SIZE >= 768
	benchKaratsuba<12>();
#endif
#if MCL_MAX_BIT_SIZE >= 896
	benchKaratsuba<14>();
#endif
#if MCL_MAX_BIT_SIZE >= 1024
	benchKaratsuba<16>();
#endif
}

