// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "db.h"

#include "addrman.h"
#include "fs.h"
#include "hash.h"
#include "protocol.h"
#include "util.h"
#include "utilstrencodings.h"

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
void CheckUniqueFileid(const CDBEnv& env, const std::string& filename, Db& db)
{
    if (env.IsMock()) return;

    u_int8_t fileid[DB_FILE_ID_LEN];
    int ret = db.get_mpf()->get_fileid(fileid);
    if (ret != 0) {
        throw std::runtime_error(strprintf("CDB: Can't open database %s (get_fileid failed with %d)", filename, ret));
    }

    for (const auto& item : env.mapDb) {
        u_int8_t item_fileid[DB_FILE_ID_LEN];
        if (item.second && item.second->get_mpf()->get_fileid(item_fileid) == 0 &&
            memcmp(fileid, item_fileid, sizeof(fileid)) == 0) {
            const char* item_filename = nullptr;
            item.second->get_dbname(&item_filename, nullptr);
            throw std::runtime_error(strprintf("CDB: Can't open database %s (duplicates fileid %s from %s)", filename,
                HexStr(std::begin(item_fileid), std::end(item_fileid)),
                item_filename ? item_filename : "(unknown database)"));
        }
    }
}
} // namespace

//
// CDB
//

CDBEnv bitdb;

void CDBEnv::EnvShutdown()
{
    if (!fDbEnvInit)
        return;

    fDbEnvInit = false;
    int ret = dbenv->close(0);
    if (ret != 0)
        LogPrintf("CDBEnv::EnvShutdown: Error %d shutting down database environment: %s\n", ret, DbEnv::strerror(ret));
    if (!fMockDb)
        DbEnv((u_int32_t)0).remove(strPath.c_str(), 0);
}

void CDBEnv::Reset()
{
    delete dbenv;
    dbenv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    fDbEnvInit = false;
    fMockDb = false;
}

CDBEnv::CDBEnv() : dbenv(nullptr)
{
    Reset();
}

CDBEnv::~CDBEnv()
{
    EnvShutdown();
    delete dbenv;
    dbenv = nullptr;
}

void CDBEnv::Close()
{
    EnvShutdown();
}

bool CDBEnv::Open(const fs::path& pathIn)
{
    if (fDbEnvInit)
        return true;

    boost::this_thread::interruption_point();

    strPath = pathIn.string();
    fs::path pathLogDir = pathIn / "database";
    TryCreateDirectories(pathLogDir);
    fs::path pathErrorFile = pathIn / "db.log";
    LogPrintf("CDBEnv::Open: LogDir=%s ErrorFile=%s\n", pathLogDir.string(), pathErrorFile.string());

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
        dbenv->close(0);
        return error("CDBEnv::Open: Error %d opening database environment: %s\n", ret, DbEnv::strerror(ret));
    }

    fDbEnvInit = true;
    fMockDb = false;
    return true;
}

void CDBEnv::MakeMock()
{
    if (fDbEnvInit)
        throw std::runtime_error("CDBEnv::MakeMock: Already initialized");

    boost::this_thread::interruption_point();

    LogPrint(BCLog::DB, "CDBEnv::MakeMock\n");

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
        throw std::runtime_error(strprintf("CDBEnv::MakeMock: Error %d opening database environment.", ret));

    fDbEnvInit = true;
    fMockDb = true;
}

CDBEnv::VerifyResult CDBEnv::Verify(const std::string& strFile, recoverFunc_type recoverFunc, std::string& out_backup_filename)
{
    LOCK(cs_db);
    assert(mapFileUseCount.count(strFile) == 0);

    Db db(dbenv, 0);
    int result = db.verify(strFile.c_str(), nullptr, nullptr, 0);
    if (result == 0)
        return VERIFY_OK;
    else if (recoverFunc == nullptr)
        return RECOVER_FAIL;

    // Try to recover:
    bool fRecovered = (*recoverFunc)(strFile, out_backup_filename);
    return (fRecovered ? RECOVER_OK : RECOVER_FAIL);
}

