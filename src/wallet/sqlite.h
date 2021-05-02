// Copyright (c) 2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XBIT_WALLET_SQLITE_H
#define XBIT_WALLET_SQLITE_H

#include <wallet/db.h>

#include <sqlite3.h>

struct bilingual_str;
class SQLiteDatabase;

/** RAII class that provides access to a WalletDatabase */
class SQLiteBatch : public DatabaseBatch
{
private:
    SQLiteDatabase& m_database;

    bool m_cursor_init = false;

    sqlite3_stmt* m_read_stmt{nullptr};
    sqlite3_stmt* m_insert_stmt{nullptr};
    sqlite3_stmt* m_overwrite_stmt{nullptr};
    sqlite3_stmt* m_delete_stmt{nullptr};
    sqlite3_stmt* m_cursor_stmt{nullptr};

    void SetupSQLStatements();

    bool ReadKey(CDataStream&& key, CDataStream& value) override;
    bool WriteKey(CDataStream&& key, CDataStream&& value, bool overwrite = true) override;
    bool EraseKey(CDataStream&& key) override;
    bool HasKey(CDataStream&& key) override;

public:
    explicit SQLiteBatch(SQLiteDatabase& database);
    ~SQLiteBatch() override { Close(); }

    /* No-op. See commeng on SQLiteDatabase::Flush */
    void Flush() override {}

    void Close() override;

    bool StartCursor() override;
    bool ReadAtCursor(CDataStream& key, CDataStream& value, bool& complete) override;
    void CloseCursor() override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    bool TxnAbort() override;
};

/** An instance of this class represents one SQLite3 database.
 **/
class SQLiteDatabase : public WalletDatabase
{
private:
    const bool m_mock{false};

    const std::string m_dir_path;

    const std::string m_file_path;

    void Cleanup() noexcept;

public:
    SQLiteDatabase() = delete;

    /** Create DB handle to real database */
    SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, bool mock = false);

    ~SQLiteDatabase();

    bool Verify(bilingual_str& error);

    /** Open the database if it is not already opened */
    void Open() override;

    /** Close the database */
    void Close() override;

    /* These functions are unused */
    void AddRef() override { assert(false); }
    void RemoveRef() override { assert(false); }

    /** Rewrite the entire database on disk */
    bool Rewrite(const char* skip = nullptr) override;

    /** Back up the entire database to a file.
     */
    bool Backup(const std::string& dest) const override;

    /** No-ops
     *
     * SQLite always flushes everything to the database file after each transaction
     * (each Read/Write/Erase that we do is its own transaction unless we called
     * TxnBegin) so there is no need to have Flush or Periodic Flush.
     *
     * There is no DB env to reload, so ReloadDbEnv has nothing to do
     */
    void Flush() override {}
    bool PeriodicFlush() override { return false; }
    void ReloadDbEnv() override {}

    void IncrementUpdateCounter() override { ++nUpdateCounter; }

    std::string Filename() override { return m_file_path; }
    std::string Format() override { return "sqlite"; }

    /** Make a SQLiteBatch connected to this database */
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override;

    sqlite3* m_db{nullptr};
};

bool ExistsSQLiteDatabase(const fs::path& path);
std::unique_ptr<SQLiteDatabase> MakeSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);

std::string SQLiteDatabaseVersion();
bool IsSQLiteFile(const fs::path& path);

#endif // XBIT_WALLET_SQLITE_H
