// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_SERVER_H
#define BITCOIN_COINJOIN_SERVER_H

#include <coinjoin/coinjoin.h>

#include <net_processing.h>
#include <net_types.h>
#include <protocol.h>
#include <util/hasher.h>

#include <map>
#include <optional>
#include <unordered_set>

class CActiveMasternodeManager;
class CConnman;
class CDataStream;
class CDeterministicMNManager;
class CDSTXManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CNode;
class CTxMemPool;

class UniValue;

/** What the server's timeout check reads from a mixing session to decide whether to reset it */
struct CoinJoinSessionProgress {
    PoolState state{POOL_STATE_IDLE};
    bool timed_out{false};
    //! The queue has enough collaterals covering both sides to start accepting entries
    bool queue_ready{false};
    size_t entries{0};
    size_t collaterals{0};
    //! The sides of the session denomination occupied by the submitted entries
    CoinJoin::MixSideCounts sides{};
    bool signatures_complete{false};
};

/**
 * Should a session be reset for timing out? Not while it can still make the transition that
 * CheckForCompleteQueue() or CheckPool() would make for it: start accepting entries, finalize
 * (or, with an entry for every collateral but a side uncovered, reset with the participants
 * told), or commit.
 */
bool ShouldResetTimedOutSession(const CoinJoinSessionProgress& session);

/** Used to keep track of current status of mixing pool
 */
class CCoinJoinServer : public CCoinJoinBaseSession, public NetHandler
{
private:
    CoinJoinQueueManager m_queueman;

    ChainstateManager& m_chainman;
    CConnman& connman;
    CDeterministicMNManager& m_dmnman;
    CDSTXManager& m_dstxman;
    CMasternodeMetaMan& m_mn_metaman;
    CTxMemPool& mempool;
    const CActiveMasternodeManager& m_mn_activeman;
    const CMasternodeSync& m_mn_sync;
    const llmq::CInstantSendManager& m_isman;

protected:
    // Session state and entry admission live in the protected section so unit tests can seed
    // and drive them through a test subclass.

    /// The collateral transactions of every peer admitted to the current session.
    ///
    /// Mixing uses collateral transactions to trust parties entering the pool to behave
    /// honestly. If they don't it takes their money.
    ///
    /// Session collaterals are only ever test-accepted, never added to the mempool, so nothing
    /// pins their identity: the same UTXO can be re-signed into arbitrarily many distinct txids.
    /// Matching on input prevouts is what makes a resent or replayed dsa recognisable as the
    /// same participant.
    class SessionCollaterals
    {
    public:
        void Add(const CMutableTransaction& txCollateral)
        {
            m_txs.push_back(MakeTransactionRef(txCollateral));
            for (const auto& txin : txCollateral.vin) {
                m_prevouts.insert(txin.prevout);
            }
        }
        void Clear()
        {
            m_txs.clear();
            m_prevouts.clear();
        }
        //! The first input of txCollateral that an already admitted collateral also spends, if any.
        std::optional<COutPoint> FindCommittedPrevout(const CMutableTransaction& txCollateral) const
        {
            for (const auto& txin : txCollateral.vin) {
                if (m_prevouts.contains(txin.prevout)) return txin.prevout;
            }
            return std::nullopt;
        }
        const std::vector<CTransactionRef>& txs() const { return m_txs; }
        size_t size() const { return m_txs.size(); }
        bool empty() const { return m_txs.empty(); }

    private:
        std::vector<CTransactionRef> m_txs;
        std::unordered_set<COutPoint, SaltedOutpointHasher> m_prevouts;
    };
    SessionCollaterals m_session_collaterals GUARDED_BY(cs_coinjoin);

    // Post-V24: true once a participant has been admitted that declared a promotion/demotion,
    // i.e. the final transaction may come out unbalanced. Latched on admission rather than
    // fixed by the creator's protocol version, so a session only commits to this once someone
    // actually asks for it.
    bool m_fRebalanceSession GUARDED_BY(cs_coinjoin){false};
    // Post-V24: true once a participant below COINJOIN_REBALANCE_VERSION has been admitted.
    // Such a peer cannot validate an unbalanced final transaction, so it must never share a
    // session with a rebalance participant. Mutually exclusive with m_fRebalanceSession.
    bool m_fHasLegacyParticipant GUARDED_BY(cs_coinjoin){false};
    // The mixing direction each accepted participant declared in its dsa, keyed by collateral
    // hash. Tells us which side of the session denomination a participant will occupy before
    // its entry arrives, and entitles it (and only it) to submit an entry of that shape.
    std::map<uint256, CoinJoin::MixShape> m_mapDeclaredShapes GUARDED_BY(cs_coinjoin);

    /// Sides of the session denomination the accepted participants declared they will occupy
    CoinJoin::MixSideCounts GetDeclaredSideCounts() const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    /// Add a clients entry to the pool
    bool AddEntry(const CCoinJoinEntry& entry, PoolMessage& nMessageIDRet) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Build and relay the final transaction if the live session is still eligible, charging one
    /// missing participant first if charge_fees is set
    void CreateFinalTransaction(int session_id, bool charge_fees) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

private:
    bool fUnitTest;

    /// Serializes CheckPool() against itself and against CheckTimeout(). CheckPool() runs both on
    /// the scheduler thread and on the message-handling thread, and its finalize and commit steps
    /// have to be single-shot: relaying DSFINALTX twice makes every client sign twice, and the
    /// duplicate signatures then abort the session for all of them. CheckTimeout() uses the same
    /// guard so it cannot reset a session during finalization or commit. Always acquired with
    /// TRY_LOCK, so a contended caller skips the round rather than blocking msghand.
    Mutex cs_check_pool;

