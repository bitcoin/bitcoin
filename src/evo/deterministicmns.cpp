// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/deterministicmns.h>
#include <evo/dmn_types.h>
#include <evo/dmnstate.h>
#include <evo/evodb.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <index/txindex.h>

#include <base58.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <messagesigner.h>
#include <script/standard.h>
#include <stats/client.h>
#include <uint256.h>
#include <univalue.h>
#include <util/irange.h>
#include <util/pointer.h>

#include <optional>
#include <memory>

static const std::string DB_LIST_SNAPSHOT = "dmn_S3";
static const std::string DB_LIST_DIFF = "dmn_D3";

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

UniValue CDeterministicMN::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("type", std::string(GetMnType(nType).description));
    obj.pushKV("proTxHash", proTxHash.ToString());
    obj.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    obj.pushKV("collateralIndex", (int)collateralOutpoint.n);

    if (g_txindex) {
        CTransactionRef collateralTx;
        uint256 nBlockHash;
        g_txindex->FindTx(collateralOutpoint.hash, nBlockHash, collateralTx);
        if (collateralTx) {
            CTxDestination dest;
            if (ExtractDestination(collateralTx->vout[collateralOutpoint.n].scriptPubKey, dest)) {
                obj.pushKV("collateralAddress", EncodeDestination(dest));
            }
        }
    }

    obj.pushKV("operatorReward", (double)nOperatorReward / 100);
    obj.pushKV("state", pdmnState->ToJson(nType));
    return obj;
}

bool CDeterministicMNList::IsMNValid(const uint256& proTxHash) const
{
    auto p = mnMap.find(proTxHash);
    if (p == nullptr) {
        return false;
    }
    return !(*p)->pdmnState->IsBanned();
}

bool CDeterministicMNList::IsMNPoSeBanned(const uint256& proTxHash) const
{
    auto p = mnMap.find(proTxHash);
    if (p == nullptr) {
        return false;
    }
    return (*p)->pdmnState->IsBanned();
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
    if (dmn && dmn->pdmnState->IsBanned()) {
        return nullptr;
    }
    return dmn;
}

CDeterministicMNCPtr CDeterministicMNList::GetMNByOperatorKey(const CBLSPublicKey& pubKey) const
{
    const auto it = ranges::find_if(mnMap,
                              [&pubKey](const auto& p){return p.second->pdmnState->pubKeyOperator.Get() == pubKey;});
    if (it == mnMap.end()) {
        return nullptr;
    }
    return it->second;
}

CDeterministicMNCPtr CDeterministicMNList::GetMNByCollateral(const COutPoint& collateralOutpoint) const
{
    return GetUniquePropertyMN(collateralOutpoint);
}

CDeterministicMNCPtr CDeterministicMNList::GetValidMNByCollateral(const COutPoint& collateralOutpoint) const
{
    auto dmn = GetMNByCollateral(collateralOutpoint);
    if (dmn && dmn->pdmnState->IsBanned()) {
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
static bool CompareByLastPaid(const CDeterministicMN* _a, const CDeterministicMN* _b)
{
    return CompareByLastPaid(*_a, *_b);
}

CDeterministicMNCPtr CDeterministicMNList::GetMNPayee(gsl::not_null<const CBlockIndex*> pindexPrev) const
{
    if (mnMap.size() == 0) {
        return nullptr;
    }

    // The flag is-v19-activate is used for optimization; we don't need to go over all masternodes every pre-v19 block
    const bool isv19Active{DeploymentActiveAfter(pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_V19)};
    const bool isMNRewardReallocation{DeploymentActiveAfter(pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_MN_RR)};
    // EvoNodes are rewarded 4 blocks in a row until MNRewardReallocation (Platform release)
    // For optimization purposes we also check if v19 active to avoid loop over all masternodes
    CDeterministicMNCPtr best = nullptr;
    if (isv19Active && !isMNRewardReallocation) {
        ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
            if (dmn->pdmnState->nLastPaidHeight == nHeight) {
                // We found the last MN Payee.
                // If the last payee is an EvoNode, we need to check its consecutive payments and pay him again if needed
                if (dmn->nType == MnType::Evo && dmn->pdmnState->nConsecutivePayments < dmn_types::Evo.voting_weight) {
                    best = dmn;
                }
            }
        });

        if (best != nullptr) return best;

        // Note: If the last payee was a regular MN or if the payee is an EvoNode that was removed from the mnList then that's fine.
        // We can proceed with classic MN payee selection
    }

    ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
        if (best == nullptr || CompareByLastPaid(dmn.get(), best.get())) {
            best = dmn;
        }
    });

    return best;
}

