// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/deterministicmns.h>
#include <evo/specialtx.h>

#include <base58.h>
#include <chainparams.h>
#include <core_io.h>
#include <script/script.h>
#include <node/interface_ui.h>
#include <validation.h>
#include <validationinterface.h>

#include <llmq/quorums_commitment.h>
#include <llmq/quorums_utils.h>
#include <univalue.h>
#include <shutdown.h>
#include <common/args.h>
#include <logging.h>
#include <interfaces/chain.h>
#include <util/fs.h>
#include <util/fs_helpers.h>

#include <algorithm>
#include <fstream>
bool fMasternodeMode = false;
int64_t DEFAULT_MAX_RECOVERED_SIGS_AGE = 60 * 60 * 24 * 7; // keep them for a week

std::unique_ptr<CDeterministicMNManager> deterministicMNManager;

namespace {
using EvoSnapshotEntry = std::pair<uint256, CDeterministicMNList>;
using EvoEraseSet = std::unordered_set<uint256, StaticSaltedHasher>;

fs::path RewriteBackupPath(const fs::path& db_path)
{
    fs::path backup_path{db_path};
    backup_path += ".rewrite-backup";
    return backup_path;
}

fs::path RewriteMarkerPath(const fs::path& db_path)
{
    fs::path marker_path{db_path};
    marker_path += ".rewrite-in-progress";
    return marker_path;
}

enum class RewriteMarkerState {
    NONE,
    PREPARED,
    COMPLETE,
    UNKNOWN,
};

bool WriteRewriteMarker(const fs::path& marker_path, const char* state)
{
    FILE* marker_file{fsbridge::fopen(marker_path, "w")};
    if (!marker_file) {
        return false;
    }
    const bool write_ok = std::fputs(state, marker_file) >= 0;
    const bool commit_ok = write_ok && FileCommit(marker_file);
    std::fclose(marker_file);
    if (!commit_ok) {
        return false;
    }
    DirectoryCommit(marker_path.parent_path());
    return true;
}

RewriteMarkerState ReadRewriteMarkerState(const fs::path& marker_path)
{
    if (!fs::exists(marker_path)) {
        return RewriteMarkerState::NONE;
    }

    std::ifstream marker_file(fs::PathToString(marker_path));
    if (!marker_file.good()) {
        return RewriteMarkerState::UNKNOWN;
    }

    std::string state;
    marker_file >> state;
    if (state == "prepared") {
        return RewriteMarkerState::PREPARED;
    }
    if (state == "complete") {
        return RewriteMarkerState::COMPLETE;
    }
    return RewriteMarkerState::UNKNOWN;
}

void RemoveRewriteMarker(const fs::path& marker_path)
{
    std::error_code ec;
    fs::remove(marker_path, ec);
}

void RecoverRewriteBackupIfPresent(const DBParams& db_params)
{
    if (db_params.memory_only || db_params.path.empty()) {
        return;
    }

    const fs::path backup_path{RewriteBackupPath(db_params.path)};
    const fs::path marker_path{RewriteMarkerPath(db_params.path)};
    const bool has_backup{fs::exists(backup_path)};
    const RewriteMarkerState marker_state{ReadRewriteMarkerState(marker_path)};
    const bool has_marker{marker_state != RewriteMarkerState::NONE};

    if (has_backup && has_marker) {
        if (marker_state == RewriteMarkerState::COMPLETE && fs::exists(db_params.path)) {
            std::error_code ec;
            fs::remove_all(backup_path, ec);
            if (ec) {
                LogPrint(BCLog::SYS,
                         "CDeterministicMNManager::%s -- Failed to remove completed EvoDB rewrite backup %s: %s\n",
                         __func__, fs::PathToString(backup_path), ec.message());
            } else {
                LogPrint(BCLog::SYS,
                         "CDeterministicMNManager::%s -- Kept completed EvoDB rewrite at %s and removed stale backup %s\n",
                         __func__, fs::PathToString(db_params.path), fs::PathToString(backup_path));
                RemoveRewriteMarker(marker_path);
            }
        } else {
            std::error_code ec;
            fs::remove_all(db_params.path, ec);
            ec.clear();
            fs::rename(backup_path, db_params.path, ec);
            if (ec) {
                LogPrint(BCLog::SYS,
                         "CDeterministicMNManager::%s -- Failed to recover EvoDB rewrite backup %s -> %s: %s\n",
                         __func__, fs::PathToString(backup_path), fs::PathToString(db_params.path), ec.message());
            } else {
                LogPrint(BCLog::SYS,
                         "CDeterministicMNManager::%s -- Recovered EvoDB rewrite backup %s -> %s\n",
                         __func__, fs::PathToString(backup_path), fs::PathToString(db_params.path));
                RemoveRewriteMarker(marker_path);
            }
        }
        return;
    }

    if (has_backup) {
        if (!fs::exists(db_params.path)) {
            std::error_code ec;
            fs::rename(backup_path, db_params.path, ec);
            if (ec) {
                LogPrint(BCLog::SYS,
                         "CDeterministicMNManager::%s -- Failed to recover EvoDB backup without marker %s -> %s: %s\n",
                         __func__, fs::PathToString(backup_path), fs::PathToString(db_params.path), ec.message());
            } else {
                LogPrint(BCLog::SYS,
                         "CDeterministicMNManager::%s -- Recovered EvoDB backup without marker %s -> %s\n",
                         __func__, fs::PathToString(backup_path), fs::PathToString(db_params.path));
            }
        } else {
            LogPrint(BCLog::SYS,
                     "CDeterministicMNManager::%s -- Preserving unmatched EvoDB backup %s because live DB %s also exists\n",
                     __func__, fs::PathToString(backup_path), fs::PathToString(db_params.path));
        }
    }

    if (has_marker) {
        RemoveRewriteMarker(marker_path);
    }
}

bool SnapshotEntryLess(const EvoSnapshotEntry& a, const EvoSnapshotEntry& b)
{
    if (a.second.GetHeight() != b.second.GetHeight()) {
        return a.second.GetHeight() < b.second.GetHeight();
    }
    return a.first < b.first;
}

bool SnapshotEntryGreater(const EvoSnapshotEntry& a, const EvoSnapshotEntry& b)
{
    return SnapshotEntryLess(b, a);
}

void RetainNewestSnapshot(
    std::vector<EvoSnapshotEntry>& retained_snapshots,
    size_t& total_snapshots,
    EvoSnapshotEntry snapshot)
{
    ++total_snapshots;

    if (retained_snapshots.size() < CDeterministicMNManager::LIST_CACHE_SIZE) {
        retained_snapshots.emplace_back(std::move(snapshot));
        std::push_heap(retained_snapshots.begin(), retained_snapshots.end(), SnapshotEntryGreater);
        return;
    }

    if (SnapshotEntryLess(retained_snapshots.front(), snapshot)) {
        std::pop_heap(retained_snapshots.begin(), retained_snapshots.end(), SnapshotEntryGreater);
        retained_snapshots.back() = std::move(snapshot);
        std::push_heap(retained_snapshots.begin(), retained_snapshots.end(), SnapshotEntryGreater);
    }
}

bool CollectNewestPersistedSnapshots(
    CEvoDB<uint256, CDeterministicMNList, StaticSaltedHasher>& evo_db,
    const EvoEraseSet& skip_keys,
    std::vector<EvoSnapshotEntry>& retained_snapshots,
    size_t& total_snapshots)
{
    std::unique_ptr<CDBIterator> cursor(evo_db.NewIterator());
    if (!cursor) {
        LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to create EvoDB iterator\n", __func__);
        return false;
    }

    for (cursor->SeekToFirst(); cursor->Valid(); cursor->Next()) {
        uint256 key;
        if (!cursor->GetKey(key)) {
            continue;
        }
        if (skip_keys.count(key) != 0) {
            continue;
        }

        CDeterministicMNList snapshot;
        if (!cursor->GetValue(snapshot)) {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to read EvoDB snapshot for %s\n", __func__, key.ToString());
            return false;
        }
        RetainNewestSnapshot(retained_snapshots, total_snapshots, std::make_pair(key, std::move(snapshot)));
    }

    return true;
}

bool ReadAllPersistedSnapshots(
    CEvoDB<uint256, CDeterministicMNList, StaticSaltedHasher>& evo_db,
    std::vector<EvoSnapshotEntry>& snapshots)
{
    std::unique_ptr<CDBIterator> cursor(evo_db.NewIterator());
    if (!cursor) {
        LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to create EvoDB iterator\n", __func__);
        return false;
    }

    for (cursor->SeekToFirst(); cursor->Valid(); cursor->Next()) {
        uint256 key;
        if (!cursor->GetKey(key)) {
            continue;
        }

        CDeterministicMNList snapshot;
        if (!cursor->GetValue(snapshot)) {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to read EvoDB snapshot for %s\n", __func__, key.ToString());
            return false;
        }
        snapshots.emplace_back(key, std::move(snapshot));
    }

    return true;
}

bool RewriteSnapshotWindow(
    CEvoDB<uint256, CDeterministicMNList, StaticSaltedHasher>& evo_db,
    const std::vector<EvoSnapshotEntry>& snapshots)
{
    const auto db_path = evo_db.StoragePath();
    std::vector<EvoSnapshotEntry> memory_only_backup;
    fs::path backup_path;
    fs::path marker_path;

    if (db_path) {
        backup_path = RewriteBackupPath(*db_path);
        marker_path = RewriteMarkerPath(*db_path);

        std::error_code ec;
        fs::remove_all(backup_path, ec);
        RemoveRewriteMarker(marker_path);
        if (!WriteRewriteMarker(marker_path, "prepared")) {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to create rewrite marker %s\n",
                     __func__, fs::PathToString(marker_path));
            return false;
        }

        evo_db.CloseDB();
        fs::rename(*db_path, backup_path, ec);
        if (ec) {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to back up EvoDB for rewrite: %s\n", __func__, ec.message());
            RemoveRewriteMarker(marker_path);
            evo_db.OpenDB(/*create_new=*/false);
            return false;
        }
        evo_db.OpenDB(/*create_new=*/true);
    } else {
        if (!ReadAllPersistedSnapshots(evo_db, memory_only_backup)) {
            return false;
        }
        evo_db.ResetDB();
    }

