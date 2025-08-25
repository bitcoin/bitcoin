// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/server.h>

#include <core_io.h>
#include <evo/deterministicmns.h>
#include <masternode/meta.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <net.h>
#include <netmessagemaker.h>
#include <net_processing.h>
#include <script/interpreter.h>
#include <shutdown.h>
#include <streams.h>
#include <txmempool.h>
#include <util/moneystr.h>
#include <util/ranges.h>
#include <util/system.h>
#include <validation.h>

#include <univalue.h>

MessageProcessingResult CCoinJoinServer::ProcessMessage(CNode& peer, std::string_view msg_type, CDataStream& vRecv)
{
    if (!m_mn_sync.IsBlockchainSynced()) return {};

    if (msg_type == NetMsgType::DSACCEPT) {
        ProcessDSACCEPT(peer, vRecv);
    } else if (msg_type == NetMsgType::DSQUEUE) {
        return ProcessDSQUEUE(peer.GetId(), vRecv);
    } else if (msg_type == NetMsgType::DSVIN) {
        ProcessDSVIN(peer, vRecv);
    } else if (msg_type == NetMsgType::DSSIGNFINALTX) {
        ProcessDSSIGNFINALTX(vRecv);
    }
    return {};
}

void CCoinJoinServer::ProcessDSACCEPT(CNode& peer, CDataStream& vRecv)
{
    assert(m_mn_metaman.IsValid());

    if (IsSessionReady()) {
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

    if (vecSessionCollaterals.empty()) {
        {
            TRY_LOCK(cs_vecqueue, lockRecv);
            if (!lockRecv) return;

            auto mnOutpoint = m_mn_activeman.GetOutPoint();

            if (ranges::any_of(vecCoinJoinQueue,
                               [&mnOutpoint](const auto& q){return q.masternodeOutpoint == mnOutpoint;})) {
                // refuse to create another queue this often
                LogPrint(BCLog::COINJOIN, "DSACCEPT -- last dsq is still in queue, refuse to mix\n");
                PushStatus(peer, STATUS_REJECTED, ERR_RECENT);
                return;
            }
        }

        int64_t nLastDsq = m_mn_metaman.GetMetaInfo(dmn->proTxHash)->GetLastDsq();
        int64_t nDsqThreshold = m_mn_metaman.GetDsqThreshold(dmn->proTxHash, mnList.GetValidMNsCount());
        if (nLastDsq != 0 && nDsqThreshold > m_mn_metaman.GetDsqCount()) {
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

    bool fResult = nSessionID == 0 ? CreateNewSession(dsa, nMessageID)
            : AddUserToExistingSession(dsa, nMessageID);
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

MessageProcessingResult CCoinJoinServer::ProcessDSQUEUE(NodeId from, CDataStream& vRecv)
{
    assert(m_mn_metaman.IsValid());

    CCoinJoinQueue dsq;
    vRecv >> dsq;

    MessageProcessingResult ret{};
    ret.m_to_erase = CInv{MSG_DSQ, dsq.GetHash()};

    if (dsq.masternodeOutpoint.IsNull() && dsq.m_protxHash.IsNull()) {
        ret.m_error = MisbehavingError{100};
        return ret;
    }

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
    if (dsq.masternodeOutpoint.IsNull()) {
        if (auto dmn = tip_mn_list.GetValidMN(dsq.m_protxHash)) {
            dsq.masternodeOutpoint = dmn->collateralOutpoint;
        } else {
            ret.m_error = MisbehavingError{10};
            return ret;
        }
    }

    {
        TRY_LOCK(cs_vecqueue, lockRecv);
        if (!lockRecv) return ret;

        // process every dsq only once
        for (const auto& q : vecCoinJoinQueue) {
            if (q == dsq) {
                return ret;
            }
            if (q.fReady == dsq.fReady && q.masternodeOutpoint == dsq.masternodeOutpoint) {
                // no way the same mn can send another dsq with the same readiness this soon
                LogPrint(BCLog::COINJOIN, "DSQUEUE -- Peer %d is sending WAY too many dsq messages for a masternode with collateral %s\n", from, dsq.masternodeOutpoint.ToStringShort());
                return ret;
            }
        }
    } // cs_vecqueue

    LogPrint(BCLog::COINJOIN, "DSQUEUE -- %s new\n", dsq.ToString());

    if (dsq.IsTimeOutOfBounds()) return ret;

    auto dmn = tip_mn_list.GetValidMNByCollateral(dsq.masternodeOutpoint);
    if (!dmn) return ret;

    if (dsq.m_protxHash.IsNull()) {
        dsq.m_protxHash = dmn->proTxHash;
    }

    if (!dsq.CheckSignature(dmn->pdmnState->pubKeyOperator.Get())) {
        ret.m_error = MisbehavingError{10};
        return ret;
    }

    if (!dsq.fReady) {
        int64_t nLastDsq = m_mn_metaman.GetMetaInfo(dmn->proTxHash)->GetLastDsq();
        int64_t nDsqThreshold = m_mn_metaman.GetDsqThreshold(dmn->proTxHash, tip_mn_list.GetValidMNsCount());
        LogPrint(BCLog::COINJOIN, "DSQUEUE -- nLastDsq: %d  nDsqThreshold: %d  nDsqCount: %d\n", nLastDsq, nDsqThreshold, m_mn_metaman.GetDsqCount());
        //don't allow a few nodes to dominate the queuing process
        if (nLastDsq != 0 && nDsqThreshold > m_mn_metaman.GetDsqCount()) {
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- node sending too many dsq messages, masternode=%s\n", dmn->proTxHash.ToString());
            return ret;
        }
        m_mn_metaman.AllowMixing(dmn->proTxHash);

        LogPrint(BCLog::COINJOIN, "DSQUEUE -- new CoinJoin queue, masternode=%s, queue=%s\n", dmn->proTxHash.ToString(), dsq.ToString());

        TRY_LOCK(cs_vecqueue, lockRecv);
        if (!lockRecv) return ret;
        vecCoinJoinQueue.push_back(dsq);
        m_peerman.RelayDSQ(dsq);
    }
    return ret;
}

void CCoinJoinServer::ProcessDSVIN(CNode& peer, CDataStream& vRecv)
{
    //do we have enough users in the current session?
    if (!IsSessionReady()) {
        LogPrint(BCLog::COINJOIN, "DSVIN -- session not complete!\n");
        PushStatus(peer, STATUS_REJECTED, ERR_SESSION);
        return;
    }

    CCoinJoinEntry entry;
    vRecv >> entry;

    LogPrint(BCLog::COINJOIN, "DSVIN -- txCollateral %s", entry.txCollateral->ToString()); /* Continued */

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

void CCoinJoinServer::ProcessDSSIGNFINALTX(CDataStream& vRecv)
{
    std::vector<CTxIn> vecTxIn;
    vRecv >> vecTxIn;

    LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- vecTxIn.size() %s\n", vecTxIn.size());

    int nTxInIndex = 0;
    int nTxInsCount = (int)vecTxIn.size();

    for (const auto& txin : vecTxIn) {
        nTxInIndex++;
        if (!AddScriptSig(txin)) {
            LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- AddScriptSig() failed at %d/%d, session: %d\n", nTxInIndex, nTxInsCount, nSessionID);
            LOCK(cs_coinjoin);
            RelayStatus(STATUS_REJECTED);
            return;
        }
        LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- AddScriptSig() %d/%d success\n", nTxInIndex, nTxInsCount);
    }
    // all is good
    CheckPool();
}

void CCoinJoinServer::SetNull()
{
    AssertLockHeld(cs_coinjoin);
    // MN side
    vecSessionCollaterals.clear();

    CCoinJoinBaseSession::SetNull();
    CCoinJoinBaseManager::SetNull();
}

//
// Check the mixing progress and send client updates if a Masternode
//
void CCoinJoinServer::CheckPool()
{
    if (int entries = GetEntriesCount(); entries != 0) LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- entries count %lu\n", entries);

    // If we have an entry for each collateral, then create final tx
    if (nState == POOL_STATE_ACCEPTING_ENTRIES && size_t(GetEntriesCount()) == vecSessionCollaterals.size()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- FINALIZE TRANSACTIONS\n");
        CreateFinalTransaction();
        return;
    }

    // Check for Time Out
    // If we timed out while accepting entries, then if we have more than minimum, create final tx
    if (nState == POOL_STATE_ACCEPTING_ENTRIES && CCoinJoinServer::HasTimedOut()
            && GetEntriesCount() >= CoinJoin::GetMinPoolParticipants()) {
        // Punish misbehaving participants
        ChargeFees();
        // Try to complete this session ignoring the misbehaving ones
        CreateFinalTransaction();
        return;
    }

    // If we have all the signatures, try to compile the transaction
    if (nState == POOL_STATE_SIGNING && IsSignaturesComplete()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- SIGNING\n");
        CommitFinalTransaction();
        return;
    }
}

void CCoinJoinServer::CreateFinalTransaction()
{
    AssertLockNotHeld(cs_coinjoin);
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- FINALIZE TRANSACTIONS\n");

    LOCK(cs_coinjoin);

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
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- finalMutableTransaction=%s", txNew.ToString()); /* Continued */

    // request signatures from clients
    SetState(POOL_STATE_SIGNING);
    RelayFinalTransaction(CTransaction(finalMutableTransaction));
}

void CCoinJoinServer::CommitFinalTransaction()
{
    AssertLockNotHeld(cs_coinjoin);

    CTransactionRef finalTransaction = WITH_LOCK(cs_coinjoin, return MakeTransactionRef(finalMutableTransaction));
    uint256 hashTx = finalTransaction->GetHash();

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- finalTransaction=%s", finalTransaction->ToString()); /* Continued */

    {
        // See if the transaction is valid
        TRY_LOCK(::cs_main, lockMain);
        mempool.PrioritiseTransaction(hashTx, 0.1 * COIN);
        if (!lockMain || !ATMPIfSaneFee(m_chainman, finalTransaction)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- ATMPIfSaneFee() error: Transaction not valid\n");
            WITH_LOCK(cs_coinjoin, SetNull());
            // not much we can do in this case, just notify clients
            RelayCompletedTransaction(ERR_INVALID_TX);
            return;
        }
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- CREATING DSTX\n");

    // create and sign masternode dstx transaction
    if (!m_dstxman.GetDSTX(hashTx)) {
        CCoinJoinBroadcastTx dstxNew(finalTransaction, m_mn_activeman.GetOutPoint(), m_mn_activeman.GetProTxHash(),
                                     GetAdjustedTime());
        dstxNew.Sign(m_mn_activeman);
        m_dstxman.AddDSTX(dstxNew);
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- TRANSMITTING DSTX\n");

    CInv inv(MSG_DSTX, hashTx);
    m_peerman.RelayInv(inv);

    // Tell the clients it was successful
    RelayCompletedTransaction(MSG_SUCCESS);

    // Randomly charge clients
    ChargeRandomFees();

    // Reset
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- COMPLETED -- RESETTING\n");
    WITH_LOCK(cs_coinjoin, SetNull());
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
void CCoinJoinServer::ChargeFees() const
{
    AssertLockNotHeld(cs_coinjoin);

    //we don't need to charge collateral for every offence.
    if (GetRand<int>(/*nMax=*/100) > 33) return;

    std::vector<CTransactionRef> vecOffendersCollaterals;

    if (nState == POOL_STATE_ACCEPTING_ENTRIES) {
        LOCK(cs_coinjoin);
        for (const auto& txCollateral : vecSessionCollaterals) {
            bool fFound = ranges::any_of(vecEntries, [&txCollateral](const auto& entry){
                return *entry.txCollateral == *txCollateral;
            });

            // This queue entry didn't send us the promised transaction
            if (!fFound) {
                LogPrint(BCLog::COINJOIN, "CCoinJoinServer::ChargeFees -- found uncooperative node (didn't send transaction), found offence\n");
                vecOffendersCollaterals.push_back(txCollateral);
            }
        }
    }

    if (nState == POOL_STATE_SIGNING) {
        // who didn't sign?
        LOCK(cs_coinjoin);
        for (const auto& entry : vecEntries) {
            for (const auto& txdsin : entry.vecTxDSIn) {
                if (!txdsin.fHasSig) {
                    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::ChargeFees -- found uncooperative node (didn't sign), found offence\n");
                    vecOffendersCollaterals.push_back(entry.txCollateral);
                }
            }
        }
    }

    // no offences found
    if (vecOffendersCollaterals.empty()) return;

    //mostly offending? Charge sometimes
    if (vecOffendersCollaterals.size() >= vecSessionCollaterals.size() - 1 && GetRand<int>(/*nMax=*/100) > 33) return;

    //everyone is an offender? That's not right
    if (vecOffendersCollaterals.size() >= vecSessionCollaterals.size()) return;

    //charge one of the offenders randomly
    Shuffle(vecOffendersCollaterals.begin(), vecOffendersCollaterals.end(), FastRandomContext());

    if (nState == POOL_STATE_ACCEPTING_ENTRIES || nState == POOL_STATE_SIGNING) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::ChargeFees -- found uncooperative node (didn't %s transaction), charging fees: %s", /* Continued */
            (nState == POOL_STATE_SIGNING) ? "sign" : "send", vecOffendersCollaterals[0]->ToString());
        ConsumeCollateral(vecOffendersCollaterals[0]);
    }
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
void CCoinJoinServer::ChargeRandomFees() const
{
    for (const auto& txCollateral : vecSessionCollaterals) {
        if (GetRand<int>(/*nMax=*/100) > 10) return;
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::ChargeRandomFees -- charging random fees, txCollateral=%s", txCollateral->ToString()); /* Continued */
        ConsumeCollateral(txCollateral);
    }
}

void CCoinJoinServer::ConsumeCollateral(const CTransactionRef& txref) const
{
    LOCK(::cs_main);
    if (!ATMPIfSaneFee(m_chainman, txref)) {
        LogPrint(BCLog::COINJOIN, "%s -- ATMPIfSaneFee failed\n", __func__);
    } else {
        m_peerman.RelayTransaction(txref->GetHash());
        LogPrint(BCLog::COINJOIN, "%s -- Collateral was consumed\n", __func__);
    }
}

bool CCoinJoinServer::HasTimedOut() const
{
    if (nState == POOL_STATE_IDLE) return false;

    int nTimeout = (nState == POOL_STATE_SIGNING) ? COINJOIN_SIGNING_TIMEOUT : COINJOIN_QUEUE_TIMEOUT;

    return GetTime() - nTimeLastSuccessfulStep >= nTimeout;
}

//
// Check for extraneous timeout
//
void CCoinJoinServer::CheckTimeout()
{
    CheckQueue();

    // Too early to do anything
    if (!CCoinJoinServer::HasTimedOut()) return;

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckTimeout -- %s timed out -- resetting\n",
        (nState == POOL_STATE_SIGNING) ? "Signing" : "Session");
    ChargeFees();
    WITH_LOCK(cs_coinjoin, SetNull());
}

/*
    Check to see if we're ready for submissions from clients
    After receiving multiple dsa messages, the queue will switch to "accepting entries"
    which is the active state right before merging the transaction
*/
void CCoinJoinServer::CheckForCompleteQueue()
{
    if (nState == POOL_STATE_QUEUE && IsSessionReady()) {
        SetState(POOL_STATE_ACCEPTING_ENTRIES);

        CCoinJoinQueue dsq(nSessionDenom, m_mn_activeman.GetOutPoint(), m_mn_activeman.GetProTxHash(),
                           GetAdjustedTime(), true);
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckForCompleteQueue -- queue is ready, signing and relaying (%s) " /* Continued */
                                     "with %d participants\n", dsq.ToString(), vecSessionCollaterals.size());
        dsq.Sign(m_mn_activeman);
        m_peerman.RelayDSQ(dsq);
        WITH_LOCK(cs_vecqueue, vecCoinJoinQueue.push_back(dsq));
    }
}

// Check to make sure a given input matches an input in the pool and its scriptSig is valid
bool CCoinJoinServer::IsInputScriptSigValid(const CTxIn& txin) const
{
    AssertLockHeld(cs_coinjoin);
    CMutableTransaction txNew;
    txNew.vin.clear();
    txNew.vout.clear();

    int nTxInIndex = -1;
    CScript sigPubKey = CScript();

    {
        int i = 0;
        for (const auto &entry: vecEntries) {
            for (const auto &txout: entry.vecTxOut) {
                txNew.vout.push_back(txout);
            }
            for (const auto &txdsin: entry.vecTxDSIn) {
                txNew.vin.push_back(txdsin);

                if (txdsin.prevout == txin.prevout) {
                    nTxInIndex = i;
                    sigPubKey = txdsin.prevPubKey;
                }
                i++;
            }
        }
    }
    if (nTxInIndex >= 0) { //might have to do this one input at a time?
        txNew.vin[nTxInIndex].scriptSig = txin.scriptSig;
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::IsInputScriptSigValid -- verifying scriptSig %s\n", ScriptToAsmStr(txin.scriptSig).substr(0, 24));
        // TODO we're using amount=0 here but we should use the correct amount. This works because Dash ignores the amount while signing/verifying (only used in Bitcoin/Segwit)
        if (!VerifyScript(txNew.vin[nTxInIndex].scriptSig, sigPubKey, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC, MutableTransactionSignatureChecker(&txNew, nTxInIndex, 0, MissingDataBehavior::ASSERT_FAIL))) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::IsInputScriptSigValid -- VerifyScript() failed on input %d\n", nTxInIndex);
            return false;
        }
    } else {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::IsInputScriptSigValid -- Failed to find matching input in pool, %s\n", txin.ToString());
        return false;
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::IsInputScriptSigValid -- Successfully validated input and scriptSig\n");
    return true;
}

//
// Add a client's transaction inputs/outputs to the pool
//
bool CCoinJoinServer::AddEntry(const CCoinJoinEntry& entry, PoolMessage& nMessageIDRet)
{
    AssertLockNotHeld(cs_coinjoin);

    if (size_t(GetEntriesCount()) >= vecSessionCollaterals.size()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: entries is full!\n", __func__);
        nMessageIDRet = ERR_ENTRIES_FULL;
        return false;
    }

    if (!CoinJoin::IsCollateralValid(m_chainman, m_isman, mempool, *entry.txCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: collateral not valid!\n", __func__);
        nMessageIDRet = ERR_INVALID_COLLATERAL;
        return false;
    }

    if (entry.vecTxDSIn.size() > COINJOIN_ENTRY_MAX_SIZE) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: too many inputs! %d/%d\n", __func__, entry.vecTxDSIn.size(), COINJOIN_ENTRY_MAX_SIZE);
        nMessageIDRet = ERR_MAXIMUM;
        ConsumeCollateral(entry.txCollateral);
        return false;
    }

    std::vector<CTxIn> vin;
    for (const auto& txin : entry.vecTxDSIn) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- txin=%s\n", __func__, txin.ToString());
        LOCK(cs_coinjoin);
        for (const auto& inner_entry : vecEntries) {
            if (ranges::any_of(inner_entry.vecTxDSIn,
                            [&txin](const auto& txdsin){
                                    return txdsin.prevout == txin.prevout;
                            })) {
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
    if (!IsValidInOuts(m_chainman.ActiveChainstate(), m_isman, mempool, vin, entry.vecTxOut, nMessageIDRet,
                       &fConsumeCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR! IsValidInOuts() failed: %s\n", __func__, CoinJoin::GetMessageByID(nMessageIDRet).translated);
        if (fConsumeCollateral) {
            ConsumeCollateral(entry.txCollateral);
        }
        return false;
    }

    WITH_LOCK(cs_coinjoin, vecEntries.push_back(entry));

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- adding entry %d of %d required\n", __func__, GetEntriesCount(), CoinJoin::GetMaxPoolParticipants());
    nMessageIDRet = MSG_ENTRIES_ADDED;

    return true;
}

bool CCoinJoinServer::AddScriptSig(const CTxIn& txinNew)
{
    AssertLockNotHeld(cs_coinjoin);
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0, 24));

    LOCK(cs_coinjoin);
    for (const auto& entry : vecEntries) {
        if (ranges::any_of(entry.vecTxDSIn,
                        [&txinNew](const auto& txdsin){ return txdsin.scriptSig == txinNew.scriptSig; })){
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- already exists\n");
            return false;
        }
    }

    if (!IsInputScriptSigValid(txinNew)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- Invalid scriptSig\n");
        return false;
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- scriptSig=%s new\n", ScriptToAsmStr(txinNew.scriptSig).substr(0, 24));

    for (auto& txin : finalMutableTransaction.vin) {
        if (txin.prevout == txinNew.prevout && txin.nSequence == txinNew.nSequence) {
            txin.scriptSig = txinNew.scriptSig;
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- adding to finalMutableTransaction, scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0, 24));
        }
    }
    for (auto& entry : vecEntries) {
        if (entry.AddScriptSig(txinNew)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- adding to entries, scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0, 24));
            return true;
        }
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- Couldn't set sig!\n");
    return false;
}

// Check to make sure everything is signed
bool CCoinJoinServer::IsSignaturesComplete() const
{
    AssertLockNotHeld(cs_coinjoin);
    LOCK(cs_coinjoin);

    return ranges::all_of(vecEntries, [](const auto& entry){
        return ranges::all_of(entry.vecTxDSIn, [](const auto& txdsin){return txdsin.fHasSig;});
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

bool CCoinJoinServer::CreateNewSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet)
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

    // start new session
    nMessageIDRet = MSG_NOERR;
    nSessionID = GetRand<int>(/*nMax=*/999999) + 1;
    nSessionDenom = dsa.nDenom;

    SetState(POOL_STATE_QUEUE);

    if (!fUnitTest) {
        //broadcast that I'm accepting entries, only if it's the first entry through
        CCoinJoinQueue dsq(nSessionDenom, m_mn_activeman.GetOutPoint(), m_mn_activeman.GetProTxHash(),
                           GetAdjustedTime(), false);
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- signing and relaying new queue: %s\n", dsq.ToString());
        dsq.Sign(m_mn_activeman);
        m_peerman.RelayDSQ(dsq);
        LOCK(cs_vecqueue);
        vecCoinJoinQueue.push_back(dsq);
    }

    vecSessionCollaterals.push_back(MakeTransactionRef(dsa.txCollateral));
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- new session created, nSessionID: %d  nSessionDenom: %d (%s)  vecSessionCollaterals.size(): %d  CoinJoin::GetMaxPoolParticipants(): %d\n",
        nSessionID, nSessionDenom, CoinJoin::DenominationToString(nSessionDenom), vecSessionCollaterals.size(), CoinJoin::GetMaxPoolParticipants());

    return true;
}

bool CCoinJoinServer::AddUserToExistingSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet)
{
    if (nSessionID == 0 || IsSessionReady()) return false;

    if (!IsAcceptableDSA(dsa, nMessageIDRet)) {
        return false;
    }

    // we only add new users to an existing session when we are in queue mode
    if (nState != POOL_STATE_QUEUE) {
        nMessageIDRet = ERR_MODE;
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- incompatible mode: nState=%d\n", nState);
        return false;
    }

    if (dsa.nDenom != nSessionDenom) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- incompatible denom %d (%s) != nSessionDenom %d (%s)\n",
            dsa.nDenom, CoinJoin::DenominationToString(dsa.nDenom), nSessionDenom, CoinJoin::DenominationToString(nSessionDenom));
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // count new user as accepted to an existing session

    nMessageIDRet = MSG_NOERR;
    vecSessionCollaterals.push_back(MakeTransactionRef(dsa.txCollateral));

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- new user accepted, nSessionID: %d  nSessionDenom: %d (%s)  vecSessionCollaterals.size(): %d  CoinJoin::GetMaxPoolParticipants(): %d\n",
        nSessionID, nSessionDenom, CoinJoin::DenominationToString(nSessionDenom), vecSessionCollaterals.size(), CoinJoin::GetMaxPoolParticipants());

    return true;
}

// Returns true if either max size has been reached or if the mix timed out and min size was reached
bool CCoinJoinServer::IsSessionReady() const
{
    if (nState == POOL_STATE_QUEUE) {
        if ((int)vecSessionCollaterals.size() >= CoinJoin::GetMaxPoolParticipants()) {
            return true;
        }
        if (CCoinJoinServer::HasTimedOut() && (int)vecSessionCollaterals.size() >= CoinJoin::GetMinPoolParticipants()) {
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

void CCoinJoinServer::RelayCompletedTransaction(PoolMessage nMessageID)
{
    AssertLockNotHeld(cs_coinjoin);
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- nSessionID: %d  nSessionDenom: %d (%s)\n",
        __func__, nSessionID, nSessionDenom, CoinJoin::DenominationToString(nSessionDenom));

    // final mixing tx with empty signatures should be relayed to mixing participants only
    LOCK(cs_coinjoin);
    for (const auto& entry : vecEntries) {
        bool fOk = connman.ForNode(entry.addr, [&nMessageID, this](CNode* pnode) {
            CNetMsgMaker msgMaker(pnode->GetCommonVersion());
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSCOMPLETE, nSessionID.load(), nMessageID));
            return true;
        });
        if (!fOk) {
            // no such node? maybe client disconnected or our own connection went down
            RelayStatus(STATUS_REJECTED);
            break;
        }
    }
}

void CCoinJoinServer::SetState(PoolState nStateNew)
{
    if (nStateNew == POOL_STATE_ERROR) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::SetState -- Can't set state to ERROR as a Masternode. \n");
        return;
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::SetState -- nState: %d, nStateNew: %d\n", nState, nStateNew);
    nTimeLastSuccessfulStep = GetTime();
    nState = nStateNew;
}

void CCoinJoinServer::DoMaintenance()
{
    if (!m_mn_sync.IsBlockchainSynced()) return;
    if (ShutdownRequested()) return;

    CheckForCompleteQueue();
    CheckPool();
    CheckTimeout();
}

void CCoinJoinServer::GetJsonInfo(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("queue_size",    GetQueueSize());
    obj.pushKV("denomination",  ValueFromAmount(CoinJoin::DenominationToAmount(nSessionDenom)));
    obj.pushKV("state",         GetStateString());
    obj.pushKV("entries_count", GetEntriesCount());
}
