// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/deterministicmns.h>
#include <evo/specialtx.h>
#include <evo/simplifiedmns.h>

#include <base58.h>
#include <chainparams.h>
#include <core_io.h>
#include <script/standard.h>
#include <node/ui_interface.h>
#include <validation.h>
#include <validationinterface.h>

#include <llmq/quorums_commitment.h>
#include <llmq/quorums_utils.h>
#include <univalue.h>
#include <shutdown.h>
static const std::string DB_LIST_SNAPSHOT = "dmn_S";
static const std::string DB_LIST_DIFF = "dmn_D";

std::unique_ptr<CDeterministicMNManager> deterministicMNManager;

std::string CDeterministicMNState::ToString() const
{
    CTxDestination dest;
    std::string payoutAddress = "unknown";
    std::string operatorPayoutAddress = "none";
    if (ExtractDestination(scriptPayout, dest)) {
        payoutAddress = EncodeDestination(dest);
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        operatorPayoutAddress = EncodeDestination(dest);
    }

    return strprintf("CDeterministicMNState(nRegisteredHeight=%d, nCollateralHeight=%d, nLastPaidHeight=%d, nPoSePenalty=%d, nPoSeRevivedHeight=%d, nPoSeBanHeight=%d, nRevocationReason=%d, "
        "ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, addr=%s, payoutAddress=%s, operatorPayoutAddress=%s)",
        nRegisteredHeight, nCollateralHeight, nLastPaidHeight, nPoSePenalty, nPoSeRevivedHeight, nPoSeBanHeight, nRevocationReason,
        EncodeDestination(WitnessV0KeyHash(keyIDOwner)), pubKeyOperator.Get().ToString(), EncodeDestination(WitnessV0KeyHash(keyIDVoting)), addr.ToStringIPPort(), payoutAddress, operatorPayoutAddress);
}

void CDeterministicMNState::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("service", addr.ToStringIPPort());
    obj.pushKV("registeredHeight", nRegisteredHeight);
    obj.pushKV("lastPaidHeight", nLastPaidHeight);
    obj.pushKV("collateralHeight", nCollateralHeight);
    obj.pushKV("PoSePenalty", nPoSePenalty);
    obj.pushKV("PoSeRevivedHeight", nPoSeRevivedHeight);
    obj.pushKV("PoSeBanHeight", nPoSeBanHeight);
    obj.pushKV("revocationReason", nRevocationReason);
    obj.pushKV("ownerAddress", EncodeDestination(WitnessV0KeyHash(keyIDOwner)));
    obj.pushKV("votingAddress", EncodeDestination(WitnessV0KeyHash(keyIDVoting)));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        obj.pushKV("payoutAddress", EncodeDestination(dest));
    }
    obj.pushKV("pubKeyOperator", pubKeyOperator.Get().ToString());
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
}

uint64_t CDeterministicMN::GetInternalId() const
{
    // can't get it if it wasn't set yet
    assert(internalId != std::numeric_limits<uint64_t>::max());
    return internalId;
}

std::string CDeterministicMN::ToString() const
{
    return strprintf("CDeterministicMN(proTxHash=%s, collateralOutpoint=%s, nOperatorReward=%f, state=%s", proTxHash.ToString(), collateralOutpoint.ToStringShort(), (double)nOperatorReward / 100, pdmnState->ToString());
}

void CDeterministicMN::ToJson(ChainstateManager& chainman, UniValue& obj) const
{
    obj.clear();
    obj.setObject();

    UniValue stateObj;
    pdmnState->ToJson(stateObj);

    obj.pushKV("proTxHash", proTxHash.ToString());
    obj.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    obj.pushKV("collateralIndex", (int)collateralOutpoint.n);

    Coin coin;
    if (GetUTXOCoin(chainman, collateralOutpoint, coin)) {
        CTxDestination dest;
        if (ExtractDestination(coin.out.scriptPubKey, dest)) {
            obj.pushKV("collateralAddress", EncodeDestination(dest));
        }
    }

    obj.pushKV("operatorReward", (double)nOperatorReward / 100);
    obj.pushKV("state", stateObj);
}

bool CDeterministicMNList::IsMNValid(const uint256& proTxHash) const
{
    auto p = mnMap.find(proTxHash);
    if (p == nullptr) {
        return false;
    }
    return IsMNValid(*p);
}

bool CDeterministicMNList::IsMNPoSeBanned(const uint256& proTxHash) const
{
    auto p = mnMap.find(proTxHash);
    if (p == nullptr) {
        return false;
    }
    return IsMNPoSeBanned(*p);
}

bool CDeterministicMNList::IsMNValid(const CDeterministicMNCPtr& dmn)
{
    return !IsMNPoSeBanned(dmn);
}

bool CDeterministicMNList::IsMNPoSeBanned(const CDeterministicMNCPtr& dmn)
{
    assert(dmn);
    return dmn->pdmnState->IsBanned();
}

CDeterministicMNCPtr CDeterministicMNList::GetMN(const uint256& proTxHash) const
{
    auto p = mnMap.find(proTxHash);
    if (p == nullptr) {
        return nullptr;
    }
    return *p;
}

CDeterministicMNCPtr CDeterministicMNList::GetValidMN(const uint256& proTxHash) const
{
    auto dmn = GetMN(proTxHash);
    if (dmn && !IsMNValid(dmn)) {
        return nullptr;
    }
    return dmn;
}

