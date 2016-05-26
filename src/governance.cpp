// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

#include "flat-database.h"
#include "governance.h"
#include "governance-vote.h"
#include "masternode.h"
#include "governance.h"
#include "darksend.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

class CNode;

CGovernanceManager governance;
CCriticalSection cs_budget;

std::map<uint256, int64_t> mapAskedForGovernanceObject;
std::vector<CGovernanceObject> vecImmatureGovernanceObjects;

int nSubmittedFinalBudget;

bool IsCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf, CAmount minFee)
{
    CTransaction txCollateral;
    uint256 nBlockHash;
    if(!GetTransaction(nTxCollateralHash, txCollateral, Params().GetConsensus(), nBlockHash, true)){
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
        return false;
    }

    if(txCollateral.vout.size() < 1) {
        strError = strprintf("tx vout size less than 1 | %d", txCollateral.vout.size());
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
        return false;
    }

    CScript findScript;
    findScript << OP_RETURN << ToByteVector(nExpectedHash);

    bool foundOpReturn = false;
    BOOST_FOREACH(const CTxOut o, txCollateral.vout){
        if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()){
            strError = strprintf("Invalid Script %s", txCollateral.ToString());
            LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
            return false;
        }
        if(o.scriptPubKey == findScript && o.nValue >= minFee) foundOpReturn = true;

    }
    if(!foundOpReturn){
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral.ToString());
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
        return false;
    }

    LOCK(cs_main);
    int conf = GetIXConfirmations(nTxCollateralHash);
    if (nBlockHash != uint256()) {
        BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                conf += chainActive.Height() - pindex->nHeight + 1;
                nTime = pindex->nTime;
            }
        }
    }

    nConf = conf;

    //if we're syncing we won't have instantX information, so accept 1 confirmation 
    if(conf >= GOVERNANCE_FEE_CONFIRMATIONS){
        strError = "valid";
        return true;
    } else {
        strError = strprintf("Collateral requires at least %d confirmations - %d confirmations", GOVERNANCE_FEE_CONFIRMATIONS, conf);
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s - %d confirmations\n", strError, conf);
        return false;
    }
}

void CGovernanceManager::CheckOrphanVotes()
{
    LOCK(cs);

    std::string strError = "";
    std::map<uint256, CGovernanceVote>::iterator it1 = mapOrphanVotes.begin();
    while(it1 != mapOrphanVotes.end()){
        if(UpdateGovernanceObject(((*it1).second), NULL, strError)){
            LogPrintf("CGovernanceManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanVotes.erase(it1++);
        } else {
            ++it1;
        }
    }
}

bool CGovernanceManager::AddGovernanceObject(CGovernanceObject& budgetProposal)
{
    LOCK(cs);
    std::string strError = "";
    if(!budgetProposal.IsValid(pCurrentBlockIndex, strError)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject - invalid governance object - %s\n", strError);
        return false;
    }

    if(mapObjects.count(budgetProposal.GetHash())) {
        LogPrintf("CGovernanceManager::AddGovernanceObject - already have governance object - %s\n", strError);
        return false;
    }

    mapObjects.insert(make_pair(budgetProposal.GetHash(), budgetProposal));
    return true;
}

void CGovernanceManager::CheckAndRemove()
{
    LogPrintf("CGovernanceManager::CheckAndRemove \n");

    // DELETE OBJECTS WHICH MASTERNODE HAS FLAGGED DELETE=TRUE

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end())
    {
        CGovernanceObject* pObj = &((*it).second);

        pObj->UpdateLocalValidity(pCurrentBlockIndex);
        pObj->UpdateSentinelVariables(pCurrentBlockIndex);
        ++it;
    }

    // UPDATE CACHING MECHANISMS FOR GOVERNANCE OBJECTS

    if(!pCurrentBlockIndex) return;

    std::string strError = "";

    std::map<uint256, CGovernanceObject>::iterator it2 = mapObjects.begin();
    while(it2 != mapObjects.end())
    {
        CGovernanceObject* pObj = &((*it2).second);

        // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
        pObj->UpdateLocalValidity(pCurrentBlockIndex);

        // UPDATE SENTINEL SIGNALING VARIABLES
        pObj->UpdateSentinelVariables(pCurrentBlockIndex);
        ++it2;
    }
}

