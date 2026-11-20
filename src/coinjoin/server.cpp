// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/server.h>

#include <active/masternode.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <masternode/meta.h>
#include <masternode/sync.h>

#include <coins.h>
#include <core_io.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <policy/policy.h>
#include <scheduler.h>
#include <script/interpreter.h>
#include <serialize.h>
#include <shutdown.h>
#include <streams.h>
#include <txmempool.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <validation.h>

#include <univalue.h>

#include <algorithm>
#include <iterator>
#include <ranges>

CCoinJoinServer::CCoinJoinServer(PeerManagerInternal* peer_manager, ChainstateManager& chainman, CConnman& _connman,
                                 CDeterministicMNManager& dmnman, CDSTXManager& dstxman, CMasternodeMetaMan& mn_metaman,
                                 CTxMemPool& mempool, const CActiveMasternodeManager& mn_activeman,
                                 const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman) :
    NetHandler(peer_manager),
    m_chainman{chainman},
    connman{_connman},
    m_dmnman{dmnman},
    m_dstxman{dstxman},
    m_mn_metaman{mn_metaman},
    mempool{mempool},
    m_mn_activeman{mn_activeman},
    m_mn_sync{mn_sync},
    m_isman{isman},
    fUnitTest{false}
{
}

CCoinJoinServer::~CCoinJoinServer() = default;

void CCoinJoinServer::ProcessMessage(CNode& peer, const std::string& msg_type, CDataStream& vRecv)
{
    if (!m_mn_sync.IsBlockchainSynced()) return;

    if (msg_type == NetMsgType::DSACCEPT) {
        ProcessDSACCEPT(peer, vRecv);
    } else if (msg_type == NetMsgType::DSQUEUE) {
        ProcessDSQUEUE(peer.GetId(), vRecv);
    } else if (msg_type == NetMsgType::DSVIN) {
        ProcessDSVIN(peer, vRecv);
    } else if (msg_type == NetMsgType::DSSIGNFINALTX) {
        ProcessDSSIGNFINALTX(peer, vRecv);
    }
}

void CCoinJoinServer::ProcessDSACCEPT(CNode& peer, CDataStream& vRecv)
{
    assert(m_mn_metaman.IsValid());

    if (WITH_LOCK(cs_coinjoin, return IsSessionReady())) {
        // too many users in this session already, reject new ones
        LogPrint(BCLog::COINJOIN, "DSACCEPT -- queue is already full!\n");
        PushStatus(peer, STATUS_REJECTED, ERR_QUEUE_FULL);
        return;
    }

    CCoinJoinAccept dsa;
    vRecv >> dsa;

    LogPrint(BCLog::COINJOIN, "DSACCEPT -- nDenom %d (%s)  txCollateral %s", dsa.nDenom, CoinJoin::DenominationToString(dsa.nDenom), dsa.txCollateral.ToString()); /* Continued */

    auto mnList = m_dmnman.GetListAtChainTip();
    auto dmn = mnList.GetValidMNByCollateral(m_mn_activeman.GetOutPoint());
    if (!dmn) {
        PushStatus(peer, STATUS_REJECTED, ERR_MN_LIST);
        return;
    }

    if (WITH_LOCK(cs_coinjoin, return m_session_collaterals.empty())) {
        {
            const auto hasQueue = m_queueman.TryHasQueueFromMasternode(m_mn_activeman.GetOutPoint());
            if (!hasQueue.has_value()) return;
            if (*hasQueue) {
                // refuse to create another queue this often
                LogPrint(BCLog::COINJOIN, "DSACCEPT -- last dsq is still in queue, refuse to mix\n");
                PushStatus(peer, STATUS_REJECTED, ERR_RECENT);
                return;
            }
        }

        if (m_mn_metaman.IsMixingThresholdExceeded(dmn->proTxHash, mnList.GetCounts().enabled())) {
            if (fLogIPs) {
                LogPrint(BCLog::COINJOIN, "DSACCEPT -- last dsq too recent, must wait: peer=%d, addr=%s\n",
                         peer.GetId(), peer.addr.ToStringAddrPort());
            } else {
                LogPrint(BCLog::COINJOIN, "DSACCEPT -- last dsq too recent, must wait: peer=%d\n", peer.GetId());
            }
            PushStatus(peer, STATUS_REJECTED, ERR_RECENT);
            return;
        }
    }

    PoolMessage nMessageID = MSG_NOERR;

    bool fResult = nSessionID == 0 ? CreateNewSession(dsa, peer.GetCommonVersion(), nMessageID)
            : AddUserToExistingSession(dsa, peer.GetCommonVersion(), nMessageID);
    if (fResult) {
        LogPrint(BCLog::COINJOIN, "DSACCEPT -- is compatible, please submit!\n");
        PushStatus(peer, STATUS_ACCEPTED, nMessageID);
        return;
    } else {
        LogPrint(BCLog::COINJOIN, "DSACCEPT -- not compatible with existing transactions!\n");
        PushStatus(peer, STATUS_REJECTED, nMessageID);
        return;
    }
}

void CCoinJoinServer::ProcessDSQUEUE(NodeId from, CDataStream& vRecv)
{
    assert(m_mn_metaman.IsValid());

    CCoinJoinQueue dsq;
    vRecv >> dsq;

    WITH_LOCK(cs_main, m_peer_manager->PeerEraseObjectRequest(from, CInv{MSG_DSQ, dsq.GetHash()}));

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
    const auto [dmn, misbehavior] = CoinJoin::ValidateDSQueue(dsq, tip_mn_list, [this, from](const CCoinJoinQueue& q) {
        const auto isDup = m_queueman.TryCheckDuplicate(q);
        if (!isDup.has_value()) return true;
        if (*isDup) {
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- Peer %d is sending WAY too many dsq messages for a masternode with collateral %s\n", from, q.masternodeOutpoint.ToStringShort());
        }
        return *isDup;
    });
    if (misbehavior > 0) {
        m_peer_manager->PeerMisbehaving(from, misbehavior);
        return;
    }
    if (!dmn) return;

    if (!dsq.fReady) {
        //don't allow a few nodes to dominate the queuing process
        if (m_mn_metaman.IsMixingThresholdExceeded(dmn->proTxHash, tip_mn_list.GetCounts().enabled())) {
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- node sending too many dsq messages, masternode=%s\n", dmn->proTxHash.ToString());
            return;
        }
        m_mn_metaman.AllowMixing(dmn->proTxHash);

        LogPrint(BCLog::COINJOIN, "DSQUEUE -- new CoinJoin queue, masternode=%s, queue=%s\n", dmn->proTxHash.ToString(), dsq.ToString());

        if (!m_queueman.TryAddQueue(dsq)) return;
        WITH_LOCK(cs_main, m_peer_manager->PeerForgetObjectRequest(CInv{MSG_DSQ, dsq.GetHash()}));
        m_peer_manager->PeerRelayDSQ(dsq);
    }
}

