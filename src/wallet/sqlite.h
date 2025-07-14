// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SQLITE_H
#define BITCOIN_WALLET_SQLITE_H

#include <sync.h>
#include <primitives/transaction_identifier.h>
#include <wallet/db.h>

#include <optional>
#include <semaphore>

struct bilingual_str;

struct sqlite3;

namespace wallet {
class SQLiteDatabase;
class SQLiteStatement;

/** RAII class that provides a database cursor */
class SQLiteCursor : public DatabaseCursor
{
private:
    std::unique_ptr<SQLiteStatement> m_cursor_stmt{nullptr};
public:
    // Copies of the prefix things for the prefix cursor.
    // Prevents SQLite from accessing temp variables for the prefix things.
    std::vector<std::byte> m_prefix_range_start;
    std::vector<std::byte> m_prefix_range_end;

    explicit SQLiteCursor(sqlite3& db, const std::string& stmt_text);
    explicit SQLiteCursor(sqlite3& db, const std::string& stmt_text, std::vector<std::byte> start_range, std::vector<std::byte> end_range);

    ~SQLiteCursor() override;

    Status Next(DataStream& key, DataStream& value) override;
    Status NextTx(
        Txid& txid,
        DataStream& ser_tx,
        std::optional<std::string>& comment,
        std::optional<std::string>& comment_to,
        std::optional<Txid>& replaces,
        std::optional<Txid>& replaced_by,
        uint32_t& timesmart,
        uint32_t& timereceived,
        int64_t& order_pos,
        std::vector<std::string>& messages,
        std::vector<std::string>& payment_requests,
        int32_t& state_type,
        std::vector<unsigned char>& state_data
    );
};

/** Class responsible for executing SQL statements in SQLite databases.
 *  Methods are virtual so they can be overridden by unit tests testing unusual database conditions. */
class SQliteExecHandler
{
public:
    virtual ~SQliteExecHandler() = default;
    virtual int Exec(SQLiteDatabase& database, const std::string& statement);
};

/** RAII class that provides access to a WalletDatabase */
class SQLiteBatch : public DatabaseBatch
{
private:
    SQLiteDatabase& m_database;
    std::unique_ptr<SQliteExecHandler> m_exec_handler{std::make_unique<SQliteExecHandler>()};

    std::unique_ptr<SQLiteStatement> m_read_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_insert_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_overwrite_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_delete_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_delete_prefix_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_insert_tx_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_update_full_tx_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_update_tx_replaced_by_stmt{nullptr};
    std::unique_ptr<SQLiteStatement> m_update_tx_state_stmt{nullptr};

    /** Whether this batch has started a database transaction and whether it owns SQLiteDatabase::m_write_semaphore.
     * If the batch starts a db tx, it acquires the semaphore and sets this to true, keeping the semaphore
     * until the transaction ends to prevent other batch objects from writing to the database.
     *
     * If this batch did not start a transaction, the semaphore is acquired transiently when writing and m_txn
     * is not set.
     *
     * m_txn is different from HasActiveTxn() as it is only true when this batch has started the transaction,
     * not just when any batch has started a transaction.
     */
    bool m_txn{false};

    void SetupSQLStatements();
    bool ExecStatement(SQLiteStatement* stmt);
    bool ExecEraseStatement(SQLiteStatement* stmt, std::span<const std::byte> blob);

protected:
    bool ReadKey(DataStream&& key, DataStream& value) override;
    bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite = true) override;
    bool EraseKey(DataStream&& key) override;
    bool HasKey(DataStream&& key) override;
    bool ErasePrefix(std::span<const std::byte> prefix) override;

public:
    explicit SQLiteBatch(SQLiteDatabase& database);
    ~SQLiteBatch() override;

    void SetExecHandler(std::unique_ptr<SQliteExecHandler>&& handler) { m_exec_handler = std::move(handler); }

    bool WriteTx(const Txid& txid,
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
                 const std::vector<unsigned char>& state_data);
    bool UpdateFullTx(const Txid& txid,
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
                      const std::vector<unsigned char>& state_data);
    bool UpdateTxReplacedBy(const Txid& txid, const Txid& replaced_by);
    bool UpdateTxState(const Txid& txid, int32_t state_type, const std::vector<unsigned char>& state_data);

    void Close() override;

    std::unique_ptr<DatabaseCursor> GetNewCursor() override;
    std::unique_ptr<DatabaseCursor> GetNewPrefixCursor(std::span<const std::byte> prefix) override;
    std::unique_ptr<DatabaseCursor> GetNewTransactionsCursor() override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    bool TxnAbort() override;
    bool HasActiveTxn() override { return m_txn; }

    bool CreateTxsTable();
};

/** An instance of this class represents one SQLite3 database.
 **/
class SQLiteDatabase : public WalletDatabase
{
private:
    bool m_has_txs_table{false};

    const fs::path m_dir_path;

    const std::string m_file_path;

    /**
     * This mutex protects SQLite initialization and shutdown.
     * sqlite3_config() and sqlite3_shutdown() are not thread-safe (sqlite3_initialize() is).
     * Concurrent threads that execute SQLiteDatabase::SQLiteDatabase() should have just one
     * of them do the init and the rest wait for it to complete before all can proceed.
     */
    static Mutex g_sqlite_mutex;
    static int g_sqlite_count GUARDED_BY(g_sqlite_mutex);

    void Cleanup() noexcept EXCLUSIVE_LOCKS_REQUIRED(!g_sqlite_mutex);

    void Open(int additional_flags);

protected:
    SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, const DatabaseOptions& options, int additional_flags);

public:
    SQLiteDatabase() = delete;

    /** Create DB handle to real database */
    SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, const DatabaseOptions& options);

    ~SQLiteDatabase();

    // Batches must acquire this semaphore on writing, and release when done writing.
    // This ensures that only one batch is modifying the database at a time.
    std::binary_semaphore m_write_semaphore;

    bool Verify(bilingual_str& error);

    /** Open the database if it is not already opened */
    void Open() override;

    /** Close the database */
    void Close() override;

    /** Rewrite the entire database on disk */
    bool Rewrite() override;

    /** Back up the entire database to a file.
     */
    bool Backup(const std::string& dest) const override;

    std::string Filename() override { return m_file_path; }
    /** Return paths to all database created files */
    std::vector<fs::path> Files() override
    {
        std::vector<fs::path> files;
        files.emplace_back(m_dir_path / fs::PathFromString(m_file_path));
        files.emplace_back(m_dir_path / fs::PathFromString(m_file_path + "-journal"));
        return files;
    }
    std::string Format() override { return "sqlite"; }

    /** Make a SQLiteBatch connected to this database */
    std::unique_ptr<DatabaseBatch> MakeBatch() override;

    /** Return true if there is an on-going txn in this connection */
    bool HasActiveTxn();

    sqlite3* m_db{nullptr};
    bool m_use_unsafe_sync;

    bool HasTxsTable() const;
    bool CreateTxsTable();
};

std::unique_ptr<SQLiteDatabase> MakeSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);

std::string SQLiteDatabaseVersion();
} // namespace wallet

#endif // BITCOIN_WALLET_SQLITE_H
