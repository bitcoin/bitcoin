// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITCOIN_BITCOIND
#define _BITCOIN_BITCOIND 1

#include "rpcserver.h"
#include "rpcclient.h"
#include "init.h"
#include "main.h"
#include "noui.h"
#include "ui_interface.h"
#include "util.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

extern void DetectShutdownThread(boost::thread_group* threadGroup);
extern bool AppInit(int argc, char* argv[]);

#endif
