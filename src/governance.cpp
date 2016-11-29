// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

#include "flat-database.h"
#include "governance.h"
#include "governance-object.h"
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

const std::string CGovernanceManager::SERIALIZATION_VERSION_STRING = "CGovernanceManager-Version-2";

CGovernanceManager::CGovernanceManager()
    : pCurrentBlockIndex(NULL),
      nTimeLastDiff(0),
      nCachedBlockHeight(0),
      mapObjects(),
      mapSeenGovernanceObjects(),
      mapMasternodeOrphanObjects(),
      mapWatchdogObjects(),
      mapVoteToObject(MAX_CACHE_SIZE),
      mapInvalidVotes(MAX_CACHE_SIZE),
      mapOrphanVotes(MAX_CACHE_SIZE),
      mapLastMasternodeTrigger(),
      setRequestedObjects(),
      fRateChecksEnabled(true),
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

        LOCK(cs);

        if(mapSeenGovernanceObjects.count(nHash)) {
            // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
            LogPrint("gobject", "CGovernanceManager -- Received already seen object: %s\n", strHash);
            return;
        }

        std::string strError = "";
        // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN

        bool fMasternodeMissing = false;
        bool fIsValid = govobj.IsValidLocally(pCurrentBlockIndex, strError, fMasternodeMissing, true);

        if(fMasternodeMissing) {
            mapMasternodeOrphanObjects.insert(std::make_pair(govobj.GetHash(), object_time_pair_t(govobj, GetAdjustedTime() + GOVERNANCE_ORPHAN_EXPIRATION_TIME)));
            LogPrint("gobject", "CGovernanceManager -- Missing masternode for: %s\n", strHash);
            // fIsValid must also be false here so we will return early in the next if block
        }
        if(!fIsValid) {
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
    std::vector<vote_time_pair_t> vecVotePairs;
    mapOrphanVotes.GetAll(nHash, vecVotePairs);

    int64_t nNow = GetAdjustedTime();
    for(size_t i = 0; i < vecVotePairs.size(); ++i) {
        bool fRemove = false;
        vote_time_pair_t& pairVote = vecVotePairs[i];
        CGovernanceVote& vote = pairVote.first;
        CGovernanceException exception;
        if(pairVote.second < nNow) {
            fRemove = true;
        }
        else if(govobj.ProcessVote(pfrom, vote, exception)) {
            vote.Relay();
            fRemove = true;
        }
        else {
            if((exception.GetNodePenalty() != 0) && masternodeSync.IsSynced()) {
                Misbehaving(pfrom->GetId(), exception.GetNodePenalty());
            }
        }
        if(fRemove) {
            mapOrphanVotes.Erase(nHash, pairVote);
        }
    }
}

