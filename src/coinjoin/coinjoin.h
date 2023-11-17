// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_COINJOIN_H
#define BITCOIN_COINJOIN_COINJOIN_H

#include <core_io.h>
#include <netaddress.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <timedata.h>
#include <univalue.h>
#include <util/ranges.h>
#include <util/translation.h>
#include <version.h>

#include <atomic>
#include <optional>
#include <utility>

class CCoinJoin;
class CConnman;
class CBLSPublicKey;
class CBlockIndex;
class CMasternodeSync;
class CTxMemPool;

namespace llmq {
class CChainLocksHandler;
} // namespace llmq

// timeouts
static constexpr int COINJOIN_AUTO_TIMEOUT_MIN = 5;
static constexpr int COINJOIN_AUTO_TIMEOUT_MAX = 15;
static constexpr int COINJOIN_QUEUE_TIMEOUT = 30;
static constexpr int COINJOIN_SIGNING_TIMEOUT = 15;

static constexpr size_t COINJOIN_ENTRY_MAX_SIZE = 9;

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
    int nSessionID{0};
    PoolState nState{POOL_STATE_IDLE};
    int nEntriesCount{0}; // deprecated, kept for backwards compatibility
    PoolStatusUpdate nStatusUpdate{STATUS_ACCEPTED};
    PoolMessage nMessageID{MSG_NOERR};

    constexpr CCoinJoinStatusUpdate() = default;

    constexpr CCoinJoinStatusUpdate(int nSessionID, PoolState nState, int nEntriesCount, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID) :
        nSessionID(nSessionID),
        nState(nState),
        nEntriesCount(nEntriesCount),
        nStatusUpdate(nStatusUpdate),
        nMessageID(nMessageID) {};

    SERIALIZE_METHODS(CCoinJoinStatusUpdate, obj)
    {
        READWRITE(obj.nSessionID, obj.nState, obj.nStatusUpdate, obj.nMessageID);
    }
};

/** Holds a mixing input
 */
class CTxDSIn : public CTxIn
{
public:
    // memory only
    CScript prevPubKey;
    bool fHasSig{false}; // flag to indicate if signed
    int nRounds{-10};

    CTxDSIn(const CTxIn& txin, CScript script, int nRounds) :
        CTxIn(txin),
        prevPubKey(std::move(script)),
        nRounds(nRounds)
    {
    }

    CTxDSIn() = default;
};

class CCoinJoinAccept
{
public:
    int nDenom{0};
    CMutableTransaction txCollateral;

    CCoinJoinAccept() = default;

    CCoinJoinAccept(int nDenom, CMutableTransaction txCollateral) :
        nDenom(nDenom),
        txCollateral(std::move(txCollateral)){};

    SERIALIZE_METHODS(CCoinJoinAccept, obj)
    {
        READWRITE(obj.nDenom, obj.txCollateral);
    }

