// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/validation.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "protocol.h"
#include "instantx.h"
#include "activemasternode.h"
#include "darksend.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "spork.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

extern CWallet* pwalletMain;

bool fEnableInstantSend = true;
int nInstantSendDepth = DEFAULT_INSTANTSEND_DEPTH;
int nCompleteTXLocks;

std::map<uint256, CTransaction> mapTxLockReq;
std::map<uint256, CTransaction> mapTxLockReqRejected;
std::map<uint256, CConsensusVote> mapTxLockVote;
std::map<COutPoint, uint256> mapLockedInputs;

std::map<uint256, CTransactionLock> mapTxLocks;
std::map<uint256, int64_t> mapUnknownVotes; //track votes with no tx for DOS

CCriticalSection cs_instantsend;

//txlock - Locks transaction
//
//step 1.) Broadcast intention to lock transaction inputs, "txlreg", CTransaction
//step 2.) Top INSTANTSEND_SIGNATURES_TOTAL masternodes, open connect to top 1 masternode.
//         Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 masternode, waits for INSTANTSEND_SIGNATURES_REQUIRED messages. Upon success, sends "txlock'

void ProcessMessageInstantSend(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; // disable all Dash specific functionality
    if(!sporkManager.IsSporkActive(SPORK_2_INSTANTSEND_ENABLED)) return;

    // Ignore any InstantSend messages until masternode list is synced
    if(!masternodeSync.IsMasternodeListSynced()) return;

    if (strCommand == NetMsgType::IX)
    {
        //LogPrintf("ProcessMessageInstantSend\n");
        CDataStream vMsg(vRecv);
        CTransaction tx;
        vRecv >> tx;

        // FIXME: this part of simulating inv is not good actually, leaving it only for 12.1 backwards compatibility
        // and since we are using invs for relaying even initial ix request, this can (and should) be safely removed in 12.2
        CInv inv(MSG_TXLOCK_REQUEST, tx.GetHash());
        pfrom->AddInventoryKnown(inv);
        GetMainSignals().Inventory(inv.hash);

        // have we seen it already?
        if(mapTxLockReq.count(inv.hash) || mapTxLockReqRejected.count(inv.hash)) return;
        // is it a valid one?
        if(!IsInstantSendTxValid(tx)) return;

        BOOST_FOREACH(const CTxOut o, tx.vout) {
            // IX supports normal scripts and unspendable scripts (used in DS collateral and Budget collateral).
            // TODO: Look into other script types that are normal and can be included
            if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()) {
                LogPrintf("ProcessMessageInstantSend -- Invalid Script %s", tx.ToString());
                return;
            }
        }

        int nBlockHeight = CreateNewLock(tx);

        bool fMissingInputs = false;
        CValidationState state;

        bool fAccepted = false;
        {
            LOCK(cs_main);
            fAccepted = AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs);
        }
        if(fAccepted) {
            RelayInv(inv);

            DoConsensusVote(tx, nBlockHeight);

            mapTxLockReq.insert(std::make_pair(tx.GetHash(), tx));

            LogPrintf("ProcessMessageInstantSend -- Transaction Lock Request: %s %s : accepted %s\n",
                pfrom->addr.ToString(), pfrom->cleanSubVer,
                tx.GetHash().ToString()
            );

            // Masternodes will sometimes propagate votes before the transaction is known to the client.
            // If this just happened - update transaction status, try forcing external script notification,
            // lock inputs and resolve conflicting locks
            if(IsLockedInstandSendTransaction(tx.GetHash())) {
                UpdateLockedTransaction(tx, true);
                LockTransactionInputs(tx);
                ResolveConflicts(tx);
            }

            return;

        } else {
            mapTxLockReqRejected.insert(std::make_pair(tx.GetHash(), tx));

            // can we get the conflicting transaction as proof?

            LogPrintf("ProcessMessageInstantSend -- Transaction Lock Request: %s %s : rejected %s\n",
                pfrom->addr.ToString(), pfrom->cleanSubVer,
                tx.GetHash().ToString()
            );

            LockTransactionInputs(tx);
            ResolveConflicts(tx);

            return;
        }
    }
    else if (strCommand == NetMsgType::IXLOCKVOTE) //InstantX Lock Consensus Votes
    {
        CConsensusVote vote;
        vRecv >> vote;

        CInv inv(MSG_TXLOCK_VOTE, vote.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapTxLockVote.count(vote.GetHash())) return;
        mapTxLockVote.insert(std::make_pair(vote.GetHash(), vote));

        if(ProcessConsensusVote(pfrom, vote)) {
            //Spam/Dos protection
            /*
                Masternodes will sometimes propagate votes before the transaction is known to the client.
                This tracks those messages and allows it at the same rate of the rest of the network, if
                a peer violates it, it will simply be ignored
            */
            if(!mapTxLockReq.count(vote.txHash) && !mapTxLockReqRejected.count(vote.txHash)) {
                if(!mapUnknownVotes.count(vote.vinMasternode.prevout.hash))
                    mapUnknownVotes[vote.vinMasternode.prevout.hash] = GetTime()+(60*10);

                if(mapUnknownVotes[vote.vinMasternode.prevout.hash] > GetTime() &&
                    mapUnknownVotes[vote.vinMasternode.prevout.hash] - GetAverageVoteTime() > 60*10) {
                        LogPrintf("ProcessMessageInstantSend -- masternode is spamming transaction votes: %s %s\n",
                            vote.vinMasternode.ToString(),
                            vote.txHash.ToString()
                        );
                        return;
                } else {
                    mapUnknownVotes[vote.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }
            }
            RelayInv(inv);
        }

        return;
    }
}