void CCoinJoinServer::ProcessDSVIN(CNode& peer, CDataStream& vRecv)
{
    //do we have enough users in the current session?
    if (!WITH_LOCK(cs_coinjoin, return IsSessionReady())) {
        LogPrint(BCLog::COINJOIN, "DSVIN -- session not complete!\n");
        PushStatus(peer, STATUS_REJECTED, ERR_SESSION);
        return;
    }

    CCoinJoinEntry entry;
    vRecv >> entry;

    LogPrint(BCLog::COINJOIN, "DSVIN -- txCollateral %s", entry.txCollateral->ToString()); /* Continued */

    // Note: unbalanced (promotion/demotion) entries are only valid post-V24; AddEntry ->
    // IsValidInOuts rejects them pre-V24 and consumes the collateral to keep spam costly

    PoolMessage nMessageID = MSG_NOERR;

    entry.addr = peer.addr;
    if (AddEntry(entry, nMessageID)) {
        PushStatus(peer, STATUS_ACCEPTED, nMessageID);
        CheckPool();
        LOCK(cs_coinjoin);
        RelayStatus(STATUS_ACCEPTED);
    } else {
        PushStatus(peer, STATUS_REJECTED, nMessageID);
    }
}

void CCoinJoinServer::ProcessDSSIGNFINALTX(CNode& peer, CDataStream& vRecv)
{
    // Only accept signatures while we are actually collecting them, and only
    // from peers that are active participants in this session. Otherwise a
    // stray or unauthenticated peer could abort the session for everyone.
    int session_id{0};
    {
        LOCK(cs_coinjoin);
        if (nState != POOL_STATE_SIGNING) {
            LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- wrong state, nState=%d, peer=%d\n", nState.load(), peer.GetId());
            PushStatus(peer, STATUS_REJECTED, ERR_SESSION);
            return;
        }
        const bool is_participant = std::ranges::any_of(
            vecEntries, [&peer](const auto& entry) { return entry.addr == peer.addr; });
        if (!is_participant) {
            LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- ignoring message from non-participant peer=%d\n",
                     peer.GetId());
            PushStatus(peer, STATUS_REJECTED, ERR_INVALID_INPUT);
            return;
        }
        session_id = nSessionID;
    }

    const size_t max_txins{CoinJoin::GetMaxPoolInputOutputCount()};
    std::vector<CTxIn> vecTxIn;
    // Reject an over-cap count through this peer-local ERR_MAXIMUM path before a
    // single CTxIn is decoded or allocated. A count above the generic MAX_SIZE cap
    // is malformed and throws from ReadCompactSize, as it does for any other message.
    if (!UnserializeVectorWithMaxSize(vRecv, vecTxIn, max_txins)) {
        LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- too many inputs. max=%d, peer=%d\n",
                 static_cast<int>(max_txins), peer.GetId());
        PushStatus(peer, STATUS_REJECTED, ERR_MAXIMUM);
        return;
    }

    LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- vecTxIn.size() %s\n", vecTxIn.size());

    {
        LOCK(cs_coinjoin);
        // Apply the whole message in one critical section, against the signing session it was
        // checked for above. The lock was released while the body was decoded, and releasing it
        // between inputs would let a timeout reset the session halfway through one sender's
        // already-received signatures, after charging that sender as a non-signer.
        if (nSessionID != session_id || nState != POOL_STATE_SIGNING) {
            LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- session %d is no longer signing, peer=%d\n", session_id,
                     peer.GetId());
            PushStatus(peer, STATUS_REJECTED, ERR_SESSION);
            return;
        }
        int nTxInIndex = 0;
        int nTxInsCount = static_cast<int>(vecTxIn.size());
        for (const auto& txin : vecTxIn) {
            nTxInIndex++;
            if (!AddScriptSig(txin, peer.addr)) {
                LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- AddScriptSig() failed at %d/%d, session: %d\n", nTxInIndex,
                         nTxInsCount, session_id);
                RelayStatus(STATUS_REJECTED);
                return;
            }
            LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- AddScriptSig() %d/%d success\n", nTxInIndex, nTxInsCount);
        }
    }
    // all is good
    CheckPool();
}

void CCoinJoinServer::SetNull()
{
    AssertLockHeld(cs_coinjoin);
    // MN side
    m_session_collaterals.Clear();
    m_fRebalanceSession = false;
    m_fHasLegacyParticipant = false;
    m_mapDeclaredShapes.clear();

    CCoinJoinBaseSession::SetNull();
    m_queueman.SetNull();
}

//
// Check the mixing progress and send client updates if a Masternode
//
void CCoinJoinServer::CheckPool()
{
    AssertLockNotHeld(cs_coinjoin);

    // Both the scheduler thread and the message-handling thread get here. Skip the round if the
    // other one is already in it rather than blocking msghand behind its mempool work: whichever
    // thread holds the lock is performing the same check we would.
    TRY_LOCK(cs_check_pool, lock_check_pool);
    if (!lock_check_pool) return;

    // Every decision below reads several pieces of session state at once, so take them as one
    // snapshot. Sampling them separately let the message-handling thread commit an entry
    // between two reads: the side counts could be read before the last entry arrived while the
    // entry count was read after it, so a session that was about to finalize looked like one
    // whose entries were all in but left a side uncovered - and got reset. Reading
    // the session collaterals without the lock could also race SetNull() clearing them.
    const auto snap = GetPoolSnapshot();

    if (snap.entries != 0)
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- entries count %lu\n", snap.entries);

    // PRIVACY: a final transaction is only worth publishing when each side of the session
    // denomination is occupied by nobody or by at least two participants; a lone participant
    // on a side would have the only coins of that size there and be trivially identifiable.

    // If we have an entry for each collateral, then create final tx
    if (snap.state == POOL_STATE_ACCEPTING_ENTRIES && snap.entries == snap.collaterals) {
        if (snap.sides.IsCovered()) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- FINALIZE TRANSACTIONS\n");
            CreateFinalTransaction(snap.session_id, /*charge_fees=*/false);
            return;
        }
        // The participant set is frozen once entries are being accepted, so this session can
        // never gain the missing counterparty - reset instead of stalling everyone until
        // timeout. Shouldn't happen: admission checks the declared shapes up front.
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- all entries received but a session denom side is uncovered (%d in, %d out), resetting session\n",
                 snap.sides.inputs, snap.sides.outputs);
        // Only drop the session the snapshot described; the message-handling thread may have
        // reset and restarted one since. Capture its participants in the same step, then tell
        // them, so they release their inputs and collateral right away instead of waiting out
        // their own timeout.
        std::vector<CService> participants;
        {
            LOCK(cs_coinjoin);
            if (!IsCurrentSession(snap.session_id)) return;
            participants = GetParticipantAddrs();
            SetNull();
        }
        RelayCompletedTransaction(snap.session_id, participants, ERR_SESSION);
        return;
    }

    // Check for Time Out
    // If we timed out while accepting entries, then if we have more than minimum, create final tx
    if (snap.state == POOL_STATE_ACCEPTING_ENTRIES && CCoinJoinServer::HasTimedOut() && snap.sides.IsCovered() &&
        snap.entries >= static_cast<size_t>(CoinJoin::GetMinPoolParticipants())) {
        // Punish misbehaving participants and try to complete this session ignoring them
        CreateFinalTransaction(snap.session_id, /*charge_fees=*/true);
        return;
    }

    // If we have all the signatures, try to compile the transaction
    if (snap.state == POOL_STATE_SIGNING && snap.signatures_complete) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- SIGNING\n");
        CommitFinalTransaction(snap.session_id);
        return;
    }
}

