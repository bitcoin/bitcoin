// zdeflate.cpp - written and placed in the public domain by Wei Dai

// Many of the algorithms and tables used here came from the deflate implementation
// by Jean-loup Gailly, which was included in Crypto++ 4.0 and earlier. I completely
// rewrote it in order to fix a bug that I could not figure out. This code
// is less clever, but hopefully more understandable and maintainable.

#include "pch.h"
#include "zdeflate.h"
#include "stdcpp.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

LowFirstBitWriter::LowFirstBitWriter(BufferedTransformation *attachment)
	: Filter(attachment), m_counting(false), m_bitCount(0), m_buffer(0)
	, m_bitsBuffered(0), m_bytesBuffered(0)
{
}

void LowFirstBitWriter::StartCounting()
{
	CRYPTOPP_ASSERT(!m_counting);
	m_counting = true;
	m_bitCount = 0;
}

unsigned long LowFirstBitWriter::FinishCounting()
{
	CRYPTOPP_ASSERT(m_counting);
	m_counting = false;
	return m_bitCount;
}

void LowFirstBitWriter::PutBits(unsigned long value, unsigned int length)
{
	if (m_counting)
		m_bitCount += length;
	else
	{
		m_buffer |= value << m_bitsBuffered;
		m_bitsBuffered += length;
		CRYPTOPP_ASSERT(m_bitsBuffered <= sizeof(unsigned long)*8);
		while (m_bitsBuffered >= 8)
		{
			m_outputBuffer[m_bytesBuffered++] = (byte)m_buffer;
			if (m_bytesBuffered == m_outputBuffer.size())
			{
				AttachedTransformation()->PutModifiable(m_outputBuffer, m_bytesBuffered);
				m_bytesBuffered = 0;
			}
			m_buffer >>= 8;
			m_bitsBuffered -= 8;
		}
	}
}

void LowFirstBitWriter::FlushBitBuffer()
{
	if (m_counting)
		m_bitCount += 8*(m_bitsBuffered > 0);
	else
	{
		if (m_bytesBuffered > 0)
		{
			AttachedTransformation()->PutModifiable(m_outputBuffer, m_bytesBuffered);
			m_bytesBuffered = 0;
		}
		if (m_bitsBuffered > 0)
		{
			AttachedTransformation()->Put((byte)m_buffer);
			m_buffer = 0;
			m_bitsBuffered = 0;
		}
	}
}

void LowFirstBitWriter::ClearBitBuffer()
{
	m_buffer = 0;
	m_bytesBuffered = 0;
	m_bitsBuffered = 0;
}

HuffmanEncoder::HuffmanEncoder(const unsigned int *codeBits, unsigned int nCodes)
{
	Initialize(codeBits, nCodes);
}

struct HuffmanNode
{
	// Coverity finding on uninitalized 'symbol' member
	HuffmanNode()
		: symbol(0), parent(0) {}
	HuffmanNode(const HuffmanNode& rhs)
		: symbol(rhs.symbol), parent(rhs.parent) {}

	size_t symbol;
	union {size_t parent; unsigned depth, freq;};
};

struct FreqLessThan
{
	inline bool operator()(unsigned int lhs, const HuffmanNode &rhs) {return lhs < rhs.freq;}
	inline bool operator()(const HuffmanNode &lhs, const HuffmanNode &rhs) const {return lhs.freq < rhs.freq;}
	// needed for MSVC .NET 2005
	inline bool operator()(const HuffmanNode &lhs, unsigned int rhs) {return lhs.freq < rhs;}
};

