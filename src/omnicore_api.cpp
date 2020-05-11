// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <omnicore_api.h>

#include <omnicore/log.h>
#include <omnicore/omnicore.h>
#include <omnicore/utilsui.h>

static bool fInitialed = false;

void omnicore_api::Init()
{
    LOCK2(cs_main, ::mempool.cs);
    mastercore_init();

    fInitialed = true;
}

void omnicore_api::Shutdown()
{
    if (!fInitialed) return ;

    LOCK(cs_main);
    ::mastercore_shutdown();
}

bool omnicore_api::Enabled()
{
    return fInitialed;
}

void omnicore_api::HandlerDiscBegin(int nHeight)
{
    assert(fInitialed);
    AssertLockHeld(cs_main);
    AssertLockHeld(::mempool.cs);
    ::mastercore_handler_disc_begin(nHeight);
}

int omnicore_api::HandlerBlockBegin(int nBlockNow, CBlockIndex const * pBlockIndex)
{
    assert(fInitialed);
    AssertLockHeld(cs_main);
    AssertLockHeld(::mempool.cs);
    return ::mastercore_handler_block_begin(nBlockNow, pBlockIndex);
}

int omnicore_api::HandlerBlockEnd(int nBlockNow, CBlockIndex const * pBlockIndex, unsigned int countMP)
{
    assert(fInitialed);
    AssertLockHeld(cs_main);
    AssertLockHeld(::mempool.cs);
    return ::mastercore_handler_block_end(nBlockNow, pBlockIndex, countMP);
}

bool omnicore_api::HandlerTx(const CTransaction& tx, int nBlock, unsigned int idx, const CBlockIndex* pBlockIndex, const std::shared_ptr<std::map<COutPoint, Coin>> removedCoins)
{
    assert(fInitialed);
    AssertLockHeld(cs_main);
    AssertLockHeld(::mempool.cs);
    return ::mastercore_handler_tx(tx, nBlock, idx, pBlockIndex, removedCoins);
}

void omnicore_api::TryToAddToMarkerCache(const CTransactionRef& tx)
{
    assert(fInitialed);
    AssertLockHeld(cs_main);
    AssertLockHeld(::mempool.cs);
    ::TryToAddToMarkerCache(tx);
}

void omnicore_api::RemoveFromMarkerCache(const uint256& txHash)
{
    assert(fInitialed);
    AssertLockHeld(cs_main);
    AssertLockHeld(::mempool.cs);
    ::RemoveFromMarkerCache(txHash);
}

void omnicore_api::CheckWalletUpdate(bool forceUpdate)
{
    assert(fInitialed);
    LOCK(cs_main);
    ::CheckWalletUpdate(forceUpdate);
}

void omnicore_api::ReopenDebugLog()
{
    ::ReopenDebugLog();
}

void omnicore_api::EnableQtMode()
{
    fQtMode = true;
}
