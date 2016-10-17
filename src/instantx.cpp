


<<<<<<< HEAD
=======
#include "bignum.h"
>>>>>>> origin/dirty-merge-dash-0.11.0
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
<<<<<<< HEAD
#include "base58.h"
#include "protocol.h"
#include "instantx.h"
#include "activemasternode.h"
#include "masternodeman.h"
=======
#include "script.h"
#include "base58.h"
#include "protocol.h"
#include "instantx.h"
#include "activethrone.h"
#include "throneman.h"
>>>>>>> origin/dirty-merge-dash-0.11.0
#include "darksend.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>

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
<<<<<<< HEAD
//step 2.) Top INSTANTX_SIGNATURES_TOTAL masternodes, open connect to top 1 masternode.
//         Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 masternode, waits for INSTANTX_SIGNATURES_REQUIRED messages. Upon success, sends "txlock'

void ProcessMessageInstantX(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/masternode related functionality
    if(!IsSporkActive(SPORK_2_INSTANTX)) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    if (strCommand == "ix")
    {
        //LogPrintf("ProcessMessageInstantX::ix\n");
=======
//step 2.) Top 10 thrones, open connect to top 1 throne. Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 throne, waits for 10 messages. Upon success, sends "txlock'

void ProcessMessageInstantX(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/throne related functionality
    if(!IsSporkActive(SPORK_2_INSTANTX)) return;
    if(IsInitialBlockDownload()) return;

    if (strCommand == "txlreq")
    {
        //LogPrintf("ProcessMessageInstantX::txlreq\n");
>>>>>>> origin/dirty-merge-dash-0.11.0
        CDataStream vMsg(vRecv);
        CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TXLOCK_REQUEST, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapTxLockReq.count(tx.GetHash()) || mapTxLockReqRejected.count(tx.GetHash())){
            return;
        }

        if(!IsIXTXValid(tx)){
            return;
        }

        BOOST_FOREACH(const CTxOut o, tx.vout){
<<<<<<< HEAD
            // IX supports normal scripts and unspendable scripts (used in DS collateral and Budget collateral).
            // TODO: Look into other script types that are normal and can be included
            if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()){
                LogPrintf("ProcessMessageInstantX::ix - Invalid Script %s\n", tx.ToString().c_str());
=======
            if(!o.scriptPubKey.IsNormalPaymentScript()){
                LogPrintf("ProcessMessageInstantX::txlreq - Invalid Script %s\n", tx.ToString().c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0
                return;
            }
        }

        int nBlockHeight = CreateNewLock(tx);

        bool fMissingInputs = false;
        CValidationState state;

<<<<<<< HEAD
        bool fAccepted = false;
        {
            LOCK(cs_main);
            fAccepted = AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs);
        }
        if (fAccepted)
        {
            RelayInv(inv);
=======

        if (AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs))
        {
            vector<CInv> vInv;
            vInv.push_back(inv);
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
                pnode->PushMessage("inv", vInv);
>>>>>>> origin/dirty-merge-dash-0.11.0

            DoConsensusVote(tx, nBlockHeight);

            mapTxLockReq.insert(make_pair(tx.GetHash(), tx));

<<<<<<< HEAD
            LogPrintf("ProcessMessageInstantX::ix - Transaction Lock Request: %s %s : accepted %s\n",
=======
            LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request: %s %s : accepted %s\n",
>>>>>>> origin/dirty-merge-dash-0.11.0
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str()
            );

            return;

        } else {
            mapTxLockReqRejected.insert(make_pair(tx.GetHash(), tx));

            // can we get the conflicting transaction as proof?

<<<<<<< HEAD
            LogPrintf("ProcessMessageInstantX::ix - Transaction Lock Request: %s %s : rejected %s\n",
=======
            LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request: %s %s : rejected %s\n",
>>>>>>> origin/dirty-merge-dash-0.11.0
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str()
            );

            BOOST_FOREACH(const CTxIn& in, tx.vin){
                if(!mapLockedInputs.count(in.prevout)){
                    mapLockedInputs.insert(make_pair(in.prevout, tx.GetHash()));
                }
            }

            // resolve conflicts
            std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(tx.GetHash());
            if (i != mapTxLocks.end()){
                //we only care if we have a complete tx lock
                if((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED){
                    if(!CheckForConflictingLocks(tx)){
<<<<<<< HEAD
                        LogPrintf("ProcessMessageInstantX::ix - Found Existing Complete IX Lock\n");

                        //reprocess the last 15 blocks
                        ReprocessBlocks(15);
=======
                        LogPrintf("ProcessMessageInstantX::txlreq - Found Existing Complete IX Lock\n");

                        CValidationState state;
                        DisconnectBlockAndInputs(state, tx);
>>>>>>> origin/dirty-merge-dash-0.11.0
                        mapTxLockReq.insert(make_pair(tx.GetHash(), tx));
                    }
                }
            }

            return;
        }
    }
    else if (strCommand == "txlvote") //InstantX Lock Consensus Votes
    {
        CConsensusVote ctx;
        vRecv >> ctx;

        CInv inv(MSG_TXLOCK_VOTE, ctx.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapTxLockVote.count(ctx.GetHash())){
            return;
        }

        mapTxLockVote.insert(make_pair(ctx.GetHash(), ctx));

<<<<<<< HEAD
        if(ProcessConsensusVote(pfrom, ctx)){
            //Spam/Dos protection
            /*
                Masternodes will sometimes propagate votes before the transaction is known to the client.
=======
        if(ProcessConsensusVote(ctx)){
            //Spam/Dos protection
            /*
                Thrones will sometimes propagate votes before the transaction is known to the client.
>>>>>>> origin/dirty-merge-dash-0.11.0
                This tracks those messages and allows it at the same rate of the rest of the network, if
                a peer violates it, it will simply be ignored
            */
            if(!mapTxLockReq.count(ctx.txHash) && !mapTxLockReqRejected.count(ctx.txHash)){
<<<<<<< HEAD
                if(!mapUnknownVotes.count(ctx.vinMasternode.prevout.hash)){
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }

                if(mapUnknownVotes[ctx.vinMasternode.prevout.hash] > GetTime() &&
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] - GetAverageVoteTime() > 60*10){
                        LogPrintf("ProcessMessageInstantX::ix - masternode is spamming transaction votes: %s %s\n",
                            ctx.vinMasternode.ToString().c_str(),
=======
                if(!mapUnknownVotes.count(ctx.vinThrone.prevout.hash)){
                    mapUnknownVotes[ctx.vinThrone.prevout.hash] = GetTime()+(60*10);
                }

                if(mapUnknownVotes[ctx.vinThrone.prevout.hash] > GetTime() &&
                    mapUnknownVotes[ctx.vinThrone.prevout.hash] - GetAverageVoteTime() > 60*10){
                        LogPrintf("ProcessMessageInstantX::txlreq - throne is spamming transaction votes: %s %s\n",
                            ctx.vinThrone.ToString().c_str(),
>>>>>>> origin/dirty-merge-dash-0.11.0
                            ctx.txHash.ToString().c_str()
                        );
                        return;
                } else {
<<<<<<< HEAD
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }
            }
            RelayInv(inv);
=======
                    mapUnknownVotes[ctx.vinThrone.prevout.hash] = GetTime()+(60*10);
                }
            }
            vector<CInv> vInv;
            vInv.push_back(inv);
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
                pnode->PushMessage("inv", vInv);

>>>>>>> origin/dirty-merge-dash-0.11.0
        }

        return;
    }
}

bool IsIXTXValid(const CTransaction& txCollateral){
    if(txCollateral.vout.size() < 1) return false;
    if(txCollateral.nLockTime != 0) return false;

    int64_t nValueIn = 0;
    int64_t nValueOut = 0;
    bool missingTx = false;

    BOOST_FOREACH(const CTxOut o, txCollateral.vout)
        nValueOut += o.nValue;

    BOOST_FOREACH(const CTxIn i, txCollateral.vin){
        CTransaction tx2;
        uint256 hash;
        if(GetTransaction(i.prevout.hash, tx2, hash, true)){
            if(tx2.vout.size() > i.prevout.n) {
                nValueIn += tx2.vout[i.prevout.n].nValue;
            }
        } else{
            missingTx = true;
        }
    }

    if(nValueOut > GetSporkValue(SPORK_5_MAX_VALUE)*COIN){
<<<<<<< HEAD
        LogPrint("instantx", "IsIXTXValid - Transaction value too high - %s\n", txCollateral.ToString().c_str());
=======
        if(fDebug) LogPrintf ("IsIXTXValid - Transaction value too high - %s\n", txCollateral.ToString().c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    if(missingTx){
<<<<<<< HEAD
        LogPrint("instantx", "IsIXTXValid - Unknown inputs in IX transaction - %s\n", txCollateral.ToString().c_str());
=======
        if(fDebug) LogPrintf ("IsIXTXValid - Unknown inputs in IX transaction - %s\n", txCollateral.ToString().c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0
        /*
            This happens sometimes for an unknown reason, so we'll return that it's a valid transaction.
            If someone submits an invalid transaction it will be rejected by the network anyway and this isn't
            very common, but we don't want to block IX just because the client can't figure out the fee.
        */
        return true;
    }

    if(nValueIn-nValueOut < COIN*0.01) {
<<<<<<< HEAD
        LogPrint("instantx", "IsIXTXValid - did not include enough fees in transaction %d\n%s\n", nValueOut-nValueIn, txCollateral.ToString().c_str());
=======
        if(fDebug) LogPrintf ("IsIXTXValid - did not include enough fees in transaction %d\n%s\n", nValueOut-nValueIn, txCollateral.ToString().c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    return true;
}

int64_t CreateNewLock(CTransaction tx)
{

    int64_t nTxAge = 0;
    BOOST_REVERSE_FOREACH(CTxIn i, tx.vin){
        nTxAge = GetInputAge(i);
<<<<<<< HEAD
        if(nTxAge < 5) //1 less than the "send IX" gui requires, incase of a block propagating the network at the time
=======
        if(nTxAge < 6)
>>>>>>> origin/dirty-merge-dash-0.11.0
        {
            LogPrintf("CreateNewLock - Transaction not found / too new: %d / %s\n", nTxAge, tx.GetHash().ToString().c_str());
            return 0;
        }
    }

    /*
        Use a blockheight newer than the input.
<<<<<<< HEAD
        This prevents attackers from using transaction mallibility to predict which masternodes
=======
        This prevents attackers from using transaction mallibility to predict which thrones
>>>>>>> origin/dirty-merge-dash-0.11.0
        they'll use.
    */
    int nBlockHeight = (chainActive.Tip()->nHeight - nTxAge)+4;

    if (!mapTxLocks.count(tx.GetHash())){
        LogPrintf("CreateNewLock - New Transaction Lock %s !\n", tx.GetHash().ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = nBlockHeight;
<<<<<<< HEAD
        newLock.nExpiration = GetTime()+(60*60); //locks expire after 60 minutes (24 confirmations)
=======
        newLock.nExpiration = GetTime()+(60*60); //locks expire after 60 minutes (6 confirmations)
>>>>>>> origin/dirty-merge-dash-0.11.0
        newLock.nTimeout = GetTime()+(60*5);
        newLock.txHash = tx.GetHash();
        mapTxLocks.insert(make_pair(tx.GetHash(), newLock));
    } else {
        mapTxLocks[tx.GetHash()].nBlockHeight = nBlockHeight;
<<<<<<< HEAD
        LogPrint("instantx", "CreateNewLock - Transaction Lock Exists %s !\n", tx.GetHash().ToString().c_str());
=======
        if(fDebug) LogPrintf("CreateNewLock - Transaction Lock Exists %s !\n", tx.GetHash().ToString().c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0
    }



    return nBlockHeight;
}

// check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, int64_t nBlockHeight)
{
<<<<<<< HEAD
    if(!fMasterNode) return;

    int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    if(n == -1)
    {
        LogPrint("instantx", "InstantX::DoConsensusVote - Unknown Masternode\n");
=======
    if(!fThroNe) return;

    int n = mnodeman.GetThroneRank(activeThrone.vin, nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    if(n == -1)
    {
        if(fDebug) LogPrintf("InstantX::DoConsensusVote - Unknown Throne\n");
>>>>>>> origin/dirty-merge-dash-0.11.0
        return;
    }

    if(n > INSTANTX_SIGNATURES_TOTAL)
    {
<<<<<<< HEAD
        LogPrint("instantx", "InstantX::DoConsensusVote - Masternode not in the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);
=======
        if(fDebug) LogPrintf("InstantX::DoConsensusVote - Throne not in the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);
>>>>>>> origin/dirty-merge-dash-0.11.0
        return;
    }
    /*
        nBlockHeight calculated from the transaction is the authoritive source
    */

<<<<<<< HEAD
    LogPrint("instantx", "InstantX::DoConsensusVote - In the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);

    CConsensusVote ctx;
    ctx.vinMasternode = activeMasternode.vin;
=======
    if(fDebug) LogPrintf("InstantX::DoConsensusVote - In the top %d (%d)\n", INSTANTX_SIGNATURES_TOTAL, n);

    CConsensusVote ctx;
    ctx.vinThrone = activeThrone.vin;
>>>>>>> origin/dirty-merge-dash-0.11.0
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
<<<<<<< HEAD
    RelayInv(inv);
}

//received a consensus vote
bool ProcessConsensusVote(CNode* pnode, CConsensusVote& ctx)
{
    int n = mnodeman.GetMasternodeRank(ctx.vinMasternode, ctx.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    CMasternode* pmn = mnodeman.Find(ctx.vinMasternode);
    if(pmn != NULL)
        LogPrint("instantx", "InstantX::ProcessConsensusVote - Masternode ADDR %s %d\n", pmn->addr.ToString().c_str(), n);
=======

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }

}

//received a consensus vote
bool ProcessConsensusVote(CConsensusVote& ctx)
{
    int n = mnodeman.GetThroneRank(ctx.vinThrone, ctx.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    CThrone* pmn = mnodeman.Find(ctx.vinThrone);
    if(pmn != NULL)
    {
        if(fDebug) LogPrintf("InstantX::ProcessConsensusVote - Throne ADDR %s %d\n", pmn->addr.ToString().c_str(), n);
    }
>>>>>>> origin/dirty-merge-dash-0.11.0

    if(n == -1)
    {
        //can be caused by past versions trying to vote with an invalid protocol
<<<<<<< HEAD
        LogPrint("instantx", "InstantX::ProcessConsensusVote - Unknown Masternode\n");
        mnodeman.AskForMN(pnode, ctx.vinMasternode);
=======
        if(fDebug) LogPrintf("InstantX::ProcessConsensusVote - Unknown Throne\n");
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    if(n > INSTANTX_SIGNATURES_TOTAL)
    {
<<<<<<< HEAD
        LogPrint("instantx", "InstantX::ProcessConsensusVote - Masternode not in the top %d (%d) - %s\n", INSTANTX_SIGNATURES_TOTAL, n, ctx.GetHash().ToString().c_str());
=======
        if(fDebug) LogPrintf("InstantX::ProcessConsensusVote - Throne not in the top %d (%d) - %s\n", INSTANTX_SIGNATURES_TOTAL, n, ctx.GetHash().ToString().c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    if(!ctx.SignatureValid()) {
        LogPrintf("InstantX::ProcessConsensusVote - Signature invalid\n");
<<<<<<< HEAD
        // don't ban, it could just be a non-synced masternode
        mnodeman.AskForMN(pnode, ctx.vinMasternode);
=======
        //don't ban, it could just be a non-synced throne
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    if (!mapTxLocks.count(ctx.txHash)){
        LogPrintf("InstantX::ProcessConsensusVote - New Transaction Lock %s !\n", ctx.txHash.ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = 0;
        newLock.nExpiration = GetTime()+(60*60);
        newLock.nTimeout = GetTime()+(60*5);
        newLock.txHash = ctx.txHash;
        mapTxLocks.insert(make_pair(ctx.txHash, newLock));
<<<<<<< HEAD
    } else
        LogPrint("instantx", "InstantX::ProcessConsensusVote - Transaction Lock Exists %s !\n", ctx.txHash.ToString().c_str());
=======
    } else {
        if(fDebug) LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Exists %s !\n", ctx.txHash.ToString().c_str());
    }
>>>>>>> origin/dirty-merge-dash-0.11.0

    //compile consessus vote
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(ctx.txHash);
    if (i != mapTxLocks.end()){
        (*i).second.AddSignature(ctx);

#ifdef ENABLE_WALLET
        if(pwalletMain){
            //when we get back signatures, we'll count them as requests. Otherwise the client will think it didn't propagate.
            if(pwalletMain->mapRequestCount.count(ctx.txHash))
                pwalletMain->mapRequestCount[ctx.txHash]++;
        }
#endif

<<<<<<< HEAD
        LogPrint("instantx", "InstantX::ProcessConsensusVote - Transaction Lock Votes %d - %s !\n", (*i).second.CountSignatures(), ctx.GetHash().ToString().c_str());

        if((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED){
            LogPrint("instantx", "InstantX::ProcessConsensusVote - Transaction Lock Is Complete %s !\n", (*i).second.GetHash().ToString().c_str());
=======
        if(fDebug) LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Votes %d - %s !\n", (*i).second.CountSignatures(), ctx.GetHash().ToString().c_str());

        if((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED){
            if(fDebug) LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Is Complete %s !\n", (*i).second.GetHash().ToString().c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0

            CTransaction& tx = mapTxLockReq[ctx.txHash];
            if(!CheckForConflictingLocks(tx)){

#ifdef ENABLE_WALLET
                if(pwalletMain){
                    if(pwalletMain->UpdatedTransaction((*i).second.txHash)){
                        nCompleteTXLocks++;
                    }
                }
#endif

                if(mapTxLockReq.count(ctx.txHash)){
                    BOOST_FOREACH(const CTxIn& in, tx.vin){
                        if(!mapLockedInputs.count(in.prevout)){
                            mapLockedInputs.insert(make_pair(in.prevout, ctx.txHash));
                        }
                    }
                }

                // resolve conflicts

                //if this tx lock was rejected, we need to remove the conflicting blocks
                if(mapTxLockReqRejected.count((*i).second.txHash)){
<<<<<<< HEAD
                    //reprocess the last 15 blocks
                    ReprocessBlocks(15);
=======
                    CValidationState state;
                    DisconnectBlockAndInputs(state, mapTxLockReqRejected[(*i).second.txHash]);
>>>>>>> origin/dirty-merge-dash-0.11.0
                }
            }
        }
        return true;
    }


    return false;
}

bool CheckForConflictingLocks(CTransaction& tx)
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
                LogPrintf("InstantX::CheckForConflictingLocks - found two complete conflicting locks - removing both. %s %s", tx.GetHash().ToString().c_str(), mapLockedInputs[in.prevout].ToString().c_str());
                if(mapTxLocks.count(tx.GetHash())) mapTxLocks[tx.GetHash()].nExpiration = GetTime();
                if(mapTxLocks.count(mapLockedInputs[in.prevout])) mapTxLocks[mapLockedInputs[in.prevout]].nExpiration = GetTime();
                return true;
            }
        }
    }

    return false;
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
    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.begin();

    while(it != mapTxLocks.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old transaction lock %s\n", it->second.txHash.ToString().c_str());

<<<<<<< HEAD
=======
            // loop through thrones that responded
            for(int nRank = 0; nRank <= INSTANTX_SIGNATURES_TOTAL; nRank++)
            {
                CThrone* pmn = mnodeman.GetThroneByRank(nRank, it->second.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);
                if(!pmn) continue;

                bool fFound = false;
                BOOST_FOREACH(CConsensusVote& v, it->second.vecConsensusVotes)
                {
                    if(pmn->vin == v.vinThrone){ //Throne responded
                        fFound = true;
                    }
                }

                if(!fFound){
                    //increment a scanning error
                    CThroneScanningError mnse(pmn->vin, SCANNING_ERROR_IX_NO_RESPONSE, it->second.nBlockHeight);
                    pmn->ApplyScanningError(mnse);
                }
            }

>>>>>>> origin/dirty-merge-dash-0.11.0
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

uint256 CConsensusVote::GetHash() const
{
<<<<<<< HEAD
    return vinMasternode.prevout.hash + vinMasternode.prevout.n + txHash;
=======
    return vinThrone.prevout.hash + vinThrone.prevout.n + txHash;
>>>>>>> origin/dirty-merge-dash-0.11.0
}


bool CConsensusVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("verify strMessage %s \n", strMessage.c_str());

<<<<<<< HEAD
    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if(pmn == NULL)
    {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
=======
    CThrone* pmn = mnodeman.Find(vinThrone);

    if(pmn == NULL)
    {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Unknown Throne\n");
        return false;
    }

    //LogPrintf("verify addr %s \n", vecThrones[0].addr.ToString().c_str());
    //LogPrintf("verify addr %s \n", vecThrones[1].addr.ToString().c_str());
    //LogPrintf("verify addr %d %s \n", n, vecThrones[n].addr.ToString().c_str());

    CScript pubkey;
    pubkey.SetDestination(pmn->pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CCrowncoinAddress address2(address1);
    //LogPrintf("verify pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchThroNeSignature, strMessage, errorMessage)) {
>>>>>>> origin/dirty-merge-dash-0.11.0
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CConsensusVote::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("signing strMessage %s \n", strMessage.c_str());
<<<<<<< HEAD
    //LogPrintf("signing privkey %s \n", strMasterNodePrivKey.c_str());

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CConsensusVote::Sign() - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, key2)) {
        LogPrintf("CConsensusVote::Sign() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CConsensusVote::Sign() - Verify message failed");
=======
    //LogPrintf("signing privkey %s \n", strThroNePrivKey.c_str());

    if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CActiveThrone::RegisterAsThroNe() - ERROR: Invalid throneprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CCrowncoinAddress address2(address1);
    //LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchThroNeSignature, key2)) {
        LogPrintf("CActiveThrone::RegisterAsThroNe() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchThroNeSignature, strMessage, errorMessage)) {
        LogPrintf("CActiveThrone::RegisterAsThroNe() - Verify message failed");
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    return true;
}


bool CTransactionLock::SignaturesValid()
{

    BOOST_FOREACH(CConsensusVote vote, vecConsensusVotes)
    {
<<<<<<< HEAD
        int n = mnodeman.GetMasternodeRank(vote.vinMasternode, vote.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

        if(n == -1)
        {
            LogPrintf("CTransactionLock::SignaturesValid() - Unknown Masternode\n");
=======
        int n = mnodeman.GetThroneRank(vote.vinThrone, vote.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

        if(n == -1)
        {
            LogPrintf("InstantX::DoConsensusVote - Unknown Throne\n");
>>>>>>> origin/dirty-merge-dash-0.11.0
            return false;
        }

        if(n > INSTANTX_SIGNATURES_TOTAL)
        {
<<<<<<< HEAD
            LogPrintf("CTransactionLock::SignaturesValid() - Masternode not in the top %s\n", INSTANTX_SIGNATURES_TOTAL);
=======
            LogPrintf("InstantX::DoConsensusVote - Throne not in the top %s\n", INSTANTX_SIGNATURES_TOTAL);
>>>>>>> origin/dirty-merge-dash-0.11.0
            return false;
        }

        if(!vote.SignatureValid()){
<<<<<<< HEAD
            LogPrintf("CTransactionLock::SignaturesValid() - Signature not valid\n");
=======
            LogPrintf("InstantX::CTransactionLock::SignaturesValid - Signature not valid\n");
>>>>>>> origin/dirty-merge-dash-0.11.0
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
