// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/bdb.h>
#include <wallet/db.h>

#include <util/strencodings.h>
#include <util/translation.h>

#include <stdint.h>

#ifndef WIN32
#include <sys/stat.h>
#endif

namespace {

//! Make sure database has a unique fileid within the environment. If it
//! doesn't, throw an error. BDB caches do not work properly when more than one
//! open database has the same fileid (values written to one database may show
//! up in reads to other databases).
//!
//! BerkeleyDB generates unique fileids by default
//! (https://docs.oracle.com/cd/E17275_01/html/programmer_reference/program_copy.html),
//! so bitcoin should never create different databases with the same fileid, but
//! this error can be triggered if users manually copy database files.
void CheckUniqueFileid(const BerkeleyEnvironment& env, const std::string& filename, Db& db, WalletDatabaseFileId& fileid)
{
    if (env.IsMock()) return;

    int ret = db.get_mpf()->get_fileid(fileid.value);
    if (ret != 0) {
        throw std::runtime_error(strprintf("BerkeleyDatabase: Can't open database %s (get_fileid failed with %d)", filename, ret));
    }

    for (const auto& item : env.m_fileids) {
        if (fileid == item.second && &fileid != &item.second) {
            throw std::runtime_error(strprintf("BerkeleyDatabase: Can't open database %s (duplicates fileid %s from %s)", filename,
                HexStr(item.second.value), item.first));
        }
    }
}

RecursiveMutex cs_db;
std::map<std::string, std::weak_ptr<BerkeleyEnvironment>> g_dbenvs GUARDED_BY(cs_db); //!< Map from directory name to db environment.
} // namespace

bool WalletDatabaseFileId::operator==(const WalletDatabaseFileId& rhs) const
{
    return memcmp(value, &rhs.value, sizeof(value)) == 0;
}

/**
 * @param[in] env_directory Path to environment directory
 * @return A shared pointer to the BerkeleyEnvironment object for the wallet directory, never empty because ~BerkeleyEnvironment
 * erases the weak pointer from the g_dbenvs map.
 * @post A new BerkeleyEnvironment weak pointer is inserted into g_dbenvs if the directory path key was not already in the map.
 */
std::shared_ptr<BerkeleyEnvironment> GetBerkeleyEnv(const fs::path& env_directory)
{
    LOCK(cs_db);
    auto inserted = g_dbenvs.emplace(env_directory.string(), std::weak_ptr<BerkeleyEnvironment>());
    if (inserted.second) {
        auto env = std::make_shared<BerkeleyEnvironment>(env_directory.string());
        inserted.first->second = env;
        return env;
    }
    return inserted.first->second.lock();
}

//
// BerkeleyBatch
//

void BerkeleyEnvironment::Close()
{
    if (!fDbEnvInit)
        return;

    fDbEnvInit = false;

    for (auto& db : m_databases) {
        BerkeleyDatabase& database = db.second.get();
        assert(database.m_refcount <= 0);
        if (database.m_db) {
            database.m_db->close(0);
            database.m_db.reset();
        }
    }

    FILE* error_file = nullptr;
    dbenv->get_errfile(&error_file);

    int ret = dbenv->close(0);
    if (ret != 0)
        LogPrintf("BerkeleyEnvironment::Close: Error %d closing database environment: %s\n", ret, DbEnv::strerror(ret));
    if (!fMockDb)
        DbEnv((u_int32_t)0).remove(strPath.c_str(), 0);

    if (error_file) fclose(error_file);

    UnlockDirectory(strPath, ".walletlock");
}

void BerkeleyEnvironment::Reset()
{
    dbenv.reset(new DbEnv(DB_CXX_NO_EXCEPTIONS));
    fDbEnvInit = false;
    fMockDb = false;
}

BerkeleyEnvironment::BerkeleyEnvironment(const fs::path& dir_path) : strPath(dir_path.string())
{
    Reset();
}

BerkeleyEnvironment::~BerkeleyEnvironment()
{
    LOCK(cs_db);
    g_dbenvs.erase(strPath);
    Close();
}

