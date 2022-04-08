#pragma once
/**
	@file
	@brief Fp generator
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#if CYBOZU_HOST == CYBOZU_HOST_INTEL

#if MCL_SIZEOF_UNIT == 8
#include <stdio.h>
#include <assert.h>
#include <cybozu/exception.hpp>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4127)
	#pragma warning(disable : 4458)
#endif

namespace mcl {

#ifdef MCL_DUMP_JIT
struct DumpCode {
	FILE *fp_;
	DumpCode()
		: fp_(stdout)
	{
	}
	void set(const std::string& name, const uint8_t *begin, const size_t size)
	{
		fprintf(fp_, "segment .text\n");
		fprintf(fp_, "global %s\n", name.c_str());
		fprintf(fp_, "align 16\n");
		fprintf(fp_, "%s:\n", name.c_str());
		const uint8_t *p = begin;
		size_t remain = size;
		while (remain > 0) {
			size_t n = remain >= 16 ? 16 : remain;
			fprintf(fp_, "db ");
			for (size_t i = 0; i < n; i++) {
				fprintf(fp_, "0x%02x,", *p++);
			}
			fprintf(fp_, "\n");
			remain -= n;
		}
	}
	void dumpData(const void *begin, const void *end)
	{
		fprintf(fp_, "align 16\n");
		fprintf(fp_, "dq ");
		const uint64_t *p = (const uint64_t*)begin;
		const uint64_t *pe = (const uint64_t*)end;
		const size_t n = pe - p;
		for (size_t i = 0; i < n; i++) {
			fprintf(fp_, "0x%016llx,", (unsigned long long)*p++);
		}
		fprintf(fp_, "\n");
	}
};
template<class T>
void setFuncInfo(DumpCode& prof, const char *suf, const char *name, const T& begin, const uint8_t* end)
{
	if (suf == 0) suf = "";
	const uint8_t*p = (const uint8_t*)begin;
#ifdef __APPLE__
	std::string pre = "_mclx_";
#else
	std::string pre = "mclx_";
#endif
	prof.set(pre + suf + name, p, end - p);
}
#else
template<class T>
void setFuncInfo(Xbyak::util::Profiler& prof, const char *suf, const char *name, const T& begin, const uint8_t* end)
{
	if (suf == 0) suf = "";
	const uint8_t*p = (const uint8_t*)begin;
	prof.set((std::string("mclx_") + suf + name).c_str(), p, end - p);
}
#endif

namespace fp_gen_local {

class MemReg {
	const Xbyak::Reg64 *r_;
	const Xbyak::RegExp *m_;
	size_t offset_;
public:
	MemReg(const Xbyak::Reg64 *r, const Xbyak::RegExp *m, size_t offset) : r_(r), m_(m), offset_(offset) {}
	bool isReg() const { return r_ != 0; }
	const Xbyak::Reg64& getReg() const { return *r_; }
	Xbyak::RegExp getMem() const { return *m_ + offset_ * sizeof(size_t); }
};

struct MixPack {
	static const size_t useAll = 100;
	Xbyak::util::Pack p;
	Xbyak::RegExp m;
	size_t mn;
	MixPack() : mn(0) {}
	MixPack(Xbyak::util::Pack& remain, size_t& rspPos, size_t n, size_t useRegNum = useAll)
	{
		init(remain, rspPos, n, useRegNum);
	}
	void init(Xbyak::util::Pack& remain, size_t& rspPos, size_t n, size_t useRegNum = useAll)
	{
		size_t pn = (std::min)(remain.size(), n);
		if (useRegNum != useAll && useRegNum < pn) pn = useRegNum;
		this->mn = n - pn;
		this->m = Xbyak::util::rsp + rspPos;
		this->p = remain.sub(0, pn);
		remain = remain.sub(pn);
		rspPos += mn * 8;
	}
	size_t size() const { return p.size() + mn; }
	bool isReg(size_t n) const { return n < p.size(); }
	const Xbyak::Reg64& getReg(size_t n) const
	{
		assert(n < p.size());
		return p[n];
	}
	Xbyak::RegExp getMem(size_t n) const
	{
		const size_t pn = p.size();
		assert(pn <= n && n < size());
		return m + (int)((n - pn) * sizeof(size_t));
	}
	MemReg operator[](size_t n) const
	{
		const size_t pn = p.size();
		return MemReg((n < pn) ? &p[n] : 0, (n < pn) ? 0 : &m, n - pn);
	}
	void removeLast()
	{
		assert(size());
		if (mn > 0) {
			mn--;
		} else {
			p = p.sub(0, p.size() - 1);
		}
	}
	/*
		replace Mem with r if possible
	*/
	bool replaceMemWith(Xbyak::CodeGenerator *code, const Xbyak::Reg64& r)
	{
		if (mn == 0) return false;
		p.append(r);
		code->mov(r, code->ptr [m]);
		m = m + 8;
		mn--;
		return true;
	}
};

} // fp_gen_local

/*
	op(r, rm);
	r  : reg
	rm : Reg/Mem
*/
#define MCL_FP_GEN_OP_RM(op, r, rm) \
if (rm.isReg()) { \
	op(r, rm.getReg()); \
} else { \
	op(r, qword [rm.getMem()]); \
}

/*
	op(rm, r);
	rm : Reg/Mem
	r  : reg
*/
#define MCL_FP_GEN_OP_MR(op, rm, r) \
if (rm.isReg()) { \
	op(rm.getReg(), r); \
} else { \
	op(qword [rm.getMem()], r); \
}

namespace fp {

struct FpGenerator : Xbyak::CodeGenerator {
	typedef Xbyak::RegExp RegExp;
	typedef Xbyak::Reg64 Reg64;
	typedef Xbyak::Xmm Xmm;
	typedef Xbyak::Operand Operand;
	typedef Xbyak::Label Label;
	typedef Xbyak::util::StackFrame StackFrame;
	typedef Xbyak::util::Pack Pack;
	typedef fp_gen_local::MixPack MixPack;
	typedef fp_gen_local::MemReg MemReg;
	static const int UseRDX = Xbyak::util::UseRDX;
	static const int UseRCX = Xbyak::util::UseRCX;
	/*
		classes to calculate offset and size
	*/
	struct Ext1 {
		Ext1(int FpByte, const Reg64& r, int n = 0)
			: r_(r)
			, n_(n)
			, next(FpByte + n)
		{
		}
		operator RegExp() const { return r_ + n_; }
		const Reg64& r_;
		const int n_;
		const int next;
	private:
		Ext1(const Ext1&);
		void operator=(const Ext1&);
	};
	struct Ext2 {
		Ext2(int FpByte, const Reg64& r, int n = 0)
			: r_(r)
			, n_(n)
			, next(FpByte * 2 + n)
			, a(FpByte, r, n)
			, b(FpByte, r, n + FpByte)
		{
		}
		operator RegExp() const { return r_ + n_; }
		const Reg64& r_;
		const int n_;
		const int next;
		Ext1 a;
		Ext1 b;
	private:
		Ext2(const Ext2&);
		void operator=(const Ext2&);
	};
	const Reg64& gp0;
	const Reg64& gp1;
	const Reg64& gp2;
	const Reg64& gt0;
	const Reg64& gt1;
	const Reg64& gt2;
	const Reg64& gt3;
	const Reg64& gt4;
	const Reg64& gt5;
	const Reg64& gt6;
	const Reg64& gt7;
	const Reg64& gt8;
	const Reg64& gt9;
	const mcl::fp::Op *op_;
	Label pL_; // pointer to p
	// the following labels assume sf(this, 3, 10 | UseRDX)
	Label fp_mulPreL;
	Label fp_sqrPreL;
	Label fpDbl_modL;
	Label fp_mulL;
	Label fp2Dbl_mulPreL;
	const uint64_t *p_;
	uint64_t rp_;
	int pn_;
	int FpByte_;
	bool isFullBit_;
	bool useMulx_;
	bool useAdx_;
#ifdef MCL_DUMP_JIT
	DumpCode prof_;
#else
	Xbyak::util::Profiler prof_;
#endif

