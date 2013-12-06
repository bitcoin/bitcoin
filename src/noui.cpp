// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "noui.h"

#include "log.h"
#include "ui_interface.h"
#include "util.h"

#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>

static bool noui_ThreadSafeMessageBox(const std::string& message, const std::string& caption, unsigned int style)
{
    std::ostringstream ossCaption;
    // Check for usage of predefined caption
    switch (style) {
    case CClientUIInterface::MSG_ERROR:
        ossCaption << _<std::string>("Error");
        break;
    case CClientUIInterface::MSG_WARNING:
        ossCaption << _<std::string>("Warning");
        break;
    case CClientUIInterface::MSG_INFORMATION:
        ossCaption << _<std::string>("Information");
        break;
    default:
        ossCaption << caption; // Use supplied caption (can be empty)
    }

    Log() << ossCaption << ": " << message << "\n";
    std::cerr << ossCaption << ": " << message << "\n";
    return false;
}

static bool noui_ThreadSafeAskFee(int64_t /*nFeeRequired*/)
{
    return true;
}

static void noui_InitMessage(const std::string &message)
{
    Log() << "init message: " << message << "\n";
}

void noui_connect()
{
    // Connect bitcoind signal handlers
    uiInterface.ThreadSafeMessageBox.connect(noui_ThreadSafeMessageBox);
    uiInterface.ThreadSafeAskFee.connect(noui_ThreadSafeAskFee);
    uiInterface.InitMessage.connect(noui_InitMessage);
}
