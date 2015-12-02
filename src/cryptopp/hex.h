// hex.h - written and placed in the public domain by Wei Dai

//! \file
//! \brief Classes for HexEncoder and HexDecoder

#ifndef CRYPTOPP_HEX_H
#define CRYPTOPP_HEX_H

#include "cryptlib.h"
#include "basecode.h"

NAMESPACE_BEGIN(CryptoPP)

//! Converts given data to base 16
class CRYPTOPP_DLL HexEncoder : public SimpleProxyFilter
{
public:
	HexEncoder(BufferedTransformation *attachment = NULL, bool uppercase = true, int outputGroupSize = 0, const std::string &separator = ":", const std::string &terminator = "")
		: SimpleProxyFilter(new BaseN_Encoder(new Grouper), attachment)
	{
		IsolatedInitialize(MakeParameters(Name::Uppercase(), uppercase)(Name::GroupSize(), outputGroupSize)(Name::Separator(), ConstByteArrayParameter(separator))(Name::Terminator(), ConstByteArrayParameter(terminator)));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! Decode base 16 data back to bytes
class CRYPTOPP_DLL HexDecoder : public BaseN_Decoder
{
public:
	HexDecoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDefaultDecodingLookupArray(), 4, attachment) {}

	void IsolatedInitialize(const NameValuePairs &parameters);

private:
	static const int * CRYPTOPP_API GetDefaultDecodingLookupArray();
};

NAMESPACE_END

#endif
