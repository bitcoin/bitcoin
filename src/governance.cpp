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
#include "netfulfilledman.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <univalue.h>

CGovernanceManager governance;

std::map<uint256, int64_t> mapAskedForGovernanceObject;

int nSubmittedFinalBudget;

const std::string CGovernanceManager::SERIALIZATION_VERSION_STRING = "CGovernanceManager-Version-1";

CGovernanceManager::CGovernanceManager()
    : pCurrentBlockIndex(NULL),
      nTimeLastDiff(0),
      nCachedBlockHeight(0),
      mapObjects(),
      mapSeenGovernanceObjects(),
      mapVoteToObject(MAX_CACHE_SIZE),
      mapInvalidVotes(MAX_CACHE_SIZE),
      mapOrphanVotes(MAX_CACHE_SIZE),
      mapLastMasternodeTrigger(),
      setRequestedObjects(),
      cs()
{}

// Accessors for thread-safe access to maps
bool CGovernanceManager::HaveObjectForHash(uint256 nHash) {
    LOCK(cs);
    return (mapObjects.count(nHash) == 1);
}

bool CGovernanceManager::SerializeObjectForHash(uint256 nHash, CDataStream& ss)
{
    LOCK(cs);
    object_m_it it = mapObjects.find(nHash);
    if (it == mapObjects.end()) {
        return false;
    }
    ss << it->second;
    return true;
}

bool CGovernanceManager::HaveVoteForHash(uint256 nHash)
{
    LOCK(cs);

    CGovernanceObject* pGovobj = NULL;
    if(!mapVoteToObject.Get(nHash,pGovobj)) {
        return false;
    }

    if(!pGovobj->GetVoteFile().HasVote(nHash)) {
        return false;
    }
    return true;
}

bool CGovernanceManager::SerializeVoteForHash(uint256 nHash, CDataStream& ss)
{
    LOCK(cs);

    CGovernanceObject* pGovobj = NULL;
    if(!mapVoteToObject.Get(nHash,pGovobj)) {
        return false;
    }

    CGovernanceVote vote;
    if(!pGovobj->GetVoteFile().GetVote(nHash, vote)) {
        return false;
    }

    ss << vote;
    return true;
}

void CGovernanceManager::AddSeenGovernanceObject(uint256 nHash, int status)
{
    LOCK(cs);
    mapSeenGovernanceObjects[nHash] = status;
}

void CGovernanceManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    if(pfrom->nVersion < MIN_GOVERNANCE_PEER_PROTO_VERSION) return;

    LOCK(cs);

    // ANOTHER USER IS ASKING US TO HELP THEM SYNC GOVERNANCE OBJECT DATA
    if (strCommand == NetMsgType::MNGOVERNANCESYNC)
    {

        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!masternodeSync.IsSynced()) return;

        uint256 nProp;
        vRecv >> nProp;

        if(nProp == uint256()) {
            if(netfulfilledman.HasFulfilledRequest(pfrom->addr, NetMsgType::MNGOVERNANCESYNC)) {
                // Asking for the whole list multiple times in a short period of time is no good
                LogPrint("gobject", "MNGOVERNANCESYNC -- peer already asked me for the list\n");
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
            netfulfilledman.AddFulfilledRequest(pfrom->addr, NetMsgType::MNGOVERNANCESYNC);
        }

        Sync(pfrom, nProp);
        LogPrint("gobject", "MNGOVERNANCESYNC -- syncing governance objects to our peer at %s\n", pfrom->addr.ToString());

    }

    // A NEW GOVERNANCE OBJECT HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECT)

    {
        // MAKE SURE WE HAVE A VALID REFERENCE TO THE TIP BEFORE CONTINUING

        if(!pCurrentBlockIndex) {
            LogPrintf("CGovernanceManager::ProcessMessage MNGOVERNANCEOBJECT -- pCurrentBlockIndex is NULL\n");
            return;
        }

        CGovernanceObject govobj;
        vRecv >> govobj;

        uint256 nHash = govobj.GetHash();
        std::string strHash = nHash.ToString();

        LogPrint("gobject", "CGovernanceManager -- Received object: %s\n", strHash);

        if(!AcceptObjectMessage(nHash)) {
            LogPrintf("CGovernanceManager -- Received unrequested object: %s\n", strHash);
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        if(mapSeenGovernanceObjects.count(nHash)) {
            // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
            LogPrint("gobject", "CGovernanceManager -- Received already seen object: %s\n", strHash);
            return;
        }

        std::string strError = "";
        // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN

        if(!govobj.IsValidLocally(pCurrentBlockIndex, strError, true)) {
            mapSeenGovernanceObjects.insert(std::make_pair(nHash, SEEN_OBJECT_ERROR_INVALID));
            LogPrintf("MNGOVERNANCEOBJECT -- Governance object is invalid - %s\n", strError);
            return;
        }

        // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANANGED DATA

        govobj.UpdateSentinelVariables(pCurrentBlockIndex); //this sets local vars in object

        if(AddGovernanceObject(govobj))
        {
            LogPrintf("MNGOVERNANCEOBJECT -- %s new\n", strHash);
            govobj.Relay();
        }

        // UPDATE THAT WE'VE SEEN THIS OBJECT
        mapSeenGovernanceObjects.insert(std::make_pair(govobj.GetHash(), SEEN_OBJECT_IS_VALID));
        masternodeSync.AddedBudgetItem(govobj.GetHash());


        // WE MIGHT HAVE PENDING/ORPHAN VOTES FOR THIS OBJECT

        CGovernanceException exception;
        CheckOrphanVotes(pfrom, govobj, exception);
    }

    // A NEW GOVERNANCE OBJECT VOTE HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECTVOTE)
    {
        // Ignore such messages until masternode list is synced
        if(!masternodeSync.IsMasternodeListSynced()) {
            LogPrint("gobject", "CGovernanceManager::ProcessMessage MNGOVERNANCEOBJECTVOTE -- masternode list not synced\n");
            return;
        }

        CGovernanceVote vote;
        vRecv >> vote;

        LogPrint("gobject", "CGovernanceManager -- Received vote: %s\n", vote.ToString());

        if(!AcceptVoteMessage(vote.GetHash())) {
            LogPrintf("CGovernanceManager -- Received unrequested vote object: %s, hash: %s, peer = %d\n",
                      vote.ToString(),
                      vote.GetHash().ToString(),
                      pfrom->GetId());
            //Misbehaving(pfrom->GetId(), 20);
            return;
        }

        CGovernanceException exception;
        if(ProcessVote(pfrom, vote, exception)) {
            LogPrint("gobject", "CGovernanceManager -- Accepted vote\n");
        }
        else {
            LogPrint("gobject", "CGovernanceManager -- Rejected vote, error = %s\n", exception.what());
            if((exception.GetNodePenalty() != 0) && masternodeSync.IsSynced()) {
                Misbehaving(pfrom->GetId(), exception.GetNodePenalty());
            }
            return;
        }

    }
}