CDeterministicMNCPtr CDeterministicMNList::GetMNByOperatorKey(const CBLSPublicKey& pubKey)
{
    for (const auto& p : mnMap) {
        if (p.second->pdmnState->pubKeyOperator.Get() == pubKey) {
            return p.second;
        }
    }
    return nullptr;
}

CDeterministicMNCPtr CDeterministicMNList::GetMNByCollateral(const COutPoint& collateralOutpoint) const
{
    return GetUniquePropertyMN(collateralOutpoint);
}

CDeterministicMNCPtr CDeterministicMNList::GetValidMNByCollateral(const COutPoint& collateralOutpoint) const
{
    auto dmn = GetMNByCollateral(collateralOutpoint);
    if (dmn && !IsMNValid(dmn)) {
        return nullptr;
    }
    return dmn;
}

CDeterministicMNCPtr CDeterministicMNList::GetMNByService(const CService& service) const
{
    return GetUniquePropertyMN(service);
}

CDeterministicMNCPtr CDeterministicMNList::GetMNByInternalId(uint64_t internalId) const
{
    auto proTxHash = mnInternalIdMap.find(internalId);
    if (!proTxHash) {
        return nullptr;
    }
    return GetMN(*proTxHash);
}

static int CompareByLastPaid_GetHeight(const CDeterministicMN& dmn)
{
    int height = dmn.pdmnState->nLastPaidHeight;
    if (dmn.pdmnState->nPoSeRevivedHeight != -1 && dmn.pdmnState->nPoSeRevivedHeight > height) {
        height = dmn.pdmnState->nPoSeRevivedHeight;
    } else if (height == 0) {
        height = dmn.pdmnState->nRegisteredHeight;
    }
    return height;
}

static bool CompareByLastPaid(const CDeterministicMN& _a, const CDeterministicMN& _b)
{
    int ah = CompareByLastPaid_GetHeight(_a);
    int bh = CompareByLastPaid_GetHeight(_b);
    if (ah == bh) {
        return _a.proTxHash < _b.proTxHash;
    } else {
        return ah < bh;
    }
}
static bool CompareByLastPaid(const CDeterministicMNCPtr& _a, const CDeterministicMNCPtr& _b)
{
    return CompareByLastPaid(*_a, *_b);
}

CDeterministicMNCPtr CDeterministicMNList::GetMNPayee() const
{
    if (mnMap.size() == 0) {
        return nullptr;
    }

    CDeterministicMNCPtr best;
    ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        if (!best || CompareByLastPaid(dmn, best)) {
            best = dmn;
        }
    });

    return best;
}

void CDeterministicMNList::GetProjectedMNPayees(size_t nCount, std::vector<CDeterministicMNCPtr>& result) const
{
    if (nCount > GetValidMNsCount()) {
        nCount = GetValidMNsCount();
    }

    result.clear();
    result.reserve(nCount);

    ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        result.emplace_back(dmn);
    });
    std::sort(result.begin(), result.end(), [&](const CDeterministicMNCPtr& a, const CDeterministicMNCPtr& b) {
        return CompareByLastPaid(a, b);
    });

    result.resize(nCount);
}

void CDeterministicMNList::CalculateQuorum(size_t maxSize, const uint256& modifier, std::vector<CDeterministicMNCPtr> &members) const
{
    members.clear();
    std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> scores;
    CalculateScores(modifier, scores);

    // sort is descending order
    std::sort(scores.rbegin(), scores.rend(), [](const std::pair<arith_uint256, CDeterministicMNCPtr>& a, std::pair<arith_uint256, CDeterministicMNCPtr>& b) {
        if (a.first == b.first) {
            // this should actually never happen, but we should stay compatible with how the non deterministic MNs did the sorting
            return a.second->collateralOutpoint < b.second->collateralOutpoint;
        }
        return a.first < b.first;
    });

    // take top maxSize entries and return it
    members.resize(std::min(maxSize, scores.size()));
    for (size_t i = 0; i < members.size(); i++) {
        members[i] = std::move(scores[i].second);
    }
}

void CDeterministicMNList::CalculateScores(const uint256& modifier, std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> &scores) const
{
    scores.clear();
    scores.reserve(GetAllMNsCount());
    ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        if (dmn->pdmnState->confirmedHash.IsNull()) {
            // we only take confirmed MNs into account to avoid hash grinding on the ProRegTxHash to sneak MNs into a
            // future quorums
            return;
        }
        // calculate sha256(sha256(proTxHash, confirmedHash), modifier) per MN
        // Please note that this is not a double-sha256 but a single-sha256
        // The first part is already precalculated (confirmedHashWithProRegTxHash)
        // TODO When https://github.com/bitcoin/bitcoin/pull/13191 gets backported, implement something that is similar but for single-sha256
        uint256 h;
        CSHA256 sha256;
        sha256.Write(dmn->pdmnState->confirmedHashWithProRegTxHash.begin(), dmn->pdmnState->confirmedHashWithProRegTxHash.size());
        sha256.Write(modifier.begin(), modifier.size());
        sha256.Finalize(h.begin());

        scores.emplace_back(UintToArith256(h), dmn);
    });

}

int CDeterministicMNList::CalcMaxPoSePenalty() const
{
    // Maximum PoSe penalty is dynamic and equals the number of registered MNs
    // It's however at least 100.
    // This means that the max penalty is usually equal to a full payment cycle
    return std::max(100, (int)GetAllMNsCount());
}

