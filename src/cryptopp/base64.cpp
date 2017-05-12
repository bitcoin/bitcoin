// base64.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "base64.h"

NAMESPACE_BEGIN(CryptoPP)

namespace
{
	const byte s_stdVec[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const byte s_urlVec[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	const byte s_padding = '=';
}

void Base64Encoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	bool insertLineBreaks = parameters.GetValueWithDefault(Name::InsertLineBreaks(), true);
	int maxLineLength = parameters.GetIntValueWithDefault(Name::MaxLineLength(), 72);

	const char *lineBreak = insertLineBreaks ? "\n" : "";

	m_filter->Initialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::EncodingLookupArray(), &s_stdVec[0], false)
			(Name::PaddingByte(), s_padding)
			(Name::GroupSize(), insertLineBreaks ? maxLineLength : 0)
			(Name::Separator(), ConstByteArrayParameter(lineBreak))
			(Name::Terminator(), ConstByteArrayParameter(lineBreak))
			(Name::Log2Base(), 6, true)));
}

void Base64URLEncoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	bool insertLineBreaks = parameters.GetValueWithDefault(Name::InsertLineBreaks(), true);
	int maxLineLength = parameters.GetIntValueWithDefault(Name::MaxLineLength(), 72);

	const char *lineBreak = insertLineBreaks ? "\n" : "";

	m_filter->Initialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::EncodingLookupArray(), &s_urlVec[0], false)
			(Name::PaddingByte(), s_padding)
			(Name::GroupSize(), insertLineBreaks ? maxLineLength : 0)
			(Name::Separator(), ConstByteArrayParameter(lineBreak))
			(Name::Terminator(), ConstByteArrayParameter(lineBreak))
			(Name::Log2Base(), 6, true)));
}

void Base64Decoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	BaseN_Decoder::IsolatedInitialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::DecodingLookupArray(), GetDecodingLookupArray(), false)(Name::Log2Base(), 6, true)));
}

const int *Base64Decoder::GetDecodingLookupArray()
{
	static volatile bool s_initialized = false;
	static int s_array[256];

	if (!s_initialized)
	{
		InitializeDecodingLookupArray(s_array, s_stdVec, 64, false);
		s_initialized = true;
	}
	return s_array;
}

void Base64URLDecoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	BaseN_Decoder::IsolatedInitialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::DecodingLookupArray(), GetDecodingLookupArray(), false)(Name::Log2Base(), 6, true)));
}

const int *Base64URLDecoder::GetDecodingLookupArray()
{
	static volatile bool s_initialized = false;
	static int s_array[256];

	if (!s_initialized)
	{
		InitializeDecodingLookupArray(s_array, s_urlVec, 64, false);
		s_initialized = true;
	}
	return s_array;
}

NAMESPACE_END