	/*
		@param op [in] ; use op.p, op.N, op.isFullBit
	*/
	FpGenerator()
		: CodeGenerator(4096 * 9, Xbyak::DontSetProtectRWE)
#ifdef XBYAK64_WIN
		, gp0(rcx)
		, gp1(r11)
		, gp2(r8)
		, gt0(r9)
		, gt1(r10)
		, gt2(rdi)
		, gt3(rsi)
#else
		, gp0(rdi)
		, gp1(rsi)
		, gp2(r11)
		, gt0(rcx)
		, gt1(r8)
		, gt2(r9)
		, gt3(r10)
#endif
		, gt4(rbx)
		, gt5(rbp)
		, gt6(r12)
		, gt7(r13)
		, gt8(r14)
		, gt9(r15)
		, op_(0)
		, p_(0)
		, rp_(0)
		, pn_(0)
		, FpByte_(0)
	{
	}
	bool init(Op& op, const Xbyak::util::Cpu& cpu)
	{
#ifdef MCL_DUMP_JIT
		useMulx_ = true;
		useAdx_ = true;
		(void)cpu;
#else
		if (!cpu.has(Xbyak::util::Cpu::tAVX)) return false;
		useMulx_ = cpu.has(Xbyak::util::Cpu::tBMI2);
		useAdx_ = cpu.has(Xbyak::util::Cpu::tADX);
		if (!(useMulx_ && useAdx_)) return false;
#endif
		reset(); // reset jit code for reuse
#ifndef MCL_DUMP_JIT
		setProtectModeRW(); // read/write memory
#endif
		init_inner(op);
		// ToDo : recover op if false
		if (Xbyak::GetError()) return false;
#ifndef NDEBUG
		if (hasUndefinedLabel()) {
			fprintf(stderr, "fp_generator has bugs.\n");
			exit(1);
			return false;
		}
#endif
//		printf("code size=%d\n", (int)getSize());
#ifndef MCL_DUMP_JIT
		setProtectModeRE(); // set read/exec memory
#endif
		return true;
	}
private:
	void init_inner(Op& op)
	{
		const char *suf = op.xi_a ? "Fp" : "Fr";
		op_ = &op;
		L(pL_);
		p_ = reinterpret_cast<const uint64_t*>(getCurr());
		for (size_t i = 0; i < op.N; i++) {
			dq(op.p[i]);
		}
#ifdef MCL_DUMP_JIT
		prof_.dumpData(p_, getCurr());
#endif
		rp_ = fp::getMontgomeryCoeff(p_[0]);
		pn_ = (int)op.N;
		FpByte_ = int(op.maxN * sizeof(uint64_t));
		isFullBit_ = op.isFullBit;
//		printf("p=%p, pn_=%d, isFullBit_=%d\n", p_, pn_, isFullBit_);
#ifdef MCL_USE_PROF
		int profMode = 0;
#ifdef XBYAK_USE_VTUNE
		profMode = 2;
#endif
		{
			const char *s = getenv("MCL_PROF");
			if (s && s[0] && s[1] == '\0') profMode = s[0] - '0';
		}
		if (profMode) {
			prof_.init(profMode);
			prof_.setStartAddr(getCurr());
		}
#else
		(void)suf;
#endif

		if (gen_addSubPre(op.fp_addPre, true, pn_)) {
			setFuncInfo(prof_, suf, "_addPre", op.fp_addPre, getCurr());
		}

		if (gen_addSubPre(op.fp_subPre, false, pn_)) {
			setFuncInfo(prof_, suf, "_subPre", op.fp_subPre, getCurr());
		}

		if (gen_fp_add(op.fp_addA_)) {
			setFuncInfo(prof_, suf, "_add", op.fp_addA_, getCurr());
		}

		if (gen_fp_sub(op.fp_subA_)) {
			setFuncInfo(prof_, suf, "_sub", op.fp_subA_, getCurr());
		}

		if (gen_shr1(op.fp_shr1)) {
			setFuncInfo(prof_, suf, "_shr1", op.fp_shr1, getCurr());
		}

		if (gen_mul2(op.fp_mul2A_)) {
			setFuncInfo(prof_, suf, "_mul2", op.fp_mul2A_, getCurr());
		}

		if (gen_fp_neg(op.fp_negA_)) {
			setFuncInfo(prof_, suf, "_neg", op.fp_negA_, getCurr());
		}
		if (gen_fpDbl_mod(op.fpDbl_modA_, op)) {
			setFuncInfo(prof_, suf, "Dbl_mod", op.fpDbl_modA_, getCurr());
		}

		if (gen_preInv(op.fp_preInv, op)) {
			setFuncInfo(prof_, suf, "_preInv", op.fp_preInv, getCurr());
		}

		// call from Fp::mul and Fp::sqr
		if (gen_fpDbl_mulPre(op.fpDbl_mulPre)) {
			setFuncInfo(prof_, suf, "Dbl_mulPre", op.fpDbl_mulPre, getCurr());
		}

		if (gen_fpDbl_sqrPre(op.fpDbl_sqrPre)) {
			setFuncInfo(prof_, suf, "Dbl_sqrPre", op.fpDbl_sqrPre, getCurr());
		}
		if (gen_mul(op.fp_mulA_)) {
			setFuncInfo(prof_, suf, "_mul", op.fp_mulA_, getCurr());
			op.fp_mul = fp::func_ptr_cast<void4u>(op.fp_mulA_); // used in toMont/fromMont
		}

		if (gen_sqr(op.fp_sqrA_)) {
			setFuncInfo(prof_, suf, "_sqr", op.fp_sqrA_, getCurr());
		}
		if (op.xi_a == 0) return; // Fp2 is not used

		if (gen_fpDbl_add(op.fpDbl_addA_)) {
			setFuncInfo(prof_, suf, "Dbl_add", op.fpDbl_addA_, getCurr());
		}

		if (gen_fpDbl_sub(op.fpDbl_subA_)) {
			setFuncInfo(prof_, suf, "Dbl_sub", op.fpDbl_subA_, getCurr());
		}

		if (gen_addSubPre(op.fpDbl_addPre, true, pn_ * 2)) {
			setFuncInfo(prof_, suf, "Dbl_addPre", op.fpDbl_addPre, getCurr());
		}

		if (gen_addSubPre(op.fpDbl_subPre, false, pn_ * 2)) {
			setFuncInfo(prof_, suf, "Dbl_subPre", op.fpDbl_subPre, getCurr());
		}

		if (gen_fp2_add(op.fp2_addA_)) {
			setFuncInfo(prof_, suf, "2_add", op.fp2_addA_, getCurr());
		}

		if (gen_fp2_sub(op.fp2_subA_)) {
			setFuncInfo(prof_, suf, "2_sub", op.fp2_subA_, getCurr());
		}

		if (gen_fp2_neg(op.fp2_negA_)) {
			setFuncInfo(prof_, suf, "2_neg", op.fp2_negA_, getCurr());
		}

		if (gen_fp2_mul2(op.fp2_mul2A_)) {
			setFuncInfo(prof_, suf, "2_mul2", op.fp2_mul2A_, getCurr());
		}

		op.fp2_mulNF = 0;
		if (gen_fp2Dbl_mulPre(op.fp2Dbl_mulPreA_)) {
			setFuncInfo(prof_, suf, "2Dbl_mulPre", op.fp2Dbl_mulPreA_, getCurr());
		}

		if (gen_fp2Dbl_sqrPre(op.fp2Dbl_sqrPreA_)) {
			setFuncInfo(prof_, suf, "2Dbl_sqrPre", op.fp2Dbl_sqrPreA_, getCurr());
		}

		if (gen_fp2Dbl_mul_xi(op.fp2Dbl_mul_xiA_)) {
			setFuncInfo(prof_, suf, "2Dbl_mul_xi", op.fp2Dbl_mul_xiA_, getCurr());
		}

		if (gen_fp2_mul(op.fp2_mulA_)) {
			setFuncInfo(prof_, suf, "2_mul", op.fp2_mulA_, getCurr());
		}

		if (gen_fp2_sqr(op.fp2_sqrA_)) {
			setFuncInfo(prof_, suf, "2_sqr", op.fp2_sqrA_, getCurr());
		}

		if (gen_fp2_mul_xi(op.fp2_mul_xiA_)) {
			setFuncInfo(prof_, suf, "2_mul_xi", op.fp2_mul_xiA_, getCurr());
		}
	}
	bool gen_addSubPre(u3u& func, bool isAdd, int n)
	{
		align(16);
		func = getCurr<u3u>();
		StackFrame sf(this, 3);
		if (isAdd) {
			gen_raw_add(sf.p[0], sf.p[1], sf.p[2], rax, n);
		} else {
			gen_raw_sub(sf.p[0], sf.p[1], sf.p[2], rax, n);
		}
		return true;
	}
	/*
		pz[] = px[] + py[]
	*/
	void gen_raw_add(const RegExp& pz, const RegExp& px, const RegExp& py, const Reg64& t, int n)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [px + i * 8]);
			if (i == 0) {
				add(t, ptr [py + i * 8]);
			} else {
				adc(t, ptr [py + i * 8]);
			}
			mov(ptr [pz + i * 8], t);
		}
	}
	/*
		pz[] = px[] - py[]
	*/
	void gen_raw_sub(const RegExp& pz, const RegExp& px, const RegExp& py, const Reg64& t, int n)
	{
		mov(t, ptr [px]);
		sub(t, ptr [py]);
		mov(ptr [pz], t);
		for (int i = 1; i < n; i++) {
			mov(t, ptr [px + i * 8]);
			sbb(t, ptr [py + i * 8]);
			mov(ptr [pz + i * 8], t);
		}
	}
	/*
		pz[] = -px[]
		use rax, rdx
	*/
	void gen_raw_neg(const RegExp& pz, const RegExp& px, const Pack& t)
	{
		Label nonZero, exit;
		load_rm(t, px);
		mov(rax, t[0]);
		if (t.size() > 1) {
			for (size_t i = 1; i < t.size(); i++) {
				or_(rax, t[i]);
			}
		} else {
			test(rax, rax);
		}
		jnz(nonZero);
		// rax = 0
		for (size_t i = 0; i < t.size(); i++) {
			mov(ptr[pz + i * 8], rax);
		}
		jmp(exit);
	L(nonZero);
		lea(rax, ptr[rip+pL_]);
		for (size_t i = 0; i < t.size(); i++) {
			mov(rdx, ptr [rax + i * 8]);
			if (i == 0) {
				sub(rdx, t[i]);
			} else {
				sbb(rdx, t[i]);
			}
			mov(ptr [pz + i * 8], rdx);
		}
	L(exit);
	}
	/*
		(rdx:pz[0..n-1]) = px[0..n-1] * y
		use t, rax, rdx
		if n > 2
		use
		wk[0] if useMulx_
		wk[0..n-2] otherwise
	*/
	void gen_raw_mulUnit(const RegExp& pz, const RegExp& px, const Reg64& y, const MixPack& wk, const Reg64& t, size_t n)
	{
		if (n == 1) {
			mov(rax, ptr [px]);
			mul(y);
			mov(ptr [pz], rax);
			return;
		}
		if (n == 2) {
			mov(rax, ptr [px]);
			mul(y);
			mov(ptr [pz], rax);
			mov(t, rdx);
			mov(rax, ptr [px + 8]);
			mul(y);
			add(rax, t);
			adc(rdx, 0);
			mov(ptr [pz + 8], rax);
			return;
		}
		assert(wk.size() > 0 && wk.isReg(0));
		const Reg64& t1 = wk.getReg(0);
		// mulx(H, L, x) = [H:L] = x * rdx
		mov(rdx, y);
		mulx(t1, rax, ptr [px]); // [y:rax] = px * y
		mov(ptr [pz], rax);
		const Reg64 *pt0 = &t;
		const Reg64 *pt1 = &t1;
		for (size_t i = 1; i < n - 1; i++) {
			mulx(*pt0, rax, ptr [px + i * 8]);
			if (i == 1) {
				add(rax, *pt1);
			} else {
				adc(rax, *pt1);
			}
			mov(ptr [pz + i * 8], rax);
			std::swap(pt0, pt1);
		}
		mulx(rdx, rax, ptr [px + (n - 1) * 8]);
		adc(rax, *pt1);
		mov(ptr [pz + (n - 1) * 8], rax);
		adc(rdx, 0);
	}
	void gen_mulUnit()
	{
//		assert(pn_ >= 2);
		const int regNum = 2;
		const int stackSize = 0;
		StackFrame sf(this, 3, regNum | UseRDX, stackSize);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& y = sf.p[2];
		size_t rspPos = 0;
		Pack remain = sf.t.sub(1);
		MixPack wk(remain, rspPos, pn_ - 1);
		gen_raw_mulUnit(pz, px, y, wk, sf.t[0], pn_);
		mov(rax, rdx);
	}
	/*
		pz[] = px[]
	*/
	void mov_mm(const RegExp& pz, const RegExp& px, const Reg64& t, int n)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [px + i * 8]);
			mov(ptr [pz + i * 8], t);
		}
	}
	void gen_raw_fp_add(const RegExp& pz, const RegExp& px, const RegExp& py, const Pack& t, bool withCarry = false, const Reg64 *H = 0)
	{
		const Pack& t1 = t.sub(0, pn_);
		const Pack& t2 = t.sub(pn_, pn_);
		load_rm(t1, px);
		add_rm(t1, py, withCarry);
		if (H) {
			mov(*H, 0);
			adc(*H, 0);
		}
		sub_p_mod(t2, t1, rip + pL_, H);
		store_mr(pz, t2);
	}
	bool gen_fp_add(void3u& func)
	{
		if (!(pn_ < 6 || (pn_ == 6 && !isFullBit_))) return false;
		align(16);
		func = getCurr<void3u>();
		int n = pn_ * 2 - 1;
		if (isFullBit_) {
			n++;
		}
		StackFrame sf(this, 3, n);

		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];
		Pack t = sf.t;
		t.append(rax);
		const Reg64 *H = isFullBit_ ? &rax : 0;
		gen_raw_fp_add(pz, px, py, t, false, H);
		return true;
	}
	bool gen_fpDbl_add(void3u& func)
	{
		if (!(pn_ < 6 || (pn_ == 6 && !isFullBit_))) return false;
		align(16);
		func = getCurr<void3u>();
		int n = pn_ * 2 - 1;
		if (isFullBit_) {
			n++;
		}
		StackFrame sf(this, 3, n);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];
		Pack t = sf.t;
		t.append(rax);
		const Reg64 *H = isFullBit_ ? &rax : 0;
		gen_raw_add(pz, px, py, rax, pn_);
		gen_raw_fp_add(pz + 8 * pn_, px + 8 * pn_, py + 8 * pn_, t, true, H);
		return true;
	}
	bool gen_fpDbl_sub(void3u& func)
	{
		if (pn_ > 6) return false;
		align(16);
		func = getCurr<void3u>();
		int n = pn_ * 2 - 1;
		StackFrame sf(this, 3, n);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];
		Pack t = sf.t;
		t.append(rax);
		gen_raw_sub(pz, px, py, rax, pn_);
		gen_raw_fp_sub(pz + pn_ * 8, px + pn_ * 8, py + pn_ * 8, t, true);
		return true;
	}
	// require t.size() >= pn_ * 2
	void gen_raw_fp_sub(const RegExp& pz, const RegExp& px, const RegExp& py, const Pack& t, bool withCarry)
	{
		Pack t1 = t.sub(0, pn_);
		Pack t2 = t.sub(pn_, pn_);
		load_rm(t1, px);
		sub_rm(t1, py, withCarry);
		push(t1[0]);
		lea(t1[0], ptr[rip + pL_]);
		load_rm(t2, t1[0]);
		sbb(t1[0], t1[0]);
		and_pr(t2, t1[0]);
		pop(t1[0]);
		add_rr(t1, t2);
		store_mr(pz, t1);
	}
	bool gen_fp_sub(void3u& func)
	{
		if (pn_ > 6) return false;
		align(16);
		func = getCurr<void3u>();
		/*
			micro-benchmark of jmp is faster than and-mask
			but it's slower for pairings
		*/
		int n = pn_ * 2 - 1;
		StackFrame sf(this, 3, n);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];
		Pack t = sf.t;
		t.append(rax);
		gen_raw_fp_sub(pz, px, py, t, false);
		return true;
	}
	bool gen_fp_neg(void2u& func)
	{
		align(16);
		func = getCurr<void2u>();
		StackFrame sf(this, 2, UseRDX | pn_);
		gen_raw_neg(sf.p[0], sf.p[1], sf.t);
		return true;
	}
	bool gen_shr1(void2u& func)
	{
		align(16);
		func = getCurr<void2u>();
		const int c = 1;
		StackFrame sf(this, 2, 1);
		const Reg64 *t0 = &rax;
		const Reg64 *t1 = &sf.t[0];
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		mov(*t0, ptr [px]);
		for (int i = 0; i < pn_ - 1; i++) {
			mov(*t1, ptr [px + 8 * (i + 1)]);
			shrd(*t0, *t1, c);
			mov(ptr [pz + i * 8], *t0);
			std::swap(t0, t1);
		}
		shr(*t0, c);
		mov(ptr [pz + (pn_ - 1) * 8], *t0);
		return true;
	}
	// x = x << 1
	// H = top bit of x
	void shl1(const Pack& x, const Reg64 *H = 0)
	{
		const int n = (int)x.size();
		if (H) {
			mov(*H, x[n - 1]);
			shr(*H, 63);
		}
		for (int i = n - 1; i > 0; i--) {
			shld(x[i], x[i - 1], 1);
		}
		shl(x[0], 1);
	}
	/*
		y = (x >= p[]) x - p[] : x
	*/
	template<class ADDR>
	void sub_p_mod(const Pack& y, const Pack& x, const ADDR& p, const Reg64 *H = 0)
	{
		mov_rr(y, x);
		sub_rm(y, p);
		if (H) {
			sbb(*H, 0);
		}
		cmovc_rr(y, x);
	}
	bool gen_mul2(void2u& func)
	{
		if (pn_ > 6) return false;
		align(16);
		func = getCurr<void2u>();
		int n = pn_ * 2 - 1;
		StackFrame sf(this, 2, n + (isFullBit_ ? 1 : 0));
		Pack x = sf.t.sub(0, pn_);
		Pack t = sf.t.sub(pn_, n - pn_);
		t.append(sf.p[1]);
		lea(rax, ptr[rip + pL_]);
		load_rm(x, sf.p[1]);
		if (isFullBit_) {
			const Reg64& H = sf.t[n];
			shl1(x, &H);
			sub_p_mod(t, x, rax, &H);
		} else {
			shl1(x);
			sub_p_mod(t, x, rax);
		}
		store_mr(sf.p[0], t);
		return true;
	}
	bool gen_fp2_mul2(void2u& func)
	{
		if (isFullBit_ || pn_ > 6) return false;
		align(16);
		func = getCurr<void2u>();
		int n = pn_ * 2;
		StackFrame sf(this, 2, n);
		Pack x = sf.t.sub(0, pn_);
		Pack t = sf.t.sub(pn_, pn_);
		lea(rax, ptr[rip + pL_]);
		for (int i = 0; i < 2; i++) {
			load_rm(x, sf.p[1] + FpByte_ * i);
			shl1(x);
			sub_p_mod(t, x, rax);
			store_mr(sf.p[0] + FpByte_ * i, t);
		}
		return true;
	}
	bool gen_mul(void3u& func)
	{
		align(16);
		if (op_->primeMode == PM_NIST_P192) {
			func = getCurr<void3u>();
			StackFrame sf(this, 3, 10 | UseRDX, 8 * 6);
			mulPre3(rsp, sf.p[1], sf.p[2], sf.t);
			fpDbl_mod_NIST_P192(sf.p[0], rsp, sf.t);
			return true;
		}
		if (op_->primeMode == PM_SECP256K1) {
			func = getCurr<void3u>();
			StackFrame sf(this, 3, 10 | UseRDX, 8 * 8);
			mulPre4(rsp, sf.p[1], sf.p[2], sf.t);
			gen_fpDbl_mod_SECP256K1(sf.p[0], rsp, sf.t);
			return true;
		}
		if (pn_ == 3) {
			func = getCurr<void3u>();
			gen_montMul3();
			return true;
		}
		if (pn_ == 4) {
			func = getCurr<void3u>();
			gen_montMul4();
			return true;
		}
		if (pn_ == 6 && !isFullBit_) {
			func = getCurr<void3u>();
#if 1
			// a little faster
			gen_montMul6();
#else
			if (fp_mulPreL.getAddress() == 0 || fpDbl_modL.getAddress() == 0) return 0;
			StackFrame sf(this, 3, 10 | UseRDX, 12 * 8);
			/*
				use xm3
				rsp
				[0, ..12 * 8) ; mul(x, y)
			*/
			vmovq(xm3, gp0);
			mov(gp0, rsp);
			call(fp_mulPreL); // gp0, x, y
			vmovq(gp0, xm3);
			mov(gp1, rsp);
			call(fpDbl_modL);
#endif
			return true;
		}
		return false;
	}
	/*
		@input (z, xy)
		z[1..0] <- montgomery reduction(x[3..0])
		@note destroy rax, rdx, t0, ..., t8
	*/
	void gen_fpDbl_mod2()
	{
		StackFrame sf(this, 2, 9 | UseRDX);
		const Reg64& z = sf.p[0];
		const Reg64& xy = sf.p[1];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];
		const Reg64& t8 = sf.t[8];

		const Reg64& a = rax;
		const Reg64& d = rdx;

		mov(t6, ptr [xy + 8 * 0]);

		mov(a, rp_);
		mul(t6);
		lea(t0, ptr[rip+pL_]);
		mov(t7, a); // q

		// [d:t7:t1] = p * q
		mul2x1(t0, t7, t1);

		xor_(t8, t8);
		if (isFullBit_) {
			xor_(t5, t5);
		}
		mov(t4, d);
		add(t1, t6);
		add_rm(Pack(t8, t4, t7), xy + 8 * 1, true);
		// [t8:t4:t7]
		if (isFullBit_) {
			adc(t5, 0);
		}

		mov(a, rp_);
		mul(t7);
		mov(t6, a); // q

		// [d:t6:xy] = p * q
		mul2x1(t0, t6, xy);

		add_rr(Pack(t8, t4, t7), Pack(d, t6, xy));
		// [t8:t4]
		if (isFullBit_) {
			adc(t5, 0);
		}

		mov_rr(Pack(t2, t1), Pack(t8, t4));
		sub_rm(Pack(t8, t4), t0);
		if (isFullBit_) {
			sbb(t5, 0);
		}
		cmovc_rr(Pack(t8, t4), Pack(t2, t1));
		store_mr(z, Pack(t8, t4));
	}
	/*
		@input (z, xy)
		z[2..0] <- montgomery reduction(x[5..0])
		@note destroy rax, rdx, t0, ..., t10
	*/
	void gen_fpDbl_mod3()
	{
		StackFrame sf(this, 3, 10 | UseRDX);
		const Reg64& z = sf.p[0];
		const Reg64& xy = sf.p[1];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];
		const Reg64& t8 = sf.t[8];
		const Reg64& t9 = sf.t[9];
		const Reg64& t10 = sf.p[2];

		const Reg64& a = rax;
		const Reg64& d = rdx;

		mov(t10, ptr [xy + 8 * 0]);

		mov(a, rp_);
		mul(t10);
		lea(t0, ptr[rip+pL_]);
		mov(t7, a); // q

		// [d:t7:t2:t1] = p * q
		mul3x1(t0, t7, t2, t1, t8);

		xor_(t8, t8);
		xor_(t9, t9);
		if (isFullBit_) {
			xor_(t5, t5);
		}
		mov(t4, d);
		add(t1, t10);
		add_rm(Pack(t9, t8, t4, t7, t2), xy + 8 * 1, true);
		// [t9:t8:t4:t7:t2]
		if (isFullBit_) {
			adc(t5, 0);
		}

		mov(a, rp_);
		mul(t2);
		mov(t10, a); // q

		// [d:t10:t6:xy] = p * q
		mul3x1(t0, t10, t6, xy, t3);

		add_rr(Pack(t8, t4, t7, t2), Pack(d, t10, t6, xy));
		adc(t9, 0); // [t9:t8:t4:t7]
		if (isFullBit_) {
			adc(t5, 0);
		}

		mov(a, rp_);
		mul(t7);
		mov(t10, a); // q

		// [d:t10:xy:t6] = p * q
		mul3x1(t0, t10, xy, t6, t2);

		add_rr(Pack(t9, t8, t4, t7), Pack(d, t10, xy, t6));
		// [t9:t8:t4]
		if (isFullBit_) {
			adc(t5, 0);
		}

		mov_rr(Pack(t2, t1, t10), Pack(t9, t8, t4));
		sub_rm(Pack(t9, t8, t4), t0);
		if (isFullBit_) {
			sbb(t5, 0);
		}
		cmovc_rr(Pack(t9, t8, t4), Pack(t2, t1, t10));
		store_mr(z, Pack(t9, t8, t4));
	}
	/*
		@input (z, xy)
		z[3..0] <- montgomery reduction(x[7..0])
		@note destroy rax, rdx, t0, ..., t10, xm0, xm1
		xm2 if isFullBit_
	*/
	void gen_fpDbl_mod4(const Reg64& z, const Reg64& xy, const Pack& t)
	{
		if (!isFullBit_) {
			gen_fpDbl_modNF(z, xy, t, 4);
			return;
		}
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];
		const Reg64& t7 = t[7];
		const Reg64& t8 = t[8];
		const Reg64& t9 = t[9];
		const Reg64& t10 = t[10];

		const Reg64& a = rax;
		const Reg64& d = rdx;

		vmovq(xm0, z);
		mov(z, ptr [xy + 8 * 0]);

		mov(a, rp_);
		mul(z);
		lea(t0, ptr[rip+pL_]);
		mov(t7, a); // q

		// [d:t7:t3:t2:t1] = p * q
		mul4x1(t0, t7, t3, t2, t1);

		xor_(t8, t8);
		xor_(t9, t9);
		xor_(t10, t10);
		mov(t4, d);
		add(t1, z);
		adc(t2, qword [xy + 8 * 1]);
		adc(t3, qword [xy + 8 * 2]);
		adc(t7, qword [xy + 8 * 3]);
		adc(t4, ptr [xy + 8 * 4]);
		adc(t8, ptr [xy + 8 * 5]);
		adc(t9, ptr [xy + 8 * 6]);
		adc(t10, ptr [xy + 8 * 7]);
		// [t10:t9:t8:t4:t7:t3:t2]
		if (isFullBit_) {
			mov(t5, 0);
			adc(t5, 0);
			vmovq(xm2, t5);
		}

		// free z, t0, t1, t5, t6, xy

		mov(a, rp_);
		mul(t2);
		mov(z, a); // q

		vmovq(xm1, t10);
		// [d:z:t5:t6:xy] = p * q
		mul4x1(t0, z, t5, t6, xy);
		vmovq(t10, xm1);

		add_rr(Pack(t8, t4, t7, t3, t2), Pack(d, z, t5, t6, xy));
		adc(t9, 0);
		adc(t10, 0); // [t10:t9:t8:t4:t7:t3]
		if (isFullBit_) {
			vmovq(t5, xm2);
			adc(t5, 0);
			vmovq(xm2, t5);
		}

		// free z, t0, t1, t2, t5, t6, xy

		mov(a, rp_);
		mul(t3);
		mov(z, a); // q

		// [d:z:t5:xy:t6] = p * q
		mul4x1(t0, z, t5, xy, t6);

		add_rr(Pack(t9, t8, t4, t7, t3), Pack(d, z, t5, xy, t6));
		adc(t10, 0); // c' = [t10:t9:t8:t4:t7]
		if (isFullBit_) {
			vmovq(t3, xm2);
			adc(t3, 0);
		}

		// free z, t1, t2, t7, t5, xy, t6

		mov(a, rp_);
		mul(t7);
		mov(z, a); // q

		// [d:z:t5:xy:t6] = p * q
		mul4x1(t0, z, t5, xy, t6);

		add_rr(Pack(t10, t9, t8, t4, t7), Pack(d, z, t5, xy, t6));
		// [t10:t9:t8:t4]
		if (isFullBit_) {
			adc(t3, 0);
		}

		mov_rr(Pack(t6, t2, t1, z), Pack(t10, t9, t8, t4));
		sub_rm(Pack(t10, t9, t8, t4), t0);
		if (isFullBit_) {
			sbb(t3, 0);
		}
		cmovc(t4, z);
		cmovc(t8, t1);
		cmovc(t9, t2);
		cmovc(t10, t6);

		vmovq(z, xm0);
		store_mr(z, Pack(t10, t9, t8, t4));
	}
	bool gen_fpDbl_mod(void2u& func, const fp::Op& op)
	{
		align(16);
		if (op.primeMode == PM_NIST_P192) {
			func = getCurr<void2u>();
			StackFrame sf(this, 2, 6 | UseRDX);
			fpDbl_mod_NIST_P192(sf.p[0], sf.p[1], sf.t);
			return true;
		}
		if (op.primeMode == PM_SECP256K1) {
			func = getCurr<void2u>();
			StackFrame sf(this, 2, 8 | UseRDX);
			gen_fpDbl_mod_SECP256K1(sf.p[0], sf.p[1], sf.t);
			return true;
		}
#if 0
		if (op.primeMode == PM_NIST_P521) {
			func = getCurr<void2u>();
			StackFrame sf(this, 2, 8 | UseRDX);
			fpDbl_mod_NIST_P521(sf.p[0], sf.p[1], sf.t);
			return true;
		}
#endif
		if (pn_ == 2) {
			func = getCurr<void2u>();
			gen_fpDbl_mod2();
			return true;
		}
		if (pn_ == 3) {
			func = getCurr<void2u>();
			gen_fpDbl_mod3();
			return true;
		}
		if (pn_ == 4) {
			func = getCurr<void2u>();
			StackFrame sf(this, 3, 10 | UseRDX, 0, false);
			call(fpDbl_modL);
			sf.close();
		L(fpDbl_modL);
			Pack t = sf.t;
			t.append(gp2);
			gen_fpDbl_mod4(gp0, gp1, t);
			ret();
			return true;
		}
		if (pn_ == 6 && !isFullBit_) {
			func = getCurr<void2u>();
			StackFrame sf(this, 3, 10 | UseRDX, 0, false);
			call(fpDbl_modL);
			sf.close();
		L(fpDbl_modL);
			Pack t = sf.t;
			t.append(gp2);
			gen_fpDbl_modNF(gp0, gp1, t, 6);
			ret();
			return true;
		}
		return false;
	}
	bool gen_sqr(void2u& func)
	{
		align(16);
		if (op_->primeMode == PM_NIST_P192) {
			func = getCurr<void2u>();
			StackFrame sf(this, 3, 10 | UseRDX, 6 * 8);
			Pack t = sf.t;
			t.append(sf.p[2]);
			sqrPre3(rsp, sf.p[1], t);
			fpDbl_mod_NIST_P192(sf.p[0], rsp, sf.t);
			return true;
		}
		if (op_->primeMode == PM_SECP256K1) {
			func = getCurr<void2u>();
			StackFrame sf(this, 3, 10 | UseRDX, 8 * 8);
			Pack t = sf.t;
			t.append(sf.p[2]);
			sqrPre4(rsp, sf.p[1], t);
			gen_fpDbl_mod_SECP256K1(sf.p[0], rsp, t);
			return true;
		}
		if (pn_ == 3) {
			func = getCurr<void2u>();
			gen_montSqr3();
			return true;
		}
		if (pn_ == 4) {
			func = getCurr<void2u>();
#if 0
			// sqr(y, x) = mul(y, x, x)
#ifdef XBYAK64_WIN
			mov(r8, rdx);
#else
			mov(rdx, rsi);
#endif
			jmp((const void*)op_->fp_mulA_);
#else // (sqrPre + mod) is faster than mul
			StackFrame sf(this, 3, 10 | UseRDX, 8 * 8);
			Pack t = sf.t;
			t.append(sf.p[2]);
			sqrPre4(rsp, sf.p[1], t);
			mov(gp0, sf.p[0]);
			mov(gp1, rsp);
			call(fpDbl_modL);
#endif
			return true;
		}
		if (pn_ == 6 && !isFullBit_) {
			func = getCurr<void2u>();
#if 1
			StackFrame sf(this, 3, 10 | UseRDX);
			Pack t = sf.t;
			t.append(sf.p[2]);
			int stackSize = 12 * 8 + 8;
			sub(rsp, stackSize);
			mov(ptr[rsp], gp0);
			lea(gp0, ptr[rsp + 8]);
			call(fp_sqrPreL);
			mov(gp0, ptr[rsp]);
			lea(gp1, ptr[rsp + 8]);
			call(fpDbl_modL);
			add(rsp, stackSize);
			return true;
#else
			StackFrame sf(this, 3, 10 | UseRDX, 12 * 8);
			Pack t = sf.t;
			t.append(sf.p[2]);
			sqrPre6(rsp, sf.p[1], t);
			lea(gp1, ptr[rsp]);
			call(fpDbl_modL);
			return func;
#endif
		}
		return false;
	}
	/*
		input (z, x, y) = (p0, p1, p2)
		z[0..3] <- montgomery(x[0..3], y[0..3])
		destroy gt0, ..., gt9, xm0, xm1, p2
	*/
	void gen_montMul4()
	{
		StackFrame sf(this, 3, 10 | UseRDX, 0, false);
		call(fp_mulL);
		sf.close();
#if 0 // slower than mont
	L(fp_mulL);
		int stackSize = 8 * 8 /* xy */ + 8;
		sub(rsp, stackSize);
		mov(ptr[rsp], gp0); // save z
		lea(gp0, ptr[rsp + 8]);
		call(fp_mulPreL); // stack <- x * y
		mov(gp0, ptr[rsp]);
		lea(gp1, ptr[rsp + 8]);
		call(fpDbl_modL); // z <- stack
		add(rsp, stackSize);
		ret();
#else
		const Reg64& p0 = sf.p[0];
		const Reg64& p1 = sf.p[1];
		const Reg64& p2 = sf.p[2];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];

	L(fp_mulL);
		vmovq(xm0, p0); // save p0
		lea(p0, ptr[rip+pL_]);
		vmovq(xm1, p2);
		mov(p2, ptr [p2]);
		montgomery4_1(rp_, t0, t7, t3, t2, t1, p1, p2, p0, t4, t5, t6, true, xm2);

		vmovq(p2, xm1);
		mov(p2, ptr [p2 + 8]);
		montgomery4_1(rp_, t1, t0, t7, t3, t2, p1, p2, p0, t4, t5, t6, false, xm2);

		vmovq(p2, xm1);
		mov(p2, ptr [p2 + 16]);
		montgomery4_1(rp_, t2, t1, t0, t7, t3, p1, p2, p0, t4, t5, t6, false, xm2);

		vmovq(p2, xm1);
		mov(p2, ptr [p2 + 24]);
		montgomery4_1(rp_, t3, t2, t1, t0, t7, p1, p2, p0, t4, t5, t6, false, xm2);
		// [t7:t3:t2:t1:t0]

		mov(t4, t0);
		mov(t5, t1);
		mov(t6, t2);
		mov(rdx, t3);
		sub_rm(Pack(t3, t2, t1, t0), p0);
		if (isFullBit_) sbb(t7, 0);
		cmovc(t0, t4);
		cmovc(t1, t5);
		cmovc(t2, t6);
		cmovc(t3, rdx);

		vmovq(p0, xm0); // load p0
		store_mr(p0, Pack(t3, t2, t1, t0));
		ret();