int CDeterministicMNList::CalcPenalty(int percent) const
{
    assert(percent > 0);
    return (CalcMaxPoSePenalty() * percent) / 100;
}

void CDeterministicMNList::PoSePunish(const uint256& proTxHash, int penalty, bool debugLogs)
{
    assert(penalty > 0);

    auto dmn = GetMN(proTxHash);
    if (!dmn) {
        throw(std::runtime_error(strprintf("%s: Can't find a masternode with proTxHash=%s", __func__, proTxHash.ToString())));
    }

    int maxPenalty = CalcMaxPoSePenalty();

    auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
    newState->nPoSePenalty += penalty;
    newState->nPoSePenalty = std::min(maxPenalty, newState->nPoSePenalty);

    if (debugLogs) {
        LogPrintf("CDeterministicMNList::%s -- punished MN %s, penalty %d->%d (max=%d)\n",
                  __func__, proTxHash.ToString(), dmn->pdmnState->nPoSePenalty, newState->nPoSePenalty, maxPenalty);
    }

    if (newState->nPoSePenalty >= maxPenalty && !newState->IsBanned()) {
        newState->BanIfNotBanned(nHeight);
        if (debugLogs) {
            LogPrintf("CDeterministicMNList::%s -- banned MN %s at height %d\n",
                      __func__, proTxHash.ToString(), nHeight);
        }
    }
    UpdateMN(proTxHash, newState);
}

void CDeterministicMNList::PoSeDecrease(const uint256& proTxHash)
{
    auto dmn = GetMN(proTxHash);
    if (!dmn) {
        throw(std::runtime_error(strprintf("%s: Can't find a masternode with proTxHash=%s", __func__, proTxHash.ToString())));
    }
    assert(dmn->pdmnState->nPoSePenalty > 0 && !dmn->pdmnState->IsBanned());

    auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
    newState->nPoSePenalty--;
    UpdateMN(proTxHash, newState);
}

void CDeterministicMNList::BuildDiff(const CDeterministicMNList& to, CDeterministicMNListDiff &diffRet) const
{

    to.ForEachMN(false, [&](const CDeterministicMNCPtr& toPtr) {
        auto fromPtr = GetMN(toPtr->proTxHash);
        if (fromPtr == nullptr) {
            diffRet.addedMNs.emplace_back(toPtr);
        } else if (fromPtr != toPtr || fromPtr->pdmnState != toPtr->pdmnState) {
            CDeterministicMNStateDiff stateDiff(*fromPtr->pdmnState, *toPtr->pdmnState);
            if (stateDiff.fields) {
                diffRet.updatedMNs.try_emplace(toPtr->GetInternalId(), std::move(stateDiff));
            }
        }
    });
    ForEachMN(false, [&](const CDeterministicMNCPtr& fromPtr) {
        auto toPtr = to.GetMN(fromPtr->proTxHash);
        if (toPtr == nullptr) {
            diffRet.removedMns.emplace(fromPtr->GetInternalId());
        }
    });

    // added MNs need to be sorted by internalId so that these are added in correct order when the diff is applied later
    // otherwise internalIds will not match with the original list
    std::sort(diffRet.addedMNs.begin(), diffRet.addedMNs.end(), [](const CDeterministicMNCPtr& a, const CDeterministicMNCPtr& b) {
        return a->GetInternalId() < b->GetInternalId();
    });
}

CSimplifiedMNListDiff CDeterministicMNList::BuildSimplifiedDiff(const CDeterministicMNList& to) const
{
    CSimplifiedMNListDiff diffRet;
    diffRet.baseBlockHash = blockHash;
    diffRet.blockHash = to.blockHash;

    to.ForEachMN(false, [&](const CDeterministicMNCPtr& toPtr) {
        auto fromPtr = GetMN(toPtr->proTxHash);
        if (fromPtr == nullptr) {
            diffRet.mnList.emplace_back(*toPtr);
        } else {
            CSimplifiedMNListEntry sme1(*toPtr);
            CSimplifiedMNListEntry sme2(*fromPtr);
            if (sme1 != sme2) {
                diffRet.mnList.emplace_back(*toPtr);
            }
        }
    });
    ForEachMN(false, [&](const CDeterministicMNCPtr& fromPtr) {
        auto toPtr = to.GetMN(fromPtr->proTxHash);
        if (toPtr == nullptr) {
            diffRet.deletedMNs.emplace_back(fromPtr->proTxHash);
        }
    });

    return diffRet;
}

CDeterministicMNList CDeterministicMNList::ApplyDiff(const CBlockIndex* pindex, const CDeterministicMNListDiff& diff) const
{
    CDeterministicMNList result = *this;
    result.blockHash = pindex->GetBlockHash();
    result.nHeight = pindex->nHeight;

    for (const auto& id : diff.removedMns) {
        auto dmn = result.GetMNByInternalId(id);
        if (!dmn) {
            throw(std::runtime_error(strprintf("%s: can't find a removed masternode, id=%d", __func__, id)));
        }
        result.RemoveMN(dmn->proTxHash);
    }
    for (const auto& dmn : diff.addedMNs) {
        result.AddMN(dmn);
    }
    for (const auto& p : diff.updatedMNs) {
        auto dmn = result.GetMNByInternalId(p.first);
        result.UpdateMN(dmn, p.second);
    }

    return result;
}

