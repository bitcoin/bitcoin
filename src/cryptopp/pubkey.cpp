// pubkey.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#ifndef CRYPTOPP_IMPORTS

#include "pubkey.h"
#include "integer.h"
#include "filters.h"

NAMESPACE_BEGIN(CryptoPP)

void P1363_MGF1KDF2_Common(HashTransformation &hash, byte *output, size_t outputLength, const byte *input, size_t inputLength, const byte *derivationParams, size_t derivationParamsLength, bool mask, unsigned int counterStart)
{
	ArraySink *sink;
	HashFilter filter(hash, sink = mask ? new ArrayXorSink(output, outputLength) : new ArraySink(output, outputLength));
	word32 counter = counterStart;
	while (sink->AvailableSize() > 0)
	{
		filter.Put(input, inputLength);
		filter.PutWord32(counter++);
		filter.Put(derivationParams, derivationParamsLength);
		filter.MessageEnd();
	}
}

bool PK_DeterministicSignatureMessageEncodingMethod::VerifyMessageRepresentative(
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength) const
{
	SecByteBlock computedRepresentative(BitsToBytes(representativeBitLength));
	ComputeMessageRepresentative(NullRNG(), NULL, 0, hash, hashIdentifier, messageEmpty, computedRepresentative, representativeBitLength);
	return VerifyBufsEqual(representative, computedRepresentative, computedRepresentative.size());
}

bool PK_RecoverableSignatureMessageEncodingMethod::VerifyMessageRepresentative(
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength) const
{
	SecByteBlock recoveredMessage(MaxRecoverableLength(representativeBitLength, hashIdentifier.second, hash.DigestSize()));
	DecodingResult result = RecoverMessageFromRepresentative(
		hash, hashIdentifier, messageEmpty, representative, representativeBitLength, recoveredMessage);
	return result.isValidCoding && result.messageLength == 0;
}

void TF_SignerBase::InputRecoverableMessage(PK_MessageAccumulator &messageAccumulator, const byte *recoverableMessage, size_t recoverableMessageLength) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	HashIdentifier id = GetHashIdentifier();
	const MessageEncodingInterface &encoding = GetMessageEncodingInterface();

	if (MessageRepresentativeBitLength() < encoding.MinRepresentativeBitLength(id.second, ma.AccessHash().DigestSize()))
		throw PK_SignatureScheme::KeyTooShort();

	size_t maxRecoverableLength = encoding.MaxRecoverableLength(MessageRepresentativeBitLength(), GetHashIdentifier().second, ma.AccessHash().DigestSize());

	if (maxRecoverableLength == 0)
		{throw NotImplemented("TF_SignerBase: this algorithm does not support messsage recovery or the key is too short");}
	if (recoverableMessageLength > maxRecoverableLength)
		throw InvalidArgument("TF_SignerBase: the recoverable message part is too long for the given key and algorithm");

	ma.m_recoverableMessage.Assign(recoverableMessage, recoverableMessageLength);
	encoding.ProcessRecoverableMessage(
		ma.AccessHash(), 
		recoverableMessage, recoverableMessageLength,
		NULL, 0, ma.m_semisignature);
}

size_t TF_SignerBase::SignAndRestart(RandomNumberGenerator &rng, PK_MessageAccumulator &messageAccumulator, byte *signature, bool restart) const
{
	CRYPTOPP_UNUSED(restart);

	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	HashIdentifier id = GetHashIdentifier();
	const MessageEncodingInterface &encoding = GetMessageEncodingInterface();

	if (MessageRepresentativeBitLength() < encoding.MinRepresentativeBitLength(id.second, ma.AccessHash().DigestSize()))
		throw PK_SignatureScheme::KeyTooShort();

	SecByteBlock representative(MessageRepresentativeLength());
	encoding.ComputeMessageRepresentative(rng, 
		ma.m_recoverableMessage, ma.m_recoverableMessage.size(), 
		ma.AccessHash(), id, ma.m_empty,
		representative, MessageRepresentativeBitLength());
	ma.m_empty = true;

	Integer r(representative, representative.size());
	size_t signatureLength = SignatureLength();
	GetTrapdoorFunctionInterface().CalculateRandomizedInverse(rng, r).Encode(signature, signatureLength);
	return signatureLength;
}

