// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

#include "flat-database.h"
#include "governance.h"
#include "governance-vote.h"
#include "governance-classes.h"
#include "masternode.h"
#include "governance.h"
#include "darksend.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <univalue.h>

class CNode;
class CBudgetVote;

CGovernanceManager governance;
CCriticalSection cs_budget;

std::map<uint256, int64_t> mapAskedForGovernanceObject;

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

    DBG( cout << "IsCollateralValid txCollateral.vout.size() = " << txCollateral.vout.size() << endl; );

    DBG( cout << "IsCollateralValid: findScript = " << ScriptToAsmStr( findScript, false ) << endl; );


    bool foundOpReturn = false;
    BOOST_FOREACH(const CTxOut o, txCollateral.vout){
        DBG( cout << "IsCollateralValid txout : " << o.ToString() 
             << ", o.nValue = " << o.nValue
             << ", o.scriptPubKey = " << ScriptToAsmStr( o.scriptPubKey, false )
             << endl; );
        if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()){
            strError = strprintf("Invalid Script %s", txCollateral.ToString());
            LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
            return false;
        }
        if(o.scriptPubKey == findScript && o.nValue >= minFee) {
            DBG( cout << "IsCollateralValid foundOpReturn = true" << endl; );
            foundOpReturn = true;
        }
        else  {
            DBG( cout << "IsCollateralValid No match, continuing" << endl; );
        }

    }
    // 12.1 - todo: resolved
    /*
        Governance object is not valid - 4f4c4c9bf19d28ddcf819a71002adde9a517570778ec7fe1f9b819f7c3e02711 - Couldn't find opReturn 4f4c4c9bf19d28ddcf819a71002adde9a517570778ec7fe1f9b819f7c3e02711 in CTransaction(hash=8a5eb3c478, ver=1, vin.size=1, vout.size=2, nLockTime=48761)
        CTxIn(COutPoint(455cd6da9357b267bcbae23b905ef9207b5ceced4436f9f0abfadf87a0a783ce, 1), scriptSig=4730440220538eb8c02ce268, nSequence=4294967294)
        CTxOut(nValue=0.10000000, scriptPubKey=6a200e607ecda9227d293a261ffd87)
        CTxOut(nValue=9.07485900, scriptPubKey=76a914d57173b7da84724a4569671e)
    */

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

// Accessors for thread-safe access to maps
bool CGovernanceManager::HaveObjectForHash(uint256 nHash)  {
    LOCK(cs);
    return (mapObjects.count(nHash) == 1);
}

bool CGovernanceManager::HaveVoteForHash(uint256 nHash)  {
    LOCK(cs);
    return (mapVotesByHash.count(nHash) == 1);
}

// int CGovernanceManager::GetVoteCountByHash(uint256 nHash)  {
//     int count = 0;
//     LOCK(cs);
//     count_m_cit it = mapVotesByHash.find(nHash);
//     if (it != mapVotesByHash.end())  {
//         count = it->second;
//     }
//     return count;
// }

bool CGovernanceManager::SerializeObjectForHash(uint256 nHash, CDataStream& ss)  {
    LOCK(cs);
    object_m_it it = mapObjects.find(nHash);
    if (it == mapObjects.end())  {
        return false;
    }
    ss << it->second;
    return true;
}

bool CGovernanceManager::SerializeVoteForHash(uint256 nHash, CDataStream& ss)  {
    LOCK(cs);
    vote_m_it it = mapVotesByHash.find(nHash);
    if (it == mapVotesByHash.end())  {
        return false;
    }
    ss << it->second;
    return true;
}

void CGovernanceManager::AddSeenGovernanceObject(uint256 nHash, int status)
{
    LOCK(cs);
    mapSeenGovernanceObjects[nHash] = status;
}

