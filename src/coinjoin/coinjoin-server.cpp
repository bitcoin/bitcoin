// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/coinjoin-server.h>

#include <masternode/activemasternode.h>
#include <evo/deterministicmns.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <masternode/masternode-meta.h>
#include <masternode/masternode-sync.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <script/interpreter.h>
#include <shutdown.h>
#include <txmempool.h>
#include <util/system.h>
#include <util/moneystr.h>
#include <validation.h>
#include <version.h>

#include <llmq/quorums_instantsend.h>

#include <univalue.h>

CCoinJoinServer coinJoinServer;

void CCoinJoinServer::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, bool enable_bip61)
{
    if (!fMasternodeMode) return;
    if (!masternodeSync.IsBlockchainSynced()) return;

    if (strCommand == NetMsgType::DSACCEPT) {
        if (pfrom->nVersion < MIN_COINJOIN_PEER_PROTO_VERSION) {
            LogPrint(BCLog::COINJOIN, "DSACCEPT -- peer=%d using obsolete version %i\n", pfrom->GetId(), pfrom->nVersion);
            if (enable_bip61) {
                connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand,
                                                                                      REJECT_OBSOLETE, strprintf(
                                "Version must be %d or greater", MIN_COINJOIN_PEER_PROTO_VERSION)));
            }
            PushStatus(pfrom, STATUS_REJECTED, ERR_VERSION, connman);
            return;
        }

        if (IsSessionReady()) {
            // too many users in this session already, reject new ones
            LogPrint(BCLog::COINJOIN, "DSACCEPT -- queue is already full!\n");
            PushStatus(pfrom, STATUS_REJECTED, ERR_QUEUE_FULL, connman);
            return;
        }

        CCoinJoinAccept dsa;
        vRecv >> dsa;

        LogPrint(BCLog::COINJOIN, "DSACCEPT -- nDenom %d (%s)  txCollateral %s", dsa.nDenom, CCoinJoin::DenominationToString(dsa.nDenom), dsa.txCollateral.ToString()); /* Continued */

        auto mnList = deterministicMNManager->GetListAtChainTip();
        auto dmn = WITH_LOCK(activeMasternodeInfoCs, return mnList.GetValidMNByCollateral(activeMasternodeInfo.outpoint));
        if (!dmn) {
            PushStatus(pfrom, STATUS_REJECTED, ERR_MN_LIST, connman);
            return;
        }

        if (vecSessionCollaterals.empty()) {
            {
                TRY_LOCK(cs_vecqueue, lockRecv);
                if (!lockRecv) return;

                for (const auto& q : vecCoinJoinQueue) {
                    if (WITH_LOCK(activeMasternodeInfoCs, return q.masternodeOutpoint == activeMasternodeInfo.outpoint)) {
                        // refuse to create another queue this often
                        LogPrint(BCLog::COINJOIN, "DSACCEPT -- last dsq is still in queue, refuse to mix\n");
                        PushStatus(pfrom, STATUS_REJECTED, ERR_RECENT, connman);
                        return;
                    }
                }
            }

            int64_t nLastDsq = mmetaman.GetMetaInfo(dmn->proTxHash)->GetLastDsq();
            int64_t nDsqThreshold = mmetaman.GetDsqThreshold(dmn->proTxHash, mnList.GetValidMNsCount());
            if (nLastDsq != 0 && nDsqThreshold > mmetaman.GetDsqCount()) {
                if (fLogIPs) {
                    LogPrint(BCLog::COINJOIN, "DSACCEPT -- last dsq too recent, must wait: peer=%d, addr=%s\n", pfrom->GetId(), pfrom->addr.ToString());
                } else {
                    LogPrint(BCLog::COINJOIN, "DSACCEPT -- last dsq too recent, must wait: peer=%d\n", pfrom->GetId());
                }
                PushStatus(pfrom, STATUS_REJECTED, ERR_RECENT, connman);
                return;
            }
        }

        PoolMessage nMessageID = MSG_NOERR;

        bool fResult = nSessionID == 0 ? CreateNewSession(dsa, nMessageID, connman)
                                       : AddUserToExistingSession(dsa, nMessageID);
        if (fResult) {
            LogPrint(BCLog::COINJOIN, "DSACCEPT -- is compatible, please submit!\n");
            PushStatus(pfrom, STATUS_ACCEPTED, nMessageID, connman);
            return;
        } else {
            LogPrint(BCLog::COINJOIN, "DSACCEPT -- not compatible with existing transactions!\n");
            PushStatus(pfrom, STATUS_REJECTED, nMessageID, connman);
            return;
        }

    } else if (strCommand == NetMsgType::DSQUEUE) {
        if (pfrom->nVersion < MIN_COINJOIN_PEER_PROTO_VERSION) {
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- peer=%d using obsolete version %i\n", pfrom->GetId(), pfrom->nVersion);
            if (enable_bip61) {
                connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand,
                                                                                      REJECT_OBSOLETE, strprintf(
                                "Version must be %d or greater", MIN_COINJOIN_PEER_PROTO_VERSION)));
            }
            return;
        }

        CCoinJoinQueue dsq;
        vRecv >> dsq;

        {
            TRY_LOCK(cs_vecqueue, lockRecv);
            if (!lockRecv) return;

            // process every dsq only once
            for (const auto& q : vecCoinJoinQueue) {
                if (q == dsq) {
                    return;
                }
                if (q.fReady == dsq.fReady && q.masternodeOutpoint == dsq.masternodeOutpoint) {
                    // no way the same mn can send another dsq with the same readiness this soon
                    LogPrint(BCLog::COINJOIN, "DSQUEUE -- Peer %s is sending WAY too many dsq messages for a masternode with collateral %s\n", pfrom->GetLogString(), dsq.masternodeOutpoint.ToStringShort());
                    return;
                }
            }
        } // cs_vecqueue

        LogPrint(BCLog::COINJOIN, "DSQUEUE -- %s new\n", dsq.ToString());

        if (dsq.IsTimeOutOfBounds()) return;

        auto mnList = deterministicMNManager->GetListAtChainTip();
        auto dmn = mnList.GetValidMNByCollateral(dsq.masternodeOutpoint);
        if (!dmn) return;

        if (!dsq.CheckSignature(dmn->pdmnState->pubKeyOperator.Get())) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 10);
            return;
        }

        if (!dsq.fReady) {
            int64_t nLastDsq = mmetaman.GetMetaInfo(dmn->proTxHash)->GetLastDsq();
            int64_t nDsqThreshold = mmetaman.GetDsqThreshold(dmn->proTxHash, mnList.GetValidMNsCount());
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- nLastDsq: %d  nDsqThreshold: %d  nDsqCount: %d\n", nLastDsq, nDsqThreshold, mmetaman.GetDsqCount());
            //don't allow a few nodes to dominate the queuing process
            if (nLastDsq != 0 && nDsqThreshold > mmetaman.GetDsqCount()) {
                LogPrint(BCLog::COINJOIN, "DSQUEUE -- Masternode %s is sending too many dsq messages\n", dmn->pdmnState->addr.ToString());
                return;
            }
            mmetaman.AllowMixing(dmn->proTxHash);

            LogPrint(BCLog::COINJOIN, "DSQUEUE -- new CoinJoin queue (%s) from masternode %s\n", dsq.ToString(), dmn->pdmnState->addr.ToString());

            TRY_LOCK(cs_vecqueue, lockRecv);
            if (!lockRecv) return;
            vecCoinJoinQueue.push_back(dsq);
            dsq.Relay(connman);
        }

    } else if (strCommand == NetMsgType::DSVIN) {
        if (pfrom->nVersion < MIN_COINJOIN_PEER_PROTO_VERSION) {
            LogPrint(BCLog::COINJOIN, "DSVIN -- peer=%d using obsolete version %i\n", pfrom->GetId(), pfrom->nVersion);
            if (enable_bip61) {
                connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand,
                                                                                      REJECT_OBSOLETE, strprintf(
                                "Version must be %d or greater", MIN_COINJOIN_PEER_PROTO_VERSION)));
            }
            PushStatus(pfrom, STATUS_REJECTED, ERR_VERSION, connman);
            return;
        }

        //do we have enough users in the current session?
        if (!IsSessionReady()) {
            LogPrint(BCLog::COINJOIN, "DSVIN -- session not complete!\n");
            PushStatus(pfrom, STATUS_REJECTED, ERR_SESSION, connman);
            return;
        }

        CCoinJoinEntry entry;
        vRecv >> entry;

        LogPrint(BCLog::COINJOIN, "DSVIN -- txCollateral %s", entry.txCollateral->ToString()); /* Continued */

        PoolMessage nMessageID = MSG_NOERR;

        entry.addr = pfrom->addr;
        if (AddEntry(connman, entry, nMessageID)) {
            PushStatus(pfrom, STATUS_ACCEPTED, nMessageID, connman);
            CheckPool(connman);
            RelayStatus(STATUS_ACCEPTED, connman);
        } else {
            PushStatus(pfrom, STATUS_REJECTED, nMessageID, connman);
        }

    } else if (strCommand == NetMsgType::DSSIGNFINALTX) {
        if (pfrom->nVersion < MIN_COINJOIN_PEER_PROTO_VERSION) {
            LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- peer=%d using obsolete version %i\n", pfrom->GetId(), pfrom->nVersion);
            if (enable_bip61) {
                connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand,
                                                                                      REJECT_OBSOLETE, strprintf(
                                "Version must be %d or greater", MIN_COINJOIN_PEER_PROTO_VERSION)));
            }
            return;
        }

        std::vector<CTxIn> vecTxIn;
        vRecv >> vecTxIn;

        LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- vecTxIn.size() %s\n", vecTxIn.size());

        int nTxInIndex = 0;
        int nTxInsCount = (int)vecTxIn.size();

        for (const auto& txin : vecTxIn) {
            nTxInIndex++;
            if (!AddScriptSig(txin)) {
                LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- AddScriptSig() failed at %d/%d, session: %d\n", nTxInIndex, nTxInsCount, nSessionID);
                RelayStatus(STATUS_REJECTED, connman);
                return;
            }
            LogPrint(BCLog::COINJOIN, "DSSIGNFINALTX -- AddScriptSig() %d/%d success\n", nTxInIndex, nTxInsCount);
        }
        // all is good
        CheckPool(connman);
    }
}