void CGovernanceManager::CheckOrphanVotes(CNode* pfrom, CGovernanceObject& govobj, CGovernanceException& exception)
{
    uint256 nHash = govobj.GetHash();
    std::vector<CGovernanceVote> vecVotes;
    mapOrphanVotes.GetAll(nHash, vecVotes);

    for(size_t i = 0; i < vecVotes.size(); ++i) {
        CGovernanceVote& vote = vecVotes[i];
        CGovernanceException exception;
        if(govobj.ProcessVote(pfrom, vote, exception)) {
            vecVotes[i].Relay();
            mapOrphanVotes.Erase(nHash, vote);
        }
        else {
            if((exception.GetNodePenalty() != 0) && masternodeSync.IsSynced()) {
                Misbehaving(pfrom->GetId(), exception.GetNodePenalty());
            }
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
        LogPrintf("CGovernanceManager::AddGovernanceObject -- invalid governance object - %s - (pCurrentBlockIndex nHeight %d) \n", strError, pCurrentBlockIndex->nHeight);
        return false;
    }

    // IF WE HAVE THIS OBJECT ALREADY, WE DON'T WANT ANOTHER COPY

    if(mapObjects.count(govobj.GetHash())) {
        LogPrintf("CGovernanceManager::AddGovernanceObject -- already have governance object - %s\n", strError);
        return false;
    }

    // INSERT INTO OUR GOVERNANCE OBJECT MEMORY
    mapObjects.insert(std::make_pair(govobj.GetHash(), govobj));

    // SHOULD WE ADD THIS OBJECT TO ANY OTHER MANANGERS?

    DBG( cout << "CGovernanceManager::AddGovernanceObject Before trigger block, strData = "
              << govobj.GetDataAsString()
              << ", nObjectType = " << govobj.nObjectType
              << endl; );

    if(govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) {
        mapLastMasternodeTrigger[govobj.GetMasternodeVin().prevout] = nCachedBlockHeight;
    }

    switch(govobj.nObjectType) {
    case GOVERNANCE_OBJECT_TRIGGER:
        mapLastMasternodeTrigger[govobj.vinMasternode.prevout] = nCachedBlockHeight;
        DBG( cout << "CGovernanceManager::AddGovernanceObject Before AddNewTrigger" << endl; );
        triggerman.AddNewTrigger(govobj.GetHash());
        DBG( cout << "CGovernanceManager::AddGovernanceObject After AddNewTrigger" << endl; );
        break;
    default:
        break;
    }

    DBG( cout << "CGovernanceManager::AddGovernanceObject END" << endl; );

    return true;
}

void CGovernanceManager::UpdateCachesAndClean()
{
    LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean\n");

    std::vector<uint256> vecDirtyHashes = mnodeman.GetAndClearDirtyGovernanceObjectHashes();

    LOCK(cs);

    for(size_t i = 0; i < vecDirtyHashes.size(); ++i) {
        object_m_it it = mapObjects.find(vecDirtyHashes[i]);
        if(it == mapObjects.end()) {
            continue;
        }
        it->second.ClearMasternodeVotes();
        it->second.fDirtyCache = true;
    }

    // DOUBLE CHECK THAT WE HAVE A VALID POINTER TO TIP

    if(!pCurrentBlockIndex) return;

    LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean -- After pCurrentBlockIndex (not NULL)\n");

    // UPDATE CACHE FOR EACH OBJECT THAT IS FLAGGED DIRTYCACHE=TRUE

    object_m_it it = mapObjects.begin();

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
        if(pObj->IsSetDirtyCache()) {
            // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
            pObj->UpdateLocalValidity(pCurrentBlockIndex);

            // UPDATE SENTINEL SIGNALING VARIABLES
            pObj->UpdateSentinelVariables(pCurrentBlockIndex);
        }

        // IF DELETE=TRUE, THEN CLEAN THE MESS UP!

        if(pObj->IsSetCachedDelete() || pObj->IsSetExpired()) {
            LogPrintf("CGovernanceManager::UpdateCachesAndClean -- erase obj %s\n", (*it).first.ToString());
            mnodeman.RemoveGovernanceObject(pObj->GetHash());

            // Remove vote references
            const object_ref_cache_t::list_t& listItems = mapVoteToObject.GetItemList();
            object_ref_cache_t::list_cit lit = listItems.begin();
            while(lit != listItems.end()) {
                if(lit->value == pObj) {
                    uint256 nKey = lit->key;
                    ++lit;
                    mapVoteToObject.Erase(nKey);
                }
                else {
                    ++lit;
                }
            }

            mapObjects.erase(it++);
        } else {
            ++it;
        }
    }
}

CGovernanceObject *CGovernanceManager::FindGovernanceObject(const uint256& nHash)
{
    LOCK(cs);

    if(mapObjects.count(nHash))
        return &mapObjects[nHash];

    return NULL;
}

std::vector<CGovernanceVote> CGovernanceManager::GetMatchingVotes(const uint256& nParentHash)
{
    LOCK(cs);
    std::vector<CGovernanceVote> vecResult;

    object_m_it it = mapObjects.find(nParentHash);
    if(it == mapObjects.end()) {
        return vecResult;
    }
    CGovernanceObject& govobj = it->second;

    return govobj.GetVoteFile().GetVotes();
}

