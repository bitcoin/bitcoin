// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "governance.h"
#include "governance-object.h"
#include "governance-vote.h"
#include "governance-classes.h"
#include "net_processing.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "netfulfilledman.h"
#include "util.h"

CGovernanceManager governance;

int nSubmittedFinalBudget;

const std::string CGovernanceManager::SERIALIZATION_VERSION_STRING = "CGovernanceManager-Version-12";
const int CGovernanceManager::MAX_TIME_FUTURE_DEVIATION = 60*60;
const int CGovernanceManager::RELIABLE_PROPAGATION_TIME = 60;

CGovernanceManager::CGovernanceManager()
    : nTimeLastDiff(0),
      nCachedBlockHeight(0),
      mapObjects(),
      mapErasedGovernanceObjects(),
      mapMasternodeOrphanObjects(),
      mapWatchdogObjects(),
      nHashWatchdogCurrent(),
      nTimeWatchdogCurrent(0),
      mapVoteToObject(MAX_CACHE_SIZE),
      mapInvalidVotes(MAX_CACHE_SIZE),
      mapOrphanVotes(MAX_CACHE_SIZE),
      mapLastMasternodeObject(),
      setRequestedObjects(),
      fRateChecksEnabled(true),
      cs()
{}

// Accessors for thread-safe access to maps
bool CGovernanceManager::HaveObjectForHash(uint256 nHash) {
    LOCK(cs);
    return (mapObjects.count(nHash) == 1 || mapPostponedObjects.count(nHash) == 1);
}