bool CDB::Recover(const std::string& filename, void *callbackDataIn, bool (*recoverKVcallback)(void* callbackData, CDataStream ssKey, CDataStream ssValue), std::string& newFilename)
{
    // Recovery procedure:
    // move wallet file to walletfilename.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to fresh wallet file
    // Set -rescan so any missing transactions will be
    // found.
    int64_t now = GetTime();
    newFilename = strprintf("%s.%d.bak", filename, now);

    int result = bitdb.dbenv->dbrename(nullptr, filename.c_str(), nullptr,
                                       newFilename.c_str(), DB_AUTO_COMMIT);
    if (result == 0)
        LogPrintf("Renamed %s to %s\n", filename, newFilename);
    else
    {
        LogPrintf("Failed to rename %s to %s\n", filename, newFilename);
        return false;
    }

    std::vector<CDBEnv::KeyValPair> salvagedData;
    bool fSuccess = bitdb.Salvage(newFilename, true, salvagedData);
    if (salvagedData.empty())
    {
        LogPrintf("Salvage(aggressive) found no records in %s.\n", newFilename);
        return false;
    }
    LogPrintf("Salvage(aggressive) found %u records\n", salvagedData.size());

    std::unique_ptr<Db> pdbCopy(new Db(bitdb.dbenv, 0));
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

    DbTxn* ptxn = bitdb.TxnBegin();
    for (CDBEnv::KeyValPair& row : salvagedData)
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

bool CDB::VerifyEnvironment(const std::string& walletFile, const fs::path& dataDir, std::string& errorStr)
{
    LogPrintf("Using BerkeleyDB version %s\n", DbEnv::version(0, 0, 0));
    LogPrintf("Using wallet %s\n", walletFile);

    // Wallet file must be a plain filename without a directory
    if (walletFile != fs::basename(walletFile) + fs::extension(walletFile))
    {
        errorStr = strprintf(_("Wallet %s resides outside data directory %s"), walletFile, dataDir.string());
        return false;
    }

    if (!bitdb.Open(dataDir))
    {
        // try moving the database env out of the way
        fs::path pathDatabase = dataDir / "database";
        fs::path pathDatabaseBak = dataDir / strprintf("database.%d.bak", GetTime());
        try {
            fs::rename(pathDatabase, pathDatabaseBak);
            LogPrintf("Moved old %s to %s. Retrying.\n", pathDatabase.string(), pathDatabaseBak.string());
        } catch (const fs::filesystem_error&) {
            // failure is ok (well, not really, but it's not worse than what we started with)
        }

        // try again
        if (!bitdb.Open(dataDir)) {
            // if it still fails, it probably means we can't even create the database env
            errorStr = strprintf(_("Error initializing wallet database environment %s!"), GetDataDir());
            return false;
        }
    }
    return true;
}

bool CDB::VerifyDatabaseFile(const std::string& walletFile, const fs::path& dataDir, std::string& warningStr, std::string& errorStr, CDBEnv::recoverFunc_type recoverFunc)
{
    if (fs::exists(dataDir / walletFile))
    {
        std::string backup_filename;
        CDBEnv::VerifyResult r = bitdb.Verify(walletFile, recoverFunc, backup_filename);
        if (r == CDBEnv::RECOVER_OK)
        {
            warningStr = strprintf(_("Warning: Wallet file corrupt, data salvaged!"
                                     " Original %s saved as %s in %s; if"
                                     " your balance or transactions are incorrect you should"
                                     " restore from a backup."),
                                   walletFile, backup_filename, dataDir);
        }
        if (r == CDBEnv::RECOVER_FAIL)
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

bool CDBEnv::Salvage(const std::string& strFile, bool fAggressive, std::vector<CDBEnv::KeyValPair>& vResult)
{
    LOCK(cs_db);
    assert(mapFileUseCount.count(strFile) == 0);

    u_int32_t flags = DB_SALVAGE;
    if (fAggressive)
        flags |= DB_AGGRESSIVE;

    std::stringstream strDump;

    Db db(dbenv, 0);
    int result = db.verify(strFile.c_str(), nullptr, &strDump, flags);
    if (result == DB_VERIFY_BAD) {
        LogPrintf("CDBEnv::Salvage: Database salvage found errors, all data may not be recoverable.\n");
        if (!fAggressive) {
            LogPrintf("CDBEnv::Salvage: Rerun with aggressive mode to ignore errors and continue.\n");
            return false;
        }
    }
    if (result != 0 && result != DB_VERIFY_BAD) {
        LogPrintf("CDBEnv::Salvage: Database salvage failed with result %d.\n", result);
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
                LogPrintf("CDBEnv::Salvage: WARNING: Number of keys in data does not match number of values.\n");
                break;
            }
            vResult.push_back(make_pair(ParseHex(keyHex), ParseHex(valueHex)));
        }
    }

    if (keyHex != DATA_END) {
        LogPrintf("CDBEnv::Salvage: WARNING: Unexpected end of file while reading salvage output.\n");
        return false;
    }

    return (result == 0);
}


void CDBEnv::CheckpointLSN(const std::string& strFile)
{
    dbenv->txn_checkpoint(0, 0, 0);
    if (fMockDb)
        return;
    dbenv->lsn_reset(strFile.c_str(), 0);
}


CDB::CDB(CWalletDBWrapper& dbw, const char* pszMode, bool fFlushOnCloseIn) : pdb(nullptr), activeTxn(nullptr)
{
    fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));
    fFlushOnClose = fFlushOnCloseIn;
    env = dbw.env;
    if (dbw.IsDummy()) {
        return;
    }
    const std::string &strFilename = dbw.strFile;

    bool fCreate = strchr(pszMode, 'c') != nullptr;
    unsigned int nFlags = DB_THREAD;
    if (fCreate)
        nFlags |= DB_CREATE;

    {
        LOCK(env->cs_db);
        if (!env->Open(GetDataDir()))
            throw std::runtime_error("CDB: Failed to open database environment.");

        pdb = env->mapDb[strFilename];
        if (pdb == nullptr) {
            int ret;
            std::unique_ptr<Db> pdb_temp(new Db(env->dbenv, 0));

            bool fMockDb = env->IsMock();
            if (fMockDb) {
                DbMpoolFile* mpf = pdb_temp->get_mpf();
                ret = mpf->set_flags(DB_MPOOL_NOFILE, 1);
                if (ret != 0) {
                    throw std::runtime_error(strprintf("CDB: Failed to configure for no temp file backing for database %s", strFilename));
                }
            }

            ret = pdb_temp->open(nullptr,                             // Txn pointer
                            fMockDb ? nullptr : strFilename.c_str(),  // Filename
                            fMockDb ? strFilename.c_str() : "main",   // Logical db name
                            DB_BTREE,                                 // Database type
                            nFlags,                                   // Flags
                            0);

            if (ret != 0) {
                throw std::runtime_error(strprintf("CDB: Error %d, can't open database %s", ret, strFilename));
            }
            CheckUniqueFileid(*env, strFilename, *pdb_temp);

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

void CDB::Flush()
{
    if (activeTxn)
        return;

    // Flush database activity from memory pool to disk log
    unsigned int nMinutes = 0;
    if (fReadOnly)
        nMinutes = 1;

    env->dbenv->txn_checkpoint(nMinutes ? gArgs.GetArg("-dblogsize", DEFAULT_WALLET_DBLOGSIZE) * 1024 : 0, nMinutes, 0);
}

void CWalletDBWrapper::IncrementUpdateCounter()
{
    ++nUpdateCounter;
}

void CDB::Close()
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
        LOCK(env->cs_db);
        --env->mapFileUseCount[strFile];
    }
}

