// pssr.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "pssr.h"
#include "misc.h"

#include <functional>

NAMESPACE_BEGIN(CryptoPP)

// more in dll.cpp
template<> const byte EMSA2HashId<RIPEMD160>::id = 0x31;
template<> const byte EMSA2HashId<RIPEMD128>::id = 0x32;
template<> const byte EMSA2HashId<Whirlpool>::id = 0x37;

#ifndef CRYPTOPP_IMPORTS

size_t PSSR_MEM_Base::MinRepresentativeBitLength(size_t hashIdentifierLength, size_t digestLength) const
{
	size_t saltLen = SaltLen(digestLength);
	size_t minPadLen = MinPadLen(digestLength);
	return 9 + 8*(minPadLen + saltLen + digestLength + hashIdentifierLength);
}

size_t PSSR_MEM_Base::MaxRecoverableLength(size_t representativeBitLength, size_t hashIdentifierLength, size_t digestLength) const
{
	if (AllowRecovery())
		return SaturatingSubtract(representativeBitLength, MinRepresentativeBitLength(hashIdentifierLength, digestLength)) / 8;
	return 0;
}

bool PSSR_MEM_Base::IsProbabilistic() const
{
	return SaltLen(1) > 0;
}

bool PSSR_MEM_Base::AllowNonrecoverablePart() const
{
	return true;
}

bool PSSR_MEM_Base::RecoverablePartFirst() const
{
	return false;
}

void PSSR_MEM_Base::ComputeMessageRepresentative(RandomNumberGenerator &rng,
	const byte *recoverableMessage, size_t recoverableMessageLength,
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength) const
{
	CRYPTOPP_UNUSED(rng), CRYPTOPP_UNUSED(recoverableMessage), CRYPTOPP_UNUSED(recoverableMessageLength);
	CRYPTOPP_UNUSED(messageEmpty), CRYPTOPP_UNUSED(hashIdentifier);
	CRYPTOPP_ASSERT(representativeBitLength >= MinRepresentativeBitLength(hashIdentifier.second, hash.DigestSize()));

	const size_t u = hashIdentifier.second + 1;
	const size_t representativeByteLength = BitsToBytes(representativeBitLength);
	const size_t digestSize = hash.DigestSize();
	const size_t saltSize = SaltLen(digestSize);
	byte *const h = representative + representativeByteLength - u - digestSize;

	SecByteBlock digest(digestSize), salt(saltSize);
	hash.Final(digest);
	rng.GenerateBlock(salt, saltSize);

	// compute H = hash of M'
	byte c[8];
	PutWord(false, BIG_ENDIAN_ORDER, c, (word32)SafeRightShift<29>(recoverableMessageLength));
	PutWord(false, BIG_ENDIAN_ORDER, c+4, word32(recoverableMessageLength << 3));
	hash.Update(c, 8);
	hash.Update(recoverableMessage, recoverableMessageLength);
	hash.Update(digest, digestSize);
	hash.Update(salt, saltSize);
	hash.Final(h);

	// compute representative
	GetMGF().GenerateAndMask(hash, representative, representativeByteLength - u - digestSize, h, digestSize, false);
	byte *xorStart = representative + representativeByteLength - u - digestSize - salt.size() - recoverableMessageLength - 1;
	xorStart[0] ^= 1;
	if (recoverableMessage && recoverableMessageLength)
		xorbuf(xorStart + 1, recoverableMessage, recoverableMessageLength);
	xorbuf(xorStart + 1 + recoverableMessageLength, salt, salt.size());
	if (hashIdentifier.first && hashIdentifier.second)
	{
		memcpy(representative + representativeByteLength - u, hashIdentifier.first, hashIdentifier.second);
		representative[representativeByteLength - 1] = 0xcc;
	}
	else
	{
		representative[representativeByteLength - 1] = 0xbc;
	}
	if (representativeBitLength % 8 != 0)
		representative[0] = (byte)Crop(representative[0], representativeBitLength % 8);
}

DecodingResult PSSR_MEM_Base::RecoverMessageFromRepresentative(
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength,
	byte *recoverableMessage) const
{
	CRYPTOPP_UNUSED(recoverableMessage), CRYPTOPP_UNUSED(messageEmpty), CRYPTOPP_UNUSED(hashIdentifier);
	CRYPTOPP_ASSERT(representativeBitLength >= MinRepresentativeBitLength(hashIdentifier.second, hash.DigestSize()));

	const size_t u = hashIdentifier.second + 1;
	const size_t representativeByteLength = BitsToBytes(representativeBitLength);
	const size_t digestSize = hash.DigestSize();
	const size_t saltSize = SaltLen(digestSize);
	const byte *const h = representative + representativeByteLength - u - digestSize;

	SecByteBlock digest(digestSize);
	hash.Final(digest);

	DecodingResult result(0);
	bool &valid = result.isValidCoding;
	size_t &recoverableMessageLength = result.messageLength;

	valid = (representative[representativeByteLength - 1] == (hashIdentifier.second ? 0xcc : 0xbc)) && valid;

	if (hashIdentifier.first && hashIdentifier.second)
		valid = VerifyBufsEqual(representative + representativeByteLength - u, hashIdentifier.first, hashIdentifier.second) && valid;

	GetMGF().GenerateAndMask(hash, representative, representativeByteLength - u - digestSize, h, digestSize);
	if (representativeBitLength % 8 != 0)
		representative[0] = (byte)Crop(representative[0], representativeBitLength % 8);

	// extract salt and recoverableMessage from DB = 00 ... || 01 || M || salt
	byte *salt = representative + representativeByteLength - u - digestSize - saltSize;
	byte *M = std::find_if(representative, salt-1, std::bind2nd(std::not_equal_to<byte>(), byte(0)));
	recoverableMessageLength = salt-M-1;
	if (*M == 0x01 &&
	   (size_t)(M - representative - (representativeBitLength % 8 != 0)) >= MinPadLen(digestSize) &&
	   recoverableMessageLength <= MaxRecoverableLength(representativeBitLength, hashIdentifier.second, digestSize))
	{
		if (recoverableMessage)
			memcpy(recoverableMessage, M+1, recoverableMessageLength);
	}
	else
	{
		recoverableMessageLength = 0;
		valid = false;
	}

	// verify H = hash of M'
	byte c[8];
	PutWord(false, BIG_ENDIAN_ORDER, c, (word32)SafeRightShift<29>(recoverableMessageLength));
	PutWord(false, BIG_ENDIAN_ORDER, c+4, word32(recoverableMessageLength << 3));
	hash.Update(c, 8);
	hash.Update(recoverableMessage, recoverableMessageLength);
	hash.Update(digest, digestSize);
	hash.Update(salt, saltSize);
	valid = hash.Verify(h) && valid;

	if (!AllowRecovery() && valid && recoverableMessageLength != 0)
		{throw NotImplemented("PSSR_MEM: message recovery disabled");}

	return result;
}

#endif

NAMESPACE_END
