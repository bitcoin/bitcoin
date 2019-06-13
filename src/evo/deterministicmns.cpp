// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "deterministicmns.h"
#include "specialtx.h"

#include "base58.h"
#include "chainparams.h"
#include "core_io.h"
#include "script/standard.h"
#include "ui_interface.h"
#include "validation.h"
#include "validationinterface.h"

#include "llmq/quorums_commitment.h"
#include "llmq/quorums_utils.h"

#include <univalue.h>

static const std::string DB_LIST_SNAPSHOT = "dmn_S";
static const std::string DB_LIST_DIFF = "dmn_D";

CDeterministicMNManager* deterministicMNManager;

std::string CDeterministicMNState::ToString() const
{
    CTxDestination dest;
    std::string payoutAddress = "unknown";
    std::string operatorPayoutAddress = "none";
    if (ExtractDestination(scriptPayout, dest)) {
        payoutAddress = CBitcoinAddress(dest).ToString();
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        operatorPayoutAddress = CBitcoinAddress(dest).ToString();
    }

    return strprintf("CDeterministicMNState(nRegisteredHeight=%d, nLastPaidHeight=%d, nPoSePenalty=%d, nPoSeRevivedHeight=%d, nPoSeBanHeight=%d, nRevocationReason=%d, "
        "ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, addr=%s, payoutAddress=%s, operatorPayoutAddress=%s)",
        nRegisteredHeight, nLastPaidHeight, nPoSePenalty, nPoSeRevivedHeight, nPoSeBanHeight, nRevocationReason,
        CBitcoinAddress(keyIDOwner).ToString(), pubKeyOperator.Get().ToString(), CBitcoinAddress(keyIDVoting).ToString(), addr.ToStringIPPort(false), payoutAddress, operatorPayoutAddress);
}

void CDeterministicMNState::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("service", addr.ToStringIPPort(false)));
    obj.push_back(Pair("registeredHeight", nRegisteredHeight));
    obj.push_back(Pair("lastPaidHeight", nLastPaidHeight));
    obj.push_back(Pair("PoSePenalty", nPoSePenalty));
    obj.push_back(Pair("PoSeRevivedHeight", nPoSeRevivedHeight));
    obj.push_back(Pair("PoSeBanHeight", nPoSeBanHeight));
    obj.push_back(Pair("revocationReason", nRevocationReason));
    obj.push_back(Pair("ownerAddress", CBitcoinAddress(keyIDOwner).ToString()));
    obj.push_back(Pair("votingAddress", CBitcoinAddress(keyIDVoting).ToString()));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        CBitcoinAddress payoutAddress(dest);
        obj.push_back(Pair("payoutAddress", payoutAddress.ToString()));
    }
    obj.push_back(Pair("pubKeyOperator", pubKeyOperator.Get().ToString()));
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        CBitcoinAddress operatorPayoutAddress(dest);
        obj.push_back(Pair("operatorPayoutAddress", operatorPayoutAddress.ToString()));
    }
}

std::string CDeterministicMN::ToString() const
{
    return strprintf("CDeterministicMN(proTxHash=%s, collateralOutpoint=%s, nOperatorReward=%f, state=%s", proTxHash.ToString(), collateralOutpoint.ToStringShort(), (double)nOperatorReward / 100, pdmnState->ToString());
}

void CDeterministicMN::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();

    UniValue stateObj;
    pdmnState->ToJson(stateObj);

    obj.push_back(Pair("proTxHash", proTxHash.ToString()));
    obj.push_back(Pair("collateralHash", collateralOutpoint.hash.ToString()));
    obj.push_back(Pair("collateralIndex", (int)collateralOutpoint.n));

    Coin coin;
    if (GetUTXOCoin(collateralOutpoint, coin)) {
        CTxDestination dest;
        if (ExtractDestination(coin.out.scriptPubKey, dest)) {
            obj.push_back(Pair("collateralAddress", CBitcoinAddress(dest).ToString()));
        }
    }

    obj.push_back(Pair("operatorReward", (double)nOperatorReward / 100));
    obj.push_back(Pair("state", stateObj));
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

bool CDeterministicMNList::IsMNValid(const CDeterministicMNCPtr& dmn) const
{
    return !IsMNPoSeBanned(dmn);
}

