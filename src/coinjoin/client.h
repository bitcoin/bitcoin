// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_CLIENT_H
#define BITCOIN_COINJOIN_CLIENT_H

#include <coinjoin/util.h>
#include <coinjoin/coinjoin.h>

#include <net_types.h>
#include <util/translation.h>

#include <atomic>
#include <deque>
#include <memory>
#include <utility>

class CCoinJoinClientManager;
class CCoinJoinClientQueueManager;
class CConnman;
class CDeterministicMN;
class CDeterministicMNManager;
class CNode;
class CMasternodeMetaMan;
class CMasternodeSync;
class CoinJoinWalletManager;
class CTxMemPool;

class UniValue;

using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;

class CPendingDsaRequest
{
private:
    static constexpr int TIMEOUT = 15;

    CService addr;
    CCoinJoinAccept dsa;
    int64_t nTimeCreated{0};

public:
    CPendingDsaRequest() = default;

    CPendingDsaRequest(CService addr_, CCoinJoinAccept dsa_) :
        addr(std::move(addr_)),
        dsa(std::move(dsa_)),
        nTimeCreated(GetTime())
    {
    }

    [[nodiscard]] CService GetAddr() const { return addr; }
    [[nodiscard]] CCoinJoinAccept GetDSA() const { return dsa; }
    [[nodiscard]] bool IsExpired() const { return GetTime() - nTimeCreated > TIMEOUT; }

    friend bool operator==(const CPendingDsaRequest& a, const CPendingDsaRequest& b)
    {
        return a.addr == b.addr && a.dsa == b.dsa;
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
    CoinJoinWalletManager(CChainState& chainstate, CConnman& connman, CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman, CTxMemPool& mempool,
                          const CMasternodeSync& mn_sync, const std::unique_ptr<CCoinJoinClientQueueManager>& queueman, bool is_masternode)
        : m_chainstate(chainstate), m_connman(connman), m_dmnman(dmnman), m_mn_metaman(mn_metaman), m_mempool(mempool), m_mn_sync(mn_sync),
          m_queueman(queueman), m_is_masternode{is_masternode}
    {}

    ~CoinJoinWalletManager() {
        for (auto& [wallet_name, cj_man] : m_wallet_manager_map) {
            cj_man.reset();
        }
    }

    void Add(CWallet& wallet);
    void DoMaintenance();

    void Remove(const std::string& name);
    void Flush(const std::string& name);

    CCoinJoinClientManager* Get(const std::string& name) const;

    const wallet_name_cjman_map& raw() const { return m_wallet_manager_map; }

private:
    CChainState& m_chainstate;
    CConnman& m_connman;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    CTxMemPool& m_mempool;
    const CMasternodeSync& m_mn_sync;
    const std::unique_ptr<CCoinJoinClientQueueManager>& m_queueman;

    const bool m_is_masternode;
    wallet_name_cjman_map m_wallet_manager_map;
};

class CCoinJoinClientSession : public CCoinJoinBaseSession
{
private:
    CWallet& m_wallet;
    CoinJoinWalletManager& m_walletman;
    CCoinJoinClientManager& m_manager;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    const CMasternodeSync& m_mn_sync;
    const std::unique_ptr<CCoinJoinClientQueueManager>& m_queueman;

    // Track node type
    const bool m_is_masternode;

    std::vector<COutPoint> vecOutPointLocked;

    bilingual_str strLastMessage;
    bilingual_str strAutoDenomResult;

    CDeterministicMNCPtr mixingMasternode;
    CMutableTransaction txMyCollateral; // client side collateral
    CPendingDsaRequest pendingDsaRequest;

    CKeyHolderStorage keyHolderStorage; // storage for keys used in PrepareDenominate

    /// Create denominations
    bool CreateDenominated(CAmount nBalanceToDenominate);
    bool CreateDenominated(CAmount nBalanceToDenominate, const CompactTallyItem& tallyItem, bool fCreateMixingCollaterals);

    /// Split up large inputs or make fee sized inputs
    bool MakeCollateralAmounts();
    bool MakeCollateralAmounts(const CompactTallyItem& tallyItem, bool fTryDenominated);

    bool CreateCollateralTransaction(CMutableTransaction& txCollateral, std::string& strReason);

