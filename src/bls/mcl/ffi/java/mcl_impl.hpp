#include <mcl/bls12_381.hpp>
#include <stdint.h>
#include <sstream>

#if defined(__GNUC__) && !defined(__EMSCRIPTEN__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

void SystemInit(int curveType) throw(std::exception)
{
	if (curveType == MCL_SECP256K1) {
		mcl::initCurve<mcl::bn::G1, mcl::bn::Fr>(curveType);
		return;
	}
	mcl::CurveParam cp;
	switch (curveType) {
	case MCL_BN254: cp = mcl::BN254; break;
	case MCL_BN_SNARK1: cp = mcl::BN_SNARK1; break;
	case MCL_BLS12_381: cp = mcl::BLS12_381; break;
	default:
		throw std::runtime_error("bad curveType");
	}
	mcl::bn::initPairing(cp);
}

template<class T>
void deserializeT(T& x, const char *cbuf, size_t bufSize)
{
	if (x.deserialize(cbuf, bufSize) == 0) {
		throw std::runtime_error("deserialize");
	}
}

template<class T>
void setLittleEndianModT(T& x, const char *cbuf, size_t bufSize)
{
	x.setLittleEndianMod((const uint8_t*)cbuf, bufSize);
}

template<class T>
void setHashOfT(T& x, const char *cbuf, size_t bufSize)
{
	x.setHashOf(cbuf, bufSize);
}

template<class T>
void serializeT(std::string& out, const T& x)
{
	out.resize(48 * 12);
	size_t n = x.serialize(&out[0], out.size());
	if (n == 0) throw std::runtime_error("serializeT");
	out.resize(n);
}

class G1;
class G2;
class GT;
/*
	Fr = Z / rZ
*/
class Fr {
	mcl::bn::Fr self_;
	friend class G1;
	friend class G2;
	friend class GT;
	friend void neg(Fr& y, const Fr& x);
	friend void inv(Fr& y, const Fr& x);
	friend void add(Fr& z, const Fr& x, const Fr& y);
	friend void sub(Fr& z, const Fr& x, const Fr& y);
	friend void mul(Fr& z, const Fr& x, const Fr& y);
	friend void mul(G1& z, const G1& x, const Fr& y);
	friend void mul(G2& z, const G2& x, const Fr& y);
	friend void div(Fr& z, const Fr& x, const Fr& y);
	friend void pow(GT& z, const GT& x, const Fr& y);
public:
	Fr() {}
	Fr(const Fr& rhs) : self_(rhs.self_) {}
	Fr(int x) : self_(x) {}
	Fr(const std::string& str, int base = 0) throw(std::exception)
		: self_(str, base) {}
	bool equals(const Fr& rhs) const { return self_ == rhs.self_; }
	bool isZero() const { return self_.isZero(); }
	bool isOne() const { return self_.isOne(); }
	void setStr(const std::string& str, int base = 0) throw(std::exception)
	{
		self_.setStr(str, base);
	}
	void setInt(int x)
	{
		self_ = x;
	}
	void clear()
	{
		self_.clear();
	}
	void setByCSPRNG()
	{
		self_.setByCSPRNG();
	}
	std::string toString(int base = 0) const throw(std::exception)
	{
		return self_.getStr(base);
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		deserializeT(self_, cbuf, bufSize);
	}
	void setLittleEndianMod(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		setLittleEndianModT(self_, cbuf, bufSize);
	}
	void setHashOf(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		setHashOfT(self_, cbuf, bufSize);
	}
	void serialize(std::string& out) const throw(std::exception)
	{
		serializeT(out, self_);
	}
};

void neg(Fr& y, const Fr& x)
{
	mcl::bn::Fr::neg(y.self_, x.self_);
}

void inv(Fr& y, const Fr& x)
{
	mcl::bn::Fr::inv(y.self_, x.self_);
}

void add(Fr& z, const Fr& x, const Fr& y)
{
	mcl::bn::Fr::add(z.self_, x.self_, y.self_);
}

void sub(Fr& z, const Fr& x, const Fr& y)
{
	mcl::bn::Fr::sub(z.self_, x.self_, y.self_);
}

void mul(Fr& z, const Fr& x, const Fr& y)
{
	mcl::bn::Fr::mul(z.self_, x.self_, y.self_);
}

void div(Fr& z, const Fr& x, const Fr& y)
{
	mcl::bn::Fr::div(z.self_, x.self_, y.self_);
}