void CCoinJoinServer::SetNull()
{
    // MN side
    vecSessionCollaterals.clear();

    CCoinJoinBaseSession::SetNull();
    CCoinJoinBaseManager::SetNull();
}

//
// Check the mixing progress and send client updates if a Masternode
//
void CCoinJoinServer::CheckPool(CConnman& connman)
{
    if (!fMasternodeMode) return;

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- entries count %lu\n", GetEntriesCount());

    // If we have an entry for each collateral, then create final tx
    if (nState == POOL_STATE_ACCEPTING_ENTRIES && GetEntriesCount() == vecSessionCollaterals.size()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- FINALIZE TRANSACTIONS\n");
        CreateFinalTransaction(connman);
        return;
    }

    // Check for Time Out
    // If we timed out while accepting entries, then if we have more than minimum, create final tx
    if (nState == POOL_STATE_ACCEPTING_ENTRIES && CCoinJoinServer::HasTimedOut()
            && GetEntriesCount() >= CCoinJoin::GetMinPoolParticipants()) {
        // Punish misbehaving participants
        ChargeFees(connman);
        // Try to complete this session ignoring the misbehaving ones
        CreateFinalTransaction(connman);
        return;
    }

    // If we have all of the signatures, try to compile the transaction
    if (nState == POOL_STATE_SIGNING && IsSignaturesComplete()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckPool -- SIGNING\n");
        CommitFinalTransaction(connman);
        return;
    }
}

