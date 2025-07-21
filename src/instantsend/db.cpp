// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <instantsend/db.h>

#include <chain.h>
#include <dbwrapper.h>
#include <primitives/block.h>
#include <util/system.h>

static constexpr std::string_view DB_ARCHIVED_BY_HASH{"is_a2"};
static constexpr std::string_view DB_ARCHIVED_BY_HEIGHT_AND_HASH{"is_a1"};
static constexpr std::string_view DB_HASH_BY_OUTPOINT{"is_in"};
static constexpr std::string_view DB_HASH_BY_TXID{"is_tx"};
static constexpr std::string_view DB_ISLOCK_BY_HASH{"is_i"};
static constexpr std::string_view DB_MINED_BY_HEIGHT_AND_HASH{"is_m"};
static constexpr std::string_view DB_VERSION{"is_v"};

namespace instantsend {
namespace {
static std::tuple<std::string, uint32_t, uint256> BuildInversedISLockKey(std::string_view k, int nHeight,
                                                                         const uint256& islockHash)
{
    return std::make_tuple(std::string{k}, htobe32_internal(std::numeric_limits<uint32_t>::max() - nHeight), islockHash);
}
} // anonymous namespace

CInstantSendDb::CInstantSendDb(bool unitTests, bool fWipe) :
    db(std::make_unique<CDBWrapper>(unitTests ? "" : (gArgs.GetDataDirNet() / "llmq/isdb"), 32 << 20, unitTests, fWipe))
{
    Upgrade(unitTests);
}

CInstantSendDb::~CInstantSendDb() = default;

void CInstantSendDb::Upgrade(bool unitTests)
{
    LOCK(cs_db);
    int v{0};
    if (!db->Read(DB_VERSION, v) || v < CInstantSendDb::CURRENT_VERSION) {
        // Wipe db
        db.reset();
        db = std::make_unique<CDBWrapper>(unitTests ? "" : (gArgs.GetDataDirNet() / "llmq/isdb"), 32 << 20, unitTests,
                                          /*fWipe=*/true);
        CDBBatch batch(*db);
        batch.Write(DB_VERSION, CInstantSendDb::CURRENT_VERSION);
        // Sync DB changes to disk
        db->WriteBatch(batch, /*fSync=*/true);
        batch.Clear();
    }
}

void CInstantSendDb::WriteNewInstantSendLock(const uint256& hash, const InstantSendLock& islock)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    batch.Write(std::make_tuple(DB_ISLOCK_BY_HASH, hash), islock);
    batch.Write(std::make_tuple(DB_HASH_BY_TXID, islock.txid), hash);
    for (const auto& in : islock.inputs) {
        batch.Write(std::make_tuple(DB_HASH_BY_OUTPOINT, in), hash);
    }
    db->WriteBatch(batch);

    islockCache.insert(hash, std::make_shared<InstantSendLock>(islock));
    txidCache.insert(islock.txid, hash);
    for (const auto& in : islock.inputs) {
        outpointCache.insert(in, hash);
    }
}

void CInstantSendDb::RemoveInstantSendLock(CDBBatch& batch, const uint256& hash, InstantSendLockPtr islock, bool keep_cache)
{
    AssertLockHeld(cs_db);
    if (!islock) {
        islock = GetInstantSendLockByHashInternal(hash, false);
        if (!islock) {
            return;
        }
    }

    batch.Erase(std::make_tuple(DB_ISLOCK_BY_HASH, hash));
    batch.Erase(std::make_tuple(DB_HASH_BY_TXID, islock->txid));
    for (auto& in : islock->inputs) {
        batch.Erase(std::make_tuple(DB_HASH_BY_OUTPOINT, in));
    }

    if (!keep_cache) {
        islockCache.erase(hash);
        txidCache.erase(islock->txid);
        for (const auto& in : islock->inputs) {
            outpointCache.erase(in);
        }
    }
}

void CInstantSendDb::WriteInstantSendLockMined(const uint256& hash, int nHeight)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    WriteInstantSendLockMined(batch, hash, nHeight);
    db->WriteBatch(batch);
}

void CInstantSendDb::WriteInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight)
{
    AssertLockHeld(cs_db);
    batch.Write(BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nHeight, hash), true);
}

void CInstantSendDb::RemoveInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight)
{
    AssertLockHeld(cs_db);
    batch.Erase(BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nHeight, hash));
}

