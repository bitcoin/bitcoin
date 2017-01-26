// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define __STDC_WANT_LIB_EXT1__ 1 // for gmtime_s

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "utiltime.h"

#include <chrono>
#include <thread>
#include <locale>
#include <sstream>
#include <string.h>
#include <time.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

using namespace std;

static int64_t nMockTime = 0; //!< For unit testing

static inline const tm gmtime_int(time_t time)
{
    tm out = {};
#if defined(HAVE_W32_GMTIME_S)
    gmtime_s(&out, &time);
#elif defined(HAVE_C11_GMTIME_S)
    gmtime_s(&time, &out);
#elif defined(HAVE_GMTIME_R)
    gmtime_r(&time, &out);
#else
    // Not thread-safe!
    out = *gmtime(&time);
#endif
    return out;
}

bool ChronoSanityCheck()
{
    // std::chrono::system_clock.time_since_epoch and time_t(0) are not guaranteed
    // to use the Unix epoch timestamp, but in practice they almost certainly will.
    // Any differing behavior will be assumed to be an error, unless certain
    // platforms prove to consistently deviate, at which point we'll cope with it
    // by adding offsets.

    // Create a new clock from time_t(0) and make sure that it represents 0
    // seconds from the system_clock's time_since_epoch. Then convert that back
    // to a time_t and verify that it's the same as before.
    const time_t zeroTime{};
    auto clock = std::chrono::system_clock::from_time_t(zeroTime);
    if (std::chrono::duration_cast<std::chrono::seconds>(clock.time_since_epoch()).count() != 0)
        return false;

    time_t nTime = std::chrono::system_clock::to_time_t(clock);
    if (nTime != zeroTime)
        return false;

    // Check that the above zero time is actually equal to the known unix timestamp.
    tm epoch = gmtime_int(nTime);
    if ((epoch.tm_sec != 0)  || \
       (epoch.tm_min  != 0)  || \
       (epoch.tm_hour != 0)  || \
       (epoch.tm_mday != 1)  || \
       (epoch.tm_mon  != 0)  || \
       (epoch.tm_year != 70))
        return false; 
    return true;
}

template <typename T>
static inline int64_t GetCurrentTime()
{
    return std::chrono::duration_cast<T>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t GetTime()
{
    if (nMockTime) return nMockTime;
    return GetCurrentTime<std::chrono::seconds>();
}

void SetMockTime(int64_t nMockTimeIn)
{
    nMockTime = nMockTimeIn;
}

int64_t GetTimeMillis()
{
    return GetCurrentTime<std::chrono::milliseconds>();
}

int64_t GetTimeMicros()
{
    return GetCurrentTime<std::chrono::microseconds>();
}

/** Return a time useful for the debug log */
int64_t GetLogTimeMicros()
{
    if (nMockTime) return nMockTime*1000000;

    return GetTimeMicros();
}

void MilliSleep(int64_t n)
{

/**
 * Boost's sleep_for was uninterruptable when backed by nanosleep from 1.50
 * until fixed in 1.52. Use the deprecated sleep method for the broken case.
 * See: https://svn.boost.org/trac/boost/ticket/7238
 */
#if defined(HAVE_WORKING_BOOST_SLEEP_FOR)
    boost::this_thread::sleep_for(boost::chrono::milliseconds(n));
#elif defined(HAVE_WORKING_BOOST_SLEEP)
    boost::this_thread::sleep(boost::posix_time::milliseconds(n));
#else
//should never get here
#error missing boost sleep implementation
#endif
}

std::string DateTimeStrFormat(const char* pszFormat, int64_t nSecs)
{
    static std::locale classic(std::locale::classic());
    time_t nTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point{std::chrono::seconds{nSecs}});
    const tm& now = gmtime_int(nTime);
    std::stringstream ss;
    ss.imbue(classic);
    std::use_facet<std::time_put<char>>(ss.getloc()).put(ss.rdbuf(), ss, ' ', &now, pszFormat, pszFormat + strlen(pszFormat));
    return ss.str();
}
