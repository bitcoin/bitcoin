// asn.h - written and placed in the public domain by Wei Dai

//! \file asn.h
//! \brief Classes and functions for working with ANS.1 objects

#ifndef CRYPTOPP_ASN_H
#define CRYPTOPP_ASN_H

#include "cryptlib.h"
#include "filters.h"
#include "smartptr.h"
#include "stdcpp.h"
#include "queue.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \brief ASN.1 types
//! \note These tags and flags are not complete
enum ASNTag
{
	BOOLEAN 			= 0x01,
	INTEGER 			= 0x02,
	BIT_STRING			= 0x03,
	OCTET_STRING		= 0x04,
	TAG_NULL			= 0x05,
	OBJECT_IDENTIFIER	= 0x06,
	OBJECT_DESCRIPTOR	= 0x07,
	EXTERNAL			= 0x08,
	REAL				= 0x09,
	ENUMERATED			= 0x0a,
	UTF8_STRING			= 0x0c,
	SEQUENCE			= 0x10,
	SET 				= 0x11,
	NUMERIC_STRING		= 0x12,
	PRINTABLE_STRING 	= 0x13,
	T61_STRING			= 0x14,
	VIDEOTEXT_STRING 	= 0x15,
	IA5_STRING			= 0x16,
	UTC_TIME 			= 0x17,
	GENERALIZED_TIME 	= 0x18,
	GRAPHIC_STRING		= 0x19,
	VISIBLE_STRING		= 0x1a,
	GENERAL_STRING		= 0x1b
};

//! \brief ASN.1 flags
//! \note These tags and flags are not complete
enum ASNIdFlag
{
	UNIVERSAL			= 0x00,
//	DATA				= 0x01,
//	HEADER				= 0x02,
	CONSTRUCTED 		= 0x20,
	APPLICATION 		= 0x40,
	CONTEXT_SPECIFIC	= 0x80,
	PRIVATE 			= 0xc0
};

//! \brief Raises a BERDecodeErr
inline void BERDecodeError() {throw BERDecodeErr();}

//! \brief Exception thrown when an unknown object identifier is encountered
class CRYPTOPP_DLL UnknownOID : public BERDecodeErr
{
public:
	//! \brief Construct an UnknownOID
	UnknownOID() : BERDecodeErr("BER decode error: unknown object identifier") {}
	//! \brief Construct an UnknownOID
	//! \param err error message to use for the execption
	UnknownOID(const char *err) : BERDecodeErr(err) {}
};

// unsigned int DERLengthEncode(unsigned int length, byte *output=0);

//! \brief DER encode a length
//! \param bt BufferedTransformation object for writing
//! \param length the size to encode
//! \returns the number of octets used for the encoding
CRYPTOPP_DLL size_t CRYPTOPP_API DERLengthEncode(BufferedTransformation &bt, lword length);

//! \brief BER decode a length
//! \param bt BufferedTransformation object for reading
//! \param length the decoded size
//! \returns true if the value was decoded
//! \throws BERDecodeError if the value fails to decode or is too large for size_t
//! \details BERLengthDecode() returns false if the encoding is indefinite length.
CRYPTOPP_DLL bool CRYPTOPP_API BERLengthDecode(BufferedTransformation &bt, size_t &length);

//! \brief DER encode NULL
//! \param bt BufferedTransformation object for writing
CRYPTOPP_DLL void CRYPTOPP_API DEREncodeNull(BufferedTransformation &bt);

//! \brief BER decode NULL
//! \param bt BufferedTransformation object for reading
CRYPTOPP_DLL void CRYPTOPP_API BERDecodeNull(BufferedTransformation &bt);

//! \brief DER encode octet string
//! \param bt BufferedTransformation object for writing
//! \param str the string to encode
//! \param strLen the length of the string
//! \returns the number of octets used for the encoding
CRYPTOPP_DLL size_t CRYPTOPP_API DEREncodeOctetString(BufferedTransformation &bt, const byte *str, size_t strLen);

//! \brief DER encode octet string
//! \param bt BufferedTransformation object for reading
//! \param str the string to encode
//! \returns the number of octets used for the encoding
CRYPTOPP_DLL size_t CRYPTOPP_API DEREncodeOctetString(BufferedTransformation &bt, const SecByteBlock &str);

//! \brief BER decode octet string
//! \param bt BufferedTransformation object for reading
//! \param str the decoded string
//! \returns the number of octets used for the encoding
CRYPTOPP_DLL size_t CRYPTOPP_API BERDecodeOctetString(BufferedTransformation &bt, SecByteBlock &str);

