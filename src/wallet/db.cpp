// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/db.h>

#include <addrman.h>
#include <hash.h>
#include <protocol.h>
#include <utilstrencodings.h>
#include <wallet/walletutil.h>

#include <stdint.h>

#ifndef WIN32
#include <sys/stat.h>
#endif

#include <boost/thread.hpp>

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
void CheckUniqueFileid(const BerkeleyEnvironment& env, const std::string& filename, Db& db)
{
    if (env.IsMock()) return;

    u_int8_t fileid[DB_FILE_ID_LEN];
    int ret = db.get_mpf()->get_fileid(fileid);
    if (ret != 0) {
        throw std::runtime_error(strprintf("BerkeleyBatch: Can't open database %s (get_fileid failed with %d)", filename, ret));
    }

    for (const auto& item : env.mapDb) {
        u_int8_t item_fileid[DB_FILE_ID_LEN];
        if (item.second && item.second->get_mpf()->get_fileid(item_fileid) == 0 &&
            memcmp(fileid, item_fileid, sizeof(fileid)) == 0) {
            const char* item_filename = nullptr;
            item.second->get_dbname(&item_filename, nullptr);
            throw std::runtime_error(strprintf("BerkeleyBatch: Can't open database %s (duplicates fileid %s from %s)", filename,
                HexStr(std::begin(item_fileid), std::end(item_fileid)),
                item_filename ? item_filename : "(unknown database)"));
        }
    }
}

CCriticalSection cs_db;
std::map<std::string, BerkeleyEnvironment> g_dbenvs GUARDED_BY(cs_db); //!< Map from directory name to open db environment.
} // namespace

BerkeleyEnvironment* GetWalletEnv(const fs::path& wallet_path, std::string& database_filename)
{
    fs::path env_directory;
    if (fs::is_regular_file(wallet_path)) {
        // Special case for backwards compatibility: if wallet path points to an
        // existing file, treat it as the path to a BDB data file in a parent
        // directory that also contains BDB log files.
        env_directory = wallet_path.parent_path();
        database_filename = wallet_path.filename().string();
    } else {
        // Normal case: Interpret wallet path as a directory path containing
        // data and log files.
        env_directory = wallet_path;
        database_filename = "wallet.dat";
    }
    LOCK(cs_db);
    // Note: An unused temporary BerkeleyEnvironment object may be created inside the
    // emplace function if the key already exists. This is a little inefficient,
    // but not a big concern since the map will be changed in the future to hold
    // pointers instead of objects, anyway.
    return &g_dbenvs.emplace(std::piecewise_construct, std::forward_as_tuple(env_directory.string()), std::forward_as_tuple(env_directory)).first->second;
}

//
// BerkeleyBatch
//

void BerkeleyEnvironment::Close()
{
    if (!fDbEnvInit)
        return;

    fDbEnvInit = false;

    for (auto& db : mapDb) {
        auto count = mapFileUseCount.find(db.first);
        assert(count == mapFileUseCount.end() || count->second == 0);
        if (db.second) {
            db.second->close(0);
            delete db.second;
            db.second = nullptr;
        }
    }

    int ret = dbenv->close(0);
    if (ret != 0)
        LogPrintf("BerkeleyEnvironment::Close: Error %d closing database environment: %s\n", ret, DbEnv::strerror(ret));
    if (!fMockDb)
        DbEnv((u_int32_t)0).remove(strPath.c_str(), 0);
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
    Close();
}

bool BerkeleyEnvironment::Open(bool retry)
{
    if (fDbEnvInit)
        return true;

    boost::this_thread::interruption_point();

    fs::path pathIn = strPath;
    TryCreateDirectories(pathIn);
    if (!LockDirectory(pathIn, ".walletlock")) {
        LogPrintf("Cannot obtain a lock on wallet directory %s. Another instance of bitcoin may be using it.\n", strPath);
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
        if (retry) {
            // try moving the database env out of the way
            fs::path pathDatabaseBak = pathIn / strprintf("database.%d.bak", GetTime());
            try {
                fs::rename(pathLogDir, pathDatabaseBak);
                LogPrintf("Moved old %s to %s. Retrying.\n", pathLogDir.string(), pathDatabaseBak.string());
            } catch (const fs::filesystem_error&) {
                // failure is ok (well, not really, but it's not worse than what we started with)
            }
            // try opening it again one more time
            if (!Open(false /* retry */)) {
                // if it still fails, it probably means we can't even create the database env
                return false;
            }
        } else {
            return false;
        }
    }

    fDbEnvInit = true;
    fMockDb = false;
    return true;
}