bool CGovernanceManager::SerializeObjectForHash(uint256 nHash, CDataStream& ss)
{
    LOCK(cs);
    object_m_it it = mapObjects.find(nHash);
    if (it == mapObjects.end()) {
        it = mapPostponedObjects.find(nHash);
        if (it == mapPostponedObjects.end())
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

int CGovernanceManager::GetVoteCount() const
{
    LOCK(cs);
    return (int)mapVoteToObject.GetSize();
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

void CGovernanceManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman)
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
        CBloomFilter filter;

        vRecv >> nProp;

        if(pfrom->nVersion >= GOVERNANCE_FILTER_PROTO_VERSION) {
            vRecv >> filter;
            filter.UpdateEmptyFull();
        }
        else {
            filter.clear();
        }

        if(nProp == uint256()) {
            if(netfulfilledman.HasFulfilledRequest(pfrom->addr, NetMsgType::MNGOVERNANCESYNC)) {
                // Asking for the whole list multiple times in a short period of time is no good
                LogPrint("gobject", "MNGOVERNANCESYNC -- peer already asked me for the list\n");
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
            netfulfilledman.AddFulfilledRequest(pfrom->addr, NetMsgType::MNGOVERNANCESYNC);
        }

        Sync(pfrom, nProp, filter, connman);
        LogPrint("gobject", "MNGOVERNANCESYNC -- syncing governance objects to our peer at %s\n", pfrom->addr.ToString());

    }

    // A NEW GOVERNANCE OBJECT HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECT)
    {
        // MAKE SURE WE HAVE A VALID REFERENCE TO THE TIP BEFORE CONTINUING


        if(!masternodeSync.IsMasternodeListSynced()) {
            LogPrint("gobject", "MNGOVERNANCEOBJECT -- masternode list not synced\n");
            return;
        }

        CGovernanceObject govobj;
        vRecv >> govobj;

        uint256 nHash = govobj.GetHash();
        std::string strHash = nHash.ToString();

        pfrom->setAskFor.erase(nHash);

        LogPrint("gobject", "MNGOVERNANCEOBJECT -- Received object: %s\n", strHash);

        if(!AcceptObjectMessage(nHash)) {
            LogPrintf("MNGOVERNANCEOBJECT -- Received unrequested object: %s\n", strHash);
            return;
        }

        LOCK2(cs_main, cs);

        if(mapObjects.count(nHash) || mapPostponedObjects.count(nHash) ||
           mapErasedGovernanceObjects.count(nHash) || mapMasternodeOrphanObjects.count(nHash)) {
            // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
            LogPrint("gobject", "MNGOVERNANCEOBJECT -- Received already seen object: %s\n", strHash);
            return;
        }

        bool fRateCheckBypassed = false;
        if(!MasternodeRateCheck(govobj, true, false, fRateCheckBypassed)) {
            LogPrintf("MNGOVERNANCEOBJECT -- masternode rate check failed - %s - (current block height %d) \n", strHash, nCachedBlockHeight);
            return;
        }

        std::string strError = "";
        // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN

        bool fMasternodeMissing = false;
        bool fMissingConfirmations = false;
        bool fIsValid = govobj.IsValidLocally(strError, fMasternodeMissing, fMissingConfirmations, true);

        if(fRateCheckBypassed && (fIsValid || fMasternodeMissing)) {
            if(!MasternodeRateCheck(govobj, true)) {
                LogPrintf("MNGOVERNANCEOBJECT -- masternode rate check failed (after signature verification) - %s - (current block height %d) \n", strHash, nCachedBlockHeight);
                return;
            }
        }

        if(!fIsValid) {
            if(fMasternodeMissing) {

                int& count = mapMasternodeOrphanCounter[govobj.GetMasternodeVin().prevout];
                if (count >= 10) {
                    LogPrint("gobject", "MNGOVERNANCEOBJECT -- Too many orphan objects, missing masternode=%s\n", govobj.GetMasternodeVin().prevout.ToStringShort());
                    // ask for this object again in 2 minutes
                    CInv inv(MSG_GOVERNANCE_OBJECT, govobj.GetHash());
                    pfrom->AskFor(inv);
                    return;
                }

                count++;
                ExpirationInfo info(pfrom->GetId(), GetAdjustedTime() + GOVERNANCE_ORPHAN_EXPIRATION_TIME);
                mapMasternodeOrphanObjects.insert(std::make_pair(nHash, object_info_pair_t(govobj, info)));
                LogPrintf("MNGOVERNANCEOBJECT -- Missing masternode for: %s, strError = %s\n", strHash, strError);
            } else if(fMissingConfirmations) {
                AddPostponedObject(govobj);
                LogPrintf("MNGOVERNANCEOBJECT -- Not enough fee confirmations for: %s, strError = %s\n", strHash, strError);
            } else {
                LogPrintf("MNGOVERNANCEOBJECT -- Governance object is invalid - %s\n", strError);
                // apply node's ban score
                Misbehaving(pfrom->GetId(), 20);
            }

            return;
        }

        AddGovernanceObject(govobj, connman, pfrom);
    }

    // A NEW GOVERNANCE OBJECT VOTE HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECTVOTE)
    {
        // Ignore such messages until masternode list is synced
        if(!masternodeSync.IsMasternodeListSynced()) {
            LogPrint("gobject", "MNGOVERNANCEOBJECTVOTE -- masternode list not synced\n");
            return;
        }

        CGovernanceVote vote;
        vRecv >> vote;

        LogPrint("gobject", "MNGOVERNANCEOBJECTVOTE -- Received vote: %s\n", vote.ToString());

        uint256 nHash = vote.GetHash();
        std::string strHash = nHash.ToString();

        pfrom->setAskFor.erase(nHash);

        if(!AcceptVoteMessage(nHash)) {
            LogPrint("gobject", "MNGOVERNANCEOBJECTVOTE -- Received unrequested vote object: %s, hash: %s, peer = %d\n",
                      vote.ToString(), strHash, pfrom->GetId());
            return;
        }

        CGovernanceException exception;
        if(ProcessVote(pfrom, vote, exception, connman)) {
            LogPrint("gobject", "MNGOVERNANCEOBJECTVOTE -- %s new\n", strHash);
            masternodeSync.BumpAssetLastTime("MNGOVERNANCEOBJECTVOTE");
            vote.Relay(connman);
        }
        else {
            LogPrint("gobject", "MNGOVERNANCEOBJECTVOTE -- Rejected vote, error = %s\n", exception.what());
            if((exception.GetNodePenalty() != 0) && masternodeSync.IsSynced()) {
                Misbehaving(pfrom->GetId(), exception.GetNodePenalty());
            }
            return;
        }

    }
}

void CGovernanceManager::CheckOrphanVotes(CGovernanceObject& govobj, CGovernanceException& exception, CConnman& connman)
{
    uint256 nHash = govobj.GetHash();
    std::vector<vote_time_pair_t> vecVotePairs;
    mapOrphanVotes.GetAll(nHash, vecVotePairs);

    ScopedLockBool guard(cs, fRateChecksEnabled, false);

    int64_t nNow = GetAdjustedTime();
    for(size_t i = 0; i < vecVotePairs.size(); ++i) {
        bool fRemove = false;
        vote_time_pair_t& pairVote = vecVotePairs[i];
        CGovernanceVote& vote = pairVote.first;
        CGovernanceException exception;
        if(pairVote.second < nNow) {
            fRemove = true;
        }
        else if(govobj.ProcessVote(NULL, vote, exception, connman)) {
            vote.Relay(connman);
            fRemove = true;
        }
        if(fRemove) {
            mapOrphanVotes.Erase(nHash, pairVote);
        }
    }
}

void CGovernanceManager::AddGovernanceObject(CGovernanceObject& govobj, CConnman& connman, CNode* pfrom)
{
    DBG( cout << "CGovernanceManager::AddGovernanceObject START" << endl; );

    uint256 nHash = govobj.GetHash();
    std::string strHash = nHash.ToString();

    // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANANGED DATA

    govobj.UpdateSentinelVariables(); //this sets local vars in object

    LOCK2(cs_main, cs);
    std::string strError = "";

    // MAKE SURE THIS OBJECT IS OK

    if(!govobj.IsValidLocally(strError, true)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject -- invalid governance object - %s - (nCachedBlockHeight %d) \n", strError, nCachedBlockHeight);
        return;
    }

    // IF WE HAVE THIS OBJECT ALREADY, WE DON'T WANT ANOTHER COPY

    if(mapObjects.count(nHash)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject -- already have governance object %s\n", nHash.ToString());
        return;
    }

    LogPrint("gobject", "CGovernanceManager::AddGovernanceObject -- Adding object: hash = %s, type = %d\n", nHash.ToString(), govobj.GetObjectType());

    if(govobj.nObjectType == GOVERNANCE_OBJECT_WATCHDOG) {
        // If it's a watchdog, make sure it fits required time bounds
        if((govobj.GetCreationTime() < GetAdjustedTime() - GOVERNANCE_WATCHDOG_EXPIRATION_TIME ||
            govobj.GetCreationTime() > GetAdjustedTime() + GOVERNANCE_WATCHDOG_EXPIRATION_TIME)
            ) {
            // drop it
            LogPrint("gobject", "CGovernanceManager::AddGovernanceObject -- CreationTime is out of bounds: hash = %s\n", nHash.ToString());
            return;
        }

        if(!UpdateCurrentWatchdog(govobj)) {
            // Allow wd's which are not current to be reprocessed
            if(pfrom && (nHashWatchdogCurrent != uint256())) {
                pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT, nHashWatchdogCurrent));
            }
            LogPrint("gobject", "CGovernanceManager::AddGovernanceObject -- Watchdog not better than current: hash = %s\n", nHash.ToString());
            return;
        }
    }

    // INSERT INTO OUR GOVERNANCE OBJECT MEMORY
    mapObjects.insert(std::make_pair(nHash, govobj));

    // SHOULD WE ADD THIS OBJECT TO ANY OTHER MANANGERS?

    DBG( cout << "CGovernanceManager::AddGovernanceObject Before trigger block, strData = "
              << govobj.GetDataAsString()
              << ", nObjectType = " << govobj.nObjectType
              << endl; );

    switch(govobj.nObjectType) {
    case GOVERNANCE_OBJECT_TRIGGER:
        DBG( cout << "CGovernanceManager::AddGovernanceObject Before AddNewTrigger" << endl; );
        triggerman.AddNewTrigger(nHash);
        DBG( cout << "CGovernanceManager::AddGovernanceObject After AddNewTrigger" << endl; );
        break;
    case GOVERNANCE_OBJECT_WATCHDOG:
        mapWatchdogObjects[nHash] = govobj.GetCreationTime() + GOVERNANCE_WATCHDOG_EXPIRATION_TIME;
        LogPrint("gobject", "CGovernanceManager::AddGovernanceObject -- Added watchdog to map: hash = %s\n", nHash.ToString());
        break;
    default:
        break;
    }

    LogPrintf("AddGovernanceObject -- %s new, received form %s\n", strHash, pfrom? pfrom->addrName : "NULL");
    govobj.Relay(connman);

    // Update the rate buffer
    MasternodeRateUpdate(govobj);

    masternodeSync.BumpAssetLastTime("CGovernanceManager::AddGovernanceObject");

    // WE MIGHT HAVE PENDING/ORPHAN VOTES FOR THIS OBJECT

    CGovernanceException exception;
    CheckOrphanVotes(govobj, exception, connman);

    DBG( cout << "CGovernanceManager::AddGovernanceObject END" << endl; );
}

