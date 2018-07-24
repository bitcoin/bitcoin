// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_COINJOIN_H
#define BITCOIN_COINJOIN_COINJOIN_H

#include <chainparams.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <timedata.h>
#include <tinyformat.h>

class CCoinJoin;
class CConnman;
class CBLSPublicKey;
class CBlockIndex;

// timeouts
static const int COINJOIN_AUTO_TIMEOUT_MIN = 5;
static const int COINJOIN_AUTO_TIMEOUT_MAX = 15;
static const int COINJOIN_QUEUE_TIMEOUT = 30;
static const int COINJOIN_SIGNING_TIMEOUT = 15;

static const size_t COINJOIN_ENTRY_MAX_SIZE = 9;

// pool responses
enum PoolMessage : int32_t {
    ERR_ALREADY_HAVE,
    ERR_DENOM,
    ERR_ENTRIES_FULL,
    ERR_EXISTING_TX,
    ERR_FEES,
    ERR_INVALID_COLLATERAL,
    ERR_INVALID_INPUT,
    ERR_INVALID_SCRIPT,
    ERR_INVALID_TX,
    ERR_MAXIMUM,
    ERR_MN_LIST,
    ERR_MODE,
    ERR_NON_STANDARD_PUBKEY, // not used
    ERR_NOT_A_MN, // not used
    ERR_QUEUE_FULL,
    ERR_RECENT,
    ERR_SESSION,
    ERR_MISSING_TX,
    ERR_VERSION,
    MSG_NOERR,
    MSG_SUCCESS,
    MSG_ENTRIES_ADDED,
    ERR_SIZE_MISMATCH,
    MSG_POOL_MIN = ERR_ALREADY_HAVE,
    MSG_POOL_MAX = ERR_SIZE_MISMATCH
};
template<> struct is_serializable_enum<PoolMessage> : std::true_type {};

// pool states
enum PoolState : int32_t {
    POOL_STATE_IDLE,
    POOL_STATE_QUEUE,
    POOL_STATE_ACCEPTING_ENTRIES,
    POOL_STATE_SIGNING,
    POOL_STATE_ERROR,
    POOL_STATE_MIN = POOL_STATE_IDLE,
    POOL_STATE_MAX = POOL_STATE_ERROR
};
template<> struct is_serializable_enum<PoolState> : std::true_type {};

// status update message constants
enum PoolStatusUpdate : int32_t {
    STATUS_REJECTED,
    STATUS_ACCEPTED
};
template<> struct is_serializable_enum<PoolStatusUpdate> : std::true_type {};

class CCoinJoinStatusUpdate
{
public:
    int nSessionID;
    PoolState nState;
    int nEntriesCount; // deprecated, kept for backwards compatibility
    PoolStatusUpdate nStatusUpdate;
    PoolMessage nMessageID;

    CCoinJoinStatusUpdate() :
        nSessionID(0),
        nState(POOL_STATE_IDLE),
        nEntriesCount(0),
        nStatusUpdate(STATUS_ACCEPTED),
        nMessageID(MSG_NOERR) {};

    CCoinJoinStatusUpdate(int nSessionID, PoolState nState, int nEntriesCount, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID) :
        nSessionID(nSessionID),
        nState(nState),
        nEntriesCount(nEntriesCount),
        nStatusUpdate(nStatusUpdate),
        nMessageID(nMessageID) {};

    SERIALIZE_METHODS(CCoinJoinStatusUpdate, obj)
    {
        READWRITE(obj.nSessionID, obj.nState);
        if (s.GetVersion() <= 702015) {
            READWRITE(obj.nEntriesCount);
        }
        READWRITE(obj.nStatusUpdate, obj.nMessageID);
    }
};

/** Holds a mixing input
 */
class CTxDSIn : public CTxIn
{
public:
    // memory only
    CScript prevPubKey;
    bool fHasSig; // flag to indicate if signed
    int nRounds;

    CTxDSIn(const CTxIn& txin, const CScript& script, int nRounds) :
        CTxIn(txin),
        prevPubKey(script),
        fHasSig(false),
        nRounds(nRounds)
    {
    }

    CTxDSIn() :
        CTxIn(),
        prevPubKey(),
        fHasSig(false),
        nRounds(-10)
    {
    }
};

class CCoinJoinAccept
{
public:
    int nDenom;
    CMutableTransaction txCollateral;

    CCoinJoinAccept() :
        nDenom(0),
        txCollateral(CMutableTransaction()){};

    CCoinJoinAccept(int nDenom, const CMutableTransaction& txCollateral) :
        nDenom(nDenom),
        txCollateral(txCollateral){};

    SERIALIZE_METHODS(CCoinJoinAccept, obj)
    {
        READWRITE(obj.nDenom, obj.txCollateral);
    }