CCoinJoinServer::PoolSnapshot CCoinJoinServer::GetPoolSnapshot() const
{
    AssertLockNotHeld(cs_coinjoin);
    LOCK(cs_coinjoin);
    return PoolSnapshot{nSessionID, nState, vecEntries.size(), m_session_collaterals.size(),
                        GetMixSideCountsLocked(), IsSignaturesComplete()};
}

void CCoinJoinServer::CreateFinalTransaction(int session_id, bool charge_fees)
{
    AssertLockNotHeld(cs_coinjoin);
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- FINALIZE TRANSACTIONS\n");

    CTransactionRef collateral_to_charge;
    {
        LOCK(cs_coinjoin);

        // The decision to finalize came from a snapshot taken before this lock, so make sure it
        // still describes an eligible live session. An entry can finish validation and commit in
        // between, changing a covered side from empty to a lone participant. Check the live
        // entries under the same lock used to build the transaction so that entry admission
        // cannot invalidate the decision before the state moves to signing.
        if (!IsCurrentSession(session_id)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- session changed, not finalizing\n");
            return;
        }
        const auto sides = GetMixSideCountsLocked();
        if (vecEntries.size() < static_cast<size_t>(CoinJoin::GetMinPoolParticipants()) || !sides.IsCovered()) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- session no longer eligible, entries=%d, sides=%d/%d\n",
                     vecEntries.size(), sides.inputs, sides.outputs);
            // Deliberately no timer refresh: on the timeout path the session stays timed out and
            // the scheduler's next CheckTimeout() resets it. The missing side already had the full
            // entry window, so waiting another one would only keep everyone else's coins locked.
            return;
        }

        // Selecting the offenders and moving to signing below happen under one lock, so a
        // participant whose entry commits before the cutoff can no longer be charged as missing.
        if (charge_fees) {
            collateral_to_charge = SelectCollateralToCharge();
        }

        CMutableTransaction txNew;

        // make our new transaction
        for (const auto& entry : vecEntries) {
            for (const auto& txout : entry.vecTxOut) {
                txNew.vout.push_back(txout);
            }
            for (const auto& txdsin : entry.vecTxDSIn) {
                txNew.vin.push_back(txdsin);
            }
        }

        sort(txNew.vin.begin(), txNew.vin.end(), CompareInputBIP69());
        sort(txNew.vout.begin(), txNew.vout.end(), CompareOutputBIP69());

        finalMutableTransaction = txNew;
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- finalMutableTransaction=%s", /* Continued */
                 txNew.ToString());

        // request signatures from clients
        SetState(POOL_STATE_SIGNING);
        RelayFinalTransaction(CTransaction(finalMutableTransaction));
    }

    if (collateral_to_charge) {
        ConsumeReservedCollateral(collateral_to_charge);
    }
}

void CCoinJoinServer::CommitFinalTransaction(int session_id)
{
    AssertLockNotHeld(cs_coinjoin);

    CTransactionRef finalTransaction;
    std::vector<CService> participants;
    std::vector<CTransactionRef> collaterals;
    {
        LOCK(cs_coinjoin);
        // Committing a session that is already gone would push a cleared finalMutableTransaction
        // through ATMP and notify the participants of a failure that never happened.
        if (nSessionID != session_id || nState != POOL_STATE_SIGNING) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- session %d is gone, not committing\n",
                     session_id);
            return;
        }
        finalTransaction = MakeTransactionRef(finalMutableTransaction);
        participants = GetParticipantAddrs();
        collaterals = m_session_collaterals.txs();
    }
    uint256 hashTx = finalTransaction->GetHash();

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- finalTransaction=%s", /* Continued */
             finalTransaction->ToString());

    {
        // See if the transaction is valid
        TRY_LOCK(::cs_main, lockMain);
        mempool.PrioritiseTransaction(hashTx, 0.1 * COIN);
        if (!lockMain || !ATMPIfSaneFee(m_chainman, finalTransaction)) {
            LogPrint(BCLog::COINJOIN, /* Continued */
                     "CCoinJoinServer::CommitFinalTransaction -- ATMPIfSaneFee() error: Transaction not valid\n");
            // not much we can do in this case, just notify clients
            RelayCompletedTransaction(session_id, participants, ERR_INVALID_TX);
            ResetSigningSessionIfCurrent(session_id);
            return;
        }
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- CREATING DSTX\n");

    // create and sign masternode dstx transaction
    if (!m_dstxman.GetDSTX(hashTx)) {
        CCoinJoinBroadcastTx dstxNew(finalTransaction, m_mn_activeman.GetOutPoint(), m_mn_activeman.GetProTxHash(),
                                     GetAdjustedTime());
        dstxNew.vchSig = m_mn_activeman.SignBasic(dstxNew.GetSignatureHash());
        m_dstxman.AddDSTX(dstxNew);
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- TRANSMITTING DSTX\n");

    CInv inv(MSG_DSTX, hashTx);
    m_peer_manager->PeerRelayInv(inv);

    // Tell the clients it was successful
    RelayCompletedTransaction(session_id, participants, MSG_SUCCESS);

    // Randomly charge clients
    ChargeRandomFees(collaterals);

    // Reset
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- COMPLETED -- RESETTING\n");
    ResetSigningSessionIfCurrent(session_id);
}

//
// Charge clients a fee if they're abusive
//
// Why bother? CoinJoin uses collateral to ensure abuse to the process is kept to a minimum.
// The submission and signing stages are completely separate. In the cases where
// a client submits a transaction then refused to sign, there must be a cost. Otherwise, they
// would be able to do this over and over again and bring the mixing to a halt.
//
// How does this work? Messages to Masternodes come in via NetMsgType::DSVIN, these require a valid collateral
// transaction for the client to be able to enter the pool. This transaction is kept by the Masternode
// until the transaction is either complete or fails.
//
/*
 * Select one offender while cs_coinjoin still binds the state, entries and collaterals to the
 * same session. The caller must close the relevant admission path before releasing the lock and
 * consuming the returned collateral, so a late entry or signature cannot make the snapshot stale.
 * The returned collateral stays reserved until ConsumeReservedCollateral() has run.
 */
CTransactionRef CCoinJoinServer::SelectCollateralToCharge()
{
    AssertLockHeld(cs_coinjoin);

    //we don't need to charge collateral for every offence.
    if (GetRand<int>(/*nMax=*/100) > 33) return {};

    std::vector<CTransactionRef> vecOffendersCollaterals;
    const PoolState state{nState};
    const size_t nSessionCollaterals{m_session_collaterals.size()};

    if (state == POOL_STATE_ACCEPTING_ENTRIES) {
        for (const auto& txCollateral : m_session_collaterals.txs()) {
            bool fFound = std::ranges::any_of(vecEntries, [&txCollateral](const auto& entry) {
                return *entry.txCollateral == *txCollateral;
            });

            // This queue entry didn't send us the promised transaction
            if (!fFound) {
                LogPrint(BCLog::COINJOIN, /* Continued */
                         "CCoinJoinServer::SelectCollateralToCharge -- found uncooperative node (didn't send "
                         "transaction), found offence\n");
                vecOffendersCollaterals.push_back(txCollateral);
            }
        }
    } else if (state == POOL_STATE_SIGNING) {
        // who didn't sign?
        for (const auto& entry : vecEntries) {
            for (const auto& txdsin : entry.vecTxDSIn) {
                if (!txdsin.fHasSig) {
                    LogPrint(BCLog::COINJOIN, /* Continued */
                             "CCoinJoinServer::SelectCollateralToCharge -- found uncooperative node (didn't sign), "
                             "found offence\n");
                    vecOffendersCollaterals.push_back(entry.txCollateral);
                    break;
                }
            }
        }
    }

    // no offences found
    if (vecOffendersCollaterals.empty()) return {};

    //mostly offending? Charge sometimes
    if (vecOffendersCollaterals.size() >= nSessionCollaterals - 1 && GetRand<int>(/*nMax=*/100) > 33) return {};

    //everyone is an offender? That's not right
    if (vecOffendersCollaterals.size() >= nSessionCollaterals) return {};

    //charge one of the offenders randomly
    Shuffle(vecOffendersCollaterals.begin(), vecOffendersCollaterals.end(), FastRandomContext());

    LogPrint(BCLog::COINJOIN, /* Continued */
             "CCoinJoinServer::SelectCollateralToCharge -- found uncooperative node (didn't %s transaction), charging "
             "fees: %s",
             (state == POOL_STATE_SIGNING) ? "sign" : "send", vecOffendersCollaterals[0]->ToString());
    m_collaterals_being_charged.push_back(vecOffendersCollaterals[0]);
    return vecOffendersCollaterals[0];
}

/*
    Charge the collateral randomly.
    Mixing is completely free, to pay miners we randomly pay the collateral of users.

    Collateral Fee Charges:

    Being that mixing has "no fees" we need to have some kind of cost associated
    with using it to stop abuse. Otherwise, it could serve as an attack vector and
    allow endless transaction that would bloat Dash and make it unusable. To
    stop these kinds of attacks 1 in 10 successful transactions are charged. This
    adds up to a cost of 0.001DRK per transaction on average.
*/
void CCoinJoinServer::ChargeRandomFees(const std::vector<CTransactionRef>& collaterals) const
{
    AssertLockNotHeld(cs_coinjoin);

    for (const auto& txCollateral : collaterals) {
        if (GetRand<int>(/*nMax=*/100) > 10) return;
        LogPrint(BCLog::COINJOIN, /* Continued */
                 "CCoinJoinServer::ChargeRandomFees -- charging random fees, txCollateral=%s", txCollateral->ToString());
        ConsumeCollateral(txCollateral);
    }
}

void CCoinJoinServer::ConsumeCollateral(const CTransactionRef& txref) const
{
    LOCK(::cs_main);
    if (!ATMPIfSaneFee(m_chainman, txref)) {
        LogPrint(BCLog::COINJOIN, "%s -- ATMPIfSaneFee failed\n", __func__);
    } else {
        m_peer_manager->PeerRelayTransaction(txref->GetHash());
        LogPrint(BCLog::COINJOIN, "%s -- Collateral was consumed\n", __func__);
    }
}

void CCoinJoinServer::ConsumeReservedCollateral(const CTransactionRef& txref)
{
    AssertLockNotHeld(cs_coinjoin);
    ConsumeCollateral(txref);

    // From here on the mempool, not the reservation, keeps a consumed collateral out of new sessions.
    LOCK(cs_coinjoin);
    const auto it = std::ranges::find(m_collaterals_being_charged, txref);
    if (Assume(it != m_collaterals_being_charged.end())) m_collaterals_being_charged.erase(it);
}

bool CCoinJoinServer::SpendsCollateralBeingCharged(const CMutableTransaction& txCollateral) const
{
    AssertLockHeld(cs_coinjoin);
    return std::ranges::any_of(m_collaterals_being_charged, [&txCollateral](const CTransactionRef& charged) {
        return std::ranges::any_of(charged->vin, [&txCollateral](const CTxIn& charged_in) {
            return std::ranges::any_of(txCollateral.vin,
                                       [&charged_in](const CTxIn& txin) { return txin.prevout == charged_in.prevout; });
        });
    });
}

void CCoinJoinServer::ConsumeCollateralIfCurrentSession(int session_id, const CTransactionRef& txref)
{
    AssertLockNotHeld(cs_coinjoin);

    // cs_coinjoin is released around the collateral and UTXO checks in AddEntry, so by the time
    // we get here the scheduler thread may have timed the session out and started another one.
    // Charging then would spend the collateral of a participant that is no longer in any
    // session - it may even be a replay of an innocent participant's collateral.
    bool fStillCurrent{false};
    {
        LOCK(cs_coinjoin);
        fStillCurrent = IsCurrentSession(session_id) && HasSessionCollateral(txref);
        if (fStillCurrent) m_collaterals_being_charged.push_back(txref);
    }
    if (!fStillCurrent) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- session changed, not consuming collateral %s\n", __func__,
                 txref->GetHash().ToString());
        return;
    }
    ConsumeReservedCollateral(txref);
}