void HuffmanEncoder::GenerateCodeLengths(unsigned int *codeBits, unsigned int maxCodeBits, const unsigned int *codeCounts, size_t nCodes)
{
	CRYPTOPP_ASSERT(nCodes > 0);
	CRYPTOPP_ASSERT(nCodes <= ((size_t)1 << maxCodeBits));

	size_t i;
	SecBlockWithHint<HuffmanNode, 2*286> tree(nCodes);
	for (i=0; i<nCodes; i++)
	{
		tree[i].symbol = i;
		tree[i].freq = codeCounts[i];
	}
	std::sort(tree.begin(), tree.end(), FreqLessThan());
	size_t treeBegin = std::upper_bound(tree.begin(), tree.end(), 0, FreqLessThan()) - tree.begin();
	if (treeBegin == nCodes)
	{	// special case for no codes
		std::fill(codeBits, codeBits+nCodes, 0);
		return;
	}
	tree.resize(nCodes + nCodes - treeBegin - 1);

	size_t leastLeaf = treeBegin, leastInterior = nCodes;
	for (i=nCodes; i<tree.size(); i++)
	{
		size_t least;
		least = (leastLeaf == nCodes || (leastInterior < i && tree[leastInterior].freq < tree[leastLeaf].freq)) ? leastInterior++ : leastLeaf++;
		tree[i].freq = tree[least].freq;
		tree[least].parent = i;
		least = (leastLeaf == nCodes || (leastInterior < i && tree[leastInterior].freq < tree[leastLeaf].freq)) ? leastInterior++ : leastLeaf++;
		tree[i].freq += tree[least].freq;
		tree[least].parent = i;
	}

	tree[tree.size()-1].depth = 0;
	if (tree.size() >= 2)
		for (i=tree.size()-2; i>=nCodes; i--)
			tree[i].depth = tree[tree[i].parent].depth + 1;
	unsigned int sum = 0;
	SecBlockWithHint<unsigned int, 15+1> blCount(maxCodeBits+1);
	std::fill(blCount.begin(), blCount.end(), 0);
	for (i=treeBegin; i<nCodes; i++)
	{
		const size_t n = tree[i].parent;
		const size_t depth = STDMIN(maxCodeBits, tree[n].depth + 1);
		blCount[depth]++;
		sum += 1 << (maxCodeBits - depth);
	}

	unsigned int overflow = sum > (unsigned int)(1 << maxCodeBits) ? sum - (1 << maxCodeBits) : 0;

	while (overflow--)
	{
		unsigned int bits = maxCodeBits-1;
		while (blCount[bits] == 0)
			bits--;
		blCount[bits]--;
		blCount[bits+1] += 2;
		CRYPTOPP_ASSERT(blCount[maxCodeBits] > 0);
		blCount[maxCodeBits]--;
	}

	for (i=0; i<treeBegin; i++)
		codeBits[tree[i].symbol] = 0;
	unsigned int bits = maxCodeBits;
	for (i=treeBegin; i<nCodes; i++)
	{
		while (blCount[bits] == 0)
			bits--;
		codeBits[tree[i].symbol] = bits;
		blCount[bits]--;
	}
	CRYPTOPP_ASSERT(blCount[bits] == 0);
}

void HuffmanEncoder::Initialize(const unsigned int *codeBits, unsigned int nCodes)
{
	CRYPTOPP_ASSERT(nCodes > 0);
	unsigned int maxCodeBits = *std::max_element(codeBits, codeBits+nCodes);
	if (maxCodeBits == 0)
		return;		// assume this object won't be used

	SecBlockWithHint<unsigned int, 15+1> blCount(maxCodeBits+1);
	std::fill(blCount.begin(), blCount.end(), 0);
	unsigned int i;
	for (i=0; i<nCodes; i++)
		blCount[codeBits[i]]++;

	code_t code = 0;
	SecBlockWithHint<code_t, 15+1> nextCode(maxCodeBits+1);
	nextCode[1] = 0;
	for (i=2; i<=maxCodeBits; i++)
	{
		code = (code + blCount[i-1]) << 1;
		nextCode[i] = code;
	}
	CRYPTOPP_ASSERT(maxCodeBits == 1 || code == (1 << maxCodeBits) - blCount[maxCodeBits]);

	m_valueToCode.resize(nCodes);
	for (i=0; i<nCodes; i++)
	{
		unsigned int len = m_valueToCode[i].len = codeBits[i];
		if (len != 0)
			m_valueToCode[i].code = BitReverse(nextCode[len]++) >> (8*sizeof(code_t)-len);
	}
}

