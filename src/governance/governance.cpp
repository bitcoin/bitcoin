// Copyright (c) 2014-2025 The Dash Core developers
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
#include <masternode/sync.h>
#include <node/interface_ui.h>
#include <protocol.h>
#include <shutdown.h>
#include <spork.h>
#include <timedata.h>
#include <util/ranges.h>
#include <util/thread.h>
#include <util/time.h>
#include <validationinterface.h>

const std::string GovernanceStore::SERIALIZATION_VERSION_STRING = "CGovernanceManager-Version-16";

namespace {
constexpr std::chrono::seconds GOVERNANCE_DELETION_DELAY{10min};
constexpr std::chrono::seconds GOVERNANCE_ORPHAN_EXPIRATION_TIME{10min};
constexpr std::chrono::seconds MAX_TIME_FUTURE_DEVIATION{1h};
constexpr std::chrono::seconds RELIABLE_PROPAGATION_TIME{1min};

class ScopedLockBool
{
    bool& ref;
    bool fPrevValue;

public:
    ScopedLockBool(Mutex& _cs, bool& _ref, bool _value) :
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
    cs_store(),
    mapObjects(),
    mapErasedGovernanceObjects(),
    cmapInvalidVotes(MAX_CACHE_SIZE),
    cmmapOrphanVotes(MAX_CACHE_SIZE),
    mapLastMasternodeObject(),
    lastMNListForVotingKeys(std::make_shared<CDeterministicMNList>())
{
}

CGovernanceManager::CGovernanceManager(CMasternodeMetaMan& mn_metaman,
                                       const ChainstateManager& chainman,
                                       const std::unique_ptr<CDeterministicMNManager>& dmnman, CMasternodeSync& mn_sync) :
    m_db{std::make_unique<db_type>("governance.dat", "magicGovernanceCache")},
    m_mn_metaman{mn_metaman},
    m_chainman{chainman},
    m_dmnman{dmnman},
    m_mn_sync{mn_sync},
    cmapVoteToObject{MAX_CACHE_SIZE},
    mapPostponedObjects{},
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
    AssertLockNotHeld(cs_store);
    assert(m_db != nullptr);
    is_valid = load_cache ? m_db->Load(*this) : m_db->Store(*this);
    if (is_valid && load_cache) {
        CheckAndRemove();
        InitOnLoad();
    }
    return is_valid;
}

void CGovernanceManager::RelayObject(const CGovernanceObject& obj)
{
    AssertLockNotHeld(cs_relay);
    if (!m_mn_sync.IsSynced()) {
        LogPrint(BCLog::GOBJECT, "%s -- won't relay until fully synced\n", __func__);
        return;
    }

    LOCK(cs_relay);
    m_relay_invs.emplace_back(MSG_GOVERNANCE_OBJECT, obj.GetHash());
}

void CGovernanceManager::RelayVote(const CGovernanceVote& vote)
{
    AssertLockNotHeld(cs_relay);
    if (!m_mn_sync.IsSynced()) {
        LogPrint(BCLog::GOBJECT, "%s -- won't relay until fully synced\n", __func__);
        return;
    }

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    auto dmn = tip_mn_list.GetMNByCollateral(vote.GetMasternodeOutpoint());
    if (!dmn) {
        return;
    }

    LOCK(cs_relay);
    m_relay_invs.emplace_back(MSG_GOVERNANCE_OBJECT_VOTE, vote.GetHash());
}

// Accessors for thread-safe access to maps
bool CGovernanceManager::HaveObjectForHash(const uint256& nHash) const
{
    LOCK(cs_store);
    return (mapObjects.count(nHash) == 1 || mapPostponedObjects.count(nHash) == 1);
}

bool CGovernanceManager::SerializeObjectForHash(const uint256& nHash, CDataStream& ss) const
{
    LOCK(cs_store);
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
    LOCK(cs_store);

    std::shared_ptr<CGovernanceObject> pGovobj{nullptr};
    return cmapVoteToObject.Get(nHash, pGovobj) && WITH_LOCK(pGovobj->cs, return pGovobj->GetVoteFile().HasVote(nHash));
}

int CGovernanceManager::GetVoteCount() const
{
    LOCK(cs_store);
    return (int)cmapVoteToObject.GetSize();
}

bool CGovernanceManager::SerializeVoteForHash(const uint256& nHash, CDataStream& ss) const
{
    LOCK(cs_store);

    std::shared_ptr<CGovernanceObject> pGovobj{nullptr};
    return cmapVoteToObject.Get(nHash, pGovobj) && WITH_LOCK(pGovobj->cs, return pGovobj->GetVoteFile().SerializeVoteToStream(nHash, ss));
}

void CGovernanceManager::AddPostponedObject(const CGovernanceObject& govobj)
{
    LOCK(cs_store);
    AddPostponedObjectInternal(govobj);
}

void CGovernanceManager::AddPostponedObjectInternal(const CGovernanceObject& govobj)
{
    AssertLockHeld(cs_store);
    mapPostponedObjects.emplace(govobj.GetHash(), std::make_shared<CGovernanceObject>(govobj));
}

bool CGovernanceManager::ProcessObject(const CNode& peer, const uint256& nHash, CGovernanceObject& govobj)
{
    std::string strHash = nHash.ToString();

    LOCK(cs_store);

    if (mapObjects.count(nHash) || mapPostponedObjects.count(nHash) || mapErasedGovernanceObjects.count(nHash)) {
        // TODO - print error code? what if it's GOVOBJ_ERROR_IMMATURE?
        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received already seen object: %s\n", strHash);
        return true;
    }

    bool fRateCheckBypassed = false;
    if (!MasternodeRateCheck(govobj, true, false, fRateCheckBypassed)) {
        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode rate check failed - %s - (current block height %d) \n",
                 strHash, nCachedBlockHeight);
        return true;
    }

