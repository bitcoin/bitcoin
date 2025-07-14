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

static void ErrorLogCallback(void* arg, int code, const char* msg)
{
    // From sqlite3_config() documentation for the SQLITE_CONFIG_LOG option:
    // "The void pointer that is the second argument to SQLITE_CONFIG_LOG is passed through as
    // the first parameter to the application-defined logger function whenever that function is
    // invoked."
    // Assert that this is the case:
    assert(arg == nullptr);
    LogWarning("SQLite Error. Code: %d. Message: %s", code, msg);
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

template <typename T>
concept BindBlob = requires(T a)
{
    { a.data() } -> std::convertible_to<const void*>;
    { a.size() } -> std::convertible_to<size_t>;
};

template <typename T>
concept ColumnBlob = requires(T a, T::element_type* data, size_t size)
{
    T(data, size);
};

template <typename T>
concept ColumnSpanBlob = requires(T a, std::span<const std::byte> s)
{
    T(s);
};

/** RAII class that encapsulates a sqlite3_stmt */
class SQLiteStatement
{
private:
    sqlite3& m_db;
    sqlite3_stmt* m_stmt{nullptr};

public:
    explicit SQLiteStatement(sqlite3& db, const std::string& stmt_text)
        : m_db(db)
    {
        int res = sqlite3_prepare_v2(&m_db, stmt_text.c_str(), -1, &m_stmt, nullptr);
        if (res != SQLITE_OK) {
            throw std::runtime_error(strprintf(
                "SQLiteStatement: Failed to prepare SQL statement '%s': %s\n", stmt_text, sqlite3_errstr(res)));
        }
    }

    ~SQLiteStatement()
    {
        sqlite3_finalize(m_stmt);
    }

    int Step()
    {
        return sqlite3_step(m_stmt);
    }

    void Reset()
    {
        sqlite3_clear_bindings(m_stmt);
        sqlite3_reset(m_stmt);
    }

    template<typename T>
    bool Bind(int index, T& data, const std::string& description)
    {
        int res = SQLITE_ERROR;
        if constexpr (std::is_same_v<T, std::string>) { // Check for string first since strings also satisfy BindBlob
            res = sqlite3_bind_text(m_stmt, index, data.data(), data.size(), SQLITE_STATIC);
        } else if constexpr (BindBlob<T>) {
            // Pass a pointer to the empty string "" below instead of passing the
            // blob.data() pointer if the blob.data() pointer is null. Passing a null
            // data pointer to bind_blob would cause sqlite to bind the SQL NULL value
            // instead of the empty blob value X'', which would mess up SQL comparisons.
            res = sqlite3_bind_blob(m_stmt, index, data.data() ? static_cast<const void*>(data.data()) : "", data.size(), SQLITE_STATIC);
        } else if constexpr (std::integral<T> ) {
            res = sqlite3_bind_int64(m_stmt, index, static_cast<sqlite3_int64>(data));
        } else {
            static_assert(ALWAYS_FALSE<T>);
        }

        if (res != SQLITE_OK) {
            LogWarning("Unable to bind %s to statement: %s", description, sqlite3_errstr(res));
            Reset();
            return false;
        }

        return true;
    }

    template<typename T>
    std::optional<T> Column(int col)
    {
        Assert(col < sqlite3_column_count(m_stmt));
        int column_type = sqlite3_column_type(m_stmt, col);
        if (column_type == SQLITE_NULL) {
            return std::nullopt;
        }

        if constexpr (std::integral<T> && sizeof(T) <= 4) {
            return sqlite3_column_int(m_stmt, col);
        } else if constexpr (std::integral<T> && std::is_signed_v<T> && sizeof(T) <= 8) {
            return static_cast<int64_t>(sqlite3_column_int64(m_stmt, col));
        } else if constexpr (std::is_same_v<T, std::string>) {
            const char* text = (const char*)sqlite3_column_text(m_stmt, col);
            if (!text) {
                return "";
            }
            size_t size = sqlite3_column_bytes(m_stmt, col);
            std::string str_text(text, size);
            return str_text;
        } else if constexpr (ColumnBlob<T>) {
            return T(
                reinterpret_cast<T::element_type*>(sqlite3_column_blob(m_stmt, col)),
                static_cast<size_t>(sqlite3_column_bytes(m_stmt, col))
            );
        } else if constexpr (ColumnSpanBlob<T>) {
            const std::byte* data = reinterpret_cast<const std::byte*>(sqlite3_column_blob(m_stmt, col));
            size_t size = static_cast<size_t>(sqlite3_column_bytes(m_stmt, col));
            if (!data || !size) return std::nullopt;
            return T({data, size});
        } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
            const unsigned char* data = reinterpret_cast<const unsigned char*>(sqlite3_column_blob(m_stmt, col));
            size_t size = static_cast<size_t>(sqlite3_column_bytes(m_stmt, col));
            if (!data || !size) return std::nullopt;
            return T(data, data + size);
        } else {
            static_assert(ALWAYS_FALSE<T>);
        }
    }
};

