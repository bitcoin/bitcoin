// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"

#include "crypto/sha256.h"
#include "key.h"
#include "main.h"
#include "util.h"
#include "sync.h"
#include <boost/thread.hpp>

// BU add lockstack stuff here for bitcoin-cli, because I need to carefully
// order it in globals.cpp for bitcoind and bitcoin-qt
boost::mutex dd_mutex;
std::map<std::pair<void*, void*>, LockStack> lockorders;
boost::thread_specific_ptr<LockStack> lockstack;

int
main(int argc, char** argv)
{
    SHA256AutoDetect();
    ECC_Start();
    SetupEnvironment();
    fPrintToDebugLog = false; // don't want to write to debug.log file

    benchmark::BenchRunner::RunAll();

    ECC_Stop();
}
