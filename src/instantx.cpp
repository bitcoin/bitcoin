// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "init.h"
#include "instantx.h"
#include "key.h"
#include "validation.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternode-utils.h"
#include "messagesigner.h"
#include "net.h"
#include "netmessagemaker.h"
#include "protocol.h"
#include "spork.h"
#include "sync.h"
#include "txmempool.h"
#include "util.h"
#include "consensus/validation.h"
#include "validationinterface.h"
#include "warnings.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif // ENABLE_WALLET

#include "llmq/quorums_instantsend.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>

#ifdef ENABLE_WALLET
extern CWallet* pwalletMain;
#endif // ENABLE_WALLET
extern CTxMemPool mempool;

bool fEnableInstantSend = true;

std::atomic<bool> CInstantSend::isAutoLockBip9Active{false};
const double CInstantSend::AUTO_IX_MEMPOOL_THRESHOLD = 0.1;

CInstantSend instantsend;
const std::string CInstantSend::SERIALIZATION_VERSION_STRING = "CInstantSend-Version-1";

// Transaction Locks
//
// step 1) Some node announces intention to lock transaction inputs via "txlockrequest" message (ix)
// step 2) Top nInstantSendSigsTotal masternodes per each spent outpoint push "txlockvote" message (txlvote)
// step 3) Once there are nInstantSendSigsRequired valid "txlockvote" messages (txlvote) per each spent outpoint
//         for a corresponding "txlockrequest" message (ix), all outpoints from that tx are treated as locked

//
// CInstantSend
//

void CInstantSend::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (fLiteMode) return; // disable all Dash specific functionality
    if (!llmq::IsOldInstantSendEnabled()) return;

    // NOTE: NetMsgType::TXLOCKREQUEST is handled via ProcessMessage() in net_processing.cpp

    if (strCommand == NetMsgType::TXLOCKVOTE) { // InstantSend Transaction Lock Consensus Votes
        if(pfrom->nVersion < MIN_INSTANTSEND_PROTO_VERSION) {
            LogPrint("instantsend", "TXLOCKVOTE -- peer=%d using obsolete version %i\n", pfrom->id, pfrom->nVersion);
            connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand, REJECT_OBSOLETE,
                               strprintf("Version must be %d or greater", MIN_INSTANTSEND_PROTO_VERSION)));
            return;
        }

        CTxLockVote vote;
        vRecv >> vote;


        uint256 nVoteHash = vote.GetHash();

        {
            LOCK(cs_main);
            connman.RemoveAskFor(nVoteHash);
        }

        // Ignore any InstantSend messages until blockchain is synced
        if (!masternodeSync.IsBlockchainSynced()) return;

        {
            LOCK(cs_instantsend);
            auto ret = mapTxLockVotes.emplace(nVoteHash, vote);
            if (!ret.second) return;
        }

        ProcessNewTxLockVote(pfrom, vote, connman);

        return;
    }
}

bool CInstantSend::ProcessTxLockRequest(const CTxLockRequest& txLockRequest, CConnman& connman)
{
    LOCK(cs_main);
#ifdef ENABLE_WALLET
    LOCK(pwalletMain ? &pwalletMain->cs_wallet : NULL);
#endif
    LOCK2(mempool.cs, cs_instantsend);

    uint256 txHash = txLockRequest.GetHash();

    // Check to see if we conflict with existing completed lock
    for (const auto& txin : txLockRequest.tx->vin) {
        std::map<COutPoint, uint256>::iterator it = mapLockedOutpoints.find(txin.prevout);
        if (it != mapLockedOutpoints.end() && it->second != txLockRequest.GetHash()) {
            // Conflicting with complete lock, proceed to see if we should cancel them both
            LogPrintf("CInstantSend::ProcessTxLockRequest -- WARNING: Found conflicting completed Transaction Lock, txid=%s, completed lock txid=%s\n",
                    txLockRequest.GetHash().ToString(), it->second.ToString());
        }
    }

    // Check to see if there are votes for conflicting request,
    // if so - do not fail, just warn user
    for (const auto& txin : txLockRequest.tx->vin) {
        std::map<COutPoint, std::set<uint256> >::iterator it = mapVotedOutpoints.find(txin.prevout);
        if (it != mapVotedOutpoints.end()) {
            for (const auto& hash : it->second) {
                if (hash != txLockRequest.GetHash()) {
                    LogPrint("instantsend", "CInstantSend::ProcessTxLockRequest -- Double spend attempt! %s\n", txin.prevout.ToStringShort());
                    // do not fail here, let it go and see which one will get the votes to be locked
                    // NOTIFY ZMQ
                    CTransaction txCurrent = *txLockRequest.tx; // currently processed tx
                    auto itPrevious = mapTxLockCandidates.find(hash);
                    if (itPrevious != mapTxLockCandidates.end() && itPrevious->second.txLockRequest) {
                        CTransaction txPrevious = *itPrevious->second.txLockRequest.tx; // previously locked one
                        GetMainSignals().NotifyInstantSendDoubleSpendAttempt(txCurrent, txPrevious);
                    }
                }
            }
        }
    }

    if (!CreateTxLockCandidate(txLockRequest)) {
        // smth is not right
        LogPrintf("CInstantSend::ProcessTxLockRequest -- CreateTxLockCandidate failed, txid=%s\n", txHash.ToString());
        return false;
    }
    LogPrintf("CInstantSend::ProcessTxLockRequest -- accepted, txid=%s\n", txHash.ToString());

    // Masternodes will sometimes propagate votes before the transaction is known to the client.
    // If this just happened - process orphan votes, lock inputs, resolve conflicting locks,
    // update transaction status forcing external script/zmq notifications.
    ProcessOrphanTxLockVotes();
    std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.find(txHash);
    TryToFinalizeLockCandidate(itLockCandidate->second);

    return true;
}

