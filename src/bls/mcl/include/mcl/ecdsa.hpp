#pragma once
/**
	@file
	@brief ECDSA
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mcl/fp.hpp>
#include <mcl/ec.hpp>
#include <mcl/ecparam.hpp>
#include <mcl/window_method.hpp>

namespace mcl { namespace ecdsa {

enum {
	SerializeOld = 0,
	SerializeBitcoin = 1
};

namespace local {

#ifndef MCLSHE_WIN_SIZE
	#define MCLSHE_WIN_SIZE 10
#endif
static const size_t winSize = MCLSHE_WIN_SIZE;

struct FpTag;
struct ZnTag;

} // mcl::ecdsa::local

typedef mcl::FpT<local::FpTag, 256> Fp;
typedef mcl::FpT<local::ZnTag, 256> Zn;
typedef mcl::EcT<Fp> Ec;

namespace local {

struct Param {
	Ec P;
	mcl::fp::WindowMethod<Ec> Pbase;
	size_t bitSize;
	int serializeMode;
};

inline Param& getParam()
{
	static Param p;
	return p;
}

/*
	y = x mod n
*/
inline void FpToZn(Zn& y, const Fp& x)
{
	fp::Block b;
	x.getBlock(b);
	bool ret;
	y.setArrayMod(&ret, b.p, b.n);
	assert(ret);
	(void)ret;
}

inline void setHashOf(Zn& x, const void *msg, size_t msgSize)
{
	const size_t mdSize = 32;
	uint8_t md[mdSize];
	mcl::fp::sha256(md, mdSize, msg, msgSize);
	bool b;
	x.setBigEndianMod(&b, md, mdSize);
	assert(b);
	(void)b;
}

template<class InputStream>
bool readByte(uint8_t& v, InputStream& is)
{
	return cybozu::readSome(&v, 1, is) == 1;
}

// is[0] : 0x02
// is[1] : size
// is[2..2+size) : big endian
// return readSize
// return 0 if error
template<class InputStream>
size_t loadBigEndian(Zn& x, InputStream& is)
{
	uint8_t n;
	if (!readByte(n, is) || n != 0x02) return 0;
	if (!readByte(n, is)) return 0;
	uint8_t buf[33];
	if (n == 0 || n > sizeof(buf)) return 0;
	if (cybozu::readSome(buf, n, is) != n) return 0;
	// negative
	if ((buf[0] & 0x80) != 0) return 0;
	// unnecessary zero
	if (buf[0] == 0 && n > 1 && ((buf[1] & 0x80) == 0)) return 0;
	size_t adj = n > 1 && buf[0] == 0;
	bool b;
	fp::local::byteSwap(buf + adj, n - adj);
	x.setArray(&b, buf + adj, n - adj);
	if (!b) return 0;
	return n + 2;
}

inline size_t getBigEndian(uint8_t *buf, size_t bufSize, const Zn& x)
{
	size_t n = x.getLittleEndian(buf, bufSize);
	if (n == 0) return 0;
	mcl::fp::local::byteSwap(buf, n);
	return n;
}

} // mcl::ecdsa::local

const local::Param& param = local::getParam();

inline void init(bool *pb)
{
	local::Param& p = local::getParam();
	mcl::initCurve<Ec, Zn>(pb, MCL_SECP256K1, &p.P);
	if (!*pb) return;
	p.bitSize = 256;
	p.Pbase.init(pb, p.P, p.bitSize, local::winSize);
	// isValid() checks the order
	Ec::setOrder(Zn::getOp().mp);
	Fp::setETHserialization(true);
	Zn::setETHserialization(true);
	p.serializeMode = SerializeBitcoin;
//	Ec::setIoMode(mcl::IoEcAffineSerialize);
}

inline int setSeriailzeMode(int mode)
{
	local::getParam().serializeMode = mode;
	return 0;
}

#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void init()
{
	bool b;
	init(&b);
	if (!b) throw cybozu::Exception("ecdsa:init");
}
#endif

