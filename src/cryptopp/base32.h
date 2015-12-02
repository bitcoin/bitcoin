// base32.h - written and placed in the public domain by Wei Dai

//! \file
//! \brief Classes for Base32Encoder and Base32Decoder

#ifndef CRYPTOPP_BASE32_H
#define CRYPTOPP_BASE32_H

#include "cryptlib.h"
#include "basecode.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Base32Encoder
//! \brief Base32 encodes data
//! \details Converts data to base32. The default code is based on draft-ietf-idn-dude-02.txt.
//! \details To specify alternative alpahabet or code, call Initialize() with EncodingLookupArray parameter.
class Base32Encoder : public SimpleProxyFilter
{
public:
	//! \brief Construct a Base32Encoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \param uppercase a flag indicating uppercase output
	//! \param groupSize the size of the grouping
	//! \param separator the separator to use between groups
	//! \param terminator the terminator appeand after processing
	//! \details Base32Encoder() constructs a default encoder. The constructor lacks fields for padding and
	//!   line breaks. You must use IsolatedInitialize() to change the default padding character or suppress it.
	//! \sa IsolatedInitialize() for an example of modifying a Base32Encoder after construction.
	Base32Encoder(BufferedTransformation *attachment = NULL, bool uppercase = true, int groupSize = 0, const std::string &separator = ":", const std::string &terminator = "")
		: SimpleProxyFilter(new BaseN_Encoder(new Grouper), attachment)
	{
		IsolatedInitialize(MakeParameters(Name::Uppercase(), uppercase)(Name::GroupSize(), groupSize)(Name::Separator(), ConstByteArrayParameter(separator))(Name::Terminator(), ConstByteArrayParameter(terminator)));
	}

	//! \brief Initialize or reinitialize this object, without signal propagation
	//! \param parameters a set of NameValuePairs used to initialize this object
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on attached
	//!   transformations. If initialization should be propagated, then use the Initialize() function.
	//! \details The following code modifies the padding and line break parameters for an encoder:
	//!   <pre>
	//!     Base32Encoder encoder;
	//!     AlgorithmParameters params = MakeParameters(Pad(), false)(InsertLineBreaks(), false);
	//!     encoder.IsolatedInitialize(params);
	//!   </pre>
	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! \class Base32Decoder
//! \brief Base32 decodes data
//! \details Decode base32 data. The default code is based on draft-ietf-idn-dude-02.txt
//! \details To specify alternative alpahabet or code, call Initialize() with EncodingLookupArray parameter.
class Base32Decoder : public BaseN_Decoder
{
public:
	//! \brief Construct a Base32Decoder
	//! \param attachment a BufferedTrasformation to attach to this object
	Base32Decoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDefaultDecodingLookupArray(), 5, attachment) {}

	void IsolatedInitialize(const NameValuePairs &parameters);

private:
	static const int * CRYPTOPP_API GetDefaultDecodingLookupArray();
};

NAMESPACE_END

#endif