    friend bool operator==(const CCoinJoinAccept& a, const CCoinJoinAccept& b)
    {
        return a.nDenom == b.nDenom && CTransaction(a.txCollateral) == CTransaction(b.txCollateral);
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
        txCollateral(MakeTransactionRef())
    {
    }

    CCoinJoinEntry(std::vector<CTxDSIn> vecTxDSIn, std::vector<CTxOut> vecTxOut, const CTransaction& txCollateral) :
            vecTxDSIn(std::move(vecTxDSIn)),
            vecTxOut(std::move(vecTxOut)),
            txCollateral(MakeTransactionRef(txCollateral))
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
    int nDenom{0};
    COutPoint masternodeOutpoint;
    uint256 m_protxHash;
    int64_t nTime{0};
    bool fReady{false}; //ready for submit
    std::vector<unsigned char> vchSig;
    // memory only
    bool fTried{false};

    CCoinJoinQueue() = default;

    CCoinJoinQueue(int nDenom, const COutPoint& outpoint, const uint256& proTxHash, int64_t nTime, bool fReady) :
        nDenom(nDenom),
        masternodeOutpoint(outpoint),
        m_protxHash(proTxHash),
        nTime(nTime),
        fReady(fReady)
    {
    }

    SERIALIZE_METHODS(CCoinJoinQueue, obj)
    {
        READWRITE(obj.nDenom, obj.m_protxHash, obj.nTime, obj.fReady);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
    }

    [[nodiscard]] uint256 GetSignatureHash() const;
    /** Sign this mixing transaction
     *  return true if all conditions are met:
     *     1) we have an active Masternode,
     *     2) we have a valid Masternode private key,
     *     3) we signed the message successfully, and
     *     4) we verified the message successfully
     */
    bool Sign();
    /// Check if we have a valid Masternode address
    [[nodiscard]] bool CheckSignature(const CBLSPublicKey& blsPubKey) const;

    bool Relay(CConnman& connman);

    /// Check if a queue is too old or too far into the future
    [[nodiscard]] bool IsTimeOutOfBounds(int64_t current_time = GetAdjustedTime()) const;

    [[nodiscard]] std::string ToString() const;

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
    // when corresponding tx is 0-confirmed or conflicted, nConfirmedHeight is std::nullopt
    std::optional<int> nConfirmedHeight{std::nullopt};

public:
    CTransactionRef tx;
    COutPoint masternodeOutpoint;
    uint256 m_protxHash;
    std::vector<unsigned char> vchSig;
    int64_t sigTime{0};

    CCoinJoinBroadcastTx() :
        tx(MakeTransactionRef())
    {
    }

    CCoinJoinBroadcastTx(CTransactionRef _tx, const COutPoint& _outpoint, const uint256& proTxHash, int64_t _sigTime) :
        tx(std::move(_tx)),
        masternodeOutpoint(_outpoint),
        m_protxHash(proTxHash),
        sigTime(_sigTime)
    {
    }

    SERIALIZE_METHODS(CCoinJoinBroadcastTx, obj)
    {
        READWRITE(obj.tx, obj.m_protxHash);

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

    [[nodiscard]] uint256 GetSignatureHash() const;

    bool Sign();
    [[nodiscard]] bool CheckSignature(const CBLSPublicKey& blsPubKey) const;

    void SetConfirmedHeight(std::optional<int> nConfirmedHeightIn) { assert(nConfirmedHeightIn == std::nullopt || *nConfirmedHeightIn > 0); nConfirmedHeight = nConfirmedHeightIn; }
    bool IsExpired(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler) const;
    [[nodiscard]] bool IsValidStructure() const;
};

// base class
class CCoinJoinBaseSession
{
protected:
    mutable Mutex cs_coinjoin;

    std::vector<CCoinJoinEntry> vecEntries GUARDED_BY(cs_coinjoin); // Masternode/clients entries

    std::atomic<PoolState> nState{POOL_STATE_IDLE}; // should be one of the POOL_STATE_XXX values
    std::atomic<int64_t> nTimeLastSuccessfulStep{0}; // the time when last successful mixing step was performed

    std::atomic<int> nSessionID{0}; // 0 if no mixing session is active

    CMutableTransaction finalMutableTransaction GUARDED_BY(cs_coinjoin); // the finalized transaction ready for signing

    void SetNull() EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    bool IsValidInOuts(const CTxMemPool& mempool, const std::vector<CTxIn>& vin, const std::vector<CTxOut>& vout, PoolMessage& nMessageIDRet, bool* fConsumeCollateralRet) const;

public:
    int nSessionDenom{0}; // Users must submit a denom matching this

    CCoinJoinBaseSession() = default;

    int GetState() const { return nState; }
    std::string GetStateString() const;

    int GetEntriesCount() const LOCKS_EXCLUDED(cs_coinjoin) { LOCK(cs_coinjoin); return vecEntries.size(); }
    int GetEntriesCountLocked() const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin) { return vecEntries.size(); }
};

// base class
class CCoinJoinBaseManager
{
protected:
    mutable Mutex cs_vecqueue;

    // The current mixing sessions in progress on the network
    std::vector<CCoinJoinQueue> vecCoinJoinQueue GUARDED_BY(cs_vecqueue);

    void SetNull() LOCKS_EXCLUDED(cs_vecqueue);
    void CheckQueue() LOCKS_EXCLUDED(cs_vecqueue);

public:
    CCoinJoinBaseManager() = default;

    int GetQueueSize() const LOCKS_EXCLUDED(cs_vecqueue) { LOCK(cs_vecqueue); return vecCoinJoinQueue.size(); }
    bool GetQueueItemAndTry(CCoinJoinQueue& dsqRet) LOCKS_EXCLUDED(cs_vecqueue);
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
    static constexpr std::array<CAmount, 5> vecStandardDenominations{
            (10 * COIN) + 10000,
            (1 * COIN) + 1000,
            (COIN / 10) + 100,
            (COIN / 100) + 10,
            (COIN / 1000) + 1,
    };

