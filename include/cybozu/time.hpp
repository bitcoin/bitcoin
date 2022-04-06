#pragma once
/**
	@file
	@brief tiny time class

	@author MITSUNARI Shigeo(@herumi)
*/
#include <ctime>
#include <cybozu/exception.hpp>
#include <cybozu/atoi.hpp>
#include <cybozu/itoa.hpp>
#ifdef _WIN32
	#include <sys/timeb.h>
#else
	#include <sys/time.h>
#endif

namespace cybozu {

/**
	time struct with time_t and msec
	@note time MUST be latesr than 1970/1/1
*/
class Time {
	static const uint64_t epochBias = 116444736000000000ull;
	std::time_t time_;
	int msec_;
public:
	explicit Time(std::time_t time = 0, int msec = 0)
		: time_(time)
		, msec_(msec)
	{
	}
	explicit Time(bool doSet)
	{
		if (doSet) setCurrentTime();
	}
	Time& setTime(std::time_t time, int msec = 0)
	{
		time_ = time;
		msec_ = msec;
		return *this;
	}
	/*
		Windows FILETIME is defined as
		struct FILETILME {
			DWORD dwLowDateTime;
			DWORD dwHighDateTime;
		};
		the value represents the number of 100-nanosecond intervals since January 1, 1601 (UTC).
	*/
	void setByFILETIME(uint32_t low, uint32_t high)
	{
		const uint64_t fileTime = (((uint64_t(high) << 32) | low) - epochBias) / 10000;
		time_ = fileTime / 1000;
		msec_ = fileTime % 1000;
	}
	/*
		DWORD is defined as unsigned long in windows
	*/
	template<class dword>
	void getFILETIME(dword& low, dword& high) const
	{
		const uint64_t fileTime = (time_ * 1000 + msec_) * 10000 + epochBias;
		low = dword(fileTime);
		high = dword(fileTime >> 32);
	}
	explicit Time(const std::string& in)
	{
		fromString(in);
	}
	explicit Time(const char *in)
	{
		fromString(in, in + strlen(in));
	}
	const std::time_t& getTime() const { return time_; }
	int getMsec() const { return msec_; }
	double getTimeSec() const { return time_ + msec_ * 1e-3; }
	void addSec(int sec) { time_ += sec; }
	bool operator<(const Time& rhs) const { return (time_ < rhs.time_) || (time_ == rhs.time_ && msec_ < rhs.msec_); }
//	bool operator<=(const Time& rhs) const { return (*this < rhs) || (*this == rhs); }
//	bool operator>(const Time& rhs) const { return rhs < *this; }
	bool operator==(const Time& rhs) const { return (time_ == rhs.time_) && (msec_ == rhs.msec_); }
	bool operator!=(const Time& rhs) const { return !(*this == rhs); }
	/**
		set time from string such as
		2009-Jan-23T02:53:44Z
		2009-Jan-23T02:53:44.078Z
		2009-01-23T02:53:44Z
		2009-01-23T02:53:44.078Z
		@note 'T' may be ' '. '-' may be '/'. last char 'Z' is omissible
	*/
	void fromString(bool *pb, const std::string& in) { fromString(pb, &in[0], &in[0] + in.size()); }
	void fromString(const std::string& in) { fromString(0, in); }

