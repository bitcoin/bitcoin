// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>

#include <coins.h>
#include <dbwrapper.h>
#include <logging/timer.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/log.h>
#include <util/threadnames.h>
#include <util/vector.h>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <future>
#include <iterator>
#include <utility>

static constexpr uint8_t DB_COIN{'C'};
static constexpr uint8_t DB_BEST_BLOCK{'B'};
static constexpr uint8_t DB_HEAD_BLOCKS{'H'};
// Keys used in previous version that might still be found in the DB:
static constexpr uint8_t DB_COINS{'c'};

// Threshold for warning when writing this many dirty cache entries to disk.
static constexpr size_t WARN_FLUSH_COINS_COUNT{10'000'000};

bool CCoinsViewDB::NeedsUpgrade()
{
    std::unique_ptr<CDBIterator> cursor{m_db->NewIterator()};
    // DB_COINS was deprecated in v0.15.0, commit
    // 1088b02f0ccd7358d2b7076bb9e122d59d502d02
    cursor->Seek(std::make_pair(DB_COINS, uint256{}));
    return cursor->Valid();
}

namespace {

struct CoinEntry {
    COutPoint* outpoint;
    uint8_t key{DB_COIN};
    explicit CoinEntry(const COutPoint* ptr) : outpoint(const_cast<COutPoint*>(ptr)) {}

    SERIALIZE_METHODS(CoinEntry, obj) { READWRITE(obj.key, obj.outpoint->hash, VARINT(obj.outpoint->n)); }
};

} // namespace

std::vector<std::byte> RawCoinDataDBWriter::Batch::Take()
{
    std::vector<std::byte> out{std::move(m_bytes)};
    m_bytes.clear();
    m_bytes.reserve(m_limit);
    return out;
}

std::vector<std::byte> RawCoinDataDBWriter::Batch::Add(const COutPoint& outpoint, const Coin& coin)
{
    std::vector<std::byte> to_send;

    m_key.clear();
    m_value.clear();
    m_key << CoinEntry(&outpoint);
    m_value << coin;

    const size_t add{sizeof(uint32_t) * 2 + m_key.size() + m_value.size()};
    if (!m_bytes.empty() && m_bytes.size() + add > m_limit) to_send = Take();

    const auto append = [this](std::span<const std::byte> bytes) {
        m_bytes.insert(m_bytes.end(), bytes.begin(), bytes.end());
    };
    const uint32_t key_size{static_cast<uint32_t>(m_key.size())};
    const uint32_t value_size{static_cast<uint32_t>(m_value.size())};
    append(std::as_bytes(std::span{&key_size, 1}));
    append(std::span<const std::byte>{m_key.data(), m_key.size()});
    append(std::as_bytes(std::span{&value_size, 1}));
    append(std::span<const std::byte>{m_value.data(), m_value.size()});
    return to_send;
}

RawCoinDataDBWriter::RawCoinDataDBWriter(CDBWrapper& db, uint64_t flush_size) :
    m_db{db},
    m_batch{db},
    m_flush_size{flush_size} { }

void RawCoinDataDBWriter::Write(std::span<const std::byte> batch)
{
    std::span<const std::byte> in{batch};
    while (!in.empty()) {
        uint32_t key_size{0};
        uint32_t value_size{0};
        if (in.size() < sizeof(key_size)) throw std::runtime_error{"truncated coin data batch"};
        std::memcpy(&key_size, in.data(), sizeof(key_size));
        in = in.subspan(sizeof(key_size));
        if (in.size() < key_size + sizeof(value_size)) throw std::runtime_error{"truncated coin data batch"};
        const auto key{in.first(key_size)};
        in = in.subspan(key_size);
        std::memcpy(&value_size, in.data(), sizeof(value_size));
        in = in.subspan(sizeof(value_size));
        if (in.size() < value_size) throw std::runtime_error{"truncated coin data batch"};
        m_batch.Write(key, in.first(value_size));
        in = in.subspan(value_size);
        if (m_batch.ApproximateSize() >= m_flush_size) Flush();
    }
}

void RawCoinDataDBWriter::Flush()
{
    m_db.WriteBatch(m_batch);
    m_batch.Clear();
}

void RawCoinDataDBWriter::WriteBestBlock(const uint256& block_hash)
{
    m_db.Write(DB_BEST_BLOCK, block_hash);
}

CCoinsViewDB::CCoinsViewDB(DBParams db_params, CoinsViewOptions options) :
    m_db_params{std::move(db_params)},
    m_options{std::move(options)},
    m_db{std::make_unique<CDBWrapper>(m_db_params)} { }