    static Mutex cs_mapdstx;
    static std::map<uint256, CCoinJoinBroadcastTx> mapDSTX GUARDED_BY(cs_mapdstx);

    static void CheckDSTXes(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler) LOCKS_EXCLUDED(cs_mapdstx);

public:
    static constexpr std::array<CAmount, 5> GetStandardDenominations() { return vecStandardDenominations; }
    static constexpr CAmount GetSmallestDenomination() { return vecStandardDenominations.back(); }

    static constexpr bool IsDenominatedAmount(CAmount nInputAmount) { return AmountToDenomination(nInputAmount) > 0; }
    static constexpr bool IsValidDenomination(int nDenom) { return DenominationToAmount(nDenom) > 0; }

    /*
        Return a bitshifted integer representing a denomination in vecStandardDenominations
        or 0 if none was found
    */
    static constexpr int AmountToDenomination(CAmount nInputAmount)
    {
        for (size_t i = 0; i < vecStandardDenominations.size(); ++i) {
            if (nInputAmount == vecStandardDenominations[i]) {
                return 1 << i;
            }
        }
        return 0;
    }

    /*
        Returns:
        - one of standard denominations from vecStandardDenominations based on the provided bitshifted integer
        - 0 for non-initialized sessions (nDenom = 0)
        - a value below 0 if an error occurred while converting from one to another
    */
    static constexpr CAmount DenominationToAmount(int nDenom)
    {
        if (nDenom == 0) {
            // not initialized
            return 0;
        }

        size_t nMaxDenoms = vecStandardDenominations.size();

        if (nDenom >= (1 << nMaxDenoms) || nDenom < 0) {
            // out of bounds
            return -1;
        }

        if ((nDenom & (nDenom - 1)) != 0) {
            // non-denom
            return -2;
        }

        CAmount nDenomAmount{-3};

        for (size_t i = 0; i < nMaxDenoms; ++i) {
            if (nDenom & (1 << i)) {
                nDenomAmount = vecStandardDenominations[i];
                break;
            }
        }

        return nDenomAmount;
    }

    /*
    Same as DenominationToAmount but returns a string representation
    */
    static std::string DenominationToString(int nDenom);

    static bilingual_str GetMessageByID(PoolMessage nMessageID);

    /// Get the minimum/maximum number of participants for the pool
    static int GetMinPoolParticipants();
    static int GetMaxPoolParticipants();

    static constexpr CAmount GetMaxPoolAmount() { return COINJOIN_ENTRY_MAX_SIZE * vecStandardDenominations.front(); }

    /// If the collateral is valid given by a client
    static bool IsCollateralValid(CTxMemPool& mempool, const CTransaction& txCollateral);
    static constexpr CAmount GetCollateralAmount() { return GetSmallestDenomination() / 10; }
    static constexpr CAmount GetMaxCollateralAmount() { return GetCollateralAmount() * 4; }

    static constexpr bool IsCollateralAmount(CAmount nInputAmount)
    {
        // collateral input can be anything between 1x and "max" (including both)
        return (nInputAmount >= GetCollateralAmount() && nInputAmount <= GetMaxCollateralAmount());
    }

    static constexpr int CalculateAmountPriority(CAmount nInputAmount)
    {
        if (auto optDenom = ranges::find_if_opt(GetStandardDenominations(), [&nInputAmount](const auto& denom) {
            return nInputAmount == denom;
        })) {
            return (float)COIN / *optDenom * 10000;
        }
        if (nInputAmount < COIN) {
            return 20000;
        }

        //nondenom return largest first
        return -1 * (nInputAmount / COIN);
    }


    static void AddDSTX(const CCoinJoinBroadcastTx& dstx) LOCKS_EXCLUDED(cs_mapdstx);
    static CCoinJoinBroadcastTx GetDSTX(const uint256& hash) LOCKS_EXCLUDED(cs_mapdstx);

    static void UpdatedBlockTip(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler, const CMasternodeSync& mn_sync);
    static void NotifyChainLock(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler, const CMasternodeSync& mn_sync);

    static void TransactionAddedToMempool(const CTransactionRef& tx) LOCKS_EXCLUDED(cs_mapdstx);
    static void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex) LOCKS_EXCLUDED(cs_mapdstx);
    static void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex*) LOCKS_EXCLUDED(cs_mapdstx);
private:
    static void UpdateDSTXConfirmedHeight(const CTransactionRef& tx, std::optional<int> nHeight);
};

#endif // BITCOIN_COINJOIN_COINJOIN_H