bool CInstantSend::CreateTxLockCandidate(const CTxLockRequest& txLockRequest)
{
    if (!txLockRequest.IsValid()) return false;

    LOCK(cs_instantsend);

    uint256 txHash = txLockRequest.GetHash();

    std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.find(txHash);
    if (itLockCandidate == mapTxLockCandidates.end()) {
        LogPrintf("CInstantSend::CreateTxLockCandidate -- new, txid=%s\n", txHash.ToString());

        CTxLockCandidate txLockCandidate(txLockRequest);
        // all inputs should already be checked by txLockRequest.IsValid() above, just use them now
        for (const auto& txin : txLockRequest.tx->vin) {
            txLockCandidate.AddOutPointLock(txin.prevout);
        }
        mapTxLockCandidates.insert(std::make_pair(txHash, txLockCandidate));
    } else if (!itLockCandidate->second.txLockRequest) {
        // i.e. empty Transaction Lock Candidate was created earlier, let's update it with actual data
        itLockCandidate->second.txLockRequest = txLockRequest;
        if (itLockCandidate->second.IsTimedOut()) {
            LogPrintf("CInstantSend::CreateTxLockCandidate -- timed out, txid=%s\n", txHash.ToString());
            return false;
        }
        LogPrintf("CInstantSend::CreateTxLockCandidate -- update empty, txid=%s\n", txHash.ToString());

        // all inputs should already be checked by txLockRequest.IsValid() above, just use them now
        for (const auto& txin : txLockRequest.tx->vin) {
            itLockCandidate->second.AddOutPointLock(txin.prevout);
        }
    } else {
        LogPrint("instantsend", "CInstantSend::CreateTxLockCandidate -- seen, txid=%s\n", txHash.ToString());
    }

    return true;
}

void CInstantSend::CreateEmptyTxLockCandidate(const uint256& txHash)
{
    if (mapTxLockCandidates.find(txHash) != mapTxLockCandidates.end())
        return;
    LogPrintf("CInstantSend::CreateEmptyTxLockCandidate -- new, txid=%s\n", txHash.ToString());
    const CTxLockRequest txLockRequest = CTxLockRequest();
    mapTxLockCandidates.insert(std::make_pair(txHash, CTxLockCandidate(txLockRequest)));
}

void CInstantSend::Vote(const uint256& txHash, CConnman& connman)
{
    AssertLockHeld(cs_main);
#ifdef ENABLE_WALLET
    LOCK(pwalletMain ? &pwalletMain->cs_wallet : NULL);
#endif

    CTxLockRequest dummyRequest;
    CTxLockCandidate txLockCandidate(dummyRequest);
    {
        LOCK(cs_instantsend);
        auto itLockCandidate = mapTxLockCandidates.find(txHash);
        if (itLockCandidate == mapTxLockCandidates.end()) return;
        txLockCandidate = itLockCandidate->second;
        Vote(txLockCandidate, connman);
    }

    // Let's see if our vote changed smth
    LOCK2(mempool.cs, cs_instantsend);
    TryToFinalizeLockCandidate(txLockCandidate);
}

void CInstantSend::Vote(CTxLockCandidate& txLockCandidate, CConnman& connman)
{
    if (!fMasternodeMode) return;
    if (!llmq::IsOldInstantSendEnabled()) return;

    AssertLockHeld(cs_main);
    AssertLockHeld(cs_instantsend);

    uint256 txHash = txLockCandidate.GetHash();
    // We should never vote on a Transaction Lock Request that was not (yet) accepted by the mempool
    if (mapLockRequestAccepted.find(txHash) == mapLockRequestAccepted.end()) return;
    // check if we need to vote on this candidate's outpoints,
    // it's possible that we need to vote for several of them
    for (auto& outpointLockPair : txLockCandidate.mapOutPointLocks) {
        int nPrevoutHeight = GetUTXOHeight(outpointLockPair.first);
        if (nPrevoutHeight == -1) {
            LogPrint("instantsend", "CInstantSend::Vote -- Failed to find UTXO %s\n", outpointLockPair.first.ToStringShort());
            return;
        }

        int nLockInputHeight = nPrevoutHeight + Params().GetConsensus().nInstantSendConfirmationsRequired - 2;

        int nRank;
        uint256 quorumModifierHash;
        if (!CMasternodeUtils::GetMasternodeRank(activeMasternodeInfo.outpoint, nRank, quorumModifierHash, nLockInputHeight)) {
            LogPrint("instantsend", "CInstantSend::Vote -- Can't calculate rank for masternode %s\n", activeMasternodeInfo.outpoint.ToStringShort());
            continue;
        }

        int nSignaturesTotal = Params().GetConsensus().nInstantSendSigsTotal;
        if (nRank > nSignaturesTotal) {
            LogPrint("instantsend", "CInstantSend::Vote -- Masternode not in the top %d (%d)\n", nSignaturesTotal, nRank);
            continue;
        }

        LogPrint("instantsend", "CInstantSend::Vote -- In the top %d (%d)\n", nSignaturesTotal, nRank);

        std::map<COutPoint, std::set<uint256> >::iterator itVoted = mapVotedOutpoints.find(outpointLockPair.first);

        // Check to see if we already voted for this outpoint,
        // refuse to vote twice or to include the same outpoint in another tx
        bool fAlreadyVoted = false;
        if (itVoted != mapVotedOutpoints.end()) {
            for (const auto& hash : itVoted->second) {
                std::map<uint256, CTxLockCandidate>::iterator it2 = mapTxLockCandidates.find(hash);
                if (it2->second.HasMasternodeVoted(outpointLockPair.first, activeMasternodeInfo.outpoint)) {
                    // we already voted for this outpoint to be included either in the same tx or in a competing one,
                    // skip it anyway
                    fAlreadyVoted = true;
                    LogPrintf("CInstantSend::Vote -- WARNING: We already voted for this outpoint, skipping: txHash=%s, outpoint=%s\n",
                            txHash.ToString(), outpointLockPair.first.ToStringShort());
                    break;
                }
            }
        }
        if (fAlreadyVoted) {
            continue; // skip to the next outpoint
        }

        // we haven't voted for this outpoint yet, let's try to do this now
        // Please note that activeMasternodeInfo.proTxHash is only valid after spork15 activation
        CTxLockVote vote(txHash, outpointLockPair.first, activeMasternodeInfo.outpoint, quorumModifierHash, activeMasternodeInfo.proTxHash);

        if (!vote.Sign()) {
            LogPrintf("CInstantSend::Vote -- Failed to sign consensus vote\n");
            return;
        }
        if (!vote.CheckSignature()) {
            LogPrintf("CInstantSend::Vote -- Signature invalid\n");
            return;
        }

        // vote constructed sucessfully, let's store and relay it
        uint256 nVoteHash = vote.GetHash();
        mapTxLockVotes.insert(std::make_pair(nVoteHash, vote));
        if (outpointLockPair.second.AddVote(vote)) {
            LogPrintf("CInstantSend::Vote -- Vote created successfully, relaying: txHash=%s, outpoint=%s, vote=%s\n",
                    txHash.ToString(), outpointLockPair.first.ToStringShort(), nVoteHash.ToString());

            if (itVoted == mapVotedOutpoints.end()) {
                std::set<uint256> setHashes;
                setHashes.insert(txHash);
                mapVotedOutpoints.insert(std::make_pair(outpointLockPair.first, setHashes));
            } else {
                mapVotedOutpoints[outpointLockPair.first].insert(txHash);
                if (mapVotedOutpoints[outpointLockPair.first].size() > 1) {
                    // it's ok to continue, just warn user
                    LogPrintf("CInstantSend::Vote -- WARNING: Vote conflicts with some existing votes: txHash=%s, outpoint=%s, vote=%s\n",
                            txHash.ToString(), outpointLockPair.first.ToStringShort(), nVoteHash.ToString());
                }
            }

            vote.Relay(connman);
        }
    }
}

