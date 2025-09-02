// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governance.h>

#include <chain.h>
#include <chainparams.h>
#include <common/bloom.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <flat-database.h>
#include <governance/classes.h>
#include <governance/common.h>
#include <governance/validators.h>
#include <masternode/meta.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <net_processing.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <protocol.h>
#include <shutdown.h>
#include <spork.h>
#include <timedata.h>
#include <util/ranges.h>
#include <util/time.h>
#include <validation.h>

int nSubmittedFinalBudget;

const std::string GovernanceStore::SERIALIZATION_VERSION_STRING = "CGovernanceManager-Version-16";
const int CGovernanceManager::MAX_TIME_FUTURE_DEVIATION = 60 * 60;
const int CGovernanceManager::RELIABLE_PROPAGATION_TIME = 60;

namespace {
class ScopedLockBool
{
    bool& ref;
    bool fPrevValue;

public:
    ScopedLockBool(RecursiveMutex& _cs, bool& _ref, bool _value) :
        ref(_ref)
    {
        AssertLockHeld(_cs);
        fPrevValue = ref;
        ref = _value;
    }

    ~ScopedLockBool() { ref = fPrevValue; }
};
} // anonymous namespace

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

CGovernanceManager::CGovernanceManager(CMasternodeMetaMan& mn_metaman, CNetFulfilledRequestManager& netfulfilledman,
                                       const ChainstateManager& chainman,
                                       const std::unique_ptr<CDeterministicMNManager>& dmnman, CMasternodeSync& mn_sync) :
    m_db{std::make_unique<db_type>("governance.dat", "magicGovernanceCache")},
    m_mn_metaman{mn_metaman},
    m_netfulfilledman{netfulfilledman},
    m_chainman{chainman},
    m_dmnman{dmnman},
    m_mn_sync{mn_sync},
    nTimeLastDiff(0),
    nCachedBlockHeight(0),
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

MessageProcessingResult CGovernanceManager::ProcessMessage(CNode& peer, CConnman& connman, PeerManager& peerman, std::string_view msg_type, CDataStream& vRecv)
{
    if (!IsValid()) return {};
    if (!m_mn_sync.IsBlockchainSynced()) return {};

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    // ANOTHER USER IS ASKING US TO HELP THEM SYNC GOVERNANCE OBJECT DATA
    if (msg_type == NetMsgType::MNGOVERNANCESYNC) {
        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!m_mn_sync.IsSynced()) return {};

        uint256 nProp;
        CBloomFilter filter;
        vRecv >> nProp;
        vRecv >> filter;

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCESYNC -- syncing governance objects to our peer %s\n", peer.GetLogString());
        if (nProp == uint256()) {
            return SyncObjects(peer, connman);
        } else {
            return SyncSingleObjVotes(peer, nProp, filter, connman);
        }

        return {};
    }

    // A NEW GOVERNANCE OBJECT HAS ARRIVED
    else if (msg_type == NetMsgType::MNGOVERNANCEOBJECT) {
        // MAKE SURE WE HAVE A VALID REFERENCE TO THE TIP BEFORE CONTINUING

        CGovernanceObject govobj;
        vRecv >> govobj;

        uint256 nHash = govobj.GetHash();

        MessageProcessingResult ret{};
        ret.m_to_erase = CInv{MSG_GOVERNANCE_OBJECT, nHash};

        if (!m_mn_sync.IsBlockchainSynced()) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode list not synced\n");
            return ret;
        }

        std::string strHash = nHash.ToString();

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received object: %s\n", strHash);

        if (!AcceptMessage(nHash)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received unrequested object: %s\n", strHash);
            return ret;
        }

        LOCK2(::cs_main, cs);

        if (mapObjects.count(nHash) || mapPostponedObjects.count(nHash) || mapErasedGovernanceObjects.count(nHash)) {
            // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received already seen object: %s\n", strHash);
            return ret;
        }

        bool fRateCheckBypassed = false;
        if (!MasternodeRateCheck(govobj, true, false, fRateCheckBypassed)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode rate check failed - %s - (current block height %d) \n", strHash, nCachedBlockHeight);
            return ret;
        }

        std::string strError;
        // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN

        bool fMissingConfirmations = false;
        bool fIsValid = govobj.IsValidLocally(tip_mn_list, m_chainman, strError, fMissingConfirmations, true);

        if (fRateCheckBypassed && fIsValid && !MasternodeRateCheck(govobj, true)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode rate check failed (after signature verification) - %s - (current block height %d)\n", strHash, nCachedBlockHeight);
            return ret;
        }

        if (!fIsValid) {
            if (fMissingConfirmations) {
                AddPostponedObject(govobj);
                LogPrintf("MNGOVERNANCEOBJECT -- Not enough fee confirmations for: %s, strError = %s\n", strHash, strError);
            } else {
                LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Governance object is invalid - %s\n", strError);
                // apply node's ban score
                ret.m_error = MisbehavingError{20};
                return ret;
            }

            return ret;
        }

        AddGovernanceObject(govobj, peerman, &peer);
        return ret;
    }

    // A NEW GOVERNANCE OBJECT VOTE HAS ARRIVED
    else if (msg_type == NetMsgType::MNGOVERNANCEOBJECTVOTE) {
        CGovernanceVote vote;
        vRecv >> vote;

        uint256 nHash = vote.GetHash();

        MessageProcessingResult ret{};
        ret.m_to_erase = CInv{MSG_GOVERNANCE_OBJECT_VOTE, nHash};

        // Ignore such messages until masternode list is synced
        if (!m_mn_sync.IsBlockchainSynced()) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- masternode list not synced\n");
            return ret;
        }

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Received vote: %s\n", vote.ToString(tip_mn_list));

        std::string strHash = nHash.ToString();

        if (!AcceptMessage(nHash)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Received unrequested vote object: %s, hash: %s, peer = %d\n",
                vote.ToString(tip_mn_list), strHash, peer.GetId());
            return ret;
        }

        CGovernanceException exception;
        if (ProcessVote(&peer, vote, exception, connman)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- %s new\n", strHash);
            m_mn_sync.BumpAssetLastTime("MNGOVERNANCEOBJECTVOTE");
            vote.Relay(peerman, m_mn_sync, tip_mn_list);
        } else {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Rejected vote, error = %s\n", exception.what());
            if ((exception.GetNodePenalty() != 0) && m_mn_sync.IsSynced()) {
                ret.m_error = MisbehavingError{exception.GetNodePenalty()};
                return ret;
            }
            return ret;
        }
        return ret;
    }

    return {};
}

