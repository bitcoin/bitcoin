// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_INDEX_H
#define BITCOIN_TEST_UTIL_INDEX_H

class BaseIndex;

class IndexTester
{
public:
    IndexTester(BaseIndex& index) : m_index{index} {}

    //! Bring the index up to the current chain tip and leave it connected, so it
    //! keeps indexing blocks that are connected afterwards (as on a running node).
    //! Internally this runs the index's background sync (StartBackgroundSync +
    //! WaitForBackgroundSync): a sync thread is created and joined within this
    //! call, so individual tests don't have to manage one. This is the method
    //! tests should normally use.
    void Sync();

    //! Bring the index up to the current chain tip once, synchronously in the
    //! foreground, and leave it disconnected: blocks connected after this returns
    //! are NOT indexed. Intended for the sync benchmark, where a deterministic
    //! foreground sync with no background thread is wanted.
    void SyncOnce();

    BaseIndex& m_index;
};

#endif // BITCOIN_TEST_UTIL_INDEX_H