bool CInstantSend::ProcessNewTxLockVote(CNode* pfrom, const CTxLockVote& vote, CConnman& connman)
{
    uint256 txHash = vote.GetTxHash();
    uint256 nVoteHash = vote.GetHash();

    if (!vote.IsValid(pfrom, connman)) {
        // could be because of missing MN
        LogPrint("instantsend", "CInstantSend::%s -- Vote is invalid, txid=%s\n", __func__, txHash.ToString());
        return false;
    }

    // relay valid vote asap
    vote.Relay(connman);

    LOCK(cs_main);
#ifdef ENABLE_WALLET
    LOCK(pwalletMain ? &pwalletMain->cs_wallet : NULL);
#endif
    LOCK2(mempool.cs, cs_instantsend);

    // Masternodes will sometimes propagate votes before the transaction is known to the client,
    // will actually process only after the lock request itself has arrived

    std::map<uint256, CTxLockCandidate>::iterator it = mapTxLockCandidates.find(txHash);
    if (it == mapTxLockCandidates.end() || !it->second.txLockRequest) {
        // no or empty tx lock candidate
        if (it == mapTxLockCandidates.end()) {
            // start timeout countdown after the very first vote
            CreateEmptyTxLockCandidate(txHash);
        }
        bool fInserted = mapTxLockVotesOrphan.emplace(nVoteHash, vote).second;
        LogPrint("instantsend", "CInstantSend::%s -- Orphan vote: txid=%s  masternode=%s %s\n",
                __func__, txHash.ToString(), vote.GetMasternodeOutpoint().ToStringShort(), fInserted ? "new" : "seen");

        // This tracks those messages and allows only the same rate as of the rest of the network
        // TODO: make sure this works good enough for multi-quorum

        int nMasternodeOrphanExpireTime = GetTime() + 60*10; // keep time data for 10 minutes
        auto itMnOV = mapMasternodeOrphanVotes.find(vote.GetMasternodeOutpoint());
        if (itMnOV == mapMasternodeOrphanVotes.end()) {
            mapMasternodeOrphanVotes.emplace(vote.GetMasternodeOutpoint(), nMasternodeOrphanExpireTime);
        } else {
            if (itMnOV->second > GetTime() && itMnOV->second > GetAverageMasternodeOrphanVoteTime()) {
                LogPrint("instantsend", "CInstantSend::%s -- masternode is spamming orphan Transaction Lock Votes: txid=%s  masternode=%s\n",
                        __func__, txHash.ToString(), vote.GetMasternodeOutpoint().ToStringShort());
                // Misbehaving(pfrom->id, 1);
                return false;
            }
            // not spamming, refresh
            itMnOV->second = nMasternodeOrphanExpireTime;
        }

        return true;
    }

    // We have a valid (non-empty) tx lock candidate
    CTxLockCandidate& txLockCandidate = it->second;

    if (txLockCandidate.IsTimedOut()) {
        LogPrint("instantsend", "CInstantSend::%s -- too late, Transaction Lock timed out, txid=%s\n", __func__, txHash.ToString());
        return false;
    }

    LogPrint("instantsend", "CInstantSend::%s -- Transaction Lock Vote, txid=%s\n", __func__, txHash.ToString());

    UpdateVotedOutpoints(vote, txLockCandidate);

    if (!txLockCandidate.AddVote(vote)) {
        // this should never happen
        return false;
    }

    int nSignatures = txLockCandidate.CountVotes();
    int nSignaturesMax = txLockCandidate.txLockRequest.GetMaxSignatures();
    LogPrint("instantsend", "CInstantSend::%s -- Transaction Lock signatures count: %d/%d, vote hash=%s\n", __func__,
            nSignatures, nSignaturesMax, nVoteHash.ToString());

    TryToFinalizeLockCandidate(txLockCandidate);

    return true;
}

bool CInstantSend::ProcessOrphanTxLockVote(const CTxLockVote& vote)
{
    // cs_main, cs_wallet and cs_instantsend should be already locked
    AssertLockHeld(cs_main);
#ifdef ENABLE_WALLET
    if (pwalletMain)
        AssertLockHeld(pwalletMain->cs_wallet);
#endif
    AssertLockHeld(cs_instantsend);

    uint256 txHash = vote.GetTxHash();

    // We shouldn't process orphan votes without a valid tx lock candidate
    std::map<uint256, CTxLockCandidate>::iterator it = mapTxLockCandidates.find(txHash);
    if (it == mapTxLockCandidates.end() || !it->second.txLockRequest)
        return false; // this shouldn never happen

    CTxLockCandidate& txLockCandidate = it->second;

    if (txLockCandidate.IsTimedOut()) {
        LogPrint("instantsend", "CInstantSend::%s -- too late, Transaction Lock timed out, txid=%s\n", __func__, txHash.ToString());
        return false;
    }

    LogPrint("instantsend", "CInstantSend::%s -- Transaction Lock Vote, txid=%s\n", __func__, txHash.ToString());

    UpdateVotedOutpoints(vote, txLockCandidate);

    if (!txLockCandidate.AddVote(vote)) {
        // this should never happen
        return false;
    }

    int nSignatures = txLockCandidate.CountVotes();
    int nSignaturesMax = txLockCandidate.txLockRequest.GetMaxSignatures();
    LogPrint("instantsend", "CInstantSend::%s -- Transaction Lock signatures count: %d/%d, vote hash=%s\n",
            __func__, nSignatures, nSignaturesMax, vote.GetHash().ToString());

    return true;
}

