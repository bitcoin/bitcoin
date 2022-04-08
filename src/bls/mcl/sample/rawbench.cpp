#define PUT(x) std::cout << #x "=" << (x) << std::endl
#include <cybozu/benchmark.hpp>
#include <cybozu/option.hpp>
#include <cybozu/xorshift.hpp>
#include <mcl/fp.hpp>
#include <mcl/fp_tower.hpp>

typedef mcl::FpT<mcl::FpTag> Fp;
typedef mcl::Fp2T<Fp> Fp2;
typedef mcl::FpDblT<Fp> FpDbl;
typedef mcl::Fp6T<Fp> Fp6;
typedef mcl::Fp12T<Fp> Fp12;

typedef mcl::fp::Unit Unit;

void mul9(const mcl::fp::Op& op, Unit *y, const Unit *x, const Unit *p)
{
	const size_t maxN = sizeof(Fp) / sizeof(Unit);
	Unit tmp[maxN];
	op.fp_add(tmp, x, x, p); // 2x
	op.fp_add(tmp, tmp, tmp, p); // 4x
	op.fp_add(tmp, tmp, tmp, p); // 8x
	op.fp_add(y, tmp, x, p); // 9x
}

void benchRaw(const char *p, mcl::fp::Mode mode)
{
	Fp::init(1, p, mode);
	Fp2::init();
	const size_t maxN = sizeof(Fp) / sizeof(Unit);
	const mcl::fp::Op& op = Fp::getOp();
	cybozu::XorShift rg;
	Fp fx, fy;
	fx.setRand(rg);
	fy.setRand(rg);
	Unit ux[maxN * 2] = {};
	Unit uy[maxN * 2] = {};
	Unit uz[maxN * 2] = {};
	memcpy(ux, fx.getUnit(), sizeof(Unit) * op.N);
	memcpy(ux + op.N, fx.getUnit(), sizeof(Unit) * op.N);
	memcpy(uy, fy.getUnit(), sizeof(Unit) * op.N);
	memcpy(ux + op.N, fx.getUnit(), sizeof(Unit) * op.N);
	double fp_addT, fp_subT;
	double fp_addPreT, fp_subPreT;
	double fp_sqrT, fp_mulT;
	double fp_mulUnitT;
	double mul9T;
	double fp_mulUnitPreT;
	double fpN1_modT;
	double fpDbl_addT, fpDbl_subT;
	double fpDbl_sqrPreT, fpDbl_mulPreT, fpDbl_modT;
	double fp2_sqrT, fp2_mulT;
	CYBOZU_BENCH_T(fp_addT, op.fp_add, uz, ux, uy, op.p);
	CYBOZU_BENCH_T(fp_subT, op.fp_sub, uz, uy, ux, op.p);
	CYBOZU_BENCH_T(fp_addPreT, op.fp_addPre, uz, ux, uy);
	CYBOZU_BENCH_T(fp_subPreT, op.fp_subPre, uz, uy, ux);
	CYBOZU_BENCH_T(fp_sqrT, op.fp_sqr, uz, ux, op.p);
	CYBOZU_BENCH_T(fp_mulT, op.fp_mul, uz, ux, uy, op.p);
	CYBOZU_BENCH_T(fp_mulUnitT, op.fp_mulUnit, uz, ux, 9, op.p);
	CYBOZU_BENCH_T(mul9T, mul9, op, uz, ux, op.p);
	CYBOZU_BENCH_T(fp_mulUnitPreT, op.fp_mulUnitPre, ux, ux, 9);
	CYBOZU_BENCH_T(fpN1_modT, op.fpN1_mod, ux, uy, op.p);
	CYBOZU_BENCH_T(fpDbl_addT, op.fpDbl_add, uz, ux, uy, op.p);
	CYBOZU_BENCH_T(fpDbl_subT, op.fpDbl_sub, uz, uy, ux, op.p);
	CYBOZU_BENCH_T(fpDbl_sqrPreT, op.fpDbl_sqrPre, uz, ux);
	CYBOZU_BENCH_T(fpDbl_mulPreT, op.fpDbl_mulPre, uz, ux, uy);
	CYBOZU_BENCH_T(fpDbl_modT, op.fpDbl_mod, uz, ux, op.p);
	Fp2 f2x, f2y;
	f2x.a = fx;
	f2x.b = fy;
	f2y = f2x;
	CYBOZU_BENCH_T(fp2_sqrT, Fp2::sqr, f2x, f2x);
	CYBOZU_BENCH_T(fp2_mulT, Fp2::mul, f2x, f2x, f2y);
	printf("%s\n", mcl::fp::ModeToStr(mode));
	const char *tStrTbl[] = {
		"fp_add", "fp_sub",
		"addPre", "subPre",
		"fp_sqr", "fp_mul",
		"mulUnit",
		"mul9",
		"mulUnitP",
		"fpN1_mod",
		"D_add", "D_sub",
		"D_sqrPre", "D_mulPre", "D_mod",
		"fp2_sqr", "fp2_mul",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tStrTbl); i++) {
		printf(" %8s", tStrTbl[i]);
	}
	printf("\n");
	const double tTbl[] = {
		fp_addT, fp_subT,
		fp_addPreT, fp_subPreT,
		fp_sqrT, fp_mulT,
		fp_mulUnitT,
		mul9T,
		fp_mulUnitPreT,
		fpN1_modT,
		fpDbl_addT, fpDbl_subT,
		fpDbl_sqrPreT, fpDbl_mulPreT, fpDbl_modT,
		fp2_sqrT, fp2_mulT,
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tTbl); i++) {
		printf(" %8.2f", tTbl[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	size_t bitSize;
	opt.appendOpt(&bitSize, 0, "s", ": bitSize");
	opt.appendHelp("h", ": show this message");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	const char *tbl[] = {
		// N = 2
		"0x0000000000000001000000000000000d",
		"0x7fffffffffffffffffffffffffffffff",
		"0x8000000000000000000000000000001d",
		"0xffffffffffffffffffffffffffffff61",

		// N = 3
		"0x000000000000000100000000000000000000000000000033", // min prime
		"0x70000000000000000000000000000000000000000000001f",
		"0x800000000000000000000000000000000000000000000005",
		"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d",
		"0xfffffffffffffffffffffffffffffffeffffffffffffffff",
		"0xffffffffffffffffffffffffffffffffffffffffffffff13", // max prime

		// N = 4
		"0x0000000000000001000000000000000000000000000000000000000000000085", // min prime
		"0x2523648240000001ba344d80000000086121000000000013a700000000000013", // BN254
		"0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47", // Snark
		"0x7523648240000001ba344d80000000086121000000000013a700000000000017",
		"0x800000000000000000000000000000000000000000000000000000000000005f",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff43", // max prime
		// N = 5
		"0x80000000000000000000000000000000000000000000000000000000000000000000000000000009",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff3b",
		// N = 6
		"0x800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000171",
		"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffec3",
		// N = 7
		"0x8000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000063",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff35",
		// N = 8
		"0x8000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006f",
		"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffdc7",
#if MCL_MAX_BIT_SIZE == 1024
		"0xc70b1ddda9b96e3965e5855942aa5852d8f8e052c760ac32cdfec16a2ed3d56981e1a475e20a70144ed2f5061ba64900f69451492803f815d446ee133d0668f7a7f3276d6301c95ce231f0e4b0d0f3882f10014fca04454cff55d2e2d4cfc1aad33b8d38397e2fc8b623177e63d0b783269c40a85b8f105654783b8ed2e737df",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff97",
#endif
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const char *p = tbl[i];
		if (bitSize > 0 && (strlen(p) - 2) * 4 != bitSize) {
			continue;
		}
		printf("prime=%s\n", p);
		benchRaw(tbl[i], mcl::fp::FP_GMP);
		benchRaw(tbl[i], mcl::fp::FP_GMP_MONT);
#ifdef MCL_USE_LLVM
		benchRaw(tbl[i], mcl::fp::FP_LLVM);
		benchRaw(tbl[i], mcl::fp::FP_LLVM_MONT);
#endif
#ifdef MCL_X64_ASM
		if (bitSize <= 384) {
			benchRaw(tbl[i], mcl::fp::FP_XBYAK);
		}
#endif
	}
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
	return 1;
}
