// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governance.h>

#include <evo/deterministicmns.h>
#include <flat-database.h>
#include <governance/common.h>
#include <governance/object.h>
#include <governance/superblock.h>
#include <masternode/meta.h>
#include <masternode/sync.h>

#include <chain.h>
#include <chainparams.h>
#include <common/bloom.h>
#include <deploymentstatus.h>
#include <net.h>
#include <node/interface_ui.h>
#include <protocol.h>
#include <random.h>
#include <timedata.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <univalue.h>

#include <ranges>

const std::string GovernanceStore::SERIALIZATION_VERSION_STRING = "CGovernanceManager-Version-17";

namespace {
constexpr std::chrono::seconds GOVERNANCE_DELETION_DELAY{10min};
constexpr std::chrono::seconds GOVERNANCE_ORPHAN_EXPIRATION_TIME{10min};
constexpr std::chrono::seconds MAX_TIME_FUTURE_DEVIATION{1h};
// Margin below MAX_TIME_FUTURE_DEVIATION so lagging-clock peers don't reject near-limit objects.
constexpr std::chrono::seconds RELIABLE_PROPAGATION_TIME{60};

bool IsSyncableObject(const std::shared_ptr<CGovernanceObject>& govobj)
{
    const auto& obj = *Assert(govobj);
    return !obj.IsSetCachedDelete() && !obj.IsSetExpired();
}

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
    cmmapOrphanVotes(MAX_CACHE_SIZE),
    mapLastMasternodeObject(),
    lastMNListForVotingKeys(std::make_shared<CDeterministicMNList>())
{
}

CGovernanceManager::CGovernanceManager(CMasternodeMetaMan& mn_metaman, const ChainstateManager& chainman,
                                       governance::SuperblockManager& superblocks, CDeterministicMNManager& dmnman,
                                       CMasternodeSync& mn_sync) :
    m_db{std::make_unique<db_type>("governance.dat", "magicGovernanceCache")},
    m_mn_metaman{mn_metaman},
    m_chainman{chainman},
    m_superblocks{superblocks},
    m_dmnman{dmnman},
    m_mn_sync{mn_sync},
    cmapVoteToObject{MAX_CACHE_SIZE},
    mapPostponedObjects{}
{
}

CGovernanceManager::~CGovernanceManager()
{
    // Reset the paired SuperblockManager so its loaded-flag and trigger map don't
    // outlive us. chain_helper (its owner) must outlive govman -- see PrepareShutdown
    // in init.cpp and the test teardown ordering.
    m_superblocks.Clear();
    if (!is_loaded) return;
    m_db->Store(*this);
}

bool CGovernanceManager::LoadCache(bool load_cache)
{
    AssertLockNotHeld(cs_store);
    assert(m_db != nullptr);
    is_loaded = load_cache ? m_db->Load(*this) : m_db->Store(*this);
    if (is_loaded && load_cache) {
        nCachedBlockHeight = WITH_LOCK(::cs_main, return m_chainman.ActiveChain().Height());
        CheckAndRemove();
        InitOnLoad();
    }
    m_superblocks.SetLoaded(is_loaded);
    return is_loaded;
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

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
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

bool CGovernanceManager::HaveObjectForFetch(const uint256& nHash) const
{
    LOCK(cs_store);

    if (mapErasedGovernanceObjects.count(nHash) != 0) {
        return false;
    }

    auto it = mapObjects.find(nHash);
    if (it != mapObjects.end()) {
        return IsSyncableObject(it->second);
    }

    it = mapPostponedObjects.find(nHash);
    if (it != mapPostponedObjects.end()) {
        return IsSyncableObject(it->second);
    }

    return false;
}

bool CGovernanceManager::HaveSyncableObjectForHash(const uint256& nHash) const
{
    LOCK(cs_store);
    const auto it = mapObjects.find(nHash);
    if (it == mapObjects.end()) {
        return false;
    }

    return IsSyncableObject(it->second);
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
    return static_cast<int>(cmapVoteToObject.GetSize());
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

bool CGovernanceManager::ProcessObject(const std::string& peer_str, const uint256& nHash, CGovernanceObject& govobj)
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

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
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

    AddGovernanceObjectInternal(govobj, peer_str);
    return true;
}

void CGovernanceManager::CheckOrphanVotes(CGovernanceObject& govobj)
{
    AssertLockHeld(cs_store);
    AssertLockNotHeld(cs_relay);

    uint256 nHash = govobj.GetHash();
    std::vector<governance::OrphanVote> orphan_votes;
    cmmapOrphanVotes.GetAll(nHash, orphan_votes);

    ScopedLockBool guard(cs_store, fRateChecksEnabled, false);

    const auto now{std::chrono::time_point_cast<std::chrono::seconds>(GetAdjustedTime())};
    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
    for (const auto& orphan_vote : orphan_votes) {
        const auto& vote = orphan_vote.vote;
        bool fRemove = false;
        CGovernanceException e;
        if (orphan_vote.expiration < now) {
            fRemove = true;
        } else if (govobj.ProcessVote(m_mn_metaman, fRateChecksEnabled, tip_mn_list, vote, e)) {
            RelayVote(vote);
            fRemove = true;
        }
        if (fRemove) {
            cmmapOrphanVotes.Erase(nHash, orphan_vote);
        }
    }
}

void CGovernanceManager::AddGovernanceObjectInternal(CGovernanceObject& insert_obj, const std::string& peer_str)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(cs_store);
    AssertLockNotHeld(cs_relay);

    uint256 nHash = insert_obj.GetHash();
    std::string strHash = nHash.ToString();

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();

    // UPDATE CACHED VARIABLES FOR THIS OBJECT AND ADD IT TO OUR MANAGED DATA

    insert_obj.UpdateSentinelVariables(tip_mn_list); //this sets local vars in object

    std::string strError;

    // MAKE SURE THIS OBJECT IS OK

    if (!insert_obj.IsValidLocally(tip_mn_list, m_chainman, strError, true)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- invalid governance object - %s - (nCachedBlockHeight %d) \n", strError, nCachedBlockHeight);
        return;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- Adding object: hash = %s, type = %d\n", nHash.ToString(),
             std23::to_underlying(insert_obj.GetObjectType()));

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
                Assert(govobj)->GetDataAsPlainString(), std23::to_underlying(govobj->GetObjectType()));