bool CGovernanceManager::AddGovernanceObject(CGovernanceObject& govobj)
{
    LOCK(cs);
    std::string strError = "";

    DBG( cout << "CGovernanceManager::AddGovernanceObject START" << endl; );

    uint256 nHash = govobj.GetHash();

    // MAKE SURE THIS OBJECT IS OK

    if(!govobj.IsValidLocally(pCurrentBlockIndex, strError, true)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject -- invalid governance object - %s - (pCurrentBlockIndex nHeight %d) \n", strError, pCurrentBlockIndex->nHeight);
        return false;
    }

    // IF WE HAVE THIS OBJECT ALREADY, WE DON'T WANT ANOTHER COPY

    if(mapObjects.count(nHash)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject -- already have governance object - %s\n", strError);
        return false;
    }

    // INSERT INTO OUR GOVERNANCE OBJECT MEMORY
    mapObjects.insert(std::make_pair(nHash, govobj));

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
        triggerman.AddNewTrigger(nHash);
        DBG( cout << "CGovernanceManager::AddGovernanceObject After AddNewTrigger" << endl; );
        break;
    case GOVERNANCE_OBJECT_WATCHDOG:
        mapWatchdogObjects[nHash] = GetAdjustedTime() + GOVERNANCE_WATCHDOG_EXPIRATION_TIME;
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

    // Flag expired watchdogs for removal
    int64_t nNow = GetAdjustedTime();
    if(mapWatchdogObjects.size() > 1) {
        hash_time_m_it it = mapWatchdogObjects.begin();
        while(it != mapWatchdogObjects.end()) {
            if(it->second < nNow) {
                object_m_it it2 = mapObjects.find(it->first);
                if(it2 != mapObjects.end()) {
                    it2->second.fExpired = true;
                    it2->second.nDeletionTime = nNow;
                }
                mapWatchdogObjects.erase(it++);
            }
            else {
                ++it;
            }
        }
    }

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

        int64_t nTimeSinceDeletion = GetAdjustedTime() - pObj->GetDeletionTime();

        if((pObj->IsSetCachedDelete() || pObj->IsSetExpired()) &&
           (nTimeSinceDeletion >= GOVERNANCE_DELETION_DELAY)) {
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

std::vector<CGovernanceVote> CGovernanceManager::GetCurrentVotes(const uint256& nParentHash, const CTxIn& mnCollateralOutpointFilter)
{
    LOCK(cs);
    std::vector<CGovernanceVote> vecResult;

    // Find the governance object or short-circuit.
    object_m_it it = mapObjects.find(nParentHash);
    if(it == mapObjects.end()) return vecResult;
    CGovernanceObject& govobj = it->second;

    // Compile a list of Masternode collateral outpoints for which to get votes
    std::vector<CTxIn> vecMNTxIn;
    if (mnCollateralOutpointFilter == CTxIn()) {
        std::vector<CMasternode> mnlist = mnodeman.GetFullMasternodeVector();
        for (std::vector<CMasternode>::iterator it = mnlist.begin(); it != mnlist.end(); ++it)
        {
            vecMNTxIn.push_back(it->vin);
        }
    }
    else {
        vecMNTxIn.push_back(mnCollateralOutpointFilter);
    }

    // Loop thru each MN collateral outpoint and get the votes for the `nParentHash` governance object
    for (std::vector<CTxIn>::iterator it = vecMNTxIn.begin(); it != vecMNTxIn.end(); ++it)
    {
        CTxIn &mnCollateralOutpoint = *it;

        // get a vote_rec_t from the govobj
        vote_rec_t voteRecord;
        if (!govobj.GetCurrentMNVotes(mnCollateralOutpoint, voteRecord)) continue;

        for (vote_instance_m_it it3 = voteRecord.mapInstances.begin(); it3 != voteRecord.mapInstances.end(); ++it3) {
            int signal = (it3->first);
            int outcome = ((it3->second).eOutcome);
            int64_t nTime = ((it3->second).nTime);

            CGovernanceVote vote = CGovernanceVote(mnCollateralOutpoint, nParentHash, (vote_signal_enum_t)signal, (vote_outcome_enum_t)outcome);
            vote.SetTime(nTime);

            vecResult.push_back(vote);
        }
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

            std::string strError;
            if(govobj.IsSetCachedValid() &&
               (nProp == uint256() || h == nProp) &&
               govobj.IsValidLocally(pCurrentBlockIndex, strError, true)) {
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
    LogPrintf("CGovernanceManager::Sync -- sent %d items, peer=%d\n", nInvCount, pfrom->id);
}

bool CGovernanceManager::MasternodeRateCheck(const CTxIn& vin, int nObjectType)
{
    LOCK(cs);

    if(!fRateChecksEnabled) {
        return true;
    }

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
        if(mapOrphanVotes.Insert(nHashGovobj, vote_time_pair_t(vote, GetAdjustedTime() + GOVERNANCE_ORPHAN_EXPIRATION_TIME))) {
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
    fRateChecksEnabled = false;
    for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
        it->second.CheckOrphanVotes();
    }
    fRateChecksEnabled = true;
}

void CGovernanceManager::CheckMasternodeOrphanObjects()
{
    LOCK(cs);
    int64_t nNow = GetAdjustedTime();
    fRateChecksEnabled = false;
    object_time_m_it it = mapMasternodeOrphanObjects.begin();
    while(it != mapMasternodeOrphanObjects.end()) {
        object_time_pair_t& pair = it->second;
        CGovernanceObject& govobj = pair.first;

        if(pair.second < nNow) {
            mapMasternodeOrphanObjects.erase(it++);
            continue;
        }

        string strError;
        bool fMasternodeMissing = false;
        bool fIsValid = govobj.IsValidLocally(pCurrentBlockIndex, strError, fMasternodeMissing, true);
        if(!fIsValid) {
            if(!fMasternodeMissing) {
                mapMasternodeOrphanObjects.erase(it++);
            }
            else {
                ++it;
            }
            continue;
        }

        if(AddGovernanceObject(govobj)) {
            LogPrintf("CGovernanceManager::CheckMasternodeOrphanObjects -- %s new\n", govobj.GetHash().ToString());
            govobj.Relay();
            mapMasternodeOrphanObjects.erase(it++);
        }
        else {
            ++it;
        }
    }
    fRateChecksEnabled = true;
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

void CGovernanceManager::InitOnLoad()
{
    LOCK(cs);
    RebuildIndexes();
    AddCachedTriggers();
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
