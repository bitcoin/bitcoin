#include "llvm_gen.hpp"
#include <cybozu/option.hpp>
#include <mcl/op.hpp>
#include <map>
#include <set>
#include <fstream>

typedef std::set<std::string> StrSet;

struct Code : public mcl::Generator {
	typedef std::map<int, Function> FunctionMap;
	typedef std::vector<Operand> OperandVec;
	Operand Void;
	uint32_t unit;
	uint32_t unit2;
	uint32_t bit;
	uint32_t N;
	const StrSet *privateFuncList;
	bool wasm;
	std::string suf;
	std::string unitStr;
	Function mulUU;
	Function mul32x32; // for WASM
	Function extractHigh;
	Function mulPos;
	Function makeNIST_P192;
	Function mcl_fpDbl_mod_NIST_P192;
	Function mcl_fp_sqr_NIST_P192;
	FunctionMap mcl_fp_shr1_M;
	FunctionMap mcl_fp_addPreM;
	FunctionMap mcl_fp_subPreM;
	FunctionMap mcl_fp_addM;
	FunctionMap mcl_fp_subM;
	FunctionMap mulPvM;
	FunctionMap mcl_fp_mulUnitPreM;
	FunctionMap mcl_fpDbl_mulPreM;
	FunctionMap mcl_fpDbl_sqrPreM;
	FunctionMap mcl_fp_montM;
	FunctionMap mcl_fp_montRedM;
	Code() : unit(0), unit2(0), bit(0), N(0), privateFuncList(0), wasm(false) { }
	void verifyAndSetPrivate(Function& f)
	{
		if (privateFuncList && privateFuncList->find(f.name) != privateFuncList->end()) {
			f.setPrivate();
		}
	}
	void storeN(Operand r, Operand p, int offset = 0)
	{
		if (p.bit != unit) {
			throw cybozu::Exception("bad IntPtr size") << p.bit;
		}
		if (offset > 0) {
			p = getelementptr(p, offset);
		}
		if (r.bit == unit) {
			store(r, p);
			return;
		}
		const size_t n = r.bit / unit;
		for (uint32_t i = 0; i < n; i++) {
			store(trunc(r, unit), getelementptr(p, i));
			if (i < n - 1) {
				r = lshr(r, unit);
			}
		}
	}
	Operand loadN(Operand p, size_t n, int offset = 0)
	{
		if (p.bit != unit) {
			throw cybozu::Exception("bad IntPtr size") << p.bit;
		}
		if (offset > 0) {
			p = getelementptr(p, offset);
		}
		Operand v = load(p);
		for (uint32_t i = 1; i < n; i++) {
			v = zext(v, v.bit + unit);
			Operand t = load(getelementptr(p, i));
			t = zext(t, v.bit);
			t = shl(t, unit * i);
			v = _or(v, t);
		}
		return v;
	}
	void gen_mul32x32()
	{
		const int u = 32;
		resetGlobalIdx();
		Operand z(Int, u * 2);
		Operand x(Int, u);
		Operand y(Int, u);
		mul32x32 = Function("mul32x32L", z, x, y);
		mul32x32.setPrivate();
		verifyAndSetPrivate(mul32x32);
		beginFunc(mul32x32);

		x = zext(x, u * 2);
		y = zext(y, u * 2);
		z = mul(x, y);
		ret(z);
		endFunc();
	}
	void gen_mul64x64(Operand& z, Operand& x, Operand& y)
	{
		Operand a = trunc(lshr(x, 32), 32);
		Operand b = trunc(x, 32);
		Operand c = trunc(lshr(y, 32), 32);
		Operand d = trunc(y, 32);
		Operand ad = call(mul32x32, a, d);
		Operand bd = call(mul32x32, b, d);
		bd = zext(bd, 96);
		ad = shl(zext(ad, 96), 32);
		ad = add(ad, bd);
		Operand ac = call(mul32x32, a, c);
		Operand bc = call(mul32x32, b, c);
		bc = zext(bc, 96);
		ac = shl(zext(ac, 96), 32);
		ac = add(ac, bc);
		ad = zext(ad, 128);
		ac = shl(zext(ac, 128), 32);
		z = add(ac, ad);
	}
	void gen_multi3()
	{
		resetGlobalIdx();
		Operand z(Int, unit2);
		Operand x(Int, unit);
		Operand y(Int, unit);
		std::string name = "__multi3";
		Function f(name, z, x, y);
//		f.setPrivate();
		verifyAndSetPrivate(f);
		beginFunc(f);

		gen_mul64x64(z, x, y);
		ret(z);
		endFunc();
	}
	void gen_mulUU()
	{
		if (wasm) {
			gen_mul32x32();
			gen_multi3();
		}
		resetGlobalIdx();
		Operand z(Int, unit2);
		Operand x(Int, unit);
		Operand y(Int, unit);
		std::string name = "mul";
		name += unitStr + "x" + unitStr + "L";
		mulUU = Function(name, z, x, y);
		mulUU.setPrivate();
		verifyAndSetPrivate(mulUU);
		beginFunc(mulUU);

		if (wasm) {
			gen_mul64x64(z, x, y);
		} else {
			x = zext(x, unit2);
			y = zext(y, unit2);
			z = mul(x, y);
		}
		ret(z);
		endFunc();
	}
	void gen_extractHigh()
	{
		resetGlobalIdx();
		Operand z(Int, unit);
		Operand x(Int, unit2);
		std::string name = "extractHigh";
		name += unitStr;
		extractHigh = Function(name, z, x);
		extractHigh.setPrivate();
		beginFunc(extractHigh);

		x = lshr(x, unit);
		z = trunc(x, unit);
		ret(z);
		endFunc();
	}
	void gen_mulPos()
	{
		resetGlobalIdx();
		Operand xy(Int, unit2);
		Operand px(IntPtr, unit);
		Operand y(Int, unit);
		Operand i(Int, unit);
		std::string name = "mulPos";
		name += unitStr + "x" + unitStr;
		mulPos = Function(name, xy, px, y, i);
		mulPos.setPrivate();
		beginFunc(mulPos);

		Operand x = load(getelementptr(px, i));
		xy = call(mulUU, x, y);
		ret(xy);
		endFunc();
	}
	Operand extract192to64(const Operand& x, uint32_t shift)
	{
		Operand y = lshr(x, shift);
		y = trunc(y, 64);
		return y;
	}
	void gen_makeNIST_P192()
	{
		resetGlobalIdx();
		Operand p(Int, 192);
		Operand p0(Int, 64);
		Operand p1(Int, 64);
		Operand p2(Int, 64);
		Operand _0 = makeImm(64, 0);
		Operand _1 = makeImm(64, 1);
		Operand _2 = makeImm(64, 2);
		makeNIST_P192 = Function("makeNIST_P192L" + suf, p);
		verifyAndSetPrivate(makeNIST_P192);
		beginFunc(makeNIST_P192);
		p0 = sub(_0, _1);
		p1 = sub(_0, _2);
		p2 = sub(_0, _1);
		p0 = zext(p0, 192);
		p1 = zext(p1, 192);
		p2 = zext(p2, 192);
		p1 = shl(p1, 64);
		p2 = shl(p2, 128);
		p = add(p0, p1);
		p = add(p, p2);
		ret(p);
		endFunc();
	}
	/*
		NIST_P192
		p = 0xfffffffffffffffffffffffffffffffeffffffffffffffff
		      0                1                2
		ffffffffffffffff fffffffffffffffe ffffffffffffffff

		p = (1 << 192) - (1 << 64) - 1
		(1 << 192) % p = (1 << 64) + 1
		L : 192bit
		Hi: 64bit
		x = [H:L] = [H2:H1:H0:L]
		mod p
		x = L + H + (H << 64)
		  = L + H + [H1:H0:0] + H2 + (H2 << 64)
		[e:t] = L + H + [H1:H0:H2] + [H2:0] ; 2bit(e) over
		y = t + e + (e << 64)
		if (y >= p) y -= p
	*/
	void gen_mcl_fpDbl_mod_NIST_P192()
	{
		resetGlobalIdx();
		Operand out(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand dummy(IntPtr, unit);
		mcl_fpDbl_mod_NIST_P192 = Function("mcl_fpDbl_mod_NIST_P192L" + suf, Void, out, px, dummy);
		verifyAndSetPrivate(mcl_fpDbl_mod_NIST_P192);
		beginFunc(mcl_fpDbl_mod_NIST_P192);

		const int n = 192 / unit;
		Operand L = loadN(px, n);
		L = zext(L, 256);

		Operand H192 = loadN(px, n, n);
		Operand H = zext(H192, 256);

		Operand H10 = shl(H192, 64);
		H10 = zext(H10, 256);

		Operand H2 = extract192to64(H192, 128);
		H2 = zext(H2, 256);
		Operand H102 = _or(H10, H2);

		H2 = shl(H2, 64);

		Operand t = add(L, H);
		t = add(t, H102);
		t = add(t, H2);

		Operand e = lshr(t, 192);
		e = trunc(e, 64);
		e = zext(e, 256);
		Operand e2 = shl(e, 64);
		e = _or(e, e2);

		t = trunc(t, 192);
		t = zext(t, 256);

		Operand z = add(t, e);
		Operand p = call(makeNIST_P192);
		p = zext(p, 256);
		Operand zp = sub(z, p);
		Operand c = trunc(lshr(zp, 192), 1);
		z = trunc(select(c, z, zp), 192);
		storeN(z, out);
		ret(Void);
		endFunc();
	}
	/*
		NIST_P521
		p = (1 << 521) - 1
		x = [H:L]
		x % p = (L + H) % p
	*/
	void gen_mcl_fpDbl_mod_NIST_P521()
	{
		resetGlobalIdx();
		const uint32_t len = 521;
		const uint32_t n = len / unit;
		const uint32_t round = unit * (n + 1);
		const uint32_t rem = len - n * unit;
		const size_t mask = -(1 << rem);
		const Operand py(IntPtr, unit);
		const Operand px(IntPtr, unit);
		const Operand dummy(IntPtr, unit);
		Function f("mcl_fpDbl_mod_NIST_P521L" + suf, Void, py, px, dummy);
		verifyAndSetPrivate(f);
		beginFunc(f);
		Operand x = loadN(px, n * 2 + 1);
		Operand L = trunc(x, len);
		L = zext(L, round);
		Operand H = lshr(x, len);
		H = trunc(H, round); // x = [H:L]
		Operand t = add(L, H);
		Operand t0 = lshr(t, len);
		t0 = _and(t0, makeImm(round, 1));
		t = add(t, t0);
		t = trunc(t, len);
		Operand z0 = zext(t, round);
		t = extract(z0, n * unit);
		Operand m = _or(t, makeImm(unit, mask));
		for (uint32_t i = 0; i < n; i++) {
			Operand s = extract(z0, unit * i);
			m = _and(m, s);
		}
		Operand c = icmp(eq, m, makeImm(unit, -1));
		Label zero("zero");
		Label nonzero("nonzero");
		br(c, zero, nonzero);
	putLabel(zero);
		for (uint32_t i = 0; i < n + 1; i++) {
			storeN(makeImm(unit, 0), py, i);
		}
		ret(Void);
	putLabel(nonzero);
		storeN(z0, py);
		ret(Void);
		endFunc();
	}
	void gen_mcl_fp_sqr_NIST_P192()
	{
		resetGlobalIdx();
		Operand py(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand dummy(IntPtr, unit);
		mcl_fp_sqr_NIST_P192 = Function("mcl_fp_sqr_NIST_P192L" + suf, Void, py, px, dummy);
		verifyAndSetPrivate(mcl_fp_sqr_NIST_P192);
		beginFunc(mcl_fp_sqr_NIST_P192);
		Operand buf = alloca_(unit, 192 * 2 / unit);
		// QQQ define later
		Function mcl_fpDbl_sqrPre("mcl_fpDbl_sqrPre" + cybozu::itoa(192 / unit) + "L" + suf, Void, buf, px);
		call(mcl_fpDbl_sqrPre, buf, px);
		call(mcl_fpDbl_mod_NIST_P192, py, buf, buf/*dummy*/);
		ret(Void);
		endFunc();
	}
	void gen_mcl_fp_mulNIST_P192()
	{
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		Operand dummy(IntPtr, unit);
		Function f("mcl_fp_mulNIST_P192L" + suf, Void, pz, px, py, dummy);
		verifyAndSetPrivate(f);
		beginFunc(f);
		Operand buf = alloca_(unit, 192 * 2 / unit);
		// QQQ define later
		Function mcl_fpDbl_mulPre("mcl_fpDbl_mulPre" + cybozu::itoa(192 / unit) + "L" + suf, Void, buf, px, py);
		call(mcl_fpDbl_mulPre, buf, px, py);
		call(mcl_fpDbl_mod_NIST_P192, pz, buf, buf/*dummy*/);
		ret(Void);
		endFunc();
	}
	void gen_once()
	{
		gen_mulUU();
		gen_extractHigh();
		gen_mulPos();
		gen_makeNIST_P192();
		gen_mcl_fpDbl_mod_NIST_P192();
		gen_mcl_fp_sqr_NIST_P192();
		gen_mcl_fp_mulNIST_P192();
		gen_mcl_fpDbl_mod_NIST_P521();
	}
	Operand extract(const Operand& x, uint32_t shift)
	{
		Operand t = lshr(x, shift);
		t = trunc(t, unit);
		return t;
	}
	void gen_mcl_fp_addsubPre(bool isAdd)
	{
		resetGlobalIdx();
		Operand r(Int, unit);
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		std::string name;
		if (isAdd) {
			name = "mcl_fp_addPre" + cybozu::itoa(N) + "L" + suf;
			mcl_fp_addPreM[N] = Function(name, r, pz, px, py);
			verifyAndSetPrivate(mcl_fp_addPreM[N]);
			beginFunc(mcl_fp_addPreM[N]);
		} else {
			name = "mcl_fp_subPre" + cybozu::itoa(N) + "L" + suf;
			mcl_fp_subPreM[N] = Function(name, r, pz, px, py);
			verifyAndSetPrivate(mcl_fp_subPreM[N]);
			beginFunc(mcl_fp_subPreM[N]);
		}
		Operand x = zext(loadN(px, N), bit + unit);
		Operand y = zext(loadN(py, N), bit + unit);
		Operand z;
		if (isAdd) {
			z = add(x, y);
			storeN(trunc(z, bit), pz);
			r = trunc(lshr(z, bit), unit);
		} else {
			z = sub(x, y);
			storeN(trunc(z, bit), pz);
			r = _and(trunc(lshr(z, bit), unit), makeImm(unit, 1));
		}
		ret(r);
		endFunc();
	}
#if 0 // void-return version
	void gen_mcl_fp_addsubPre(bool isAdd)
	{
		resetGlobalIdx();
		Operand pz(IntPtr, bit);
		Operand px(IntPtr, bit);
		Operand py(IntPtr, bit);
		std::string name;
		if (isAdd) {
			name = "mcl_fp_addPre" + cybozu::itoa(bit) + "L";
			mcl_fp_addPreM[bit] = Function(name, Void, pz, px, py);
			verifyAndSetPrivate(mcl_fp_addPreM[bit]);
			beginFunc(mcl_fp_addPreM[bit]);
		} else {
			name = "mcl_fp_subPre" + cybozu::itoa(bit) + "L";
			mcl_fp_subPreM[bit] = Function(name, Void, pz, px, py);
			verifyAndSetPrivate(mcl_fp_subPreM[bit]);
			beginFunc(mcl_fp_subPreM[bit]);
		}
		Operand x = load(px);
		Operand y = load(py);
		Operand z;
		if (isAdd) {
			z = add(x, y);
		} else {
			z = sub(x, y);
		}
		store(z, pz);
		ret(Void);
		endFunc();
	}
#endif
	void gen_mcl_fp_shr1()
	{
		resetGlobalIdx();
		Operand py(IntPtr, unit);
		Operand px(IntPtr, unit);
		std::string name = "mcl_fp_shr1_" + cybozu::itoa(N) + "L" + suf;
		mcl_fp_shr1_M[N] = Function(name, Void, py, px);
		verifyAndSetPrivate(mcl_fp_shr1_M[N]);
		beginFunc(mcl_fp_shr1_M[N]);
		Operand x = loadN(px, N);
		x = lshr(x, 1);
		storeN(x, py);
		ret(Void);
		endFunc();
	}
	void gen_mcl_fp_add(bool isFullBit = true)
	{
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		Operand pp(IntPtr, unit);
		std::string name = "mcl_fp_add";
		if (!isFullBit) {
			name += "NF";
		}
		name += cybozu::itoa(N) + "L" + suf;
		mcl_fp_addM[N] = Function(name, Void, pz, px, py, pp);
		verifyAndSetPrivate(mcl_fp_addM[N]);
		beginFunc(mcl_fp_addM[N]);
		Operand x = loadN(px, N);
		Operand y = loadN(py, N);
		if (isFullBit) {
			x = zext(x, bit + unit);
			y = zext(y, bit + unit);
			Operand t0 = add(x, y);
			Operand t1 = trunc(t0, bit);
			storeN(t1, pz);
			Operand p = loadN(pp, N);
			p = zext(p, bit + unit);
			Operand vc = sub(t0, p);
			Operand c = lshr(vc, bit);
			c = trunc(c, 1);
		Label carry("carry");
		Label nocarry("nocarry");
			br(c, carry, nocarry);
		putLabel(nocarry);
			storeN(trunc(vc, bit), pz);
			ret(Void);
		putLabel(carry);
		} else {
			x = add(x, y);
			Operand p = loadN(pp, N);
			y = sub(x, p);
			Operand c = trunc(lshr(y, bit - 1), 1);
			x = select(c, x, y);
			storeN(x, pz);
		}
		ret(Void);
		endFunc();
	}
	void gen_mcl_fp_sub(bool isFullBit = true)
	{
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		Operand pp(IntPtr, unit);
		std::string name = "mcl_fp_sub";
		if (!isFullBit) {
			name += "NF";
		}
		name += cybozu::itoa(N) + "L" + suf;
		mcl_fp_subM[N] = Function(name, Void, pz, px, py, pp);
		verifyAndSetPrivate(mcl_fp_subM[N]);
		beginFunc(mcl_fp_subM[N]);
		Operand x = loadN(px, N);
		Operand y = loadN(py, N);
		if (isFullBit) {
			x = zext(x, bit + unit);
			y = zext(y, bit + unit);
			Operand vc = sub(x, y);
			Operand v, c;
			v = trunc(vc, bit);
			c = lshr(vc, bit);
			c = trunc(c, 1);
			storeN(v, pz);
		Label carry("carry");
		Label nocarry("nocarry");
			br(c, carry, nocarry);
		putLabel(nocarry);
			ret(Void);
		putLabel(carry);
			Operand p = loadN(pp, N);
			Operand t = add(v, p);
			storeN(t, pz);
		} else {
			Operand v = sub(x, y);
			Operand c;
			c = trunc(lshr(v, bit - 1), 1);
			Operand p = loadN(pp, N);
			c = select(c, p, makeImm(bit, 0));
			Operand t = add(v, c);
			storeN(t, pz);
		}
		ret(Void);
		endFunc();
	}
	void gen_mcl_fpDbl_add()
	{
		// QQQ : generate unnecessary memory copy for large bit
		const int bu = bit + unit;
		const int b2 = bit * 2;
		const int b2u = b2 + unit;
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		Operand pp(IntPtr, unit);
		std::string name = "mcl_fpDbl_add" + cybozu::itoa(N) + "L" + suf;
		Function f(name, Void, pz, px, py, pp);
		verifyAndSetPrivate(f);
		beginFunc(f);
		Operand x = loadN(px, N * 2);
		Operand y = loadN(py, N * 2);
		x = zext(x, b2u);
		y = zext(y, b2u);
		Operand t = add(x, y); // x + y = [H:L]
		Operand L = trunc(t, bit);
		storeN(L, pz);

		Operand H = lshr(t, bit);
		H = trunc(H, bu);
		Operand p = loadN(pp, N);
		p = zext(p, bu);
		Operand Hp = sub(H, p);
		t = lshr(Hp, bit);
		t = trunc(t, 1);
		t = select(t, H, Hp);
		t = trunc(t, bit);
		storeN(t, pz, N);
		ret(Void);
		endFunc();
	}
	void gen_mcl_fpDbl_sub()
	{
		// QQQ : rol is used?
		const int b2 = bit * 2;
		const int b2u = b2 + unit;
		resetGlobalIdx();
		std::string name = "mcl_fpDbl_sub" + cybozu::itoa(N) + "L" + suf;
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		Operand pp(IntPtr, unit);
		Function f(name, Void, pz, px, py, pp);
		verifyAndSetPrivate(f);
		beginFunc(f);
		Operand x = loadN(px, N * 2);
		Operand y = loadN(py, N * 2);
		x = zext(x, b2u);
		y = zext(y, b2u);
		Operand vc = sub(x, y); // x - y = [H:L]
		Operand L = trunc(vc, bit);
		storeN(L, pz);

		Operand H = lshr(vc, bit);
		H = trunc(H, bit);
		Operand c = lshr(vc, b2);
		c = trunc(c, 1);
		Operand p = loadN(pp, N);
		c = select(c, p, makeImm(bit, 0));
		Operand t = add(H, c);
		storeN(t, pz, N);
		ret(Void);
		endFunc();
	}
	/*
		return [px[n-1]:px[n-2]:...:px[0]]
	*/
	Operand pack(const Operand *px, size_t n)
	{
		Operand x = px[0];
		for (size_t i = 1; i < n; i++) {
			Operand y = px[i];
			uint32_t shift = x.bit;
			uint32_t size = x.bit + y.bit;
			x = zext(x, size);
			y = zext(y, size);
			y = shl(y, shift);
			x = _or(x, y);
		}
		return x;
	}
	/*
		z = px[0..N] * y
	*/
	void gen_mulPv()
	{
		const int bu = bit + unit;
		resetGlobalIdx();
		Operand z(Int, bu);
		Operand px(IntPtr, unit);
		Operand y(Int, unit);
		std::string name = "mulPv" + cybozu::itoa(bit) + "x" + cybozu::itoa(unit) + suf;
		mulPvM[bit] = Function(name, z, px, y);
		// workaround at https://github.com/herumi/mcl/pull/82
//		mulPvM[bit].setPrivate();
		verifyAndSetPrivate(mulPvM[bit]);
		beginFunc(mulPvM[bit]);
		OperandVec L(N), H(N);
		for (uint32_t i = 0; i < N; i++) {
			Operand xy = call(mulPos, px, y, makeImm(unit, i));
			L[i] = trunc(xy, unit);
			H[i] = call(extractHigh, xy);
		}
		Operand LL = pack(&L[0], N);
		Operand HH = pack(&H[0], N);
		LL = zext(LL, bu);
		HH = zext(HH, bu);
		HH = shl(HH, unit);
		z = add(LL, HH);
		ret(z);
		endFunc();
	}
	void gen_mcl_fp_mulUnitPre()
	{
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand y(Int, unit);
		std::string name = "mcl_fp_mulUnitPre" + cybozu::itoa(N) + "L" + suf;
		mcl_fp_mulUnitPreM[N] = Function(name, Void, pz, px, y);
		verifyAndSetPrivate(mcl_fp_mulUnitPreM[N]);
		beginFunc(mcl_fp_mulUnitPreM[N]);
		Operand z = call(mulPvM[bit], px, y);
		storeN(z, pz);
		ret(Void);
		endFunc();
	}
	void generic_fpDbl_mul(const Operand& pz, const Operand& px, const Operand& py)
	{
		if (N == 1) {
			Operand x = load(px);
			Operand y = load(py);
			x = zext(x, unit * 2);
			y = zext(y, unit * 2);
			Operand z = mul(x, y);
			storeN(z, pz);
			ret(Void);
		} else if (N > 8 && (N % 2) == 0) {
			/*
				W = 1 << half
				(aW + b)(cW + d) = acW^2 + (ad + bc)W + bd
				ad + bc = (a + b)(c + d) - ac - bd
				@note Karatsuba is slower for N = 8
			*/
			const int H = N / 2;
			const int half = bit / 2;
			Operand pxW = getelementptr(px, H);
			Operand pyW = getelementptr(py, H);
			Operand pzWW = getelementptr(pz, N);
			call(mcl_fpDbl_mulPreM[H], pz, px, py); // bd
			call(mcl_fpDbl_mulPreM[H], pzWW, pxW, pyW); // ac

			Operand a = zext(loadN(pxW, H), half + unit);
			Operand b = zext(loadN(px, H), half + unit);
			Operand c = zext(loadN(pyW, H), half + unit);
			Operand d = zext(loadN(py, H), half + unit);
			Operand t1 = add(a, b);
			Operand t2 = add(c, d);
			Operand buf = alloca_(unit, N);
			Operand t1L = trunc(t1, half);
			Operand t2L = trunc(t2, half);
			Operand c1 = trunc(lshr(t1, half), 1);
			Operand c2 = trunc(lshr(t2, half), 1);
			Operand c0 = _and(c1, c2);
			c1 = select(c1, t2L, makeImm(half, 0));
			c2 = select(c2, t1L, makeImm(half, 0));
			Operand buf1 = alloca_(unit, half / unit);
			Operand buf2 = alloca_(unit, half / unit);
			storeN(t1L, buf1);
			storeN(t2L, buf2);
			call(mcl_fpDbl_mulPreM[N / 2], buf, buf1, buf2);
			Operand t = loadN(buf, N);
			t = zext(t, bit + unit);
			c0 = zext(c0, bit + unit);
			c0 = shl(c0, bit);
			t = _or(t, c0);
			c1 = zext(c1, bit + unit);
			c2 = zext(c2, bit + unit);
			c1 = shl(c1, half);
			c2 = shl(c2, half);
			t = add(t, c1);
			t = add(t, c2);
			t = sub(t, zext(loadN(pz, N), bit + unit));
			t = sub(t, zext(loadN(pz, N, N), bit + unit));
			if (bit + half > t.bit) {
				t = zext(t, bit + half);
			}
			t = add(t, loadN(pz, N + H, H));
			storeN(t, pz, H);
			ret(Void);
		} else {
			Operand y = load(py);
			Operand xy = call(mulPvM[bit], px, y);
			store(trunc(xy, unit), pz);
			Operand t = lshr(xy, unit);
			for (uint32_t i = 1; i < N; i++) {
				y = loadN(py, 1, i);
				xy = call(mulPvM[bit], px, y);
				t = add(t, xy);
				if (i < N - 1) {
					storeN(trunc(t, unit), pz, i);
					t = lshr(t, unit);
				}
			}
			storeN(t, pz, N - 1);
			ret(Void);
		}
	}
	void gen_mcl_fpDbl_mulPre()
	{
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		std::string name = "mcl_fpDbl_mulPre" + cybozu::itoa(N) + "L" + suf;
		mcl_fpDbl_mulPreM[N] = Function(name, Void, pz, px, py);
		verifyAndSetPrivate(mcl_fpDbl_mulPreM[N]);
		beginFunc(mcl_fpDbl_mulPreM[N]);
		generic_fpDbl_mul(pz, px, py);
		endFunc();
	}
	void gen_mcl_fpDbl_sqrPre()
	{
		resetGlobalIdx();
		Operand py(IntPtr, unit);
		Operand px(IntPtr, unit);
		std::string name = "mcl_fpDbl_sqrPre" + cybozu::itoa(N) + "L" + suf;
		mcl_fpDbl_sqrPreM[N] = Function(name, Void, py, px);
		verifyAndSetPrivate(mcl_fpDbl_sqrPreM[N]);
		beginFunc(mcl_fpDbl_sqrPreM[N]);
		generic_fpDbl_mul(py, px, px);
		endFunc();
	}
	void gen_mcl_fp_mont(bool isFullBit = true)
	{
		const int bu = bit + unit;
		const int bu2 = bit + unit * 2;
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand px(IntPtr, unit);
		Operand py(IntPtr, unit);
		Operand pp(IntPtr, unit);
		std::string name = "mcl_fp_mont";
		if (!isFullBit) {
			name += "NF";
		}
		name += cybozu::itoa(N) + "L" + suf;
		mcl_fp_montM[N] = Function(name, Void, pz, px, py, pp);
		mcl_fp_montM[N].setAlias();
		verifyAndSetPrivate(mcl_fp_montM[N]);
		beginFunc(mcl_fp_montM[N]);
		Operand rp = load(getelementptr(pp, -1));
		Operand z, s, a;
		if (isFullBit) {
			for (uint32_t i = 0; i < N; i++) {
				Operand y = load(getelementptr(py, i));
				Operand xy = call(mulPvM[bit], px, y);
				Operand at;
				if (i == 0) {
					a = zext(xy, bu2);
					at = trunc(xy, unit);
				} else {
					xy = zext(xy, bu2);
					a = add(s, xy);
					at = trunc(a, unit);
				}
				Operand q = mul(at, rp);
				Operand pq = call(mulPvM[bit], pp, q);
				pq = zext(pq, bu2);
				Operand t = add(a, pq);
				s = lshr(t, unit);
			}
			s = trunc(s, bu);
			Operand p = zext(loadN(pp, N), bu);
			Operand vc = sub(s, p);
			Operand c = trunc(lshr(vc, bit), 1);
			z = select(c, s, vc);
			z = trunc(z, bit);
			storeN(z, pz);
		} else {
			Operand y = load(py);
			Operand xy = call(mulPvM[bit], px, y);
			Operand c0 = trunc(xy, unit);
			Operand q = mul(c0, rp);
			Operand pq = call(mulPvM[bit], pp, q);
			Operand t = add(xy, pq);
			t = lshr(t, unit); // bu-bit
			for (uint32_t i = 1; i < N; i++) {
				y = load(getelementptr(py, i));
				xy = call(mulPvM[bit], px, y);
				t = add(t, xy);
				c0 = trunc(t, unit);
				q = mul(c0, rp);
				pq = call(mulPvM[bit], pp, q);
				t = add(t, pq);
				t = lshr(t, unit);
			}
			t = trunc(t, bit);
			Operand vc = sub(t, loadN(pp, N));
			Operand c = trunc(lshr(vc, bit - 1), 1);
			z = select(c, t, vc);
			storeN(z, pz);
		}
		ret(Void);
		endFunc();
	}
	// return [H:L]
	Operand pack(Operand H, Operand L)
	{
		int size = H.bit + L.bit;
		H = zext(H, size);
		H = shl(H, L.bit);
		L = zext(L, size);
		H = _or(H, L);
		return H;
	}
	// split x to [ret:L] s.t. size of L = sizeL
	Operand split(Operand *L, const Operand& x, int sizeL)
	{
		Operand ret = lshr(x, sizeL);
		ret = trunc(ret, ret.bit - sizeL);
		*L = trunc(x, sizeL);
		return ret;
	}
	void gen_mcl_fp_montRed(bool isFullBit = true)
	{
		resetGlobalIdx();
		Operand pz(IntPtr, unit);
		Operand pxy(IntPtr, unit);
		Operand pp(IntPtr, unit);
		std::string name = "mcl_fp_montRed";
		if (!isFullBit) {
			name += "NF";
		}
		name += cybozu::itoa(N) + "L" + suf;
		mcl_fp_montRedM[N] = Function(name, Void, pz, pxy, pp);
		verifyAndSetPrivate(mcl_fp_montRedM[N]);
		beginFunc(mcl_fp_montRedM[N]);
		Operand rp = load(getelementptr(pp, -1));
		Operand p = loadN(pp, N);
		const int bu = bit + unit;
		const int bu2 = bit + unit * 2;
		Operand t = loadN(pxy, N);
		Operand H;
		for (uint32_t i = 0; i < N; i++) {
			Operand q;
			if (N == 1) {
				q = mul(t, rp);
			} else {
				q = mul(trunc(t, unit), rp);
			}
			Operand pq = call(mulPvM[bit], pp, q);
			if (i > 0) {
				H = zext(H, bu);
				H = shl(H, bit);
				pq = add(pq, H);
			}
			Operand next = load(getelementptr(pxy, N + i));
			t = pack(next, t);
			t = zext(t, bu2);
			pq = zext(pq, bu2);
			t = add(t, pq);
			t = lshr(t, unit);
			t = trunc(t, bu);
			H = split(&t, t, bit);
		}
		Operand z;
		if (isFullBit) {
			p = zext(p, bu);
			t = zext(t, bu);
			Operand vc = sub(t, p);
			Operand c = trunc(lshr(vc, bit), 1);
			z = select(c, t, vc);
			z = trunc(z, bit);
		} else {
			Operand vc = sub(t, p);
			Operand c = trunc(lshr(vc, bit - 1), 1);
			z = select(c, t, vc);
		}
		storeN(z, pz);
		ret(Void);
		endFunc();
	}
	void gen_all()
	{
		gen_mcl_fp_addsubPre(true);
		gen_mcl_fp_addsubPre(false);
		gen_mcl_fp_shr1();
	}
	void gen_addsub()
	{
		gen_mcl_fp_add(true);
		gen_mcl_fp_add(false);
		gen_mcl_fp_sub(true);
		gen_mcl_fp_sub(false);
		gen_mcl_fpDbl_add();
		gen_mcl_fpDbl_sub();
	}
	void gen_mul()
	{
		gen_mulPv();
		gen_mcl_fp_mulUnitPre();
		gen_mcl_fpDbl_mulPre();
		gen_mcl_fpDbl_sqrPre();
		gen_mcl_fp_mont(true);
		gen_mcl_fp_mont(false);
		gen_mcl_fp_montRed(true);
		gen_mcl_fp_montRed(false);
	}
	void setBit(uint32_t bit)
	{
		this->bit = bit;
		N = bit / unit;
	}
	void setUnit(uint32_t unit)
	{
		this->unit = unit;
		unit2 = unit * 2;
		unitStr = cybozu::itoa(unit);
	}
	void gen(const StrSet& privateFuncList, uint32_t maxBitSize, const std::string& suf)
	{
		this->suf = suf;
		this->privateFuncList = &privateFuncList;
#ifdef FOR_WASM
		gen_mulUU();
#else
		gen_once();
#if 1
		int bitTbl[] = {
			192,
			224,
			256,
			384,
			512
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(bitTbl); i++) {
			uint32_t bit = bitTbl[i];
			if (unit == 64 && bit == 224) continue;
			setBit(bit);
			gen_mul();
			gen_all();
			gen_addsub();
		}
#else
		uint32_t end = ((maxBitSize + unit - 1) / unit);
		for (uint32_t n = 1; n <= end; n++) {
			setBit(n * unit);
			gen_mul();
			gen_all();
			gen_addsub();
		}
#endif
		if (unit == 64 && maxBitSize == 768) {
			for (uint32_t i = maxBitSize + unit * 2; i <= maxBitSize * 2; i += unit * 2) {
				setBit(i);
				gen_all();
			}
		}
#endif
	}
};

int main(int argc, char *argv[])
	try
{
	uint32_t unit;
	int llvmVer;
	bool wasm;
	std::string suf;
	std::string privateFile;
	cybozu::Option opt;
	opt.appendOpt(&unit, uint32_t(sizeof(void*)) * 8, "u", ": unit");
	opt.appendOpt(&llvmVer, 0x70, "ver", ": llvm version");
	opt.appendBoolOpt(&wasm, "wasm", ": for wasm");
	opt.appendOpt(&suf, "", "s", ": suffix of function name");
	opt.appendOpt(&privateFile, "", "f", ": private function list file");
	opt.appendHelp("h");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	StrSet privateFuncList;
	if (!privateFile.empty()) {
		std::ifstream ifs(privateFile.c_str(), std::ios::binary);
		std::string name;
		while (ifs >> name) {
			privateFuncList.insert(name);
		}
	}
	Code c;
	fprintf(stderr, "llvmVer=0x%02x\n", llvmVer);
	c.setLlvmVer(llvmVer);
	c.wasm = wasm;
	c.setUnit(unit);
	uint32_t maxBitSize = MCL_MAX_BIT_SIZE;
	c.gen(privateFuncList, maxBitSize, suf);
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
	return 1;
}
