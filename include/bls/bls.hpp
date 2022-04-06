#pragma once
/**
	@file
	@brief BLS threshold signature on BN curve
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <bls/bls.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <iosfwd>
#include <stdint.h>
#include <memory.h>

namespace bls {

// same value with IoMode of mcl/op.hpp
enum {
	IoBin = 2, // binary number
	IoDec = 10, // decimal number
	IoHex = 16, // hexadecimal number
	IoPrefix = 128, // append '0b'(bin) or '0x'(hex)
	IoSerialize = 512,
	IoFixedByteSeq = IoSerialize // fixed byte representation
};

/*
	BLS signature
	e : G2 x G1 -> Fp12
	Q in G2 ; fixed global parameter
	H : {str} -> G1
	s : secret key
	sQ ; public key
	s H(m) ; signature of m
	verify ; e(sQ, H(m)) = e(Q, s H(m))
*/

/*
	initialize this library
	call this once before using the other method
	@param curve [in] type of curve
	@param compiledTimevar [in] use the default value
	@note init() is not thread safe
*/
inline void init(int curve = mclBn_CurveFp254BNb, int compiledTimeVar = MCLBN_COMPILED_TIME_VAR)
{
	if (blsInit(curve, compiledTimeVar) != 0) throw std::invalid_argument("blsInit");
}
inline size_t getOpUnitSize() { return blsGetOpUnitSize(); }

inline void getCurveOrder(std::string& str)
{
	str.resize(1024);
	mclSize n = blsGetCurveOrder(&str[0], str.size());
	if (n == 0) throw std::runtime_error("blsGetCurveOrder");
	str.resize(n);
}
inline void getFieldOrder(std::string& str)
{
	str.resize(1024);
	mclSize n = blsGetFieldOrder(&str[0], str.size());
	if (n == 0) throw std::runtime_error("blsGetFieldOrder");
	str.resize(n);
}
inline int getG1ByteSize() { return blsGetG1ByteSize(); }
inline int getFrByteSize() { return blsGetFrByteSize(); }

namespace local {
/*
	the value of secretKey and Id must be less than
	r = 0x2523648240000001ba344d8000000007ff9f800000000010a10000000000000d
	sizeof(uint64_t) * keySize byte
*/
const size_t keySize = MCLBN_FR_UNIT_SIZE;

inline void convertArray(uint8_t *dst, const uint64_t *src, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		uint64_t x = src[i];
		for (size_t j = 0; j < 8; j++) {
			*dst++ = uint8_t(x);
			x >>= 8;
		}
	}
}

}

class SecretKey;
class PublicKey;
class Signature;
class Id;

typedef std::vector<SecretKey> SecretKeyVec;
typedef std::vector<PublicKey> PublicKeyVec;
typedef std::vector<Signature> SignatureVec;
typedef std::vector<Id> IdVec;

class Id {
	blsId self_;
	friend class PublicKey;
	friend class SecretKey;
	friend class Signature;
public:
	const blsId *getPtr() const { return &self_; }
	Id(unsigned int id = 0)
	{
		blsIdSetInt(&self_, id);
	}
	bool operator==(const Id& rhs) const
	{
		return blsIdIsEqual(&self_, &rhs.self_) == 1;
	}
	bool operator!=(const Id& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const Id& id)
	{
		std::string str;
		id.getStr(str, 16|IoPrefix);
		return os << str;
	}
	friend std::istream& operator>>(std::istream& is, Id& id)
	{
		std::string str;
		is >> str;
		id.setStr(str, 16);
		return is;
	}
	void getStr(std::string& str, int ioMode = 0) const
	{
		str.resize(1024);
		size_t n = mclBnFr_getStr(&str[0], str.size(), &self_.v, ioMode);
		if (n == 0) throw std::runtime_error("mclBnFr_getStr");
		str.resize(n);
	}
	std::string getStr(int ioMode = 0) const
	{
		std::string str;
		getStr(str, ioMode);
		return str;
	}
	std::string serializeToHexStr() const { return getStr(2048); }
	void deserializeHexStr(const std::string& str)
	{
		setStr(str, 2048);
	}
	void clear() { memset(&self_, 0, sizeof(self_)); }
	void setStr(const std::string& str, int ioMode = 0)
	{
		int ret = mclBnFr_setStr(&self_.v, str.c_str(), str.size(), ioMode);
		if (ret != 0) throw std::runtime_error("mclBnFr_setStr");
	}
	bool isZero() const
	{
		return mclBnFr_isZero(&self_.v) == 1;
	}
	/*
		set p[0, .., keySize)
		@note the value must be less than r
	*/
	void set(const uint64_t *p)
	{
		const size_t n = blsGetFrByteSize() / sizeof(uint64_t);
		uint8_t x[sizeof(*this)];
		local::convertArray(x, p, n);
		setLittleEndian(x, n * sizeof(uint64_t));
	}
	// bufSize is truncted/zero extended to keySize
	void setLittleEndian(const uint8_t *buf, size_t bufSize)
	{
		mclBnFr_setLittleEndian(&self_.v, buf, bufSize);
	}
};