inline void HuffmanEncoder::Encode(LowFirstBitWriter &writer, value_t value) const
{
	CRYPTOPP_ASSERT(m_valueToCode[value].len > 0);
	writer.PutBits(m_valueToCode[value].code, m_valueToCode[value].len);
}

Deflator::Deflator(BufferedTransformation *attachment, int deflateLevel, int log2WindowSize, bool detectUncompressible)
	: LowFirstBitWriter(attachment)
	, m_deflateLevel(-1)
{
	InitializeStaticEncoders();
	IsolatedInitialize(MakeParameters("DeflateLevel", deflateLevel)("Log2WindowSize", log2WindowSize)("DetectUncompressible", detectUncompressible));
}

Deflator::Deflator(const NameValuePairs &parameters, BufferedTransformation *attachment)
	: LowFirstBitWriter(attachment)
	, m_deflateLevel(-1)
{
	InitializeStaticEncoders();
	IsolatedInitialize(parameters);
}

void Deflator::InitializeStaticEncoders()
{
	unsigned int codeLengths[288];
	std::fill(codeLengths + 0, codeLengths + 144, 8);
	std::fill(codeLengths + 144, codeLengths + 256, 9);
	std::fill(codeLengths + 256, codeLengths + 280, 7);
	std::fill(codeLengths + 280, codeLengths + 288, 8);
	m_staticLiteralEncoder.Initialize(codeLengths, 288);
	std::fill(codeLengths + 0, codeLengths + 32, 5);
	m_staticDistanceEncoder.Initialize(codeLengths, 32);
}

void Deflator::IsolatedInitialize(const NameValuePairs &parameters)
{
	int log2WindowSize = parameters.GetIntValueWithDefault("Log2WindowSize", DEFAULT_LOG2_WINDOW_SIZE);
	if (!(MIN_LOG2_WINDOW_SIZE <= log2WindowSize && log2WindowSize <= MAX_LOG2_WINDOW_SIZE))
		throw InvalidArgument("Deflator: " + IntToString(log2WindowSize) + " is an invalid window size");

	m_log2WindowSize = log2WindowSize;
	DSIZE = 1 << m_log2WindowSize;
	DMASK = DSIZE - 1;
	HSIZE = 1 << m_log2WindowSize;
	HMASK = HSIZE - 1;
	m_byteBuffer.New(2*DSIZE);
	m_head.New(HSIZE);
	m_prev.New(DSIZE);
	m_matchBuffer.New(DSIZE/2);
	Reset(true);

	const int deflateLevel = parameters.GetIntValueWithDefault("DeflateLevel", DEFAULT_DEFLATE_LEVEL);
	CRYPTOPP_ASSERT(deflateLevel >= MIN_DEFLATE_LEVEL /*0*/ && deflateLevel <= MAX_DEFLATE_LEVEL /*9*/);
	SetDeflateLevel(deflateLevel);
	bool detectUncompressible = parameters.GetValueWithDefault("DetectUncompressible", true);
	m_compressibleDeflateLevel = detectUncompressible ? m_deflateLevel : 0;
}

void Deflator::Reset(bool forceReset)
{
	if (forceReset)
		ClearBitBuffer();
	else
		CRYPTOPP_ASSERT(m_bitsBuffered == 0);

	m_headerWritten = false;
	m_matchAvailable = false;
	m_dictionaryEnd = 0;
	m_stringStart = 0;
	m_lookahead = 0;
	m_minLookahead = MAX_MATCH;
	m_matchBufferEnd = 0;
	m_blockStart = 0;
	m_blockLength = 0;

	m_detectCount = 1;
	m_detectSkip = 0;

	// m_prev will be initialized automaticly in InsertString
	std::fill(m_head.begin(), m_head.end(), byte(0));

	std::fill(m_literalCounts.begin(), m_literalCounts.end(), byte(0));
	std::fill(m_distanceCounts.begin(), m_distanceCounts.end(), byte(0));
}