bool CCoinJoinServer::IsCurrentSession(int session_id) const
{
    AssertLockHeld(cs_coinjoin);
    return nSessionID != 0 && nSessionID == session_id && nState == POOL_STATE_ACCEPTING_ENTRIES;
}

bool CCoinJoinServer::HasSessionCollateral(const CTransactionRef& txref) const
{
    AssertLockHeld(cs_coinjoin);
    return std::ranges::any_of(m_session_collaterals.txs(),
                               [&txref](const CTransactionRef& ref) { return *ref == *txref; });
}

bool CCoinJoinServer::HasTimedOut() const
{
    if (nState == POOL_STATE_IDLE) return false;

    int nTimeout = (nState == POOL_STATE_SIGNING) ? COINJOIN_SIGNING_TIMEOUT : COINJOIN_QUEUE_TIMEOUT;

    return GetTime() - nTimeLastSuccessfulStep >= nTimeout;
}

bool ShouldResetTimedOutSession(const CoinJoinSessionProgress& session)
{
    if (!session.timed_out) return false;

    const bool can_advance{(session.state == POOL_STATE_QUEUE && session.queue_ready) ||
                           (session.state == POOL_STATE_ACCEPTING_ENTRIES &&
                            (session.entries == session.collaterals ||
                             (session.entries >= static_cast<size_t>(CoinJoin::GetMinPoolParticipants()) &&
                              session.sides.IsCovered()))) ||
                           (session.state == POOL_STATE_SIGNING && session.signatures_complete)};
    return !can_advance;
}

//
// Check for extraneous timeout
//
void CCoinJoinServer::CheckTimeout()
{
    m_queueman.CheckQueue();

    // CheckPool can be finalizing or committing on the message-handling thread. Skipping this tick
    // keeps timeout reset and finalization/commit single-flight without blocking the scheduler.
    TRY_LOCK(cs_check_pool, lock_check_pool);
    if (!lock_check_pool) return;

    CTransactionRef collateral_to_charge;
    {
        LOCK(cs_coinjoin);

        // Decide while holding the lock so selecting an offender and closing the session form one
        // atomic cutoff for late entries and signatures.
        // CheckForCompleteQueue() and CheckPool() run before this method on the scheduler thread,
        // but a final collateral, entry, or signature can arrive after their snapshots. The
        // message-handling thread then skips its own CheckPool() if this scheduler tick still holds
        // cs_check_pool. A session that can now advance takes priority over resetting it; the next
        // scheduler tick will perform the transition, finalization, or commit.
        if (!ShouldResetTimedOutSession({.state = nState,
                                         .timed_out = CCoinJoinServer::HasTimedOut(),
                                         .queue_ready = IsSessionReady(),
                                         .entries = vecEntries.size(),
                                         .collaterals = m_session_collaterals.size(),
                                         .sides = GetMixSideCountsLocked(),
                                         .signatures_complete = IsSignaturesComplete()})) {
            return;
        }

        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckTimeout -- %s timed out -- resetting\n",
                 (nState == POOL_STATE_SIGNING) ? "Signing" : "Session");
        collateral_to_charge = SelectCollateralToCharge();
        SetNull();
    }

    if (collateral_to_charge) {
        ConsumeReservedCollateral(collateral_to_charge);
    }
}