bool BerkeleyEnvironment::Open(bilingual_str& err)
{
    if (fDbEnvInit) {
        return true;
    }

    fs::path pathIn = strPath;
    TryCreateDirectories(pathIn);
    if (!LockDirectory(pathIn, ".walletlock")) {
        LogPrintf("Cannot obtain a lock on wallet directory %s. Another instance of bitcoin may be using it.\n", strPath);
        err = strprintf(_("Error initializing wallet database environment %s!"), Directory());
        return false;
    }

    fs::path pathLogDir = pathIn / "database";
    TryCreateDirectories(pathLogDir);
    fs::path pathErrorFile = pathIn / "db.log";
    LogPrintf("BerkeleyEnvironment::Open: LogDir=%s ErrorFile=%s\n", pathLogDir.string(), pathErrorFile.string());

    unsigned int nEnvFlags = 0;
    if (gArgs.GetBoolArg("-privdb", DEFAULT_WALLET_PRIVDB))
        nEnvFlags |= DB_PRIVATE;

    dbenv->set_lg_dir(pathLogDir.string().c_str());
    dbenv->set_cachesize(0, 0x100000, 1); // 1 MiB should be enough for just the wallet
    dbenv->set_lg_bsize(0x10000);
    dbenv->set_lg_max(1048576);
    dbenv->set_lk_max_locks(40000);
    dbenv->set_lk_max_objects(40000);
    dbenv->set_errfile(fsbridge::fopen(pathErrorFile, "a")); /// debug
    dbenv->set_flags(DB_AUTO_COMMIT, 1);
    dbenv->set_flags(DB_TXN_WRITE_NOSYNC, 1);
    dbenv->log_set_config(DB_LOG_AUTO_REMOVE, 1);
    int ret = dbenv->open(strPath.c_str(),
                         DB_CREATE |
                             DB_INIT_LOCK |
                             DB_INIT_LOG |
                             DB_INIT_MPOOL |
                             DB_INIT_TXN |
                             DB_THREAD |
                             DB_RECOVER |
                             nEnvFlags,
                         S_IRUSR | S_IWUSR);
    if (ret != 0) {
        LogPrintf("BerkeleyEnvironment::Open: Error %d opening database environment: %s\n", ret, DbEnv::strerror(ret));
        int ret2 = dbenv->close(0);
        if (ret2 != 0) {
            LogPrintf("BerkeleyEnvironment::Open: Error %d closing failed database environment: %s\n", ret2, DbEnv::strerror(ret2));
        }
        Reset();
        err = strprintf(_("Error initializing wallet database environment %s!"), Directory());
        if (ret == DB_RUNRECOVERY) {
            err += Untranslated(" ") + _("This error could occur if this wallet was not shutdown cleanly and was last loaded using a build with a newer version of Berkeley DB. If so, please use the software that last loaded this wallet");
        }
        return false;
    }

    fDbEnvInit = true;
    fMockDb = false;
    return true;
}

//! Construct an in-memory mock Berkeley environment for testing
BerkeleyEnvironment::BerkeleyEnvironment()
{
    Reset();

    LogPrint(BCLog::WALLETDB, "BerkeleyEnvironment::MakeMock\n");

    dbenv->set_cachesize(1, 0, 1);
    dbenv->set_lg_bsize(10485760 * 4);
    dbenv->set_lg_max(10485760);
    dbenv->set_lk_max_locks(10000);
    dbenv->set_lk_max_objects(10000);
    dbenv->set_flags(DB_AUTO_COMMIT, 1);
    dbenv->log_set_config(DB_LOG_IN_MEMORY, 1);
    int ret = dbenv->open(nullptr,
                         DB_CREATE |
                             DB_INIT_LOCK |
                             DB_INIT_LOG |
                             DB_INIT_MPOOL |
                             DB_INIT_TXN |
                             DB_THREAD |
                             DB_PRIVATE,
                         S_IRUSR | S_IWUSR);
    if (ret > 0) {
        throw std::runtime_error(strprintf("BerkeleyEnvironment::MakeMock: Error %d opening database environment.", ret));
    }

    fDbEnvInit = true;
    fMockDb = true;
}