void CDBEnv::CloseDb(const std::string& strFile)
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

bool CDB::Rewrite(CWalletDBWrapper& dbw, const char* pszSkip)
{
    if (dbw.IsDummy()) {
        return true;
    }
    CDBEnv *env = dbw.env;
    const std::string& strFile = dbw.strFile;
    while (true) {
        {
            LOCK(env->cs_db);
            if (!env->mapFileUseCount.count(strFile) || env->mapFileUseCount[strFile] == 0) {
                // Flush log data to the dat file
                env->CloseDb(strFile);
                env->CheckpointLSN(strFile);
                env->mapFileUseCount.erase(strFile);

                bool fSuccess = true;
                LogPrintf("CDB::Rewrite: Rewriting %s...\n", strFile);
                std::string strFileRes = strFile + ".rewrite";
                { // surround usage of db with extra {}
                    CDB db(dbw, "r");
                    Db* pdbCopy = new Db(env->dbenv, 0);

                    int ret = pdbCopy->open(nullptr,               // Txn pointer
                                            strFileRes.c_str(), // Filename
                                            "main",             // Logical db name
                                            DB_BTREE,           // Database type
                                            DB_CREATE,          // Flags
                                            0);
                    if (ret > 0) {
                        LogPrintf("CDB::Rewrite: Can't create database file %s\n", strFileRes);
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
                    delete pdbCopy;
                }
                if (fSuccess) {
                    Db dbA(env->dbenv, 0);
                    if (dbA.remove(strFile.c_str(), nullptr, 0))
                        fSuccess = false;
                    Db dbB(env->dbenv, 0);
                    if (dbB.rename(strFileRes.c_str(), nullptr, strFile.c_str(), 0))
                        fSuccess = false;
                }
                if (!fSuccess)
                    LogPrintf("CDB::Rewrite: Failed to rewrite database file %s\n", strFileRes);
                return fSuccess;
            }
        }
        MilliSleep(100);
    }
    return false;
}


void CDBEnv::Flush(bool fShutdown)
{
    int64_t nStart = GetTimeMillis();
    // Flush log data to the actual data file on all files that are not in use
    LogPrint(BCLog::DB, "CDBEnv::Flush: Flush(%s)%s\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started");
    if (!fDbEnvInit)
        return;
    {
        LOCK(cs_db);
        std::map<std::string, int>::iterator mi = mapFileUseCount.begin();
        while (mi != mapFileUseCount.end()) {
            std::string strFile = (*mi).first;
            int nRefCount = (*mi).second;
            LogPrint(BCLog::DB, "CDBEnv::Flush: Flushing %s (refcount = %d)...\n", strFile, nRefCount);
            if (nRefCount == 0) {
                // Move log data to the dat file
                CloseDb(strFile);
                LogPrint(BCLog::DB, "CDBEnv::Flush: %s checkpoint\n", strFile);
                dbenv->txn_checkpoint(0, 0, 0);
                LogPrint(BCLog::DB, "CDBEnv::Flush: %s detach\n", strFile);
                if (!fMockDb)
                    dbenv->lsn_reset(strFile.c_str(), 0);
                LogPrint(BCLog::DB, "CDBEnv::Flush: %s closed\n", strFile);
                mapFileUseCount.erase(mi++);
            } else
                mi++;
        }
        LogPrint(BCLog::DB, "CDBEnv::Flush: Flush(%s)%s took %15dms\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started", GetTimeMillis() - nStart);
        if (fShutdown) {
            char** listp;
            if (mapFileUseCount.empty()) {
                dbenv->log_archive(&listp, DB_ARCH_REMOVE);
                Close();
                if (!fMockDb)
                    fs::remove_all(fs::path(strPath) / "database");
            }
        }
    }
}

bool CDB::PeriodicFlush(CWalletDBWrapper& dbw)
{
    if (dbw.IsDummy()) {
        return true;
    }
    bool ret = false;
    CDBEnv *env = dbw.env;
    const std::string& strFile = dbw.strFile;
    TRY_LOCK(bitdb.cs_db,lockDb);
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

bool CWalletDBWrapper::Rewrite(const char* pszSkip)
{
    return CDB::Rewrite(*this, pszSkip);
}

bool CWalletDBWrapper::Backup(const std::string& strDest)
{
    if (IsDummy()) {
        return false;
    }
    while (true)
    {
        {
            LOCK(env->cs_db);
            if (!env->mapFileUseCount.count(strFile) || env->mapFileUseCount[strFile] == 0)
            {
                // Flush log data to the dat file
                env->CloseDb(strFile);
                env->CheckpointLSN(strFile);
                env->mapFileUseCount.erase(strFile);

                // Copy wallet file
                fs::path pathSrc = GetDataDir() / strFile;
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
                    LogPrintf("error copying %s to %s - %s\n", strFile, pathDest.string(), e.what());
                    return false;
                }
            }
        }
        MilliSleep(100);
    }
    return false;
}

void CWalletDBWrapper::Flush(bool shutdown)
{
    if (!IsDummy()) {
        env->Flush(shutdown);
    }
}
