#ifndef CRYPTOPP_FILES_H
#define CRYPTOPP_FILES_H

#include "cryptlib.h"
#include "filters.h"
#include "argnames.h"
#include "smartptr.h"

#include <iostream>
#include <fstream>

NAMESPACE_BEGIN(CryptoPP)

//! file-based implementation of Store interface
class CRYPTOPP_DLL FileStore : public Store, private FilterPutSpaceHelper, public NotCopyable
{
public:
	class Err : public Exception
	{
	public:
		Err(const std::string &s) : Exception(IO_ERROR, s) {}
	};
	class OpenErr : public Err {public: OpenErr(const std::string &filename) : Err("FileStore: error opening file for reading: " + filename) {}};
	class ReadErr : public Err {public: ReadErr() : Err("FileStore: error reading file") {}};

	FileStore() : m_stream(NULL), m_space(NULL), m_len(0), m_waiting(0) {}
	FileStore(std::istream &in) : m_stream(NULL), m_space(NULL), m_len(0), m_waiting(0)
		{StoreInitialize(MakeParameters(Name::InputStreamPointer(), &in));}
	FileStore(const char *filename) : m_stream(NULL), m_space(NULL), m_len(0), m_waiting(0)
		{StoreInitialize(MakeParameters(Name::InputFileName(), filename ? filename : ""));}
#if defined(CRYPTOPP_UNIX_AVAILABLE) || _MSC_VER >= 1400
	//! specify file with Unicode name. On non-Windows OS, this function assumes that setlocale() has been called.
	FileStore(const wchar_t *filename)
		{StoreInitialize(MakeParameters(Name::InputFileNameWide(), filename));}
#endif

	std::istream* GetStream() {return m_stream;}

	lword MaxRetrievable() const;
	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const;
	lword Skip(lword skipMax=ULONG_MAX);

private:
	void StoreInitialize(const NameValuePairs &parameters);

	member_ptr<std::ifstream> m_file;
	std::istream *m_stream;
	byte *m_space;
	size_t m_len;
	bool m_waiting;
};

//! file-based implementation of Source interface
class CRYPTOPP_DLL FileSource : public SourceTemplate<FileStore>
{
public:
	typedef FileStore::Err Err;
	typedef FileStore::OpenErr OpenErr;
	typedef FileStore::ReadErr ReadErr;

	FileSource(BufferedTransformation *attachment = NULL)
		: SourceTemplate<FileStore>(attachment) {}
	FileSource(std::istream &in, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<FileStore>(attachment) {SourceInitialize(pumpAll, MakeParameters(Name::InputStreamPointer(), &in));}
	FileSource(const char *filename, bool pumpAll, BufferedTransformation *attachment = NULL, bool binary=true)
		: SourceTemplate<FileStore>(attachment) {SourceInitialize(pumpAll, MakeParameters(Name::InputFileName(), filename)(Name::InputBinaryMode(), binary));}
#if defined(CRYPTOPP_UNIX_AVAILABLE) || _MSC_VER >= 1400
	//! specify file with Unicode name. On non-Windows OS, this function assumes that setlocale() has been called.
	FileSource(const wchar_t *filename, bool pumpAll, BufferedTransformation *attachment = NULL, bool binary=true)
		: SourceTemplate<FileStore>(attachment) {SourceInitialize(pumpAll, MakeParameters(Name::InputFileNameWide(), filename)(Name::InputBinaryMode(), binary));}
#endif

	std::istream* GetStream() {return m_store.GetStream();}
};

//! file-based implementation of Sink interface
class CRYPTOPP_DLL FileSink : public Sink, public NotCopyable
{
public:
	class Err : public Exception
	{
	public:
		Err(const std::string &s) : Exception(IO_ERROR, s) {}
	};
	class OpenErr : public Err {public: OpenErr(const std::string &filename) : Err("FileSink: error opening file for writing: " + filename) {}};
	class WriteErr : public Err {public: WriteErr() : Err("FileSink: error writing file") {}};

	FileSink() : m_stream(NULL) {}
	FileSink(std::ostream &out)
		{IsolatedInitialize(MakeParameters(Name::OutputStreamPointer(), &out));}
	FileSink(const char *filename, bool binary=true)
		{IsolatedInitialize(MakeParameters(Name::OutputFileName(), filename)(Name::OutputBinaryMode(), binary));}
#if defined(CRYPTOPP_UNIX_AVAILABLE) || _MSC_VER >= 1400
	//! specify file with Unicode name. On non-Windows OS, this function assumes that setlocale() has been called.
	FileSink(const wchar_t *filename, bool binary=true)
		{IsolatedInitialize(MakeParameters(Name::OutputFileNameWide(), filename)(Name::OutputBinaryMode(), binary));}
#endif

	std::ostream* GetStream() {return m_stream;}

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking);
	bool IsolatedFlush(bool hardFlush, bool blocking);

private:
	member_ptr<std::ofstream> m_file;
	std::ostream *m_stream;
};

NAMESPACE_END

#endif