void CInstantSendDb::WriteInstantSendLockArchived(CDBBatch& batch, const uint256& hash, int nHeight)
{
    AssertLockHeld(cs_db);
    batch.Write(BuildInversedISLockKey(DB_ARCHIVED_BY_HEIGHT_AND_HASH, nHeight, hash), true);
    batch.Write(std::make_tuple(DB_ARCHIVED_BY_HASH, hash), true);
}

std::unordered_map<uint256, InstantSendLockPtr, StaticSaltedHasher> CInstantSendDb::RemoveConfirmedInstantSendLocks(int nUntilHeight)
{
    LOCK(cs_db);
    if (nUntilHeight <= best_confirmed_height) {
        LogPrint(BCLog::ALL, "CInstantSendDb::%s -- Attempting to confirm height %d, however we've already confirmed height %d. This should never happen.\n", __func__,
                 nUntilHeight, best_confirmed_height);
        return {};
    }
    best_confirmed_height = nUntilHeight;

    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());

    auto firstKey = BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nUntilHeight, uint256());

    it->Seek(firstKey);

    CDBBatch batch(*db);
    std::unordered_map<uint256, InstantSendLockPtr, StaticSaltedHasher> ret;
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_MINED_BY_HEIGHT_AND_HASH) {
            break;
        }
        uint32_t nHeight = std::numeric_limits<uint32_t>::max() - be32toh_internal(std::get<1>(curKey));
        if (nHeight > uint32_t(nUntilHeight)) {
            break;
        }

        auto& islockHash = std::get<2>(curKey);

        if (auto islock = GetInstantSendLockByHashInternal(islockHash, false)) {
            RemoveInstantSendLock(batch, islockHash, islock);
            ret.try_emplace(islockHash, std::move(islock));
        }

        // archive the islock hash, so that we're still able to check if we've seen the islock in the past
        WriteInstantSendLockArchived(batch, islockHash, nHeight);

        batch.Erase(curKey);

        it->Next();
    }

    db->WriteBatch(batch);

    return ret;
}

void CInstantSendDb::RemoveArchivedInstantSendLocks(int nUntilHeight)
{
    LOCK(cs_db);
    if (nUntilHeight <= 0) {
        return;
    }

    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());

    auto firstKey = BuildInversedISLockKey(DB_ARCHIVED_BY_HEIGHT_AND_HASH, nUntilHeight, uint256());

    it->Seek(firstKey);

    CDBBatch batch(*db);
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_ARCHIVED_BY_HEIGHT_AND_HASH) {
            break;
        }
        uint32_t nHeight = std::numeric_limits<uint32_t>::max() - be32toh_internal(std::get<1>(curKey));
        if (nHeight > uint32_t(nUntilHeight)) {
            break;
        }

        auto& islockHash = std::get<2>(curKey);
        batch.Erase(std::make_tuple(DB_ARCHIVED_BY_HASH, islockHash));
        batch.Erase(curKey);

        it->Next();
    }

    db->WriteBatch(batch);
}

void CInstantSendDb::WriteBlockInstantSendLocks(const gsl::not_null<std::shared_ptr<const CBlock>>& pblock,
                                                gsl::not_null<const CBlockIndex*> pindexConnected)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            // coinbase and TXs with no inputs can't be locked
            continue;
        }
        uint256 islockHash = GetInstantSendLockHashByTxidInternal(tx->GetHash());
        // update DB about when an IS lock was mined
        if (!islockHash.IsNull()) {
            WriteInstantSendLockMined(batch, islockHash, pindexConnected->nHeight);
        }
    }
    db->WriteBatch(batch);
}

void CInstantSendDb::RemoveBlockInstantSendLocks(const gsl::not_null<std::shared_ptr<const CBlock>>& pblock,
                                                 gsl::not_null<const CBlockIndex*> pindexDisconnected)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            // coinbase and TXs with no inputs can't be locked
            continue;
        }
        uint256 islockHash = GetInstantSendLockHashByTxidInternal(tx->GetHash());
        if (!islockHash.IsNull()) {
            RemoveInstantSendLockMined(batch, islockHash, pindexDisconnected->nHeight);
        }
    }
    db->WriteBatch(batch);
}

bool CInstantSendDb::KnownInstantSendLock(const uint256& islockHash) const
{
    LOCK(cs_db);
    return GetInstantSendLockByHashInternal(islockHash) != nullptr ||
           db->Exists(std::make_tuple(DB_ARCHIVED_BY_HASH, islockHash));
}