std::vector<CDeterministicMNCPtr> CDeterministicMNList::GetProjectedMNPayees(gsl::not_null<const CBlockIndex* const> pindexPrev, int nCount) const
{
    if (nCount < 0 ) {
        return {};
    }
    const bool isMNRewardReallocation = DeploymentActiveAfter(pindexPrev, Params().GetConsensus(),
                                                              Consensus::DEPLOYMENT_MN_RR);
    const auto weighted_count = isMNRewardReallocation ? GetValidMNsCount() : GetValidWeightedMNsCount();
    nCount = std::min(nCount, int(weighted_count));

    std::vector<CDeterministicMNCPtr> result;
    result.reserve(weighted_count);

    int remaining_evo_payments{0};
    CDeterministicMNCPtr evo_to_be_skipped{nullptr};
    if (!isMNRewardReallocation) {
        ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
            if (dmn->pdmnState->nLastPaidHeight == nHeight) {
                // We found the last MN Payee.
                // If the last payee is an EvoNode, we need to check its consecutive payments and pay him again if needed
                if (dmn->nType == MnType::Evo && dmn->pdmnState->nConsecutivePayments < dmn_types::Evo.voting_weight) {
                    remaining_evo_payments = dmn_types::Evo.voting_weight - dmn->pdmnState->nConsecutivePayments;
                    for ([[maybe_unused]] auto _ : irange::range(remaining_evo_payments)) {
                        result.emplace_back(dmn);
                        evo_to_be_skipped = dmn;
                    }
                }
            }
        });
    }

    ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
        if (dmn == evo_to_be_skipped) return;
        for ([[maybe_unused]] auto _ : irange::range(isMNRewardReallocation ? 1 : GetMnType(dmn->nType).voting_weight)) {
            result.emplace_back(dmn);
        }
    });

    if (evo_to_be_skipped != nullptr) {
        // if EvoNode is in the middle of payments, add entries for already paid ones to the end of the list
        for ([[maybe_unused]] auto _ : irange::range(evo_to_be_skipped->pdmnState->nConsecutivePayments)) {
            result.emplace_back(evo_to_be_skipped);
        }
    }

    std::sort(result.begin() + remaining_evo_payments, result.end(), [&](const CDeterministicMNCPtr& a, const CDeterministicMNCPtr& b) {
        return CompareByLastPaid(a.get(), b.get());
    });

    result.resize(nCount);

    return result;
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

    if (debugLogs && dmn->pdmnState->nPoSePenalty != maxPenalty) {
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

void CDeterministicMNList::DecreaseScores()
{
    std::vector<CDeterministicMNCPtr> toDecrease;
    toDecrease.reserve(GetAllMNsCount() / 10);
    // only iterate and decrease for valid ones (not PoSe banned yet)
    // if a MN ever reaches the maximum, it stays in PoSe banned state until revived
    ForEachMNShared(true /* onlyValid */, [&toDecrease](auto& dmn) {
        // There is no reason to check if this MN is banned here since onlyValid=true will only run on non-banned MNs
        if (dmn->pdmnState->nPoSePenalty > 0) {
            toDecrease.emplace_back(dmn);
        }
    });

    for (const auto& proTxHash : toDecrease) {
        PoSeDecrease(*proTxHash);
    }
}

void CDeterministicMNList::PoSeDecrease(const CDeterministicMN& dmn)
{
    assert(dmn.pdmnState->nPoSePenalty > 0 && !dmn.pdmnState->IsBanned());

    auto newState = std::make_shared<CDeterministicMNState>(*dmn.pdmnState);
    newState->nPoSePenalty--;
    UpdateMN(dmn, newState);
}

CDeterministicMNListDiff CDeterministicMNList::BuildDiff(const CDeterministicMNList& to) const
{
    CDeterministicMNListDiff diffRet;

    for (const auto& p : to.mnMap) {
        const auto& toPtr = p.second;
        auto fromPtr = GetMN(toPtr->proTxHash);
        if (fromPtr == nullptr) {
            diffRet.addedMNs.emplace_back(toPtr);
        } else if (fromPtr != toPtr || fromPtr->pdmnState != toPtr->pdmnState) {
            CDeterministicMNStateDiff stateDiff(*fromPtr->pdmnState, *toPtr->pdmnState);
            if (stateDiff.fields) {
                diffRet.updatedMNs.emplace(toPtr->GetInternalId(), std::move(stateDiff));
            }
        }
    }
    if (mnMap.size() + diffRet.addedMNs.size() != to.mnMap.size()) {
        for (auto& fromPtr : mnMap) {
            const auto toPtr = to.GetMN(fromPtr.second->proTxHash);
            if (toPtr == nullptr) {
                diffRet.removedMns.emplace(fromPtr.second->GetInternalId());
                if (mnMap.size() + diffRet.addedMNs.size() - diffRet.removedMns.size() == to.mnMap.size()) break;
            }
        };
    }

    // added MNs need to be sorted by internalId so that these are added in correct order when the diff is applied later
    // otherwise internalIds will not match with the original list
    std::sort(diffRet.addedMNs.begin(), diffRet.addedMNs.end(), [](const CDeterministicMNCPtr& a, const CDeterministicMNCPtr& b) {
        return a->GetInternalId() < b->GetInternalId();
    });

    return diffRet;
}

void CDeterministicMNList::ApplyDiff(gsl::not_null<const CBlockIndex*> pindex, const CDeterministicMNListDiff& diff)
{
    blockHash = pindex->GetBlockHash();
    nHeight = pindex->nHeight;

    for (const auto& id : diff.removedMns) {
        auto dmn = GetMNByInternalId(id);
        if (!dmn) {
            throw std::runtime_error(strprintf("%s: can't find a removed masternode, id=%d", __func__, id));
        }
        RemoveMN(dmn->proTxHash);
    }
    for (const auto& dmn : diff.addedMNs) {
        AddMN(dmn);
    }
    for (const auto& p : diff.updatedMNs) {
        auto dmn = GetMNByInternalId(p.first);
        if (!dmn) {
            throw std::runtime_error(strprintf("%s: can't find an updated masternode, id=%d", __func__, p.first));
        }
        UpdateMN(*dmn, p.second);
    }
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
    // Using this temporary map as a checkpoint to roll back to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    if (!AddUniqueProperty(*dmn, dmn->collateralOutpoint)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate collateralOutpoint=%s", __func__,
                dmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort())));
    }
    for (const NetInfoEntry& entry : dmn->pdmnState->netInfo->GetEntries()) {
        if (const auto& service_opt{entry.GetAddrPort()}; service_opt.has_value()) {
            const CService& service{service_opt.value()};
            if (!AddUniqueProperty(*dmn, service)) {
                mnUniquePropertyMap = mnUniquePropertyMapSaved;
                throw std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate address=%s",
                                                   __func__, dmn->proTxHash.ToString(), service.ToStringAddrPort()));
            }
        } else {
            mnUniquePropertyMap = mnUniquePropertyMapSaved;
            throw std::runtime_error(
                strprintf("%s: Can't add a masternode %s with invalid address", __func__, dmn->proTxHash.ToString()));
        }
    }
    if (!AddUniqueProperty(*dmn, dmn->pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate keyIDOwner=%s", __func__,
                dmn->proTxHash.ToString(), EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner)))));
    }
    if (dmn->pdmnState->pubKeyOperator != CBLSLazyPublicKey() && !AddUniqueProperty(*dmn, dmn->pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate pubKeyOperator=%s", __func__,
                dmn->proTxHash.ToString(), dmn->pdmnState->pubKeyOperator.ToString())));
    }

    if (dmn->nType == MnType::Evo) {
        if (dmn->pdmnState->platformNodeID != uint160() && !AddUniqueProperty(*dmn, dmn->pdmnState->platformNodeID)) {
            mnUniquePropertyMap = mnUniquePropertyMapSaved;
            throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate platformNodeID=%s", __func__,
                                               dmn->proTxHash.ToString(), dmn->pdmnState->platformNodeID.ToString())));
        }
    }

    mnMap = mnMap.set(dmn->proTxHash, dmn);
    mnInternalIdMap = mnInternalIdMap.set(dmn->GetInternalId(), dmn->proTxHash);
    if (fBumpTotalCount) {
        // nTotalRegisteredCount acts more like a checkpoint, not as a limit,
        nTotalRegisteredCount = std::max(dmn->GetInternalId() + 1, (uint64_t)nTotalRegisteredCount);
    }
}

