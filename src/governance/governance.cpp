// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governance.h>

#include <common/bloom.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <flatdatabase.h>
#include <governance/governanceclasses.h>
#include <governance/governancecommon.h>
#include <governance/governancevalidators.h>
#include <masternode/masternodemeta.h>
#include <masternode/activemasternode.h>
#include <masternode/masternodesync.h>
#include <net_processing.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <protocol.h>
#include <shutdown.h>
#include <spork.h>
#include <util/time.h>
#include <validation.h>
#include <timedata.h>
std::unique_ptr<CGovernanceManager> governance;
int nSubmittedFinalBudget;

const std::string GovernanceStore::SERIALIZATION_VERSION_STRING = "CGovernanceManager-Version-16";
const int CGovernanceManager::MAX_TIME_FUTURE_DEVIATION = 60 * 60;
const int CGovernanceManager::RELIABLE_PROPAGATION_TIME = 60;

GovernanceStore::GovernanceStore() :
    cs(),
    mapObjects(),
    mapErasedGovernanceObjects(),
    cmapVoteToObject(MAX_CACHE_SIZE),
    cmapInvalidVotes(MAX_CACHE_SIZE),
    cmmapOrphanVotes(MAX_CACHE_SIZE),
    mapLastMasternodeObject(),
    lastMNListForVotingKeys(std::make_shared<CDeterministicMNList>())
{
}

CGovernanceManager::CGovernanceManager(ChainstateManager& _chainman) :
    chainman(_chainman),
    m_db{std::make_unique<db_type>("governance.dat", "magicGovernanceCache")},
    nTimeLastDiff(0),
    nCachedBlockHeight(0),
    setRequestedObjects(),
    fRateChecksEnabled(true),
    votedFundingYesTriggerHash(std::nullopt),
    mapTrigger{}
{
}

CGovernanceManager::~CGovernanceManager()
{
    if (!is_valid) return;
    m_db->Store(*this);
}

bool CGovernanceManager::LoadCache(bool load_cache)
{
    assert(m_db != nullptr);
    is_valid = load_cache ? m_db->Load(*this) : m_db->Store(*this);
    if (is_valid && load_cache) {
        CheckAndRemove();
        InitOnLoad();
    }
    return is_valid;
}

// Accessors for thread-safe access to maps
bool CGovernanceManager::HaveObjectForHash(const uint256& nHash) const
{
    LOCK(cs);
    return (mapObjects.count(nHash) == 1 || mapPostponedObjects.count(nHash) == 1);
}

bool CGovernanceManager::SerializeObjectForHash(const uint256& nHash, CDataStream& ss) const
{
    LOCK(cs);
    auto it = mapObjects.find(nHash);
    if (it == mapObjects.end()) {
        it = mapPostponedObjects.find(nHash);
        if (it == mapPostponedObjects.end())
            return false;
    }
    ss << it->second;
    return true;
}

bool CGovernanceManager::HaveVoteForHash(const uint256& nHash) const
{
    LOCK(cs);

    CGovernanceObject* pGovobj = nullptr;
    return cmapVoteToObject.Get(nHash, pGovobj) && pGovobj->GetVoteFile().HasVote(nHash);
}

int CGovernanceManager::GetVoteCount() const
{
    LOCK(cs);
    return (int)cmapVoteToObject.GetSize();
}

bool CGovernanceManager::SerializeVoteForHash(const uint256& nHash, CDataStream& ss) const
{
    LOCK(cs);

    CGovernanceObject* pGovobj = nullptr;
    return cmapVoteToObject.Get(nHash, pGovobj) && pGovobj->GetVoteFile().SerializeVoteToStream(nHash, ss);
}

void CGovernanceManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, PeerManager& peerman)
{
    if (!IsValid()) return;
    if (!masternodeSync.IsBlockchainSynced()) return;
    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    // ANOTHER USER IS ASKING US TO HELP THEM SYNC GOVERNANCE OBJECT DATA
    if (strCommand == NetMsgType::MNGOVERNANCESYNC) {
        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!masternodeSync.IsSynced()) return;

        uint256 nProp;
        CBloomFilter filter;

        vRecv >> nProp;

        vRecv >> filter;

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCESYNC -- syncing governance objects to our peer %s\n", pfrom->addr.ToStringAddr());
        if (nProp == uint256()) {
            SyncObjects(pfrom, connman, peerman);
            return;
        } else {
            SyncSingleObjVotes(pfrom, nProp, filter, connman, peerman);
        }
    }

    // A NEW GOVERNANCE OBJECT HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECT) {
        // MAKE SURE WE HAVE A VALID REFERENCE TO THE TIP BEFORE CONTINUING

        CGovernanceObject govobj;
        vRecv >> govobj;

        uint256 nHash = govobj.GetHash();
        PeerRef peer = peerman.GetPeerRef(pfrom->GetId());
        if (peer)
            peerman.AddKnownTx(*peer, nHash);
        {
            LOCK(cs_main);
            peerman.ReceivedResponse(pfrom->GetId(), nHash);
        }

        if (!masternodeSync.IsBlockchainSynced()) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode list not synced\n");
            return;
        }

        std::string strHash = nHash.ToString();

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received object: %s\n", strHash);

        if (!AcceptObjectMessage(nHash)) {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), nHash);
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received unrequested object: %s\n", strHash);
            return;
        }

        bool bReturn = false;
        {
            LOCK(cs);
            if (mapObjects.count(nHash) || mapPostponedObjects.count(nHash) || mapErasedGovernanceObjects.count(nHash)) {
                // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
                LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received already seen object: %s\n", strHash);
                bReturn = true;
            }
        }
        if(bReturn) {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), nHash);
            return;
        }

        bool fRateCheckBypassed = false;
        if (!MasternodeRateCheck(govobj, true, false, fRateCheckBypassed)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode rate check failed - %s - (current block height %d) \n", strHash, nCachedBlockHeight);
            return;
        }

        std::string strError;
        // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN

        bool fMissingConfirmations = false;
        bool fIsValid = govobj.IsValidLocally(chainman, tip_mn_list, strError, fMissingConfirmations, true);

        if (fRateCheckBypassed && fIsValid && !MasternodeRateCheck(govobj, true)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode rate check failed (after signature verification) - %s - (current block height %d)\n", strHash, nCachedBlockHeight);
            return;
        }

        if (!fIsValid) {
            if (fMissingConfirmations) {
                AddPostponedObject(govobj);
                LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Not enough fee confirmations for: %s, strError = %s\n", strHash, strError);
            } else {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), nHash);
                LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Governance object is invalid - %s\n", strError);
                // apply node's ban score
                if(peer)
                    peerman.Misbehaving(*peer, 20, "invalid governance object");
            }

            return;
        }

        AddGovernanceObject(govobj, peerman, pfrom);
    }

    // A NEW GOVERNANCE OBJECT VOTE HAS ARRIVED
    else if (strCommand == NetMsgType::MNGOVERNANCEOBJECTVOTE) {
        CGovernanceVote vote;
        vRecv >> vote;

        const uint256 &nHash = vote.GetHash();
        PeerRef peer = peerman.GetPeerRef(pfrom->GetId());
        if (peer)
            peerman.AddKnownTx(*peer, nHash);

        {
            LOCK(cs_main);
            peerman.ReceivedResponse(pfrom->GetId(), nHash);
        }

        // Ignore such messages until masternode list is synced
        if (!masternodeSync.IsBlockchainSynced()) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- masternode list not synced\n");
            return;
        }

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Received vote: %s\n", vote.ToString());

        std::string strHash = nHash.ToString();

        if (!AcceptVoteMessage(nHash)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Received unrequested vote object: %s, hash: %s, peer = %d\n",
                vote.ToString(), strHash, pfrom->GetId());
            return;
        }

        CGovernanceException exception;
        if (ProcessVote(pfrom, vote, exception, connman)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- %s new\n", strHash);
            masternodeSync.BumpAssetLastTime("MNGOVERNANCEOBJECTVOTE");
            vote.Relay(peerman, tip_mn_list);
        } else {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Rejected vote, error = %s\n", exception.what());
            if ((exception.GetNodePenalty() != 0) && masternodeSync.IsSynced()) {
                {
                    LOCK(cs_main);
                    peerman.ForgetTxHash(pfrom->GetId(), nHash);
                }
                if(peer)
                    peerman.Misbehaving(*peer, exception.GetNodePenalty(), "rejected vote");
            }
            return;
        }
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), nHash);
        }
    }
}