void CDeterministicMNList::AddMN(const CDeterministicMNCPtr& dmn, bool fBumpTotalCount)
{
    assert(dmn != nullptr);

    if (mnMap.find(dmn->proTxHash)) {
        throw(std::runtime_error(strprintf("%s: Can't add a masternode with a duplicate proTxHash=%s", __func__, dmn->proTxHash.ToString())));
    }
    if (mnInternalIdMap.find(dmn->GetInternalId())) {
        throw(std::runtime_error(strprintf("%s: Can't add a masternode with a duplicate internalId=%d", __func__, dmn->GetInternalId())));
    }

    // All mnUniquePropertyMap's updates must be atomic.
    // Using this temporary map as a checkpoint to rollback to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    if (!AddUniqueProperty(dmn, dmn->collateralOutpoint)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate collateralOutpoint=%s", __func__,
                dmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort())));
    }
    if (dmn->pdmnState->addr != CService() && !AddUniqueProperty(dmn, dmn->pdmnState->addr)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate address=%s", __func__,
                dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToStringIPPort())));
    }
    if (!AddUniqueProperty(dmn, dmn->pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate keyIDOwner=%s", __func__,
                dmn->proTxHash.ToString(), EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)))));
    }
    if (dmn->pdmnState->pubKeyOperator.Get().IsValid() && !AddUniqueProperty(dmn, dmn->pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate pubKeyOperator=%s", __func__,
                dmn->proTxHash.ToString(), dmn->pdmnState->pubKeyOperator.Get().ToString())));
    }

    mnMap = mnMap.set(dmn->proTxHash, dmn);
    mnInternalIdMap = mnInternalIdMap.set(dmn->GetInternalId(), dmn->proTxHash);
    if (fBumpTotalCount) {
        // nTotalRegisteredCount acts more like a checkpoint, not as a limit,
        nTotalRegisteredCount = std::max(dmn->GetInternalId() + 1, (uint64_t)nTotalRegisteredCount);
    }
}

void CDeterministicMNList::UpdateMN(const CDeterministicMNCPtr& oldDmn, const CDeterministicMNStateCPtr& pdmnState)
{
    assert(oldDmn != nullptr);

    auto dmn = std::make_shared<CDeterministicMN>(*oldDmn);
    auto oldState = dmn->pdmnState;
    dmn->pdmnState = pdmnState;

    // All mnUniquePropertyMap's updates must be atomic.
    // Using this temporary map as a checkpoint to rollback to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    if (!UpdateUniqueProperty(dmn, oldState->addr, pdmnState->addr)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate address=%s", __func__,
                oldDmn->proTxHash.ToString(), pdmnState->addr.ToStringIPPort())));
    }
    if (!UpdateUniqueProperty(dmn, oldState->keyIDOwner, pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate keyIDOwner=%s", __func__,
                oldDmn->proTxHash.ToString(), EncodeDestination(WitnessV0KeyHash(pdmnState->keyIDOwner)))));
    }
    if (!UpdateUniqueProperty(dmn, oldState->pubKeyOperator, pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate pubKeyOperator=%s", __func__,
                oldDmn->proTxHash.ToString(), pdmnState->pubKeyOperator.Get().ToString())));
    }

    mnMap = mnMap.set(oldDmn->proTxHash, dmn);
}

void CDeterministicMNList::UpdateMN(const uint256& proTxHash, const CDeterministicMNStateCPtr& pdmnState)
{
    auto oldDmn = mnMap.find(proTxHash);
    if (!oldDmn) {
        throw(std::runtime_error(strprintf("%s: Can't find a masternode with proTxHash=%s", __func__, proTxHash.ToString())));
    }
    UpdateMN(*oldDmn, pdmnState);
}

void CDeterministicMNList::UpdateMN(const CDeterministicMNCPtr& oldDmn, const CDeterministicMNStateDiff& stateDiff)
{
    assert(oldDmn != nullptr);
    auto oldState = oldDmn->pdmnState;
    auto newState = std::make_shared<CDeterministicMNState>(*oldState);
    stateDiff.ApplyToState(*newState);
    UpdateMN(oldDmn, newState);
}

void CDeterministicMNList::RemoveMN(const uint256& proTxHash)
{
    auto dmn = GetMN(proTxHash);
    if (!dmn) {
        throw(std::runtime_error(strprintf("%s: Can't find a masternode with proTxHash=%s", __func__, proTxHash.ToString())));
    }

    // All mnUniquePropertyMap's updates must be atomic.
    // Using this temporary map as a checkpoint to rollback to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    if (!DeleteUniqueProperty(dmn, dmn->collateralOutpoint)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a collateralOutpoint=%s", __func__,
                proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort())));
    }
    if (dmn->pdmnState->addr != CService() && !DeleteUniqueProperty(dmn, dmn->pdmnState->addr)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a address=%s", __func__,
                proTxHash.ToString(), dmn->pdmnState->addr.ToStringIPPort())));
    }
    if (!DeleteUniqueProperty(dmn, dmn->pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a keyIDOwner=%s", __func__,
                proTxHash.ToString(), EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)))));
    }
    if (dmn->pdmnState->pubKeyOperator.Get().IsValid() && !DeleteUniqueProperty(dmn, dmn->pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a pubKeyOperator=%s", __func__,
                proTxHash.ToString(), dmn->pdmnState->pubKeyOperator.Get().ToString())));
    }

    mnMap = mnMap.erase(proTxHash);
    mnInternalIdMap = mnInternalIdMap.erase(dmn->GetInternalId());
}

