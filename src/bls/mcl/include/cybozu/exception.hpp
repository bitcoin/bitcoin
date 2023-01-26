#pragma once
/**
	@file
	@brief definition of abstruct exception class
	@author MITSUNARI Shigeo(@herumi)
*/
#ifdef CYBOZU_MINIMUM_EXCEPTION

#include <cybozu/inttype.hpp>

namespace cybozu {

namespace exception {
inline const char *makeString(const char *, size_t)
{
	return "";
}

} // cybozu::exception

class Exception {
public:
	explicit Exception(const char* = 0, bool = true)
	{
	}
	~Exception() CYBOZU_NOEXCEPT {}
	const char *what() const CYBOZU_NOEXCEPT { return "cybozu:Exception"; }
	template<class T>
	Exception& operator<<(const T&)
	{
		return *this;
	}
};

} // cybozu

#else

#include <string>
#include <algorithm>
#include <sstream>
#include <errno.h>
#include <stdio.h>
#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <string.h> // for strerror_r
#endif
#include <cybozu/inttype.hpp>
#ifdef CYBOZU_EXCEPTION_WITH_STACKTRACE
	#include <cybozu/stacktrace.hpp>
#endif

namespace cybozu {

const bool DontThrow = true;

namespace exception {

/* get max 16 characters to avoid buffer overrun */
inline std::string makeString(const char *str, size_t size)
{
	return std::string(str, std::min<size_t>(size, 16));
}

#ifdef _WIN32
inline std::string wstr2str(const std::wstring& wstr)
{
	std::string str;
	for (size_t i = 0; i < wstr.size(); i++) {
		uint16_t c = wstr[i];
		if (c < 0x80) {
			str += char(c);
		} else {
			char buf[16];
			CYBOZU_SNPRINTF(buf, sizeof(buf), "\\u%04x", c);
			str += buf;
		}
	}
	return str;
}
#endif

} // cybozu::exception

/**
	convert errno to string
	@param err [in] errno
	@note for both windows and linux
*/
inline std::string ConvertErrorNoToString(int err)
{
	char errBuf[256];
	std::string ret;
#ifdef _WIN32
	if (strerror_s(errBuf, sizeof(errBuf), err) == 0) {
		ret = errBuf;
	} else {
		ret = "err";
	}
#elif defined(_GNU_SOURCE)
	ret = ::strerror_r(err, errBuf, sizeof(errBuf));
#else
	if (strerror_r(err, errBuf, sizeof(errBuf)) == 0) {
		ret = errBuf;
	} else {
		ret = "err";
	}
#endif
	char buf2[64];
	CYBOZU_SNPRINTF(buf2, sizeof(buf2), "(%d)", err);
	ret += buf2;
	return ret;
}

class Exception : public std::exception {
	mutable std::string str_;
#ifdef CYBOZU_EXCEPTION_WITH_STACKTRACE
	mutable std::string stackTrace_;
#endif
public:
	explicit Exception(const std::string& name = "", bool enableStackTrace = true)
		: str_(name)
	{
#ifdef CYBOZU_EXCEPTION_WITH_STACKTRACE
		if (enableStackTrace) stackTrace_ = cybozu::StackTrace().toString();
#else
		cybozu::disable_warning_unused_variable(enableStackTrace);
#endif
	}
	~Exception() CYBOZU_NOEXCEPT {}
	const char *what() const CYBOZU_NOEXCEPT { return toString().c_str(); }
	const std::string& toString() const CYBOZU_NOEXCEPT
	{
#ifdef CYBOZU_EXCEPTION_WITH_STACKTRACE
		try {
			if (!stackTrace_.empty()) {
#ifdef CYBOZU_STACKTRACE_ONELINE
				str_ += "\n<<<STACKTRACE>>> ";
				str_ += stackTrace_;
#else
				str_ += "\n<<<STACKTRACE\n";
				str_ += stackTrace_;
				str_ += "\n>>>STACKTRACE";
#endif
			}
		} catch (...) {
		}
		stackTrace_.clear();
#endif
		return str_;
	}
	Exception& operator<<(const char *s)
	{
		str_ += ':';
		str_ += s;
		return *this;
	}
	Exception& operator<<(const std::string& s)
	{
		return operator<<(s.c_str());
	}
#ifdef _WIN32
	Exception& operator<<(const std::wstring& s)
	{
		return operator<<(cybozu::exception::wstr2str(s));
	}
#endif
	template<class T>
	Exception& operator<<(const T& x)
	{
		std::ostringstream os;
		os << x;
		return operator<<(os.str());
	}
};

class ErrorNo {
public:
#ifdef _WIN32
	typedef unsigned int NativeErrorNo;
#else
	typedef int NativeErrorNo;
#endif
	explicit ErrorNo(NativeErrorNo err)
		: err_(err)
	{
	}
	ErrorNo()
		: err_(getLatestNativeErrorNo())
	{
	}
	NativeErrorNo getLatestNativeErrorNo() const
	{
#ifdef _WIN32
		return ::GetLastError();
#else
		return errno;
#endif
	}
	/**
		convert NativeErrNo to string(maybe UTF8)
		@note Linux   : same as ConvertErrorNoToString
			  Windows : for Win32 API(use en-us)
	*/
	std::string toString() const
	{
#ifdef _WIN32
		const int msgSize = 256;
		wchar_t msg[msgSize];
		int size = FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0,
			err_,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			msg,
			msgSize,
			NULL
		);
		if (size <= 0) return "";
		// remove last "\r\n"
		if (size > 2 && msg[size - 2] == '\r') {
			msg[size - 2] = 0;
			size -= 2;
		}
		std::string ret;
		ret.resize(size);
		// assume ascii only
		for (int i = 0; i < size; i++) {
			ret[i] = (char)msg[i];
		}
		char buf2[64];
		CYBOZU_SNPRINTF(buf2, sizeof(buf2), "(%u)", err_);
		ret += buf2;
		return ret;
#else
		return ConvertErrorNoToString(err_);
#endif
	}
private:
	NativeErrorNo err_;
};

inline std::ostream& operator<<(std::ostream& os, const cybozu::ErrorNo& self)
{
	return os << self.toString();
}

} // cybozu
#endif