void CDeterministicMNList::UpdateMN(const CDeterministicMN& oldDmn, const std::shared_ptr<const CDeterministicMNState>& pdmnState)
{
    auto dmn = std::make_shared<CDeterministicMN>(oldDmn);
    auto oldState = dmn->pdmnState;

    // All mnUniquePropertyMap's updates must be atomic.
    // Using this temporary map as a checkpoint to roll back to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    auto updateNetInfo = [this](const CDeterministicMN& dmn, const std::shared_ptr<NetInfoInterface>& oldInfo,
                                const std::shared_ptr<NetInfoInterface>& newInfo) -> std::string {
        if (util::shared_ptr_not_equal(oldInfo, newInfo)) {
            // We track each individual entry in netInfo as opposed to netInfo itself (preventing us from
            // using UpdateUniqueProperty()), so we need to successfully purge all old entries and insert
            // new entries to successfully update.
            for (const NetInfoEntry& old_entry : oldInfo->GetEntries()) {
                if (const auto& service_opt{old_entry.GetAddrPort()}) {
                    const CService& service{service_opt.value()};
                    if (!DeleteUniqueProperty(dmn, service)) {
                        return "internal error"; // This shouldn't be possible
                    }
                } else {
                    return "invalid address";
                }
            }
            for (const NetInfoEntry& new_entry : newInfo->GetEntries()) {
                if (const auto& service_opt{new_entry.GetAddrPort()}) {
                    const CService& service{service_opt.value()};
                    if (!AddUniqueProperty(dmn, service)) {
                        return strprintf("duplicate (%s)", service.ToStringAddrPort());
                    }
                } else {
                    return "invalid address";
                }
            }
        }
        return "";
    };

    assert(oldState->netInfo && pdmnState->netInfo);
    if (auto err = updateNetInfo(*dmn, oldState->netInfo, pdmnState->netInfo); !err.empty()) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update masternode %s with addresses, reason=%s", __func__,
                                           oldDmn.proTxHash.ToString(), err)));
    }
    if (!UpdateUniqueProperty(*dmn, oldState->keyIDOwner, pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate keyIDOwner=%s", __func__,
                oldDmn.proTxHash.ToString(), EncodeDestination(PKHash(pdmnState->keyIDOwner)))));
    }
    if (!UpdateUniqueProperty(*dmn, oldState->pubKeyOperator, pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate pubKeyOperator=%s", __func__,
                oldDmn.proTxHash.ToString(), pdmnState->pubKeyOperator.ToString())));
    }
    if (dmn->nType == MnType::Evo) {
        if (!UpdateUniqueProperty(*dmn, oldState->platformNodeID, pdmnState->platformNodeID)) {
            mnUniquePropertyMap = mnUniquePropertyMapSaved;
            throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate platformNodeID=%s", __func__,
                                               oldDmn.proTxHash.ToString(), pdmnState->platformNodeID.ToString())));
        }
    }

    dmn->pdmnState = pdmnState;
    mnMap = mnMap.set(oldDmn.proTxHash, dmn);
}

