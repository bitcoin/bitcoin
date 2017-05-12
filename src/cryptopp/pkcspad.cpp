// pkcspad.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_PKCSPAD_CPP	// SunCC workaround: compiler could cause this file to be included twice
#define CRYPTOPP_PKCSPAD_CPP

#include "pkcspad.h"
#include "emsa2.h"
#include "misc.h"
#include "trap.h"

NAMESPACE_BEGIN(CryptoPP)

// More in dll.cpp. Typedef/cast change due to Clang, http://github.com/weidai11/cryptopp/issues/300
template<> const byte PKCS_DigestDecoration<Weak1::MD2>::decoration[] = {0x30,0x20,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x02,0x05,0x00,0x04,0x10};
template<> const unsigned int PKCS_DigestDecoration<Weak1::MD2>::length = (unsigned int)sizeof(PKCS_DigestDecoration<Weak1::MD2>::decoration);

template<> const byte PKCS_DigestDecoration<Weak1::MD5>::decoration[] = {0x30,0x20,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x04,0x10};
template<> const unsigned int PKCS_DigestDecoration<Weak1::MD5>::length = (unsigned int)sizeof(PKCS_DigestDecoration<Weak1::MD5>::decoration);

template<> const byte PKCS_DigestDecoration<RIPEMD160>::decoration[] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2b,0x24,0x03,0x02,0x01,0x05,0x00,0x04,0x14};
template<> const unsigned int PKCS_DigestDecoration<RIPEMD160>::length = (unsigned int)sizeof(PKCS_DigestDecoration<RIPEMD160>::decoration);

template<> const byte PKCS_DigestDecoration<Tiger>::decoration[] = {0x30,0x29,0x30,0x0D,0x06,0x09,0x2B,0x06,0x01,0x04,0x01,0xDA,0x47,0x0C,0x02,0x05,0x00,0x04,0x18};
template<> const unsigned int PKCS_DigestDecoration<Tiger>::length = (unsigned int)sizeof(PKCS_DigestDecoration<Tiger>::decoration);

// Inclusion based on DLL due to Clang, http://github.com/weidai11/cryptopp/issues/300
#ifndef CRYPTOPP_IS_DLL
template<> const byte PKCS_DigestDecoration<SHA1>::decoration[] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x0E,0x03,0x02,0x1A,0x05,0x00,0x04,0x14};
template<> const unsigned int PKCS_DigestDecoration<SHA1>::length = (unsigned int)sizeof(PKCS_DigestDecoration<SHA1>::decoration);

template<> const byte PKCS_DigestDecoration<SHA224>::decoration[] = {0x30,0x2d,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x04,0x05,0x00,0x04,0x1c};
template<> const unsigned int PKCS_DigestDecoration<SHA224>::length = (unsigned int)sizeof(PKCS_DigestDecoration<SHA224>::decoration);

template<> const byte PKCS_DigestDecoration<SHA256>::decoration[] = {0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};
template<> const unsigned int PKCS_DigestDecoration<SHA256>::length = (unsigned int)sizeof(PKCS_DigestDecoration<SHA256>::decoration);

template<> const byte PKCS_DigestDecoration<SHA384>::decoration[] = {0x30,0x41,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x02,0x05,0x00,0x04,0x30};
template<> const unsigned int PKCS_DigestDecoration<SHA384>::length = (unsigned int)sizeof(PKCS_DigestDecoration<SHA384>::decoration);

template<> const byte PKCS_DigestDecoration<SHA512>::decoration[] = {0x30,0x51,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x03,0x05,0x00,0x04,0x40};
template<> const unsigned int PKCS_DigestDecoration<SHA512>::length = (unsigned int)sizeof(PKCS_DigestDecoration<SHA512>::decoration);