void Deflator::SetDeflateLevel(int deflateLevel)
{
	if (!(MIN_DEFLATE_LEVEL <= deflateLevel && deflateLevel <= MAX_DEFLATE_LEVEL))
		throw InvalidArgument("Deflator: " + IntToString(deflateLevel) + " is an invalid deflate level");

	if (deflateLevel == m_deflateLevel)
		return;

	EndBlock(false);

	static const unsigned int configurationTable[10][4] = {
		/*      good lazy nice chain */
		/* 0 */ {0,    0,  0,    0},  /* store only */
		/* 1 */ {4,    3,  8,    4},  /* maximum speed, no lazy matches */
		/* 2 */ {4,    3, 16,    8},
		/* 3 */ {4,    3, 32,   32},
		/* 4 */ {4,    4, 16,   16},  /* lazy matches */
		/* 5 */ {8,   16, 32,   32},
		/* 6 */ {8,   16, 128, 128},
		/* 7 */ {8,   32, 128, 256},
		/* 8 */ {32, 128, 258, 1024},
		/* 9 */ {32, 258, 258, 4096}}; /* maximum compression */

	GOOD_MATCH = configurationTable[deflateLevel][0];
	MAX_LAZYLENGTH = configurationTable[deflateLevel][1];
	MAX_CHAIN_LENGTH = configurationTable[deflateLevel][3];

	m_deflateLevel = deflateLevel;
}

unsigned int Deflator::FillWindow(const byte *str, size_t length)
{
	unsigned int maxBlockSize = (unsigned int)STDMIN(2UL*DSIZE, 0xffffUL);

	if (m_stringStart >= maxBlockSize - MAX_MATCH)
	{
		if (m_blockStart < DSIZE)
			EndBlock(false);

		memcpy(m_byteBuffer, m_byteBuffer + DSIZE, DSIZE);

		m_dictionaryEnd = m_dictionaryEnd < DSIZE ? 0 : m_dictionaryEnd-DSIZE;
		CRYPTOPP_ASSERT(m_stringStart >= DSIZE);
		m_stringStart -= DSIZE;
		CRYPTOPP_ASSERT(!m_matchAvailable || m_previousMatch >= DSIZE);
		m_previousMatch -= DSIZE;
		CRYPTOPP_ASSERT(m_blockStart >= DSIZE);
		m_blockStart -= DSIZE;

		// These are set to the same value in IsolatedInitialize(). If they
		//   are the same, then we can clear a Coverity false alarm.
		CRYPTOPP_ASSERT(DSIZE == HSIZE);

		unsigned int i;
		for (i=0; i<HSIZE; i++)
			m_head[i] = SaturatingSubtract(m_head[i], HSIZE); // was DSIZE???

		for (i=0; i<DSIZE; i++)
			m_prev[i] = SaturatingSubtract(m_prev[i], DSIZE);
	}

	CRYPTOPP_ASSERT(maxBlockSize > m_stringStart+m_lookahead);
	unsigned int accepted = UnsignedMin(maxBlockSize-(m_stringStart+m_lookahead), length);
	CRYPTOPP_ASSERT(accepted > 0);
	memcpy(m_byteBuffer + m_stringStart + m_lookahead, str, accepted);
	m_lookahead += accepted;
	return accepted;
}

inline unsigned int Deflator::ComputeHash(const byte *str) const
{
	CRYPTOPP_ASSERT(str+3 <= m_byteBuffer + m_stringStart + m_lookahead);
	return ((str[0] << 10) ^ (str[1] << 5) ^ str[2]) & HMASK;
}