CCoinsViewDB::~CCoinsViewDB()
{
    if (m_compaction.valid()) {
        if (m_compaction.wait_for(std::chrono::seconds{0}) != std::future_status::ready) {
            LogInfo("Waiting for background chainstate compaction of %s", fs::PathToString(m_db_params.path));
        }
        m_compaction.wait();
    }
}

void CCoinsViewDB::ResizeCache(size_t new_cache_size)
{
    // We can't do this operation with an in-memory DB since we'll lose all the coins upon
    // reset.
    if (!m_db_params.memory_only) {
        LOCK(m_db_mutex);
        // Have to do a reset first to get the original `m_db` state to release its
        // filesystem lock.
        m_db.reset();
        m_db_params.cache_bytes = new_cache_size;
        m_db_params.wipe_data = false;
        m_db = std::make_unique<CDBWrapper>(m_db_params);
    }
}

std::optional<Coin> CCoinsViewDB::GetCoin(const COutPoint& outpoint) const
{
    Coin coin;
    const CDBWrapper::ReadStatus res = m_db->TryRead(CoinEntry(&outpoint), coin);
    if (!res) {
        // Propagate errors so CCoinsViewErrorCatcher triggers a clean shutdown.
        switch (const auto& [err_code, err_msg] = res.error(); err_code) {
            case CDBWrapper::ReadFailure::Code::DeserializationError:
                throw dbwrapper_error{strprintf("Coin deserialization failure: %s", err_msg)};
            case CDBWrapper::ReadFailure::Code::DatabaseError:
                throw dbwrapper_error{strprintf("Coin DB read failure: %s", err_msg)};
        } // no default case, so the compiler can warn about missing cases
        std::abort(); // unreachable
    }

    // Check whether the coin exists
    if (!res.value()) return std::nullopt;
    // Coin found, ensure UTXO database never contains spent coins
    Assert(!coin.IsSpent());
    return coin;
}

std::optional<Coin> CCoinsViewDB::PeekCoin(const COutPoint& outpoint) const
{
    return GetCoin(outpoint);
}

bool CCoinsViewDB::HaveCoin(const COutPoint& outpoint) const
{
    return m_db->Exists(CoinEntry(&outpoint));
}

uint256 CCoinsViewDB::GetBestBlock() const {
    uint256 hashBestChain;
    if (!m_db->Read(DB_BEST_BLOCK, hashBestChain))
        return uint256();
    return hashBestChain;
}

std::vector<uint256> CCoinsViewDB::GetHeadBlocks() const {
    std::vector<uint256> vhashHeadBlocks;
    if (!m_db->Read(DB_HEAD_BLOCKS, vhashHeadBlocks)) {
        return std::vector<uint256>();
    }
    return vhashHeadBlocks;
}

void CCoinsViewDB::BatchWrite(CoinsViewCacheCursor& cursor, const uint256& block_hash)
{
    CDBBatch batch(*m_db);
    size_t count = 0;
    const size_t dirty_count{cursor.GetDirtyCount()};
    assert(!block_hash.IsNull());

    uint256 old_tip = GetBestBlock();
    if (old_tip.IsNull()) {
        // We may be in the middle of replaying.
        std::vector<uint256> old_heads = GetHeadBlocks();
        if (old_heads.size() == 2) {
            if (old_heads[0] != block_hash) {
                LogError("The coins database detected an inconsistent state, likely due to a previous crash or shutdown. You will need to restart bitcoind with the -reindex-chainstate or -reindex configuration option.\n");
            }
            assert(old_heads[0] == block_hash);
            old_tip = old_heads[1];
        }
    }

    if (dirty_count > WARN_FLUSH_COINS_COUNT) LogWarning("Flushing large (%d entries) UTXO set to disk, it may take several minutes", dirty_count);
    LOG_TIME_MILLIS_WITH_CATEGORY(strprintf("write coins cache to disk (%d out of %d cached coins)",
        dirty_count, cursor.GetTotalCount()), BCLog::BENCH);

    // In the first batch, mark the database as being in the middle of a
    // transition from old_tip to block_hash.
    // A vector is used for future extensibility, as we may want to support
    // interrupting after partial writes from multiple independent reorgs.
    batch.Erase(DB_BEST_BLOCK);
    batch.Write(DB_HEAD_BLOCKS, Vector(block_hash, old_tip));

    for (auto it{cursor.Begin()}; it != cursor.End();) {
        if (it->second.IsDirty()) {
            CoinEntry entry(&it->first);
            if (it->second.coin.IsSpent()) {
                batch.Erase(entry);
            } else {
                batch.Write(entry, it->second.coin);
            }
        }
        count++;
        it = cursor.NextAndMaybeErase(*it);
        if (batch.ApproximateSize() > m_options.batch_write_bytes) {
            LogDebug(BCLog::COINDB, "Writing partial batch of %.2f MiB\n", batch.ApproximateSize() / double(1_MiB));

            m_db->WriteBatch(batch);
            batch.Clear();
            if (m_options.simulate_crash_ratio) {
                static FastRandomContext rng;
                if (rng.randrange(m_options.simulate_crash_ratio) == 0) {
                    LogError("Simulating a crash. Goodbye.");
                    _Exit(0);
                }
            }
        }
    }

    // In the last batch, mark the database as consistent with block_hash again.
    batch.Erase(DB_HEAD_BLOCKS);
    batch.Write(DB_BEST_BLOCK, block_hash);

    LogDebug(BCLog::COINDB, "Writing final batch of %.2f MiB\n", batch.ApproximateSize() / double(1_MiB));
    m_db->WriteBatch(batch);
    LogDebug(BCLog::COINDB, "Committed %u changed transaction outputs (out of %u) to coin database...", (unsigned int)dirty_count, (unsigned int)count);
}

