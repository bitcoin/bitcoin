#include <cybozu/benchmark.hpp>
#include <cybozu/option.hpp>
#include <cybozu/xorshift.hpp>
#include <mcl/fp.hpp>
#include <mcl/conversion.hpp>
#include <mcl/ecparam.hpp>

typedef mcl::FpT<> Fp;
typedef mcl::FpT<mcl::ZnTag> Zn;
typedef mcl::EcT<Fp> Ec;

void benchFpSub(const char *pStr, const char *xStr, const char *yStr, mcl::fp::Mode mode)
{
	const char *s = mcl::fp::ModeToStr(mode);
	Fp::init(pStr, mode);
	Fp x(xStr);
	Fp y(yStr);

	double addT, subT, mulT, sqrT, invT;
	CYBOZU_BENCH_T(addT, Fp::add, x, x, x);
	CYBOZU_BENCH_T(subT, Fp::sub, x, x, y);
	CYBOZU_BENCH_T(mulT, Fp::mul, x, x, x);
	CYBOZU_BENCH_T(sqrT, Fp::sqr, x, x);
	CYBOZU_BENCH_T(invT, x += y;Fp::inv, x, x); // avoid same jmp
	printf("%10s bit % 3d add %8.2f sub %8.2f mul %8.2f sqr %8.2f inv %8.2f\n", s, (int)Fp::getBitSize(), addT, subT, mulT, sqrT, invT);
}

void benchFp(size_t bitSize, int mode)
{
	const struct {
		size_t bitSize;
		const char *p;
		const char *x;
		const char *y;
	} tbl[] = {
		{
			192,
			"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d",
			"0x148094810948190412345678901234567900342423332197",
			"0x7fffffffffffffffffffffe26f2fc170f69466a74defd8d",
		},
		{
			256,
			"0x2523648240000001ba344d80000000086121000000000013a700000000000013",
			"0x1480948109481904123456789234234242423424201234567900342423332197",
			"0x151342342342341517fffffffffffffffffffffe26f2fc170f69466a74defd8d",
		},
		{
			384,
			"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff",
			"0x19481084109481094820948209482094820984290482212345678901234567900342308472047204720422423332197",
			"0x209348209481094820984209842094820948204204243123456789012345679003423084720472047204224233321972",
			
		},
#if MCL_MAX_BIT_SIZE >= 521
		{
			521,
			"0x1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
			"0x2908209582095820941098410948109482094820984209840294829049240294242498540975555312345678901234567900342308472047204720422423332197",
			"0x3948384209834029834092384204920349820948205872380573205782385729385729385723985837ffffffffffffffffffffffe26f2fc170f69466a74defd8d",

		},
#endif
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		if (bitSize != 0 && tbl[i].bitSize != bitSize) continue;
		if (mode & 1) benchFpSub(tbl[i].p, tbl[i].x, tbl[i].y, mcl::fp::FP_GMP);
		if (mode & 2) benchFpSub(tbl[i].p, tbl[i].x, tbl[i].y, mcl::fp::FP_GMP_MONT);
#ifdef MCL_USE_LLVM
		if (mode & 4) benchFpSub(tbl[i].p, tbl[i].x, tbl[i].y, mcl::fp::FP_LLVM);
		if (mode & 8) benchFpSub(tbl[i].p, tbl[i].x, tbl[i].y, mcl::fp::FP_LLVM_MONT);
#endif
#ifdef MCL_X64_ASM
		if (mode & 16) benchFpSub(tbl[i].p, tbl[i].x, tbl[i].y, mcl::fp::FP_XBYAK);
#endif
	}
}