    CDBBatch batch(evo_db);
    std::size_t items{0};
    auto restore_original = [&]() {
        if (db_path) {
            evo_db.CloseDB();

            std::error_code ec;
            fs::remove_all(*db_path, ec);
            fs::rename(backup_path, *db_path, ec);
            if (ec) {
                LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to restore backed up EvoDB: %s\n", __func__, ec.message());
            } else {
                RemoveRewriteMarker(marker_path);
            }
            evo_db.OpenDB(/*create_new=*/false);
        } else {
            evo_db.ResetDB();
            CDBBatch restore_batch(evo_db);
            std::size_t restore_items{0};
            for (const auto& [key, snapshot] : memory_only_backup) {
                restore_batch.Write(key, snapshot);
                if (++restore_items == 256) {
                    if (!evo_db.SubmitBatchForTesting(restore_batch)) {
                        return false;
                    }
                    restore_batch.Clear();
                    restore_items = 0;
                }
            }
            if (restore_batch.SizeEstimate() != 0 && !evo_db.SubmitBatchForTesting(restore_batch)) {
                return false;
            }
        }
        return true;
    };
    auto restore_after_failure = [&](const char* message, const std::string& error = std::string()) {
        if (error.empty()) {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- %s\n", __func__, message);
        } else {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- %s: %s\n", __func__, message, error);
        }
        try {
            restore_original();
        } catch (const std::exception& e) {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to restore EvoDB after rewrite failure: %s\n", __func__, e.what());
        } catch (...) {
            LogPrint(BCLog::SYS, "CDeterministicMNManager::%s -- Failed to restore EvoDB after rewrite failure: unknown exception\n", __func__);
        }
        return false;
    };

