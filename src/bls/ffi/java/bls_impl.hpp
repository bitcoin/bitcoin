#include <bls/bls384_256.h>
#include <stdint.h>
#include <sstream>
#include <vector>
#include <set>
#include <bls/msg.hpp>

#if defined(__GNUC__) && !defined(__EMSCRIPTEN__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

void init(int curveType) throw(std::exception)
{
	int ret = blsInit(curveType, MCLBN_COMPILED_TIME_VAR);
	if (ret) {
		throw std::runtime_error("bad curveType");
	}
}

bool isDefinedBLS_ETH()
{
#ifdef BLS_ETH
	return true;
#else
	return false;
#endif
}

class SecretKey;
class PublicKey;
class Signature;

typedef std::vector<SecretKey> SecretKeyVec;
typedef std::vector<PublicKey> PublicKeyVec;
typedef std::vector<Signature> SignatureVec;

class SecretKey {
	blsSecretKey self_;
	friend class PublicKey;
	friend class Signature;
public:
	SecretKey() {}
	SecretKey(const SecretKey& rhs) : self_(rhs.self_) {}
	SecretKey(int x)
    {
        setInt(x);
    }
	bool equals(const SecretKey& rhs) const
	{
		return blsSecretKeyIsEqual(&self_, &rhs.self_) != 0;
	}
	bool isZero() const { return blsSecretKeyIsZero(&self_) != 0; }
	void setStr(const std::string& str, int base = 10) throw(std::exception)
	{
		size_t n = 0;
		const size_t len = str.size();
		if (base == 10) {
			n = blsSecretKeySetDecStr(&self_, str.c_str(), len);
		} else if (base == 16) {
			n = blsSecretKeySetHexStr(&self_, str.c_str(), len);
		}
		if (n != 0) throw std::runtime_error("bad str");
	}
	void setInt(int x)
	{
        blsIdSetInt((blsId*)&self_, x);
	}
	void clear()
	{
        memset(&self_, 0, sizeof(self_));
	}
	void setByCSPRNG()
	{
        blsSecretKeySetByCSPRNG(&self_);
	}
	std::string toString(int base = 10) const throw(std::exception)
	{
		size_t n = 0;
		std::string s;
		s.resize(128);
		if (base == 10) {
			n = blsSecretKeyGetDecStr(&s[0], s.size(), &self_);
		} else if (base == 16) {
			n = blsSecretKeyGetHexStr(&s[0], s.size(), &self_);
		}
		if (n == 0) throw std::runtime_error("err toString");
		s.resize(n);
		return s;
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
        int n = blsSecretKeyDeserialize(&self_, cbuf, bufSize);
        if (n == 0) {
            throw std::runtime_error("blsSecretKeyDeserialize");
        }
	}
	void serialize(std::string& out) const throw(std::exception)
	{
        out.resize(128);
        size_t n = blsSecretKeySerialize(&out[0], out.size(), &self_);
        if (n == 0) {
            throw std::runtime_error("blsSecretKeySerialize");
        }
        out.resize(n);
	}
	void setLittleEndian(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		int r = blsSecretKeySetLittleEndian(&self_, cbuf, bufSize);
		if (r != 0) {
			throw std::runtime_error("blsSecretKeySetLittleEndian");
		}
	}
	void setLittleEndianMod(const char *cbuf, size_t bufSize) throw(std::exception)
	{
		int r = blsSecretKeySetLittleEndianMod(&self_, cbuf, bufSize);
		if (r != 0) {
			throw std::runtime_error("blsSecretKeySetLittleEndianMod");
		}
	}
	void add(const SecretKey& rhs)
	{
		blsSecretKeyAdd(&self_, &rhs.self_);
	}
	void sub(const SecretKey& rhs)
	{
		blsSecretKeySub(&self_, &rhs.self_);
	}
	void mul(const SecretKey& rhs)
	{
		blsSecretKeyMul(&self_, &rhs.self_);
	}
	void neg()
	{
		blsSecretKeyNeg(&self_);
	}
	void getPublicKey(PublicKey& pub) const;
	PublicKey getPublicKey() const;
	void sign(Signature& sig, const char *cbuf, size_t bufSize) const;
	void signHash(Signature& sig, const char *cbuf, size_t bufSize) const;
	Signature sign(const char *cbuf, size_t bufSize) const;
	Signature signHash(const char *cbuf, size_t bufSize) const;
	void share(const SecretKeyVec& secVec, const SecretKey& id);
	void recover(const SecretKeyVec& secVec, const SecretKeyVec& idVec);
	void setHashOf(const char *cbuf, size_t bufSize) throw(std::exception);
};