void CGovernanceManager::CheckOrphanVotes(CGovernanceObject& govobj, PeerManager& peerman)
{
    AssertLockHeld(cs);
    uint256 nHash = govobj.GetHash();
    std::vector<vote_time_pair_t> vecVotePairs;
    cmmapOrphanVotes.GetAll(nHash, vecVotePairs);

    ScopedLockBool guard(cs, fRateChecksEnabled, false);
    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    int64_t nNow = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
    for (const auto& pairVote : vecVotePairs) {
        bool fRemove = false;
        const CGovernanceVote& vote = pairVote.first;
        CGovernanceException e;
        if (pairVote.second < nNow) {
            fRemove = true;
        } else if (govobj.ProcessVote(tip_mn_list, vote, e)) {
            vote.Relay(peerman, tip_mn_list);
            fRemove = true;
        }
        if (fRemove) {
            cmmapOrphanVotes.Erase(nHash, pairVote);
        }
    }
}

void CGovernanceManager::AddGovernanceObject(CGovernanceObject& govobj, PeerManager& peerman, const CNode* pfrom)
{
    uint256 nHash = govobj.GetHash();
    std::string strHash = nHash.ToString();

    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANAGED DATA

    govobj.UpdateSentinelVariables(tip_mn_list); //this sets local vars in object

    std::string strError;

    // MAKE SURE THIS OBJECT IS OK

    if (!govobj.IsValidLocally(chainman, tip_mn_list, strError, true)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- invalid governance object - %s - (nCachedBlockHeight %d) \n", strError, nCachedBlockHeight);
        return;
    }
    {
        LOCK(cs);
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- Adding object: hash = %s, type = %d\n", nHash.ToString(),
                govobj.GetObjectType());

        // INSERT INTO OUR GOVERNANCE OBJECT MEMORY
        // IF WE HAVE THIS OBJECT ALREADY, WE DON'T WANT ANOTHER COPY
        auto objpair = mapObjects.emplace(nHash, govobj);

        if (!objpair.second) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- already have governance object %s\n", nHash.ToString());
            return;
        }

        // SHOULD WE ADD THIS OBJECT TO ANY OTHER MANAGERS?

        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- Before trigger block, GetDataAsPlainString = %s, nObjectType = %d\n",
                    govobj.GetDataAsPlainString(), govobj.GetObjectType());

        if (govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER && !AddNewTrigger(nHash)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- undo adding invalid trigger object: hash = %s\n", nHash.ToString());
            objpair.first->second.PrepareDeletion(GetTime<std::chrono::seconds>().count());
            return;
        }

        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- %s new, received from peer %s\n", strHash, pfrom ? pfrom->addr.ToStringAddr() : "nullptr");
        govobj.Relay(peerman);

        // Update the rate buffer
        MasternodeRateUpdate(govobj);

        masternodeSync.BumpAssetLastTime("CGovernanceManager::AddGovernanceObject");

        // WE MIGHT HAVE PENDING/ORPHAN VOTES FOR THIS OBJECT

        CheckOrphanVotes(govobj, peerman);
    }

    // SEND NOTIFICATION TO SCRIPT/ZMQ
    GetMainSignals().NotifyGovernanceObject(nHash);
}

void CGovernanceManager::CheckAndRemove()
{
    // Return on initial sync, spammed the debug.log and provided no use
    if (!masternodeSync.IsBlockchainSynced()) return;

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean\n");

    std::vector<uint256> vecDirtyHashes = mmetaman->GetAndClearDirtyGovernanceObjectHashes();
    int nHeight = WITH_LOCK(chainman.GetMutex(), return chainman.ActiveHeight());
    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    LOCK(cs);

    for (const uint256& nHash : vecDirtyHashes) {
        auto it = mapObjects.find(nHash);
        if (it == mapObjects.end()) {
            continue;
        }
        it->second.ClearMasternodeVotes(tip_mn_list);
    }

    ScopedLockBool guard(cs, fRateChecksEnabled, false);

    // Clean up any expired or invalid triggers
    CleanAndRemoveTriggers();

    auto it = mapObjects.begin();
    int64_t nNow = GetTime<std::chrono::seconds>().count();

    while (it != mapObjects.end()) {
        CGovernanceObject* pObj = &((*it).second);

        uint256 nHash = it->first;
        std::string strHash = nHash.ToString();

        // IF CACHE IS NOT DIRTY, WHY DO THIS?
        if (pObj->IsSetDirtyCache()) {
            // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
            pObj->UpdateLocalValidity(chainman, tip_mn_list);

            // UPDATE SENTINEL SIGNALING VARIABLES
            pObj->UpdateSentinelVariables(tip_mn_list);
        }

        // IF DELETE=TRUE, THEN CLEAN THE MESS UP!

        int64_t nTimeSinceDeletion = nNow - pObj->GetDeletionTime();

        LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- Checking object for deletion: %s, deletion time = %d, time since deletion = %d, delete flag = %d, expired flag = %d\n",
            strHash, pObj->GetDeletionTime(), nTimeSinceDeletion, pObj->IsSetCachedDelete(), pObj->IsSetExpired());

        if ((pObj->IsSetCachedDelete() || pObj->IsSetExpired()) &&
            (nTimeSinceDeletion >= GOVERNANCE_DELETION_DELAY)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- erase obj %s\n", (*it).first.ToString());
            mmetaman->RemoveGovernanceObject(pObj->GetHash());

            // Remove vote references
            const object_ref_cm_t::list_t& listItems = cmapVoteToObject.GetItemList();
            auto lit = listItems.begin();
            while (lit != listItems.end()) {
                if (lit->value == pObj) {
                    uint256 nKey = lit->key;
                    ++lit;
                    cmapVoteToObject.Erase(nKey);
                } else {
                    ++lit;
                }
            }

            int64_t nTimeExpired{0};

            if (pObj->GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
                // keep hashes of deleted proposals forever
                nTimeExpired = std::numeric_limits<int64_t>::max();
            } else {
                int64_t nSuperblockCycleSeconds = Params().GetConsensus().SuperBlockCycle(nHeight) * Params().GetConsensus().PowTargetSpacing(nHeight);
                nTimeExpired = pObj->GetCreationTime() + 2 * nSuperblockCycleSeconds + GOVERNANCE_DELETION_DELAY;
            }

            mapErasedGovernanceObjects.insert(std::make_pair(nHash, nTimeExpired));
            mapObjects.erase(it++);
        } else {
            if (pObj->GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
                CProposalValidator validator(pObj->GetDataAsHexString());
                if (!validator.Validate()) {
                    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- set for deletion expired obj %s\n", strHash);
                    pObj->PrepareDeletion(nNow);
                }
            }
            ++it;
        }
    }

    // forget about expired deleted objects
    auto s_it = mapErasedGovernanceObjects.begin();
    while (s_it != mapErasedGovernanceObjects.end()) {
        if (s_it->second < nNow) {
            mapErasedGovernanceObjects.erase(s_it++);
        } else {
            ++s_it;
        }
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- %s\n", ToString());
}