unsigned int Deflator::LongestMatch(unsigned int &bestMatch) const
{
	CRYPTOPP_ASSERT(m_previousLength < MAX_MATCH);

	bestMatch = 0;
	unsigned int bestLength = STDMAX(m_previousLength, (unsigned int)MIN_MATCH-1);
	if (m_lookahead <= bestLength)
		return 0;

	const byte *scan = m_byteBuffer + m_stringStart, *scanEnd = scan + STDMIN((unsigned int)MAX_MATCH, m_lookahead);
	unsigned int limit = m_stringStart > (DSIZE-MAX_MATCH) ? m_stringStart - (DSIZE-MAX_MATCH) : 0;
	unsigned int current = m_head[ComputeHash(scan)];

	unsigned int chainLength = MAX_CHAIN_LENGTH;
	if (m_previousLength >= GOOD_MATCH)
		chainLength >>= 2;

	while (current > limit && --chainLength > 0)
	{
		const byte *match = m_byteBuffer + current;
		CRYPTOPP_ASSERT(scan + bestLength < m_byteBuffer + m_stringStart + m_lookahead);
		if (scan[bestLength-1] == match[bestLength-1] && scan[bestLength] == match[bestLength] && scan[0] == match[0] && scan[1] == match[1])
		{
			CRYPTOPP_ASSERT(scan[2] == match[2]);
			unsigned int len = (unsigned int)(
#if defined(_STDEXT_BEGIN) && !(defined(_MSC_VER) && (_MSC_VER < 1400 || _MSC_VER >= 1600)) && !defined(_STLPORT_VERSION)
				stdext::unchecked_mismatch
#else
				std::mismatch
#endif
#if _MSC_VER >= 1600
				(stdext::make_unchecked_array_iterator(scan)+3, stdext::make_unchecked_array_iterator(scanEnd), stdext::make_unchecked_array_iterator(match)+3).first - stdext::make_unchecked_array_iterator(scan));
#else
				(scan+3, scanEnd, match+3).first - scan);
#endif
			CRYPTOPP_ASSERT(len != bestLength);
			if (len > bestLength)
			{
				bestLength = len;
				bestMatch = current;

				CRYPTOPP_ASSERT(scanEnd >= scan);
				if (len == (unsigned int)(scanEnd - scan))
					break;
			}
		}
		current = m_prev[current & DMASK];
	}
	return (bestMatch > 0) ? bestLength : 0;
}

inline void Deflator::InsertString(unsigned int start)
{
	CRYPTOPP_ASSERT(start <= 0xffff);
	unsigned int hash = ComputeHash(m_byteBuffer + start);
	m_prev[start & DMASK] = m_head[hash];
	m_head[hash] = word16(start);
}

void Deflator::ProcessBuffer()
{
	if (!m_headerWritten)
	{
		WritePrestreamHeader();
		m_headerWritten = true;
	}

	if (m_deflateLevel == 0)
	{
		m_stringStart += m_lookahead;
		m_lookahead = 0;
		m_blockLength = m_stringStart - m_blockStart;
		m_matchAvailable = false;
		return;
	}

	while (m_lookahead > m_minLookahead)
	{
		while (m_dictionaryEnd < m_stringStart && m_dictionaryEnd+3 <= m_stringStart+m_lookahead)
			InsertString(m_dictionaryEnd++);

		if (m_matchAvailable)
		{
			unsigned int matchPosition = 0, matchLength = 0;
			bool usePreviousMatch;
			if (m_previousLength >= MAX_LAZYLENGTH)
				usePreviousMatch = true;
			else
			{
				matchLength = LongestMatch(matchPosition);
				usePreviousMatch = (matchLength == 0);
			}
			if (usePreviousMatch)
			{
				MatchFound(m_stringStart-1-m_previousMatch, m_previousLength);
				m_stringStart += m_previousLength-1;
				m_lookahead -= m_previousLength-1;
				m_matchAvailable = false;
			}
			else
			{
				m_previousLength = matchLength;
				m_previousMatch = matchPosition;
				LiteralByte(m_byteBuffer[m_stringStart-1]);
				m_stringStart++;
				m_lookahead--;
			}
		}
		else
		{
			m_previousLength = 0;
			m_previousLength = LongestMatch(m_previousMatch);
			if (m_previousLength)
				m_matchAvailable = true;
			else
				LiteralByte(m_byteBuffer[m_stringStart]);
			m_stringStart++;
			m_lookahead--;
		}

		CRYPTOPP_ASSERT(m_stringStart - (m_blockStart+m_blockLength) == (unsigned int)m_matchAvailable);
	}

	if (m_minLookahead == 0 && m_matchAvailable)
	{
		LiteralByte(m_byteBuffer[m_stringStart-1]);
		m_matchAvailable = false;
	}
}

