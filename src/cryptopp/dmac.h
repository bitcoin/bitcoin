// dmac.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile dmac.h
//! \brief Classes for DMAC message authentication code

#ifndef CRYPTOPP_DMAC_H
#define CRYPTOPP_DMAC_H

#include "cbcmac.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
template <class T>
class CRYPTOPP_NO_VTABLE DMAC_Base : public SameKeyLengthAs<T>, public MessageAuthenticationCode
{
public:
	static std::string StaticAlgorithmName() {return std::string("DMAC(") + T::StaticAlgorithmName() + ")";}

	CRYPTOPP_CONSTANT(DIGESTSIZE=T::BLOCKSIZE)

	DMAC_Base() : m_subkeylength(0), m_counter(0) {}

	void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params);
	void Update(const byte *input, size_t length);
	void TruncatedFinal(byte *mac, size_t size);
	unsigned int DigestSize() const {return DIGESTSIZE;}

private:
	byte *GenerateSubKeys(const byte *key, size_t keylength);

	size_t m_subkeylength;
	SecByteBlock m_subkeys;
	CBC_MAC<T> m_mac1;
	typename T::Encryption m_f2;
	unsigned int m_counter;
};

//! DMAC
/*! Based on "CBC MAC for Real-Time Data Sources" by Erez Petrank
	and Charles Rackoff. T should be a class derived from BlockCipherDocumentation.
*/
template <class T>
class DMAC : public MessageAuthenticationCodeFinal<DMAC_Base<T> >
{
public:
	DMAC() {}
	DMAC(const byte *key, size_t length=DMAC_Base<T>::DEFAULT_KEYLENGTH)
		{this->SetKey(key, length);}
};

template <class T>
void DMAC_Base<T>::UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params)
{
	m_subkeylength = T::StaticGetValidKeyLength(T::BLOCKSIZE);
	m_subkeys.resize(2*UnsignedMin((unsigned int)T::BLOCKSIZE, m_subkeylength));
	m_mac1.SetKey(GenerateSubKeys(key, length), m_subkeylength, params);
	m_f2.SetKey(m_subkeys+m_subkeys.size()/2, m_subkeylength, params);
	m_counter = 0;
	m_subkeys.resize(0);
}

template <class T>
void DMAC_Base<T>::Update(const byte *input, size_t length)
{
	m_mac1.Update(input, length);
	m_counter = (unsigned int)((m_counter + length) % T::BLOCKSIZE);
}

template <class T>
void DMAC_Base<T>::TruncatedFinal(byte *mac, size_t size)
{
	ThrowIfInvalidTruncatedSize(size);

	byte pad[T::BLOCKSIZE];
	byte padByte = byte(T::BLOCKSIZE-m_counter);
	memset(pad, padByte, padByte);
	m_mac1.Update(pad, padByte);
	m_mac1.TruncatedFinal(mac, size);
	m_f2.ProcessBlock(mac);

	m_counter = 0;	// reset for next message
}

template <class T>
byte *DMAC_Base<T>::GenerateSubKeys(const byte *key, size_t keylength)
{
	typename T::Encryption cipher(key, keylength);
	memset(m_subkeys, 0, m_subkeys.size());
	cipher.ProcessBlock(m_subkeys);
	m_subkeys[m_subkeys.size()/2 + T::BLOCKSIZE - 1] = 1;
	cipher.ProcessBlock(m_subkeys+m_subkeys.size()/2);
	return m_subkeys;
}

NAMESPACE_END

#endif