    // Count the attempt before AddTrigger can reject it: MasternodeRateCheck allows any
    // masternode without a rate-buffer entry, so a key whose triggers all fail AddTrigger
    // would otherwise never be throttled.
    MasternodeRateUpdate(*govobj);

    if (govobj->GetObjectType() == GovernanceObject::TRIGGER && !m_superblocks.AddTrigger(govobj, nCachedBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- undo adding invalid trigger object: hash = %s\n", nHash.ToString());
        govobj->PrepareDeletion(GetTime<std::chrono::seconds>().count());
        return;
    }

    // Only objects we keep may be announced. Scheduling this before the AddTrigger
    // check would make us re-announce, and serve on GETDATA, a trigger we just
    // undid and marked for deletion.
    ScheduleTriggerRelay(*govobj);

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::AddGovernanceObject -- %s new, received from peer %s\n", strHash, peer_str);
    RelayObject(*govobj);

    m_mn_sync.BumpAssetLastTime("CGovernanceManager::AddGovernanceObject");

    // WE MIGHT HAVE PENDING/ORPHAN VOTES FOR THIS OBJECT

    CheckOrphanVotes(*govobj);

    // SEND NOTIFICATION TO SCRIPT/ZMQ
    GetMainSignals().NotifyGovernanceObject(std::make_shared<const Governance::Object>(govobj->Object()), nHash.ToString());
    uiInterface.NotifyGovernanceChanged();
}

void CGovernanceManager::AddGovernanceObject(CGovernanceObject& govobj, const std::string& peer_str)
{
    LOCK2(::cs_main, cs_store);
    AddGovernanceObjectInternal(govobj, peer_str);
}

void CGovernanceManager::AddGovernanceObjectForTesting(const CGovernanceObject& govobj)
{
    AssertLockNotHeld(cs_store);
    LOCK(cs_store);
    mapObjects.emplace(govobj.GetHash(), std::make_shared<CGovernanceObject>(govobj));
}

void CGovernanceManager::AddOrphanVoteForTesting(const CGovernanceVote& vote)
{
    AssertLockNotHeld(cs_store);
    LOCK(cs_store);
    cmmapOrphanVotes.Insert(vote.GetParentHash(),
                            governance::OrphanVote{vote, Now<NodeSeconds>() + GOVERNANCE_ORPHAN_EXPIRATION_TIME});
}

void CGovernanceManager::CheckAndRemove()
{
    AssertLockNotHeld(cs_store);
    assert(m_mn_metaman.IsValid());

    // Return on initial sync, spammed the debug.log and provided no use
    if (!m_mn_sync.IsBlockchainSynced()) return;

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean\n");

    std::vector<uint256> vecDirtyHashes = m_mn_metaman.GetAndClearDirtyGovernanceObjectHashes();

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();

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
    m_superblocks.Clean(nCachedBlockHeight);

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

        // Read the flags before the deletion time: another thread can mark the object for deletion
        // at any point, and a flag read after a zero deletion time would skip GOVERNANCE_DELETION_DELAY.
        const bool fCachedDelete{pObj->IsSetCachedDelete()};
        const bool fExpired{pObj->IsSetExpired()};
        const int64_t nDeletionTime{pObj->GetDeletionTime()};
        const auto nTimeSinceDeletion = nNow - std::chrono::seconds{nDeletionTime};

        LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- Checking object for deletion: %s, deletion time = %d, time since deletion = %d, delete flag = %d, expired flag = %d\n",
            strHash, nDeletionTime, nTimeSinceDeletion.count(), fCachedDelete, fExpired);