size_t CCoinsViewDB::EstimateSize() const
{
    return m_db->EstimateSize(DB_COIN, uint8_t(DB_COIN + 1));
}

std::optional<std::string> CCoinsViewDB::GetDBProperty(const std::string& property)
{
    return m_db->GetProperty(property);
}

std::shared_future<void> CCoinsViewDB::CompactFullAsync()
{
    AssertLockHeld(::cs_main);
    if (m_compaction.valid() && m_compaction.wait_for(std::chrono::seconds{0}) != std::future_status::ready) return m_compaction;
    m_compaction = std::async(std::launch::async, [this] {
        try {
            util::ThreadRename("utxocompact");
            LOCK(m_db_mutex);

            LogDebug(BCLog::COINDB, "Starting chainstate compaction of %s", fs::PathToString(m_db_params.path));
            m_db->CompactFull();
            LogDebug(BCLog::COINDB, "Finished chainstate compaction of %s", fs::PathToString(m_db_params.path));
        } catch (const std::exception& e) {
            LogWarning("Failed chainstate compaction (%s)", e.what());
        }
    }).share();
    return m_compaction;
}

/** Specialization of CCoinsViewCursor to iterate over a CCoinsViewDB */
class CCoinsViewDBCursor: public CCoinsViewCursor
{
public:
    // Prefer using CCoinsViewDB::Cursor() since we want to perform some
    // cache warmup on instantiation.
    CCoinsViewDBCursor(CDBIterator* pcursorIn, const uint256& in_block_hash):
        CCoinsViewCursor(in_block_hash), pcursor(pcursorIn) {}
    ~CCoinsViewDBCursor() = default;

    bool GetKey(COutPoint &key) const override;
    bool GetValue(Coin &coin) const override;

    bool Valid() const override;
    void Next() override;

private:
    std::unique_ptr<CDBIterator> pcursor;
    std::pair<char, COutPoint> keyTmp;

    friend class CCoinsViewDB;
};

std::unique_ptr<CCoinsViewCursor> CCoinsViewDB::Cursor() const
{
    auto i = std::make_unique<CCoinsViewDBCursor>(
        const_cast<CDBWrapper&>(*m_db).NewIterator(), GetBestBlock());
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    i->pcursor->Seek(DB_COIN);
    // Cache key of first record
    if (i->pcursor->Valid()) {
        CoinEntry entry(&i->keyTmp.second);
        i->pcursor->GetKey(entry);
        i->keyTmp.first = entry.key;
    } else {
        i->keyTmp.first = 0; // Make sure Valid() and GetKey() return false
    }
    return i;
}

bool CCoinsViewDBCursor::GetKey(COutPoint &key) const
{
    // Return cached key
    if (keyTmp.first == DB_COIN) {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool CCoinsViewDBCursor::GetValue(Coin &coin) const
{
    return pcursor->GetValue(coin);
}

bool CCoinsViewDBCursor::Valid() const
{
    return keyTmp.first == DB_COIN;
}

void CCoinsViewDBCursor::Next()
{
    pcursor->Next();
    CoinEntry entry(&keyTmp.second);
    if (!pcursor->Valid() || !pcursor->GetKey(entry)) {
        keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
    } else {
        keyTmp.first = entry.key;
    }
}