void TF_VerifierBase::InputSignature(PK_MessageAccumulator &messageAccumulator, const byte *signature, size_t signatureLength) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	HashIdentifier id = GetHashIdentifier();
	const MessageEncodingInterface &encoding = GetMessageEncodingInterface();

	if (MessageRepresentativeBitLength() < encoding.MinRepresentativeBitLength(id.second, ma.AccessHash().DigestSize()))
		throw PK_SignatureScheme::KeyTooShort();

	ma.m_representative.New(MessageRepresentativeLength());
	Integer x = GetTrapdoorFunctionInterface().ApplyFunction(Integer(signature, signatureLength));
	if (x.BitCount() > MessageRepresentativeBitLength())
		x = Integer::Zero();	// don't return false here to prevent timing attack
	x.Encode(ma.m_representative, ma.m_representative.size());
}

bool TF_VerifierBase::VerifyAndRestart(PK_MessageAccumulator &messageAccumulator) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	HashIdentifier id = GetHashIdentifier();
	const MessageEncodingInterface &encoding = GetMessageEncodingInterface();

	if (MessageRepresentativeBitLength() < encoding.MinRepresentativeBitLength(id.second, ma.AccessHash().DigestSize()))
		throw PK_SignatureScheme::KeyTooShort();

	bool result = encoding.VerifyMessageRepresentative(
		ma.AccessHash(), id, ma.m_empty, ma.m_representative, MessageRepresentativeBitLength());
	ma.m_empty = true;
	return result;
}

DecodingResult TF_VerifierBase::RecoverAndRestart(byte *recoveredMessage, PK_MessageAccumulator &messageAccumulator) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	HashIdentifier id = GetHashIdentifier();
	const MessageEncodingInterface &encoding = GetMessageEncodingInterface();

	if (MessageRepresentativeBitLength() < encoding.MinRepresentativeBitLength(id.second, ma.AccessHash().DigestSize()))
		throw PK_SignatureScheme::KeyTooShort();

	DecodingResult result = encoding.RecoverMessageFromRepresentative(
		ma.AccessHash(), id, ma.m_empty, ma.m_representative, MessageRepresentativeBitLength(), recoveredMessage);
	ma.m_empty = true;
	return result;
}

DecodingResult TF_DecryptorBase::Decrypt(RandomNumberGenerator &rng, const byte *ciphertext, size_t ciphertextLength, byte *plaintext, const NameValuePairs &parameters) const
{
	if (ciphertextLength != FixedCiphertextLength())
			throw InvalidArgument(AlgorithmName() + ": ciphertext length of " + IntToString(ciphertextLength) + " doesn't match the required length of " + IntToString(FixedCiphertextLength()) + " for this key");

	SecByteBlock paddedBlock(PaddedBlockByteLength());
	Integer x = GetTrapdoorFunctionInterface().CalculateInverse(rng, Integer(ciphertext, ciphertextLength));
	if (x.ByteCount() > paddedBlock.size())
		x = Integer::Zero();	// don't return false here to prevent timing attack
	x.Encode(paddedBlock, paddedBlock.size());
	return GetMessageEncodingInterface().Unpad(paddedBlock, PaddedBlockBitLength(), plaintext, parameters);
}

void TF_EncryptorBase::Encrypt(RandomNumberGenerator &rng, const byte *plaintext, size_t plaintextLength, byte *ciphertext, const NameValuePairs &parameters) const
{
	if (plaintextLength > FixedMaxPlaintextLength())
	{
		if (FixedMaxPlaintextLength() < 1)
			throw InvalidArgument(AlgorithmName() + ": this key is too short to encrypt any messages");
		else
			throw InvalidArgument(AlgorithmName() + ": message length of " + IntToString(plaintextLength) + " exceeds the maximum of " + IntToString(FixedMaxPlaintextLength()) + " for this public key");
	}

	SecByteBlock paddedBlock(PaddedBlockByteLength());
	GetMessageEncodingInterface().Pad(rng, plaintext, plaintextLength, paddedBlock, PaddedBlockBitLength(), parameters);
	GetTrapdoorFunctionInterface().ApplyRandomizedFunction(rng, Integer(paddedBlock, paddedBlock.size())).Encode(ciphertext, FixedCiphertextLength());
}

NAMESPACE_END

#endif
