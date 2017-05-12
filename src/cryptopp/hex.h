// hex.h - written and placed in the public domain by Wei Dai

//! \file hex.h
//! \brief Classes for HexEncoder and HexDecoder

#ifndef CRYPTOPP_HEX_H
#define CRYPTOPP_HEX_H

#include "cryptlib.h"
#include "basecode.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class HexEncoder
//! \brief Converts given data to base 16
class CRYPTOPP_DLL HexEncoder : public SimpleProxyFilter
{
public:
	//! \brief Construct a HexEncoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \param uppercase a flag indicating uppercase output
	//! \param groupSize the size of the output grouping
	//! \param separator the separator to use between groups
	//! \param terminator the terminator append after processing
	HexEncoder(BufferedTransformation *attachment = NULL, bool uppercase = true, int groupSize = 0, const std::string &separator = ":", const std::string &terminator = "")
		: SimpleProxyFilter(new BaseN_Encoder(new Grouper), attachment)
	{
		IsolatedInitialize(MakeParameters(Name::Uppercase(), uppercase)(Name::GroupSize(), groupSize)(Name::Separator(), ConstByteArrayParameter(separator))(Name::Terminator(), ConstByteArrayParameter(terminator)));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! \class HexDecoder
//! \brief Decode base 16 data back to bytes
class CRYPTOPP_DLL HexDecoder : public BaseN_Decoder
{
public:
	//! \brief Construct a HexDecoder
	//! \param attachment a BufferedTrasformation to attach to this object
	HexDecoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDefaultDecodingLookupArray(), 4, attachment) {}

	void IsolatedInitialize(const NameValuePairs &parameters);

private:
	static const int * CRYPTOPP_API GetDefaultDecodingLookupArray();
};

NAMESPACE_END

#endif