    friend bool operator==(const CCoinJoinAccept& a, const CCoinJoinAccept& b)
    {
        return a.nDenom == b.nDenom && a.txCollateral == b.txCollateral;
    }
};

// A client's transaction in the mixing pool
class CCoinJoinEntry
{
public:
    std::vector<CTxDSIn> vecTxDSIn;
    std::vector<CTxOut> vecTxOut;
    CTransactionRef txCollateral;
    // memory only
    CService addr;

    CCoinJoinEntry() :
        vecTxDSIn(std::vector<CTxDSIn>()),
        vecTxOut(std::vector<CTxOut>()),
        txCollateral(MakeTransactionRef()),
        addr(CService())
    {
    }

    CCoinJoinEntry(const std::vector<CTxDSIn>& vecTxDSIn, const std::vector<CTxOut>& vecTxOut, const CTransaction& txCollateral) :
            vecTxDSIn(vecTxDSIn),
            vecTxOut(vecTxOut),
            txCollateral(MakeTransactionRef(txCollateral)),
            addr(CService())
    {
    }

    SERIALIZE_METHODS(CCoinJoinEntry, obj)
    {
        READWRITE(obj.vecTxDSIn, obj.txCollateral, obj.vecTxOut);
    }

    bool AddScriptSig(const CTxIn& txin);
};


/**
 * A currently in progress mixing merge and denomination information
 */
class CCoinJoinQueue
{
public:
    int nDenom;
    COutPoint masternodeOutpoint;
    int64_t nTime;
    bool fReady; //ready for submit
    std::vector<unsigned char> vchSig;
    // memory only
    bool fTried;

    CCoinJoinQueue() :
        nDenom(0),
        masternodeOutpoint(COutPoint()),
        nTime(0),
        fReady(false),
        vchSig(std::vector<unsigned char>()),
        fTried(false)
    {
    }

    CCoinJoinQueue(int nDenom, COutPoint outpoint, int64_t nTime, bool fReady) :
        nDenom(nDenom),
        masternodeOutpoint(outpoint),
        nTime(nTime),
        fReady(fReady),
        vchSig(std::vector<unsigned char>()),
        fTried(false)
    {
    }

    SERIALIZE_METHODS(CCoinJoinQueue, obj)
    {
        READWRITE(obj.nDenom, obj.masternodeOutpoint, obj.nTime, obj.fReady);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
    }

    uint256 GetSignatureHash() const;
    /** Sign this mixing transaction
     *  \return true if all conditions are met:
     *     1) we have an active Masternode,
     *     2) we have a valid Masternode private key,
     *     3) we signed the message successfully, and
     *     4) we verified the message successfully
     */
    bool Sign();
    /// Check if we have a valid Masternode address
    bool CheckSignature(const CBLSPublicKey& blsPubKey) const;

    bool Relay(CConnman& connman);

    /// Check if a queue is too old or too far into the future
    bool IsTimeOutOfBounds() const;

    std::string ToString() const
    {
        return strprintf("nDenom=%d, nTime=%lld, fReady=%s, fTried=%s, masternode=%s",
            nDenom, nTime, fReady ? "true" : "false", fTried ? "true" : "false", masternodeOutpoint.ToStringShort());
    }

    friend bool operator==(const CCoinJoinQueue& a, const CCoinJoinQueue& b)
    {
        return a.nDenom == b.nDenom && a.masternodeOutpoint == b.masternodeOutpoint && a.nTime == b.nTime && a.fReady == b.fReady;
    }
};

/** Helper class to store mixing transaction (tx) information.
 */
class CCoinJoinBroadcastTx
{
private:
    // memory only
    // when corresponding tx is 0-confirmed or conflicted, nConfirmedHeight is -1
    int nConfirmedHeight;

public:
    CTransactionRef tx;
    COutPoint masternodeOutpoint;
    std::vector<unsigned char> vchSig;
    int64_t sigTime;

    CCoinJoinBroadcastTx() :
        nConfirmedHeight(-1),
        tx(MakeTransactionRef()),
        masternodeOutpoint(),
        vchSig(),
        sigTime(0)
    {
    }

    CCoinJoinBroadcastTx(const CTransactionRef& _tx, COutPoint _outpoint, int64_t _sigTime) :
        nConfirmedHeight(-1),
        tx(_tx),
        masternodeOutpoint(_outpoint),
        vchSig(),
        sigTime(_sigTime)
    {
    }

    SERIALIZE_METHODS(CCoinJoinBroadcastTx, obj)
    {
        READWRITE(obj.tx, obj.masternodeOutpoint);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
        READWRITE(obj.sigTime);
    }

    friend bool operator==(const CCoinJoinBroadcastTx& a, const CCoinJoinBroadcastTx& b)
    {
        return *a.tx == *b.tx;
    }
    friend bool operator!=(const CCoinJoinBroadcastTx& a, const CCoinJoinBroadcastTx& b)
    {
        return !(a == b);
    }
    explicit operator bool() const
    {
        return *this != CCoinJoinBroadcastTx();
    }