#endif
	}
	/*
		c[n..0] = c[n-1..0] + px[n-1..0] * rdx if is_cn_zero = true
		c[n..0] = c[n..0] + px[n-1..0] * rdx if is_cn_zero = false
		use rax, t0
	*/
	void mulAdd(const Pack& c, int n, const RegExp& px, const Reg64& t0, bool is_cn_zero)
	{
//		assert(!isFullBit_);
		const Reg64& a = rax;
		if (is_cn_zero) {
			xor_(c[n], c[n]);
		} else {
			xor_(a, a);
		}
		for (int i = 0; i < n; i++) {
			mulx(t0, a, ptr [px + i * 8]);
			adox(c[i], a);
			if (i == n - 1) break;
			adcx(c[i + 1], t0);
		}
		adox(c[n], t0);
		adc(c[n], 0);
	}
	/*
		output : CF:c[n..0] = c[n..0] + px[n-1..0] * rdx + (CF << n)
		inout : CF = 0 or 1
		use rax, tt
	*/
	void mulAdd2(const Pack& c, const RegExp& pxy, const RegExp& pp, const Reg64& tt, const Reg64& CF, bool addCF, bool updateCF = true)
	{
		assert(!isFullBit_);
		const Reg64& a = rax;
		xor_(a, a);
		for (int i = 0; i < pn_; i++) {
			mulx(tt, a, ptr [pp + i * 8]);
			adox(c[i], a);
			if (i == 0) mov(c[pn_], ptr[pxy]);
			if (i == pn_ - 1) break;
			adcx(c[i + 1], tt);
		}
		// we can suppose that c[0] = 0
		adox(tt, c[0]); // no carry
		if (addCF) adox(tt, CF); // no carry
		adcx(c[pn_], tt);
		if (updateCF) setc(CF.cvt8());
	}
	/*
		input
		c[5..0]
		rdx = yi
		use rax, rdx
		output
		c[6..1]

		if first:
		  c = x[5..0] * rdx
		else:
		  c += x[5..0] * rdx
		q = uint64_t(c0 * rp)
		c += p * q
		c >>= 64
	*/
	void montgomery6_1(const Pack& c, const RegExp& px, const RegExp& pp, const Reg64& t1, bool isFirst)
	{
		assert(!isFullBit_);
		const int n = 6;
		const Reg64& d = rdx;
		if (isFirst) {
			// c[6..0] = px[5..0] * rdx
			mulPack1(c, n, px);
		} else {
			// c[6..0] = c[5..0] + px[5..0] * rdx because of not fuill bit
			mulAdd(c, 6, px, t1, true);
		}
		mov(d, rp_);
		imul(d, c[0]); // d = q = uint64_t(d * c[0])
		// c[6..0] += p * q because of not fuill bit
		mulAdd(c, 6, pp, t1, false);
	}
	/*
		input (z, x, y) = (p0, p1, p2)
		z[0..5] <- montgomery(x[0..5], y[0..5])
		destroy t0, ..., t9, rax, rdx
	*/
	void gen_montMul6()
	{
		assert(!isFullBit_);
		StackFrame sf(this, 3, 10 | UseRDX, 0, false);
		call(fp_mulL);
		sf.close();
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];
		const Reg64& t8 = sf.t[8];
		const Reg64& pp = sf.t[9];
	L(fp_mulL);
		lea(pp, ptr[rip+pL_]);
		mov(rdx, ptr [py + 0 * 8]);
		montgomery6_1(Pack(t6, t5, t4, t3, t2, t1, t0), px, pp, t8, true);
		mov(rdx, ptr [py + 1 * 8]);
		montgomery6_1(Pack(t0, t6, t5, t4, t3, t2, t1), px, pp, t8, false);
		mov(rdx, ptr [py + 2 * 8]);
		montgomery6_1(Pack(t1, t0, t6, t5, t4, t3, t2), px, pp, t8, false);
		mov(rdx, ptr [py + 3 * 8]);
		montgomery6_1(Pack(t2, t1, t0, t6, t5, t4, t3), px, pp, t8, false);
		mov(rdx, ptr [py + 4 * 8]);
		montgomery6_1(Pack(t3, t2, t1, t0, t6, t5, t4), px, pp, t8, false);
		mov(rdx, ptr [py + 5 * 8]);
		montgomery6_1(Pack(t4, t3, t2, t1, t0, t6, t5), px, pp, t8, false);

		const Pack z = Pack(t4, t3, t2, t1, t0, t6);
		const Pack keep = Pack(rdx, rax, px, py, t7, t8);
		mov_rr(keep, z);
		sub_rm(z, pp);
		cmovc_rr(z, keep);
		store_mr(pz, z);
		ret();
	}
	/*
		input (z, x, y) = (p0, p1, p2)
		z[0..2] <- montgomery(x[0..2], y[0..2])
		destroy gt0, ..., gt9, xm0, xm1, p2
	*/
	void gen_montMul3()
	{
		StackFrame sf(this, 3, 10 | UseRDX);
		const Reg64& p0 = sf.p[0];
		const Reg64& p1 = sf.p[1];
		const Reg64& p2 = sf.p[2];

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];
		const Reg64& t8 = sf.t[8];
		const Reg64& t9 = sf.t[9];

		vmovq(xm0, p0); // save p0
		lea(t7, ptr[rip+pL_]);
		mov(t9, ptr [p2]);
		//                c3, c2, c1, c0, px, y,  p,
		montgomery3_1(rp_, t0, t3, t2, t1, p1, t9, t7, t4, t5, t8, p0, true);
		mov(t9, ptr [p2 + 8]);
		montgomery3_1(rp_, t1, t0, t3, t2, p1, t9, t7, t4, t5, t8, p0, false);

		mov(t9, ptr [p2 + 16]);
		montgomery3_1(rp_, t2, t1, t0, t3, p1, t9, t7, t4, t5, t8, p0, false);

		// [(t3):t2:t1:t0]
		mov(t4, t0);
		mov(t5, t1);
		mov(t6, t2);
		sub_rm(Pack(t2, t1, t0), t7);
		if (isFullBit_) sbb(t3, 0);
		cmovc(t0, t4);
		cmovc(t1, t5);
		cmovc(t2, t6);
		vmovq(p0, xm0);
		store_mr(p0, Pack(t2, t1, t0));
	}
	/*
		input (pz, px)
		z[0..2] <- montgomery(px[0..2], px[0..2])
		destroy gt0, ..., gt9, xm0, xm1, p2
	*/
	void gen_montSqr3()
	{
		StackFrame sf(this, 3, 10 | UseRDX, 16 * 3);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
//		const Reg64& py = sf.p[2]; // not used

		const Reg64& t0 = sf.t[0];
		const Reg64& t1 = sf.t[1];
		const Reg64& t2 = sf.t[2];
		const Reg64& t3 = sf.t[3];
		const Reg64& t4 = sf.t[4];
		const Reg64& t5 = sf.t[5];
		const Reg64& t6 = sf.t[6];
		const Reg64& t7 = sf.t[7];
		const Reg64& t8 = sf.t[8];
		const Reg64& t9 = sf.t[9];

		vmovq(xm0, pz); // save pz
		lea(t7, ptr[rip+pL_]);
		mov(t9, ptr [px]);
		mul3x1_sqr1(px, t9, t3, t2, t1, t0);
		mov(t0, rdx);
		montgomery3_sub(rp_, t0, t9, t2, t1, px, t3, t7, t4, t5, t8, pz, true);

		mov(t3, ptr [px + 8]);
		mul3x1_sqr2(px, t3, t6, t5, t4);
		add_rr(Pack(t1, t0, t9, t2), Pack(rdx, rax, t5, t4));
		if (isFullBit_) setc(pz.cvt8());
		montgomery3_sub(rp_, t1, t3, t9, t2, px, t0, t7, t4, t5, t8, pz, false);

		mov(t0, ptr [px + 16]);
		mul3x1_sqr3(t0, t5, t4);
		add_rr(Pack(t2, t1, t3, t9), Pack(rdx, rax, t5, t4));
		if (isFullBit_) setc(pz.cvt8());
		montgomery3_sub(rp_, t2, t0, t3, t9, px, t1, t7, t4, t5, t8, pz, false);

		// [t9:t2:t0:t3]
		mov(t4, t3);
		mov(t5, t0);
		mov(t6, t2);
		sub_rm(Pack(t2, t0, t3), t7);
		if (isFullBit_) sbb(t9, 0);
		cmovc(t3, t4);
		cmovc(t0, t5);
		cmovc(t2, t6);
		vmovq(pz, xm0);
		store_mr(pz, Pack(t2, t0, t3));
	}
	/*
		py[5..0] <- px[2..0]^2
		@note use rax, rdx
	*/
	void sqrPre3(const RegExp& py, const RegExp& px, const Pack& t)
	{
		const Reg64& a = rax;
		const Reg64& d = rdx;
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];
		const Reg64& t7 = t[7];
		const Reg64& t8 = t[8];
		const Reg64& t9 = t[9];
		const Reg64& t10 = t[10];

		mov(d, ptr [px + 8 * 0]);
		mulx(t0, a, d);
		mov(ptr [py + 8 * 0], a);

		mov(t7, ptr [px + 8 * 1]);
		mov(t9, ptr [px + 8 * 2]);
		mulx(t2, t1, t7);
		mulx(t4, t3, t9);

		mov(t5, t2);
		mov(t6, t4);

		add(t0, t1);
		adc(t5, t3);
		adc(t6, 0); // [t6:t5:t0]

		mov(d, t7);
		mulx(t8, t7, d);
		mulx(t10, t9, t9);
		add(t2, t7);
		adc(t8, t9);
		mov(t7, t10);
		adc(t7, 0); // [t7:t8:t2:t1]

		add(t0, t1);
		adc(t2, t5);
		adc(t6, t8);
		adc(t7, 0);
		mov(ptr [py + 8 * 1], t0); // [t7:t6:t2]

		mov(a, ptr [px + 8 * 2]);
		mul(a);
		add(t4, t9);
		adc(a, t10);
		adc(d, 0); // [d:a:t4:t3]

		add(t2, t3);
		adc(t6, t4);
		adc(t7, a);
		adc(d, 0);
		store_mr(py + 8 * 2, Pack(d, t7, t6, t2));
	}
	/*
		c[n..0] = px[n-1..0] * rdx
		use rax
	*/
	void mulPack1(const Pack& c, int n, const RegExp& px)
	{
		mulx(c[1], c[0], ptr [px + 0 * 8]);
		for (int i = 1; i < n; i++) {
			mulx(c[i + 1], rax, ptr[px + i * 8]);
			if (i == 1) {
				add(c[i], rax);
			} else {
				adc(c[i], rax);
			}
		}
		adc(c[n], 0);
	}
	/*
		[pd:pz[0]] <- py[n-1..0] * px[0]
	*/
	void mulPack(const RegExp& pz, const RegExp& px, const RegExp& py, const Pack& pd)
	{
		const Reg64& a = rax;
		const Reg64& d = rdx;
		mov(d, ptr [px]);
		mulx(pd[0], a, ptr [py + 8 * 0]);
		mov(ptr [pz + 8 * 0], a);
		xor_(a, a);
		for (size_t i = 1; i < pd.size(); i++) {
			mulx(pd[i], a, ptr [py + 8 * i]);
			adcx(pd[i - 1], a);
		}
		adc(pd[pd.size() - 1], 0);
	}
	/*
		[hi:Pack(d_(n-1), .., d1):pz[0]] <- Pack(d_(n-1), ..., d0) + py[n-1..0] * px[0]
	*/
	void mulPackAdd(const RegExp& pz, const RegExp& px, const RegExp& py, const Reg64& hi, const Pack& pd)
	{
		const Reg64& a = rax;
		const Reg64& d = rdx;
		mov(d, ptr [px]);
		xor_(a, a);
		for (size_t i = 0; i < pd.size(); i++) {
			mulx(hi, a, ptr [py + i * 8]);
			adox(pd[i], a);
			if (i == 0) mov(ptr[pz], pd[0]);
			if (i == pd.size() - 1) break;
			adcx(pd[i + 1], hi);
		}
		mov(a, 0);
		adox(hi, a);
		adc(hi, a);
	}
	/*
		input : z[n], p[n-1], rdx(implicit)
		output: z[] += p[] * rdx, rax = 0 and set CF
		use rax, rdx
	*/
	void mulPackAddShr(const Pack& z, const RegExp& p, const Reg64& H, bool last = false)
	{
		const Reg64& a = rax;
		const size_t n = z.size();
		assert(n >= 3);
		// clear CF and OF
		xor_(a, a);
		const size_t loop = last ? n - 1 : n - 3;
		for (size_t i = 0; i < loop; i++) {
			// mulx(H, L, x) = [H:L] = x * rdx
			mulx(H, a, ptr [p + i * 8]);
			adox(z[i], a);
			adcx(z[i + 1], H);
		}
		if (last) {
			mov(a, 0);
			adox(z[n - 1], a);
			return;
		}
		/*
			reorder addtion not to propage OF outside this routine
			         H
		             +
			 rdx     a
			  |      |
			  v      v
			z[n-1] z[n-2]
		*/
		mulx(H, a, ptr [p + (n - 3) * 8]);
		adox(z[n - 3], a);
		mulx(rdx, a, ptr [p + (n - 2) * 8]); // destroy rdx
		adox(H, a);
		mov(a, 0);
		adox(rdx, a);
		adcx(z[n - 2], H);
		adcx(z[n - 1], rdx);
	}
	/*
		pz[5..0] <- px[2..0] * py[2..0]
	*/
	void mulPre3(const RegExp& pz, const RegExp& px, const RegExp& py, const Pack& t)
	{
		const Reg64& d = rdx;
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];
		const Reg64& t8 = t[8];
		const Reg64& t9 = t[9];

		mulPack(pz, px, py, Pack(t2, t1, t0));