/*
	s ; secret key
*/
class SecretKey {
	blsSecretKey self_;
public:
	const blsSecretKey *getPtr() const { return &self_; }
	bool operator==(const SecretKey& rhs) const
	{
		return blsSecretKeyIsEqual(&self_, &rhs.self_) == 1;
	}
	bool operator!=(const SecretKey& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const SecretKey& sec)
	{
		std::string str;
		sec.getStr(str, 16|IoPrefix);
		return os << str;
	}
	friend std::istream& operator>>(std::istream& is, SecretKey& sec)
	{
		std::string str;
		is >> str;
		sec.setStr(str);
		return is;
	}
	void getStr(std::string& str, int ioMode = 0) const
	{
		str.resize(1024);
		size_t n = mclBnFr_getStr(&str[0], str.size(), &self_.v, ioMode);
		if (n == 0) throw std::runtime_error("mclBnFr_getStr");
		str.resize(n);
	}
	std::string getStr(int ioMode = 0) const
	{
		std::string str;
		getStr(str, ioMode);
		return str;
	}
	std::string serializeToHexStr() const { return getStr(2048); }
	void deserializeHexStr(const std::string& str)
	{
		setStr(str, 2048);
	}
	void clear() { memset(&self_, 0, sizeof(self_)); }
	void setStr(const std::string& str, int ioMode = 0)
	{
		int ret = mclBnFr_setStr(&self_.v, str.c_str(), str.size(), ioMode);
		if (ret != 0) throw std::runtime_error("mclBnFr_setStr");
	}
	/*
		initialize secretKey with random number
	*/
	void init()
	{
		int ret = blsSecretKeySetByCSPRNG(&self_);
		if (ret != 0) throw std::runtime_error("blsSecretKeySetByCSPRNG");
	}
	/*
		set secretKey with p[0, .., keySize) and set id = 0
		@note the value must be less than r
	*/
	void set(const uint64_t *p)
	{
		const size_t n = blsGetFrByteSize() / sizeof(uint64_t);
		uint8_t x[sizeof(*this)];
		local::convertArray(x, p, n);
		setLittleEndian(x, n * sizeof(uint64_t));
	}
	// bufSize is truncted/zero extended to keySize
	void setLittleEndian(const uint8_t *buf, size_t bufSize)
	{
		mclBnFr_setLittleEndian(&self_.v, buf, bufSize);
	}
	// set hash of buf
	void setHashOf(const void *buf, size_t bufSize)
	{
		int ret = mclBnFr_setHashOf(&self_.v, buf, bufSize);
		if (ret != 0) throw std::runtime_error("mclBnFr_setHashOf");
	}
	void getPublicKey(PublicKey& pub) const;
	// constant time sign
	// sign hash(m)
	void sign(Signature& sig, const void *m, size_t size) const;
	void sign(Signature& sig, const std::string& m) const
	{
		sign(sig, m.c_str(), m.size());
	}
	// sign hashed value
	void signHash(Signature& sig, const void *h, size_t size) const;
	void signHash(Signature& sig, const std::string& h) const
	{
		signHash(sig, h.c_str(), h.size());
	}
	/*
		make Pop(Proof of Possesion)
		pop = prv.sign(pub)
	*/
	void getPop(Signature& pop) const;
	/*
		make [s_0, ..., s_{k-1}] to prepare k-out-of-n secret sharing
	*/
	void getMasterSecretKey(SecretKeyVec& msk, size_t k) const
	{
		if (k <= 1) throw std::invalid_argument("getMasterSecretKey");
		msk.resize(k);
		msk[0] = *this;
		for (size_t i = 1; i < k; i++) {
			msk[i].init();
		}
	}
	/*
		set a secret key for id > 0 from msk
	*/
	void set(const SecretKeyVec& msk, const Id& id)
	{
		set(msk.data(), msk.size(), id);
	}
	/*
		recover secretKey from k secVec
	*/
	void recover(const SecretKeyVec& secVec, const IdVec& idVec)
	{
		if (secVec.size() != idVec.size()) throw std::invalid_argument("SecretKey:recover");
		recover(secVec.data(), idVec.data(), idVec.size());
	}
	/*
		add secret key
	*/
	void add(const SecretKey& rhs);