bool IsInstantSendTxValid(const CTransaction& txCollateral)
{
    if(txCollateral.vout.size() < 1) return false;

    if(!CheckFinalTx(txCollateral)) {
        LogPrint("instantsend", "IsInstantSendTxValid -- Transaction is not final: txCollateral=%s", txCollateral.ToString());
        return false;
    }

    int64_t nValueIn = 0;
    int64_t nValueOut = 0;
    bool missingTx = false;

    BOOST_FOREACH(const CTxOut txout, txCollateral.vout)
        nValueOut += txout.nValue;

    BOOST_FOREACH(const CTxIn txin, txCollateral.vin) {
        CTransaction tx2;
        uint256 hash;
        if(GetTransaction(txin.prevout.hash, tx2, Params().GetConsensus(), hash, true)) {
            if(tx2.vout.size() > txin.prevout.n)
                nValueIn += tx2.vout[txin.prevout.n].nValue;
        } else {
            missingTx = true;
        }
    }

    if(nValueOut > sporkManager.GetSporkValue(SPORK_5_INSTANTSEND_MAX_VALUE)*COIN) {
        LogPrint("instantsend", "IsInstantSendTxValid -- Transaction value too high: nValueOut=%d, txCollateral=%s", nValueOut, txCollateral.ToString());
        return false;
    }

    if(missingTx) {
        LogPrint("instantsend", "IsInstantSendTxValid -- Unknown inputs in IX transaction: txCollateral=%s", txCollateral.ToString());
        /*
            This happens sometimes for an unknown reason, so we'll return that it's a valid transaction.
            If someone submits an invalid transaction it will be rejected by the network anyway and this isn't
            very common, but we don't want to block IX just because the client can't figure out the fee.
        */
        return true;
    }

    if(nValueIn-nValueOut < INSTANTSEND_MIN_FEE) {
        LogPrint("instantsend", "IsInstantSendTxValid -- did not include enough fees in transaction: fees=%d, txCollateral=%s", nValueOut-nValueIn, txCollateral.ToString());
        return false;
    }

    return true;
}