	void fromString(bool *pb, const char *begin, const char *end)
	{
		const size_t len = end - begin;
		if (len >= 19) {
			const char *p = begin;
			struct tm tm;
			int num;
			bool b;
			tm.tm_year = getNum(&b, p, 4, 1970, 3000) - 1900;
			if (!b) goto ERR;
			p += 4;
			char sep = *p++;
			if (sep != '-' && sep != '/') goto ERR;

			p = getMonth(&num, p);
			if (p == 0) goto ERR;
			tm.tm_mon = num;
			if (*p++ != sep) goto ERR;

			tm.tm_mday = getNum(&b, p, 2, 1, 31);
			if (!b) goto ERR;
			p += 2;
			if (*p != ' ' && *p != 'T') goto ERR;
			p++;

			tm.tm_hour = getNum(&b, p, 2, 0, 23);
			if (!b) goto ERR;
			p += 2;
			if (*p++ != ':') goto ERR;

			tm.tm_min = getNum(&b, p, 2, 0, 59);
			if (!b) goto ERR;
			p += 2;
			if (*p++ != ':') goto ERR;

			tm.tm_sec = getNum(&b, p, 2, 0, 59);
			if (!b) goto ERR;
			p += 2;

			if (p == end) {
				msec_ = 0;
			} else if (p + 1 == end && *p == 'Z') {
				msec_ = 0;
				p++;
			} else if (*p == '.' && (p + 4 == end || (p + 5 == end && *(p + 4) == 'Z'))) {
				msec_ = getNum(&b, p + 1, 3, 0, 999);
				if (!b) goto ERR;
//				p += 4;
			} else {
				goto ERR;
			}
#ifdef _WIN32
			time_ = _mkgmtime64(&tm);
			if (time_ == -1) goto ERR;
#else
			time_ = timegm(&tm);
#endif
			if (pb) {
				*pb = true;
			}
			return;
		}
	ERR:
		if (pb) {
			*pb = false;
			return;
		}
		throw cybozu::Exception("time::fromString") << std::string(begin, 24);
	}
	void fromString(const char *begin, const char *end) { fromString(0, begin, end); }

	/**
		get current time with format
		@param out [out] output string
		@param format [in] foramt for strftime and append three digits for msec
		@param appendMsec [in] appemd <mmm>
		@param doClear (append to out if false)
		@note ex. "%Y-%b-%d %H:%M:%S." to get 2009-Jan-23 02:53:44.078
	*/
	void toString(std::string& out, const char *format, bool appendMsec = true, bool doClear = true) const
	{
		if (doClear) out.clear();
		char buf[128];
		struct tm tm;
#ifdef _WIN32
		bool isOK = _gmtime64_s(&tm, &time_) == 0;
#else
		bool isOK = gmtime_r(&time_, &tm) != 0;
#endif
		if (!isOK) throw cybozu::Exception("time::toString") << time_;
#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
		if (std::strftime(buf, sizeof(buf), format, &tm) == 0) {
			throw cybozu::Exception("time::toString::too long") << format << time_;
		}
#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif
		out += buf;
		if (appendMsec) {
			out += cybozu::itoaWithZero(msec_, 3);
		}
	}

	/**
		get current time such as 2009-01-23 02:53:44.078
		@param out [out] sink string
	*/
	void toString(std::string& out, bool appendMsec = true, bool doClear = true) const
	{
		const char *format = appendMsec ? "%Y-%m-%d %H:%M:%S." : "%Y-%m-%d %H:%M:%S";
		toString(out, format, appendMsec, doClear);
	}
	std::string toString(bool appendMsec = true) const { std::string out; toString(out, appendMsec); return out; }
	/**
		get current time
	*/
	Time& setCurrentTime()
	{
#ifdef _WIN32
		struct _timeb timeb;
		_ftime_s(&timeb);
		time_ = timeb.time;
		msec_ = timeb.millitm;
#else
		struct timeval tv;
		gettimeofday(&tv, 0);
		time_ = tv.tv_sec;
		msec_ = tv.tv_usec / 1000;
#endif
		return *this;
	}
private:

	int getNum(bool *b, const char *in, size_t len, int min, int max) const
	{
		int ret = cybozu::atoi(b, in, len);
		if (min <= ret && ret <= max) {
			return ret;
		} else {
			*b = false;
			return 0;
		}
	}

	/*
		convert month-str to [0, 11]
		@param ret [out] return idx
		@param p [in] month-str
		@retval next pointer or null
	*/
	const char *getMonth(int *ret, const char *p) const
	{
		static const char monthTbl[12][4] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};

		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(monthTbl); i++) {
			if (memcmp(p, monthTbl[i], 3) == 0) {
				*ret = (int)i;
				return p + 3;
			}
		}
		bool b;
		*ret = getNum(&b, p, 2, 1, 12) - 1;
		if (b) {
			return p + 2;
		} else {
			return 0;
		}
	}
};

inline std::ostream& operator<<(std::ostream& os, const cybozu::Time& time)
{
	return os << time.toString();
}

inline double GetCurrentTimeSec()
{
	return cybozu::Time(true).getTimeSec();
}

} // cybozu
