// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/index.h>

#include <index/base.h>
#include <util/check.h>
#include <util/time.h>

void IndexWaitSynced(BaseIndex& index)
{
    const auto timeout{SteadyClock::now() + 120s};
    while (!index.BlockUntilSyncedToCurrentChain()) {
        Assert(timeout > SteadyClock::now());
        UninterruptibleSleep(100ms);
    }
}
