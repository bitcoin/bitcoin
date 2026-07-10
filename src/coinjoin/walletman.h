// Copyright (c) 2023-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_WALLETMAN_H
#define BITCOIN_COINJOIN_WALLETMAN_H

#include <evo/types.h>
#include <msg_result.h>

#include <validationinterface.h>

#include <functional>
#include <optional>
#include <string_view>

class CBlockIndex;
class CChainState;
class CCoinJoinClientManager;
class CCoinJoinQueue;
class CConnman;
class CDataStream;
class CDeterministicMNManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CNode;
class CScheduler;
class CTxMemPool;
namespace llmq {
class CInstantSendManager;
} // namespace llmq
namespace wallet {
class CWallet;
} // namespace wallet

class CJWalletManager : public CValidationInterface
{
public:
    static std::unique_ptr<CJWalletManager> make(ChainstateManager& chainman, CDeterministicMNManager& dmnman,
                                                 CMasternodeMetaMan& mn_metaman, CTxMemPool& mempool,
                                                 const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                                                 bool relay_txes);
    virtual ~CJWalletManager() = default;

public:
    virtual void Schedule(CConnman& connman, CScheduler& scheduler) = 0;

public:
    virtual bool hasQueue(const uint256& hash) const = 0;
    //! Execute func under the wallet manager lock for the client identified by name.
    //! Returns true if the client was found and func was called, false otherwise.
    virtual bool doForClient(const std::string& name, const std::function<void(CCoinJoinClientManager&)>& func) = 0;
    virtual MessageProcessingResult processMessage(CNode& peer, CChainState& chainstate, CConnman& connman,
                                                   CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv) = 0;
    virtual std::optional<CCoinJoinQueue> getQueueFromHash(const uint256& hash) const = 0;
    virtual std::optional<int> getQueueSize() const = 0;
    virtual std::vector<CDeterministicMNCPtr> getMixingMasternodes() = 0;
    virtual void addWallet(const std::shared_ptr<wallet::CWallet>& wallet) = 0;
    virtual void removeWallet(const std::string& name) = 0;
    virtual void flushWallet(const std::string& name) = 0;

protected:
    // CValidationInterface
    virtual void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork,
                                 bool fInitialDownload) override = 0;
};

#endif // BITCOIN_COINJOIN_WALLETMAN_H
