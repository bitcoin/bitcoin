// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/sqlite.h>

#include <logging.h>
#include <sync.h>
#include <util/memory.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/db.h>

#include <sqlite3.h>
#include <stdint.h>

static const char* const DATABASE_FILENAME = "wallet.dat";

static Mutex g_sqlite_mutex;
static int g_sqlite_count GUARDED_BY(g_sqlite_mutex) = 0;

static void ErrorLogCallback(void* arg, int code, const char* msg)
{
    // From sqlite3_config() documentation for the SQLITE_CONFIG_LOG option:
    // "The void pointer that is the second argument to SQLITE_CONFIG_LOG is passed through as
    // the first parameter to the application-defined logger function whenever that function is
    // invoked."
    // Assert that this is the case:
    assert(arg == nullptr);
    LogPrintf("SQLite Error. Code: %d. Message: %s\n", code, msg);
}

SQLiteDatabase::SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, bool mock)
    : WalletDatabase(), m_mock(mock), m_dir_path(dir_path.string()), m_file_path(file_path.string())
{
    {
        LOCK(g_sqlite_mutex);
        LogPrintf("Using SQLite Version %s\n", SQLiteDatabaseVersion());
        LogPrintf("Using wallet %s\n", m_dir_path);

        if (++g_sqlite_count == 1) {
            // Setup logging
            int ret = sqlite3_config(SQLITE_CONFIG_LOG, ErrorLogCallback, nullptr);
            if (ret != SQLITE_OK) {
                throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup error log: %s\n", sqlite3_errstr(ret)));
            }
        }
        int ret = sqlite3_initialize(); // This is a no-op if sqlite3 is already initialized
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to initialize SQLite: %s\n", sqlite3_errstr(ret)));
        }
    }

    try {
        Open();
    } catch (const std::runtime_error&) {
        // If open fails, cleanup this object and rethrow the exception
        Cleanup();
        throw;
    }
}

