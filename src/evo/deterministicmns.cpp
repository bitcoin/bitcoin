// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "deterministicmns.h"
#include "specialtx.h"

#include "validation.h"
#include "validationinterface.h"
#include "chainparams.h"
#include "script/standard.h"
#include "base58.h"
#include "core_io.h"
#include "spork.h"

#include <univalue.h>

static const std::string DB_LIST_SNAPSHOT = "dmn_S";
static const std::string DB_LIST_DIFF = "dmn_D";

CDeterministicMNManager* deterministicMNManager;

std::string CDeterministicMNState::ToString() const
{
    CTxDestination dest;
    std::string payoutAddress = "unknown";
    std::string operatorRewardAddress = "none";
    if (ExtractDestination(scriptPayout, dest)) {
        payoutAddress = CBitcoinAddress(dest).ToString();
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        operatorRewardAddress = CBitcoinAddress(dest).ToString();
    }

    return strprintf("CDeterministicMNState(nRegisteredHeight=%d, nLastPaidHeight=%d, nPoSePenalty=%d, nPoSeRevivedHeight=%d, nPoSeBanHeight=%d, nRevocationReason=%d, "
                     "keyIDOwner=%s, keyIDOperator=%s, keyIDVoting=%s, addr=%s, nProtocolVersion=%d, payoutAddress=%s, operatorRewardAddress=%s)",
                     nRegisteredHeight, nLastPaidHeight, nPoSePenalty, nPoSeRevivedHeight, nPoSeBanHeight, nRevocationReason,
                     keyIDOwner.ToString(), keyIDOperator.ToString(), keyIDVoting.ToString(), addr.ToStringIPPort(false), nProtocolVersion, payoutAddress, operatorRewardAddress);
}

void CDeterministicMNState::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("registeredHeight", nRegisteredHeight));
    obj.push_back(Pair("lastPaidHeight", nLastPaidHeight));
    obj.push_back(Pair("PoSePenalty", nPoSePenalty));
    obj.push_back(Pair("PoSeRevivedHeight", nPoSeRevivedHeight));
    obj.push_back(Pair("PoSeBanHeight", nPoSeBanHeight));
    obj.push_back(Pair("revocationReason", nRevocationReason));
    obj.push_back(Pair("keyIDOwner", keyIDOwner.ToString()));
    obj.push_back(Pair("keyIDOperator", keyIDOperator.ToString()));
    obj.push_back(Pair("keyIDVoting", keyIDVoting.ToString()));
    obj.push_back(Pair("addr", addr.ToStringIPPort(false)));
    obj.push_back(Pair("protocolVersion", nProtocolVersion));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        CBitcoinAddress bitcoinAddress(dest);
        obj.push_back(Pair("payoutAddress", bitcoinAddress.ToString()));
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        CBitcoinAddress bitcoinAddress(dest);
        obj.push_back(Pair("operatorRewardAddress", bitcoinAddress.ToString()));
    }
}

std::string CDeterministicMN::ToString() const
{
    return strprintf("CDeterministicMN(proTxHash=%s, nCollateralIndex=%d, nOperatorReward=%f, state=%s", proTxHash.ToString(), nCollateralIndex, (double)nOperatorReward / 100, pdmnState->ToString());
}

void CDeterministicMN::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();

    UniValue stateObj;
    pdmnState->ToJson(stateObj);

    obj.push_back(Pair("proTxHash", proTxHash.ToString()));
    obj.push_back(Pair("collateralIndex", (int)nCollateralIndex));
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

CDeterministicMNCPtr CDeterministicMNList::GetMNByOperatorKey(const CKeyID& keyID)
{
    for (const auto& p : mnMap) {
        if (p.second->pdmnState->keyIDOperator == keyID) {
            return p.second;
        }
    }
    return nullptr;
}

static int CompareByLastPaid_GetHeight(const CDeterministicMN &dmn)
{
    int h = dmn.pdmnState->nLastPaidHeight;
    if (dmn.pdmnState->nPoSeRevivedHeight != -1 && dmn.pdmnState->nPoSeRevivedHeight > h) {
        h = dmn.pdmnState->nPoSeRevivedHeight;
    } else if (h == 0) {
        h = dmn.pdmnState->nRegisteredHeight;
    }
    return h;
}

