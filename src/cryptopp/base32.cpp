// base32.cpp - written and placed in the public domain by Frank Palazzolo, based on hex.cpp by Wei Dai

#include "pch.h"
#include "base32.h"

NAMESPACE_BEGIN(CryptoPP)

namespace
{
	const byte s_vecUpper[] = "ABCDEFGHIJKMNPQRSTUVWXYZ23456789";
	const byte s_vecLower[] = "abcdefghijkmnpqrstuvwxyz23456789";
}

void Base32Encoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	bool uppercase = parameters.GetValueWithDefault(Name::Uppercase(), true);
	m_filter->Initialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::EncodingLookupArray(), uppercase ? &s_vecUpper[0] : &s_vecLower[0], false)(Name::Log2Base(), 5, true)));
}

void Base32Decoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	BaseN_Decoder::IsolatedInitialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::DecodingLookupArray(), GetDefaultDecodingLookupArray(), false)(Name::Log2Base(), 5, true)));
}

const int *Base32Decoder::GetDefaultDecodingLookupArray()
{
	static volatile bool s_initialized = false;
	static int s_array[256];

	if (!s_initialized)
	{
		InitializeDecodingLookupArray(s_array, s_vecUpper, 32, true);
		s_initialized = true;
	}
	return s_array;
}

NAMESPACE_END