void SQLiteBatch::SetupSQLStatements()
{
    int res;
    if (!m_read_stmt) {
        if ((res = sqlite3_prepare_v2(m_database.m_db, "SELECT value FROM main WHERE key = ?", -1, &m_read_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_insert_stmt) {
        if ((res = sqlite3_prepare_v2(m_database.m_db, "INSERT INTO main VALUES(?, ?)", -1, &m_insert_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_overwrite_stmt) {
        if ((res = sqlite3_prepare_v2(m_database.m_db, "INSERT or REPLACE into main values(?, ?)", -1, &m_overwrite_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_delete_stmt) {
        if ((res = sqlite3_prepare_v2(m_database.m_db, "DELETE FROM main WHERE key = ?", -1, &m_delete_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_cursor_stmt) {
        if ((res = sqlite3_prepare_v2(m_database.m_db, "SELECT key, value FROM main", -1, &m_cursor_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements : %s\n", sqlite3_errstr(res)));
        }
    }
}

SQLiteDatabase::~SQLiteDatabase()
{
    Cleanup();
}

void SQLiteDatabase::Cleanup() noexcept
{
    Close();

    LOCK(g_sqlite_mutex);
    if (--g_sqlite_count == 0) {
        int ret = sqlite3_shutdown();
        if (ret != SQLITE_OK) {
            LogPrintf("SQLiteDatabase: Failed to shutdown SQLite: %s\n", sqlite3_errstr(ret));
        }
    }
}

void SQLiteDatabase::Open()
{
    int flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    if (m_mock) {
        flags |= SQLITE_OPEN_MEMORY; // In memory database for mock db
    }

    if (m_db == nullptr) {
        TryCreateDirectories(m_dir_path);
        int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to open database: %s\n", sqlite3_errstr(ret)));
        }
    }

    if (sqlite3_db_readonly(m_db, "main") != 0) {
        throw std::runtime_error("SQLiteDatabase: Database opened in readonly mode but read-write permissions are needed");
    }

    // Acquire an exclusive lock on the database
    // First change the locking mode to exclusive
    int ret = sqlite3_exec(m_db, "PRAGMA locking_mode = exclusive", nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to change database locking mode to exclusive: %s\n", sqlite3_errstr(ret)));
    }
    // Now begin a transaction to acquire the exclusive lock. This lock won't be released until we close because of the exclusive locking mode.
    ret = sqlite3_exec(m_db, "BEGIN EXCLUSIVE TRANSACTION", nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error("SQLiteDatabase: Unable to obtain an exclusive lock on the database, is it being used by another bitcoind?\n");
    }
    ret = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to end exclusive lock transaction: %s\n", sqlite3_errstr(ret)));
    }

    // Enable fullfsync for the platforms that use it
    ret = sqlite3_exec(m_db, "PRAGMA fullfsync = true", nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to enable fullfsync: %s\n", sqlite3_errstr(ret)));
    }

    // Make the table for our key-value pairs
    // First check that the main table exists
    sqlite3_stmt* check_main_stmt{nullptr};
    ret = sqlite3_prepare_v2(m_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='main'", -1, &check_main_stmt, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to prepare statement to check table existence: %s\n", sqlite3_errstr(ret)));
    }
    ret = sqlite3_step(check_main_stmt);
    if (sqlite3_finalize(check_main_stmt) != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to finalize statement checking table existence: %s\n", sqlite3_errstr(ret)));
    }
    bool table_exists;
    if (ret == SQLITE_DONE) {
        table_exists = false;
    } else if (ret == SQLITE_ROW) {
        table_exists = true;
    } else {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to execute statement to check table existence: %s\n", sqlite3_errstr(ret)));
    }

    // Do the db setup things because the table doesn't exist only when we are creating a new wallet
    if (!table_exists) {
        ret = sqlite3_exec(m_db, "CREATE TABLE main(key BLOB PRIMARY KEY NOT NULL, value BLOB NOT NULL)", nullptr, nullptr, nullptr);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to create new database: %s\n", sqlite3_errstr(ret)));
        }
    }
}

bool SQLiteDatabase::Rewrite(const char* skip)
{
    return false;
}

bool SQLiteDatabase::Backup(const std::string& dest) const
{
    return false;
}

void SQLiteDatabase::Close()
{
    int res = sqlite3_close(m_db);
    if (res != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to close database: %s\n", sqlite3_errstr(res)));
    }
    m_db = nullptr;
}

std::unique_ptr<DatabaseBatch> SQLiteDatabase::MakeBatch(bool flush_on_close)
{
    return nullptr;
}

SQLiteBatch::SQLiteBatch(SQLiteDatabase& database)
    : m_database(database)
{
    // Make sure we have a db handle
    assert(m_database.m_db);

    SetupSQLStatements();
}

void SQLiteBatch::Close()
{
    // If m_db is in a transaction (i.e. not in autocommit mode), then abort the transaction in progress
    if (m_database.m_db && sqlite3_get_autocommit(m_database.m_db) == 0) {
        if (TxnAbort()) {
            LogPrintf("SQLiteBatch: Batch closed unexpectedly without the transaction being explicitly committed or aborted\n");
        } else {
            LogPrintf("SQLiteBatch: Batch closed and failed to abort transaction\n");
        }
    }

    // Free all of the prepared statements
    int ret = sqlite3_finalize(m_read_stmt);
    if (ret != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Batch closed but could not finalize read statement: %s\n", sqlite3_errstr(ret));
    }
    ret = sqlite3_finalize(m_insert_stmt);
    if (ret != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Batch closed but could not finalize insert statement: %s\n", sqlite3_errstr(ret));
    }
    ret = sqlite3_finalize(m_overwrite_stmt);
    if (ret != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Batch closed but could not finalize overwrite statement: %s\n", sqlite3_errstr(ret));
    }
    ret = sqlite3_finalize(m_delete_stmt);
    if (ret != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Batch closed but could not finalize delete statement: %s\n", sqlite3_errstr(ret));
    }
    ret = sqlite3_finalize(m_cursor_stmt);
    if (ret != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Batch closed but could not finalize cursor statement: %s\n", sqlite3_errstr(ret));
    }
    m_read_stmt = nullptr;
    m_insert_stmt = nullptr;
    m_overwrite_stmt = nullptr;
    m_delete_stmt = nullptr;
    m_cursor_stmt = nullptr;
}

bool SQLiteBatch::ReadKey(CDataStream&& key, CDataStream& value)
{
    if (!m_database.m_db) return false;
    assert(m_read_stmt);

    // Bind: leftmost parameter in statement is index 1
    int res = sqlite3_bind_blob(m_read_stmt, 1, key.data(), key.size(), SQLITE_STATIC);
    if (res != SQLITE_OK) {
        LogPrintf("%s: Unable to bind statement: %s\n", __func__, sqlite3_errstr(res));
        sqlite3_clear_bindings(m_read_stmt);
        sqlite3_reset(m_read_stmt);
        return false;
    }
    res = sqlite3_step(m_read_stmt);
    if (res != SQLITE_ROW) {
        if (res != SQLITE_DONE) {
            // SQLITE_DONE means "not found", don't log an error in that case.
            LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
        }
        sqlite3_clear_bindings(m_read_stmt);
        sqlite3_reset(m_read_stmt);
        return false;
    }
    // Leftmost column in result is index 0
    const char* data = reinterpret_cast<const char*>(sqlite3_column_blob(m_read_stmt, 0));
    int data_size = sqlite3_column_bytes(m_read_stmt, 0);
    value.write(data, data_size);

    sqlite3_clear_bindings(m_read_stmt);
    sqlite3_reset(m_read_stmt);
    return true;
}

bool SQLiteBatch::WriteKey(CDataStream&& key, CDataStream&& value, bool overwrite)
{
    if (!m_database.m_db) return false;
    assert(m_insert_stmt && m_overwrite_stmt);

    sqlite3_stmt* stmt;
    if (overwrite) {
        stmt = m_overwrite_stmt;
    } else {
        stmt = m_insert_stmt;
    }

    // Bind: leftmost parameter in statement is index 1
    // Insert index 1 is key, 2 is value
    int res = sqlite3_bind_blob(stmt, 1, key.data(), key.size(), SQLITE_STATIC);
    if (res != SQLITE_OK) {
        LogPrintf("%s: Unable to bind key to statement: %s\n", __func__, sqlite3_errstr(res));
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        return false;
    }
    res = sqlite3_bind_blob(stmt, 2, value.data(), value.size(), SQLITE_STATIC);
    if (res != SQLITE_OK) {
        LogPrintf("%s: Unable to bind value to statement: %s\n", __func__, sqlite3_errstr(res));
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        return false;
    }

    // Execute
    res = sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }
    return res == SQLITE_DONE;
}

bool SQLiteBatch::EraseKey(CDataStream&& key)
{
    if (!m_database.m_db) return false;
    assert(m_delete_stmt);

    // Bind: leftmost parameter in statement is index 1
    int res = sqlite3_bind_blob(m_delete_stmt, 1, key.data(), key.size(), SQLITE_STATIC);
    if (res != SQLITE_OK) {
        LogPrintf("%s: Unable to bind statement: %s\n", __func__, sqlite3_errstr(res));
        sqlite3_clear_bindings(m_delete_stmt);
        sqlite3_reset(m_delete_stmt);
        return false;
    }

    // Execute
    res = sqlite3_step(m_delete_stmt);
    sqlite3_clear_bindings(m_delete_stmt);
    sqlite3_reset(m_delete_stmt);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }
    return res == SQLITE_DONE;
}

bool SQLiteBatch::HasKey(CDataStream&& key)
{
    if (!m_database.m_db) return false;
    assert(m_read_stmt);

    // Bind: leftmost parameter in statement is index 1
    bool ret = false;
    int res = sqlite3_bind_blob(m_read_stmt, 1, key.data(), key.size(), SQLITE_STATIC);
    if (res == SQLITE_OK) {
        res = sqlite3_step(m_read_stmt);
        if (res == SQLITE_ROW) {
            ret = true;
        }
    }

    sqlite3_clear_bindings(m_read_stmt);
    sqlite3_reset(m_read_stmt);
    return ret;
}

bool SQLiteBatch::StartCursor()
{
    return false;
}

bool SQLiteBatch::ReadAtCursor(CDataStream& key, CDataStream& value, bool& complete)
{
    return false;
}

void SQLiteBatch::CloseCursor()
{
}

bool SQLiteBatch::TxnBegin()
{
    return false;
}

bool SQLiteBatch::TxnCommit()
{
    return false;
}

bool SQLiteBatch::TxnAbort()
{
    return false;
}

bool ExistsSQLiteDatabase(const fs::path& path)
{
    return false;
}

std::unique_ptr<SQLiteDatabase> MakeSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    return MakeUnique<SQLiteDatabase>(path, path / DATABASE_FILENAME);
}

std::string SQLiteDatabaseVersion()
{
    return std::string(sqlite3_libversion());
}
