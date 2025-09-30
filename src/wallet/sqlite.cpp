// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <wallet/sqlite.h>

#include <chainparams.h>
#include <crypto/common.h>
#include <logging.h>
#include <sync.h>
#include <util/check.h>
#include <util/fs_helpers.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <wallet/db.h>

#include <sqlite3.h>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace wallet {
static constexpr int32_t WALLET_SCHEMA_VERSION = 0;

static std::span<const std::byte> SpanFromBlob(sqlite3_stmt* stmt, int col)
{
    return {reinterpret_cast<const std::byte*>(sqlite3_column_blob(stmt, col)),
            static_cast<size_t>(sqlite3_column_bytes(stmt, col))};
}

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

static int TraceSqlCallback(unsigned code, void* context, void* param1, void* param2)
{
    auto* db = static_cast<SQLiteDatabase*>(context);
    if (code == SQLITE_TRACE_STMT) {
        auto* stmt = static_cast<sqlite3_stmt*>(param1);
        // To be conservative and avoid leaking potentially secret information
        // in the log file, only expand statements that query the database, not
        // statements that update the database.
        char* expanded{sqlite3_stmt_readonly(stmt) ? sqlite3_expanded_sql(stmt) : nullptr};
        LogTrace(BCLog::WALLETDB, "[%s] SQLite Statement: %s\n", db->Filename(), expanded ? expanded : sqlite3_sql(stmt));
        if (expanded) sqlite3_free(expanded);
    }
    return SQLITE_OK;
}

static bool BindBlobToStatement(sqlite3_stmt* stmt,
                                int index,
                                std::span<const std::byte> blob,
                                const std::string& description)
{
    // Pass a pointer to the empty string "" below instead of passing the
    // blob.data() pointer if the blob.data() pointer is null. Passing a null
    // data pointer to bind_blob would cause sqlite to bind the SQL NULL value
    // instead of the empty blob value X'', which would mess up SQL comparisons.
    int res = sqlite3_bind_blob(stmt, index, blob.data() ? static_cast<const void*>(blob.data()) : "", blob.size(), SQLITE_STATIC);
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
    : WalletDatabase(), m_mock(mock), m_dir_path(dir_path), m_file_path(fs::PathToString(file_path)), m_write_semaphore(1), m_use_unsafe_sync(options.use_unsafe_sync)
{
    {
        LOCK(g_sqlite_mutex);
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
        {&m_delete_prefix_stmt, "DELETE FROM main WHERE instr(key, ?) = 1"},
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
    uint32_t net_magic = ReadBE32(Params().MessageStart().data());
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
            TryCreateDirectories(m_dir_path);
        }
        int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to open database: %s\n", sqlite3_errstr(ret)));
        }
        ret = sqlite3_extended_result_codes(m_db, 1);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to enable extended result codes: %s\n", sqlite3_errstr(ret)));
        }
        // Trace SQL statements if tracing is enabled with -debug=walletdb -loglevel=walletdb:trace
        if (LogAcceptCategory(BCLog::WALLETDB, BCLog::Level::Trace)) {
           ret = sqlite3_trace_v2(m_db, SQLITE_TRACE_STMT, TraceSqlCallback, this);
           if (ret != SQLITE_OK) {
               LogPrintf("Failed to enable SQL tracing for %s\n", Filename());
           }
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
        throw std::runtime_error("SQLiteDatabase: Unable to obtain an exclusive lock on the database, is it being used by another instance of " CLIENT_NAME "?\n");
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
        uint32_t app_id = ReadBE32(Params().MessageStart().data());
        SetPragma(m_db, "application_id", strprintf("%d", static_cast<int32_t>(app_id)),
                  "Failed to set the application id");

        // Set the user version
        SetPragma(m_db, "user_version", strprintf("%d", WALLET_SCHEMA_VERSION),
                  "Failed to set the wallet schema version");
    }
}

bool SQLiteDatabase::Rewrite()
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

bool SQLiteDatabase::HasActiveTxn()
{
    // 'sqlite3_get_autocommit' returns true by default, and false if a transaction has begun and not been committed or rolled back.
    return m_db && sqlite3_get_autocommit(m_db) == 0;
}

int SQliteExecHandler::Exec(SQLiteDatabase& database, const std::string& statement)
{
    return sqlite3_exec(database.m_db, statement.data(), nullptr, nullptr, nullptr);
}

std::unique_ptr<DatabaseBatch> SQLiteDatabase::MakeBatch()
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
    bool force_conn_refresh = false;

    // If we began a transaction, and it wasn't committed, abort the transaction in progress
    if (m_txn) {
        if (TxnAbort()) {
            LogPrintf("SQLiteBatch: Batch closed unexpectedly without the transaction being explicitly committed or aborted\n");
        } else {
            // If transaction cannot be aborted, it means there is a bug or there has been data corruption. Try to recover in this case
            // by closing and reopening the database. Closing the database should also ensure that any changes made since the transaction
            // was opened will be rolled back and future transactions can succeed without committing old data.
            force_conn_refresh = true;
            LogPrintf("SQLiteBatch: Batch closed and failed to abort transaction, resetting db connection..\n");
        }
    }

    // Free all of the prepared statements
    const std::vector<std::pair<sqlite3_stmt**, const char*>> statements{
        {&m_read_stmt, "read"},
        {&m_insert_stmt, "insert"},
        {&m_overwrite_stmt, "overwrite"},
        {&m_delete_stmt, "delete"},
        {&m_delete_prefix_stmt, "delete prefix"},
    };

    for (const auto& [stmt_prepared, stmt_description] : statements) {
        int res = sqlite3_finalize(*stmt_prepared);
        if (res != SQLITE_OK) {
            LogPrintf("SQLiteBatch: Batch closed but could not finalize %s statement: %s\n",
                      stmt_description, sqlite3_errstr(res));
        }
        *stmt_prepared = nullptr;
    }

    if (force_conn_refresh) {
        m_database.Close();
        try {
            m_database.Open();
            // If TxnAbort failed and we refreshed the connection, the semaphore was not released, so release it here to avoid deadlocks on future writes.
            m_database.m_write_semaphore.release();
        } catch (const std::runtime_error&) {
            // If open fails, cleanup this object and rethrow the exception
            m_database.Close();
            throw;
        }
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
    value.clear();
    value.write(SpanFromBlob(m_read_stmt, 0));

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

    // Acquire semaphore if not previously acquired when creating a transaction.
    if (!m_txn) m_database.m_write_semaphore.acquire();

    // Execute
    int res = sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }

    if (!m_txn) m_database.m_write_semaphore.release();

    return res == SQLITE_DONE;
}