    bool JoinExistingQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman);
    bool StartNewQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman);

    /// step 0: select denominated inputs and txouts
    bool SelectDenominate(std::string& strErrorRet, std::vector<CTxDSIn>& vecTxDSInRet);
    /// step 1: prepare denominated inputs and outputs
    bool PrepareDenominate(int nMinRounds, int nMaxRounds, std::string& strErrorRet, const std::vector<CTxDSIn>& vecTxDSIn, std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsRet, bool fDryRun = false);
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
    explicit CCoinJoinClientSession(CWallet& wallet, CoinJoinWalletManager& walletman, CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                                    const CMasternodeSync& mn_sync, const std::unique_ptr<CCoinJoinClientQueueManager>& queueman, bool is_masternode);

    void ProcessMessage(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv);

    void UnlockCoins();

    void ResetPool() EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    bilingual_str GetStatus(bool fWaitForBlock) const;

    bool GetMixingMasternodeInfo(CDeterministicMNCPtr& ret) const;

    /// Passively run mixing in the background according to the configuration in settings
    bool DoAutomaticDenominating(CChainState& active_chainstate, CConnman& connman, CTxMemPool& mempool, bool fDryRun = false) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

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
    CConnman& connman;
    CoinJoinWalletManager& m_walletman;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    const CMasternodeSync& m_mn_sync;

    mutable Mutex cs_ProcessDSQueue;
    const bool m_is_masternode;

public:
    explicit CCoinJoinClientQueueManager(CConnman& _connman, CoinJoinWalletManager& walletman, CDeterministicMNManager& dmnman,
                                         CMasternodeMetaMan& mn_metaman, const CMasternodeSync& mn_sync, bool is_masternode) :
        connman(_connman), m_walletman(walletman), m_dmnman(dmnman), m_mn_metaman(mn_metaman), m_mn_sync(mn_sync), m_is_masternode{is_masternode} {};

    PeerMsgRet ProcessMessage(const CNode& peer, std::string_view msg_type, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);
    PeerMsgRet ProcessDSQueue(const CNode& peer, CDataStream& vRecv);
    void DoMaintenance();
};

/** Used to keep track of current status of mixing pool
 */
class CCoinJoinClientManager
{
private:
    CWallet& m_wallet;
    CoinJoinWalletManager& m_walletman;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    const CMasternodeSync& m_mn_sync;
    const std::unique_ptr<CCoinJoinClientQueueManager>& m_queueman;

    // Track node type
    const bool m_is_masternode;

    // Keep track of the used Masternodes
    std::vector<COutPoint> vecMasternodesUsed;

    mutable Mutex cs_deqsessions;
    // TODO: or map<denom, CCoinJoinClientSession> ??
    std::deque<CCoinJoinClientSession> deqSessions GUARDED_BY(cs_deqsessions);

    std::atomic<bool> fMixing{false};

    int nCachedLastSuccessBlock{0};
    int nMinBlocksToWait{1}; // how many blocks to wait for after one successful mixing tx in non-multisession mode
    bilingual_str strAutoDenomResult;

    // Keep track of current block height
    int nCachedBlockHeight{0};

    bool WaitForAnotherBlock() const;

    // Make sure we have enough keys since last backup
    bool CheckAutomaticBackup();

public:
    int nCachedNumBlocks{std::numeric_limits<int>::max()};    // used for the overview screen
    bool fCreateAutoBackups{true}; // builtin support for automatic backups

    CCoinJoinClientManager() = delete;
    CCoinJoinClientManager(CCoinJoinClientManager const&) = delete;
    CCoinJoinClientManager& operator=(CCoinJoinClientManager const&) = delete;

    explicit CCoinJoinClientManager(CWallet& wallet, CoinJoinWalletManager& walletman, CDeterministicMNManager& dmnman,
                                    CMasternodeMetaMan& mn_metaman, const CMasternodeSync& mn_sync,
                                    const std::unique_ptr<CCoinJoinClientQueueManager>& queueman, bool is_masternode) :
        m_wallet(wallet), m_walletman(walletman), m_dmnman(dmnman), m_mn_metaman(mn_metaman), m_mn_sync(mn_sync), m_queueman(queueman),
        m_is_masternode{is_masternode} {}

    void ProcessMessage(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    bool StartMixing();
    void StopMixing();
    bool IsMixing() const;
    void ResetPool() EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    bilingual_str GetStatuses() EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
    std::string GetSessionDenoms() EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    bool GetMixingMasternodesInfo(std::vector<CDeterministicMNCPtr>& vecDmnsRet) const EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    /// Passively run mixing in the background according to the configuration in settings
    bool DoAutomaticDenominating(CChainState& active_chainstate, CConnman& connman, CTxMemPool& mempool, bool fDryRun = false) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    bool TrySubmitDenominate(const CService& mnAddr, CConnman& connman) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
    bool MarkAlreadyJoinedQueueAsTried(CCoinJoinQueue& dsq) const EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    void CheckTimeout() EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    void ProcessPendingDsaRequest(CConnman& connman) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    void AddUsedMasternode(const COutPoint& outpointMn);
    CDeterministicMNCPtr GetRandomNotUsedMasternode();

    void UpdatedSuccessBlock();

    void UpdatedBlockTip(const CBlockIndex* pindex);

    void DoMaintenance(CChainState& active_chainstate, CConnman& connman, CTxMemPool& mempool) EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);

    void GetJsonInfo(UniValue& obj) const EXCLUSIVE_LOCKS_REQUIRED(!cs_deqsessions);
};

#endif // BITCOIN_COINJOIN_CLIENT_H
