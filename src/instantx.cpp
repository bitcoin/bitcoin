


#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "protocol.h"
#include "instantx.h"
#include "masternode.h"
#include "activemasternode.h"
#include "darksend.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

std::vector<CTransactionLock> vecTxLocks;

std::map<uint256, CTransaction> mapTxLockReq;
std::map<uint256, CTransaction> mapTxLockReqRejected;
std::map<uint256, CTransactionLock> mapTxLocks;

#define INSTANTX_SIGNATURES_REQUIRED           2

//txlock - Locks transaction
//
//step 1.) Broadcast intention to lock transaction inputs, "txlreg", CTransaction
//step 2.) Top 10 masternodes, open connect to top 1 masternode. Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 masternode, waits for 10 messages. Upon success, sends "txlock'

void ProcessMessageInstantX(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    return;

    if (strCommand == "txlreq")
    {
        //LogPrintf("ProcessMessageInstantX::txlreq\n");
        CDataStream vMsg(vRecv);
        CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TXLOCK_REQUEST, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapTxLockReq.count(inv.hash) || mapTxLockReqRejected.count(inv.hash)){
            return;
        }

        int nTxAge = GetInputAge(tx.vin[0]);
        if(nTxAge < 5){
            LogPrintf("ProcessMessageInstantX::txlreq - Transaction not found / too new: %s\n", tx.GetHash().ToString().c_str());
            return;
        }
        int nBlockHeight = chainActive.Tip()->nHeight - nTxAge; //calculate the height

        BOOST_FOREACH(const CTxOut o, tx.vout){
            if(!o.scriptPubKey.IsNormalPaymentScript()){
                LogPrintf ("ProcessMessageInstantX::txlreq - Invalid Script %s\n", tx.ToString().c_str());
                return;
            }
        }

        bool fMissingInputs = false;
        CValidationState state;

        if (AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs))
        {
            RelayTransactionLockReq(tx, inv.hash);
            DoConsensusVote(tx, true, nBlockHeight);

            mapTxLockReq.insert(make_pair(inv.hash, tx));

            LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request: %s %s : accepted %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str()
            );

            return;

        } else {
            mapTxLockReqRejected.insert(make_pair(inv.hash, tx));

            // can we get the conflicting transaction as proof?

            RelayTransactionLockReq(tx, inv.hash);
            DoConsensusVote(tx, false, nBlockHeight);

            LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request: %s %s : rejected %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str()
            );

            //record prevout, increment the amount of times seen. Ban if over 100

            return;
        }
    }
    else if (strCommand == "txlvote") //InstantX Lock Consensus Votes
    {
        CConsensusVote ctx;
        vRecv >> ctx;

        ProcessConsensusVote(ctx);

        return;
    }
    else if (strCommand == "txlock") //InstantX Lock Transaction Inputs
    {
        LogPrintf("ProcessMessageInstantX::txlock\n");

        CDataStream vMsg(vRecv);
        CTransactionLock ctxl;
        vRecv >> ctxl;

        CInv inv(MSG_TXLOCK, ctxl.GetHash());
        pfrom->AddInventoryKnown(inv);

        LogPrintf(" -- ProcessMessageInstantX::txlock %d  %s\n", mapTxLocks.count(inv.hash), inv.hash.ToString().c_str());


        if(!mapTxLocks.count(inv.hash)){
            if(ctxl.CountSignatures() < INSTANTX_SIGNATURES_REQUIRED){
                LogPrintf("InstantX::txlock - not enough signatures\n");
                return;
            }
            if(!ctxl.SignaturesValid()){
                LogPrintf("InstantX::txlock - got invalid TransactionLock, rejected\n");
                return;
            }
            if(!ctxl.AllInFavor()){
                LogPrintf("InstantX::txlock - not all in favor of lock, rejected\n");
                return;
            }

            BOOST_FOREACH(const CTxOut o, ctxl.tx.vout){
                if(!o.scriptPubKey.IsNormalPaymentScript()){
                    LogPrintf ("ProcessMessageInstantX::cxlock - Invalid Script %s\n", ctxl.tx.ToString().c_str());
                    return;
                }
            }
            mapTxLocks.insert(make_pair(inv.hash, ctxl));

            //we should have the lock request in place
            if(!mapTxLockReq.count(inv.hash)){
                //if we don't
                bool fMissingInputs = false;
                CValidationState state;

                if (AcceptToMemoryPool(mempool, state, ctxl.tx, true, &fMissingInputs))
                {
                    mapTxLockReq.insert(make_pair(inv.hash, ctxl.tx));

                    LogPrintf("ProcessMessageInstantX::txlock - Transaction Lock Request: %s %s : accepted (no reversing) %s\n",
                        pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                        ctxl.tx.GetHash().ToString().c_str()
                    );

                } else {
                    // we have a conflicting transaction (an attack)
                    CValidationState state;
                    DisconnectBlockAndInputs(state, ctxl.tx);

                    if (AcceptToMemoryPool(mempool, state, ctxl.tx, true, &fMissingInputs))
                    {
                        mapTxLockReq.insert(make_pair(inv.hash, ctxl.tx));

                        LogPrintf("ProcessMessageInstantX::txlock - Transaction Lock Request: %s %s : accepted (reversed) %s\n",
                            pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                            ctxl.tx.GetHash().ToString().c_str()
                        );
                    } else {
                        LogPrintf("ProcessMessageInstantX::txlock - Transaction Lock Request: %s %s : rejected (reversed) %s\n",
                            pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                            ctxl.tx.GetHash().ToString().c_str()
                        );
                    }
                }
            }

            //broadcast the new lock
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                if(!pnode->fRelayTxes)
                    continue;

                pnode->PushMessage("txlock", ctxl);
            }

            pwalletMain->UpdatedTransaction(ctxl.GetHash());

            LogPrintf("InstantX :: Got Transaction Lock: %s %s : accepted %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                ctxl.GetHash().ToString().c_str()
            );
        }
    }
}