    std::string strError;
    // CHECK OBJECT AGAINST LOCAL BLOCKCHAIN

    const auto tip_mn_list = GetMNManager().GetListAtChainTip();
    bool fMissingConfirmations = false;
    bool fIsValid = govobj.IsValidLocally(tip_mn_list, m_chainman, strError, fMissingConfirmations, true);

    bool unused_rcb;
    if (fRateCheckBypassed && fIsValid && !MasternodeRateCheck(govobj, true, true, unused_rcb)) {
        LogPrint(BCLog::GOBJECT, /* Continued */
                 "MNGOVERNANCEOBJECT -- masternode rate check failed (after signature verification) - %s - (current "
                 "block height %d)\n",
                 strHash, nCachedBlockHeight);
        return true;
    }

    if (!fIsValid) {
        if (fMissingConfirmations) {
            AddPostponedObjectInternal(govobj);
            LogPrintf("MNGOVERNANCEOBJECT -- Not enough fee confirmations for: %s, strError = %s\n", strHash, strError);
            return true;
        } else {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Governance object is invalid - %s\n", strError);
            return false;
        }
    }

    AddGovernanceObjectInternal(govobj, &peer);
    return true;
}

void CGovernanceManager::CheckOrphanVotes(CGovernanceObject& govobj)
{
    AssertLockHeld(cs_store);
    AssertLockNotHeld(cs_relay);

    uint256 nHash = govobj.GetHash();
    std::vector<vote_time_pair_t> vecVotePairs;
    cmmapOrphanVotes.GetAll(nHash, vecVotePairs);

    ScopedLockBool guard(cs_store, fRateChecksEnabled, false);

    int64_t nNow = GetAdjustedTime();
    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    for (const auto& pairVote : vecVotePairs) {
        const auto& [vote, time] = pairVote;
        bool fRemove = false;
        CGovernanceException e;
        if (time < nNow) {
            fRemove = true;
        } else if (govobj.ProcessVote(m_mn_metaman, *this, tip_mn_list, vote, e)) {
            RelayVote(vote);
            fRemove = true;
        }
        if (fRemove) {
            cmmapOrphanVotes.Erase(nHash, pairVote);
        }
    }
}

void CGovernanceManager::AddGovernanceObjectInternal(CGovernanceObject& insert_obj, const CNode* pfrom)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(cs_store);
    AssertLockNotHeld(cs_relay);

    uint256 nHash = insert_obj.GetHash();
    std::string strHash = nHash.ToString();

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();

    // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANAGED DATA

    insert_obj.UpdateSentinelVariables(tip_mn_list); //this sets local vars in object

    std::string strError;

    // MAKE SURE THIS OBJECT IS OK

    if (!insert_obj.IsValidLocally(tip_mn_list, m_chainman, strError, true)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- invalid governance object - %s - (nCachedBlockHeight %d) \n", strError, nCachedBlockHeight);
        return;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- Adding object: hash = %s, type = %d\n", nHash.ToString(),
             ToUnderlying(insert_obj.GetObjectType()));

    // INSERT INTO OUR GOVERNANCE OBJECT MEMORY
    // IF WE HAVE THIS OBJECT ALREADY, WE DON'T WANT ANOTHER COPY
    auto [emplace_ret, emplace_status] = mapObjects.emplace(nHash, std::make_shared<CGovernanceObject>(insert_obj));

    if (!emplace_status) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- already have governance object %s\n", nHash.ToString());
        return;
    }

    // SHOULD WE ADD THIS OBJECT TO ANY OTHER MANAGERS?

    auto& [_, govobj] = *emplace_ret;
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- Before trigger block, GetDataAsPlainString = %s, nObjectType = %d\n",
                Assert(govobj)->GetDataAsPlainString(), ToUnderlying(govobj->GetObjectType()));

    if (govobj->GetObjectType() == GovernanceObject::TRIGGER && !AddNewTrigger(nHash)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- undo adding invalid trigger object: hash = %s\n", nHash.ToString());
        govobj->PrepareDeletion(GetTime<std::chrono::seconds>().count());
        return;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- %s new, received from peer %s\n", strHash, pfrom ? pfrom->GetLogString() : "nullptr");
    RelayObject(*govobj);

    // Update the rate buffer
    MasternodeRateUpdate(*govobj);

    m_mn_sync.BumpAssetLastTime("CGovernanceManager::AddGovernanceObject");

    // WE MIGHT HAVE PENDING/ORPHAN VOTES FOR THIS OBJECT

    CheckOrphanVotes(*govobj);

    // SEND NOTIFICATION TO SCRIPT/ZMQ
    GetMainSignals().NotifyGovernanceObject(std::make_shared<const Governance::Object>(govobj->Object()), nHash.ToString());
    uiInterface.NotifyGovernanceChanged();
}

