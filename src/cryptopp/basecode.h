// basecode.h - written and placed in the public domain by Wei Dai

//! \file
//! \brief Base classes for working with encoders and decoders.

#ifndef CRYPTOPP_BASECODE_H
#define CRYPTOPP_BASECODE_H

#include "cryptlib.h"
#include "filters.h"
#include "algparam.h"
#include "argnames.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class BaseN_Encoder
//! \brief Encoder for bases that are a power of 2
class CRYPTOPP_DLL BaseN_Encoder : public Unflushable<Filter>
{
public:
	//! \brief Construct a BaseN_Encoder
	//! \param attachment a BufferedTransformation to attach to this object
	BaseN_Encoder(BufferedTransformation *attachment=NULL)
		: m_alphabet(NULL), m_padding(0), m_bitsPerChar(0)
		, m_outputBlockSize(0), m_bytePos(0), m_bitPos(0)
			{Detach(attachment);}

	//! \brief Construct a BaseN_Encoder
	//! \param alphabet table of ASCII characters to use as the alphabet
	//! \param log2base the log<sub>2</sub>base
	//! \param attachment a BufferedTransformation to attach to this object
	//! \param padding the character to use as padding
	//! \pre log2base must be between 1 and 7 inclusive
	//! \throws InvalidArgument if log2base is not between 1 and 7
	BaseN_Encoder(const byte *alphabet, int log2base, BufferedTransformation *attachment=NULL, int padding=-1)
		: m_alphabet(NULL), m_padding(0), m_bitsPerChar(0)
		, m_outputBlockSize(0), m_bytePos(0), m_bitPos(0)
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters(Name::EncodingLookupArray(), alphabet)
			(Name::Log2Base(), log2base)
			(Name::Pad(), padding != -1)
			(Name::PaddingByte(), byte(padding)));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);

private:
	const byte *m_alphabet;
	int m_padding, m_bitsPerChar, m_outputBlockSize;
	int m_bytePos, m_bitPos;
	SecByteBlock m_outBuf;
};

//! \class BaseN_Decoder
//! \brief Decoder for bases that are a power of 2
class CRYPTOPP_DLL BaseN_Decoder : public Unflushable<Filter>
{
public:
	//! \brief Construct a BaseN_Decoder
	//! \param attachment a BufferedTransformation to attach to this object
	//! \details padding is set to -1, which means use default padding. If not
	//!   required, then the value must be set via IsolatedInitialize().
	BaseN_Decoder(BufferedTransformation *attachment=NULL)
		: m_lookup(0), m_padding(0), m_bitsPerChar(0)
		, m_outputBlockSize(0), m_bytePos(0), m_bitPos(0)
			{Detach(attachment);}

	//! \brief Construct a BaseN_Decoder
	//! \param lookup table of values
	//! \param log2base the log<sub>2</sub>base
	//! \param attachment a BufferedTransformation to attach to this object
	//! \details log2base is the exponent (like 5 in 2<sup>5</sup>), and not
	//!   the number of elements (like 32).
	//! \details padding is set to -1, which means use default padding. If not
	//!   required, then the value must be set via IsolatedInitialize().
	BaseN_Decoder(const int *lookup, int log2base, BufferedTransformation *attachment=NULL)
		: m_lookup(0), m_padding(0), m_bitsPerChar(0)
		, m_outputBlockSize(0), m_bytePos(0), m_bitPos(0)
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters(Name::DecodingLookupArray(), lookup)(Name::Log2Base(), log2base));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);

	//! \brief Intializes BaseN lookup array
	//! \param lookup table of values
	//! \param alphabet table of ASCII characters
	//! \param base the base for the encoder
	//! \param caseInsensitive flag indicating whether the alpabet is case sensitivie
	//! \pre COUNTOF(lookup) == 256
	//! \pre COUNTOF(alphabet) == base
	//! \details Internally, the function sets the first 256 elements in the lookup table to
	//   their value from the alphabet array or -1. base is the number of element (like 32),
	//!  and not an exponent (like 5 in 2<sup>5</sup>)
	static void CRYPTOPP_API InitializeDecodingLookupArray(int *lookup, const byte *alphabet, unsigned int base, bool caseInsensitive);

private:
	const int *m_lookup;
	int m_padding, m_bitsPerChar, m_outputBlockSize;
	int m_bytePos, m_bitPos;
	SecByteBlock m_outBuf;
};

//! \class Grouper
//! \brief Filter that breaks input stream into groups of fixed size
class CRYPTOPP_DLL Grouper : public Bufferless<Filter>
{
public:
	//! \brief Construct a Grouper
	//! \param attachment a BufferedTransformation to attach to this object
	Grouper(BufferedTransformation *attachment=NULL)
		: m_groupSize(0), m_counter(0) {Detach(attachment);}

	//! \brief Construct a Grouper
	//! \param groupSize the size of the grouping
	//! \param separator the separator to use between groups
	//! \param terminator the terminator appeand after processing
	//! \param attachment a BufferedTransformation to attach to this object
	Grouper(int groupSize, const std::string &separator, const std::string &terminator, BufferedTransformation *attachment=NULL)
		: m_groupSize(0), m_counter(0)
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters(Name::GroupSize(), groupSize)
			(Name::Separator(), ConstByteArrayParameter(separator))
			(Name::Terminator(), ConstByteArrayParameter(terminator)));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);

private:
	SecByteBlock m_separator, m_terminator;
	size_t m_groupSize, m_counter;
};

NAMESPACE_END

#endif