void CGovernanceManager::CheckOrphanVotes(CGovernanceObject& govobj, PeerManager& peerman)
{
    uint256 nHash = govobj.GetHash();
    std::vector<vote_time_pair_t> vecVotePairs;
    cmmapOrphanVotes.GetAll(nHash, vecVotePairs);

    ScopedLockBool guard(cs, fRateChecksEnabled, false);

    int64_t nNow = GetAdjustedTime();
    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    for (const auto& pairVote : vecVotePairs) {
        bool fRemove = false;
        const CGovernanceVote& vote = pairVote.first;
        CGovernanceException e;
        if (pairVote.second < nNow) {
            fRemove = true;
        } else if (govobj.ProcessVote(m_mn_metaman, *this, tip_mn_list, vote, e)) {
            vote.Relay(peerman, m_mn_sync, tip_mn_list);
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

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();

    // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANAGED DATA

    govobj.UpdateSentinelVariables(tip_mn_list); //this sets local vars in object

    LOCK2(::cs_main, cs);
    std::string strError;

    // MAKE SURE THIS OBJECT IS OK

    if (!govobj.IsValidLocally(tip_mn_list, m_chainman, strError, true)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- invalid governance object - %s - (nCachedBlockHeight %d) \n", strError, nCachedBlockHeight);
        return;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- Adding object: hash = %s, type = %d\n", nHash.ToString(),
             ToUnderlying(govobj.GetObjectType()));

    // INSERT INTO OUR GOVERNANCE OBJECT MEMORY
    // IF WE HAVE THIS OBJECT ALREADY, WE DON'T WANT ANOTHER COPY
    auto objpair = mapObjects.emplace(nHash, govobj);

    if (!objpair.second) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- already have governance object %s\n", nHash.ToString());
        return;
    }

    // SHOULD WE ADD THIS OBJECT TO ANY OTHER MANAGERS?

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- Before trigger block, GetDataAsPlainString = %s, nObjectType = %d\n",
                govobj.GetDataAsPlainString(), ToUnderlying(govobj.GetObjectType()));

    if (govobj.GetObjectType() == GovernanceObject::TRIGGER && !AddNewTrigger(nHash)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- undo adding invalid trigger object: hash = %s\n", nHash.ToString());
        objpair.first->second.PrepareDeletion(GetTime<std::chrono::seconds>().count());
        return;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- %s new, received from peer %s\n", strHash, pfrom ? pfrom->GetLogString() : "nullptr");
    govobj.Relay(peerman, m_mn_sync);

    // Update the rate buffer
    MasternodeRateUpdate(govobj);

    m_mn_sync.BumpAssetLastTime("CGovernanceManager::AddGovernanceObject");

    // WE MIGHT HAVE PENDING/ORPHAN VOTES FOR THIS OBJECT

    CheckOrphanVotes(govobj, peerman);

    // SEND NOTIFICATION TO SCRIPT/ZMQ
    GetMainSignals().NotifyGovernanceObject(std::make_shared<const Governance::Object>(govobj.Object()), nHash.ToString());
}

void CGovernanceManager::CheckAndRemove()
{
    assert(m_mn_metaman.IsValid());

    // Return on initial sync, spammed the debug.log and provided no use
    if (!m_mn_sync.IsBlockchainSynced()) return;

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean\n");

    std::vector<uint256> vecDirtyHashes = m_mn_metaman.GetAndClearDirtyGovernanceObjectHashes();

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();

    LOCK2(::cs_main, cs);

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
            pObj->UpdateLocalValidity(tip_mn_list, m_chainman);

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
            m_mn_metaman.RemoveGovernanceObject(pObj->GetHash());

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

            if (pObj->GetObjectType() == GovernanceObject::PROPOSAL) {
                // keep hashes of deleted proposals forever
                nTimeExpired = std::numeric_limits<int64_t>::max();
            } else {
                int64_t nSuperblockCycleSeconds = Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().nPowTargetSpacing;
                nTimeExpired = pObj->GetCreationTime() + 2 * nSuperblockCycleSeconds + GOVERNANCE_DELETION_DELAY;
            }

            mapErasedGovernanceObjects.insert(std::make_pair(nHash, nTimeExpired));
            mapObjects.erase(it++);
        } else {
            if (pObj->GetObjectType() == GovernanceObject::PROPOSAL) {
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

    // forget about expired requests
    auto r_it = m_requested_hash_time.begin();
    while (r_it != m_requested_hash_time.end()) {
        if (r_it->second < std::chrono::seconds(nNow)) {
            m_requested_hash_time.erase(r_it++);
        } else {
            ++r_it;
        }
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- %s, m_requested_hash_time size=%d\n",
             ToString(), m_requested_hash_time.size());
}

const CGovernanceObject* CGovernanceManager::FindConstGovernanceObject(const uint256& nHash) const
{
    AssertLockHeld(cs);

    auto it = mapObjects.find(nHash);
    if (it != mapObjects.end()) return &(it->second);

    return nullptr;
}

CGovernanceObject* CGovernanceManager::FindGovernanceObject(const uint256& nHash)
{
    AssertLockHeld(cs);

    if (mapObjects.count(nHash)) return &mapObjects[nHash];

    return nullptr;
}

CGovernanceObject* CGovernanceManager::FindGovernanceObjectByDataHash(const uint256 &nDataHash)
{
    AssertLockHeld(cs);

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

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
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

        for (const auto& [signal, vote_instance] : voteRecord.mapInstances) {
            CGovernanceVote vote = CGovernanceVote(mnpair.first, nParentHash, (vote_signal_enum_t)signal,
                                                   vote_instance.eOutcome);
            vote.SetTime(vote_instance.nCreationTime);
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
    if (!m_mn_sync.IsSynced()) return std::nullopt;
    if (nHeight % Params().GetConsensus().nSuperblockCycle < Params().GetConsensus().nSuperblockCycle - Params().GetConsensus().nSuperblockMaturityWindow) return std::nullopt;
    if (HasAlreadyVotedFundingTrigger()) return std::nullopt;

    // A proposal is considered passing if (YES votes) >= (Total Weight of Masternodes / 10),
    // count total valid (ENABLED) masternodes to determine passing threshold.
    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    const int nWeightedMnCount = tip_mn_list.GetValidWeightedMNsCount();
    const int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nWeightedMnCount / 10);

    // Use std::vector of std::shared_ptr<const CGovernanceObject> because CGovernanceObject doesn't support move operations (needed for sorting the vector later)
    std::vector<std::shared_ptr<const CGovernanceObject>> approvedProposals;

    {
        LOCK(cs);
        for (const auto& [unused, object] : mapObjects) {
            // Skip all non-proposals objects
            if (object.GetObjectType() != GovernanceObject::PROPOSAL) continue;

            const int absYesCount = object.GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
            // Skip non-passing proposals
            if (absYesCount < nAbsVoteReq) continue;

            approvedProposals.emplace_back(std::make_shared<const CGovernanceObject>(object));
        }
    } // cs

    // Sort approved proposals by absolute Yes votes descending
    std::sort(approvedProposals.begin(), approvedProposals.end(), [tip_mn_list](std::shared_ptr<const CGovernanceObject> a, std::shared_ptr<const CGovernanceObject> b) {
        const auto a_yes = a->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
        const auto b_yes = b->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
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
    auto SBEpochTime = static_cast<int64_t>(GetTime<std::chrono::seconds>().count() + (nNextSuperblock - nHeight) * 2.62 * 60);
    auto governanceBudget = CSuperblock::GetPaymentsLimit(m_chainman.ActiveChain(), nNextSuperblock);

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

std::optional<const CGovernanceObject> CGovernanceManager::CreateGovernanceTrigger(const std::optional<const CSuperblock>& sb_opt, PeerManager& peerman,
                                                                                   const CActiveMasternodeManager& mn_activeman)
{
    // no sb_opt, no trigger
    if (!sb_opt.has_value()) return std::nullopt;

    //TODO: Check if nHashParentIn, nRevision and nCollateralHashIn are correct
    LOCK2(::cs_main, cs);

    // Check if identical trigger (equal DataHash()) is already created (signed by other masternode)
    CGovernanceObject gov_sb(uint256(), 1, GetAdjustedTime(), uint256(), sb_opt.value().GetHexStrData());
    if (auto identical_sb = FindGovernanceObjectByDataHash(gov_sb.GetDataHash())) {
        // Somebody submitted a trigger with the same data, support it instead of submitting a duplicate
        return std::make_optional<CGovernanceObject>(*identical_sb);
    }

    // Nobody submitted a trigger we'd like to see, so let's do it but only if we are the payee
    const CBlockIndex *tip = WITH_LOCK(::cs_main, return m_chainman.ActiveChain().Tip());
    const auto mnList = Assert(m_dmnman)->GetListForBlock(tip);
    const auto mn_payees = mnList.GetProjectedMNPayees(tip);

    if (mn_payees.empty()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s payee list is empty\n", __func__);
        return std::nullopt;
    }

    if (mn_payees.front()->proTxHash != mn_activeman.GetProTxHash()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s we are not the payee, skipping\n", __func__);
        return std::nullopt;
    }
    gov_sb.SetMasternodeOutpoint(mn_activeman.GetOutPoint());
    gov_sb.Sign(mn_activeman);

    if (std::string strError; !gov_sb.IsValidLocally(m_dmnman->GetListAtChainTip(), m_chainman, strError, true)) {
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

void CGovernanceManager::VoteGovernanceTriggers(const std::optional<const CGovernanceObject>& trigger_opt, CConnman& connman, PeerManager& peerman,
                                                const CActiveMasternodeManager& mn_activeman)
{
    // only active masternodes can vote on triggers
    if (mn_activeman.GetProTxHash().IsNull()) return;

    LOCK2(::cs_main, cs);

    if (trigger_opt.has_value()) {
        // We should never vote "yes" on another trigger or the same trigger twice
        assert(!votedFundingYesTriggerHash.has_value());
        // Vote YES-FUNDING for the trigger we like, unless we already did
        const uint256 gov_sb_hash = trigger_opt.value().GetHash();
        bool voted_already{false};
        if (vote_rec_t voteRecord; trigger_opt.value().GetCurrentMNVotes(mn_activeman.GetOutPoint(), voteRecord)) {
            const auto& strFunc = __func__;
            // Let's see if there is a VOTE_SIGNAL_FUNDING vote from us already
            voted_already = ranges::any_of(voteRecord.mapInstances, [&](const auto& voteInstancePair) {
                if (voteInstancePair.first == VOTE_SIGNAL_FUNDING) {
                    if (voteInstancePair.second.eOutcome == VOTE_OUTCOME_YES) {
                        votedFundingYesTriggerHash = gov_sb_hash;
                    }
                    LogPrint(BCLog::GOBJECT, /* Continued */
                             "CGovernanceManager::%s "
                             "Not voting YES-FUNDING for trigger:%s, we voted %s for it already\n",
                             strFunc, gov_sb_hash.ToString(),
                             CGovernanceVoting::ConvertOutcomeToString(voteInstancePair.second.eOutcome));
                    return true;
                }
                return false;
            });
        }
        if (!voted_already) {
            // No previous VOTE_SIGNAL_FUNDING was found, vote now
            if (VoteFundingTrigger(gov_sb_hash, VOTE_OUTCOME_YES, connman, peerman, mn_activeman)) {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting YES-FUNDING for new trigger:%s success\n",
                         __func__, gov_sb_hash.ToString());
                votedFundingYesTriggerHash = gov_sb_hash;
            } else {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting YES-FUNDING for new trigger:%s failed\n",
                         __func__, gov_sb_hash.ToString());
                // this should never happen, bail out
                return;
            }
        }
    }

    // Vote NO-FUNDING for the rest of the active triggers
    const auto activeTriggers = GetActiveTriggers();
    for (const auto& trigger : activeTriggers) {
        const auto govobj = FindGovernanceObject(trigger->GetGovernanceObjHash());
        const uint256 trigger_hash = govobj->GetHash();
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
        if (vote_rec_t voteRecord; govobj->GetCurrentMNVotes(mn_activeman.GetOutPoint(), voteRecord)) {
            const auto& strFunc = __func__;
            if (ranges::any_of(voteRecord.mapInstances, [&](const auto& voteInstancePair) {
                    if (voteInstancePair.first == VOTE_SIGNAL_FUNDING) {
                        LogPrint(BCLog::GOBJECT, /* Continued */
                                 "CGovernanceManager::%s "
                                 "Not voting NO-FUNDING for trigger:%s, we voted %s for it already\n",
                                 strFunc, trigger_hash.ToString(),
                                 CGovernanceVoting::ConvertOutcomeToString(voteInstancePair.second.eOutcome));
                        return true;
                    }
                    return false;
                })) {
                continue;
            }
        }
        if (!VoteFundingTrigger(trigger_hash, VOTE_OUTCOME_NO, connman, peerman, mn_activeman)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting NO-FUNDING for trigger:%s failed\n", __func__, trigger_hash.ToString());
            // failing here is ok-ish
            continue;
        }
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s Voting NO-FUNDING for trigger:%s success\n", __func__, trigger_hash.ToString());
    }
}

bool CGovernanceManager::VoteFundingTrigger(const uint256& nHash, const vote_outcome_enum_t outcome, CConnman& connman, PeerManager& peerman,
                                            const CActiveMasternodeManager& mn_activeman)
{
    CGovernanceVote vote(mn_activeman.GetOutPoint(), nHash, VOTE_SIGNAL_FUNDING, outcome);
    vote.SetTime(GetAdjustedTime());
    vote.Sign(mn_activeman);

    CGovernanceException exception;
    if (!ProcessVoteAndRelay(vote, exception, connman, peerman)) {
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
    if (!m_mn_sync.IsSynced()) return;
    if (ShutdownRequested()) return;

    // CHECK OBJECTS WE'VE ASKED FOR, REMOVE OLD ENTRIES
    CleanOrphanObjects();
    RequestOrphanObjects(connman);

    // CHECK AND REMOVE - REPROCESS GOVERNANCE OBJECTS
    CheckAndRemove();
}

bool CGovernanceManager::ConfirmInventoryRequest(const CInv& inv)
{
    // do not request objects until it's time to sync
    if (!m_mn_sync.IsBlockchainSynced()) return false;

    LOCK(cs);

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest inv = %s\n", inv.ToString());

    // First check if we've already recorded this object
    switch (inv.type) {
    case MSG_GOVERNANCE_OBJECT: {
        if (mapObjects.count(inv.hash) == 1 || mapPostponedObjects.count(inv.hash) == 1) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest already have governance object, returning false\n");
            return false;
        }
        break;
    }
    case MSG_GOVERNANCE_OBJECT_VOTE: {
        if (cmapVoteToObject.HasKey(inv.hash)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest already have governance vote, returning false\n");
            return false;
        }
        break;
    }
    default:
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest unknown type, returning false\n");
        return false;
    }

    const auto valid_until = GetTime<std::chrono::seconds>() + std::chrono::seconds(RELIABLE_PROPAGATION_TIME);
    const auto& [_itr, inserted] = m_requested_hash_time.emplace(inv.hash, valid_until);

    if (inserted) {
        LogPrint(BCLog::GOBJECT, /* Continued */
                 "CGovernanceManager::ConfirmInventoryRequest added %s inv hash to m_requested_hash_time, size=%d\n",
                 inv.type == MSG_GOVERNANCE_OBJECT ? "object" : "vote", m_requested_hash_time.size());
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest reached end, returning true\n");
    return true;
}

MessageProcessingResult CGovernanceManager::SyncSingleObjVotes(CNode& peer, const uint256& nProp, const CBloomFilter& filter, CConnman& connman)
{
    // do not provide any data until our node is synced
    if (!m_mn_sync.IsSynced()) return {};

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- syncing single object to peer=%d, nProp = %s\n", __func__, peer.GetId(), nProp.ToString());

    LOCK(cs);

    // single valid object and its valid votes
    auto it = mapObjects.find(nProp);
    if (it == mapObjects.end()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- no matching object for hash %s, peer=%d\n", __func__, nProp.ToString(), peer.GetId());
        return {};
    }
    const CGovernanceObject& govobj = it->second;
    std::string strHash = it->first.ToString();

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- attempting to sync govobj: %s, peer=%d\n", __func__, strHash, peer.GetId());

    if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- not syncing deleted/expired govobj: %s, peer=%d\n", __func__,
            strHash, peer.GetId());
        return {};
    }

    const auto& fileVotes = govobj.GetVoteFile();
    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();

    MessageProcessingResult ret{};
    for (const auto& vote : fileVotes.GetVotes()) {
        uint256 nVoteHash = vote.GetHash();

        bool onlyVotingKeyAllowed = govobj.GetObjectType() == GovernanceObject::PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

        if (filter.contains(nVoteHash) || !vote.IsValid(tip_mn_list, onlyVotingKeyAllowed)) {
            continue;
        }
        ret.m_inventory.emplace_back(MSG_GOVERNANCE_OBJECT_VOTE, nVoteHash);
    }

    CNetMsgMaker msgMaker(peer.GetCommonVersion());
    connman.PushMessage(&peer, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ_VOTE,
                                             static_cast<int>(ret.m_inventory.size())));
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- sent %d votes to peer=%d\n", __func__, ret.m_inventory.size(),
             peer.GetId());
    return ret;
}

MessageProcessingResult CGovernanceManager::SyncObjects(CNode& peer, CConnman& connman) const
{
    assert(m_netfulfilledman.IsValid());

    // do not provide any data until our node is synced
    if (!m_mn_sync.IsSynced()) return {};

    if (m_netfulfilledman.HasFulfilledRequest(peer.addr, NetMsgType::MNGOVERNANCESYNC)) {
        // Asking for the whole list multiple times in a short period of time is no good
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- peer already asked me for the list\n", __func__);
        return MisbehavingError{20};
    }
    m_netfulfilledman.AddFulfilledRequest(peer.addr, NetMsgType::MNGOVERNANCESYNC);

    // SYNC GOVERNANCE OBJECTS WITH OTHER CLIENT

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- syncing all objects to peer=%d\n", __func__, peer.GetId());

    LOCK(cs);

    // all valid objects, no votes
    MessageProcessingResult ret{};
    for (const auto& objPair : mapObjects) {
        uint256 nHash = objPair.first;
        const CGovernanceObject& govobj = objPair.second;
        std::string strHash = nHash.ToString();

        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- attempting to sync govobj: %s, peer=%d\n", __func__, strHash, peer.GetId());

        if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- not syncing deleted/expired govobj: %s, peer=%d\n", __func__,
                strHash, peer.GetId());
            continue;
        }

        if (peer.nVersion < GOVSCRIPT_PROTO_VERSION && govobj.GetObjectType() == GovernanceObject::PROPOSAL) {
            // We know this proposal is valid locally, otherwise we would not store it.
            // But we don't want to relay it to pre-GOVSCRIPT_PROTO_VERSION peers if payment_address is p2sh
            // because they won't accept it anyway and will simply ban us eventually.
            CProposalValidator validator(govobj.GetDataAsHexString(), false /* no script */);
            if (!validator.Validate(false /* ignore expiration */)) {
                // The only way we could get here is when proposal is valid but payment_address is actually p2sh.
                LogPrintf("CGovernanceManager::%s -- not syncing p2sh govobj to older node: %s, peer=%d\n", __func__,
                    strHash, peer.GetId());
                continue;
            }
        }

        // Push the inventory budget proposal message over to the other client
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- syncing govobj: %s, peer=%d\n", __func__, strHash, peer.GetId());
        ret.m_inventory.emplace_back(MSG_GOVERNANCE_OBJECT, nHash);
    }

    CNetMsgMaker msgMaker(peer.GetCommonVersion());
    connman.PushMessage(&peer, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ,
                                             static_cast<int>(ret.m_inventory.size())));
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- sent %d objects to peer=%d\n", __func__, ret.m_inventory.size(),
             peer.GetId());
    return ret;
}