void CDeterministicMNList::UpdateMN(const uint256& proTxHash, const std::shared_ptr<const CDeterministicMNState>& pdmnState)
{
    auto oldDmn = mnMap.find(proTxHash);
    if (!oldDmn) {
        throw(std::runtime_error(strprintf("%s: Can't find a masternode with proTxHash=%s", __func__, proTxHash.ToString())));
    }
    UpdateMN(**oldDmn, pdmnState);
}

void CDeterministicMNList::UpdateMN(const CDeterministicMN& oldDmn, const CDeterministicMNStateDiff& stateDiff)
{
    auto oldState = oldDmn.pdmnState;
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
    // Using this temporary map as a checkpoint to roll back to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    if (!DeleteUniqueProperty(*dmn, dmn->collateralOutpoint)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a collateralOutpoint=%s", __func__,
                proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort())));
    }
    for (const NetInfoEntry& entry : dmn->pdmnState->netInfo->GetEntries()) {
        if (const auto& service_opt{entry.GetAddrPort()}; service_opt.has_value()) {
            const CService& service{service_opt.value()};
            if (!DeleteUniqueProperty(*dmn, service)) {
                mnUniquePropertyMap = mnUniquePropertyMapSaved;
                throw std::runtime_error(strprintf("%s: Can't delete a masternode %s with an address=%s", __func__,
                                                   proTxHash.ToString(), service.ToStringAddrPort()));
            }
        } else {
            mnUniquePropertyMap = mnUniquePropertyMapSaved;
            throw std::runtime_error(strprintf("%s: Can't delete a masternode %s with invalid address", __func__,
                                               dmn->proTxHash.ToString()));
        }
    }
    if (!DeleteUniqueProperty(*dmn, dmn->pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a keyIDOwner=%s", __func__,
                proTxHash.ToString(), EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner)))));
    }
    if (dmn->pdmnState->pubKeyOperator != CBLSLazyPublicKey() &&
        !DeleteUniqueProperty(*dmn, dmn->pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a pubKeyOperator=%s", __func__,
                proTxHash.ToString(), dmn->pdmnState->pubKeyOperator.ToString())));
    }

    if (dmn->nType == MnType::Evo) {
        if (dmn->pdmnState->platformNodeID != uint160() && !DeleteUniqueProperty(*dmn, dmn->pdmnState->platformNodeID)) {
            mnUniquePropertyMap = mnUniquePropertyMapSaved;
            throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a duplicate platformNodeID=%s", __func__,
                                               dmn->proTxHash.ToString(), dmn->pdmnState->platformNodeID.ToString())));
        }
    }

    mnMap = mnMap.erase(proTxHash);
    mnInternalIdMap = mnInternalIdMap.erase(dmn->GetInternalId());
}