void BerkeleyEnvironment::MakeMock()
{
    if (fDbEnvInit)
        throw std::runtime_error("BerkeleyEnvironment::MakeMock: Already initialized");

    boost::this_thread::interruption_point();

    LogPrint(BCLog::DB, "BerkeleyEnvironment::MakeMock\n");

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
    if (ret > 0)
        throw std::runtime_error(strprintf("BerkeleyEnvironment::MakeMock: Error %d opening database environment.", ret));

    fDbEnvInit = true;
    fMockDb = true;
}

BerkeleyEnvironment::VerifyResult BerkeleyEnvironment::Verify(const std::string& strFile, recoverFunc_type recoverFunc, std::string& out_backup_filename)
{
    LOCK(cs_db);
    assert(mapFileUseCount.count(strFile) == 0);

    Db db(dbenv.get(), 0);
    int result = db.verify(strFile.c_str(), nullptr, nullptr, 0);
    if (result == 0)
        return VerifyResult::VERIFY_OK;
    else if (recoverFunc == nullptr)
        return VerifyResult::RECOVER_FAIL;

    // Try to recover:
    bool fRecovered = (*recoverFunc)(fs::path(strPath) / strFile, out_backup_filename);
    return (fRecovered ? VerifyResult::RECOVER_OK : VerifyResult::RECOVER_FAIL);
}

BerkeleyBatch::SafeDbt::SafeDbt(u_int32_t flags)
{
    m_dbt.set_flags(flags);
}

BerkeleyBatch::SafeDbt::SafeDbt(void *data, size_t size)
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