std::vector<CGovernanceObject*> CGovernanceManager::GetAllNewerThan(int64_t nMoreThanTime)
{
    LOCK(cs);

    std::vector<CGovernanceObject*> vGovObjs;

    object_m_it it = mapObjects.begin();
    while(it != mapObjects.end())
    {
        // IF THIS OBJECT IS OLDER THAN TIME, CONTINUE

        if((*it).second.GetCreationTime() < nMoreThanTime) {
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
        return (UintToArith256(left.first->GetCollateralHash()) > UintToArith256(right.first->GetCollateralHash()));
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
    while(it != mapAskedForGovernanceObject.end()) {
        if((*it).second > GetTime() - (60*60*24)) {
            ++it;
        } else {
            mapAskedForGovernanceObject.erase(it++);
        }
    }

    // CHECK AND REMOVE - REPROCESS GOVERNANCE OBJECTS

    UpdateCachesAndClean();
}

bool CGovernanceManager::ConfirmInventoryRequest(const CInv& inv)
{
    LOCK(cs);

    LogPrint("gobject", "CGovernanceManager::ConfirmInventoryRequest inv = %s\n", inv.ToString());

    // First check if we've already recorded this object
    switch(inv.type) {
    case MSG_GOVERNANCE_OBJECT:
    {
        object_m_it it = mapObjects.find(inv.hash);
        if(it != mapObjects.end()) {
            LogPrint("gobject", "CGovernanceManager::ConfirmInventoryRequest already have governance object, returning false\n");
            return false;
        }
    }
    break;
    case MSG_GOVERNANCE_OBJECT_VOTE:
    {
        if(mapVoteToObject.HasKey(inv.hash)) {
            LogPrint("gobject", "CGovernanceManager::ConfirmInventoryRequest already have governance vote, returning false\n");
            return false;
        }
    }
    break;
    default:
        LogPrint("gobject", "CGovernanceManager::ConfirmInventoryRequest unknown type, returning false\n");
        return false;
    }


    hash_s_t* setHash = NULL;
    switch(inv.type) {
    case MSG_GOVERNANCE_OBJECT:
        setHash = &setRequestedObjects;
        break;
    case MSG_GOVERNANCE_OBJECT_VOTE:
        setHash = &setRequestedVotes;
        break;
    default:
        return false;
    }

    hash_s_cit it = setHash->find(inv.hash);
    if(it == setHash->end()) {
        setHash->insert(inv.hash);
        LogPrint("gobject", "CGovernanceManager::ConfirmInventoryRequest added inv to requested set\n");
    }

    LogPrint("gobject", "CGovernanceManager::ConfirmInventoryRequest reached end, returning true\n");
    return true;
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
        for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
            uint256 h = it->first;

            CGovernanceObject& govobj = it->second;

            if(govobj.IsSetCachedValid() && (nProp == uint256() || h == nProp)) {
                // Push the inventory budget proposal message over to the other client
                pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT, h));
                ++nInvCount;

                std::vector<CGovernanceVote> vecVotes = govobj.GetVoteFile().GetVotes();
                for(size_t i = 0; i < vecVotes.size(); ++i) {
                    if(!vecVotes[i].IsValid(true)) {
                        continue;
                    }
                    pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT_VOTE, vecVotes[i].GetHash()));
                    ++nInvCount;
                }
            }
        }
    }

    pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ, nInvCount);
    LogPrintf("CGovernanceManager::Sync -- sent %d items\n", nInvCount);
}

void CGovernanceManager::SyncParentObjectByVote(CNode* pfrom, const CGovernanceVote& vote)
{
    if(!mapAskedForGovernanceObject.count(vote.GetParentHash())){
        pfrom->PushMessage(NetMsgType::MNGOVERNANCESYNC, vote.GetParentHash());
        mapAskedForGovernanceObject[vote.GetParentHash()] = GetTime();
    }
}

bool CGovernanceManager::MasternodeRateCheck(const CTxIn& vin, int nObjectType)
{
    LOCK(cs);

    int mindiff = 0;
    switch(nObjectType) {
    case GOVERNANCE_OBJECT_TRIGGER:
        mindiff = Params().GetConsensus().nSuperblockCycle - Params().GetConsensus().nSuperblockCycle / 10;
        break;
    case GOVERNANCE_OBJECT_WATCHDOG:
        mindiff = 1;
        break;
    default:
        break;
    }

    txout_m_it it  = mapLastMasternodeTrigger.find(vin.prevout);
    if(it == mapLastMasternodeTrigger.end()) {
        return true;
    }
    // Allow 1 trigger per mn per cycle, with a small fudge factor
    if((nCachedBlockHeight - it->second) > mindiff) {
        return true;
    }

    LogPrintf("CGovernanceManager::MasternodeRateCheck -- Rate too high: vin = %s, current height = %d, last MN height = %d, minimum difference = %d\n",
              vin.prevout.ToStringShort(), nCachedBlockHeight, it->second, mindiff);
    return false;
}

bool CGovernanceManager::ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception)
{
    LOCK(cs);
    uint256 nHashVote = vote.GetHash();
    if(mapInvalidVotes.HasKey(nHashVote)) {
        std::ostringstream ostr;
        ostr << "CGovernanceManager::ProcessVote -- Old invalid vote "
                << ", MN outpoint = " << vote.GetVinMasternode().prevout.ToStringShort()
                << ", governance object hash = " << vote.GetParentHash().ToString() << "\n";
        LogPrintf(ostr.str().c_str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        return false;
    }

    uint256 nHashGovobj = vote.GetParentHash();
    object_m_it it = mapObjects.find(nHashGovobj);
    if(it == mapObjects.end()) {
        std::ostringstream ostr;
        ostr << "CGovernanceManager::ProcessVote -- Unknown parent object "
             << ", MN outpoint = " << vote.GetVinMasternode().prevout.ToStringShort()
             << ", governance object hash = " << vote.GetParentHash().ToString() << "\n";
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_WARNING);
        if(mapOrphanVotes.Insert(nHashGovobj, vote)) {
            RequestGovernanceObject(pfrom, nHashGovobj);
            LogPrintf(ostr.str().c_str());
        }
        else {
            LogPrint("gobject", ostr.str().c_str());
        }
        return false;
    }

    CGovernanceObject& govobj = it->second;
    bool fOk = govobj.ProcessVote(pfrom, vote, exception);
    if(fOk) {
        mapVoteToObject.Insert(vote.GetHash(), &govobj);

        if(govobj.GetObjectType() == GOVERNANCE_OBJECT_WATCHDOG) {
            mnodeman.UpdateWatchdogVoteTime(vote.GetVinMasternode());
        }

        vote.Relay();
    }
    return fOk;
}

