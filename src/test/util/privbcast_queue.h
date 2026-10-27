// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_PRIVBCAST_QUEUE_H
#define BITCOIN_TEST_UTIL_PRIVBCAST_QUEUE_H

#include <kernel/mempool_entry.h>
#include <node/privbcast.h>
#include <primitives/transaction.h>

#include <chrono>

namespace node {
/** Runs the queue's scheduler passes and its mempool notifications on the test's thread. */
struct PrivbcastQueueTest {
    static constexpr std::chrono::milliseconds POLL_INTERVAL{PrivbcastQueue::POLL_INTERVAL};
    static void Poll(PrivbcastQueue& queue) { queue.Poll(); }
    static void TransactionAddedToMempool(PrivbcastQueue& queue, const CTransactionRef& tx)
    {
        queue.TransactionAddedToMempool(NewMempoolTransactionInfo{tx, /*fee=*/0, /*vsize=*/0, /*height=*/0,
                                                                  /*mempool_limit_bypassed=*/false, /*submitted_in_package=*/false,
                                                                  /*chainstate_is_current=*/true, /*has_no_mempool_parents=*/true},
                                        /*mempool_sequence=*/0);
    }
};
} // namespace node

#endif // BITCOIN_TEST_UTIL_PRIVBCAST_QUEUE_H