bool BerkeleyBatch::Recover(const fs::path& file_path, void *callbackDataIn, bool (*recoverKVcallback)(void* callbackData, CDataStream ssKey, CDataStream ssValue), std::string& newFilename)
{
    std::string filename;
    BerkeleyEnvironment* env = GetWalletEnv(file_path, filename);

    // Recovery procedure:
    // move wallet file to walletfilename.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to fresh wallet file
    // Set -rescan so any missing transactions will be
    // found.
    int64_t now = GetTime();
    newFilename = strprintf("%s.%d.bak", filename, now);

    int result = env->dbenv->dbrename(nullptr, filename.c_str(), nullptr,
                                       newFilename.c_str(), DB_AUTO_COMMIT);
    if (result == 0)
        LogPrintf("Renamed %s to %s\n", filename, newFilename);
    else
    {
        LogPrintf("Failed to rename %s to %s\n", filename, newFilename);
        return false;
    }

    std::vector<BerkeleyEnvironment::KeyValPair> salvagedData;
    bool fSuccess = env->Salvage(newFilename, true, salvagedData);
    if (salvagedData.empty())
    {
        LogPrintf("Salvage(aggressive) found no records in %s.\n", newFilename);
        return false;
    }
    LogPrintf("Salvage(aggressive) found %u records\n", salvagedData.size());

    std::unique_ptr<Db> pdbCopy = MakeUnique<Db>(env->dbenv.get(), 0);
    int ret = pdbCopy->open(nullptr,               // Txn pointer
                            filename.c_str(),   // Filename
                            "main",             // Logical db name
                            DB_BTREE,           // Database type
                            DB_CREATE,          // Flags
                            0);
    if (ret > 0) {
        LogPrintf("Cannot create database file %s\n", filename);
        pdbCopy->close(0);
        return false;
    }

    DbTxn* ptxn = env->TxnBegin();
    for (BerkeleyEnvironment::KeyValPair& row : salvagedData)
    {
        if (recoverKVcallback)
        {
            CDataStream ssKey(row.first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(row.second, SER_DISK, CLIENT_VERSION);
            if (!(*recoverKVcallback)(callbackDataIn, ssKey, ssValue))
                continue;
        }
        Dbt datKey(&row.first[0], row.first.size());
        Dbt datValue(&row.second[0], row.second.size());
        int ret2 = pdbCopy->put(ptxn, &datKey, &datValue, DB_NOOVERWRITE);
        if (ret2 > 0)
            fSuccess = false;
    }
    ptxn->commit(0);
    pdbCopy->close(0);

    return fSuccess;
}

bool BerkeleyBatch::VerifyEnvironment(const fs::path& file_path, std::string& errorStr)
{
    std::string walletFile;
    BerkeleyEnvironment* env = GetWalletEnv(file_path, walletFile);
    fs::path walletDir = env->Directory();

    LogPrintf("Using BerkeleyDB version %s\n", DbEnv::version(0, 0, 0));
    LogPrintf("Using wallet %s\n", walletFile);

    // Wallet file must be a plain filename without a directory
    if (walletFile != fs::basename(walletFile) + fs::extension(walletFile))
    {
        errorStr = strprintf(_("Wallet %s resides outside wallet directory %s"), walletFile, walletDir.string());
        return false;
    }

    if (!env->Open(true /* retry */)) {
        errorStr = strprintf(_("Error initializing wallet database environment %s!"), walletDir);
        return false;
    }

    return true;
}

bool BerkeleyBatch::VerifyDatabaseFile(const fs::path& file_path, std::string& warningStr, std::string& errorStr, BerkeleyEnvironment::recoverFunc_type recoverFunc)
{
    std::string walletFile;
    BerkeleyEnvironment* env = GetWalletEnv(file_path, walletFile);
    fs::path walletDir = env->Directory();

    if (fs::exists(walletDir / walletFile))
    {
        std::string backup_filename;
        BerkeleyEnvironment::VerifyResult r = env->Verify(walletFile, recoverFunc, backup_filename);
        if (r == BerkeleyEnvironment::VerifyResult::RECOVER_OK)
        {
            warningStr = strprintf(_("Warning: Wallet file corrupt, data salvaged!"
                                     " Original %s saved as %s in %s; if"
                                     " your balance or transactions are incorrect you should"
                                     " restore from a backup."),
                                   walletFile, backup_filename, walletDir);
        }
        if (r == BerkeleyEnvironment::VerifyResult::RECOVER_FAIL)
        {
            errorStr = strprintf(_("%s corrupt, salvage failed"), walletFile);
            return false;
        }
    }
    // also return true if files does not exists
    return true;
}

/* End of headers, beginning of key/value data */
static const char *HEADER_END = "HEADER=END";
/* End of key/value data */
static const char *DATA_END = "DATA=END";

bool BerkeleyEnvironment::Salvage(const std::string& strFile, bool fAggressive, std::vector<BerkeleyEnvironment::KeyValPair>& vResult)
{
    LOCK(cs_db);
    assert(mapFileUseCount.count(strFile) == 0);

    u_int32_t flags = DB_SALVAGE;
    if (fAggressive)
        flags |= DB_AGGRESSIVE;

    std::stringstream strDump;

    Db db(dbenv.get(), 0);
    int result = db.verify(strFile.c_str(), nullptr, &strDump, flags);
    if (result == DB_VERIFY_BAD) {
        LogPrintf("BerkeleyEnvironment::Salvage: Database salvage found errors, all data may not be recoverable.\n");
        if (!fAggressive) {
            LogPrintf("BerkeleyEnvironment::Salvage: Rerun with aggressive mode to ignore errors and continue.\n");
            return false;
        }
    }
    if (result != 0 && result != DB_VERIFY_BAD) {
        LogPrintf("BerkeleyEnvironment::Salvage: Database salvage failed with result %d.\n", result);
        return false;
    }

    // Format of bdb dump is ascii lines:
    // header lines...
    // HEADER=END
    //  hexadecimal key
    //  hexadecimal value
    //  ... repeated
    // DATA=END

    std::string strLine;
    while (!strDump.eof() && strLine != HEADER_END)
        getline(strDump, strLine); // Skip past header

    std::string keyHex, valueHex;
    while (!strDump.eof() && keyHex != DATA_END) {
        getline(strDump, keyHex);
        if (keyHex != DATA_END) {
            if (strDump.eof())
                break;
            getline(strDump, valueHex);
            if (valueHex == DATA_END) {
                LogPrintf("BerkeleyEnvironment::Salvage: WARNING: Number of keys in data does not match number of values.\n");
                break;
            }
            vResult.push_back(make_pair(ParseHex(keyHex), ParseHex(valueHex)));
        }
    }

    if (keyHex != DATA_END) {
        LogPrintf("BerkeleyEnvironment::Salvage: WARNING: Unexpected end of file while reading salvage output.\n");
        return false;
    }

    return (result == 0);
}


void BerkeleyEnvironment::CheckpointLSN(const std::string& strFile)
{
    dbenv->txn_checkpoint(0, 0, 0);
    if (fMockDb)
        return;
    dbenv->lsn_reset(strFile.c_str(), 0);
}


BerkeleyBatch::BerkeleyBatch(BerkeleyDatabase& database, const char* pszMode, bool fFlushOnCloseIn) : pdb(nullptr), activeTxn(nullptr)
{
    fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));
    fFlushOnClose = fFlushOnCloseIn;
    env = database.env;
    if (database.IsDummy()) {
        return;
    }
    const std::string &strFilename = database.strFile;

    bool fCreate = strchr(pszMode, 'c') != nullptr;
    unsigned int nFlags = DB_THREAD;
    if (fCreate)
        nFlags |= DB_CREATE;

    {
        LOCK(cs_db);
        if (!env->Open(false /* retry */))
            throw std::runtime_error("BerkeleyBatch: Failed to open database environment.");

        pdb = env->mapDb[strFilename];
        if (pdb == nullptr) {
            int ret;
            std::unique_ptr<Db> pdb_temp = MakeUnique<Db>(env->dbenv.get(), 0);

            bool fMockDb = env->IsMock();
            if (fMockDb) {
                DbMpoolFile* mpf = pdb_temp->get_mpf();
                ret = mpf->set_flags(DB_MPOOL_NOFILE, 1);
                if (ret != 0) {
                    throw std::runtime_error(strprintf("BerkeleyBatch: Failed to configure for no temp file backing for database %s", strFilename));
                }
            }

            ret = pdb_temp->open(nullptr,                             // Txn pointer
                            fMockDb ? nullptr : strFilename.c_str(),  // Filename
                            fMockDb ? strFilename.c_str() : "main",   // Logical db name
                            DB_BTREE,                                 // Database type
                            nFlags,                                   // Flags
                            0);

            if (ret != 0) {
                throw std::runtime_error(strprintf("BerkeleyBatch: Error %d, can't open database %s", ret, strFilename));
            }

            // Call CheckUniqueFileid on the containing BDB environment to
            // avoid BDB data consistency bugs that happen when different data
            // files in the same environment have the same fileid.
            //
            // Also call CheckUniqueFileid on all the other g_dbenvs to prevent
            // bitcoin from opening the same data file through another
            // environment when the file is referenced through equivalent but
            // not obviously identical symlinked or hard linked or bind mounted
            // paths. In the future a more relaxed check for equal inode and
            // device ids could be done instead, which would allow opening
            // different backup copies of a wallet at the same time. Maybe even
            // more ideally, an exclusive lock for accessing the database could
            // be implemented, so no equality checks are needed at all. (Newer
            // versions of BDB have an set_lk_exclusive method for this
            // purpose, but the older version we use does not.)
            for (const auto& env : g_dbenvs) {
                CheckUniqueFileid(env.second, strFilename, *pdb_temp);
            }

            pdb = pdb_temp.release();
            env->mapDb[strFilename] = pdb;

            if (fCreate && !Exists(std::string("version"))) {
                bool fTmp = fReadOnly;
                fReadOnly = false;
                WriteVersion(CLIENT_VERSION);
                fReadOnly = fTmp;
            }
        }
        ++env->mapFileUseCount[strFilename];
        strFile = strFilename;
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

    env->dbenv->txn_checkpoint(nMinutes ? gArgs.GetArg("-dblogsize", DEFAULT_WALLET_DBLOGSIZE) * 1024 : 0, nMinutes, 0);
}

