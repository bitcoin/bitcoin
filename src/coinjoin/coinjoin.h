// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_COINJOIN_H
#define BITCOIN_COINJOIN_COINJOIN_H

#include <coinjoin/common.h>

#include <core_io.h>
#include <evo/types.h>
#include <kernel/cs_main.h> // IWYU pragma: export
#include <netaddress.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <sync.h>
#include <timedata.h>
#include <util/helpers.h>
#include <util/translation.h>
#include <version.h>

#include <atomic>
#include <functional>
#include <map>
#include <optional>
#include <utility>

#include <univalue.h>

class Chainstate;
class CBLSPublicKey;
class CBlockIndex;
class CDeterministicMNList;
class ChainstateManager;
class CTxMemPool;

namespace chainlock {
class Chainlocks;
} // namespace chainlock

namespace llmq {
class CInstantSendManager;
} // namespace llmq

// timeouts
static constexpr int COINJOIN_AUTO_TIMEOUT_MIN = 5;
static constexpr int COINJOIN_AUTO_TIMEOUT_MAX = 15;
static constexpr int COINJOIN_QUEUE_TIMEOUT = 30;
static constexpr int COINJOIN_SIGNING_TIMEOUT = 15;
//! How long a successfully mixed input is kept locked while waiting for the finalized
//! mixing transaction before chain/mempool spentness is double-checked and the input is
//! potentially released, in seconds
static constexpr int64_t COINJOIN_PENDING_OBSERVATION_TIMEOUT = 60 * 60;

static constexpr size_t COINJOIN_ENTRY_MAX_SIZE = 9;

namespace CoinJoin {
/// Get the minimum/maximum number of participants for the pool
int GetMinPoolParticipants();
int GetMaxPoolParticipants();

/// Maximum number of inputs or outputs across a full pool
inline size_t GetMaxPoolInputOutputCount() { return static_cast<size_t>(GetMaxPoolParticipants()) * COINJOIN_ENTRY_MAX_SIZE; }
} // namespace CoinJoin

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

class CCoinJoinAccept
{
public:
    //! dsa flags (post-V24): which kind of rebalance entry this participant intends to submit.
    //! The direction matters to the masternode because a promotion only ever adds coins at the
    //! session denomination to the input side and a demotion only to the output side, and each
    //! side needs either nobody or at least two participants (see CoinJoin::MixSideCounts).
    static constexpr uint8_t FLAG_PROMOTION{1 << 0};
    static constexpr uint8_t FLAG_DEMOTION{1 << 1};

    int nDenom{0};
    CMutableTransaction txCollateral;
    //! Only serialized between peers at or above COINJOIN_REBALANCE_VERSION; old peers
    //! neither send nor receive it, so their wire format is unchanged
    uint8_t nFlags{0};

    CCoinJoinAccept() = default;

    CCoinJoinAccept(int nDenom, CMutableTransaction txCollateral, uint8_t nFlags = 0) :
        nDenom(nDenom),
        txCollateral(std::move(txCollateral)),
        nFlags(nFlags){};

    SERIALIZE_METHODS(CCoinJoinAccept, obj)
    {
        READWRITE(obj.nDenom, obj.txCollateral);
        if (s.GetVersion() >= COINJOIN_REBALANCE_VERSION) {
            READWRITE(obj.nFlags);
        }
    }

    [[nodiscard]] bool IsPromotion() const { return (nFlags & FLAG_PROMOTION) != 0; }
    [[nodiscard]] bool IsDemotion() const { return (nFlags & FLAG_DEMOTION) != 0; }
    [[nodiscard]] bool IsRebalance() const { return IsPromotion() || IsDemotion(); }
    //! A participant mixes in exactly one direction; anything else is malformed
    [[nodiscard]] bool HasValidFlags() const { return !(IsPromotion() && IsDemotion()); }