void CGovernanceManager::CheckMasternodeOrphanVotes()
{
    LOCK(cs);
    for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
        it->second.CheckOrphanVotes();
    }
}

void CGovernanceManager::RequestGovernanceObject(CNode* pfrom, const uint256& nHash)
{
    if(!pfrom) {
        return;
    }

    pfrom->PushMessage(NetMsgType::MNGOVERNANCESYNC, nHash);
}

bool CGovernanceManager::AcceptObjectMessage(const uint256& nHash)
{
    LOCK(cs);
    return AcceptMessage(nHash, setRequestedObjects);
}

bool CGovernanceManager::AcceptVoteMessage(const uint256& nHash)
{
    LOCK(cs);
    return AcceptMessage(nHash, setRequestedVotes);
}

bool CGovernanceManager::AcceptMessage(const uint256& nHash, hash_s_t& setHash)
{
    hash_s_it it = setHash.find(nHash);
    if(it == setHash.end()) {
        // We never requested this
        return false;
    }
    // Only accept one response
    setHash.erase(it);
    return true;
}

void CGovernanceManager::RebuildIndexes()
{
    mapVoteToObject.Clear();
    for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
        CGovernanceObject& govobj = it->second;
        std::vector<CGovernanceVote> vecVotes = govobj.GetVoteFile().GetVotes();
        for(size_t i = 0; i < vecVotes.size(); ++i) {
            mapVoteToObject.Insert(vecVotes[i].GetHash(), &govobj);
        }
    }
}

int CGovernanceManager::GetMasternodeIndex(const CTxIn& masternodeVin)
{
    LOCK(cs);
    bool fIndexRebuilt = false;
    int nMNIndex = mnodeman.GetMasternodeIndex(masternodeVin, fIndexRebuilt);
    while(fIndexRebuilt) {
        RebuildVoteMaps();
        nMNIndex = mnodeman.GetMasternodeIndex(masternodeVin, fIndexRebuilt);
    }
    return nMNIndex;
}

void CGovernanceManager::RebuildVoteMaps()
{
    for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
        it->second.RebuildVoteMap();
    }
}

void CGovernanceManager::AddCachedTriggers()
{
    LOCK(cs);

    for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
        CGovernanceObject& govobj = it->second;
        
        if(govobj.nObjectType != GOVERNANCE_OBJECT_TRIGGER) {
            continue;
        }

        triggerman.AddNewTrigger(govobj.GetHash());
    }    
}