void BerkeleyDatabase::IncrementUpdateCounter()
{
    ++nUpdateCounter;
}

void BerkeleyBatch::Close()
{
    if (!pdb)
        return;
    if (activeTxn)
        activeTxn->abort();
    activeTxn = nullptr;
    pdb = nullptr;

    if (fFlushOnClose)
        Flush();

    {
        LOCK(cs_db);
        --env->mapFileUseCount[strFile];
    }
    env->m_db_in_use.notify_all();
}

void BerkeleyEnvironment::CloseDb(const std::string& strFile)
{
    {
        LOCK(cs_db);
        if (mapDb[strFile] != nullptr) {
            // Close the database handle
            Db* pdb = mapDb[strFile];
            pdb->close(0);
            delete pdb;
            mapDb[strFile] = nullptr;
        }
    }
}

void BerkeleyEnvironment::ReloadDbEnv()
{
    // Make sure that no Db's are in use
    AssertLockNotHeld(cs_db);
    std::unique_lock<CCriticalSection> lock(cs_db);
    m_db_in_use.wait(lock, [this](){
        for (auto& count : mapFileUseCount) {
            if (count.second > 0) return false;
        }
        return true;
    });

    std::vector<std::string> filenames;
    for (auto it : mapDb) {
        filenames.push_back(it.first);
    }
    // Close the individual Db's
    for (const std::string& filename : filenames) {
        CloseDb(filename);
    }
    // Reset the environment
    Flush(true); // This will flush and close the environment
    Reset();
    Open(true);
}

