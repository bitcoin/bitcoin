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

bool IsCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int& nConf, CAmount minFee)
{

    CTransaction txCollateral;
    uint256 nBlockHash;

    // RETRIEVE TRANSACTION IN QUESTION  

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

    // LOOK FOR SPECIALIZED GOVERNANCE SCRIPT (PROOF OF BURN)

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

    // GET CONFIRMATIONS FOR TRANSACTION

    LOCK(cs_main);
    int nConfirmationsIn = GetIXConfirmations(nTxCollateralHash);
    if (nBlockHash != uint256()) {
        BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                nConfirmationsIn += chainActive.Height() - pindex->nHeight + 1;
            }
        }
    }

    nConf = nConfirmationsIn;

    //if we're syncing we won't have instantX information, so accept 1 confirmation 
    if(nConfirmationsIn >= GOVERNANCE_FEE_CONFIRMATIONS){
        strError = "valid";
    } else {
        strError = strprintf("Collateral requires at least %d confirmations - %d confirmations", GOVERNANCE_FEE_CONFIRMATIONS, nConfirmationsIn);
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s - %d confirmations\n", strError, nConfirmationsIn);
        return false;
    }
    
    return true;
}

void CGovernanceManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    //
    // REMOVE AFTER MIGRATION TO 12.1
    //
    if(pfrom->nVersion < 70201) return;
    //
    // END REMOVE
    //

    LOCK(cs_budget);

    // GOVERANCE SYNCING FUNCTIONALITY
    if (strCommand == NetMsgType::MNGOVERNANCESYNC)
    {

        // ignore such request until we are fully synced
        if (!masternodeSync.IsSynced()) return;

        uint256 nProp;
        vRecv >> nProp;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(nProp == uint256()) {
                if(pfrom->HasFulfilledRequest(NetMsgType::MNGOVERNANCESYNC)) {
                    LogPrint("mngovernance", "peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest(NetMsgType::MNGOVERNANCESYNC);
            }
        }

        // ask for a specific proposal and it's votes
        Sync(pfrom, nProp);
        LogPrint("mngovernance", "syncing governance objects to our peer at %s\n", pfrom->addr.ToString());
    
    }

    // NEW GOVERNANCE OBJECT
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECT)

    {
        // MAKE SURE WE HAVE A VALID REFERENCE TO THE TIP BEFORE CONTINUING

        if(!pCurrentBlockIndex) return;

        CGovernanceObject govobj;
        vRecv >> govobj;

        if(mapSeenGovernanceObjects.count(govobj.GetHash())){
            // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
            masternodeSync.AddedBudgetItem(govobj.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid(govobj.nFeeTXHash, govobj.GetHash(), strError, nConf, GOVERNANCE_FEE_TX)){
            LogPrintf("Governance object collateral tx is not valid - %s - %s\n", govobj.nFeeTXHash.ToString(), strError);
            //todo 12.1
            //if(nConf >= 1) vecImmatureGovernanceObjects.push_back(govobj);
            return;
        }

        if(!govobj.IsValid(pCurrentBlockIndex, strError)) {
            mapSeenGovernanceObjects.insert(make_pair(govobj.GetHash(), SEEN_OBJECT_ERROR_INVALID));
            LogPrintf("Governance object is invalid - %s\n", strError);
            return;
        }
        
        // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANANGED DATA

        govobj.UpdateSentinelVariables(pCurrentBlockIndex);
        if(AddGovernanceObject(govobj)) govobj.Relay();

        mapSeenGovernanceObjects.insert(make_pair(govobj.GetHash(), SEEN_OBJECT_IS_VALID));
        masternodeSync.AddedBudgetItem(govobj.GetHash());

        LogPrintf("MNGOVERNANCEOBJECT -- %s new\n", govobj.GetHash().ToString());
        //We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    
    }

    // NEW GOVERNANCE OBJECT VOTE
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECTVOTE)
    {

        CGovernanceVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vinMasternode);
        if(pmn == NULL) {
            LogPrint("mngovernance", "mngovernance - unknown masternode - vin: %s\n", vote.vinMasternode.ToString());
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
            return;
        }

        if(!vote.IsValid(true)){
            LogPrintf("mngovernance - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
            mapSeenVotes.insert(make_pair(vote.GetHash(), SEEN_OBJECT_ERROR_INVALID));
            return;
        } else {
            mapSeenVotes.insert(make_pair(vote.GetHash(), SEEN_OBJECT_IS_VALID));
        }

        std::string strError = "";
        if(UpdateGovernanceObject(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());
        }

        LogPrint("mngovernance", "new vote - %s\n", vote.GetHash().ToString());
    }

}


void CGovernanceManager::CheckOrphanVotes()
{
    LOCK(cs);

    std::string strError = "";
    std::map<uint256, CGovernanceVote>::iterator it1 = mapOrphanVotes.begin();
    while(it1 != mapOrphanVotes.end()){
        if(UpdateGovernanceObject(((*it1).second), NULL, strError)){
            LogPrintf("CGovernanceManager::CheckOrphanVotes - Governance object is known, activating and removing orphan vote\n");
            mapOrphanVotes.erase(it1++);
        } else {
            ++it1;
        }
    }
}