void CCoinJoinServer::CreateFinalTransaction(CConnman& connman)
{
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- FINALIZE TRANSACTIONS\n");

    CMutableTransaction txNew;

    // make our new transaction
    for (int i = 0; i < GetEntriesCount(); i++) {
        for (const auto& txout : vecEntries[i].vecTxOut) {
            txNew.vout.push_back(txout);
        }
        for (const auto& txdsin : vecEntries[i].vecTxDSIn) {
            txNew.vin.push_back(txdsin);
        }
    }

    sort(txNew.vin.begin(), txNew.vin.end(), CompareInputBIP69());
    sort(txNew.vout.begin(), txNew.vout.end(), CompareOutputBIP69());

    finalMutableTransaction = txNew;
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateFinalTransaction -- finalMutableTransaction=%s", txNew.ToString()); /* Continued */

    // request signatures from clients
    SetState(POOL_STATE_SIGNING);
    RelayFinalTransaction(finalMutableTransaction, connman);
}

void CCoinJoinServer::CommitFinalTransaction(CConnman& connman)
{
    if (!fMasternodeMode) return; // check and relay final tx only on masternode

    CTransactionRef finalTransaction = MakeTransactionRef(finalMutableTransaction);
    uint256 hashTx = finalTransaction->GetHash();

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- finalTransaction=%s", finalTransaction->ToString()); /* Continued */

    {
        // See if the transaction is valid
        TRY_LOCK(cs_main, lockMain);
        CValidationState validationState;
        mempool.PrioritiseTransaction(hashTx, 0.1 * COIN);
        if (!lockMain || !AcceptToMemoryPool(mempool, validationState, finalTransaction, nullptr /* pfMissingInputs */, false /* bypass_limits */, maxTxFee /* nAbsurdFee */)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- AcceptToMemoryPool() error: Transaction not valid\n");
            SetNull();
            // not much we can do in this case, just notify clients
            RelayCompletedTransaction(ERR_INVALID_TX, connman);
            return;
        }
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- CREATING DSTX\n");

    // create and sign masternode dstx transaction
    if (!CCoinJoin::GetDSTX(hashTx)) {
        CCoinJoinBroadcastTx dstxNew(finalTransaction, WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.outpoint), GetAdjustedTime());
        dstxNew.Sign();
        CCoinJoin::AddDSTX(dstxNew);
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- TRANSMITTING DSTX\n");

    CInv inv(MSG_DSTX, hashTx);
    connman.RelayInv(inv);

    // Tell the clients it was successful
    RelayCompletedTransaction(MSG_SUCCESS, connman);

    // Randomly charge clients
    ChargeRandomFees(connman);

    // Reset
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CommitFinalTransaction -- COMPLETED -- RESETTING\n");
    SetNull();
}

//
// Charge clients a fee if they're abusive
//
// Why bother? CoinJoin uses collateral to ensure abuse to the process is kept to a minimum.
// The submission and signing stages are completely separate. In the cases where
// a client submits a transaction then refused to sign, there must be a cost. Otherwise they
// would be able to do this over and over again and bring the mixing to a halt.
//
// How does this work? Messages to Masternodes come in via NetMsgType::DSVIN, these require a valid collateral
// transaction for the client to be able to enter the pool. This transaction is kept by the Masternode
// until the transaction is either complete or fails.
//
void CCoinJoinServer::ChargeFees(CConnman& connman)
{
    if (!fMasternodeMode) return;

    //we don't need to charge collateral for every offence.
    if (GetRandInt(100) > 33) return;

    std::vector<CTransactionRef> vecOffendersCollaterals;

    if (nState == POOL_STATE_ACCEPTING_ENTRIES) {
        for (const auto& txCollateral : vecSessionCollaterals) {
            bool fFound = false;
            for (const auto& entry : vecEntries) {
                if (*entry.txCollateral == *txCollateral) {
                    fFound = true;
                    break;
                }
            }

            // This queue entry didn't send us the promised transaction
            if (!fFound) {
                LogPrint(BCLog::COINJOIN, "CCoinJoinServer::ChargeFees -- found uncooperative node (didn't send transaction), found offence\n");
                vecOffendersCollaterals.push_back(txCollateral);
            }
        }
    }

    if (nState == POOL_STATE_SIGNING) {
        // who didn't sign?
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
    if ((int)vecOffendersCollaterals.size() >= vecSessionCollaterals.size() - 1 && GetRandInt(100) > 33) return;

    //everyone is an offender? That's not right
    if ((int)vecOffendersCollaterals.size() >= vecSessionCollaterals.size()) return;

    //charge one of the offenders randomly
    Shuffle(vecOffendersCollaterals.begin(), vecOffendersCollaterals.end(), FastRandomContext());

    if (nState == POOL_STATE_ACCEPTING_ENTRIES || nState == POOL_STATE_SIGNING) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::ChargeFees -- found uncooperative node (didn't %s transaction), charging fees: %s", /* Continued */
            (nState == POOL_STATE_SIGNING) ? "sign" : "send", vecOffendersCollaterals[0]->ToString());
        ConsumeCollateral(connman, vecOffendersCollaterals[0]);
    }
}

/*
    Charge the collateral randomly.
    Mixing is completely free, to pay miners we randomly pay the collateral of users.

    Collateral Fee Charges:

    Being that mixing has "no fees" we need to have some kind of cost associated
    with using it to stop abuse. Otherwise it could serve as an attack vector and
    allow endless transaction that would bloat Dash and make it unusable. To
    stop these kinds of attacks 1 in 10 successful transactions are charged. This
    adds up to a cost of 0.001DRK per transaction on average.
*/
void CCoinJoinServer::ChargeRandomFees(CConnman& connman)
{
    if (!fMasternodeMode) return;

    for (const auto& txCollateral : vecSessionCollaterals) {
        if (GetRandInt(100) > 10) return;
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::ChargeRandomFees -- charging random fees, txCollateral=%s", txCollateral->ToString()); /* Continued */
        ConsumeCollateral(connman, txCollateral);
    }
}

void CCoinJoinServer::ConsumeCollateral(CConnman& connman, const CTransactionRef& txref)
{
    LOCK(cs_main);
    CValidationState validationState;
    if (!AcceptToMemoryPool(mempool, validationState, txref, nullptr /* pfMissingInputs */, false /* bypass_limits */, 0 /* nAbsurdFee */)) {
        LogPrint(BCLog::COINJOIN, "%s -- AcceptToMemoryPool failed\n", __func__);
    } else {
        connman.RelayTransaction(*txref);
        LogPrint(BCLog::COINJOIN, "%s -- Collateral was consumed\n", __func__);
    }
}

bool CCoinJoinServer::HasTimedOut()
{
    if (!fMasternodeMode) return false;

    if (nState == POOL_STATE_IDLE) return false;

    int nTimeout = (nState == POOL_STATE_SIGNING) ? COINJOIN_SIGNING_TIMEOUT : COINJOIN_QUEUE_TIMEOUT;

    return GetTime() - nTimeLastSuccessfulStep >= nTimeout;
}

//
// Check for extraneous timeout
//
void CCoinJoinServer::CheckTimeout(CConnman& connman)
{
    if (!fMasternodeMode) return;

    CheckQueue();

    // Too early to do anything
    if (!CCoinJoinServer::HasTimedOut()) return;

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckTimeout -- %s timed out -- resetting\n",
        (nState == POOL_STATE_SIGNING) ? "Signing" : "Session");
    ChargeFees(connman);
    SetNull();
}

