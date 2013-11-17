// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcointime.h"

#include "log.h"
#include "sync.h"

#ifndef WIN32
#include <sys/time.h>
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>

using namespace std;

namespace BitcoinTime
{
    int64_t GetTimeMillis()
    {
        return (boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) -
                boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_milliseconds();
    }

    int64_t GetTimeMicros()
    {
        return (boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) -
                boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_microseconds();
    }

    string DateTimeStrFormat(const char* pszFormat, int64_t nTime)
    {
        time_t n = nTime;
        struct tm* ptmTime = gmtime(&n);
        char pszTime[200];
        strftime(pszTime, sizeof(pszTime), pszFormat, ptmTime);
        return pszTime;
    }



    //
    // "Never go to sea with two chronometers; take one or three."
    // Our three time sources are:
    //  - System clock
    //  - Median of other nodes clocks
    //  - The user (asking the user to fix the system clock if the first two disagree)
    //
    static int64_t nMockTime = 0;  // For unit testing

    int64_t GetTime()
    {
        if (nMockTime) return nMockTime;

        return time(NULL);
    }

    void SetMockTime(int64_t nMockTimeIn)
    {
        nMockTime = nMockTimeIn;
    }

    int64_t GetAdjustedTime()
    {
        return GetTime() + GetTimeOffset();
    }

    static CCriticalSection cs_nTimeOffset;
    static int64_t nTimeOffset = 0;
    
    int64_t GetTimeOffset()
    {
        LOCK(cs_nTimeOffset);
        return nTimeOffset;
    }

    void SetTimeOffset(int64_t n)
    {
        LOCK(cs_nTimeOffset);
        nTimeOffset = n;
    }

}
