#pragma once
/**
	@file
	@brief LLVM IR generator
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
//#define CYBOZU_EXCEPTION_WITH_STACKTRACE
#include <string>
#include <vector>
#include <cybozu/exception.hpp>
#include <cybozu/itoa.hpp>
#include <stdio.h>
#ifdef _MSC_VER
//	#pragma warning(push)
	#pragma warning(disable : 4458)
#endif

namespace mcl {

namespace impl {

struct File {
	FILE *fp;
	File() : fp(stdout) {}
	~File() { if (fp != stdout) fclose(fp); }
	void open(const std::string& file)
	{
#ifdef _MSC_VER
		bool isOK = fopen_s(&fp, file.c_str(), "wb") != 0;
#else
		fp = fopen(file.c_str(), "wb");
		bool isOK = fp != NULL;
#endif
		if (!isOK) throw cybozu::Exception("File:open") << file;
	}
	void write(const std::string& str)
	{
		int ret = fprintf(fp, "%s\n", str.c_str());
		if (ret < 0) {
			throw cybozu::Exception("File:write") << str;
		}
	}
};
template<size_t dummy=0>
struct Param {
	static File f;
	static int llvmVer;
};

template<size_t dummy>
File Param<dummy>::f;

} // mcl::impl


struct Generator {
	static const uint8_t None = 0;
	static const uint8_t Int = 1;
	static const uint8_t Imm = 2;
	static const uint8_t Ptr = 1 << 7;
	static const uint8_t IntPtr = Int | Ptr;
	int llvmVer;
	void setOldLLVM() { llvmVer = 0x37; }
	static const int V8 = 0x90;
	bool isNewer(int ver) const
	{
		return llvmVer > ver;
	}
	void setLlvmVer(int ver) { llvmVer = ver; }
	struct Type {
		uint8_t type;
		bool isPtr;
		Type(int type = 0)
			: type(static_cast<uint8_t>(type & ~Ptr))
			, isPtr((type & Ptr) != 0)
		{
		}
		inline friend std::ostream& operator<<(std::ostream& os, const Type& self)
		{
			return os << (self.type | (self.isPtr ? Ptr : 0));
		}
	};
	enum CondType {
		eq = 1,
		neq = 2,
		ugt = 3,
		uge = 4,
		ult = 5,
		ule = 6,
		sgt = 7,
		sge = 8,
		slt = 9,
		sle = 10
	};
	static inline const std::string& toStr(CondType type)
	{
		static const std::string tbl[] = {
			"eq", "neq", "ugt", "uge", "ult", "ule", "sgt", "sge", "slt", "sle"
		};
		return tbl[type - 1];
	}
	void open(const std::string& file)
	{
		impl::Param<>::f.open(file);
	}
	struct Operand;
	struct Function;
	struct Eval;
	struct Label {
		std::string name;
		explicit Label(const std::string& name = "") : name(name) {}
		std::string toStr() const { return std::string("label %") + name; }
	};
	void putLabel(const Label& label)
	{
		put(label.name + ":");
	}
	static inline int& getGlobalIdx()
	{
		static int globalIdx = 0;
		return ++globalIdx;
	}
	static inline void resetGlobalIdx()
	{
		getGlobalIdx() = 0;
	}
	static inline void put(const std::string& str)
	{
		impl::Param<>::f.write(str);
	}
	void beginFunc(const Function& f);
	void endFunc()
	{
		put("}");
	}
	Eval zext(const Operand& x, uint32_t size);
	Eval mul(const Operand& x, const Operand& y);
	Eval add(const Operand& x, const Operand& y);
	Eval sub(const Operand& x, const Operand& y);
	Eval _and(const Operand& x, const Operand& y);
	Eval _or(const Operand& x, const Operand& y);
	void ret(const Operand& r);
	Eval lshr(const Operand& x, uint32_t size);
	Eval ashr(const Operand& x, uint32_t size);
	Eval shl(const Operand& x, uint32_t size);
	Eval trunc(const Operand& x, uint32_t size);
	Eval getelementptr(const Operand& p, const Operand& i);
	Eval getelementptr(const Operand& p, int n);
	Eval load(const Operand& p);
	void store(const Operand& r, const Operand& p);
	Eval select(const Operand& c, const Operand& r1, const Operand& r2);
	Eval alloca_(uint32_t bit, uint32_t n);
	// QQQ : type of type must be Type
	Eval bitcast(const Operand& r, const Operand& type);
	Eval icmp(CondType type, const Operand& r1, const Operand& r2);
	void br(const Operand& op, const Label& ifTrue, const Label& ifFalse);
	Eval call(const Function& f);
	Eval call(const Function& f, const Operand& op1);
	Eval call(const Function& f, const Operand& op1, const Operand& op2);
	Eval call(const Function& f, const Operand& op1, const Operand& op2, const Operand& op3);
	Eval call(const Function& f, const Operand& op1, const Operand& op2, const Operand& op3, const Operand& op4);
	Eval call(const Function& f, const Operand& op1, const Operand& op2, const Operand& op3, const Operand& op4, const Operand& op5);

	Operand makeImm(uint32_t bit, int64_t imm);
};

struct Generator::Operand {
	Type type;
	uint32_t bit;
	int64_t imm;
	uint32_t idx;
	Operand() : type(None), bit(0), imm(0), idx(0) {}
	Operand(Type type, uint32_t bit)
		: type(type), bit(bit), imm(0), idx(getGlobalIdx())
	{
	}
	Operand(const Operand& rhs)
		: type(rhs.type), bit(rhs.bit), imm(rhs.imm), idx(rhs.idx)
	{
	}
	void operator=(const Operand& rhs)
	{
		type = rhs.type;
		bit = rhs.bit;
		imm = rhs.imm;
		idx = rhs.idx;
	}
	void update()
	{
		idx = getGlobalIdx();
	}
	Operand(const Eval& e);
	void operator=(const Eval& e);

	std::string toStr(bool isAlias = true) const
	{
		if (type.isPtr) {
			return getType(isAlias) + " " + getName();
		}
		switch (type.type) {
		default:
			return getType();
		case Int:
		case Imm:
			return getType() + " " + getName();
		}
	}
	std::string getType(bool isAlias = true) const
	{
		std::string s;
		switch (type.type) {
		default:
			return "void";
		case Int:
		case Imm:
			s = std::string("i") + cybozu::itoa(bit);
			break;
		}
		if (type.isPtr) {
			s += "*";
			if (!isAlias) {
				s += " noalias ";
			}
		}
		return s;
	}
	std::string getName() const
	{
		switch (type.type) {
		default:
			return "";
		case Int:
			return std::string("%r") + cybozu::itoa(idx);
		case Imm:
			return cybozu::itoa(imm);
		}
	}
};

inline Generator::Operand Generator::makeImm(uint32_t bit, int64_t imm)
{
	Generator::Operand v(Generator::Imm, bit);
	v.imm = imm;
	return v;
}

struct Generator::Eval {
	std::string s;
	Generator::Operand op;
	mutable bool used;
	Eval() : used(false) {}
	~Eval()
	{
		if (used) return;
		put(s);
	}
};

inline Generator::Operand::Operand(const Generator::Eval& e)
{
	*this = e.op;
	update();
	put(getName() + " = " + e.s);
	e.used = true;
}

inline void Generator::Operand::operator=(const Generator::Eval& e)
{
	*this = e.op;
	update();
	put(getName() + " = " + e.s);
	e.used = true;
}

struct Generator::Function {
	typedef std::vector<Generator::Operand> OperandVec;
	std::string name;
	Generator::Operand ret;
	OperandVec opv;
	bool isPrivate;
	bool isAlias;
	void clear()
	{
		isPrivate = false;
		isAlias = false;
	}
	explicit Function(const std::string& name = "") : name(name) { clear(); }
	Function(const std::string& name, const Operand& ret)
		: name(name), ret(ret)
	{
		clear();
	}
	Function(const std::string& name, const Operand& ret, const Operand& op1)
		: name(name), ret(ret)
	{
		clear();
		opv.push_back(op1);
	}
	Function(const std::string& name, const Operand& ret, const Operand& op1, const Operand& op2)
		: name(name), ret(ret)
	{
		clear();
		opv.push_back(op1);
		opv.push_back(op2);
	}
	Function(const std::string& name, const Operand& ret, const Operand& op1, const Operand& op2, const Operand& op3)
		: name(name), ret(ret)
	{
		clear();
		opv.push_back(op1);
		opv.push_back(op2);
		opv.push_back(op3);
	}
	Function(const std::string& name, const Operand& ret, const Operand& op1, const Operand& op2, const Operand& op3, const Operand& op4)
		: name(name), ret(ret)
	{
		clear();
		opv.push_back(op1);
		opv.push_back(op2);
		opv.push_back(op3);
		opv.push_back(op4);
	}
	Function(const std::string& name, const Operand& ret, const Operand& op1, const Operand& op2, const Operand& op3, const Operand& op4, const Operand& op5)
		: name(name), ret(ret)
	{
		clear();
		opv.push_back(op1);
		opv.push_back(op2);
		opv.push_back(op3);
		opv.push_back(op4);
		opv.push_back(op5);
	}
	void setPrivate()
	{
		isPrivate = true;
	}
	void setAlias()
	{
		isAlias = true;
	}
	std::string toStr() const
	{
		std::string str = std::string("define ");
		if (isPrivate) {
			str += "private ";
		}
		str += ret.getType();
		str += " @" + name + "(";
		for (size_t i = 0; i < opv.size(); i++) {
			if (i > 0) str += ", ";
			str += opv[i].toStr(isAlias);
		}
		str += ")";
		return str;
	}
};

namespace impl {

inline Generator::Eval callSub(const Generator::Function& f, const Generator::Operand **opTbl, size_t opNum)
{
	if (f.opv.size() != opNum) throw cybozu::Exception("impl:callSub:bad num of arg") << f.opv.size() << opNum;
	if (f.name.empty()) throw cybozu::Exception("impl:callSub:no name");
	Generator::Eval e;
	e.op = f.ret;
	e.s = "call ";
	e.s += f.ret.getType();
	e.s += " @" + f.name + "(";
	for (size_t i = 0; i < opNum; i++) {
		if (i > 0) {
			e.s += ", ";
		}
		e.s += opTbl[i]->toStr();
	}
	e.s += ")";
	return e;
}

inline Generator::Eval aluSub(const char *name, const Generator::Operand& x, const Generator::Operand& y)
{
	if (x.bit != y.bit) throw cybozu::Exception("Generator:aluSub:bad size") << name << x.bit << y.bit;
	Generator::Eval e;
	e.op.type = Generator::Int;
	e.op.bit = x.bit;
	e.s = name;
	e.s += " ";
	e.s += x.toStr() + ", " + y.getName();
	return e;
}

inline Generator::Eval shiftSub(const char *name, const Generator::Operand& x, uint32_t size)
{
	Generator::Eval e;
	e.op = x;
	e.s = name;
	e.s += " ";
	e.s += x.toStr() + ", " + cybozu::itoa(size);
	return e;
}

} // mcl::impl

inline void Generator::beginFunc(const Generator::Function& f)
{
	put(f.toStr() + "\n{");
}

inline Generator::Eval Generator::zext(const Generator::Operand& x, uint32_t size)
{
	if (x.bit >= size) throw cybozu::Exception("Generator:zext:bad size") << x.bit << size;
	Eval e;
	e.op = x;
	e.op.bit = size;
	e.s = "zext ";
	e.s += x.toStr() + " to i" + cybozu::itoa(size);
	return e;
}

inline Generator::Eval Generator::mul(const Generator::Operand& x, const Generator::Operand& y)
{
	return impl::aluSub("mul", x, y);
}

inline Generator::Eval Generator::add(const Generator::Operand& x, const Generator::Operand& y)
{
	return impl::aluSub("add", x, y);
}

inline Generator::Eval Generator::sub(const Generator::Operand& x, const Generator::Operand& y)
{
	return impl::aluSub("sub", x, y);
}

inline Generator::Eval Generator::_and(const Generator::Operand& x, const Generator::Operand& y)
{
	return impl::aluSub("and", x, y);
}

inline Generator::Eval Generator::_or(const Generator::Operand& x, const Generator::Operand& y)
{
	return impl::aluSub("or", x, y);
}

inline void Generator::ret(const Generator::Operand& x)
{
	std::string s = "ret " + x.toStr();
	put(s);
}

inline Generator::Eval Generator::lshr(const Generator::Operand& x, uint32_t size)
{
	return impl::shiftSub("lshr", x, size);
}

inline Generator::Eval Generator::ashr(const Generator::Operand& x, uint32_t size)
{
	return impl::shiftSub("ashr", x, size);
}

inline Generator::Eval Generator::shl(const Generator::Operand& x, uint32_t size)
{
	return impl::shiftSub("shl", x, size);
}

inline Generator::Eval Generator::trunc(const Generator::Operand& x, uint32_t size)
{
	Eval e;
	e.op = x;
	e.op.bit = size;
	e.s = "trunc ";
	e.s += x.toStr() + " to i" + cybozu::itoa(size);
	return e;
}

inline Generator::Eval Generator::getelementptr(const Generator::Operand& p, const Generator::Operand& i)
{
	Eval e;
	e.op = p;
	e.s = "getelementptr ";
	const std::string bit = cybozu::itoa(p.bit);
	if (isNewer(V8)) {
		e.s += " inbounds " + bit + ", ";
	}
	e.s += "i" + bit + ", ";
	e.s += p.toStr() + ", " + i.toStr();
	return e;
}

inline Generator::Eval Generator::getelementptr(const Generator::Operand& p, int n)
{
	return Generator::getelementptr(p, makeImm(32, n));
}

inline Generator::Eval Generator::load(const Generator::Operand& p)
{
	if (!p.type.isPtr) throw cybozu::Exception("Generator:load:not pointer") << p.type;
	Eval e;
	e.op = p;
	e.op.type.isPtr = false;
	e.s = "load ";
	const std::string bit = cybozu::itoa(p.bit);
	if (isNewer(V8)) {
		e.s += "i" + bit + ", ";
	}
	e.s += "i" + bit + ", ";
	e.s += p.toStr();
	return e;
}

inline void Generator::store(const Generator::Operand& r, const Generator::Operand& p)
{
	if (!p.type.isPtr) throw cybozu::Exception("Generator:store:not pointer") << p.type;
	std::string s = "store ";
	s += r.toStr();
	s += ", ";
	s += p.toStr();
	put(s);
}

inline Generator::Eval Generator::select(const Generator::Operand& c, const Generator::Operand& r1, const Generator::Operand& r2)
{
	if (c.bit != 1) throw cybozu::Exception("Generator:select:bad bit") << c.bit;
	Eval e;
	e.op = r1;
	e.s = "select ";
	e.s += c.toStr();
	e.s += ", ";
	e.s += r1.toStr();
	e.s += ", ";
	e.s += r2.toStr();
	return e;
}

inline Generator::Eval Generator::alloca_(uint32_t bit, uint32_t n)
{
	Eval e;
	e.op = Operand(IntPtr, bit);
	e.s = "alloca i";
	e.s += cybozu::itoa(bit);
	e.s += ", i32 ";
	e.s += cybozu::itoa(n);
	return e;
}

inline Generator::Eval Generator::bitcast(const Generator::Operand& r, const Generator::Operand& type)
{
	Eval e;
	e.op = type;
	e.s = "bitcast ";
	e.s += r.toStr();
	e.s += " to ";
	e.s += type.getType();
	return e;
}

inline Generator::Eval Generator::icmp(Generator::CondType type, const Generator::Operand& r1, const Generator::Operand& r2)
{
	Eval e;
	e.op.type = Int;
	e.op.bit = 1;
	e.s = "icmp ";
	e.s += toStr(type);
	e.s += " ";
	e.s += r1.toStr();
	e.s += ", ";
	e.s += r2.getName();
	return e;
}

inline void Generator::br(const Generator::Operand& op, const Generator::Label& ifTrue, const Generator::Label& ifFalse)
{
	if (op.bit != 1) throw cybozu::Exception("Generator:br:bad reg size") << op.bit;
	std::string s = "br i1";
	s += op.getName();
	s += ", ";
	s += ifTrue.toStr();
	s += ", ";
	s += ifFalse.toStr();
	put(s);
}

inline Generator::Eval Generator::call(const Generator::Function& f)
{
	return impl::callSub(f, 0, 0);
}

inline Generator::Eval Generator::call(const Generator::Function& f, const Generator::Operand& op1)
{
	const Operand *tbl[] = { &op1 };
	return impl::callSub(f, tbl, CYBOZU_NUM_OF_ARRAY(tbl));
}

inline Generator::Eval Generator::call(const Generator::Function& f, const Generator::Operand& op1, const Generator::Operand& op2)
{
	const Operand *tbl[] = { &op1, &op2 };
	return impl::callSub(f, tbl, CYBOZU_NUM_OF_ARRAY(tbl));
}

inline Generator::Eval Generator::call(const Generator::Function& f, const Generator::Operand& op1, const Generator::Operand& op2, const Generator::Operand& op3)
{
	const Operand *tbl[] = { &op1, &op2, &op3 };
	return impl::callSub(f, tbl, CYBOZU_NUM_OF_ARRAY(tbl));
}

inline Generator::Eval Generator::call(const Generator::Function& f, const Generator::Operand& op1, const Generator::Operand& op2, const Generator::Operand& op3, const Generator::Operand& op4)
{
	const Operand *tbl[] = { &op1, &op2, &op3, &op4 };
	return impl::callSub(f, tbl, CYBOZU_NUM_OF_ARRAY(tbl));
}

inline Generator::Eval Generator::call(const Generator::Function& f, const Generator::Operand& op1, const Generator::Operand& op2, const Generator::Operand& op3, const Generator::Operand& op4, const Generator::Operand& opt5)
{
	const Operand *tbl[] = { &op1, &op2, &op3, &op4, &opt5 };
	return impl::callSub(f, tbl, CYBOZU_NUM_OF_ARRAY(tbl));
}

#define MCL_GEN_FUNCTION(name, ...) Function name(#name, __VA_ARGS__)

} // mcl

#ifdef _MSC_VER
//	#pragma warning(pop)
#endif
