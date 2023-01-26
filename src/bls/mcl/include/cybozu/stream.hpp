#pragma once
/**
	@file
	@brief stream and line stream class

	@author MITSUNARI Shigeo(@herumi)
*/
#ifndef CYBOZU_DONT_USE_STRING
#include <string>
#include <iosfwd>
#endif
#ifndef CYBOZU_DONT_USE_EXCEPTION
#include <cybozu/exception.hpp>
#endif
#include <memory.h>

namespace cybozu {

namespace stream_local {

template <typename From, typename To>
struct is_convertible {
	typedef char yes;
	typedef int no;

	static no test(...);
	static yes test(const To*);
	static const bool value = sizeof(test(static_cast<const From*>(0))) == sizeof(yes);
};

template <bool b, class T = void>
struct enable_if { typedef T type; };

template <class T>
struct enable_if<false, T> {};

#ifndef CYBOZU_DONT_USE_STRING
/* specialization for istream */
template<class InputStream>
size_t readSome_inner(void *buf, size_t size, InputStream& is, typename enable_if<is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	if (size > 0x7fffffff) size = 0x7fffffff;
	is.read(static_cast<char *>(buf), size);
	const int64_t readSize = is.gcount();
	if (readSize < 0) return 0;
	if (size == 1 && readSize == 0) is.clear();
	return static_cast<size_t>(readSize);
}

/* generic version for size_t readSome(void *, size_t) */
template<class InputStream>
size_t readSome_inner(void *buf, size_t size, InputStream& is, typename enable_if<!is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	return is.readSome(buf, size);
}
#else
template<class InputStream>
size_t readSome_inner(void *buf, size_t size, InputStream& is)
{
	return is.readSome(buf, size);
}
#endif

#ifndef CYBOZU_DONT_USE_EXCEPTION
/* specialization for ostream */
template<class OutputStream>
void writeSub(OutputStream& os, const void *buf, size_t size, typename enable_if<is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	if (!os.write(static_cast<const char *>(buf), size)) throw cybozu::Exception("stream:writeSub") << size;
}
#endif

#ifndef CYBOZU_DONT_USE_STRING
/* generic version for void write(const void*, size_t), which writes all data */
template<class OutputStream>
void writeSub(OutputStream& os, const void *buf, size_t size, typename enable_if<!is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	os.write(buf, size);
}

template<class OutputStream>
void writeSub(bool *pb, OutputStream& os, const void *buf, size_t size, typename enable_if<is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	*pb = !!os.write(static_cast<const char *>(buf), size);
}

/* generic version for void write(const void*, size_t), which writes all data */
template<class OutputStream>
void writeSub(bool *pb, OutputStream& os, const void *buf, size_t size, typename enable_if<!is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	os.write(pb, buf, size);
}
#else
template<class OutputStream>
void writeSub(bool *pb, OutputStream& os, const void *buf, size_t size)
{
	os.write(pb, buf, size);
}
#endif

} // stream_local

/*
	make a specializaiton of class to use new InputStream, OutputStream
*/
template<class InputStream>
struct InputStreamTag {
	static size_t readSome(void *buf, size_t size, InputStream& is)
	{
		return stream_local::readSome_inner<InputStream>(buf, size, is);
	}
	static bool readChar(char *c, InputStream& is)
	{
		return readSome(c, 1, is) == 1;
	}
};

template<class OutputStream>
struct OutputStreamTag {
	static void write(OutputStream& os, const void *buf, size_t size)
	{
		stream_local::writeSub<OutputStream>(os, buf, size);
	}
};

class MemoryInputStream {
	const char *p_;
	size_t size_;
	size_t pos;
public:
	MemoryInputStream(const void *p, size_t size) : p_(static_cast<const char *>(p)), size_(size), pos(0) {}
	size_t readSome(void *buf, size_t size)
	{
		if (size > size_  - pos) size = size_ - pos;
		memcpy(buf, p_ + pos, size);
		pos += size;
		return size;
	}
	size_t getPos() const { return pos; }
};

class MemoryOutputStream {
	char *p_;
	size_t size_;
	size_t pos;
public:
	MemoryOutputStream(void *p, size_t size) : p_(static_cast<char *>(p)), size_(size), pos(0) {}
	void write(bool *pb, const void *buf, size_t size)
	{
		if (size > size_ - pos) {
			*pb = false;
			return;
		}
		memcpy(p_ + pos, buf, size);
		pos += size;
		*pb = true;
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void write(const void *buf, size_t size)
	{
		bool b;
		write(&b, buf, size);
		if (!b) throw cybozu::Exception("MemoryOutputStream:write") << size << size_ << pos;
	}
#endif
	size_t getPos() const { return pos; }
};

#ifndef CYBOZU_DONT_USE_STRING
class StringInputStream {
	const std::string& str_;
	size_t pos;
	StringInputStream(const StringInputStream&);
	void operator=(const StringInputStream&);
public:
	explicit StringInputStream(const std::string& str) : str_(str), pos(0) {}
	size_t readSome(void *buf, size_t size)
	{
		const size_t remainSize = str_.size() - pos;
		if (size > remainSize) size = remainSize;
		memcpy(buf, &str_[pos], size);
		pos += size;
		return size;
	}
	size_t getPos() const { return pos; }
};

class StringOutputStream {
	std::string& str_;
	StringOutputStream(const StringOutputStream&);
	void operator=(const StringOutputStream&);
public:
	explicit StringOutputStream(std::string& str) : str_(str) {}
	void write(bool *pb, const void *buf, size_t size)
	{
		str_.append(static_cast<const char *>(buf), size);
		*pb = true;
	}
	void write(const void *buf, size_t size)
	{
		str_.append(static_cast<const char *>(buf), size);
	}
	size_t getPos() const { return str_.size(); }
};
#endif

template<class InputStream>
size_t readSome(void *buf, size_t size, InputStream& is)
{
	return stream_local::readSome_inner(buf, size, is);
}

template<class OutputStream>
void write(OutputStream& os, const void *buf, size_t size)
{
	stream_local::writeSub(os, buf, size);
}

template<class OutputStream>
void write(bool *pb, OutputStream& os, const void *buf, size_t size)
{
	stream_local::writeSub(pb, os, buf, size);
}

template<typename InputStream>
void read(bool *pb, void *buf, size_t size, InputStream& is)
{
	char *p = static_cast<char*>(buf);
	while (size > 0) {
		size_t readSize = cybozu::readSome(p, size, is);
		if (readSize == 0) {
			*pb = false;
			return;
		}
		p += readSize;
		size -= readSize;
	}
	*pb = true;
}

#ifndef CYBOZU_DONT_USE_EXCEPTION
template<typename InputStream>
void read(void *buf, size_t size, InputStream& is)
{
	bool b;
	read(&b, buf, size, is);
	if (!b) throw cybozu::Exception("stream:read");
}
#endif

template<class InputStream>
bool readChar(char *c, InputStream& is)
{
	return readSome(c, 1, is) == 1;
}

template<class OutputStream>
void writeChar(OutputStream& os, char c)
{
	cybozu::write(os, &c, 1);
}

template<class OutputStream>
void writeChar(bool *pb, OutputStream& os, char c)
{
	cybozu::write(pb, os, &c, 1);
}

} // cybozu