/*
    Check to see if we're ready for submissions from clients
    After receiving multiple dsa messages, the queue will switch to "accepting entries"
    which is the active state right before merging the transaction
*/
void CCoinJoinServer::CheckForCompleteQueue()
{
    AssertLockNotHeld(cs_coinjoin);

    int session_denom;
    size_t participants;
    {
        LOCK(cs_coinjoin);
        if (nState != POOL_STATE_QUEUE || !IsSessionReady()) return;

        SetState(POOL_STATE_ACCEPTING_ENTRIES);
        session_denom = nSessionDenom;
        participants = m_session_collaterals.size();
    }

    CCoinJoinQueue dsq(session_denom, m_mn_activeman.GetOutPoint(), m_mn_activeman.GetProTxHash(), GetAdjustedTime(), true);
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckForCompleteQueue -- ready queue %s with %d participants\n",
             dsq.ToString(), participants);
    dsq.vchSig = m_mn_activeman.SignBasic(dsq.GetSignatureHash());
    m_peer_manager->PeerRelayDSQ(dsq);
    m_queueman.AddQueue(std::move(dsq));
}

//
// Add a client's transaction inputs/outputs to the pool
//
bool CCoinJoinServer::AddEntry(const CCoinJoinEntry& entry, PoolMessage& nMessageIDRet)
{
    AssertLockNotHeld(cs_coinjoin);

    const auto hasEntryForCollateral = [&entry](const CCoinJoinEntry& other) {
        return *other.txCollateral == *entry.txCollateral;
    };

    CoinJoin::MixShape declaredShape{CoinJoin::MixShape::STANDARD};
    int session_denom{0};
    int session_id{0};
    bool fRebalanceSession{false};
    {
        LOCK(cs_coinjoin);

        // Entries belong to a session that is actually collecting them. IsSessionReady() was
        // checked by the caller before the entry was deserialized, so the state may already
        // have moved on by now.
        if (nSessionID == 0 || nState != POOL_STATE_ACCEPTING_ENTRIES) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: not accepting entries, nState=%d\n", __func__,
                     nState.load());
            nMessageIDRet = ERR_SESSION;
            return false;
        }

        if (static_cast<size_t>(GetEntriesCountLocked()) >= m_session_collaterals.size()) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: entries is full!\n", __func__);
            nMessageIDRet = ERR_ENTRIES_FULL;
            return false;
        }
        session_denom = nSessionDenom;
        session_id = nSessionID;
        fRebalanceSession = m_fRebalanceSession;

        // Entries are keyed one-to-one to the collaterals accepted at dsa time: a collateral
        // that never went through dsa acceptance cannot submit an entry and an accepted
        // collateral covers exactly one entry. Otherwise, once the ready queue is public,
        // anyone holding a valid collateral could fill entry slots (and with them the
        // declared-shape cover) belonging to the admitted participants. Don't consume the
        // collateral in either case - it may be a replay of an innocent participant's
        // collateral rather than misbehavior by its owner.
        if (!HasSessionCollateral(entry.txCollateral)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: collateral %s was not accepted into this session!\n",
                     __func__, entry.txCollateral->GetHash().ToString());
            nMessageIDRet = ERR_SESSION;
            return false;
        }
        if (std::ranges::any_of(vecEntries, hasEntryForCollateral)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: already have an entry for collateral %s!\n",
                     __func__, entry.txCollateral->GetHash().ToString());
            nMessageIDRet = ERR_ALREADY_HAVE;
            return false;
        }

        if (const auto it = m_mapDeclaredShapes.find(entry.txCollateral->GetHash()); it != m_mapDeclaredShapes.end()) {
            declaredShape = it->second;
        }
    }

    // Entry shape rules follow the session, not the current tip: m_fRebalanceSession was fixed
    // when the session was created and the participants were admitted under it. Re-deriving them
    // from the tip would let a reorg across the V24 boundary retroactively shrink the cap and
    // charge an admitted participant for an entry that was legal when it was accepted.
    //
    // Post-V24: promotion entries carry PROMOTION_RATIO (10) inputs and demotion entries
    // PROMOTION_RATIO outputs; pre-V24 entries are capped at COINJOIN_ENTRY_MAX_SIZE (9)
    const size_t nMaxEntrySize = fRebalanceSession ? static_cast<size_t>(CoinJoin::PROMOTION_RATIO) : COINJOIN_ENTRY_MAX_SIZE;
    if (entry.vecTxDSIn.size() > nMaxEntrySize || entry.vecTxOut.size() > nMaxEntrySize) {
        LogPrint(BCLog::COINJOIN, /* Continued */
                 "CCoinJoinServer::%s -- ERROR: too many inputs or outputs! inputs=%s/%s, outputs=%s/%s\n", __func__,
                 entry.vecTxDSIn.size(), nMaxEntrySize, entry.vecTxOut.size(), nMaxEntrySize);
        nMessageIDRet = ERR_MAXIMUM;
        ConsumeCollateralIfCurrentSession(session_id, entry.txCollateral);
        return false;
    }

    if (!CoinJoin::IsCollateralValid(m_chainman, m_isman, mempool, *entry.txCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: collateral not valid!\n", __func__);
        nMessageIDRet = ERR_INVALID_COLLATERAL;
        return false;
    }

    // An entry that occupies neither side of the session denomination (empty, or a shape that
    // is neither standard nor promotion/demotion) contributes nothing and must never take up
    // a participant slot - it would otherwise count toward the session denomination coverage
    // without providing any.
    if (entry.GetMixShape() == CoinJoin::MixShape::UNKNOWN) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: entry occupies no side of the session denom! inputs=%s, outputs=%s\n",
                 __func__, entry.vecTxDSIn.size(), entry.vecTxOut.size());
        nMessageIDRet = ERR_SIZE_MISMATCH;
        ConsumeCollateralIfCurrentSession(session_id, entry.txCollateral);
        return false;
    }

    // Pre-V24, unbalanced entries deliberately fall through to IsValidInOuts so their
    // collateral is consumed (anti-spam), same as before this feature existed
    if (fRebalanceSession) {
        // Every entry must match the direction its participant declared in the dsa - standard
        // entries included. Admission counted on the declared shapes to decide the session
        // covers both sides of the session denomination, so a participant deviating (e.g.
        // declaring a promotion but submitting a standard entry) could strip a side of its
        // cover and force a fee-free session reset for everyone. Deviating from the declared
        // direction therefore costs the collateral.
        if (entry.GetMixShape() != declaredShape) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: entry shape doesn't match the declared direction! inputs=%s, outputs=%s\n",
                     __func__, entry.vecTxDSIn.size(), entry.vecTxOut.size());
            nMessageIDRet = ERR_SIZE_MISMATCH;
            ConsumeCollateralIfCurrentSession(session_id, entry.txCollateral);
            return false;
        }
    }

    std::vector<CTxIn> vin;
    for (const auto& txin : entry.vecTxDSIn) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- txin=%s\n", __func__, txin.ToString());
        LOCK(cs_coinjoin);
        for (const auto& inner_entry : vecEntries) {
            if (std::ranges::any_of(inner_entry.vecTxDSIn,
                                    [&txin](const auto& txdsin) { return txdsin.prevout == txin.prevout; })) {
                LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: already have this txin in entries\n", __func__);
                nMessageIDRet = ERR_ALREADY_HAVE;
                // Two peers sent the same input? Can't really say who is the malicious one here,
                // could be that someone is picking someone else's inputs randomly trying to force
                // collateral consumption. Do not punish.
                return false;
            }
        }
        vin.emplace_back(txin);
    }

    bool fConsumeCollateral{false};
    if (!IsValidInOuts(m_chainman.ActiveChainstate(), m_isman, mempool, vin, entry.vecTxOut, session_denom,
                       /*fAllowRebalanceShapes=*/fRebalanceSession, nMessageIDRet, &fConsumeCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR! IsValidInOuts() failed: %s\n", __func__, CoinJoin::GetMessageByID(nMessageIDRet).translated);
        if (fConsumeCollateral) {
            ConsumeCollateralIfCurrentSession(session_id, entry.txCollateral);
        }
        return false;
    }

    CCoinJoinEntry admitted_entry{entry};
    {
        LOCK(cs_main);
        CCoinsViewMemPool view{&m_chainman.ActiveChainstate().CoinsTip(), mempool};
        for (auto& input : admitted_entry.vecTxDSIn) {
            Coin coin;
            if (!view.GetCoin(input.prevout, coin) || coin.IsSpent()) {
                nMessageIDRet = ERR_MISSING_TX;
                return false;
            }
            // prevPubKey is memory-only; keep the admitted script even if chainstate changes before signing.
            input.prevPubKey = coin.out.scriptPubKey;
        }
    }

    {
        LOCK(cs_coinjoin);
        // cs_coinjoin was released around the UTXO checks above, so the scheduler thread may
        // have finalized or reset the session in between; re-verify so admission stays atomic
        // with the state, membership and duplicate-use checks. Admitting into a session that
        // already built its final transaction would add an entry no one can sign, stalling it
        // until the signing timeout charges the honest participants.
        if (!IsCurrentSession(session_id) || !HasSessionCollateral(entry.txCollateral) ||
            std::ranges::any_of(vecEntries, hasEntryForCollateral)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: session changed while validating the entry!\n", __func__);
            nMessageIDRet = ERR_SESSION;
            return false;
        }
        vecEntries.push_back(std::move(admitted_entry));
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- adding entry %d of %d required\n", __func__, GetEntriesCount(), CoinJoin::GetMaxPoolParticipants());
    nMessageIDRet = MSG_ENTRIES_ADDED;

    return true;
}

bool CCoinJoinServer::AddScriptSig(const CTxIn& txinNew, const CService& sender)
{
    AssertLockHeld(cs_coinjoin);
    if (nState != POOL_STATE_SIGNING) return false;

    const auto matches_input = [&txinNew](const CTxIn& input) {
        return input.prevout == txinNew.prevout && input.nSequence == txinNew.nSequence;
    };
    const auto entry = std::ranges::find_if(vecEntries, [&](const auto& candidate) {
        return candidate.addr == sender && std::ranges::any_of(candidate.vecTxDSIn, matches_input);
    });
    if (entry == vecEntries.end()) return false;

    const auto admitted_input = std::ranges::find_if(entry->vecTxDSIn, matches_input);
    const auto final_input = std::ranges::find_if(finalMutableTransaction.vin, matches_input);
    if (final_input == finalMutableTransaction.vin.end() || admitted_input->prevPubKey.empty()) return false;
    if (admitted_input->fHasSig && admitted_input->scriptSig == txinNew.scriptSig) return true;

    const auto index = static_cast<unsigned int>(std::distance(finalMutableTransaction.vin.begin(), final_input));
    // Dash's legacy signature hash does not use the input amount.
    if (!VerifyScript(txinNew.scriptSig, admitted_input->prevPubKey,
                      STANDARD_SCRIPT_VERIFY_FLAGS | SCRIPT_VERIFY_SIGPUSHONLY,
                      MutableTransactionSignatureChecker(&finalMutableTransaction, index, 0, MissingDataBehavior::ASSERT_FAIL))) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- Invalid scriptSig\n");
        return false;
    }
    if (admitted_input->fHasSig) return true;
    if (!entry->AddScriptSig(txinNew)) return false;
    final_input->scriptSig = txinNew.scriptSig;
    return true;
}