CGovernanceObject::CGovernanceObject()
: cs(),
  nObjectType(GOVERNANCE_OBJECT_UNKNOWN),
  nHashParent(),
  nRevision(0),
  nTime(0),
  nCollateralHash(),
  strData(),
  vinMasternode(),
  vchSig(),
  fCachedLocalValidity(false),
  strLocalValidityError(),
  fCachedFunding(false),
  fCachedValid(true),
  fCachedDelete(false),
  fCachedEndorsed(false),
  fDirtyCache(true),
  fExpired(false),
  fUnparsable(false),
  mapCurrentMNVotes(),
  mapOrphanVotes(),
  fileVotes()
{
    // PARSE JSON DATA STORAGE (STRDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, int64_t nTimeIn, uint256 nCollateralHashIn, std::string strDataIn)
: cs(),
  nObjectType(GOVERNANCE_OBJECT_UNKNOWN),
  nHashParent(nHashParentIn),
  nRevision(nRevisionIn),
  nTime(nTimeIn),
  nCollateralHash(nCollateralHashIn),
  strData(strDataIn),
  vinMasternode(),
  vchSig(),
  fCachedLocalValidity(false),
  strLocalValidityError(),
  fCachedFunding(false),
  fCachedValid(true),
  fCachedDelete(false),
  fCachedEndorsed(false),
  fDirtyCache(true),
  fExpired(false),
  fUnparsable(false),
  mapCurrentMNVotes(),
  mapOrphanVotes(),
  fileVotes()
{
    // PARSE JSON DATA STORAGE (STRDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(const CGovernanceObject& other)
: cs(),
  nObjectType(other.nObjectType),
  nHashParent(other.nHashParent),
  nRevision(other.nRevision),
  nTime(other.nTime),
  nCollateralHash(other.nCollateralHash),
  strData(other.strData),
  vinMasternode(other.vinMasternode),
  vchSig(other.vchSig),
  fCachedLocalValidity(other.fCachedLocalValidity),
  strLocalValidityError(other.strLocalValidityError),
  fCachedFunding(other.fCachedFunding),
  fCachedValid(other.fCachedValid),
  fCachedDelete(other.fCachedDelete),
  fCachedEndorsed(other.fCachedEndorsed),
  fDirtyCache(other.fDirtyCache),
  fExpired(other.fExpired),
  fUnparsable(other.fUnparsable),
  mapCurrentMNVotes(other.mapCurrentMNVotes),
  mapOrphanVotes(other.mapOrphanVotes),
  fileVotes(other.fileVotes)
{}

bool CGovernanceObject::ProcessVote(CNode* pfrom,
                                    const CGovernanceVote& vote,
                                    CGovernanceException& exception)
{
    int nMNIndex = governance.GetMasternodeIndex(vote.GetVinMasternode());
    if(nMNIndex < 0) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::UpdateVote -- Masternode index not found\n";
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_WARNING);
        if(mapOrphanVotes.Insert(vote.GetVinMasternode(), vote)) {
            if(pfrom) {
                mnodeman.AskForMN(pfrom, vote.GetVinMasternode());
            }
            LogPrintf(ostr.str().c_str());
        }
        else {
            LogPrint("gobject", ostr.str().c_str());
        }
        return false;
    }

    vote_m_it it = mapCurrentMNVotes.find(nMNIndex);
    if(it == mapCurrentMNVotes.end()) {
        it = mapCurrentMNVotes.insert(vote_m_t::value_type(nMNIndex,vote_rec_t())).first;
    }
    vote_rec_t& recVote = it->second;
    vote_signal_enum_t eSignal = vote.GetSignal();
    if(eSignal == VOTE_SIGNAL_NONE) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::UpdateVote -- Vote signal: none" << "\n";
        LogPrint("gobject", ostr.str().c_str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_WARNING);
        return false;
    }
    if(eSignal > MAX_SUPPORTED_VOTE_SIGNAL) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::UpdateVote -- Unsupported vote signal:" << CGovernanceVoting::ConvertSignalToString(vote.GetSignal()) << "\n";
        LogPrintf(ostr.str().c_str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        return false;
    }
    vote_instance_m_it it2 = recVote.mapInstances.find(int(eSignal));
    if(it2 == recVote.mapInstances.end()) {
        it2 = recVote.mapInstances.insert(vote_instance_m_t::value_type(int(eSignal), vote_instance_t())).first;
    }
    vote_instance_t& voteInstance = it2->second;
    int64_t nNow = GetTime();
    int64_t nTimeDelta = nNow - voteInstance.nTime;
    if(nTimeDelta < GOVERNANCE_UPDATE_MIN) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::UpdateVote -- Masternode voting too often "
                << ", MN outpoint = " << vote.GetVinMasternode().prevout.ToStringShort()
                << ", governance object hash = " << GetHash().ToString()
                << ", time delta = " << nTimeDelta << "\n";
        LogPrint("gobject", ostr.str().c_str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_TEMPORARY_ERROR);
        return false;
    }
    // Finally check that the vote is actually valid (done last because of cost of signature verification)
    if(!vote.IsValid(true)) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::UpdateVote -- Invalid vote "
                << ", MN outpoint = " << vote.GetVinMasternode().prevout.ToStringShort()
                << ", governance object hash = " << GetHash().ToString()
                << ", vote hash = " << vote.GetHash().ToString() << "\n";
        LogPrintf(ostr.str().c_str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        governance.AddInvalidVote(vote);
        return false;
    }
    voteInstance = vote_instance_t(vote.GetOutcome(), nNow);
    fileVotes.AddVote(vote);
    fDirtyCache = true;
    return true;
}

void CGovernanceObject::RebuildVoteMap()
{
    vote_m_t mapMNVotesNew;
    for(vote_m_it it = mapCurrentMNVotes.begin(); it != mapCurrentMNVotes.end(); ++it) {
        CTxIn vinMasternode;
        if(mnodeman.GetMasternodeVinForIndexOld(it->first, vinMasternode)) {
            int nNewIndex = mnodeman.GetMasternodeIndex(vinMasternode);
            if((nNewIndex >= 0)) {
                mapMNVotesNew[nNewIndex] = it->second;
            }
        }
    }
    mapCurrentMNVotes = mapMNVotesNew;
}

