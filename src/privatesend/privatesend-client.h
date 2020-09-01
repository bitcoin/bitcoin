// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVATESEND_PRIVATESEND_CLIENT_H
#define BITCOIN_PRIVATESEND_PRIVATESEND_CLIENT_H

#include <privatesend/privatesend-util.h>
#include <privatesend/privatesend.h>
#include <wallet/wallet.h>

#include <evo/deterministicmns.h>

class CPrivateSendClientOptions;
class CPrivateSendClientManager;
class CPrivateSendClientQueueManager;

class CConnman;
class CNode;

class UniValue;

static const int MIN_PRIVATESEND_SESSIONS = 1;
static const int MIN_PRIVATESEND_ROUNDS = 2;
static const int MIN_PRIVATESEND_AMOUNT = 2;
static const int MIN_PRIVATESEND_DENOMS_GOAL = 10;
static const int MIN_PRIVATESEND_DENOMS_HARDCAP = 10;
static const int MAX_PRIVATESEND_SESSIONS = 10;
static const int MAX_PRIVATESEND_ROUNDS = 16;
static const int MAX_PRIVATESEND_DENOMS_GOAL = 100000;
static const int MAX_PRIVATESEND_DENOMS_HARDCAP = 100000;
static const int MAX_PRIVATESEND_AMOUNT = MAX_MONEY / COIN;
static const int DEFAULT_PRIVATESEND_SESSIONS = 4;
static const int DEFAULT_PRIVATESEND_ROUNDS = 4;
static const int DEFAULT_PRIVATESEND_AMOUNT = 1000;
static const int DEFAULT_PRIVATESEND_DENOMS_GOAL = 50;
static const int DEFAULT_PRIVATESEND_DENOMS_HARDCAP = 300;

static const bool DEFAULT_PRIVATESEND_AUTOSTART = false;
static const bool DEFAULT_PRIVATESEND_MULTISESSION = false;

// How many new denom outputs to create before we consider the "goal" loop in CreateDenominated
// a final one and start creating an actual tx. Same limit applies for the "hard cap" part of the algo.
// NOTE: We do not allow txes larger than 100kB, so we have to limit the number of outputs here.
// We still want to create a lot of outputs though.
// Knowing that each CTxOut is ~35b big, 400 outputs should take 400 x ~35b = ~17.5kb.
// More than 500 outputs starts to make qt quite laggy.
// Additionally to need all 500 outputs (assuming a max per denom of 50) you'd need to be trying to
// create denominations for over 3000 dash!
static const int PRIVATESEND_DENOM_OUTPUTS_THRESHOLD = 500;

// Warn user if mixing in gui or try to create backup if mixing in daemon mode
// when we have only this many keys left
static const int PRIVATESEND_KEYS_THRESHOLD_WARNING = 100;
// Stop mixing completely, it's too dangerous to continue when we have only this many keys left
static const int PRIVATESEND_KEYS_THRESHOLD_STOP = 50;
// Pseudorandomly mix up to this many times in addition to base round count
static const int PRIVATESEND_RANDOM_ROUNDS = 3;

// The main object for accessing mixing
extern std::map<const std::string, CPrivateSendClientManager*> privateSendClientManagers;

// The object to track mixing queues
extern CPrivateSendClientQueueManager privateSendClientQueueManager;

class CPendingDsaRequest
{
private:
    static const int TIMEOUT = 15;

    CService addr;
    CPrivateSendAccept dsa;
    int64_t nTimeCreated;

public:
    CPendingDsaRequest() :
        addr(CService()),
        dsa(CPrivateSendAccept()),
        nTimeCreated(0)
    {
    }

    CPendingDsaRequest(const CService& addr_, const CPrivateSendAccept& dsa_) :
        addr(addr_),
        dsa(dsa_),
        nTimeCreated(GetTime())
    {
    }

    CService GetAddr() { return addr; }
    CPrivateSendAccept GetDSA() { return dsa; }
    bool IsExpired() const { return GetTime() - nTimeCreated > TIMEOUT; }

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

class CPrivateSendClientSession : public CPrivateSendBaseSession
{
private:
    std::vector<COutPoint> vecOutPointLocked;