void CInstantSend::UpdateVotedOutpoints(const CTxLockVote& vote, CTxLockCandidate& txLockCandidate)
{
    AssertLockHeld(cs_instantsend);

    uint256 txHash = vote.GetTxHash();

    std::map<COutPoint, std::set<uint256> >::iterator it1 = mapVotedOutpoints.find(vote.GetOutpoint());
    if (it1 != mapVotedOutpoints.end()) {
        for (const auto& hash : it1->second) {
            if (hash != txHash) {
                // same outpoint was already voted to be locked by another tx lock request,
                // let's see if it was the same masternode who voted on this outpoint
                // for another tx lock request
                std::map<uint256, CTxLockCandidate>::iterator it2 = mapTxLockCandidates.find(hash);
                if (it2 !=mapTxLockCandidates.end() && it2->second.HasMasternodeVoted(vote.GetOutpoint(), vote.GetMasternodeOutpoint())) {
                    // yes, it was the same masternode
                    LogPrintf("CInstantSend::%s -- masternode sent conflicting votes! %s\n", __func__, vote.GetMasternodeOutpoint().ToStringShort());
                    // mark both Lock Candidates as attacked, none of them should complete,
                    // or at least the new (current) one shouldn't even
                    // if the second one was already completed earlier
                    txLockCandidate.MarkOutpointAsAttacked(vote.GetOutpoint());
                    it2->second.MarkOutpointAsAttacked(vote.GetOutpoint());
                    // apply maximum PoSe ban score to this masternode i.e. PoSe-ban it instantly
                    // TODO Call new PoSe system when it's ready
                    //mnodeman.PoSeBan(vote.GetMasternodeOutpoint());
                    // NOTE: This vote must be relayed further to let all other nodes know about such
                    // misbehaviour of this masternode. This way they should also be able to construct
                    // conflicting lock and PoSe-ban this masternode.
                }
            }
        }
        // store all votes, regardless of them being sent by malicious masternode or not
        it1->second.insert(txHash);
    } else {
        mapVotedOutpoints.emplace(vote.GetOutpoint(), std::set<uint256>({txHash}));
    }
}

void CInstantSend::ProcessOrphanTxLockVotes()
{
    AssertLockHeld(cs_main);
    AssertLockHeld(cs_instantsend);

    std::map<uint256, CTxLockVote>::iterator it = mapTxLockVotesOrphan.begin();
    while (it != mapTxLockVotesOrphan.end()) {
        if (ProcessOrphanTxLockVote(it->second)) {
            mapTxLockVotesOrphan.erase(it++);
        } else {
            ++it;
        }
    }
}

void CInstantSend::TryToFinalizeLockCandidate(const CTxLockCandidate& txLockCandidate)
{
    if (!llmq::IsOldInstantSendEnabled()) return;

    AssertLockHeld(cs_main);
    AssertLockHeld(cs_instantsend);

    uint256 txHash = txLockCandidate.txLockRequest.tx->GetHash();
    if (txLockCandidate.IsAllOutPointsReady() && !IsLockedInstantSendTransaction(txHash)) {
        // we have enough votes now
        LogPrint("instantsend", "CInstantSend::TryToFinalizeLockCandidate -- Transaction Lock is ready to complete, txid=%s\n", txHash.ToString());
        if (ResolveConflicts(txLockCandidate)) {
            LockTransactionInputs(txLockCandidate);
            UpdateLockedTransaction(txLockCandidate);
        }
    }
}

void CInstantSend::UpdateLockedTransaction(const CTxLockCandidate& txLockCandidate)
{
    // cs_main, cs_wallet and cs_instantsend should be already locked
    AssertLockHeld(cs_main);
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        AssertLockHeld(pwalletMain->cs_wallet);
    }
#endif
    AssertLockHeld(cs_instantsend);

    uint256 txHash = txLockCandidate.GetHash();

    if (!IsLockedInstantSendTransaction(txHash)) return; // not a locked tx, do not update/notify

#ifdef ENABLE_WALLET
    if (pwalletMain && pwalletMain->UpdatedTransaction(txHash)) {
        // notify an external script once threshold is reached
        std::string strCmd = GetArg("-instantsendnotify", "");
        if (!strCmd.empty()) {
            boost::replace_all(strCmd, "%s", txHash.GetHex());
            boost::thread t(runCommand, strCmd); // thread runs free
        }
    }
#endif

    GetMainSignals().NotifyTransactionLock(*txLockCandidate.txLockRequest.tx);

    LogPrint("instantsend", "CInstantSend::UpdateLockedTransaction -- done, txid=%s\n", txHash.ToString());
}

void CInstantSend::LockTransactionInputs(const CTxLockCandidate& txLockCandidate)
{
    if (!llmq::IsOldInstantSendEnabled()) return;

    LOCK(cs_instantsend);

    uint256 txHash = txLockCandidate.GetHash();

    if (!txLockCandidate.IsAllOutPointsReady()) return;

    for (const auto& pair : txLockCandidate.mapOutPointLocks) {
        mapLockedOutpoints.insert(std::make_pair(pair.first, txHash));
    }
    LogPrint("instantsend", "CInstantSend::LockTransactionInputs -- done, txid=%s\n", txHash.ToString());
}

bool CInstantSend::GetLockedOutPointTxHash(const COutPoint& outpoint, uint256& hashRet)
{
    LOCK(cs_instantsend);
    std::map<COutPoint, uint256>::iterator it = mapLockedOutpoints.find(outpoint);
    if (it == mapLockedOutpoints.end()) return false;
    hashRet = it->second;
    return true;
}