static std::optional<int> ReadPragmaInteger(sqlite3& db, const std::string& key, const std::string& description, bilingual_str& error)
{
    std::string stmt_text = strprintf("PRAGMA %s", key);
    try {
        SQLiteStatement pragma_read_stmt(db, stmt_text);
        int ret = pragma_read_stmt.Step();
        if (ret != SQLITE_ROW) {
            error = Untranslated(strprintf("SQLiteDatabase: Failed to fetch %s: %s", description, sqlite3_errstr(ret)));
            return std::nullopt;
        }
        int result = *pragma_read_stmt.Column<int>(0);
        return result;
    } catch (std::runtime_error& e) {
        error = Untranslated(e.what());
        return std::nullopt;
    }
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

SQLiteDatabase::SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, const DatabaseOptions& options)
    : SQLiteDatabase(dir_path, file_path, options, /*additional_flags=*/0)
{}

SQLiteDatabase::SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, const DatabaseOptions& options, int additional_flags)
    : WalletDatabase(), m_dir_path(dir_path), m_file_path(fs::PathToString(file_path)), m_write_semaphore(1), m_use_unsafe_sync(options.use_unsafe_sync)
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
        Open(additional_flags);
    } catch (const std::runtime_error&) {
        // If open fails, cleanup this object and rethrow the exception
        Cleanup();
        throw;
    }
}

void SQLiteBatch::SetupSQLStatements()
{
    m_read_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "SELECT value FROM main WHERE key = ?");
    m_insert_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "INSERT INTO main VALUES(?, ?)");
    m_overwrite_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "INSERT or REPLACE into main values(?, ?)");
    m_delete_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "DELETE FROM main WHERE key = ?");
    m_delete_prefix_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "DELETE FROM main WHERE instr(key, ?) = 1");

    if (m_database.HasTxsTable()) {
        m_insert_tx_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "INSERT or REPLACE INTO transactions (txid, tx, comment, comment_to, replaces, replaced_by, timesmart, timereceived, order_pos, messages, payment_requests, state_type, state_data) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        m_update_full_tx_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "UPDATE transactions SET comment = ?, comment_to = ?, replaces = ?, replaced_by = ? , timesmart = ?, timereceived = ?, order_pos = ?, messages= ?, payment_requests = ?, state_type =?, state_data = ? WHERE txid = ?");
        m_update_tx_replaced_by_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "UPDATE transactions SET replaced_by = ? WHERE txid = ?");
        m_update_tx_state_stmt = std::make_unique<SQLiteStatement>(*m_database.m_db, "UPDATE transactions SET state_type = ?, state_data = ? WHERE txid = ?");
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
            LogWarning("SQLiteDatabase: Failed to shutdown SQLite: %s", sqlite3_errstr(ret));
        }
    }
}