    friend bool operator==(const CCoinJoinAccept& a, const CCoinJoinAccept& b)
    {
        return a.nDenom == b.nDenom && a.nFlags == b.nFlags && CTransaction(a.txCollateral) == CTransaction(b.txCollateral);
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
        txCollateral(MakeTransactionRef(CMutableTransaction{}))
    {
    }

    CCoinJoinEntry(std::vector<CTxDSIn> vecTxDSIn, std::vector<CTxOut> vecTxOut, const CTransaction& txCollateral) :
            vecTxDSIn(std::move(vecTxDSIn)),
            vecTxOut(std::move(vecTxOut)),
            txCollateral(MakeTransactionRef(txCollateral))
    {
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << vecTxDSIn << txCollateral << vecTxOut;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        const size_t max_count{CoinJoin::GetMaxPoolInputOutputCount()};
        if (!UnserializeVectorWithMaxSize(s, vecTxDSIn, max_count)) {
            throw std::ios_base::failure("CCoinJoinEntry::vecTxDSIn size too large");
        }

        s >> txCollateral;

        if (!UnserializeVectorWithMaxSize(s, vecTxOut, max_count)) {
            throw std::ios_base::failure("CCoinJoinEntry::vecTxOut size too large");
        }
    }

    bool AddScriptSig(const CTxIn& txin);

    /// Which side(s) of the session denomination this entry occupies, derived from its shape.
    /// Empty and malformed entries are UNKNOWN; they occupy neither side and are rejected
    /// before they reach the pool.
    [[nodiscard]] CoinJoin::MixShape GetMixShape() const
    {
        using CoinJoin::MixShape;
        if (vecTxDSIn.empty() || vecTxOut.empty()) return MixShape::UNKNOWN;
        if (vecTxDSIn.size() == vecTxOut.size()) return MixShape::STANDARD;
        if (vecTxDSIn.size() == static_cast<size_t>(CoinJoin::PROMOTION_RATIO) && vecTxOut.size() == 1) return MixShape::PROMOTION;
        if (vecTxDSIn.size() == 1 && vecTxOut.size() == static_cast<size_t>(CoinJoin::PROMOTION_RATIO)) return MixShape::DEMOTION;
        return MixShape::UNKNOWN;
    }

    [[nodiscard]] bool IsStandardMixingEntry() const { return GetMixShape() == CoinJoin::MixShape::STANDARD; }
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

    CCoinJoinQueue(int nDenom, const COutPoint& outpoint, const uint256& proTxHash, NodeClock::time_point time, bool fReady) :
        nDenom(nDenom),
        masternodeOutpoint(outpoint),
        m_protxHash(proTxHash),
        nTime(TicksSinceEpoch<std::chrono::seconds>(time)),
        fReady(fReady)
    {
    }

    NodeSeconds Time() const { return NodeSeconds{std::chrono::seconds{nTime}}; }

    SERIALIZE_METHODS(CCoinJoinQueue, obj)
    {
        READWRITE(obj.nDenom, obj.m_protxHash, obj.nTime, obj.fReady);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
    }

    [[nodiscard]] uint256 GetHash() const;
    [[nodiscard]] uint256 GetSignatureHash() const;

    /// Check if we have a valid Masternode address
    [[nodiscard]] bool CheckSignature(const CBLSPublicKey& blsPubKey) const;

    /// Check if a queue is too old or too far into the future
    [[nodiscard]] bool IsTimeOutOfBounds(NodeSeconds current_time) const;
    [[nodiscard]] bool IsTimeOutOfBounds() const;

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
        tx(MakeTransactionRef(CMutableTransaction{}))
    {
    }

    CCoinJoinBroadcastTx(CTransactionRef _tx, const COutPoint& _outpoint, const uint256& proTxHash, NodeClock::time_point time) :
        tx(std::move(_tx)),
        masternodeOutpoint(_outpoint),
        m_protxHash(proTxHash),
        sigTime(TicksSinceEpoch<std::chrono::seconds>(time))
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

    [[nodiscard]] bool CheckSignature(const CBLSPublicKey& blsPubKey) const;

    [[nodiscard]] const std::optional<int>& GetConfirmedHeight() const { return nConfirmedHeight; }
    void SetConfirmedHeight(std::optional<int> nConfirmedHeightIn) { assert(nConfirmedHeightIn == std::nullopt || *nConfirmedHeightIn > 0); nConfirmedHeight = nConfirmedHeightIn; }
    [[nodiscard]] bool IsExpired(const CBlockIndex* pindex, const chainlock::Chainlocks& clhandler) const;
    /**
     * Trivial structural checks against the V24-dependent rules at pindex. When the tx fails
     * only because V24 is not active at pindex but would be structurally valid on a post-V24
     * tip, fPossiblyValidPostV24Ret (if provided) is set to true so callers can tolerate tip
     * skew around the activation boundary.
     */
    [[nodiscard]] bool IsValidStructure(const CBlockIndex* pindex, const ChainstateManager& chainman,
                                        bool* fPossiblyValidPostV24Ret = nullptr) const;
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

    virtual void SetNull() EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    /**
     * Validate inputs/outputs of a single mixing entry or, with fFinalTx=true, of the
     * aggregated final transaction. Post-V24 a final transaction concatenates standard (N:N),
     * promotion (10:1) and demotion (1:10) entries, so per-entry shape rules don't apply to it;
     * aggregate rules (allowed denominations, value balance, input/output consistency) are
     * checked instead. session_denom is the caller's snapshot of the session denomination.
     *
     * fAllowRebalanceShapes tells us whether promotion/demotion shapes are permitted here. It is
     * passed in rather than derived from the tip so that it stays fixed for the lifetime of a
     * session: the masternode passes the session's own capability and the client the
     * boundary-tolerant tip check, and neither can be flipped mid-session by a reorg across the
     * V24 boundary.
     */
    static bool IsValidInOuts(Chainstate& active_chainstate, const llmq::CInstantSendManager& isman,
                              const CTxMemPool& mempool, const std::vector<CTxIn>& vin, const std::vector<CTxOut>& vout,
                              int session_denom, bool fAllowRebalanceShapes, PoolMessage& nMessageIDRet,
                              bool* fConsumeCollateralRet, bool fFinalTx = false,
                              CoinJoin::SessionDenomCounts* pDenomCountsRet = nullptr);

public:
    // Atomic because the message-handling and scheduler threads write it while those threads and
    // RPC callers also read it without holding cs_coinjoin.
    std::atomic<int> nSessionDenom{0};

    CCoinJoinBaseSession() = default;
    virtual ~CCoinJoinBaseSession() = default;

    int GetState() const { return nState; }
    std::string GetStateString() const;

    int GetEntriesCount() const EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin) { LOCK(cs_coinjoin); return vecEntries.size(); }
    int GetEntriesCountLocked() const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin) { return vecEntries.size(); }

    /// Participants occupying each side of the session denomination among the entries received
    CoinJoin::MixSideCounts GetMixSideCounts() const EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin)
    {
        LOCK(cs_coinjoin);
        return GetMixSideCountsLocked();
    }
    CoinJoin::MixSideCounts GetMixSideCountsLocked() const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin)
    {
        CoinJoin::MixSideCounts counts;
        for (const auto& entry : vecEntries) {
            counts.Add(entry.GetMixShape());
        }
        return counts;
    }
};

