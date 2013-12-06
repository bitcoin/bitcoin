// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOG_H
#define BITCOIN_LOG_H

#include "bitcointime.h"

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/thread/once.hpp>
#include <boost/thread/mutex.hpp>

class StaticLogMembers;


class Log
{
    public:
        Log(const std::string& category = "");

        // Must log strings specially since they can contain new lines
        Log& operator<<(const char* message) { return operator<<(std::string(message)); }

        // Value types
        Log& operator<<(bool   n) { return insert(n); }
        Log& operator<<(float  n) { return insert(n); }
        Log& operator<<(double n) { return insert(n); }
        Log& operator<<(void*  n) { return insert(n); }

        Log& operator<<(char      n) { return insert(n); }
        Log& operator<<(short     n) { return insert(n); }
        Log& operator<<(int       n) { return insert(n); }
        Log& operator<<(long      n) { return insert(n); }
        Log& operator<<(long long n) { return insert(n); }

        Log& operator<<(unsigned char      n) { return insert(n); }
        Log& operator<<(unsigned short     n) { return insert(n); }
        Log& operator<<(unsigned int       n) { return insert(n); }
        Log& operator<<(unsigned long      n) { return insert(n); }
        Log& operator<<(unsigned long long n) { return insert(n); }

        // catch-all
        template <typename T> Log& operator<<(const T& message) { return insert(message); }

        static bool fDebug;
        static bool fPrintToConsole;
        static bool fPrintToDebugger;
        static bool fLogTimestamps;
        static volatile bool fReopenDebugLog;
        static bool fStartedNewLine;

    private:
        
        // String implementation
        Log& insert(const std::string& message);

        // Non-string implementation
        template <typename T> Log& insert(const T& message);

        static void StaticLogMembersInit();
        static void FileoutInit();

        std::vector<std::ostream*> vLogStreams;

        // We use boost::call_once() to make sure these are initialized in a
        //   thread-safe manner, the first time their respective members are used:
        static boost::once_flag initStaticLogMembersFlag;
        static boost::once_flag initFileoutFlag;

        // Static variables that require class construction MUST be contained
        //   within the StaticLogMembers singleton class.
        //
        // Since the destruction order of static variables is undefined and
        //   some static classes call Log functions upon their own destruction, 
        //   we must define our static class members in a way that they are
        //   never destructed.
        static StaticLogMembers* slm;
};


// Singleton class to be constructed just in time for the Log class, 
//   and only for the Log class
class StaticLogMembers
{
    friend class Log;

    private:
        StaticLogMembers() { }

        std::ofstream fileout;
        
        boost::mutex mutexDebugLog;

#ifdef WIN32
        std::ostringstream windowsDebugLog;
#endif
};


// Just insert data into stream for non-strings
template <typename T> 
Log& Log::insert(const T& message)
{
    boost::mutex::scoped_lock scoped_lock(slm->mutexDebugLog);

    for (std::vector<std::ostream*>::iterator it = vLogStreams.begin(); it != vLogStreams.end(); ++it)
    {
        if (*it == &slm->fileout)
        {
            // Debug print useful for profiling
            if (fLogTimestamps && fStartedNewLine)
                **it << BitcoinTime::DateTimeStrFormat("%Y-%m-%d %H:%M:%S", BitcoinTime::GetTime()) << " ";
            
            fStartedNewLine = false;
        }
        **it << message << std::flush;
    }

    return *this;
}

#endif