int64_t CreateNewLock(CTransaction tx)
{

    int64_t nTxAge = 0;
    BOOST_REVERSE_FOREACH(CTxIn txin, tx.vin) {
        nTxAge = GetInputAge(txin);
        if(nTxAge < 5) { //1 less than the "send IX" gui requires, incase of a block propagating the network at the time
            LogPrintf("CreateNewLock -- Transaction not found / too new: nTxAge=%d, txid=%s\n", nTxAge, tx.GetHash().ToString());
            return 0;
        }
    }

    /*
        Use a blockheight newer than the input.
        This prevents attackers from using transaction mallibility to predict which masternodes
        they'll use.
    */
    int nBlockHeight = 0;
    {
        LOCK(cs_main);
        CBlockIndex* tip = chainActive.Tip();
        if(tip)
            nBlockHeight = tip->nHeight - nTxAge + 4;
        else
            return 0;
    }

    if(!mapTxLocks.count(tx.GetHash())) {
        LogPrintf("CreateNewLock -- New Transaction Lock! txid=%s\n", tx.GetHash().ToString());

        CTransactionLock newLock;
        newLock.nBlockHeight = nBlockHeight;
        //locks expire after nInstantSendKeepLock confirmations
        newLock.nLockExpirationBlock = chainActive.Height() + Params().GetConsensus().nInstantSendKeepLock;
        newLock.nTimeout = GetTime()+(60*5);
        newLock.txHash = tx.GetHash();
        mapTxLocks.insert(std::make_pair(tx.GetHash(), newLock));
    } else {
        mapTxLocks[tx.GetHash()].nBlockHeight = nBlockHeight;
        LogPrint("instantsend", "CreateNewLock -- Transaction Lock Exists! txid=%s\n", tx.GetHash().ToString());
    }



    return nBlockHeight;
}

// check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, int64_t nBlockHeight)
{
    if(!fMasterNode) return;

    int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_INSTANTSEND_PROTO_VERSION);

    if(n == -1) {
        LogPrint("instantsend", "DoConsensusVote -- Unknown Masternode %s\n", activeMasternode.vin.ToString());
        return;
    }

    if(n > INSTANTSEND_SIGNATURES_TOTAL) {
        LogPrint("instantsend", "DoConsensusVote -- Masternode not in the top %d (%d)\n", INSTANTSEND_SIGNATURES_TOTAL, n);
        return;
    }
    /*
        nBlockHeight calculated from the transaction is the authoritive source
    */

    LogPrint("instantsend", "DoConsensusVote -- In the top %d (%d)\n", INSTANTSEND_SIGNATURES_TOTAL, n);

    CConsensusVote vote;
    vote.vinMasternode = activeMasternode.vin;
    vote.txHash = tx.GetHash();
    vote.nBlockHeight = nBlockHeight;
    if(!vote.Sign()) {
        LogPrintf("DoConsensusVote -- Failed to sign consensus vote\n");
        return;
    }
    if(!vote.CheckSignature()) {
        LogPrintf("DoConsensusVote -- Signature invalid\n");
        return;
    }

    {
        LOCK(cs_instantsend);
        mapTxLockVote[vote.GetHash()] = vote;
    }

    CInv inv(MSG_TXLOCK_VOTE, vote.GetHash());
    RelayInv(inv);
}