/*
    Check to see if we're ready for submissions from clients
    After receiving multiple dsa messages, the queue will switch to "accepting entries"
    which is the active state right before merging the transaction
*/
void CCoinJoinServer::CheckForCompleteQueue(CConnman& connman)
{
    if (!fMasternodeMode) return;

    if (nState == POOL_STATE_QUEUE && IsSessionReady()) {
        SetState(POOL_STATE_ACCEPTING_ENTRIES);

        CCoinJoinQueue dsq(nSessionDenom, WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.outpoint), GetAdjustedTime(), true);
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CheckForCompleteQueue -- queue is ready, signing and relaying (%s) " /* Continued */
                                     "with %d participants\n", dsq.ToString(), vecSessionCollaterals.size());
        dsq.Sign();
        dsq.Relay(connman);
    }
}

// Check to make sure a given input matches an input in the pool and its scriptSig is valid
bool CCoinJoinServer::IsInputScriptSigValid(const CTxIn& txin)
{
    CMutableTransaction txNew;
    txNew.vin.clear();
    txNew.vout.clear();

    int i = 0;
    int nTxInIndex = -1;
    CScript sigPubKey = CScript();

    for (const auto& entry : vecEntries) {
        for (const auto& txout : entry.vecTxOut) {
            txNew.vout.push_back(txout);
        }
        for (const auto& txdsin : entry.vecTxDSIn) {
            txNew.vin.push_back(txdsin);

            if (txdsin.prevout == txin.prevout) {
                nTxInIndex = i;
                sigPubKey = txdsin.prevPubKey;
            }
            i++;
        }
    }

    if (nTxInIndex >= 0) { //might have to do this one input at a time?
        txNew.vin[nTxInIndex].scriptSig = txin.scriptSig;
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::IsInputScriptSigValid -- verifying scriptSig %s\n", ScriptToAsmStr(txin.scriptSig).substr(0, 24));
        // TODO we're using amount=0 here but we should use the correct amount. This works because Dash ignores the amount while signing/verifying (only used in Bitcoin/Segwit)
        if (!VerifyScript(txNew.vin[nTxInIndex].scriptSig, sigPubKey, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC, MutableTransactionSignatureChecker(&txNew, nTxInIndex, 0))) {
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
bool CCoinJoinServer::AddEntry(CConnman& connman, const CCoinJoinEntry& entry, PoolMessage& nMessageIDRet)
{
    if (!fMasternodeMode) return false;

    if (GetEntriesCount() >= vecSessionCollaterals.size()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: entries is full!\n", __func__);
        nMessageIDRet = ERR_ENTRIES_FULL;
        return false;
    }

    if (!CCoinJoin::IsCollateralValid(*entry.txCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: collateral not valid!\n", __func__);
        nMessageIDRet = ERR_INVALID_COLLATERAL;
        return false;
    }

    if (entry.vecTxDSIn.size() > COINJOIN_ENTRY_MAX_SIZE) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: too many inputs! %d/%d\n", __func__, entry.vecTxDSIn.size(), COINJOIN_ENTRY_MAX_SIZE);
        nMessageIDRet = ERR_MAXIMUM;
        ConsumeCollateral(connman, entry.txCollateral);
        return false;
    }

    std::vector<CTxIn> vin;
    for (const auto& txin : entry.vecTxDSIn) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- txin=%s\n", __func__, txin.ToString());

        for (const auto& entry : vecEntries) {
            for (const auto& txdsin : entry.vecTxDSIn) {
                if (txdsin.prevout == txin.prevout) {
                    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR: already have this txin in entries\n", __func__);
                    nMessageIDRet = ERR_ALREADY_HAVE;
                    // Two peers sent the same input? Can't really say who is the malicious one here,
                    // could be that someone is picking someone else's inputs randomly trying to force
                    // collateral consumption. Do not punish.
                    return false;
                }
            }
        }
        vin.emplace_back(txin);
    }

    bool fConsumeCollateral{false};
    if (!IsValidInOuts(vin, entry.vecTxOut, nMessageIDRet, &fConsumeCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- ERROR! IsValidInOuts() failed: %s\n", __func__, CCoinJoin::GetMessageByID(nMessageIDRet));
        if (fConsumeCollateral) {
            ConsumeCollateral(connman, entry.txCollateral);
        }
        return false;
    }

    vecEntries.push_back(entry);

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- adding entry %d of %d required\n", __func__, GetEntriesCount(), CCoinJoin::GetMaxPoolParticipants());
    nMessageIDRet = MSG_ENTRIES_ADDED;

    return true;
}

bool CCoinJoinServer::AddScriptSig(const CTxIn& txinNew)
{
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0, 24));

    for (const auto& entry : vecEntries) {
        for (const auto& txdsin : entry.vecTxDSIn) {
            if (txdsin.scriptSig == txinNew.scriptSig) {
                LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- already exists\n");
                return false;
            }
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
    for (int i = 0; i < GetEntriesCount(); i++) {
        if (vecEntries[i].AddScriptSig(txinNew)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- adding to entries, scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0, 24));
            return true;
        }
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddScriptSig -- Couldn't set sig!\n");
    return false;
}

// Check to make sure everything is signed
bool CCoinJoinServer::IsSignaturesComplete()
{
    for (const auto& entry : vecEntries) {
        for (const auto& txdsin : entry.vecTxDSIn) {
            if (!txdsin.fHasSig) return false;
        }
    }

    return true;
}

bool CCoinJoinServer::IsAcceptableDSA(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet)
{
    if (!fMasternodeMode) return false;

    // is denom even something legit?
    if (!CCoinJoin::IsValidDenomination(dsa.nDenom)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- denom not valid!\n", __func__);
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // check collateral
    if (!fUnitTest && !CCoinJoin::IsCollateralValid(dsa.txCollateral)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- collateral not valid!\n", __func__);
        nMessageIDRet = ERR_INVALID_COLLATERAL;
        return false;
    }

    return true;
}

bool CCoinJoinServer::CreateNewSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet, CConnman& connman)
{
    if (!fMasternodeMode || nSessionID != 0) return false;

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
    nSessionID = GetRandInt(999999) + 1;
    nSessionDenom = dsa.nDenom;

    SetState(POOL_STATE_QUEUE);

    if (!fUnitTest) {
        //broadcast that I'm accepting entries, only if it's the first entry through
        CCoinJoinQueue dsq(nSessionDenom, WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.outpoint), GetAdjustedTime(), false);
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- signing and relaying new queue: %s\n", dsq.ToString());
        dsq.Sign();
        dsq.Relay(connman);
        vecCoinJoinQueue.push_back(dsq);
    }

    vecSessionCollaterals.push_back(MakeTransactionRef(dsa.txCollateral));
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::CreateNewSession -- new session created, nSessionID: %d  nSessionDenom: %d (%s)  vecSessionCollaterals.size(): %d  CCoinJoin::GetMaxPoolParticipants(): %d\n",
        nSessionID, nSessionDenom, CCoinJoin::DenominationToString(nSessionDenom), vecSessionCollaterals.size(), CCoinJoin::GetMaxPoolParticipants());

    return true;
}