void CGovernanceObject::ClearMasternodeVotes()
{
    vote_m_it it = mapCurrentMNVotes.begin();
    while(it != mapCurrentMNVotes.end()) {
        bool fIndexRebuilt = false;
        CTxIn vinMasternode;
        bool fRemove = true;
        if(mnodeman.Get(it->first, vinMasternode, fIndexRebuilt)) {
            if(mnodeman.Has(vinMasternode)) {
                fRemove = false;
            }
            else {
                fileVotes.RemoveVotesFromMasternode(vinMasternode);
            }
        }

        if(fRemove) {
            mapCurrentMNVotes.erase(it++);
        }
        else {
            ++it;
        }
    }
}

void CGovernanceObject::SetMasternodeInfo(const CTxIn& vin)
{
    vinMasternode = vin;
}

bool CGovernanceObject::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    LOCK(cs);

    std::string strError;
    uint256 nHash = GetHash();
    std::string strMessage = nHash.ToString();

    if(!darkSendSigner.SignMessage(strMessage, vchSig, keyMasternode)) {
        LogPrintf("CGovernanceObject::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CGovernanceObject::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    LogPrint("gobject", "CGovernanceObject::Sign -- pubkey id = %s, vin = %s\n",
             pubKeyMasternode.GetID().ToString(), vinMasternode.prevout.ToStringShort());


    return true;
}