// check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, bool approved, int64_t nBlockHeight)
{
    if(!fMasterNode) {
        LogPrintf("InstantX::DoConsensusVote - Not masternode\n");
        return;
    }

    int winner = GetCurrentMasterNode(1, nBlockHeight);
    int n = GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    if(n == -1 || winner == -1)
    {
        LogPrintf("InstantX::DoConsensusVote - Unknown Masternode\n");
        return;
    }

    if(n == 1)
    { // winner, I'll be keeping track of this
        LogPrintf("InstantX::DoConsensusVote - Managing Masternode\n");
        CTransactionLock newLock;
        newLock.nBlockHeight = nBlockHeight;
        newLock.tx = tx;
        vecTxLocks.push_back(newLock);
    }

    CConsensusVote ctx;
    ctx.vinMasternode = activeMasternode.vin;
    ctx.approved = approved;
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


    if(n == 1){ //I'm the winner
        ProcessConsensusVote(ctx);
    } else if(n <= 10){ // not winner, but in the top10
        if(ConnectNode((CAddress)vecMasternodes[winner].addr, NULL, true)){
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                if(vecMasternodes[winner].addr != pnode->addr) continue;

                pnode->PushMessage("txlvote", ctx);
                LogPrintf("InstantX::DoConsensusVote --- connected, sending vote %s\n", pnode->addr.ToString().c_str());
                return;
            }
        } else {
            LogPrintf("InstantX::DoConsensusVote --- error connecting \n");
            return;
        }
    }
}