bool CDeterministicMNManager::ProcessBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex,
                                           BlockValidationState& state, const CDeterministicMNList& newList,
                                           std::optional<MNListUpdates>& updatesRet)
{
    AssertLockHeld(::cs_main);

    const auto& consensusParams = Params().GetConsensus();
    if (!DeploymentActiveAt(*pindex, consensusParams, Consensus::DEPLOYMENT_DIP0003)) {
        return true;
    }

    CDeterministicMNList oldList;
    CDeterministicMNListDiff diff;

    int nHeight = pindex->nHeight;

    try {
        LOCK(cs);

        oldList = GetListForBlockInternal(pindex->pprev);
        diff = oldList.BuildDiff(newList);

        m_evoDb.Write(std::make_pair(DB_LIST_DIFF, newList.GetBlockHash()), diff);
        if ((nHeight % DISK_SNAPSHOT_PERIOD) == 0 || pindex->pprev == m_initial_snapshot_index) {
            m_evoDb.Write(std::make_pair(DB_LIST_SNAPSHOT, newList.GetBlockHash()), newList);
            mnListsCache.emplace(newList.GetBlockHash(), newList);
            LogPrintf("CDeterministicMNManager::%s -- Wrote snapshot. nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                __func__, nHeight, newList.GetAllMNsCount());
        }

        diff.nHeight = pindex->nHeight;
        mnListDiffsCache.emplace(pindex->GetBlockHash(), diff);
    } catch (const std::exception& e) {
        LogPrintf("CDeterministicMNManager::%s -- internal error: %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-dmn-block");
    }

    if (diff.HasChanges()) {
        updatesRet = {newList, oldList, diff};
    }

    if (::g_stats_client->active()) {
        ::g_stats_client->gauge("masternodes.count", newList.GetAllMNsCount());
        ::g_stats_client->gauge("masternodes.weighted_count", newList.GetValidWeightedMNsCount());
        ::g_stats_client->gauge("masternodes.enabled", newList.GetValidMNsCount());
        ::g_stats_client->gauge("masternodes.weighted_enabled", newList.GetValidWeightedMNsCount());
        ::g_stats_client->gauge("masternodes.evo.count", newList.GetAllEvoCount());
        ::g_stats_client->gauge("masternodes.evo.enabled", newList.GetValidEvoCount());
        ::g_stats_client->gauge("masternodes.mn.count", newList.GetAllMNsCount() - newList.GetAllEvoCount());
        ::g_stats_client->gauge("masternodes.mn.enabled", newList.GetValidMNsCount() - newList.GetValidEvoCount());
    }

    if (nHeight == consensusParams.DIP0003EnforcementHeight) {
        if (!consensusParams.DIP0003EnforcementHash.IsNull() && consensusParams.DIP0003EnforcementHash != pindex->GetBlockHash()) {
            LogPrintf("CDeterministicMNManager::%s -- DIP3 enforcement block has wrong hash: hash=%s, expected=%s, nHeight=%d\n", __func__,
                    pindex->GetBlockHash().ToString(), consensusParams.DIP0003EnforcementHash.ToString(), nHeight);
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-dip3-enf-block");
        }
        LogPrintf("CDeterministicMNManager::%s -- DIP3 is enforced now. nHeight=%d\n", __func__, nHeight);
    }
    if (nHeight > to_cleanup) to_cleanup = nHeight;

    return true;
}

bool CDeterministicMNManager::UndoBlock(gsl::not_null<const CBlockIndex*> pindex, std::optional<MNListUpdates>& updatesRet)
{
    int nHeight = pindex->nHeight;
    uint256 blockHash = pindex->GetBlockHash();

    CDeterministicMNList prevList;
    CDeterministicMNListDiff diff;
    {
        LOCK(cs);
        m_evoDb.Read(std::make_pair(DB_LIST_DIFF, blockHash), diff);

        if (diff.HasChanges()) {
            // need to call this before erasing
            prevList = GetListForBlockInternal(pindex->pprev);
        }

        mnListsCache.erase(blockHash);
        mnListDiffsCache.erase(blockHash);
    }
    if (diff.HasChanges()) {
        CDeterministicMNList curList{prevList};
        curList.ApplyDiff(pindex, diff);

        auto inversedDiff{curList.BuildDiff(prevList)};
        updatesRet = {curList, prevList, inversedDiff};
    }

    const auto& consensusParams = Params().GetConsensus();
    if (nHeight == consensusParams.DIP0003EnforcementHeight) {
        LogPrintf("CDeterministicMNManager::%s -- DIP3 is not enforced anymore. nHeight=%d\n", __func__, nHeight);
    }

    return true;
}

void CDeterministicMNManager::UpdatedBlockTip(gsl::not_null<const CBlockIndex*> pindex)
{
    LOCK(cs);

    tipIndex = pindex;
}

CDeterministicMNList CDeterministicMNManager::GetListForBlockInternal(gsl::not_null<const CBlockIndex*> pindex)
{
    CDeterministicMNList snapshot;

    if (!DeploymentActiveAt(*pindex, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0003)) {
        return snapshot;
    }

    AssertLockHeld(cs);

    std::list<const CBlockIndex*> listDiffIndexes;

    while (true) {
        // try using cache before reading from disk
        auto itLists = mnListsCache.find(pindex->GetBlockHash());
        if (itLists != mnListsCache.end()) {
            snapshot = itLists->second;
            break;
        }

        if (m_evoDb.Read(std::make_pair(DB_LIST_SNAPSHOT, pindex->GetBlockHash()), snapshot)) {
            mnListsCache.emplace(pindex->GetBlockHash(), snapshot);
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
        if (!m_evoDb.Read(std::make_pair(DB_LIST_DIFF, pindex->GetBlockHash()), diff)) {
            // no snapshot and no diff on disk means that it's the initial snapshot
            m_initial_snapshot_index = pindex;
            snapshot = CDeterministicMNList(pindex->GetBlockHash(), pindex->nHeight, 0);
            mnListsCache.emplace(pindex->GetBlockHash(), snapshot);
            LogPrintf("CDeterministicMNManager::%s -- initial snapshot. blockHash=%s nHeight=%d\n",
                    __func__, snapshot.GetBlockHash().ToString(), snapshot.GetHeight());
            break;
        }

        diff.nHeight = pindex->nHeight;
        mnListDiffsCache.emplace(pindex->GetBlockHash(), std::move(diff));
        listDiffIndexes.emplace_front(pindex);
        pindex = pindex->pprev;
    }

    for (const auto& diffIndex : listDiffIndexes) {
        const auto& diff = mnListDiffsCache.at(diffIndex->GetBlockHash());
        snapshot.ApplyDiff(diffIndex, diff);
    }

    if (tipIndex) {
        // always keep a snapshot for the tip
        if (snapshot.GetBlockHash() == tipIndex->GetBlockHash()) {
            mnListsCache.emplace(snapshot.GetBlockHash(), snapshot);
        } else {
            // keep snapshots for yet alive quorums
            if (ranges::any_of(Params().GetConsensus().llmqs,
                               [&snapshot, this](const auto& params) EXCLUSIVE_LOCKS_REQUIRED(cs) {
                                   AssertLockHeld(cs);
                                   return (snapshot.GetHeight() % params.dkgInterval == 0) &&
                                          (snapshot.GetHeight() + params.dkgInterval * (params.keepOldConnections + 1) >=
                                           tipIndex->nHeight);
                               })) {
                mnListsCache.emplace(snapshot.GetBlockHash(), snapshot);
            }
        }
    }

    assert(snapshot.GetHeight() != -1);
    return snapshot;
}

CDeterministicMNList CDeterministicMNManager::GetListAtChainTip()
{
    LOCK(cs);
    if (!tipIndex) {
        return {};
    }
    return GetListForBlockInternal(tipIndex);
}

bool CDeterministicMNManager::IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n)
{
    if (!tx->IsSpecialTxVersion() || tx->nType != TRANSACTION_PROVIDER_REGISTER) {
        return false;
    }
    const auto opt_proTx = GetTxPayload<CProRegTx>(*tx);
    if (!opt_proTx) {
        return false;
    }
    auto& proTx = *opt_proTx;

    if (!proTx.collateralOutpoint.hash.IsNull()) {
        return false;
    }
    if (proTx.collateralOutpoint.n >= tx->vout.size() || proTx.collateralOutpoint.n != n) {
        return false;
    }


    if (const CAmount expectedCollateral = GetMnType(proTx.nType).collat_amount; tx->vout[n].nValue != expectedCollateral) {
        return false;
    }
    return true;
}

void CDeterministicMNManager::CleanupCache(int nHeight)
{
    AssertLockHeld(cs);

    std::vector<uint256> toDeleteLists;
    std::vector<uint256> toDeleteDiffs;
    for (const auto& p : mnListsCache) {
        if (p.second.GetHeight() + LIST_DIFFS_CACHE_SIZE < nHeight) {
            // too old, drop it
            toDeleteLists.emplace_back(p.first);
            continue;
        }
        if (tipIndex != nullptr && p.first == tipIndex->GetBlockHash()) {
            // it's a snapshot for the tip, keep it
            continue;
        }
        bool fQuorumCache = ranges::any_of(Params().GetConsensus().llmqs, [&nHeight, &p](const auto& params){
            return (p.second.GetHeight() % params.dkgInterval == 0) &&
                   (p.second.GetHeight() + params.dkgInterval * (params.keepOldConnections + 1) >= nHeight);
        });
        if (fQuorumCache) {
            // at least one quorum could be using it, keep it
            continue;
        }
        // none of the above, drop it
        toDeleteLists.emplace_back(p.first);
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

template <typename ProTx>
static bool CheckService(const ProTx& proTx, TxValidationState& state)
{
    switch (proTx.netInfo->Validate()) {
    case NetInfoStatus::BadAddress:
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-addr");
    case NetInfoStatus::BadPort:
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-port");
    case NetInfoStatus::BadType:
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-addr-type");
    case NetInfoStatus::NotRoutable:
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-addr-unroutable");
    case NetInfoStatus::Malformed:
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-bad");
    case NetInfoStatus::Success:
        return true;
    // Shouldn't be possible during self-checks
    case NetInfoStatus::BadInput:
    case NetInfoStatus::MaxLimit:
        assert(false);
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

template <typename ProTx>
static bool CheckPlatformFields(const ProTx& proTx, TxValidationState& state)
{
    if (proTx.platformNodeID.IsNull()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-platform-nodeid");
    }

    // TODO: use real args here
    static int mainnetPlatformP2PPort = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN)->GetDefaultPlatformP2PPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (proTx.platformP2PPort != mainnetPlatformP2PPort) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-platform-p2p-port");
        }
    }

    // TODO: use real args here
    static int mainnetPlatformHTTPPort = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN)->GetDefaultPlatformHTTPPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (proTx.platformHTTPPort != mainnetPlatformHTTPPort) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-platform-http-port");
        }
    }

    // TODO: use real args here
    static int mainnetDefaultP2PPort = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN)->GetDefaultPort();
    if (proTx.platformP2PPort == mainnetDefaultP2PPort) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-platform-p2p-port");
    }
    if (proTx.platformHTTPPort == mainnetDefaultP2PPort) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-platform-http-port");
    }

    if (proTx.platformP2PPort == proTx.platformHTTPPort || proTx.platformP2PPort == proTx.netInfo->GetPrimary().GetPort() ||
        proTx.platformHTTPPort == proTx.netInfo->GetPrimary().GetPort()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-platform-dup-ports");
    }

    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const PKHash& pkhash, TxValidationState& state)
{
    if (std::string strError; !CHashSigner::VerifyHash(::SerializeHash(proTx), ToKeyID(pkhash), proTx.vchSig, strError)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
    }
    return true;
}