#if 0 // a little slow
		if (useAdx_) {
			// [t2:t1:t0]
			mulPackAdd(pz + 8 * 1, px + 8 * 1, py, t3, Pack(t2, t1, t0));
			// [t3:t2:t1]
			mulPackAdd(pz + 8 * 2, px + 8 * 2, py, t4, Pack(t3, t2, t1));
			// [t4:t3:t2]
			store_mr(pz + 8 * 3, Pack(t4, t3, t2));
			return;
		}
#endif
		// here [t2:t1:t0]

		mov(t9, ptr [px + 8]);

		// [d:t9:t6:t5] = px[1] * py[2..0]
		mul3x1(py, t9, t6, t5, t4);
		add_rr(Pack(t2, t1, t0), Pack(t9, t6, t5));
		adc(d, 0);
		mov(t8, d);
		mov(ptr [pz + 8], t0);
		// here [t8:t2:t1]

		mov(t9, ptr [px + 16]);

		// [d:t9:t5:t4]
		mul3x1(py, t9, t5, t4, t0);
		add_rr(Pack(t8, t2, t1), Pack(t9, t5, t4));
		adc(d, 0);
		store_mr(pz + 8 * 2, Pack(d, t8, t2, t1));
	}
	void sqrPre2(const Reg64& py, const Reg64& px, const Pack& t)
	{
		// QQQ
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];
		load_rm(Pack(px, t0), px); // x = [px:t0]
		sqr2(t4, t3, t2, t1, px, t0, t5, t6);
		store_mr(py, Pack(t4, t3, t2, t1));
	}
	/*
		[y3:y2:y1:y0] = [x1:x0] ^ 2
		use rdx
	*/
	void sqr2(const Reg64& y3, const Reg64& y2, const Reg64& y1, const Reg64& y0, const Reg64& x1, const Reg64& x0, const Reg64& t1, const Reg64& t0)
	{
		mov(rdx, x0);
		mulx(y1, y0, x0); // x0^2
		mov(rdx, x1);
		mulx(y3, y2, x1); // x1^2
		mulx(t1, t0, x0); // x0 x1
		add(y1, t0);
		adc(y2, t1);
		adc(y3, 0);
		add(y1, t0);
		adc(y2, t1);
		adc(y3, 0);
	}
	/*
		[t3:t2:t1:t0] = px[1, 0] * py[1, 0]
		use rax, rdx
	*/
	void mul2x2(const RegExp& px, const RegExp& py, const Reg64& t4, const Reg64& t3, const Reg64& t2, const Reg64& t1, const Reg64& t0)
	{
#if 0
		// # of add is less, but a little slower
		mov(t4, ptr [py + 8 * 0]);
		mov(rdx, ptr [px + 8 * 1]);
		mulx(t2, t1, t4);
		mov(rdx, ptr [px + 8 * 0]);
		mulx(t0, rax, ptr [py + 8 * 1]);
		xor_(t3, t3);
		add_rr(Pack(t3, t2, t1), Pack(t3, t0, rax));
		// [t3:t2:t1] = ad + bc
		mulx(t4, t0, t4);
		mov(rax, ptr [px + 8 * 1]);
		mul(qword [py + 8 * 1]);
		add_rr(Pack(t3, t2, t1), Pack(rdx, rax, t4));
#else
		mov(rdx, ptr [py + 8 * 0]);
		mov(rax, ptr [px + 8 * 0]);
		mulx(t1, t0, rax);
		mov(t3, ptr [px + 8 * 1]);
		mulx(t2, rdx, t3);
		add(t1, rdx);
		adc(t2, 0); // [t2:t1:t0]

		mov(rdx, ptr [py + 8 * 1]);
		mulx(rax, t4, rax);
		mulx(t3, rdx, t3);
		add(rax, rdx);
		adc(t3, 0); // [t3:rax:t4]
		add(t1, t4);
		adc(t2, rax);
		adc(t3, 0); // t3:t2:t1:t0]
#endif
	}
	void mulPre2(const RegExp& pz, const RegExp& px, const RegExp& py, const Pack& t)
	{
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		mul2x2(px, py, t4, t3, t2, t1, t0);
		store_mr(pz, Pack(t3, t2, t1, t0));
	}
	/*
		(3, 3)(2, 2)(1, 1)(0, 0)
		   t5 t4 t3 t2 t1 t0
		   (3, 2)(2, 1)(1, 0)x2
		      (3, 1)(2, 0)x2
		         (3, 0)x2
	*/
	void sqrPre4NF(const Reg64& py, const Reg64& px, const Pack& t)
	{
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& x0 = t[6];
		const Reg64& x1 = t[7];
		const Reg64& x2 = t[8];
		const Reg64& x3 = t[9];
		const Reg64& H = t[10];

		load_rm(Pack(x3, x2, x1, x0), px);
		mov(rdx, x0);
		mulx(t3, t2, x3); // (3, 0)
		mulx(rax, t1, x2); // (2, 0)
		add(t2, rax);
		mov(rdx, x1);
		mulx(t4, rax, x3); // (3, 1)
		adc(t3, rax);
		adc(t4, 0); // [t4:t3:t2:t1]
		mulx(rax, t0, x0); // (1, 0)
		add(t1, rax);
		mulx(rdx, rax, x2); // (2, 1)
		adc(t2, rax);
		adc(t3, rdx);
		mov(rdx, x3);
		mulx(t5, rax, x2); // (3, 2)
		adc(t4, rax);
		adc(t5, 0);

		shl1(Pack(t5, t4, t3, t2, t1, t0), &H);
		mov(rdx, x0);
		mulx(rdx, rax, rdx);
		mov(ptr[py + 8 * 0], rax);
		add(rdx, t0);
		mov(ptr[py + 8 * 1], rdx);
		mov(rdx, x1);
		mulx(rdx, rax, rdx);
		adc(rax, t1);
		mov(ptr[py + 8 * 2], rax);
		adc(rdx, t2);
		mov(ptr[py + 8 * 3], rdx);
		mov(rdx, x2);
		mulx(rdx, rax, rdx);
		adc(rax, t3);
		mov(ptr[py + 8 * 4], rax);
		adc(rdx, t4);
		mov(ptr[py + 8 * 5], rdx);
		mov(rdx, x3);
		mulx(rdx, rax, rdx);
		adc(rax, t5);
		mov(ptr[py + 8 * 6], rax);
		adc(rdx, H);
		mov(ptr[py + 8 * 7], rdx);
	}
	/*
		py[7..0] = px[3..0] ^ 2
		use xmm0
	*/
	void sqrPre4(const Reg64& py, const Reg64& px, const Pack& t)
	{
		sqrPre4NF(py, px, t);
	}
	/*
		(5, 5)(4, 4)(3, 3)(2, 2)(1, 1)(0, 0)
		   t9 t8 t7 t6 t5 t4 t3 t2 t1 t0
		   (5, 4)(4, 3)(3, 2)(2, 1)(1, 0)
		      (5, 3)(4, 2)(3, 1)(2, 0)
		         (5, 2)(4, 1)(3, 0)
		            (5, 1)(4, 0)
		               (5, 0)
	*/
	void sqrPre6(const RegExp& py, const RegExp& px, const Pack& t)
	{
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];
		const Reg64& t7 = t[7];
		const Reg64& t8 = t[8];
		const Reg64& t9 = t[9];
		const Reg64& H = t[10];

		mov(rdx, ptr[px + 8 * 0]);
		mulx(t5, t4, ptr[px + 8 * 5]); // [t5:t4] = (5, 0)
		mulx(rax, t3, ptr[px + 8 * 4]); // (4, 0)
		add(t4, rax);
		mov(rdx, ptr[px + 8 * 1]);
		mulx(t6, rax, ptr[px + 8 * 5]); // (5, 1)
		adc(t5, rax);
		adc(t6, 0); // [t6:t5:t4:t3]
		mov(rdx, ptr[px + 8 * 0]);
		mulx(rax, t2, ptr[px + 8 * 3]);
		add(t3, rax);
		mov(rdx, ptr[px + 8 * 1]);
		mulx(H, rax, ptr[px + 8 * 4]);
		adc(t4, rax);
		adc(t5, H);
		mov(rdx, ptr[px + 8 * 2]);
		mulx(t7, rax, ptr[px + 8 * 5]);
		adc(t6, rax);
		adc(t7, 0); // [t7:...:t2]

		mov(rdx, ptr[px + 8 * 0]);
		mulx(H, t1, ptr[px + 8 * 2]);
		adc(t2, H);
		mov(rdx, ptr[px + 8 * 1]);
		mulx(H, rax, ptr[px + 8 * 3]);
		adc(t3, rax);
		adc(t4, H);
		mov(rdx, ptr[px + 8 * 2]);
		mulx(H, rax, ptr[px + 8 * 4]);
		adc(t5, rax);
		adc(t6, H);
		mov(rdx, ptr[px + 8 * 3]);
		mulx(t8, rax, ptr[px + 8 * 5]);
		adc(t7, rax);
		adc(t8, 0); // [t8:...:t1]
		mov(rdx, ptr[px + 8 * 0]);
		mulx(H, t0, ptr[px + 8 * 1]);
		add(t1, H);
		mov(rdx, ptr[px + 8 * 1]);
		mulx(H, rax, ptr[px + 8 * 2]);
		adc(t2, rax);
		adc(t3, H);
		mov(rdx, ptr[px + 8 * 2]);
		mulx(H, rax, ptr[px + 8 * 3]);
		adc(t4, rax);
		adc(t5, H);
		mov(rdx, ptr[px + 8 * 3]);
		mulx(H, rax, ptr[px + 8 * 4]);
		adc(t6, rax);
		adc(t7, H);
		mov(rdx, ptr[px + 8 * 4]);
		mulx(t9, rax, ptr[px + 8 * 5]);
		adc(t8, rax);
		adc(t9, 0); // [t9...:t0]
		shl1(Pack(t9, t8, t7, t6, t5, t4, t3, t2, t1, t0), &H);

		mov(rdx, ptr[px + 8 * 0]);
		mulx(rdx, rax, rdx);
		mov(ptr[py + 8 * 0], rax);
		add(t0, rdx);
		mov(ptr[py + 8 * 1], t0);

		mov(rdx, ptr[px + 8 * 1]);
		mulx(rdx, rax, rdx);
		adc(t1, rax);
		mov(ptr[py + 8 * 2], t1);
		adc(t2, rdx);
		mov(ptr[py + 8 * 3], t2);

		mov(rdx, ptr[px + 8 * 2]);
		mulx(rdx, rax, rdx);
		adc(t3, rax);
		mov(ptr[py + 8 * 4], t3);
		adc(t4, edx);
		mov(ptr[py + 8 * 5], t4);

		mov(rdx, ptr[px + 8 * 3]);
		mulx(rdx, rax, rdx);
		adc(t5, rax);
		mov(ptr[py + 8 * 6], t5);
		adc(t6, rdx);
		mov(ptr[py + 8 * 7], t6);

		mov(rdx, ptr[px + 8 * 4]);
		mulx(rdx, rax, rdx);
		adc(t7, rax);
		mov(ptr[py + 8 * 8], t7);
		adc(t8, rdx);
		mov(ptr[py + 8 * 9], t8);

		mov(rdx, ptr[px + 8 * 5]);
		mulx(rdx, rax, rdx);
		adc(t9, rax);
		mov(ptr[py + 8 * 10], t9);
		adc(rdx, H);
		mov(ptr[py + 8 * 11], rdx);
	}
	/*
		pz[7..0] <- px[3..0] * py[3..0]
	*/
	void mulPre4(const RegExp& pz, const RegExp& px, const RegExp& py, const Pack& t)
	{
		const Reg64& d = rdx;
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];
		const Reg64& t7 = t[7];
		const Reg64& t8 = t[8];
		const Reg64& t9 = t[9];