static bool CompareByLastPaid(const CDeterministicMN &_a, const CDeterministicMN &_b)
{
    int ah = CompareByLastPaid_GetHeight(_a);
    int bh = CompareByLastPaid_GetHeight(_b);
    if (ah == bh) {
        return _a.proTxHash < _b.proTxHash;
    } else {
        return ah < bh;
    }
}
static bool CompareByLastPaid(const CDeterministicMNCPtr &_a, const CDeterministicMNCPtr &_b)
{
    return CompareByLastPaid(*_a, *_b);
}

CDeterministicMNCPtr CDeterministicMNList::GetMNPayee() const
{
    if (mnMap.size() == 0)
        return nullptr;

    CDeterministicMNCPtr best;
    for (const auto& dmn : valid_range()) {
        if (!best || CompareByLastPaid(dmn, best))
            best = dmn;
    }

    return best;
}

std::vector<CDeterministicMNCPtr> CDeterministicMNList::GetProjectedMNPayees(int nCount) const
{
    std::vector<CDeterministicMNCPtr> result;
    result.reserve(nCount);

    CDeterministicMNList tmpMNList = *this;
    for (int h = nHeight; h < nHeight + nCount; h++) {
        tmpMNList.SetHeight(h);

        CDeterministicMNCPtr payee = tmpMNList.GetMNPayee();
        // push the original MN object instead of the one from the temporary list
        result.push_back(GetMN(payee->proTxHash));

        CDeterministicMNStatePtr newState = std::make_shared<CDeterministicMNState>(*payee->pdmnState);
        newState->nLastPaidHeight = h;
        tmpMNList.UpdateMN(payee->proTxHash, newState);
    }

    return result;
}