    uint256 GetSignatureHash() const;

    bool Sign();
    bool CheckSignature(const CBLSPublicKey& blsPubKey) const;

    void SetConfirmedHeight(int nConfirmedHeightIn) { nConfirmedHeight = nConfirmedHeightIn; }
    bool IsExpired(const CBlockIndex* pindex) const;
    bool IsValidStructure();
};

// base class
class CCoinJoinBaseSession
{
protected:
    mutable CCriticalSection cs_coinjoin;

    std::vector<CCoinJoinEntry> vecEntries; // Masternode/clients entries

    PoolState nState;                // should be one of the POOL_STATE_XXX values
    int64_t nTimeLastSuccessfulStep; // the time when last successful mixing step was performed

    int nSessionID; // 0 if no mixing session is active

    CMutableTransaction finalMutableTransaction; // the finalized transaction ready for signing

    void SetNull();

    bool IsValidInOuts(const std::vector<CTxIn>& vin, const std::vector<CTxOut>& vout, PoolMessage& nMessageIDRet, bool* fConsumeCollateralRet) const;

public:
    int nSessionDenom; // Users must submit a denom matching this

    CCoinJoinBaseSession() :
        vecEntries(),
        nState(POOL_STATE_IDLE),
        nTimeLastSuccessfulStep(0),
        nSessionID(0),
        finalMutableTransaction(),
        nSessionDenom(0)
    {
    }

    int GetState() const { return nState; }
    std::string GetStateString() const;

    int GetEntriesCount() const { return vecEntries.size(); }
};

// base class
class CCoinJoinBaseManager
{
protected:
    mutable CCriticalSection cs_vecqueue;

    // The current mixing sessions in progress on the network
    std::vector<CCoinJoinQueue> vecCoinJoinQueue;

    void SetNull();
    void CheckQueue();

public:
    CCoinJoinBaseManager() :
        vecCoinJoinQueue() {}

    int GetQueueSize() const { return vecCoinJoinQueue.size(); }
    bool GetQueueItemAndTry(CCoinJoinQueue& dsqRet);
};

// helper class
class CCoinJoin
{
private:
    // make constructor, destructor and copying not available
    CCoinJoin() = default;
    ~CCoinJoin() = default;
    CCoinJoin(CCoinJoin const&) = delete;
    CCoinJoin& operator=(CCoinJoin const&) = delete;

    // static members
    static std::vector<CAmount> vecStandardDenominations;
    static std::map<uint256, CCoinJoinBroadcastTx> mapDSTX;

    static CCriticalSection cs_mapdstx;

    static void CheckDSTXes(const CBlockIndex* pindex);

public:
    static void InitStandardDenominations();
    static std::vector<CAmount> GetStandardDenominations() { return vecStandardDenominations; }
    static CAmount GetSmallestDenomination() { return vecStandardDenominations.back(); }

    static bool IsDenominatedAmount(CAmount nInputAmount);
    static bool IsValidDenomination(int nDenom);

    static int AmountToDenomination(CAmount nInputAmount);
    static CAmount DenominationToAmount(int nDenom);
    static std::string DenominationToString(int nDenom);

    static std::string GetMessageByID(PoolMessage nMessageID);

    /// Get the minimum/maximum number of participants for the pool
    static int GetMinPoolParticipants() { return Params().PoolMinParticipants(); }
    static int GetMaxPoolParticipants() { return Params().PoolMaxParticipants(); }

    static CAmount GetMaxPoolAmount() { return vecStandardDenominations.empty() ? 0 : COINJOIN_ENTRY_MAX_SIZE * vecStandardDenominations.front(); }

    /// If the collateral is valid given by a client
    static bool IsCollateralValid(const CTransaction& txCollateral);
    static CAmount GetCollateralAmount() { return GetSmallestDenomination() / 10; }
    static CAmount GetMaxCollateralAmount() { return GetCollateralAmount() * 4; }

    static bool IsCollateralAmount(CAmount nInputAmount);
    static int CalculateAmountPriority(CAmount nInputAmount);

    static void AddDSTX(const CCoinJoinBroadcastTx& dstx);
    static CCoinJoinBroadcastTx GetDSTX(const uint256& hash);

    static void UpdatedBlockTip(const CBlockIndex* pindex);
    static void NotifyChainLock(const CBlockIndex* pindex);

    static void UpdateDSTXConfirmedHeight(const CTransactionRef& tx, int nHeight);
    static void TransactionAddedToMempool(const CTransactionRef& tx);
    static void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted);
    static void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected);

};

#endif // BITCOIN_COINJOIN_COINJOIN_H
