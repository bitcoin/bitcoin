// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_TEST_IPC_TEST_H
#define BITCOIN_IPC_TEST_IPC_TEST_H

#include <interfaces/types.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <univalue.h>
#include <util/fs.h>
#include <validation.h>

#include <condition_variable>
#include <mutex>

class FooImplementation
{
public:
    int add(int a, int b) { return a + b; }
    COutPoint passOutPoint(COutPoint o) { return o; }
    UniValue passUniValue(UniValue v) { return v; }
    CTransactionRef passTransaction(CTransactionRef t) { return t; }
    std::vector<CTransactionRef> passTransactions(std::vector<CTransactionRef> t) { return t; }
    std::vector<char> passVectorChar(std::vector<char> v) { return v; }
    BlockValidationState passBlockState(BlockValidationState s) { return s; }
    CScript passScript(CScript s) { return s; }
    void waitCancel(interfaces::CancelArg cancel)
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool canceled{false};
        const interfaces::CancelGuard guard{cancel([&] {
            const std::lock_guard lock{mutex};
            canceled = true;
            cv.notify_all();
        })};
        std::unique_lock lock{mutex};
        cv.wait(lock, [&] { return canceled; });
    }
};

#endif // BITCOIN_IPC_TEST_IPC_TEST_H