bool CGovernanceObject::CheckSignature(CPubKey& pubKeyMasternode)
{
    LOCK(cs);
    std::string strError;
    uint256 nHash = GetHash();
    std::string strMessage = nHash.ToString();

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CGovernance::CheckSignature -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
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
    ss << nTime;
    ss << strData;
    // fee_tx is left out on purpose
    uint256 h1 = ss.GetHash();

    DBG( printf("CGovernanceObject::GetHash %i %li %s\n", nRevision, nTime, strData.c_str()); );

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
        fUnparsable = true;
        std::ostringstream ostr;
        ostr << "CGovernanceObject::LoadData Error parsing JSON"
             << ", e.what() = " << e.what();
        DBG( cout << ostr.str() << endl; );
        LogPrintf( ostr.str().c_str() );
        return;
    }
    catch(...) {
        fUnparsable = true;
        std::ostringstream ostr;
        ostr << "CGovernanceObject::LoadData Unknown Error parsing JSON";
        DBG( cout << ostr.str() << endl; );
        LogPrintf( ostr.str().c_str() );
        return;
    }
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

    if(fUnparsable) {
        return false;
    }

    switch(nObjectType) {
        case GOVERNANCE_OBJECT_PROPOSAL:
        case GOVERNANCE_OBJECT_TRIGGER:
        case GOVERNANCE_OBJECT_WATCHDOG:
            break;
        default:
            strError = strprintf("Invalid object type %d", nObjectType);
            return false;
    }

    // IF ABSOLUTE NO COUNT (NO-YES VALID VOTES) IS MORE THAN 10% OF THE NETWORK MASTERNODES, OBJ IS INVALID

    if(GetAbsoluteNoCount(VOTE_SIGNAL_VALID) > mnodeman.CountEnabled(MIN_GOVERNANCE_PEER_PROTO_VERSION)/10) {
        strError = "Automated removal";
        return false;
    }

    // CHECK COLLATERAL IF REQUIRED (HIGH CPU USAGE)

    if(fCheckCollateral) { 
        if((nObjectType == GOVERNANCE_OBJECT_TRIGGER) || (nObjectType == GOVERNANCE_OBJECT_WATCHDOG)) {
            std::string strOutpoint = vinMasternode.prevout.ToStringShort();
            masternode_info_t infoMn = mnodeman.GetMasternodeInfo(vinMasternode);
            if(!infoMn.fInfoValid) {
                strError = "Masternode not found: " + strOutpoint;
                return false;
            }

            // Check that we have a valid MN signature
            if(!CheckSignature(infoMn.pubKeyMasternode)) {
                strError = "Invalid masternode signature for: " + strOutpoint + ", pubkey id = " + infoMn.pubKeyMasternode.GetID().ToString();
                return false;
            }

            // Only perform rate check if we are synced because during syncing it is expected
            // that objects will be seen in rapid succession
            if(masternodeSync.IsSynced()) {
                if(!governance.MasternodeRateCheck(vinMasternode, nObjectType)) {
                    strError = "Masternode attempting to create too many objects: " + strOutpoint;
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
    // Only 1 type has a fee for the moment but switch statement allows for future object types
    switch(nObjectType) {
        case GOVERNANCE_OBJECT_PROPOSAL:    return GOVERNANCE_PROPOSAL_FEE_TX;
        case GOVERNANCE_OBJECT_TRIGGER:     return 0;
        case GOVERNANCE_OBJECT_WATCHDOG:    return 0;
        default:                            return MAX_MONEY;
    }
}

bool CGovernanceObject::IsCollateralValid(std::string& strError)
{
    strError = "";
    CAmount nMinFee = GetMinCollateralFee();
    uint256 nExpectedHash = GetHash();

    CTransaction txCollateral;
    uint256 nBlockHash;

    // RETRIEVE TRANSACTION IN QUESTION

    if(!GetTransaction(nCollateralHash, txCollateral, Params().GetConsensus(), nBlockHash, true)){
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);
        return false;
    }

    if(txCollateral.vout.size() < 1) {
        strError = strprintf("tx vout size less than 1 | %d", txCollateral.vout.size());
        LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);
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
            LogPrintf ("CGovernanceObject::IsCollateralValid -- %s\n", strError);
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
        LogPrintf ("CGovernanceObject::IsCollateralValid -- %s\n", strError);
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
        LogPrintf ("CGovernanceObject::IsCollateralValid -- %s - %d confirmations\n", strError, nConfirmationsIn);
        return false;
    }

    return true;
}

int CGovernanceObject::CountMatchingVotes(vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn) const
{
    int nCount = 0;
    for(vote_m_cit it = mapCurrentMNVotes.begin(); it != mapCurrentMNVotes.end(); ++it) {
        const vote_rec_t& recVote = it->second;
        vote_instance_m_cit it2 = recVote.mapInstances.find(eVoteSignalIn);
        if(it2 == recVote.mapInstances.end()) {
            continue;
        }
        const vote_instance_t& voteInstance = it2->second;
        if(voteInstance.eOutcome == eVoteOutcomeIn) {
            ++nCount;
        }
    }
    return nCount;
}

/**
*   Get specific vote counts for each outcome (funding, validity, etc)
*/

int CGovernanceObject::GetAbsoluteYesCount(vote_signal_enum_t eVoteSignalIn) const
{
    return GetYesCount(eVoteSignalIn) - GetNoCount(eVoteSignalIn);
}

int CGovernanceObject::GetAbsoluteNoCount(vote_signal_enum_t eVoteSignalIn) const
{
    return GetNoCount(eVoteSignalIn) - GetYesCount(eVoteSignalIn);
}

int CGovernanceObject::GetYesCount(vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(eVoteSignalIn, VOTE_OUTCOME_YES);
}

int CGovernanceObject::GetNoCount(vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(eVoteSignalIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetAbstainCount(vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(eVoteSignalIn, VOTE_OUTCOME_ABSTAIN);
}

void CGovernanceObject::Relay()
{
    CInv inv(MSG_GOVERNANCE_OBJECT, GetHash());
    RelayInv(inv, PROTOCOL_VERSION);
}

std::string CGovernanceManager::ToString() const
{
    std::ostringstream info;

    info << "Governance Objects: " << (int)mapObjects.size() <<
            ", Seen Budgets : " << (int)mapSeenGovernanceObjects.size() <<
            ", Vote Count   : " << (int)mapVoteToObject.GetSize();

    return info.str();
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex *pindex)
{
    // Note this gets called from ActivateBestChain without cs_main being held
    // so it should be safe to lock our mutex here without risking a deadlock
    // On the other hand it should be safe for us to access pindex without holding a lock
    // on cs_main because the CBlockIndex objects are dynamically allocated and
    // presumably never deleted.
    if(!pindex) {
        return;
    }

    LOCK(cs);
    pCurrentBlockIndex = pindex;
    nCachedBlockHeight = pCurrentBlockIndex->nHeight;
    LogPrint("gobject", "CGovernanceManager::UpdatedBlockTip pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    // TO REPROCESS OBJECTS WE SHOULD BE SYNCED

    if(!fLiteMode && masternodeSync.IsSynced())
        NewBlock();
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

void CGovernanceObject::CheckOrphanVotes()
{
    const vote_mcache_t::list_t& listVotes = mapOrphanVotes.GetItemList();
    for(vote_mcache_t::list_cit it = listVotes.begin(); it != listVotes.end(); ++it) {
        const CGovernanceVote& vote = it->value;
        if(!mnodeman.Has(vote.GetVinMasternode())) {
            continue;
        }
        CGovernanceException exception;
        if(!ProcessVote(NULL, vote, exception)) {
            LogPrintf("CGovernanceObject::CheckOrphanVotes -- Failed to add orphan vote: %s\n", exception.what());
        }
    }
}