bool CCoinJoinServer::AddUserToExistingSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet)
{
    if (!fMasternodeMode || nSessionID == 0 || IsSessionReady()) return false;

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
            dsa.nDenom, CCoinJoin::DenominationToString(dsa.nDenom), nSessionDenom, CCoinJoin::DenominationToString(nSessionDenom));
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // count new user as accepted to an existing session

    nMessageIDRet = MSG_NOERR;
    vecSessionCollaterals.push_back(MakeTransactionRef(dsa.txCollateral));

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::AddUserToExistingSession -- new user accepted, nSessionID: %d  nSessionDenom: %d (%s)  vecSessionCollaterals.size(): %d  CCoinJoin::GetMaxPoolParticipants(): %d\n",
        nSessionID, nSessionDenom, CCoinJoin::DenominationToString(nSessionDenom), vecSessionCollaterals.size(), CCoinJoin::GetMaxPoolParticipants());

    return true;
}

// Returns true if either max size has been reached or if the mix timed out and min size was reached
bool CCoinJoinServer::IsSessionReady()
{
    if (nState == POOL_STATE_QUEUE) {
        if ((int)vecSessionCollaterals.size() >= CCoinJoin::GetMaxPoolParticipants()) {
            return true;
        }
        if (CCoinJoinServer::HasTimedOut() && (int)vecSessionCollaterals.size() >= CCoinJoin::GetMinPoolParticipants()) {
            return true;
        }
    }
    if (nState == POOL_STATE_ACCEPTING_ENTRIES) {
        return true;
    }
    return false;
}