class Fp {
	mcl::bn::Fp self_;
	friend class G1;
	friend class G2;
	friend class GT;
	friend void neg(Fp& y, const Fp& x);
	friend void inv(Fp& y, const Fp& x);
	friend void add(Fp& z, const Fp& x, const Fp& y);
	friend void sub(Fp& z, const Fp& x, const Fp& y);
	friend void mul(Fp& z, const Fp& x, const Fp& y);
	friend void div(Fp& z, const Fp& x, const Fp& y);
public:
	Fp() {}
	Fp(const Fp& rhs) : self_(rhs.self_) {}
	Fp(int x) : self_(x) {}
	Fp(const std::string& str, int base = 0) throw(std::exception)
		: self_(str, base) {}
	bool equals(const Fp& rhs) const { return self_ == rhs.self_; }
	bool isZero() const { return self_.isZero(); }
	bool isOne() const { return self_.isOne(); }
	void setStr(const std::string& str, int base = 0) throw(std::exception)
	{
		self_.setStr(str, base);
	}
	void setInt(int x)
	{
		self_ = x;
	}
	void clear()
	{
		self_.clear();
	}
	void setByCSPRNG()
	{
		self_.setByCSPRNG();
	}
	std::string toString(int base = 0) const throw(std::exception)
	{
		return self_.getStr(base);
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		deserializeT(self_, cbuf, bufSize);
	}
	void serialize(std::string& out) const throw(std::exception)
	{
		serializeT(out, self_);
	}
};

void neg(Fp& y, const Fp& x)
{
	mcl::bn::Fp::neg(y.self_, x.self_);
}

void inv(Fp& y, const Fp& x)
{
	mcl::bn::Fp::inv(y.self_, x.self_);
}

void add(Fp& z, const Fp& x, const Fp& y)
{
	mcl::bn::Fp::add(z.self_, x.self_, y.self_);
}

void sub(Fp& z, const Fp& x, const Fp& y)
{
	mcl::bn::Fp::sub(z.self_, x.self_, y.self_);
}

void mul(Fp& z, const Fp& x, const Fp& y)
{
	mcl::bn::Fp::mul(z.self_, x.self_, y.self_);
}

void div(Fp& z, const Fp& x, const Fp& y)
{
	mcl::bn::Fp::div(z.self_, x.self_, y.self_);
}


/*
	#G1 = r
*/
class G1 {
	mcl::bn::G1 self_;
	friend void neg(G1& y, const G1& x);
	friend void dbl(G1& y, const G1& x);
	friend void add(G1& z, const G1& x, const G1& y);
	friend void sub(G1& z, const G1& x, const G1& y);
	friend void mul(G1& z, const G1& x, const Fr& y);
	friend void pairing(GT& e, const G1& P, const G2& Q);
	friend void hashAndMapToG1(G1& P, const char *cbuf, size_t bufSize) throw(std::exception);
public:
	G1() {}
	G1(const G1& rhs) : self_(rhs.self_) {}
	G1(const Fp& x, const Fp& y) throw(std::exception)
		: self_(x.self_, y.self_) { }
	bool equals(const G1& rhs) const { return self_ == rhs.self_; }
	bool isZero() const { return self_.isZero(); }
	bool isValidOrder() const { return self_.isValidOrder(); }
	void set(const Fp& x, const Fp& y) throw(std::exception)
	{
		self_.set(x.self_, y.self_);
	}
	void clear()
	{
		self_.clear();
	}
	/*
		compressed format
	*/
	void setStr(const std::string& str, int base = 0) throw(std::exception)
	{
		self_.setStr(str, base);
	}
	std::string toString(int base = 0) const throw(std::exception)
	{
		return self_.getStr(base);
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		deserializeT(self_, cbuf, bufSize);
	}
	void serialize(std::string& out) const throw(std::exception)
	{
		serializeT(out, self_);
	}
	void normalize()
	{
		self_.normalize();
	}
	void tryAndIncMapTo(const Fp& x)
	{
		mcl::ec::tryAndIncMapTo(self_, x.self_);
	}
	Fp getX() const
	{
		Fp ret;
		ret.self_ = self_.x;
		return ret;
	}
	Fp getY() const
	{
		Fp ret;
		ret.self_ = self_.y;
		return ret;
	}
	Fp getZ() const
	{
		Fp ret;
		ret.self_ = self_.z;
		return ret;
	}
};

void neg(G1& y, const G1& x)
{
	mcl::bn::G1::neg(y.self_, x.self_);
}
void dbl(G1& y, const G1& x)
{
	mcl::bn::G1::dbl(y.self_, x.self_);
}
void add(G1& z, const G1& x, const G1& y)
{
	mcl::bn::G1::add(z.self_, x.self_, y.self_);
}
void sub(G1& z, const G1& x, const G1& y)
{
	mcl::bn::G1::sub(z.self_, x.self_, y.self_);
}
void mul(G1& z, const G1& x, const Fr& y)
{
	mcl::bn::G1::mul(z.self_, x.self_, y.self_);
}