// Check to make sure everything is signed
bool CCoinJoinServer::IsSignaturesComplete() const
{
    AssertLockHeld(cs_coinjoin);

    return std::ranges::all_of(vecEntries, [](const auto& entry) {
        return std::ranges::all_of(entry.vecTxDSIn, [](const auto& txdsin) { return txdsin.fHasSig; });
    });
}

bool CCoinJoinServer::IsAcceptableDSA(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet) const
{
    // is denom even something legit?
    if (!CoinJoin::IsValidDenomination(dsa.nDenom)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- denom not valid!\n", __func__);
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // check collateral
    if (!fUnitTest && !CoinJoin::IsCollateralValid(m_chainman, m_isman, mempool, CTransaction(dsa.txCollateral))) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- collateral not valid!\n", __func__);
        nMessageIDRet = ERR_INVALID_COLLATERAL;
        return false;
    }

    return true;
}

//! Which side of the session denomination a participant will occupy, from its declared dsa
//! direction. A participant that declares nothing is mixing 1:1 and occupies both sides.
static CoinJoin::MixShape DeclaredShape(const CCoinJoinAccept& dsa)
{
    if (dsa.IsPromotion()) return CoinJoin::MixShape::PROMOTION;
    if (dsa.IsDemotion()) return CoinJoin::MixShape::DEMOTION;
    return CoinJoin::MixShape::STANDARD;
}

CoinJoin::MixSideCounts CCoinJoinServer::GetDeclaredSideCounts() const
{
    AssertLockHeld(cs_coinjoin);
    CoinJoin::MixSideCounts counts;
    for (const auto& [_, shape] : m_mapDeclaredShapes) {
        counts.Add(shape);
    }
    return counts;
}