bool CGovernanceManager::UpdateCurrentWatchdog(CGovernanceObject& watchdogNew)
{
    bool fAccept = false;

    arith_uint256 nHashNew = UintToArith256(watchdogNew.GetHash());
    arith_uint256 nHashCurrent = UintToArith256(nHashWatchdogCurrent);

    int64_t nExpirationDelay = GOVERNANCE_WATCHDOG_EXPIRATION_TIME / 2;
    int64_t nNow = GetAdjustedTime();

    if(nHashWatchdogCurrent == uint256() ||                                             // no known current OR
       ((nNow - watchdogNew.GetCreationTime() < nExpirationDelay) &&                    // (new one is NOT expired AND
        ((nNow - nTimeWatchdogCurrent > nExpirationDelay) || (nHashNew > nHashCurrent)))//  (current is expired OR
                                                                                        //   its hash is lower))
    ) {
        LOCK(cs);
        object_m_it it = mapObjects.find(nHashWatchdogCurrent);
        if(it != mapObjects.end()) {
            LogPrint("gobject", "CGovernanceManager::UpdateCurrentWatchdog -- Expiring previous current watchdog, hash = %s\n", nHashWatchdogCurrent.ToString());
            it->second.fExpired = true;
            if(it->second.nDeletionTime == 0) {
                it->second.nDeletionTime = nNow;
            }
        }
        nHashWatchdogCurrent = watchdogNew.GetHash();
        nTimeWatchdogCurrent = watchdogNew.GetCreationTime();
        fAccept = true;
        LogPrint("gobject", "CGovernanceManager::UpdateCurrentWatchdog -- Current watchdog updated to: hash = %s\n",
                 ArithToUint256(nHashNew).ToString());
    }

    return fAccept;
}

