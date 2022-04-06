/*
	large prime sample for 64-bit arch
	make MCL_USE_LLVM=1 MCL_MAX_BIT_SIZE=768
*/
#include <mcl/fp.hpp>
#include <cybozu/benchmark.hpp>
#include <iostream>
#include "../src/low_func.hpp"

typedef mcl::FpT<> Fp;

using namespace mcl::fp;
const size_t N = 12;

void testMul()
{
	Unit ux[N], uy[N], a[N * 2], b[N * 2];
	for (size_t i = 0; i < N; i++) {
		ux[i] = -i * i + 5;
		uy[i] = -i * i + 9;
	}
	MulPreCore<N, Gtag>::f(a, ux, uy);
	MulPreCore<N, Ltag>::f(b, ux, uy);
	for (size_t i = 0; i < N * 2; i++) {
		if (a[i] != b[i]) {
			printf("ERR %016llx %016llx\n", (long long)a[i], (long long)b[i]);
		}
	}
	puts("end testMul");
	CYBOZU_BENCH("gmp ", (MulPreCore<N, Gtag>::f), ux, ux, uy);
	CYBOZU_BENCH("kara", (MulPre<N, Gtag>::karatsuba), ux, ux, uy);
}

void mulGmp(mpz_class& z, const mpz_class& x, const mpz_class& y, const mpz_class& p)
{
	z = (x * y) % p;
}
void compareGmp(const std::string& pStr)
{
	Fp::init(pStr);
	std::string xStr = "2104871209348712947120947102843728";
	std::string s1, s2;
	{
		Fp x(xStr);
		CYBOZU_BENCH_C("mul by mcl", 1000, Fp::mul, x, x, x);
		std::ostringstream os;
		os << x;
		s1 = os.str();
	}
	{
		const mpz_class p(pStr);
		mpz_class x(xStr);
		CYBOZU_BENCH_C("mul by GMP", 1000, mulGmp, x, x, x, p);
		std::ostringstream os;
		os << x;
		s2 = os.str();
	}
	if (s1 != s2) {
		puts("ERR");
	}
}

void test(const std::string& pStr, mcl::fp::Mode mode)
{
	printf("test %s\n", mcl::fp::ModeToStr(mode));
	Fp::init(pStr, mode);
	const mcl::fp::Op& op = Fp::getOp();
	printf("bitSize=%d\n", (int)Fp::getBitSize());
	mpz_class p(pStr);
	Fp x = 123456;
	Fp y;
	Fp::pow(y, x, p);
	std::cout << y << std::endl;
	if (x != y) {
		std::cout << "err:pow:" << y << std::endl;
		return;
	}
	const size_t N = 24;
	mcl::fp::Unit ux[N], uy[N];
	for (size_t i = 0; i < N; i++) {
		ux[i] = -i * i + 5;
		uy[i] = -i * i + 9;
	}
	CYBOZU_BENCH("mulPre", op.fpDbl_mulPre, ux, ux, uy);
	CYBOZU_BENCH("sqrPre", op.fpDbl_sqrPre, ux, ux);
	CYBOZU_BENCH("add", op.fpDbl_add, ux, ux, ux, op.p);
	CYBOZU_BENCH("sub", op.fpDbl_sub, ux, ux, ux, op.p);
	if (op.fpDbl_addPre) {
		CYBOZU_BENCH("addPre", op.fpDbl_addPre, ux, ux, ux);
		CYBOZU_BENCH("subPre", op.fpDbl_subPre, ux, ux, ux);
	}
	CYBOZU_BENCH("mont", op.fpDbl_mod, ux, ux, op.p);
	CYBOZU_BENCH("mul", Fp::mul, x, x, x);
	compareGmp(pStr);
}

void testAll(const std::string& pStr)
{
	test(pStr, mcl::fp::FP_GMP);
	test(pStr, mcl::fp::FP_GMP_MONT);
#ifdef MCL_USE_LLVM
	test(pStr, mcl::fp::FP_LLVM);
	test(pStr, mcl::fp::FP_LLVM_MONT);
#endif
	compareGmp(pStr);
}
int main()
	try
{
	const char *pTbl[] = {
		"40347654345107946713373737062547060536401653012956617387979052445947619094013143666088208645002153616185987062074179207",
		"13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006083527",
		"776259046150354467574489744231251277628443008558348305569526019013025476343188443165439204414323238975243865348565536603085790022057407195722143637520590569602227488010424952775132642815799222412631499596858234375446423426908029627",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(pTbl); i++) {
		testAll(pTbl[i]);
	}
	testMul();
} catch (std::exception& e) {
	printf("err %s\n", e.what());
	puts("make clean");
	puts("make -DMCL_MAX_BIT_SIZE=768");
	return 1;
}

