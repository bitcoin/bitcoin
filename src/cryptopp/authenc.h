// authenc.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile authenc.h
//! \brief Base classes for working with authenticated encryption modes of encryption
//! \since Crypto++ 5.6.0

#ifndef CRYPTOPP_AUTHENC_H
#define CRYPTOPP_AUTHENC_H

#include "cryptlib.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class AuthenticatedSymmetricCipherBase
//! \brief Base implementation for one direction (encryption or decryption) of a stream cipher or block cipher mode with authentication
//! \since Crypto++ 5.6.0
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AuthenticatedSymmetricCipherBase : public AuthenticatedSymmetricCipher
{
public:
	AuthenticatedSymmetricCipherBase() : m_state(State_Start), m_bufferedDataLength(0),
		m_totalHeaderLength(0), m_totalMessageLength(0), m_totalFooterLength(0) {}

	bool IsRandomAccess() const {return false;}
	bool IsSelfInverting() const {return true;}

	//! \brief Sets the key for this object without performing parameter validation
	//! \param key a byte buffer used to key the cipher
	//! \param length the length of the byte buffer
	//! \param params additional parameters passed as  NameValuePairs
	//! \details key must be at least DEFAULT_KEYLENGTH in length.
	void UncheckedSetKey(const byte * key, unsigned int length,const CryptoPP::NameValuePairs &params)
		{CRYPTOPP_UNUSED(key), CRYPTOPP_UNUSED(length), CRYPTOPP_UNUSED(params); CRYPTOPP_ASSERT(false);}

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