void benchEcSub(const mcl::EcParam& para, mcl::fp::Mode mode, mcl::ec::Mode ecMode)
{
	Ec P;
	mcl::initCurve<Ec, Zn>(para.curveType, &P, mode, ecMode);
	Ec P2; Ec::add(P2, P, P);
	Ec Q = P + P + P;
	double addT, add2T, subT, dblT, mulT, mulCTT, mulRandT, mulCTRandT, normT;
	CYBOZU_BENCH_T(addT, P = P2; Ec::add, Q, P, Q);
	P.normalize();
	CYBOZU_BENCH_T(add2T, Ec::add, Q, P, Q);
	CYBOZU_BENCH_T(subT, Ec::sub, Q, P, Q);
	CYBOZU_BENCH_T(dblT, Ec::dbl, P, P);
	Zn z("3");
	CYBOZU_BENCH_T(mulT, Ec::mul, Q, P, z);
	CYBOZU_BENCH_T(mulCTT, Ec::mulCT, Q, P, z);
	cybozu::XorShift rg;
	z.setRand(rg);
	CYBOZU_BENCH_T(mulRandT, Ec::mul, Q, P, z);
	CYBOZU_BENCH_T(mulCTRandT, Ec::mulCT, Q, P, z);
	CYBOZU_BENCH_T(normT, Q = P; Q.normalize);
	printf("%10s %10s add %8.2f add2 %8.2f sub %8.2f dbl %8.2f mul(3) %8.2f mulCT(3) %8.2f mul(rand) %8.2f mulCT(rand) %8.2f norm %8.2f\n", para.name, mcl::fp::ModeToStr(mode), addT, add2T, subT, dblT, mulT, mulCTT, mulRandT, mulCTRandT, normT);

}
void benchEc(size_t bitSize, int mode, mcl::ec::Mode ecMode)
{
	const struct mcl::EcParam tbl[] = {
		mcl::ecparam::p160_1,
		mcl::ecparam::secp160k1,
		mcl::ecparam::secp192k1,
		mcl::ecparam::NIST_P192,
		mcl::ecparam::secp224k1,
		mcl::ecparam::secp256k1,
		mcl::ecparam::NIST_P224,
		mcl::ecparam::NIST_P256,
//		mcl::ecparam::secp384r1,
		mcl::ecparam::NIST_P384,
#if MCL_MAX_BIT_SIZE >= 521
//		mcl::ecparam::secp521r1,
		mcl::ecparam::NIST_P521,
#endif
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		if (bitSize != 0 && tbl[i].bitSize != bitSize) continue;
		benchEcSub(tbl[i], mcl::fp::FP_AUTO, ecMode);
		if (mode & 1) benchEcSub(tbl[i], mcl::fp::FP_GMP, ecMode);
		if (mode & 2) benchEcSub(tbl[i], mcl::fp::FP_GMP_MONT, ecMode);
#ifdef MCL_USE_LLVM
		if (mode & 4) benchEcSub(tbl[i], mcl::fp::FP_LLVM, ecMode);
		if (mode & 8) benchEcSub(tbl[i], mcl::fp::FP_LLVM_MONT, ecMode);
#endif
#ifdef MCL_X64_ASM
		if (mode & 16) benchEcSub(tbl[i], mcl::fp::FP_XBYAK, ecMode);
#endif
	}
}

void benchToStr16()
{
	puts("benchToStr16");
	const char *tbl[] = {
		"0x0",
		"0x5",
		"0x123",
		"0x123456789012345679adbc",
		"0xffffffff26f2fc170f69466a74defd8d",
		"0x100000000000000000000000000000033",
		"0x11ee12312312940000000000000000000000000002342343"
	};
	Fp::init("0xffffffffffffffffffffffffffffffffffffffffffffff13");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		char buf[128];
		std::string str;
		Fp x(tbl[i]);
		CYBOZU_BENCH("fp::arrayToHex", mcl::fp::arrayToHex, buf, sizeof(buf), x.getUnit(), x.getUnitSize(), true);
		mpz_class y(tbl[i]);
		CYBOZU_BENCH("gmp:getStr ", mcl::gmp::getStr, str, y, 16);
	}
}

void benchFromStr16()
{
	puts("benchFromStr16");
	const char *tbl[] = {
		"0",
		"5",
		"123",
		"123456789012345679adbc",
		"ffffffff26f2fc170f69466a74defd8d",
		"100000000000000000000000000000033",
		"11ee12312312940000000000000000000000000002342343"
	};
	Fp::init("0xffffffffffffffffffffffffffffffffffffffffffffff13");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str = tbl[i];
		Fp x;
		const size_t N = 64;
		mcl::fp::Unit buf[N];
		CYBOZU_BENCH("fp:hexToArray", mcl::fp::hexToArray, buf, N, str.c_str(), str.size());

		mpz_class y;
		CYBOZU_BENCH("gmp:setStr  ", mcl::gmp::setStr, y, str, 16);
	}
}

int main(int argc, char *argv[])
	try
{
	size_t bitSize;
	int mode;
	bool ecOnly;
	bool fpOnly;
	bool misc;
	mcl::ec::Mode ecMode;
	std::string ecModeStr;
	cybozu::Option opt;
	opt.appendOpt(&bitSize, 0, "s", ": bitSize");
	opt.appendOpt(&mode, 0, "m", ": mode(0:all, sum of 1:gmp, 2:gmp+mont, 4:llvm, 8:llvm+mont, 16:xbyak");
	opt.appendBoolOpt(&ecOnly, "ec", ": ec only");
	opt.appendBoolOpt(&fpOnly, "fp", ": fp only");
	opt.appendBoolOpt(&misc, "misc", ": other benchmark");
	opt.appendOpt(&ecModeStr, "jacobi", "ecmode", ": jacobi or proj");
	opt.appendHelp("h", ": show this message");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	if (ecModeStr == "jacobi") {
		ecMode = mcl::ec::Jacobi;
	} else if (ecModeStr == "proj") {
		ecMode = mcl::ec::Proj;
	} else {
		printf("bad ecstr %s\n", ecModeStr.c_str());
		opt.usage();
		return 1;
	}
	if (mode < 0 || mode > 31) {
		printf("bad mode %d\n", mode);
		opt.usage();
		return 1;
	}
	if (mode == 0) mode = 31;
	if (misc) {
		benchToStr16();
		benchFromStr16();
	} else {
		if (!ecOnly) benchFp(bitSize, mode);
		if (!fpOnly) {
			printf("ecMode=%s\n", ecModeStr.c_str());
			benchEc(bitSize, mode, ecMode);
		}
	}
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
}