CDeterministicMNManager::CDeterministicMNManager()
{
}

bool CDeterministicMNManager::ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& _state, CCoinsViewCache& view, bool fJustCheck)
{
    AssertLockHeld(cs_main);
    const auto& consensusParams = Params().GetConsensus();
    bool fDIP0003Active = pindex->nHeight >= consensusParams.DIP0003Height;
    if (!fDIP0003Active) {
        return true;
    }

    CDeterministicMNList oldList, newList;
    CDeterministicMNListDiff diff;

    int nHeight = pindex->nHeight;

    try {
        LOCK(cs);

        if (!BuildNewListFromBlock(block, pindex->pprev, _state, view, newList, true)) {
            // pass the state returned by the function above
            return false;
        }

        if (fJustCheck) {
            return true;
        }

        if (newList.GetHeight() == -1) {
            newList.SetHeight(nHeight);
        }

        newList.SetBlockHash(block.GetHash());
        CDeterministicMNList oldList;
        GetListForBlock(pindex->pprev, oldList);
        CDeterministicMNListDiff diff;
        oldList.BuildDiff(newList, diff);
        if(evoDb) {
            evoDb->Write(std::make_pair(DB_LIST_DIFF, newList.GetBlockHash()), diff);
            if ((nHeight % DISK_SNAPSHOT_PERIOD) == 0 || oldList.GetHeight() == -1) {
                evoDb->Write(std::make_pair(DB_LIST_SNAPSHOT, newList.GetBlockHash()), newList);
                mnListsCache.try_emplace(newList.GetBlockHash(), newList);
                LogPrintf("CDeterministicMNManager::%s -- Wrote snapshot. nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                    __func__, nHeight, newList.GetAllMNsCount());
            }
        }

        diff.nHeight = pindex->nHeight;
        mnListDiffsCache.try_emplace(pindex->GetBlockHash(), diff);
    } catch (const std::exception& e) {
        LogPrintf("CDeterministicMNManager::%s -- internal error: %s\n", __func__, e.what());
        return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-dmn-block");
    }
    if (diff.HasChanges()) {
        GetMainSignals().NotifyMasternodeListChanged(false, oldList, diff);
    }
    // always update interface for payment detail changes
    uiInterface.NotifyMasternodeListChanged(newList);
    
    {
        LOCK(cs);
        CleanupCache(nHeight);
    }
    return true;
}

bool CDeterministicMNManager::UndoBlock(const CBlock& block, const CBlockIndex* pindex)
{
    uint256 blockHash = block.GetHash();

    CDeterministicMNList curList;
    CDeterministicMNList prevList;
    CDeterministicMNListDiff diff;
    {
        LOCK(cs);
        if(evoDb)
            evoDb->Read(std::make_pair(DB_LIST_DIFF, blockHash), diff);

        if (diff.HasChanges()) {
            // need to call this before erasing
            GetListForBlock(pindex, curList);
            GetListForBlock(pindex->pprev, prevList);
        }

        mnListsCache.erase(blockHash);
        mnListDiffsCache.erase(blockHash);
    }

    if (diff.HasChanges()) {
        CDeterministicMNListDiff inversedDiff;
        curList.BuildDiff(prevList, inversedDiff);
        GetMainSignals().NotifyMasternodeListChanged(true, curList, inversedDiff);
    }
    // always update interface
    uiInterface.NotifyMasternodeListChanged(prevList);

    return true;
}

void CDeterministicMNManager::UpdatedBlockTip(const CBlockIndex* pindex)
{
    LOCK(cs);
    tipIndex = pindex;
}

bool CDeterministicMNManager::BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, BlockValidationState& _state, CCoinsViewCache& view, CDeterministicMNList& mnListRet, bool debugLogs, const llmq::CFinalCommitmentTxPayload *qcIn)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(cs);

    int nHeight = pindexPrev->nHeight + 1;

    CDeterministicMNList oldList;
    GetListForBlock(pindexPrev, oldList);
    CDeterministicMNList newList = oldList;
    newList.SetBlockHash(uint256()); // we can't know the final block hash, so better not return a (invalid) block hash
    newList.SetHeight(nHeight);

    auto payee = oldList.GetMNPayee();
    // at least 2 rounds of payments before registered MN's gets put in list
    const size_t &mnCountThreshold = oldList.GetValidMNsCount()*2;
    // we iterate the oldList here and update the newList
    // this is only valid as long these have not diverged at this point, which is the case as long as we don't add
    // code above this loop that modifies newList
    oldList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        if (!dmn->pdmnState->confirmedHash.IsNull()) {
            // already confirmed
            return;
        }
        // this works on the previous block, so confirmation will happen one block after mnCountThreshold
        // has been reached, but the block hash will then point to the block at mnCountThreshold
        size_t nConfirmations = pindexPrev->nHeight - dmn->pdmnState->nRegisteredHeight;
        if (nConfirmations >= mnCountThreshold) {
            auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
            newState->UpdateConfirmedHash(dmn->proTxHash, pindexPrev->GetBlockHash());
            newList.UpdateMN(dmn->proTxHash, newState);
        }
    });
    DecreasePoSePenalties(newList);
    // coinbase can be quorum commitments
    // qcIn passed in by createnewblock, but connectblock will pass in null, use gettxpayload there if version is for mn quorum
    const bool &IsQCIn = qcIn && !qcIn->IsNull();
    if(IsQCIn || (block.vtx[0] && block.vtx[0]->nVersion == SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT)) {
        llmq::CFinalCommitmentTxPayload qc;
        if(IsQCIn)
            qc = *qcIn;
        else if (!GetTxPayload(*block.vtx[0], qc)) {
            return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-payload");
        }
        for(const auto& commitment: qc.commitments) {
            if (!commitment.IsNull()) {
                const auto& params = Params().GetConsensus().llmqs.at(commitment.llmqType);
                uint32_t quorumHeight = qc.cbTx.nHeight - (qc.cbTx.nHeight % params.dkgInterval);
                auto quorumIndex = pindexPrev->GetAncestor(quorumHeight);
                if (!quorumIndex || quorumIndex->GetBlockHash() != commitment.quorumHash) {
                    // we should actually never get into this case as validation should have caught it...but lets be sure
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-quorum-hash");
                }

                HandleQuorumCommitment(commitment, quorumIndex, newList, debugLogs);
            }
        }
    }
    // for all other tx's MN register/update tx handling
    for (int i = 1; i < (int)block.vtx.size(); i++) {
        const CTransaction& tx = *block.vtx[i];

        switch(tx.nVersion) {
            case(SYSCOIN_TX_VERSION_MN_REGISTER): {
                CProRegTx proTx;
                if (!GetTxPayload(tx, proTx)) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
                }

                auto dmn = std::make_shared<CDeterministicMN>(newList.GetTotalRegisteredCount());
                dmn->proTxHash = tx.GetHash();

                // collateralOutpoint is either pointing to an external collateral or to the ProRegTx itself
                if (proTx.collateralOutpoint.hash.IsNull()) {
                    dmn->collateralOutpoint = COutPoint(tx.GetHash(), proTx.collateralOutpoint.n);
                } else {
                    dmn->collateralOutpoint = proTx.collateralOutpoint;
                }

                Coin coin;
                if (!proTx.collateralOutpoint.hash.IsNull() && (!view.GetCoin(dmn->collateralOutpoint, coin) || coin.IsSpent() || coin.out.nValue != nMNCollateralRequired)) {
                    // should actually never get to this point as CheckProRegTx should have handled this case.
                    // We do this additional check nevertheless to be 100% sure
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-collateral");
                }

                auto replacedDmn = newList.GetMNByCollateral(dmn->collateralOutpoint);
                if (replacedDmn != nullptr) {
                    // This might only happen with a ProRegTx that refers an external collateral
                    // In that case the new ProRegTx will replace the old one. This means the old one is removed
                    // and the new one is added like a completely fresh one, which is also at the bottom of the payment list
                    newList.RemoveMN(replacedDmn->proTxHash);
                    if (debugLogs) {
                        LogPrintf("CDeterministicMNManager::%s -- MN %s removed from list because collateral was used for a new ProRegTx. collateralOutpoint=%s, nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                                __func__, replacedDmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort(), nHeight, newList.GetAllMNsCount());
                    }
                }

                if (newList.HasUniqueProperty(proTx.addr)) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-addr");
                }
                if (newList.HasUniqueProperty(proTx.keyIDOwner) || newList.HasUniqueProperty(proTx.pubKeyOperator)) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
                }

                dmn->nOperatorReward = proTx.nOperatorReward;
                dmn->pdmnState = std::make_shared<CDeterministicMNState>(proTx);
                auto dmnState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
                dmnState->nRegisteredHeight = nHeight;
                // if using external collateral,  height from when collateral was created
                if(!proTx.collateralOutpoint.hash.IsNull())
                    dmnState->nCollateralHeight = coin.nHeight;
                else
                    dmnState->nCollateralHeight = nHeight;

                if (proTx.addr == CService()) {
                    // start in banned pdmnState as we need to wait for a ProUpServTx
                    dmnState->BanIfNotBanned(nHeight);
                }
                dmn->pdmnState = dmnState;

                newList.AddMN(dmn);

                if (debugLogs) {
                    LogPrintf("CDeterministicMNManager::%s -- MN %s added at height %d: %s\n",
                        __func__, tx.GetHash().ToString(), nHeight, proTx.ToString());
                }
                break;
            } 
            case(SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE): {
                CProUpServTx proTx;
                if (!GetTxPayload(tx, proTx)) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
                }

                if (newList.HasUniqueProperty(proTx.addr) && newList.GetUniquePropertyMN(proTx.addr)->proTxHash != proTx.proTxHash) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-addr");
                }

                CDeterministicMNCPtr dmn = newList.GetMN(proTx.proTxHash);
                if (!dmn) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
                }
                auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
                newState->addr = proTx.addr;
                newState->scriptOperatorPayout = proTx.scriptOperatorPayout;

                if (newState->IsBanned()) {
                    // only revive when all keys are set
                    if (newState->pubKeyOperator.Get().IsValid() && !newState->keyIDVoting.IsNull() && !newState->keyIDOwner.IsNull()) {
                        newState->Revive(nHeight);
                        if (debugLogs) {
                            LogPrintf("CDeterministicMNManager::%s -- MN %s revived at height %d\n",
                                __func__, proTx.proTxHash.ToString(), nHeight);
                        }
                    }
                }

                newList.UpdateMN(proTx.proTxHash, newState);
                if (debugLogs) {
                    LogPrintf("CDeterministicMNManager::%s -- MN %s updated at height %d: %s\n",
                        __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
                }
                break;
            } 
            case(SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR): {
                CProUpRegTx proTx;
                if (!GetTxPayload(tx, proTx)) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
                }

                CDeterministicMNCPtr dmn = newList.GetMN(proTx.proTxHash);
                if (!dmn) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
                }
                auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
                if (newState->pubKeyOperator.Get() != proTx.pubKeyOperator) {
                    // reset all operator related fields and put MN into PoSe-banned state in case the operator key changes
                    newState->ResetOperatorFields();
                    newState->BanIfNotBanned(nHeight);
                }
                newState->pubKeyOperator.Set(proTx.pubKeyOperator);
                newState->keyIDVoting = proTx.keyIDVoting;
                newState->scriptPayout = proTx.scriptPayout;

                newList.UpdateMN(proTx.proTxHash, newState);

                if (debugLogs) {
                    LogPrintf("CDeterministicMNManager::%s -- MN %s updated at height %d: %s\n",
                        __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
                }
                break;
            } 
            case(SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE): {
                CProUpRevTx proTx;
                if (!GetTxPayload(tx, proTx)) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
                }

                CDeterministicMNCPtr dmn = newList.GetMN(proTx.proTxHash);
                if (!dmn) {
                    return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
                }
                auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
                newState->ResetOperatorFields();
                newState->BanIfNotBanned(nHeight);
                newState->nRevocationReason = proTx.nReason;

                newList.UpdateMN(proTx.proTxHash, newState);

                if (debugLogs) {
                    LogPrintf("CDeterministicMNManager::%s -- MN %s revoked operator key at height %d: %s\n",
                        __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
                }
                break; 
            }
        }
    }

    // we skip the coinbase
    for (int i = 1; i < (int)block.vtx.size(); i++) {
        const CTransaction& tx = *block.vtx[i];

        // check if any existing MN collateral is spent by this transaction
        for (const auto& in : tx.vin) {
            auto dmn = newList.GetMNByCollateral(in.prevout);
            if (dmn && dmn->collateralOutpoint == in.prevout) {
                newList.RemoveMN(dmn->proTxHash);

                if (debugLogs) {
                    LogPrintf("CDeterministicMNManager::%s -- MN %s removed from list because collateral was spent. collateralOutpoint=%s, nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                              __func__, dmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort(), nHeight, newList.GetAllMNsCount());
                }
            }
        }
    }

    // The payee for the current block was determined by the previous block's list but it might have disappeared in the
    // current block. We still pay that MN one last time however.
    if (payee && newList.HasMN(payee->proTxHash)) {
        auto newState = std::make_shared<CDeterministicMNState>(*newList.GetMN(payee->proTxHash)->pdmnState);
        newState->nLastPaidHeight = nHeight;
        newList.UpdateMN(payee->proTxHash, newState);
    }

    mnListRet = std::move(newList);

    return true;
}