    /// Add signature to a txin
    bool AddScriptSig(const CTxIn& txin, const CService& sender) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    /// Collaterals chosen to be charged whose consumption has not finished yet. They are chosen
    /// under cs_coinjoin, but ConsumeCollateral() takes cs_main and runs after cs_coinjoin is
    /// released, by which time the session may have been reset. Until then admission refuses any
    /// collateral that spends one of their inputs, so a charged participant cannot be readmitted
    /// into the next session with a collateral that is about to be spent. Survives SetNull().
    std::vector<CTransactionRef> m_collaterals_being_charged GUARDED_BY(cs_coinjoin);
    /// Does txCollateral spend an input of a collateral that is being charged?
    bool SpendsCollateralBeingCharged(const CMutableTransaction& txCollateral) const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    /// Choose one bad actor whose collateral should be consumed, if any, and reserve it. The
    /// result must be passed to ConsumeReservedCollateral().
    CTransactionRef SelectCollateralToCharge() EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);
    /// Rarely charge fees to pay miners
    void ChargeRandomFees(const std::vector<CTransactionRef>& collaterals) const EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Consume collateral in cases when peer misbehaved. Takes cs_main, which this class never
    /// takes under cs_coinjoin.
    void ConsumeCollateral(const CTransactionRef& txref) const EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Consume a collateral reserved in m_collaterals_being_charged, then release the reservation
    void ConsumeReservedCollateral(const CTransactionRef& txref) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Consume collateral, but only while session_id is still the live session holding it
    void ConsumeCollateralIfCurrentSession(int session_id, const CTransactionRef& txref)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    /// Is session_id still the session we are accepting entries for?
    bool IsCurrentSession(int session_id) const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    //! One consistent view of the session, for decisions that read several pieces of its state.
    //! Sampling them one at a time lets the message-handling thread commit an entry in between,
    //! producing a mix of old and new values that describes no state the session was ever in.
    struct PoolSnapshot {
        int session_id{0};
        PoolState state{POOL_STATE_IDLE};
        size_t entries{0};
        size_t collaterals{0};
        CoinJoin::MixSideCounts sides;
        bool signatures_complete{false};
    };
    PoolSnapshot GetPoolSnapshot() const EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Is txref one of the collaterals accepted into the current session?
    bool HasSessionCollateral(const CTransactionRef& txref) const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    /// Check for process
    void CheckPool() EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin, !cs_check_pool);

    void CommitFinalTransaction(int session_id) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Tell the given participants of session_id how it ended. Side-effect free: the participant
    /// list is captured by the caller, so a reset or replacement session cannot be notified.
    void RelayCompletedTransaction(int session_id, const std::vector<CService>& participants, PoolMessage nMessageID)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Reset the pool, but only while session_id is still the signing session being completed
    void ResetSigningSessionIfCurrent(int session_id) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    /// Is this nDenom and txCollateral acceptable?
    bool IsAcceptableDSA(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet) const;
    bool CreateNewSession(const CCoinJoinAccept& dsa, int nPeerVersion, PoolMessage& nMessageIDRet) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    bool AddUserToExistingSession(const CCoinJoinAccept& dsa, int nPeerVersion, PoolMessage& nMessageIDRet) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Do we have enough users to take entries?
    bool IsSessionReady() const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    /// Check that all inputs are signed. (Are all inputs signed?)
    bool IsSignaturesComplete() const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    // Set the 'state' value, with some logging and capturing when the state changed.
    // Requires cs_coinjoin so that a transition and the session data it describes are always
    // observed together: code that revalidates nState under the lock must not have it changed
    // out from under it by a concurrent transition.
    void SetState(PoolState nStateNew) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    /// Relay mixing Messages
    void RelayFinalTransaction(const CTransaction& txFinal) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);
    void PushStatus(CNode& peer, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID) const;
    void RelayStatus(PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID = MSG_NOERR) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);
    /// Addresses of the participants that submitted an entry to the current session
    std::vector<CService> GetParticipantAddrs() const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    void ProcessDSACCEPT(CNode& peer, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    void ProcessDSQUEUE(NodeId from, CDataStream& vRecv);
    void ProcessDSVIN(CNode& peer, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin, !cs_check_pool);
    void ProcessDSSIGNFINALTX(CNode& peer, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin, !cs_check_pool);

    void SetNull() override EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

public:
    CCoinJoinServer() = delete;
    CCoinJoinServer(const CCoinJoinServer&) = delete;
    CCoinJoinServer& operator=(const CCoinJoinServer&) = delete;
    explicit CCoinJoinServer(PeerManagerInternal* peer_manager, ChainstateManager& chainman, CConnman& _connman,
                             CDeterministicMNManager& dmnman, CDSTXManager& dstxman, CMasternodeMetaMan& mn_metaman,
                             CTxMemPool& mempool, const CActiveMasternodeManager& mn_activeman,
                             const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman);
    ~CCoinJoinServer() override;

    void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin, !cs_check_pool);
    bool ProcessGetData(CNode& pfrom, const CInv& inv, const CNetMsgMaker& msgMaker) override;
    bool AlreadyHave(const CInv& inv) override;
    void Schedule(CScheduler& scheduler) override;

    bool HasTimedOut() const;
    void CheckTimeout() EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin, !cs_check_pool);
    void CheckForCompleteQueue() EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    void GetJsonInfo(UniValue& obj) const;
};

#endif // BITCOIN_COINJOIN_SERVER_H