const CGovernanceObject* CGovernanceManager::FindConstGovernanceObject(const uint256& nHash) const
{
    LOCK(cs);

    auto it = mapObjects.find(nHash);
    if (it != mapObjects.end()) return &(it->second);

    return nullptr;
}

CGovernanceObject* CGovernanceManager::FindGovernanceObject(const uint256& nHash)
{
    LOCK(cs);

    if (mapObjects.count(nHash)) return &mapObjects[nHash];

    return nullptr;
}

CGovernanceObject* CGovernanceManager::FindGovernanceObjectByDataHash(const uint256 &nDataHash)
{
    LOCK(cs);

    for (const auto& [nHash, object] : mapObjects) {
        if (object.GetDataHash() == nDataHash) return &mapObjects[nHash];
    }

    return nullptr;
}

void CGovernanceManager::DeleteGovernanceObject(const uint256& nHash)
{
    LOCK(cs);

    if (mapObjects.count(nHash)) {
        mapObjects.erase(nHash);
    }
}

std::vector<CGovernanceVote> CGovernanceManager::GetCurrentVotes(const uint256& nParentHash, const COutPoint& mnCollateralOutpointFilter) const
{
    LOCK(cs);
    std::vector<CGovernanceVote> vecResult;

    // Find the governance object or short-circuit.
    auto it = mapObjects.find(nParentHash);
    if (it == mapObjects.end()) return vecResult;
    const CGovernanceObject& govobj = it->second;

    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    std::map<COutPoint, CDeterministicMNCPtr> mapMasternodes;
    if (mnCollateralOutpointFilter.IsNull()) {
        tip_mn_list.ForEachMNShared(false, [&](const CDeterministicMNCPtr& dmn) {
            mapMasternodes.emplace(dmn->collateralOutpoint, dmn);
        });
    } else {
        auto dmn = tip_mn_list.GetMNByCollateral(mnCollateralOutpointFilter);
        if (dmn) {
            mapMasternodes.emplace(dmn->collateralOutpoint, dmn);
        }
    }

    // Loop through each MN collateral outpoint and get the votes for the `nParentHash` governance object
    for (const auto& mnpair : mapMasternodes) {
        // get a vote_rec_t from the govobj
        vote_rec_t voteRecord;
        if (!govobj.GetCurrentMNVotes(mnpair.first, voteRecord)) continue;

        for (const auto& voteInstancePair : voteRecord.mapInstances) {
            int signal = voteInstancePair.first;
            int outcome = voteInstancePair.second.eOutcome;
            int64_t nCreationTime = voteInstancePair.second.nCreationTime;

            CGovernanceVote vote = CGovernanceVote(mnpair.first, nParentHash, (vote_signal_enum_t)signal, (vote_outcome_enum_t)outcome);
            vote.SetTime(nCreationTime);

            vecResult.push_back(vote);
        }
    }

    return vecResult;
}

void CGovernanceManager::GetAllNewerThan(std::vector<CGovernanceObject>& objs, int64_t nMoreThanTime) const
{
    LOCK(cs);

    for (const auto& objPair : mapObjects) {
        // IF THIS OBJECT IS OLDER THAN TIME, CONTINUE
        if (objPair.second.GetCreationTime() < nMoreThanTime) {
            continue;
        }

        // ADD GOVERNANCE OBJECT TO LIST
        objs.push_back(objPair.second);
    }
}

//
// Sort by votes, if there's a tie sort by their feeHash TX
//
struct sortProposalsByVotes {
    bool operator()(const std::pair<CGovernanceObject*, int>& left, const std::pair<CGovernanceObject*, int>& right) const
    {
        if (left.second != right.second) return (left.second > right.second);
        return (UintToArith256(left.first->GetCollateralHash()) > UintToArith256(right.first->GetCollateralHash()));
    }
};