bool SQLiteDatabase::Verify(bilingual_str& error)
{
    assert(m_db);

    // Check the application ID matches our network magic
    auto read_result = ReadPragmaInteger(*m_db, "application_id", "the application id", error);
    if (!read_result.has_value()) return false;
    uint32_t app_id = static_cast<uint32_t>(read_result.value());
    uint32_t net_magic = ReadBE32(Params().MessageStart().data());
    if (app_id != net_magic) {
        error = strprintf(_("SQLiteDatabase: Unexpected application id. Expected %u, got %u"), net_magic, app_id);
        return false;
    }

    // Check our schema version
    read_result = ReadPragmaInteger(*m_db, "user_version", "sqlite wallet schema version", error);
    if (!read_result.has_value()) return false;
    int32_t user_ver = read_result.value();
    if (user_ver != WALLET_SCHEMA_VERSION) {
        error = strprintf(_("SQLiteDatabase: Unknown sqlite wallet schema version %d. Only version %d is supported"), user_ver, WALLET_SCHEMA_VERSION);
        return false;
    }

    try {
        SQLiteStatement integrity_check_stmt(*m_db, "PRAGMA integrity_check");
        while (true) {
            int ret = integrity_check_stmt.Step();
            if (ret == SQLITE_DONE) {
                break;
            }
            if (ret != SQLITE_ROW) {
                error = strprintf(_("SQLiteDatabase: Failed to execute statement to verify database: %s"), sqlite3_errstr(ret));
                break;
            }
            std::string str_msg(*integrity_check_stmt.Column<std::string>(0));
            if (str_msg == "ok") {
                continue;
            }
            if (error.empty()) {
                error = _("Failed to verify database") + Untranslated("\n");
            }
            error += Untranslated(strprintf("%s\n", str_msg));
        }
    } catch (std::runtime_error& e) {
        error = Untranslated(e.what());
    }
    return error.empty();
}

void SQLiteDatabase::Open()
{
    Open(/*additional_flags*/0);
}