void CGovernanceManager::MasternodeRateUpdate(const CGovernanceObject& govobj)
{
    if (govobj.GetObjectType() != GovernanceObject::TRIGGER) return;

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
    LOCK(cs);

    fRateCheckBypassed = false;

    if (!m_mn_sync.IsSynced() || !fRateChecksEnabled) {
        return true;
    }

    if (govobj.GetObjectType() != GovernanceObject::TRIGGER) {
        return true;
    }

    const COutPoint& masternodeOutpoint = govobj.GetMasternodeOutpoint();
    int64_t nTimestamp = govobj.GetCreationTime();
    int64_t nNow = GetAdjustedTime();
    int64_t nSuperblockCycleSeconds = Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().nPowTargetSpacing;

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

bool CGovernanceManager::ProcessVoteAndRelay(const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman, PeerManager& peerman)
{
    bool fOK = ProcessVote(/* pfrom = */ nullptr, vote, exception, connman);
    if (fOK) {
        vote.Relay(peerman, m_mn_sync, Assert(m_dmnman)->GetListAtChainTip());
    }
    return fOK;
}

bool CGovernanceManager::ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman)
{
    ENTER_CRITICAL_SECTION(cs);
    uint256 nHashVote = vote.GetHash();
    uint256 nHashGovobj = vote.GetParentHash();

    if (cmapVoteToObject.HasKey(nHashVote)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::%s -- skipping known valid vote %s for object %s\n", __func__,
            nHashVote.ToString(), nHashGovobj.ToString());
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    if (cmapInvalidVotes.HasKey(nHashVote)) {
        std::string msg{strprintf("CGovernanceManager::%s -- Old invalid vote, MN outpoint = %s, governance object hash = %s",
            __func__, vote.GetMasternodeOutpoint().ToStringShort(), nHashGovobj.ToString())};
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    auto it = mapObjects.find(nHashGovobj);
    if (it == mapObjects.end()) {
        std::string msg{strprintf("CGovernanceManager::%s -- Unknown parent object %s, MN outpoint = %s", __func__,
            nHashGovobj.ToString(), vote.GetMasternodeOutpoint().ToStringShort())};
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_WARNING);
        if (cmmapOrphanVotes.Insert(nHashGovobj, vote_time_pair_t(vote, GetTime<std::chrono::seconds>().count() + GOVERNANCE_ORPHAN_EXPIRATION_TIME))) {
            LEAVE_CRITICAL_SECTION(cs);
            RequestGovernanceObject(pfrom, nHashGovobj, connman);
            LogPrint(BCLog::GOBJECT, "%s\n", msg);
            return false;
        }

        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    CGovernanceObject& govobj = it->second;

    if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::%s -- ignoring vote for expired or deleted object, hash = %s\n",
            __func__, nHashGovobj.ToString());
        LEAVE_CRITICAL_SECTION(cs);
        return false;
    }

    bool fOk = govobj.ProcessVote(m_mn_metaman, *this, Assert(m_dmnman)->GetListAtChainTip(), vote, exception) && cmapVoteToObject.Insert(nHashVote, &govobj);
    LEAVE_CRITICAL_SECTION(cs);
    return fOk;
}

