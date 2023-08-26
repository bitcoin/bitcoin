// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/thread.h>

#include <logging.h>
#include <util/system.h>
#include <util/threadnames.h>

#include <exception>

void util::TraceThread(const char* thread_name, std::function<void()> thread_func)
{
    util::ThreadRename(thread_name);
    try {
        LogPrintf("%s thread start\n", thread_name);
        thread_func();
        LogPrintf("%s thread exit\n", thread_name);
    } catch (...) {
        PrintExceptionContinue(std::current_exception(), thread_name);
        throw;
    }
}