bool CCoinJoinServer::CreateNewSession(const CCoinJoinAccept& dsa, int nPeerVersion, PoolMessage& nMessageIDRet)
{
    if (nSessionID != 0) return false;

    // new session can only be started in idle mode
    if (nState != POOL_STATE_IDLE) {
        nMessageIDRet = ERR_MODE;
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- incompatible mode: nState=%d\n", nState);
        return false;
    }

    if (!IsAcceptableDSA(dsa, nMessageIDRet)) {
        return false;
    }

    if (!dsa.HasValidFlags()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- malformed dsa flags\n");
        nMessageIDRet = ERR_VERSION;
        return false;
    }

    // Post-V24: a promotion/demotion may only be declared once V24 is active and by a peer that
    // speaks the rebalance protocol. The session takes its capability from what this dsa asks
    // for, not from the creator's version alone - a rebalance-capable peer doing ordinary 1:1
    // mixing must not fence older clients out of the session it happens to open.
    if (dsa.IsRebalance() &&
        (nPeerVersion < COINJOIN_REBALANCE_VERSION || !CoinJoin::IsPromotionDemotionActive(m_chainman))) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- rejecting rebalance dsa, promotion/demotion not active\n");
        nMessageIDRet = ERR_VERSION;
        return false;
    }

    int nDenom{0};
    size_t nParticipants{0};
    {
        LOCK(cs_coinjoin);

        // A scheduler-thread timeout can reset the session via SetNull() between the checks
        // above and taking cs_coinjoin, so revalidate: the session state and the collateral
        // that opened it have to be committed as one unit.
        if (nSessionID != 0 || nState != POOL_STATE_IDLE) {
            nMessageIDRet = ERR_MODE;
            return false;
        }
        if (SpendsCollateralBeingCharged(dsa.txCollateral)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- collateral is being charged\n");
            nMessageIDRet = ERR_INVALID_COLLATERAL;
            return false;
        }

        // start new session
        nMessageIDRet = MSG_NOERR;
        nSessionID = GetRand<int>(/*nMax=*/999999) + 1;
        nSessionDenom = dsa.nDenom;
        m_fRebalanceSession = dsa.IsRebalance();
        m_fHasLegacyParticipant = nPeerVersion < COINJOIN_REBALANCE_VERSION;
        m_mapDeclaredShapes.emplace(dsa.txCollateral.GetHash(), DeclaredShape(dsa));

        SetState(POOL_STATE_QUEUE);

        m_session_collaterals.Add(dsa.txCollateral);
        nDenom = nSessionDenom;
        nParticipants = m_session_collaterals.size();
    }

    if (!fUnitTest) {
        //broadcast that I'm accepting entries, only if it's the first entry through
        CCoinJoinQueue dsq(nDenom, m_mn_activeman.GetOutPoint(), m_mn_activeman.GetProTxHash(), GetAdjustedTime(), false);
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- signing and relaying new queue: %s\n", dsq.ToString());
        dsq.vchSig = m_mn_activeman.SignBasic(dsq.GetSignatureHash());
        m_peer_manager->PeerRelayDSQ(dsq);
        m_queueman.AddQueue(std::move(dsq));
    }

    LogPrint(BCLog::COINJOIN, /* Continued */
             "CCoinJoinServer::CreateNewSession -- new session created, nSessionID: %d  nSessionDenom: %d (%s)  "
             "participants: %d  CoinJoin::GetMaxPoolParticipants(): %d\n",
             nSessionID, nSessionDenom, CoinJoin::DenominationToString(nSessionDenom), nParticipants,
             CoinJoin::GetMaxPoolParticipants());

    return true;
}

bool CCoinJoinServer::AddUserToExistingSession(const CCoinJoinAccept& dsa, int nPeerVersion, PoolMessage& nMessageIDRet)
{
    int session_id;
    int session_denom;
    {
        LOCK(cs_coinjoin);
        if (nSessionID == 0 || nState != POOL_STATE_QUEUE) {
            nMessageIDRet = ERR_MODE;
            return false;
        }
        if (IsSessionReady()) {
            nMessageIDRet = ERR_QUEUE_FULL;
            return false;
        }
        session_id = nSessionID;
        session_denom = nSessionDenom;
    }

    if (!IsAcceptableDSA(dsa, nMessageIDRet)) {
        return false;
    }

    if (dsa.nDenom != session_denom) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- incompatible denom %d (%s) != %d (%s)\n",
                 dsa.nDenom, CoinJoin::DenominationToString(dsa.nDenom), session_denom,
                 CoinJoin::DenominationToString(session_denom));
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // Evaluated before taking cs_coinjoin: IsPromotionDemotionActive locks cs_main, and cs_main
    // is never taken under cs_coinjoin elsewhere in this class. dsa.IsRebalance() leads so that
    // cs_main is only taken for rebalance dsas.
    const bool fRebalanceAcceptable = dsa.IsRebalance() && nPeerVersion >= COINJOIN_REBALANCE_VERSION &&
                                      CoinJoin::IsPromotionDemotionActive(m_chainman);

    LOCK(cs_coinjoin);

    if (nSessionID != session_id || nSessionDenom != session_denom || nState != POOL_STATE_QUEUE) {
        nMessageIDRet = ERR_MODE;
        return false;
    }
    if (IsSessionReady()) {
        nMessageIDRet = ERR_QUEUE_FULL;
        return false;
    }

    // Checked before the rebalance gating below, which reads dsa.IsRebalance()
    if (!dsa.HasValidFlags()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- malformed dsa flags\n");
        nMessageIDRet = ERR_VERSION;
        return false;
    }

    // Post-V24: a promotion/demotion entry and a client that cannot validate the resulting
    // unbalanced final transaction must never end up in the same session - the old client would
    // refuse to sign and risk being charged its collateral. The session commits to one or the
    // other on the first participant that forces the question, so a legacy 1:1 mixer is only
    // turned away once a rebalance participant is actually present, not because the session
    // happened to be opened by an upgraded peer. ERR_VERSION is within the message range old
    // clients understand, so they reset immediately and move on to another queue.
    if (dsa.IsRebalance()) {
        if (!fRebalanceAcceptable) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- rejecting rebalance dsa from protocol=%d, promotion/demotion not available\n",
                nPeerVersion);
            nMessageIDRet = ERR_VERSION;
            return false;
        }
        if (m_fHasLegacyParticipant) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- rejecting rebalance dsa, session holds a pre-rebalance participant\n");
            nMessageIDRet = ERR_VERSION;
            return false;
        }
    } else if (nPeerVersion < COINJOIN_REBALANCE_VERSION && m_fRebalanceSession) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- rejecting peer with protocol=%d from rebalance session\n",
            nPeerVersion);
        nMessageIDRet = ERR_VERSION;
        return false;
    }

    // IsSessionReady() can now hold a full session back waiting for a missing counterparty, so
    // the participant limit has to be enforced here rather than implied by session readiness
    if (static_cast<int>(m_session_collaterals.size()) >= CoinJoin::GetMaxPoolParticipants()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- session is full\n");
        nMessageIDRet = ERR_QUEUE_FULL;
        return false;
    }

    // A resent or replayed dsa must not be counted as a new participant; see SessionCollaterals.
    if (const auto prevout = m_session_collaterals.FindCommittedPrevout(dsa.txCollateral)) {
        LogPrint(BCLog::COINJOIN, /* Continued */
                 "CCoinJoinServer::AddUserToExistingSession -- collateral %s spends prevout %s already committed to "
                 "this session\n",
                 dsa.txCollateral.GetHash().ToString(), prevout->ToStringShort());
        nMessageIDRet = ERR_ALREADY_HAVE;
        return false;
    }
    if (SpendsCollateralBeingCharged(dsa.txCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- collateral is being charged\n");
        nMessageIDRet = ERR_INVALID_COLLATERAL;
        return false;
    }

    // count new user as accepted to an existing session

    nMessageIDRet = MSG_NOERR;
    // Latch what this participant commits the session to. The checks above guarantee the two
    // never both become true.
    m_fRebalanceSession |= dsa.IsRebalance();
    m_fHasLegacyParticipant |= nPeerVersion < COINJOIN_REBALANCE_VERSION;
    m_mapDeclaredShapes.emplace(dsa.txCollateral.GetHash(), DeclaredShape(dsa));
    m_session_collaterals.Add(dsa.txCollateral);

    LogPrint(BCLog::COINJOIN, /* Continued */
             "CCoinJoinServer::AddUserToExistingSession -- new user accepted, nSessionID: %d  nSessionDenom: %d (%s)  "
             "participants: %d  CoinJoin::GetMaxPoolParticipants(): %d\n",
             nSessionID, nSessionDenom, CoinJoin::DenominationToString(nSessionDenom), m_session_collaterals.size(),
             CoinJoin::GetMaxPoolParticipants());

    return true;
}