    try {
        for (const auto& [key, snapshot] : snapshots) {
            batch.Write(key, snapshot);
            if (++items == 256) {
                if (!evo_db.SubmitBatchForTesting(batch)) {
                    return restore_after_failure("Failed to write compacted EvoDB batch");
                }
                batch.Clear();
                items = 0;
            }
        }

        if (batch.SizeEstimate() != 0 && !evo_db.SubmitBatchForTesting(batch)) {
            return restore_after_failure("Failed to write final compacted EvoDB batch");
        }
        if (db_path && !WriteRewriteMarker(marker_path, "complete")) {
            return restore_after_failure("Failed to update rewrite marker to complete");
        }
    } catch (const std::exception& e) {
        return restore_after_failure("Exception while rewriting compacted EvoDB", e.what());
    } catch (...) {
        return restore_after_failure("Unknown exception while rewriting compacted EvoDB");
    }

    if (db_path) {
        RemoveRewriteMarker(marker_path);
        DestroyDB(fs::PathToString(backup_path));
    }

    evo_db.ClearCaches();
    return true;
}
} // namespace

CDeterministicMNManager::CDeterministicMNManager(const DBParams& db_params)
{
    RecoverRewriteBackupIfPresent(db_params);
    m_evoDb = std::make_unique<CEvoDB<uint256, CDeterministicMNList, StaticSaltedHasher>>(db_params, LIST_CACHE_SIZE);
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

void CDeterministicMN::ToJson(interfaces::Chain& chain, UniValue& obj) const
{
    obj.clear();
    obj.setObject();

    UniValue stateObj;
    pdmnState->ToJson(stateObj);

    obj.pushKV("proTxHash", proTxHash.ToString());
    obj.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    obj.pushKV("collateralIndex", (int)collateralOutpoint.n);

    std::map<COutPoint, Coin> coins;
    coins[collateralOutpoint]; 
    chain.findCoins(coins);
    const Coin &coin = coins.at(collateralOutpoint);
    if (!coin.IsSpent()) {
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
    return IsMNValid(**p);
}

bool CDeterministicMNList::IsMNPoSeBanned(const uint256& proTxHash) const
{
    auto p = mnMap.find(proTxHash);
    if (p == nullptr) {
        return false;
    }
    return IsMNPoSeBanned(**p);
}

bool CDeterministicMNList::IsMNValid(const CDeterministicMN& dmn)
{
    return !IsMNPoSeBanned(dmn);
}

bool CDeterministicMNList::IsMNPoSeBanned(const CDeterministicMN& dmn)
{
    return dmn.pdmnState->IsBanned();
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
    if (dmn && !IsMNValid(*dmn)) {
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
    if (dmn && !IsMNValid(*dmn)) {
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

CDeterministicMNCPtr CDeterministicMNList::GetMNPayee() const
{
    if (mnMap.size() == 0) {
        return nullptr;
    }

    CDeterministicMNCPtr best;
    ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
        if (!best || CompareByLastPaid(dmn.get(), best.get())) {
            best = dmn;
        }
    });

    return best;
}

std::vector<CDeterministicMNCPtr> CDeterministicMNList::GetProjectedMNPayees(int nCount) const
{
    if (nCount < 0 ) {
        return {};
    }
    const size_t validCount = GetValidMNsCount();
    if ((size_t)nCount > validCount) {
        nCount = validCount;
    }

    std::vector<CDeterministicMNCPtr> result;
    result.reserve(nCount);

    ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
        result.emplace_back(dmn);
    });
    std::sort(result.begin(), result.end(), [&](const CDeterministicMNCPtr& a, const CDeterministicMNCPtr& b) {
        return CompareByLastPaid(a.get(), b.get());
    });

    result.resize(nCount);

    return result;
}

std::vector<CDeterministicMNCPtr> CDeterministicMNList::CalculateQuorum(size_t maxSize, const uint256& modifier) const
{
    auto scores = CalculateScores(modifier);

    // sort is descending order
    std::sort(scores.rbegin(), scores.rend(), [](const std::pair<arith_uint256, CDeterministicMNCPtr>& a, const std::pair<arith_uint256, CDeterministicMNCPtr>& b) {
        if (a.first == b.first) {
            // this should actually never happen, but we should stay compatible with how the non-deterministic MNs did the sorting
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
    static const int TESTNET_MIN_REGISTRATION_HEIGHT = 1000000;
    int nAllowedLegacyNodes = 25;
    int nLegacyNodeCount = 0;
    std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> scores;
    scores.reserve(GetAllMNsCount());
    ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
        if (dmn->pdmnState->confirmedHash.IsNull()) {
            // we only take confirmed MNs into account to avoid hash grinding on the ProRegTxHash to sneak MNs into a
            // future quorums
            return;
        }
        // remove old defunct nodes on testnet
         if(fTestNet && dmn->pdmnState->nRegisteredHeight < TESTNET_MIN_REGISTRATION_HEIGHT) {
            nLegacyNodeCount++;
            if(nLegacyNodeCount > nAllowedLegacyNodes) {
                // Assign the lowest possible score (0) to deprioritize with descending sort
                LogPrint(BCLog::MNLIST, "CDeterministicMNList::%s -- Assigning score 0 to testnet MN %s (registered height %d < %d) due to limit %d\n",
                        __func__, dmn->proTxHash.ToString(), dmn->pdmnState->nRegisteredHeight, TESTNET_MIN_REGISTRATION_HEIGHT, nAllowedLegacyNodes);
                scores.emplace_back(arith_uint256(0), dmn);
                return; // Skip normal calculation
            }
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

void CDeterministicMNList::PoSePunish(const uint256& proTxHash, int penalty)
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


    LogPrint(BCLog::MNLIST, "CDeterministicMNList::%s -- punished MN %s, penalty %d->%d (max=%d)\n",
                __func__, proTxHash.ToString(), dmn->pdmnState->nPoSePenalty, newState->nPoSePenalty, maxPenalty);
    

    if (newState->nPoSePenalty >= maxPenalty && !newState->IsBanned()) {
        if(!newState->vchNEVMAddress.empty()) {
            m_changed_nevm_address = true;
        }
        newState->BanIfNotBanned(nHeight);
        LogPrint(BCLog::MNLIST, "CDeterministicMNList::%s -- banned MN %s at height %d\n",
                    __func__, proTxHash.ToString(), nHeight);
    
    }
    UpdateMN(proTxHash, newState);
}

void CDeterministicMNList::PoSeDecrease(const CDeterministicMN& dmn)
{
    assert(dmn.pdmnState->nPoSePenalty > 0 && !dmn.pdmnState->IsBanned());

    auto newState = std::make_shared<CDeterministicMNState>(*dmn.pdmnState);
    newState->nPoSePenalty--;
    UpdateMN(dmn, newState);
}

void CDeterministicMNList::BuildDiff(const CDeterministicMNList& to, CDeterministicMNListDiff &diffRet, CDeterministicMNListNEVMAddressDiff &diffRetNEVMAddress) const
{
    std::unordered_map<uint256, NEVMDiffEntry, StaticSaltedHasher> nevmDiffMap;
   // Process MNs from the new list (adds and updates)
    for (const auto& p : to.mnMap) {
        const auto& toPtr = p.second;
        auto fromPtr = GetMN(toPtr->proTxHash);
        if (fromPtr == nullptr) {
            // Masternode is newly added.
            if (!toPtr->pdmnState->vchNEVMAddress.empty()) {
                NEVMDiffEntry entry;
                entry.type = NEVMDiffType::Added;
                entry.newAddress = toPtr->pdmnState->vchNEVMAddress;
                entry.collateralHeight = uint32_t(toPtr->pdmnState->nCollateralHeight);
                nevmDiffMap[toPtr->proTxHash] = entry;
            }
            diffRet.addedMNs.emplace_back(toPtr);
        } else if (fromPtr != toPtr || fromPtr->pdmnState != toPtr->pdmnState) {
            // MN exists in both lists but has been updated.
            CDeterministicMNStateDiff stateDiff(*fromPtr->pdmnState, *toPtr->pdmnState);
            if(stateDiff.fields) {
                if (stateDiff.fields & CDeterministicMNStateDiff::Field_vchNEVMAddress) {
                    NEVMDiffEntry entry;
                    if (toPtr->pdmnState->vchNEVMAddress.empty()) {
                        // Address was removed.
                        entry.type = NEVMDiffType::Removed;
                        entry.oldAddress = fromPtr->pdmnState->vchNEVMAddress;
                    } else if (fromPtr->pdmnState->vchNEVMAddress.empty()) {
                        // Address was added.
                        entry.type = NEVMDiffType::Added;
                        entry.newAddress = toPtr->pdmnState->vchNEVMAddress;
                        entry.collateralHeight = uint32_t(toPtr->pdmnState->nCollateralHeight);
                    } else {
                        // Address was updated.
                        entry.type = NEVMDiffType::Updated;
                        entry.oldAddress = fromPtr->pdmnState->vchNEVMAddress;
                        entry.newAddress = toPtr->pdmnState->vchNEVMAddress;
                        entry.collateralHeight = uint32_t(toPtr->pdmnState->nCollateralHeight);
                    }
                    nevmDiffMap[toPtr->proTxHash] = entry;
                }
                diffRet.updatedMNs.emplace(toPtr->GetInternalId(), std::move(stateDiff));
            }
        }
    };
    if (mnMap.size() + diffRet.addedMNs.size() != to.mnMap.size()) {
        // Process removals from the old list.
        for (auto& fromPtr : mnMap) {
            const auto toPtr = to.GetMN(fromPtr.second->proTxHash);
            if (toPtr == nullptr) {
                // Masternode removed entirely.
                if (!fromPtr.second->pdmnState->vchNEVMAddress.empty()) {
                    NEVMDiffEntry entry;
                    entry.type = NEVMDiffType::Removed;
                    entry.oldAddress = fromPtr.second->pdmnState->vchNEVMAddress;
                    nevmDiffMap[fromPtr.second->proTxHash] = entry;
                }
                diffRet.removedMns.emplace(fromPtr.second->GetInternalId());
                if (mnMap.size() + diffRet.addedMNs.size() - diffRet.removedMns.size() == to.mnMap.size()) break;
            } else if (toPtr->pdmnState->vchNEVMAddress.empty() && !fromPtr.second->pdmnState->vchNEVMAddress.empty()) {
                // Masternode still exists but its NEVM address was cleared.
                NEVMDiffEntry entry;
                entry.type = NEVMDiffType::Removed;
                entry.oldAddress = fromPtr.second->pdmnState->vchNEVMAddress;
                nevmDiffMap[fromPtr.second->proTxHash] = entry;
            }
        };
    }

    // Now convert the deduplicated map into the diff vectors.
    for (const auto& pair : nevmDiffMap) {
        const NEVMDiffEntry& entry = pair.second;
        switch (entry.type) {
            case NEVMDiffType::Added:
                diffRetNEVMAddress.addedMNNEVM.emplace_back(entry.newAddress, entry.collateralHeight);
                break;
            case NEVMDiffType::Updated:
                diffRetNEVMAddress.updatedMNNEVM.emplace_back(entry.oldAddress, std::make_pair(entry.newAddress, entry.collateralHeight));
                break;
            case NEVMDiffType::Removed:
                diffRetNEVMAddress.removedMNNEVM.emplace_back(entry.oldAddress);
                break;
            default:
                break;
        }
    }

    // added MNs need to be sorted by internalId so that these are added in correct order when the diff is applied later
    // otherwise internalIds will not match with the original list
    std::sort(diffRet.addedMNs.begin(), diffRet.addedMNs.end(), [](const CDeterministicMNCPtr& a, const CDeterministicMNCPtr& b) {
        return a->GetInternalId() < b->GetInternalId();
    });
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
        result.UpdateMN(*dmn, p.second);
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

    if (!AddUniqueProperty(*dmn, dmn->collateralOutpoint)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate collateralOutpoint=%s", __func__,
                dmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort())));
    }
    if (dmn->pdmnState->addr != CService() && !AddUniqueProperty(*dmn, dmn->pdmnState->addr)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate address=%s", __func__,
                dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToStringAddrPort())));
    }
    if (!AddUniqueProperty(*dmn, dmn->pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate keyIDOwner=%s", __func__,
                dmn->proTxHash.ToString(), EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)))));
    }
    if (dmn->pdmnState->pubKeyOperator.Get().IsValid() && !AddUniqueProperty(*dmn, dmn->pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate pubKeyOperator=%s", __func__,
                dmn->proTxHash.ToString(), dmn->pdmnState->pubKeyOperator.ToString())));
    }
    if (!dmn->pdmnState->vchNEVMAddress.empty() && !AddUniqueProperty(*dmn, dmn->pdmnState->vchNEVMAddress)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't add a masternode %s with a duplicate vchNEVMAddress=%s", __func__,
                dmn->proTxHash.ToString(), dmn->pdmnState->pubKeyOperator.ToString())));
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
    dmn->pdmnState = pdmnState;

    // All mnUniquePropertyMap's updates must be atomic.
    // Using this temporary map as a checkpoint to rollback to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    if (!UpdateUniqueProperty(*dmn, oldState->addr, pdmnState->addr)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate address=%s", __func__,
                oldDmn.proTxHash.ToString(), pdmnState->addr.ToStringAddrPort())));
    }
    if (!UpdateUniqueProperty(*dmn, oldState->keyIDOwner, pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate keyIDOwner=%s", __func__,
                oldDmn.proTxHash.ToString(), EncodeDestination(WitnessV0KeyHash(pdmnState->keyIDOwner)))));
    }
    if (!UpdateUniqueProperty(*dmn, oldState->pubKeyOperator, pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate pubKeyOperator=%s", __func__,
                oldDmn.proTxHash.ToString(), pdmnState->pubKeyOperator.ToString())));
    }
    if (!UpdateUniqueProperty(*dmn, oldState->vchNEVMAddress, pdmnState->vchNEVMAddress)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't update a masternode %s with a duplicate old vchNEVMAddress=%s vs new vchNEVMAddress=%s", __func__,
                oldDmn.proTxHash.ToString(), HexStr(oldState->vchNEVMAddress), HexStr(pdmnState->vchNEVMAddress))));
    }
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
    // Using this temporary map as a checkpoint to rollback to in case of any issues.
    decltype(mnUniquePropertyMap) mnUniquePropertyMapSaved = mnUniquePropertyMap;

    if (!DeleteUniqueProperty(*dmn, dmn->collateralOutpoint)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a collateralOutpoint=%s", __func__,
                proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort())));
    }
    if (dmn->pdmnState->addr != CService() && !DeleteUniqueProperty(*dmn, dmn->pdmnState->addr)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a address=%s", __func__,
                proTxHash.ToString(), dmn->pdmnState->addr.ToStringAddrPort())));
    }
    if (!DeleteUniqueProperty(*dmn, dmn->pdmnState->keyIDOwner)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a keyIDOwner=%s", __func__,
                proTxHash.ToString(), EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)))));
    }
    if (dmn->pdmnState->pubKeyOperator.Get().IsValid() && !DeleteUniqueProperty(*dmn, dmn->pdmnState->pubKeyOperator)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a pubKeyOperator=%s", __func__,
                proTxHash.ToString(), dmn->pdmnState->pubKeyOperator.ToString())));
    }
    if (!dmn->pdmnState->vchNEVMAddress.empty() && !DeleteUniqueProperty(*dmn, dmn->pdmnState->vchNEVMAddress)) {
        mnUniquePropertyMap = mnUniquePropertyMapSaved;
        throw(std::runtime_error(strprintf("%s: Can't delete a masternode %s with a vchNEVMAddress=%s", __func__,
                proTxHash.ToString(), HexStr(dmn->pdmnState->vchNEVMAddress))));
    }
    mnMap = mnMap.erase(proTxHash);
    mnInternalIdMap = mnInternalIdMap.erase(dmn->GetInternalId());
}

