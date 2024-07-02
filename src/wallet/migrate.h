// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_MIGRATE_H
#define BITCOIN_WALLET_MIGRATE_H

#include <wallet/db.h>

#include <optional>

namespace wallet {

using BerkeleyROData = std::map<SerializeData, SerializeData, std::less<>>;

/**
 * A class representing a BerkeleyDB file from which we can only read records.
 * This is used only for migration of legacy to descriptor wallets
 */
class BerkeleyRODatabase : public WalletDatabase
{
private:
    const fs::path m_filepath;

public:
    /** Create DB handle */
    BerkeleyRODatabase(const fs::path& filepath, bool open = true) : WalletDatabase(), m_filepath(filepath)
    {
        if (open) Open();
    }
    ~BerkeleyRODatabase(){};

    BerkeleyROData m_records;

    /** Open the database if it is not already opened. */
    void Open() override;

    /** Indicate the a new database user has began using the database. Increments m_refcount */
    void AddRef() override {}
    /** Indicate that database user has stopped using the database and that it could be flushed or closed. Decrement m_refcount */
    void RemoveRef() override {}

    /** Rewrite the entire database on disk, with the exception of key pszSkip if non-zero
     */
    bool Rewrite(const char* pszSkip = nullptr) override { return false; }

    /** Back up the entire database to a file.
     */
    bool Backup(const std::string& strDest) const override;

    /** Make sure all changes are flushed to database file.
     */
    void Flush() override {}
    /** Flush to the database file and close the database.
     *  Also close the environment if no other databases are open in it.
     */
    void Close() override {}
    /* flush the wallet passively (TRY_LOCK)
       ideal to be called periodically */
    bool PeriodicFlush() override { return false; }

    void IncrementUpdateCounter() override {}

    void ReloadDbEnv() override {}

    /** Return path to main database file for logs and error messages. */
    std::string Filename() override { return fs::PathToString(m_filepath); }

    std::string Format() override { return "bdb_ro"; }

    /** Make a DatabaseBatch connected to this database */
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override;
};

class BerkeleyROCursor : public DatabaseCursor
{
private:
    const BerkeleyRODatabase& m_database;
    BerkeleyROData::const_iterator m_cursor;
    BerkeleyROData::const_iterator m_cursor_end;

public:
    explicit BerkeleyROCursor(const BerkeleyRODatabase& database, Span<const std::byte> prefix = {});
    ~BerkeleyROCursor() {}

    Status Next(DataStream& key, DataStream& value) override;
};

/** RAII class that provides access to a BerkeleyRODatabase */
class BerkeleyROBatch : public DatabaseBatch
{
private:
    const BerkeleyRODatabase& m_database;

    bool ReadKey(DataStream&& key, DataStream& value) override;
    // WriteKey returns true since various automatic upgrades for older wallets will expect writing to not fail.
    // It is okay for this batch type to not actually write anything as those automatic upgrades will occur again after migration.
    bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite = true) override { return true; }
    bool EraseKey(DataStream&& key) override { return false; }
    bool HasKey(DataStream&& key) override;
    bool ErasePrefix(Span<const std::byte> prefix) override { return false; }

public:
    explicit BerkeleyROBatch(const BerkeleyRODatabase& database) : m_database(database) {}
    ~BerkeleyROBatch() {}

    BerkeleyROBatch(const BerkeleyROBatch&) = delete;
    BerkeleyROBatch& operator=(const BerkeleyROBatch&) = delete;

    void Flush() override {}
    void Close() override {}

    std::unique_ptr<DatabaseCursor> GetNewCursor() override { return std::make_unique<BerkeleyROCursor>(m_database); }
    std::unique_ptr<DatabaseCursor> GetNewPrefixCursor(Span<const std::byte> prefix) override;
    bool TxnBegin() override { return false; }
    bool TxnCommit() override { return false; }
    bool TxnAbort() override { return false; }
    bool HasActiveTxn() override { return false; }
};

//! Return object giving access to Berkeley Read Only database at specified path.
std::unique_ptr<BerkeleyRODatabase> MakeBerkeleyRODatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);
} // namespace wallet

#endif // BITCOIN_WALLET_MIGRATE_H