void CDeterministicMNManager::HandleQuorumCommitment(const llmq::CFinalCommitment& qc, const CBlockIndex* pindexQuorum, CDeterministicMNList& mnList, bool debugLogs)
{
    // The commitment has already been validated at this point so it's safe to use members of it
    std::vector<CDeterministicMNCPtr> members;
    llmq::CLLMQUtils::GetAllQuorumMembers(qc.llmqType, pindexQuorum, members);

    for (size_t i = 0; i < members.size(); i++) {
        if (!mnList.HasMN(members[i]->proTxHash)) {
            continue;
        }
        if (!qc.validMembers[i]) {
            // punish MN for failed DKG participation
            // The idea is to immediately ban a MN when it fails 2 DKG sessions with only a few blocks in-between
            // If there were enough blocks between failures, the MN has a chance to recover as he reduces his penalty by 1 every block
            // If it however fails 3 times in the timespan of a single payment cycle, it should definitely get banned
            mnList.PoSePunish(members[i]->proTxHash, mnList.CalcPenalty(66), debugLogs);
        }
    }
}

void CDeterministicMNManager::DecreasePoSePenalties(CDeterministicMNList& mnList)
{
    std::vector<uint256> toDecrease;
    toDecrease.reserve(mnList.GetValidMNsCount() / 10);
    // only iterate and decrease for valid ones (not PoSe banned yet)
    // if a MN ever reaches the maximum, it stays in PoSe banned state until revived
    mnList.ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        if (dmn->pdmnState->nPoSePenalty > 0 && !dmn->pdmnState->IsBanned()) {
            toDecrease.emplace_back(dmn->proTxHash);
        }
    });

    for (const auto& proTxHash : toDecrease) {
        mnList.PoSeDecrease(proTxHash);
    }
}