bool CDeterministicMNList::IsMNPoSeBanned(const CDeterministicMNCPtr& dmn) const
{
    assert(dmn);
    const CDeterministicMNState& state = *dmn->pdmnState;
    return state.nPoSeBanHeight != -1;
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

CDeterministicMNCPtr CDeterministicMNList::GetValidMNByService(const CService& service) const
{
    auto dmn = GetUniquePropertyMN(service);
    if (dmn && !IsMNValid(dmn)) {
        return nullptr;
    }
    return dmn;
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

std::vector<CDeterministicMNCPtr> CDeterministicMNList::GetProjectedMNPayees(int nCount) const
{
    if (nCount > GetValidMNsCount()) {
        nCount = GetValidMNsCount();
    }

    std::vector<CDeterministicMNCPtr> result;
    result.reserve(nCount);

    ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        result.emplace_back(dmn);
    });
    std::sort(result.begin(), result.end(), [&](const CDeterministicMNCPtr& a, const CDeterministicMNCPtr& b) {
        return CompareByLastPaid(a, b);
    });

    result.resize(nCount);

    return result;
}

std::vector<CDeterministicMNCPtr> CDeterministicMNList::CalculateQuorum(size_t maxSize, const uint256& modifier) const
{
    auto scores = CalculateScores(modifier);

    // sort is descending order
    std::sort(scores.rbegin(), scores.rend(), [](const std::pair<arith_uint256, CDeterministicMNCPtr>& a, std::pair<arith_uint256, CDeterministicMNCPtr>& b) {
        if (a.first == b.first) {
            // this should actually never happen, but we should stay compatible with how the non deterministic MNs did the sorting
            return a.second->collateralOutpoint < b.second->collateralOutpoint;
        }
        return a.first < b.first;
    });

    // take top maxSize entries and return it
    std::vector<CDeterministicMNCPtr> result;
    result.resize(std::min(maxSize, scores.size()));
    for (size_t i = 0; i < result.size(); i++) {
        result[i] = std::move(scores[i].second);
    }
    return result;
}

