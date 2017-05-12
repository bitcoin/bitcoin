// oaep.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "oaep.h"
#include "stdcpp.h"
#include "smartptr.h"

NAMESPACE_BEGIN(CryptoPP)

// ********************************************************

size_t OAEP_Base::MaxUnpaddedLength(size_t paddedLength) const
{
	return SaturatingSubtract(paddedLength/8, 1+2*DigestSize());
}

void OAEP_Base::Pad(RandomNumberGenerator &rng, const byte *input, size_t inputLength, byte *oaepBlock, size_t oaepBlockLen, const NameValuePairs &parameters) const
{
	CRYPTOPP_ASSERT (inputLength <= MaxUnpaddedLength(oaepBlockLen));

	// convert from bit length to byte length
	if (oaepBlockLen % 8 != 0)
	{
		oaepBlock[0] = 0;
		oaepBlock++;
	}
	oaepBlockLen /= 8;

	member_ptr<HashTransformation> pHash(NewHash());
	const size_t hLen = pHash->DigestSize();
	const size_t seedLen = hLen, dbLen = oaepBlockLen-seedLen;
	byte *const maskedSeed = oaepBlock;
	byte *const maskedDB = oaepBlock+seedLen;

	ConstByteArrayParameter encodingParameters;
	parameters.GetValue(Name::EncodingParameters(), encodingParameters);

	// DB = pHash || 00 ... || 01 || M
	pHash->CalculateDigest(maskedDB, encodingParameters.begin(), encodingParameters.size());
	memset(maskedDB+hLen, 0, dbLen-hLen-inputLength-1);
	maskedDB[dbLen-inputLength-1] = 0x01;
	memcpy(maskedDB+dbLen-inputLength, input, inputLength);

	rng.GenerateBlock(maskedSeed, seedLen);
	member_ptr<MaskGeneratingFunction> pMGF(NewMGF());
	pMGF->GenerateAndMask(*pHash, maskedDB, dbLen, maskedSeed, seedLen);
	pMGF->GenerateAndMask(*pHash, maskedSeed, seedLen, maskedDB, dbLen);
}

DecodingResult OAEP_Base::Unpad(const byte *oaepBlock, size_t oaepBlockLen, byte *output, const NameValuePairs &parameters) const
{
	bool invalid = false;

	// convert from bit length to byte length
	if (oaepBlockLen % 8 != 0)
	{
		invalid = (oaepBlock[0] != 0) || invalid;
		oaepBlock++;
	}
	oaepBlockLen /= 8;

	member_ptr<HashTransformation> pHash(NewHash());
	const size_t hLen = pHash->DigestSize();
	const size_t seedLen = hLen, dbLen = oaepBlockLen-seedLen;

	invalid = (oaepBlockLen < 2*hLen+1) || invalid;

	SecByteBlock t(oaepBlock, oaepBlockLen);
	byte *const maskedSeed = t;
	byte *const maskedDB = t+seedLen;

	member_ptr<MaskGeneratingFunction> pMGF(NewMGF());
	pMGF->GenerateAndMask(*pHash, maskedSeed, seedLen, maskedDB, dbLen);
	pMGF->GenerateAndMask(*pHash, maskedDB, dbLen, maskedSeed, seedLen);

	ConstByteArrayParameter encodingParameters;
	parameters.GetValue(Name::EncodingParameters(), encodingParameters);

	// DB = pHash' || 00 ... || 01 || M
	byte *M = std::find(maskedDB+hLen, maskedDB+dbLen, 0x01);
	invalid = (M == maskedDB+dbLen) || invalid;
	invalid = (std::find_if(maskedDB+hLen, M, std::bind2nd(std::not_equal_to<byte>(), byte(0))) != M) || invalid;
	invalid = !pHash->VerifyDigest(maskedDB, encodingParameters.begin(), encodingParameters.size()) || invalid;

	if (invalid)
		return DecodingResult();

	M++;
	memcpy(output, M, maskedDB+dbLen-M);
	return DecodingResult(maskedDB+dbLen-M);
}

NAMESPACE_END

#endif