        if ((fCachedDelete || fExpired) && (nTimeSinceDeletion >= GOVERNANCE_DELETION_DELAY)) {
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
            if (pObj->GetObjectType() == GovernanceObject::TRIGGER) {
                m_superblocks.RemoveTrigger(nHash);
            }
            mapObjects.erase(it++);
        } else {
            if (pObj->GetObjectType() == GovernanceObject::PROPOSAL) {
                std::string strValidationError;
                if (!governance::ValidateProposal(pObj->GetDataAsHexString(), strValidationError)) {
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

    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::UpdateCachesAndClean -- %s\n", ToString());
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
    auto it = mapObjects.find(nHash);
    if (it != mapObjects.end()) return it->second;
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

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
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
            CGovernanceVote vote{outpoint, nParentHash, static_cast<vote_signal_enum_t>(signal), vote_instance.eOutcome};
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

    // We don't have it and it's a known type: signal that we want to fetch it.
    // The net-layer per-peer request tracker records the pending request; acceptance
    // of the eventual response is gated on that tracker (see NetGovernance::ProcessMessage).
    return true;
}

std::vector<CInv> CGovernanceManager::GetSyncableVoteInvs(const uint256& nProp, const CBloomFilter& filter) const
{
    LOCK(cs_store);

    auto it = mapObjects.find(nProp);
    if (it == mapObjects.end()) {
        return {};
    }

    if (!IsSyncableObject(it->second)) {
        return {};
    }

    std::vector<CInv> invs;
    const auto tip_mn_list = m_dmnman.GetListAtChainTip();

    const auto& govobj = *Assert(it->second);
    LOCK(govobj.cs);
    // Visit the stored votes in place: CheckSignature memoises its verdict on
    // the vote instance, and a GetVotes() copy would discard that memo, so every
    // walk would pay a fresh ECDSA recovery or BLS pairing per vote.
    invs.reserve(govobj.GetVoteFile().GetVoteCount());
    govobj.GetVoteFile().ForEachVote([&](const CGovernanceVote& vote) {
        uint256 nVoteHash = vote.GetHash();

        bool onlyVotingKeyAllowed = govobj.GetObjectType() == GovernanceObject::PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

        if (filter.contains(nVoteHash) || !vote.IsValid(tip_mn_list, onlyVotingKeyAllowed)) {
            return;
        }
        invs.emplace_back(MSG_GOVERNANCE_OBJECT_VOTE, nVoteHash);
    });

    return invs;
}

std::vector<CInv> CGovernanceManager::GetSyncableObjectInvs() const
{
    LOCK(cs_store);

    std::vector<CInv> invs;
    invs.reserve(mapObjects.size());

    for (const auto& [nHash, govobj] : mapObjects) {
        if (!IsSyncableObject(govobj)) {
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
    auto it = mapLastMasternodeObject.try_emplace(masternodeOutpoint, /*fStatusOKIn=*/true).first;

    it->second.triggerBuffer.AddTimestamp(govobj.GetCreationTime());
    it->second.fStatusOK = true;
}

void CGovernanceManager::ScheduleTriggerRelay(const CGovernanceObject& govobj)
{
    AssertLockHeld(cs_store);

    if (govobj.GetObjectType() != GovernanceObject::TRIGGER) return;

    // A trigger created this close to the future-deviation limit is still too new for
    // peers with a lagging clock to accept, so re-announce it once it has aged past
    // RELIABLE_PROPAGATION_TIME (see CheckPostponedObjects).
    if (govobj.GetCreationTime() >
        GetTime() + count_seconds(MAX_TIME_FUTURE_DEVIATION) - count_seconds(RELIABLE_PROPAGATION_TIME)) {
        setAdditionalRelayObjects.insert(govobj.GetHash());
    }
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
    const auto timestamp{govobj.CreationTime()};
    const auto now{std::chrono::time_point_cast<std::chrono::seconds>(GetAdjustedTime())};
    const auto superblock_cycle{Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().PowTargetSpacing()};

    std::string strHash = govobj.GetHash().ToString();

    if (timestamp < now - 2 * superblock_cycle) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::MasternodeRateCheck -- object %s rejected due to too old timestamp, masternode = %s, timestamp = %d, current time = %d\n",
            strHash, masternodeOutpoint.ToStringShort(), govobj.GetCreationTime(), TicksSinceEpoch<std::chrono::seconds>(now));
        return false;
    }

    if (timestamp > now + MAX_TIME_FUTURE_DEVIATION) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::MasternodeRateCheck -- object %s rejected due to too new (future) timestamp, masternode = %s, timestamp = %d, current time = %d\n",
            strHash, masternodeOutpoint.ToStringShort(), govobj.GetCreationTime(), TicksSinceEpoch<std::chrono::seconds>(now));
        return false;
    }

    auto it = mapLastMasternodeObject.find(masternodeOutpoint);
    if (it == mapLastMasternodeObject.end()) return true;

    if (it->second.fStatusOK && !fForce) {
        fRateCheckBypassed = true;
        return true;
    }

    // Allow 1 trigger per mn per cycle, with a small fudge factor
    double dMaxRate = 2 * 1.1 / static_cast<double>(count_seconds(superblock_cycle));

    // Temporary copy to check rate after new timestamp is added
    CRateCheckBuffer buffer = it->second.triggerBuffer;

    buffer.AddTimestamp(govobj.GetCreationTime());
    double dRate = buffer.GetRate();

    if (dRate < dMaxRate) {
        return true;
    }

    LogPrint(BCLog::GOBJECT, "CGovernanceManager::MasternodeRateCheck -- Rate too high: object hash = %s, masternode = %s, object timestamp = %d, rate = %f, max rate = %f\n",
        strHash, masternodeOutpoint.ToStringShort(), govobj.GetCreationTime(), dRate, dMaxRate);

    if (fUpdateFailStatus) {
        it->second.fStatusOK = false;
    }

    return false;
}

bool CGovernanceManager::ProcessVoteAndRelay(const CGovernanceVote& vote, CGovernanceException& exception)
{
    AssertLockNotHeld(cs_store);
    AssertLockNotHeld(cs_relay);
    uint256 hashToRequest; // Ignored for local votes (no peer to request from)
    bool fOK = ProcessVote(vote, exception, hashToRequest);
    if (fOK) {
        RelayVote(vote);
    }
    return fOK;
}

bool CGovernanceManager::ProcessVote(const CGovernanceVote& vote, CGovernanceException& exception,
                                     uint256& hashToRequest)
{
    AssertLockNotHeld(cs_store);
    hashToRequest = uint256{};

    const auto tip_mn_list{m_dmnman.GetListAtChainTip()};

    LOCK(cs_store);
    uint256 nHashVote = vote.GetHash();
    uint256 nHashGovobj = vote.GetParentHash();

    if (cmapVoteToObject.HasKey(nHashVote)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::%s -- skipping known valid vote %s for object %s\n", __func__,
            nHashVote.ToString(), nHashGovobj.ToString());
        return false;
    }

