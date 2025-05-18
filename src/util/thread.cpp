// Copyright (c) 2021-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/thread.h>

#include <logging.h>
#include <util/exception.h>
#include <util/threadnames.h>

#include <exception>
#include <functional>
#include <string>
#include <utility>

void util::TraceThread(std::string_view thread_name, std::function<void()> thread_func)
{
    util::ThreadRename(std::string{thread_name});
    try {
        LogPrintf("%s thread start\n", thread_name);
        thread_func();
        LogPrintf("%s thread exit\n", thread_name);
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, thread_name);
        throw;
    } catch (...) {
        PrintExceptionContinue(nullptr, thread_name);
        throw;
    }
}