bool CInstantSend::ResolveConflicts(const CTxLockCandidate& txLockCandidate)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(cs_instantsend);

    uint256 txHash = txLockCandidate.GetHash();

    // make sure the lock is ready
    if (!txLockCandidate.IsAllOutPointsReady()) return false;

    AssertLockHeld(mempool.cs); // protect mempool.mapNextTx

    for (const auto& txin : txLockCandidate.txLockRequest.tx->vin) {
        uint256 hashConflicting;
        if (GetLockedOutPointTxHash(txin.prevout, hashConflicting) && txHash != hashConflicting) {
            // completed lock which conflicts with another completed one?
            // this means that majority of MNs in the quorum for this specific tx input are malicious!
            std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.find(txHash);
            std::map<uint256, CTxLockCandidate>::iterator itLockCandidateConflicting = mapTxLockCandidates.find(hashConflicting);
            if (itLockCandidate == mapTxLockCandidates.end() || itLockCandidateConflicting == mapTxLockCandidates.end()) {
                // safety check, should never really happen
                LogPrintf("CInstantSend::ResolveConflicts -- ERROR: Found conflicting completed Transaction Lock, but one of txLockCandidate-s is missing, txid=%s, conflicting txid=%s\n",
                        txHash.ToString(), hashConflicting.ToString());
                return false;
            }
            LogPrintf("CInstantSend::ResolveConflicts -- WARNING: Found conflicting completed Transaction Lock, dropping both, txid=%s, conflicting txid=%s\n",
                    txHash.ToString(), hashConflicting.ToString());
            CTxLockRequest txLockRequest = itLockCandidate->second.txLockRequest;
            CTxLockRequest txLockRequestConflicting = itLockCandidateConflicting->second.txLockRequest;
            itLockCandidate->second.SetConfirmedHeight(0); // expired
            itLockCandidateConflicting->second.SetConfirmedHeight(0); // expired
            CheckAndRemove(); // clean up
            // AlreadyHave should still return "true" for both of them
            mapLockRequestRejected.insert(std::make_pair(txHash, txLockRequest));
            mapLockRequestRejected.insert(std::make_pair(hashConflicting, txLockRequestConflicting));

            // TODO: clean up mapLockRequestRejected later somehow
            //       (not a big issue since we already PoSe ban malicious masternodes
            //        and they won't be able to spam)
            // TODO: ban all malicious masternodes permanently, do not accept anything from them, ever

            // TODO: notify zmq+script about this double-spend attempt
            //       and let merchant cancel/hold the order if it's not too late...

            // can't do anything else, fallback to regular txes
            return false;
        } else if (mempool.mapNextTx.count(txin.prevout)) {
            // check if it's in mempool
            hashConflicting = mempool.mapNextTx.find(txin.prevout)->second->GetHash();
            if (txHash == hashConflicting) continue; // matches current, not a conflict, skip to next txin
            // conflicts with tx in mempool
            LogPrintf("CInstantSend::ResolveConflicts -- ERROR: Failed to complete Transaction Lock, conflicts with mempool, txid=%s\n", txHash.ToString());
            return false;
        }
    } // FOREACH
    // No conflicts were found so far, check to see if it was already included in block
    CTransactionRef txTmp;
    uint256 hashBlock;
    if (GetTransaction(txHash, txTmp, Params().GetConsensus(), hashBlock, true) && hashBlock != uint256()) {
        LogPrint("instantsend", "CInstantSend::ResolveConflicts -- Done, %s is included in block %s\n", txHash.ToString(), hashBlock.ToString());
        return true;
    }
    // Not in block yet, make sure all its inputs are still unspent
    for (const auto& txin : txLockCandidate.txLockRequest.tx->vin) {
        Coin coin;
        if (!GetUTXOCoin(txin.prevout, coin)) {
            // Not in UTXO anymore? A conflicting tx was mined while we were waiting for votes.
            LogPrintf("CInstantSend::ResolveConflicts -- ERROR: Failed to find UTXO %s, can't complete Transaction Lock\n", txin.prevout.ToStringShort());
            return false;
        }
    }
    LogPrint("instantsend", "CInstantSend::ResolveConflicts -- Done, txid=%s\n", txHash.ToString());

    return true;
}

int64_t CInstantSend::GetAverageMasternodeOrphanVoteTime()
{
    LOCK(cs_instantsend);
    // NOTE: should never actually call this function when mapMasternodeOrphanVotes is empty
    if (mapMasternodeOrphanVotes.empty()) return 0;

    int64_t total = 0;
    for (const auto& pair : mapMasternodeOrphanVotes) {
        total += pair.second;
    }

    return total / mapMasternodeOrphanVotes.size();
}

void CInstantSend::CheckAndRemove()
{
    if (!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_instantsend);

    std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.begin();

    // remove expired candidates
    while (itLockCandidate != mapTxLockCandidates.end()) {
        CTxLockCandidate &txLockCandidate = itLockCandidate->second;
        uint256 txHash = txLockCandidate.GetHash();
        if (txLockCandidate.IsExpired(nCachedBlockHeight)) {
            LogPrintf("CInstantSend::CheckAndRemove -- Removing expired Transaction Lock Candidate: txid=%s\n", txHash.ToString());

            for (const auto& pair : txLockCandidate.mapOutPointLocks) {
                mapLockedOutpoints.erase(pair.first);
                mapVotedOutpoints.erase(pair.first);
            }
            mapLockRequestAccepted.erase(txHash);
            mapLockRequestRejected.erase(txHash);
            mapTxLockCandidates.erase(itLockCandidate++);
        } else {
            ++itLockCandidate;
        }
    }

    // remove expired votes
    std::map<uint256, CTxLockVote>::iterator itVote = mapTxLockVotes.begin();
    while (itVote != mapTxLockVotes.end()) {
        if (itVote->second.IsExpired(nCachedBlockHeight)) {
            LogPrint("instantsend", "CInstantSend::CheckAndRemove -- Removing expired vote: txid=%s  masternode=%s\n",
                    itVote->second.GetTxHash().ToString(), itVote->second.GetMasternodeOutpoint().ToStringShort());
            mapTxLockVotes.erase(itVote++);
        } else {
            ++itVote;
        }
    }

    // remove timed out orphan votes
    std::map<uint256, CTxLockVote>::iterator itOrphanVote = mapTxLockVotesOrphan.begin();
    while (itOrphanVote != mapTxLockVotesOrphan.end()) {
        if (itOrphanVote->second.IsTimedOut()) {
            LogPrint("instantsend", "CInstantSend::CheckAndRemove -- Removing timed out orphan vote: txid=%s  masternode=%s\n",
                    itOrphanVote->second.GetTxHash().ToString(), itOrphanVote->second.GetMasternodeOutpoint().ToStringShort());
            mapTxLockVotes.erase(itOrphanVote->first);
            mapTxLockVotesOrphan.erase(itOrphanVote++);
        } else {
            ++itOrphanVote;
        }
    }

    // remove invalid votes and votes for failed lock attempts
    itVote = mapTxLockVotes.begin();
    while (itVote != mapTxLockVotes.end()) {
        if (itVote->second.IsFailed()) {
            LogPrint("instantsend", "CInstantSend::CheckAndRemove -- Removing vote for failed lock attempt: txid=%s  masternode=%s\n",
                    itVote->second.GetTxHash().ToString(), itVote->second.GetMasternodeOutpoint().ToStringShort());
            mapTxLockVotes.erase(itVote++);
        } else {
            ++itVote;
        }
    }

    // remove timed out masternode orphan votes (DOS protection)
    std::map<COutPoint, int64_t>::iterator itMasternodeOrphan = mapMasternodeOrphanVotes.begin();
    while (itMasternodeOrphan != mapMasternodeOrphanVotes.end()) {
        if (itMasternodeOrphan->second < GetTime()) {
            LogPrint("instantsend", "CInstantSend::CheckAndRemove -- Removing timed out orphan masternode vote: masternode=%s\n",
                    itMasternodeOrphan->first.ToStringShort());
            mapMasternodeOrphanVotes.erase(itMasternodeOrphan++);
        } else {
            ++itMasternodeOrphan;
        }
    }
    LogPrintf("CInstantSend::CheckAndRemove -- %s\n", ToString());
}