BerkeleyBatch::SafeDbt::SafeDbt()
{
    m_dbt.set_flags(DB_DBT_MALLOC);
}

BerkeleyBatch::SafeDbt::SafeDbt(void* data, size_t size)
    : m_dbt(data, size)
{
}

BerkeleyBatch::SafeDbt::~SafeDbt()
{
    if (m_dbt.get_data() != nullptr) {
        // Clear memory, e.g. in case it was a private key
        memory_cleanse(m_dbt.get_data(), m_dbt.get_size());
        // under DB_DBT_MALLOC, data is malloced by the Dbt, but must be
        // freed by the caller.
        // https://docs.oracle.com/cd/E17275_01/html/api_reference/C/dbt.html
        if (m_dbt.get_flags() & DB_DBT_MALLOC) {
            free(m_dbt.get_data());
        }
    }
}

const void* BerkeleyBatch::SafeDbt::get_data() const
{
    return m_dbt.get_data();
}

u_int32_t BerkeleyBatch::SafeDbt::get_size() const
{
    return m_dbt.get_size();
}

BerkeleyBatch::SafeDbt::operator Dbt*()
{
    return &m_dbt;
}

bool BerkeleyDatabase::Verify(bilingual_str& errorStr)
{
    fs::path walletDir = env->Directory();
    fs::path file_path = walletDir / strFile;

    LogPrintf("Using BerkeleyDB version %s\n", BerkeleyDatabaseVersion());
    LogPrintf("Using wallet %s\n", file_path.string());

    if (!env->Open(errorStr)) {
        return false;
    }

    if (fs::exists(file_path))
    {
        assert(m_refcount == 0);

        Db db(env->dbenv.get(), 0);
        int result = db.verify(strFile.c_str(), nullptr, nullptr, 0);
        if (result != 0) {
            errorStr = strprintf(_("%s corrupt. Try using the wallet tool bitcoin-wallet to salvage or restoring a backup."), file_path);
            return false;
        }
    }
    // also return true if files does not exists
    return true;
}

void BerkeleyEnvironment::CheckpointLSN(const std::string& strFile)
{
    dbenv->txn_checkpoint(0, 0, 0);
    if (fMockDb)
        return;
    dbenv->lsn_reset(strFile.c_str(), 0);
}

BerkeleyDatabase::~BerkeleyDatabase()
{
    if (env) {
        LOCK(cs_db);
        env->CloseDb(strFile);
        assert(!m_db);
        size_t erased = env->m_databases.erase(strFile);
        assert(erased == 1);
        env->m_fileids.erase(strFile);
    }
}

BerkeleyBatch::BerkeleyBatch(BerkeleyDatabase& database, const bool read_only, bool fFlushOnCloseIn) : pdb(nullptr), activeTxn(nullptr), m_cursor(nullptr), m_database(database)
{
    database.AddRef();
    database.Open();
    fReadOnly = read_only;
    fFlushOnClose = fFlushOnCloseIn;
    env = database.env.get();
    pdb = database.m_db.get();
    strFile = database.strFile;
    if (!Exists(std::string("version"))) {
        bool fTmp = fReadOnly;
        fReadOnly = false;
        Write(std::string("version"), CLIENT_VERSION);
        fReadOnly = fTmp;
    }
}