void CCoinJoinServer::RelayFinalTransaction(const CTransaction& txFinal, CConnman& connman)
{
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- nSessionID: %d  nSessionDenom: %d (%s)\n",
        __func__, nSessionID, nSessionDenom, CCoinJoin::DenominationToString(nSessionDenom));

    // final mixing tx with empty signatures should be relayed to mixing participants only
    for (const auto& entry : vecEntries) {
        bool fOk = connman.ForNode(entry.addr, [&txFinal, &connman, this](CNode* pnode) {
            CNetMsgMaker msgMaker(pnode->GetSendVersion());
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSFINALTX, nSessionID, txFinal));
            return true;
        });
        if (!fOk) {
            // no such node? maybe this client disconnected or our own connection went down
            RelayStatus(STATUS_REJECTED, connman);
            break;
        }
    }
}

void CCoinJoinServer::PushStatus(CNode* pnode, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID, CConnman& connman)
{
    if (!pnode) return;
    CCoinJoinStatusUpdate psssup(nSessionID, nState, 0, nStatusUpdate, nMessageID);
    connman.PushMessage(pnode, CNetMsgMaker(pnode->GetSendVersion()).Make(NetMsgType::DSSTATUSUPDATE, psssup));
}

void CCoinJoinServer::RelayStatus(PoolStatusUpdate nStatusUpdate, CConnman& connman, PoolMessage nMessageID)
{
    unsigned int nDisconnected{};
    // status updates should be relayed to mixing participants only
    for (const auto& entry : vecEntries) {
        // make sure everyone is still connected
        bool fOk = connman.ForNode(entry.addr, [&nStatusUpdate, &nMessageID, &connman, this](CNode* pnode) {
            PushStatus(pnode, nStatusUpdate, nMessageID, connman);
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
        __func__, nDisconnected, nSessionID, nSessionDenom, CCoinJoin::DenominationToString(nSessionDenom));

    // notify everyone else that this session should be terminated
    for (const auto& entry : vecEntries) {
        connman.ForNode(entry.addr, [&connman, this](CNode* pnode) {
            PushStatus(pnode, STATUS_REJECTED, MSG_NOERR, connman);
            return true;
        });
    }

    if (nDisconnected == vecEntries.size()) {
        // all clients disconnected, there is probably some issues with our own connection
        // do not charge any fees, just reset the pool
        SetNull();
    }
}

void CCoinJoinServer::RelayCompletedTransaction(PoolMessage nMessageID, CConnman& connman)
{
    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::%s -- nSessionID: %d  nSessionDenom: %d (%s)\n",
        __func__, nSessionID, nSessionDenom, CCoinJoin::DenominationToString(nSessionDenom));

    // final mixing tx with empty signatures should be relayed to mixing participants only
    for (const auto& entry : vecEntries) {
        bool fOk = connman.ForNode(entry.addr, [&nMessageID, &connman, this](CNode* pnode) {
            CNetMsgMaker msgMaker(pnode->GetSendVersion());
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSCOMPLETE, nSessionID, nMessageID));
            return true;
        });
        if (!fOk) {
            // no such node? maybe client disconnected or our own connection went down
            RelayStatus(STATUS_REJECTED, connman);
            break;
        }
    }
}

void CCoinJoinServer::SetState(PoolState nStateNew)
{
    if (!fMasternodeMode) return;

    if (nStateNew == POOL_STATE_ERROR) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinServer::SetState -- Can't set state to ERROR as a Masternode. \n");
        return;
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinServer::SetState -- nState: %d, nStateNew: %d\n", nState, nStateNew);
    nTimeLastSuccessfulStep = GetTime();
    nState = nStateNew;
}

void CCoinJoinServer::DoMaintenance(CConnman& connman)
{
    if (!fMasternodeMode) return; // only run on masternodes

    if (!masternodeSync.IsBlockchainSynced() || ShutdownRequested()) return;

    coinJoinServer.CheckForCompleteQueue(connman);
    coinJoinServer.CheckPool(connman);
    coinJoinServer.CheckTimeout(connman);
}

void CCoinJoinServer::GetJsonInfo(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("queue_size",    GetQueueSize());
    obj.pushKV("denomination",  ValueFromAmount(CCoinJoin::DenominationToAmount(nSessionDenom)));
    obj.pushKV("state",         GetStateString());
    obj.pushKV("entries_count", GetEntriesCount());
}