//received a consensus vote
bool ProcessConsensusVote(CNode* pnode, CConsensusVote& vote)
{
    int n = mnodeman.GetMasternodeRank(vote.vinMasternode, vote.nBlockHeight, MIN_INSTANTSEND_PROTO_VERSION);

    CMasternode* pmn = mnodeman.Find(vote.vinMasternode);
    if(pmn != NULL)
        LogPrint("instantsend", "ProcessConsensusVote -- Masternode addr=%s, rank: %d\n", pmn->addr.ToString(), n);

    if(n == -1) {
        //can be caused by past versions trying to vote with an invalid protocol
        LogPrint("instantsend", "ProcessConsensusVote -- Unknown Masternode: txin=%s\n", vote.vinMasternode.ToString());
        mnodeman.AskForMN(pnode, vote.vinMasternode);
        return false;
    }

    if(n > INSTANTSEND_SIGNATURES_TOTAL) {
        LogPrint("instantsend", "ProcessConsensusVote -- Masternode not in the top %d (%d): vote hash %s\n", INSTANTSEND_SIGNATURES_TOTAL, n, vote.GetHash().ToString());
        return false;
    }

    if(!vote.CheckSignature()) {
        LogPrintf("ProcessConsensusVote -- Signature invalid\n");
        // don't ban, it could just be a non-synced masternode
        mnodeman.AskForMN(pnode, vote.vinMasternode);
        return false;
    }

    if (!mapTxLocks.count(vote.txHash)) {
        LogPrintf("ProcessConsensusVote -- New Transaction Lock! txid=%s\n", vote.txHash.ToString());

        CTransactionLock newLock;
        newLock.nBlockHeight = 0;
        //locks expire after nInstantSendKeepLock confirmations
        newLock.nLockExpirationBlock = chainActive.Height() + Params().GetConsensus().nInstantSendKeepLock;
        newLock.nTimeout = GetTime()+(60*5);
        newLock.txHash = vote.txHash;
        mapTxLocks.insert(std::make_pair(vote.txHash, newLock));
    } else {
        LogPrint("instantsend", "ProcessConsensusVote -- Transaction Lock Exists! txid=%s\n", vote.txHash.ToString());
    }

    //compile consessus vote
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(vote.txHash);
    if (i != mapTxLocks.end()) {
        (*i).second.AddVote(vote);

        int nSignatures = (*i).second.CountVotes();
        LogPrint("instantsend", "ProcessConsensusVote -- Transaction Lock signatures count: %d, vote hash=%s\n", nSignatures, vote.GetHash().ToString());

        if(nSignatures >= INSTANTSEND_SIGNATURES_REQUIRED) {
            LogPrint("instantsend", "ProcessConsensusVote -- Transaction Lock Is Complete! txid=%s\n", vote.txHash.ToString());

            // Masternodes will sometimes propagate votes before the transaction is known to the client,
            // will check for conflicting locks and update transaction status on a new vote message
            // only after the lock itself has arrived
            if(!mapTxLockReq.count(vote.txHash) && !mapTxLockReqRejected.count(vote.txHash)) return true;

            if(!FindConflictingLocks(mapTxLockReq[vote.txHash])) { //?????
                if(mapTxLockReq.count(vote.txHash)) {
                    UpdateLockedTransaction(mapTxLockReq[vote.txHash]);
                    LockTransactionInputs(mapTxLockReq[vote.txHash]);
                } else if(mapTxLockReqRejected.count(vote.txHash)) {
                    ResolveConflicts(mapTxLockReqRejected[vote.txHash]); ///?????
                } else {
                    LogPrint("instantsend", "ProcessConsensusVote -- Transaction Lock Request is missing! nSignatures=%d, vote hash %s\n", nSignatures, vote.GetHash().ToString());
                }
            }
        }
        return true;
    }


    return false;
}