size_t Deflator::Put2(const byte *str, size_t length, int messageEnd, bool blocking)
{
	if (!blocking)
		throw BlockingInputOnly("Deflator");

	size_t accepted = 0;
	while (accepted < length)
	{
		unsigned int newAccepted = FillWindow(str+accepted, length-accepted);
		ProcessBuffer();
		// call ProcessUncompressedData() after WritePrestreamHeader()
		ProcessUncompressedData(str+accepted, newAccepted);
		accepted += newAccepted;
	}
	CRYPTOPP_ASSERT(accepted == length);

	if (messageEnd)
	{
		m_minLookahead = 0;
		ProcessBuffer();
		EndBlock(true);
		FlushBitBuffer();
		WritePoststreamTail();
		Reset();
	}

	Output(0, NULL, 0, messageEnd, blocking);
	return 0;
}

bool Deflator::IsolatedFlush(bool hardFlush, bool blocking)
{
	if (!blocking)
		throw BlockingInputOnly("Deflator");

	m_minLookahead = 0;
	ProcessBuffer();
	m_minLookahead = MAX_MATCH;
	EndBlock(false);
	if (hardFlush)
		EncodeBlock(false, STORED);
	return false;
}

void Deflator::LiteralByte(byte b)
{
	if (m_matchBufferEnd == m_matchBuffer.size())
		EndBlock(false);

	m_matchBuffer[m_matchBufferEnd++].literalCode = b;
	m_literalCounts[b]++;
	m_blockLength++;
}

void Deflator::MatchFound(unsigned int distance, unsigned int length)
{
	if (m_matchBufferEnd == m_matchBuffer.size())
		EndBlock(false);

	static const unsigned int lengthCodes[] = {
		257, 258, 259, 260, 261, 262, 263, 264, 265, 265, 266, 266, 267, 267, 268, 268,
		269, 269, 269, 269, 270, 270, 270, 270, 271, 271, 271, 271, 272, 272, 272, 272,
		273, 273, 273, 273, 273, 273, 273, 273, 274, 274, 274, 274, 274, 274, 274, 274,
		275, 275, 275, 275, 275, 275, 275, 275, 276, 276, 276, 276, 276, 276, 276, 276,
		277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
		278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
		279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279,
		280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280,
		281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281,
		281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281,
		282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282,
		282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282,
		283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283,
		283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283,
		284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284,
		284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 285};
	static const unsigned int lengthBases[] =
		{3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,
		 227,258};
	static const unsigned int distanceBases[30] =
		{1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,
		 4097,6145,8193,12289,16385,24577};

	CRYPTOPP_ASSERT(m_matchBufferEnd < m_matchBuffer.size());
	EncodedMatch &m = m_matchBuffer[m_matchBufferEnd++];
	CRYPTOPP_ASSERT((length >= 3) && (length-3 < COUNTOF(lengthCodes)));
	unsigned int lengthCode = lengthCodes[length-3];
	m.literalCode = lengthCode;
	m.literalExtra = length - lengthBases[lengthCode-257];
	unsigned int distanceCode = (unsigned int)(std::upper_bound(distanceBases, distanceBases+30, distance) - distanceBases - 1);
	m.distanceCode = distanceCode;
	m.distanceExtra = distance - distanceBases[distanceCode];

	m_literalCounts[lengthCode]++;
	m_distanceCounts[distanceCode]++;
	m_blockLength += length;
}

inline unsigned int CodeLengthEncode(const unsigned int *begin,
									 const unsigned int *end,
									 const unsigned int *& p,
									 unsigned int &extraBits,
									 unsigned int &extraBitsLength)
{
	unsigned int v = *p;
	if ((end-p) >= 3)
	{
		const unsigned int *oldp = p;
		if (v==0 && p[1]==0 && p[2]==0)
		{
			for (p=p+3; p!=end && *p==0 && p!=oldp+138; p++) {}
			unsigned int repeat = (unsigned int)(p - oldp);
			if (repeat <= 10)
			{
				extraBits = repeat-3;
				extraBitsLength = 3;
				return 17;
			}
			else
			{
				extraBits = repeat-11;
				extraBitsLength = 7;
				return 18;
			}
		}
		else if (p!=begin && v==p[-1] && v==p[1] && v==p[2])
		{
			for (p=p+3; p!=end && *p==v && p!=oldp+6; p++) {}
			unsigned int repeat = (unsigned int)(p - oldp);
			extraBits = repeat-3;
			extraBitsLength = 2;
			return 16;
		}
	}
	p++;
	extraBits = 0;
	extraBitsLength = 0;
	return v;
}