template <typename ProTx>
static bool CheckStringSig(const ProTx& proTx, const PKHash& pkhash, TxValidationState& state)
{
    if (std::string strError; !CMessageSigner::VerifyMessage(ToKeyID(pkhash), proTx.vchSig, proTx.MakeSignString(), strError)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
    }
    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CBLSPublicKey& pubKey, TxValidationState& state)
{
    if (!proTx.sig.VerifyInsecure(pubKey, ::SerializeHash(proTx))) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
    }
    return true;
}

template<typename ProTx>
static std::optional<ProTx> GetValidatedPayload(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state)
{
    if (tx.nType != ProTx::SPECIALTX_TYPE) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-type");
        return std::nullopt;
    }

    auto opt_ptx = GetTxPayload<ProTx>(tx);
    if (!opt_ptx) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-payload");
        return std::nullopt;
    }
    if (!opt_ptx->IsTriviallyValid(pindexPrev, state)) {
        // pass the state returned by the function above
        return std::nullopt;
    }
    return opt_ptx;
}

/**
 * Validates potential changes to masternode state version by ProTx transaction version
 * @param[in]  pindexPrev    Previous block index to validate DEPLOYMENT_V23 activation
 * @param[in]  tx_type       Special transaction type
 * @param[in]  state_version Current masternode state version
 * @param[in]  tx_version    Proposed transaction version
 * @param[out] state         This may be set to an Error state if any error occurred processing them
 * @returns                  true if version change is valid or DEPLOYMENT_V23 is not active
 */
