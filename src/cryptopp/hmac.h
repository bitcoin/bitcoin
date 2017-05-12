// hmac.h - written and placed in the public domain by Wei Dai

//! \file hmac.h
//! \brief Classes for HMAC message authentication codes

#ifndef CRYPTOPP_HMAC_H
#define CRYPTOPP_HMAC_H

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class HMAC_Base
//! \brief HMAC information
//! \details HMAC_Base derives from VariableKeyLength and MessageAuthenticationCode
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE HMAC_Base : public VariableKeyLength<16, 0, INT_MAX>, public MessageAuthenticationCode
{
public:
	//! \brief Construct a HMAC_Base
	HMAC_Base() : m_innerHashKeyed(false) {}
	void UncheckedSetKey(const byte *userKey, unsigned int keylength, const NameValuePairs &params);

	void Restart();
	void Update(const byte *input, size_t length);
	void TruncatedFinal(byte *mac, size_t size);
	unsigned int OptimalBlockSize() const {return const_cast<HMAC_Base*>(this)->AccessHash().OptimalBlockSize();}
	unsigned int DigestSize() const {return const_cast<HMAC_Base*>(this)->AccessHash().DigestSize();}

protected:
	virtual HashTransformation & AccessHash() =0;
	byte * AccessIpad() {return m_buf;}
	byte * AccessOpad() {return m_buf + AccessHash().BlockSize();}
	byte * AccessInnerHash() {return m_buf + 2*AccessHash().BlockSize();}

private:
	void KeyInnerHash();

	SecByteBlock m_buf;
	bool m_innerHashKeyed;
};

//! \class HMAC
//! \brief HMAC
//! \tparam T HashTransformation derived class
//! \details HMAC derives from MessageAuthenticationCodeImpl. It calculates the HMAC using
//!   <tt>HMAC(K, text) = H(K XOR opad, H(K XOR ipad, text))</tt>.
//! \sa <a href="http://www.weidai.com/scan-mirror/mac.html#HMAC">HMAC</a>
template <class T>
class HMAC : public MessageAuthenticationCodeImpl<HMAC_Base, HMAC<T> >
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE=T::DIGESTSIZE)
	CRYPTOPP_CONSTANT(BLOCKSIZE=T::BLOCKSIZE)

	//! \brief Construct a HMAC
	HMAC() {}
	//! \brief Construct a HMAC
	//! \param key the HMAC key
	//! \param length the size of the HMAC key
	HMAC(const byte *key, size_t length=HMAC_Base::DEFAULT_KEYLENGTH)
		{this->SetKey(key, length);}

	static std::string StaticAlgorithmName() {return std::string("HMAC(") + T::StaticAlgorithmName() + ")";}
	std::string AlgorithmName() const {return std::string("HMAC(") + m_hash.AlgorithmName() + ")";}

private:
	HashTransformation & AccessHash() {return m_hash;}

	T m_hash;
};

NAMESPACE_END

#endif
