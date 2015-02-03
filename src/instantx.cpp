


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
std::map<uint256, int64_t> mapUnknownVotes; //track votes with no tx for DOS
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

        if(mapTxLockReq.count(tx.GetHash()) || mapTxLockReqRejected.count(tx.GetHash())){
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
            RelayTransactionLockReq(tx, tx.GetHash());
            DoConsensusVote(tx, true, nBlockHeight);

            mapTxLockReq.insert(make_pair(tx.GetHash(), tx));

            LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request: %s %s : accepted %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str()
            );

            return;

        } else {
            mapTxLockReqRejected.insert(make_pair(tx.GetHash(), tx));

            // can we get the conflicting transaction as proof?

            RelayTransactionLockReq(tx, inv.hash);
            DoConsensusVote(tx, false, nBlockHeight);

            LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request: %s %s : rejected %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str()
            );

            // resolve conflicts
            /*std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(tx.GetHash());
            if (i != mapTxLocks.end()){
                if((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED){
                    LogPrintf("ProcessMessageInstantX::txlreq - Found IX lock\n");

                    uint256 txHash = (*i).second.txHash;
                    CValidationState state;
                    bool fMissingInputs = false;
                    DisconnectBlockAndInputs(state, mapTxLockReqRejected[txHash]);

                    if (AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs))
                    {
                        LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request : accepted (resolved) %s\n",
                            pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                            tx.GetHash().ToString().c_str()
                        );
                    } else {
                        LogPrintf("ERROR: InstantX::ProcessConsensusVote - Transaction Lock Request : rejected (failed to resolve) %s\n",
                            tx.GetHash().ToString().c_str()
                        );
                    }
                }
            }*/

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

        if(mapTxLockVote.count(ctx.GetHash())){
            return;
        }

        mapTxLockVote.insert(make_pair(ctx.GetHash(), 1));

        if(ProcessConsensusVote(ctx)){
            //Spam/Dos protection
            /*
                Masternodes will sometimes propagate votes before the transaction is known to the client.
                This tracks those messages and allows it at the same rate of the rest of the network, if
                a peer violates it, it will simply be ignored
            */
            if(!mapTxLockReq.count(ctx.txHash) && !mapTxLockReqRejected.count(ctx.txHash)){
                if(!mapUnknownVotes.count(ctx.vinMasternode.prevout.hash)){
                    //TODO: Make an algorithm to calculate the average time per IX/MNCOUNT
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }

                if(mapUnknownVotes[ctx.vinMasternode.prevout.hash] > GetTime() &&
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] - GetAverageVoteTime() > 60*10){
                        LogPrintf("ProcessMessageInstantX::txlreq - masternode is spamming transaction votes: %s %s : rejected %s\n",
                            ctx.vinMasternode.ToString().c_str(),
                            ctx.txHash.ToString().c_str()
                        );
                        return;
                } else {
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime()+(60*10);
                }
            }

            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                if(!pnode->fRelayTxes)
                    continue;

                pnode->PushMessage("txlvote", ctx);
            }
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

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        //LogPrintf("%s::check -- %s %s\n", vecMasternodes[winner].addr.ToString().c_str(), pnode->addr.ToString().c_str());

        pnode->PushMessage("txlvote", ctx);
    }
}

//received a consensus vote
bool ProcessConsensusVote(CConsensusVote& ctx)
{
    int n = GetMasternodeRank(ctx.vinMasternode, ctx.nBlockHeight, MIN_INSTANTX_PROTO_VERSION);

    if(n == -1)
    {
        LogPrintf("InstantX::ProcessConsensusVote - Unknown Masternode\n");
        return false;
    }

    if(n > 10)
    {
        LogPrintf("InstantX::ProcessConsensusVote - Masternode not in the top 10\n");
        return false;
    }

    if(!ctx.SignatureValid()) {
        LogPrintf("InstantX::ProcessConsensusVote - Signature invalid\n");
        //don't ban, it could just be a non-synced masternode
        return false;
    }

    if (!mapTxLocks.count(ctx.txHash)){
        LogPrintf("InstantX::ProcessConsensusVote - New Transaction Lock %s !\n", ctx.txHash.ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = ctx.nBlockHeight;
        newLock.nExpiration = GetTime()+(60*60);
        newLock.txHash = ctx.txHash;
        mapTxLocks.insert(make_pair(ctx.txHash, newLock));
    } else {
        LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Exists %s !\n", ctx.txHash.ToString().c_str());
    }

    //compile consessus vote
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(ctx.txHash);
    if (i != mapTxLocks.end()){
        (*i).second.AddSignature(ctx);
        if((*i).second.CountSignatures() >= INSTANTX_SIGNATURES_REQUIRED){
            LogPrintf("InstantX::ProcessConsensusVote - Transaction Lock Is Complete %s !\n", (*i).second.GetHash().ToString().c_str());

            if(pwalletMain->UpdatedTransaction((*i).second.txHash)){
                nCompleteTXLocks++;
            }

            // resolve conflicts
            /*
            //if this tx lock was rejected, we need to remove the conflicting blocks
            if(mapTxLockReqRejected.count((*i).second.txHash)){
                CValidationState state;
                bool fMissingInputs = false;
                DisconnectBlockAndInputs(state, mapTxLockReqRejected[(*i).second.txHash]);

                if (AcceptToMemoryPool(mempool, state, mapTxLockReqRejected[(*i).second.txHash], true, &fMissingInputs))
                {
                    LogPrintf("ProcessMessageInstantX::txlreq - Transaction Lock Request : accepted (resolved) %s\n",
                        mapTxLockReqRejected[(*i).second.txHash].GetHash().ToString().c_str()
                    );

                } else {
                    LogPrintf("ERROR: InstantX::ProcessConsensusVote - Transaction Lock Request : rejected (failed to resolve) %s\n",
                        mapTxLockReqRejected[(*i).second.txHash].GetHash().ToString().c_str()
                    );
                }
            }*/
        }
        return true;
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
            mapTxLocks.erase(it++);
        } else {
            it++;
        }
    }

}

uint256 CConsensusVote::GetHash() const
{
    return vinMasternode.prevout.hash + txHash;
}


bool CConsensusVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
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
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
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

void CTransactionLock::AddSignature(CConsensusVote cv)
{
    vecConsensusVotes.push_back(cv);
}

int CTransactionLock::CountSignatures()
{
    return vecConsensusVotes.size();
}