template<> const byte EMSA2HashId<SHA1>::id = 0x33;
template<> const byte EMSA2HashId<SHA224>::id = 0x38;
template<> const byte EMSA2HashId<SHA256>::id = 0x34;
template<> const byte EMSA2HashId<SHA384>::id = 0x36;
template<> const byte EMSA2HashId<SHA512>::id = 0x35;
#endif

size_t PKCS_EncryptionPaddingScheme::MaxUnpaddedLength(size_t paddedLength) const
{
	return SaturatingSubtract(paddedLength/8, 10U);
}

void PKCS_EncryptionPaddingScheme::Pad(RandomNumberGenerator& rng, const byte *input, size_t inputLen, byte *pkcsBlock, size_t pkcsBlockLen, const NameValuePairs& parameters) const
{
	CRYPTOPP_UNUSED(parameters);
	CRYPTOPP_ASSERT (inputLen <= MaxUnpaddedLength(pkcsBlockLen));	// this should be checked by caller

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		pkcsBlock[0] = 0;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	pkcsBlock[0] = 2;  // block type 2

	// pad with non-zero random bytes
	for (unsigned i = 1; i < pkcsBlockLen-inputLen-1; i++)
		pkcsBlock[i] = (byte)rng.GenerateWord32(1, 0xff);

	pkcsBlock[pkcsBlockLen-inputLen-1] = 0;     // separator
	memcpy(pkcsBlock+pkcsBlockLen-inputLen, input, inputLen);
}

DecodingResult PKCS_EncryptionPaddingScheme::Unpad(const byte *pkcsBlock, size_t pkcsBlockLen, byte *output, const NameValuePairs& parameters) const
{
	CRYPTOPP_UNUSED(parameters);
	bool invalid = false;
	size_t maxOutputLen = MaxUnpaddedLength(pkcsBlockLen);

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		invalid = (pkcsBlock[0] != 0) || invalid;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	// Require block type 2.
	invalid = (pkcsBlock[0] != 2) || invalid;

	// skip past the padding until we find the separator
	size_t i=1;
	while (i<pkcsBlockLen && pkcsBlock[i++]) { // null body
		}
	CRYPTOPP_ASSERT(i==pkcsBlockLen || pkcsBlock[i-1]==0);

	size_t outputLen = pkcsBlockLen - i;
	invalid = (outputLen > maxOutputLen) || invalid;

	if (invalid)
		return DecodingResult();

	memcpy (output, pkcsBlock+i, outputLen);
	return DecodingResult(outputLen);
}

// ********************************************************

#ifndef CRYPTOPP_IMPORTS

void PKCS1v15_SignatureMessageEncodingMethod::ComputeMessageRepresentative(RandomNumberGenerator &rng,
	const byte *recoverableMessage, size_t recoverableMessageLength,
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength) const
{
	CRYPTOPP_UNUSED(rng), CRYPTOPP_UNUSED(recoverableMessage), CRYPTOPP_UNUSED(recoverableMessageLength);
	CRYPTOPP_UNUSED(messageEmpty), CRYPTOPP_UNUSED(hashIdentifier);
	CRYPTOPP_ASSERT(representativeBitLength >= MinRepresentativeBitLength(hashIdentifier.second, hash.DigestSize()));

	size_t pkcsBlockLen = representativeBitLength;
	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		representative[0] = 0;
		representative++;
	}
	pkcsBlockLen /= 8;

	representative[0] = 1;   // block type 1

	unsigned int digestSize = hash.DigestSize();
	byte *pPadding = representative + 1;
	byte *pDigest = representative + pkcsBlockLen - digestSize;
	byte *pHashId = pDigest - hashIdentifier.second;
	byte *pSeparator = pHashId - 1;

	// pad with 0xff
	memset(pPadding, 0xff, pSeparator-pPadding);
	*pSeparator = 0;
	memcpy(pHashId, hashIdentifier.first, hashIdentifier.second);
	hash.Final(pDigest);
}

#endif

NAMESPACE_END

#endif
