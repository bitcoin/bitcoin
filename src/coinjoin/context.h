// Copyright (c) 2023-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_CONTEXT_H
#define BITCOIN_COINJOIN_CONTEXT_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <memory>

class CBlockPolicyEstimator;
class CChainState;
class CCoinJoinServer;
class CConnman;
class CMasternodeSync;
class CTxMemPool;

#ifdef ENABLE_WALLET
class CCoinJoinClientQueueManager;
class CoinJoinWalletManager;
#endif // ENABLE_WALLET

struct CJContext {
    CJContext() = delete;
    CJContext(const CJContext&) = delete;
    CJContext(CChainState& chainstate, CConnman& connman, CTxMemPool& mempool, const CMasternodeSync& mn_sync, bool relay_txes);
    ~CJContext();

#ifdef ENABLE_WALLET
    // The main object for accessing mixing
    const std::unique_ptr<CoinJoinWalletManager> walletman;
    const std::unique_ptr<CCoinJoinClientQueueManager> queueman;
#endif // ENABLE_WALLET
    const std::unique_ptr<CCoinJoinServer> server;
};

#endif // BITCOIN_COINJOIN_CONTEXT_H