//received a consensus vote
void ProcessConsensusVote(CConsensusVote& ctx)
{
    if(!fMasterNode) {
        LogPrintf("InstantX::ProcessConsensusVote - Not masternode\n");
        return;
    }

    int winner = GetCurrentMasterNode(1, ctx.nBlockHeight);
    if(winner == -1) {
        LogPrintf("InstantX::ProcessConsensusVote - Can't detect winning masternode\n");
        return;
    }

    //We're not the winning masternode
    if(vecMasternodes[winner].vin != activeMasternode.vin) {
        LogPrintf("InstantX::ProcessConsensusVote - I'm not the winning masternode\n");
        return;
    }

    int n = GetMasternodeRank(ctx.vinMasternode, ctx.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    if(n == -1)
    {
        LogPrintf("InstantX::ProcessConsensusVote - Unknown Masternode\n");
        return;
    }

    if(n > 10)
    {
        LogPrintf("InstantX::ProcessConsensusVote - Masternode not in the top 10\n");
        return;
    }

    if(!ctx.SignatureValid()) {
        LogPrintf("InstantX::ProcessConsensusVote - Signature invalid\n");
        //don't ban, it could just be a non-synced masternode
        return;
    }

    //compile consessus vote
    BOOST_FOREACH(CTransactionLock& ctxl, vecTxLocks){
        if(ctxl.nBlockHeight == ctx.nBlockHeight){
            ctxl.AddSignature(ctx);
            if(ctxl.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED){
                LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Is Complete, broadcasting!\n");

                CInv inv(MSG_TXLOCK, ctxl.GetHash());
                mapTxLocks.insert(make_pair(inv.hash, ctxl));

                //broadcast the new lock
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes){
                    pnode->PushMessage("txlock", ctxl);
                }
            }
            return;
        }
    }

    return;
}

void CleanTransactionLocksList()
{
    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.begin();

    while(it != mapTxLocks.end()) {
        if(chainActive.Tip()->nHeight - it->second.nBlockHeight > 3){ //keep them for an hour
            LogPrintf("Removing old transaction lock %s\n", it->second.GetHash().ToString().c_str());
            mapTxLocks.erase(it++);
        } else {
            it++;
        }
    }

}

bool CConsensusVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(approved);
    LogPrintf("verify strMessage %s \n", strMessage.c_str());

    int n = GetMasternodeByVin(vinMasternode);

    if(n == -1)
    {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    LogPrintf("verify addr %s \n", vecMasternodes[0].addr.ToString().c_str());
    LogPrintf("verify addr %s \n", vecMasternodes[1].addr.ToString().c_str());
    LogPrintf("verify addr %d %s \n", n, vecMasternodes[n].addr.ToString().c_str());

    CScript pubkey;
    pubkey.SetDestination(vecMasternodes[n].pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcoinAddress address2(address1);
    LogPrintf("verify pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.VerifyMessage(vecMasternodes[n].pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
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
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(approved);
    LogPrintf("signing strMessage %s \n", strMessage.c_str());
    LogPrintf("signing privkey %s \n", strMasterNodePrivKey.c_str());

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        exit(0);
    }

    CScript pubkey;
    pubkey.SetDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcoinAddress address2(address1);
    LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, key2)) {
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Verify message failed");
        return false;
    }

    return true;
}


bool CTransactionLock::SignaturesValid()
{

    BOOST_FOREACH(CConsensusVote vote, vecConsensusVotes)
    {
        int n = GetMasternodeRank(vote.vinMasternode, vote.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

        if(n == -1)
        {
            LogPrintf("InstantX::DoConsensusVote - Unknown Masternode\n");
            return false;
        }

        if(n > 10)
        {
            LogPrintf("InstantX::DoConsensusVote - Masternode not in the top 10\n");
            return false;
        }

        if(!vote.SignatureValid()){
            LogPrintf("InstantX::CTransactionLock::SignaturesValid - Signature not valid\n");
            return false;
        }
    }

    return true;
}

bool CTransactionLock::AllInFavor()
{
    BOOST_FOREACH(CConsensusVote vote, vecConsensusVotes)
        if(vote.approved == false) return false;

    return true;
}

void CTransactionLock::AddSignature(CConsensusVote cv)
{
    vecConsensusVotes.push_back(cv);
}

int CTransactionLock::CountSignatures()
{
    return vecConsensusVotes.size();
}
