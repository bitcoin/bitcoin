// dh2.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "dh2.h"

NAMESPACE_BEGIN(CryptoPP)

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void DH2_TestInstantiations()
{
	DH2 dh(*(SimpleKeyAgreementDomain*)NULL);
}
#endif

bool DH2::Agree(byte *agreedValue,
		const byte *staticSecretKey, const byte *ephemeralSecretKey,
		const byte *staticOtherPublicKey, const byte *ephemeralOtherPublicKey,
		bool validateStaticOtherPublicKey) const
{
	return d1.Agree(agreedValue, staticSecretKey, staticOtherPublicKey, validateStaticOtherPublicKey)
		&& d2.Agree(agreedValue+d1.AgreedValueLength(), ephemeralSecretKey, ephemeralOtherPublicKey, true);
}

NAMESPACE_END