std::string CDeterministicMNListNEVMAddressDiff::ToString() const {
    std::string addedStr, updatedStr, removedStr;

    for (const auto& entry : addedMNNEVM) {
        addedStr += strprintf("(Address=%s, CollateralHeight=%d) ", HexStr(entry.first), entry.second);
    }

    for (const auto& entry : updatedMNNEVM) {
        updatedStr += strprintf("(OldAddress=%s, NewAddress=%s, CollateralHeight=%d) ", HexStr(entry.first), HexStr(entry.second.first), entry.second.second);
    }

    for (const auto& entry : removedMNNEVM) {
        removedStr += strprintf("(Address=%s) ", HexStr(entry));
    }

    return strprintf(
        "CDeterministicMNListNEVMAddressDiff(Added=%s, Updated=%s, Removed=%s)",
        addedStr.empty() ? "None" : addedStr,
        updatedStr.empty() ? "None" : updatedStr,
        removedStr.empty() ? "None" : removedStr
    );
}

bool CDeterministicMNManager::ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& _state, const CCoinsViewCache& view, const llmq::CFinalCommitmentTxPayload &qcTx, CDeterministicMNListNEVMAddressDiff &diffNEVM, bool fJustCheck, bool ibd)
{
    const auto& consensusParams = Params().GetConsensus();
    bool fDIP0003Active = pindex->nHeight >= consensusParams.DIP0003Height;
    bool fNexusActive = pindex->nHeight >= consensusParams.nNexusStartBlock;
    if (!fDIP0003Active) {
        return true;
    }

    CDeterministicMNList oldList, newList;
    CDeterministicMNListDiff diff;

    int nHeight = pindex->nHeight;
    try {

        if (!BuildNewListFromBlock(block, pindex->pprev, _state, view, newList, oldList, qcTx)) {
            // pass the state returned by the function above
            return false;
        }
        if (fJustCheck) {
            return true;
        }

        if (newList.GetHeight() == -1) {
            newList.SetHeight(nHeight);
        }

        newList.SetBlockHash(pindex->GetBlockHash());

        if(!ibd || (fNEVMConnection && fNexusActive && newList.m_changed_nevm_address)) {
            oldList.BuildDiff(newList, diff, diffNEVM);
        }
        if(!ibd) {
            if (diff.HasChanges()) {
                GetMainSignals().NotifyMasternodeListChanged(false, oldList, diff);
            }
            // always update interface for payment detail changes
            uiInterface.NotifyMasternodeListChanged(newList);
        }
        m_evoDb->WriteCache(pindex->GetBlockHash(), std::move(newList));
       
    } catch (const std::exception& e) {
        LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- internal error: %s\n", __func__, e.what());
        return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-dmn-block");
    }

    return true;
}