bool BerkeleyBatch::Rewrite(BerkeleyDatabase& database, const char* pszSkip)
{
    if (database.IsDummy()) {
        return true;
    }
    BerkeleyEnvironment *env = database.env;
    const std::string& strFile = database.strFile;
    while (true) {
        {
            LOCK(cs_db);
            if (!env->mapFileUseCount.count(strFile) || env->mapFileUseCount[strFile] == 0) {
                // Flush log data to the dat file
                env->CloseDb(strFile);
                env->CheckpointLSN(strFile);
                env->mapFileUseCount.erase(strFile);

                bool fSuccess = true;
                LogPrintf("BerkeleyBatch::Rewrite: Rewriting %s...\n", strFile);
                std::string strFileRes = strFile + ".rewrite";
                { // surround usage of db with extra {}
                    BerkeleyBatch db(database, "r");
                    std::unique_ptr<Db> pdbCopy = MakeUnique<Db>(env->dbenv.get(), 0);

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

                    Dbc* pcursor = db.GetCursor();
                    if (pcursor)
                        while (fSuccess) {
                            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
                            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
                            int ret1 = db.ReadAtCursor(pcursor, ssKey, ssValue);
                            if (ret1 == DB_NOTFOUND) {
                                pcursor->close();
                                break;
                            } else if (ret1 != 0) {
                                pcursor->close();
                                fSuccess = false;
                                break;
                            }
                            if (pszSkip &&
                                strncmp(ssKey.data(), pszSkip, std::min(ssKey.size(), strlen(pszSkip))) == 0)
                                continue;
                            if (strncmp(ssKey.data(), "\x07version", 8) == 0) {
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
        MilliSleep(100);
    }
}


void BerkeleyEnvironment::Flush(bool fShutdown)
{
    int64_t nStart = GetTimeMillis();
    // Flush log data to the actual data file on all files that are not in use
    LogPrint(BCLog::DB, "BerkeleyEnvironment::Flush: Flush(%s)%s\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started");
    if (!fDbEnvInit)
        return;
    {
        LOCK(cs_db);
        std::map<std::string, int>::iterator mi = mapFileUseCount.begin();
        while (mi != mapFileUseCount.end()) {
            std::string strFile = (*mi).first;
            int nRefCount = (*mi).second;
            LogPrint(BCLog::DB, "BerkeleyEnvironment::Flush: Flushing %s (refcount = %d)...\n", strFile, nRefCount);
            if (nRefCount == 0) {
                // Move log data to the dat file
                CloseDb(strFile);
                LogPrint(BCLog::DB, "BerkeleyEnvironment::Flush: %s checkpoint\n", strFile);
                dbenv->txn_checkpoint(0, 0, 0);
                LogPrint(BCLog::DB, "BerkeleyEnvironment::Flush: %s detach\n", strFile);
                if (!fMockDb)
                    dbenv->lsn_reset(strFile.c_str(), 0);
                LogPrint(BCLog::DB, "BerkeleyEnvironment::Flush: %s closed\n", strFile);
                mapFileUseCount.erase(mi++);
            } else
                mi++;
        }
        LogPrint(BCLog::DB, "BerkeleyEnvironment::Flush: Flush(%s)%s took %15dms\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started", GetTimeMillis() - nStart);
        if (fShutdown) {
            char** listp;
            if (mapFileUseCount.empty()) {
                dbenv->log_archive(&listp, DB_ARCH_REMOVE);
                Close();
                if (!fMockDb) {
                    fs::remove_all(fs::path(strPath) / "database");
                }
            }
        }
    }
}

bool BerkeleyBatch::PeriodicFlush(BerkeleyDatabase& database)
{
    if (database.IsDummy()) {
        return true;
    }
    bool ret = false;
    BerkeleyEnvironment *env = database.env;
    const std::string& strFile = database.strFile;
    TRY_LOCK(cs_db, lockDb);
    if (lockDb)
    {
        // Don't do this if any databases are in use
        int nRefCount = 0;
        std::map<std::string, int>::iterator mit = env->mapFileUseCount.begin();
        while (mit != env->mapFileUseCount.end())
        {
            nRefCount += (*mit).second;
            mit++;
        }

        if (nRefCount == 0)
        {
            boost::this_thread::interruption_point();
            std::map<std::string, int>::iterator mi = env->mapFileUseCount.find(strFile);
            if (mi != env->mapFileUseCount.end())
            {
                LogPrint(BCLog::DB, "Flushing %s\n", strFile);
                int64_t nStart = GetTimeMillis();

                // Flush wallet file so it's self contained
                env->CloseDb(strFile);
                env->CheckpointLSN(strFile);

                env->mapFileUseCount.erase(mi++);
                LogPrint(BCLog::DB, "Flushed %s %dms\n", strFile, GetTimeMillis() - nStart);
                ret = true;
            }
        }
    }

    return ret;
}

bool BerkeleyDatabase::Rewrite(const char* pszSkip)
{
    return BerkeleyBatch::Rewrite(*this, pszSkip);
}

bool BerkeleyDatabase::Backup(const std::string& strDest)
{
    if (IsDummy()) {
        return false;
    }
    while (true)
    {
        {
            LOCK(cs_db);
            if (!env->mapFileUseCount.count(strFile) || env->mapFileUseCount[strFile] == 0)
            {
                // Flush log data to the dat file
                env->CloseDb(strFile);
                env->CheckpointLSN(strFile);
                env->mapFileUseCount.erase(strFile);

                // Copy wallet file
                fs::path pathSrc = env->Directory() / strFile;
                fs::path pathDest(strDest);
                if (fs::is_directory(pathDest))
                    pathDest /= strFile;

                try {
                    if (fs::equivalent(pathSrc, pathDest)) {
                        LogPrintf("cannot backup to wallet source file %s\n", pathDest.string());
                        return false;
                    }

                    fs::copy_file(pathSrc, pathDest, fs::copy_option::overwrite_if_exists);
                    LogPrintf("copied %s to %s\n", strFile, pathDest.string());
                    return true;
                } catch (const fs::filesystem_error& e) {
                    LogPrintf("error copying %s to %s - %s\n", strFile, pathDest.string(), fsbridge::get_filesystem_error_message(e));
                    return false;
                }
            }
        }
        MilliSleep(100);
    }
}

void BerkeleyDatabase::Flush(bool shutdown)
{
    if (!IsDummy()) {
        env->Flush(shutdown);
        if (shutdown) {
            LOCK(cs_db);
            g_dbenvs.erase(env->Directory().string());
            env = nullptr;
        }
    }
}

void BerkeleyDatabase::ReloadDbEnv()
{
    if (!IsDummy()) {
        env->ReloadDbEnv();
    }
}