void CGovernanceManager::UpdateCachesAndClean()
{
    LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean\n");

    std::vector<uint256> vecDirtyHashes = mnodeman.GetAndClearDirtyGovernanceObjectHashes();

    LOCK2(cs_main, cs);

    // Flag expired watchdogs for removal
    int64_t nNow = GetAdjustedTime();
    LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean -- Number watchdogs in map: %d, current time = %d\n", mapWatchdogObjects.size(), nNow);
    if(mapWatchdogObjects.size() > 1) {
        hash_time_m_it it = mapWatchdogObjects.begin();
        while(it != mapWatchdogObjects.end()) {
            LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean -- Checking watchdog: %s, expiration time = %d\n", it->first.ToString(), it->second);
            if(it->second < nNow) {
                LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean -- Attempting to expire watchdog: %s, expiration time = %d\n", it->first.ToString(), it->second);
                object_m_it it2 = mapObjects.find(it->first);
                if(it2 != mapObjects.end()) {
                    LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean -- Expiring watchdog: %s, expiration time = %d\n", it->first.ToString(), it->second);
                    it2->second.fExpired = true;
                    if(it2->second.nDeletionTime == 0) {
                        it2->second.nDeletionTime = nNow;
                    }
                }
                if(it->first == nHashWatchdogCurrent) {
                    nHashWatchdogCurrent = uint256();
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

    ScopedLockBool guard(cs, fRateChecksEnabled, false);

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

        uint256 nHash = it->first;
        std::string strHash = nHash.ToString();

        // IF CACHE IS NOT DIRTY, WHY DO THIS?
        if(pObj->IsSetDirtyCache()) {
            // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
            pObj->UpdateLocalValidity();

            // UPDATE SENTINEL SIGNALING VARIABLES
            pObj->UpdateSentinelVariables();
        }

        if(pObj->IsSetCachedDelete() && (nHash == nHashWatchdogCurrent)) {
            nHashWatchdogCurrent = uint256();
        }

        // IF DELETE=TRUE, THEN CLEAN THE MESS UP!

        int64_t nTimeSinceDeletion = GetAdjustedTime() - pObj->GetDeletionTime();

        LogPrint("gobject", "CGovernanceManager::UpdateCachesAndClean -- Checking object for deletion: %s, deletion time = %d, time since deletion = %d, delete flag = %d, expired flag = %d\n",
                 strHash, pObj->GetDeletionTime(), nTimeSinceDeletion, pObj->IsSetCachedDelete(), pObj->IsSetExpired());

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

            int64_t nSuperblockCycleSeconds = Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().nPowTargetSpacing;
            int64_t nTimeExpired = pObj->GetCreationTime() + 2 * nSuperblockCycleSeconds + GOVERNANCE_DELETION_DELAY;

            if(pObj->GetObjectType() == GOVERNANCE_OBJECT_WATCHDOG) {
                mapWatchdogObjects.erase(nHash);
            } else if(pObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) {
                // keep hashes of deleted proposals forever
                nTimeExpired = std::numeric_limits<int64_t>::max();
            }

            mapErasedGovernanceObjects.insert(std::make_pair(nHash, nTimeExpired));
            mapObjects.erase(it++);
        } else {
            ++it;
        }
    }

    // forget about expired deleted objects
    hash_time_m_it s_it = mapErasedGovernanceObjects.begin();
    while(s_it != mapErasedGovernanceObjects.end()) {
        if(s_it->second < nNow)
            mapErasedGovernanceObjects.erase(s_it++);
        else
            ++s_it;
    }

    LogPrintf("CGovernanceManager::UpdateCachesAndClean -- %s\n", ToString());
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

std::vector<CGovernanceVote> CGovernanceManager::GetCurrentVotes(const uint256& nParentHash, const COutPoint& mnCollateralOutpointFilter)
{
    LOCK(cs);
    std::vector<CGovernanceVote> vecResult;

    // Find the governance object or short-circuit.
    object_m_it it = mapObjects.find(nParentHash);
    if(it == mapObjects.end()) return vecResult;
    CGovernanceObject& govobj = it->second;

    CMasternode mn;
    std::map<COutPoint, CMasternode> mapMasternodes;
    if(mnCollateralOutpointFilter == COutPoint()) {
        mapMasternodes = mnodeman.GetFullMasternodeMap();
    } else if (mnodeman.Get(mnCollateralOutpointFilter, mn)) {
        mapMasternodes[mnCollateralOutpointFilter] = mn;
    }

    // Loop thru each MN collateral outpoint and get the votes for the `nParentHash` governance object
    for (auto& mnpair : mapMasternodes)
    {
        // get a vote_rec_t from the govobj
        vote_rec_t voteRecord;
        if (!govobj.GetCurrentMNVotes(mnpair.first, voteRecord)) continue;

        for (vote_instance_m_it it3 = voteRecord.mapInstances.begin(); it3 != voteRecord.mapInstances.end(); ++it3) {
            int signal = (it3->first);
            int outcome = ((it3->second).eOutcome);
            int64_t nCreationTime = ((it3->second).nCreationTime);

            CGovernanceVote vote = CGovernanceVote(mnpair.first, nParentHash, (vote_signal_enum_t)signal, (vote_outcome_enum_t)outcome);
            vote.SetTime(nCreationTime);

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

void CGovernanceManager::DoMaintenance(CConnman& connman)
{
    if(fLiteMode || !masternodeSync.IsSynced()) return;

    // CHECK OBJECTS WE'VE ASKED FOR, REMOVE OLD ENTRIES

    CleanOrphanObjects();

    RequestOrphanObjects(connman);

    // CHECK AND REMOVE - REPROCESS GOVERNANCE OBJECTS

    UpdateCachesAndClean();
}

bool CGovernanceManager::ConfirmInventoryRequest(const CInv& inv)
{
    // do not request objects until it's time to sync
    if(!masternodeSync.IsWinnersListSynced()) return false;

    LOCK(cs);

    LogPrint("gobject", "CGovernanceManager::ConfirmInventoryRequest inv = %s\n", inv.ToString());

    // First check if we've already recorded this object
    switch(inv.type) {
    case MSG_GOVERNANCE_OBJECT:
    {
        if(mapObjects.count(inv.hash) == 1 || mapPostponedObjects.count(inv.hash) == 1) {
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

void CGovernanceManager::Sync(CNode* pfrom, const uint256& nProp, const CBloomFilter& filter, CConnman& connman)
{

    /*
        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.
    */

    // do not provide any data until our node is synced
    if(fMasterNode && !masternodeSync.IsSynced()) return;

    int nObjCount = 0;
    int nVoteCount = 0;

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    LogPrint("gobject", "CGovernanceManager::Sync -- syncing to peer=%d, nProp = %s\n", pfrom->id, nProp.ToString());

    {
        LOCK2(cs_main, cs);

        if(nProp == uint256()) {
            // all valid objects, no votes
            for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
                CGovernanceObject& govobj = it->second;
                std::string strHash = it->first.ToString();

                LogPrint("gobject", "CGovernanceManager::Sync -- attempting to sync govobj: %s, peer=%d\n", strHash, pfrom->id);

                if(govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
                    LogPrintf("CGovernanceManager::Sync -- not syncing deleted/expired govobj: %s, peer=%d\n",
                              strHash, pfrom->id);
                    continue;
                }

                // Push the inventory budget proposal message over to the other client
                LogPrint("gobject", "CGovernanceManager::Sync -- syncing govobj: %s, peer=%d\n", strHash, pfrom->id);
                pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT, it->first));
                ++nObjCount;
            }
        } else {
            // single valid object and its valid votes
            object_m_it it = mapObjects.find(nProp);
            if(it == mapObjects.end()) {
                LogPrint("gobject", "CGovernanceManager::Sync -- no matching object for hash %s, peer=%d\n", nProp.ToString(), pfrom->id);
                return;
            }
            CGovernanceObject& govobj = it->second;
            std::string strHash = it->first.ToString();

            LogPrint("gobject", "CGovernanceManager::Sync -- attempting to sync govobj: %s, peer=%d\n", strHash, pfrom->id);

            if(govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
                LogPrintf("CGovernanceManager::Sync -- not syncing deleted/expired govobj: %s, peer=%d\n",
                          strHash, pfrom->id);
                return;
            }

            // Push the inventory budget proposal message over to the other client
            LogPrint("gobject", "CGovernanceManager::Sync -- syncing govobj: %s, peer=%d\n", strHash, pfrom->id);
            pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT, it->first));
            ++nObjCount;

            std::vector<CGovernanceVote> vecVotes = govobj.GetVoteFile().GetVotes();
            for(size_t i = 0; i < vecVotes.size(); ++i) {
                if(filter.contains(vecVotes[i].GetHash())) {
                    continue;
                }
                if(!vecVotes[i].IsValid(true)) {
                    continue;
                }
                pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT_VOTE, vecVotes[i].GetHash()));
                ++nVoteCount;
            }
        }
    }

    connman.PushMessage(pfrom, NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ, nObjCount);
    connman.PushMessage(pfrom, NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ_VOTE, nVoteCount);
    LogPrintf("CGovernanceManager::Sync -- sent %d objects and %d votes to peer=%d\n", nObjCount, nVoteCount, pfrom->id);
}


void CGovernanceManager::MasternodeRateUpdate(const CGovernanceObject& govobj)
{
    int nObjectType = govobj.GetObjectType();
    if((nObjectType != GOVERNANCE_OBJECT_TRIGGER) && (nObjectType != GOVERNANCE_OBJECT_WATCHDOG))
        return;

    const CTxIn& vin = govobj.GetMasternodeVin();
    txout_m_it it  = mapLastMasternodeObject.find(vin.prevout);

    if(it == mapLastMasternodeObject.end())
        it = mapLastMasternodeObject.insert(txout_m_t::value_type(vin.prevout, last_object_rec(true))).first;

    int64_t nTimestamp = govobj.GetCreationTime();
    if (GOVERNANCE_OBJECT_TRIGGER == nObjectType)
        it->second.triggerBuffer.AddTimestamp(nTimestamp);
    else if (GOVERNANCE_OBJECT_WATCHDOG == nObjectType)
        it->second.watchdogBuffer.AddTimestamp(nTimestamp);

    if (nTimestamp > GetTime() + MAX_TIME_FUTURE_DEVIATION - RELIABLE_PROPAGATION_TIME) {
        // schedule additional relay for the object
        setAdditionalRelayObjects.insert(govobj.GetHash());
    }

    it->second.fStatusOK = true;
}

bool CGovernanceManager::MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus)
{
    bool fRateCheckBypassed;
    return MasternodeRateCheck(govobj, fUpdateFailStatus, true, fRateCheckBypassed);
}

bool CGovernanceManager::MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus, bool fForce, bool& fRateCheckBypassed)
{
    LOCK(cs);

    fRateCheckBypassed = false;

    if(!masternodeSync.IsSynced()) {
        return true;
    }

    if(!fRateChecksEnabled) {
        return true;
    }

    int nObjectType = govobj.GetObjectType();
    if((nObjectType != GOVERNANCE_OBJECT_TRIGGER) && (nObjectType != GOVERNANCE_OBJECT_WATCHDOG)) {
        return true;
    }

    const CTxIn& vin = govobj.GetMasternodeVin();
    int64_t nTimestamp = govobj.GetCreationTime();
    int64_t nNow = GetAdjustedTime();
    int64_t nSuperblockCycleSeconds = Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().nPowTargetSpacing;

    std::string strHash = govobj.GetHash().ToString();

    if(nTimestamp < nNow - 2 * nSuperblockCycleSeconds) {
        LogPrintf("CGovernanceManager::MasternodeRateCheck -- object %s rejected due to too old timestamp, masternode vin = %s, timestamp = %d, current time = %d\n",
                 strHash, vin.prevout.ToStringShort(), nTimestamp, nNow);
        return false;
    }

    if(nTimestamp > nNow + MAX_TIME_FUTURE_DEVIATION) {
        LogPrintf("CGovernanceManager::MasternodeRateCheck -- object %s rejected due to too new (future) timestamp, masternode vin = %s, timestamp = %d, current time = %d\n",
                 strHash, vin.prevout.ToStringShort(), nTimestamp, nNow);
        return false;
    }

    txout_m_it it  = mapLastMasternodeObject.find(vin.prevout);
    if(it == mapLastMasternodeObject.end())
        return true;

    if(it->second.fStatusOK && !fForce) {
        fRateCheckBypassed = true;
        return true;
    }

    double dMaxRate = 1.1 / nSuperblockCycleSeconds;
    double dRate = 0.0;
    CRateCheckBuffer buffer;
    switch(nObjectType) {
    case GOVERNANCE_OBJECT_TRIGGER:
        // Allow 1 trigger per mn per cycle, with a small fudge factor
        buffer = it->second.triggerBuffer;
        dMaxRate = 2 * 1.1 / double(nSuperblockCycleSeconds);
        break;
    case GOVERNANCE_OBJECT_WATCHDOG:
        buffer = it->second.watchdogBuffer;
        dMaxRate = 2 * 1.1 / 3600.;
        break;
    default:
        break;
    }

    buffer.AddTimestamp(nTimestamp);
    dRate = buffer.GetRate();

    bool fRateOK = ( dRate < dMaxRate );

    if(!fRateOK)
    {
        LogPrintf("CGovernanceManager::MasternodeRateCheck -- Rate too high: object hash = %s, masternode vin = %s, object timestamp = %d, rate = %f, max rate = %f\n",
                  strHash, vin.prevout.ToStringShort(), nTimestamp, dRate, dMaxRate);

        if (fUpdateFailStatus)
            it->second.fStatusOK = false;
    }

    return fRateOK;
}

bool CGovernanceManager::ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman)
{
    ENTER_CRITICAL_SECTION(cs);
    uint256 nHashVote = vote.GetHash();
    if(mapInvalidVotes.HasKey(nHashVote)) {
        std::ostringstream ostr;
        ostr << "CGovernanceManager::ProcessVote -- Old invalid vote "
                << ", MN outpoint = " << vote.GetMasternodeOutpoint().ToStringShort()
                << ", governance object hash = " << vote.GetParentHash().ToString();
        LogPrintf("%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    uint256 nHashGovobj = vote.GetParentHash();
    object_m_it it = mapObjects.find(nHashGovobj);
    if(it == mapObjects.end()) {
        std::ostringstream ostr;
        ostr << "CGovernanceManager::ProcessVote -- Unknown parent object "
             << ", MN outpoint = " << vote.GetMasternodeOutpoint().ToStringShort()
             << ", governance object hash = " << vote.GetParentHash().ToString();
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_WARNING);
        if(mapOrphanVotes.Insert(nHashGovobj, vote_time_pair_t(vote, GetAdjustedTime() + GOVERNANCE_ORPHAN_EXPIRATION_TIME))) {
            LEAVE_CRITICAL_SECTION(cs);
            RequestGovernanceObject(pfrom, nHashGovobj, connman);
            LogPrintf("%s\n", ostr.str());
            return false;
        }

        LogPrint("gobject", "%s\n", ostr.str());
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    CGovernanceObject& govobj = it->second;

    if(govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
        LogPrint("gobject", "CGovernanceObject::ProcessVote -- ignoring vote for expired or deleted object, hash = %s\n", nHashGovobj.ToString());
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    bool fOk = govobj.ProcessVote(pfrom, vote, exception, connman);
    if(fOk) {
        mapVoteToObject.Insert(nHashVote, &govobj);

        if(govobj.GetObjectType() == GOVERNANCE_OBJECT_WATCHDOG) {
            mnodeman.UpdateWatchdogVoteTime(vote.GetMasternodeOutpoint());
            LogPrint("gobject", "CGovernanceObject::ProcessVote -- GOVERNANCE_OBJECT_WATCHDOG vote for %s\n", vote.GetParentHash().ToString());
        }
    }
    LEAVE_CRITICAL_SECTION(cs);
    return fOk;
}

void CGovernanceManager::CheckMasternodeOrphanVotes(CConnman& connman)
{
    LOCK2(cs_main, cs);

    ScopedLockBool guard(cs, fRateChecksEnabled, false);

    for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
        it->second.CheckOrphanVotes(connman);
    }
}

void CGovernanceManager::CheckMasternodeOrphanObjects(CConnman& connman)
{
    LOCK2(cs_main, cs);
    int64_t nNow = GetAdjustedTime();
    ScopedLockBool guard(cs, fRateChecksEnabled, false);
    object_info_m_it it = mapMasternodeOrphanObjects.begin();
    while(it != mapMasternodeOrphanObjects.end()) {
        object_info_pair_t& pair = it->second;
        CGovernanceObject& govobj = pair.first;

        if(pair.second.nExpirationTime >= nNow) {
            string strError;
            bool fMasternodeMissing = false;
            bool fConfirmationsMissing = false;
            bool fIsValid = govobj.IsValidLocally(strError, fMasternodeMissing, fConfirmationsMissing, true);

            if(fIsValid) {
                AddGovernanceObject(govobj, connman);
            } else if(fMasternodeMissing) {
                ++it;
                continue;
            }
        } else {
            // apply node's ban score
            Misbehaving(pair.second.idFrom, 20);
        }

        auto it_count = mapMasternodeOrphanCounter.find(govobj.GetMasternodeVin().prevout);
        if(--it_count->second == 0)
            mapMasternodeOrphanCounter.erase(it_count);

        mapMasternodeOrphanObjects.erase(it++);
    }
}

void CGovernanceManager::CheckPostponedObjects(CConnman& connman)
{
    if(!masternodeSync.IsSynced()) return;

    LOCK2(cs_main, cs);

    // Check postponed proposals
    for(object_m_it it = mapPostponedObjects.begin(); it != mapPostponedObjects.end();) {

        const uint256& nHash = it->first;
        CGovernanceObject& govobj = it->second;

        assert(govobj.GetObjectType() != GOVERNANCE_OBJECT_WATCHDOG &&
               govobj.GetObjectType() != GOVERNANCE_OBJECT_TRIGGER);

        std::string strError;
        bool fMissingConfirmations;
        if (govobj.IsCollateralValid(strError, fMissingConfirmations))
        {
            if(govobj.IsValidLocally(strError, false))
                AddGovernanceObject(govobj, connman);
            else
                LogPrintf("CGovernanceManager::CheckPostponedObjects -- %s invalid\n", nHash.ToString());

        } else if(fMissingConfirmations) {
            // wait for more confirmations
            ++it;
            continue;
        }

        // remove processed or invalid object from the queue
        mapPostponedObjects.erase(it++);
    }


    // Perform additional relays for triggers/watchdogs
    int64_t nNow = GetAdjustedTime();
    int64_t nSuperblockCycleSeconds = Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().nPowTargetSpacing;

    for(hash_s_it it = setAdditionalRelayObjects.begin(); it != setAdditionalRelayObjects.end();) {

        object_m_it itObject = mapObjects.find(*it);
        if(itObject != mapObjects.end()) {

            CGovernanceObject& govobj = itObject->second;

            int64_t nTimestamp = govobj.GetCreationTime();

            bool fValid = (nTimestamp <= nNow + MAX_TIME_FUTURE_DEVIATION) && (nTimestamp >= nNow - 2 * nSuperblockCycleSeconds);
            bool fReady = (nTimestamp <= nNow + MAX_TIME_FUTURE_DEVIATION - RELIABLE_PROPAGATION_TIME);

            if(fValid) {
                if(fReady) {
                    LogPrintf("CGovernanceManager::CheckPostponedObjects -- additional relay: hash = %s\n", govobj.GetHash().ToString());
                    govobj.Relay(connman);
                } else {
                    it++;
                    continue;
                }
            }

        } else {
            LogPrintf("CGovernanceManager::CheckPostponedObjects -- additional relay of unknown object: %s\n", it->ToString());
        }

        setAdditionalRelayObjects.erase(it++);
    }
}

void CGovernanceManager::RequestGovernanceObject(CNode* pfrom, const uint256& nHash, CConnman& connman, bool fUseFilter)
{
    if(!pfrom) {
        return;
    }

    LogPrint("gobject", "CGovernanceObject::RequestGovernanceObject -- hash = %s (peer=%d)\n", nHash.ToString(), pfrom->GetId());

    if(pfrom->nVersion < GOVERNANCE_FILTER_PROTO_VERSION) {
        connman.PushMessage(pfrom, NetMsgType::MNGOVERNANCESYNC, nHash);
        return;
    }

    CBloomFilter filter;
    filter.clear();

    int nVoteCount = 0;
    if(fUseFilter) {
        LOCK(cs);
        CGovernanceObject* pObj = FindGovernanceObject(nHash);

        if(pObj) {
            filter = CBloomFilter(Params().GetConsensus().nGovernanceFilterElements, GOVERNANCE_FILTER_FP_RATE, GetRandInt(999999), BLOOM_UPDATE_ALL);
            std::vector<CGovernanceVote> vecVotes = pObj->GetVoteFile().GetVotes();
            nVoteCount = vecVotes.size();
            for(size_t i = 0; i < vecVotes.size(); ++i) {
                filter.insert(vecVotes[i].GetHash());
            }
        }
    }

    LogPrint("gobject", "CGovernanceManager::RequestGovernanceObject -- nHash %s nVoteCount %d peer=%d\n", nHash.ToString(), nVoteCount, pfrom->id);
    connman.PushMessage(pfrom, NetMsgType::MNGOVERNANCESYNC, nHash, filter);
}

int CGovernanceManager::RequestGovernanceObjectVotes(CNode* pnode, CConnman& connman)
{
    if(pnode->nVersion < MIN_GOVERNANCE_PEER_PROTO_VERSION) return -3;
    std::vector<CNode*> vNodesCopy;
    vNodesCopy.push_back(pnode);
    return RequestGovernanceObjectVotes(vNodesCopy, connman);
}

int CGovernanceManager::RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy, CConnman& connman)
{
    static std::map<uint256, std::map<CService, int64_t> > mapAskedRecently;

    if(vNodesCopy.empty()) return -1;

    int64_t nNow = GetTime();
    int nTimeout = 60 * 60;
    size_t nPeersPerHashMax = 3;

    std::vector<CGovernanceObject*> vpGovObjsTmp;
    std::vector<CGovernanceObject*> vpGovObjsTriggersTmp;

    // This should help us to get some idea about an impact this can bring once deployed on mainnet.
    // Testnet is ~40 times smaller in masternode count, but only ~1000 masternodes usually vote,
    // so 1 obj on mainnet == ~10 objs or ~1000 votes on testnet. However we want to test a higher
    // number of votes to make sure it's robust enough, so aim at 2000 votes per masternode per request.
    // On mainnet nMaxObjRequestsPerNode is always set to 1.
    int nMaxObjRequestsPerNode = 1;
    size_t nProjectedVotes = 2000;
    if(Params().NetworkIDString() != CBaseChainParams::MAIN) {
        nMaxObjRequestsPerNode = std::max(1, int(nProjectedVotes / std::max(1, mnodeman.size())));
    }

    {
        LOCK2(cs_main, cs);

        if(mapObjects.empty()) return -2;

        for(object_m_it it = mapObjects.begin(); it != mapObjects.end(); ++it) {
            if(mapAskedRecently.count(it->first)) {
                std::map<CService, int64_t>::iterator it1 = mapAskedRecently[it->first].begin();
                while(it1 != mapAskedRecently[it->first].end()) {
                    if(it1->second < nNow) {
                        mapAskedRecently[it->first].erase(it1++);
                    } else {
                        ++it1;
                    }
                }
                if(mapAskedRecently[it->first].size() >= nPeersPerHashMax) continue;
            }
            if(it->second.nObjectType == GOVERNANCE_OBJECT_TRIGGER) {
                vpGovObjsTriggersTmp.push_back(&(it->second));
            } else {
                vpGovObjsTmp.push_back(&(it->second));
            }
        }
    }

    LogPrint("gobject", "CGovernanceManager::RequestGovernanceObjectVotes -- start: vpGovObjsTriggersTmp %d vpGovObjsTmp %d mapAskedRecently %d\n",
                vpGovObjsTriggersTmp.size(), vpGovObjsTmp.size(), mapAskedRecently.size());

    InsecureRand insecureRand;
    // shuffle pointers
    std::random_shuffle(vpGovObjsTriggersTmp.begin(), vpGovObjsTriggersTmp.end(), insecureRand);
    std::random_shuffle(vpGovObjsTmp.begin(), vpGovObjsTmp.end(), insecureRand);

    for (int i = 0; i < nMaxObjRequestsPerNode; ++i) {
        uint256 nHashGovobj;

        // ask for triggers first
        if(vpGovObjsTriggersTmp.size()) {
            nHashGovobj = vpGovObjsTriggersTmp.back()->GetHash();
        } else {
            if(vpGovObjsTmp.empty()) break;
            nHashGovobj = vpGovObjsTmp.back()->GetHash();
        }
        bool fAsked = false;
        BOOST_FOREACH(CNode* pnode, vNodesCopy) {
            // Only use regular peers, don't try to ask from outbound "masternode" connections -
            // they stay connected for a short period of time and it's possible that we won't get everything we should.
            // Only use outbound connections - inbound connection could be a "masternode" connection
            // initiated from another node, so skip it too.
            if(pnode->fMasternode || (fMasterNode && pnode->fInbound)) continue;
            // only use up to date peers
            if(pnode->nVersion < MIN_GOVERNANCE_PEER_PROTO_VERSION) continue;
            // stop early to prevent setAskFor overflow
            size_t nProjectedSize = pnode->setAskFor.size() + nProjectedVotes;
            if(nProjectedSize > SETASKFOR_MAX_SZ/2) continue;
            // to early to ask the same node
            if(mapAskedRecently[nHashGovobj].count(pnode->addr)) continue;

            RequestGovernanceObject(pnode, nHashGovobj, connman, true);
            mapAskedRecently[nHashGovobj][pnode->addr] = nNow + nTimeout;
            fAsked = true;
            // stop loop if max number of peers per obj was asked
            if(mapAskedRecently[nHashGovobj].size() >= nPeersPerHashMax) break;
        }
        // NOTE: this should match `if` above (the one before `while`)
        if(vpGovObjsTriggersTmp.size()) {
            vpGovObjsTriggersTmp.pop_back();
        } else {
            vpGovObjsTmp.pop_back();
        }
        if(!fAsked) i--;
    }
    LogPrint("gobject", "CGovernanceManager::RequestGovernanceObjectVotes -- end: vpGovObjsTriggersTmp %d vpGovObjsTmp %d mapAskedRecently %d\n",
                vpGovObjsTriggersTmp.size(), vpGovObjsTmp.size(), mapAskedRecently.size());

    return int(vpGovObjsTriggersTmp.size() + vpGovObjsTmp.size());
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
    int64_t nStart = GetTimeMillis();
    LogPrintf("Preparing masternode indexes and governance triggers...\n");
    RebuildIndexes();
    AddCachedTriggers();
    LogPrintf("Masternode indexes and governance triggers prepared  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("     %s\n", ToString());
}

std::string CGovernanceManager::ToString() const
{
    LOCK(cs);

    int nProposalCount = 0;
    int nTriggerCount = 0;
    int nWatchdogCount = 0;
    int nOtherCount = 0;

    object_m_cit it = mapObjects.begin();

    while(it != mapObjects.end()) {
        switch(it->second.GetObjectType()) {
            case GOVERNANCE_OBJECT_PROPOSAL:
                nProposalCount++;
                break;
            case GOVERNANCE_OBJECT_TRIGGER:
                nTriggerCount++;
                break;
            case GOVERNANCE_OBJECT_WATCHDOG:
                nWatchdogCount++;
                break;
            default:
                nOtherCount++;
                break;
        }
        ++it;
    }

    return strprintf("Governance Objects: %d (Proposals: %d, Triggers: %d, Watchdogs: %d/%d, Other: %d; Erased: %d), Votes: %d",
                    (int)mapObjects.size(),
                    nProposalCount, nTriggerCount, nWatchdogCount, mapWatchdogObjects.size(), nOtherCount, (int)mapErasedGovernanceObjects.size(),
                    (int)mapVoteToObject.GetSize());
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex *pindex, CConnman& connman)
{
    // Note this gets called from ActivateBestChain without cs_main being held
    // so it should be safe to lock our mutex here without risking a deadlock
    // On the other hand it should be safe for us to access pindex without holding a lock
    // on cs_main because the CBlockIndex objects are dynamically allocated and
    // presumably never deleted.
    if(!pindex) {
        return;
    }

    nCachedBlockHeight = pindex->nHeight;
    LogPrint("gobject", "CGovernanceManager::UpdatedBlockTip -- nCachedBlockHeight: %d\n", nCachedBlockHeight);

    CheckPostponedObjects(connman);
}

void CGovernanceManager::RequestOrphanObjects(CConnman& connman)
{
    std::vector<CNode*> vNodesCopy = connman.CopyNodeVector();

    std::vector<uint256> vecHashesFiltered;
    {
        std::vector<uint256> vecHashes;
        LOCK(cs);
        mapOrphanVotes.GetKeys(vecHashes);
        for(size_t i = 0; i < vecHashes.size(); ++i) {
            const uint256& nHash = vecHashes[i];
            if(mapObjects.find(nHash) == mapObjects.end()) {
                vecHashesFiltered.push_back(nHash);
            }
        }
    }

    LogPrint("gobject", "CGovernanceObject::RequestOrphanObjects -- number objects = %d\n", vecHashesFiltered.size());
    for(size_t i = 0; i < vecHashesFiltered.size(); ++i) {
        const uint256& nHash = vecHashesFiltered[i];
        for(size_t j = 0; j < vNodesCopy.size(); ++j) {
            CNode* pnode = vNodesCopy[j];
            if(pnode->fMasternode) {
                continue;
            }
            RequestGovernanceObject(pnode, nHash, connman);
        }
    }

    connman.ReleaseNodeVector(vNodesCopy);
}

void CGovernanceManager::CleanOrphanObjects()
{
    LOCK(cs);
    const vote_mcache_t::list_t& items = mapOrphanVotes.GetItemList();

    int64_t nNow = GetAdjustedTime();

    vote_mcache_t::list_cit it = items.begin();
    while(it != items.end()) {
        vote_mcache_t::list_cit prevIt = it;
        ++it;
        const vote_time_pair_t& pairVote = prevIt->value;
        if(pairVote.second < nNow) {
            mapOrphanVotes.Erase(prevIt->key, prevIt->value);
        }
    }
}