void CDeterministicMNManager::GetListForBlock(const CBlockIndex* pindex, CDeterministicMNList &snapshot)
{
    LOCK(cs);
    snapshot.clear();
    std::list<const CBlockIndex*> listDiffIndexes;
    while (true && pindex) {
        // try using cache before reading from disk
        auto itLists = mnListsCache.find(pindex->GetBlockHash());
        if (itLists != mnListsCache.end()) {
            snapshot = itLists->second;
            break;
        }

        if (evoDb && evoDb->Read(std::make_pair(DB_LIST_SNAPSHOT, pindex->GetBlockHash()), snapshot)) {
            mnListsCache.try_emplace(pindex->GetBlockHash(), snapshot);
            break;
        }
        // no snapshot found yet, check diffs
        auto itDiffs = mnListDiffsCache.find(pindex->GetBlockHash());
        if (itDiffs != mnListDiffsCache.end()) {
            listDiffIndexes.emplace_front(pindex);
            pindex = pindex->pprev;
            continue;
        }
        CDeterministicMNListDiff diff;
        if (evoDb && !evoDb->Read(std::make_pair(DB_LIST_DIFF, pindex->GetBlockHash()), diff)) {
            // no snapshot and no diff on disk means that it's the initial snapshot
            snapshot = CDeterministicMNList(pindex->GetBlockHash(), -1, 0);
            mnListsCache.try_emplace(pindex->GetBlockHash(), snapshot);
            break;
        }
        diff.nHeight = pindex->nHeight;
        mnListDiffsCache.try_emplace(pindex->GetBlockHash(), std::move(diff));
        listDiffIndexes.emplace_front(pindex);
        pindex = pindex->pprev;
    }
    for (const auto& diffIndex : listDiffIndexes) {
        const auto& diff = mnListDiffsCache.at(diffIndex->GetBlockHash());
        if (diff.HasChanges()) {
            snapshot = snapshot.ApplyDiff(diffIndex, diff);
        } else {
            snapshot.SetBlockHash(diffIndex->GetBlockHash());
            snapshot.SetHeight(diffIndex->nHeight);
        }
    }
    if (tipIndex) {
        // always keep a snapshot for the tip
        if (snapshot.GetBlockHash() == tipIndex->GetBlockHash()) {
            mnListsCache.try_emplace(snapshot.GetBlockHash(), snapshot);
        } else {
            // keep snapshots for yet alive quorums
            for (auto& p_llmq : Params().GetConsensus().llmqs) {
                if ((snapshot.GetHeight() % p_llmq.second.dkgInterval == 0) && (snapshot.GetHeight() + p_llmq.second.dkgInterval * (p_llmq.second.keepOldConnections + 1) >= tipIndex->nHeight)) {
                    mnListsCache.try_emplace(snapshot.GetBlockHash(), snapshot);
                    break;
                }
            }
        }
    }
}

