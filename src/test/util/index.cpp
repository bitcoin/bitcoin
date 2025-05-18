// Copyright (c) 2020-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/index.h>

#include <index/base.h>
#include <util/check.h>
#include <util/signalinterrupt.h>
#include <util/time.h>

void IndexWaitSynced(const BaseIndex& index, const util::SignalInterrupt& interrupt)
{
    while (!index.BlockUntilSyncedToCurrentChain()) {
        // Assert shutdown was not requested to abort the test, instead of looping forever, in case
        // there was an unexpected error in the index that caused it to stop syncing and request a shutdown.
        Assert(!interrupt);

        UninterruptibleSleep(100ms);
    }
    assert(index.GetSummary().synced);
}