CGovernanceObject *CGovernanceManager::FindGovernanceObject(const std::string &strName)
{
    //find the prop with the highest yes count

    int nYesCount = -99999;
    CGovernanceObject* pbudgetProposal = NULL;

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end()){
        if((*it).second.strName == strName && (*it).second.GetYesCount(VOTE_SIGNAL_FUNDING) > nYesCount){
            pbudgetProposal = &((*it).second);
            nYesCount = pbudgetProposal->GetYesCount(VOTE_SIGNAL_FUNDING);
        }
        ++it;
    }

    if(nYesCount == -99999) return NULL;

    return pbudgetProposal;
}

CGovernanceObject *CGovernanceManager::FindGovernanceObject(uint256 nHash)
{
    LOCK(cs);

    if(mapObjects.count(nHash))
        return &mapObjects[nHash];

    return NULL;
}

std::vector<CGovernanceObject*> CGovernanceManager::GetAllProposals(int64_t nMoreThanTime)
{
    LOCK(cs);

    std::vector<CGovernanceObject*> vBudgetProposalRet;

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end())
    {
        // if((*it).second.nTime < nMoreThanTime) {
        //     ++it;
        //     continue;
        // }
        // (*it).second.CleanAndRemove(false);

        CGovernanceObject* pbudgetProposal = &((*it).second);
        vBudgetProposalRet.push_back(pbudgetProposal);

        ++it;
    }

    return vBudgetProposalRet;
}

//
// Sort by votes, if there's a tie sort by their feeHash TX
//
struct sortProposalsByVotes {
    bool operator()(const std::pair<CGovernanceObject*, int> &left, const std::pair<CGovernanceObject*, int> &right) {
      if( left.second != right.second)
        return (left.second > right.second);
      return (UintToArith256(left.first->nFeeTXHash) > UintToArith256(right.first->nFeeTXHash));
    }
};

void CGovernanceManager::NewBlock()
{
    TRY_LOCK(cs, fBudgetNewBlock);
    if(!fBudgetNewBlock) return;

    if(!pCurrentBlockIndex) return;

    // todo - 12.1 - add govobj sync
    if (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_BUDGET) return;

    //this function should be called 1/6 blocks, allowing up to 100 votes per day on all proposals
    if(pCurrentBlockIndex->nHeight % 6 != 0) return;

    // description: incremental sync with our peers
    // note: incremental syncing seems excessive, well just have clients ask for specific objects and their votes
    // note: 12.1 - remove
    // if(masternodeSync.IsSynced()){
    //     /*
    //         Needs comment below:

    //         - Why are we doing a partial sync with our peers on new blocks? 
    //         - Why only partial? Why not partial when we recieve the request directly?
    //         - Can we simplify this somehow?
    //         - Why are we marking everything synced? Was this to correct a bug? 
    //     */

    //     LogPrintf("CGovernanceManager::NewBlock - incremental sync started\n");
    //     if(pCurrentBlockIndex->nHeight % 600 == rand() % 600) {
    //         ClearSeen();
    //         ResetSync();
    //     }

    //     LOCK(cs_vNodes);
    //     BOOST_FOREACH(CNode* pnode, vNodes)
    //         if(pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) 
    //             Sync(pnode, uint256());

    //     MarkSynced();
    // }

    CheckAndRemove();

    //remove invalid votes once in a while (we have to check the signatures and validity of every vote, somewhat CPU intensive)

    std::map<uint256, int64_t>::iterator it = mapAskedForGovernanceObject.begin();
    while(it != mapAskedForGovernanceObject.end()){
        if((*it).second > GetTime() - (60*60*24)){
            ++it;
        } else {
            mapAskedForGovernanceObject.erase(it++);
        }
    }

    std::map<uint256, CGovernanceObject>::iterator it2 = mapObjects.begin();
    while(it2 != mapObjects.end()){
        (*it2).second.CleanAndRemove(false);
        ++it2;
    }

    std::vector<CGovernanceObject>::iterator it4 = vecImmatureGovernanceObjects.begin();
    while(it4 != vecImmatureGovernanceObjects.end())
    {
        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid((*it4).nFeeTXHash, (*it4).GetHash(), strError, (*it4).nTime, nConf, GOVERNANCE_FEE_TX)){
            ++it4;
            continue;
        }

        // 12.1 -- fix below
        // if(!(*it4).IsValid(pCurrentBlockIndex, strError)) {
        //     LogPrintf("mprop (immature) - invalid budget proposal - %s\n", strError);
        //     it4 = vecImmatureGovernanceObjects.erase(it4); 
        //     continue;
        // }

        // CGovernanceObject budgetProposal((*it4));
        // if(AddGovernanceObject(budgetProposal)) {(*it4).Relay();}

        // LogPrintf("mprop (immature) - new budget - %s\n", (*it4).GetHash().ToString());
        // it4 = vecImmatureGovernanceObjects.erase(it4); 
    }

}

void CGovernanceManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_budget);

    if (strCommand == NetMsgType::MNGOVERNANCEVOTESYNC) { //Masternode vote sync
        uint256 nProp;
        vRecv >> nProp;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(nProp == uint256()) {
                if(pfrom->HasFulfilledRequest(NetMsgType::MNGOVERNANCEVOTESYNC)) {
                    LogPrintf("mnvs - peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest(NetMsgType::MNGOVERNANCEVOTESYNC);
            }
        }

        // ask for a specific proposal and it's votes
        Sync(pfrom, nProp);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString());
    }

    if (strCommand == NetMsgType::MNGOVERNANCEPROPOSAL) { //Masternode Proposal
        CGovernanceObject budgetProposalBroadcast;
        vRecv >> budgetProposalBroadcast;

        if(mapSeenGovernanceObjects.count(budgetProposalBroadcast.GetHash())){
            // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
            masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid(budgetProposalBroadcast.nFeeTXHash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf, GOVERNANCE_FEE_TX)){
            LogPrintf("Proposal FeeTX is not valid - %s - %s\n", budgetProposalBroadcast.nFeeTXHash.ToString(), strError);
            //todo 12.1
            //if(nConf >= 1) vecImmatureGovernanceObjects.push_back(budgetProposalBroadcast);
            return;
        }

        if(!budgetProposalBroadcast.IsValid(pCurrentBlockIndex, strError)) {
            mapSeenGovernanceObjects.insert(make_pair(budgetProposalBroadcast.GetHash(), SEEN_OBJECT_ERROR_INVALID));
            LogPrintf("mprop - invalid budget proposal - %s\n", strError);
            return;
        }

        if(AddGovernanceObject(budgetProposalBroadcast))
        {
            budgetProposalBroadcast.Relay();
        }
        mapSeenGovernanceObjects.insert(make_pair(budgetProposalBroadcast.GetHash(), SEEN_OBJECT_IS_VALID));
        masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());

        LogPrintf("mprop - new budget - %s\n", budgetProposalBroadcast.GetHash().ToString());
        //We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    }

    if (strCommand == NetMsgType::MNGOVERNANCEVOTE) { //Masternode Vote
        CGovernanceVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vinMasternode);
        if(pmn == NULL) {
            LogPrint("mngovernance", "mvote - unknown masternode - vin: %s\n", vote.vinMasternode.ToString());
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
            return;
        }

        mapSeenVotes.insert(make_pair(vote.GetHash(), SEEN_OBJECT_IS_VALID));
        if(!vote.IsValid(true)){
            LogPrintf("mvote - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
            return;
        }

        std::string strError = "";
        if(UpdateGovernanceObject(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());
        }

        LogPrintf("mvote - new budget vote - %s\n", vote.GetHash().ToString());
    }

}

// description: incremental sync with our peers
// note: incremental syncing seems excessive, well just have clients ask for specific objects and their votes
// note: 12.1 - remove
// void CGovernanceManager::ResetSync()
// {
//     LOCK(cs);

//     std::map<uint256, std::map<uint256, CGovernanceVote> >::iterator it1 = mapVotes.begin();
//     while(it1 != mapVotes.end()){
//         (*it1).second.second.fSynced = false;
//         ++it1;
//     }
// }

// description: incremental sync with our peers
// note: incremental syncing seems excessive, well just have clients ask for specific objects and their votes
// note: 12.1 - remove
// void CGovernanceManager::MarkSynced()
// {
//     LOCK(cs);

//     /*
//         Mark that we've sent all valid items
//     */

//     // this could screw up syncing, so let's log it
//     LogPrintf("CGovernanceManager::MarkSynced\n");