bool CDeterministicMNManager::UndoBlock(const CBlockIndex* pindex, CDeterministicMNListNEVMAddressDiff &inversedDiffNEVMAddress)
{
    uint256 blockHash = pindex->GetBlockHash();

    CDeterministicMNList curList;
    CDeterministicMNList prevList;
    bool readCache = m_evoDb->ReadCache(blockHash, curList);
    if(readCache) {
        prevList = GetListForBlockInternal(pindex->pprev);
        CDeterministicMNListDiff inversedDiff;
        curList.BuildDiff(prevList, inversedDiff, inversedDiffNEVMAddress);
        if(inversedDiff.HasChanges()) {
            GetMainSignals().NotifyMasternodeListChanged(true, prevList, inversedDiff);
        }
        // SYSCOIN always update interface
        uiInterface.NotifyMasternodeListChanged(prevList);
    }
    return true;
}

bool CDeterministicMNManager::BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, BlockValidationState& _state, const CCoinsViewCache& view, CDeterministicMNList& mnListRet, CDeterministicMNList& oldList, const llmq::CFinalCommitmentTxPayload &qcTxIn)
{

    int nHeight = pindexPrev->nHeight + 1;

    oldList = GetListForBlock(pindexPrev);
    CDeterministicMNList newList = oldList;
    newList.SetBlockHash(uint256()); // we can't know the final block hash, so better not return a (invalid) block hash
    newList.SetHeight(nHeight);

    bool decreasePoSE = false;
    if(!fRegTest) {
        // in sys we only run one quorum so we need to be more sure in this service we will catch a bad MN within around 1 payment round
        if((nHeight % 3) == 0) {
            decreasePoSE = true;
        }
    } else {
        decreasePoSE = true;
    }
    auto payee = oldList.GetMNPayee();
    // at least 2 rounds of payments before registered MN's gets put in list
    const size_t mnCountThreshold = oldList.GetValidMNsCount()*2;
    // we iterate the oldList here and update the newList
    // this is only valid as long these have not diverged at this point, which is the case as long as we don't add
    // code above this loop that modifies newList
    std::vector<CDeterministicMNCPtr> toDecrease;
    toDecrease.reserve(oldList.GetAllMNsCount() / 10);
    oldList.ForEachMNShared(false, [&decreasePoSE, &oldList, &toDecrease, &mnCountThreshold, &pindexPrev, &newList](const CDeterministicMNCPtr& dmn) {
        if (dmn->pdmnState->confirmedHash.IsNull()) {
            // this works on the previous block, so confirmation will happen one block after mnCountThreshold
            // has been reached, but the block hash will then point to the block at mnCountThreshold
            const size_t nConfirmations = pindexPrev->nHeight - dmn->pdmnState->nRegisteredHeight;
            if (nConfirmations >= mnCountThreshold) {
                auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
                newState->UpdateConfirmedHash(dmn->proTxHash, pindexPrev->GetBlockHash());
                newList.UpdateMN(dmn->proTxHash, newState);
            }
        }
        if(decreasePoSE && oldList.IsMNValid(*dmn)) {
            if (dmn->pdmnState->nPoSePenalty > 0) {
                toDecrease.emplace_back(dmn);
            }
        }
    });
    // decrease PoSe ban score
    if(decreasePoSE) {
        DecreasePoSePenalties(newList, toDecrease);
    }

    if(!qcTxIn.commitment.IsNull()) {
        const auto& params = Params().GetConsensus().llmqTypeChainLocks;
        uint32_t quorumHeight = qcTxIn.nHeight - (qcTxIn.nHeight % params.dkgInterval);
        auto quorumIndex = pindexPrev->GetAncestor(quorumHeight);
        if (!quorumIndex || quorumIndex->GetBlockHash() != qcTxIn.commitment.quorumHash) {
            // we should actually never get into this case as validation should have caught it...but let's be sure
            return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-quorum-hash");
        }
        HandleQuorumCommitment(qcTxIn.commitment, quorumIndex, newList);            
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
                    if(!replacedDmn->pdmnState->vchNEVMAddress.empty()) {
                        newList.m_changed_nevm_address = true;
                    }
                    newList.RemoveMN(replacedDmn->proTxHash);
                    LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- MN %s removed from list because collateral was used for a new ProRegTx. collateralOutpoint=%s, nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                                __func__, replacedDmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort(), nHeight, newList.GetAllMNsCount());
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
                    if(!dmnState->vchNEVMAddress.empty()) {
                        newList.m_changed_nevm_address = true;
                    }
                    dmnState->BanIfNotBanned(nHeight);
                }
                dmn->pdmnState = dmnState;

                newList.AddMN(dmn);
                LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- MN %s added at height %d: %s\n",
                        __func__, tx.GetHash().ToString(), nHeight, proTx.ToString());
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
                // Validate NEVM address only if non-empty
                if (!proTx.vchNEVMAddress.empty()) {
                    if (proTx.vchNEVMAddress.size() != 20) {
                        return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-invalid-nevmaddress-size");
                    }
                    if (newList.HasUniqueProperty(proTx.vchNEVMAddress) && 
                        newList.GetUniquePropertyMN(proTx.vchNEVMAddress)->proTxHash != proTx.proTxHash) {
                        return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-nevm-address");
                    }
                }
                // Always handle NEVM address changes
                if (newState->vchNEVMAddress != proTx.vchNEVMAddress) {
                    if (newState->confirmedHash.IsNull()) {
                        return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-unconfirmed-nevm-address");
                    }
                    if(newState->IsBanned()) {
                        return _state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-banned-nevm-address");
                    }
                    newState->m_changed_nevm_address = true;
                    newState->vchNEVMAddress = proTx.vchNEVMAddress;
                }
                if (newState->IsBanned()) {
                    // only revive when all keys are set
                    if (newState->pubKeyOperator.Get().IsValid() && !newState->keyIDVoting.IsNull() && !newState->keyIDOwner.IsNull()) {
                        newState->Revive(nHeight);
                        LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- MN %s revived at height %d\n",
                                __func__, proTx.proTxHash.ToString(), nHeight);
                    }
                } 
                
                newList.UpdateMN(proTx.proTxHash, newState);
                LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- MN %s updated at height %d: %s\n",
                        __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
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
            
                // Handle pubKeyOperator changes
                if (newState->pubKeyOperator != proTx.pubKeyOperator) {
                    if(!newState->vchNEVMAddress.empty()) {
                        newList.m_changed_nevm_address = true;
                    }
                    newState->ResetOperatorFields();
                    newState->BanIfNotBanned(nHeight);
                    newState->nVersion = proTx.nVersion;
                    newState->pubKeyOperator = proTx.pubKeyOperator;
                }
            
                newState->keyIDVoting = proTx.keyIDVoting;
                newState->scriptPayout = proTx.scriptPayout;
                newList.UpdateMN(proTx.proTxHash, newState);
            
                LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- MN %s updated at height %d: %s\n",
                         __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
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
                if(!newState->vchNEVMAddress.empty()) {
                    newList.m_changed_nevm_address = true;
                }
                newState->ResetOperatorFields();
                newState->BanIfNotBanned(nHeight);
                newState->nRevocationReason = proTx.nReason;
                newList.UpdateMN(proTx.proTxHash, newState);
                LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- MN %s revoked operator key at height %d: %s\n",
                        __func__, proTx.proTxHash.ToString(), nHeight, proTx.ToString());
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
                if(!dmn->pdmnState->vchNEVMAddress.empty()) {
                    newList.m_changed_nevm_address = true;
                }
                newList.RemoveMN(dmn->proTxHash);
                LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- MN %s removed from list because collateral was spent. collateralOutpoint=%s, nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                              __func__, dmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort(), nHeight, newList.GetAllMNsCount());
            }
        }
    }
    
    // The payee for the current block was determined by the previous block's list, but it might have disappeared in the
    // current block. We still pay that MN one last time however.
    if (auto dmn = payee ? newList.GetMN(payee->proTxHash) : nullptr) {
        auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
        newState->nLastPaidHeight = nHeight;
        newList.UpdateMN(payee->proTxHash, newState);
    }

    mnListRet = std::move(newList);
    return true;
}