/*
	#G2 = r
*/
class G2 {
	mcl::bn::G2 self_;
	friend void neg(G2& y, const G2& x);
	friend void dbl(G2& y, const G2& x);
	friend void add(G2& z, const G2& x, const G2& y);
	friend void sub(G2& z, const G2& x, const G2& y);
	friend void mul(G2& z, const G2& x, const Fr& y);
	friend void pairing(GT& e, const G1& P, const G2& Q);
	friend void hashAndMapToG2(G2& P, const char *cbuf, size_t bufSize) throw(std::exception);
public:
	G2() {}
	G2(const G2& rhs) : self_(rhs.self_) {}
	G2(const Fp& ax, const Fp& ay, const Fp& bx, const Fp& by) throw(std::exception)
		: self_(mcl::bn::Fp2(ax.self_, ay.self_), mcl::bn::Fp2(bx.self_, by.self_))
	{
	}
	bool equals(const G2& rhs) const { return self_ == rhs.self_; }
	bool isZero() const { return self_.isZero(); }
	void set(const Fp& ax, const Fp& ay, const Fp& bx, const Fp& by) throw(std::exception)
	{
		self_.set(mcl::bn::Fp2(ax.self_, ay.self_), mcl::bn::Fp2(bx.self_, by.self_));
	}
	void clear()
	{
		self_.clear();
	}
	/*
		compressed format
	*/
	void setStr(const std::string& str, int base = 0) throw(std::exception)
	{
		self_.setStr(str, base);
	}
	std::string toString(int base = 0) const throw(std::exception)
	{
		return self_.getStr(base);
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		deserializeT(self_, cbuf, bufSize);
	}
	void serialize(std::string& out) const throw(std::exception)
	{
		serializeT(out, self_);
	}
	void normalize()
	{
		self_.normalize();
	}
};

void neg(G2& y, const G2& x)
{
	mcl::bn::G2::neg(y.self_, x.self_);
}
void dbl(G2& y, const G2& x)
{
	mcl::bn::G2::dbl(y.self_, x.self_);
}
void add(G2& z, const G2& x, const G2& y)
{
	mcl::bn::G2::add(z.self_, x.self_, y.self_);
}
void sub(G2& z, const G2& x, const G2& y)
{
	mcl::bn::G2::sub(z.self_, x.self_, y.self_);
}
void mul(G2& z, const G2& x, const Fr& y)
{
	mcl::bn::G2::mul(z.self_, x.self_, y.self_);
}

/*
	#GT = r
*/
class GT {
	mcl::bn::Fp12 self_;
	friend void mul(GT& z, const GT& x, const GT& y);
	friend void inv(GT& y, GT& x);
	friend void pow(GT& z, const GT& x, const Fr& y);
	friend void pairing(GT& e, const G1& P, const G2& Q);
public:
	GT() {}
	GT(const GT& rhs) : self_(rhs.self_) {}
	bool equals(const GT& rhs) const { return self_ == rhs.self_; }
	bool isOne() const { return self_.isOne(); }
	void clear()
	{
		self_.clear();
	}
	void setStr(const std::string& str, int base = 0) throw(std::exception)
	{
		self_.setStr(str, base);
	}
	std::string toString(int base = 0) const throw(std::exception)
	{
		return self_.getStr(base);
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		deserializeT(self_, cbuf, bufSize);
	}
	void serialize(std::string& out) const throw(std::exception)
	{
		serializeT(out, self_);
	}
};

void mul(GT& z, const GT& x, const GT& y)
{
	mcl::bn::Fp12::mul(z.self_, x.self_, y.self_);
}
void inv(GT& y, GT& x)
{
	mcl::bn::Fp12::unitaryInv(y.self_, x.self_);
}
void pow(GT& z, const GT& x, const Fr& y)
{
	mcl::bn::Fp12::pow(z.self_, x.self_, y.self_);
}
void pairing(GT& e, const G1& P, const G2& Q)
{
	mcl::bn::pairing(e.self_, P.self_, Q.self_);
}

void hashAndMapToG1(G1& P, const char *cbuf, size_t bufSize) throw(std::exception)
{
	mcl::bn::hashAndMapToG1(P.self_, cbuf, bufSize);
}

void hashAndMapToG2(G2& P, const char *cbuf, size_t bufSize) throw(std::exception)
{
	mcl::bn::hashAndMapToG2(P.self_, cbuf, bufSize);
}

void verifyOrderG1(bool doVerify)
{
    mcl::bn::verifyOrderG1(doVerify);
}

void verifyOrderG2(bool doVerify)
{
    mcl::bn::verifyOrderG2(doVerify);
}

#if defined(__GNUC__) && !defined(__EMSCRIPTEN__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
