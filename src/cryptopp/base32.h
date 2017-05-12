// base32.h - written and placed in the public domain by Frank Palazzolo, based on hex.cpp by Wei Dai

//! \file
//! \brief Classes for Base32Encoder and Base32Decoder

#ifndef CRYPTOPP_BASE32_H
#define CRYPTOPP_BASE32_H

#include "cryptlib.h"
#include "basecode.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Base32Encoder
//! \brief Base32 encodes data
//! \details Converts data to base32. The default code is based on <A HREF="http://www.ietf.org/proceedings/51/I-D/draft-ietf-idn-dude-02.txt">Differential Unicode Domain Encoding (DUDE) (draft-ietf-idn-dude-02.txt)</A>.
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
	//!     encoder.IsolatedInitialize(params);</pre>
	//! \details You can change the encoding to <A HREF="http://tools.ietf.org/html/rfc4648#page-10">RFC 4648, Base
	//!   32 Encoding with Extended Hex Alphabet</A> by performing the following:
	//!   <pre>
	//!     Base32Encoder encoder;
	//!     const byte ALPHABET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
	//!     AlgorithmParameters params = MakeParameters(Name::EncodingLookupArray(),(const byte *)ALPHABET);
	//!     encoder.IsolatedInitialize(params);</pre>
	//! \details If you change the encoding alphabet, then you will need to change the decoding alphabet \a and
	//!   the decoder's lookup table.
	//! \sa Base32Decoder::IsolatedInitialize() for an example of changing a Base32Decoder's lookup table.
	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! \class Base32Decoder
//! \brief Base32 decodes data
//! \details Decode base32 data. The default code is based on <A HREF="http://www.ietf.org/proceedings/51/I-D/draft-ietf-idn-dude-02.txt">Differential Unicode Domain Encoding (DUDE) (draft-ietf-idn-dude-02.txt)</A>.
class Base32Decoder : public BaseN_Decoder
{
public:
	//! \brief Construct a Base32Decoder
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \sa IsolatedInitialize() for an example of modifying a Base32Decoder after construction.
	Base32Decoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDefaultDecodingLookupArray(), 5, attachment) {}

	//! \brief Initialize or reinitialize this object, without signal propagation
	//! \param parameters a set of NameValuePairs used to initialize this object
	//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
	//!   number of arbitrarily typed arguments. IsolatedInitialize() does not call Initialize() on attached
	//!   transformations. If initialization should be propagated, then use the Initialize() function.
	//! \details You can change the encoding to <A HREF="http://tools.ietf.org/html/rfc4648#page-10">RFC 4648, Base
	//!   32 Encoding with Extended Hex Alphabet</A> by performing the following:
	//!   <pre>
	//!     int lookup[256];
	//!     const byte ALPHABET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
	//!     Base32Decoder::InitializeDecodingLookupArray(lookup, ALPHABET, 32, true /*insensitive*/);
	//!
	//!     Base32Decoder decoder;
	//!     AlgorithmParameters params = MakeParameters(Name::DecodingLookupArray(),(const int *)lookup);
	//!     decoder.IsolatedInitialize(params);</pre>
	//! \sa Base32Encoder::IsolatedInitialize() for an example of changing a Base32Encoder's alphabet.
	void IsolatedInitialize(const NameValuePairs &parameters);

private:
	//! \brief Provides the default decoding lookup table
	//! \return default decoding lookup table
	static const int * CRYPTOPP_API GetDefaultDecodingLookupArray();
};

NAMESPACE_END

#endif
