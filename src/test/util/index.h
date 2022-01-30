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

    //! Sync index in foreground without creating background thread.
    void Sync();

    BaseIndex& m_index;
};

#endif // BITCOIN_TEST_UTIL_INDEX_H