class PublicKey {
	blsPublicKey self_;
	friend class SecretKey;
	friend class Signature;
	friend void setGeneratorOfPublicKey(const PublicKey& pub) throw(std::exception);
public:
	PublicKey() {}
	PublicKey(const PublicKey& rhs) : self_(rhs.self_) {}
	bool equals(const PublicKey& rhs) const
	{
		return blsPublicKeyIsEqual(&self_, &rhs.self_) != 0;
	}
	bool isZero() const { return blsPublicKeyIsZero(&self_) != 0; }
	void setStr(const std::string& str) throw(std::exception)
	{
		const size_t len = str.size();
		size_t n = blsPublicKeySetHexStr(&self_, str.c_str(), len);
		if (n != 0) throw std::runtime_error("bad str");
	}
	void clear()
	{
        memset(&self_, 0, sizeof(self_));
	}
	std::string toString() const throw(std::exception)
	{
		char buf[512];
		size_t n = blsPublicKeyGetHexStr(buf, sizeof(buf), &self_);
		if (n == 0) throw std::runtime_error("err toString");
		return std::string(buf, n);
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
        int n = blsPublicKeyDeserialize(&self_, cbuf, bufSize);
        if (n == 0) {
            throw std::runtime_error("blsPublicKeyDeserialize");
        }
	}
	void serialize(std::string& out) const throw(std::exception)
	{
        out.resize(128);
        size_t n = blsPublicKeySerialize(&out[0], out.size(), &self_);
        if (n == 0) {
            throw std::runtime_error("blsPublicKeySerialize");
        }
        out.resize(n);
	}
	void add(const PublicKey& rhs)
	{
		blsPublicKeyAdd(&self_, &rhs.self_);
	}
	void sub(const PublicKey& rhs)
	{
		blsPublicKeySub(&self_, &rhs.self_);
	}
	void mul(const SecretKey& rhs)
	{
		blsPublicKeyMul(&self_, &rhs.self_);
	}
	void neg()
	{
		blsPublicKeyNeg(&self_);
	}
	void share(const PublicKeyVec& secVec, const SecretKey& id);
	void recover(const PublicKeyVec& secVec, const SecretKeyVec& idVec);
};

