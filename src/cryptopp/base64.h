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
//! \details Base64 encodes data per <A HREF="http://tools.ietf.org/html/rfc4648#section-4">RFC 4648, Base 64 Encoding</A>.
class Base64Encoder : public SimpleProxyFilter
{
public:
	//! \brief Construct a Base64Encoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \param insertLineBreaks a BufferedTrasformation to attach to this object
	//! \param maxLineLength the lenght of a line if line breaks are used
	//! \details Base64Encoder constructs a default encoder. The constructor lacks a parameter for padding, and you must
	//!   use IsolatedInitialize() to modify the Base64Encoder after construction.
	//! \sa IsolatedInitialize() for an example of modifying an encoder after construction.
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
	//!     encoder.IsolatedInitialize(params);</pre>
	//! \details You can change the encoding to RFC 4648 web safe alphabet by performing the following:
	//!   <pre>
	//!     Base64Encoder encoder;
	//!     const byte ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	//!     AlgorithmParameters params = MakeParameters(Name::EncodingLookupArray(),(const byte *)ALPHABET);
	//!     encoder.IsolatedInitialize(params);</pre>
	//! \details If you change the encoding alphabet, then you will need to change the decoding alphabet \a and
	//!   the decoder's lookup table.
	//! \sa Base64URLEncoder for an encoder that provides the web safe alphabet, and Base64Decoder::IsolatedInitialize()
	//!   for an example of modifying a decoder's lookup table after construction.
	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! \class Base64Decoder
//! \brief Base64 decodes data
//! \details Base64 encodes data per <A HREF="http://tools.ietf.org/html/rfc4648#section-4">RFC 4648, Base 64 Encoding</A>.
class Base64Decoder : public BaseN_Decoder
{
public:
	//! \brief Construct a Base64Decoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \sa IsolatedInitialize() for an example of modifying an encoder after construction.
	Base64Decoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDecodingLookupArray(), 6, attachment) {}

	//! \brief Initialize or reinitialize this object, without signal propagation
	//! \param parameters a set of NameValuePairs used to initialize this object
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on attached
	//!   transformations. If initialization should be propagated, then use the Initialize() function.
	//! \details The default decoding alpahbet is RFC 4868. You can change the to RFC 4868 web safe alphabet
	//!   by performing the following:
	//!   <pre>
	//!     int lookup[256];
	//!     const byte ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	//!     Base64Decoder::InitializeDecodingLookupArray(lookup, ALPHABET, 64, false);
	//!
	//!     Base64Decoder decoder;
	//!     AlgorithmParameters params = MakeParameters(Name::DecodingLookupArray(),(const int *)lookup);
	//!     decoder.IsolatedInitialize(params);</pre>
	//! \sa Base64URLDecoder for a decoder that provides the web safe alphabet, and Base64Encoder::IsolatedInitialize()
	//!   for an example of modifying an encoder's alphabet after construction.
	void IsolatedInitialize(const NameValuePairs &parameters);

private:
	//! \brief Provides the default decoding lookup table
	//! \return default decoding lookup table
	static const int * CRYPTOPP_API GetDecodingLookupArray();
};

//! \class Base64URLEncoder
//! \brief Base64 encodes data using a web safe alphabet
//! \details Base64 encodes data per <A HREF="http://tools.ietf.org/html/rfc4648#section-5">RFC 4648, Base 64 Encoding
//!   with URL and Filename Safe Alphabet</A>.
class Base64URLEncoder : public SimpleProxyFilter
{
public:
	//! \brief Construct a Base64URLEncoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \param insertLineBreaks a BufferedTrasformation to attach to this object
	//! \param maxLineLength the lenght of a line if line breaks are used
	//! \details Base64URLEncoder() constructs a default encoder using a web safe alphabet. The constructor ignores
	//!   insertLineBreaks and maxLineLength because the web and URL safe specifications don't use them. They are
	//!   present in the constructor for API compatibility with Base64Encoder so it is a drop-in replacement. The
	//!   constructor also disables padding on the encoder for the same reason.
	//! \details If you need line breaks or padding, then you must use IsolatedInitialize() to set them
	//!   after constructing a Base64URLEncoder.
	//! \sa Base64Encoder for an encoder that provides a classic alphabet, and Base64URLEncoder::IsolatedInitialize
	//!   for an example of modifying an encoder after construction.
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
	//!     encoder.IsolatedInitialize(params);</pre>
	//! \sa Base64Encoder for an encoder that provides a classic alphabet.
	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! \class Base64URLDecoder
//! \brief Base64 decodes data using a web safe alphabet
//! \details Base64 encodes data per <A HREF="http://tools.ietf.org/html/rfc4648#section-5">RFC 4648, Base 64 Encoding
//!   with URL and Filename Safe Alphabet</A>.
class Base64URLDecoder : public BaseN_Decoder
{
public:
	//! \brief Construct a Base64URLDecoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \details Base64URLDecoder() constructs a default decoder using a web safe alphabet.
	//! \sa Base64Decoder for a decoder that provides a classic alphabet.
	Base64URLDecoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDecodingLookupArray(), 6, attachment) {}

	//! \brief Initialize or reinitialize this object, without signal propagation
	//! \param parameters a set of NameValuePairs used to initialize this object
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on
	//!  attached transformations. If initialization should be propagated, then use the Initialize() function.
	//! \sa Base64Decoder for a decoder that provides a classic alphabet, and Base64URLEncoder::IsolatedInitialize
	//!   for an example of modifying an encoder after construction.
	void IsolatedInitialize(const NameValuePairs &parameters);

private:
	//! \brief Provides the default decoding lookup table
	//! \return default decoding lookup table
	static const int * CRYPTOPP_API GetDecodingLookupArray();
};

NAMESPACE_END

#endif
