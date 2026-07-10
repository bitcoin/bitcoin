// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_CLIENT_H
#define BITCOIN_COINJOIN_CLIENT_H

#include <coinjoin/coinjoin.h>
#include <coinjoin/util.h>
#include <evo/types.h>
#include <interfaces/coinjoin.h>
#include <msg_result.h>

#include <net_types.h>
#include <protocol.h>
#include <util/ranges.h>
#include <util/translation.h>

#include <deque>
#include <memory>
#include <utility>

class CCoinJoinClientManager;
class CCoinJoinClientQueueManager;
class CConnman;
class CDeterministicMNManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CNode;
class CoinJoinWalletManager;
class CTxMemPool;

class UniValue;

class CPendingDsaRequest
{
private:
    static constexpr int TIMEOUT = 15;

    uint256 proTxHash;
    CCoinJoinAccept dsa;
    int64_t nTimeCreated{0};

public:
    CPendingDsaRequest() = default;

    CPendingDsaRequest(uint256 proTxHash_, CCoinJoinAccept dsa_) :
        proTxHash(std::move(proTxHash_)),
        dsa(std::move(dsa_)),
        nTimeCreated(GetTime())
    {
    }

    [[nodiscard]] uint256 GetProTxHash() const { return proTxHash; }
    [[nodiscard]] CCoinJoinAccept GetDSA() const { return dsa; }
    [[nodiscard]] bool IsExpired() const { return GetTime() - nTimeCreated > TIMEOUT; }

    friend bool operator==(const CPendingDsaRequest& a, const CPendingDsaRequest& b)
    {
        return a.proTxHash == b.proTxHash && a.dsa == b.dsa;
    }
    friend bool operator!=(const CPendingDsaRequest& a, const CPendingDsaRequest& b)
    {
        return !(a == b);
    }
    explicit operator bool() const
    {
        return *this != CPendingDsaRequest();
    }
};

class CoinJoinWalletManager {
public:
    using wallet_name_cjman_map = std::map<const std::string, std::unique_ptr<CCoinJoinClientManager>>;

public:
    CoinJoinWalletManager() = delete;
    CoinJoinWalletManager(const CoinJoinWalletManager&) = delete;
    CoinJoinWalletManager& operator=(const CoinJoinWalletManager&) = delete;
    explicit CoinJoinWalletManager(ChainstateManager& chainman, CDeterministicMNManager& dmnman,
                                   CMasternodeMetaMan& mn_metaman, const CTxMemPool& mempool,
                                   const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                                   const std::unique_ptr<CCoinJoinClientQueueManager>& queueman);
    ~CoinJoinWalletManager();

    void Add(const std::shared_ptr<wallet::CWallet>& wallet) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);
    void DoMaintenance(CConnman& connman) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);

    void Remove(const std::string& name) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);

    template <typename Callable>
    void ForEachCJClientMan(Callable&& func) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map)
    {
        LOCK(cs_wallet_manager_map);
        for (auto&& [_, clientman] : m_wallet_manager_map) {
            func(clientman);
        }
    };

    template <typename Callable>
    bool ForAnyCJClientMan(Callable&& func) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map)
    {
        LOCK(cs_wallet_manager_map);
        return ranges::any_of(m_wallet_manager_map, [&](auto& pair) { return func(pair.second); });
    };

    //! Execute func under the wallet manager lock for the client identified by name.
    //! Returns true if the client was found and func was called, false otherwise.
    template <typename Callable>
    bool DoForClient(const std::string& name, Callable&& func) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map)
    {
        LOCK(cs_wallet_manager_map);
        auto it = m_wallet_manager_map.find(name);
        if (it == m_wallet_manager_map.end()) return false;
        func(*it->second);
        return true;
    };

private:
    ChainstateManager& m_chainman;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    const CTxMemPool& m_mempool;
    const CMasternodeSync& m_mn_sync;
    const llmq::CInstantSendManager& m_isman;
    const std::unique_ptr<CCoinJoinClientQueueManager>& m_queueman;

    mutable Mutex cs_wallet_manager_map;
    wallet_name_cjman_map m_wallet_manager_map GUARDED_BY(cs_wallet_manager_map);
};

