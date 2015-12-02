// authenc.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile authenc.h
//! \brief Base classes for working with authenticated encryption modes of encryption

#ifndef CRYPTOPP_AUTHENC_H
#define CRYPTOPP_AUTHENC_H

#include "cryptlib.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class AuthenticatedSymmetricCipherBase
//! \brief 
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AuthenticatedSymmetricCipherBase : public AuthenticatedSymmetricCipher
{
public:
	AuthenticatedSymmetricCipherBase() : m_state(State_Start), m_bufferedDataLength(0),
		m_totalHeaderLength(0), m_totalMessageLength(0), m_totalFooterLength(0) {}

	bool IsRandomAccess() const {return false;}
	bool IsSelfInverting() const {return true;}
	void UncheckedSetKey(const byte *,unsigned int,const CryptoPP::NameValuePairs &) {assert(false);}

	void SetKey(const byte *userKey, size_t keylength, const NameValuePairs &params);
	void Restart() {if (m_state > State_KeySet) m_state = State_KeySet;}
	void Resynchronize(const byte *iv, int length=-1);
	void Update(const byte *input, size_t length);
	void ProcessData(byte *outString, const byte *inString, size_t length);
	void TruncatedFinal(byte *mac, size_t macSize);

protected:
	void AuthenticateData(const byte *data, size_t len);
	const SymmetricCipher & GetSymmetricCipher() const {return const_cast<AuthenticatedSymmetricCipherBase *>(this)->AccessSymmetricCipher();};

	virtual SymmetricCipher & AccessSymmetricCipher() =0;
	virtual bool AuthenticationIsOnPlaintext() const =0;
	virtual unsigned int AuthenticationBlockSize() const =0;
	virtual void SetKeyWithoutResync(const byte *userKey, size_t keylength, const NameValuePairs &params) =0;
	virtual void Resync(const byte *iv, size_t len) =0;
	virtual size_t AuthenticateBlocks(const byte *data, size_t len) =0;
	virtual void AuthenticateLastHeaderBlock() =0;
	virtual void AuthenticateLastConfidentialBlock() {}
	virtual void AuthenticateLastFooterBlock(byte *mac, size_t macSize) =0;

	enum State {State_Start, State_KeySet, State_IVSet, State_AuthUntransformed, State_AuthTransformed, State_AuthFooter};
	State m_state;
	unsigned int m_bufferedDataLength;
	lword m_totalHeaderLength, m_totalMessageLength, m_totalFooterLength;
	AlignedSecByteBlock m_buffer;
};

NAMESPACE_END

#endif