class CoinJoinQueueManager
{
private:
    mutable Mutex cs_vecqueue;

    // The current mixing sessions in progress on the network
    std::vector<CCoinJoinQueue> vecCoinJoinQueue GUARDED_BY(cs_vecqueue);

public:
    void SetNull() EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);

    //! Remove timed-out queue entries. Call periodically (e.g. every second).
    void CheckQueue() EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);

    int GetQueueSize() const EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue) { LOCK(cs_vecqueue); return vecCoinJoinQueue.size(); }
    //! nDenomFilter != 0 restricts the search to that denomination. Queues it skips are left
    //! untried, so a rebalance attempt doesn't consume the announcements standard mixing needs.
    bool GetQueueItemAndTry(CCoinJoinQueue& dsqRet, int nDenomFilter = 0) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);

    bool HasQueue(const uint256& queueHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue)
    {
        LOCK(cs_vecqueue);
        return std::any_of(vecCoinJoinQueue.begin(), vecCoinJoinQueue.end(),
                           [&queueHash](auto q) { return q.GetHash() == queueHash; });
    }
    std::optional<CCoinJoinQueue> GetQueueFromHash(const uint256& queueHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue)
    {
        LOCK(cs_vecqueue);
        return util::find_if_opt(vecCoinJoinQueue, [&queueHash](const auto& q) { return q.GetHash() == queueHash; });
    }

    //! True if any queue entry matches the given masternode outpoint and readiness state.
    //! Used to detect when a masternode is broadcasting queues too quickly.
    bool HasQueueFromMasternode(const COutPoint& outpoint, bool fReady) const EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue)
    {
        LOCK(cs_vecqueue);
        return std::any_of(vecCoinJoinQueue.begin(), vecCoinJoinQueue.end(),
                           [&](const auto& q) { return q.masternodeOutpoint == outpoint && q.fReady == fReady; });
    }
    //! TRY_LOCK variant: returns nullopt if lock can't be acquired; true if any queue entry has this
    //! outpoint (any readiness).
    [[nodiscard]] std::optional<bool> TryHasQueueFromMasternode(const COutPoint& outpoint) const EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);
    //! TRY_LOCK combined duplicate check: returns nullopt if lock can't be acquired; true if dsq is
    //! an exact duplicate or the masternode is sending too many dsqs with the same readiness.
    [[nodiscard]] std::optional<bool> TryCheckDuplicate(const CCoinJoinQueue& dsq) const EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);

    //! Append a queue entry (caller must have already checked for duplicates).
    void AddQueue(CCoinJoinQueue dsq) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue)
    {
        LOCK(cs_vecqueue);
        vecCoinJoinQueue.push_back(std::move(dsq));
    }
    //! TRY_LOCK variant of AddQueue: returns false if the lock cannot be acquired.
    bool TryAddQueue(CCoinJoinQueue dsq) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);
};

// Various helpers and dstx manager implementation
namespace CoinJoin
{
    bilingual_str GetMessageByID(PoolMessage nMessageID);

    constexpr CAmount GetMaxPoolAmount() { return COINJOIN_ENTRY_MAX_SIZE * vecStandardDenominations.front(); }