bool CGovernanceManager::AddGovernanceObject(CGovernanceObject& govobj)
{
    LOCK(cs);
    std::string strError = "";
    if(!govobj.IsValid(pCurrentBlockIndex, strError)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject - invalid governance object - %s - (pCurrentBlockIndex nHeight %d) \n", strError, pCurrentBlockIndex->nHeight);
        return false;
    }

    if(mapObjects.count(govobj.GetHash())) {
        LogPrintf("CGovernanceManager::AddGovernanceObject - already have governance object - %s\n", strError);
        return false;
    }

    mapObjects.insert(make_pair(govobj.GetHash(), govobj));
    return true;
}

void CGovernanceManager::CheckAndRemove()
{
    LogPrintf("CGovernanceManager::CheckAndRemove \n");

    // DOUBLE CHECK THAT WE HAVE A VALID POINTER TO TIP

    if(!pCurrentBlockIndex) return;

    // DELETE OBJECTS WHICH MASTERNODE HAS FLAGGED DELETE=TRUE

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end())
    {   
        CGovernanceObject* pObj = &((*it).second);

        // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
        pObj->UpdateLocalValidity(pCurrentBlockIndex);
        // UPDATE SENTINEL SIGNALING VARIABLES
        pObj->UpdateSentinelVariables(pCurrentBlockIndex);

        // SHOULD WE DELETE THIS OBJECT FROM MEMORY

        /*
            - delete objects from memory where fCachedDelete is true
            - this should be robust enough that if someone sends us the proposal later, we should know it was deleted
        */

        ++it;
    }

}

CGovernanceObject *CGovernanceManager::FindGovernanceObject(const std::string &strName)
{
    // find the prop with the highest yes count

    int nYesCount = -99999;
    CGovernanceObject* pGovObj = NULL;

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end()){
        pGovObj = &((*it).second);
        int nGovObjYesCount = pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING);
        if((*it).second.strName == strName && nGovObjYesCount > nYesCount){
            nYesCount = nGovObjYesCount;
        }
        ++it;
    }

    if(nYesCount == -99999) return NULL;

    return pGovObj;
}

CGovernanceObject *CGovernanceManager::FindGovernanceObject(const uint256& nHash)
{
    LOCK(cs);

    if(mapObjects.count(nHash))
        return &mapObjects[nHash];

    return NULL;
}

std::vector<CGovernanceObject*> CGovernanceManager::GetAllProposals(int64_t nMoreThanTime)
{
    LOCK(cs);

    std::vector<CGovernanceObject*> vGovObjs;

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end())
    {
        // if((*it).second.nTime < nMoreThanTime) {
        //     ++it;
        //     continue;
        // }
        // (*it).second.CleanAndRemove(false);

        CGovernanceObject* pGovObj = &((*it).second);
        vGovObjs.push_back(pGovObj);

        ++it;
    }

    return vGovObjs;
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
    if (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_GOVERNANCE) return;

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
    //         if(pnode->nVersion >= MSG_GOVERNANCE_PEER_PROTO_VERSION) 
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
        if(!IsCollateralValid((*it4).nFeeTXHash, (*it4).GetHash(), strError, nConf, GOVERNANCE_FEE_TX)){
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

void CGovernanceManager::Sync(CNode* pfrom, uint256 nProp)
{
    LOCK(cs);

    /*
        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.
    */

    int nInvCount = 0;

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    std::map<uint256, CGovernanceObject>::iterator it1 = mapObjects.begin();
    while(it1 != mapObjects.end()) {
        uint256 h = (*it1).first;

        if((*it1).second.fCachedValid && ((nProp == uint256() || (h == nProp)))){
            // Push the inventory budget proposal message over to the other client
            pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT, h));
            nInvCount++;
        }
        ++it1;
    }

    // SYNC OUR GOVERNANCE OBJECT VOTES WITH THEIR GOVERNANCE OBJECT VOTES

    std::map<uint256, CGovernanceVote>::iterator it2 = mapVotesByHash.begin();
    while(it2 != mapVotesByHash.end()) {
        pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT_VOTE, (*it2).first));
        nInvCount++;
        ++it2;
    }

    pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ, nInvCount);
    LogPrintf("CGovernanceManager::Sync - sent %d items\n", nInvCount);
}

void CGovernanceManager::SyncParentObjectByVote(CNode* pfrom, const CGovernanceVote& vote)
{
    if(!mapAskedForGovernanceObject.count(vote.nParentHash)){
        pfrom->PushMessage(NetMsgType::MNGOVERNANCESYNC, vote.nParentHash);
        mapAskedForGovernanceObject[vote.nParentHash] = GetTime();
    }
}