bool CInstantSend::AlreadyHave(const uint256& hash)
{
    if (!llmq::IsOldInstantSendEnabled()) {
        return true;
    }

    LOCK(cs_instantsend);
    return mapLockRequestAccepted.count(hash) ||
            mapLockRequestRejected.count(hash) ||
            mapTxLockVotes.count(hash);
}

void CInstantSend::AcceptLockRequest(const CTxLockRequest& txLockRequest)
{
    LOCK(cs_instantsend);
    mapLockRequestAccepted.insert(std::make_pair(txLockRequest.GetHash(), txLockRequest));
}

void CInstantSend::RejectLockRequest(const CTxLockRequest& txLockRequest)
{
    LOCK(cs_instantsend);
    mapLockRequestRejected.insert(std::make_pair(txLockRequest.GetHash(), txLockRequest));
}

bool CInstantSend::HasTxLockRequest(const uint256& txHash)
{
    CTxLockRequest txLockRequestTmp;
    return GetTxLockRequest(txHash, txLockRequestTmp);
}

bool CInstantSend::GetTxLockRequest(const uint256& txHash, CTxLockRequest& txLockRequestRet)
{
    if (!llmq::IsOldInstantSendEnabled()) {
        return false;
    }

    LOCK(cs_instantsend);

    std::map<uint256, CTxLockCandidate>::iterator it = mapTxLockCandidates.find(txHash);
    if (it == mapTxLockCandidates.end() || !it->second.txLockRequest) return false;
    txLockRequestRet = it->second.txLockRequest;

    return true;
}

bool CInstantSend::GetTxLockVote(const uint256& hash, CTxLockVote& txLockVoteRet)
{
    if (!llmq::IsOldInstantSendEnabled()) {
        return false;
    }

    LOCK(cs_instantsend);

    std::map<uint256, CTxLockVote>::iterator it = mapTxLockVotes.find(hash);
    if (it == mapTxLockVotes.end()) return false;
    txLockVoteRet = it->second;

    return true;
}

void CInstantSend::Clear()
{
    LOCK(cs_instantsend);

    mapLockRequestAccepted.clear();
    mapLockRequestRejected.clear();
    mapTxLockVotes.clear();
    mapTxLockVotesOrphan.clear();
    mapTxLockCandidates.clear();
    mapVotedOutpoints.clear();
    mapLockedOutpoints.clear();
    mapMasternodeOrphanVotes.clear();
    nCachedBlockHeight = 0;
}

bool CInstantSend::IsLockedInstantSendTransaction(const uint256& txHash)
{
    if (!fEnableInstantSend || GetfLargeWorkForkFound() || GetfLargeWorkInvalidChainFound() ||
        !sporkManager.IsSporkActive(SPORK_3_INSTANTSEND_BLOCK_FILTERING)) return false;

    LOCK(cs_instantsend);

    // there must be a lock candidate
    std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.find(txHash);
    if (itLockCandidate == mapTxLockCandidates.end()) return false;

    // which should have outpoints
    if (itLockCandidate->second.mapOutPointLocks.empty()) return false;

    // and all of these outputs must be included in mapLockedOutpoints with correct hash
    for (const auto& pair : itLockCandidate->second.mapOutPointLocks) {
        uint256 hashLocked;
        if (!GetLockedOutPointTxHash(pair.first, hashLocked) || hashLocked != txHash) return false;
    }

    return true;
}

int CInstantSend::GetTransactionLockSignatures(const uint256& txHash)
{
    if (!fEnableInstantSend) return -1;
    if (GetfLargeWorkForkFound() || GetfLargeWorkInvalidChainFound()) return -2;
    if (!llmq::IsOldInstantSendEnabled()) return -3;

    LOCK(cs_instantsend);

    std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.find(txHash);
    if (itLockCandidate != mapTxLockCandidates.end()) {
        return itLockCandidate->second.CountVotes();
    }

    return -1;
}

bool CInstantSend::IsTxLockCandidateTimedOut(const uint256& txHash)
{
    if (!fEnableInstantSend) return false;

    LOCK(cs_instantsend);

    std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.find(txHash);
    if (itLockCandidate != mapTxLockCandidates.end()) {
        return !itLockCandidate->second.IsAllOutPointsReady() &&
                itLockCandidate->second.IsTimedOut();
    }

    return false;
}

void CInstantSend::Relay(const uint256& txHash, CConnman& connman)
{
    LOCK(cs_instantsend);

    std::map<uint256, CTxLockCandidate>::const_iterator itLockCandidate = mapTxLockCandidates.find(txHash);
    if (itLockCandidate != mapTxLockCandidates.end()) {
        itLockCandidate->second.Relay(connman);
    }
}

void CInstantSend::UpdatedBlockTip(const CBlockIndex *pindex)
{
    nCachedBlockHeight = pindex->nHeight;
}

void CInstantSend::SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, int posInBlock)
{
    // Update lock candidates and votes if corresponding tx confirmed
    // or went from confirmed to 0-confirmed or conflicted.

    if (tx.IsCoinBase()) return;

    LOCK2(cs_main, cs_instantsend);

    uint256 txHash = tx.GetHash();

    // When tx is 0-confirmed or conflicted, posInBlock is SYNC_TRANSACTION_NOT_IN_BLOCK and nHeightNew should be set to -1
    int nHeightNew = posInBlock == CMainSignals::SYNC_TRANSACTION_NOT_IN_BLOCK ? -1 : pindex->nHeight;

    LogPrint("instantsend", "CInstantSend::SyncTransaction -- txid=%s nHeightNew=%d\n", txHash.ToString(), nHeightNew);

    // Check lock candidates
    std::map<uint256, CTxLockCandidate>::iterator itLockCandidate = mapTxLockCandidates.find(txHash);
    if (itLockCandidate != mapTxLockCandidates.end()) {
        LogPrint("instantsend", "CInstantSend::SyncTransaction -- txid=%s nHeightNew=%d lock candidate updated\n",
                txHash.ToString(), nHeightNew);
        itLockCandidate->second.SetConfirmedHeight(nHeightNew);
        // Loop through outpoint locks
        for (const auto& pair : itLockCandidate->second.mapOutPointLocks) {
            // Check corresponding lock votes
            for (const auto& vote : pair.second.GetVotes()) {
                uint256 nVoteHash = vote.GetHash();
                LogPrint("instantsend", "CInstantSend::SyncTransaction -- txid=%s nHeightNew=%d vote %s updated\n",
                        txHash.ToString(), nHeightNew, nVoteHash.ToString());
                const auto& it = mapTxLockVotes.find(nVoteHash);
                if (it != mapTxLockVotes.end()) {
                    it->second.SetConfirmedHeight(nHeightNew);
                }
            }
        }
    }

    // check orphan votes
    for (const auto& pair : mapTxLockVotesOrphan) {
        if (pair.second.GetTxHash() == txHash) {
            LogPrint("instantsend", "CInstantSend::SyncTransaction -- txid=%s nHeightNew=%d vote %s updated\n",
                    txHash.ToString(), nHeightNew, pair.first.ToString());
            mapTxLockVotes[pair.first].SetConfirmedHeight(nHeightNew);
        }
    }
}

