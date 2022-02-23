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
    // SYSCOIN keep copy to work with dynamic thread names in LLMQ code
    std::string strName = std::string(thread_name);
    util::ThreadRename(thread_name);
    try {
        LogPrintf("%s thread start\n", strName);
        thread_func();
        LogPrintf("%s thread exit\n", strName);
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, strName.c_str());
        throw;
    } catch (...) {
        PrintExceptionContinue(nullptr, strName.c_str());
        throw;
    }
}