    /// Whether denomination promotion/demotion (post-V24) is active at the current chain tip.
    /// With fNextBlock, also counts a deployment that activates in the block following the tip:
    /// the peer that sent us a rebalance-shaped transaction may be one block ahead of us at the
    /// activation boundary, and treating its transaction as malformed would cost us either a
    /// discouraged peer or, when signing, our collateral.
    bool IsPromotionDemotionActive(const ChainstateManager& chainman, bool fNextBlock = false);

    /// If the collateral is valid given by a client
    bool IsCollateralValid(ChainstateManager& chainman, const llmq::CInstantSendManager& isman,
                           const CTxMemPool& mempool, const CTransaction& txCollateral);

    /**
     * Validate a promotion entry: 10 inputs of smaller denom → 1 output of larger denom
     * @param vecTxIn The inputs for this entry
     * @param vecTxOut The outputs for this entry
     * @param nSessionDenom The session denomination (the smaller denom for promotion)
     * @param nMessageIDRet Error message if validation fails
     * @return true if valid promotion entry
     */
    bool ValidatePromotionEntry(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut,
                                int nSessionDenom, PoolMessage& nMessageIDRet);

    /**
     * Validate a demotion entry: 1 input of larger denom → 10 outputs of smaller denom
     * @param vecTxIn The inputs for this entry
     * @param vecTxOut The outputs for this entry
     * @param nSessionDenom The session denomination (the smaller denom for demotion outputs)
     * @param nMessageIDRet Error message if validation fails
     * @return true if valid demotion entry
     */
    bool ValidateDemotionEntry(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut,
                               int nSessionDenom, PoolMessage& nMessageIDRet);

    /**
     * Check that a final transaction's aggregate input/output counts are composable from valid
     * standard (N:N), promotion (PROMOTION_RATIO:1) and demotion (1:PROMOTION_RATIO) entries.
     * Inputs/outputs at the larger adjacent denomination are counted separately from the
     * session-denomination remainder; value balance is checked by the caller.
     * @param nTotalInputs Total number of inputs in the final tx
     * @param nTotalOutputs Total number of outputs in the final tx
     * @param nLargerInputs Number of inputs at the larger adjacent denomination (demotions)
     * @param nLargerOutputs Number of outputs at the larger adjacent denomination (promotions)
     * @return true if the counts are consistent with a mix of valid entries
     */
    bool ValidateFinalTxComposition(size_t nTotalInputs, size_t nTotalOutputs,
                                    size_t nLargerInputs, size_t nLargerOutputs);

    struct DSQueueValidation {
        //! The masternode that signed the queue, null if the queue was rejected
        CDeterministicMNCPtr dmn{};
        //! Score to penalize the sending peer with, 0 if the queue was accepted or the rejection
        //! is not the peer's fault
        int misbehavior{0};
    };

    /**
     * Runs the DSQUEUE checks common to mixing clients and servers and fills in the queue's
     * masternodeOutpoint from the masternode list.
     * @param is_duplicate Called once the outpoint is known and before the signature is verified,
     *                     so duplicates are dropped without a BLS check. Returning true rejects
     *                     the queue without a penalty.
     */
    DSQueueValidation ValidateDSQueue(CCoinJoinQueue& dsq, const CDeterministicMNList& mn_list,
                                      const std::function<bool(const CCoinJoinQueue&)>& is_duplicate);
}

class CDSTXManager
{
    const chainlock::Chainlocks& m_chainlocks;
    Mutex cs_mapdstx;
    std::map<uint256, CCoinJoinBroadcastTx> mapDSTX GUARDED_BY(cs_mapdstx);

public:
    CDSTXManager(const CDSTXManager&) = delete;
    CDSTXManager& operator=(const CDSTXManager&) = delete;
    explicit CDSTXManager(const chainlock::Chainlocks& chainlocks);
    ~CDSTXManager();

    void AddDSTX(const CCoinJoinBroadcastTx& dstx) EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);
    CCoinJoinBroadcastTx GetDSTX(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);

    // CDSNotificationInterface
    void UpdatedBlockTip(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);
    void NotifyChainLock(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);
    void TransactionAddedToMempool(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex*)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);

private:
    bool IsTxExpired(const CCoinJoinBroadcastTx& tx, const CBlockIndex* pindex) const EXCLUSIVE_LOCKS_REQUIRED(cs_mapdstx);
    void CheckDSTXes(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_mapdstx);
    void UpdateDSTXConfirmedHeight(const CTransactionRef& tx, std::optional<int> nHeight)
        EXCLUSIVE_LOCKS_REQUIRED(cs_mapdstx);
};

bool ATMPIfSaneFee(ChainstateManager& chainman, const CTransactionRef& tx, bool test_accept = false)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

#endif // BITCOIN_COINJOIN_COINJOIN_H