std::string CInstantSend::ToString() const
{
    LOCK(cs_instantsend);
    return strprintf("Lock Candidates: %llu, Votes %llu", mapTxLockCandidates.size(), mapTxLockVotes.size());
}

void CInstantSend::DoMaintenance()
{
    if (ShutdownRequested()) return;

    CheckAndRemove();
}

bool CInstantSend::CanAutoLock()
{
    if (!isAutoLockBip9Active || !llmq::IsOldInstantSendEnabled()) {
        return false;
    }
    if (!sporkManager.IsSporkActive(SPORK_16_INSTANTSEND_AUTOLOCKS)) {
        return false;
    }
    return (mempool.UsedMemoryShare() < AUTO_IX_MEMPOOL_THRESHOLD);
}

//
// CTxLockRequest
//

bool CTxLockRequest::IsValid() const
{
    if (tx->vout.size() < 1) return false;

    if (tx->vin.size() > WARN_MANY_INPUTS) {
        LogPrint("instantsend", "CTxLockRequest::IsValid -- WARNING: Too many inputs: tx=%s", ToString());
    }

    AssertLockHeld(cs_main);
    if (!CheckFinalTx(*tx)) {
        LogPrint("instantsend", "CTxLockRequest::IsValid -- Transaction is not final: tx=%s", ToString());
        return false;
    }

    CAmount nValueIn = 0;

    int nInstantSendConfirmationsRequired = Params().GetConsensus().nInstantSendConfirmationsRequired;

    for (const auto& txin : tx->vin) {

        Coin coin;

        if (!GetUTXOCoin(txin.prevout, coin)) {
            LogPrint("instantsend", "CTxLockRequest::IsValid -- Failed to find UTXO %s\n", txin.prevout.ToStringShort());
            return false;
        }

        int nTxAge = chainActive.Height() - coin.nHeight + 1;
        // 1 less than the "send IX" gui requires, in case of a block propagating the network at the time
        int nConfirmationsRequired = nInstantSendConfirmationsRequired - 1;

        if (nTxAge < nConfirmationsRequired) {
            LogPrint("instantsend", "CTxLockRequest::IsValid -- outpoint %s too new: nTxAge=%d, nConfirmationsRequired=%d, txid=%s\n",
                    txin.prevout.ToStringShort(), nTxAge, nConfirmationsRequired, GetHash().ToString());
            return false;
        }

        nValueIn += coin.out.nValue;
    }

    if (nValueIn > sporkManager.GetSporkValue(SPORK_5_INSTANTSEND_MAX_VALUE)*COIN) {
        LogPrint("instantsend", "CTxLockRequest::IsValid -- Transaction value too high: nValueIn=%d, tx=%s", nValueIn, ToString());
        return false;
    }

    CAmount nValueOut = tx->GetValueOut();

    if (nValueIn - nValueOut < GetMinFee(false)) {
        LogPrint("instantsend", "CTxLockRequest::IsValid -- did not include enough fees in transaction: fees=%d, tx=%s", nValueOut - nValueIn, ToString());
        return false;
    }

    return true;
}

CAmount CTxLockRequest::GetMinFee(bool fForceMinFee) const
{
    if (!fForceMinFee && CInstantSend::CanAutoLock() && IsSimple()) {
        return CAmount();
    }
    CAmount nMinFee = MIN_FEE;
    return std::max(nMinFee, CAmount(tx->vin.size() * nMinFee));
}

int CTxLockRequest::GetMaxSignatures() const
{
    return tx->vin.size() * Params().GetConsensus().nInstantSendSigsTotal;
}

bool CTxLockRequest::IsSimple() const
{
    return (tx->vin.size() <= MAX_INPUTS_FOR_AUTO_IX);
}

//
// CTxLockVote
//

bool CTxLockVote::IsValid(CNode* pnode, CConnman& connman) const
{
    auto mnList = deterministicMNManager->GetListAtChainTip();

    if (!mnList.HasValidMNByCollateral(outpointMasternode)) {
        LogPrint("instantsend", "CTxLockVote::IsValid -- Unknown masternode %s\n", outpointMasternode.ToStringShort());
        return false;
    }

    // Verify that masternodeProTxHash belongs to the same MN referred by the collateral
    // This is a leftover from the legacy non-deterministic MN list where we used the collateral to identify MNs
    // TODO eventually remove the collateral from CTxLockVote
    if (!masternodeProTxHash.IsNull()) {
        auto dmn = mnList.GetValidMN(masternodeProTxHash);
        if (!dmn || dmn->collateralOutpoint != outpointMasternode) {
            LogPrint("instantsend", "CTxLockVote::IsValid -- invalid masternodeProTxHash %s\n", masternodeProTxHash.ToString());
            return false;
        }
    } else {
        LogPrint("instantsend", "CTxLockVote::IsValid -- missing masternodeProTxHash\n");
        return false;
    }

    Coin coin;
    if (!GetUTXOCoin(outpoint, coin)) {
        LogPrint("instantsend", "CTxLockVote::IsValid -- Failed to find UTXO %s\n", outpoint.ToStringShort());
        return false;
    }

    int nLockInputHeight = coin.nHeight + Params().GetConsensus().nInstantSendConfirmationsRequired - 2;

    int nRank;
    uint256 expectedQuorumModifierHash;
    if (!CMasternodeUtils::GetMasternodeRank(outpointMasternode, nRank, expectedQuorumModifierHash, nLockInputHeight)) {
        //can be caused by past versions trying to vote with an invalid protocol
        LogPrint("instantsend", "CTxLockVote::IsValid -- Can't calculate rank for masternode %s\n", outpointMasternode.ToStringShort());
        return false;
    }
    if (!quorumModifierHash.IsNull()) {
        if (quorumModifierHash != expectedQuorumModifierHash) {
            LogPrint("instantsend", "CTxLockVote::IsValid -- invalid quorumModifierHash %s, expected %s\n", quorumModifierHash.ToString(), expectedQuorumModifierHash.ToString());
            return false;
        }
    } else {
        LogPrint("instantsend", "CTxLockVote::IsValid -- missing quorumModifierHash\n");
        return false;
    }

    LogPrint("instantsend", "CTxLockVote::IsValid -- Masternode %s, rank=%d\n", outpointMasternode.ToStringShort(), nRank);

    int nSignaturesTotal = Params().GetConsensus().nInstantSendSigsTotal;
    if (nRank > nSignaturesTotal) {
        LogPrint("instantsend", "CTxLockVote::IsValid -- Masternode %s is not in the top %d (%d), vote hash=%s\n",
                outpointMasternode.ToStringShort(), nSignaturesTotal, nRank, GetHash().ToString());
        return false;
    }

    if (!CheckSignature()) {
        LogPrintf("CTxLockVote::IsValid -- Signature invalid\n");
        return false;
    }

    return true;
}

