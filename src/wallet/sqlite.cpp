// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/sqlite.h>

#include <chainparams.h>
#include <crypto/common.h>
#include <logging.h>
#include <sync.h>
#include <util/fs_helpers.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <wallet/db.h>

#include <sqlite3.h>
#include <stdint.h>

#include <optional>
#include <utility>
#include <vector>

namespace wallet {
static constexpr int32_t WALLET_SCHEMA_VERSION = 0;

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

static bool BindBlobToStatement(sqlite3_stmt* stmt,
                                int index,
                                Span<const std::byte> blob,
                                const std::string& description)
{
    int res = sqlite3_bind_blob(stmt, index, blob.data(), blob.size(), SQLITE_STATIC);
    if (res != SQLITE_OK) {
        LogPrintf("Unable to bind %s to statement: %s\n", description, sqlite3_errstr(res));
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        return false;
    }

    return true;
}

static std::optional<int> ReadPragmaInteger(sqlite3* db, const std::string& key, const std::string& description, bilingual_str& error)
{
    std::string stmt_text = strprintf("PRAGMA %s", key);
    sqlite3_stmt* pragma_read_stmt{nullptr};
    int ret = sqlite3_prepare_v2(db, stmt_text.c_str(), -1, &pragma_read_stmt, nullptr);
    if (ret != SQLITE_OK) {
        sqlite3_finalize(pragma_read_stmt);
        error = Untranslated(strprintf("SQLiteDatabase: Failed to prepare the statement to fetch %s: %s", description, sqlite3_errstr(ret)));
        return std::nullopt;
    }
    ret = sqlite3_step(pragma_read_stmt);
    if (ret != SQLITE_ROW) {
        sqlite3_finalize(pragma_read_stmt);
        error = Untranslated(strprintf("SQLiteDatabase: Failed to fetch %s: %s", description, sqlite3_errstr(ret)));
        return std::nullopt;
    }
    int result = sqlite3_column_int(pragma_read_stmt, 0);
    sqlite3_finalize(pragma_read_stmt);
    return result;
}

static void SetPragma(sqlite3* db, const std::string& key, const std::string& value, const std::string& err_msg)
{
    std::string stmt_text = strprintf("PRAGMA %s = %s", key, value);
    int ret = sqlite3_exec(db, stmt_text.c_str(), nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: %s: %s\n", err_msg, sqlite3_errstr(ret)));
    }
}

Mutex SQLiteDatabase::g_sqlite_mutex;
int SQLiteDatabase::g_sqlite_count = 0;

SQLiteDatabase::SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, const DatabaseOptions& options, bool mock)
    : WalletDatabase(), m_mock(mock), m_dir_path(fs::PathToString(dir_path)), m_file_path(fs::PathToString(file_path)), m_use_unsafe_sync(options.use_unsafe_sync)
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
            // Force serialized threading mode
            ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
            if (ret != SQLITE_OK) {
                throw std::runtime_error(strprintf("SQLiteDatabase: Failed to configure serialized threading mode: %s\n", sqlite3_errstr(ret)));
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
    const std::vector<std::pair<sqlite3_stmt**, const char*>> statements{
        {&m_read_stmt, "SELECT value FROM main WHERE key = ?"},
        {&m_insert_stmt, "INSERT INTO main VALUES(?, ?)"},
        {&m_overwrite_stmt, "INSERT or REPLACE into main values(?, ?)"},
        {&m_delete_stmt, "DELETE FROM main WHERE key = ?"},
    };

    for (const auto& [stmt_prepared, stmt_text] : statements) {
        if (*stmt_prepared == nullptr) {
            int res = sqlite3_prepare_v2(m_database.m_db, stmt_text, -1, stmt_prepared, nullptr);
            if (res != SQLITE_OK) {
                throw std::runtime_error(strprintf(
                    "SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
            }
        }
    }
}

SQLiteDatabase::~SQLiteDatabase()
{
    Cleanup();
}

void SQLiteDatabase::Cleanup() noexcept
{
    AssertLockNotHeld(g_sqlite_mutex);

    Close();

    LOCK(g_sqlite_mutex);
    if (--g_sqlite_count == 0) {
        int ret = sqlite3_shutdown();
        if (ret != SQLITE_OK) {
            LogPrintf("SQLiteDatabase: Failed to shutdown SQLite: %s\n", sqlite3_errstr(ret));
        }
    }
}

bool SQLiteDatabase::Verify(bilingual_str& error)
{
    assert(m_db);

    // Check the application ID matches our network magic
    auto read_result = ReadPragmaInteger(m_db, "application_id", "the application id", error);
    if (!read_result.has_value()) return false;
    uint32_t app_id = static_cast<uint32_t>(read_result.value());
    uint32_t net_magic = ReadBE32(Params().MessageStart());
    if (app_id != net_magic) {
        error = strprintf(_("SQLiteDatabase: Unexpected application id. Expected %u, got %u"), net_magic, app_id);
        return false;
    }

    // Check our schema version
    read_result = ReadPragmaInteger(m_db, "user_version", "sqlite wallet schema version", error);
    if (!read_result.has_value()) return false;
    int32_t user_ver = read_result.value();
    if (user_ver != WALLET_SCHEMA_VERSION) {
        error = strprintf(_("SQLiteDatabase: Unknown sqlite wallet schema version %d. Only version %d is supported"), user_ver, WALLET_SCHEMA_VERSION);
        return false;
    }

    sqlite3_stmt* stmt{nullptr};
    int ret = sqlite3_prepare_v2(m_db, "PRAGMA integrity_check", -1, &stmt, nullptr);
    if (ret != SQLITE_OK) {
        sqlite3_finalize(stmt);
        error = strprintf(_("SQLiteDatabase: Failed to prepare statement to verify database: %s"), sqlite3_errstr(ret));
        return false;
    }
    while (true) {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_DONE) {
            break;
        }
        if (ret != SQLITE_ROW) {
            error = strprintf(_("SQLiteDatabase: Failed to execute statement to verify database: %s"), sqlite3_errstr(ret));
            break;
        }
        const char* msg = (const char*)sqlite3_column_text(stmt, 0);
        if (!msg) {
            error = strprintf(_("SQLiteDatabase: Failed to read database verification error: %s"), sqlite3_errstr(ret));
            break;
        }
        std::string str_msg(msg);
        if (str_msg == "ok") {
            continue;
        }
        if (error.empty()) {
            error = _("Failed to verify database") + Untranslated("\n");
        }
        error += Untranslated(strprintf("%s\n", str_msg));
    }
    sqlite3_finalize(stmt);
    return error.empty();
}

void SQLiteDatabase::Open()
{
    int flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    if (m_mock) {
        flags |= SQLITE_OPEN_MEMORY; // In memory database for mock db
    }

    if (m_db == nullptr) {
        if (!m_mock) {
            TryCreateDirectories(fs::PathFromString(m_dir_path));
        }
        int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to open database: %s\n", sqlite3_errstr(ret)));
        }
        ret = sqlite3_extended_result_codes(m_db, 1);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to enable extended result codes: %s\n", sqlite3_errstr(ret)));
        }
    }

    if (sqlite3_db_readonly(m_db, "main") != 0) {
        throw std::runtime_error("SQLiteDatabase: Database opened in readonly mode but read-write permissions are needed");
    }

    // Acquire an exclusive lock on the database
    // First change the locking mode to exclusive
    SetPragma(m_db, "locking_mode", "exclusive", "Unable to change database locking mode to exclusive");
    // Now begin a transaction to acquire the exclusive lock. This lock won't be released until we close because of the exclusive locking mode.
    int ret = sqlite3_exec(m_db, "BEGIN EXCLUSIVE TRANSACTION", nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error("SQLiteDatabase: Unable to obtain an exclusive lock on the database, is it being used by another instance of " PACKAGE_NAME "?\n");
    }
    ret = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to end exclusive lock transaction: %s\n", sqlite3_errstr(ret)));
    }

    // Enable fullfsync for the platforms that use it
    SetPragma(m_db, "fullfsync", "true", "Failed to enable fullfsync");

    if (m_use_unsafe_sync) {
        // Use normal synchronous mode for the journal
        LogPrintf("WARNING SQLite is configured to not wait for data to be flushed to disk. Data loss and corruption may occur.\n");
        SetPragma(m_db, "synchronous", "OFF", "Failed to set synchronous mode to OFF");
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

        // Set the application id
        uint32_t app_id = ReadBE32(Params().MessageStart());
        SetPragma(m_db, "application_id", strprintf("%d", static_cast<int32_t>(app_id)),
                  "Failed to set the application id");

        // Set the user version
        SetPragma(m_db, "user_version", strprintf("%d", WALLET_SCHEMA_VERSION),
                  "Failed to set the wallet schema version");
    }
}