std::optional<const CSuperblock> CGovernanceManager::CreateSuperblockCandidate(int nHeight) const
{
    if (!IsValid()) return std::nullopt;
    if (!masternodeSync.IsSynced()) return std::nullopt;
    if (nHeight % Params().GetConsensus().SuperBlockCycle(nHeight) < Params().GetConsensus().SuperBlockCycle(nHeight) - Params().GetConsensus().nSuperblockMaturityWindow) return std::nullopt;
    if (HasAlreadyVotedFundingTrigger()) return std::nullopt;

    // A proposal is considered passing if (YES votes) >= (Total Weight of Masternodes / 10),
    // count total valid (ENABLED) masternodes to determine passing threshold.
    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    const int nWeightedMnCount = tip_mn_list.GetValidMNsCount();
    const int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nWeightedMnCount / 10);

    // Use std::vector of std::shared_ptr<const CGovernanceObject> because CGovernanceObject doesn't support move operations (needed for sorting the vector later)
    std::vector<std::shared_ptr<const CGovernanceObject>> approvedProposals;

    {
        LOCK(cs);
        for (const auto& [unused, object] : mapObjects) {
            // Skip all non-proposals objects
            if (object.GetObjectType() != GOVERNANCE_OBJECT_PROPOSAL) continue;

            const int absYesCount = object.GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
            // Skip non-passing proposals
            if (absYesCount < nAbsVoteReq) continue;

            approvedProposals.emplace_back(std::make_shared<const CGovernanceObject>(object));
        }
    } // cs

    // Sort approved proposals by absolute Yes votes descending
    std::sort(approvedProposals.begin(), approvedProposals.end(), [](std::shared_ptr<const CGovernanceObject> a, std::shared_ptr<const CGovernanceObject> b) {
        const auto a_yes = a->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
        const auto b_yes = b->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
        return a_yes == b_yes ? UintToArith256(a->GetHash()) > UintToArith256(b->GetHash()) : a_yes > b_yes;
    });

    if (approvedProposals.empty()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s nHeight:%d empty approvedProposals\n", __func__, nHeight);
        return std::nullopt;
    }

    std::vector<CGovernancePayment> payments;
    int nLastSuperblock;
    int nNextSuperblock;

    CSuperblock::GetNearestSuperblocksHeights(nHeight, nLastSuperblock, nNextSuperblock);
    auto SBEpochTime = static_cast<int64_t>(GetTime<std::chrono::seconds>().count() + (nNextSuperblock - nHeight) * 2.5 * 60);
    auto governanceBudget = CSuperblock::GetPaymentsLimit(nNextSuperblock);

    CAmount budgetAllocated{};
    for (const auto& proposal : approvedProposals) {
        // Extract payment address and amount from proposal
        UniValue jproposal = proposal->GetJSONObject();

        CTxDestination dest = DecodeDestination(jproposal["payment_address"].getValStr());
        if (!IsValidDestination(dest)) continue;

        CAmount nAmount{};
        try {
            nAmount = ParsePaymentAmount(jproposal["payment_amount"].getValStr());
        }
        catch (const std::runtime_error& e) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s nHeight:%d Skipping payment exception:%s\n", __func__,nHeight, e.what());
            continue;
        }

        // Construct CGovernancePayment object and make sure it is valid
        CGovernancePayment payment(dest, nAmount, proposal->GetHash());
        if (!payment.IsValid()) continue;

        // Skip proposals that are too expensive
        if (budgetAllocated + payment.nAmount > governanceBudget) continue;

        int64_t windowStart = jproposal["start_epoch"].getInt<int64_t>() - GOVERNANCE_FUDGE_WINDOW;
        int64_t windowEnd = jproposal["end_epoch"].getInt<int64_t>() + GOVERNANCE_FUDGE_WINDOW;

        // Skip proposals if the SB isn't within the proposal time window
        if (SBEpochTime < windowStart) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s nHeight:%d SB:%d windowStart:%d\n", __func__,nHeight, SBEpochTime, windowStart);
            continue;
        }
        if (SBEpochTime > windowEnd) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s nHeight:%d SB:%d windowEnd:%d\n", __func__,nHeight, SBEpochTime, windowEnd);
            continue;
        }

        // Keep track of total budget allocation
        budgetAllocated += payment.nAmount;

        // Add the payment
        payments.push_back(payment);
    }

    // No proposals made the cut
    if (payments.empty()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s CreateSuperblockCandidate nHeight:%d empty payments\n", __func__, nHeight);
        return std::nullopt;
    }

    // Sort by proposal hash descending
    std::sort(payments.begin(), payments.end(), [](const CGovernancePayment& a, const CGovernancePayment& b) {
        return UintToArith256(a.proposalHash) > UintToArith256(b.proposalHash);
    });

    // Create Superblock
    return CSuperblock(nNextSuperblock, std::move(payments));
}

std::optional<const CGovernanceObject> CGovernanceManager::CreateGovernanceTrigger(const std::optional<const CSuperblock>& sb_opt, PeerManager& peerman)
{
    // no sb_opt, no trigger
    if (!sb_opt.has_value()) return std::nullopt;

    //TODO: Check if nHashParentIn, nRevision and nCollateralHashIn are correct
    //LOCK2(cs_main, cs);

    // Check if identical trigger (equal DataHash()) is already created (signed by other masternode)
    CGovernanceObject gov_sb(uint256(), 1, TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime()), uint256(), sb_opt.value().GetHexStrData());
    if (auto identical_sb = FindGovernanceObjectByDataHash(gov_sb.GetDataHash())) {
        // Somebody submitted a trigger with the same data, support it instead of submitting a duplicate
        return std::make_optional<CGovernanceObject>(*identical_sb);
    }

    // Nobody submitted a trigger we'd like to see, so let's do it but only if we are the payee
    const auto mnList = deterministicMNManager->GetListAtChainTip();
    const auto mn_payees = mnList.GetProjectedMNPayees();

    if (mn_payees.empty()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s payee list is empty\n", __func__);
        return std::nullopt;
    }

    if (mn_payees.front()->proTxHash != activeMasternodeInfo.proTxHash) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s we are not the payee, skipping\n", __func__);
        return std::nullopt;
    }
    gov_sb.SetMasternodeOutpoint(activeMasternodeInfo.outpoint);
    gov_sb.Sign();

    if (std::string strError; !gov_sb.IsValidLocally(chainman, mnList, strError, true)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Created trigger is invalid:%s\n", __func__, strError);
        return std::nullopt;
    }

    if (!MasternodeRateCheck(gov_sb)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Trigger rejected because of rate check failure hash(%s)\n", __func__, gov_sb.GetHash().ToString());
        return std::nullopt;
    }

    // The trigger we just created looks good, submit it
    AddGovernanceObject(gov_sb, peerman);
    return std::make_optional<CGovernanceObject>(gov_sb);
}

void CGovernanceManager::VoteGovernanceTriggers(const std::optional<const CGovernanceObject>& trigger_opt, CConnman& connman, PeerManager& peerman)
{
    // only active masternodes can vote on triggers
    if (activeMasternodeInfo.proTxHash.IsNull()) return;

    LOCK(cs);

    if (trigger_opt.has_value()) {
        // We should never vote "yes" on another trigger or the same trigger twice
        assert(!votedFundingYesTriggerHash.has_value());
        // Vote YES-FUNDING for the trigger we like
        const uint256 gov_sb_hash = trigger_opt.value().GetHash();
        if (!VoteFundingTrigger(gov_sb_hash, VOTE_OUTCOME_YES, connman, peerman)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting YES-FUNDING for new trigger:%s failed\n", __func__, gov_sb_hash.ToString());
            // this should never happen, bail out
            return;
        }
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting YES-FUNDING for new trigger:%s success\n", __func__, gov_sb_hash.ToString());
        votedFundingYesTriggerHash = gov_sb_hash;
    }

    // Vote NO-FUNDING for the rest of the active triggers
    const auto activeTriggers = GetActiveTriggers();
    for (const auto& trigger : activeTriggers) {
        
        const uint256 &trigger_hash = WITH_LOCK(governance->cs, return trigger->GetGovernanceObject()->GetHash());
        if (trigger->GetBlockHeight() <= nCachedBlockHeight) {
            // ignore triggers from the past
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Not voting NO-FUNDING for outdated trigger:%s\n", __func__, trigger_hash.ToString());
            continue;
        }
        if (trigger_hash == votedFundingYesTriggerHash) {
            // Skip actual trigger
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Not voting NO-FUNDING for trigger:%s, we voted yes for it already\n", __func__, trigger_hash.ToString());
            continue;
        }
        if (!VoteFundingTrigger(trigger_hash, VOTE_OUTCOME_NO, connman, peerman)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting NO-FUNDING for trigger:%s failed\n", __func__, trigger_hash.ToString());
            // failing here is ok-ish
            continue;
        }
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting NO-FUNDING for trigger:%s success\n", __func__, trigger_hash.ToString());
    }
}

