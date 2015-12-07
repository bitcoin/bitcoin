#ifndef CRYPTOPP_ZDEFLATE_H
#define CRYPTOPP_ZDEFLATE_H

#include "cryptlib.h"
#include "filters.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
class LowFirstBitWriter : public Filter
{
public:
	LowFirstBitWriter(BufferedTransformation *attachment);
	void PutBits(unsigned long value, unsigned int length);
	void FlushBitBuffer();
	void ClearBitBuffer();

	void StartCounting();
	unsigned long FinishCounting();

protected:
	bool m_counting;
	unsigned long m_bitCount;
	unsigned long m_buffer;
	unsigned int m_bitsBuffered, m_bytesBuffered;
	FixedSizeSecBlock<byte, 256> m_outputBuffer;
};

//! \class HuffmanEncoder
class HuffmanEncoder
{
public:
	typedef unsigned int code_t;
	typedef unsigned int value_t;

	//! \brief Construct a HuffmanEncoder
	HuffmanEncoder() {}
	
	//! \brief Construct a HuffmanEncoder
	//! \param codeBits a table of code bits
	//! \param nCodes the number of codes in the table
	HuffmanEncoder(const unsigned int *codeBits, unsigned int nCodes);
	
	//! \brief Initialize or reinitialize this object
	//! \param codeBits a table of code bits
	//! \param nCodes the number of codes in the table
	void Initialize(const unsigned int *codeBits, unsigned int nCodes);

	static void GenerateCodeLengths(unsigned int *codeBits, unsigned int maxCodeBits, const unsigned int *codeCounts, size_t nCodes);

	void Encode(LowFirstBitWriter &writer, value_t value) const;

	struct Code
	{
		unsigned int code;
		unsigned int len;
	};

	SecBlock<Code> m_valueToCode;
};

//! DEFLATE (RFC 1951) compressor

class Deflator : public LowFirstBitWriter
{
public:
	enum {MIN_DEFLATE_LEVEL = 0, DEFAULT_DEFLATE_LEVEL = 6, MAX_DEFLATE_LEVEL = 9};
	enum {MIN_LOG2_WINDOW_SIZE = 9, DEFAULT_LOG2_WINDOW_SIZE = 15, MAX_LOG2_WINDOW_SIZE = 15};
	/*! \note detectUncompressible makes it faster to process uncompressible files, but
		if a file has both compressible and uncompressible parts, it may fail to compress some of the
		compressible parts. */
	Deflator(BufferedTransformation *attachment=NULL, int deflateLevel=DEFAULT_DEFLATE_LEVEL, int log2WindowSize=DEFAULT_LOG2_WINDOW_SIZE, bool detectUncompressible=true);
	//! possible parameter names: Log2WindowSize, DeflateLevel, DetectUncompressible
	Deflator(const NameValuePairs &parameters, BufferedTransformation *attachment=NULL);

	//! this function can be used to set the deflate level in the middle of compression
	void SetDeflateLevel(int deflateLevel);
	int GetDeflateLevel() const {return m_deflateLevel;}
	int GetLog2WindowSize() const {return m_log2WindowSize;}

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking);
	bool IsolatedFlush(bool hardFlush, bool blocking);

protected:
	virtual void WritePrestreamHeader() {}
	virtual void ProcessUncompressedData(const byte *string, size_t length)
		{CRYPTOPP_UNUSED(string), CRYPTOPP_UNUSED(length);}
	virtual void WritePoststreamTail() {}

	enum {STORED = 0, STATIC = 1, DYNAMIC = 2};
	enum {MIN_MATCH = 3, MAX_MATCH = 258};

	void InitializeStaticEncoders();
	void Reset(bool forceReset = false);
	unsigned int FillWindow(const byte *str, size_t length);
	unsigned int ComputeHash(const byte *str) const;
	unsigned int LongestMatch(unsigned int &bestMatch) const;
	void InsertString(unsigned int start);
	void ProcessBuffer();

	void LiteralByte(byte b);
	void MatchFound(unsigned int distance, unsigned int length);
	void EncodeBlock(bool eof, unsigned int blockType);
	void EndBlock(bool eof);

	struct EncodedMatch
	{
		unsigned literalCode : 9;
		unsigned literalExtra : 5;
		unsigned distanceCode : 5;
		unsigned distanceExtra : 13;
	};

	int m_deflateLevel, m_log2WindowSize, m_compressibleDeflateLevel;
	unsigned int m_detectSkip, m_detectCount;
	unsigned int DSIZE, DMASK, HSIZE, HMASK, GOOD_MATCH, MAX_LAZYLENGTH, MAX_CHAIN_LENGTH;
	bool m_headerWritten, m_matchAvailable;
	unsigned int m_dictionaryEnd, m_stringStart, m_lookahead, m_minLookahead, m_previousMatch, m_previousLength;
	HuffmanEncoder m_staticLiteralEncoder, m_staticDistanceEncoder, m_dynamicLiteralEncoder, m_dynamicDistanceEncoder;
	SecByteBlock m_byteBuffer;
	SecBlock<word16> m_head, m_prev;
	FixedSizeSecBlock<unsigned int, 286> m_literalCounts;
	FixedSizeSecBlock<unsigned int, 30> m_distanceCounts;
	SecBlock<EncodedMatch> m_matchBuffer;
	unsigned int m_matchBufferEnd, m_blockStart, m_blockLength;
};

NAMESPACE_END

#endif
