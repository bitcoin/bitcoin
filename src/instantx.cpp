


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

std::map<uint256, CTransaction> mapTxLockReq;
std::map<uint256, CTransaction> mapTxLockReqRejected;
std::map<uint256, int> mapTxLockVote;
std::map<uint256, CTransactionLock> mapTxLocks;
int nCompleteTXLocks;

//txlock - Locks transaction
//
//step 1.) Broadcast intention to lock transaction inputs, "txlreg", CTransaction
//step 2.) Top 10 masternodes, open connect to top 1 masternode. Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 masternode, waits for 10 messages. Upon success, sends "txlock'

void ProcessMessageInstantX(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/masternode related functionality

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

        int nTxAge = 0;
        BOOST_REVERSE_FOREACH(CTxIn i, tx.vin){
            nTxAge = GetInputAge(i);
            if(nTxAge <= 5){
                LogPrintf("ProcessMessageInstantX::txlreq - Transaction not found / too new: %d / %s\n", nTxAge, tx.GetHash().ToString().c_str());
                return;
            }
        }
        int nBlockHeight = chainActive.Tip()->nHeight - nTxAge; //calculate the height

        BOOST_FOREACH(const CTxOut o, tx.vout){
            if(!o.scriptPubKey.IsNormalPaymentScript()){
                printf ("ProcessMessageInstantX::txlreq - Invalid Script %s\n", tx.ToString().c_str());
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

        CInv inv(MSG_TXLOCK_VOTE, ctx.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapTxLockVote.count(inv.hash)){
            return;
        }

        mapTxLockVote.insert(make_pair(inv.hash, 1));

        ProcessConsensusVote(ctx);

        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            pnode->PushMessage("txlvote", ctx);
        }
        return;
    }
}

// check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, bool approved, int64_t nBlockHeight)
{
    if(!fMasterNode) return;

    CConsensusVote ctx;
    ctx.vinMasternode = activeMasternode.vin;
    ctx.approved = approved;
    ctx.tx = tx;
    ctx.nBlockHeight = nBlockHeight;
    if(!ctx.Sign()){
        LogPrintf("InstantX::DoConsensusVote - Failed to sign consensus vote\n");
        return;
    }
    if(!ctx.SignatureValid()) {
        LogPrintf("InstantX::DoConsensusVote - Signature invalid\n");
        return;
    }

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        //LogPrintf("%s::check -- %s %s\n", vecMasternodes[winner].addr.ToString().c_str(), pnode->addr.ToString().c_str());

        pnode->PushMessage("txlvote", ctx);
    }
}

//received a consensus vote
void ProcessConsensusVote(CConsensusVote& ctx)
{
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

    if (!mapTxLocks.count(ctx.tx.GetHash())){
        LogPrintf("InstantX::ProcessConsensusVote - New Transaction Lock %s !\n", ctx.tx.GetHash().ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = ctx.nBlockHeight;
        newLock.nExpiration = GetTime()+(60*60);
        newLock.tx = ctx.tx;
        mapTxLocks.insert(make_pair(ctx.tx.GetHash(), newLock));
    } else {
        LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Exists %s !\n", ctx.tx.GetHash().ToString().c_str());
    }

    //compile consessus vote
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(ctx.tx.GetHash());
    if (i != mapTxLocks.end()){
        (*i).second.AddSignature(ctx);
        if((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED){
            LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Is Complete %s !\n", (*i).second.GetHash().ToString().c_str());
            pwalletMain->UpdatedTransaction((*i).second.tx.GetHash());
            nCompleteTXLocks++;
        }
        return;
    }


    return;
}

void CleanTransactionLocksList()
{
    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.begin();

    while(it != mapTxLocks.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old transaction lock %s\n", it->second.GetHash().ToString().c_str());
            mapTxLocks.erase(it++);
        } else {
            it++;
        }
    }

}

uint256 CConsensusVote::GetHash() const
{

    std::string strHash = tx.GetHash().ToString().c_str();
    strHash += vinMasternode.ToString().c_str();
    strHash += boost::lexical_cast<std::string>(nBlockHeight);
    strHash += boost::lexical_cast<std::string>(approved);

    printf("%s\n", strHash.c_str());

    return Hash(BEGIN(strHash), END(strHash));
}


bool CConsensusVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = tx.GetHash().ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(approved);
    //LogPrintf("verify strMessage %s \n", strMessage.c_str());

    int n = GetMasternodeByVin(vinMasternode);

    if(n == -1)
    {
        LogPrintf("InstantX::CConsensusVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    //LogPrintf("verify addr %s \n", vecMasternodes[0].addr.ToString().c_str());
    //LogPrintf("verify addr %s \n", vecMasternodes[1].addr.ToString().c_str());
    //LogPrintf("verify addr %d %s \n", n, vecMasternodes[n].addr.ToString().c_str());

    CScript pubkey;
    pubkey.SetDestination(vecMasternodes[n].pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcoinAddress address2(address1);
    //LogPrintf("verify pubkey2 %s \n", address2.ToString().c_str());

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
    std::string strMessage = tx.GetHash().ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(approved);
    //LogPrintf("signing strMessage %s \n", strMessage.c_str());
    //LogPrintf("signing privkey %s \n", strMasterNodePrivKey.c_str());

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcoinAddress address2(address1);
    //LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

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
