#include <mcl/op.hpp>
#include <mcl/util.hpp>
#include <cybozu/sha2.hpp>
#include <cybozu/endian.hpp>
#include <mcl/conversion.hpp>
#if defined(__EMSCRIPTEN__) && MCL_SIZEOF_UNIT == 4
#define USE_WASM
#include "low_func_wasm.hpp"
#endif

#if defined(MCL_STATIC_CODE) || defined(MCL_USE_XBYAK) || (defined(MCL_USE_LLVM) && (CYBOZU_HOST == CYBOZU_HOST_INTEL))

#ifdef MCL_USE_XBYAK
	#define XBYAK_DISABLE_AVX512
	#define XBYAK_NO_EXCEPTION
#else
	#define XBYAK_ONLY_CLASS_CPU
#endif

#include "xbyak/xbyak_util.h"
Xbyak::util::Cpu g_cpu;

#ifdef MCL_STATIC_CODE
#include "fp_static_code.hpp"
#endif
#ifdef MCL_USE_XBYAK
#include "fp_generator.hpp"
#endif

#endif

#include "low_func.hpp"
#ifdef MCL_USE_LLVM
#include "proto.hpp"
#include "low_func_llvm.hpp"
#endif
#include <cybozu/itoa.hpp>
#include <mcl/randgen.hpp>

#ifdef _MSC_VER
	#pragma warning(disable : 4127)
#endif