#if 0 // a little slower
		mulPack(pz, px, py, Pack(t3, t2, t1, t0));
		mulPackAdd(pz + 8 * 1, px + 8 * 1, py, t4, Pack(t3, t2, t1, t0));
		mulPackAdd(pz + 8 * 2, px + 8 * 2, py, t0, Pack(t4, t3, t2, t1));
		mulPackAdd(pz + 8 * 3, px + 8 * 3, py, t1, Pack(t0, t4, t3, t2));
		store_mr(pz + 8 * 4, Pack(t1, t0, t4, t3));
		return;
#endif
#if 0
		// a little slower
		if (!useMulx_) {
			throw cybozu::Exception("mulPre4:not support mulx");
		}
		mul2x2(px + 8 * 0, py + 8 * 2, t4, t3, t2, t1, t0);
		mul2x2(px + 8 * 2, py + 8 * 0, t9, t8, t7, t6, t5);
		xor_(t4, t4);
		add_rr(Pack(t4, t3, t2, t1, t0), Pack(t4, t8, t7, t6, t5));
		// [t4:t3:t2:t1:t0]
		mul2x2(px + 8 * 0, py + 8 * 0, t9, t8, t7, t6, t5);
		store_mr(pz, Pack(t6, t5));
		// [t8:t7]
		vmovq(xm0, t7);
		vmovq(xm1, t8);
		mul2x2(px + 8 * 2, py + 8 * 2, t8, t7, t9, t6, t5);
		vmovq(a, xm0);
		vmovq(d, xm1);
		add_rr(Pack(t4, t3, t2, t1, t0), Pack(t9, t6, t5, d, a));
		adc(t7, 0);
		store_mr(pz + 8 * 2, Pack(t7, t4, t3, t2, t1, t0));
#else
		mulPack(pz, px, py, Pack(t3, t2, t1, t0));

		// here [t3:t2:t1:t0]

		mov(t9, ptr [px + 8]);

		// [d:t9:t7:t6:t5] = px[1] * py[3..0]
		mul4x1(py, t9, t7, t6, t5);
		add_rr(Pack(t3, t2, t1, t0), Pack(t9, t7, t6, t5));
		adc(d, 0);
		mov(t8, d);
		mov(ptr [pz + 8], t0);
		// here [t8:t3:t2:t1]

		mov(t9, ptr [px + 16]);

		// [d:t9:t6:t5:t4]
		mul4x1(py, t9, t6, t5, t4);
		add_rr(Pack(t8, t3, t2, t1), Pack(t9, t6, t5, t4));
		adc(d, 0);
		mov(t7, d);
		mov(ptr [pz + 16], t1);

		mov(t9, ptr [px + 24]);

		// [d:t9:t5:t4:t1]
		mul4x1(py, t9, t5, t4, t1);
		add_rr(Pack(t7, t8, t3, t2), Pack(t9, t5, t4, t1));
		adc(d, 0);
		store_mr(pz + 8 * 3, Pack(t7, t8, t3, t2));
		mov(ptr [pz + 8 * 7], d);
#endif
	}
	// [gp0] <- [gp1] * [gp2]
	void mulPre5(const Pack& t)
	{
		const Reg64& pz = gp0;
		const Reg64& px = gp1;
		const Reg64& py = gp2;
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];

		mulPack(pz, px, py, Pack(t4, t3, t2, t1, t0)); // [t4:t3:t2:t1:t0]
		mulPackAdd(pz + 8 * 1, px + 8 * 1, py, t5, Pack(t4, t3, t2, t1, t0)); // [t5:t4:t3:t2:t1]
		mulPackAdd(pz + 8 * 2, px + 8 * 2, py, t0, Pack(t5, t4, t3, t2, t1)); // [t0:t5:t4:t3:t2]
		mulPackAdd(pz + 8 * 3, px + 8 * 3, py, t1, Pack(t0, t5, t4, t3, t2)); // [t1:t0:t5:t4:t3]
		mulPackAdd(pz + 8 * 4, px + 8 * 4, py, t2, Pack(t1, t0, t5, t4, t3)); // [t2:t1:t0:t5:t4]
		store_mr(pz + 8 * 5, Pack(t2, t1, t0, t5, t4));
	}
	// [gp0] <- [gp1] * [gp2]
	void mulPre6(const Pack& t)
	{
		const Reg64& pz = gp0;
		const Reg64& px = gp1;
		const Reg64& py = gp2;
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
#if 0 // slower than basic multiplication(56clk -> 67clk)
//		const Reg64& t7 = t[7];
//		const Reg64& t8 = t[8];
//		const Reg64& t9 = t[9];
		const Reg64& a = rax;
		const Reg64& d = rdx;
		const int stackSize = (3 + 3 + 6 + 1 + 1 + 1) * 8; // a+b, c+d, (a+b)(c+d), x, y, z
		const int abPos = 0;
		const int cdPos = abPos + 3 * 8;
		const int abcdPos = cdPos + 3 * 8;
		const int zPos = abcdPos + 6 * 8;
		const int yPos = zPos + 8;
		const int xPos = yPos + 8;

		sub(rsp, stackSize);
		mov(ptr[rsp + zPos], pz);
		mov(ptr[rsp + xPos], px);
		mov(ptr[rsp + yPos], py);
		/*
			x = aN + b, y = cN + d
			xy = abN^2 + ((a+b)(c+d) - ac - bd)N + bd
		*/
		xor_(a, a);
		load_rm(Pack(t2, t1, t0), px); // b
		add_rm(Pack(t2, t1, t0), px + 3 * 8); // a + b
		adc(a, 0);
		store_mr(pz, Pack(t2, t1, t0));
		vmovq(xm0, a); // carry1

		xor_(a, a);
		load_rm(Pack(t2, t1, t0), py); // d
		add_rm(Pack(t2, t1, t0), py + 3 * 8); // c + d
		adc(a, 0);
		store_mr(pz + 3 * 8, Pack(t2, t1, t0));
		vmovq(xm1, a); // carry2

		mulPre3(rsp + abcdPos, pz, pz + 3 * 8, t); // (a+b)(c+d)

		vmovq(a, xm0);
		vmovq(d, xm1);
		mov(t3, a);
		and_(t3, d); // t3 = carry1 & carry2
		Label doNothing;
		je(doNothing);
		load_rm(Pack(t2, t1, t0), rsp + abcdPos + 3 * 8);
		test(a, a);
		je("@f");
		// add (c+d)
		add_rm(Pack(t2, t1, t0), pz + 3 * 8);
		adc(t3, 0);
	L("@@");
		test(d, d);
		je("@f");
		// add(a+b)
		add_rm(Pack(t2, t1, t0), pz);
		adc(t3, 0);
	L("@@");
		store_mr(rsp + abcdPos + 3 * 8, Pack(t2, t1, t0));
	L(doNothing);
		vmovq(xm0, t3); // save new carry


		mov(gp0, ptr [rsp + zPos]);
		mov(gp1, ptr [rsp + xPos]);
		mov(gp2, ptr [rsp + yPos]);
		mulPre3(gp0, gp1, gp2, t); // [rsp] <- bd

		mov(gp0, ptr [rsp + zPos]);
		mov(gp1, ptr [rsp + xPos]);
		mov(gp2, ptr [rsp + yPos]);
		mulPre3(gp0 + 6 * 8, gp1 + 3 * 8, gp2 + 3 * 8, t); // [rsp + 6 * 8] <- ac

		mov(pz, ptr[rsp + zPos]);
		vmovq(d, xm0);
		for (int i = 0; i < 6; i++) {
			mov(a, ptr[pz + (3 + i) * 8]);
			if (i == 0) {
				add(a, ptr[rsp + abcdPos + i * 8]);
			} else {
				adc(a, ptr[rsp + abcdPos + i * 8]);
			}
			mov(ptr[pz + (3 + i) * 8], a);
		}
		mov(a, ptr[pz + 9 * 8]);
		adc(a, d);
		mov(ptr[pz + 9 * 8], a);
		jnc("@f");
		for (int i = 10; i < 12; i++) {
			mov(a, ptr[pz + i * 8]);
			adc(a, 0);
			mov(ptr[pz + i * 8], a);
		}
	L("@@");
		add(rsp, stackSize);
#else
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];

		mulPack(pz, px, py, Pack(t5, t4, t3, t2, t1, t0)); // [t5:t4:t3:t2:t1:t0]
		mulPackAdd(pz + 8 * 1, px + 8 * 1, py, t6, Pack(t5, t4, t3, t2, t1, t0)); // [t6:t5:t4:t3:t2:t1]
		mulPackAdd(pz + 8 * 2, px + 8 * 2, py, t0, Pack(t6, t5, t4, t3, t2, t1)); // [t0:t6:t5:t4:t3:t2]
		mulPackAdd(pz + 8 * 3, px + 8 * 3, py, t1, Pack(t0, t6, t5, t4, t3, t2)); // [t1:t0:t6:t5:t4:t3]
		mulPackAdd(pz + 8 * 4, px + 8 * 4, py, t2, Pack(t1, t0, t6, t5, t4, t3)); // [t2:t1:t0:t6:t5:t4]
		mulPackAdd(pz + 8 * 5, px + 8 * 5, py, t3, Pack(t2, t1, t0, t6, t5, t4)); // [t3:t2:t1:t0:t6:t5]
		store_mr(pz + 8 * 6, Pack(t3, t2, t1, t0, t6, t5));