	// the following methods are for C api
	/*
		the size of msk must be k
	*/
	void set(const SecretKey *msk, size_t k, const Id& id)
	{
		int ret = blsSecretKeyShare(&self_, &msk->self_, k, &id.self_);
		if (ret != 0) throw std::runtime_error("blsSecretKeyShare");
	}
	void recover(const SecretKey *secVec, const Id *idVec, size_t n)
	{
		int ret = blsSecretKeyRecover(&self_, &secVec->self_, &idVec->self_, n);
		if (ret != 0) throw std::runtime_error("blsSecretKeyRecover:same id");
	}
};

/*
	sQ ; public key
*/
class PublicKey {
	blsPublicKey self_;
	friend class SecretKey;
	friend class Signature;
public:
	const blsPublicKey *getPtr() const { return &self_; }
	bool operator==(const PublicKey& rhs) const
	{
		return blsPublicKeyIsEqual(&self_, &rhs.self_) == 1;
	}
	bool operator!=(const PublicKey& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const PublicKey& pub)
	{
		std::string str;
		pub.getStr(str, 16|IoPrefix);
		return os << str;
	}
	friend std::istream& operator>>(std::istream& is, PublicKey& pub)
	{
		std::string str;
		is >> str;
		if (str != "0") {
			// 1 <x.a> <x.b> <y.a> <y.b>
			std::string t;
#ifdef BLS_ETH
			const int elemNum = 2;
#else
			const int elemNum = 4;
#endif
			for (int i = 0; i < elemNum; i++) {
				is >> t;
				str += ' ';
				str += t;
			}
		}
		pub.setStr(str, 16);
		return is;
	}
	void getStr(std::string& str, int ioMode = 0) const
	{
		str.resize(1024);
#ifdef BLS_ETH
		size_t n = mclBnG1_getStr(&str[0], str.size(), &self_.v, ioMode);
#else
		size_t n = mclBnG2_getStr(&str[0], str.size(), &self_.v, ioMode);
#endif
		if (n == 0) throw std::runtime_error("PublicKey:getStr");
		str.resize(n);
	}
	std::string getStr(int ioMode = 0) const
	{
		std::string str;
		getStr(str, ioMode);
		return str;
	}
	std::string serializeToHexStr() const { return getStr(2048); }
	void deserializeHexStr(const std::string& str)
	{
		setStr(str, 2048);
	}
	void clear() { memset(&self_, 0, sizeof(self_)); }
	void setStr(const std::string& str, int ioMode = 0)
	{
#ifdef BLS_ETH
		int ret = mclBnG1_setStr(&self_.v, str.c_str(), str.size(), ioMode);
#else
		int ret = mclBnG2_setStr(&self_.v, str.c_str(), str.size(), ioMode);
#endif
		if (ret != 0) throw std::runtime_error("PublicKey:setStr");
	}
	/*
		set public for id from mpk
	*/
	void set(const PublicKeyVec& mpk, const Id& id)
	{
		set(mpk.data(), mpk.size(), id);
	}
	/*
		recover publicKey from k pubVec
	*/
	void recover(const PublicKeyVec& pubVec, const IdVec& idVec)
	{
		if (pubVec.size() != idVec.size()) throw std::invalid_argument("PublicKey:recover");
		recover(pubVec.data(), idVec.data(), idVec.size());
	}
	/*
		add public key
	*/
	void add(const PublicKey& rhs)
	{
		blsPublicKeyAdd(&self_, &rhs.self_);
	}

	// the following methods are for C api
	void set(const PublicKey *mpk, size_t k, const Id& id)
	{
		int ret = blsPublicKeyShare(&self_, &mpk->self_, k, &id.self_);
		if (ret != 0) throw std::runtime_error("blsPublicKeyShare");
	}
	void recover(const PublicKey *pubVec, const Id *idVec, size_t n)
	{
		int ret = blsPublicKeyRecover(&self_, &pubVec->self_, &idVec->self_, n);
		if (ret != 0) throw std::runtime_error("blsPublicKeyRecover");
	}
};

