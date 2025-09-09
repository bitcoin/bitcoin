// Copyright (c) 2023-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/context.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET
#include <coinjoin/coinjoin.h>

CJContext::CJContext(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                     CTxMemPool& mempool, const CActiveMasternodeManager* const mn_activeman,
                     const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman, bool relay_txes) :
#ifdef ENABLE_WALLET
    walletman{std::make_unique<CoinJoinWalletManager>(chainman, dmnman, mn_metaman, mempool, mn_sync, isman, queueman,
                                                      /*is_masternode=*/mn_activeman != nullptr)},
    queueman{relay_txes ? std::make_unique<CCoinJoinClientQueueManager>(*walletman, dmnman, mn_metaman, mn_sync,
                                                                        /*is_masternode=*/mn_activeman != nullptr)
                        : nullptr},
#endif // ENABLE_WALLET
    dstxman{std::make_unique<CDSTXManager>()}
{}

CJContext::~CJContext() {}
