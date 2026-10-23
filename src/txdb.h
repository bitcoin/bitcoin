// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXDB_H
#define BITCOIN_TXDB_H

#include <coins.h>
#include <dbwrapper.h>
#include <kernel/caches.h>
#include <kernel/cs_main.h>
#include <streams.h>
#include <sync.h>
#include <util/check.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

class Coin;
class COutPoint;
class uint256;

//! User-controlled performance and debug options.
struct CoinsViewOptions {
    //! Maximum database write batch size in bytes.
    uint64_t batch_write_bytes{DEFAULT_DB_CACHE_BATCH};
    //! If non-zero, randomly exit when the database is flushed with (1/ratio) probability.
    int simulate_crash_ratio{0};
};

/** Writes already-serialized coin records into a coin database, without staging
 * in a CoinsView cache. */
class RawCoinDataDBWriter
{
public:
    explicit RawCoinDataDBWriter(CDBWrapper& db, uint64_t flush_size);

    /** Store one batch, flushing to leveldb as the flush size is reached. */
    void Write(std::span<const std::byte> batch);

    /** Write any pending records to leveldb */
    void Flush();

    /** Store the best block. Call this only after every coin is written; a database
     * without the marker is known to be incomplete. */
    void WriteBestBlock(const uint256& block_hash);

    /** Builds serialized Coin batches up to a fixed limit, compatible with Write */
    class Batch
    {
    public:
        explicit Batch(size_t limit) : m_limit{limit} { m_bytes.reserve(limit); }

        /** Append one coin to the batch. If it doesn't fit, return a
         * batch for writing, otherwise an empty vector. An empty batch will
         * always take the coin, even if it overflows the limit. */
        [[nodiscard]] std::vector<std::byte> Add(const COutPoint& outpoint, const Coin& coin);

        /** Hand off the bytes written so far and start a new batch. */
        [[nodiscard]] std::vector<std::byte> Take();

        bool empty() const { return m_bytes.empty(); }

    private:
        size_t m_limit;
        std::vector<std::byte> m_bytes;
        DataStream m_key;
        DataStream m_value;
    };

private:
    CDBWrapper& m_db;
    CDBBatch m_batch;
    uint64_t m_flush_size;
};

/** CCoinsView backed by the coin database (chainstate/) */
class CCoinsViewDB final : public CCoinsView
{
protected:
    DBParams m_db_params;
    CoinsViewOptions m_options;
    //! Prevents CompactFull() from using m_db while ResizeCache() replaces it.
    Mutex m_db_mutex;
    std::unique_ptr<CDBWrapper> m_db;
    std::shared_future<void> m_compaction;
public:
    explicit CCoinsViewDB(DBParams db_params, CoinsViewOptions options);
    ~CCoinsViewDB() override;

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override;
    std::optional<Coin> PeekCoin(const COutPoint& outpoint) const override;
    bool HaveCoin(const COutPoint& outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    void BatchWrite(CoinsViewCacheCursor& cursor, const uint256& block_hash) override;
    //! Get a cursor to iterate over the whole state.
    std::unique_ptr<CCoinsViewCursor> Cursor() const;

    //! Whether an unsupported database format is used.
    bool NeedsUpgrade();
    size_t EstimateSize() const override;

    //! Dynamically alter the underlying leveldb cache size.
    void ResizeCache(size_t new_cache_size) EXCLUSIVE_LOCKS_REQUIRED(cs_main, !m_db_mutex);

    //! Perform a full compaction of the underlying LevelDB on a one-shot background thread.
    std::shared_future<void> CompactFullAsync() EXCLUSIVE_LOCKS_REQUIRED(cs_main, !m_db_mutex);

    //! Return an underlying LevelDB property value, if available.
    std::optional<std::string> GetDBProperty(const std::string& property);

    //! Access the underlying database; for testing/in-memory databases only.
    CDBWrapper& GetDB() { Assert(m_db_params.memory_only); return *m_db; }
};

#endif // BITCOIN_TXDB_H