void CGovernanceManager::AddGovernanceObject(CGovernanceObject& govobj, const CNode* pfrom)
{
    LOCK2(::cs_main, cs_store);
    AddGovernanceObjectInternal(govobj, pfrom);
}

void CGovernanceManager::CheckAndRemove()
{
    AssertLockNotHeld(cs_store);
    assert(m_mn_metaman.IsValid());

    // Return on initial sync, spammed the debug.log and provided no use
    if (!m_mn_sync.IsBlockchainSynced()) return;

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean\n");

    std::vector<uint256> vecDirtyHashes = m_mn_metaman.GetAndClearDirtyGovernanceObjectHashes();

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();

    {
    LOCK2(::cs_main, cs_store);

    for (const uint256& nHash : vecDirtyHashes) {
        auto it = mapObjects.find(nHash);
        if (it == mapObjects.end()) {
            continue;
        }
        Assert(it->second)->ClearMasternodeVotes(tip_mn_list);
    }

    ScopedLockBool guard(cs_store, fRateChecksEnabled, false);

    // Clean up any expired or invalid triggers
    CleanAndRemoveTriggers();

    const auto nNow = GetTime<std::chrono::seconds>();
    for (auto it = mapObjects.begin(); it != mapObjects.end();) {
        auto [nHash, pObj] = *it;
        std::string strHash = nHash.ToString();

        // IF CACHE IS NOT DIRTY, WHY DO THIS?
        if (Assert(pObj)->IsSetDirtyCache()) {
            // UPDATE LOCAL VALIDITY AGAINST CRYPTO DATA
            pObj->UpdateLocalValidity(tip_mn_list, m_chainman);

            // UPDATE SENTINEL SIGNALING VARIABLES
            pObj->UpdateSentinelVariables(tip_mn_list);
        }

        // IF DELETE=TRUE, THEN CLEAN THE MESS UP!

        const auto nTimeSinceDeletion = nNow - std::chrono::seconds{pObj->GetDeletionTime()};

        LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- Checking object for deletion: %s, deletion time = %d, time since deletion = %d, delete flag = %d, expired flag = %d\n",
            strHash, pObj->GetDeletionTime(), nTimeSinceDeletion.count(), pObj->IsSetCachedDelete(), pObj->IsSetExpired());

        if ((pObj->IsSetCachedDelete() || pObj->IsSetExpired()) &&
            (nTimeSinceDeletion >= GOVERNANCE_DELETION_DELAY)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- erase obj %s\n", nHash.ToString());
            m_mn_metaman.RemoveGovernanceObject(pObj->GetHash());

            // Remove vote references
            const object_ref_cm_t::list_t& listItems = cmapVoteToObject.GetItemList();
            for (auto lit = listItems.begin(); lit != listItems.end();) {
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
                nTimeExpired = (std::chrono::seconds{pObj->GetCreationTime()} +
                                std::chrono::seconds{2 * nSuperblockCycleSeconds} + GOVERNANCE_DELETION_DELAY)
                                   .count();
            }

            mapErasedGovernanceObjects.insert(std::make_pair(nHash, nTimeExpired));
            mapObjects.erase(it++);
        } else {
            if (pObj->GetObjectType() == GovernanceObject::PROPOSAL) {
                CProposalValidator validator(pObj->GetDataAsHexString());
                if (!validator.Validate()) {
                    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- set for deletion expired obj %s\n", strHash);
                    pObj->PrepareDeletion(nNow.count());
                }
            }
            ++it;
        }
    }

    // forget about expired deleted objects
    for (auto s_it = mapErasedGovernanceObjects.begin(); s_it != mapErasedGovernanceObjects.end();) {
        if (s_it->second < nNow.count()) {
            mapErasedGovernanceObjects.erase(s_it++);
        } else {
            ++s_it;
        }
    }

    // forget about expired requests
    for (auto r_it = m_requested_hash_time.begin(); r_it != m_requested_hash_time.end();) {
        if (r_it->second < nNow) {
            m_requested_hash_time.erase(r_it++);
        } else {
            ++r_it;
        }
    }
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- %s, m_requested_hash_time size=%d\n",
             ToString(), m_requested_hash_time.size());
}

std::vector<CInv> CGovernanceManager::FetchRelayInventory()
{
    std::vector<CInv> ret;
    LOCK(cs_relay);
    swap(ret, m_relay_invs);
    return ret;
}

std::shared_ptr<const CGovernanceObject> CGovernanceManager::FindConstGovernanceObject(const uint256& nHash) const
{
    LOCK(cs_store);
    return FindConstGovernanceObjectInternal(nHash);
}

std::shared_ptr<const CGovernanceObject> CGovernanceManager::FindConstGovernanceObjectInternal(const uint256& nHash) const
{
    AssertLockHeld(cs_store);

    auto it = mapObjects.find(nHash);
    if (it != mapObjects.end()) return (it->second);

    return nullptr;
}