void CDeterministicMNManager::HandleQuorumCommitment(const llmq::CFinalCommitment& qc, const CBlockIndex* pQuorumBaseBlockIndex, CDeterministicMNList& mnList)
{
    // The commitment has already been validated at this point, so it's safe to use members of it
    auto members = llmq::CLLMQUtils::GetAllQuorumMembers(pQuorumBaseBlockIndex);

    for (size_t i = 0; i < members.size(); i++) {
        if (!mnList.HasMN(members[i]->proTxHash)) {
            continue;
        }
        if (!qc.validMembers[i]) {
            // punish MN for failed DKG participation
            // The idea is to immediately ban a MN when it fails 2 DKG sessions with only a few blocks in-between
            // If there were enough blocks between failures, the MN has a chance to recover as he reduces his penalty by 1 every block
            // If it however fails 3 times in the timespan of a single payment cycle, it should definitely get banned
            mnList.PoSePunish(members[i]->proTxHash, mnList.CalcPenalty(66));
        }
    }
}

void CDeterministicMNManager::DecreasePoSePenalties(CDeterministicMNList& mnList, const std::vector<CDeterministicMNCPtr> &toDecrease)
{
    for (const CDeterministicMNCPtr& dmnPtr : toDecrease) {
        mnList.PoSeDecrease(*dmnPtr);
    }
}