//     std::map<uint256, std::map<uint256, CGovernanceVote> >::iterator it1 = mapVotes.begin();
//     while(it1 != mapVotes.end()){
//         if((*it1).second.second.fValid)
//             (*it1).second.second.fSynced = true;
//         ++it1;
//     }
// }


void CGovernanceManager::Sync(CNode* pfrom, uint256 nProp)
{
    LOCK(cs);

    /*
        note 12.1 - fix comments below

        Sync with a client on the network

        --

        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.

    */

    int nInvCount = 0;

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    std::map<uint256, int>::iterator it1 = mapSeenGovernanceObjects.begin();
    while(it1 != mapSeenGovernanceObjects.end()){
        CGovernanceObject* pbudgetProposal = FindGovernanceObject((*it1).first);
        if(pbudgetProposal && pbudgetProposal->fCachedValid && ((nProp == uint256() || ((*it1).first == nProp)))){
            // Push the inventory budget proposal message over to the other client
            pfrom->PushInventory(CInv(MSG_BUDGET_PROPOSAL, (*it1).first));
            nInvCount++;
        }
        ++it1;
    }

    // SYNC OUR GOVERNANCE OBJECT VOTES WITH THEIR GOVERNANCE OBJECT VOTES

    std::map<uint256, CGovernanceVote>::iterator it2 = mapVotes.begin();
    while(it2 != mapVotes.end()){
        pfrom->PushInventory(CInv(MSG_BUDGET_VOTE, (*it2).first));
        nInvCount++;
        ++it2;
    }

    pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_BUDGET_PROP, nInvCount);

    LogPrintf("CGovernanceManager::Sync - sent %d items\n", nInvCount);
}

bool CGovernanceManager::UpdateGovernanceObject(CGovernanceVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    if(!mapObjects.count(vote.nParentHash)){
        if(pfrom){
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CGovernanceManager::UpdateGovernanceObject - Unknown proposal %d, asking for source proposal\n", vote.nParentHash.ToString());
            mapOrphanVotes[vote.nParentHash] = vote;

            if(!mapAskedForGovernanceObject.count(vote.nParentHash)){
                pfrom->PushMessage(NetMsgType::MNGOVERNANCEVOTESYNC, vote.nParentHash);
                mapAskedForGovernanceObject[vote.nParentHash] = GetTime();
            }
        }

        strError = "Proposal not found!";
        return false;
    }


    return AddOrUpdateVote(vote, strError);
}