    std::string strLastMessage;
    std::string strAutoDenomResult;

    CDeterministicMNCPtr mixingMasternode;
    CMutableTransaction txMyCollateral; // client side collateral
    CPendingDsaRequest pendingDsaRequest;

    CKeyHolderStorage keyHolderStorage; // storage for keys used in PrepareDenominate

    CWallet* mixingWallet;

    /// Create denominations
    bool CreateDenominated(CAmount nBalanceToDenominate);
    bool CreateDenominated(CAmount nBalanceToDenominate, const CompactTallyItem& tallyItem, bool fCreateMixingCollaterals);

    /// Split up large inputs or make fee sized inputs
    bool MakeCollateralAmounts();
    bool MakeCollateralAmounts(const CompactTallyItem& tallyItem, bool fTryDenominated);

    bool JoinExistingQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman);
    bool StartNewQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman);

    /// step 0: select denominated inputs and txouts
    bool SelectDenominate(std::string& strErrorRet, std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsRet);
    /// step 1: prepare denominated inputs and outputs
    bool PrepareDenominate(int nMinRounds, int nMaxRounds, std::string& strErrorRet, const std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsIn, std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsRet, bool fDryRun = false);
    /// step 2: send denominated inputs and outputs prepared in step 1
    bool SendDenominate(const std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsIn, CConnman& connman);

    /// Process Masternode updates about the progress of mixing
    void ProcessPoolStateUpdate(CPrivateSendStatusUpdate psssup);
    // Set the 'state' value, with some logging and capturing when the state changed
    void SetState(PoolState nStateNew);

    void CompletedTransaction(PoolMessage nMessageID);

    /// As a client, check and sign the final transaction
    bool SignFinalTransaction(const CTransaction& finalTransactionNew, CNode* pnode, CConnman& connman);

    void RelayIn(const CPrivateSendEntry& entry, CConnman& connman);

    void SetNull();

public:
    CPrivateSendClientSession(CWallet* pwallet) :
        vecOutPointLocked(),
        strLastMessage(),
        strAutoDenomResult(),
        mixingMasternode(),
        txMyCollateral(),
        pendingDsaRequest(),
        keyHolderStorage(),
        mixingWallet(pwallet)
    {
    }

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, bool enable_bip61);

    void UnlockCoins();

    void ResetPool();

    std::string GetStatus(bool fWaitForBlock);

    bool GetMixingMasternodeInfo(CDeterministicMNCPtr& ret) const;

    /// Passively run mixing in the background according to the configuration in settings
    bool DoAutomaticDenominating(CConnman& connman, bool fDryRun = false);

    /// As a client, submit part of a future mixing transaction to a Masternode to start the process
    bool SubmitDenominate(CConnman& connman);

    bool ProcessPendingDsaRequest(CConnman& connman);

    bool CheckTimeout();

    void GetJsonInfo(UniValue& obj) const;
};

/** Used to keep track of mixing queues
 */
class CPrivateSendClientQueueManager : public CPrivateSendBaseManager
{
public:
    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, bool enable_bip61);

    void DoMaintenance();
};

/** Used to keep track of current status of mixing pool
 */
class CPrivateSendClientManager
{
private:
    // Keep track of the used Masternodes
    std::vector<COutPoint> vecMasternodesUsed;

    // TODO: or map<denom, CPrivateSendClientSession> ??
    std::deque<CPrivateSendClientSession> deqSessions;
    mutable CCriticalSection cs_deqsessions;

    int nCachedLastSuccessBlock;
    int nMinBlocksToWait; // how many blocks to wait after one successful mixing tx in non-multisession mode
    std::string strAutoDenomResult;

    CWallet* mixingWallet;

    // Keep track of current block height
    int nCachedBlockHeight;

    bool WaitForAnotherBlock() const;

    // Make sure we have enough keys since last backup
    bool CheckAutomaticBackup();

public:
    int nCachedNumBlocks;    // used for the overview screen
    bool fCreateAutoBackups; // builtin support for automatic backups