bool CGovernanceManager::UpdateGovernanceObject(const CGovernanceVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    if(!mapObjects.count(vote.nParentHash)){
        if(pfrom){
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CGovernanceManager::UpdateGovernanceObject - Unknown object %d, asking for source\n", vote.nParentHash.ToString());
            mapOrphanVotes[vote.nParentHash] = vote;

            SyncParentObjectByVote(pfrom, vote);
        }

        strError = "Governance object not found!";
        return false;
    }


    return AddOrUpdateVote(vote, strError);
}

bool CGovernanceManager::AddOrUpdateVote(const CGovernanceVote& vote, std::string& strError)
{
    LOCK(cs);

    // GET DETERMINISTIC HASH WHICH COLLIDES ON MASTERNODE-VIN/GOVOBJ-HASH/VOTE-SIGNAL
    
    uint256 nTypeHash = vote.GetTypeHash();
    uint256 nHash = vote.GetHash();

    // LOOK FOR PREVIOUS VOTES BY THIS SPECIFIC MASTERNODE FOR THIS SPECIFIC SIGNAL

    if(mapVotesByType.count(nTypeHash)) {
        if(mapVotesByType[nTypeHash].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s", nTypeHash.ToString());
            LogPrint("mngovernance", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotesByType[nTypeHash].nTime < GOVERNANCE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli", nTypeHash.ToString(), vote.nTime - mapVotesByType[nTypeHash].nTime);
            LogPrint("mngovernance", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    // UPDATE TO NEWEST VOTE

    mapVotesByType[nTypeHash] = vote;
    mapVotesByHash[nHash] = vote;

    // // SET CACHE AS DIRTY / WILL BE UPDATED NEXT BLOCK

    // CGovernanceObject* pGovObj = FindGovernanceObject(vote.GetParentHash());
    // if(pGovObj) pGovObj->fDirtyCache = true;

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
    //fDirtyCache = true;
}

CGovernanceObject::CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, std::string strNameIn, int64_t nTimeIn, uint256 nFeeTXHashIn)
{
    nHashParent = nHashParentIn; //parent object, 0 is root
    nRevision = nRevisionIn; //object revision in the system
    strName = strNameIn;
    nTime = nTimeIn;
    nFeeTXHash = nFeeTXHashIn; //fee-tx    

    // caching
    fCachedFunding = false;
    fCachedValid = true;
    fCachedDelete = false;
    fCachedEndorsed = false;
    //fDirtyCache = true;
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

    // caching
    fCachedFunding = other.fCachedFunding;
    fCachedValid = other.fCachedValid;
    fCachedDelete = other.fCachedDelete;
    fCachedEndorsed = other.fCachedEndorsed;
    //fDirtyCache = other.fDirtyCache;
}

bool CGovernanceObject::IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{
    if(GetNoCount(VOTE_SIGNAL_VALID) - GetYesCount(VOTE_SIGNAL_VALID) > mnodeman.CountEnabled(MSG_GOVERNANCE_PEER_PROTO_VERSION)/10){
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
        strError = "Invalid object name, limit of 20 characters.";
        return false;
    }

    if(strName != SanitizeString(strName)) {
        strError = "Invalid object name, unsafe characters found.";
        return false;
    }

    if(fCheckCollateral){
        int nConf = 0;
        if(!IsCollateralValid(nFeeTXHash, GetHash(), strError, nConf, GOVERNANCE_FEE_TX)){
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

    // std::map<uint256, CGovernanceVote>::iterator it2 = mapVotesByHash.begin();
    // while(it2 != mapVotesByHash.end()){
    //     if(!(*it2).second.IsValid(fSignatureCheck))
    //     {
    //         // 12.1 - log to specialized handle (govobj?)
    //         LogPrintf("CGovernanceManager::CleanAndRemove - Proposal/Budget is known, activating and removing orphan vote\n");
    //         mapVotesByHash.erase(it2++);
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

int CGovernanceObject::GetAbsoluteNoCount(int nVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), nVoteSignalIn, VOTE_OUTCOME_NO) - governance.CountMatchingVotes((*this), nVoteSignalIn, VOTE_OUTCOME_YES);
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
    CInv inv(MSG_GOVERNANCE_OBJECT, GetHash());
    RelayInv(inv, MSG_GOVERNANCE_PEER_PROTO_VERSION);
}

std::string CGovernanceManager::ToString() const
{
    std::ostringstream info;

    info << "Governance Objects: " << (int)mapObjects.size() <<
            ", Seen Budgets: " << (int)mapSeenGovernanceObjects.size() <<
            ", Seen Budget Votes: " << (int)mapSeenVotes.size() <<
            ", VoteByHash Count: " << (int)mapVotesByHash.size() << 
            ", VoteByType Count: " << (int)mapVotesByType.size();

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

    std::map<uint256, CGovernanceVote>::iterator it2 = mapVotesByHash.begin();
    while(it2 != mapVotesByHash.end()){
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