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

using namespace std;
using namespace boost;

std::map<uint256, CTransaction> mapTxLockReq;
std::map<uint256, CTransaction> mapTxLockReqRejected;
std::map<uint256, CConsensusVote> mapTxLockVote;
std::map<uint256, CTransactionLock> mapTxLocks;
std::map<COutPoint, uint256> mapLockedInputs;
std::map<uint256, int64_t> mapUnknownVotes; //track votes with no tx for DOS
int nCompleteTXLocks;

//txlock - Locks transaction
//
//step 1.) Broadcast intention to lock transaction inputs, "txlreg", CTransaction
//step 2.) Top INSTANTX_SIGNATURES_TOTAL masternodes, open connect to top 1 masternode.
//         Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 masternode, waits for INSTANTX_SIGNATURES_REQUIRED messages. Upon success, sends "txlock'

void ProcessMessageInstantX(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/masternode related functionality
    if(!IsSporkActive(SPORK_2_INSTANTX)) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    if (strCommand == NetMsgType::IX)
    {
        //LogPrintf("ProcessMessageInstantX::ix\n");
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
        if(!IsIXTXValid(tx)) return;

        BOOST_FOREACH(const CTxOut o, tx.vout){
            // IX supports normal scripts and unspendable scripts (used in DS collateral and Budget collateral).
            // TODO: Look into other script types that are normal and can be included
            if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()){
                LogPrintf("ProcessMessageInstantX::ix - Invalid Script %s", tx.ToString());
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
        if (fAccepted)
        {
            RelayInv(inv);

            DoConsensusVote(tx, nBlockHeight);

            mapTxLockReq.insert(make_pair(tx.GetHash(), tx));

            LogPrintf("ProcessMessageInstantX::ix - Transaction Lock Request: %s %s : accepted %s\n",
                pfrom->addr.ToString(), pfrom->cleanSubVer,
                tx.GetHash().ToString()
            );

            // Masternodes will sometimes propagate votes before the transaction is known to the client.
            // If this just happened - update transaction status, try forcing external script notification,
            // lock inputs and resolve conflicting locks
            if(IsLockedIXTransaction(tx.GetHash())) {
                UpdateLockedTransaction(tx, true);
                LockTransactionInputs(tx);
                ResolveConflicts(tx);
            }

            return;

        } else {
            mapTxLockReqRejected.insert(make_pair(tx.GetHash(), tx));

            // can we get the conflicting transaction as proof?

            LogPrintf("ProcessMessageInstantX::ix - Transaction Lock Request: %s %s : rejected %s\n",
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
        CConsensusVote ctx;
        vRecv >> ctx;

        CInv inv(MSG_TXLOCK_VOTE, ctx.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapTxLockVote.count(ctx.GetHash())){
            return;
        }

        mapTxLockVote.insert(make_pair(ctx.GetHash(), ctx));

        if(ProcessConsensusVote(pfrom, ctx)){
            //Spam/Dos protection
            /*
                Masternodes will sometimes propagate votes before the transaction is known to the client.
                This tracks those messages and allows it at the same rate of the rest of the network, if
                a peer violates it, it will simply be ignored
            */
            if(!mapTxLockReq.count(ctx.txHash) && !mapTxLockReqRejected.count(ctx.txHash)){
                if(!mapUnknownVotes.count(ctx.vinMasternode.prevout.hash)){
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }

                if(mapUnknownVotes[ctx.vinMasternode.prevout.hash] > GetTime() &&
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] - GetAverageVoteTime() > 60*10){
                        LogPrintf("ProcessMessageInstantX::ix - masternode is spamming transaction votes: %s %s\n",
                            ctx.vinMasternode.ToString(),
                            ctx.txHash.ToString()
                        );
                        return;
                } else {
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }
            }
            RelayInv(inv);
        }

        return;
    }
}

bool IsIXTXValid(const CTransaction& txCollateral){
    if(txCollateral.vout.size() < 1) return false;

    if(!CheckFinalTx(txCollateral)) {
        LogPrint("instantsend", "IsIXTXValid - Transaction is not final - %s\n", txCollateral.ToString());
        return false;
    }

    int64_t nValueIn = 0;
    int64_t nValueOut = 0;
    bool missingTx = false;

    BOOST_FOREACH(const CTxOut o, txCollateral.vout)
        nValueOut += o.nValue;

    BOOST_FOREACH(const CTxIn i, txCollateral.vin){
        CTransaction tx2;
        uint256 hash;
        if(GetTransaction(i.prevout.hash, tx2, Params().GetConsensus(), hash, true)){
            if(tx2.vout.size() > i.prevout.n) {
                nValueIn += tx2.vout[i.prevout.n].nValue;
            }
        } else{
            missingTx = true;
        }
    }

    if(nValueOut > GetSporkValue(SPORK_5_MAX_VALUE)*COIN){
        LogPrint("instantsend", "IsIXTXValid - Transaction value too high - %s\n", txCollateral.ToString());
        return false;
    }

    if(missingTx){
        LogPrint("instantsend", "IsIXTXValid - Unknown inputs in IX transaction - %s\n", txCollateral.ToString());
        /*
            This happens sometimes for an unknown reason, so we'll return that it's a valid transaction.
            If someone submits an invalid transaction it will be rejected by the network anyway and this isn't
            very common, but we don't want to block IX just because the client can't figure out the fee.
        */
        return true;
    }

    if(nValueIn-nValueOut < INSTANTSEND_MIN_FEE) {
        LogPrint("instantsend", "IsIXTXValid - did not include enough fees in transaction %d\n%s\n", nValueOut-nValueIn, txCollateral.ToString());
        return false;
    }

    return true;
}

int64_t CreateNewLock(CTransaction tx)
{

    int64_t nTxAge = 0;
    BOOST_REVERSE_FOREACH(CTxIn i, tx.vin){
        nTxAge = GetInputAge(i);
        if(nTxAge < 5) //1 less than the "send IX" gui requires, incase of a block propagating the network at the time
        {
            LogPrintf("CreateNewLock - Transaction not found / too new: %d / %s\n", nTxAge, tx.GetHash().ToString());
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
        if(tip) nBlockHeight = tip->nHeight - nTxAge + 4;
        else return 0;
    }

    if (!mapTxLocks.count(tx.GetHash())){
        LogPrintf("CreateNewLock - New Transaction Lock %s !\n", tx.GetHash().ToString());

        CTransactionLock newLock;
        newLock.nBlockHeight = nBlockHeight;
        newLock.nExpiration = GetTime()+(60*60); //locks expire after 60 minutes (24 confirmations)
        newLock.nTimeout = GetTime()+(60*5);
        newLock.txHash = tx.GetHash();
        mapTxLocks.insert(make_pair(tx.GetHash(), newLock));
    } else {
        mapTxLocks[tx.GetHash()].nBlockHeight = nBlockHeight;
        LogPrint("instantsend", "CreateNewLock - Transaction Lock Exists %s !\n", tx.GetHash().ToString());
    }



    return nBlockHeight;
}

// check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, int64_t nBlockHeight)
{
    if(!fMasterNode) return;

    int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    if(n == -1)
    {
        LogPrint("instantsend", "InstantX::DoConsensusVote - Unknown Masternode %s\n", activeMasternode.vin.ToString());
        return;
    }

    if(n > INSTANTX_SIGNATURES_TOTAL)
    {
        LogPrint("instantsend", "InstantX::DoConsensusVote - Masternode not in the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);
        return;
    }
    /*
        nBlockHeight calculated from the transaction is the authoritive source
    */

    LogPrint("instantsend", "InstantX::DoConsensusVote - In the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);

    CConsensusVote ctx;
    ctx.vinMasternode = activeMasternode.vin;
    ctx.txHash = tx.GetHash();
    ctx.nBlockHeight = nBlockHeight;
    if(!ctx.Sign()){
        LogPrintf("InstantX::DoConsensusVote - Failed to sign consensus vote\n");
        return;
    }
    if(!ctx.SignatureValid()) {
        LogPrintf("InstantX::DoConsensusVote - Signature invalid\n");
        return;
    }

    mapTxLockVote[ctx.GetHash()] = ctx;

    CInv inv(MSG_TXLOCK_VOTE, ctx.GetHash());
    RelayInv(inv);
}

//received a consensus vote
bool ProcessConsensusVote(CNode* pnode, CConsensusVote& ctx)
{
    int n = mnodeman.GetMasternodeRank(ctx.vinMasternode, ctx.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    CMasternode* pmn = mnodeman.Find(ctx.vinMasternode);
    if(pmn != NULL)
        LogPrint("instantsend", "InstantX::ProcessConsensusVote - Masternode ADDR %s %d\n", pmn->addr.ToString(), n);

    if(n == -1)
    {
        //can be caused by past versions trying to vote with an invalid protocol
        LogPrint("instantsend", "InstantX::ProcessConsensusVote - Unknown Masternode %s\n", ctx.vinMasternode.ToString());
        mnodeman.AskForMN(pnode, ctx.vinMasternode);
        return false;
    }

    if(n > INSTANTX_SIGNATURES_TOTAL)
    {
        LogPrint("instantsend", "InstantX::ProcessConsensusVote - Masternode not in the top %d (%d) - %s\n", INSTANTX_SIGNATURES_TOTAL, n, ctx.GetHash().ToString());
        return false;
    }

    if(!ctx.SignatureValid()) {
        LogPrintf("InstantX::ProcessConsensusVote - Signature invalid\n");
        // don't ban, it could just be a non-synced masternode
        mnodeman.AskForMN(pnode, ctx.vinMasternode);
        return false;
    }

    if (!mapTxLocks.count(ctx.txHash)){
        LogPrintf("InstantX::ProcessConsensusVote - New Transaction Lock %s !\n", ctx.txHash.ToString());

        CTransactionLock newLock;
        newLock.nBlockHeight = 0;
        newLock.nExpiration = GetTime()+(60*60);
        newLock.nTimeout = GetTime()+(60*5);
        newLock.txHash = ctx.txHash;
        mapTxLocks.insert(make_pair(ctx.txHash, newLock));
    } else
        LogPrint("instantsend", "InstantX::ProcessConsensusVote - Transaction Lock Exists %s !\n", ctx.txHash.ToString());

    //compile consessus vote
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(ctx.txHash);
    if (i != mapTxLocks.end()){
        (*i).second.AddSignature(ctx);

        int nSignatures = (*i).second.CountSignatures();
        LogPrint("instantsend", "InstantX::ProcessConsensusVote - Transaction Lock Votes %d - %s !\n", nSignatures, ctx.GetHash().ToString());

        if(nSignatures >= INSTANTX_SIGNATURES_REQUIRED){
            LogPrint("instantsend", "InstantX::ProcessConsensusVote - Transaction Lock Is Complete %s !\n", ctx.txHash.ToString());

            // Masternodes will sometimes propagate votes before the transaction is known to the client,
            // will check for conflicting locks and update transaction status on a new vote message
            // only after the lock itself has arrived
            if(!mapTxLockReq.count(ctx.txHash) && !mapTxLockReqRejected.count(ctx.txHash)) return true;

            if(!FindConflictingLocks(mapTxLockReq[ctx.txHash])) { //?????
                if(mapTxLockReq.count(ctx.txHash)) {
                    UpdateLockedTransaction(mapTxLockReq[ctx.txHash]);
                    LockTransactionInputs(mapTxLockReq[ctx.txHash]);
                } else if(mapTxLockReqRejected.count(ctx.txHash)) {
                    ResolveConflicts(mapTxLockReqRejected[ctx.txHash]); ///?????
                } else {
                    LogPrint("instantsend", "InstantX::ProcessConsensusVote - Transaction Lock Request is missing %s ! votes %d\n", ctx.GetHash().ToString(), nSignatures);
                }
            }
        }
        return true;
    }


    return false;
}

void UpdateLockedTransaction(CTransaction& tx, bool fForceNotification) {
    // there should be no conflicting locks
    if(FindConflictingLocks(tx)) return;
    uint256 txHash = tx.GetHash();
    // there must be a successfully verified lock request
    if (!mapTxLockReq.count(txHash)) return;

#ifdef ENABLE_WALLET
    if(pwalletMain && pwalletMain->UpdatedTransaction(txHash)){
        // bumping this to update UI
        nCompleteTXLocks++;
        int nSignatures = GetTransactionLockSignatures(txHash);
        // a transaction lock must have enough signatures to trigger this notification
        if(nSignatures == INSTANTX_SIGNATURES_REQUIRED || (fForceNotification && nSignatures > INSTANTX_SIGNATURES_REQUIRED)) {
            // notify an external script once threshold is reached
            std::string strCmd = GetArg("-instantsendnotify", "");
            if ( !strCmd.empty())
            {
                boost::replace_all(strCmd, "%s", txHash.GetHex());
                boost::thread t(runCommand, strCmd); // thread runs free
            }
        }
    }
#endif
}

void LockTransactionInputs(CTransaction& tx) {
    if(mapTxLockReq.count(tx.GetHash())){
        BOOST_FOREACH(const CTxIn& in, tx.vin){
            if(!mapLockedInputs.count(in.prevout)){
                mapLockedInputs.insert(make_pair(in.prevout, tx.GetHash()));
            }
        }
    }
}

bool FindConflictingLocks(CTransaction& tx)
{
    /*
        It's possible (very unlikely though) to get 2 conflicting transaction locks approved by the network.
        In that case, they will cancel each other out.

        Blocks could have been rejected during this time, which is OK. After they cancel out, the client will
        rescan the blocks and find they're acceptable and then take the chain with the most work.
    */
    BOOST_FOREACH(const CTxIn& in, tx.vin){
        if(mapLockedInputs.count(in.prevout)){
            if(mapLockedInputs[in.prevout] != tx.GetHash()){
                LogPrintf("InstantX::FindConflictingLocks - found two complete conflicting locks - removing both. %s %s", tx.GetHash().ToString(), mapLockedInputs[in.prevout].ToString());
                if(mapTxLocks.count(tx.GetHash())) mapTxLocks[tx.GetHash()].nExpiration = GetTime();
                if(mapTxLocks.count(mapLockedInputs[in.prevout])) mapTxLocks[mapLockedInputs[in.prevout]].nExpiration = GetTime();
                return true;
            }
        }
    }

    return false;
}

void ResolveConflicts(CTransaction& tx) {
    // resolve conflicts
    if (IsLockedIXTransaction(tx.GetHash()) && !FindConflictingLocks(tx)){ //?????
        LogPrintf("ResolveConflicts - Found Existing Complete IX Lock, resolving...\n");

        //reprocess the last 15 blocks
        ReprocessBlocks(15);
        if(!mapTxLockReq.count(tx.GetHash())) mapTxLockReq.insert(make_pair(tx.GetHash(), tx)); //?????
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
    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.begin();

    while(it != mapTxLocks.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old transaction lock %s\n", it->second.txHash.ToString());

            if(mapTxLockReq.count(it->second.txHash)){
                CTransaction& tx = mapTxLockReq[it->second.txHash];

                BOOST_FOREACH(const CTxIn& in, tx.vin)
                    mapLockedInputs.erase(in.prevout);

                mapTxLockReq.erase(it->second.txHash);
                mapTxLockReqRejected.erase(it->second.txHash);

                BOOST_FOREACH(CConsensusVote& v, it->second.vecConsensusVotes)
                    mapTxLockVote.erase(v.GetHash());
            }

            mapTxLocks.erase(it++);
        } else {
            it++;
        }
    }
}

bool IsLockedIXTransaction(uint256 txHash) {
    // there must be a successfully verified lock request...
    if (!mapTxLockReq.count(txHash)) return false;
    // ...and corresponding lock must have enough signatures
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(txHash);
    return i != mapTxLocks.end() && (*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED;
}

int GetTransactionLockSignatures(uint256 txHash)
{
    if(fLargeWorkForkFound || fLargeWorkInvalidChainFound) return -2;
    if(!IsSporkActive(SPORK_2_INSTANTX)) return -3;
    if(!fEnableInstantSend) return -1;

    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(txHash);
    if (i != mapTxLocks.end()){
        return (*i).second.CountSignatures();
    }

    return -1;
}

bool IsTransactionLockTimedOut(uint256 txHash)
{
    if(!fEnableInstantSend) return 0;

    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(txHash);
    if (i != mapTxLocks.end()){
        return GetTime() > (*i).second.nTimeout;
    }

    return false;
}

uint256 CConsensusVote::GetHash() const
{
    return ArithToUint256(UintToArith256(vinMasternode.prevout.hash) + vinMasternode.prevout.n + UintToArith256(txHash));
}


bool CConsensusVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("verify strMessage %s \n", strMessage);

    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if(pmn == NULL)
    {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Unknown Masternode %s\n", vinMasternode.ToString());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CConsensusVote::Sign()
{
    std::string errorMessage;

    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("signing strMessage %s \n", strMessage);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, activeMasternode.keyMasternode)) {
        LogPrintf("CConsensusVote::Sign() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(activeMasternode.pubKeyMasternode, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CConsensusVote::Sign() - Verify message failed");
        return false;
    }

    return true;
}


bool CTransactionLock::SignaturesValid()
{

    BOOST_FOREACH(CConsensusVote vote, vecConsensusVotes)
    {
        int n = mnodeman.GetMasternodeRank(vote.vinMasternode, vote.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

        if(n == -1)
        {
            LogPrintf("CTransactionLock::SignaturesValid() - Unknown Masternode %s\n", vote.vinMasternode.ToString());
            return false;
        }

        if(n > INSTANTX_SIGNATURES_TOTAL)
        {
            LogPrintf("CTransactionLock::SignaturesValid() - Masternode not in the top %s\n", INSTANTX_SIGNATURES_TOTAL);
            return false;
        }

        if(!vote.SignatureValid()){
            LogPrintf("CTransactionLock::SignaturesValid() - Signature not valid\n");
            return false;
        }
    }

    return true;
}

void CTransactionLock::AddSignature(CConsensusVote& cv)
{
    vecConsensusVotes.push_back(cv);
}

int CTransactionLock::CountSignatures()
{
    /*
        Only count signatures where the BlockHeight matches the transaction's blockheight.
        The votes have no proof it's the correct blockheight
    */

    if(nBlockHeight == 0) return -1;

    int n = 0;
    BOOST_FOREACH(CConsensusVote v, vecConsensusVotes){
        if(v.nBlockHeight == nBlockHeight){
            n++;
        }
    }
    return n;
}