//! \brief BER decode octet string
//! \param bt BufferedTransformation object for reading
//! \param str the decoded string
//! \returns the number of octets used for the encoding
CRYPTOPP_DLL size_t CRYPTOPP_API BERDecodeOctetString(BufferedTransformation &bt, BufferedTransformation &str);

//! \brief DER encode text string
//! \param bt BufferedTransformation object for writing
//! \param str the string to encode
//! \param asnTag the ASN.1 type
//! \returns the number of octets used for the encoding
//! \details DEREncodeTextString() can be used for UTF8_STRING, PRINTABLE_STRING, and IA5_STRING
CRYPTOPP_DLL size_t CRYPTOPP_API DEREncodeTextString(BufferedTransformation &bt, const std::string &str, byte asnTag);

//! \brief BER decode text string
//! \param bt BufferedTransformation object for reading
//! \param str the string to encode
//! \param asnTag the ASN.1 type
//! \details DEREncodeTextString() can be used for UTF8_STRING, PRINTABLE_STRING, and IA5_STRING
CRYPTOPP_DLL size_t CRYPTOPP_API BERDecodeTextString(BufferedTransformation &bt, std::string &str, byte asnTag);

//! \brief DER encode bit string
//! \param bt BufferedTransformation object for writing
//! \param str the string to encode
//! \param strLen the length of the string
//! \param unusedBits the number of unused bits
//! \returns the number of octets used for the encoding
CRYPTOPP_DLL size_t CRYPTOPP_API DEREncodeBitString(BufferedTransformation &bt, const byte *str, size_t strLen, unsigned int unusedBits=0);

//! \brief DER decode bit string
//! \param bt BufferedTransformation object for reading
//! \param str the decoded string
//! \param unusedBits the number of unused bits
CRYPTOPP_DLL size_t CRYPTOPP_API BERDecodeBitString(BufferedTransformation &bt, SecByteBlock &str, unsigned int &unusedBits);

//! \brief BER decode and DER re-encode
//! \param bt BufferedTransformation object for writing
//! \param dest BufferedTransformation object
CRYPTOPP_DLL void CRYPTOPP_API DERReencode(BufferedTransformation &bt, BufferedTransformation &dest);

//! \brief Object Identifier
class CRYPTOPP_DLL OID
{
public:
	//! \brief Construct an OID
	OID() {}
	//! \brief Construct an OID
	//! \param v value to initialize the OID
	OID(word32 v) : m_values(1, v) {}
	//! \brief Construct an OID
	//! \param bt BufferedTransformation object
	OID(BufferedTransformation &bt) {BERDecode(bt);}

	//! \brief Append a value to an OID
	//! \param rhs the value to append
	inline OID & operator+=(word32 rhs) {m_values.push_back(rhs); return *this;}

	//! \brief DER encode this OID
	//! \param bt BufferedTransformation object
	void DEREncode(BufferedTransformation &bt) const;

	//! \brief BER decode an OID
	//! \param bt BufferedTransformation object
	void BERDecode(BufferedTransformation &bt);

	//! \brief BER decode an OID
	//! \param bt BufferedTransformation object
	//! \throws BERDecodeErr() if decoded value doesn't match an expected OID
	//! \details BERDecodeAndCheck() can be used to parse an OID and verify it matches an expected.
	//! <pre>
	//!   BERSequenceDecoder key(bt);
	//!   ...
	//!   BERSequenceDecoder algorithm(key);
	//!   GetAlgorithmID().BERDecodeAndCheck(algorithm);
	//! </pre>
	void BERDecodeAndCheck(BufferedTransformation &bt) const;

	std::vector<word32> m_values;

private:
	static void EncodeValue(BufferedTransformation &bt, word32 v);
	static size_t DecodeValue(BufferedTransformation &bt, word32 &v);
};

//! \brief ASN.1 encoded object filter
class EncodedObjectFilter : public Filter
{
public:
	enum Flag {PUT_OBJECTS=1, PUT_MESSANGE_END_AFTER_EACH_OBJECT=2, PUT_MESSANGE_END_AFTER_ALL_OBJECTS=4, PUT_MESSANGE_SERIES_END_AFTER_ALL_OBJECTS=8};

	//! \brief Construct an EncodedObjectFilter
	//! \param attachment a BufferedTrasformation to attach to this object
	//! \param nObjects
	//! \param flags bitwise OR of EncodedObjectFilter::Flag
	EncodedObjectFilter(BufferedTransformation *attachment = NULL, unsigned int nObjects = 1, word32 flags = 0);

