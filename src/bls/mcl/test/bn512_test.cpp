#define CYBOZU_TEST_DISABLE_AUTO_RUN
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/option.hpp>
#include <cybozu/xorshift.hpp>
#include <mcl/bn512.hpp>
#include <mcl/bn.hpp>

using namespace mcl::bn512;

#include "common_test.hpp"

mcl::fp::Mode g_mode;

#include "bench.hpp"

void testCurve(const mcl::CurveParam& cp)
{
	initPairing(cp, g_mode);
	G1 P;
	G2 Q;
	mapToG1(P, 1);
	mapToG2(Q, 1);
	GT e1, e2;
	pairing(e1, P, Q);
	cybozu::XorShift rg;
	mpz_class a, b;
	Fr r;
	r.setRand(rg); a = r.getMpz();
	r.setRand(rg); b = r.getMpz();
	G1 aP;
	G2 bQ;
	G1::mul(aP, P, a);
	G2::mul(bQ, Q, b);
	pairing(e2, aP, bQ);
	GT::pow(e1, e1, a * b);
	CYBOZU_TEST_EQUAL(e1, e2);
	testCommon(P, Q);
	testBench(P, Q);
	testSquareRoot();
	testLagrange();
}

CYBOZU_TEST_AUTO(pairing)
{
	puts("BLS12_461");
	testCurve(mcl::BLS12_461);
	puts("BN462");
	testCurve(mcl::BN462);
	puts("BN381_1");
	testCurve(mcl::BN381_1);
	puts("BLS12_381");
	testCurve(mcl::BLS12_381);
	puts("BN254");
	testCurve(mcl::BN254);
}

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	std::string mode;
	opt.appendOpt(&mode, "auto", "m", ": mode(gmp/gmp_mont/llvm/llvm_mont/xbyak)");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	g_mode = mcl::fp::StrToMode(mode);
	return cybozu::test::autoRun.run(argc, argv);
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
	return 1;
}