void UpdateLockedTransaction(CTransaction& tx, bool fForceNotification)
{
    // there should be no conflicting locks
    if(FindConflictingLocks(tx)) return;
    uint256 txHash = tx.GetHash();
    // there must be a successfully verified lock request
    if(!mapTxLockReq.count(txHash)) return;

    int nSignatures = GetTransactionLockSignatures(txHash);

#ifdef ENABLE_WALLET
    if(pwalletMain && pwalletMain->UpdatedTransaction(txHash)) {
        // bumping this to update UI
        nCompleteTXLocks++;
        // a transaction lock must have enough signatures to trigger this notification
        if(nSignatures == INSTANTSEND_SIGNATURES_REQUIRED || (fForceNotification && nSignatures > INSTANTSEND_SIGNATURES_REQUIRED)) {
            // notify an external script once threshold is reached
            std::string strCmd = GetArg("-instantsendnotify", "");
            if(!strCmd.empty()) {
                boost::replace_all(strCmd, "%s", txHash.GetHex());
                boost::thread t(runCommand, strCmd); // thread runs free
            }
        }
    }
#endif

    if(nSignatures == INSTANTSEND_SIGNATURES_REQUIRED || (fForceNotification && nSignatures > INSTANTSEND_SIGNATURES_REQUIRED))
        GetMainSignals().NotifyTransactionLock(tx);
}

void LockTransactionInputs(CTransaction& tx) {
    if(!mapTxLockReq.count(tx.GetHash())) return;

    BOOST_FOREACH(const CTxIn& txin, tx.vin)
        if(!mapLockedInputs.count(txin.prevout))
            mapLockedInputs.insert(std::make_pair(txin.prevout, tx.GetHash()));
}

bool FindConflictingLocks(CTransaction& tx)
{
    /*
        It's possible (very unlikely though) to get 2 conflicting transaction locks approved by the network.
        In that case, they will cancel each other out.

        Blocks could have been rejected during this time, which is OK. After they cancel out, the client will
        rescan the blocks and find they're acceptable and then take the chain with the most work.
    */
    BOOST_FOREACH(const CTxIn& txin, tx.vin) {
        if(mapLockedInputs.count(txin.prevout)) {
            if(mapLockedInputs[txin.prevout] != tx.GetHash()) {
                LogPrintf("FindConflictingLocks -- found two complete conflicting locks, removing both: txid=%s, txin=%s", tx.GetHash().ToString(), mapLockedInputs[txin.prevout].ToString());

                if(mapTxLocks.count(tx.GetHash()))
                    mapTxLocks[tx.GetHash()].nLockExpirationBlock = -1;

                if(mapTxLocks.count(mapLockedInputs[txin.prevout]))
                    mapTxLocks[mapLockedInputs[txin.prevout]].nLockExpirationBlock = -1;

                return true;
            }
        }
    }

    return false;
}

void ResolveConflicts(CTransaction& tx)
{
    // resolve conflicts
    if (IsLockedInstandSendTransaction(tx.GetHash()) && !FindConflictingLocks(tx)) { //?????
        LogPrintf("ResolveConflicts -- Found Existing Complete IX Lock, resolving...\n");

        //reprocess the last nInstantSendReprocessBlocks blocks
        ReprocessBlocks(Params().GetConsensus().nInstantSendReprocessBlocks);
        if(!mapTxLockReq.count(tx.GetHash()))
            mapTxLockReq.insert(std::make_pair(tx.GetHash(), tx)); //?????
    }
}

int64_t GetAverageVoteTime()
{
    std::map<uint256, int64_t>::iterator it = mapUnknownVotes.begin();
    int64_t total = 0;
    int64_t count = 0;

    while(it != mapUnknownVotes.end()) {
        total+= it->second;
        count++;
        it++;
    }

    return total / count;
}

void CleanTransactionLocksList()
{
    LOCK(cs_instantsend);

    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.begin();

    int nHeight = chainActive.Height();
    while(it != mapTxLocks.end()) {
        CTransactionLock &txLock = it->second;
        if(nHeight > txLock.nLockExpirationBlock) {
            LogPrintf("Removing old transaction lock: txid=%s\n", txLock.txHash.ToString());

            if(mapTxLockReq.count(txLock.txHash)){
                CTransaction& tx = mapTxLockReq[txLock.txHash];

                BOOST_FOREACH(const CTxIn& txin, tx.vin)
                    mapLockedInputs.erase(txin.prevout);

                mapTxLockReq.erase(txLock.txHash);
                mapTxLockReqRejected.erase(txLock.txHash);

                BOOST_FOREACH(const CConsensusVote& vote, txLock.vecConsensusVotes)
                    if(mapTxLockVote.count(vote.GetHash()))
                        mapTxLockVote.erase(vote.GetHash());
            }

            mapTxLocks.erase(it++);
        } else {
            it++;
        }
    }
}