bool SQLiteDatabase::Rewrite(const char* skip)
{
    // Rewrite the database using the VACUUM command: https://sqlite.org/lang_vacuum.html
    int ret = sqlite3_exec(m_db, "VACUUM", nullptr, nullptr, nullptr);
    return ret == SQLITE_OK;
}

bool SQLiteDatabase::Backup(const std::string& dest) const
{
    sqlite3* db_copy;
    int res = sqlite3_open(dest.c_str(), &db_copy);
    if (res != SQLITE_OK) {
        sqlite3_close(db_copy);
        return false;
    }
    sqlite3_backup* backup = sqlite3_backup_init(db_copy, "main", m_db, "main");
    if (!backup) {
        LogPrintf("%s: Unable to begin backup: %s\n", __func__, sqlite3_errmsg(m_db));
        sqlite3_close(db_copy);
        return false;
    }
    // Specifying -1 will copy all of the pages
    res = sqlite3_backup_step(backup, -1);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to backup: %s\n", __func__, sqlite3_errstr(res));
        sqlite3_backup_finish(backup);
        sqlite3_close(db_copy);
        return false;
    }
    res = sqlite3_backup_finish(backup);
    sqlite3_close(db_copy);
    return res == SQLITE_OK;
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
    // We ignore flush_on_close because we don't do manual flushing for SQLite
    return std::make_unique<SQLiteBatch>(*this);
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
    const std::vector<std::pair<sqlite3_stmt**, const char*>> statements{
        {&m_read_stmt, "read"},
        {&m_insert_stmt, "insert"},
        {&m_overwrite_stmt, "overwrite"},
        {&m_delete_stmt, "delete"},
    };

    for (const auto& [stmt_prepared, stmt_description] : statements) {
        int res = sqlite3_finalize(*stmt_prepared);
        if (res != SQLITE_OK) {
            LogPrintf("SQLiteBatch: Batch closed but could not finalize %s statement: %s\n",
                      stmt_description, sqlite3_errstr(res));
        }
        *stmt_prepared = nullptr;
    }
}

