// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/tokenizer.hpp>

#include "headers.h"

using namespace boost;
using namespace std;

void ipcShutdown()
{
    interprocess::message_queue::remove("BitcoinURL");
}

bool ipcRecover(const char* pszFilename)
{
    std::string strIpcDir;
    // get path to possible stale ipc message queue file
#if BOOST_VERSION <= 104700
    interprocess::detail::tmp_filename(pszFilename, strIpcDir);
#elif BOOST_VERSION >= 104800
    interprocess::ipcdetail::tmp_filename(pszFilename, strIpcDir);
#endif

    filesystem::path pathMessageQueue(strIpcDir);
    pathMessageQueue.make_preferred();

    // verify that the message queue file exists and try to remove it
    if (exists(pathMessageQueue))
    {
        string strLogMessage = ("ipcRecover - possible stale message queue detected, trying to remove " + pathMessageQueue.string());
        system::error_code ecErrorCode;

        // try removal
        if (remove(pathMessageQueue, ecErrorCode))
        {
            printf("%s ...success\n", strLogMessage.c_str());
            return true;
        }
        // if the file is locked (which is NOT the case after a hard crash, but if another Bitcoin-Qt instance is running)
        else if (ecErrorCode.value() == system::errc::broken_pipe)
        {
            // make clear in debug.log, that we have no "bad" situation
            printf("%s ...unneeded\n", strLogMessage.c_str());
            // return false, to not re-try init again
            return false;
        }
        else
        {
            printf("%s ...failed\nipcRecover - removal of old message queue failed with error #%d: %s\n", strLogMessage.c_str(), ecErrorCode.value(), ecErrorCode.message().c_str());
            return false;
        }
    }
    else
        return false;
}

void ipcThread(void* pArg)
{
    interprocess::message_queue* mq = (interprocess::message_queue*)pArg;
    char strBuf[257];
    size_t nSize;
    unsigned int nPriority;
    loop
    {
        posix_time::ptime d = posix_time::microsec_clock::universal_time() + posix_time::millisec(100);
        if (mq->timed_receive(&strBuf, sizeof(strBuf), nSize, nPriority, d))
        {
            ThreadSafeHandleURL(std::string(strBuf, nSize));
            Sleep(1000);
        }
        if (fShutdown)
        {
            ipcShutdown();
            break;
        }
    }
    ipcShutdown();
}

void ipcInit(bool fInitCalledAfterRecovery)
{
#ifdef MAC_OSX
    // TODO: implement bitcoin: URI handling the Mac Way
    return;
#endif
#ifdef WIN32
    // TODO: THOROUGHLY test boost::interprocess fix,
    // and make sure there are no Windows argument-handling exploitable
    // problems.
    return;
#endif

    interprocess::message_queue* mq;
    char strBuf[257];
    size_t nSize;
    unsigned int nPriority;
    try {
        mq = new interprocess::message_queue(interprocess::create_only, "BitcoinURL", 2, 256);

        // Make sure we don't lose any bitcoin: URIs
        for (int i = 0; i < 2; i++)
        {
            posix_time::ptime d = posix_time::microsec_clock::universal_time() + posix_time::millisec(1);
            if (mq->timed_receive(&strBuf, sizeof(strBuf), nSize, nPriority, d))
            {
                ThreadSafeHandleURL(std::string(strBuf, nSize));
            }
            else
                break;
        }

        // Make sure only one bitcoin instance is listening
        interprocess::message_queue::remove("BitcoinURL");
        mq = new interprocess::message_queue(interprocess::create_only, "BitcoinURL", 2, 256);
    }
    catch (interprocess::interprocess_exception &ex) {
// we currently only handle the Windows-specific case from #956
#ifdef WIN32
        printf("ipcInit    - boost interprocess exception #%d: %s\n", ex.get_error_code(), ex.what());

        // check if the exception is a "file already exists" error
        if (ex.get_error_code() == interprocess::already_exists_error)
        {
            // try a recovery to fix #956 and pass our message queue name
            if (ipcRecover("BitcoinURL") && !fInitCalledAfterRecovery)
                // try init once more and pass true, to avoid an infinite recursion
                ipcInit(true);
        }
#endif
        return;
    }
    if (!CreateThread(ipcThread, mq))
    {
        delete mq;
    }
}