bool CGovernanceManager::AddOrUpdateVote(CGovernanceVote& vote, std::string& strError)
{
    LOCK(cs);

    // store newest vote per action
    arith_uint256 a = UintToArith256(vote.nParentHash) + vote.nVoteSignal;
    uint256 hash2 = ArithToUint256(a);

    if(mapVotes.count(hash2))
    {
        if(mapVotes[hash2].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s", vote.GetHash().ToString());
            LogPrint("mngovernance", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotes[hash2].nTime < GOVERNANCE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli", vote.GetHash().ToString(), vote.nTime - mapVotes[hash2].nTime);
            LogPrint("mngovernance", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    mapVotes[hash2] = vote;
    return true;
}

CGovernanceObject::CGovernanceObject()
{
    strName = "unknown";
    nTime = 0;

    nHashParent = uint256(); //parent object, 0 is root
    nRevision = 0; //object revision in the system
    nFeeTXHash = uint256(); //fee-tx
    
    // caching
    fCachedFunding = false;
    fCachedValid = true;
    fCachedDelete = false;
    fCachedEndorsed = false;

}

CGovernanceObject::CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, std::string strNameIn, int64_t nTimeIn, uint256 nFeeTXHashIn)
{
    nHashParent = nHashParentIn; //parent object, 0 is root
    nRevision = nRevisionIn; //object revision in the system
    strName = strNameIn;
    nTime = nTimeIn;
    nFeeTXHash = nFeeTXHashIn; //fee-tx    
}

CGovernanceObject::CGovernanceObject(const CGovernanceObject& other)
{
    // COPY OTHER OBJECT'S DATA INTO THIS OBJECT

    nHashParent = other.nHashParent;
    nRevision = other.nRevision;
    strName = other.strName;
    nTime = other.nTime;
    nFeeTXHash = other.nFeeTXHash;
    strData = other.strData;
}

bool CGovernanceObject::IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{
    if(GetNoCount(VOTE_SIGNAL_VALID) - GetYesCount(VOTE_SIGNAL_VALID) > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10){
         strError = "Automated removal";
         return false;
    }

    // if(nEndTime < GetTime()) {
    //     strError = "Expired Proposal";
    //     return false;
    // }

    if(!pindex) {
        strError = "Tip is NULL";
        return true;
    }

    // if(nAmount < 10000) {
    //     strError = "Invalid proposal amount (minimum 10000)";
    //     return false;
    // }

    if(strName.size() > 20) {
        strError = "Invalid proposal name, limit of 20 characters.";
        return false;
    }

    if(strName != SanitizeString(strName)) {
        strError = "Invalid proposal name, unsafe characters found.";
        return false;
    }

    if(fCheckCollateral){
        int nConf = 0;
        if(!IsCollateralValid(nFeeTXHash, GetHash(), strError, nTime, nConf, GOVERNANCE_FEE_TX)){
            // strError set in IsCollateralValid
            return false;
        }
    }

    /*
        TODO: There might be an issue with multisig in the coinbase on mainnet, we will add support for it in a future release.
    */
    // if(address.IsPayToScriptHash()) {
    //     strError = "Multisig is not currently supported.";
    //     return false;
    // }

    // 12.1 move to pybrain
    // //can only pay out 10% of the possible coins (min value of coins)
    // if(nAmount > budget.GetTotalBudget(nBlockStart)) {
    //     strError = "Payment more than max";
    //     return false;
    // }

    return true;
}

void CGovernanceObject::CleanAndRemove(bool fSignatureCheck) {
    // TODO: do smth here
}

void CGovernanceManager::CleanAndRemove(bool fSignatureCheck)
{
    /*
    *   
    *   Loop through each item and delete the items that have become invalid
    *
    */

    // std::map<uint256, CGovernanceVote>::iterator it2 = mapVotes.begin();
    // while(it2 != mapVotes.end()){
    //     if(!(*it2).second.IsValid(fSignatureCheck))
    //     {
    //         // 12.1 - log to specialized handle (govobj?)
    //         LogPrintf("CGovernanceManager::CleanAndRemove - Proposal/Budget is known, activating and removing orphan vote\n");
    //         mapVotes.erase(it2++);
    //     } else {
    //         ++it2;
    //     }
    // }
}

/**
*   Get specific vote counts for each outcome (funding, validity, etc)
*/

int CGovernanceObject::GetAbsoluteYesCount(int nVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), nVoteSignalIn, VOTE_OUTCOME_YES) - governance.CountMatchingVotes((*this), nVoteSignalIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetYesCount(int nVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), nVoteSignalIn, VOTE_OUTCOME_YES);
}

int CGovernanceObject::GetNoCount(int nVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), nVoteSignalIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetAbstainCount(int nVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), nVoteSignalIn, VOTE_OUTCOME_ABSTAIN);
}

void CGovernanceObject::Relay()
{
    CInv inv(MSG_BUDGET_PROPOSAL, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

std::string CGovernanceManager::ToString() const
{
    std::ostringstream info;

    info << "Governance Objects: " << (int)mapObjects.size() <<
            ", Seen Budgets: " << (int)mapSeenGovernanceObjects.size() <<
            ", Seen Budget Votes: " << (int)mapSeenVotes.size() <<
            ", Vote Count: " << (int)mapVotes.size();

    return info.str();
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("mngovernance", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        NewBlock();
}

int CGovernanceManager::CountMatchingVotes(CGovernanceObject& govobj, int nVoteSignalIn, int nVoteOutcomeIn)
{
    /*
    *   
    *   Count matching votes and return
    *
    */

    int nCount = 0;

    std::map<uint256, CGovernanceVote>::iterator it2 = mapVotes.begin();
    while(it2 != mapVotes.end()){
        if((*it2).second.fValid && (*it2).second.nVoteSignal == nVoteSignalIn && (*it2).second.GetParentHash() == govobj.GetHash())
        {
            nCount += ((*it2).second.nVoteOutcome == nVoteOutcomeIn ? 1 : 0);
            ++it2;
        } else {
            ++it2;
        }
    }

    return nCount;
}