class CCoinJoinClientSession : public CCoinJoinBaseSession
{
private:
    const std::shared_ptr<wallet::CWallet> m_wallet;
    CCoinJoinClientManager& m_clientman;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    const CMasternodeSync& m_mn_sync;
    const llmq::CInstantSendManager& m_isman;
    const std::unique_ptr<CCoinJoinClientQueueManager>& m_queueman;

    std::vector<COutPoint> vecOutPointLocked;

    bilingual_str strLastMessage;
    bilingual_str strAutoDenomResult;

    CDeterministicMNCPtr mixingMasternode;
    CMutableTransaction txMyCollateral; // client side collateral
    CPendingDsaRequest pendingDsaRequest;

    CKeyHolderStorage keyHolderStorage; // storage for keys used in PrepareDenominate

    /// Create denominations
    bool CreateDenominated(CAmount nBalanceToDenominate);
    bool CreateDenominated(CAmount nBalanceToDenominate, const wallet::CompactTallyItem& tallyItem, bool fCreateMixingCollaterals)
        EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet);

    /// Split up large inputs or make fee sized inputs
    bool MakeCollateralAmounts();
    bool MakeCollateralAmounts(const wallet::CompactTallyItem& tallyItem, bool fTryDenominated)
        EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet);

    bool CreateCollateralTransaction(CMutableTransaction& txCollateral, std::string& strReason)
        EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet);

    bool JoinExistingQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman);
    bool StartNewQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman);

    /// step 0: select denominated inputs and txouts
    bool SelectDenominate(std::string& strErrorRet, std::vector<CTxDSIn>& vecTxDSInRet);
    /// step 1: prepare denominated inputs and outputs
    bool PrepareDenominate(int nMinRounds, int nMaxRounds, std::string& strErrorRet, const std::vector<CTxDSIn>& vecTxDSIn,
                           std::vector<std::pair<CTxDSIn, CTxOut>>& vecPSInOutPairsRet, bool fDryRun = false)
        EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet);
    /// step 2: send denominated inputs and outputs prepared in step 1
    bool SendDenominate(const std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsIn, CConnman& connman) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    /// Process Masternode updates about the progress of mixing
    void ProcessPoolStateUpdate(CCoinJoinStatusUpdate psssup);
    // Set the 'state' value, with some logging and capturing when the state changed
    void SetState(PoolState nStateNew);

    void CompletedTransaction(PoolMessage nMessageID);

    /// As a client, check and sign the final transaction
    bool SignFinalTransaction(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, const CTransaction& finalTransactionNew) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    void RelayIn(const CCoinJoinEntry& entry, CConnman& connman) const;

    void SetNull() override EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

public:
    explicit CCoinJoinClientSession(const std::shared_ptr<wallet::CWallet>& wallet, CCoinJoinClientManager& clientman,
                                    CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                                    const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                                    const std::unique_ptr<CCoinJoinClientQueueManager>& queueman);

    void ProcessMessage(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv);

    void UnlockCoins();

    void ResetPool() EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    bilingual_str GetStatus(bool fWaitForBlock) const;

    bool GetMixingMasternodeInfo(CDeterministicMNCPtr& ret) const;

    /// Passively run mixing in the background according to the configuration in settings
    bool DoAutomaticDenominating(ChainstateManager& chainman, CConnman& connman, const CTxMemPool& mempool,
                                 bool fDryRun = false) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    /// As a client, submit part of a future mixing transaction to a Masternode to start the process
    bool SubmitDenominate(CConnman& connman);

    bool ProcessPendingDsaRequest(CConnman& connman);

    bool CheckTimeout();

    void GetJsonInfo(UniValue& obj) const;
};

/** Used to keep track of mixing queues
 */
class CCoinJoinClientQueueManager : public CCoinJoinBaseManager
{
private:
    CoinJoinWalletManager& m_walletman;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    const CMasternodeSync& m_mn_sync;

