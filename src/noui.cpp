// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "ui_interface.h"

#include <string>
#include "init.h"

int ThreadSafeMessageBox(const std::string& message, const std::string& caption, int style)
{
    printf("%s: %s\n", caption.c_str(), message.c_str());
    fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
    return 4;
}

bool ThreadSafeAskFee(int64 nFeeRequired, const std::string& strCaption)
{
    return true;
}

void MainFrameRepaint()
{
}

void AddressBookRepaint()
{
}

void InitMessage(const std::string &message)
{
}

std::string _(const char* psz)
{
    return psz;
}

void QueueShutdown()
{
    // Without UI, Shutdown can simply be started in a new thread
    CreateThread(Shutdown, NULL);
}