void CGovernanceManager::AddSeenVote(uint256 nHash, int status)
{
    LOCK(cs);
    mapSeenVotes[nHash] = status;
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

    // ANOTHER USER IS ASKING US TO HELP THEM SYNC GOVERNANCE OBJECT DATA
    if (strCommand == NetMsgType::MNGOVERNANCESYNC)
    {

        // ignore such request until we are fully synced
        if (!masternodeSync.IsSynced()) return;

        uint256 nProp;
        vRecv >> nProp;

        // IF THE NETWORK IS MAIN, MAKE SURE THEY HAVEN'T ASKED US BEFORE

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(nProp == uint256()) {
                if(pfrom->HasFulfilledRequest(NetMsgType::MNGOVERNANCESYNC)) {
                    LogPrint("gobject", "peer already asked me for the list\n");
                    // BAD PEER! BAD!
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest(NetMsgType::MNGOVERNANCESYNC);
            }
        }

        // ask for a specific govobj and it's votes
        Sync(pfrom, nProp);
        LogPrint("gobject", "syncing governance objects to our peer at %s\n", pfrom->addr.ToString());
    
    }

    // A NEW GOVERNANCE OBJECT HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECT)

    {
        LOCK(cs);
        // MAKE SURE WE HAVE A VALID REFERENCE TO THE TIP BEFORE CONTINUING

        if(!pCurrentBlockIndex) return;

        CGovernanceObject govobj;
        vRecv >> govobj;

        if(mapSeenGovernanceObjects.count(govobj.GetHash())){
            // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
            masternodeSync.AddedBudgetItem(govobj.GetHash());
            return;
        }

        // IS THE COLLATERAL TRANSACTION ASSOCIATED WITH THIS GOVERNANCE OBJECT MATURE/VALID?

        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid(govobj.nCollateralHash, govobj.GetHash(), strError, nConf, GOVERNANCE_FEE_TX)){
            LogPrintf("Governance object collateral tx is not valid - %s - %s\n", govobj.nCollateralHash.ToString(), strError);
            return;
        }

        // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN 

        if(!govobj.IsValidLocally(pCurrentBlockIndex, strError, true)) {
            mapSeenGovernanceObjects.insert(make_pair(govobj.GetHash(), SEEN_OBJECT_ERROR_INVALID));
            LogPrintf("Governance object is invalid - %s\n", strError);
            return;
        }
        
        // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANANGED DATA

        {
            govobj.UpdateSentinelVariables(pCurrentBlockIndex); //this sets local vars in object

            if(AddGovernanceObject(govobj))
            {
                govobj.Relay();
            }
        }

        // UPDATE THAT WE'VE SEEN THIS OBJECT
        mapSeenGovernanceObjects.insert(make_pair(govobj.GetHash(), SEEN_OBJECT_IS_VALID));
        masternodeSync.AddedBudgetItem(govobj.GetHash());

        LogPrintf("MNGOVERNANCEOBJECT -- %s new\n", govobj.GetHash().ToString());
        
        // WE MIGHT HAVE PENDING/ORPHAN VOTES FOR THIS OBJECT

        CheckOrphanVotes();
    }

    // A NEW GOVERNANCE OBJECT VOTE HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECTVOTE)
    {
        CGovernanceVote vote;
        vRecv >> vote;
        vote.fValid = true;

        // IF WE'VE SEEN THIS OBJECT THEN SKIP

        if(mapSeenVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        // FIND THE MASTERNODE OF THE VOTER

        CMasternode* pmn = mnodeman.Find(vote.vinMasternode);
        if(pmn == NULL) {
            LogPrint("gobject", "gobject - unknown masternode - vin: %s\n", vote.vinMasternode.ToString());
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
            return;
        }

        // CHECK LOCAL VALIDITY AGAINST BLOCKCHAIN, TIME DATA, ETC

        if(!vote.IsValid(true)){
            LogPrintf("gobject - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
            mapSeenVotes.insert(make_pair(vote.GetHash(), SEEN_OBJECT_ERROR_INVALID));
            return;
        } else {
            mapSeenVotes.insert(make_pair(vote.GetHash(), SEEN_OBJECT_IS_VALID));
        }

        // IF EVERYTHING CHECKS OUT, UPDATE THE GOVERNANCE MANAGER

        std::string strError = "";
        if(AddOrUpdateVote(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());
            pmn->AddGovernanceVote(vote.GetParentHash());
        }

        LogPrint("gobject", "NEW governance vote: %s\n", vote.GetHash().ToString());
    }

}

void CGovernanceManager::CheckOrphanVotes()
{
    LOCK(cs);

    std::string strError = "";
    std::map<uint256, CGovernanceVote>::iterator it1 = mapOrphanVotes.begin();
    while(it1 != mapOrphanVotes.end()){
        if(AddOrUpdateVote(((*it1).second), NULL, strError)){
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

    DBG( cout << "CGovernanceManager::AddGovernanceObject START" << endl; );

    // MAKE SURE THIS OBJECT IS OK

    if(!govobj.IsValidLocally(pCurrentBlockIndex, strError, true)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject - invalid governance object - %s - (pCurrentBlockIndex nHeight %d) \n", strError, pCurrentBlockIndex->nHeight);
        return false;
    }

    // IF WE HAVE THIS OBJECT ALREADY, WE DON'T WANT ANOTHER COPY

    if(mapObjects.count(govobj.GetHash())) {
        LogPrintf("CGovernanceManager::AddGovernanceObject - already have governance object - %s\n", strError);
        return false;
    }

    // INSERT INTO OUR GOVERNANCE OBJECT MEMORY
    {
        mapObjects.insert(make_pair(govobj.GetHash(), govobj));
    }

    // SHOULD WE ADD THIS OBJECT TO ANY OTHER MANANGERS?

    DBG( cout << "CGovernanceManager::AddGovernanceObject Before trigger block, strData = "
              << govobj.GetDataAsString()
              << ", nObjectType = " << govobj.nObjectType
              << endl; );

    if(govobj.nObjectType == GOVERNANCE_OBJECT_TRIGGER)
    {
        DBG( cout << "CGovernanceManager::AddGovernanceObject Before AddNewTrigger" << endl; );
        triggerman.AddNewTrigger(govobj.GetHash());
        DBG( cout << "CGovernanceManager::AddGovernanceObject After AddNewTrigger" << endl; );
    }

    DBG( cout << "CGovernanceManager::AddGovernanceObject END" << endl; );

    return true;
}

void CGovernanceManager::UpdateCachesAndClean()
{
    LogPrintf("CGovernanceManager::UpdateCachesAndClean \n");

    LOCK(cs);

    // DOUBLE CHECK THAT WE HAVE A VALID POINTER TO TIP

    if(!pCurrentBlockIndex) return;

    // UPDATE CACHE FOR EACH OBJECT THAT IS FLAGGED DIRTYCACHE=TRUE

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();

    std::map<uint256, int> mapDirtyObjects;

    // Clean up any expired or invalid triggers
    triggerman.CleanAndRemove();

    while(it != mapObjects.end())
    {   
        CGovernanceObject* pObj = &((*it).second);

        if(!pObj)  {
            ++it;
            continue;
        }

        // IF CACHE IS NOT DIRTY, WHY DO THIS?
        if(pObj->fDirtyCache)  {
            mapDirtyObjects.insert(make_pair((*it).first, 1));
            
            // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
            
            pObj->UpdateLocalValidity(pCurrentBlockIndex);
            
            // UPDATE SENTINEL SIGNALING VARIABLES
            
            pObj->UpdateSentinelVariables(pCurrentBlockIndex);
        }

        // IF DELETE=TRUE, THEN CLEAN THE MESS UP!

        if(pObj->fCachedDelete || pObj->fExpired)
        {
            LogPrintf("UpdateCachesAndClean --- erase obj %s\n", (*it).first.ToString());
            mapObjects.erase(it++);
        } else {
            ++it;
        }
    }

    // CHECK EACH GOVERNANCE OBJECTS VALIDITY (CPU HEAVY)

    // 12.1 todo - compile issues

    // std::map<uint256, CBudgetVote>::iterator it = mapVotesByHash.begin();
    // while(it != mapVotes.end()) {

    //     // ONLY UPDATE THE DIRTY OBJECTS!

    //     if(mapDirtyObjects.count((*it).first))
    //     {
    //         (*it).second.fValid = (*it).second.IsValid(true);
    //         ++it;
    //     }
    // }

}

CGovernanceObject *CGovernanceManager::FindGovernanceObject(const std::string &strName)
{
    // find the prop with the highest yes count

    int nYesCount = -99999;
    CGovernanceObject* pGovObj = NULL;

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end()){
        int nGovObjYesCount = pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING);
        if((*it).second.strName == strName && nGovObjYesCount > nYesCount){
            nYesCount = nGovObjYesCount;
            pGovObj = &((*it).second);
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

std::vector<CGovernanceVote*> CGovernanceManager::GetMatchingVotes(const uint256& nParentHash)
{
    std::vector<CGovernanceVote*> vecResult;

    // LOOP THROUGH ALL VOTES AND FIND THOSE MATCHING USER HASH

    std::map<uint256, CGovernanceVote>::iterator it2 = mapVotesByHash.begin();
    while(it2 != mapVotesByHash.end()){
        if((*it2).second.GetParentHash() == nParentHash)
        {
            vecResult.push_back(&(*it2).second);
        }
        *it2++;
    }

    return vecResult;
}

std::vector<CGovernanceObject*> CGovernanceManager::GetAllNewerThan(int64_t nMoreThanTime)
{
    LOCK(cs);

    std::vector<CGovernanceObject*> vGovObjs;

    std::map<uint256, CGovernanceObject>::iterator it = mapObjects.begin();
    while(it != mapObjects.end())
    {
        // IF THIS OBJECT IS OLDER THAN TIME, CONTINUE

        if((*it).second.nTime < nMoreThanTime) {
            ++it;
            continue;
        }

        // ADD GOVERNANCE OBJECT TO LIST

        CGovernanceObject* pGovObj = &((*it).second);
        vGovObjs.push_back(pGovObj);

        // NEXT

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
      return (UintToArith256(left.first->nCollateralHash) > UintToArith256(right.first->nCollateralHash));
    }
};

void CGovernanceManager::NewBlock()
{
    TRY_LOCK(cs, fBudgetNewBlock);
    if(!fBudgetNewBlock || !pCurrentBlockIndex) return;

    // IF WE'RE NOT SYNCED, EXIT

    if(!masternodeSync.IsSynced()) return;

    // CHECK OBJECTS WE'VE ASKED FOR, REMOVE OLD ENTRIES

    std::map<uint256, int64_t>::iterator it = mapAskedForGovernanceObject.begin();
    while(it != mapAskedForGovernanceObject.end()){
        if((*it).second > GetTime() - (60*60*24)){
            ++it;
        } else {
            mapAskedForGovernanceObject.erase(it++);
        }
    }

    // CHECK AND REMOVE - REPROCESS GOVERNANCE OBJECTS

    UpdateCachesAndClean();
}

void CGovernanceManager::Sync(CNode* pfrom, uint256 nProp)
{

    /*
        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.
    */

    int nInvCount = 0;

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    {
       LOCK(cs);
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

bool CGovernanceManager::AddOrUpdateVote(const CGovernanceVote& vote, CNode* pfrom, std::string& strError)
{
    // MAKE SURE WE HAVE THE PARENT OBJECT THE VOTE IS FOR

    bool syncparent = false;
    uint256 votehash;
    {
        LOCK(cs);
        if(!mapObjects.count(vote.nParentHash))  {
                if(pfrom)  {
                    // only ask for missing items after our syncing process is complete -- 
                    //   otherwise we'll think a full sync succeeded when they return a result
                    if(!masternodeSync.IsSynced()) return false;
                    
                    // ADD THE VOTE AS AN ORPHAN, TO BE USED UPON RECEIVAL OF THE PARENT OBJECT
                    
                    LogPrintf("CGovernanceManager::AddOrUpdateVote - Unknown object %d, asking for source\n", vote.nParentHash.ToString());
                    mapOrphanVotes[vote.nParentHash] = vote;
                    
                    // ASK FOR THIS VOTES PARENT SPECIFICALLY FROM THIS USER (THEY SHOULD HAVE IT, NO?)

                    if(!mapAskedForGovernanceObject.count(vote.nParentHash)){
                        syncparent = true;
                        votehash = vote.nParentHash;
                        mapAskedForGovernanceObject[vote.nParentHash] = GetTime();
                    }
                }

                strError = "Governance object not found!";
                return false;
        }
    }

    // Need to keep this out of the locked section
    if(syncparent)  {
        pfrom->PushMessage(NetMsgType::MNGOVERNANCESYNC, votehash);
    }

    // Reestablish lock
    LOCK(cs);
    // GET DETERMINISTIC HASH WHICH COLLIDES ON MASTERNODE-VIN/GOVOBJ-HASH/VOTE-SIGNAL
    
    uint256 nTypeHash = vote.GetTypeHash();
    uint256 nHash = vote.GetHash();

    // LOOK FOR PREVIOUS VOTES BY THIS SPECIFIC MASTERNODE FOR THIS SPECIFIC SIGNAL

    if(mapVotesByType.count(nTypeHash)) {
        if(mapVotesByType[nTypeHash].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s", nTypeHash.ToString());
            LogPrint("gobject", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotesByType[nTypeHash].nTime < GOVERNANCE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli", nTypeHash.ToString(), vote.nTime - mapVotesByType[nTypeHash].nTime);
            LogPrint("gobject", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    // UPDATE TO NEWEST VOTE
    {
        mapVotesByHash[nHash] = vote;   
        mapVotesByType[nTypeHash] = vote;
    }

    // SET CACHE AS DIRTY / WILL BE UPDATED NEXT BLOCK

    CGovernanceObject* pGovObj = FindGovernanceObject(vote.GetParentHash());
    if(pGovObj)
    {
        pGovObj->fDirtyCache = true;
        UpdateCachesAndClean();
    } else {
        LogPrintf("Governance object not found! Can't update fDirtyCache - %s\n", vote.GetParentHash().ToString());
    }

    return true;
}

CGovernanceObject::CGovernanceObject()
{
    // MAIN OBJECT DATA

    strName = "unknown";
    nTime = 0;
    nObjectType = GOVERNANCE_OBJECT_UNKNOWN;

    nHashParent = uint256(); //parent object, 0 is root
    nRevision = 0; //object revision in the system
    nCollateralHash = uint256(); //fee-tx
    
    // CACHING FOR VARIOUS FLAGS

    fCachedFunding = false;
    fCachedValid = true;
    fCachedDelete = false;
    fCachedEndorsed = false;
    fDirtyCache = true;
    fUnparsable = false;
    fExpired = false;

    // PARSE JSON DATA STORAGE (STRDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, std::string strNameIn, int64_t nTimeIn, uint256 nCollateralHashIn, std::string strDataIn)
{
    // MAIN OBJECT DATA

    nHashParent = nHashParentIn; //parent object, 0 is root
    nRevision = nRevisionIn; //object revision in the system
    strName = strNameIn;
    nTime = nTimeIn;
    nCollateralHash = nCollateralHashIn; //fee-tx
    nObjectType = GOVERNANCE_OBJECT_UNKNOWN; // Avoid having an uninitialized variable    
    strData = strDataIn;

    // CACHING FOR VARIOUS FLAGS

    fCachedFunding = false;
    fCachedValid = true;
    fCachedDelete = false;
    fCachedEndorsed = false;
    fDirtyCache = true;
    fUnparsable = false;
    fExpired = false;

    // PARSE JSON DATA STORAGE (STRDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(const CGovernanceObject& other)
{
    // COPY OTHER OBJECT'S DATA INTO THIS OBJECT

    nHashParent = other.nHashParent;
    nRevision = other.nRevision;
    strName = other.strName;
    nTime = other.nTime;
    nCollateralHash = other.nCollateralHash;
    strData = other.strData;
    nObjectType = other.nObjectType;
    fUnparsable = true;

    // caching
    fCachedFunding = other.fCachedFunding;
    fCachedValid = other.fCachedValid;
    fCachedDelete = other.fCachedDelete;
    fCachedEndorsed = other.fCachedEndorsed;
    fDirtyCache = other.fDirtyCache;
    fExpired = other.fExpired;
}

int CGovernanceObject::GetObjectType()
{
    return nObjectType;
}

int CGovernanceObject::GetObjectSubtype()
{
    // todo - 12.1
    //   - detect subtype from strData json, obj["subtype"]

    if(nObjectType == GOVERNANCE_OBJECT_TRIGGER) return TRIGGER_SUPERBLOCK;
    return -1;
}

uint256 CGovernanceObject::GetHash()
{
    // CREATE HASH OF ALL IMPORTANT PIECES OF DATA

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << nHashParent;
    ss << nRevision;
    ss << strName;
    ss << nTime;
    ss << strData;
    // fee_tx is left out on purpose
    uint256 h1 = ss.GetHash();

    DBG( printf("CGovernanceObject::GetHash %i %s %li %s\n", nRevision, strName.c_str(), nTime, strData.c_str()); );

    return h1;
}

/**
   Return the actual object from the strData JSON structure.

   Returns an empty object on error.
 */
UniValue CGovernanceObject::GetJSONObject()
{
    UniValue obj(UniValue::VOBJ);
    if(strData.empty())  {
        return obj;
    }

    UniValue objResult(UniValue::VOBJ);
    if(!GetData(objResult)) {
        return obj;
    }

    try  {
        std::vector<UniValue> arr1 = objResult.getValues();
        std::vector<UniValue> arr2 = arr1.at( 0 ).getValues();
        obj = arr2.at( 1 );
    }
    catch(...)  {
        obj = UniValue(UniValue::VOBJ);
    }
    return obj;
}

/**
*   LoadData
*   --------------------------------------------------------
* 
*   Attempt to load data from strData 
*    
*/

void CGovernanceObject::LoadData()
{
    // todo : 12.1 - resolved
    //return;

    if(strData.empty())  {
        return;
    }

    // ATTEMPT TO LOAD JSON STRING FROM STRDATA
    UniValue objResult(UniValue::VOBJ);
    if(!GetData(objResult)) {
        fUnparsable = true;
    }

    DBG( cout << "CGovernanceObject::LoadData strData = "
              << GetDataAsString()
              << endl; );

    try
    {
        UniValue obj = GetJSONObject();
        nObjectType = obj["type"].get_int();
    }
    catch (int e)
    {
        return;
    }

}

/**
*   SetData - Example usage:
*   --------------------------------------------------------
* 
*   Data must be stored as an encoded hex/json string.
*   Other than the above requirement gov objects are data-agnostic.
*    
*/

bool CGovernanceObject::SetData(std::string& strError, std::string strDataIn)
{
    // SET DATA FIELD TO INPUT

    if(strDataIn.size() > 512*4)
    {
        // (assumption) this is equal to pythons len(strData) > 512*4, I think 
        strError = "Too big.";
        return false;
    }

    strData = strDataIn;
    return true;
}

/**
*   GetData - Example usage:
*   --------------------------------------------------------
* 
*   Decode governance object data into UniValue(VOBJ)
*    
*/

bool CGovernanceObject::GetData(UniValue& objResult)
{
    // NOTE : IS THIS SAFE? 

    try
    {
        UniValue o(UniValue::VOBJ);
        std::string s = GetDataAsString();
        o.read(s);
        objResult = o;
    }
    catch (int e)
    {
        return false;
    }

    return true;
}

/**
*   GetData - As
*   --------------------------------------------------------
* 
*/

std::string CGovernanceObject::GetDataAsHex()
{
    return strData;
}

std::string CGovernanceObject::GetDataAsString()
{
    vector<unsigned char> v = ParseHex(strData);
    string s(v.begin(), v.end());

    return s;
}

void CGovernanceObject::UpdateLocalValidity(const CBlockIndex *pCurrentBlockIndex)
{
    // THIS DOES NOT CHECK COLLATERAL, THIS IS CHECKED UPON ORIGINAL ARRIVAL
    fCachedLocalValidity = IsValidLocally(pCurrentBlockIndex, strLocalValidityError, false);
};


bool CGovernanceObject::IsValidLocally(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{
    if(!pindex) {
        strError = "Tip is NULL";
        return true;
    }

    if(strName.size() > 20) {
        strError = "Invalid object name, limit of 20 characters.";
        return false;
    }

    if(strName != SanitizeString(strName)) {
        strError = "Invalid object name, unsafe characters found.";
        return false;
    }

    // IF ABSOLUTE NO COUNT (NO-YES VALID VOTES) IS MORE THAN 10% OF THE NETWORK MASTERNODES, OBJ IS INVALID

    if(GetAbsoluteNoCount(VOTE_SIGNAL_VALID) > mnodeman.CountEnabled(MSG_GOVERNANCE_PEER_PROTO_VERSION)/10){
         strError = "Automated removal";
         return false;
    }

    // CHECK COLLATERAL IF REQUIRED (HIGH CPU USAGE)

    if(fCheckCollateral){
        int nConf = 0;
        if(!IsCollateralValid(nCollateralHash, GetHash(), strError, nConf, GOVERNANCE_FEE_TX)){
            // strError set in IsCollateralValid
            if(strError == "") strError = "Collateral is invalid";
            return false;
        }
    }

    /*
        TODO

        - There might be an issue with multisig in the coinbase on mainnet, we will add support for it in a future release.
        - Post 12.2+ (test multisig coinbase transaction)
    */

    // 12.1 - todo - compile error
    // if(address.IsPayToScriptHash()) {
    //     strError = "Governance system - multisig is not currently supported";
    //     return false;
    // }

    return true;
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
    // Note this gets called from ActivateBestChain without cs_main being held
    // so it should be safe to lock our mutex here without risking a deadlock
    // On the other hand it should be safe for us to access pindex without holding a lock 
    // on cs_main because the CBlockIndex objects are dynamically allocated and
    // presumably never deleted.
    LOCK(cs);
    pCurrentBlockIndex = pindex;
    nCachedBlockHeight = pCurrentBlockIndex->nHeight;
    LogPrint("gobject", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    // TO REPROCESS OBJECTS WE SHOULD BE SYNCED

    if(!fLiteMode && masternodeSync.GetAssetID() > MASTERNODE_SYNC_LIST)
        NewBlock();
}

int CGovernanceManager::CountMatchingVotes(CGovernanceObject& govobj, int nVoteSignalIn, int nVoteOutcomeIn)
{
    /*
    *   
    *   Count matching votes and return
    *
    */

    LOCK(cs);
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

void CGovernanceObject::UpdateSentinelVariables(const CBlockIndex *pCurrentBlockIndex)
{
    // CALCULATE MINIMUM SUPPORT LEVELS REQUIRED

    int nMnCount = mnodeman.CountEnabled();
    if(nMnCount == 0) return;

    // CALCULATE THE MINUMUM VOTE COUNT REQUIRED FOR FULL SIGNAL

    // todo - 12.1 - should be set to `10` after governance vote compression is implemented
    int nAbsVoteReq = max(1, nMnCount / 10);
    // todo - 12.1 - Temporarily set to 1 for testing - reverted
    //nAbsVoteReq = 1;

    // SET SENTINEL FLAGS TO FALSE

    fCachedFunding = false;
    fCachedValid = true; //default to valid
    fCachedDelete = false;
    fCachedEndorsed = false;
    fDirtyCache = false;
    
    // SET SENTINEL FLAGS TO TRUE IF MIMIMUM SUPPORT LEVELS ARE REACHED
    // ARE ANY OF THESE FLAGS CURRENTLY ACTIVATED?

    if(GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING) >= nAbsVoteReq) fCachedFunding = true;
    if(GetAbsoluteYesCount(VOTE_SIGNAL_VALID) >= nAbsVoteReq) fCachedValid = true;
    if(GetAbsoluteYesCount(VOTE_SIGNAL_DELETE) >= nAbsVoteReq) fCachedDelete = true;
    if(GetAbsoluteYesCount(VOTE_SIGNAL_ENDORSED) >= nAbsVoteReq) fCachedEndorsed = true;

    // ARE ANY OF THE VOTING FLAGS NEGATIVELY SET BY THE NETWORK?
    // THIS CAN BE CACHED, THE VOTES BEING HOT-LOADED AS NEEDED TO RECALCULATE

    if(GetAbsoluteNoCount(VOTE_SIGNAL_FUNDING) >= nAbsVoteReq) fCachedFunding = false;
    if(GetAbsoluteNoCount(VOTE_SIGNAL_VALID) >= nAbsVoteReq) fCachedValid = false;
    if(GetAbsoluteNoCount(VOTE_SIGNAL_DELETE) >= nAbsVoteReq) fCachedDelete = false;
    if(GetAbsoluteNoCount(VOTE_SIGNAL_ENDORSED) >= nAbsVoteReq) fCachedEndorsed = false;
}

void CGovernanceObject::swap(CGovernanceObject& first, CGovernanceObject& second) // nothrow
{
    // enable ADL (not necessary in our case, but good practice)
    using std::swap;

    // by swapping the members of two classes,
    // the two classes are effectively swapped
    swap(first.strName, second.strName);
    swap(first.nHashParent, second.nHashParent);
    swap(first.nRevision, second.nRevision);
    swap(first.nTime, second.nTime);
    swap(first.nCollateralHash, second.nCollateralHash);
    swap(first.strData, second.strData);
    swap(first.nObjectType, second.nObjectType);

    // swap all cached valid flags
    swap(first.fCachedFunding, second.fCachedFunding);
    swap(first.fCachedValid, second.fCachedValid);
    swap(first.fCachedDelete, second.fCachedDelete);
    swap(first.fCachedEndorsed, second.fCachedEndorsed);
    swap(first.fDirtyCache, second.fDirtyCache);
    swap(first.fExpired, second.fExpired);
}
