// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/index.h>

#include <index/base.h>
#include <util/time.h>

void IndexWaitSynced(const BaseIndex& index)
{
    while (!index.BlockUntilSyncedToCurrentChain()) {
        UninterruptibleSleep(100ms);
    }
}
