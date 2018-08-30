// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_RUNNER_H
#define BITCOIN_INDEX_RUNNER_H

#include <thread>

#include <threadinterrupt.h>

class BaseIndex;

/**
 * RAII interface for activating indexes to stay in sync with blockchain updates.
 */
class IndexRunner
{
private:
    BaseIndex* m_index;

    std::thread m_thread_sync;
    CThreadInterrupt m_interrupt;

public:

    /**
     * Start initializes the sync state and registers the index as a ValidationInterface so that it
     * stays in sync with blockchain updates.
     */
    explicit IndexRunner(BaseIndex* index);

    /** Stops the instance from staying in sync with blockchain updates. */
    ~IndexRunner();

    /** Interrupts the sync thread if it is running. */
    void Interrupt();
};

bool StartIndex(BaseIndex* index);
bool InterruptIndex(BaseIndex* index);
bool StopIndex(BaseIndex* index);

#endif // BITCOIN_INDEX_RUNNER_H