bool SQLiteBatch::ExecStatement(sqlite3_stmt* stmt, std::span<const std::byte> blob)
{
    if (!m_database.m_db) return false;
    assert(stmt);

    // Bind: leftmost parameter in statement is index 1
    if (!BindBlobToStatement(stmt, 1, blob, "key")) return false;

    // Acquire semaphore if not previously acquired when creating a transaction.
    if (!m_txn) m_database.m_write_semaphore.acquire();

    // Execute
    int res = sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }

    if (!m_txn) m_database.m_write_semaphore.release();

    return res == SQLITE_DONE;
}

bool SQLiteBatch::EraseKey(DataStream&& key)
{
    return ExecStatement(m_delete_stmt, key);
}

bool SQLiteBatch::ErasePrefix(std::span<const std::byte> prefix)
{
    return ExecStatement(m_delete_prefix_stmt, prefix);
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

    key.clear();
    value.clear();

    // Leftmost column in result is index 0
    key.write(SpanFromBlob(m_cursor_stmt, 0));
    value.write(SpanFromBlob(m_cursor_stmt, 1));
    return Status::MORE;
}

SQLiteCursor::~SQLiteCursor()
{
    sqlite3_clear_bindings(m_cursor_stmt);
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

std::unique_ptr<DatabaseCursor> SQLiteBatch::GetNewPrefixCursor(std::span<const std::byte> prefix)
{
    if (!m_database.m_db) return nullptr;

    // To get just the records we want, the SQL statement does a comparison of the binary data
    // where the data must be greater than or equal to the prefix, and less than
    // the prefix incremented by one (when interpreted as an integer)
    std::vector<std::byte> start_range(prefix.begin(), prefix.end());
    std::vector<std::byte> end_range(prefix.begin(), prefix.end());
    auto it = end_range.rbegin();
    for (; it != end_range.rend(); ++it) {
        if (*it == std::byte(std::numeric_limits<unsigned char>::max())) {
            *it = std::byte(0);
            continue;
        }
        *it = std::byte(std::to_integer<unsigned char>(*it) + 1);
        break;
    }
    if (it == end_range.rend()) {
        // If the prefix is all 0xff bytes, clear end_range as we won't need it
        end_range.clear();
    }

    auto cursor = std::make_unique<SQLiteCursor>(start_range, end_range);
    if (!cursor) return nullptr;

    const char* stmt_text = end_range.empty() ? "SELECT key, value FROM main WHERE key >= ?" :
                            "SELECT key, value FROM main WHERE key >= ? AND key < ?";
    int res = sqlite3_prepare_v2(m_database.m_db, stmt_text, -1, &cursor->m_cursor_stmt, nullptr);
    if (res != SQLITE_OK) {
        throw std::runtime_error(strprintf(
            "SQLiteDatabase: Failed to setup cursor SQL statement: %s\n", sqlite3_errstr(res)));
    }

    if (!BindBlobToStatement(cursor->m_cursor_stmt, 1, cursor->m_prefix_range_start, "prefix_start")) return nullptr;
    if (!end_range.empty()) {
        if (!BindBlobToStatement(cursor->m_cursor_stmt, 2, cursor->m_prefix_range_end, "prefix_end")) return nullptr;
    }

    return cursor;
}

bool SQLiteBatch::TxnBegin()
{
    if (!m_database.m_db || m_txn) return false;
    m_database.m_write_semaphore.acquire();
    Assert(!m_database.HasActiveTxn());
    int res = Assert(m_exec_handler)->Exec(m_database, "BEGIN TRANSACTION");
    if (res != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Failed to begin the transaction\n");
        m_database.m_write_semaphore.release();
    } else {
        m_txn = true;
    }
    return res == SQLITE_OK;
}

bool SQLiteBatch::TxnCommit()
{
    if (!m_database.m_db || !m_txn) return false;
    Assert(m_database.HasActiveTxn());
    int res = Assert(m_exec_handler)->Exec(m_database, "COMMIT TRANSACTION");
    if (res != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Failed to commit the transaction\n");
    } else {
        m_txn = false;
        m_database.m_write_semaphore.release();
    }
    return res == SQLITE_OK;
}

bool SQLiteBatch::TxnAbort()
{
    if (!m_database.m_db || !m_txn) return false;
    Assert(m_database.HasActiveTxn());
    int res = Assert(m_exec_handler)->Exec(m_database, "ROLLBACK TRANSACTION");
    if (res != SQLITE_OK) {
        LogPrintf("SQLiteBatch: Failed to abort the transaction\n");
    } else {
        m_txn = false;
        m_database.m_write_semaphore.release();
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