// Returns true if either max size has been reached or if the mix timed out and min size was reached
bool CCoinJoinServer::IsSessionReady() const
{
    AssertLockHeld(cs_coinjoin);

    if (nState == POOL_STATE_QUEUE) {
        // PRIVACY: don't start mixing until each side of the session denomination is occupied
        // by nobody or by at least two participants. A session that never attracts the missing
        // counterparty simply expires in queue state, where no collateral is charged.
        if (!GetDeclaredSideCounts().IsCovered()) return false;
        if (static_cast<int>(m_session_collaterals.size()) >= CoinJoin::GetMaxPoolParticipants()) {
            return true;
        }
        if (CCoinJoinServer::HasTimedOut() && static_cast<int>(m_session_collaterals.size()) >= CoinJoin::GetMinPoolParticipants()) {
            return true;
        }
    }
    if (nState == POOL_STATE_ACCEPTING_ENTRIES) {
        return true;
    }
    return false;
}

void CCoinJoinServer::RelayFinalTransaction(const CTransaction& txFinal)
{
    AssertLockHeld(cs_coinjoin);
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- nSessionID: %d  nSessionDenom: %d (%s)\n",
        __func__, nSessionID, nSessionDenom, CoinJoin::DenominationToString(nSessionDenom));

    // final mixing tx with empty signatures should be relayed to mixing participants only
    for (const auto& entry : vecEntries) {
        bool fOk = connman.ForNode(entry.addr, [&txFinal, this](CNode* pnode) {
            CNetMsgMaker msgMaker(pnode->GetCommonVersion());
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSFINALTX, nSessionID.load(), txFinal));
            return true;
        });
        if (!fOk) {
            // no such node? maybe this client disconnected or our own connection went down
            RelayStatus(STATUS_REJECTED);
            break;
        }
    }
}

void CCoinJoinServer::PushStatus(CNode& peer, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID) const
{
    CCoinJoinStatusUpdate psssup(nSessionID, nState, 0, nStatusUpdate, nMessageID);
    connman.PushMessage(&peer, CNetMsgMaker(peer.GetCommonVersion()).Make(NetMsgType::DSSTATUSUPDATE, psssup));
}

void CCoinJoinServer::RelayStatus(PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID)
{
    AssertLockHeld(cs_coinjoin);
    unsigned int nDisconnected{};
    // status updates should be relayed to mixing participants only
    for (const auto& entry : vecEntries) {
        // make sure everyone is still connected
        bool fOk = connman.ForNode(entry.addr, [&nStatusUpdate, &nMessageID, this](CNode* pnode) {
            PushStatus(*pnode, nStatusUpdate, nMessageID);
            return true;
        });
        if (!fOk) {
            // no such node? maybe this client disconnected or our own connection went down
            ++nDisconnected;
        }
    }
    if (nDisconnected == 0) return; // all is clear

    // something went wrong
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- can't continue, %llu client(s) disconnected, nSessionID: %d  nSessionDenom: %d (%s)\n",
        __func__, nDisconnected, nSessionID, nSessionDenom, CoinJoin::DenominationToString(nSessionDenom));

    // notify everyone else that this session should be terminated
    for (const auto& entry : vecEntries) {
        connman.ForNode(entry.addr, [this](CNode* pnode) {
            PushStatus(*pnode, STATUS_REJECTED, MSG_NOERR);
            return true;
        });
    }

    if (nDisconnected == vecEntries.size()) {
        // all clients disconnected, there is probably some issues with our own connection
        // do not charge any fees, just reset the pool
        SetNull();
    }
}

void CCoinJoinServer::RelayCompletedTransaction(int session_id, const std::vector<CService>& participants,
                                                PoolMessage nMessageID)
{
    AssertLockNotHeld(cs_coinjoin);
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- nSessionID: %d\n", __func__, session_id);

    for (const auto& addr : participants) {
        const bool fOk = connman.ForNode(addr, [&nMessageID, session_id, this](CNode* pnode) {
            CNetMsgMaker msgMaker(pnode->GetCommonVersion());
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSCOMPLETE, session_id, nMessageID));
            return true;
        });
        if (!fOk) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- participant disconnected before completion\n", __func__);
        }
    }
}

std::vector<CService> CCoinJoinServer::GetParticipantAddrs() const
{
    AssertLockHeld(cs_coinjoin);
    std::vector<CService> participants;
    participants.reserve(vecEntries.size());
    std::ranges::transform(vecEntries, std::back_inserter(participants), [](const auto& entry) { return entry.addr; });
    return participants;
}

void CCoinJoinServer::ResetSigningSessionIfCurrent(int session_id)
{
    AssertLockNotHeld(cs_coinjoin);
    LOCK(cs_coinjoin);
    if (nSessionID != session_id || nState != POOL_STATE_SIGNING) {
        LogPrint(BCLog::COINJOIN, /* Continued */
                 "CCoinJoinServer::%s -- signing session %d is no longer current, not resetting\n", __func__, session_id);
        return;
    }
    SetNull();
}

void CCoinJoinServer::SetState(PoolState nStateNew)
{
    AssertLockHeld(cs_coinjoin);

    if (nStateNew == POOL_STATE_ERROR) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::SetState -- Can't set state to ERROR as a Masternode. \n");
        return;
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::SetState -- nState: %d, nStateNew: %d\n", nState, nStateNew);
    nTimeLastSuccessfulStep = GetTime();
    nState = nStateNew;
}

void CCoinJoinServer::Schedule(CScheduler& scheduler)
{
    scheduler.scheduleEvery(
        [this]() -> void {
            if (!m_mn_sync.IsBlockchainSynced()) return;
            if (ShutdownRequested()) return;

            CheckForCompleteQueue();
            CheckPool();
            CheckTimeout();
        },
        std::chrono::seconds{1});
}

void CCoinJoinServer::GetJsonInfo(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("queue_size",    m_queueman.GetQueueSize());
    obj.pushKV("denomination",  ValueFromAmount(CoinJoin::DenominationToAmount(nSessionDenom)));
    obj.pushKV("state",         GetStateString());
    obj.pushKV("entries_count", GetEntriesCount());
}

bool CCoinJoinServer::AlreadyHave(const CInv& inv)
{
    return (inv.type == MSG_DSQ) ? m_queueman.HasQueue(inv.hash) : false;
}

bool CCoinJoinServer::ProcessGetData(CNode& pfrom, const CInv& inv, const CNetMsgMaker& msgMaker)
{
    if (inv.type != MSG_DSQ) return false;

    auto opt_dsq = m_queueman.GetQueueFromHash(inv.hash);
    if (!opt_dsq.has_value()) return false;

    connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::DSQUEUE, *opt_dsq));
    return true;
}