const CDeterministicMNList CDeterministicMNManager::GetListForBlockInternal(const CBlockIndex* pindex)
{
    CDeterministicMNList snapshot;
    const auto& consensusParams = Params().GetConsensus();
    bool fDIP0003Active = pindex->nHeight >= consensusParams.DIP0003Height;
    if (!fDIP0003Active) {
        return snapshot;
    }
    if (!m_evoDb->ReadCache(pindex->GetBlockHash(), snapshot)) {
        snapshot = CDeterministicMNList(pindex->GetBlockHash(), pindex->nHeight, 0);
        m_evoDb->WriteCache(pindex->GetBlockHash(), snapshot);
        LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- initial snapshot. blockHash=%s nHeight=%d\n", __func__,
                    snapshot.GetBlockHash().ToString(), snapshot.GetHeight());
        return snapshot;
    }
    assert(snapshot.GetHeight() != -1);
    return snapshot;
}
const CDeterministicMNList CDeterministicMNManager::GetListForBlock(const CBlockIndex* pindex) {
    return GetListForBlockInternal(pindex);
};
const CDeterministicMNList CDeterministicMNManager::GetListAtChainTip()
{
    const CBlockIndex* pindex;
    {
        LOCK(cs);
        pindex = tipIndex;
    }
    if (!pindex) {
        return CDeterministicMNList();
    }
    return GetListForBlockInternal(pindex);
}