/*
	s H(m) ; signature
*/
class Signature {
	blsSignature self_;
	friend class SecretKey;
public:
	const blsSignature *getPtr() const { return &self_; }
	bool operator==(const Signature& rhs) const
	{
		return blsSignatureIsEqual(&self_, &rhs.self_) == 1;
	}
	bool operator!=(const Signature& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const Signature& sig)
	{
		std::string str;
		sig.getStr(str, 16|IoPrefix);
		return os << str;
	}
	friend std::istream& operator>>(std::istream& is, Signature& sig)
	{
		std::string str;
		is >> str;
		if (str != "0") {
			// 1 <x> <y>
			std::string t;
#ifdef BLS_ETH
			const int elemNum = 4;
#else
			const int elemNum = 2;
#endif
			for (int i = 0; i < elemNum; i++) {
				is >> t;
				str += ' ';
				str += t;
			}
		}
		sig.setStr(str, 16);
		return is;
	}
	void getStr(std::string& str, int ioMode = 0) const
	{
		str.resize(1024);
#ifdef BLS_ETH
		size_t n = mclBnG2_getStr(&str[0], str.size(), &self_.v, ioMode);
#else
		size_t n = mclBnG1_getStr(&str[0], str.size(), &self_.v, ioMode);
#endif
		if (n == 0) throw std::runtime_error("Signature:tgetStr");
		str.resize(n);
	}
	std::string getStr(int ioMode = 0) const
	{
		std::string str;
		getStr(str, ioMode);
		return str;
	}
	std::string serializeToHexStr() const { return getStr(2048); }
	void deserializeHexStr(const std::string& str)
	{
		setStr(str, 2048);
	}
	void clear() { memset(&self_, 0, sizeof(self_)); }
	void setStr(const std::string& str, int ioMode = 0)
	{
#ifdef BLS_ETH
		int ret = mclBnG2_setStr(&self_.v, str.c_str(), str.size(), ioMode);
#else
		int ret = mclBnG1_setStr(&self_.v, str.c_str(), str.size(), ioMode);
#endif
		if (ret != 0) throw std::runtime_error("Signature:setStr");
	}
	bool verify(const PublicKey& pub, const void *m, size_t size) const
	{
		return blsVerify(&self_, &pub.self_, m, size) == 1;
	}
	bool verify(const PublicKey& pub, const std::string& m) const
	{
		return verify(pub, m.c_str(), m.size());
	}
	bool verifyHash(const PublicKey& pub, const void *h, size_t size) const
	{
		return blsVerifyHash(&self_, &pub.self_, h, size) == 1;
	}
	bool verifyHash(const PublicKey& pub, const std::string& h) const
	{
		return verifyHash(pub, h.c_str(), h.size());
	}
	bool verifyAggregatedHashes(const PublicKey *pubVec, const void *hVec, size_t sizeofHash, size_t n) const
	{
		return blsVerifyAggregatedHashes(&self_, &pubVec[0].self_, hVec, sizeofHash, n) == 1;
	}
	/*
		verify self(pop) with pub
	*/
	bool verify(const PublicKey& pub) const
	{
		std::string str;
		pub.getStr(str);
		return verify(pub, str);
	}
	/*
		recover sig from k sigVec
	*/
	void recover(const SignatureVec& sigVec, const IdVec& idVec)
	{
		if (sigVec.size() != idVec.size()) throw std::invalid_argument("Signature:recover");
		recover(sigVec.data(), idVec.data(), idVec.size());
	}
	/*
		add signature
	*/
	void add(const Signature& rhs)
	{
		blsSignatureAdd(&self_, &rhs.self_);
	}

	// the following methods are for C api
	void recover(const Signature* sigVec, const Id *idVec, size_t n)
	{
		int ret = blsSignatureRecover(&self_, &sigVec->self_, &idVec->self_, n);
		if (ret != 0) throw std::runtime_error("blsSignatureRecover:same id");
	}
};

/*
	make master public key [s_0 Q, ..., s_{k-1} Q] from msk
*/
inline void getMasterPublicKey(PublicKeyVec& mpk, const SecretKeyVec& msk)
{
	const size_t n = msk.size();
	mpk.resize(n);
	for (size_t i = 0; i < n; i++) {
		msk[i].getPublicKey(mpk[i]);
	}
}

inline void SecretKey::getPublicKey(PublicKey& pub) const
{
	blsGetPublicKey(&pub.self_, &self_);
}
inline void SecretKey::sign(Signature& sig, const void *m, size_t size) const
{
	blsSign(&sig.self_, &self_, m, size);
}
inline void SecretKey::signHash(Signature& sig, const void *h, size_t size) const
{
	if (blsSignHash(&sig.self_, &self_, h, size) != 0) throw std::runtime_error("bad h");
}
inline void SecretKey::getPop(Signature& pop) const
{
	PublicKey pub;
	getPublicKey(pub);
	std::string m;
	pub.getStr(m);
	sign(pop, m);
}

/*
	make pop from msk and mpk
*/
inline void getPopVec(SignatureVec& popVec, const SecretKeyVec& msk)
{
	const size_t n = msk.size();
	popVec.resize(n);
	for (size_t i = 0; i < n; i++) {
		msk[i].getPop(popVec[i]);
	}
}

inline Signature operator+(const Signature& a, const Signature& b) { Signature r(a); r.add(b); return r; }
inline PublicKey operator+(const PublicKey& a, const PublicKey& b) { PublicKey r(a); r.add(b); return r; }
inline SecretKey operator+(const SecretKey& a, const SecretKey& b) { SecretKey r(a); r.add(b); return r; }

} //bls