bool SQLiteBatch::ReadKey(DataStream&& key, DataStream& value)
{
    if (!m_database.m_db) return false;
    assert(m_read_stmt);

    // Bind: leftmost parameter in statement is index 1
    if (!BindBlobToStatement(m_read_stmt, 1, key, "key")) return false;
    int res = sqlite3_step(m_read_stmt);
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
    const std::byte* data{AsBytePtr(sqlite3_column_blob(m_read_stmt, 0))};
    size_t data_size(sqlite3_column_bytes(m_read_stmt, 0));
    value.write({data, data_size});

    sqlite3_clear_bindings(m_read_stmt);
    sqlite3_reset(m_read_stmt);
    return true;
}

bool SQLiteBatch::WriteKey(DataStream&& key, DataStream&& value, bool overwrite)
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
    if (!BindBlobToStatement(stmt, 1, key, "key")) return false;
    if (!BindBlobToStatement(stmt, 2, value, "value")) return false;

    // Execute
    int res = sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }
    return res == SQLITE_DONE;
}

bool SQLiteBatch::EraseKey(DataStream&& key)
{
    if (!m_database.m_db) return false;
    assert(m_delete_stmt);

    // Bind: leftmost parameter in statement is index 1
    if (!BindBlobToStatement(m_delete_stmt, 1, key, "key")) return false;

    // Execute
    int res = sqlite3_step(m_delete_stmt);
    sqlite3_clear_bindings(m_delete_stmt);
    sqlite3_reset(m_delete_stmt);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }
    return res == SQLITE_DONE;
}