	//! \brief Input a byte buffer for processing
	//! \param inString the byte buffer to process
	//! \param length the size of the string, in bytes
	void Put(const byte *inString, size_t length);

	unsigned int GetNumberOfCompletedObjects() const {return m_nCurrentObject;}
	unsigned long GetPositionOfObject(unsigned int i) const {return m_positions[i];}

private:
	BufferedTransformation & CurrentTarget();

	word32 m_flags;
	unsigned int m_nObjects, m_nCurrentObject, m_level;
	std::vector<unsigned int> m_positions;
	ByteQueue m_queue;
	enum State {IDENTIFIER, LENGTH, BODY, TAIL, ALL_DONE} m_state;
	byte m_id;
	lword m_lengthRemaining;
};

//! \brief BER General Decoder
class CRYPTOPP_DLL BERGeneralDecoder : public Store
{
public:
	explicit BERGeneralDecoder(BufferedTransformation &inQueue, byte asnTag);
	explicit BERGeneralDecoder(BERGeneralDecoder &inQueue, byte asnTag);
	~BERGeneralDecoder();

	bool IsDefiniteLength() const {return m_definiteLength;}
	lword RemainingLength() const {CRYPTOPP_ASSERT(m_definiteLength); return m_length;}
	bool EndReached() const;
	byte PeekByte() const;
	void CheckByte(byte b);

	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const;

	// call this to denote end of sequence
	void MessageEnd();

protected:
	BufferedTransformation &m_inQueue;
	bool m_finished, m_definiteLength;
	lword m_length;

private:
	void Init(byte asnTag);
	void StoreInitialize(const NameValuePairs &parameters)
		{CRYPTOPP_UNUSED(parameters); CRYPTOPP_ASSERT(false);}
	lword ReduceLength(lword delta);
};

// GCC (and likely other compilers) identify the explicit DERGeneralEncoder as a copy constructor;
// and not a constructor. We had to remove the default asnTag value to point the compiler in the
// proper direction. We did not break the library or versioning based on the output of
// `nm --demangle libcryptopp.a | grep DERGeneralEncoder::DERGeneralEncoder | grep -v " U "`.

//! \brief DER General Encoder
class CRYPTOPP_DLL DERGeneralEncoder : public ByteQueue
{
public:
#if defined(CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562)
	explicit DERGeneralEncoder(BufferedTransformation &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED);
	explicit DERGeneralEncoder(DERGeneralEncoder &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED);
#else
	explicit DERGeneralEncoder(BufferedTransformation &outQueue, byte asnTag /*= SEQUENCE | CONSTRUCTED*/);
	explicit DERGeneralEncoder(DERGeneralEncoder &outQueue, byte asnTag /*= SEQUENCE | CONSTRUCTED*/);
#endif
	~DERGeneralEncoder();

	// call this to denote end of sequence
	void MessageEnd();

private:
	BufferedTransformation &m_outQueue;
	bool m_finished;

	byte m_asnTag;
};

