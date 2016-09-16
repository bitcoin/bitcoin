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

CGovernanceManager governance;

std::map<uint256, int64_t> mapAskedForGovernanceObject;

int nSubmittedFinalBudget;

CGovernanceManager::CGovernanceManager()
    : mapCollateral(),
      pCurrentBlockIndex(NULL),
      nTimeLastDiff(0),
      nCachedBlockHeight(0),
      mapObjects(),
      mapSeenGovernanceObjects(),
      mapSeenVotes(),
      mapOrphanVotes(),
      mapLastMasternodeTrigger(),
      cs()
{}

// Accessors for thread-safe access to maps
bool CGovernanceManager::HaveObjectForHash(uint256 nHash) {
    LOCK(cs);
    return (mapObjects.count(nHash) == 1);
}

bool CGovernanceManager::HaveVoteForHash(uint256 nHash) {
    LOCK(cs);
    return (mapVotesByHash.count(nHash) == 1);
}

bool CGovernanceManager::SerializeObjectForHash(uint256 nHash, CDataStream& ss) {
    LOCK(cs);
    object_m_it it = mapObjects.find(nHash);
    if (it == mapObjects.end()) {
        return false;
    }
    ss << it->second;
    return true;
}

bool CGovernanceManager::SerializeVoteForHash(uint256 nHash, CDataStream& ss) {
    LOCK(cs);
    vote_m_it it = mapVotesByHash.find(nHash);
    if (it == mapVotesByHash.end()) {
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

    LOCK(governance.cs);

    // ANOTHER USER IS ASKING US TO HELP THEM SYNC GOVERNANCE OBJECT DATA
    if (strCommand == NetMsgType::MNGOVERNANCESYNC)
    {

        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
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
        // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN

        if(!govobj.IsValidLocally(pCurrentBlockIndex, strError, true)) {
            mapSeenGovernanceObjects.insert(std::make_pair(govobj.GetHash(), SEEN_OBJECT_ERROR_INVALID));
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
        // Ignore such messages until masternode list is synced
        if(!masternodeSync.IsMasternodeListSynced()) return;

        CGovernanceVote vote;
        vRecv >> vote;
        //vote.fValid = true;

        // IF WE'VE SEEN THIS OBJECT THEN SKIP

        if(mapSeenVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        // FIND THE MASTERNODE OF THE VOTER

        CMasternode* pmn = mnodeman.Find(vote.GetVinMasternode());
        if(pmn == NULL) {
            LogPrint("gobject", "gobject - unknown masternode - vin: %s\n", vote.GetVinMasternode().ToString());
            mnodeman.AskForMN(pfrom, vote.GetVinMasternode());
            return;
        }

        // CHECK LOCAL VALIDITY AGAINST BLOCKCHAIN, TIME DATA, ETC

        if(!vote.IsValid(true)){
            LogPrintf("gobject - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.GetVinMasternode());
            mapSeenVotes.insert(std::make_pair(vote.GetHash(), SEEN_OBJECT_ERROR_INVALID));
            return;
        } else {
            mapSeenVotes.insert(std::make_pair(vote.GetHash(), SEEN_OBJECT_IS_VALID));
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
    vote_m_it it1 = mapOrphanVotes.begin();
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
    mapObjects.insert(std::make_pair(govobj.GetHash(), govobj));

    // SHOULD WE ADD THIS OBJECT TO ANY OTHER MANANGERS?

    DBG( cout << "CGovernanceManager::AddGovernanceObject Before trigger block, strData = "
              << govobj.GetDataAsString()
              << ", nObjectType = " << govobj.nObjectType
              << endl; );

    if(govobj.nObjectType == GOVERNANCE_OBJECT_TRIGGER) {
        mapLastMasternodeTrigger[govobj.vinMasternode.prevout] = nCachedBlockHeight;
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

    object_m_it it = mapObjects.begin();

    count_m_t mapDirtyObjects;

    // Clean up any expired or invalid triggers
    triggerman.CleanAndRemove();

    while(it != mapObjects.end())
    {
        CGovernanceObject* pObj = &((*it).second);

        if(!pObj) {
            ++it;
            continue;
        }

        // IF CACHE IS NOT DIRTY, WHY DO THIS?
        if(pObj->fDirtyCache) {
            mapDirtyObjects.insert(std::make_pair((*it).first, 1));

            // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
            pObj->UpdateLocalValidity(pCurrentBlockIndex);

            // UPDATE SENTINEL SIGNALING VARIABLES
            pObj->UpdateSentinelVariables(pCurrentBlockIndex);
        }

        // IF DELETE=TRUE, THEN CLEAN THE MESS UP!

        if(pObj->fCachedDelete || pObj->fExpired) {
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

    object_m_it it = mapObjects.begin();
    while(it != mapObjects.end()) {
        int nGovObjYesCount = pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING);
        if((*it).second.strName == strName && nGovObjYesCount > nYesCount) {
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

    vote_m_it it2 = mapVotesByHash.begin();
    while(it2 != mapVotesByHash.end()) {
        if((*it2).second.GetParentHash() == nParentHash) {
            vecResult.push_back(&(*it2).second);
        }
        ++it2;
    }

    return vecResult;
}

std::vector<CGovernanceObject*> CGovernanceManager::GetAllNewerThan(int64_t nMoreThanTime)
{
    LOCK(cs);

    std::vector<CGovernanceObject*> vGovObjs;

    object_m_it it = mapObjects.begin();
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
        if (left.second != right.second)
            return (left.second > right.second);
        return (UintToArith256(left.first->nCollateralHash) > UintToArith256(right.first->nCollateralHash));
    }
};

void CGovernanceManager::NewBlock()
{
    // IF WE'RE NOT SYNCED, EXIT
    if(!masternodeSync.IsSynced()) return;

    TRY_LOCK(cs, fBudgetNewBlock);
    if(!fBudgetNewBlock || !pCurrentBlockIndex) return;

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
       object_m_it it1 = mapObjects.begin();
       while(it1 != mapObjects.end()) {
          uint256 h = (*it1).first;

           if((*it1).second.fCachedValid && (nProp == uint256() || h == nProp)) {
              // Push the inventory budget proposal message over to the other client
              pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT, h));
              nInvCount++;
           }
           ++it1;
       }

       // SYNC OUR GOVERNANCE OBJECT VOTES WITH THEIR GOVERNANCE OBJECT VOTES

       vote_m_it it2 = mapVotesByHash.begin();
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
    if(!mapAskedForGovernanceObject.count(vote.GetParentHash())){
        pfrom->PushMessage(NetMsgType::MNGOVERNANCESYNC, vote.GetParentHash());
        mapAskedForGovernanceObject[vote.GetParentHash()] = GetTime();
    }
}

bool CGovernanceManager::AddOrUpdateVote(const CGovernanceVote& vote, CNode* pfrom, std::string& strError)
{
    // MAKE SURE WE HAVE THE PARENT OBJECT THE VOTE IS FOR

    bool syncparent = false;
    uint256 votehash;
    {
        LOCK(cs);
        if(!mapObjects.count(vote.GetParentHash()))  {
                if(pfrom)  {
                    // only ask for missing items after our syncing process is complete --
                    //   otherwise we'll think a full sync succeeded when they return a result
                    if(!masternodeSync.IsSynced()) return false;

                    // ADD THE VOTE AS AN ORPHAN, TO BE USED UPON RECEIVAL OF THE PARENT OBJECT

                    LogPrintf("CGovernanceManager::AddOrUpdateVote - Unknown object %d, asking for source\n", vote.GetParentHash().ToString());
                    mapOrphanVotes[vote.GetParentHash()] = vote;

                    // ASK FOR THIS VOTES PARENT SPECIFICALLY FROM THIS USER (THEY SHOULD HAVE IT, NO?)

                    if(!mapAskedForGovernanceObject.count(vote.GetParentHash())){
                        syncparent = true;
                        votehash = vote.GetParentHash();
                        mapAskedForGovernanceObject[vote.GetParentHash()] = GetTime();
                    }
                }

                strError = "Governance object not found!";
                return false;
        }
    }

    // Need to keep this out of the locked section
    if(syncparent) {
        pfrom->PushMessage(NetMsgType::MNGOVERNANCESYNC, votehash);
    }

    // Reestablish lock
    LOCK(cs);
    // GET DETERMINISTIC HASH WHICH COLLIDES ON MASTERNODE-VIN/GOVOBJ-HASH/VOTE-SIGNAL

    uint256 nTypeHash = vote.GetTypeHash();
    uint256 nHash = vote.GetHash();

    // LOOK FOR PREVIOUS VOTES BY THIS SPECIFIC MASTERNODE FOR THIS SPECIFIC SIGNAL

    vote_m_it it = mapVotesByType.find(nTypeHash);
    if(it != mapVotesByType.end()) {
        if(it->second.GetTimestamp() > vote.GetTimestamp()) {
            strError = strprintf("new vote older than existing vote - %s", nTypeHash.ToString());
            LogPrint("gobject", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.GetTimestamp() - it->second.GetTimestamp() < GOVERNANCE_UPDATE_MIN) {
            strError = strprintf("time between votes is too soon - %s - %lli", nTypeHash.ToString(), vote.GetTimestamp() - it->second.GetTimestamp());
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

bool CGovernanceManager::MasternodeRateCheck(const CTxIn& vin)
{
    LOCK(cs);
    txout_m_it it  = mapLastMasternodeTrigger.find(vin.prevout);
    if(it == mapLastMasternodeTrigger.end()) {
        return true;
    }
    // Allow 1 trigger per mn per cycle, with a small fudge factor
    int mindiff = Params().GetConsensus().nSuperblockCycle - Params().GetConsensus().nSuperblockCycle / 10;
    if((nCachedBlockHeight - it->second) > mindiff) {
        return true;
    }

    LogPrintf("CGovernanceManager::MasternodeRateCheck Rate too high: vin = %s, current height = %d, last MN height = %d, minimum difference = %d\n", 
              vin.prevout.ToStringShort(), nCachedBlockHeight, it->second, mindiff);
    return false;
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

    vinMasternode = other.vinMasternode;
    vchSig = other.vchSig;

    // caching
    fCachedFunding = other.fCachedFunding;
    fCachedValid = other.fCachedValid;
    fCachedDelete = other.fCachedDelete;
    fCachedEndorsed = other.fCachedEndorsed;
    fDirtyCache = other.fDirtyCache;
    fExpired = other.fExpired;
}

void CGovernanceObject::SetMasternodeInfo(const CTxIn& vin)
{
    vinMasternode = vin;
}

bool CGovernanceObject::Sign(CKey& keyMasternode, CPubKey& pubkeyMasternode)
{
    LOCK(cs);

    std::string strError;
    uint256 nHash = GetHash();
    std::string strMessage = nHash.ToString();

    if(!darkSendSigner.SignMessage(strMessage, vchSig, keyMasternode)) {
        LogPrintf("CGovernanceObject::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CGovernanceObject::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    LogPrint("gobject", "CGovernanceObject::Sign: pubkey id = %s, vin = %s\n", 
             pubkeyMasternode.GetID().ToString(), vinMasternode.prevout.ToStringShort());


    return true;
}

bool CGovernanceObject::CheckSignature(CPubKey& pubkeyMasternode)
{
    LOCK(cs);
    std::string strError;
    uint256 nHash = GetHash();
    std::string strMessage = nHash.ToString();

    if(!darkSendSigner.VerifyMessage(pubkeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CGovernance::CheckSignature -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
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
    if(strData.empty()) {
        return obj;
    }

    UniValue objResult(UniValue::VOBJ);
    GetData(objResult);

    std::vector<UniValue> arr1 = objResult.getValues();
    std::vector<UniValue> arr2 = arr1.at( 0 ).getValues();
    obj = arr2.at( 1 );

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

    if(strData.empty()) {
        return;
    }

    try  {
        // ATTEMPT TO LOAD JSON STRING FROM STRDATA
        UniValue objResult(UniValue::VOBJ);
        GetData(objResult);

        DBG( cout << "CGovernanceObject::LoadData strData = "
             << GetDataAsString()
             << endl; );

        UniValue obj = GetJSONObject();
        nObjectType = obj["type"].get_int();
    }
    catch(std::exception& e) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::LoadData Error parsing JSON"
             << ", e.what() = " << e.what();
        DBG( cout << ostr.str() << endl; );
        LogPrintf( ostr.str().c_str() );
        return;
    }
    catch(...) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::LoadData Unknown Error parsing JSON";
        DBG( cout << ostr.str() << endl; );
        LogPrintf( ostr.str().c_str() );
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

void CGovernanceObject::GetData(UniValue& objResult)
{
    UniValue o(UniValue::VOBJ);
    std::string s = GetDataAsString();
    o.read(s);
    objResult = o;
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
    std::vector<unsigned char> v = ParseHex(strData);
    std::string s(v.begin(), v.end());

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

    if(GetAbsoluteNoCount(VOTE_SIGNAL_VALID) > mnodeman.CountEnabled(MSG_GOVERNANCE_PEER_PROTO_VERSION)/10) {
        strError = "Automated removal";
        return false;
    }

    // CHECK COLLATERAL IF REQUIRED (HIGH CPU USAGE)

    if(fCheckCollateral) {
        if(nObjectType == GOVERNANCE_OBJECT_TRIGGER) {
            std::string strVin = vinMasternode.prevout.ToStringShort();
            CMasternode mn;
            if(!mnodeman.Get(vinMasternode, mn)) {
                strError = "Masternode not found vin: " + strVin;
                return false;
            }
            if(!mn.IsEnabled()) {
                strError = "Masternode not enabled vin: " + strVin;
                return false;
            }

            // Check that we have a valid MN signature
            if(!CheckSignature(mn.pubKeyMasternode)) {
                strError = "Invalid masternode signature for: " + strVin + ", pubkey id = " + mn.pubKeyMasternode.GetID().ToString();
                return false;
            }

            // Only perform rate check if we are synced because during syncing it is expected
            // that objects will be seen in rapid succession
            if(masternodeSync.IsSynced()) {
                if(!governance.MasternodeRateCheck(vinMasternode)) {
                    strError = "Masternode attempting to create too many objects vin: " + strVin;
                    return false;
                }
            }

            return true;
        }

        if(!IsCollateralValid(strError)) {
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

CAmount CGovernanceObject::GetMinCollateralFee()
{
    CAmount nMinFee = 0;
    // Only 1 type has a fee for the moment but switch statement allows for future object types
    switch(nObjectType) {
    case GOVERNANCE_OBJECT_PROPOSAL:
        nMinFee = GOVERNANCE_PROPOSAL_FEE_TX;
        break;
    case GOVERNANCE_OBJECT_TRIGGER:
        nMinFee = 0;
        break;
    default:
      {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::GetMinCollateralFee ERROR: Unknown governance object type: " << nObjectType;
        throw std::runtime_error(ostr.str());
      }
    }
    return nMinFee;
}

bool CGovernanceObject::IsCollateralValid(std::string& strError)
{
    CAmount nMinFee = 0;
    try  {
        nMinFee = GetMinCollateralFee();
    }
    catch(std::exception& e) {
        strError = e.what();
        LogPrintf("CGovernanceObject::IsCollateralValid ERROR An exception occurred - %s\n", e.what());
        return false;
    }

    uint256 nExpectedHash = GetHash();

    CTransaction txCollateral;
    uint256 nBlockHash;

    // RETRIEVE TRANSACTION IN QUESTION

    if(!GetTransaction(nCollateralHash, txCollateral, Params().GetConsensus(), nBlockHash, true)){
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrintf("CGovernanceObject::IsCollateralValid - %s\n", strError);
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

    DBG( cout << "IsCollateralValid: nMinFee = " << nMinFee << endl; );


    bool foundOpReturn = false;
    BOOST_FOREACH(const CTxOut o, txCollateral.vout) {
        DBG( cout << "IsCollateralValid txout : " << o.ToString()
             << ", o.nValue = " << o.nValue
             << ", o.scriptPubKey = " << ScriptToAsmStr( o.scriptPubKey, false )
             << endl; );
        if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()){
            strError = strprintf("Invalid Script %s", txCollateral.ToString());
            LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
            return false;
        }
        if(o.scriptPubKey == findScript && o.nValue >= nMinFee) {
            DBG( cout << "IsCollateralValid foundOpReturn = true" << endl; );
            foundOpReturn = true;
        }
        else  {
            DBG( cout << "IsCollateralValid No match, continuing" << endl; );
        }

    }

    if(!foundOpReturn){
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral.ToString());
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
        return false;
    }

    // GET CONFIRMATIONS FOR TRANSACTION

    LOCK(cs_main);
    int nConfirmationsIn = GetIXConfirmations(nCollateralHash);
    if (nBlockHash != uint256()) {
        BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                nConfirmationsIn += chainActive.Height() - pindex->nHeight + 1;
            }
        }
    }

    if(nConfirmationsIn >= GOVERNANCE_FEE_CONFIRMATIONS) {
        strError = "valid";
    } else {
        strError = strprintf("Collateral requires at least %d confirmations - %d confirmations", GOVERNANCE_FEE_CONFIRMATIONS, nConfirmationsIn);
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s - %d confirmations\n", strError, nConfirmationsIn);
        return false;
    }

    return true;
}

/**
*   Get specific vote counts for each outcome (funding, validity, etc)
*/

int CGovernanceObject::GetAbsoluteYesCount(vote_signal_enum_t eVoteSignalIn)
{
    return GetYesCount(eVoteSignalIn) - GetNoCount(eVoteSignalIn);
}

int CGovernanceObject::GetAbsoluteNoCount(vote_signal_enum_t eVoteSignalIn)
{
    return GetNoCount(eVoteSignalIn) - GetYesCount(eVoteSignalIn);
}

int CGovernanceObject::GetYesCount(vote_signal_enum_t eVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), eVoteSignalIn, VOTE_OUTCOME_YES);
}

int CGovernanceObject::GetNoCount(vote_signal_enum_t eVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), eVoteSignalIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetAbstainCount(vote_signal_enum_t eVoteSignalIn)
{
    return governance.CountMatchingVotes((*this), eVoteSignalIn, VOTE_OUTCOME_ABSTAIN);
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

    if(!fLiteMode && masternodeSync.IsSynced())
        NewBlock();
}

int CGovernanceManager::CountMatchingVotes(CGovernanceObject& govobj, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn)
{
    /*
    *
    *   Count matching votes and return
    *
    */

    LOCK(cs);
    int nCount = 0;

    std::map<uint256, CGovernanceVote>::iterator it = mapVotesByHash.begin();
    while(it != mapVotesByHash.end())  {
        if(it->second.IsValid() &&
           it->second.GetSignal() == eVoteSignalIn &&
           it->second.GetOutcome() == eVoteOutcomeIn &&
           it->second.GetParentHash() == govobj.GetHash()) {
            ++nCount;
        }
        ++it;
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
    int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nMnCount / 10);
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