bool SQLiteBatch::HasKey(DataStream&& key)
{
    if (!m_database.m_db) return false;
    assert(m_read_stmt);

    // Bind: leftmost parameter in statement is index 1
    if (!BindBlobToStatement(m_read_stmt, 1, key, "key")) return false;
    int res = sqlite3_step(m_read_stmt);
    sqlite3_clear_bindings(m_read_stmt);
    sqlite3_reset(m_read_stmt);
    return res == SQLITE_ROW;
}

DatabaseCursor::Status SQLiteCursor::Next(DataStream& key, DataStream& value)
{
    int res = sqlite3_step(m_cursor_stmt);
    if (res == SQLITE_DONE) {
        return Status::DONE;
    }
    if (res != SQLITE_ROW) {
        LogPrintf("%s: Unable to execute cursor step: %s\n", __func__, sqlite3_errstr(res));
        return Status::FAIL;
    }

    // Leftmost column in result is index 0
    const std::byte* key_data{AsBytePtr(sqlite3_column_blob(m_cursor_stmt, 0))};
    size_t key_data_size(sqlite3_column_bytes(m_cursor_stmt, 0));
    key.write({key_data, key_data_size});
    const std::byte* value_data{AsBytePtr(sqlite3_column_blob(m_cursor_stmt, 1))};
    size_t value_data_size(sqlite3_column_bytes(m_cursor_stmt, 1));
    value.write({value_data, value_data_size});
    return Status::MORE;
}

SQLiteCursor::~SQLiteCursor()
{
    sqlite3_reset(m_cursor_stmt);
    int res = sqlite3_finalize(m_cursor_stmt);
    if (res != SQLITE_OK) {
        LogPrintf("%s: cursor closed but could not finalize cursor statement: %s\n",
                  __func__, sqlite3_errstr(res));
    }
}

std::unique_ptr<DatabaseCursor> SQLiteBatch::GetNewCursor()
{
    if (!m_database.m_db) return nullptr;
    auto cursor = std::make_unique<SQLiteCursor>();

    const char* stmt_text = "SELECT key, value FROM main";
    int res = sqlite3_prepare_v2(m_database.m_db, stmt_text, -1, &cursor->m_cursor_stmt, nullptr);
    if (res != SQLITE_OK) {
        throw std::runtime_error(strprintf(
            "%s: Failed to setup cursor SQL statement: %s\n", __func__, sqlite3_errstr(res)));
    }

    return cursor;
}

bool SQLiteBatch::TxnBegin()
{
    if (!m_database.m_db || sqlite3_get_autocommit(m_database.m_db) == 0) return false;
    int res = sqlite3_exec(m_database.m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    if (res != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Failed to begin the transaction\n");
    }
    return res == SQLITE_OK;
}

bool SQLiteBatch::TxnCommit()
{
    if (!m_database.m_db || sqlite3_get_autocommit(m_database.m_db) != 0) return false;
    int res = sqlite3_exec(m_database.m_db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
    if (res != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Failed to commit the transaction\n");
    }
    return res == SQLITE_OK;
}

bool SQLiteBatch::TxnAbort()
{
    if (!m_database.m_db || sqlite3_get_autocommit(m_database.m_db) != 0) return false;
    int res = sqlite3_exec(m_database.m_db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
    if (res != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Failed to abort the transaction\n");
    }
    return res == SQLITE_OK;
}

std::unique_ptr<SQLiteDatabase> MakeSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    try {
        fs::path data_file = SQLiteDataFile(path);
        auto db = std::make_unique<SQLiteDatabase>(data_file.parent_path(), data_file, options);
        if (options.verify && !db->Verify(error)) {
            status = DatabaseStatus::FAILED_VERIFY;
            return nullptr;
        }
        status = DatabaseStatus::SUCCESS;
        return db;
    } catch (const std::runtime_error& e) {
        status = DatabaseStatus::FAILED_LOAD;
        error = Untranslated(e.what());
        return nullptr;
    }
}

std::string SQLiteDatabaseVersion()
{
    return std::string(sqlite3_libversion());
}
} // namespace wallet