void CGovernanceManager::CheckPostponedObjects(PeerManager& peerman)
{
    if (!m_mn_sync.IsSynced()) return;

    LOCK2(::cs_main, cs);

    // Check postponed proposals
    for (auto it = mapPostponedObjects.begin(); it != mapPostponedObjects.end();) {
        const uint256& nHash = it->first;
        CGovernanceObject& govobj = it->second;

        assert(govobj.GetObjectType() != GovernanceObject::TRIGGER);

        std::string strError;
        bool fMissingConfirmations;
        if (govobj.IsCollateralValid(m_chainman, strError, fMissingConfirmations)) {
            if (govobj.IsValidLocally(Assert(m_dmnman)->GetListAtChainTip(), m_chainman, strError, false)) {
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
    int64_t nNow = GetAdjustedTime();
    int64_t nSuperblockCycleSeconds = Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().nPowTargetSpacing;

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
                    govobj.Relay(peerman, m_mn_sync);
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
            filter = CBloomFilter(Params().GetConsensus().nGovernanceFilterElements, GOVERNANCE_FILTER_FP_RATE, GetRand<int>(/*nMax=*/999999), BLOOM_UPDATE_ALL);
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

int CGovernanceManager::RequestGovernanceObjectVotes(CNode& peer, CConnman& connman, const PeerManager& peerman) const
{
    const std::vector<CNode*> vNodeCopy{&peer};
    return RequestGovernanceObjectVotes(vNodeCopy, connman, peerman);
}

int CGovernanceManager::RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy, CConnman& connman,
                                                     const PeerManager& peerman) const
{
    static std::map<uint256, std::map<CService, int64_t> > mapAskedRecently;
    // Maximum number of nodes to request votes from for the same object hash on real networks
    // (mainnet, testnet, devnets). Keep this low to avoid unnecessary bandwidth usage.
    static constexpr size_t REALNET_PEERS_PER_HASH{3};
    // Maximum number of nodes to request votes from for the same object hash on regtest.
    // During testing, nodes are isolated to create conflicting triggers. Using the real
    // networks limit of 3 nodes often results in querying only "non-isolated" nodes, missing the
    // isolated ones we need to test. This high limit ensures all available nodes are queried.
    static constexpr size_t REGTEST_PEERS_PER_HASH{std::numeric_limits<size_t>::max()};

    if (vNodesCopy.empty()) return -1;

    int64_t nNow = GetTime();
    int nTimeout = 60 * 60;
    size_t nPeersPerHashMax = Params().IsMockableChain() ? REGTEST_PEERS_PER_HASH : REALNET_PEERS_PER_HASH;

    std::vector<uint256> vTriggerObjHashes;
    std::vector<uint256> vOtherObjHashes;

    // This should help us to get some idea about an impact this can bring once deployed on mainnet.
    // Testnet is ~40 times smaller in masternode count, but only ~1000 masternodes usually vote,
    // so 1 obj on mainnet == ~10 objs or ~1000 votes on testnet. However we want to test a higher
    // number of votes to make sure it's robust enough, so aim at 2000 votes per masternode per request.
    // On mainnet nMaxObjRequestsPerNode is always set to 1.
    int nMaxObjRequestsPerNode = 1;
    size_t nProjectedVotes = 2000;
    if (Params().NetworkIDString() != CBaseChainParams::MAIN) {
        nMaxObjRequestsPerNode = std::max(1, int(nProjectedVotes / std::max(1, (int)Assert(m_dmnman)->GetListAtChainTip().GetValidMNsCount())));
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

            if (govobj.GetObjectType() == GovernanceObject::TRIGGER) {
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
                LOCK(::cs_main);
                size_t nProjectedSize = peerman.GetRequestedObjectCount(pnode->GetId()) + nProjectedVotes;
                if (nProjectedSize > MAX_INV_SZ) continue;
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

bool CGovernanceManager::AcceptMessage(const uint256& nHash)
{
    LOCK(cs);
    auto it = m_requested_hash_time.find(nHash);
    if (it == m_requested_hash_time.end()) {
        // We never requested this
        return false;
    }
    // Only accept one response
    m_requested_hash_time.erase(it);
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

        if (govobj.GetObjectType() != GovernanceObject::TRIGGER) {
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
    const auto start{SteadyClock::now()};
    LogPrintf("Preparing masternode indexes and governance triggers...\n");
    RebuildIndexes();
    AddCachedTriggers();
    LogPrintf("Masternode indexes and governance triggers prepared  %dms\n",
              Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
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
        case GovernanceObject::PROPOSAL:
            nProposalCount++;
            break;
        case GovernanceObject::TRIGGER:
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
        case GovernanceObject::PROPOSAL:
            nProposalCount++;
            break;
        case GovernanceObject::TRIGGER:
            nTriggerCount++;
            break;
        default:
            nOtherCount++;
            break;
        }
    }

    UniValue jsonObj(UniValue::VOBJ);
    jsonObj.pushKV("objects_total", mapObjects.size());
    jsonObj.pushKV("proposals", nProposalCount);
    jsonObj.pushKV("triggers", nTriggerCount);
    jsonObj.pushKV("other", nOtherCount);
    jsonObj.pushKV("erased", mapErasedGovernanceObjects.size());
    jsonObj.pushKV("votes", cmapVoteToObject.GetSize());
    return jsonObj;
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex* pindex, CConnman& connman, PeerManager& peerman, const CActiveMasternodeManager* const mn_activeman)
{
    // Note this gets called from ActivateBestChain without cs_main being held
    // so it should be safe to lock our mutex here without risking a deadlock
    // On the other hand it should be safe for us to access pindex without holding a lock
    // on cs_main because the CBlockIndex objects are dynamically allocated and
    // presumably never deleted.
    if (!pindex) {
        return;
    }

    if (mn_activeman) {
        const auto sb_opt = CreateSuperblockCandidate(pindex->nHeight);
        const auto trigger_opt = CreateGovernanceTrigger(sb_opt, peerman, *mn_activeman);
        VoteGovernanceTriggers(trigger_opt, connman, peerman, *mn_activeman);
    }

    nCachedBlockHeight = pindex->nHeight;
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdatedBlockTip -- nCachedBlockHeight: %d\n", nCachedBlockHeight);

    if (DeploymentDIP0003Enforced(pindex->nHeight, Params().GetConsensus())) {
        RemoveInvalidVotes();
    }

    CheckPostponedObjects(peerman);

    ExecuteBestSuperblock(Assert(m_dmnman)->GetListAtChainTip(), pindex->nHeight);
}

void CGovernanceManager::RequestOrphanObjects(CConnman& connman)
{
    const CConnman::NodesSnapshot snap{connman, /* cond = */ CConnman::FullyConnectedOnly};

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
    if (!m_mn_sync.IsSynced()) {
        return;
    }

    LOCK(cs);

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    auto diff = lastMNListForVotingKeys->BuildDiff(tip_mn_list);

    std::vector<COutPoint> changedKeyMNs;
    for (const auto& p : diff.updatedMNs) {
        auto oldDmn = lastMNListForVotingKeys->GetMNByInternalId(p.first);
        // BuildDiff will construct itself with MNs that we already have knowledge
        // of, meaning that fetch operations should never fail.
        assert(oldDmn);
        if ((p.second.fields & CDeterministicMNStateDiff::Field_keyIDVoting) && p.second.state.keyIDVoting != oldDmn->pdmnState->keyIDVoting) {
            changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
        } else if ((p.second.fields & CDeterministicMNStateDiff::Field_pubKeyOperator) && p.second.state.pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
            changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
        }
    }
    for (const auto& id : diff.removedMns) {
        auto oldDmn = lastMNListForVotingKeys->GetMNByInternalId(id);
        // BuildDiff will construct itself with MNs that we already have knowledge
        // of, meaning that fetch operations should never fail.
        assert(oldDmn);
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
                m_requested_hash_time.erase(voteHash);
            }
        }
    }

    // store current MN list for the next run so that we can determine which keys changed
    lastMNListForVotingKeys = std::make_shared<CDeterministicMNList>(tip_mn_list);
}

/**
 *   Add Governance Object
 */

bool CGovernanceManager::AddNewTrigger(uint256 nHash)
{
    AssertLockHeld(cs);

    // IF WE ALREADY HAVE THIS HASH, RETURN
    if (mapTrigger.count(nHash)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Already have hash, nHash = %s, count = %d, size = %s\n",
                 __func__, nHash.GetHex(), mapTrigger.count(nHash), mapTrigger.size());
        return false;
    }

    CSuperblock_sptr pSuperblock;
    try {
        const CGovernanceObject* pGovObj = FindGovernanceObject(nHash);
        if (!pGovObj) {
            throw std::runtime_error("CSuperblock: Failed to find Governance Object");
        }
        pSuperblock = std::make_shared<CSuperblock>(*pGovObj, nHash);
    } catch (std::exception& e) {
        LogPrintf("CGovernanceManager::%s -- Error creating superblock: %s\n", __func__, e.what());
        return false;
    } catch (...) {
        LogPrintf("CGovernanceManager::%s -- Unknown Error creating superblock\n", __func__);
        return false;
    }

    pSuperblock->SetStatus(SeenObjectStatus::Valid);

    mapTrigger.insert(std::make_pair(nHash, pSuperblock));

    return !pSuperblock->IsExpired(GetCachedBlockHeight());
}

/**
 *
 *   Clean And Remove
 *
 */

void CGovernanceManager::CleanAndRemoveTriggers()
{
    AssertLockHeld(cs);

    // Remove triggers that are invalid or expired
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- mapTrigger.size() = %d\n", __func__, mapTrigger.size());

    auto it = mapTrigger.begin();
    while (it != mapTrigger.end()) {
        bool remove = false;
        CGovernanceObject* pObj = nullptr;
        const CSuperblock_sptr& pSuperblock = it->second;
        if (!pSuperblock) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- nullptr superblock\n", __func__);
            remove = true;
        } else {
            pObj = FindGovernanceObject(it->first);
            if (!pObj || pObj->GetObjectType() != GovernanceObject::TRIGGER) {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Unknown or non-trigger superblock\n", __func__);
                pSuperblock->SetStatus(SeenObjectStatus::ErrorInvalid);
            }

            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- superblock status = %d\n", __func__,
                     ToUnderlying(pSuperblock->GetStatus()));
            switch (pSuperblock->GetStatus()) {
            case SeenObjectStatus::ErrorInvalid:
            case SeenObjectStatus::Unknown:
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Unknown or invalid trigger found\n", __func__);
                remove = true;
                break;
            case SeenObjectStatus::Valid:
            case SeenObjectStatus::Executed: {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Valid trigger found\n", __func__);
                if (pSuperblock->IsExpired(GetCachedBlockHeight())) {
                    // update corresponding object
                    pObj->SetExpired();
                    remove = true;
                }
                break;
            }
            default:
                break;
            }
        }
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- %smarked for removal\n", __func__, remove ? "" : "NOT ");

        if (remove) {
            std::string strDataAsPlainString = "nullptr";
            if (pObj) {
                strDataAsPlainString = pObj->GetDataAsPlainString();
                // mark corresponding object for deletion
                pObj->PrepareDeletion(GetTime<std::chrono::seconds>().count());
            }
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Removing trigger object %s\n", __func__,
                     strDataAsPlainString);
            // delete the trigger
            mapTrigger.erase(it++);
        } else {
            ++it;
        }
    }
}