    auto it = mapObjects.find(nHashGovobj);
    if (it == mapObjects.end()) {
        if (!vote.IsValidForUnknownParent(tip_mn_list)) {
            std::string msg{strprintf("CGovernanceManager::%s -- Invalid vote for unknown parent object %s, MN outpoint = %s, vote hash = %s",
                __func__, nHashGovobj.ToString(), vote.GetMasternodeOutpoint().ToStringShort(), nHashVote.ToString())};
            LogPrint(BCLog::GOBJECT, "%s\n", msg);
            exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
            return false;
        }
        std::string msg{strprintf("CGovernanceManager::%s -- Unknown parent object %s, MN outpoint = %s", __func__,
            nHashGovobj.ToString(), vote.GetMasternodeOutpoint().ToStringShort())};
        // No penalty: the vote is signed by a masternode, it just arrived before its parent object,
        // which routinely happens during governance sync. Misbehaviour scores never decay.
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_WARNING);
        if (cmmapOrphanVotes.Insert(nHashGovobj, governance::OrphanVote{vote, Now<NodeSeconds>() + GOVERNANCE_ORPHAN_EXPIRATION_TIME})) {
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

    bool fOk = govobj.ProcessVote(m_mn_metaman, fRateChecksEnabled, tip_mn_list, vote, exception);
    if (fOk) {
        fOk = cmapVoteToObject.Insert(nHashVote, it->second);
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
            if (govobj.IsValidLocally(m_dmnman.GetListAtChainTip(), m_chainman, strError, false)) {
                AddGovernanceObjectInternal(govobj, "<postponed>");
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
    const auto now{std::chrono::time_point_cast<std::chrono::seconds>(GetAdjustedTime())};
    const auto superblock_cycle{Params().GetConsensus().nSuperblockCycle * Params().GetConsensus().PowTargetSpacing()};

    for (auto it = setAdditionalRelayObjects.begin(); it != setAdditionalRelayObjects.end();) {
        auto itObject = mapObjects.find(*it);
        if (itObject != mapObjects.end()) {
            const auto& govobj = *Assert(itObject->second);

            const auto timestamp{govobj.CreationTime()};

            const bool fValid{timestamp <= now + MAX_TIME_FUTURE_DEVIATION &&
                              timestamp >= now - 2 * superblock_cycle};
            const bool fReady{timestamp <= now + MAX_TIME_FUTURE_DEVIATION - RELIABLE_PROPAGATION_TIME};

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

CDeterministicMNManager& CGovernanceManager::GetMNManager() { return m_dmnman; }

std::pair<std::vector<uint256>, std::vector<uint256>> CGovernanceManager::FetchGovernanceObjectVotes(
    size_t nPeersPerHashMax, int64_t nNow, std::map<uint256, std::map<CService, int64_t>>& mapAskedRecently) const
{
    std::vector<uint256> vTriggerObjHashes;
    std::vector<uint256> vOtherObjHashes;
    {
        LOCK(cs_store);

        for (const auto& [nHash, govobj] : mapObjects) {
            if (Assert(govobj)->IsSetCachedDelete()) continue;
            if (auto asked_it = mapAskedRecently.find(nHash); asked_it != mapAskedRecently.end()) {
                auto& asked_peers = asked_it->second;
                std::erase_if(asked_peers, [&](const auto& asked) { return asked.second < nNow; });
                if (asked_peers.size() >= nPeersPerHashMax) continue;
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

void CGovernanceManager::InitOnLoad()
{
    {
    LOCK(cs_store);
    const auto start{SteadyClock::now()};
    LogPrintf("Preparing masternode indexes and governance triggers...\n");
    RebuildIndexes();

    const int64_t nNow = GetTime<std::chrono::seconds>().count();
    for (auto& [_, govobj] : mapObjects) {
        if (Assert(govobj)->GetObjectType() != GovernanceObject::TRIGGER) continue;
        if (!m_superblocks.AddTrigger(govobj, nCachedBlockHeight)) {
            govobj->PrepareDeletion(nNow);
        }
    }
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
    fRateChecksEnabled = true;
    m_superblocks.Clear();
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
        static_cast<int>(mapObjects.size()),
        nProposalCount, nTriggerCount, nOtherCount, static_cast<int>(mapErasedGovernanceObjects.size()));
}

std::string CGovernanceManager::ToString() const
{
    AssertLockNotHeld(cs_store);
    return strprintf("%s, Votes: %d", GovernanceStore::ToString(), static_cast<int>(cmapVoteToObject.GetSize()));
}

UniValue CGovernanceManager::ToJson() const
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

    UniValue jsonObj(UniValue::VOBJ);
    jsonObj.pushKV("objects_total", mapObjects.size());
    jsonObj.pushKV("proposals", nProposalCount);
    jsonObj.pushKV("triggers", nTriggerCount);
    jsonObj.pushKV("other", nOtherCount);
    jsonObj.pushKV("erased", mapErasedGovernanceObjects.size());
    jsonObj.pushKV("votes", cmapVoteToObject.GetSize());
    return jsonObj;
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

    m_superblocks.ExecuteBestSuperblock(m_dmnman.GetListAtChainTip(), pindex->nHeight);
}

std::vector<uint256> CGovernanceManager::GetOrphanVoteObjectHashes()
{
    LOCK(cs_store);

    const auto now{Now<NodeSeconds>()};

    // Clean up expired orphan votes
    const vote_cmm_t::list_t& items = cmmapOrphanVotes.GetItemList();
    for (auto it = items.begin(); it != items.end();) {
        auto prevIt = it;
        ++it;
        if (prevIt->value.expiration < now) {
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

    // A random subset, so that parent hashes ground to sort first cannot pin which ones are requested
    if (vecHashesFiltered.size() > MAX_ORPHAN_OBJECT_REQUESTS) {
        Shuffle(vecHashesFiltered.begin(), vecHashesFiltered.end(), FastRandomContext());
        vecHashesFiltered.resize(MAX_ORPHAN_OBJECT_REQUESTS);
    }

    return vecHashesFiltered;
}

void CGovernanceManager::RemoveInvalidVotes()
{
    AssertLockHeld(cs_store);

    if (!m_mn_sync.IsSynced()) {
        return;
    }

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
    auto diff = lastMNListForVotingKeys->BuildDiff(tip_mn_list);

    std::vector<COutPoint> changedKeyMNs;
    for (const auto& [internalId, pdmnState] : diff.updatedMNs) {
        auto newDmn = tip_mn_list.GetMNByInternalId(internalId);
        if (!Assume(newDmn)) {
            continue;
        }
        auto oldDmn = lastMNListForVotingKeys->GetMN(newDmn->proTxHash);
        if (!Assume(oldDmn)) {
            continue;
        }
        if ((pdmnState.fields & CDeterministicMNStateDiff::Field_keyIDVoting) && pdmnState.state.keyIDVoting != oldDmn->pdmnState->keyIDVoting) {
            changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
        } else if ((pdmnState.fields & CDeterministicMNStateDiff::Field_pubKeyOperator) && pdmnState.state.pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
            changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
        }
    }
    for (const auto& id : diff.removedMns) {
        auto oldDmn = lastMNListForVotingKeys->GetMNByInternalId(id);
        if (!Assume(oldDmn)) {
            continue;
        }
        changedKeyMNs.emplace_back(oldDmn->collateralOutpoint);
    }

    for (const auto& outpoint : changedKeyMNs) {
        for (auto& [_, govobj] : mapObjects) {
            auto removed = Assert(govobj)->RemoveInvalidVotes(tip_mn_list, outpoint);
            if (removed.empty()) {
                continue;
            }
            for (const auto& voteHash : removed) {
                cmapVoteToObject.Erase(voteHash);
                cmmapOrphanVotes.Erase(voteHash);
            }
        }
    }

    // store current MN list for the next run so that we can determine which keys changed
    lastMNListForVotingKeys = std::make_shared<CDeterministicMNList>(tip_mn_list);
}

std::vector<std::shared_ptr<const CGovernanceObject>> CGovernanceManager::GetApprovedProposals(
    const CDeterministicMNList& tip_mn_list)
{
    AssertLockNotHeld(cs_store);

    // Use std::vector of std::shared_ptr<const CGovernanceObject> because CGovernanceObject doesn't support move operations (needed for sorting the vector later)
    std::vector<std::shared_ptr<const CGovernanceObject>> ret{};

    // A proposal is considered passing if (YES votes) >= (Total Weight of Masternodes / 10),
    // count total valid (ENABLED) masternodes to determine passing threshold.
    const int nWeightedMnCount = tip_mn_list.GetCounts().m_valid_weighted;
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