typedef Zn SecretKey;
struct PublicKey : mcl::fp::Serializable<PublicKey, Ec> {
private:
	template<class InputStream>
	void loadUncompressed(bool *pb, InputStream& is)
	{
		x.load(pb, is, IoSerialize); if (!*pb) return;
		y.load(pb, is, IoSerialize); if (!*pb) return;
		this->z = 1;
		*pb = isValid();
	}
	template<class OutputStream>
	void saveUncompressed(bool *pb, OutputStream& os) const
	{
		Ec P(*this);
		P.normalize();
		P.x.save(pb, os, IoSerialize);
		if (!*pb) return;
		P.y.save(pb, os, IoSerialize);
	}
	// 0x02 : y is even
	// 0x03 : y is odd
	template<class InputStream>
	void loadCompressed(bool *pb, InputStream& is, uint8_t header)
	{
		x.load(pb, is, IoSerialize);
		if (!*pb) return;
		bool odd = header == 0x03;
		*pb = Ec::getYfromX(y, x, odd);
		if (*pb) {
			z = 1;
		}
	}
public:
	template<class InputStream>
	void load(bool *pb, InputStream& is, int = IoSerialize)
	{
		switch (param.serializeMode) {
		case SerializeOld:
			loadUncompressed(pb, is);
			return;
		case SerializeBitcoin:
		default:
			{
				uint8_t header;
				*pb = local::readByte(header, is);
				if (!*pb) return;
				switch (header) {
				case 0x04: // uncompressed
					loadUncompressed(pb, is);
					return;
				case 0x02:
				case 0x03:
					loadCompressed(pb, is, header);
					return;
				default:
					*pb = false;
					return;
				}
			}
			return;
		}
	}
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int = IoSerialize) const
	{
		switch (param.serializeMode) {
		case SerializeOld:
			saveUncompressed(pb, os);
			return;
		case SerializeBitcoin:
		default:
			cybozu::writeChar(pb, os, 0x04);
			if (!*pb) return;
			saveUncompressed(pb, os);
			return;
		}
	}
	template<class OutputStream>
	void saveCompressed(bool *pb, OutputStream& os) const
	{
		if (isZero()) {
			*pb = false;
			return;
		}
		Ec P(*this);
		P.normalize();
		uint8_t header = y.isOdd() ? 0x03 : 0x02;
		cybozu::writeChar(pb, os, header);
		if (!*pb) return;
		x.save(pb, os, IoSerialize);
	}
	// return write bytes
	size_t serializeCompressed(void *buf, size_t maxBufSize, int = IoSerialize) const
	{
		cybozu::MemoryOutputStream os(buf, maxBufSize);
		bool b;
		saveCompressed(&b, os);
		return b ? os.getPos() : 0;
	}
};

