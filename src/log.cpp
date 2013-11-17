// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "log.h"

#include "bitcointime.h"
#include "util.h"

#include <iostream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <boost/filesystem/path.hpp>

using namespace std;

bool Log::fDebug = false;
bool Log::fPrintToConsole = false;
bool Log::fPrintToDebugger = false;
bool Log::fLogTimestamps = false;
volatile bool Log::fReopenDebugLog = false;
bool Log::fStartedNewLine = true;

boost::once_flag Log::initStaticLogMembersFlag = BOOST_ONCE_INIT;
boost::once_flag Log::initFileoutFlag = BOOST_ONCE_INIT;

StaticLogMembers* Log::slm = NULL;


Log::Log(const string& category)
{
    boost::call_once(&Log::StaticLogMembersInit, initStaticLogMembersFlag);
    boost::mutex::scoped_lock scoped_lock(slm->mutexDebugLog);

    if (category != "")
    {
        if (!fDebug)
            return;

        const vector<string>& categories = mapMultiArgs["-debug"];
        bool allCategories = count(categories.begin(), categories.end(), string(""));

        // Only look for categories, if not -debug/-debug=1 was passed,
        // as that implies every category should be logged.
        if (!allCategories)
        {
            // Category was not found (not supplied via -debug=<category>)
            if (find(categories.begin(), categories.end(), category) == categories.end())
                return;
        }
    }

    if (fPrintToConsole)
    {
        // print to console
        vLogStreams.push_back(&cout);
    }
    else if (!fPrintToDebugger)
    {
        boost::call_once(&Log::FileoutInit, initFileoutFlag);

        if (!slm->fileout.is_open())
            return;

        vLogStreams.push_back(&slm->fileout);

        // reopen the log file, if requested
        if (fReopenDebugLog) {
            fReopenDebugLog = false;
            boost::filesystem::path pathDebug = GetDataDir() / "debug.log";
            slm->fileout.close();
            slm->fileout.open(pathDebug.string().c_str(), ofstream::out | ofstream::app);
            if (slm->fileout.is_open())
                slm->fileout.rdbuf()->pubsetbuf(NULL, 0); // unbuffered
        }

    }

#ifdef WIN32
    if (fPrintToDebugger)
        vLogStreams.push_back(&slm->windowsDebugLog);
#endif
}

Log& Log::insert(const string& message)
{
    boost::mutex::scoped_lock scoped_lock(slm->mutexDebugLog);

    for (vector<ostream*>::iterator it = vLogStreams.begin(); it != vLogStreams.end(); ++it)
    {
        if (*it == &slm->fileout)
        {
            // Debug print useful for profiling
            if (fLogTimestamps && fStartedNewLine)
                **it << BitcoinTime::DateTimeStrFormat("%Y-%m-%d %H:%M:%S", BitcoinTime::GetTime()) << " ";
            
            if (message[message.length() - 1] == '\n')
                fStartedNewLine = true;
            else
                fStartedNewLine = false;

            **it << message;
#ifdef WIN32            
        } else if (*it == &slm->windowsDebugLog) {
                // Accumulate in windowsDebugLog and output a line at a time
                int line_start = 0, line_end;
                while((line_end = message.find('\n', line_start)) != -1)
                {
                    string strLine = message.substr(line_start, line_end - line_start);

                    // Include buffered data if this is the first line, clear buffer after prepending
                    if (line_start == 0)
                    {
                        strLine = slm->windowsDebugLog.str() + strLine;
                        slm->windowsDebugLog.str("");
                    }
                    
                    OutputDebugStringA(strLine.c_str());
                    line_start = line_end + 1;
                }

                // Add un-printed message contents
                **it << message.substr(line_start);
#endif
        } else {
            **it << message;
        }

        **it << flush;

    }

    return *this;
}

void Log::StaticLogMembersInit()
{
    assert(!slm);

    slm = new StaticLogMembers();
}

void Log::FileoutInit()
{
    assert(!slm->fileout.is_open());

    boost::filesystem::path pathDebug = GetDataDir() / "debug.log";
    slm->fileout.open(pathDebug.string().c_str(), ofstream::out | ofstream::app);
    if (slm->fileout.is_open()) slm->fileout.rdbuf()->pubsetbuf(NULL, 0); // unbuffered

}