bool CGovernanceManager::VoteFundingTrigger(const uint256& nHash, const vote_outcome_enum_t outcome, CConnman& connman, PeerManager& peerman)
{
    CGovernanceVote vote(activeMasternodeInfo.outpoint, nHash, VOTE_SIGNAL_FUNDING, outcome);
    vote.SetTime(TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime()));
    vote.Sign();

    CGovernanceException exception;
    if (!ProcessVoteAndRelay(vote, deterministicMNManager->GetListAtChainTip(), exception, connman, peerman)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Vote FUNDING %d for trigger:%s failed:%s\n", __func__, outcome, nHash.ToString(), exception.what());
        return false;
    }

    return true;
}

bool CGovernanceManager::HasAlreadyVotedFundingTrigger() const
{
    return votedFundingYesTriggerHash.has_value();
}

void CGovernanceManager::ResetVotedFundingTrigger()
{
    votedFundingYesTriggerHash = std::nullopt;
}

void CGovernanceManager::DoMaintenance(CConnman& connman)
{
    if (!IsValid()) return;
    if (!masternodeSync.IsSynced()) return;
    if (ShutdownRequested()) return;

    // CHECK OBJECTS WE'VE ASKED FOR, REMOVE OLD ENTRIES
    CleanOrphanObjects();
    RequestOrphanObjects(connman);

    // CHECK AND REMOVE - REPROCESS GOVERNANCE OBJECTS
    CheckAndRemove();
}

bool CGovernanceManager::ConfirmInventoryRequest(const GenTxid& gtxid)
{
    // do not request objects until it's time to sync
    if (!masternodeSync.IsBlockchainSynced()) return false;

    LOCK(cs);
    const uint32_t &type = gtxid.GetType();
    const uint256  &hash = gtxid.GetHash();
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest inv = %s-%d\n", hash.GetHex(), type);

    // First check if we've already recorded this object
    switch (type) {
    case MSG_GOVERNANCE_OBJECT: {
        if (mapObjects.count(hash) == 1 || mapPostponedObjects.count(hash) == 1) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest already have governance object, returning false\n");
            return false;
        }
        break;
    }
    case MSG_GOVERNANCE_OBJECT_VOTE: {
        if (cmapVoteToObject.HasKey(hash)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest already have governance vote, returning false\n");
            return false;
        }
        break;
    }
    default:
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest unknown type, returning false\n");
        return false;
    }


    hash_s_t* setHash = nullptr;
    switch (type) {
    case MSG_GOVERNANCE_OBJECT:
        setHash = &setRequestedObjects;
        break;
    case MSG_GOVERNANCE_OBJECT_VOTE:
        setHash = &setRequestedVotes;
        break;
    default:
        return false;
    }

    const auto& [_itr, inserted] = setHash->insert(hash);

    if (inserted) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest added inv to requested set\n");
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest reached end, returning true\n");
    return true;
}

void CGovernanceManager::SyncSingleObjVotes(CNode* pnode, const uint256& nProp, const CBloomFilter& filter, CConnman& connman, PeerManager& peerman)
{
    // do not provide any data until our node is synced
    if (!masternodeSync.IsSynced()) return;

    int nVoteCount = 0;

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- syncing single object to peer=%d, nProp = %s\n", __func__, nProp.ToString(), pnode->GetId());

    LOCK(cs);

    // single valid object and its valid votes
    auto it = mapObjects.find(nProp);
    if (it == mapObjects.end()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- no matching object for hash %s, peer=%d\n", __func__, nProp.ToString(), pnode->GetId());
        return;
    }
    const CGovernanceObject& govobj = it->second;
    std::string strHash = it->first.ToString();

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- attempting to sync govobj: %s, peer=%d\n", __func__, strHash, pnode->GetId());

    if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- not syncing deleted/expired govobj: %s, peer=%d\n", __func__,
            strHash, pnode->GetId());
        return;
    }

    const auto& fileVotes = govobj.GetVoteFile();
    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();

    for (const auto& vote : fileVotes.GetVotes()) {
        const uint256 &nVoteHash = vote.GetHash();

        bool onlyVotingKeyAllowed = govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

        if (filter.contains(nVoteHash) || !vote.IsValid(tip_mn_list, onlyVotingKeyAllowed)) {
            continue;
        }
        PeerRef peer = peerman.GetPeerRef(pnode->GetId());
        if(peer) {
            LOCK(cs_main);
            peerman.PushTxInventoryOther(*peer, CInv(MSG_GOVERNANCE_OBJECT_VOTE, nVoteHash));
        }
        ++nVoteCount;
    }

    CNetMsgMaker msgMaker(pnode->GetCommonVersion());
    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ_VOTE, nVoteCount));
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- sent %d votes to peer=%d\n", __func__, nVoteCount, pnode->GetId());
}

void CGovernanceManager::SyncObjects(CNode* pnode, CConnman& connman, PeerManager& peerman) const
{
    if (!masternodeSync.IsSynced()) return;
    PeerRef peer = peerman.GetPeerRef(pnode->GetId());
    if (netfulfilledman->HasFulfilledRequest(pnode->addr, NetMsgType::MNGOVERNANCESYNC)) {
        // Asking for the whole list multiple times in a short period of time is no good
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- peer already asked me for the list\n", __func__);
        if(peer)
            peerman.Misbehaving(*peer, 20, "peer already asked for list");
        return;
    }
    netfulfilledman->AddFulfilledRequest(pnode->addr, NetMsgType::MNGOVERNANCESYNC);

    int nObjCount = 0;

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- syncing all objects to peer=%d\n", __func__, pnode->GetId());

    LOCK(cs);

    // all valid objects, no votes
    for (const auto& objPair : mapObjects) {
        uint256 nHash = objPair.first;
        const CGovernanceObject& govobj = objPair.second;
        std::string strHash = nHash.ToString();

        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- attempting to sync govobj: %s, peer=%d\n", __func__, strHash, pnode->GetId());

        if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- not syncing deleted/expired govobj: %s, peer=%d\n", __func__,
                strHash, pnode->GetId());
            continue;
        }

        // Push the inventory budget proposal message over to the other client
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- syncing govobj: %s, peer=%d\n", __func__, strHash, pnode->GetId());
        if(peer) {
            LOCK(cs_main);
            peerman.PushTxInventoryOther(*peer, CInv(MSG_GOVERNANCE_OBJECT, nHash));
        }
        ++nObjCount;
    }

    CNetMsgMaker msgMaker(pnode->GetCommonVersion());
    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ, nObjCount));
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- sent %d objects to peer=%d\n", __func__, nObjCount, pnode->GetId());
}