void BerkeleyDatabase::Open()
{
    unsigned int nFlags = DB_THREAD | DB_CREATE;

    {
        LOCK(cs_db);
        bilingual_str open_err;
        if (!env->Open(open_err))
            throw std::runtime_error("BerkeleyDatabase: Failed to open database environment.");

        if (m_db == nullptr) {
            int ret;
            std::unique_ptr<Db> pdb_temp = std::make_unique<Db>(env->dbenv.get(), 0);

            bool fMockDb = env->IsMock();
            if (fMockDb) {
                DbMpoolFile* mpf = pdb_temp->get_mpf();
                ret = mpf->set_flags(DB_MPOOL_NOFILE, 1);
                if (ret != 0) {
                    throw std::runtime_error(strprintf("BerkeleyDatabase: Failed to configure for no temp file backing for database %s", strFile));
                }
            }

            ret = pdb_temp->open(nullptr,                             // Txn pointer
                            fMockDb ? nullptr : strFile.c_str(),      // Filename
                            fMockDb ? strFile.c_str() : "main",       // Logical db name
                            DB_BTREE,                                 // Database type
                            nFlags,                                   // Flags
                            0);

            if (ret != 0) {
                throw std::runtime_error(strprintf("BerkeleyDatabase: Error %d, can't open database %s", ret, strFile));
            }

            // Call CheckUniqueFileid on the containing BDB environment to
            // avoid BDB data consistency bugs that happen when different data
            // files in the same environment have the same fileid.
            CheckUniqueFileid(*env, strFile, *pdb_temp, this->env->m_fileids[strFile]);

            m_db.reset(pdb_temp.release());

        }
    }
}

void BerkeleyBatch::Flush()
{
    if (activeTxn)
        return;

    // Flush database activity from memory pool to disk log
    unsigned int nMinutes = 0;
    if (fReadOnly)
        nMinutes = 1;

    if (env) { // env is nullptr for dummy databases (i.e. in tests). Don't actually flush if env is nullptr so we don't segfault
        env->dbenv->txn_checkpoint(nMinutes ? gArgs.GetArg("-dblogsize", DEFAULT_WALLET_DBLOGSIZE) * 1024 : 0, nMinutes, 0);
    }
}

void BerkeleyDatabase::IncrementUpdateCounter()
{
    ++nUpdateCounter;
}

BerkeleyBatch::~BerkeleyBatch()
{
    Close();
    m_database.RemoveRef();
}

void BerkeleyBatch::Close()
{
    if (!pdb)
        return;
    if (activeTxn)
        activeTxn->abort();
    activeTxn = nullptr;
    pdb = nullptr;
    CloseCursor();

    if (fFlushOnClose)
        Flush();
}

void BerkeleyEnvironment::CloseDb(const std::string& strFile)
{
    {
        LOCK(cs_db);
        auto it = m_databases.find(strFile);
        assert(it != m_databases.end());
        BerkeleyDatabase& database = it->second.get();
        if (database.m_db) {
            // Close the database handle
            database.m_db->close(0);
            database.m_db.reset();
        }
    }
}

void BerkeleyEnvironment::ReloadDbEnv()
{
    // Make sure that no Db's are in use
    AssertLockNotHeld(cs_db);
    std::unique_lock<RecursiveMutex> lock(cs_db);
    m_db_in_use.wait(lock, [this](){
        for (auto& db : m_databases) {
            if (db.second.get().m_refcount > 0) return false;
        }
        return true;
    });

    std::vector<std::string> filenames;
    for (auto it : m_databases) {
        filenames.push_back(it.first);
    }
    // Close the individual Db's
    for (const std::string& filename : filenames) {
        CloseDb(filename);
    }
    // Reset the environment
    Flush(true); // This will flush and close the environment
    Reset();
    bilingual_str open_err;
    Open(open_err);
}

