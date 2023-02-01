// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_BDB_H
#define SYSCOIN_WALLET_BDB_H

#include <clientversion.h>
#include <fs.h>
#include <serialize.h>
#include <streams.h>
#include <util/system.h>
#include <wallet/db.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif
#include <db_cxx.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

struct bilingual_str;

namespace wallet {

struct WalletDatabaseFileId {
    uint8_t value[DB_FILE_ID_LEN];
    bool operator==(const WalletDatabaseFileId& rhs) const;
};

class BerkeleyDatabase;

class BerkeleyEnvironment
{
private:
    bool fDbEnvInit;
    bool fMockDb;
    // Don't change into fs::path, as that can result in
    // shutdown problems/crashes caused by a static initialized internal pointer.
    std::string strPath;

public:
    std::unique_ptr<DbEnv> dbenv;
    std::map<fs::path, std::reference_wrapper<BerkeleyDatabase>> m_databases;
    std::unordered_map<std::string, WalletDatabaseFileId> m_fileids;
    std::condition_variable_any m_db_in_use;
    bool m_use_shared_memory;

    explicit BerkeleyEnvironment(const fs::path& env_directory, bool use_shared_memory);
    BerkeleyEnvironment();
    ~BerkeleyEnvironment();
    void Reset();

    bool IsMock() const { return fMockDb; }
    bool IsInitialized() const { return fDbEnvInit; }
    fs::path Directory() const { return fs::PathFromString(strPath); }

    bool Open(bilingual_str& error);
    void Close();
    void Flush(bool fShutdown);
    void CheckpointLSN(const std::string& strFile);

    void CloseDb(const fs::path& filename);
    void ReloadDbEnv();

    DbTxn* TxnBegin(int flags = DB_TXN_WRITE_NOSYNC)
    {
        DbTxn* ptxn = nullptr;
        int ret = dbenv->txn_begin(nullptr, &ptxn, flags);
        if (!ptxn || ret != 0)
            return nullptr;
        return ptxn;
    }
};

/** Get BerkeleyEnvironment given a directory path. */
std::shared_ptr<BerkeleyEnvironment> GetBerkeleyEnv(const fs::path& env_directory, bool use_shared_memory);

class BerkeleyBatch;

/** An instance of this class represents one database.
 * For BerkeleyDB this is just a (env, strFile) tuple.
 **/
class BerkeleyDatabase : public WalletDatabase
{
public:
    BerkeleyDatabase() = delete;

    /** Create DB handle to real database */
    BerkeleyDatabase(std::shared_ptr<BerkeleyEnvironment> env, fs::path filename, const DatabaseOptions& options) :
        WalletDatabase(), env(std::move(env)), m_filename(std::move(filename)), m_max_log_mb(options.max_log_mb)
    {
        auto inserted = this->env->m_databases.emplace(m_filename, std::ref(*this));
        assert(inserted.second);
    }

    ~BerkeleyDatabase() override;

    /** Open the database if it is not already opened. */
    void Open() override;

    /** Rewrite the entire database on disk, with the exception of key pszSkip if non-zero
     */
    bool Rewrite(const char* pszSkip=nullptr) override;

    /** Indicate that a new database user has begun using the database. */
    void AddRef() override;
    /** Indicate that database user has stopped using the database and that it could be flushed or closed. */
    void RemoveRef() override;

    /** Back up the entire database to a file.
     */
    bool Backup(const std::string& strDest) const override;

    /** Make sure all changes are flushed to database file.
     */
    void Flush() override;
    /** Flush to the database file and close the database.
     *  Also close the environment if no other databases are open in it.
     */
    void Close() override;
    /* flush the wallet passively (TRY_LOCK)
       ideal to be called periodically */
    bool PeriodicFlush() override;

    void IncrementUpdateCounter() override;

    void ReloadDbEnv() override;

    /** Verifies the environment and database file */
    bool Verify(bilingual_str& error);

    /** Return path to main database filename */
    std::string Filename() override { return fs::PathToString(env->Directory() / m_filename); }

    std::string Format() override { return "bdb"; }
    /**
     * Pointer to shared database environment.
     *
     * Normally there is only one BerkeleyDatabase object per
     * BerkeleyEnvivonment, but in the special, backwards compatible case where
     * multiple wallet BDB data files are loaded from the same directory, this
     * will point to a shared instance that gets freed when the last data file
     * is closed.
     */
    std::shared_ptr<BerkeleyEnvironment> env;

    /** Database pointer. This is initialized lazily and reset during flushes, so it can be null. */
    std::unique_ptr<Db> m_db;

    fs::path m_filename;
    int64_t m_max_log_mb;

    /** Make a BerkeleyBatch connected to this database */
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override;
};

/** RAII class that automatically cleanses its data on destruction */
class SafeDbt final
{
    Dbt m_dbt;

public:
    // construct Dbt with internally-managed data
    SafeDbt();
    // construct Dbt with provided data
    SafeDbt(void* data, size_t size);
    ~SafeDbt();

    // delegate to Dbt
    const void* get_data() const;
    uint32_t get_size() const;

    // conversion operator to access the underlying Dbt
    operator Dbt*();
};

class BerkeleyCursor : public DatabaseCursor
{
private:
    Dbc* m_cursor;

public:
    explicit BerkeleyCursor(BerkeleyDatabase& database);
    ~BerkeleyCursor() override;

    Status Next(DataStream& key, DataStream& value) override;
};

/** RAII class that provides access to a Berkeley database */
class BerkeleyBatch : public DatabaseBatch
{
private:
    bool ReadKey(DataStream&& key, DataStream& value) override;
    bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite = true) override;
    bool EraseKey(DataStream&& key) override;
    bool HasKey(DataStream&& key) override;

protected:
    Db* pdb{nullptr};
    std::string strFile;
    DbTxn* activeTxn{nullptr};
    bool fReadOnly;
    bool fFlushOnClose;
    BerkeleyEnvironment *env;
    BerkeleyDatabase& m_database;

public:
    explicit BerkeleyBatch(BerkeleyDatabase& database, const bool fReadOnly, bool fFlushOnCloseIn=true);
    ~BerkeleyBatch() override;

    BerkeleyBatch(const BerkeleyBatch&) = delete;
    BerkeleyBatch& operator=(const BerkeleyBatch&) = delete;

    void Flush() override;
    void Close() override;

    std::unique_ptr<DatabaseCursor> GetNewCursor() override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    bool TxnAbort() override;
};

std::string BerkeleyDatabaseVersion();

/** Perform sanity check of runtime BDB version versus linked BDB version.
 */
bool BerkeleyDatabaseSanityCheck();

//! Return object giving access to Berkeley database at specified path.
std::unique_ptr<BerkeleyDatabase> MakeBerkeleyDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);
} // namespace wallet

#endif // SYSCOIN_WALLET_BDB_H