uint256 CTxLockVote::GetHash() const
{
    return SerializeHash(*this);
}

uint256 CTxLockVote::GetSignatureHash() const
{
    return GetHash();
}

bool CTxLockVote::CheckSignature() const
{
    std::string strError;

    auto dmn = deterministicMNManager->GetListAtChainTip().GetValidMN(masternodeProTxHash);
    if (!dmn) {
        LogPrintf("CTxLockVote::CheckSignature -- Unknown Masternode: masternode=%s\n", masternodeProTxHash.ToString());
        return false;
    }

    uint256 hash = GetSignatureHash();

    CBLSSignature sig;
    sig.SetBuf(vchMasternodeSignature);
    if (!sig.IsValid() || !sig.VerifyInsecure(dmn->pdmnState->pubKeyOperator.Get(), hash)) {
        LogPrintf("CTxLockVote::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }
    return true;
}

bool CTxLockVote::Sign()
{

    uint256 hash = GetSignatureHash();

    CBLSSignature sig = activeMasternodeInfo.blsKeyOperator->Sign(hash);
    if (!sig.IsValid()) {
        return false;
    }
    sig.GetBuf(vchMasternodeSignature);
    return true;
}

void CTxLockVote::Relay(CConnman& connman) const
{
    CInv inv(MSG_TXLOCK_VOTE, GetHash());
    CTxLockRequest request;
    if (instantsend.GetTxLockRequest(txHash, request) && request) {
        connman.RelayInvFiltered(inv, *request.tx);
    } else {
        connman.RelayInvFiltered(inv, txHash);
    }
}

bool CTxLockVote::IsExpired(int nHeight) const
{
    // Locks and votes expire nInstantSendKeepLock blocks after the block corresponding tx was included into.
    return (nConfirmedHeight != -1) && (nHeight - nConfirmedHeight > Params().GetConsensus().nInstantSendKeepLock);
}

bool CTxLockVote::IsTimedOut() const
{
    return GetTime() - nTimeCreated > INSTANTSEND_LOCK_TIMEOUT_SECONDS;
}

bool CTxLockVote::IsFailed() const
{
    return (GetTime() - nTimeCreated > INSTANTSEND_FAILED_TIMEOUT_SECONDS) && !instantsend.IsLockedInstantSendTransaction(GetTxHash());
}

//
// COutPointLock
//

bool COutPointLock::AddVote(const CTxLockVote& vote)
{
    return mapMasternodeVotes.emplace(vote.GetMasternodeOutpoint(), vote).second;
}

std::vector<CTxLockVote> COutPointLock::GetVotes() const
{
    std::vector<CTxLockVote> vRet;
    for (const auto& pair : mapMasternodeVotes) {
        vRet.push_back(pair.second);
    }
    return vRet;
}

bool COutPointLock::HasMasternodeVoted(const COutPoint& outpointMasternodeIn) const
{
    return mapMasternodeVotes.count(outpointMasternodeIn);
}

bool COutPointLock::IsReady() const
{
    return !fAttacked && CountVotes() >= Params().GetConsensus().nInstantSendSigsRequired;
}

void COutPointLock::Relay(CConnman& connman) const
{
    for (const auto& pair : mapMasternodeVotes) {
        pair.second.Relay(connman);
    }
}

//
// CTxLockCandidate
//

void CTxLockCandidate::AddOutPointLock(const COutPoint& outpoint)
{
    mapOutPointLocks.insert(std::make_pair(outpoint, COutPointLock(outpoint)));
}

void CTxLockCandidate::MarkOutpointAsAttacked(const COutPoint& outpoint)
{
    std::map<COutPoint, COutPointLock>::iterator it = mapOutPointLocks.find(outpoint);
    if (it != mapOutPointLocks.end())
        it->second.MarkAsAttacked();
}

bool CTxLockCandidate::AddVote(const CTxLockVote& vote)
{
    std::map<COutPoint, COutPointLock>::iterator it = mapOutPointLocks.find(vote.GetOutpoint());
    if (it == mapOutPointLocks.end()) return false;
    return it->second.AddVote(vote);
}

bool CTxLockCandidate::IsAllOutPointsReady() const
{
    if (mapOutPointLocks.empty()) return false;

    for (const auto& pair : mapOutPointLocks) {
        if (!pair.second.IsReady()) return false;
    }
    return true;
}

bool CTxLockCandidate::HasMasternodeVoted(const COutPoint& outpointIn, const COutPoint& outpointMasternodeIn)
{
    std::map<COutPoint, COutPointLock>::iterator it = mapOutPointLocks.find(outpointIn);
    return it !=mapOutPointLocks.end() && it->second.HasMasternodeVoted(outpointMasternodeIn);
}

int CTxLockCandidate::CountVotes() const
{
    // Note: do NOT use vote count to figure out if tx is locked, use IsAllOutPointsReady() instead
    int nCountVotes = 0;
    for (const auto& pair : mapOutPointLocks) {
        nCountVotes += pair.second.CountVotes();
    }
    return nCountVotes;
}

bool CTxLockCandidate::IsExpired(int nHeight) const
{
    // Locks and votes expire nInstantSendKeepLock blocks after the block corresponding tx was included into.
    return (nConfirmedHeight != -1) && (nHeight - nConfirmedHeight > Params().GetConsensus().nInstantSendKeepLock);
}

bool CTxLockCandidate::IsTimedOut() const
{
    return GetTime() - nTimeCreated > INSTANTSEND_LOCK_TIMEOUT_SECONDS;
}

void CTxLockCandidate::Relay(CConnman& connman) const
{
    connman.RelayTransaction(*txLockRequest.tx);
    for (const auto& pair : mapOutPointLocks) {
        pair.second.Relay(connman);
    }
}