void Deflator::EncodeBlock(bool eof, unsigned int blockType)
{
	PutBits(eof, 1);
	PutBits(blockType, 2);

	if (blockType == STORED)
	{
		CRYPTOPP_ASSERT(m_blockStart + m_blockLength <= m_byteBuffer.size());
		CRYPTOPP_ASSERT(m_blockLength <= 0xffff);
		FlushBitBuffer();
		AttachedTransformation()->PutWord16(word16(m_blockLength), LITTLE_ENDIAN_ORDER);
		AttachedTransformation()->PutWord16(word16(~m_blockLength), LITTLE_ENDIAN_ORDER);
		AttachedTransformation()->Put(m_byteBuffer + m_blockStart, m_blockLength);
	}
	else
	{
		if (blockType == DYNAMIC)
		{
#if defined(_MSC_VER) && !defined(__MWERKS__) && (_MSC_VER <= 1300)
			// VC60 and VC7 workaround: built-in std::reverse_iterator has two template parameters, Dinkumware only has one
			typedef std::reverse_bidirectional_iterator<unsigned int *, unsigned int> RevIt;
#elif defined(_RWSTD_NO_CLASS_PARTIAL_SPEC)
			typedef std::reverse_iterator<unsigned int *, std::random_access_iterator_tag, unsigned int> RevIt;
#else
			typedef std::reverse_iterator<unsigned int *> RevIt;
#endif

			FixedSizeSecBlock<unsigned int, 286> literalCodeLengths;
			FixedSizeSecBlock<unsigned int, 30> distanceCodeLengths;

			m_literalCounts[256] = 1;
			HuffmanEncoder::GenerateCodeLengths(literalCodeLengths, 15, m_literalCounts, 286);
			m_dynamicLiteralEncoder.Initialize(literalCodeLengths, 286);
			unsigned int hlit = (unsigned int)(std::find_if(RevIt(literalCodeLengths.end()), RevIt(literalCodeLengths.begin()+257), std::bind2nd(std::not_equal_to<unsigned int>(), 0)).base() - (literalCodeLengths.begin()+257));

			HuffmanEncoder::GenerateCodeLengths(distanceCodeLengths, 15, m_distanceCounts, 30);
			m_dynamicDistanceEncoder.Initialize(distanceCodeLengths, 30);
			unsigned int hdist = (unsigned int)(std::find_if(RevIt(distanceCodeLengths.end()), RevIt(distanceCodeLengths.begin()+1), std::bind2nd(std::not_equal_to<unsigned int>(), 0)).base() - (distanceCodeLengths.begin()+1));

			SecBlockWithHint<unsigned int, 286+30> combinedLengths(hlit+257+hdist+1);
			memcpy(combinedLengths, literalCodeLengths, (hlit+257)*sizeof(unsigned int));
			memcpy(combinedLengths+hlit+257, distanceCodeLengths, (hdist+1)*sizeof(unsigned int));

			FixedSizeSecBlock<unsigned int, 19> codeLengthCodeCounts, codeLengthCodeLengths;
			std::fill(codeLengthCodeCounts.begin(), codeLengthCodeCounts.end(), 0);
			const unsigned int *p = combinedLengths.begin(), *begin = combinedLengths.begin(), *end = combinedLengths.end();
			while (p != end)
			{
				unsigned int code=0, extraBits=0, extraBitsLength=0;
				code = CodeLengthEncode(begin, end, p, extraBits, extraBitsLength);
				codeLengthCodeCounts[code]++;
			}
			HuffmanEncoder::GenerateCodeLengths(codeLengthCodeLengths, 7, codeLengthCodeCounts, 19);
			HuffmanEncoder codeLengthEncoder(codeLengthCodeLengths, 19);
			static const unsigned int border[] = {    // Order of the bit length code lengths
				16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
			unsigned int hclen = 19;
			while (hclen > 4 && codeLengthCodeLengths[border[hclen-1]] == 0)
				hclen--;
			hclen -= 4;

			PutBits(hlit, 5);
			PutBits(hdist, 5);
			PutBits(hclen, 4);

			for (unsigned int i=0; i<hclen+4; i++)
				PutBits(codeLengthCodeLengths[border[i]], 3);

			p = combinedLengths.begin();
			while (p != end)
			{
				unsigned int code=0, extraBits=0, extraBitsLength=0;
				code = CodeLengthEncode(begin, end, p, extraBits, extraBitsLength);
				codeLengthEncoder.Encode(*this, code);
				PutBits(extraBits, extraBitsLength);
			}
		}

		static const unsigned int lengthExtraBits[] = {
			0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
			3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
		static const unsigned int distanceExtraBits[] = {
			0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
			7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
			12, 12, 13, 13};

		const HuffmanEncoder &literalEncoder = (blockType == STATIC) ? m_staticLiteralEncoder : m_dynamicLiteralEncoder;
		const HuffmanEncoder &distanceEncoder = (blockType == STATIC) ? m_staticDistanceEncoder : m_dynamicDistanceEncoder;

		for (unsigned int i=0; i<m_matchBufferEnd; i++)
		{
			unsigned int literalCode = m_matchBuffer[i].literalCode;
			literalEncoder.Encode(*this, literalCode);
			if (literalCode >= 257)
			{
				CRYPTOPP_ASSERT(literalCode <= 285);
				PutBits(m_matchBuffer[i].literalExtra, lengthExtraBits[literalCode-257]);
				unsigned int distanceCode = m_matchBuffer[i].distanceCode;
				distanceEncoder.Encode(*this, distanceCode);
				PutBits(m_matchBuffer[i].distanceExtra, distanceExtraBits[distanceCode]);
			}
		}
		literalEncoder.Encode(*this, 256);	// end of block
	}
}

void Deflator::EndBlock(bool eof)
{
	if (m_blockLength == 0 && !eof)
		return;

	if (m_deflateLevel == 0)
	{
		EncodeBlock(eof, STORED);

		if (m_compressibleDeflateLevel > 0 && ++m_detectCount == m_detectSkip)
		{
			m_deflateLevel = m_compressibleDeflateLevel;
			m_detectCount = 1;
		}
	}
	else
	{
		unsigned long storedLen = 8*((unsigned long)m_blockLength+4) + RoundUpToMultipleOf(m_bitsBuffered+3, 8U)-m_bitsBuffered;

		StartCounting();
		EncodeBlock(eof, STATIC);
		unsigned long staticLen = FinishCounting();

		unsigned long dynamicLen;
		if (m_blockLength < 128 && m_deflateLevel < 8)
			dynamicLen = ULONG_MAX;
		else
		{
			StartCounting();
			EncodeBlock(eof, DYNAMIC);
			dynamicLen = FinishCounting();
		}

		if (storedLen <= staticLen && storedLen <= dynamicLen)
		{
			EncodeBlock(eof, STORED);

			if (m_compressibleDeflateLevel > 0)
			{
				if (m_detectSkip)
					m_deflateLevel = 0;
				m_detectSkip = m_detectSkip ? STDMIN(2*m_detectSkip, 128U) : 1;
			}
		}
		else
		{
			if (staticLen <= dynamicLen)
				EncodeBlock(eof, STATIC);
			else
				EncodeBlock(eof, DYNAMIC);

			if (m_compressibleDeflateLevel > 0)
				m_detectSkip = 0;
		}
	}

	m_matchBufferEnd = 0;
	m_blockStart += m_blockLength;
	m_blockLength = 0;
	std::fill(m_literalCounts.begin(), m_literalCounts.end(), 0);
	std::fill(m_distanceCounts.begin(), m_distanceCounts.end(), 0);
}

NAMESPACE_END