void CDeterministicMNManager::GetListAtChainTip(CDeterministicMNList& result)
{
    LOCK(cs);
    if(!tipIndex) {
        result.clear();
        return;
    }
    GetListForBlock(tipIndex, result);
}

bool CDeterministicMNManager::IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n)
{
    if (tx->nVersion != SYSCOIN_TX_VERSION_MN_REGISTER) {
        return false;
    }
    CProRegTx proTx;
    if (!GetTxPayload(*tx, proTx)) {
        return false;
    }

    if (!proTx.collateralOutpoint.hash.IsNull()) {
        return false;
    }
    if (proTx.collateralOutpoint.n >= tx->vout.size() || proTx.collateralOutpoint.n != n) {
        return false;
    }
    if (tx->vout[n].nValue != nMNCollateralRequired) {
        return false;
    }
    return true;
}

bool CDeterministicMNManager::IsDIP3Enforced(int nHeight)
{
    return nHeight >= Params().GetConsensus().DIP0003EnforcementHeight;
}

void CDeterministicMNManager::CleanupCache(int nHeight)
{
    AssertLockHeld(cs);

    std::vector<uint256> toDeleteLists;
    std::vector<uint256> toDeleteDiffs;
    for (const auto& p : mnListsCache) {
        if (p.second.GetHeight() + LIST_DIFFS_CACHE_SIZE < nHeight) {
            toDeleteLists.emplace_back(p.first);
            continue;
        }
        bool fQuorumCache{false};
        for (auto& p_llmq : Params().GetConsensus().llmqs) {
            if ((p.second.GetHeight() % p_llmq.second.dkgInterval == 0) && (p.second.GetHeight() + p_llmq.second.dkgInterval * (p_llmq.second.keepOldConnections + 1) >= nHeight)) {
                fQuorumCache = true;
                break;
            }
        }
        if (fQuorumCache) {
            // at least one quorum could be using it, keep it
            continue;
        }
        // no alive quorums using it, see if it was a cache for the tip or for a now outdated quorum
        if (tipIndex && tipIndex->pprev && p.first == tipIndex->pprev->GetBlockHash()) {
            toDeleteLists.emplace_back(p.first);
        } else {
            for (auto& p_llmq : Params().GetConsensus().llmqs) {
                if (p.second.GetHeight() % p_llmq.second.dkgInterval == 0) {
                    toDeleteLists.emplace_back(p.first);
                    break;
                }
            }
        }
    }
    for (const auto& h : toDeleteLists) {
        mnListsCache.erase(h);
    }
    for (const auto& p : mnListDiffsCache) {
        if (p.second.nHeight + LIST_DIFFS_CACHE_SIZE < nHeight) {
            toDeleteDiffs.emplace_back(p.first);
        }
    }
    for (const auto& h : toDeleteDiffs) {
        mnListDiffsCache.erase(h);
    }
}