bool IsLockedInstandSendTransaction(uint256 txHash)
{
    // there must be a successfully verified lock request...
    if (!mapTxLockReq.count(txHash)) return false;
    // ...and corresponding lock must have enough signatures
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(txHash);
    return i != mapTxLocks.end() && (*i).second.CountVotes() >= INSTANTSEND_SIGNATURES_REQUIRED;
}

int GetTransactionLockSignatures(uint256 txHash)
{
    if(fLargeWorkForkFound || fLargeWorkInvalidChainFound) return -2;
    if(!sporkManager.IsSporkActive(SPORK_2_INSTANTSEND_ENABLED)) return -3;
    if(!fEnableInstantSend) return -1;

    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.find(txHash);
    if(it != mapTxLocks.end()) return it->second.CountVotes();

    return -1;
}

bool IsTransactionLockTimedOut(uint256 txHash)
{
    if(!fEnableInstantSend) return 0;

    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(txHash);
    if (i != mapTxLocks.end()) return GetTime() > (*i).second.nTimeout;

    return false;
}

uint256 CConsensusVote::GetHash() const
{
    return ArithToUint256(UintToArith256(vinMasternode.prevout.hash) + vinMasternode.prevout.n + UintToArith256(txHash));
}


bool CConsensusVote::CheckSignature()
{
    std::string strError;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("verify strMessage %s \n", strMessage);

    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if(pmn == NULL) {
        LogPrintf("CConsensusVote::CheckSignature -- Unknown Masternode: txin=%s\n", vinMasternode.ToString());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchMasterNodeSignature, strMessage, strError)) {
        LogPrintf("CConsensusVote::CheckSignature -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CConsensusVote::Sign()
{
    std::string strError;

    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);

    if(!darkSendSigner.SignMessage(strMessage, vchMasterNodeSignature, activeMasternode.keyMasternode)) {
        LogPrintf("CConsensusVote::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(activeMasternode.pubKeyMasternode, vchMasterNodeSignature, strMessage, strError)) {
        LogPrintf("CConsensusVote::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}


bool CTransactionLock::IsAllVotesValid()
{

    BOOST_FOREACH(CConsensusVote vote, vecConsensusVotes)
    {
        int n = mnodeman.GetMasternodeRank(vote.vinMasternode, vote.nBlockHeight, MIN_INSTANTSEND_PROTO_VERSION);

        if(n == -1) {
            LogPrintf("CTransactionLock::IsAllVotesValid -- Unknown Masternode, txin=%s\n", vote.vinMasternode.ToString());
            return false;
        }

        if(n > INSTANTSEND_SIGNATURES_TOTAL) {
            LogPrintf("CTransactionLock::IsAllVotesValid -- Masternode not in the top %s\n", INSTANTSEND_SIGNATURES_TOTAL);
            return false;
        }

        if(!vote.CheckSignature()) {
            LogPrintf("CTransactionLock::IsAllVotesValid -- Signature not valid\n");
            return false;
        }
    }

    return true;
}

void CTransactionLock::AddVote(CConsensusVote& vote)
{
    vecConsensusVotes.push_back(vote);
}

int CTransactionLock::CountVotes()
{
    /*
        Only count signatures where the BlockHeight matches the transaction's blockheight.
        The votes have no proof it's the correct blockheight
    */

    if(nBlockHeight == 0) return -1;

    int nCount = 0;
    BOOST_FOREACH(CConsensusVote vote, vecConsensusVotes)
        if(vote.nBlockHeight == nBlockHeight)
            nCount++;

    return nCount;
}
