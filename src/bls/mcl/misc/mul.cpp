/*
	sudo cpufreq-set -c 0 -g performance
	mycl mul.cpp -DMCL_USE_LLVM=1 ../lib/libmcl.a && ./a.out
*/
#include <stdio.h>
#include <mcl/fp.hpp>
#include <cybozu/xorshift.hpp>
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

CYBOZU_TEST_AUTO(mulPre)
{
	cybozu::XorShift rg;
	const char *pTbl[] = {
		"0x2523648240000001ba344d80000000086121000000000013a700000000000013",
		"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff",
		"6701817056313037086248947066310538444882082605308124576230408038843357549886356779857393369967010764802541005796711440355753503701056323603", // 462 bit
		"4562440617622195218641171605700291324893228507248559930579192517899275167208677386505912811317371399778642309573594407310688704721375437998252661319722214188251994674360264950082874192246603471", // 640 bit
		"1552518092300708935148979488462502555256886017116696611139052038026050952686376886330878408828646477950487730697131073206171580044114814391444287275041181139204454976020849905550265285631598444825262999193716468750892846853816057031", // 768 bit
	};
	const size_t N = 16;
	const Mode modeTbl[] = {
		FP_GMP_MONT,
#ifdef MCL_USE_LLVM
		FP_LLVM_MONT,
#endif
	};
	for (size_t j = 0; j < CYBOZU_NUM_OF_ARRAY(modeTbl); j++) {
		Mode mode = modeTbl[j];
		printf("%s\n", ModeToStr(mode));
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(pTbl); i++) {
			const char *p = pTbl[i];
			Fp::init(p, mode);
			printf("bitSize=%d\n", (int)Fp::getBitSize());
			const Op& op = Fp::getOp();
			Unit x[N], y[N * 2];
			rg.read(x, N);
			rg.read(y, N * 2);
			CYBOZU_BENCH("mul   ", op.fp_mul, y, y, x, op.p);
			CYBOZU_BENCH("sqr   ", op.fp_sqr, y, y, op.p);
			CYBOZU_BENCH("mulPre", op.fpDbl_mulPre, y, y, y);
			CYBOZU_BENCH("sqrPre", op.fpDbl_sqrPre, y, y);
			CYBOZU_BENCH("mod   ", op.fpDbl_mod, y, y, op.p);
		}
	}
}