bool IsVersionChangeValid(gsl::not_null<const CBlockIndex*> pindexPrev, const uint16_t tx_type,
                          const uint16_t state_version, const uint16_t tx_version, TxValidationState& state)
{
    if (!DeploymentActiveAfter(pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_V23)) {
        // New restrictions only apply after v23 deployment
        return true;
    }

    if (state_version >= ProTxVersion::BasicBLS && tx_version == ProTxVersion::LegacyBLS) {
        // Don't allow legacy scheme versioned transactions after upgrading to basic scheme
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version-downgrade");
    }

    if (state_version == ProTxVersion::LegacyBLS && tx_version > ProTxVersion::BasicBLS) {
        // Nodes using the legacy scheme must first upgrade to the basic scheme before upgrading further
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version-upgrade");
    }

    if (tx_type != TRANSACTION_PROVIDER_UPDATE_SERVICE && tx_version == ProTxVersion::ExtAddr) {
        // Only new entries (ProRegTx) and service updates (ProUpServTx) can use ExtAddr versioning
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version-tx-type");
    }

    return true;
}

bool CheckProRegTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, const CCoinsViewCache& view, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProRegTx>(tx, pindexPrev, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    const bool is_v23_active{DeploymentActiveAfter(pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_V23)};

    // No longer allow legacy scheme masternode registration
    if (is_v23_active && opt_ptx->nVersion < ProTxVersion::BasicBLS) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version-disallowed");
    }

    // It's allowed to set addr to 0, which will put the MN into PoSe-banned state and require a ProUpServTx to be issues later
    // If any of both is set, it must be valid however
    if (!opt_ptx->netInfo->IsEmpty() && !CheckService(*opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (opt_ptx->nType == MnType::Evo) {
        if (!CheckPlatformFields(*opt_ptx, state)) {
            return false;
        }
    }

    CTxDestination collateralTxDest;
    const PKHash *keyForPayloadSig = nullptr;
    COutPoint collateralOutpoint;

    CAmount expectedCollateral = GetMnType(opt_ptx->nType).collat_amount;

    if (!opt_ptx->collateralOutpoint.hash.IsNull()) {
        Coin coin;
        if (!view.GetCoin(opt_ptx->collateralOutpoint, coin) || coin.IsSpent() || coin.out.nValue != expectedCollateral) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral");
        }

        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-dest");
        }

        // Extract key from collateral. This only works for P2PK and P2PKH collaterals and will fail for P2SH.
        // Issuer of this ProRegTx must prove ownership with this key by signing the ProRegTx
        keyForPayloadSig = std::get_if<PKHash>(&collateralTxDest);
        if (!keyForPayloadSig) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-pkh");
        }

        collateralOutpoint = opt_ptx->collateralOutpoint;
    } else {
        if (opt_ptx->collateralOutpoint.n >= tx.vout.size()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-index");
        }
        if (tx.vout[opt_ptx->collateralOutpoint.n].nValue != expectedCollateral) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral");
        }

        if (!ExtractDestination(tx.vout[opt_ptx->collateralOutpoint.n].scriptPubKey, collateralTxDest)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-dest");
        }

        collateralOutpoint = COutPoint(tx.GetHash(), opt_ptx->collateralOutpoint.n);
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    // this check applies to internal and external collateral, but internal collaterals are not necessarily a P2PKH
    if (collateralTxDest == CTxDestination(PKHash(opt_ptx->keyIDOwner)) || collateralTxDest == CTxDestination(PKHash(opt_ptx->keyIDVoting))) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-reuse");
    }

    if (pindexPrev) {
        auto mnList = dmnman.GetListForBlock(pindexPrev);

        // only allow reusing of addresses when it's for the same collateral (which replaces the old MN)
        for (const NetInfoEntry& entry : opt_ptx->netInfo->GetEntries()) {
            if (const auto& service_opt{entry.GetAddrPort()}; service_opt.has_value()) {
                const CService& service{service_opt.value()};
                if (mnList.HasUniqueProperty(service) &&
                    mnList.GetUniquePropertyMN(service)->collateralOutpoint != collateralOutpoint) {
                    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-netinfo-entry");
                }
            } else {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-entry");
            }
        }

        // never allow duplicate keys, even if this ProTx would replace an existing MN
        if (mnList.HasUniqueProperty(opt_ptx->keyIDOwner) || mnList.HasUniqueProperty(opt_ptx->pubKeyOperator)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
        }

        // never allow duplicate platformNodeIds for EvoNodes
        if (opt_ptx->nType == MnType::Evo) {
            if (mnList.HasUniqueProperty(opt_ptx->platformNodeID)) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-platformnodeid");
            }
        }

        if (!DeploymentDIP0003Enforced(pindexPrev->nHeight, Params().GetConsensus())) {
            if (opt_ptx->keyIDOwner != opt_ptx->keyIDVoting) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-key-not-same");
            }
        }
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (keyForPayloadSig) {
        // collateral is not part of this ProRegTx, so we must verify ownership of the collateral
        if (check_sigs && !CheckStringSig(*opt_ptx, *keyForPayloadSig, state)) {
            // pass the state returned by the function above
            return false;
        }
    } else {
        // collateral is part of this ProRegTx, so we know the collateral is owned by the issuer
        if (!opt_ptx->vchSig.empty()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
        }
    }

    return true;
}

