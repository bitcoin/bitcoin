// dsa.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "dsa.h"
#include "asn.h"
#include "integer.h"
#include "filters.h"
#include "nbtheory.h"

NAMESPACE_BEGIN(CryptoPP)

size_t DSAConvertSignatureFormat(byte *buffer, size_t bufferSize, DSASignatureFormat toFormat, const byte *signature, size_t signatureLen, DSASignatureFormat fromFormat)
{
	Integer r, s;
	StringStore store(signature, signatureLen);
	ArraySink sink(buffer, bufferSize);

	switch (fromFormat)
	{
	case DSA_P1363:
		r.Decode(store, signatureLen/2);
		s.Decode(store, signatureLen/2);
		break;
	case DSA_DER:
	{
		BERSequenceDecoder seq(store);
		r.BERDecode(seq);
		s.BERDecode(seq);
		seq.MessageEnd();
		break;
	}
	case DSA_OPENPGP:
		r.OpenPGPDecode(store);
		s.OpenPGPDecode(store);
		break;
	}

	switch (toFormat)
	{
	case DSA_P1363:
		r.Encode(sink, bufferSize/2);
		s.Encode(sink, bufferSize/2);
		break;
	case DSA_DER:
	{
		DERSequenceEncoder seq(sink);
		r.DEREncode(seq);
		s.DEREncode(seq);
		seq.MessageEnd();
		break;
	}
	case DSA_OPENPGP:
		r.OpenPGPEncode(sink);
		s.OpenPGPEncode(sink);
		break;
	}

	return (size_t)sink.TotalPutLength();
}

NAMESPACE_END

#endif