class Signature {
	blsSignature self_;
	friend class SecretKey;
	friend class PublicKey;
public:
	Signature() {}
	Signature(const Signature& rhs) : self_(rhs.self_) {}
	bool equals(const Signature& rhs) const
	{
		return blsSignatureIsEqual(&self_, &rhs.self_) != 0;
	}
	bool isZero() const { return blsSignatureIsZero(&self_) != 0; }
	void setStr(const std::string& str) throw(std::exception)
	{
		const size_t len = str.size();
		size_t n = blsSignatureSetHexStr(&self_, str.c_str(), len);
		if (n != 0) throw std::runtime_error("bad str");
	}
	void clear()
	{
        memset(&self_, 0, sizeof(self_));
	}
	std::string toString() const throw(std::exception)
	{
		char buf[256];
		size_t n = blsSignatureGetHexStr(buf, sizeof(buf), &self_);
		if (n == 0) throw std::runtime_error("err toString");
		return std::string(buf, n);
	}
	void deserialize(const char *cbuf, size_t bufSize) throw(std::exception)
	{
        int n = blsSignatureDeserialize(&self_, cbuf, bufSize);
        if (n == 0) {
            throw std::runtime_error("blsSignatureDeserialize");
        }
	}
	void serialize(std::string& out) const throw(std::exception)
	{
        out.resize(128);
        size_t n = blsSignatureSerialize(&out[0], out.size(), &self_);
        if (n == 0) {
            throw std::runtime_error("blsSignatureSerialize");
        }
        out.resize(n);
	}
	void add(const Signature& rhs)
	{
		blsSignatureAdd(&self_, &rhs.self_);
	}
	void sub(const Signature& rhs)
	{
		blsSignatureSub(&self_, &rhs.self_);
	}
	void mul(const SecretKey& rhs)
	{
		blsSignatureMul(&self_, &rhs.self_);
	}
	void neg()
	{
		blsSignatureNeg(&self_);
	}
	bool verify(const PublicKey& pub, const char *cbuf, size_t bufSize) const
	{
		return blsVerify(&self_, &pub.self_, cbuf, bufSize) == 1;
	}
	bool verifyHash(const PublicKey& pub, const char *cbuf, size_t bufSize) const
	{
		return blsVerifyHash(&self_, &pub.self_, cbuf, bufSize) == 1;
	}
	void recover(const SignatureVec& sigVec, const SecretKeyVec& idVec);
	void setHashOf(const char *cbuf, size_t bufSize) throw(std::exception);
	void aggregate(const SignatureVec& sigVec) throw(std::exception)
	{
		const size_t n = sigVec.size();
		if (n == 0) throw std::runtime_error("aggregate zero");
		blsAggregateSignature(&self_, &sigVec[0].self_, n);
	}
	bool fastAggregateVerify(const PublicKeyVec& pubVec, const char *cbuf, size_t bufSize) const
	{
		const size_t n = pubVec.size();
		if (n == 0) return false;
		return blsFastAggregateVerify(&self_, &pubVec[0].self_, n, cbuf, bufSize) == 1;
	}
	bool aggregateVerifyNoCheck(const PublicKeyVec& pubVec, const char *cbuf, size_t bufSize) const
	{
		const size_t n = pubVec.size();
		if (n == 0) return false;
		if (bufSize != bls_util::MSG_SIZE * n) return false;
		return blsAggregateVerifyNoCheck(&self_, &pubVec[0].self_, cbuf, bls_util::MSG_SIZE, n) == 1;
	}
	bool aggregateVerify(const PublicKeyVec& pubVec, const char *cbuf, size_t bufSize) const
	{
		if (!bls_util::areAllMsgDifferent(cbuf, bufSize)) return false;
		return aggregateVerifyNoCheck(pubVec, cbuf, bufSize);
	}
};

inline void SecretKey::getPublicKey(PublicKey& pub) const
{
	blsGetPublicKey(&pub.self_, &self_);
}

inline PublicKey SecretKey::getPublicKey() const
{
	PublicKey pub;
	getPublicKey(pub);
	return pub;
}

inline void SecretKey::sign(Signature& sig, const char *cbuf, size_t bufSize) const
{
	blsSign(&sig.self_, &self_, cbuf, bufSize);
}

inline void SecretKey::signHash(Signature& sig, const char *cbuf, size_t bufSize) const
{
	int r = blsSignHash(&sig.self_, &self_, cbuf, bufSize);
	if (r != 0) {
		throw std::runtime_error("blsSignHash err");
	}
}

inline Signature SecretKey::sign(const char *cbuf, size_t bufSize) const
{
	Signature sig;
	sign(sig, cbuf, bufSize);
	return sig;
}

inline Signature SecretKey::signHash(const char *cbuf, size_t bufSize) const
{
	Signature sig;
	signHash(sig, cbuf, bufSize);
	return sig;
}

inline void SecretKey::share(const SecretKeyVec& secVec, const SecretKey& id)
{
	int r = blsSecretKeyShare(&self_, &secVec[0].self_, secVec.size(), (const blsId*)&id.self_);
	if (r != 0) {
		throw std::runtime_error("blsSecretKeyShare");
	}
}

inline void PublicKey::share(const PublicKeyVec& pubVec, const SecretKey& id)
{
	int r = blsPublicKeyShare(&self_, &pubVec[0].self_, pubVec.size(), (const blsId*)&id.self_);
	if (r != 0) {
		throw std::runtime_error("blsPublicKeyShare");
	}
}