bool CheckProUpServTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpServTx>(tx, pindexPrev, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    if (!CheckService(*opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (opt_ptx->nType == MnType::Evo) {
        if (!CheckPlatformFields(*opt_ptx, state)) {
            return false;
        }
    }

    auto mnList = dmnman.GetListForBlock(pindexPrev);
    auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-hash");
    }

    if (!IsVersionChangeValid(pindexPrev, tx.nType, dmn->pdmnState->nVersion, opt_ptx->nVersion, state)) {
        // pass the state returned by the function above
        return false;
    }

    // don't allow updating to addresses already used by other MNs
    for (const NetInfoEntry& entry : opt_ptx->netInfo->GetEntries()) {
        if (const auto& service_opt{entry.GetAddrPort()}; service_opt.has_value()) {
            const CService& service{service_opt.value()};
            if (mnList.HasUniqueProperty(service) && mnList.GetUniquePropertyMN(service)->proTxHash != opt_ptx->proTxHash) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-netinfo-entry");
            }
        } else {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-entry");
        }
    }

    // don't allow updating to platformNodeIds already used by other EvoNodes
    if (opt_ptx->nType == MnType::Evo) {
        if (mnList.HasUniqueProperty(opt_ptx->platformNodeID)  && mnList.GetUniquePropertyMN(opt_ptx->platformNodeID)->proTxHash != opt_ptx->proTxHash) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-platformnodeid");
        }
    }

    if (opt_ptx->scriptOperatorPayout != CScript()) {
        if (dmn->nOperatorReward == 0) {
            // don't allow setting operator reward payee in case no operatorReward was set
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-operator-payee");
        }
        if (!opt_ptx->scriptOperatorPayout.IsPayToPublicKeyHash() && !opt_ptx->scriptOperatorPayout.IsPayToScriptHash()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-operator-payee");
        }
    }

    // we can only check the signature if pindexPrev != nullptr and the MN is known
    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs && !CheckHashSig(*opt_ptx, dmn->pdmnState->pubKeyOperator.Get(), state)) {
        // pass the state returned by the function above
        return false;
    }

    return true;
}

bool CheckProUpRegTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, const CCoinsViewCache& view, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpRegTx>(tx, pindexPrev, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(opt_ptx->scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payee-dest");
    }

    auto mnList = dmnman.GetListForBlock(pindexPrev);
    auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-hash");
    }

    if (!IsVersionChangeValid(pindexPrev, tx.nType, dmn->pdmnState->nVersion, opt_ptx->nVersion, state)) {
        // pass the state returned by the function above
        return false;
    }

    // don't allow reuse of payee key for other keys (don't allow people to put the payee key onto an online server)
    if (payoutDest == CTxDestination(PKHash(dmn->pdmnState->keyIDOwner)) || payoutDest == CTxDestination(PKHash(opt_ptx->keyIDVoting))) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payee-reuse");
    }

    Coin coin;
    if (!view.GetCoin(dmn->collateralOutpoint, coin) || coin.IsSpent()) {
        // this should never happen (there would be no dmn otherwise)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-collateral");
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    CTxDestination collateralTxDest;
    if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-collateral-dest");
    }
    if (collateralTxDest == CTxDestination(PKHash(dmn->pdmnState->keyIDOwner)) || collateralTxDest == CTxDestination(PKHash(opt_ptx->keyIDVoting))) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-reuse");
    }

    if (mnList.HasUniqueProperty(opt_ptx->pubKeyOperator)) {
        auto otherDmn = mnList.GetUniquePropertyMN(opt_ptx->pubKeyOperator);
        if (opt_ptx->proTxHash != otherDmn->proTxHash) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
        }
    }

    if (!DeploymentDIP0003Enforced(pindexPrev->nHeight, Params().GetConsensus())) {
        if (dmn->pdmnState->keyIDOwner != opt_ptx->keyIDVoting) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-key-not-same");
        }
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs && !CheckHashSig(*opt_ptx, PKHash(dmn->pdmnState->keyIDOwner), state)) {
        // pass the state returned by the function above
        return false;
    }

    return true;
}

bool CheckProUpRevTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpRevTx>(tx, pindexPrev, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    auto mnList = dmnman.GetListForBlock(pindexPrev);
    auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-hash");
    }

    if (!IsVersionChangeValid(pindexPrev, tx.nType, dmn->pdmnState->nVersion, opt_ptx->nVersion, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs && !CheckHashSig(*opt_ptx, dmn->pdmnState->pubKeyOperator.Get(), state)) {
        // pass the state returned by the function above
        return false;
    }

    return true;
}
//end

void CDeterministicMNManager::DoMaintenance() {
    LOCK(cs_cleanup);
    int loc_to_cleanup = to_cleanup.load();
    if (loc_to_cleanup <= did_cleanup) return;
    LOCK(cs);
    CleanupCache(loc_to_cleanup);
    did_cleanup = loc_to_cleanup;
}