bool BerkeleyDatabase::Rewrite(const char* pszSkip)
{
    while (true) {
        {
            LOCK(cs_db);
            if (m_refcount <= 0) {
                // Flush log data to the dat file
                env->CloseDb(strFile);
                env->CheckpointLSN(strFile);
                m_refcount = -1;

                bool fSuccess = true;
                LogPrintf("BerkeleyBatch::Rewrite: Rewriting %s...\n", strFile);
                std::string strFileRes = strFile + ".rewrite";
                { // surround usage of db with extra {}
                    BerkeleyBatch db(*this, true);
                    std::unique_ptr<Db> pdbCopy = std::make_unique<Db>(env->dbenv.get(), 0);

                    int ret = pdbCopy->open(nullptr,               // Txn pointer
                                            strFileRes.c_str(), // Filename
                                            "main",             // Logical db name
                                            DB_BTREE,           // Database type
                                            DB_CREATE,          // Flags
                                            0);
                    if (ret > 0) {
                        LogPrintf("BerkeleyBatch::Rewrite: Can't create database file %s\n", strFileRes);
                        fSuccess = false;
                    }

                    if (db.StartCursor()) {
                        while (fSuccess) {
                            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
                            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
                            bool complete;
                            bool ret1 = db.ReadAtCursor(ssKey, ssValue, complete);
                            if (complete) {
                                break;
                            } else if (!ret1) {
                                fSuccess = false;
                                break;
                            }
                            if (pszSkip &&
                                strncmp((const char*)ssKey.data(), pszSkip, std::min(ssKey.size(), strlen(pszSkip))) == 0)
                                continue;
                            if (strncmp((const char*)ssKey.data(), "\x07version", 8) == 0) {
                                // Update version:
                                ssValue.clear();
                                ssValue << CLIENT_VERSION;
                            }
                            Dbt datKey(ssKey.data(), ssKey.size());
                            Dbt datValue(ssValue.data(), ssValue.size());
                            int ret2 = pdbCopy->put(nullptr, &datKey, &datValue, DB_NOOVERWRITE);
                            if (ret2 > 0)
                                fSuccess = false;
                        }
                        db.CloseCursor();
                    }
                    if (fSuccess) {
                        db.Close();
                        env->CloseDb(strFile);
                        if (pdbCopy->close(0))
                            fSuccess = false;
                    } else {
                        pdbCopy->close(0);
                    }
                }
                if (fSuccess) {
                    Db dbA(env->dbenv.get(), 0);
                    if (dbA.remove(strFile.c_str(), nullptr, 0))
                        fSuccess = false;
                    Db dbB(env->dbenv.get(), 0);
                    if (dbB.rename(strFileRes.c_str(), nullptr, strFile.c_str(), 0))
                        fSuccess = false;
                }
                if (!fSuccess)
                    LogPrintf("BerkeleyBatch::Rewrite: Failed to rewrite database file %s\n", strFileRes);
                return fSuccess;
            }
        }
        UninterruptibleSleep(std::chrono::milliseconds{100});
    }
}