std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> CDeterministicMNList::CalculateScores(const uint256& modifier) const
{
    std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> scores;
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

    return scores;
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
    assert(dmn);

    int maxPenalty = CalcMaxPoSePenalty();

    auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
    newState->nPoSePenalty += penalty;
    newState->nPoSePenalty = std::min(maxPenalty, newState->nPoSePenalty);

    if (debugLogs) {
        LogPrintf("CDeterministicMNList::%s -- punished MN %s, penalty %d->%d (max=%d)\n",
                  __func__, proTxHash.ToString(), dmn->pdmnState->nPoSePenalty, newState->nPoSePenalty, maxPenalty);
    }

    if (newState->nPoSePenalty >= maxPenalty && newState->nPoSeBanHeight == -1) {
        newState->nPoSeBanHeight = nHeight;
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
    assert(dmn);
    assert(dmn->pdmnState->nPoSePenalty > 0 && dmn->pdmnState->nPoSeBanHeight == -1);

    auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
    newState->nPoSePenalty--;
    UpdateMN(proTxHash, newState);
}

CDeterministicMNListDiff CDeterministicMNList::BuildDiff(const CDeterministicMNList& to) const
{
    CDeterministicMNListDiff diffRet;
    diffRet.prevBlockHash = blockHash;
    diffRet.blockHash = to.blockHash;
    diffRet.nHeight = to.nHeight;

    to.ForEachMN(false, [&](const CDeterministicMNCPtr& toPtr) {
        auto fromPtr = GetMN(toPtr->proTxHash);
        if (fromPtr == nullptr) {
            diffRet.addedMNs.emplace(toPtr->proTxHash, toPtr);
        } else if (*toPtr->pdmnState != *fromPtr->pdmnState) {
            diffRet.updatedMNs.emplace(toPtr->proTxHash, toPtr->pdmnState);
        }
    });
    ForEachMN(false, [&](const CDeterministicMNCPtr& fromPtr) {
        auto toPtr = to.GetMN(fromPtr->proTxHash);
        if (toPtr == nullptr) {
            diffRet.removedMns.insert(fromPtr->proTxHash);
        }
    });

    return diffRet;
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

CDeterministicMNList CDeterministicMNList::ApplyDiff(const CDeterministicMNListDiff& diff) const
{
    assert(diff.prevBlockHash == blockHash && diff.nHeight == nHeight + 1);

    CDeterministicMNList result = *this;
    result.blockHash = diff.blockHash;
    result.nHeight = diff.nHeight;

    for (const auto& hash : diff.removedMns) {
        result.RemoveMN(hash);
    }
    for (const auto& p : diff.addedMNs) {
        result.AddMN(p.second);
    }
    for (const auto& p : diff.updatedMNs) {
        result.UpdateMN(p.first, p.second);
    }

    return result;
}

void CDeterministicMNList::AddMN(const CDeterministicMNCPtr& dmn)
{
    assert(!mnMap.find(dmn->proTxHash));
    mnMap = mnMap.set(dmn->proTxHash, dmn);
    AddUniqueProperty(dmn, dmn->collateralOutpoint);
    if (dmn->pdmnState->addr != CService()) {
        AddUniqueProperty(dmn, dmn->pdmnState->addr);
    }
    AddUniqueProperty(dmn, dmn->pdmnState->keyIDOwner);
    if (dmn->pdmnState->pubKeyOperator.Get().IsValid()) {
        AddUniqueProperty(dmn, dmn->pdmnState->pubKeyOperator);
    }
}

void CDeterministicMNList::UpdateMN(const uint256& proTxHash, const CDeterministicMNStateCPtr& pdmnState)
{
    auto oldDmn = mnMap.find(proTxHash);
    assert(oldDmn != nullptr);
    auto dmn = std::make_shared<CDeterministicMN>(**oldDmn);
    auto oldState = dmn->pdmnState;
    dmn->pdmnState = pdmnState;
    mnMap = mnMap.set(proTxHash, dmn);

    UpdateUniqueProperty(dmn, oldState->addr, pdmnState->addr);
    UpdateUniqueProperty(dmn, oldState->keyIDOwner, pdmnState->keyIDOwner);
    UpdateUniqueProperty(dmn, oldState->pubKeyOperator, pdmnState->pubKeyOperator);
}

void CDeterministicMNList::RemoveMN(const uint256& proTxHash)
{
    auto dmn = GetMN(proTxHash);
    assert(dmn != nullptr);
    DeleteUniqueProperty(dmn, dmn->collateralOutpoint);
    if (dmn->pdmnState->addr != CService()) {
        DeleteUniqueProperty(dmn, dmn->pdmnState->addr);
    }
    DeleteUniqueProperty(dmn, dmn->pdmnState->keyIDOwner);
    if (dmn->pdmnState->pubKeyOperator.Get().IsValid()) {
        DeleteUniqueProperty(dmn, dmn->pdmnState->pubKeyOperator);
    }
    mnMap = mnMap.erase(proTxHash);
}

CDeterministicMNManager::CDeterministicMNManager(CEvoDB& _evoDb) :
    evoDb(_evoDb)
{
}

bool CDeterministicMNManager::ProcessBlock(const CBlock& block, const CBlockIndex* pindex, CValidationState& _state, bool fJustCheck)
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

    {
        LOCK(cs);

        if (!BuildNewListFromBlock(block, pindex->pprev, _state, newList, true)) {
            return false;
        }

        if (fJustCheck) {
            return true;
        }

        if (newList.GetHeight() == -1) {
            newList.SetHeight(nHeight);
        }

        newList.SetBlockHash(block.GetHash());

        oldList = GetListForBlock(pindex->pprev->GetBlockHash());
        diff = oldList.BuildDiff(newList);

        evoDb.Write(std::make_pair(DB_LIST_DIFF, diff.blockHash), diff);
        if ((nHeight % SNAPSHOT_LIST_PERIOD) == 0 || oldList.GetHeight() == -1) {
            evoDb.Write(std::make_pair(DB_LIST_SNAPSHOT, diff.blockHash), newList);
            LogPrintf("CDeterministicMNManager::%s -- Wrote snapshot. nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                __func__, nHeight, newList.GetAllMNsCount());
        }
    }

    // Don't hold cs while calling signals
    if (diff.HasChanges()) {
        GetMainSignals().NotifyMasternodeListChanged(false, oldList, diff);
        uiInterface.NotifyMasternodeListChanged(newList);
    }

    if (nHeight == consensusParams.DIP0003EnforcementHeight) {
        if (!consensusParams.DIP0003EnforcementHash.IsNull() && consensusParams.DIP0003EnforcementHash != pindex->GetBlockHash()) {
            LogPrintf("CDeterministicMNManager::%s -- DIP3 enforcement block has wrong hash: hash=%s, expected=%s, nHeight=%d\n", __func__,
                    pindex->GetBlockHash().ToString(), consensusParams.DIP0003EnforcementHash.ToString(), nHeight);
            return _state.DoS(100, false, REJECT_INVALID, "bad-dip3-enf-block");
        }
        LogPrintf("CDeterministicMNManager::%s -- DIP3 is enforced now. nHeight=%d\n", __func__, nHeight);
    }

    LOCK(cs);
    CleanupCache(nHeight);

    return true;
}

bool CDeterministicMNManager::UndoBlock(const CBlock& block, const CBlockIndex* pindex)
{
    int nHeight = pindex->nHeight;
    uint256 blockHash = block.GetHash();

    CDeterministicMNList curList;
    CDeterministicMNList prevList;
    CDeterministicMNListDiff diff;
    {
        LOCK(cs);
        evoDb.Read(std::make_pair(DB_LIST_DIFF, blockHash), diff);

        if (diff.HasChanges()) {
            // need to call this before erasing
            curList = GetListForBlock(blockHash);
            prevList = GetListForBlock(pindex->pprev->GetBlockHash());
        }

        evoDb.Erase(std::make_pair(DB_LIST_DIFF, blockHash));
        evoDb.Erase(std::make_pair(DB_LIST_SNAPSHOT, blockHash));

        mnListsCache.erase(blockHash);
    }

    if (diff.HasChanges()) {
        auto inversedDiff = curList.BuildDiff(prevList);
        GetMainSignals().NotifyMasternodeListChanged(true, curList, inversedDiff);
        uiInterface.NotifyMasternodeListChanged(prevList);
    }

    const auto& consensusParams = Params().GetConsensus();
    if (nHeight == consensusParams.DIP0003EnforcementHeight) {
        LogPrintf("CDeterministicMNManager::%s -- DIP3 is not enforced anymore. nHeight=%d\n", __func__, nHeight);
    }

    return true;
}

void CDeterministicMNManager::UpdatedBlockTip(const CBlockIndex* pindex)
{
    LOCK(cs);

    tipHeight = pindex->nHeight;
    tipBlockHash = pindex->GetBlockHash();
}

bool CDeterministicMNManager::BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, CValidationState& _state, CDeterministicMNList& mnListRet, bool debugLogs)
{
    AssertLockHeld(cs);

    int nHeight = pindexPrev->nHeight + 1;

    CDeterministicMNList oldList = GetListForBlock(pindexPrev->GetBlockHash());
    CDeterministicMNList newList = oldList;
    newList.SetBlockHash(uint256()); // we can't know the final block hash, so better not return a (invalid) block hash
    newList.SetHeight(nHeight);

    auto payee = oldList.GetMNPayee();

    // we iterate the oldList here and update the newList
    // this is only valid as long these have not diverged at this point, which is the case as long as we don't add
    // code above this loop that modifies newList
    oldList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        if (!dmn->pdmnState->confirmedHash.IsNull()) {
            // already confirmed
            return;
        }
        // this works on the previous block, so confirmation will happen one block after nMasternodeMinimumConfirmations
        // has been reached, but the block hash will then point to the block at nMasternodeMinimumConfirmations
        int nConfirmations = pindexPrev->nHeight - dmn->pdmnState->nRegisteredHeight;
        if (nConfirmations >= Params().GetConsensus().nMasternodeMinimumConfirmations) {
            CDeterministicMNState newState = *dmn->pdmnState;
            newState.UpdateConfirmedHash(dmn->proTxHash, pindexPrev->GetBlockHash());
            newList.UpdateMN(dmn->proTxHash, std::make_shared<CDeterministicMNState>(newState));
        }
    });

    DecreasePoSePenalties(newList);

    // we skip the coinbase
    for (int i = 1; i < (int)block.vtx.size(); i++) {
        const CTransaction& tx = *block.vtx[i];

        if (tx.nVersion != 3) {
            // only interested in special TXs
            continue;
        }

        if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
            CProRegTx proTx;
            if (!GetTxPayload(tx, proTx)) {
                assert(false); // this should have been handled already
            }

            auto dmn = std::make_shared<CDeterministicMN>();
            dmn->proTxHash = tx.GetHash();

            // collateralOutpoint is either pointing to an external collateral or to the ProRegTx itself
            if (proTx.collateralOutpoint.hash.IsNull()) {
                dmn->collateralOutpoint = COutPoint(tx.GetHash(), proTx.collateralOutpoint.n);
            } else {
                dmn->collateralOutpoint = proTx.collateralOutpoint;
            }

            Coin coin;
            if (!proTx.collateralOutpoint.hash.IsNull() && (!GetUTXOCoin(dmn->collateralOutpoint, coin) || coin.out.nValue != 1000 * COIN)) {
                // should actually never get to this point as CheckProRegTx should have handled this case.
                // We do this additional check nevertheless to be 100% sure
                return _state.DoS(100, false, REJECT_INVALID, "bad-protx-collateral");
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
                return _state.DoS(100, false, REJECT_CONFLICT, "bad-protx-dup-addr");
            }
            if (newList.HasUniqueProperty(proTx.keyIDOwner) || newList.HasUniqueProperty(proTx.pubKeyOperator)) {
                return _state.DoS(100, false, REJECT_CONFLICT, "bad-protx-dup-key");
            }

            dmn->nOperatorReward = proTx.nOperatorReward;
            dmn->pdmnState = std::make_shared<CDeterministicMNState>(proTx);

            CDeterministicMNState dmnState = *dmn->pdmnState;
            dmnState.nRegisteredHeight = nHeight;

            if (proTx.addr == CService()) {
                // start in banned pdmnState as we need to wait for a ProUpServTx
                dmnState.nPoSeBanHeight = nHeight;
            }

            dmn->pdmnState = std::make_shared<CDeterministicMNState>(dmnState);

            newList.AddMN(dmn);

            if (debugLogs) {
                LogPrintf("CDeterministicMNManager::%s -- MN %s added at height %d: %s\n",
                    __func__, tx.GetHash().ToString(), nHeight, proTx.ToString());
            }
        } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
            CProUpServTx proTx;
            if (!GetTxPayload(tx, proTx)) {
                assert(false); // this should have been handled already
            }

            if (newList.HasUniqueProperty(proTx.addr) && newList.GetUniquePropertyMN(proTx.addr)->proTxHash != proTx.proTxHash) {
                return _state.DoS(100, false, REJECT_CONFLICT, "bad-protx-dup-addr");
            }

            CDeterministicMNCPtr dmn = newList.GetMN(proTx.proTxHash);
            if (!dmn) {
                return _state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");
            }
            auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
            newState->addr = proTx.addr;
            newState->scriptOperatorPayout = proTx.scriptOperatorPayout;

            if (newState->nPoSeBanHeight != -1) {
                // only revive when all keys are set
                if (newState->pubKeyOperator.Get().IsValid() && !newState->keyIDVoting.IsNull() && !newState->keyIDOwner.IsNull()) {
                    newState->nPoSePenalty = 0;
                    newState->nPoSeBanHeight = -1;
                    newState->nPoSeRevivedHeight = nHeight;

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
        } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
            CProUpRegTx proTx;
            if (!GetTxPayload(tx, proTx)) {
                assert(false); // this should have been handled already
            }

            CDeterministicMNCPtr dmn = newList.GetMN(proTx.proTxHash);
            if (!dmn) {
                return _state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");
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
        } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
            CProUpRevTx proTx;
            if (!GetTxPayload(tx, proTx)) {
                assert(false); // this should have been handled already
            }

            CDeterministicMNCPtr dmn = newList.GetMN(proTx.proTxHash);
            if (!dmn) {
                return _state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");
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
        } else if (tx.nType == TRANSACTION_QUORUM_COMMITMENT) {
            llmq::CFinalCommitmentTxPayload qc;
            if (!GetTxPayload(tx, qc)) {
                assert(false); // this should have been handled already
            }
            if (!qc.commitment.IsNull()) {
                HandleQuorumCommitment(qc.commitment, newList, debugLogs);
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

void CDeterministicMNManager::HandleQuorumCommitment(llmq::CFinalCommitment& qc, CDeterministicMNList& mnList, bool debugLogs)
{
    // The commitment has already been validated at this point so it's safe to use members of it

    auto members = llmq::CLLMQUtils::GetAllQuorumMembers((Consensus::LLMQType)qc.llmqType, qc.quorumHash);

    for (size_t i = 0; i < members.size(); i++) {
        if (!mnList.HasMN(members[i]->proTxHash)) {
            continue;
        }
        if (!qc.validMembers[i]) {
            // punish MN for failed DKG participation
            // The idea is to immediately ban a MN when it fails 2 DKG sessions with only a few blocks in-between
            // If there were enough blocks between failures, the MN has a chance to recover as he reduces his penalty by 1 for every block
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
        if (dmn->pdmnState->nPoSePenalty > 0 && dmn->pdmnState->nPoSeBanHeight == -1) {
            toDecrease.emplace_back(dmn->proTxHash);
        }
    });

    for (const auto& proTxHash : toDecrease) {
        mnList.PoSeDecrease(proTxHash);
    }
}

CDeterministicMNList CDeterministicMNManager::GetListForBlock(const uint256& blockHash)
{
    LOCK(cs);

    auto it = mnListsCache.find(blockHash);
    if (it != mnListsCache.end()) {
        return it->second;
    }

    uint256 blockHashTmp = blockHash;
    CDeterministicMNList snapshot;
    std::list<CDeterministicMNListDiff> listDiff;

    while (true) {
        // try using cache before reading from disk
        it = mnListsCache.find(blockHashTmp);
        if (it != mnListsCache.end()) {
            snapshot = it->second;
            break;
        }

        if (evoDb.Read(std::make_pair(DB_LIST_SNAPSHOT, blockHashTmp), snapshot)) {
            mnListsCache.emplace(blockHashTmp, snapshot);
            break;
        }

        CDeterministicMNListDiff diff;
        if (!evoDb.Read(std::make_pair(DB_LIST_DIFF, blockHashTmp), diff)) {
            snapshot = CDeterministicMNList(blockHashTmp, -1);
            mnListsCache.emplace(blockHashTmp, snapshot);
            break;
        }

        listDiff.emplace_front(diff);
        blockHashTmp = diff.prevBlockHash;
    }

    for (const auto& diff : listDiff) {
        if (diff.HasChanges()) {
            snapshot = snapshot.ApplyDiff(diff);
        } else {
            snapshot.SetBlockHash(diff.blockHash);
            snapshot.SetHeight(diff.nHeight);
        }

        mnListsCache.emplace(diff.blockHash, snapshot);
    }

    return snapshot;
}

CDeterministicMNList CDeterministicMNManager::GetListAtChainTip()
{
    LOCK(cs);
    return GetListForBlock(tipBlockHash);
}

bool CDeterministicMNManager::IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n)
{
    if (tx->nVersion != 3 || tx->nType != TRANSACTION_PROVIDER_REGISTER) {
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
    if (tx->vout[n].nValue != 1000 * COIN) {
        return false;
    }
    return true;
}

bool CDeterministicMNManager::IsDIP3Enforced(int nHeight)
{
    LOCK(cs);

    if (nHeight == -1) {
        nHeight = tipHeight;
    }

    return nHeight >= Params().GetConsensus().DIP0003EnforcementHeight;
}

void CDeterministicMNManager::CleanupCache(int nHeight)
{
    AssertLockHeld(cs);

    std::vector<uint256> toDelete;
    for (const auto& p : mnListsCache) {
        if (p.second.GetHeight() + LISTS_CACHE_SIZE < nHeight) {
            toDelete.emplace_back(p.first);
        }
    }
    for (const auto& h : toDelete) {
        mnListsCache.erase(h);
    }
}
