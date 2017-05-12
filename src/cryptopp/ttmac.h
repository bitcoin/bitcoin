// ttmac.h - written and placed in the public domain by Kevin Springle

//! \file ttmac.h
//! \brief Classes for the TTMAC message authentication code

#ifndef CRYPTOPP_TTMAC_H
#define CRYPTOPP_TTMAC_H

#include "seckey.h"
#include "iterhash.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class TTMAC_Base
//! \brief TTMAC message authentication code information
class CRYPTOPP_NO_VTABLE TTMAC_Base : public FixedKeyLength<20>, public IteratedHash<word32, LittleEndian, 64, MessageAuthenticationCode>
{
public:
	static std::string StaticAlgorithmName() {return std::string("Two-Track-MAC");}
	CRYPTOPP_CONSTANT(DIGESTSIZE=20)

	unsigned int DigestSize() const {return DIGESTSIZE;};
	void UncheckedSetKey(const byte *userKey, unsigned int keylength, const NameValuePairs &params);
	void TruncatedFinal(byte *mac, size_t size);

protected:
	static void Transform (word32 *digest, const word32 *X, bool last);
	void HashEndianCorrectedBlock(const word32 *data) {Transform(m_digest, data, false);}
	void Init();
	word32* StateBuf() {return m_digest;}

	FixedSizeSecBlock<word32, 10> m_digest;
	FixedSizeSecBlock<word32, 5> m_key;
};

//! \class TTMAC
//! \brief Two-Track-MAC message authentication code
//! \tparam T HashTransformation class
//! \details 160-bit MAC with 160-bit key
//! \sa MessageAuthenticationCode(), <a href="http://www.weidai.com/scan-mirror/mac.html#TTMAC">Two-Track-MAC</a>
DOCUMENTED_TYPEDEF(MessageAuthenticationCodeFinal<TTMAC_Base>, TTMAC)

NAMESPACE_END

#endif
