#ifndef CRYPTOPP_ZINFLATE_H
#define CRYPTOPP_ZINFLATE_H

#include "cryptlib.h"
#include "secblock.h"
#include "filters.h"
#include "stdcpp.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
class LowFirstBitReader
{
public:
	LowFirstBitReader(BufferedTransformation &store)
		: m_store(store), m_buffer(0), m_bitsBuffered(0) {}
	unsigned int BitsBuffered() const {return m_bitsBuffered;}
	unsigned long PeekBuffer() const {return m_buffer;}
	bool FillBuffer(unsigned int length);
	unsigned long PeekBits(unsigned int length);
	void SkipBits(unsigned int length);
	unsigned long GetBits(unsigned int length);

private:
	BufferedTransformation &m_store;
	unsigned long m_buffer;
	unsigned int m_bitsBuffered;
};

struct CodeLessThan;

//! Huffman Decoder
class HuffmanDecoder
{
public:
	typedef unsigned int code_t;
	typedef unsigned int value_t;
	enum {MAX_CODE_BITS = sizeof(code_t)*8};

	class Err : public Exception {public: Err(const std::string &what) : Exception(INVALID_DATA_FORMAT, "HuffmanDecoder: " + what) {}};

	HuffmanDecoder() : m_maxCodeBits(0), m_cacheBits(0), m_cacheMask(0), m_normalizedCacheMask(0) {}
	HuffmanDecoder(const unsigned int *codeBitLengths, unsigned int nCodes)
		: m_maxCodeBits(0), m_cacheBits(0), m_cacheMask(0), m_normalizedCacheMask(0)
			{Initialize(codeBitLengths, nCodes);}

	void Initialize(const unsigned int *codeBitLengths, unsigned int nCodes);
	unsigned int Decode(code_t code, /* out */ value_t &value) const;
	bool Decode(LowFirstBitReader &reader, value_t &value) const;

private:
	friend struct CodeLessThan;

	struct CodeInfo
	{
		CodeInfo(code_t code=0, unsigned int len=0, value_t value=0) : code(code), len(len), value(value) {}
		inline bool operator<(const CodeInfo &rhs) const {return code < rhs.code;}
		code_t code;
		unsigned int len;
		value_t value;
	};

	struct LookupEntry
	{
		unsigned int type;
		union
		{
			value_t value;
			const CodeInfo *begin;
		};
		union
		{
			unsigned int len;
			const CodeInfo *end;
		};
	};

	static code_t NormalizeCode(code_t code, unsigned int codeBits);
	void FillCacheEntry(LookupEntry &entry, code_t normalizedCode) const;

	unsigned int m_maxCodeBits, m_cacheBits, m_cacheMask, m_normalizedCacheMask;
	std::vector<CodeInfo, AllocatorWithCleanup<CodeInfo> > m_codeToValue;
	mutable std::vector<LookupEntry, AllocatorWithCleanup<LookupEntry> > m_cache;
};

//! DEFLATE (RFC 1951) decompressor

class Inflator : public AutoSignaling<Filter>
{
public:
	class Err : public Exception
	{
	public:
		Err(ErrorType e, const std::string &s)
			: Exception(e, s) {}
	};
	class UnexpectedEndErr : public Err {public: UnexpectedEndErr() : Err(INVALID_DATA_FORMAT, "Inflator: unexpected end of compressed block") {}};
	class BadBlockErr : public Err {public: BadBlockErr() : Err(INVALID_DATA_FORMAT, "Inflator: error in compressed block") {}};

	//! \brief RFC 1951 Decompressor
	//! \param attachment the filter's attached transformation
	//! \param repeat decompress multiple compressed streams in series
	//! \param autoSignalPropagation 0 to turn off MessageEnd signal
	Inflator(BufferedTransformation *attachment = NULL, bool repeat = false, int autoSignalPropagation = -1);

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking);
	bool IsolatedFlush(bool hardFlush, bool blocking);

	virtual unsigned int GetLog2WindowSize() const {return 15;}

protected:
	ByteQueue m_inQueue;

private:
	virtual unsigned int MaxPrestreamHeaderSize() const {return 0;}
	virtual void ProcessPrestreamHeader() {}
	virtual void ProcessDecompressedData(const byte *string, size_t length)
		{AttachedTransformation()->Put(string, length);}
	virtual unsigned int MaxPoststreamTailSize() const {return 0;}
	virtual void ProcessPoststreamTail() {}

	void ProcessInput(bool flush);
	void DecodeHeader();
	bool DecodeBody();
	void FlushOutput();
	void OutputByte(byte b);
	void OutputString(const byte *string, size_t length);
	void OutputPast(unsigned int length, unsigned int distance);

	static const HuffmanDecoder *FixedLiteralDecoder();
	static const HuffmanDecoder *FixedDistanceDecoder();

	const HuffmanDecoder& GetLiteralDecoder() const;
	const HuffmanDecoder& GetDistanceDecoder() const;

	enum State {PRE_STREAM, WAIT_HEADER, DECODING_BODY, POST_STREAM, AFTER_END};
	State m_state;
	bool m_repeat, m_eof, m_wrappedAround;
	byte m_blockType;
	word16 m_storedLen;
	enum NextDecode {LITERAL, LENGTH_BITS, DISTANCE, DISTANCE_BITS};
	NextDecode m_nextDecode;
	unsigned int m_literal, m_distance;	// for LENGTH_BITS or DISTANCE_BITS
	HuffmanDecoder m_dynamicLiteralDecoder, m_dynamicDistanceDecoder;
	LowFirstBitReader m_reader;
	SecByteBlock m_window;
	size_t m_current, m_lastFlush;
};

NAMESPACE_END

#endif