void CGovernanceManager::MasternodeRateUpdate(const CGovernanceObject& govobj)
{
    if (govobj.GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) return;

    const COutPoint& masternodeOutpoint = govobj.GetMasternodeOutpoint();
    auto it = mapLastMasternodeObject.find(masternodeOutpoint);

    if (it == mapLastMasternodeObject.end()) {
        it = mapLastMasternodeObject.insert(txout_m_t::value_type(masternodeOutpoint, last_object_rec(true))).first;
    }

    int64_t nTimestamp = govobj.GetCreationTime();
    it->second.triggerBuffer.AddTimestamp(nTimestamp);

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
    int nHeight = WITH_LOCK(chainman.GetMutex(), return chainman.ActiveHeight());
    LOCK(cs);

    fRateCheckBypassed = false;

    if (!masternodeSync.IsSynced() || !fRateChecksEnabled) {
        return true;
    }

    if (govobj.GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) {
        return true;
    }

    const COutPoint& masternodeOutpoint = govobj.GetMasternodeOutpoint();
    int64_t nTimestamp = govobj.GetCreationTime();
    int64_t nNow = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
    int64_t nSuperblockCycleSeconds = Params().GetConsensus().SuperBlockCycle(nHeight) * Params().GetConsensus().PowTargetSpacing(nHeight);

    std::string strHash = govobj.GetHash().ToString();

    if (nTimestamp < nNow - 2 * nSuperblockCycleSeconds) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::MasternodeRateCheck -- object %s rejected due to too old timestamp, masternode = %s, timestamp = %d, current time = %d\n",
            strHash, masternodeOutpoint.ToStringShort(), nTimestamp, nNow);
        return false;
    }

    if (nTimestamp > nNow + MAX_TIME_FUTURE_DEVIATION) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::MasternodeRateCheck -- object %s rejected due to too new (future) timestamp, masternode = %s, timestamp = %d, current time = %d\n",
            strHash, masternodeOutpoint.ToStringShort(), nTimestamp, nNow);
        return false;
    }

    auto it = mapLastMasternodeObject.find(masternodeOutpoint);
    if (it == mapLastMasternodeObject.end()) return true;

    if (it->second.fStatusOK && !fForce) {
        fRateCheckBypassed = true;
        return true;
    }

    // Allow 1 trigger per mn per cycle, with a small fudge factor
    double dMaxRate = 2 * 1.1 / double(nSuperblockCycleSeconds);

    // Temporary copy to check rate after new timestamp is added
    CRateCheckBuffer buffer = it->second.triggerBuffer;

    buffer.AddTimestamp(nTimestamp);
    double dRate = buffer.GetRate();

    if (dRate < dMaxRate) {
        return true;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::MasternodeRateCheck -- Rate too high: object hash = %s, masternode = %s, object timestamp = %d, rate = %f, max rate = %f\n",
        strHash, masternodeOutpoint.ToStringShort(), nTimestamp, dRate, dMaxRate);

    if (fUpdateFailStatus) {
        it->second.fStatusOK = false;
    }

    return false;
}

bool CGovernanceManager::ProcessVoteAndRelay(const CGovernanceVote& vote, const CDeterministicMNList& mnList, CGovernanceException& exception, CConnman& connman, PeerManager& peerman)
{
    bool fOK = ProcessVote(/* pfrom = */ nullptr, vote, exception, connman);
    if (fOK) {
        vote.Relay(peerman, mnList);
    }
    return fOK;
}