std::shared_ptr<CGovernanceObject> CGovernanceManager::FindGovernanceObject(const uint256& nHash)
{
    AssertLockNotHeld(cs_store);
    LOCK(cs_store);
    return FindGovernanceObjectInternal(nHash);
}

std::shared_ptr<CGovernanceObject> CGovernanceManager::FindGovernanceObjectInternal(const uint256& nHash)
{
    AssertLockHeld(cs_store);
    if (mapObjects.count(nHash)) return mapObjects[nHash];
    return nullptr;
}

std::shared_ptr<CGovernanceObject> CGovernanceManager::FindGovernanceObjectByDataHash(const uint256 &nDataHash)
{
    AssertLockNotHeld(cs_store);
    LOCK(cs_store);
    for (const auto& [nHash, govobj] : mapObjects) {
        if (Assert(govobj)->GetDataHash() == nDataHash) return govobj;
    }
    return nullptr;
}

std::vector<CGovernanceVote> CGovernanceManager::GetCurrentVotes(const uint256& nParentHash, const COutPoint& mnCollateralOutpointFilter) const
{
    LOCK(cs_store);
    std::vector<CGovernanceVote> vecResult;

    // Find the governance object or short-circuit.
    auto it = mapObjects.find(nParentHash);
    if (it == mapObjects.end()) return vecResult;
    const auto& govobj = *Assert(it->second);

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    std::map<COutPoint, CDeterministicMNCPtr> mapMasternodes;
    if (mnCollateralOutpointFilter.IsNull()) {
        tip_mn_list.ForEachMNShared(/*onlyValid=*/false,
                                    [&](const auto& dmn) { mapMasternodes.emplace(dmn->collateralOutpoint, dmn); });
    } else {
        auto dmn = tip_mn_list.GetMNByCollateral(mnCollateralOutpointFilter);
        if (dmn) {
            mapMasternodes.emplace(dmn->collateralOutpoint, dmn);
        }
    }

    // Loop through each MN collateral outpoint and get the votes for the `nParentHash` governance object
    for (const auto& [outpoint, _] : mapMasternodes) {
        // get a vote_rec_t from the govobj
        vote_rec_t voteRecord;
        if (!govobj.GetCurrentMNVotes(outpoint, voteRecord)) continue;

        for (const auto& [signal, vote_instance] : voteRecord.mapInstances) {
            CGovernanceVote vote = CGovernanceVote(outpoint, nParentHash, (vote_signal_enum_t)signal,
                                                   vote_instance.eOutcome);
            vote.SetTime(vote_instance.nCreationTime);
            vecResult.push_back(vote);
        }
    }

    return vecResult;
}

void CGovernanceManager::GetAllNewerThan(std::vector<CGovernanceObject>& objs, int64_t nMoreThanTime,
                                          bool include_postponed) const
{
    LOCK(cs_store);

    for (const auto& [_, govobj] : mapObjects) {
        if (Assert(govobj)->GetCreationTime() < nMoreThanTime) {
            continue;
        }
        objs.push_back(*govobj);
    }

    if (include_postponed) {
        for (const auto& [_, govobj] : mapPostponedObjects) {
            if (Assert(govobj)->GetCreationTime() < nMoreThanTime) {
                continue;
            }
            objs.push_back(*govobj);
        }
    }
}

bool CGovernanceManager::ConfirmInventoryRequest(const CInv& inv)
{
    AssertLockNotHeld(cs_store);

    // do not request objects until it's time to sync
    if (!m_mn_sync.IsBlockchainSynced()) return false;

    LOCK(cs_store);

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

    const auto valid_until = GetTime<std::chrono::seconds>() + RELIABLE_PROPAGATION_TIME;
    const auto& [_itr, inserted] = m_requested_hash_time.emplace(inv.hash, valid_until);

    if (inserted) {
        LogPrint(BCLog::GOBJECT, /* Continued */
                 "CGovernanceManager::ConfirmInventoryRequest added %s inv hash to m_requested_hash_time, size=%d\n",
                 inv.type == MSG_GOVERNANCE_OBJECT ? "object" : "vote", m_requested_hash_time.size());
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::ConfirmInventoryRequest reached end, returning true\n");
    return true;
}

std::vector<CInv> CGovernanceManager::GetSyncableVoteInvs(const uint256& nProp, const CBloomFilter& filter) const
{
    LOCK(cs_store);

    auto it = mapObjects.find(nProp);
    if (it == mapObjects.end()) {
        return {};
    }

    const auto& govobj = *Assert(it->second);
    if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
        return {};
    }

    std::vector<CInv> invs;
    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();

    LOCK(govobj.cs);
    const auto& fileVotes = govobj.GetVoteFile();
    for (const auto& vote : fileVotes.GetVotes()) {
        uint256 nVoteHash = vote.GetHash();

        bool onlyVotingKeyAllowed = govobj.GetObjectType() == GovernanceObject::PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

        if (filter.contains(nVoteHash) || !vote.IsValid(tip_mn_list, onlyVotingKeyAllowed)) {
            continue;
        }
        invs.emplace_back(MSG_GOVERNANCE_OBJECT_VOTE, nVoteHash);
    }

    return invs;
}