CDeterministicMNListDiff CDeterministicMNList::BuildDiff(const CDeterministicMNList& to) const
{
    CDeterministicMNListDiff diffRet;
    diffRet.prevBlockHash = blockHash;
    diffRet.blockHash = to.blockHash;
    diffRet.nHeight = to.nHeight;

    for (const auto& p : to.mnMap) {
        const auto fromPtr = mnMap.find(p.first);
        if (fromPtr == nullptr) {
            diffRet.addedMNs.emplace(p.first, p.second);
        } else if (*p.second->pdmnState != *(*fromPtr)->pdmnState) {
            diffRet.updatedMNs.emplace(p.first, p.second->pdmnState);
        }
    }
    for (const auto& p : mnMap) {
        const auto toPtr = to.mnMap.find(p.first);
        if (toPtr == nullptr) {
            diffRet.removedMns.insert(p.first);
        }
    }

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

void CDeterministicMNList::AddMN(const CDeterministicMNCPtr &dmn)
{
    assert(!mnMap.find(dmn->proTxHash));
    mnMap = mnMap.set(dmn->proTxHash, dmn);
    AddUniqueProperty(dmn, dmn->pdmnState->addr);
    AddUniqueProperty(dmn, dmn->pdmnState->keyIDOwner);
    AddUniqueProperty(dmn, dmn->pdmnState->keyIDOperator);
}

void CDeterministicMNList::UpdateMN(const uint256 &proTxHash, const CDeterministicMNStateCPtr &pdmnState)
{
    auto oldDmn = mnMap.find(proTxHash);
    assert(oldDmn != nullptr);
    auto dmn = std::make_shared<CDeterministicMN>(**oldDmn);
    auto oldState = dmn->pdmnState;
    dmn->pdmnState = pdmnState;
    mnMap = mnMap.set(proTxHash, dmn);

    UpdateUniqueProperty(dmn, oldState->addr, pdmnState->addr);
    UpdateUniqueProperty(dmn, oldState->keyIDOwner, pdmnState->keyIDOwner);
    UpdateUniqueProperty(dmn, oldState->keyIDOperator, pdmnState->keyIDOperator);
}

void CDeterministicMNList::RemoveMN(const uint256& proTxHash)
{
    auto dmn = GetMN(proTxHash);
    assert(dmn != nullptr);
    DeleteUniqueProperty(dmn, dmn->pdmnState->addr);
    DeleteUniqueProperty(dmn, dmn->pdmnState->keyIDOwner);
    DeleteUniqueProperty(dmn, dmn->pdmnState->keyIDOperator);
    mnMap = mnMap.erase(proTxHash);
}

CDeterministicMNManager::CDeterministicMNManager(CEvoDB& _evoDb) :
    evoDb(_evoDb)
{
}

bool CDeterministicMNManager::ProcessBlock(const CBlock& block, const CBlockIndex* pindexPrev, CValidationState& _state)
{
    LOCK(cs);

    int nHeight = pindexPrev->nHeight + 1;

    CDeterministicMNList newList;
    if (!BuildNewListFromBlock(block, pindexPrev, _state, newList)) {
        return false;
    }

    if (newList.GetHeight() == -1) {
        newList.SetHeight(nHeight);
    }

    newList.SetBlockHash(block.GetHash());

    CDeterministicMNList oldList = GetListForBlock(pindexPrev->GetBlockHash());
    CDeterministicMNListDiff diff = oldList.BuildDiff(newList);

    evoDb.Write(std::make_pair(DB_LIST_DIFF, diff.blockHash), diff);
    if ((nHeight % SNAPSHOT_LIST_PERIOD) == 0) {
        evoDb.Write(std::make_pair(DB_LIST_SNAPSHOT, diff.blockHash), newList);
        LogPrintf("CDeterministicMNManager::%s -- Wrote snapshot. nHeight=%d, mapCurMNs.size=%d\n",
                  __func__, nHeight, newList.size());
    }

    if (nHeight == GetSpork15Value()) {
        LogPrintf("CDeterministicMNManager::%s -- spork15 is active now. nHeight=%d\n", __func__, nHeight);
    }

    CleanupCache(nHeight);

    return true;
}

bool CDeterministicMNManager::UndoBlock(const CBlock& block, const CBlockIndex* pindex)
{
    LOCK(cs);

    int nHeight = pindex->nHeight;

    evoDb.Erase(std::make_pair(DB_LIST_DIFF, block.GetHash()));
    evoDb.Erase(std::make_pair(DB_LIST_SNAPSHOT, block.GetHash()));

    if (nHeight == GetSpork15Value()) {
        LogPrintf("CDeterministicMNManager::%s -- spork15 is not active anymore. nHeight=%d\n", __func__, nHeight);
    }

    return true;
}

void CDeterministicMNManager::UpdatedBlockTip(const CBlockIndex* pindex)
{
    LOCK(cs);

    tipHeight = pindex->nHeight;
    tipBlockHash = pindex->GetBlockHash();
}

bool CDeterministicMNManager::BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, CValidationState& _state, CDeterministicMNList& mnListRet)
{
    AssertLockHeld(cs);

    int nHeight = pindexPrev->nHeight + 1;

    CDeterministicMNList oldList = GetListForBlock(pindexPrev->GetBlockHash());
    CDeterministicMNList newList = oldList;
    newList.SetBlockHash(uint256()); // we can't know the final block hash, so better not return a (invalid) block hash
    newList.SetHeight(nHeight);

    auto payee = oldList.GetMNPayee();

    // we skip the coinbase
    for (int i = 1; i < (int)block.vtx.size(); i++) {
        const CTransaction& tx = *block.vtx[i];

        // check if any existing MN collateral is spent by this transaction
        for (const auto& in : tx.vin) {
            const uint256& proTxHash = in.prevout.hash;
            auto dmn = newList.GetMN(proTxHash);
            if (dmn && dmn->nCollateralIndex == in.prevout.n) {
                newList.RemoveMN(proTxHash);

                LogPrintf("CDeterministicMNManager::%s -- MN %s removed from list because collateral was spent. nHeight=%d, mapCurMNs.size=%d\n",
                          __func__, proTxHash.ToString(), nHeight, newList.size());
            }
        }

        if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
            CProRegTx proTx;
            if (!GetTxPayload(tx, proTx)) {
                assert(false); // this should have been handled already
            }

            if (newList.HasUniqueProperty(proTx.addr))
                return _state.DoS(100, false, REJECT_CONFLICT, "bad-protx-dup-addr");
            if (newList.HasUniqueProperty(proTx.keyIDOwner) || newList.HasUniqueProperty(proTx.keyIDOperator))
                return _state.DoS(100, false, REJECT_CONFLICT, "bad-protx-dup-key");

            auto dmn = std::make_shared<CDeterministicMN>(tx.GetHash(), proTx);

            CDeterministicMNState dmnState = *dmn->pdmnState;
            dmnState.nRegisteredHeight = nHeight;

            if (proTx.addr == CService() || proTx.nProtocolVersion == 0) {
                // start in banned pdmnState as we need to wait for a ProUpServTx
                dmnState.nPoSeBanHeight = nHeight;
            }

            dmn->pdmnState = std::make_shared<CDeterministicMNState>(dmnState);

            newList.AddMN(dmn);

            LogPrintf("CDeterministicMNManager::%s -- MN %s added at height %d: %s\n",
                      __func__, tx.GetHash().ToString(), nHeight, proTx.ToString());
        } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
            CProUpServTx proTx;
            if (!GetTxPayload(tx, proTx)) {
                assert(false); // this should have been handled already
            }

            if (newList.HasUniqueProperty(proTx.addr) && newList.GetUniquePropertyMN(proTx.addr)->proTxHash != proTx.proTxHash)
                return _state.DoS(100, false, REJECT_CONFLICT, "bad-protx-dup-addr");

            CDeterministicMNCPtr dmn = newList.GetMN(proTx.proTxHash);
            if (!dmn) {
                return _state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");
            }
            auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
            newState->addr = proTx.addr;
            newState->nProtocolVersion = proTx.nProtocolVersion;
            newState->scriptOperatorPayout = proTx.scriptOperatorPayout;

            if (newState->nPoSeBanHeight != -1) {
                // only revive when all keys are set
                if (!newState->keyIDOperator.IsNull() && !newState->keyIDVoting.IsNull() && !newState->keyIDOwner.IsNull()) {
                    newState->nPoSeBanHeight = -1;
                    newState->nPoSeRevivedHeight = nHeight;

                    LogPrintf("CDeterministicMNManager::%s -- MN %s revived at height %d\n",
                              __func__, proTx.proTxHash.ToString(), nHeight);
                }
            }

            newList.UpdateMN(proTx.proTxHash, newState);

            LogPrintf("CDeterministicMNManager::%s -- MN %s updated at height %d: %s\n",
                      __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
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
            if (newState->keyIDOperator != proTx.keyIDOperator) {
                // reset all operator related fields and put MN into PoSe-banned state in case the operator key changes
                newState->ResetOperatorFields();
                newState->BanIfNotBanned(nHeight);
            }
            newState->keyIDOperator = proTx.keyIDOperator;
            newState->keyIDVoting = proTx.keyIDVoting;
            newState->scriptPayout = proTx.scriptPayout;

            newList.UpdateMN(proTx.proTxHash, newState);

            LogPrintf("CDeterministicMNManager::%s -- MN %s updated at height %d: %s\n",
                      __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
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

            LogPrintf("CDeterministicMNManager::%s -- MN %s revoked operator key at height %d: %s\n",
                      __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
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

    while(true) {
        // try using cache before reading from disk
        it = mnListsCache.find(blockHashTmp);
        if (it != mnListsCache.end()) {
            snapshot = it->second;
            break;
        }

        if (evoDb.Read(std::make_pair(DB_LIST_SNAPSHOT, blockHashTmp), snapshot)) {
            break;
        }

        CDeterministicMNListDiff diff;
        if (!evoDb.Read(std::make_pair(DB_LIST_DIFF, blockHashTmp), diff)) {
            snapshot = CDeterministicMNList(blockHashTmp, -1);
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
    }

    mnListsCache.emplace(blockHash, snapshot);
    return snapshot;
}

CDeterministicMNList CDeterministicMNManager::GetListAtChainTip()
{
    LOCK(cs);
    return GetListForBlock(tipBlockHash);
}

CDeterministicMNCPtr CDeterministicMNManager::GetMN(const uint256& blockHash, const uint256& proTxHash)
{
    auto mnList = GetListForBlock(blockHash);
    return mnList.GetMN(proTxHash);
}

bool CDeterministicMNManager::HasValidMNAtBlock(const uint256& blockHash, const uint256& proTxHash)
{
    auto mnList = GetListForBlock(blockHash);
    return mnList.IsMNValid(proTxHash);
}

bool CDeterministicMNManager::HasValidMNAtChainTip(const uint256& proTxHash)
{
    return GetListAtChainTip().IsMNValid(proTxHash);
}

int64_t CDeterministicMNManager::GetSpork15Value()
{
    return sporkManager.GetSporkValue(SPORK_15_DETERMINISTIC_MNS_ENABLED);
}

bool CDeterministicMNManager::IsDeterministicMNsSporkActive(int nHeight)
{
    LOCK(cs);

    if (nHeight == -1) {
        nHeight = tipHeight;
    }

    int64_t spork15Value = GetSpork15Value();
    return nHeight >= spork15Value;
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