bool CGovernanceManager::ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman)
{
    ENTER_CRITICAL_SECTION(cs);
    const uint256 &nHashVote = vote.GetHash();
    const uint256 &nHashGovobj = vote.GetParentHash();

    if (cmapVoteToObject.HasKey(nHashVote)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::ProcessVote -- skipping known valid vote %s for object %s\n", nHashVote.ToString(), nHashGovobj.ToString());
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    if (cmapInvalidVotes.HasKey(nHashVote)) {
        std::ostringstream ostr;
        ostr << "CGovernanceManager::ProcessVote -- Old invalid vote "
             << ", MN outpoint = " << vote.GetMasternodeOutpoint().ToStringShort()
             << ", governance object hash = " << nHashGovobj.ToString();
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    auto it = mapObjects.find(nHashGovobj);
    if (it == mapObjects.end()) {
        std::ostringstream ostr;
        ostr << "CGovernanceManager::ProcessVote -- Unknown parent object " << nHashGovobj.ToString()
             << ", MN outpoint = " << vote.GetMasternodeOutpoint().ToStringShort();
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_WARNING);
        if (cmmapOrphanVotes.Insert(nHashGovobj, vote_time_pair_t(vote, GetTime<std::chrono::seconds>().count() + GOVERNANCE_ORPHAN_EXPIRATION_TIME))) {
            LEAVE_CRITICAL_SECTION(cs);
            RequestGovernanceObject(pfrom, nHashGovobj, connman);
            LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
            return false;
        }

        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    CGovernanceObject& govobj = it->second;

    if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::ProcessVote -- ignoring vote for expired or deleted object, hash = %s\n", nHashGovobj.ToString());
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    bool fOk = govobj.ProcessVote(deterministicMNManager->GetListAtChainTip(), vote, exception) && cmapVoteToObject.Insert(nHashVote, &govobj);
    LEAVE_CRITICAL_SECTION(cs);
    return fOk;
}

void CGovernanceManager::CheckPostponedObjects(PeerManager& peerman)
{
    if (!masternodeSync.IsSynced()) return;

    LOCK2(cs_main, cs);
    const auto mnList = deterministicMNManager->GetListAtChainTip();
    // Check postponed proposals
    for (auto it = mapPostponedObjects.begin(); it != mapPostponedObjects.end();) {
        const uint256& nHash = it->first;
        CGovernanceObject& govobj = it->second;

        assert(govobj.GetObjectType() != GOVERNANCE_OBJECT_TRIGGER);

        std::string strError;
        bool fMissingConfirmations;
        if (govobj.IsCollateralValid(chainman, strError, fMissingConfirmations)) {
            if (govobj.IsValidLocally(chainman, mnList, strError, false)) {
                AddGovernanceObject(govobj, peerman);
            } else {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::CheckPostponedObjects -- %s invalid\n", nHash.ToString());
            }

        } else if (fMissingConfirmations) {
            // wait for more confirmations
            ++it;
            continue;
        }

        // remove processed or invalid object from the queue
        mapPostponedObjects.erase(it++);
    }


    // Perform additional relays for triggers
    int64_t nNow = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
    int nHeight = chainman.ActiveHeight();
    int64_t nSuperblockCycleSeconds = Params().GetConsensus().SuperBlockCycle(nHeight) * Params().GetConsensus().PowTargetSpacing(nHeight);

    for (auto it = setAdditionalRelayObjects.begin(); it != setAdditionalRelayObjects.end();) {
        auto itObject = mapObjects.find(*it);
        if (itObject != mapObjects.end()) {
            const CGovernanceObject& govobj = itObject->second;

            int64_t nTimestamp = govobj.GetCreationTime();

            bool fValid = (nTimestamp <= nNow + MAX_TIME_FUTURE_DEVIATION) && (nTimestamp >= nNow - 2 * nSuperblockCycleSeconds);
            bool fReady = (nTimestamp <= nNow + MAX_TIME_FUTURE_DEVIATION - RELIABLE_PROPAGATION_TIME);

            if (fValid) {
                if (fReady) {
                    LogPrint(BCLog::GOBJECT, "CGovernanceManager::CheckPostponedObjects -- additional relay: hash = %s\n", govobj.GetHash().ToString());
                    govobj.Relay(peerman);
                } else {
                    it++;
                    continue;
                }
            }

        } else {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::CheckPostponedObjects -- additional relay of unknown object: %s\n", it->ToString());
        }

        setAdditionalRelayObjects.erase(it++);
    }
}

void CGovernanceManager::RequestGovernanceObject(CNode* pfrom, const uint256& nHash, CConnman& connman, bool fUseFilter) const
{
    if (!pfrom) {
        return;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::RequestGovernanceObject -- nHash %s peer=%d\n", nHash.ToString(), pfrom->GetId());

    CNetMsgMaker msgMaker(pfrom->GetCommonVersion());

    CBloomFilter filter;

    size_t nVoteCount = 0;
    if (fUseFilter) {
        LOCK(cs);
        const CGovernanceObject* pObj = FindConstGovernanceObject(nHash);

        if (pObj) {
            filter = CBloomFilter(Params().GetConsensus().nGovernanceFilterElements, GOVERNANCE_FILTER_FP_RATE, GetRand(999999), BLOOM_UPDATE_ALL);
            std::vector<CGovernanceVote> vecVotes = pObj->GetVoteFile().GetVotes();
            nVoteCount = vecVotes.size();
            for (const auto& vote : vecVotes) {
                filter.insert(vote.GetHash());
            }
        }
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::RequestGovernanceObject -- nHash %s nVoteCount %d peer=%d\n", nHash.ToString(), nVoteCount, pfrom->GetId());
    connman.PushMessage(pfrom, msgMaker.Make(NetMsgType::MNGOVERNANCESYNC, nHash, filter));
}

int CGovernanceManager::RequestGovernanceObjectVotes(CNode* pnode, CConnman& connman, const PeerManager& peerman) const
{
    const std::vector<CNode*> vNodeCopy{pnode};
    return RequestGovernanceObjectVotes(vNodeCopy, connman, peerman);
}

int CGovernanceManager::RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy, CConnman& connman, const PeerManager& peerman) const
{
    static std::map<uint256, std::map<CService, int64_t> > mapAskedRecently;

    if (vNodesCopy.empty()) return -1;

    int64_t nNow = GetTime();
    int nTimeout = 60 * 60;
    size_t nPeersPerHashMax = 3;

    std::vector<uint256> vTriggerObjHashes;
    std::vector<uint256> vOtherObjHashes;

    // This should help us to get some idea about an impact this can bring once deployed on mainnet.
    // Testnet is ~40 times smaller in masternode count, but only ~1000 masternodes usually vote,
    // so 1 obj on mainnet == ~10 objs or ~1000 votes on testnet. However we want to test a higher
    // number of votes to make sure it's robust enough, so aim at 2000 votes per masternode per request.
    // On mainnet nMaxObjRequestsPerNode is always set to 1.
    int nMaxObjRequestsPerNode = 1;
    size_t nProjectedVotes = 2000;
    if (Params().GetChainType() != ChainType::MAIN) {
        nMaxObjRequestsPerNode = std::max(1, int(nProjectedVotes / std::max(1, (int)deterministicMNManager->GetListAtChainTip().GetValidMNsCount())));
    }

    {
        LOCK(cs);

        if (mapObjects.empty()) return -2;

        for (const auto& [nHash, govobj] : mapObjects) {
            if (govobj.IsSetCachedDelete()) continue;
            if (mapAskedRecently.count(nHash)) {
                auto it = mapAskedRecently[nHash].begin();
                while (it != mapAskedRecently[nHash].end()) {
                    if (it->second < nNow) {
                        mapAskedRecently[nHash].erase(it++);
                    } else {
                        ++it;
                    }
                }
                if (mapAskedRecently[nHash].size() >= nPeersPerHashMax) continue;
            }

            if (govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) {
                vTriggerObjHashes.push_back(nHash);
            } else {
                vOtherObjHashes.push_back(nHash);
            }
        }
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::RequestGovernanceObjectVotes -- start: vTriggerObjHashes %d vOtherObjHashes %d mapAskedRecently %d\n",
        vTriggerObjHashes.size(), vOtherObjHashes.size(), mapAskedRecently.size());

    Shuffle(vTriggerObjHashes.begin(), vTriggerObjHashes.end(), FastRandomContext());
    Shuffle(vOtherObjHashes.begin(), vOtherObjHashes.end(), FastRandomContext());

    for (int i = 0; i < nMaxObjRequestsPerNode; ++i) {
        uint256 nHashGovobj;

        // ask for triggers first
        if (!vTriggerObjHashes.empty()) {
            nHashGovobj = vTriggerObjHashes.back();
        } else {
            if (vOtherObjHashes.empty()) break;
            nHashGovobj = vOtherObjHashes.back();
        }
        bool fAsked = false;
        for (const auto& pnode : vNodesCopy) {
            // Don't try to sync any data from outbound non-relay "masternode" connections.
            // Inbound connection this early is most likely a "masternode" connection
            // initiated from another node, so skip it too.
            if (!pnode->CanRelay() || (fMasternodeMode && pnode->IsInboundConn())) continue;
            // stop early to prevent setAskFor overflow
            {
                LOCK(cs_main);
                size_t nProjectedSize = peerman.GetRequestedCount(pnode->GetId()) + nProjectedVotes;
                if (nProjectedSize > GetMaxInv()) continue;
                // to early to ask the same node
                if (mapAskedRecently[nHashGovobj].count(pnode->addr)) continue;
            }

            RequestGovernanceObject(pnode, nHashGovobj, connman, true);
            mapAskedRecently[nHashGovobj][pnode->addr] = nNow + nTimeout;
            fAsked = true;
            // stop loop if max number of peers per obj was asked
            if (mapAskedRecently[nHashGovobj].size() >= nPeersPerHashMax) break;
        }
        // NOTE: this should match `if` above (the one before `while`)
        if (!vTriggerObjHashes.empty()) {
            vTriggerObjHashes.pop_back();
        } else {
            vOtherObjHashes.pop_back();
        }
        if (!fAsked) i--;
    }
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::RequestGovernanceObjectVotes -- end: vTriggerObjHashes %d vOtherObjHashes %d mapAskedRecently %d\n",
        vTriggerObjHashes.size(), vOtherObjHashes.size(), mapAskedRecently.size());

    return int(vTriggerObjHashes.size() + vOtherObjHashes.size());
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
    auto it = setHash.find(nHash);
    if (it == setHash.end()) {
        // We never requested this
        return false;
    }
    // Only accept one response
    setHash.erase(it);
    return true;
}

void CGovernanceManager::RebuildIndexes()
{
    LOCK(cs);

    cmapVoteToObject.Clear();
    for (auto& objPair : mapObjects) {
        CGovernanceObject& govobj = objPair.second;
        std::vector<CGovernanceVote> vecVotes = govobj.GetVoteFile().GetVotes();
        for (const auto& vecVote : vecVotes) {
            cmapVoteToObject.Insert(vecVote.GetHash(), &govobj);
        }
    }
}

void CGovernanceManager::AddCachedTriggers()
{
    LOCK(cs);

    int64_t nNow = GetTime<std::chrono::seconds>().count();

    for (auto& objpair : mapObjects) {
        CGovernanceObject& govobj = objpair.second;

        if (govobj.GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) {
            continue;
        }

        if (!AddNewTrigger(govobj.GetHash())) {
            govobj.PrepareDeletion(nNow);
        }
    }
}

void CGovernanceManager::InitOnLoad()
{
    LOCK(cs);
    int64_t nStart = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    LogPrintf("Preparing masternode indexes and governance triggers...\n");
    RebuildIndexes();
    AddCachedTriggers();
    LogPrintf("Masternode indexes and governance triggers prepared  %dms\n", TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStart);
    LogPrintf("     %s\n", ToString());
}

void GovernanceStore::Clear()
{
    LOCK(cs);

    LogPrint(BCLog::GOBJECT, "Governance object manager was cleared\n");
    mapObjects.clear();
    mapErasedGovernanceObjects.clear();
    cmapVoteToObject.Clear();
    cmapInvalidVotes.Clear();
    cmmapOrphanVotes.Clear();
    mapLastMasternodeObject.clear();
}

std::string GovernanceStore::ToString() const
{
    LOCK(cs);

    int nProposalCount = 0;
    int nTriggerCount = 0;
    int nOtherCount = 0;

    for (const auto& objPair : mapObjects) {
        switch (objPair.second.GetObjectType()) {
        case GOVERNANCE_OBJECT_PROPOSAL:
            nProposalCount++;
            break;
        case GOVERNANCE_OBJECT_TRIGGER:
            nTriggerCount++;
            break;
        default:
            nOtherCount++;
            break;
        }
    }

    return strprintf("Governance Objects: %d (Proposals: %d, Triggers: %d, Other: %d; Erased: %d), Votes: %d",
        (int)mapObjects.size(),
        nProposalCount, nTriggerCount, nOtherCount, (int)mapErasedGovernanceObjects.size(),
        (int)cmapVoteToObject.GetSize());
}

UniValue CGovernanceManager::ToJson() const
{
    LOCK(cs);

    int nProposalCount = 0;
    int nTriggerCount = 0;
    int nOtherCount = 0;

    for (const auto& objpair : mapObjects) {
        switch (objpair.second.GetObjectType()) {
        case GOVERNANCE_OBJECT_PROPOSAL:
            nProposalCount++;
            break;
        case GOVERNANCE_OBJECT_TRIGGER:
            nTriggerCount++;
            break;
        default:
            nOtherCount++;
            break;
        }
    }

    UniValue jsonObj(UniValue::VOBJ);
    jsonObj.pushKV("objects_total", (int)mapObjects.size());
    jsonObj.pushKV("proposals", nProposalCount);
    jsonObj.pushKV("triggers", nTriggerCount);
    jsonObj.pushKV("other", nOtherCount);
    jsonObj.pushKV("erased", (int)mapErasedGovernanceObjects.size());
    jsonObj.pushKV("votes", (int)cmapVoteToObject.GetSize());
    return jsonObj;
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex* pindex, CConnman& connman, PeerManager& peerman)
{
    // Note this gets called from ActivateBestChain without cs_main being held
    // so it should be safe to lock our mutex here without risking a deadlock
    // On the other hand it should be safe for us to access pindex without holding a lock
    // on cs_main because the CBlockIndex objects are dynamically allocated and
    // presumably never deleted.
    if (!pindex) {
        return;
    }

    if (!activeMasternodeInfo.proTxHash.IsNull()) {
        const auto sb_opt = CreateSuperblockCandidate(pindex->nHeight);
        const auto trigger_opt = CreateGovernanceTrigger(sb_opt, peerman);
        VoteGovernanceTriggers(trigger_opt, connman, peerman);
    }

    nCachedBlockHeight = pindex->nHeight;
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdatedBlockTip -- nCachedBlockHeight: %d\n", nCachedBlockHeight);

    if (deterministicMNManager && deterministicMNManager->IsDIP3Enforced(pindex->nHeight)) {
        RemoveInvalidVotes();
    }

    CheckPostponedObjects(peerman);

    CSuperblockManager::ExecuteBestSuperblock(pindex->nHeight);
}

void CGovernanceManager::RequestOrphanObjects(CConnman& connman)
{
    const CConnman::NodesSnapshot snap{connman, /* filter = */ FullyConnectedOnly};

    std::vector<uint256> vecHashesFiltered;
    {
        std::vector<uint256> vecHashes;
        LOCK(cs);
        cmmapOrphanVotes.GetKeys(vecHashes);
        for (const uint256& nHash : vecHashes) {
            if (mapObjects.find(nHash) == mapObjects.end()) {
                vecHashesFiltered.push_back(nHash);
            }
        }
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceObject::RequestOrphanObjects -- number objects = %d\n", vecHashesFiltered.size());
    for (const uint256& nHash : vecHashesFiltered) {
        for (CNode* pnode : snap.Nodes()) {
            if (!pnode->CanRelay()) {
                continue;
            }
            RequestGovernanceObject(pnode, nHash, connman);
        }
    }
}

void CGovernanceManager::CleanOrphanObjects()
{
    LOCK(cs);
    const vote_cmm_t::list_t& items = cmmapOrphanVotes.GetItemList();

    int64_t nNow = GetTime<std::chrono::seconds>().count();

    auto it = items.begin();
    while (it != items.end()) {
        auto prevIt = it;
        ++it;
        const vote_time_pair_t& pairVote = prevIt->value;
        if (pairVote.second < nNow) {
            cmmapOrphanVotes.Erase(prevIt->key, prevIt->value);
        }
    }
}

void CGovernanceManager::RemoveInvalidVotes()
{
    if (!masternodeSync.IsSynced()) {
        return;
    }

    LOCK(cs);

    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    auto diff = lastMNListForVotingKeys->BuildDiff(tip_mn_list);

    std::vector<COutPoint> changedKeyMNs;
    for (const auto& p : diff.updatedMNs) {
        auto oldDmn = lastMNListForVotingKeys->GetMNByInternalId(p.first);
        if ((p.second.fields & CDeterministicMNStateDiff::Field_keyIDVoting) && p.second.state.keyIDVoting != oldDmn->pdmnState->keyIDVoting) {
            changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
        } else if ((p.second.fields & CDeterministicMNStateDiff::Field_pubKeyOperator) && p.second.state.pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
            changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
        }
    }
    for (const auto& id : diff.removedMns) {
        auto oldDmn = lastMNListForVotingKeys->GetMNByInternalId(id);
        changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
    }

    for (const auto& outpoint : changedKeyMNs) {
        for (auto& p : mapObjects) {
            auto removed = p.second.RemoveInvalidVotes(tip_mn_list, outpoint);
            if (removed.empty()) {
                continue;
            }
            for (auto& voteHash : removed) {
                cmapVoteToObject.Erase(voteHash);
                cmapInvalidVotes.Erase(voteHash);
                cmmapOrphanVotes.Erase(voteHash);
                setRequestedVotes.erase(voteHash);
            }
        }
    }

    // store current MN list for the next run so that we can determine which keys changed
    lastMNListForVotingKeys = std::make_shared<CDeterministicMNList>(tip_mn_list);
}

bool AreSuperblocksEnabled()
{
    return sporkManager->IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED);
}