/**
 *   Get Active Triggers
 *
 *   - Look through triggers and scan for active ones
 *   - Return the triggers in a list
 */

std::vector<CSuperblock_sptr> CGovernanceManager::GetActiveTriggers() const
{
    AssertLockHeld(cs);
    std::vector<CSuperblock_sptr> vecResults;

    // LOOK AT THESE OBJECTS AND COMPILE A VALID LIST OF TRIGGERS
    for (const auto& pair : mapTrigger) {
        const CGovernanceObject* pObj = FindConstGovernanceObject(pair.first);
        if (pObj) {
            vecResults.push_back(pair.second);
        }
    }

    return vecResults;
}

bool CGovernanceManager::IsSuperblockTriggered(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- Start nBlockHeight = %d\n", nBlockHeight);
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    LOCK(cs);
    // GET ALL ACTIVE TRIGGERS
    std::vector<CSuperblock_sptr> vecTriggers = GetActiveTriggers();

    LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- vecTriggers.size() = %d\n", vecTriggers.size());

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock) {
            LogPrintf("IsSuperblockTriggered -- Non-superblock found, continuing\n");
            continue;
        }

        CGovernanceObject* pObj = FindGovernanceObject(pSuperblock->GetGovernanceObjHash());
        if (!pObj) {
            LogPrintf("IsSuperblockTriggered -- pObj == nullptr, continuing\n");
            continue;
        }

        LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- data = %s\n", pObj->GetDataAsPlainString());

        // note : 12.1 - is epoch calculation correct?

        if (nBlockHeight != pSuperblock->GetBlockHeight()) {
            LogPrint(BCLog::GOBJECT, /* Continued */
                     "IsSuperblockTriggered -- block height doesn't match nBlockHeight = %d, blockStart = %d, "
                     "continuing\n",
                     nBlockHeight, pSuperblock->GetBlockHeight());
            continue;
        }

        // MAKE SURE THIS TRIGGER IS ACTIVE VIA FUNDING CACHE FLAG

        pObj->UpdateSentinelVariables(tip_mn_list);

        if (pObj->IsSetCachedFunding()) {
            LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- fCacheFunding = true, returning true\n");
            return true;
        } else {
            LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- fCacheFunding = false, continuing\n");
        }
    }

    return false;
}


