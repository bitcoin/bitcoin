// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DB_H
#define BITCOIN_WALLET_DB_H

#include <clientversion.h>
#include <streams.h>
#include <support/allocators/secure.h>
#include <util/fs.h>

#include <atomic>
#include <memory>
#include <optional>
#include <string>

class ArgsManager;
struct bilingual_str;

namespace wallet {
// BytePrefix compares equality with other byte spans that begin with the same prefix.
struct BytePrefix {
    Span<const std::byte> prefix;
};
bool operator<(BytePrefix a, Span<const std::byte> b);
bool operator<(Span<const std::byte> a, BytePrefix b);

class DatabaseCursor
{
public:
    explicit DatabaseCursor() {}
    virtual ~DatabaseCursor() {}

    DatabaseCursor(const DatabaseCursor&) = delete;
    DatabaseCursor& operator=(const DatabaseCursor&) = delete;

    enum class Status
    {
        FAIL,
        MORE,
        DONE,
    };

    virtual Status Next(DataStream& key, DataStream& value) { return Status::FAIL; }
};

/** RAII class that provides access to a WalletDatabase */
class DatabaseBatch
{
private:
    virtual bool ReadKey(DataStream&& key, DataStream& value) = 0;
    virtual bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite = true) = 0;
    virtual bool EraseKey(DataStream&& key) = 0;
    virtual bool HasKey(DataStream&& key) = 0;

public:
    explicit DatabaseBatch() {}
    virtual ~DatabaseBatch() {}

    DatabaseBatch(const DatabaseBatch&) = delete;
    DatabaseBatch& operator=(const DatabaseBatch&) = delete;

    virtual void Flush() = 0;
    virtual void Close() = 0;

    template <typename K, typename T>
    bool Read(const K& key, T& value)
    {
        DataStream ssKey{};
        ssKey.reserve(1000);
        ssKey << key;

        DataStream ssValue{};
        if (!ReadKey(std::move(ssKey), ssValue)) return false;
        try {
            ssValue >> value;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    template <typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite = true)
    {
        DataStream ssKey{};
        ssKey.reserve(1000);
        ssKey << key;

        DataStream ssValue{};
        ssValue.reserve(10000);
        ssValue << value;

        return WriteKey(std::move(ssKey), std::move(ssValue), fOverwrite);
    }

    template <typename K>
    bool Erase(const K& key)
    {
        DataStream ssKey{};
        ssKey.reserve(1000);
        ssKey << key;

        return EraseKey(std::move(ssKey));
    }

    template <typename K>
    bool Exists(const K& key)
    {
        DataStream ssKey{};
        ssKey.reserve(1000);
        ssKey << key;

        return HasKey(std::move(ssKey));
    }
    virtual bool ErasePrefix(Span<const std::byte> prefix) = 0;

    virtual std::unique_ptr<DatabaseCursor> GetNewCursor() = 0;
    virtual std::unique_ptr<DatabaseCursor> GetNewPrefixCursor(Span<const std::byte> prefix) = 0;
    virtual bool TxnBegin() = 0;
    virtual bool TxnCommit() = 0;
    virtual bool TxnAbort() = 0;
};

/** An instance of this class represents one database.
 **/
class WalletDatabase
{
public:
    /** Create dummy DB handle */
    WalletDatabase() : nUpdateCounter(0) {}
    virtual ~WalletDatabase() {};

    /** Open the database if it is not already opened. */
    virtual void Open() = 0;

    //! Counts the number of active database users to be sure that the database is not closed while someone is using it
    std::atomic<int> m_refcount{0};
    /** Indicate the a new database user has began using the database. Increments m_refcount */
    virtual void AddRef() = 0;
    /** Indicate that database user has stopped using the database and that it could be flushed or closed. Decrement m_refcount */
    virtual void RemoveRef() = 0;

    /** Rewrite the entire database on disk, with the exception of key pszSkip if non-zero
     */
    virtual bool Rewrite(const char* pszSkip=nullptr) = 0;

    /** Back up the entire database to a file.
     */
    virtual bool Backup(const std::string& strDest) const = 0;

    /** Make sure all changes are flushed to database file.
     */
    virtual void Flush() = 0;
    /** Flush to the database file and close the database.
     *  Also close the environment if no other databases are open in it.
     */
    virtual void Close() = 0;
    /* flush the wallet passively (TRY_LOCK)
       ideal to be called periodically */
    virtual bool PeriodicFlush() = 0;

    virtual void IncrementUpdateCounter() = 0;

    virtual void ReloadDbEnv() = 0;

    /** Return path to main database file for logs and error messages. */
    virtual std::string Filename() = 0;

    virtual std::string Format() = 0;

    std::atomic<unsigned int> nUpdateCounter;
    unsigned int nLastSeen{0};
    unsigned int nLastFlushed{0};
    int64_t nLastWalletUpdate{0};

    /** Make a DatabaseBatch connected to this database */
    virtual std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) = 0;
};

enum class DatabaseFormat {
    SQLITE,
    BERKELEY_RO,
};

struct DatabaseOptions {
    bool require_existing = false;
    bool require_create = false;
    std::optional<DatabaseFormat> require_format;
    uint64_t create_flags = 0;
    SecureString create_passphrase;

    // Specialized options. Not every option is supported by every backend.
    bool verify = true;             //!< Check data integrity on load.
    bool use_unsafe_sync = false;   //!< Disable file sync for faster performance.
    bool use_shared_memory = false; //!< Let other processes access the database.
    int64_t max_log_mb = 100;       //!< Max log size to allow before consolidating.
};

enum class DatabaseStatus {
    SUCCESS,
    FAILED_BAD_PATH,
    FAILED_BAD_FORMAT,
    FAILED_ALREADY_LOADED,
    FAILED_ALREADY_EXISTS,
    FAILED_NOT_FOUND,
    FAILED_CREATE,
    FAILED_LOAD,
    FAILED_VERIFY,
    FAILED_ENCRYPT,
    FAILED_INVALID_BACKUP_FILE,
};

/** Recursively list database paths in directory. */
std::vector<std::pair<fs::path, std::string>> ListDatabases(const fs::path& path);

void ReadDatabaseArgs(const ArgsManager& args, DatabaseOptions& options);
std::unique_ptr<WalletDatabase> MakeDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);

fs::path BDBDataFile(const fs::path& path);
fs::path SQLiteDataFile(const fs::path& path);
bool IsBDBFile(const fs::path& path);
bool IsSQLiteFile(const fs::path& path);
} // namespace wallet

#endif // BITCOIN_WALLET_DB_H
