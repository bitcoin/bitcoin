// base64.h - written and placed in the public domain by Wei Dai

//! \file base64.h
//! \brief Classes for the Base64Encoder, Base64Decoder, Base64URLEncoder and Base64URLDecoder

#ifndef CRYPTOPP_BASE64_H
#define CRYPTOPP_BASE64_H

#include "cryptlib.h"
#include "basecode.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Base64Encoder
//! \brief Base64 encodes data
//! \details Base64 encodes data per RFC 4648 (http://tools.ietf.org/html/rfc4648#section-4)
//! \details To specify alternative alpahabet or code, call Initialize() with EncodingLookupArray parameter.
class Base64Encoder : public SimpleProxyFilter
{
public:
	//! \brief Construct a Base64Encoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \param insertLineBreaks a BufferedTrasformation to attach to this object
	//! \param maxLineLength the lenght of a line if line breaks are used
	//! \details Base64Encoder() constructs a default encoder. The constructor lacks parameters for padding.
	//!    You must use IsolatedInitialize() to modify the Base64Encoder after construction.
	//! \sa IsolatedInitialize() for an example of modifying a Base64Encoder after construction.
	Base64Encoder(BufferedTransformation *attachment = NULL, bool insertLineBreaks = true, int maxLineLength = 72)
		: SimpleProxyFilter(new BaseN_Encoder(new Grouper), attachment)
	{
		IsolatedInitialize(MakeParameters(Name::InsertLineBreaks(), insertLineBreaks)(Name::MaxLineLength(), maxLineLength));
	}

	//! \brief Initialize or reinitialize this object, without signal propagation
	//! \param parameters a set of NameValuePairs used to initialize this object
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on attached
	//!   transformations. If initialization should be propagated, then use the Initialize() function.
	//! \details The following code modifies the padding and line break parameters for an encoder:
	//!   <pre>
	//!     Base64Encoder encoder;
	//!     AlgorithmParameters params = MakeParameters(Pad(), false)(InsertLineBreaks(), false);
	//!     encoder.IsolatedInitialize(params);
	//!   </pre>
	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! \class Base64Decoder
//! \brief Base64 decodes data
//! \details Base64 decodes data per RFC 4648 (http://tools.ietf.org/html/rfc4648#section-4)
//! \details To specify alternative alpahabet or code, call Initialize() with EncodingLookupArray parameter.
class Base64Decoder : public BaseN_Decoder
{
public:
	//! \brief Construct a Base64Decoder
	//! \param attachment a BufferedTrasformation to attach to this object
	Base64Decoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDecodingLookupArray(), 6, attachment) {}
    
	//! \brief Initialize or reinitialize this object, without signal propagation
	//! \param parameters a set of NameValuePairs used to initialize this object
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on
	//!  attached transformations. If initialization should be propagated, then use the Initialize() function.
	void IsolatedInitialize(const NameValuePairs &parameters)
		{CRYPTOPP_UNUSED(parameters);}
    
private:
	static const int * CRYPTOPP_API GetDecodingLookupArray();
};

//! \class Base64URLEncoder
//! \brief Base64 encodes data using a web safe alphabet
//! \details Base64 encodes data using a web safe alphabet per RFC 4648 (http://tools.ietf.org/html/rfc4648#section-5)
//! \details To specify alternative alpahabet or code, call Initialize() with EncodingLookupArray parameter.
class Base64URLEncoder : public SimpleProxyFilter
{
public:
	//! \brief Construct a Base64URLEncoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \param insertLineBreaks a BufferedTrasformation to attach to this object
	//! \param maxLineLength the lenght of a line if line breaks are used
	//! \details Base64URLEncoder() constructs a default encoder. The constructor ignores insertLineBreaks
	//!   and maxLineLength because the web and URL safe specifications don't use them. They are present
	//!   in the constructor for API compatibility with Base64Encoder (drop-in replacement). The
	//!   constructor also disables padding on the encoder for the same reason.
	//! \details If you need line breaks or padding, then you must use IsolatedInitialize() to set them
	//!   after constructing a Base64URLEncoder.
	//! \sa IsolatedInitialize() for an example of modifying a Base64URLEncoder after construction.
	Base64URLEncoder(BufferedTransformation *attachment = NULL, bool insertLineBreaks = false, int maxLineLength = -1)
		: SimpleProxyFilter(new BaseN_Encoder(new Grouper), attachment)
	{
		CRYPTOPP_UNUSED(insertLineBreaks), CRYPTOPP_UNUSED(maxLineLength);
		IsolatedInitialize(MakeParameters(Name::InsertLineBreaks(), false)(Name::MaxLineLength(), -1)(Name::Pad(),false));
	}
    
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on attached
	//!   transformations. If initialization should be propagated, then use the Initialize() function.
	//! \details The following code modifies the padding and line break parameters for an encoder:
	//!   <pre>
	//!     Base64URLEncoder encoder;
	//!     AlgorithmParameters params = MakeParameters(Name::Pad(), true)(Name::InsertLineBreaks(), true);
	//!     encoder.IsolatedInitialize(params);
	//!   </pre>
	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! \class Base64URLDecoder
//! \brief Base64 decodes data using a web safe alphabet
//! \details Base64 decodes data using a web safe alphabet per RFC 4648 (http://tools.ietf.org/html/rfc4648#section-5)
//! \details To specify alternative alpahabet or code, call Initialize() with EncodingLookupArray parameter.
class Base64URLDecoder : public BaseN_Decoder
{
public:
	//! \brief Construct a Base64URLDecoder
	//! \param attachment a BufferedTrasformation to attach to this object
	Base64URLDecoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDecodingLookupArray(), 6, attachment) {}
    
	//! \brief Initialize or reinitialize this object, without signal propagation
	//! \param parameters a set of NameValuePairs used to initialize this object
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on
	//!  attached transformations. If initialization should be propagated, then use the Initialize() function.
	void IsolatedInitialize(const NameValuePairs &parameters)
		{CRYPTOPP_UNUSED(parameters);}
    
private:
	static const int * CRYPTOPP_API GetDecodingLookupArray();
};

NAMESPACE_END

#endif