bool CGovernanceManager::GetBestSuperblock(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& pSuperblockRet,
                                           int nBlockHeight)
{
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    AssertLockHeld(cs);
    std::vector<CSuperblock_sptr> vecTriggers = GetActiveTriggers();
    int nYesCount = 0;

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock || nBlockHeight != pSuperblock->GetBlockHeight()) {
            continue;
        }

        const CGovernanceObject* pObj = FindGovernanceObject(pSuperblock->GetGovernanceObjHash());
        if (!pObj) {
            continue;
        }

        // DO WE HAVE A NEW WINNER?

        int nTempYesCount = pObj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
        if (nTempYesCount > nYesCount) {
            nYesCount = nTempYesCount;
            pSuperblockRet = pSuperblock;
        }
    }

    return nYesCount > 0;
}

bool CGovernanceManager::GetSuperblockPayments(const CDeterministicMNList& tip_mn_list, int nBlockHeight,
                                               std::vector<CTxOut>& voutSuperblockRet)
{
    LOCK(cs);

    // GET THE BEST SUPERBLOCK FOR THIS BLOCK HEIGHT

    CSuperblock_sptr pSuperblock;
    if (!GetBestSuperblock(tip_mn_list, pSuperblock, nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "GetSuperblockPayments -- Can't find superblock for height %d\n", nBlockHeight);
        return false;
    }

    // make sure it's empty, just in case
    voutSuperblockRet.clear();

    // GET SUPERBLOCK OUTPUTS

    // Superblock payments will be appended to the end of the coinbase vout vector

    // TODO: How many payments can we add before things blow up?
    //       Consider at least following limits:
    //          - max coinbase tx size
    //          - max "budget" available
    for (int i = 0; i < pSuperblock->CountPayments(); i++) {
        CGovernancePayment payment;
        if (pSuperblock->GetPayment(i, payment)) {
            // SET COINBASE OUTPUT TO SUPERBLOCK SETTING

            CTxOut txout = CTxOut(payment.nAmount, payment.script);
            voutSuperblockRet.push_back(txout);

            // PRINT NICE LOG OUTPUT FOR SUPERBLOCK PAYMENT

            CTxDestination dest;
            ExtractDestination(payment.script, dest);

            LogPrint(BCLog::GOBJECT, "GetSuperblockPayments -- NEW Superblock: output %d (addr %s, amount %d.%08d)\n",
                     i, EncodeDestination(dest), payment.nAmount / COIN, payment.nAmount % COIN);
        } else {
            LogPrint(BCLog::GOBJECT, "GetSuperblockPayments -- Payment not found\n");
        }
    }

    return true;
}

bool CGovernanceManager::IsValidSuperblock(const CChain& active_chain, const CDeterministicMNList& tip_mn_list,
                                           const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
{
    // GET BEST SUPERBLOCK, SHOULD MATCH
    LOCK(cs);

    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblock(tip_mn_list, pSuperblock, nBlockHeight)) {
        return pSuperblock->IsValid(active_chain, txNew, nBlockHeight, blockReward);
    }

    return false;
}

void CGovernanceManager::ExecuteBestSuperblock(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    LOCK(cs);

    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblock(tip_mn_list, pSuperblock, nBlockHeight)) {
        // All checks are done in CSuperblock::IsValid via IsBlockValueValid and IsBlockPayeeValid,
        // tip wouldn't be updated if anything was wrong. Mark this trigger as executed.
        pSuperblock->SetExecuted();
        ResetVotedFundingTrigger();
    }
}


bool AreSuperblocksEnabled(const CSporkManager& sporkman)
{
    return sporkman.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED);
}
