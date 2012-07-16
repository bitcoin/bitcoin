// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/tokenizer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/version.hpp>

#if defined(WIN32) && (!defined(BOOST_INTERPROCESS_HAS_WINDOWS_KERNEL_BOOTTIME) || !defined(BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME) || BOOST_VERSION < 104900)
#warning Compiling without BOOST_INTERPROCESS_HAS_WINDOWS_KERNEL_BOOTTIME and BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME uncommented in boost/interprocess/detail/tmp_dir_helpers.hpp or using a boost version before 1.49 may have unintended results see svn.boost.org/trac/boost/ticket/5392
#endif

#include "ui_interface.h"
#include "qtipcserver.h"
#include "util.h"

using namespace boost::interprocess;
using namespace boost::posix_time;
using namespace boost;
using namespace std;

#ifdef MAC_OSX
// URI handling not implemented on OSX yet

void ipcInit() { }
void ipcShutdown() { }

#else

void ipcShutdown()
{
    message_queue::remove(BITCOINURI_QUEUE_NAME);
}

void ipcThread(void* parg)
{
    // Make this thread recognisable as the GUI-IPC thread
    RenameThread("bitcoin-gui-ipc");

    message_queue* mq = (message_queue*)parg;
    char strBuf[257];
    size_t nSize;
    unsigned int nPriority;
    loop
    {
        ptime d = boost::posix_time::microsec_clock::universal_time() + millisec(100);
        if(mq->timed_receive(&strBuf, sizeof(strBuf), nSize, nPriority, d))
        {
            uiInterface.ThreadSafeHandleURI(std::string(strBuf, nSize));
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
    message_queue* mq;
    char strBuf[257];
    size_t nSize;
    unsigned int nPriority;
    try {
        mq = new message_queue(open_or_create, BITCOINURI_QUEUE_NAME, 2, 256);

        // Make sure we don't lose any bitcoin: URIs
        for (int i = 0; i < 2; i++)
        {
            ptime d = boost::posix_time::microsec_clock::universal_time() + millisec(1);
            if(mq->timed_receive(&strBuf, sizeof(strBuf), nSize, nPriority, d))
            {
                uiInterface.ThreadSafeHandleURI(std::string(strBuf, nSize));
            }
            else
                break;
        }

        // Make sure only one bitcoin instance is listening
        message_queue::remove(BITCOINURI_QUEUE_NAME);
        mq = new message_queue(open_or_create, BITCOINURI_QUEUE_NAME, 2, 256);
    }
    catch (interprocess_exception &ex) {
        return;
    }
    if (!CreateThread(ipcThread, mq))
    {
        delete mq;
    }
}

#endif
