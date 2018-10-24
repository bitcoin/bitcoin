// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DB_H
#define BITCOIN_WALLET_DB_H

#include <clientversion.h>
#include <fs.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <util/system.h>
#include <version.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <db_cxx.h>

static const unsigned int DEFAULT_WALLET_DBLOGSIZE = 100;
static const bool DEFAULT_WALLET_PRIVDB = true;

struct WalletDatabaseFileId {
    u_int8_t value[DB_FILE_ID_LEN];
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
    std::map<std::string, int> mapFileUseCount;
    std::map<std::string, std::reference_wrapper<BerkeleyDatabase>> m_databases;
    std::unordered_map<std::string, WalletDatabaseFileId> m_fileids;
    std::condition_variable_any m_db_in_use;

    BerkeleyEnvironment(const fs::path& env_directory);
    ~BerkeleyEnvironment();
    void Reset();

    void MakeMock();
    bool IsMock() const { return fMockDb; }
    bool IsInitialized() const { return fDbEnvInit; }
    fs::path Directory() const { return strPath; }

    /**
     * Verify that database file strFile is OK. If it is not,
     * call the callback to try to recover.
     * This must be called BEFORE strFile is opened.
     * Returns true if strFile is OK.
     */
    enum class VerifyResult { VERIFY_OK,
                        RECOVER_OK,
                        RECOVER_FAIL };
    typedef bool (*recoverFunc_type)(const fs::path& file_path, std::string& out_backup_filename);
    VerifyResult Verify(const std::string& strFile, recoverFunc_type recoverFunc, std::string& out_backup_filename);
    /**
     * Salvage data from a file that Verify says is bad.
     * fAggressive sets the DB_AGGRESSIVE flag (see berkeley DB->verify() method documentation).
     * Appends binary key/value pairs to vResult, returns true if successful.
     * NOTE: reads the entire database into memory, so cannot be used
     * for huge databases.
     */
    typedef std::pair<std::vector<unsigned char>, std::vector<unsigned char> > KeyValPair;
    bool Salvage(const std::string& strFile, bool fAggressive, std::vector<KeyValPair>& vResult);

    bool Open(bool retry);
    void Close();
    void Flush(bool fShutdown);
    void CheckpointLSN(const std::string& strFile);

    void CloseDb(const std::string& strFile);
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

/** Get BerkeleyEnvironment and database filename given a wallet path. */
BerkeleyEnvironment* GetWalletEnv(const fs::path& wallet_path, std::string& database_filename);

/** An instance of this class represents one database.
 * For BerkeleyDB this is just a (env, strFile) tuple.
 **/
class BerkeleyDatabase
{
    friend class BerkeleyBatch;
public:
    /** Create dummy DB handle */
    BerkeleyDatabase() : nUpdateCounter(0), nLastSeen(0), nLastFlushed(0), nLastWalletUpdate(0), env(nullptr)
    {
    }

    /** Create DB handle to real database */
    BerkeleyDatabase(const fs::path& wallet_path, bool mock = false) :
        nUpdateCounter(0), nLastSeen(0), nLastFlushed(0), nLastWalletUpdate(0)
    {
        env = GetWalletEnv(wallet_path, strFile);
        auto inserted = env->m_databases.emplace(strFile, std::ref(*this));
        assert(inserted.second);
        if (mock) {
            env->Close();
            env->Reset();
            env->MakeMock();
        }
    }

    ~BerkeleyDatabase() {
        if (env) {
            size_t erased = env->m_databases.erase(strFile);
            assert(erased == 1);
        }
    }

    /** Return object for accessing database at specified path. */
    static std::unique_ptr<BerkeleyDatabase> Create(const fs::path& path)
    {
        return MakeUnique<BerkeleyDatabase>(path);
    }

    /** Return object for accessing dummy database with no read/write capabilities. */
    static std::unique_ptr<BerkeleyDatabase> CreateDummy()
    {
        return MakeUnique<BerkeleyDatabase>();
    }

    /** Return object for accessing temporary in-memory database. */
    static std::unique_ptr<BerkeleyDatabase> CreateMock()
    {
        return MakeUnique<BerkeleyDatabase>("", true /* mock */);
    }

    /** Rewrite the entire database on disk, with the exception of key pszSkip if non-zero
     */
    bool Rewrite(const char* pszSkip=nullptr);

    /** Back up the entire database to a file.
     */
    bool Backup(const std::string& strDest);

    /** Make sure all changes are flushed to disk.
     */
    void Flush(bool shutdown);

    void IncrementUpdateCounter();

    void ReloadDbEnv();

    std::atomic<unsigned int> nUpdateCounter;
    unsigned int nLastSeen;
    unsigned int nLastFlushed;
    int64_t nLastWalletUpdate;

    /** Database pointer. This is initialized lazily and reset during flushes, so it can be null. */
    std::unique_ptr<Db> m_db;

private:
    /** BerkeleyDB specific */
    BerkeleyEnvironment *env;
    std::string strFile;

    /** Return whether this database handle is a dummy for testing.
     * Only to be used at a low level, application should ideally not care
     * about this.
     */
    bool IsDummy() { return env == nullptr; }
};


/** RAII class that provides access to a Berkeley database */
class BerkeleyBatch
{
protected:
    Db* pdb;
    std::string strFile;
    DbTxn* activeTxn;
    bool fReadOnly;
    bool fFlushOnClose;
    BerkeleyEnvironment *env;

public:
    explicit BerkeleyBatch(BerkeleyDatabase& database, const char* pszMode = "r+", bool fFlushOnCloseIn=true);
    ~BerkeleyBatch() { Close(); }