void SQLiteDatabase::Open(int additional_flags)
{
    int flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | additional_flags;

    if (m_db == nullptr) {
        if (!(flags & SQLITE_OPEN_MEMORY)) {
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
               LogWarning("Failed to enable SQL tracing for %s", Filename());
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
        LogWarning("SQLite is configured to not wait for data to be flushed to disk. Data loss and corruption may occur.");
        SetPragma(m_db, "synchronous", "OFF", "Failed to set synchronous mode to OFF");
    }

    // Make the table for our key-value pairs
    // First check that the main table exists
    SQLiteStatement check_main_stmt(*m_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='main'");
    ret = check_main_stmt.Step();
    bool table_exists;
    if (ret == SQLITE_DONE) {
        table_exists = false;
    } else if (ret == SQLITE_ROW) {
        table_exists = true;
    } else {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to execute statement to check main table existence: %s\n", sqlite3_errstr(ret)));
    }

    // Then check that the transactions table exists
    SQLiteStatement check_txs_stmt(*m_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='transactions'");
    ret = check_txs_stmt.Step();
    if (ret == SQLITE_DONE) {
        m_has_txs_table = false;
    } else if (ret == SQLITE_ROW) {
        m_has_txs_table = true;
    } else {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to execute statement to check transactions table existence: %s\n", sqlite3_errstr(ret)));
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

    if (!m_has_txs_table) {
        if (!CreateTxsTable()) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to create new transactions database: %s\n", sqlite3_errstr(ret)));
        }
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
        LogWarning("Unable to begin sqlite backup: %s", sqlite3_errmsg(m_db));
        sqlite3_close(db_copy);
        return false;
    }
    // Specifying -1 will copy all of the pages
    res = sqlite3_backup_step(backup, -1);
    if (res != SQLITE_DONE) {
        LogWarning("Unable to continue sqlite backup: %s", sqlite3_errstr(res));
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

bool SQLiteDatabase::HasTxsTable() const
{
    return m_has_txs_table;
}

bool SQLiteDatabase::CreateTxsTable()
{
    int ret = sqlite3_exec(m_db, "CREATE TABLE transactions(txid BLOB PRIMARY KEY NOT NULL, tx BLOB NOT NULL, comment STRING, comment_to STRING, replaces BLOB, replaced_by BLOB, timesmart INTEGER, timereceived INTEGER, order_pos INTEGER, messages BLOB, payment_requests BLOB, state_type INTEGER, state_data BLOB)", nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        return false;
    }
    m_has_txs_table = true;
    return true;
}

SQLiteBatch::SQLiteBatch(SQLiteDatabase& database)
    : m_database(database)
{
    // Make sure we have a db handle
    assert(m_database.m_db);

    SetupSQLStatements();
}

SQLiteBatch::~SQLiteBatch()
{
    Close();
}

void SQLiteBatch::Close()
{
    bool force_conn_refresh = false;

    // If we began a transaction, and it wasn't committed, abort the transaction in progress
    if (m_txn) {
        if (TxnAbort()) {
            LogWarning("SQLiteBatch: Batch closed unexpectedly without the transaction being explicitly committed or aborted");
        } else {
            // If transaction cannot be aborted, it means there is a bug or there has been data corruption. Try to recover in this case
            // by closing and reopening the database. Closing the database should also ensure that any changes made since the transaction
            // was opened will be rolled back and future transactions can succeed without committing old data.
            force_conn_refresh = true;
            LogWarning("SQLiteBatch: Batch closed and failed to abort transaction, resetting db connection..");
        }
    }

    // Free all of the prepared statements
    m_read_stmt.reset();
    m_insert_stmt.reset();
    m_overwrite_stmt.reset();
    m_delete_stmt.reset();
    m_delete_prefix_stmt.reset();
    m_insert_tx_stmt.reset();
    m_update_full_tx_stmt.reset();
    m_update_tx_replaced_by_stmt.reset();
    m_update_tx_state_stmt.reset();

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
    if (!m_read_stmt->Bind(1, key, "key")) return false;
    int res = m_read_stmt->Step();
    if (res != SQLITE_ROW) {
        if (res != SQLITE_DONE) {
            // SQLITE_DONE means "not found", don't log an error in that case.
            LogWarning("Unable to execute read statement: %s", sqlite3_errstr(res));
        }
        m_read_stmt->Reset();
        return false;
    }
    // Leftmost column in result is index 0
    value.clear();
    value.write(*m_read_stmt->Column<std::span<const std::byte>>(0));

    m_read_stmt->Reset();
    return true;
}

bool SQLiteBatch::ExecStatement(SQLiteStatement* stmt)
{
    if (!m_database.m_db) return false;
    assert(stmt);

    // Acquire semaphore if not previously acquired when creating a transaction.
    if (!m_txn) m_database.m_write_semaphore.acquire();

    // Execute
    int res = stmt->Step();
    stmt->Reset();
    if (res != SQLITE_DONE) {
        LogError("Unable to execute statement: %s\n", sqlite3_errstr(res));
    }

    if (!m_txn) m_database.m_write_semaphore.release();

    return res == SQLITE_DONE;
}

bool SQLiteBatch::WriteKey(DataStream&& key, DataStream&& value, bool overwrite)
{
    if (!m_database.m_db) return false;
    assert(m_insert_stmt && m_overwrite_stmt);

    SQLiteStatement* stmt;
    if (overwrite) {
        stmt = m_overwrite_stmt.get();
    } else {
        stmt = m_insert_stmt.get();
    }

    // Bind: leftmost parameter in statement is index 1
    // Insert index 1 is key, 2 is value
    if (!stmt->Bind(1, key, "key")) return false;
    if (!stmt->Bind(2, value, "value")) return false;
    return ExecStatement(stmt);
}

bool SQLiteBatch::ExecEraseStatement(SQLiteStatement* stmt, std::span<const std::byte> blob)
{
    if (!m_database.m_db) return false;
    assert(stmt);

    // Bind: leftmost parameter in statement is index 1
    if (!stmt->Bind(1, blob, "key")) return false;
    return ExecStatement(stmt);
}

bool SQLiteBatch::EraseKey(DataStream&& key)
{
    return ExecEraseStatement(m_delete_stmt.get(), key);
}

bool SQLiteBatch::ErasePrefix(std::span<const std::byte> prefix)
{
    return ExecEraseStatement(m_delete_prefix_stmt.get(), prefix);
}

bool SQLiteBatch::HasKey(DataStream&& key)
{
    if (!m_database.m_db) return false;
    assert(m_read_stmt);

    // Bind: leftmost parameter in statement is index 1
    if (!m_read_stmt->Bind(1, key, "key")) return false;
    int res = m_read_stmt->Step();
    m_read_stmt->Reset();
    return res == SQLITE_ROW;
}

bool SQLiteBatch::CreateTxsTable()
{
    if (m_database.HasTxsTable()) return true;
    if (!m_database.CreateTxsTable()) return false;
    SetupSQLStatements();
    return true;
}

bool SQLiteBatch::WriteTx(
    const Txid& txid,
    const std::span<std::byte>& serialized_tx,
    const std::optional<std::string>& comment,
    const std::optional<std::string>& comment_to,
    const std::optional<Txid>& replaces,
    const std::optional<Txid>& replaced_by,
    uint32_t timesmart,
    uint32_t timereceived,
    int64_t order_pos,
    const std::vector<std::string>& messages,
    const std::vector<std::string>& payment_requests,
    int32_t state_type,
    const std::vector<unsigned char>& state_data
)
{
    if (!m_database.HasTxsTable()) return true;

    // Lifetime of DataStream needs to be until the statement is executed.
    DataStream ser_messages, ser_payment_reqs;

    if (!m_insert_tx_stmt->Bind(1, txid, "txid")) return false;
    if (!m_insert_tx_stmt->Bind(2, serialized_tx, "tx")) return false;
    if (comment && !m_insert_tx_stmt->Bind(3, *comment, "comment")) return false;
    if (comment_to && !m_insert_tx_stmt->Bind(4, *comment_to, "comment_to")) return false;
    if (replaces && !m_insert_tx_stmt->Bind(5, *replaces, "replaces")) return false;
    if (replaced_by && !m_insert_tx_stmt->Bind(6, *replaced_by, "replaced_by")) return false;
    if (!m_insert_tx_stmt->Bind(7, timesmart, "timesmart")) return false;
    if (!m_insert_tx_stmt->Bind(8, timereceived, "timereceived")) return false;
    if (!m_insert_tx_stmt->Bind(9, order_pos, "order_pos")) return false;
    if (!messages.empty()) {
        ser_messages << messages;
        if (!m_insert_tx_stmt->Bind(10, ser_messages, "messages")) return false;
    }
    if (!payment_requests.empty()) {
        ser_payment_reqs << payment_requests;
        if (!m_insert_tx_stmt->Bind(11, ser_payment_reqs, "payment_requests")) return false;
    }
    if (!m_insert_tx_stmt->Bind(12, state_type, "state_type")) return false;
    if (!m_insert_tx_stmt->Bind(13, state_data, "state_data")) return false;
    return ExecStatement(m_insert_tx_stmt.get());
}

bool SQLiteBatch::UpdateFullTx(
    const Txid& txid,
    const std::optional<std::string>& comment,
    const std::optional<std::string>& comment_to,
    const std::optional<Txid>& replaces,
    const std::optional<Txid>& replaced_by,
    uint32_t timesmart,
    uint32_t timereceived,
    int64_t order_pos,
    const std::vector<std::string>& messages,
    const std::vector<std::string>& payment_requests,
    int32_t state_type,
    const std::vector<unsigned char>& state_data
)
{
    if (!m_database.HasTxsTable()) return true;

    // Lifetime of DataStream needs to be until the statement is executed.
    DataStream ser_messages, ser_payment_reqs;

    if (comment && !m_update_full_tx_stmt->Bind(1, *comment, "comment")) return false;
    if (comment_to && !m_update_full_tx_stmt->Bind(2, *comment_to, "comment_to")) return false;
    if (replaces && !m_update_full_tx_stmt->Bind(3, *replaces, "replaces")) return false;
    if (replaced_by && !m_update_full_tx_stmt->Bind(4, *replaced_by, "replaced_by")) return false;
    if (!m_update_full_tx_stmt->Bind(5, timesmart, "timesmart")) return false;
    if (!m_update_full_tx_stmt->Bind(6, timereceived, "timereceived")) return false;
    if (!m_update_full_tx_stmt->Bind(7, order_pos, "order_pos")) return false;
    if (!messages.empty()) {
        ser_messages << messages;
        if (!m_update_full_tx_stmt->Bind(8, ser_messages, "messages")) return false;
    }
    if (!payment_requests.empty()) {
        ser_payment_reqs << payment_requests;
        if (!m_update_full_tx_stmt->Bind(9, ser_payment_reqs, "payment_requests")) return false;
    }
    if (!m_update_full_tx_stmt->Bind(10, state_type, "state_type")) return false;
    if (!m_update_full_tx_stmt->Bind(11, state_data, "state_data")) return false;
    if (!m_update_full_tx_stmt->Bind(12, txid, "txid")) return false;
    return ExecStatement(m_update_full_tx_stmt.get());
}

bool SQLiteBatch::UpdateTxReplacedBy(const Txid& txid, const Txid& replaced_by)
{
    if (!m_database.HasTxsTable()) return true;

    if (!m_update_tx_replaced_by_stmt->Bind(1, replaced_by, "replaced_by")) return false;
    if (!m_update_tx_replaced_by_stmt->Bind(2, txid, "txid")) return false;
    return ExecStatement(m_update_tx_replaced_by_stmt.get());
}

bool SQLiteBatch::UpdateTxState(const Txid& txid, int32_t state_type, const std::vector<unsigned char>& state_data)
{
    if (!m_database.HasTxsTable()) return true;

    if (!m_update_tx_state_stmt->Bind(1, state_type, "state_type")) return false;
    if (!m_update_tx_state_stmt->Bind(2, state_data, "state_data")) return false;
    if (!m_update_tx_state_stmt->Bind(3, txid, "txid")) return false;
    return ExecStatement(m_update_tx_state_stmt.get());
}

DatabaseCursor::Status SQLiteCursor::Next(DataStream& key, DataStream& value)
{
    int res = m_cursor_stmt->Step();
    if (res == SQLITE_DONE) {
        return Status::DONE;
    }
    if (res != SQLITE_ROW) {
        LogWarning("Unable to execute cursor step: %s", sqlite3_errstr(res));
        return Status::FAIL;
    }

    key.clear();
    value.clear();

    // Leftmost column in result is index 0
    key.write(*m_cursor_stmt->Column<std::span<const std::byte>>(0));
    value.write(*m_cursor_stmt->Column<std::span<const std::byte>>(1));
    return Status::MORE;
}

SQLiteCursor::SQLiteCursor(sqlite3& db, const std::string& stmt_text)
    : m_cursor_stmt{std::make_unique<SQLiteStatement>(db, stmt_text)}
{}

SQLiteCursor::SQLiteCursor(sqlite3& db, const std::string& stmt_text, std::vector<std::byte> start_range, std::vector<std::byte> end_range)
    : m_cursor_stmt{std::make_unique<SQLiteStatement>(db, stmt_text)},
    m_prefix_range_start(std::move(start_range)),
    m_prefix_range_end(std::move(end_range))
{
    if (!m_cursor_stmt->Bind(1, m_prefix_range_start, "prefix_start")) {
        throw std::runtime_error("Unable to bind prefix start to prefix cursor");
    }
    if (!m_prefix_range_end.empty() && !m_cursor_stmt->Bind(2, m_prefix_range_end, "prefix_end")) {
        throw std::runtime_error("Unable to bind prefix end to prefix cursor");
    }
}

SQLiteCursor::~SQLiteCursor() = default;

std::unique_ptr<DatabaseCursor> SQLiteBatch::GetNewCursor()
{
    if (!m_database.m_db) return nullptr;
    return std::make_unique<SQLiteCursor>(*m_database.m_db, "SELECT key, value FROM main");
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

    const char* stmt_text = end_range.empty() ? "SELECT key, value FROM main WHERE key >= ?" :
                            "SELECT key, value FROM main WHERE key >= ? AND key < ?";
    return std::make_unique<SQLiteCursor>(*m_database.m_db, stmt_text, start_range, end_range);
}

bool SQLiteBatch::TxnBegin()
{
    if (!m_database.m_db || m_txn) return false;
    m_database.m_write_semaphore.acquire();
    Assert(!m_database.HasActiveTxn());
    int res = Assert(m_exec_handler)->Exec(m_database, "BEGIN TRANSACTION");
    if (res != SQLITE_OK) {
        LogWarning("SQLiteBatch: Failed to begin the transaction");
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
        LogWarning("SQLiteBatch: Failed to commit the transaction");
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
        LogWarning("SQLiteBatch: Failed to abort the transaction");
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
