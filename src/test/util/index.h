// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_INDEX_H
#define BITCOIN_TEST_UTIL_INDEX_H

class BaseIndex;

/** Block until the index is synced to the current chain */
void IndexWaitSynced(BaseIndex& index);

#endif // BITCOIN_TEST_UTIL_INDEX_H