//! \brief BER Sequence Decoder
class CRYPTOPP_DLL BERSequenceDecoder : public BERGeneralDecoder
{
public:
	explicit BERSequenceDecoder(BufferedTransformation &inQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: BERGeneralDecoder(inQueue, asnTag) {}
	explicit BERSequenceDecoder(BERSequenceDecoder &inQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: BERGeneralDecoder(inQueue, asnTag) {}
};

//! \brief DER Sequence Encoder
class CRYPTOPP_DLL DERSequenceEncoder : public DERGeneralEncoder
{
public:
	explicit DERSequenceEncoder(BufferedTransformation &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: DERGeneralEncoder(outQueue, asnTag) {}
	explicit DERSequenceEncoder(DERSequenceEncoder &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: DERGeneralEncoder(outQueue, asnTag) {}
};

//! \brief BER Set Decoder
class CRYPTOPP_DLL BERSetDecoder : public BERGeneralDecoder
{
public:
	explicit BERSetDecoder(BufferedTransformation &inQueue, byte asnTag = SET | CONSTRUCTED)
		: BERGeneralDecoder(inQueue, asnTag) {}
	explicit BERSetDecoder(BERSetDecoder &inQueue, byte asnTag = SET | CONSTRUCTED)
		: BERGeneralDecoder(inQueue, asnTag) {}
};

//! \brief DER Set Encoder
class CRYPTOPP_DLL DERSetEncoder : public DERGeneralEncoder
{
public:
	explicit DERSetEncoder(BufferedTransformation &outQueue, byte asnTag = SET | CONSTRUCTED)
		: DERGeneralEncoder(outQueue, asnTag) {}
	explicit DERSetEncoder(DERSetEncoder &outQueue, byte asnTag = SET | CONSTRUCTED)
		: DERGeneralEncoder(outQueue, asnTag) {}
};

//! \brief Optional data encoder and decoder
//! \tparam T class or type
template <class T>
class ASNOptional : public member_ptr<T>
{
public:
	//! \brief BER decode optional data
	//! \param seqDecoder sequence with the optional ASN.1 data
	//! \param tag ASN.1 tag to match as optional data
	//! \param mask the mask to apply when matching the tag
	//! \sa ASNTag and ASNIdFlag
	void BERDecode(BERSequenceDecoder &seqDecoder, byte tag, byte mask = ~CONSTRUCTED)
	{
		byte b;
		if (seqDecoder.Peek(b) && (b & mask) == tag)
			reset(new T(seqDecoder));
	}

	//! \brief DER encode optional data
	//! \param out BufferedTransformation object
	void DEREncode(BufferedTransformation &out)
	{
		if (this->get() != NULL)
			this->get()->DEREncode(out);
	}
};

//! \brief Encode and decode ASN.1 objects with additional information
//! \tparam BASE base class or type
//! \details Encodes and decodes public keys, private keys and group
//!   parameters with OID identifying the algorithm or scheme.
template <class BASE>
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE ASN1CryptoMaterial : public ASN1Object, public BASE
{
public:
	//! \brief DER encode ASN.1 object
	//! \param bt BufferedTransformation object
	//! \details Save() will write the OID associated with algorithm or scheme.
	//!   In the case of public and private keys, this function writes the
	//!   subjectPubicKeyInfo and privateKeyInfo parts.
	void Save(BufferedTransformation &bt) const
		{BEREncode(bt);}

	//! \brief BER decode ASN.1 object
	//! \param bt BufferedTransformation object
	void Load(BufferedTransformation &bt)
		{BERDecode(bt);}
};

//! \brief Encodes and decodes subjectPublicKeyInfo
class CRYPTOPP_DLL X509PublicKey : public ASN1CryptoMaterial<PublicKey>
{
public:
	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	//! \brief Retrieves the OID of the algorithm
	//! \returns OID of the algorithm
	virtual OID GetAlgorithmID() const =0;
	virtual bool BERDecodeAlgorithmParameters(BufferedTransformation &bt)
		{BERDecodeNull(bt); return false;}
	virtual bool DEREncodeAlgorithmParameters(BufferedTransformation &bt) const
		{DEREncodeNull(bt); return false;}	// see RFC 2459, section 7.3.1

	//! decode subjectPublicKey part of subjectPublicKeyInfo, without the BIT STRING header
	virtual void BERDecodePublicKey(BufferedTransformation &bt, bool parametersPresent, size_t size) =0;
	//! encode subjectPublicKey part of subjectPublicKeyInfo, without the BIT STRING header
	virtual void DEREncodePublicKey(BufferedTransformation &bt) const =0;
};

//! \brief Encodes and decodesprivateKeyInfo
class CRYPTOPP_DLL PKCS8PrivateKey : public ASN1CryptoMaterial<PrivateKey>
{
public:
	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	//! \brief Retrieves the OID of the algorithm
	//! \returns OID of the algorithm
	virtual OID GetAlgorithmID() const =0;
	virtual bool BERDecodeAlgorithmParameters(BufferedTransformation &bt)
		{BERDecodeNull(bt); return false;}
	virtual bool DEREncodeAlgorithmParameters(BufferedTransformation &bt) const
		{DEREncodeNull(bt); return false;}	// see RFC 2459, section 7.3.1

	//! decode privateKey part of privateKeyInfo, without the OCTET STRING header
	virtual void BERDecodePrivateKey(BufferedTransformation &bt, bool parametersPresent, size_t size) =0;
	//! encode privateKey part of privateKeyInfo, without the OCTET STRING header
	virtual void DEREncodePrivateKey(BufferedTransformation &bt) const =0;

	//! decode optional attributes including context-specific tag
	/*! /note default implementation stores attributes to be output in DEREncodeOptionalAttributes */
	virtual void BERDecodeOptionalAttributes(BufferedTransformation &bt);
	//! encode optional attributes including context-specific tag
	virtual void DEREncodeOptionalAttributes(BufferedTransformation &bt) const;

protected:
	ByteQueue m_optionalAttributes;
};

// ********************************************************

//! \brief DER Encode unsigned value
//! \tparam T class or type
//! \param out BufferedTransformation object
//! \param w unsigned value to encode
//! \param asnTag the ASN.1 type
//! \details DEREncodeUnsigned() can be used with INTEGER, BOOLEAN, and ENUM
template <class T>
size_t DEREncodeUnsigned(BufferedTransformation &out, T w, byte asnTag = INTEGER)
{
	byte buf[sizeof(w)+1];
	unsigned int bc;
	if (asnTag == BOOLEAN)
	{
		buf[sizeof(w)] = w ? 0xff : 0;
		bc = 1;
	}
	else
	{
		buf[0] = 0;
		for (unsigned int i=0; i<sizeof(w); i++)
			buf[i+1] = byte(w >> (sizeof(w)-1-i)*8);
		bc = sizeof(w);
		while (bc > 1 && buf[sizeof(w)+1-bc] == 0)
			--bc;
		if (buf[sizeof(w)+1-bc] & 0x80)
			++bc;
	}
	out.Put(asnTag);
	size_t lengthBytes = DERLengthEncode(out, bc);
	out.Put(buf+sizeof(w)+1-bc, bc);
	return 1+lengthBytes+bc;
}

//! \brief BER Decode unsigned value
//! \tparam T class or type
//! \param in BufferedTransformation object
//! \param w unsigned value to encode
//! \param asnTag the ASN.1 type
//! \param minValue the minimum expected value
//! \param maxValue the maximum expected value
//! \throws BERDecodeErr() if the value cannot be parsed or the decoded value is not within range.
//! \details DEREncodeUnsigned() can be used with INTEGER, BOOLEAN, and ENUM
template <class T>
void BERDecodeUnsigned(BufferedTransformation &in, T &w, byte asnTag = INTEGER,
					   T minValue = 0, T maxValue = ((std::numeric_limits<T>::max)()))
{
	byte b;
	if (!in.Get(b) || b != asnTag)
		BERDecodeError();

	size_t bc;
	bool definite = BERLengthDecode(in, bc);
	if (!definite)
		BERDecodeError();

	SecByteBlock buf(bc);

	if (bc != in.Get(buf, bc))
		BERDecodeError();

	const byte *ptr = buf;
	while (bc > sizeof(w) && *ptr == 0)
	{
		bc--;
		ptr++;
	}
	if (bc > sizeof(w))
		BERDecodeError();

	w = 0;
	for (unsigned int i=0; i<bc; i++)
		w = (w << 8) | ptr[i];

	if (w < minValue || w > maxValue)
		BERDecodeError();
}

#ifdef CRYPTOPP_DOXYGEN_PROCESSING
//! \brief Compare two OIDs for equality
//! \param lhs the first OID
//! \param rhs the second OID
//! \returns true if the OIDs are equal, false otherwise
inline bool operator==(const OID &lhs, const OID &rhs);
//! \brief Compare two OIDs for inequality
//! \param lhs the first OID
//! \param rhs the second OID
//! \returns true if the OIDs are not equal, false otherwise
inline bool operator!=(const OID &lhs, const OID &rhs);
//! \brief Compare two OIDs for ordering
//! \param lhs the first OID
//! \param rhs the second OID
//! \returns true if the first OID is less than the second OID, false otherwise
//! \details operator<() calls std::lexicographical_compare() on each element in the array of values.
inline bool operator<(const OID &lhs, const OID &rhs);
//! \brief Append a value to an OID
//! \param lhs the OID
//! \param rhs the value to append
inline OID operator+(const OID &lhs, unsigned long rhs);
#else
inline bool operator==(const ::CryptoPP::OID &lhs, const ::CryptoPP::OID &rhs)
	{return lhs.m_values == rhs.m_values;}
inline bool operator!=(const ::CryptoPP::OID &lhs, const ::CryptoPP::OID &rhs)
	{return lhs.m_values != rhs.m_values;}
inline bool operator<(const ::CryptoPP::OID &lhs, const ::CryptoPP::OID &rhs)
	{return std::lexicographical_compare(lhs.m_values.begin(), lhs.m_values.end(), rhs.m_values.begin(), rhs.m_values.end());}
inline ::CryptoPP::OID operator+(const ::CryptoPP::OID &lhs, unsigned long rhs)
	{return ::CryptoPP::OID(lhs)+=rhs;}
#endif

NAMESPACE_END

#endif
