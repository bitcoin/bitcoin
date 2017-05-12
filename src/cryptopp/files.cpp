// files.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "files.h"

#include <limits>

NAMESPACE_BEGIN(CryptoPP)

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void Files_TestInstantiations()
{
	FileStore f0;
	FileSource f1;
	FileSink f2;
}
#endif

void FileStore::StoreInitialize(const NameValuePairs &parameters)
{
	m_waiting = false;
	m_stream = NULL;
	m_file.release();

	const char *fileName = NULL;
#if defined(CRYPTOPP_UNIX_AVAILABLE) || _MSC_VER >= 1400
	const wchar_t *fileNameWide = NULL;
	if (!parameters.GetValue(Name::InputFileNameWide(), fileNameWide))
#endif
		if (!parameters.GetValue(Name::InputFileName(), fileName))
		{
			parameters.GetValue(Name::InputStreamPointer(), m_stream);
			return;
		}

	std::ios::openmode binary = parameters.GetValueWithDefault(Name::InputBinaryMode(), true) ? std::ios::binary : std::ios::openmode(0);
	m_file.reset(new std::ifstream);
#ifdef CRYPTOPP_UNIX_AVAILABLE
	std::string narrowed;
	if (fileNameWide)
		fileName = (narrowed = StringNarrow(fileNameWide)).c_str();
#endif
#if _MSC_VER >= 1400
	if (fileNameWide)
	{
		m_file->open(fileNameWide, std::ios::in | binary);
		if (!*m_file)
			throw OpenErr(StringNarrow(fileNameWide, false));
	}
#endif
	if (fileName)
	{
		m_file->open(fileName, std::ios::in | binary);
		if (!*m_file)
			throw OpenErr(fileName);
	}
	m_stream = m_file.get();
}

lword FileStore::MaxRetrievable() const
{
	if (!m_stream)
		return 0;

	std::streampos current = m_stream->tellg();
	std::streampos end = m_stream->seekg(0, std::ios::end).tellg();
	m_stream->seekg(current);
	return end-current;
}

size_t FileStore::TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel, bool blocking)
{
	if (!m_stream)
	{
		transferBytes = 0;
		return 0;
	}

	lword size=transferBytes;
	transferBytes = 0;

	if (m_waiting)
		goto output;

	while (size && m_stream->good())
	{
		{
		size_t spaceSize = 1024;
		m_space = HelpCreatePutSpace(target, channel, 1, UnsignedMin(size_t(SIZE_MAX), size), spaceSize);

		m_stream->read((char *)m_space, (unsigned int)STDMIN(size, (lword)spaceSize));
		}
		m_len = (size_t)m_stream->gcount();
		size_t blockedBytes;
output:
		blockedBytes = target.ChannelPutModifiable2(channel, m_space, m_len, 0, blocking);
		m_waiting = blockedBytes > 0;
		if (m_waiting)
			return blockedBytes;
		size -= m_len;
		transferBytes += m_len;
	}

	if (!m_stream->good() && !m_stream->eof())
		throw ReadErr();

	return 0;
}

size_t FileStore::CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end, const std::string &channel, bool blocking) const
{
	if (!m_stream)
		return 0;

	if (begin == 0 && end == 1)
	{
		int result = m_stream->peek();
		if (result == std::char_traits<char>::eof())
			return 0;
		else
		{
			size_t blockedBytes = target.ChannelPut(channel, byte(result), blocking);
			begin += 1-blockedBytes;
			return blockedBytes;
		}
	}

	// TODO: figure out what happens on cin
	std::streampos current = m_stream->tellg();
	std::streampos endPosition = m_stream->seekg(0, std::ios::end).tellg();
	std::streampos newPosition = current + static_cast<std::streamoff>(begin);

	if (newPosition >= endPosition)
	{
		m_stream->seekg(current);
		return 0;	// don't try to seek beyond the end of file
	}
	m_stream->seekg(newPosition);
	try
	{
		CRYPTOPP_ASSERT(!m_waiting);
		lword copyMax = end-begin;
		size_t blockedBytes = const_cast<FileStore *>(this)->TransferTo2(target, copyMax, channel, blocking);
		begin += copyMax;
		if (blockedBytes)
		{
			const_cast<FileStore *>(this)->m_waiting = false;
			return blockedBytes;
		}
	}
	catch(...)
	{
		m_stream->clear();
		m_stream->seekg(current);
		throw;
	}
	m_stream->clear();
	m_stream->seekg(current);

	return 0;
}

lword FileStore::Skip(lword skipMax)
{
	if (!m_stream)
		return 0;

	lword oldPos = m_stream->tellg();
	std::istream::off_type offset;
	if (!SafeConvert(skipMax, offset))
		throw InvalidArgument("FileStore: maximum seek offset exceeded");
	m_stream->seekg(offset, std::ios::cur);
	return (lword)m_stream->tellg() - oldPos;
}

void FileSink::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_stream = NULL;
	m_file.release();

	const char *fileName = NULL;
#if defined(CRYPTOPP_UNIX_AVAILABLE) || _MSC_VER >= 1400
	const wchar_t *fileNameWide = NULL;
	if (!parameters.GetValue(Name::OutputFileNameWide(), fileNameWide))
#endif
		if (!parameters.GetValue(Name::OutputFileName(), fileName))
		{
			parameters.GetValue(Name::OutputStreamPointer(), m_stream);
			return;
		}

	std::ios::openmode binary = parameters.GetValueWithDefault(Name::OutputBinaryMode(), true) ? std::ios::binary : std::ios::openmode(0);
	m_file.reset(new std::ofstream);
#ifdef CRYPTOPP_UNIX_AVAILABLE
	std::string narrowed;
	if (fileNameWide)
		fileName = (narrowed = StringNarrow(fileNameWide)).c_str();
#elif (CRYPTOPP_MSC_VERSION >= 1400)
	if (fileNameWide)
	{
		m_file->open(fileNameWide, std::ios::out | std::ios::trunc | binary);
		if (!*m_file)
			throw OpenErr(StringNarrow(fileNameWide, false));
	}
#endif
	if (fileName)
	{
		m_file->open(fileName, std::ios::out | std::ios::trunc | binary);
		if (!*m_file)
			throw OpenErr(fileName);
	}
	m_stream = m_file.get();
}

bool FileSink::IsolatedFlush(bool hardFlush, bool blocking)
{
	CRYPTOPP_UNUSED(hardFlush), CRYPTOPP_UNUSED(blocking);
	if (!m_stream)
		throw Err("FileSink: output stream not opened");

	m_stream->flush();
	if (!m_stream->good())
		throw WriteErr();

	return false;
}

size_t FileSink::Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
{
	CRYPTOPP_UNUSED(blocking);
	if (!m_stream)
		throw Err("FileSink: output stream not opened");

	while (length > 0)
	{
		std::streamsize size;
		if (!SafeConvert(length, size))
			size = ((std::numeric_limits<std::streamsize>::max)());
		m_stream->write((const char *)inString, size);
		inString += size;
		length -= (size_t)size;
	}

	if (messageEnd)
		m_stream->flush();

	if (!m_stream->good())
		throw WriteErr();

	return 0;
}

NAMESPACE_END

#endif
