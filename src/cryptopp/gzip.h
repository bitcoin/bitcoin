// gzip.h - written and placed in the public domain by Wei Dai

//! \file gzip.h
//! \brief GZIP compression and decompression (RFC 1952)

#ifndef CRYPTOPP_GZIP_H
#define CRYPTOPP_GZIP_H

#include "cryptlib.h"
#include "zdeflate.h"
#include "zinflate.h"
#include "crc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Gzip
//! \brief GZIP Compression (RFC 1952)
class Gzip : public Deflator
{
public:
	//! \brief Construct a Gzip compressor
	//! \param attachment an attached transformation
	//! \param deflateLevel the deflate level
	//! \param log2WindowSize the window size
	//! \param detectUncompressible flag to detect if data is compressible
	//! \details detectUncompressible makes it faster to process uncompressible files, but
	//!   if a file has both compressible and uncompressible parts, it may fail to compress
	//!   some of the compressible parts.
	Gzip(BufferedTransformation *attachment=NULL, unsigned int deflateLevel=DEFAULT_DEFLATE_LEVEL, unsigned int log2WindowSize=DEFAULT_LOG2_WINDOW_SIZE, bool detectUncompressible=true)
		: Deflator(attachment, deflateLevel, log2WindowSize, detectUncompressible), m_totalLen(0) {}
	//! \brief Construct a Gzip compressor
	//! \param parameters a set of NameValuePairs to initialize this object
	//! \param attachment an attached transformation
	//! \details Possible parameter names: Log2WindowSize, DeflateLevel, DetectUncompressible
	Gzip(const NameValuePairs &parameters, BufferedTransformation *attachment=NULL)
		: Deflator(parameters, attachment), m_totalLen(0) {}

protected:
	enum {MAGIC1=0x1f, MAGIC2=0x8b,   // flags for the header
		  DEFLATED=8, FAST=4, SLOW=2};

	void WritePrestreamHeader();
	void ProcessUncompressedData(const byte *string, size_t length);
	void WritePoststreamTail();

	word32 m_totalLen;
	CRC32 m_crc;
};

//! \class Gunzip
//! \brief GZIP Decompression (RFC 1952)
class Gunzip : public Inflator
{
public:
	typedef Inflator::Err Err;

	//! \class HeaderErr
	//! \brief Exception thrown when a header decoding error occurs
	class HeaderErr : public Err {public: HeaderErr() : Err(INVALID_DATA_FORMAT, "Gunzip: header decoding error") {}};
	//! \class TailErr
	//! \brief Exception thrown when the tail is too short
	class TailErr : public Err {public: TailErr() : Err(INVALID_DATA_FORMAT, "Gunzip: tail too short") {}};
	//! \class CrcErr
	//! \brief Exception thrown when a CRC error occurs
	class CrcErr : public Err {public: CrcErr() : Err(DATA_INTEGRITY_CHECK_FAILED, "Gunzip: CRC check error") {}};
	//! \class LengthErr
	//! \brief Exception thrown when a length error occurs
	class LengthErr : public Err {public: LengthErr() : Err(DATA_INTEGRITY_CHECK_FAILED, "Gunzip: length check error") {}};

	//! \brief Construct a Gunzip decompressor
	//! \param attachment an attached transformation
	//! \param repeat decompress multiple compressed streams in series
	//! \param autoSignalPropagation 0 to turn off MessageEnd signal
	Gunzip(BufferedTransformation *attachment = NULL, bool repeat = false, int autoSignalPropagation = -1);

protected:
	enum {
		//! \brief First header magic value
		MAGIC1=0x1f,
		//! \brief Second header magic value
		MAGIC2=0x8b,
		//! \brief Deflated flag
		DEFLATED=8
	};

	enum FLAG_MASKS {
		CONTINUED=2, EXTRA_FIELDS=4, FILENAME=8, COMMENTS=16, ENCRYPTED=32};

	unsigned int MaxPrestreamHeaderSize() const {return 1024;}
	void ProcessPrestreamHeader();
	void ProcessDecompressedData(const byte *string, size_t length);
	unsigned int MaxPoststreamTailSize() const {return 8;}
	void ProcessPoststreamTail();

	word32 m_length;
	CRC32 m_crc;
};

NAMESPACE_END

#endif