#endif
	}
	Pack rotatePack(const Pack& p) const
	{
		Pack q = p.sub(1);
		q.append(p[0]);
		return q;
	}
	/*
		@input (z, xy)
		z[n-1..0] <- montgomery reduction(x[2n-1..0])
	*/
	void gen_fpDbl_modNF(const Reg64& z, const Reg64& xy, const Pack& t, int n)
	{
		assert(!isFullBit_);
		assert(n + 3 <= 11);

		const Reg64& d = rdx;
		Pack pk = t.sub(0, n + 1);
		Reg64 CF = t[n + 1];
		const Reg64& tt = t[n + 2];
		const Reg64& pp = t[n + 3];

		lea(pp, ptr[rip + pL_]);

		xor_(CF, CF);
		load_rm(pk.sub(0, n), xy);
		mov(d, rp_);
		imul(d, pk[0]); // q
		mulAdd2(pk, xy + n * 8, pp, tt, CF, false);

		for (int i = 1; i < n; i++) {
			pk.append(pk[0]);
			pk = pk.sub(1);
			mov(d, rp_);
			imul(d, pk[0]);
			mulAdd2(pk, xy + (n + i) * 8, pp, tt, CF, true, i < n - 1);
		}

		Reg64 pk0 = pk[0];
		Pack zp = pk.sub(1);
		Pack keep = Pack(xy, rax, rdx, tt, CF, pk0).sub(0, n);
		mov_rr(keep, zp);
		sub_rm(zp, pp); // z -= p
		cmovc_rr(zp, keep);
		store_mr(z, zp);
	}
	bool gen_fpDbl_sqrPre(void2u& func)
	{
		align(16);
		void2u f = getCurr<void2u>();
		switch (pn_) {
		case 2:
			{
				StackFrame sf(this, 2, 7 | UseRDX);
				sqrPre2(sf.p[0], sf.p[1], sf.t);
			}
			break;
		case 3:
			{
				StackFrame sf(this, 3, 10 | UseRDX);
				Pack t = sf.t;
				t.append(sf.p[2]);
				sqrPre3(sf.p[0], sf.p[1], t);
			}
			break;
		case 4:
			{
				StackFrame sf(this, 3, 10 | UseRDX);
				Pack t = sf.t;
				t.append(sf.p[2]);
				sqrPre4(sf.p[0], sf.p[1], t);
			}
			break;
		case 6:
			{
				StackFrame sf(this, 3, 10 | UseRDX);
				call(fp_sqrPreL);
				sf.close();
			L(fp_sqrPreL);
				Pack t = sf.t;
				t.append(sf.p[2]);
				sqrPre6(sf.p[0], sf.p[1], t);
				ret();
			}
			break;
		default:
			return false;
		}
		func = f;
		return true;
	}
	bool gen_fpDbl_mulPre(void3u& func)
	{
		align(16);
		void3u f = getCurr<void3u>();
		switch (pn_) {
		case 2:
			{
				StackFrame sf(this, 3, 5 | UseRDX);
				mulPre2(sf.p[0], sf.p[1], sf.p[2], sf.t);
			}
			break;
		case 3:
			{
				StackFrame sf(this, 3, 10 | UseRDX);
				mulPre3(sf.p[0], sf.p[1], sf.p[2], sf.t);
			}
			break;
		case 4:
			{
				/*
					fpDbl_mulPre is available as C function
					this function calls fp_mulPreL directly.
				*/
				StackFrame sf(this, 3, 10 | UseRDX, 0, false);
				mulPre4(gp0, gp1, gp2, sf.t);
	//			call(fp_mulPreL);
				sf.close(); // make epilog
			L(fp_mulPreL); // called only from asm code
				mulPre4(gp0, gp1, gp2, sf.t);
				ret();
			}
			break;
		case 5:
			{
				StackFrame sf(this, 3, 10 | UseRDX, 0, false);
				call(fp_mulPreL);
				sf.close(); // make epilog
			L(fp_mulPreL); // called only from asm code
				mulPre5(sf.t);
				ret();
			}
			break;
		case 6:
			{
				StackFrame sf(this, 3, 10 | UseRDX, 0, false);
				call(fp_mulPreL);
				sf.close(); // make epilog
			L(fp_mulPreL); // called only from asm code
				mulPre6(sf.t);
				ret();
			}
			break;
		default:
			return false;
		}
		func = f;
		return true;
	}
	static inline void debug_put_inner(const uint64_t *ptr, int n)
	{
		printf("debug ");
		for (int i = 0; i < n; i++) {
			printf("%016llx", (long long)ptr[n - 1 - i]);
		}
		printf("\n");
	}
#ifdef _MSC_VER
	void debug_put(const RegExp& m, int n)
	{
		assert(n <= 8);
		static uint64_t regBuf[7];

		push(rax);
		mov(rax, (size_t)regBuf);
		mov(ptr [rax + 8 * 0], rcx);
		mov(ptr [rax + 8 * 1], rdx);
		mov(ptr [rax + 8 * 2], r8);
		mov(ptr [rax + 8 * 3], r9);
		mov(ptr [rax + 8 * 4], r10);
		mov(ptr [rax + 8 * 5], r11);
		mov(rcx, ptr [rsp + 8]); // org rax
		mov(ptr [rax + 8 * 6], rcx); // save
		mov(rcx, ptr [rax + 8 * 0]); // org rcx
		pop(rax);

		lea(rcx, ptr [m]);
		mov(rdx, n);
		mov(rax, (size_t)debug_put_inner);
		sub(rsp, 32);
		call(rax);
		add(rsp, 32);

		push(rax);
		mov(rax, (size_t)regBuf);
		mov(rcx, ptr [rax + 8 * 0]);
		mov(rdx, ptr [rax + 8 * 1]);
		mov(r8, ptr [rax + 8 * 2]);
		mov(r9, ptr [rax + 8 * 3]);
		mov(r10, ptr [rax + 8 * 4]);
		mov(r11, ptr [rax + 8 * 5]);
		mov(rax, ptr [rax + 8 * 6]);
		add(rsp, 8);
	}
#endif
	/*
		z >>= c
		@note shrd(r/m, r, imm)
	*/
	void shr_mp(const MixPack& z, uint8_t c, const Reg64& t)
	{
		const size_t n = z.size();
		for (size_t i = 0; i < n - 1; i++) {
			const Reg64 *p;
			if (z.isReg(i + 1)) {
				p = &z.getReg(i + 1);
			} else {
				mov(t, ptr [z.getMem(i + 1)]);
				p = &t;
			}
			if (z.isReg(i)) {
				shrd(z.getReg(i), *p, c);
			} else {
				shrd(qword [z.getMem(i)], *p, c);
			}
		}
		if (z.isReg(n - 1)) {
			shr(z.getReg(n - 1), c);
		} else {
			shr(qword [z.getMem(n - 1)], c);
		}
	}
	/*
		z *= 2
	*/
	void twice_mp(const MixPack& z, const Reg64& t)
	{
		g_add(z[0], z[0], t);
		for (size_t i = 1, n = z.size(); i < n; i++) {
			g_adc(z[i], z[i], t);
		}
	}
	/*
		z += x
	*/
	void add_mp(const MixPack& z, const MixPack& x, const Reg64& t)
	{
		assert(z.size() == x.size());
		g_add(z[0], x[0], t);
		for (size_t i = 1, n = z.size(); i < n; i++) {
			g_adc(z[i], x[i], t);
		}
	}
	void add_mm(const RegExp& mz, const RegExp& mx, const Reg64& t, int n)
	{
		for (int i = 0; i < n; i++) {
			mov(t, ptr [mx + i * 8]);
			if (i == 0) {
				add(ptr [mz + i * 8], t);
			} else {
				adc(ptr [mz + i * 8], t);
			}
		}
	}
	/*
		mz[] = mx[] - y
	*/
	void sub_m_mp_m(const RegExp& mz, const RegExp& mx, const MixPack& y, const Reg64& t)
	{
		for (size_t i = 0; i < y.size(); i++) {
			mov(t, ptr [mx + i * 8]);
			if (i == 0) {
				if (y.isReg(i)) {
					sub(t, y.getReg(i));
				} else {
					sub(t, ptr [y.getMem(i)]);
				}
			} else {
				if (y.isReg(i)) {
					sbb(t, y.getReg(i));
				} else {
					sbb(t, ptr [y.getMem(i)]);
				}
			}
			mov(ptr [mz + i * 8], t);
		}
	}
	/*
		z -= x
	*/
	void sub_mp(const MixPack& z, const MixPack& x, const Reg64& t)
	{
		assert(z.size() == x.size());
		g_sub(z[0], x[0], t);
		for (size_t i = 1, n = z.size(); i < n; i++) {
			g_sbb(z[i], x[i], t);
		}
	}
	/*
		z -= px[]
	*/
	void sub_mp_m(const MixPack& z, const RegExp& px, const Reg64& t)
	{
		if (z.isReg(0)) {
			sub(z.getReg(0), ptr [px]);
		} else {
			mov(t, ptr [px]);
			sub(ptr [z.getMem(0)], t);
		}
		for (size_t i = 1, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				sbb(z.getReg(i), ptr [px + i * 8]);
			} else {
				mov(t, ptr [px + i * 8]);
				sbb(ptr [z.getMem(i)], t);
			}
		}
	}
	void store_mp(const RegExp& m, const MixPack& z, const Reg64& t)
	{
		for (size_t i = 0, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				mov(ptr [m + i * 8], z.getReg(i));
			} else {
				mov(t, ptr [z.getMem(i)]);
				mov(ptr [m + i * 8], t);
			}
		}
	}
	void load_mp(const MixPack& z, const RegExp& m, const Reg64& t)
	{
		for (size_t i = 0, n = z.size(); i < n; i++) {
			if (z.isReg(i)) {
				mov(z.getReg(i), ptr [m + i * 8]);
			} else {
				mov(t, ptr [m + i * 8]);
				mov(ptr [z.getMem(i)], t);
			}
		}
	}
	void set_mp(const MixPack& z, const Reg64& t)
	{
		for (size_t i = 0, n = z.size(); i < n; i++) {
			MCL_FP_GEN_OP_MR(mov, z[i], t)
		}
	}
	void mov_mp(const MixPack& z, const MixPack& x, const Reg64& t)
	{
		for (size_t i = 0, n = z.size(); i < n; i++) {
			const MemReg zi = z[i], xi = x[i];
			if (z.isReg(i)) {
				MCL_FP_GEN_OP_RM(mov, zi.getReg(), xi)
			} else {
				if (x.isReg(i)) {
					mov(ptr [z.getMem(i)], x.getReg(i));
				} else {
					mov(t, ptr [x.getMem(i)]);
					mov(ptr [z.getMem(i)], t);
				}
			}
		}
	}
#ifdef _MSC_VER
	void debug_put_mp(const MixPack& mp, int n, const Reg64& t)
	{
		if (n >= 10) exit(1);
		static uint64_t buf[10];
		vmovq(xm0, rax);
		mov(rax, (size_t)buf);
		store_mp(rax, mp, t);
		vmovq(rax, xm0);
		push(rax);
		mov(rax, (size_t)buf);
		debug_put(rax, n);
		pop(rax);
	}
#endif

	std::string mkLabel(const char *label, int n) const
	{
		return std::string(label) + Xbyak::Label::toStr(n);
	}
	/*
		int k = preInvC(pr, px)
	*/
	bool gen_preInv(int2u& func, const Op& op)
	{
		// support general op.N but not fast for op.N > 6
		if (op.primeMode == PM_NIST_P192 || op.N > 6) return false;
		assert(1 <= pn_ && pn_ <= 6);
		const int freeRegNum = 13;
		align(16);
		func = getCurr<int2u>();
		StackFrame sf(this, 2, 10 | UseRDX | UseRCX, (std::max<int>(0, pn_ * 5 - freeRegNum) + 1 + (isFullBit_ ? 1 : 0)) * 8);
		const Reg64& pr = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& t = rcx;
		/*
			k = rax, t = rcx : temporary
			use rdx, pr, px in main loop, so we can use 13 registers
			v = t[0, pn_) : all registers
		*/
		size_t rspPos = 0;

		assert(sf.t.size() >= (size_t)pn_);
		Pack remain = sf.t;

		const MixPack rr(remain, rspPos, pn_);
		remain.append(rdx);
		const MixPack ss(remain, rspPos, pn_);
		remain.append(px);
		const int rSize = (int)remain.size();
		MixPack vv(remain, rspPos, pn_, rSize > 0 ? rSize / 2 : -1);
		remain.append(pr);
		MixPack uu(remain, rspPos, pn_);

		const RegExp keep_pr = rsp + rspPos;
		rspPos += 8;
		const RegExp rTop = rsp + rspPos; // used if isFullBit_

		inLocalLabel();
		mov(ptr [keep_pr], pr);
		mov(rax, px);
		// px is free frome here
		load_mp(vv, rax, t); // v = x
		lea(rax, ptr[rip+pL_]);
		load_mp(uu, rax, t); // u = p_
		// k = 0
		xor_(rax, rax);
		// rTop = 0
		if (isFullBit_) {
			mov(ptr [rTop], rax);
		}
		// r = 0;
		set_mp(rr, rax);
		// s = 1
		set_mp(ss, rax);
		if (ss.isReg(0)) {
			mov(ss.getReg(0), 1);
		} else {
			mov(qword [ss.getMem(0)], 1);
		}
		for (int cn = pn_; cn > 0; cn--) {
			const std::string _lp = mkLabel(".lp", cn);
			const std::string _u_v_odd = mkLabel(".u_v_odd", cn);
			const std::string _u_even = mkLabel(".u_even", cn);
			const std::string _v_even = mkLabel(".v_even", cn);
			const std::string _v_ge_u = mkLabel(".v_ge_u", cn);
			const std::string _v_lt_u = mkLabel(".v_lt_u", cn);
		L(_lp);
			or_mp(vv, t);
			jz(".exit", T_NEAR);

			g_test(uu[0], 1);
			jz(_u_even, T_NEAR);
			g_test(vv[0], 1);
			jz(_v_even, T_NEAR);
		L(_u_v_odd);
			if (cn > 1) {
				isBothZero(vv[cn - 1], uu[cn - 1], t);
				jz(mkLabel(".u_v_odd", cn - 1), T_NEAR);
			}
			for (int i = cn - 1; i >= 0; i--) {
				g_cmp(vv[i], uu[i], t);
				jc(_v_lt_u, T_NEAR);
				if (i > 0) jnz(_v_ge_u, T_NEAR);
			}

		L(_v_ge_u);
			sub_mp(vv, uu, t);
			add_mp(ss, rr, t);
		L(_v_even);
			shr_mp(vv, 1, t);
			twice_mp(rr, t);
			if (isFullBit_) {
				sbb(t, t);
				mov(ptr [rTop], t);
			}
			inc(rax);
			jmp(_lp, T_NEAR);
		L(_v_lt_u);
			sub_mp(uu, vv, t);
			add_mp(rr, ss, t);
			if (isFullBit_) {
				sbb(t, t);
				mov(ptr [rTop], t);
			}
		L(_u_even);
			shr_mp(uu, 1, t);
			twice_mp(ss, t);
			inc(rax);
			jmp(_lp, T_NEAR);

			if (cn > 0) {
				vv.removeLast();
				uu.removeLast();
			}
		}
	L(".exit");
		assert(ss.isReg(0));
		const Reg64& t2 = ss.getReg(0);
		const Reg64& t3 = rdx;

		lea(t2, ptr[rip+pL_]);
		if (isFullBit_) {
			mov(t, ptr [rTop]);
			test(t, t);
			jz("@f");
			sub_mp_m(rr, t2, t);
		L("@@");
		}
		mov(t3, ptr [keep_pr]);
		// pr[] = p[] - rr
		sub_m_mp_m(t3, t2, rr, t);
		jnc("@f");
		// pr[] += p[]
		add_mm(t3, t2, t, pn_);
	L("@@");
		outLocalLabel();
		return true;
	}
	void fpDbl_mod_NIST_P192(const RegExp &py, const RegExp& px, const Pack& t)
	{
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		load_rm(Pack(t2, t1, t0), px); // L=[t2:t1:t0]
		load_rm(Pack(rax, t5, t4), px + 8 * 3); // H = [rax:t5:t4]
		xor_(t3, t3);
		add_rr(Pack(t3, t2, t1, t0), Pack(t3, rax, t5, t4)); // [t3:t2:t1:t0] = L + H
		add_rr(Pack(t2, t1, t0), Pack(t5, t4, rax));
		adc(t3, 0); // [t3:t2:t1:t0] = L + H + [H1:H0:H2]
		add(t1, rax);
		adc(t2, 0);
		adc(t3, 0); // e = t3, t = [t2:t1:t0]
		xor_(t4, t4);
		add(t0, t3);
		adc(t1, 0);
		adc(t2, 0);
		adc(t4, 0); // t + e = [t4:t2:t1:t0]
		add(t1, t3);
		adc(t2, 0);
		adc(t4, 0); // t + e + (e << 64)
		// p = [ffffffffffffffff:fffffffffffffffe:ffffffffffffffff]
		mov(rax, size_t(-1));
		mov(rdx, size_t(-2));
		jz("@f");
		sub_rr(Pack(t2, t1, t0), Pack(rax, rdx, rax));
	L("@@");
		mov_rr(Pack(t5, t4, t3), Pack(t2, t1, t0));
		sub_rr(Pack(t2, t1, t0), Pack(rax, rdx, rax));
		cmovc_rr(Pack(t2, t1, t0), Pack(t5, t4, t3));
		store_mr(py, Pack(t2, t1, t0));
	}
	/*
		p = (1 << 521) - 1
		x = [H:L]
		x % p = (L + H) % p
	*/
	void fpDbl_mod_NIST_P521(const RegExp& py, const RegExp& px, const Pack& t)
	{
		const Reg64& t0 = t[0];
		const Reg64& t1 = t[1];
		const Reg64& t2 = t[2];
		const Reg64& t3 = t[3];
		const Reg64& t4 = t[4];
		const Reg64& t5 = t[5];
		const Reg64& t6 = t[6];
		const Reg64& t7 = t[7];
		const int c = 9;
		const uint32_t mask = (1 << c) - 1;
		const Pack pack(rdx, rax, t6, t5, t4, t3, t2, t1, t0);
		load_rm(pack, px + 64);
		mov(t7, mask);
		and_(t7, t0);
		shrd(t0, t1, c);
		shrd(t1, t2, c);
		shrd(t2, t3, c);
		shrd(t3, t4, c);
		shrd(t4, t5, c);
		shrd(t5, t6, c);
		shrd(t6, rax, c);
		shrd(rax, rdx, c);
		shr(rdx, c);
		// pack = L + H
		add_rm(Pack(rax, t6, t5, t4, t3, t2, t1, t0), px);
		adc(rdx, t7);

		// t = (L + H) >> 521, add t
		mov(t7, rdx);
		shr(t7, c);
		add(t0, t7);
		adc(t1, 0);
		adc(t2, 0);
		adc(t3, 0);
		adc(t4, 0);
		adc(t5, 0);
		adc(t6, 0);
		adc(rax, 0);
		adc(rdx, 0);
		and_(rdx, mask);
		store_mr(py, pack);

		// if [rdx..t0] == p then 0
		and_(rax, t0);
		and_(rax, t1);
		and_(rax, t2);
		and_(rax, t3);
		and_(rax, t4);
		and_(rax, t5);
		and_(rax, t6);
		not_(rax);
		xor_(rdx, (1 << c) - 1);
		or_(rax, rdx);
		jnz("@f");
		xor_(rax, rax);
		for (int i = 0; i < 9; i++) {
			mov(ptr[py + i * 8], rax);
		}
	L("@@");
	}
	void gen_fpDbl_mod_SECP256K1(const RegExp &py, const RegExp& px, const Pack& t)
	{
		Pack c = t.sub(0, 5);
		Pack c4 = t.sub(0, 4);
		const Reg64& t0 = t[5];
		const Reg64& t1 = t[6];
		load_rm(c4, px); // L
		mov(rdx, 0x1000003d1);
		mulAdd(c, 4, px + 4 * 8, t0, true); // c = L + H * rdx
		xor_(rax, rax); // zero
		mulx(t1, t0, c[4]); // [t1:t0] = c[4] * rdx
		add(c[0], t0);
		adc(c[1], t1);
		adc(c[2], rax);
		adc(c[3], rax);
		cmovnc(rdx, rax); // CF ? rdx : 0
		add(c[0], rdx);
		adc(c[1], rax);
		adc(c[2], rax);
		adc(c[3], rax);
		mov(rax, size_t(p_));
		Pack t4 = t.sub(4, 4);
		sub_p_mod(t4, c4, rax);
		store_mr(py, t4);
	}