    mutable Mutex cs_ProcessDSQueue;

public:
    CCoinJoinClientQueueManager() = delete;
    CCoinJoinClientQueueManager(const CCoinJoinClientQueueManager&) = delete;
    CCoinJoinClientQueueManager& operator=(const CCoinJoinClientQueueManager&) = delete;
    explicit CCoinJoinClientQueueManager(CoinJoinWalletManager& walletman, CDeterministicMNManager& dmnman,
                                         CMasternodeMetaMan& mn_metaman, const CMasternodeSync& mn_sync);
    ~CCoinJoinClientQueueManager();

    [[nodiscard]] MessageProcessingResult ProcessMessage(NodeId from, CConnman& connman, std::string_view msg_type,
                                                         CDataStream& vRecv)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue, !cs_ProcessDSQueue);
    void DoMaintenance();
};

/** Used to keep track of current status of mixing pool
 */
class CCoinJoinClientManager : public interfaces::CoinJoin::Client
{
private:
    const std::shared_ptr<wallet::CWallet> m_wallet;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    const CMasternodeSync& m_mn_sync;
    const llmq::CInstantSendManager& m_isman;
    const std::unique_ptr<CCoinJoinClientQueueManager>& m_queueman;

    mutable Mutex cs_deqsessions;
    // TODO: or map<denom, CCoinJoinClientSession> ??
    std::deque<CCoinJoinClientSession> deqSessions GUARDED_BY(cs_deqsessions);

    int nCachedLastSuccessBlock{0};
    int nMinBlocksToWait{1}; // how many blocks to wait for after one successful mixing tx in non-multisession mode
    bilingual_str strAutoDenomResult;

    // Keep track of current block height
    int nCachedBlockHeight{0};

    int nCachedNumBlocks{std::numeric_limits<int>::max()};    // used for the overview screen
    bool fCreateAutoBackups{true}; // builtin support for automatic backups

    bool WaitForAnotherBlock() const;

    // Make sure we have enough keys since last backup
    bool CheckAutomaticBackup();

public:
    CCoinJoinClientManager() = delete;
    CCoinJoinClientManager(const CCoinJoinClientManager&) = delete;
    CCoinJoinClientManager& operator=(const CCoinJoinClientManager&) = delete;
    explicit CCoinJoinClientManager(const std::shared_ptr<wallet::CWallet>& wallet, CDeterministicMNManager& dmnman,
                                    CMasternodeMetaMan& mn_metaman, const CMasternodeSync& mn_sync,
                                    const llmq::CInstantSendManager& isman,
                                    const std::unique_ptr<CCoinJoinClientQueueManager>& queueman);
    ~CCoinJoinClientManager();

    void ProcessMessage(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    bool GetMixingMasternodesInfo(std::vector<CDeterministicMNCPtr>& vecDmnsRet) const EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    /// Passively run mixing in the background according to the configuration in settings
    bool DoAutomaticDenominating(ChainstateManager& chainman, CConnman& connman, const CTxMemPool& mempool,
                                 bool fDryRun = false) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    bool TrySubmitDenominate(const uint256& proTxHash, CConnman& connman) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
    bool MarkAlreadyJoinedQueueAsTried(CCoinJoinQueue& dsq) const EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    void CheckTimeout() EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    void ProcessPendingDsaRequest(CConnman& connman) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    void AddUsedMasternode(const uint256& proTxHash);
    CDeterministicMNCPtr GetRandomNotUsedMasternode();

    void UpdatedSuccessBlock();

    void UpdatedBlockTip(const CBlockIndex* pindex);

    void DoMaintenance(ChainstateManager& chainman, CConnman& connman, const CTxMemPool& mempool)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    // interfaces::CoinJoin::Client overrides
    void resetCachedBlocks() override { nCachedNumBlocks = std::numeric_limits<int>::max(); }
    int getCachedBlocks() const override { return nCachedNumBlocks; }
    void setCachedBlocks(int nCachedBlocks) override { nCachedNumBlocks = nCachedBlocks; }
    void disableAutobackups() override { fCreateAutoBackups = false; }
    void resetPool() override EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
    UniValue getJsonInfo() const override EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
    std::vector<std::string> getSessionStatuses() const override EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
    std::string getSessionDenoms() const override EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
    bool isMixing() const override;
    bool startMixing() override;
    void stopMixing() override;
};

#endif // BITCOIN_COINJOIN_CLIENT_H