size_t CInstantSendDb::GetInstantSendLockCount() const
{
    LOCK(cs_db);
    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());
    auto firstKey = std::make_tuple(std::string{DB_ISLOCK_BY_HASH}, uint256());

    it->Seek(firstKey);

    size_t cnt = 0;
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_ISLOCK_BY_HASH) {
            break;
        }

        cnt++;

        it->Next();
    }

    return cnt;
}

InstantSendLockPtr CInstantSendDb::GetInstantSendLockByHashInternal(const uint256& hash, bool use_cache) const
{
    AssertLockHeld(cs_db);
    if (hash.IsNull()) {
        return nullptr;
    }

    InstantSendLockPtr ret;
    if (use_cache && islockCache.get(hash, ret)) {
        return ret;
    }

    ret = std::make_shared<InstantSendLock>();
    bool exists = db->Read(std::make_tuple(DB_ISLOCK_BY_HASH, hash), *ret);
    if (!exists || (::SerializeHash(*ret) != hash)) {
        ret = std::make_shared<InstantSendLock>();
        exists = db->Read(std::make_tuple(DB_ISLOCK_BY_HASH, hash), *ret);
        if (!exists || (::SerializeHash(*ret) != hash)) {
            ret = nullptr;
        }
    }
    islockCache.insert(hash, ret);
    return ret;
}

uint256 CInstantSendDb::GetInstantSendLockHashByTxidInternal(const uint256& txid) const
{
    AssertLockHeld(cs_db);
    uint256 islockHash;
    if (!txidCache.get(txid, islockHash)) {
        if (!db->Read(std::make_tuple(DB_HASH_BY_TXID, txid), islockHash)) {
            return {};
        }
        txidCache.insert(txid, islockHash);
    }
    return islockHash;
}

InstantSendLockPtr CInstantSendDb::GetInstantSendLockByTxid(const uint256& txid) const
{
    LOCK(cs_db);
    return GetInstantSendLockByHashInternal(GetInstantSendLockHashByTxidInternal(txid));
}

InstantSendLockPtr CInstantSendDb::GetInstantSendLockByInput(const COutPoint& outpoint) const
{
    LOCK(cs_db);
    uint256 islockHash;
    if (!outpointCache.get(outpoint, islockHash)) {
        if (!db->Read(std::make_tuple(DB_HASH_BY_OUTPOINT, outpoint), islockHash)) {
            return nullptr;
        }
        outpointCache.insert(outpoint, islockHash);
    }
    return GetInstantSendLockByHashInternal(islockHash);
}

std::vector<uint256> CInstantSendDb::GetInstantSendLocksByParent(const uint256& parent) const
{
    AssertLockHeld(cs_db);
    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());
    auto firstKey = std::make_tuple(std::string{DB_HASH_BY_OUTPOINT}, COutPoint(parent, 0));
    it->Seek(firstKey);

    std::vector<uint256> result;

    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_HASH_BY_OUTPOINT) {
            break;
        }
        const auto& outpoint = std::get<1>(curKey);
        if (outpoint.hash != parent) {
            break;
        }

        uint256 islockHash;
        if (!it->GetValue(islockHash)) {
            break;
        }
        result.emplace_back(islockHash);
        it->Next();
    }

    return result;
}

std::vector<uint256> CInstantSendDb::RemoveChainedInstantSendLocks(const uint256& islockHash, const uint256& txid,
                                                                   int nHeight)
{
    LOCK(cs_db);
    std::vector<uint256> result;

    std::vector<uint256> stack;
    std::unordered_set<uint256, StaticSaltedHasher> added;
    stack.emplace_back(txid);

    CDBBatch batch(*db);
    while (!stack.empty()) {
        auto children = GetInstantSendLocksByParent(stack.back());
        stack.pop_back();

        for (auto& childIslockHash : children) {
            auto childIsLock = GetInstantSendLockByHashInternal(childIslockHash, false);
            if (!childIsLock) {
                continue;
            }

            RemoveInstantSendLock(batch, childIslockHash, childIsLock, false);
            WriteInstantSendLockArchived(batch, childIslockHash, nHeight);
            result.emplace_back(childIslockHash);

            if (added.emplace(childIsLock->txid).second) {
                stack.emplace_back(childIsLock->txid);
            }
        }
    }

    RemoveInstantSendLock(batch, islockHash, nullptr, false);
    WriteInstantSendLockArchived(batch, islockHash, nHeight);
    result.emplace_back(islockHash);

    db->WriteBatch(batch);

    return result;
}
} // namespace instantsend