inline void SecretKey::recover(const SecretKeyVec& secVec, const SecretKeyVec& idVec)
{
	size_t n = secVec.size();
	if (n == 0 || n != idVec.size()) {
		throw std::runtime_error("bad length");
	}
	int r = blsSecretKeyRecover(&self_, &secVec[0].self_, (const blsId*)&idVec[0].self_, n);
	if (r != 0) {
		throw std::runtime_error("blsSecretKeyRecover");
	}
}

inline void PublicKey::recover(const PublicKeyVec& pubVec, const SecretKeyVec& idVec)
{
	size_t n = pubVec.size();
	if (n == 0 || n != idVec.size()) {
		throw std::runtime_error("bad length");
	}
	int r = blsPublicKeyRecover(&self_, &pubVec[0].self_, (const blsId*)&idVec[0].self_, n);
	if (r != 0) {
		throw std::runtime_error("blsPublicKeyRecover");
	}
}

inline void Signature::recover(const SignatureVec& sigVec, const SecretKeyVec& idVec)
{
	size_t n = sigVec.size();
	if (n == 0 || n != idVec.size()) {
		throw std::runtime_error("bad length");
	}
	int r = blsSignatureRecover(&self_, &sigVec[0].self_, (const blsId*)&idVec[0].self_, n);
	if (r != 0) {
		throw std::runtime_error("blsSignatureRecover");
	}
}

inline void SecretKey::setHashOf(const char *cbuf, size_t bufSize) throw(std::exception)
{
	int r = blsHashToSecretKey(&self_, cbuf, bufSize);
	if (r != 0) {
		throw std::runtime_error("blsHashToSecretKey");
	}
}

inline void Signature::setHashOf(const char *cbuf, size_t bufSize) throw(std::exception)
{
	int r = blsHashToSignature(&self_, cbuf, bufSize);
	if (r != 0) {
		throw std::runtime_error("blsHashToSignature");
	}
}

SecretKey share(const SecretKeyVec& secVec, const SecretKey& id)
{
	SecretKey sec;
	sec.share(secVec, id);
	return sec;
}

PublicKey share(const PublicKeyVec& pubVec, const SecretKey& id)
{
	PublicKey pub;
	pub.share(pubVec, id);
	return pub;
}

SecretKey recover(const SecretKeyVec& secVec, const SecretKeyVec& idVec)
{
	SecretKey sec;
	sec.recover(secVec, idVec);
	return sec;
}

PublicKey recover(const PublicKeyVec& secVec, const SecretKeyVec& idVec)
{
	PublicKey sec;
	sec.recover(secVec, idVec);
	return sec;
}

Signature recover(const SignatureVec& sigVec, const SecretKeyVec& idVec)
{
	Signature sig;
	sig.recover(sigVec, idVec);
	return sig;
}

Signature aggregate(const SignatureVec& sigVec)
{
	Signature sig;
	sig.aggregate(sigVec);
	return sig;
}

void setETHserialization(bool ETH)
{
	blsSetETHserialization(ETH);
}

void setGeneratorOfPublicKey(const PublicKey& pub) throw(std::exception)
{
	int r = blsSetGeneratorOfPublicKey(&pub.self_);
	if (r != 0) {
		throw std::runtime_error("blsSetGeneratorOfPublicKey");
	}
}

/*
    change map-to-function
    MAP_TO_MODE_ORIGINAL ; default if BLS_ETH is not defined (for backward compatibility)
    MAP_TO_MODE_HASH_TO_CURVE ; default if BLS_ETH=1 (defined in irtf)
*/
void setMapToMode(int mode)
{
	int r = blsSetMapToMode(mode);
	if (r != 0) {
		throw std::runtime_error("blsSetMapToMode");
	}
}

void verifySignatureOrder(bool doVerify)
{
	blsSignatureVerifyOrder(doVerify);
}

void verifyPublicKeyOrder(bool doVerify)
{
	blsPublicKeyVerifyOrder(doVerify);
}

#if defined(__GNUC__) && !defined(__EMSCRIPTEN__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
