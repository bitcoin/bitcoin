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
    boost::interprocess::message_queue::remove("BitcoinURL");
}

void ipcThread(void* parg)
{
    boost::interprocess::message_queue* mq = (boost::interprocess::message_queue*)parg;
    char strBuf[257];
    size_t nSize;
    unsigned int nPriority;
    loop
    {
        boost::posix_time::ptime d = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::millisec(100);
        if(mq->timed_receive(&strBuf, sizeof(strBuf), nSize, nPriority, d))
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

void ipcInit()
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

    boost::interprocess::message_queue* mq;
    char strBuf[257];
    size_t nSize;
    unsigned int nPriority;
    try {
        mq = new boost::interprocess::message_queue(boost::interprocess::create_only, "BitcoinURL", 2, 256);

        // Make sure we don't lose any bitcoin: URIs
        for (int i = 0; i < 2; i++)
        {
            boost::posix_time::ptime d = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::millisec(1);
            if(mq->timed_receive(&strBuf, sizeof(strBuf), nSize, nPriority, d))
            {
                ThreadSafeHandleURL(std::string(strBuf, nSize));
            }
            else
                break;
        }

        // Make sure only one bitcoin instance is listening
        boost::interprocess::message_queue::remove("BitcoinURL");
        mq = new boost::interprocess::message_queue(boost::interprocess::create_only, "BitcoinURL", 2, 256);
    }
    catch (interprocess_exception &ex) {
        printf("ipcInit - boost::interprocess exeption: %s\n", ex.what());
        return;
    }
    if (!CreateThread(ipcThread, mq))
    {
        delete mq;
    }
}
