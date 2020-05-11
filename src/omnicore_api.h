// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OMINICORE_API_H
#define BITCOIN_OMINICORE_API_H

#include <primitives/transaction.h>
#include <sync.h>
#include <txmempool.h>

#include <map>
#include <memory>

class CBlockIndex;
class Coin;
class COutPoint;

class uint256;

extern CCriticalSection cs_main;
extern CTxMemPool mempool;

//! Lock order: cs_main > mempool.cs > cs_tally > cs_pending
namespace omnicore_api {

/** Handler to initialize Omni Core. */
void Init() LOCKS_EXCLUDED(cs_main);

/** Handler to shut down Omni Core. */
void Shutdown() LOCKS_EXCLUDED(cs_main);

/** Return true if enable omnicore */
bool Enabled();

/** Block and transaction handlers. */
void HandlerDiscBegin(int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);
int HandlerBlockBegin(int nBlockNow, CBlockIndex const * pBlockIndex) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);
int HandlerBlockEnd(int nBlockNow, CBlockIndex const * pBlockIndex, unsigned int countMP) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);
bool HandlerTx(const CTransaction& tx, int nBlock, unsigned int idx, const CBlockIndex* pBlockIndex, const std::shared_ptr<std::map<COutPoint, Coin>> removedCoins) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);

/** Scans for marker and if one is found, add transaction to marker cache. */
void TryToAddToMarkerCache(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);

/** Removes transaction from marker cache. */
void RemoveFromMarkerCache(const uint256& txHash) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);

/** Global handler to total wallet balances. */
void CheckWalletUpdate(bool forceUpdate = false) LOCKS_EXCLUDED(cs_main);

/** Set the log file should be reopened. */
void ReopenDebugLog();

/** Indicate UI mode. */
void EnableQtMode();

}

#endif // BITCOIN_OMINICORE_API_H