std::vector<CInv> CGovernanceManager::GetSyncableObjectInvs() const
{
    LOCK(cs_store);

    std::vector<CInv> invs;
    invs.reserve(mapObjects.size());

    for (const auto& [nHash, govobj] : mapObjects) {
        if (Assert(govobj)->IsSetCachedDelete() || govobj->IsSetExpired()) {
            continue;
        }
        invs.emplace_back(MSG_GOVERNANCE_OBJECT, nHash);
    }

    return invs;
}

void CGovernanceManager::MasternodeRateUpdate(const CGovernanceObject& govobj)
{
    AssertLockHeld(cs_store);

    if (govobj.GetObjectType() != GovernanceObject::TRIGGER) return;

    const COutPoint& masternodeOutpoint = govobj.GetMasternodeOutpoint();
    auto it = mapLastMasternodeObject.find(masternodeOutpoint);

    if (it == mapLastMasternodeObject.end()) {
        it = mapLastMasternodeObject.insert(txout_m_t::value_type(masternodeOutpoint, last_object_rec(true))).first;
    }

    int64_t nTimestamp = govobj.GetCreationTime();
    it->second.triggerBuffer.AddTimestamp(nTimestamp);

    if (nTimestamp > GetTime() + count_seconds(MAX_TIME_FUTURE_DEVIATION) - count_seconds(RELIABLE_PROPAGATION_TIME)) {
        // schedule additional relay for the object
        setAdditionalRelayObjects.insert(govobj.GetHash());
    }

    it->second.fStatusOK = true;
}

bool CGovernanceManager::MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus)
{
    LOCK(cs_store);
    bool fRateCheckBypassed;
    return MasternodeRateCheck(govobj, fUpdateFailStatus, true, fRateCheckBypassed);
}

bool CGovernanceManager::MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus, bool fForce, bool& fRateCheckBypassed)
{
    AssertLockHeld(cs_store);

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

    if (nTimestamp > nNow + count_seconds(MAX_TIME_FUTURE_DEVIATION)) {
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

bool CGovernanceManager::ProcessVoteAndRelay(const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman)
{
    AssertLockNotHeld(cs_store);
    AssertLockNotHeld(cs_relay);
    uint256 hashToRequest; // Ignored for local votes (no peer to request from)
    bool fOK = ProcessVote(/*pfrom=*/nullptr, vote, exception, hashToRequest);
    if (fOK) {
        RelayVote(vote);
    }
    return fOK;
}

bool CGovernanceManager::ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception,
                                     uint256& hashToRequest)
{
    AssertLockNotHeld(cs_store);
    hashToRequest = uint256{};

    LOCK(cs_store);
    uint256 nHashVote = vote.GetHash();
    uint256 nHashGovobj = vote.GetParentHash();

    if (cmapVoteToObject.HasKey(nHashVote)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::%s -- skipping known valid vote %s for object %s\n", __func__,
            nHashVote.ToString(), nHashGovobj.ToString());
        return false;
    }

    if (cmapInvalidVotes.HasKey(nHashVote)) {
        std::string msg{strprintf("CGovernanceManager::%s -- Old invalid vote, MN outpoint = %s, governance object hash = %s",
            __func__, vote.GetMasternodeOutpoint().ToStringShort(), nHashGovobj.ToString())};
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        return false;
    }

    auto it = mapObjects.find(nHashGovobj);
    if (it == mapObjects.end()) {
        std::string msg{strprintf("CGovernanceManager::%s -- Unknown parent object %s, MN outpoint = %s", __func__,
            nHashGovobj.ToString(), vote.GetMasternodeOutpoint().ToStringShort())};
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_WARNING);
        if (cmmapOrphanVotes.Insert(nHashGovobj, vote_time_pair_t(vote, count_seconds(GetTime<std::chrono::seconds>() +
                                                                                      GOVERNANCE_ORPHAN_EXPIRATION_TIME)))) {
            hashToRequest = nHashGovobj; // Caller should request this object
        }
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        return false;
    }

    auto& govobj = *Assert(it->second);

    if (govobj.IsSetCachedDelete() || govobj.IsSetExpired()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::%s -- ignoring vote for expired or deleted object, hash = %s\n",
            __func__, nHashGovobj.ToString());
        return false;
    }

    bool fOk = govobj.ProcessVote(m_mn_metaman, *this, Assert(m_dmnman)->GetListAtChainTip(), vote, exception);
    if (fOk) {
        fOk = cmapVoteToObject.Insert(nHashVote, it->second);
    } else if (exception.GetType() == GOVERNANCE_EXCEPTION_PERMANENT_ERROR && exception.GetNodePenalty() == 20) {
        cmapInvalidVotes.Insert(nHashVote, vote);
    }
    return fOk;
}