    CPrivateSendClientManager() :
        vecMasternodesUsed(),
        deqSessions(),
        nCachedLastSuccessBlock(0),
        nMinBlocksToWait(1),
        strAutoDenomResult(),
        nCachedBlockHeight(0),
        nCachedNumBlocks(std::numeric_limits<int>::max()),
        fCreateAutoBackups(true),
        mixingWallet(nullptr)
    {
    }

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, bool enable_bip61);

    bool StartMixing(CWallet* pwallet);
    void StopMixing();
    bool IsMixing() const;
    void ResetPool();

    std::string GetStatuses();
    std::string GetSessionDenoms();

    bool GetMixingMasternodesInfo(std::vector<CDeterministicMNCPtr>& vecDmnsRet) const;

    /// Passively run mixing in the background according to the configuration in settings
    bool DoAutomaticDenominating(CConnman& connman, bool fDryRun = false);

    bool TrySubmitDenominate(const CService& mnAddr, CConnman& connman);
    bool MarkAlreadyJoinedQueueAsTried(CPrivateSendQueue& dsq) const;

    void CheckTimeout();

    void ProcessPendingDsaRequest(CConnman& connman);

    void AddUsedMasternode(const COutPoint& outpointMn);
    CDeterministicMNCPtr GetRandomNotUsedMasternode();

    void UpdatedSuccessBlock();

    void UpdatedBlockTip(const CBlockIndex* pindex);

    void DoMaintenance(CConnman& connman);

    void GetJsonInfo(UniValue& obj) const;
};

/* Application wide mixing options */
class CPrivateSendClientOptions
{
public:
    static int GetSessions() { return CPrivateSendClientOptions::Get().nPrivateSendSessions; }
    static int GetRounds() { return CPrivateSendClientOptions::Get().nPrivateSendRounds; }
    static int GetRandomRounds() { return CPrivateSendClientOptions::Get().nPrivateSendRandomRounds; }
    static int GetAmount() { return CPrivateSendClientOptions::Get().nPrivateSendAmount; }
    static int GetDenomsGoal() { return CPrivateSendClientOptions::Get().nPrivateSendDenomsGoal; }
    static int GetDenomsHardCap() { return CPrivateSendClientOptions::Get().nPrivateSendDenomsHardCap; }

    static void SetEnabled(bool fEnabled);
    static void SetMultiSessionEnabled(bool fEnabled);
    static void SetRounds(int nRounds);
    static void SetAmount(CAmount amount);

    static bool IsEnabled() { return CPrivateSendClientOptions::Get().fEnablePrivateSend; }
    static bool IsMultiSessionEnabled() { return CPrivateSendClientOptions::Get().fPrivateSendMultiSession; }

    static void GetJsonInfo(UniValue& obj);

private:
    static CPrivateSendClientOptions* _instance;
    static std::once_flag onceFlag;

    CCriticalSection cs_ps_options;
    int nPrivateSendSessions;
    int nPrivateSendRounds;
    int nPrivateSendRandomRounds;
    int nPrivateSendAmount;
    int nPrivateSendDenomsGoal;
    int nPrivateSendDenomsHardCap;
    bool fEnablePrivateSend;
    bool fPrivateSendMultiSession;

    CPrivateSendClientOptions() :
        nPrivateSendRounds(DEFAULT_PRIVATESEND_ROUNDS),
        nPrivateSendRandomRounds(PRIVATESEND_RANDOM_ROUNDS),
        nPrivateSendAmount(DEFAULT_PRIVATESEND_AMOUNT),
        nPrivateSendDenomsGoal(DEFAULT_PRIVATESEND_DENOMS_GOAL),
        nPrivateSendDenomsHardCap(DEFAULT_PRIVATESEND_DENOMS_HARDCAP),
        fEnablePrivateSend(false),
        fPrivateSendMultiSession(DEFAULT_PRIVATESEND_MULTISESSION)
    {
    }

    CPrivateSendClientOptions(const CPrivateSendClientOptions& other) = delete;
    CPrivateSendClientOptions& operator=(const CPrivateSendClientOptions&) = delete;

    static CPrivateSendClientOptions& Get();
    static void Init();
};

void DoPrivateSendMaintenance(CConnman& connman);

#endif // BITCOIN_PRIVATESEND_PRIVATESEND_CLIENT_H
