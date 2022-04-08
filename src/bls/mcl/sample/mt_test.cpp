#include <cybozu/benchmark.hpp>
#include <mcl/bls12_381.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/option.hpp>

using namespace mcl::bls12;

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	size_t n;
	size_t cpuN;
	opt.appendOpt(&n, 100, "n");
	opt.appendOpt(&cpuN, 0, "cpu");
	opt.appendHelp("h");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	printf("n=%zd cpuN=%zd\n", n, cpuN);
	int C = 10;
	if (n >= 1000) {
		C = 1;
	}

	initPairing(mcl::BLS12_381);
	cybozu::XorShift rg;
	std::vector<G1> Pvec(n);
	std::vector<G2> Qvec(n);
	std::vector<Fr> xVec(n);
	char c = '0';
	for (size_t i = 0; i < n; i++) {
		hashAndMapToG1(Pvec[i], &c, 1);
		hashAndMapToG2(Qvec[i], &c, 1);
		xVec[i].setRand(rg);
		c++;
	}
	G1 P1, P2;
	CYBOZU_BENCH_C("single", C, G1::mulVec, P1, Pvec.data(), xVec.data(), n);
	CYBOZU_BENCH_C("multi ", C, G1::mulVecMT, P2, Pvec.data(), xVec.data(), n, cpuN);
	printf("G1 ret %s\n", P1 == P2 ? "ok" : "ng");
	G2 Q1, Q2;
	CYBOZU_BENCH_C("single", C, G2::mulVec, Q1, Qvec.data(), xVec.data(), n);
	CYBOZU_BENCH_C("multi ", C, G2::mulVecMT, Q2, Qvec.data(), xVec.data(), n, cpuN);
	printf("G2 ret %s\n", Q1 == Q2 ? "ok" : "ng");
	Fp12 e1, e2;
	CYBOZU_BENCH_C("single", C, millerLoopVec, e1, Pvec.data(), Qvec.data(), n);
	CYBOZU_BENCH_C("multi ", C, millerLoopVecMT, e2, Pvec.data(), Qvec.data(), n, cpuN);
	printf("GT ret %s\n", e1 == e2 ? "ok" : "ng");
} catch (std::exception& e) {
	printf("err %s\n", e.what());
	return 1;
}