void CGovernanceManager::CheckPostponedObjects()
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(cs_store);
    AssertLockNotHeld(cs_relay);
    if (!m_mn_sync.IsSynced()) return;

    // Check postponed proposals
    for (auto it = mapPostponedObjects.begin(); it != mapPostponedObjects.end();) {
        const uint256& nHash = it->first;
        auto& govobj = *Assert(it->second);

        assert(govobj.GetObjectType() != GovernanceObject::TRIGGER);

        std::string strError;
        bool fMissingConfirmations;
        if (govobj.IsCollateralValid(m_chainman, strError, fMissingConfirmations)) {
            if (govobj.IsValidLocally(Assert(m_dmnman)->GetListAtChainTip(), m_chainman, strError, false)) {
                AddGovernanceObjectInternal(govobj);
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
            const auto& govobj = *Assert(itObject->second);

            int64_t nTimestamp = govobj.GetCreationTime();

            bool fValid = (nTimestamp <= nNow + count_seconds(MAX_TIME_FUTURE_DEVIATION)) &&
                          (nTimestamp >= nNow - 2 * nSuperblockCycleSeconds);
            bool fReady = (nTimestamp <=
                           nNow + count_seconds(MAX_TIME_FUTURE_DEVIATION) - count_seconds(RELIABLE_PROPAGATION_TIME));

            if (fValid) {
                if (fReady) {
                    LogPrint(BCLog::GOBJECT, "CGovernanceManager::CheckPostponedObjects -- additional relay: hash = %s\n", govobj.GetHash().ToString());
                    RelayObject(govobj);
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

CBloomFilter CGovernanceManager::GetVoteBloomFilter(const uint256& nHash) const
{
    LOCK(cs_store);

    auto pObj = FindConstGovernanceObjectInternal(nHash);
    if (!pObj) {
        return CBloomFilter{};
    }

    CBloomFilter filter(Params().GetConsensus().nGovernanceFilterElements, GOVERNANCE_FILTER_FP_RATE,
                        GetRand<int>(/*nMax=*/999999), BLOOM_UPDATE_ALL);

    std::vector<CGovernanceVote> vecVotes = WITH_LOCK(pObj->cs, return pObj->GetVoteFile().GetVotes());
    for (const auto& vote : vecVotes) {
        filter.insert(vote.GetHash());
    }

    return filter;
}

CDeterministicMNManager& CGovernanceManager::GetMNManager() { return *Assert(m_dmnman); }

std::pair<std::vector<uint256>, std::vector<uint256>> CGovernanceManager::FetchGovernanceObjectVotes(
    size_t nPeersPerHashMax, int64_t nNow, std::map<uint256, std::map<CService, int64_t>>& mapAskedRecently) const
{
    std::vector<uint256> vTriggerObjHashes;
    std::vector<uint256> vOtherObjHashes;
    {
        LOCK(cs_store);

        for (const auto& [nHash, govobj] : mapObjects) {
            if (Assert(govobj)->IsSetCachedDelete()) continue;
            if (mapAskedRecently.count(nHash)) {
                for (auto it = mapAskedRecently[nHash].begin(); it != mapAskedRecently[nHash].end();) {
                    if (it->second < nNow) {
                        mapAskedRecently[nHash].erase(it++);
                    } else {
                        ++it;
                    }
                }
                if (mapAskedRecently[nHash].size() >= nPeersPerHashMax) continue;
            }

            if (govobj->GetObjectType() == GovernanceObject::TRIGGER) {
                vTriggerObjHashes.push_back(nHash);
            } else {
                vOtherObjHashes.push_back(nHash);
            }
        }
    }
    return {vTriggerObjHashes, vOtherObjHashes};
}

bool CGovernanceManager::AcceptMessage(const uint256& nHash)
{
    AssertLockNotHeld(cs_store);
    LOCK(cs_store);
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
    AssertLockHeld(cs_store);

    cmapVoteToObject.Clear();
    for (auto& [_, govobj] : mapObjects) {
        assert(govobj);
        std::vector<CGovernanceVote> vecVotes = WITH_LOCK(govobj->cs, return govobj->GetVoteFile().GetVotes());
        for (const auto& vecVote : vecVotes) {
            cmapVoteToObject.Insert(vecVote.GetHash(), govobj);
        }
    }
}

void CGovernanceManager::AddCachedTriggers()
{
    AssertLockHeld(cs_store);

    int64_t nNow = GetTime<std::chrono::seconds>().count();

    for (auto& [_, govobj] : mapObjects) {
        if (Assert(govobj)->GetObjectType() != GovernanceObject::TRIGGER) {
            continue;
        }

        if (!AddNewTrigger(govobj->GetHash())) {
            govobj->PrepareDeletion(nNow);
        }
    }
}

void CGovernanceManager::InitOnLoad()
{
    {
    LOCK(cs_store);
    const auto start{SteadyClock::now()};
    LogPrintf("Preparing masternode indexes and governance triggers...\n");
    RebuildIndexes();
    AddCachedTriggers();
    LogPrintf("Masternode indexes and governance triggers prepared  %dms\n",
              Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
    }
    LogPrintf("     %s\n", ToString());
}

void GovernanceStore::Clear()
{
    LOCK(cs_store);
    mapObjects.clear();
    mapErasedGovernanceObjects.clear();
    cmapInvalidVotes.Clear();
    cmmapOrphanVotes.Clear();
    mapLastMasternodeObject.clear();
    lastMNListForVotingKeys = std::make_shared<CDeterministicMNList>();
}

void CGovernanceManager::Clear()
{
    AssertLockNotHeld(cs_store);
    LogPrint(BCLog::GOBJECT, "Governance object manager was cleared\n");
    GovernanceStore::Clear();
    nTimeLastDiff = 0;
    nCachedBlockHeight = 0;
    cmapVoteToObject.Clear();
    mapPostponedObjects.clear();
    setAdditionalRelayObjects.clear();
    m_requested_hash_time.clear();
    fRateChecksEnabled = true;
    mapTrigger.clear();
}

std::string GovernanceStore::ToString() const
{
    LOCK(cs_store);

    int nProposalCount = 0;
    int nTriggerCount = 0;
    int nOtherCount = 0;

    for (const auto& [_, govobj] : mapObjects) {
        switch (Assert(govobj)->GetObjectType()) {
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

    return strprintf("Governance Objects: %d (Proposals: %d, Triggers: %d, Other: %d; Erased: %d)",
        (int)mapObjects.size(),
        nProposalCount, nTriggerCount, nOtherCount, (int)mapErasedGovernanceObjects.size());
}

std::string CGovernanceManager::ToString() const
{
    AssertLockNotHeld(cs_store);
    return strprintf("%s, Votes: %d", GovernanceStore::ToString(), (int)cmapVoteToObject.GetSize());
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex* pindex)
{
    AssertLockNotHeld(cs_store);
    AssertLockNotHeld(cs_relay);
    // Note this gets called from ActivateBestChain without cs_main being held
    // so it should be safe to lock our mutex here without risking a deadlock
    // On the other hand it should be safe for us to access pindex without holding a lock
    // on cs_main because the CBlockIndex objects are dynamically allocated and
    // presumably never deleted.
    if (!pindex) {
        return;
    }

    nCachedBlockHeight = pindex->nHeight;
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdatedBlockTip -- nCachedBlockHeight: %d\n", nCachedBlockHeight);

    LOCK2(::cs_main, cs_store);
    if (DeploymentDIP0003Enforced(pindex->nHeight, Params().GetConsensus())) {
        RemoveInvalidVotes();
    }

    CheckPostponedObjects();

    ExecuteBestSuperblock(Assert(m_dmnman)->GetListAtChainTip(), pindex->nHeight);
}

std::vector<uint256> CGovernanceManager::GetOrphanVoteObjectHashes()
{
    LOCK(cs_store);

    int64_t nNow = GetTime<std::chrono::seconds>().count();

    // Clean up expired orphan votes
    const vote_cmm_t::list_t& items = cmmapOrphanVotes.GetItemList();
    for (auto it = items.begin(); it != items.end();) {
        auto prevIt = it;
        ++it;
        const auto& [_, time] = prevIt->value;
        if (time < nNow) {
            cmmapOrphanVotes.Erase(prevIt->key, prevIt->value);
        }
    }

    // Get hashes of objects we don't have yet
    std::vector<uint256> vecHashesFiltered;
    std::vector<uint256> vecHashes;
    cmmapOrphanVotes.GetKeys(vecHashes);
    for (const uint256& nHash : vecHashes) {
        if (mapObjects.find(nHash) == mapObjects.end()) {
            vecHashesFiltered.push_back(nHash);
        }
    }

    return vecHashesFiltered;
}

void CGovernanceManager::RemoveInvalidVotes()
{
    AssertLockHeld(cs_store);

    if (!m_mn_sync.IsSynced()) {
        return;
    }

    const auto tip_mn_list = Assert(m_dmnman)->GetListAtChainTip();
    auto diff = lastMNListForVotingKeys->BuildDiff(tip_mn_list);

    std::vector<COutPoint> changedKeyMNs;
    for (const auto& [internalId, pdmnState] : diff.updatedMNs) {
        auto oldDmn = lastMNListForVotingKeys->GetMNByInternalId(internalId);
        // BuildDiff will construct itself with MNs that we already have knowledge
        // of, meaning that fetch operations should never fail.
        assert(oldDmn);
        if ((pdmnState.fields & CDeterministicMNStateDiff::Field_keyIDVoting) && pdmnState.state.keyIDVoting != oldDmn->pdmnState->keyIDVoting) {
            changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
        } else if ((pdmnState.fields & CDeterministicMNStateDiff::Field_pubKeyOperator) && pdmnState.state.pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
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
        for (auto& [_, govobj] : mapObjects) {
            auto removed = Assert(govobj)->RemoveInvalidVotes(tip_mn_list, outpoint);
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
    AssertLockHeld(cs_store);

    // IF WE ALREADY HAVE THIS HASH, RETURN
    if (mapTrigger.count(nHash)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Already have hash, nHash = %s, count = %d, size = %s\n",
                 __func__, nHash.GetHex(), mapTrigger.count(nHash), mapTrigger.size());
        return false;
    }

    CSuperblock_sptr pSuperblock;
    try {
        auto pGovObj = FindGovernanceObjectInternal(nHash);
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
    AssertLockHeld(cs_store);

    // Remove triggers that are invalid or expired
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- mapTrigger.size() = %d\n", __func__, mapTrigger.size());

    for (auto it = mapTrigger.begin(); it != mapTrigger.end();) {
        bool remove = false;
        std::shared_ptr<CGovernanceObject> pObj = nullptr;
        const CSuperblock_sptr& pSuperblock = it->second;
        if (!pSuperblock) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- nullptr superblock\n", __func__);
            remove = true;
        } else {
            pObj = FindGovernanceObjectInternal(it->first);
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
    AssertLockNotHeld(cs_store);
    LOCK(cs_store);
    return GetActiveTriggersInternal();
}

std::vector<CSuperblock_sptr> CGovernanceManager::GetActiveTriggersInternal() const
{
    AssertLockHeld(cs_store);
    std::vector<CSuperblock_sptr> vecResults;

    // LOOK AT THESE OBJECTS AND COMPILE A VALID LIST OF TRIGGERS
    for (const auto& [nHash, pSuperblock] : mapTrigger) {
        auto pObj = FindConstGovernanceObjectInternal(nHash);
        if (pObj) {
            vecResults.push_back(pSuperblock);
        }
    }

    return vecResults;
}

bool CGovernanceManager::IsSuperblockTriggered(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    AssertLockNotHeld(cs_store);
    LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- Start nBlockHeight = %d\n", nBlockHeight);
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    LOCK(cs_store);
    // GET ALL ACTIVE TRIGGERS
    std::vector<CSuperblock_sptr> vecTriggers = GetActiveTriggersInternal();

    LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- vecTriggers.size() = %d\n", vecTriggers.size());

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock) {
            LogPrintf("IsSuperblockTriggered -- Non-superblock found, continuing\n");
            continue;
        }

        auto pObj = FindGovernanceObjectInternal(pSuperblock->GetGovernanceObjHash());
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
    AssertLockNotHeld(cs_store);
    LOCK(cs_store);
    return GetBestSuperblockInternal(tip_mn_list, pSuperblockRet, nBlockHeight);
}

bool CGovernanceManager::GetBestSuperblockInternal(const CDeterministicMNList& tip_mn_list,
                                                   CSuperblock_sptr& pSuperblockRet, int nBlockHeight)
{
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    AssertLockHeld(cs_store);
    std::vector<CSuperblock_sptr> vecTriggers = GetActiveTriggersInternal();
    int nYesCount = 0;

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock || nBlockHeight != pSuperblock->GetBlockHeight()) {
            continue;
        }

        auto pObj = FindGovernanceObjectInternal(pSuperblock->GetGovernanceObjHash());
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
    LOCK(cs_store);

    // GET THE BEST SUPERBLOCK FOR THIS BLOCK HEIGHT

    CSuperblock_sptr pSuperblock;
    if (!GetBestSuperblockInternal(tip_mn_list, pSuperblock, nBlockHeight)) {
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
    LOCK(cs_store);

    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblockInternal(tip_mn_list, pSuperblock, nBlockHeight)) {
        return pSuperblock->IsValid(active_chain, txNew, nBlockHeight, blockReward);
    }

    return false;
}

void CGovernanceManager::ExecuteBestSuperblock(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    AssertLockHeld(cs_store);

    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblockInternal(tip_mn_list, pSuperblock, nBlockHeight)) {
        // All checks are done in CSuperblock::IsValid via IsBlockValueValid and IsBlockPayeeValid,
        // tip wouldn't be updated if anything was wrong. Mark this trigger as executed.
        pSuperblock->SetExecuted();
    }
}

std::vector<std::shared_ptr<const CGovernanceObject>> CGovernanceManager::GetApprovedProposals(
    const CDeterministicMNList& tip_mn_list)
{
    AssertLockNotHeld(cs_store);

    // Use std::vector of std::shared_ptr<const CGovernanceObject> because CGovernanceObject doesn't support move operations (needed for sorting the vector later)
    std::vector<std::shared_ptr<const CGovernanceObject>> ret{};

    // A proposal is considered passing if (YES votes) >= (Total Weight of Masternodes / 10),
    // count total valid (ENABLED) masternodes to determine passing threshold.
    const int nWeightedMnCount = tip_mn_list.GetValidWeightedMNsCount();
    const int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nWeightedMnCount / 10);

    LOCK(cs_store);
    for (const auto& [_, govobj] : mapObjects) {
        // Skip all non-proposals objects
        if (Assert(govobj)->GetObjectType() != GovernanceObject::PROPOSAL) continue;
        // Skip non-passing proposals
        const int absYesCount = govobj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
        if (absYesCount < nAbsVoteReq) continue;
        ret.emplace_back(govobj);
    }

    // Sort approved proposals by absolute Yes votes descending
    std::sort(ret.begin(), ret.end(), [&tip_mn_list](auto& lhs, auto& rhs) {
        const auto lhs_yes = lhs->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
        const auto rhs_yes = rhs->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
        return lhs_yes == rhs_yes ? UintToArith256(lhs->GetHash()) > UintToArith256(rhs->GetHash()) : lhs_yes > rhs_yes;
    });

    return ret;
}

bool AreSuperblocksEnabled(const CSporkManager& sporkman)
{
    return sporkman.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED);
}