private:
	FpGenerator(const FpGenerator&);
	void operator=(const FpGenerator&);
	void make_op_rm(void (Xbyak::CodeGenerator::*op)(const Xbyak::Operand&, const Xbyak::Operand&), const Reg64& op1, const MemReg& op2)
	{
		if (op2.isReg()) {
			(this->*op)(op1, op2.getReg());
		} else {
			(this->*op)(op1, qword [op2.getMem()]);
		}
	}
	void make_op_mr(void (Xbyak::CodeGenerator::*op)(const Xbyak::Operand&, const Xbyak::Operand&), const MemReg& op1, const Reg64& op2)
	{
		if (op1.isReg()) {
			(this->*op)(op1.getReg(), op2);
		} else {
			(this->*op)(qword [op1.getMem()], op2);
		}
	}
	void make_op(void (Xbyak::CodeGenerator::*op)(const Xbyak::Operand&, const Xbyak::Operand&), const MemReg& op1, const MemReg& op2, const Reg64& t)
	{
		if (op1.isReg()) {
			make_op_rm(op, op1.getReg(), op2);
		} else if (op2.isReg()) {
			(this->*op)(ptr [op1.getMem()], op2.getReg());
		} else {
			mov(t, ptr [op2.getMem()]);
			(this->*op)(ptr [op1.getMem()], t);
		}
	}
	void g_add(const MemReg& op1, const MemReg& op2, const Reg64& t) { make_op(&Xbyak::CodeGenerator::add, op1, op2, t); }
	void g_adc(const MemReg& op1, const MemReg& op2, const Reg64& t) { make_op(&Xbyak::CodeGenerator::adc, op1, op2, t); }
	void g_sub(const MemReg& op1, const MemReg& op2, const Reg64& t) { make_op(&Xbyak::CodeGenerator::sub, op1, op2, t); }
	void g_sbb(const MemReg& op1, const MemReg& op2, const Reg64& t) { make_op(&Xbyak::CodeGenerator::sbb, op1, op2, t); }
	void g_cmp(const MemReg& op1, const MemReg& op2, const Reg64& t) { make_op(&Xbyak::CodeGenerator::cmp, op1, op2, t); }
	void g_or(const Reg64& r, const MemReg& op) { make_op_rm(&Xbyak::CodeGenerator::or_, r, op); }
	void g_test(const MemReg& op1, const MemReg& op2, const Reg64& t)
	{
		const MemReg *pop1 = &op1;
		const MemReg *pop2 = &op2;
		if (!pop1->isReg()) {
			std::swap(pop1, pop2);
		}
		// (M, M), (R, M), (R, R)
		if (pop1->isReg()) {
			MCL_FP_GEN_OP_MR(test, (*pop2), pop1->getReg())
		} else {
			mov(t, ptr [pop1->getMem()]);
			test(ptr [pop2->getMem()], t);
		}
	}
	void g_mov(const MemReg& op, const Reg64& r)
	{
		make_op_mr(&Xbyak::CodeGenerator::mov, op, r);
	}
	void g_mov(const Reg64& r, const MemReg& op)
	{
		make_op_rm(&Xbyak::CodeGenerator::mov, r, op);
	}
	void g_add(const Reg64& r, const MemReg& mr) { MCL_FP_GEN_OP_RM(add, r, mr) }
	void g_adc(const Reg64& r, const MemReg& mr) { MCL_FP_GEN_OP_RM(adc, r, mr) }
	void isBothZero(const MemReg& op1, const MemReg& op2, const Reg64& t)
	{
		g_mov(t, op1);
		g_or(t, op2);
	}
	void g_test(const MemReg& op, int imm)
	{
		MCL_FP_GEN_OP_MR(test, op, imm)
	}
	/*
		z[] = x[]
	*/
	void mov_rr(const Pack& z, const Pack& x)
	{
		assert(z.size() == x.size());
		for (int i = 0, n = (int)x.size(); i < n; i++) {
			mov(z[i], x[i]);
		}
	}
	/*
		m[] = x[]
	*/
	void store_mr(const RegExp& m, const Pack& x)
	{
		for (int i = 0, n = (int)x.size(); i < n; i++) {
			mov(ptr [m + 8 * i], x[i]);
		}
	}
	void store_mr(const Xbyak::RegRip& m, const Pack& x)
	{
		for (int i = 0, n = (int)x.size(); i < n; i++) {
			mov(ptr [m + 8 * i], x[i]);
		}
	}
	/*
		x[] = m[]
	*/
	template<class ADDR>
	void load_rm(const Pack& z, const ADDR& m)
	{
		for (int i = 0, n = (int)z.size(); i < n; i++) {
			mov(z[i], ptr [m + 8 * i]);
		}
	}
	/*
		z[] += x[]
	*/
	void add_rr(const Pack& z, const Pack& x)
	{
		add(z[0], x[0]);
		assert(z.size() == x.size());
		for (size_t i = 1, n = z.size(); i < n; i++) {
			adc(z[i], x[i]);
		}
	}
	/*
		z[] -= x[]
	*/
	void sub_rr(const Pack& z, const Pack& x)
	{
		sub(z[0], x[0]);
		assert(z.size() == x.size());
		for (size_t i = 1, n = z.size(); i < n; i++) {
			sbb(z[i], x[i]);
		}
	}
	/*
		z[] += m[]
	*/
	template<class ADDR>
	void add_rm(const Pack& z, const ADDR& m, bool withCarry = false)
	{
		if (withCarry) {
			adc(z[0], ptr [m + 8 * 0]);
		} else {
			add(z[0], ptr [m + 8 * 0]);
		}
		for (int i = 1, n = (int)z.size(); i < n; i++) {
			adc(z[i], ptr [m + 8 * i]);
		}
	}
	/*
		z[] -= m[]
	*/
	template<class ADDR>
	void sub_rm(const Pack& z, const ADDR& m, bool withCarry = false)
	{
		if (withCarry) {
			sbb(z[0], ptr [m + 8 * 0]);
		} else {
			sub(z[0], ptr [m + 8 * 0]);
		}
		for (int i = 1, n = (int)z.size(); i < n; i++) {
			sbb(z[i], ptr [m + 8 * i]);
		}
	}
	void cmovc_rr(const Pack& z, const Pack& x)
	{
		for (int i = 0, n = (int)z.size(); i < n; i++) {
			cmovc(z[i], x[i]);
		}
	}
	/*
		t = all or z[i]
		ZF = z is zero
	*/
	void or_mp(const MixPack& z, const Reg64& t)
	{
		const size_t n = z.size();
		if (n == 1) {
			if (z.isReg(0)) {
				test(z.getReg(0), z.getReg(0));
			} else {
				mov(t, ptr [z.getMem(0)]);
				test(t, t);
			}
		} else {
			g_mov(t, z[0]);
			for (size_t i = 1; i < n; i++) {
				g_or(t, z[i]);
			}
		}
	}
	// y[i] &= t
	void and_pr(const Pack& y, const Reg64& t)
	{
		for (int i = 0; i < (int)y.size(); i++) {
			and_(y[i], t);
		}
	}
	/*
		[rdx:x:t0] <- py[1:0] * x
		destroy x, t0
	*/
	void mul2x1(const RegExp& py, const Reg64& x, const Reg64& t0)
	{
		// mulx(H, L, x) = [H:L] = x * rdx
		/*
			rdx:x
			   rax:t0
		*/
		mov(rdx, x);
		mulx(rax, t0, ptr [py]); // [rax:t0] = py[0] * x
		mulx(rdx, x, ptr [py + 8]); // [t:t1] = py[1] * x
		add(x, rax);
		adc(rdx, 0);
	}
	/*
		[rdx:x:t1:t0] <- py[2:1:0] * x
		destroy x, t
	*/
	void mul3x1(const RegExp& py, const Reg64& x, const Reg64& t1, const Reg64& t0, const Reg64& t)
	{
		// mulx(H, L, x) = [H:L] = x * rdx
		/*
			rdx:x
			    t:t1
			      rax:t0
		*/
		mov(rdx, x);
		mulx(rax, t0, ptr [py]); // [rax:t0] = py[0] * x
		mulx(t, t1, ptr [py + 8]); // [t:t1] = py[1] * x
		add(t1, rax);
		mulx(rdx, x, ptr [py + 8 * 2]);
		adc(x, t);
		adc(rdx, 0);
	}
	/*
		[x2:x1:x0] * x0
	*/
	void mul3x1_sqr1(const RegExp& px, const Reg64& x0, const Reg64& t2, const Reg64& t1, const Reg64& t0, const Reg64& t)
	{
		mov(rax, x0);
		mul(x0);
		mov(t0, rax);
		mov(t1, rdx);
		mov(rax, ptr [px + 8]);
		mul(x0);
		mov(ptr [rsp + 0 * 8], rax); // (x0 * x1)_L
		mov(ptr [rsp + 1 * 8], rdx); // (x0 * x1)_H
		mov(t, rax);
		mov(t2, rdx);
		mov(rax, ptr [px + 8 * 2]);
		mul(x0);
		mov(ptr [rsp + 2 * 8], rax); // (x0 * x2)_L
		mov(ptr [rsp + 3 * 8], rdx); // (x0 * x2)_H
		/*
			rdx:rax
			     t2:t
			        t1:t0
		*/
		add(t1, t);
		adc(t2, rax);
		adc(rdx, 0);
	}
	/*
		[x2:x1:x0] * x1
	*/
	void mul3x1_sqr2(const RegExp& px, const Reg64& x1, const Reg64& t2, const Reg64& t1, const Reg64& t0)
	{
		mov(t0, ptr [rsp + 0 * 8]);// (x0 * x1)_L
		mov(rax, x1);
		mul(x1);
		mov(t1, rax);
		mov(t2, rdx);
		mov(rax, ptr [px + 8 * 2]);
		mul(x1);
		mov(ptr [rsp + 4 * 8], rax); // (x1 * x2)_L
		mov(ptr [rsp + 5 * 8], rdx); // (x1 * x2)_H
		/*
			rdx:rax
			     t2:t1
			         t:t0
		*/
		add(t1, ptr [rsp + 1 * 8]); // (x0 * x1)_H
		adc(rax, t2);
		adc(rdx, 0);
	}
	/*
		[rdx:rax:t1:t0] = [x2:x1:x0] * x2
	*/
	void mul3x1_sqr3(const Reg64& x2, const Reg64& t1, const Reg64& t0)
	{
		mov(rax, x2);
		mul(x2);
		/*
			rdx:rax
			     t2:t
			        t1:t0
		*/
		mov(t0, ptr [rsp + 2 * 8]); // (x0 * x2)_L
		mov(t1, ptr [rsp + 3 * 8]); // (x0 * x2)_H
		add(t1, ptr [rsp + 4 * 8]); // (x1 * x2)_L
		adc(rax, ptr [rsp + 5 * 8]); // (x1 * x2)_H
		adc(rdx, 0);
	}

	/*
		c = [c3:y:c1:c0] = c + x[2..0] * y
		q = uint64_t(c0 * pp)
		c = (c + q * p) >> 64
		input  [c3:c2:c1:c0], px, y, p
		output [c0:c3:c2:c1] ; c0 == 0 unless isFullBit_

		@note use rax, rdx, destroy y
	*/
	void montgomery3_sub(uint64_t pp, const Reg64& c3, const Reg64& c2, const Reg64& c1, const Reg64& c0,
		const Reg64& /*px*/, const Reg64& y, const Reg64& p,
		const Reg64& t0, const Reg64& t1, const Reg64& t3, const Reg64& t4, bool isFirst)
	{
		// input [c3:y:c1:0]
		// [t4:c3:y:c1:c0]
		// t4 = 0 or 1 if isFullBit_, = 0 otherwise
		mov(rax, pp);
		mul(c0); // q = rax
		mov(c2, rax);
		mul3x1(p, c2, t1, t0, t3);
		// [rdx:c2:t1:t0] = p * q
		add(c0, t0); // always c0 is zero
		adc(c1, t1);
		adc(c2, y);
		adc(c3, rdx);
		if (isFullBit_) {
			if (isFirst) {
				setc(c0.cvt8());
			} else {
				adc(c0.cvt8(), t4.cvt8());
			}
		}
	}
	/*
		c = [c3:c2:c1:c0]
		c += x[2..0] * y
		q = uint64_t(c0 * pp)
		c = (c + q * p) >> 64
		input  [c3:c2:c1:c0], px, y, p
		output [c0:c3:c2:c1] ; c0 == 0 unless isFullBit_

		@note use rax, rdx, destroy y
	*/
	void montgomery3_1(uint64_t pp, const Reg64& c3, const Reg64& c2, const Reg64& c1, const Reg64& c0,
		const Reg64& px, const Reg64& y, const Reg64& p,
		const Reg64& t0, const Reg64& t1, const Reg64& t3, const Reg64& t4, bool isFirst)
	{
		if (isFirst) {
			mul3x1(px, y, c1, c0, c3);
			mov(c3, rdx);
			// [c3:y:c1:c0] = px[2..0] * y
		} else {
			mul3x1(px, y, t1, t0, t3);
			// [rdx:y:t1:t0] = px[2..0] * y
			add_rr(Pack(c3, y, c1, c0), Pack(rdx, c2, t1, t0));
			if (isFullBit_) setc(t4.cvt8());
		}
		montgomery3_sub(pp, c3, c2, c1, c0, px, y, p, t0, t1, t3, t4, isFirst);
	}
	/*
		[rdx:x:t2:t1:t0] <- py[3:2:1:0] * x
		destroy x, t
	*/
	void mul4x1(const RegExp& py, const Reg64& x, const Reg64& t2, const Reg64& t1, const Reg64& t0)
	{
		mov(rdx, x);
		mulx(t1, t0, ptr [py + 8 * 0]);
		mulx(t2, rax, ptr [py + 8 * 1]);
		add(t1, rax);
		mulx(x, rax, ptr [py + 8 * 2]);
		adc(t2, rax);
		mulx(rdx, rax, ptr [py + 8 * 3]);
		adc(x, rax);
		adc(rdx, 0);
	}

	/*
		c = [c4:c3:c2:c1:c0]
		c += x[3..0] * y
		q = uint64_t(c0 * pp)
		c = (c + q * p) >> 64
		input  [c4:c3:c2:c1:c0], px, y, p
		output [c0:c4:c3:c2:c1]

		@note use rax, rdx, destroy y
		use xt if isFullBit_
	*/
	void montgomery4_1(uint64_t pp, const Reg64& c4, const Reg64& c3, const Reg64& c2, const Reg64& c1, const Reg64& c0,
		const Reg64& px, const Reg64& y, const Reg64& p,
		const Reg64& t0, const Reg64& t1, const Reg64& t2, bool isFirst, const Xmm& xt)
	{
		if (isFirst) {
			mul4x1(px, y, c2, c1, c0);
			mov(c4, rdx);
			// [c4:y:c2:c1:c0] = px[3..0] * y
		} else {
			mul4x1(px, y, t2, t1, t0);
			// [rdx:y:t2:t1:t0] = px[3..0] * y
			if (isFullBit_) {
				vmovq(xt, px);
				xor_(px, px);
			}
			add_rr(Pack(c4, y, c2, c1, c0), Pack(rdx, c3, t2, t1, t0));
			if (isFullBit_) {
				adc(px, 0);
			}
		}
		// [px:c4:y:c2:c1:c0]
		// px = 0 or 1 if isFullBit_, = 0 otherwise
		mov(rax, pp);
		mul(c0); // q = rax
		mov(c3, rax);
		mul4x1(p, c3, t2, t1, t0);
		add(c0, t0); // always c0 is zero
		adc(c1, t1);
		adc(c2, t2);
		adc(c3, y);
		adc(c4, rdx);
		if (isFullBit_) {
			if (isFirst) {
				adc(c0, 0);
			} else {
				adc(c0, px);
				vmovq(px, xt);
			}
		}
	}
	bool gen_fp2Dbl_mulPre(void3u& func)
	{
		if (isFullBit_) return false;
		if (!(pn_ == 4 || pn_ == 6)) return false;
		align(16);
		func = getCurr<void3u>();
		bool embedded = pn_ == 4;

		StackFrame sf(this, 3, 10 | UseRDX, 0, false);
		call(fp2Dbl_mulPreL);
		sf.close();

	L(fp2Dbl_mulPreL);
		const RegExp z = rsp + 0 * 8;
		const RegExp x = rsp + 1 * 8;
		const RegExp y = rsp + 2 * 8;
		const Ext1 s(FpByte_, rsp, 3 * 8);
		const Ext1 t(FpByte_, rsp, s.next);
		const Ext1 d2(FpByte_ * 2, rsp, t.next);
		const int SS = d2.next;
		sub(rsp, SS);
		mov(ptr[z], gp0);
		mov(ptr[x], gp1);
		mov(ptr[y], gp2);
		// s = a + b
		gen_raw_add(s, gp1, gp1 + FpByte_, rax, pn_);
		// t = c + d
		gen_raw_add(t, gp2, gp2 + FpByte_, rax, pn_);
		// d1 = (a + b)(c + d)
		lea(gp0, ptr [gp0 + FpByte_ * 2]);
		if (embedded) {
			mulPre4(gp0, s, t, sf.t);
		} else {
			lea(gp1, ptr [s]);
			lea(gp2, ptr [t]);
			call(fp_mulPreL);
		}
		// d0 = z.a = a c
		mov(gp0, ptr [z]);
		mov(gp1, ptr [x]);
		mov(gp2, ptr [y]);
		if (embedded) {
			mulPre4(gp0, gp1, gp2, sf.t);
		} else {
			call(fp_mulPreL);
		}
		// d2 = z.b = b d
		mov(gp1, ptr [x]);
		add(gp1, FpByte_);
		mov(gp2, ptr [y]);
		add(gp2, FpByte_);
		if (embedded) {
			mulPre4(d2, gp1, gp2, sf.t);
		} else {
			lea(gp0, ptr [d2]);
			call(fp_mulPreL);
		}

		{
			Pack t2 = sf.t;
			if (pn_ == 4) {
				t2 = t2.sub(0, pn_ * 2);
			} else if (pn_ == 6) {
				t2.append(gp1);
				t2.append(gp2);
			}
			assert((int)t2.size() == pn_ * 2);

			mov(gp0, ptr [z]);
			load_rm(t2, gp0 + FpByte_ * 2);
			sub_rm(t2, gp0); // d1 -= d0
			sub_rm(t2, (RegExp)d2); // d1 -= d2
			store_mr(gp0 + FpByte_ * 2, t2);

			gen_raw_sub(gp0, gp0, d2, rax, pn_);
			const RegExp& d0H = gp0 + pn_ * 8;
			const RegExp& d2H = (RegExp)d2 + pn_ * 8;
			gen_raw_fp_sub(d0H, d0H, d2H, t2, true);
		}
		add(rsp, SS);
		ret();
		return true;
	}
	bool gen_fp2Dbl_sqrPre(void2u& func)
	{
		if (isFullBit_) return false;
		if (pn_ != 4 && pn_ != 6) return false;
		align(16);
		func = getCurr<void2u>();
		const RegExp y = rsp + 0 * 8;
		const RegExp x = rsp + 1 * 8;
		const Ext1 t1(FpByte_, rsp, 2 * 8);
		const Ext1 t2(FpByte_, rsp, t1.next);
		// use fp_mulPreL then use 3
		StackFrame sf(this, 3 /* not 2 */, 10 | UseRDX, t2.next);
		mov(ptr [y], gp0);
		mov(ptr [x], gp1);
		Pack t = sf.t;
		if (pn_ == 6) {
			t.append(gp2);
			t.append(rdx);
		}
		const Pack a = t.sub(0, pn_);
		const Pack b = t.sub(pn_, pn_);
		load_rm(b, gp1 + FpByte_);
		for (int i = 0; i < pn_; i++) {
			mov(rax, b[i]);
			if (i == 0) {
				add(rax, rax);
			} else {
				adc(rax, rax);
			}
			mov(ptr [(const RegExp&)t1 + i * 8], rax);
		}
		load_rm(a, gp1);
		add_rr(a, b);
		store_mr(t2, a);
		mov(gp0, ptr [y]);
		add(gp0, FpByte_ * 2);
		lea(gp1, ptr [t1]);
		mov(gp2, ptr [x]);
		call(fp_mulPreL);
		mov(gp0, ptr [x]);
		gen_raw_fp_sub(t1, gp0, gp0 + FpByte_, t, false);
		mov(gp0, ptr [y]);
		lea(gp1, ptr [t1]);
		lea(gp2, ptr [t2]);
		call(fp_mulPreL);
		return true;
	}
	bool gen_fp2Dbl_mul_xi(void2u& func)
	{
		if (isFullBit_) return false;
		if (op_->xi_a != 1) return false;
		if (pn_ > 6) return false;
		align(16);
		func = getCurr<void2u>();
		// y = (x.a - x.b, x.a + x.b)
		StackFrame sf(this, 2, pn_ * 2, FpByte_ * 2);
		Pack t1 = sf.t.sub(0, pn_);
		Pack t2 = sf.t.sub(pn_, pn_);
		const RegExp& ya = sf.p[0];
		const RegExp& yb = sf.p[0] + FpByte_ * 2;
		const RegExp& xa = sf.p[1];
		const RegExp& xb = sf.p[1] + FpByte_ * 2;
		// [rsp] = x.a + x.b
		gen_raw_add(rsp, xa, xb, rax, pn_ * 2);
		// low : x.a =  x.a - x.b
		gen_raw_sub(ya, xa, xb, rax, pn_);
		gen_raw_fp_sub(ya + pn_ * 8, xa + pn_ * 8, xb + pn_ * 8, sf.t, true);

		// low : y.b = [rsp]
		mov_mm(yb, rsp, rax, pn_);
		// high : y.b = (x.a + x.b) % p
		load_rm(t1, rsp + pn_ * 8);
		lea(rax, ptr[rip + pL_]);
		sub_p_mod(t2, t1, rax);
		store_mr(yb + pn_ * 8, t2);
		return true;
	}
	bool gen_fp2_add(void3u& func)
	{
		if (!(pn_ < 6 || (pn_ == 6 && !isFullBit_))) return false;
		align(16);
		func = getCurr<void3u>();
		int n = pn_ * 2 - 1;
		if (isFullBit_) {
			n++;
		}
		StackFrame sf(this, 3, n);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];
		Pack t = sf.t;
		t.append(rax);
		const Reg64 *H = isFullBit_ ? &rax : 0;
		gen_raw_fp_add(pz, px, py, t, false, H);
		gen_raw_fp_add(pz + FpByte_, px + FpByte_, py + FpByte_, t, false, H);
		return true;
	}
	bool gen_fp2_sub(void3u& func)
	{
		if (pn_ > 6) return false;
		align(16);
		func = getCurr<void3u>();
		int n = pn_ * 2 - 1;
		StackFrame sf(this, 3, n);
		const Reg64& pz = sf.p[0];
		const Reg64& px = sf.p[1];
		const Reg64& py = sf.p[2];
		Pack t = sf.t;
		t.append(rax);
		gen_raw_fp_sub(pz, px, py, t, false);
		gen_raw_fp_sub(pz + FpByte_, px + FpByte_, py + FpByte_, t, false);
		return true;
	}
	/*
		for only xi_a = 1
		y.a = a - b
		y.b = a + b
	*/
	void gen_fp2_mul_xi4()
	{
		assert(!isFullBit_);
		StackFrame sf(this, 2, 11 | UseRDX);
		const Reg64& py = sf.p[0];
		const Reg64& px = sf.p[1];
		Pack a = sf.t.sub(0, 4);
		Pack b = sf.t.sub(4, 4);
		Pack t = sf.t.sub(8);
		t.append(rdx);
		assert(t.size() == 4);
		load_rm(a, px);
		load_rm(b, px + FpByte_);
		for (int i = 0; i < pn_; i++) {
			mov(t[i], a[i]);
			if (i == 0) {
				add(t[i], b[i]);
			} else {
				adc(t[i], b[i]);
			}
		}
		sub_rr(a, b);
		lea(rax, ptr[rip+pL_]);
		load_rm(b, rax);
		sbb(rax, rax);
		for (int i = 0; i < pn_; i++) {
			and_(b[i], rax);
		}
		add_rr(a, b);
		store_mr(py, a);
		lea(rax, ptr[rip+pL_]);
		mov_rr(a, t);
		sub_rm(t, rax);
		cmovc_rr(t, a);
		store_mr(py + FpByte_, t);
	}
	void gen_fp2_mul_xi6()
	{
		assert(!isFullBit_);
		StackFrame sf(this, 2, 12);
		const Reg64& py = sf.p[0];
		const Reg64& px = sf.p[1];
		Pack a = sf.t.sub(0, 6);
		Pack b = sf.t.sub(6);
		load_rm(a, px);
		mov_rr(b, a);
		add_rm(b, px + FpByte_);
		sub_rm(a, px + FpByte_);
		lea(rax, ptr[rip+pL_]);
		jnc("@f");
		add_rm(a, rax);
	L("@@");
		store_mr(py, a);
		mov_rr(a, b);
		sub_rm(b, rax);
		cmovc_rr(b, a);
		store_mr(py + FpByte_, b);
	}
	bool gen_fp2_mul_xi(void2u& func)
	{
		if (isFullBit_) return false;
		if (op_->xi_a != 1) return false;
		align(16);
		if (pn_ == 4) {
			func = getCurr<void2u>();
			gen_fp2_mul_xi4();
			return true;
		}
		if (pn_ == 6) {
			func = getCurr<void2u>();
			gen_fp2_mul_xi6();
			return true;
		}
		return false;
	}
	bool gen_fp2_neg(void2u& func)
	{
		if (pn_ > 6) return false;
		align(16);
		func = getCurr<void2u>();
		StackFrame sf(this, 2, UseRDX | pn_);
		gen_raw_neg(sf.p[0], sf.p[1], sf.t);
		gen_raw_neg(sf.p[0] + FpByte_, sf.p[1] + FpByte_, sf.t);
		return true;
	}
	bool gen_fp2_mul(void3u& func)
	{
		if (isFullBit_) return false;
		if (!(pn_ == 4 || pn_ == 6)) return false;
		align(16);
		func = getCurr<void3u>();
		int stackSize = 8 + FpByte_ * 4;
		StackFrame sf(this, 3, 10 | UseRDX, stackSize);
		const RegExp d = rsp + 8;
		mov(ptr[rsp], gp0);
		lea(gp0, ptr [d]);
		// d <- x * y
		call(fp2Dbl_mulPreL);
		mov(gp0, ptr [rsp]);
		lea(gp1, ptr [d]);
		call(fpDbl_modL);

		mov(gp0, ptr [rsp]);
		add(gp0, FpByte_);
		lea(gp1, ptr[d + FpByte_ * 2]);
		call(fpDbl_modL);
		return true;
	}
	bool gen_fp2_sqr(void2u& func)
	{
		if (isFullBit_) return false;
		if (!(pn_ == 4 || pn_ == 6)) return false;
		bool nocarry = (p_[pn_ - 1] >> 62) == 0;
		if (!nocarry) return false;
		align(16);
		func = getCurr<void2u>();

		const RegExp y = rsp + 0 * 8;
		const RegExp x = rsp + 1 * 8;
		const Ext1 t1(FpByte_, rsp, 2 * 8);
		const Ext1 t2(FpByte_, rsp, t1.next);
		const Ext1 t3(FpByte_, rsp, t2.next);
		StackFrame sf(this, 3, 10 | UseRDX, t3.next);
		mov(ptr [y], gp0);
		mov(ptr [x], gp1);
		// t1 = b + b
		lea(gp0, ptr [t1]);
		{
			Pack t = sf.t.sub(0, pn_);
			load_rm(t, gp1 + FpByte_);
			shl1(t);
			store_mr(gp0, t);
		}
		// t1 = 2ab
		mov(gp1, gp0);
		mov(gp2, ptr [x]);
		call(fp_mulL);

		{
			Pack t = sf.t;
			t.append(rdx);
			t.append(gp1);
			Pack a = t.sub(0, pn_);
			Pack b = t.sub(pn_, pn_);
			mov(gp0, ptr [x]);
			load_rm(a, gp0);
			load_rm(b, gp0 + FpByte_);
			// t2 = a + b
			for (int i = 0; i < pn_; i++) {
				mov(rax, a[i]);
				if (i == 0) {
					add(rax, b[i]);
				} else {
					adc(rax, b[i]);
				}
				mov(ptr [(RegExp)t2 + i * 8], rax);
			}
			// t3 = a + p - b
			lea(rax, ptr[rip+pL_]);
			add_rm(a, rax);
			sub_rr(a, b);
			store_mr(t3, a);
		}

		mov(gp0, ptr [y]);
		lea(gp1, ptr [t2]);
		lea(gp2, ptr [t3]);
		call(fp_mulL);
		mov(gp0, ptr [y]);
		mov_mm(gp0 + FpByte_, t1, rax, pn_);
		return true;
	}
};

} } // mcl::fp

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#endif
#endif