void BerkeleyEnvironment::Flush(bool fShutdown)
{
    int64_t nStart = GetTimeMillis();
    // Flush log data to the actual data file on all files that are not in use
    LogPrint(BCLog::WALLETDB, "BerkeleyEnvironment::Flush: [%s] Flush(%s)%s\n", strPath, fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started");
    if (!fDbEnvInit)
        return;
    {
        LOCK(cs_db);
        bool no_dbs_accessed = true;
        for (auto& db_it : m_databases) {
            std::string strFile = db_it.first;
            int nRefCount = db_it.second.get().m_refcount;
            if (nRefCount < 0) continue;
            LogPrint(BCLog::WALLETDB, "BerkeleyEnvironment::Flush: Flushing %s (refcount = %d)...\n", strFile, nRefCount);
            if (nRefCount == 0) {
                // Move log data to the dat file
                CloseDb(strFile);
                LogPrint(BCLog::WALLETDB, "BerkeleyEnvironment::Flush: %s checkpoint\n", strFile);
                dbenv->txn_checkpoint(0, 0, 0);
                LogPrint(BCLog::WALLETDB, "BerkeleyEnvironment::Flush: %s detach\n", strFile);
                if (!fMockDb)
                    dbenv->lsn_reset(strFile.c_str(), 0);
                LogPrint(BCLog::WALLETDB, "BerkeleyEnvironment::Flush: %s closed\n", strFile);
                nRefCount = -1;
            } else {
                no_dbs_accessed = false;
            }
        }
        LogPrint(BCLog::WALLETDB, "BerkeleyEnvironment::Flush: Flush(%s)%s took %15dms\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started", GetTimeMillis() - nStart);
        if (fShutdown) {
            char** listp;
            if (no_dbs_accessed) {
                dbenv->log_archive(&listp, DB_ARCH_REMOVE);
                Close();
                if (!fMockDb) {
                    fs::remove_all(fs::path(strPath) / "database");
                }
            }
        }
    }
}

bool BerkeleyDatabase::PeriodicFlush()
{
    // Don't flush if we can't acquire the lock.
    TRY_LOCK(cs_db, lockDb);
    if (!lockDb) return false;

    // Don't flush if any databases are in use
    for (auto& it : env->m_databases) {
        if (it.second.get().m_refcount > 0) return false;
    }

    // Don't flush if there haven't been any batch writes for this database.
    if (m_refcount < 0) return false;

    LogPrint(BCLog::WALLETDB, "Flushing %s\n", strFile);
    int64_t nStart = GetTimeMillis();

    // Flush wallet file so it's self contained
    env->CloseDb(strFile);
    env->CheckpointLSN(strFile);
    m_refcount = -1;

    LogPrint(BCLog::WALLETDB, "Flushed %s %dms\n", strFile, GetTimeMillis() - nStart);

    return true;
}

bool BerkeleyDatabase::Backup(const std::string& strDest) const
{
    while (true)
    {
        {
            LOCK(cs_db);
            if (m_refcount <= 0)
            {
                // Flush log data to the dat file
                env->CloseDb(strFile);
                env->CheckpointLSN(strFile);

                // Copy wallet file
                fs::path pathSrc = env->Directory() / strFile;
                fs::path pathDest(strDest);
                if (fs::is_directory(pathDest))
                    pathDest /= strFile;

                try {
                    if (fs::exists(pathDest) && fs::equivalent(pathSrc, pathDest)) {
                        LogPrintf("cannot backup to wallet source file %s\n", pathDest.string());
                        return false;
                    }

                    fs::copy_file(pathSrc, pathDest, fs::copy_options::overwrite_existing);
                    LogPrintf("copied %s to %s\n", strFile, pathDest.string());
                    return true;
                } catch (const fs::filesystem_error& e) {
                    LogPrintf("error copying %s to %s - %s\n", strFile, pathDest.string(), fsbridge::get_filesystem_error_message(e));
                    return false;
                }
            }
        }
        UninterruptibleSleep(std::chrono::milliseconds{100});
    }
}

void BerkeleyDatabase::Flush()
{
    env->Flush(false);
}

void BerkeleyDatabase::Close()
{
    env->Flush(true);
}

void BerkeleyDatabase::ReloadDbEnv()
{
    env->ReloadDbEnv();
}

bool BerkeleyBatch::StartCursor()
{
    assert(!m_cursor);
    if (!pdb)
        return false;
    int ret = pdb->cursor(nullptr, &m_cursor, 0);
    return ret == 0;
}

bool BerkeleyBatch::ReadAtCursor(CDataStream& ssKey, CDataStream& ssValue, bool& complete)
{
    complete = false;
    if (m_cursor == nullptr) return false;
    // Read at cursor
    SafeDbt datKey;
    SafeDbt datValue;
    int ret = m_cursor->get(datKey, datValue, DB_NEXT);
    if (ret == DB_NOTFOUND) {
        complete = true;
    }
    if (ret != 0)
        return false;
    else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
        return false;

    // Convert to streams
    ssKey.SetType(SER_DISK);
    ssKey.clear();
    ssKey.write((char*)datKey.get_data(), datKey.get_size());
    ssValue.SetType(SER_DISK);
    ssValue.clear();
    ssValue.write((char*)datValue.get_data(), datValue.get_size());
    return true;
}

void BerkeleyBatch::CloseCursor()
{
    if (!m_cursor) return;
    m_cursor->close();
    m_cursor = nullptr;
}

bool BerkeleyBatch::TxnBegin()
{
    if (!pdb || activeTxn)
        return false;
    DbTxn* ptxn = env->TxnBegin();
    if (!ptxn)
        return false;
    activeTxn = ptxn;
    return true;
}

bool BerkeleyBatch::TxnCommit()
{
    if (!pdb || !activeTxn)
        return false;
    int ret = activeTxn->commit(0);
    activeTxn = nullptr;
    return (ret == 0);
}

bool BerkeleyBatch::TxnAbort()
{
    if (!pdb || !activeTxn)
        return false;
    int ret = activeTxn->abort();
    activeTxn = nullptr;
    return (ret == 0);
}

bool BerkeleyDatabaseSanityCheck()
{
    int major, minor;
    DbEnv::version(&major, &minor, nullptr);

    /* If the major version differs, or the minor version of library is *older*
     * than the header that was compiled against, flag an error.
     */
    if (major != DB_VERSION_MAJOR || minor < DB_VERSION_MINOR) {
        LogPrintf("BerkeleyDB database version conflict: header version is %d.%d, library version is %d.%d\n",
            DB_VERSION_MAJOR, DB_VERSION_MINOR, major, minor);
        return false;
    }

    return true;
}

std::string BerkeleyDatabaseVersion()
{
    return DbEnv::version(nullptr, nullptr, nullptr);
}

bool BerkeleyBatch::ReadKey(CDataStream&& key, CDataStream& value)
{
    if (!pdb)
        return false;

    SafeDbt datKey(key.data(), key.size());

    SafeDbt datValue;
    int ret = pdb->get(activeTxn, datKey, datValue, 0);
    if (ret == 0 && datValue.get_data() != nullptr) {
        value.write((char*)datValue.get_data(), datValue.get_size());
        return true;
    }
    return false;
}

bool BerkeleyBatch::WriteKey(CDataStream&& key, CDataStream&& value, bool overwrite)
{
    if (!pdb)
        return false;
    if (fReadOnly)
        assert(!"Write called on database in read-only mode");

    SafeDbt datKey(key.data(), key.size());

    SafeDbt datValue(value.data(), value.size());

    int ret = pdb->put(activeTxn, datKey, datValue, (overwrite ? 0 : DB_NOOVERWRITE));
    return (ret == 0);
}

bool BerkeleyBatch::EraseKey(CDataStream&& key)
{
    if (!pdb)
        return false;
    if (fReadOnly)
        assert(!"Erase called on database in read-only mode");

    SafeDbt datKey(key.data(), key.size());

    int ret = pdb->del(activeTxn, datKey, 0);
    return (ret == 0 || ret == DB_NOTFOUND);
}

bool BerkeleyBatch::HasKey(CDataStream&& key)
{
    if (!pdb)
        return false;

    SafeDbt datKey(key.data(), key.size());

    int ret = pdb->exists(activeTxn, datKey, 0);
    return ret == 0;
}

void BerkeleyDatabase::AddRef()
{
    LOCK(cs_db);
    if (m_refcount < 0) {
        m_refcount = 1;
    } else {
        m_refcount++;
    }
}

void BerkeleyDatabase::RemoveRef()
{
    LOCK(cs_db);
    m_refcount--;
    if (env) env->m_db_in_use.notify_all();
}

std::unique_ptr<DatabaseBatch> BerkeleyDatabase::MakeBatch(bool flush_on_close)
{
    return std::make_unique<BerkeleyBatch>(*this, false, flush_on_close);
}

std::unique_ptr<BerkeleyDatabase> MakeBerkeleyDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    fs::path data_file = BDBDataFile(path);
    std::unique_ptr<BerkeleyDatabase> db;
    {
        LOCK(cs_db); // Lock env.m_databases until insert in BerkeleyDatabase constructor
        std::string data_filename = data_file.filename().string();
        std::shared_ptr<BerkeleyEnvironment> env = GetBerkeleyEnv(data_file.parent_path());
        if (env->m_databases.count(data_filename)) {
            error = Untranslated(strprintf("Refusing to load database. Data file '%s' is already loaded.", (env->Directory() / data_filename).string()));
            status = DatabaseStatus::FAILED_ALREADY_LOADED;
            return nullptr;
        }
        db = std::make_unique<BerkeleyDatabase>(std::move(env), std::move(data_filename));
    }

    if (options.verify && !db->Verify(error)) {
        status = DatabaseStatus::FAILED_VERIFY;
        return nullptr;
    }

    status = DatabaseStatus::SUCCESS;
    return db;
}