    BerkeleyBatch(const BerkeleyBatch&) = delete;
    BerkeleyBatch& operator=(const BerkeleyBatch&) = delete;

    void Flush();
    void Close();
    static bool Recover(const fs::path& file_path, void *callbackDataIn, bool (*recoverKVcallback)(void* callbackData, CDataStream ssKey, CDataStream ssValue), std::string& out_backup_filename);

    /* flush the wallet passively (TRY_LOCK)
       ideal to be called periodically */
    static bool PeriodicFlush(BerkeleyDatabase& database);
    /* verifies the database environment */
    static bool VerifyEnvironment(const fs::path& file_path, std::string& errorStr);
    /* verifies the database file */
    static bool VerifyDatabaseFile(const fs::path& file_path, std::string& warningStr, std::string& errorStr, BerkeleyEnvironment::recoverFunc_type recoverFunc);

public:
    template <typename K, typename T>
    bool Read(const K& key, T& value)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(ssKey.data(), ssKey.size());

        // Read
        Dbt datValue;
        datValue.set_flags(DB_DBT_MALLOC);
        int ret = pdb->get(activeTxn, &datKey, &datValue, 0);
        memory_cleanse(datKey.get_data(), datKey.get_size());
        bool success = false;
        if (datValue.get_data() != nullptr) {
            // Unserialize value
            try {
                CDataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK, CLIENT_VERSION);
                ssValue >> value;
                success = true;
            } catch (const std::exception&) {
                // In this case success remains 'false'
            }

            // Clear and free memory
            memory_cleanse(datValue.get_data(), datValue.get_size());
            free(datValue.get_data());
        }
        return ret == 0 && success;
    }

    template <typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite = true)
    {
        if (!pdb)
            return true;
        if (fReadOnly)
            assert(!"Write called on database in read-only mode");

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(ssKey.data(), ssKey.size());

        // Value
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(10000);
        ssValue << value;
        Dbt datValue(ssValue.data(), ssValue.size());

        // Write
        int ret = pdb->put(activeTxn, &datKey, &datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));

        // Clear memory in case it was a private key
        memory_cleanse(datKey.get_data(), datKey.get_size());
        memory_cleanse(datValue.get_data(), datValue.get_size());
        return (ret == 0);
    }

    template <typename K>
    bool Erase(const K& key)
    {
        if (!pdb)
            return false;
        if (fReadOnly)
            assert(!"Erase called on database in read-only mode");

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(ssKey.data(), ssKey.size());

        // Erase
        int ret = pdb->del(activeTxn, &datKey, 0);

        // Clear memory
        memory_cleanse(datKey.get_data(), datKey.get_size());
        return (ret == 0 || ret == DB_NOTFOUND);
    }

    template <typename K>
    bool Exists(const K& key)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(ssKey.data(), ssKey.size());

        // Exists
        int ret = pdb->exists(activeTxn, &datKey, 0);

        // Clear memory
        memory_cleanse(datKey.get_data(), datKey.get_size());
        return (ret == 0);
    }

    Dbc* GetCursor()
    {
        if (!pdb)
            return nullptr;
        Dbc* pcursor = nullptr;
        int ret = pdb->cursor(nullptr, &pcursor, 0);
        if (ret != 0)
            return nullptr;
        return pcursor;
    }

    int ReadAtCursor(Dbc* pcursor, CDataStream& ssKey, CDataStream& ssValue, bool setRange = false)
    {
        // Read at cursor
        Dbt datKey;
        unsigned int fFlags = DB_NEXT;
        if (setRange) {
            datKey.set_data(ssKey.data());
            datKey.set_size(ssKey.size());
            fFlags = DB_SET_RANGE;
        }
        Dbt datValue;
        datKey.set_flags(DB_DBT_MALLOC);
        datValue.set_flags(DB_DBT_MALLOC);
        int ret = pcursor->get(&datKey, &datValue, fFlags);
        if (ret != 0)
            return ret;
        else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
            return 99999;

        // Convert to streams
        ssKey.SetType(SER_DISK);
        ssKey.clear();
        ssKey.write((char*)datKey.get_data(), datKey.get_size());
        ssValue.SetType(SER_DISK);
        ssValue.clear();
        ssValue.write((char*)datValue.get_data(), datValue.get_size());

        // Clear and free memory
        memory_cleanse(datKey.get_data(), datKey.get_size());
        memory_cleanse(datValue.get_data(), datValue.get_size());
        free(datKey.get_data());
        free(datValue.get_data());
        return 0;
    }

public:
    bool TxnBegin()
    {
        if (!pdb || activeTxn)
            return false;
        DbTxn* ptxn = env->TxnBegin();
        if (!ptxn)
            return false;
        activeTxn = ptxn;
        return true;
    }

    bool TxnCommit()
    {
        if (!pdb || !activeTxn)
            return false;
        int ret = activeTxn->commit(0);
        activeTxn = nullptr;
        return (ret == 0);
    }

    bool TxnAbort()
    {
        if (!pdb || !activeTxn)
            return false;
        int ret = activeTxn->abort();
        activeTxn = nullptr;
        return (ret == 0);
    }

    bool ReadVersion(int& nVersion)
    {
        nVersion = 0;
        return Read(std::string("version"), nVersion);
    }

    bool WriteVersion(int nVersion)
    {
        return Write(std::string("version"), nVersion);
    }

    bool static Rewrite(BerkeleyDatabase& database, const char* pszSkip = nullptr);
};

#endif // BITCOIN_WALLET_DB_H
