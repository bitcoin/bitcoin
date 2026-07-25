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
#include <sync.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class COutPoint;
class uint256;

//! User-controlled performance and debug options.
struct CoinsViewOptions {
    //! Maximum database write batch size in bytes.
    uint64_t batch_write_bytes{DEFAULT_DB_CACHE_BATCH};
    //! If non-zero, randomly exit when the database is flushed with (1/ratio) probability.
    int simulate_crash_ratio{0};
};

/** CCoinsView backed by the coin database (chainstate/) */
class CCoinsViewDB final : public CCoinsView
{
    friend struct CoinsViewDBTestAccess;

protected:
    DBParams m_db_params;
    CoinsViewOptions m_options;
    mutable SharedMutex m_db_mutex; //!< Shared by cursors and compaction, exclusive for resize
    std::unique_ptr<CDBWrapper> m_db;
    std::shared_future<void> m_compaction GUARDED_BY(::cs_main); //!< Destructor has exclusive access
public:
    explicit CCoinsViewDB(DBParams db_params, CoinsViewOptions options);
    ~CCoinsViewDB() override;

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override;
    std::optional<Coin> PeekCoin(const COutPoint& outpoint) const override;
    bool HaveCoin(const COutPoint& outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    void BatchWrite(CoinsViewCacheCursor& cursor, const uint256& block_hash) override;
    //! A cursor must not outlive its DB or leave its creating thread. That thread cannot lock cs_main or open another cursor for this DB.
    std::unique_ptr<CCoinsViewCursor> Cursor() const EXCLUSIVE_LOCKS_REQUIRED(!m_db_mutex);

    //! Whether an unsupported database format is used.
    bool NeedsUpgrade();
    size_t EstimateSize() const override;

    //! Resize the LevelDB cache after live cursors and compaction finish.
    void ResizeCache(size_t new_cache_size) EXCLUSIVE_LOCKS_REQUIRED(cs_main, !m_db_mutex);

    //! Perform a full compaction of the underlying LevelDB on a one-shot background thread.
    std::shared_future<void> CompactFullAsync() EXCLUSIVE_LOCKS_REQUIRED(cs_main, !m_db_mutex);

    //! Return an underlying LevelDB property value, if available.
    std::optional<std::string> GetDBProperty(const std::string& property);
};

#endif // BITCOIN_TXDB_H