namespace mcl {

namespace fp {

#ifdef MCL_USE_XBYAK
FpGenerator *Op::createFpGenerator()
{
	return new FpGenerator();
}
void Op::destroyFpGenerator(FpGenerator *fg)
{
	delete fg;
}
#endif

inline void setUnitAsLE(void *p, Unit x)
{
#if MCL_SIZEOF_UNIT == 4
	cybozu::Set32bitAsLE(p, x);
#else
	cybozu::Set64bitAsLE(p, x);
#endif
}
inline Unit getUnitAsLE(const void *p)
{
#if MCL_SIZEOF_UNIT == 4
	return cybozu::Get32bitAsLE(p);
#else
	return cybozu::Get64bitAsLE(p);
#endif
}

const char *ModeToStr(Mode mode)
{
	switch (mode) {
	case FP_AUTO: return "auto";
	case FP_GMP: return "gmp";
	case FP_GMP_MONT: return "gmp_mont";
	case FP_LLVM: return "llvm";
	case FP_LLVM_MONT: return "llvm_mont";
	case FP_XBYAK: return "xbyak";
	default:
		assert(0);
		return 0;
	}
}

Mode StrToMode(const char *s)
{
	static const struct {
		const char *s;
		Mode mode;
	} tbl[] = {
		{ "auto", FP_AUTO },
		{ "gmp", FP_GMP },
		{ "gmp_mont", FP_GMP_MONT },
		{ "llvm", FP_LLVM },
		{ "llvm_mont", FP_LLVM_MONT },
		{ "xbyak", FP_XBYAK },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		if (strcmp(s, tbl[i].s) == 0) return tbl[i].mode;
	}
	return FP_AUTO;
}

bool isEnableJIT()
{
#if defined(MCL_USE_XBYAK)
	/* -1:not init, 0:disable, 1:enable */
	static int status = -1;
	if (status == -1) {
#ifndef _MSC_VER
		status = 1;
		FILE *fp = fopen("/sys/fs/selinux/enforce", "rb");
		if (fp) {
			char c;
			if (fread(&c, 1, 1, fp) == 1 && c == '1') {
				status = 0;
			}
			fclose(fp);
		}
#endif
		if (status != 0) {
			MIE_ALIGN(4096) char buf[4096];
			bool ret = Xbyak::CodeArray::protect(buf, sizeof(buf), true);
			status = ret ? 1 : 0;
			if (ret) {
				Xbyak::CodeArray::protect(buf, sizeof(buf), false);
			}
		}
	}
	return status != 0;
#else
	return false;
#endif
}

uint32_t sha256(void *out, uint32_t maxOutSize, const void *msg, uint32_t msgSize)
{
	return (uint32_t)cybozu::Sha256().digest(out, maxOutSize, msg, msgSize);
}

uint32_t sha512(void *out, uint32_t maxOutSize, const void *msg, uint32_t msgSize)
{
	return (uint32_t)cybozu::Sha512().digest(out, maxOutSize, msg, msgSize);
}

void expand_message_xmd(uint8_t out[], size_t outSize, const void *msg, size_t msgSize, const void *dst, size_t dstSize)
{
	const size_t mdSize = 32;
	assert((outSize % mdSize) == 0 && 0 < outSize && outSize <= 256);
	const size_t r_in_bytes = 64;
	const size_t n = outSize / mdSize;
	static const uint8_t Z_pad[r_in_bytes] = {};
	uint8_t largeDst[mdSize];
	if (dstSize > 255) {
		cybozu::Sha256 h;
		h.update("H2C-OVERSIZE-DST-", 17);
		h.digest(largeDst, mdSize, dst, dstSize);
		dst = largeDst;
		dstSize = mdSize;
	}
	/*
		Z_apd | msg | BE(outSize, 2) | BE(0, 1) | DST | BE(dstSize, 1)
	*/
	uint8_t lenBuf[2];
	uint8_t iBuf = 0;
	uint8_t dstSizeBuf = uint8_t(dstSize);
	cybozu::Set16bitAsBE(lenBuf, uint16_t(outSize));
	cybozu::Sha256 h;
	h.update(Z_pad, r_in_bytes);
	h.update(msg, msgSize);
	h.update(lenBuf, sizeof(lenBuf));
	h.update(&iBuf, 1);
	h.update(dst, dstSize);
	uint8_t md[mdSize];
	h.digest(md, mdSize, &dstSizeBuf, 1);
	h.clear();
	h.update(md, mdSize);
	iBuf = 1;
	h.update(&iBuf, 1);
	h.update(dst, dstSize);
	h.digest(out, mdSize, &dstSizeBuf, 1);
	uint8_t mdXor[mdSize];
	for (size_t i = 1; i < n; i++) {
		h.clear();
		for (size_t j = 0; j < mdSize; j++) {
			mdXor[j] = md[j] ^ out[mdSize * (i - 1) + j];
		}
		h.update(mdXor, mdSize);
		iBuf = uint8_t(i + 1);
		h.update(&iBuf, 1);
		h.update(dst, dstSize);
		h.digest(out + mdSize * i, mdSize, &dstSizeBuf, 1);
	}
}


#ifndef MCL_USE_VINT
static inline void set_mpz_t(mpz_t& z, const Unit* p, int n)
{
	int s = n;
	while (s > 0) {
		if (p[s - 1]) break;
		s--;
	}
	z->_mp_alloc = n;
	z->_mp_size = s;
	z->_mp_d = (mp_limb_t*)const_cast<Unit*>(p);
}
#endif

/*
	y = (1/x) mod op.p
*/
static inline void fp_invC(Unit *y, const Unit *x, const Op& op)
{
	const int N = (int)op.N;
	bool b = false;
#ifdef MCL_USE_VINT
	Vint vx, vy, vp;
	vx.setArray(&b, x, N);
	assert(b); (void)b;
	vp.setArray(&b, op.p, N);
	assert(b); (void)b;
	Vint::invMod(vy, vx, vp);
	vy.getArray(&b, y, N);
	assert(b); (void)b;
#else
	mpz_class my;
	mpz_t mx, mp;
	set_mpz_t(mx, x, N);
	set_mpz_t(mp, op.p, N);
	mpz_invert(my.get_mpz_t(), mx, mp);
	gmp::getArray(&b, y, N, my);
	assert(b);
#endif
}

/*
	inv(xR) = (1/x)R^-1 -toMont-> 1/x -toMont-> (1/x)R
*/
static void fp_invOpC(Unit *y, const Unit *x, const Op& op)
{
	fp_invC(y, x, op);
	if (op.isMont) op.fp_mul(y, y, op.R3, op.p);
}

/*
	large (N * 2) specification of AddPre, SubPre
*/
template<size_t N, bool enable>
struct SetFpDbl {
	static inline void exec(Op&) {}
};

template<size_t N>
struct SetFpDbl<N, true> {
	static inline void exec(Op& op)
	{
//		if (!op.isFullBit) {
			op.fpDbl_addPre = AddPre<N * 2, Ltag>::f;
			op.fpDbl_subPre = SubPre<N * 2, Ltag>::f;
//		}
	}
};

template<size_t N, bool isFullBit>
void Mul2(Unit *y, const Unit *x, const Unit *p)
{
#ifdef MCL_USE_LLVM
	Add<N, isFullBit, Ltag>::f(y, x, x, p);
#else
	const size_t bit = 1;
	const size_t rBit = sizeof(Unit) * 8 - bit;
	Unit tmp[N];
	Unit prev = x[N - 1];
	Unit H = isFullBit ? (x[N - 1] >> rBit) : 0;
	for (size_t i = N - 1; i > 0; i--) {
		Unit t = x[i - 1];
		tmp[i] = (prev << bit) | (t >> rBit);
		prev = t;
	}
	tmp[0] = prev << bit;
	bool c;
	if (isFullBit) {
		H -= SubPre<N, Gtag>::f(y, tmp, p);
		c = H >> rBit;
	} else {
		c = SubPre<N, Gtag>::f(y, tmp, p);
	}
	if (c) {
		copyC<N>(y, tmp);
	}
#endif
}

template<size_t N, class Tag, bool enableFpDbl, bool gmpIsFasterThanLLVM>
void setOp2(Op& op)
{
	op.fp_shr1 = Shr1<N, Tag>::f;
	op.fp_neg = Neg<N, Tag>::f;
	if (op.isFullBit) {
		op.fp_add = Add<N, true, Tag>::f;
		op.fp_sub = Sub<N, true, Tag>::f;
		op.fp_mul2 = Mul2<N, true>;
	} else {
		op.fp_add = Add<N, false, Tag>::f;
		op.fp_sub = Sub<N, false, Tag>::f;
		op.fp_mul2 = Mul2<N, false>;
	}
	if (op.isMont) {
		if (op.isFullBit) {
			op.fp_mul = Mont<N, true, Tag>::f;
			op.fp_sqr = SqrMont<N, true, Tag>::f;
			op.fpDbl_mod = MontRed<N, true, Tag>::f;
		} else {
			op.fp_mul = Mont<N, false, Tag>::f;
			op.fp_sqr = SqrMont<N, false, Tag>::f;
			op.fpDbl_mod = MontRed<N, false, Tag>::f;
		}
	} else {
		op.fp_mul = Mul<N, Tag>::f;
		op.fp_sqr = Sqr<N, Tag>::f;
		op.fpDbl_mod = Dbl_Mod<N, Tag>::f;
	}
	op.fp_mulUnit = MulUnit<N, Tag>::f;
	if (!gmpIsFasterThanLLVM) {
		op.fpDbl_mulPre = MulPre<N, Tag>::f;
		op.fpDbl_sqrPre = SqrPre<N, Tag>::f;
	}
	op.fp_mulUnitPre = MulUnitPre<N, Tag>::f;
	op.fpN1_mod = N1_Mod<N, Tag>::f;
	op.fpDbl_add = DblAdd<N, Tag>::f;
	op.fpDbl_sub = DblSub<N, Tag>::f;
	op.fp_addPre = AddPre<N, Tag>::f;
	op.fp_subPre = SubPre<N, Tag>::f;
	op.fp2_mulNF = Fp2MulNF<N, Tag>::f;
	SetFpDbl<N, enableFpDbl>::exec(op);
}

template<size_t N>
void setOp(Op& op, Mode mode)
{
	// generic setup
	op.fp_isZero = isZeroC<N>;
	op.fp_clear = clearC<N>;
	op.fp_copy = copyC<N>;
	op.fp_invOp = fp_invOpC;
	setOp2<N, Gtag, true, false>(op);
#ifdef MCL_USE_LLVM
	if (mode != fp::FP_GMP && mode != fp::FP_GMP_MONT) {
#if MCL_LLVM_BMI2 == 1
		const bool gmpIsFasterThanLLVM = false;//(N == 8 && MCL_SIZEOF_UNIT == 8);
		if (g_cpu.has(Xbyak::util::Cpu::tBMI2)) {
			setOp2<N, LBMI2tag, (N * UnitBitSize <= 384), gmpIsFasterThanLLVM>(op);
		} else
#endif
		{
			setOp2<N, Ltag, (N * UnitBitSize <= 384), false>(op);
		}
	}
#else
	(void)mode;
#endif
}

#ifdef MCL_X64_ASM
inline void invOpForMontC(Unit *y, const Unit *x, const Op& op)
{
	int k = op.fp_preInv(y, x);
	/*
		S = UnitBitSize
		xr = 2^k
		if isMont:
		R = 2^(N * S)
		get r2^(-k)R^2 = r 2^(N * S * 2 - k)
		else:
		r 2^(-k)
	*/
	op.fp_mul(y, y, op.invTbl.data() + k * op.N, op.p);
}

static void initInvTbl(Op& op)
{
	const size_t N = op.N;
	const Unit *p = op.p;
	const size_t invTblN = N * sizeof(Unit) * 8 * 2;
	op.invTbl.resize(invTblN * N);
	if (op.isMont) {
		Unit t[maxUnitSize] = {};
		t[0] = 2;
		Unit *tbl = op.invTbl.data() + (invTblN - 1) * N;
		op.toMont(tbl, t);
		for (size_t i = 0; i < invTblN - 1; i++) {
			op.fp_add(tbl - N, tbl, tbl, p);
			tbl -= N;
		}
	} else {
		/*
			half = 1/2
			tbl[i] = half^(i)
		*/
		Unit *tbl = op.invTbl.data();
		memset(tbl, 0, sizeof(Unit) * N);
		tbl[0] = 1;
		mpz_class half = (op.mp + 1) >> 1;
		bool b;
		mcl::gmp::getArray(&b, tbl + N, N, half);
		assert(b); (void)b;
		for (size_t i = 2; i < invTblN; i++) {
			op.fp_mul(tbl + N * i, tbl + N * (i-1), tbl + N, p);
		}
	}
}
#endif

static bool initForMont(Op& op, const Unit *p, Mode mode)
{
	const size_t N = op.N;
	bool b;
	{
		mpz_class t = 1, R;
		gmp::getArray(&b, op.one, N, t);
		if (!b) return false;
		R = (t << (N * UnitBitSize)) % op.mp;
		t = (R * R) % op.mp;
		gmp::getArray(&b, op.R2, N, t);
		if (!b) return false;
		t = (t * R) % op.mp;
		gmp::getArray(&b, op.R3, N, t);
		if (!b) return false;
	}
	op.rp = getMontgomeryCoeff(p[0]);

	(void)mode;
#ifdef MCL_X64_ASM

#ifdef MCL_USE_XBYAK
#ifndef MCL_DUMP_JIT
	if (mode != FP_XBYAK) return true;
#endif
	if (op.fg == 0) op.fg = Op::createFpGenerator();
	op.fg->init(op, g_cpu);
#ifdef MCL_DUMP_JIT
	return true;
#endif
#elif defined(MCL_STATIC_CODE)
	if (mode != FP_XBYAK) return true;
	fp::setStaticCode(op);
#endif // MCL_USE_XBYAK

#ifdef MCL_USE_VINT
	const int maxInvN = 6;
#else
	const int maxInvN = 4;
#endif
	if (op.fp_preInv && N <= maxInvN) {
		op.fp_invOp = &invOpForMontC;
		initInvTbl(op);
	}
#endif // MCL_X64_ASM
	return true;
}

#ifdef USE_WASM
template<size_t N>
void setWasmOp(Op& op)
{
	if (!(op.isMont && !op.isFullBit)) return;
//EM_ASM({console.log($0)}, N);
//	op.fp_addPre = mcl::addT<N>;
//	op.fp_subPre = mcl::subT<N>;
//	op.fpDbl_addPre = mcl::addT<N * 2>;
//	op.fpDbl_subPre = mcl::subT<N * 2>;
	op.fp_add = mcl::addModT<N>;
	op.fp_sub = mcl::subModT<N>;
	op.fp_mul = mcl::mulMontT<N>;
	op.fp_sqr = mcl::sqrMontT<N>;
	op.fpDbl_mulPre = mulPreT<N>;
//	op.fpDbl_sqrPre = sqrPreT<N>;
	op.fpDbl_mod = modT<N>;
}
#endif

bool Op::init(const mpz_class& _p, size_t maxBitSize, int _xi_a, Mode mode, size_t mclMaxBitSize)
{
	if (mclMaxBitSize != MCL_MAX_BIT_SIZE) return false;
#ifdef MCL_USE_VINT
	assert(sizeof(mcl::vint::Unit) == sizeof(Unit));
#else
	assert(sizeof(mp_limb_t) == sizeof(Unit));
#endif
	if (maxBitSize > MCL_MAX_BIT_SIZE) return false;
	if (_p <= 0) return false;
	clear();
	maxN = (maxBitSize + fp::UnitBitSize - 1) / fp::UnitBitSize;
	N = gmp::getUnitSize(_p);
	if (N > maxN) return false;
	{
		bool b;
		gmp::getArray(&b, p, N, _p);
		if (!b) return false;
	}
	mp = _p;
	bitSize = gmp::getBitSize(mp);
	pmod4 = gmp::getUnit(mp, 0) % 4;
	this->xi_a = _xi_a;
/*
	priority : MCL_USE_XBYAK > MCL_USE_LLVM > none
	Xbyak > llvm_mont > llvm > gmp_mont > gmp
*/
#ifdef MCL_X64_ASM
	if (mode == FP_AUTO) mode = FP_XBYAK;
	if (mode == FP_XBYAK && bitSize > 384) {
		mode = FP_AUTO;
	}
#ifdef MCL_USE_XBYAK
	if (!isEnableJIT()) {
		mode = FP_AUTO;
	}
#elif defined(MCL_STATIC_CODE)
	{
		// static jit code uses avx, mulx, adox, adcx
		using namespace Xbyak::util;
		if (!(g_cpu.has(Cpu::tAVX) && g_cpu.has(Cpu::tBMI2) && g_cpu.has(Cpu::tADX))) {
			mode = FP_AUTO;
		}
	}
#endif
#else
	if (mode == FP_XBYAK) mode = FP_AUTO;
#endif
#ifdef MCL_USE_LLVM
	if (mode == FP_AUTO) mode = FP_LLVM_MONT;
#else
	if (mode == FP_LLVM || mode == FP_LLVM_MONT) mode = FP_AUTO;
#endif
	if (mode == FP_AUTO) mode = FP_GMP_MONT;
	isMont = mode == FP_GMP_MONT || mode == FP_LLVM_MONT || mode == FP_XBYAK;
#if 0
	fprintf(stderr, "mode=%s, isMont=%d, maxBitSize=%d"
#ifdef MCL_USE_XBYAK
		" MCL_USE_XBYAK"
#endif
#ifdef MCL_USE_LLVM
		" MCL_USE_LLVM"
#endif
	"\n", ModeToStr(mode), isMont, (int)maxBitSize);
#endif
	isFullBit = (bitSize % UnitBitSize) == 0;

#if defined(MCL_USE_LLVM) || defined(MCL_USE_XBYAK)
	if (mode == FP_AUTO || mode == FP_LLVM || mode == FP_XBYAK) {
		const struct {
			PrimeMode mode;
			const char *str;
		} tbl[] = {
			{ PM_NIST_P192, "0xfffffffffffffffffffffffffffffffeffffffffffffffff" },
#if MCL_MAX_BIT_SIZE >= 521
			{ PM_NIST_P521, "0x1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" },
#endif
		};
		// use fastMode for special primes
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			bool b;
			mpz_class target;
			gmp::setStr(&b, target, tbl[i].str);
			if (b && mp == target) {
				primeMode = tbl[i].mode;
				isMont = false;
				isFastMod = true;
				break;
			}
		}
	}
#endif
	if (mode == FP_XBYAK
#if defined(MCL_USE_VINT)
		|| mode != FP_LLVM
#endif
	) {
		const char *secp256k1Str = "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f";
		bool b;
		mpz_class secp256k1;
		gmp::setStr(&b, secp256k1, secp256k1Str);
		if (b && mp == secp256k1) {
			primeMode = PM_SECP256K1;
			isMont = false;
			isFastMod = true;
		}
	}
	switch (N) {
	case 192/(MCL_SIZEOF_UNIT * 8):  setOp<192/(MCL_SIZEOF_UNIT * 8)>(*this, mode); break;
#if (MCL_SIZEOF_UNIT * 8) == 32
	case 224/(MCL_SIZEOF_UNIT * 8):  setOp<224/(MCL_SIZEOF_UNIT * 8)>(*this, mode); break;
#endif
	case 256/(MCL_SIZEOF_UNIT * 8):  setOp<256/(MCL_SIZEOF_UNIT * 8)>(*this, mode); break;
#if MCL_MAX_BIT_SIZE >= 320
	case 320/(MCL_SIZEOF_UNIT * 8):  setOp<320/(MCL_SIZEOF_UNIT * 8)>(*this, mode); break;
#endif
#if MCL_MAX_BIT_SIZE >= 384
	case 384/(MCL_SIZEOF_UNIT * 8):  setOp<384/(MCL_SIZEOF_UNIT * 8)>(*this, mode); break;
#endif
#if MCL_MAX_BIT_SIZE >= 512
	case 512/(MCL_SIZEOF_UNIT * 8):  setOp<512/(MCL_SIZEOF_UNIT * 8)>(*this, mode); break;
#endif
#if MCL_MAX_BIT_SIZE >= 576
	case 576/(MCL_SIZEOF_UNIT * 8):  setOp<576/(MCL_SIZEOF_UNIT * 8)>(*this, mode); break;
#endif
	default:
		return false;
	}
#if defined(USE_WASM) && MCL_SIZEOF_UNIT == 4
	if (N == 8) {
		setWasmOp<8>(*this);
	} else if (N == 12) {
		setWasmOp<12>(*this);
	}
#endif
#ifdef MCL_USE_LLVM
	if (primeMode == PM_NIST_P192) {
		fp_mul = &mcl_fp_mulNIST_P192L;
		fp_sqr = &mcl_fp_sqr_NIST_P192L;
		fpDbl_mod = &mcl_fpDbl_mod_NIST_P192L;
	}
#if MCL_MAX_BIT_SIZE >= 521
	if (primeMode == PM_NIST_P521) {
		fpDbl_mod = &mcl_fpDbl_mod_NIST_P521L;
	}
#endif
#endif
#if defined(MCL_USE_VINT)
	if (mode != FP_XBYAK && primeMode == PM_SECP256K1) {
#if defined(USE_WASM) && MCL_SIZEOF_UNIT == 4
		fp_mul = &mcl::mcl_fp_mul_SECP256K1_wasm;
		fp_sqr = &mcl::mcl_fp_sqr_SECP256K1_wasm;
#else
		fp_mul = &mcl::vint::mcl_fp_mul_SECP256K1;
		fp_sqr = &mcl::vint::mcl_fp_sqr_SECP256K1;
#endif
		fpDbl_mod = &mcl::vint::mcl_fpDbl_mod_SECP256K1;
	}
#endif
	if (N * UnitBitSize <= 256) {
		hash = sha256;
	} else {
		hash = sha512;
	}
	{
		bool b;
		sq.set(&b, mp);
		if (!b) return false;
	}
	modp.init(mp);
	smallModp.init(mp);
	return fp::initForMont(*this, p, mode);
}

#ifndef CYBOZU_DONT_USE_STRING
int detectIoMode(int ioMode, const std::ios_base& ios)
{
	if (ioMode & ~IoPrefix) return ioMode;
	// IoAuto or IoPrefix
	const std::ios_base::fmtflags f = ios.flags();
	assert(!(f & std::ios_base::oct));
	ioMode |= (f & std::ios_base::hex) ? IoHex : 0;
	if (f & std::ios_base::showbase) {
		ioMode |= IoPrefix;
	}
	return ioMode;
}
#endif

static bool isInUint64(uint64_t *pv, const fp::Block& b)
{
	assert(fp::UnitBitSize == 32 || fp::UnitBitSize == 64);
	const size_t start = 64 / fp::UnitBitSize;
	for (size_t i = start; i < b.n; i++) {
		if (b.p[i]) return false;
	}
#if MCL_SIZEOF_UNIT == 4
	*pv = b.p[0] | (uint64_t(b.p[1]) << 32);
#else
	*pv = b.p[0];
#endif
	return true;
}

uint64_t getUint64(bool *pb, const fp::Block& b)
{
	uint64_t v;
	if (isInUint64(&v, b)) {
		*pb = true;
		return v;
	}
	*pb = false;
	return 0;
}

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4146)
#endif

int64_t getInt64(bool *pb, fp::Block& b, const fp::Op& op)
{
	bool isNegative = false;
	if (fp::isGreaterOrEqualArray(b.p, op.half, op.N)) {
		op.fp_neg(b.v_, b.p, op.p);
		b.p = b.v_;
		isNegative = true;
	}
	uint64_t v;
	if (fp::isInUint64(&v, b)) {
		const uint64_t c = uint64_t(1) << 63;
		if (isNegative) {
			if (v <= c) { // include c
				*pb = true;
				// -1 << 63
				if (v == c) return int64_t(-9223372036854775807ll - 1);
				return int64_t(-v);
			}
		} else {
			if (v < c) { // not include c
				*pb = true;
				return int64_t(v);
			}
		}
	}
	*pb = false;
	return 0;
}

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

} } // mcl::fp