struct PrecomputedPublicKey {
	mcl::fp::WindowMethod<Ec> pubBase_;
	void init(bool *pb, const PublicKey& pub)
	{
		pubBase_.init(pb, pub, param.bitSize, local::winSize);
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void init(const PublicKey& pub)
	{
		bool b;
		init(&b, pub);
		if (!b) throw cybozu::Exception("ecdsa:PrecomputedPublicKey:init");
	}
#endif
};

inline void getPublicKey(PublicKey& pub, const SecretKey& sec)
{
	Ec::mul(pub, param.P, sec);
	pub.normalize();
}

/*
  serialize/deserialize DER format
  https://www.oreilly.com/library/view/programming-bitcoin/9781492031482/ch04.html
*/
struct Signature : public mcl::fp::Serializable<Signature> {
	Zn r, s;
	template<class OutputStream>
	bool writeByte(OutputStream& os, uint8_t v) const
	{
		bool b;
		cybozu::writeChar(&b, os, v);
		return b;
	}
	template<class InputStream>
	void load(bool *pb, InputStream& is, int ioMode = IoSerialize)
	{
		switch (param.serializeMode) {
		case SerializeOld:
			r.load(pb, is, ioMode); if (!*pb) return;
			s.load(pb, is, ioMode);
			return;
		case SerializeBitcoin:
		default:
			{
				(void)ioMode;
				*pb = false;
				uint8_t len;
				if (!local::readByte(len, is) || len != 0x30) return;
				if (!local::readByte(len, is)) return;
				if (len >= 0x80) return;
				size_t rn = local::loadBigEndian(r, is);
				if (rn == 0) return;
				size_t sn = local::loadBigEndian(s, is);
				if (sn == 0) return;
				*pb = len == rn + sn;
			}
			return;
		}
	}
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int ioMode = IoSerialize) const
	{
		switch (param.serializeMode) {
		case SerializeOld:
			{
				const char sep = *fp::getIoSeparator(ioMode);
				r.save(pb, os, ioMode); if (!*pb) return;
				if (sep) {
					cybozu::writeChar(pb, os, sep);
					if (!*pb) return;
				}
				s.save(pb, os, ioMode);
			}
			return;
		case SerializeBitcoin:
		default:
			{
				(void)ioMode;
				*pb = false;
				const size_t bufSize = 32;
				uint8_t rBuf[bufSize], sBuf[bufSize];
				size_t rn, sn;
				rn = local::getBigEndian(rBuf, bufSize, r);
				if (rn == 0) return;
				sn = local::getBigEndian(sBuf, bufSize, s);
				if (sn == 0) return;
				bool rAdj = rBuf[0] >= 0x80;
				bool sAdj = sBuf[0] >= 0x80;
				size_t n = 4 + rn + sn + rAdj + sAdj;
				if (!writeByte(os, 0x30)) return;
				if (!writeByte(os, n)) return;
				if (!writeByte(os, 0x02)) return;
				if (!writeByte(os, rn + rAdj)) return;
				if (rAdj && !writeByte(os, 0)) return;
				cybozu::write(pb, os, rBuf, rn); if (!*pb) return;
				if (!writeByte(os, 0x02)) return;
				if (!writeByte(os, sn + sAdj)) return;
				if (sAdj && !writeByte(os, 0)) return;
				cybozu::write(pb, os, sBuf, sn);
			}
			return;
		}
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	template<class InputStream>
	void load(InputStream& is, int ioMode = IoSerialize)
	{
		bool b;
		load(&b, is, ioMode);
		if (!b) throw cybozu::Exception("ecdsa:Signature:load");
	}
	template<class OutputStream>
	void save(OutputStream& os, int ioMode = IoSerialize) const
	{
		bool b;
		save(&b, os, ioMode);
		if (!b) throw cybozu::Exception("ecdsa:Signature:save");
	}
#endif
#ifndef CYBOZU_DONT_USE_STRING
	friend std::istream& operator>>(std::istream& is, Signature& self)
	{
		self.load(is, fp::detectIoMode(Ec::getIoMode(), is));
		return is;
	}
	friend std::ostream& operator<<(std::ostream& os, const Signature& self)
	{
		self.save(os, fp::detectIoMode(Ec::getIoMode(), os));
		return os;
	}
#endif
};

// normalize a signature to lower S signature (r, s) if s < half else (r, -s)
inline void normalizeSignature(Signature& sig)
{
	if (sig.s.isNegative()) {
		Zn::neg(sig.s, sig.s);
	}
}

inline void sign(Signature& sig, const SecretKey& sec, const void *msg, size_t msgSize)
{
	Zn& r = sig.r;
	Zn& s = sig.s;
	Zn z, k;
	local::setHashOf(z, msg, msgSize);
	Ec Q;
	for (;;) {
		bool b;
		k.setByCSPRNG(&b);
		(void)b;
		param.Pbase.mul(Q, k);
		if (Q.isZero()) continue;
		Q.normalize();
		local::FpToZn(r, Q.x);
		if (r.isZero()) continue;
		Zn::mul(s, r, sec);
		s += z;
		if (s.isZero()) continue;
		s /= k;
		normalizeSignature(sig);
		return;
	}
}

namespace local {

inline void mulDispatch(Ec& Q, const PublicKey& pub, const Zn& y)
{
	Ec::mul(Q, pub, y);
}

inline void mulDispatch(Ec& Q, const PrecomputedPublicKey& ppub, const Zn& y)
{
	ppub.pubBase_.mul(Q, y);
}

// accept only lower S signature
template<class Pub>
inline bool verify(const Signature& sig, const Pub& pub, const void *msg, size_t msgSize)
{
	const Zn& r = sig.r;
	const Zn& s = sig.s;
	if (r.isZero() || s.isZero()) return false;
	if (s.isNegative()) return false;
	Zn z, w, u1, u2;
	local::setHashOf(z, msg, msgSize);
	Zn::inv(w, s);
	Zn::mul(u1, z, w);
	Zn::mul(u2, r, w);
	Ec Q1, Q2;
	param.Pbase.mul(Q1, u1);
//	Ec::mul(Q2, pub, u2);
	local::mulDispatch(Q2, pub, u2);
	Q1 += Q2;
	if (Q1.isZero()) return false;
	Q1.normalize();
	Zn x;
	local::FpToZn(x, Q1.x);
	return r == x;
}

} // mcl::ecdsa::local

inline bool verify(const Signature& sig, const PublicKey& pub, const void *msg, size_t msgSize)
{
	return local::verify(sig, pub, msg, msgSize);
}

inline bool verify(const Signature& sig, const PrecomputedPublicKey& ppub, const void *msg, size_t msgSize)
{
	return local::verify(sig, ppub, msg, msgSize);
}

} } // mcl::ecdsa