void CDeterministicMNManager::UpdatedBlockTip(const CBlockIndex* pindex) {
    WITH_LOCK(cs, tipIndex = pindex;);
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

bool CDeterministicMNManager::DoMaintenance(bool bForceFlush) {
    LOCK(m_evoDb->cs);
    if (m_evoDb->IsCacheFull()) {
        m_evoDb->ResetDB();
        bForceFlush = true;
        LogPrint(BCLog::SYS, "CDeterministicMNManager::DoMaintenance Database successfully wiped and recreated.\n");
    } else if (bForceFlush) {
        const size_t cache_entry_count{m_evoDb->GetReadWriteCacheSize()};
        const size_t erase_entry_count{m_evoDb->GetEraseCacheSize()};

        // Shutdown can call the forced flush path twice. If the first pass already
        // persisted all dirty EvoDB state, avoid rescanning and rewriting the
        // persisted snapshot window again on the second pass.
        if (cache_entry_count == 0 && erase_entry_count == 0) {
            return true;
        }

        EvoEraseSet skipped_persisted;
        skipped_persisted.reserve(cache_entry_count + erase_entry_count);
        m_evoDb->ForEachEraseEntry([&](const uint256& key) {
            skipped_persisted.insert(key);
        });
        m_evoDb->ForEachCachedEntry([&](const uint256& key, const CDeterministicMNList&) {
            skipped_persisted.insert(key);
        });

        std::vector<EvoSnapshotEntry> live_snapshots;
        live_snapshots.reserve(LIST_CACHE_SIZE);

        size_t total_live_snapshots{0};
        if (!CollectNewestPersistedSnapshots(*m_evoDb, skipped_persisted, live_snapshots, total_live_snapshots)) {
            return false;
        }
        m_evoDb->ForEachCachedEntry([&](const uint256& key, const CDeterministicMNList& snapshot) {
            RetainNewestSnapshot(live_snapshots, total_live_snapshots, std::make_pair(key, snapshot));
        });

        if (total_live_snapshots > LIST_CACHE_SIZE) {
            std::sort(live_snapshots.begin(), live_snapshots.end(), SnapshotEntryLess);

            if (!RewriteSnapshotWindow(*m_evoDb, live_snapshots)) {
                return false;
            }

            LogPrint(BCLog::SYS,
                     "CDeterministicMNManager::%s compacted EvoDB to %zu newest snapshots before forced flush.\n",
                     __func__, live_snapshots.size());
            return true;
        }
    }
    if(bForceFlush) {
        if(!m_evoDb->FlushCacheToDisk()) {
            return false;
        }
    }
    return true;
}
bool CDeterministicMNManager::FlushCacheToDisk(bool bForceFlush) {
    return DoMaintenance(bForceFlush);
}
bool CDeterministicMNManager::GetEvoDBStats(EvoDBStats& stats)
{
    if (!m_evoDb) {
        LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- EvoDB not initialized.\n", __func__);
        stats = {}; // Clear stats
        return false;
    }

    try {
        // Get DB path from parameters used to initialize CEvoDB
        stats.dbPath = fs::PathToString(m_evoDb->GetDBParams().path);
        stats.cacheEntries = m_evoDb->GetReadWriteCacheSize();
        stats.eraseCacheEntries = m_evoDb->GetEraseCacheSize();
        stats.approxPersistedEntries = m_evoDb->CountPersistedEntries(); 

        // Calculate disk size by iterating directory
        stats.estimatedDiskSizeBytes = 0; // Initialize size
        if (!stats.dbPath.empty() && fs::is_directory(stats.dbPath)) {
            try { // Add inner try-catch for filesystem iteration errors
                for (const auto& dir_entry : fs::recursive_directory_iterator(stats.dbPath)) {
                    if (fs::is_regular_file(dir_entry.path())) {
                        std::error_code ec;
                        uint64_t fileSize = fs::file_size(dir_entry.path(), ec);
                        if (ec) {
                            LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- Error getting file size for %s: %s\n", __func__, fs::PathToString(dir_entry.path()), ec.message());
                            // Optionally continue or return false depending on desired strictness
                        } else {
                            stats.estimatedDiskSizeBytes += fileSize;
                        }
                    }
                }
            } catch (const fs::filesystem_error& e) {
                 LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- Filesystem error while iterating %s: %s\n", __func__, stats.dbPath, e.what());
                 // Can't reliably estimate size, maybe return false or keep size 0
                 return false; // Indicate failure if iteration fails
            }
        } else if (!stats.dbPath.empty()) {
            LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- DB path '%s' is not a valid directory.\n", __func__, stats.dbPath);
             // Path specified but not a directory, size is effectively 0, but maybe log warning.
        } else {
            LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- DB path is empty.\n", __func__);
        }

        return true;

    } catch (const std::exception& e) {
        LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- Exception: %s\n", __func__, e.what());
        stats = {}; // Clear stats on error
        return false;
    } catch (...) {
        LogPrint(BCLog::MNLIST, "CDeterministicMNManager::%s -- Unknown exception.\n", __func__);
        stats = {}; // Clear stats on error
        return false;
    }
}
