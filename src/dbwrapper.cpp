// Copyright (c) 2012-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbwrapper.h"

#include "util.h"
#include "random.h"

#include <boost/filesystem.hpp>

#include <memenv.h>
#include <stdio.h>
#include <stdint.h>

static const char *sql_db_init[] = {
    "CREATE TABLE tab(k BLOB PRIMARY KEY, v BLOB)",
};

static const char *sql_stmt_text[] = {
    "REPLACE INTO tab(k, v) VALUES(:k, :v)",
    "SELECT v FROM tab WHERE k = :k",
    "DELETE FROM tab WHERE k = :k",
    "BEGIN TRANSACTION",
    "COMMIT TRANSACTION",
    "SELECT COUNT(*) FROM tab",
    "SELECT k, v FROM tab WHERE k >= :k ORDER BY k",
};

CDBWrapper::CDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate)
{
    std::string fp;
    const char *filename = NULL;
    bool need_init = false;

    if (fMemory) {
        filename = ":memory:";
        need_init = true;
    } else {
        fp = path.string();
        fp = fp + "/db";

        if (fWipe) {
            LogPrintf("Wiping DB %s\n", fp);
            boost::filesystem::remove(fp);
        }
        TryCreateDirectory(path);
        LogPrintf("Opening DB in %s\n", path.string());

        filename = fp.c_str();
    }

    // open existing sqlite db
    int orc = sqlite3_open_v2(filename, &psql, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, NULL);

    // if open-existing failed, attempt to create new db
    if (orc != SQLITE_OK) {
        orc = sqlite3_open_v2(filename, &psql, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
        if (orc != SQLITE_OK)
            throw(dbwrapper_error("DB open failed"));

        need_init = true;
    }

    // initialize new db
    if (need_init) {
        for (unsigned int i = 0; i < ARRAYLEN(sql_db_init); i++)
            if (sqlite3_exec(psql, sql_db_init[i], NULL, NULL, NULL) != SQLITE_OK)
                throw(dbwrapper_error("DB one-time setup failed"));
    }

    // pre-compile SQL statements
    for (unsigned sqlidx = 0; sqlidx < ARRAYLEN(sql_stmt_text); sqlidx++) {
        sqlite3_stmt *pstmt = NULL;

        int src = sqlite3_prepare_v2(psql,
                               sql_stmt_text[sqlidx],
                               strlen(sql_stmt_text[sqlidx]),
                               &pstmt, NULL);
        if (src != SQLITE_OK) {
            char numstr[32];
            snprintf(numstr, sizeof(numstr), "(%d)", src);
            throw(dbwrapper_error("DB compile failed" + std::string(numstr) + ": " + std::string(sql_stmt_text[sqlidx])));
        }

        stmts.push_back(pstmt);
    }

    LogPrintf("Opened DB successfully\n");

    // The base-case obfuscation key, which is a noop.
    obfuscate_key = std::vector<unsigned char>(OBFUSCATE_KEY_NUM_BYTES, '\000');

    bool key_exists = Read(OBFUSCATE_KEY_KEY, obfuscate_key);

    if (!key_exists && obfuscate && IsEmpty()) {
        // Initialize non-degenerate obfuscation if it won't upset
        // existing, non-obfuscated data.
        std::vector<unsigned char> new_key = CreateObfuscateKey();

        // Write `new_key` so we don't obfuscate the key with itself
        Write(OBFUSCATE_KEY_KEY, new_key);
        obfuscate_key = new_key;

        LogPrintf("Wrote new obfuscate key for %s: %s\n", path.string(), GetObfuscateKeyHex());
    }

    LogPrintf("Using obfuscation key for %s: %s\n", path.string(), GetObfuscateKeyHex());
}

CDBWrapper::~CDBWrapper()
{
    for (unsigned int i = 0; i < stmts.size(); i++)
        sqlite3_finalize(stmts[i]);
    stmts.clear();

    sqlite3_close(psql);
    psql = NULL;
}

bool CDBWrapper::WriteBatch(CDBBatch& batch, bool fSync) throw(dbwrapper_error)
{
    // begin DB transaction
    int src = sqlite3_step(stmts[DBW_BEGIN]);
    int rrc = sqlite3_reset(stmts[DBW_BEGIN]);

    if ((src != SQLITE_DONE) || (rrc != SQLITE_OK))
        throw dbwrapper_error("DB cannot begin transaction");

    for (unsigned int wr = 0; wr < batch.entries.size(); wr++) {
        BatchEntry& be = batch.entries[wr];
        sqlite3_stmt *stmt = NULL;

        // get the SQL stmt to exec
        if (be.isErase)
            stmt = stmts[DBW_DELETE];
        else
            stmt = stmts[DBW_PUT];

        // bind column 0: key
        if (sqlite3_bind_blob(stmt, 1, be.key.c_str(), be.key.size(),
                              SQLITE_TRANSIENT) != SQLITE_OK)
            throw dbwrapper_error("DB cannot bind blob key");

        // if inserting, bind column 1: value
        if (!be.isErase)
            if (sqlite3_bind_blob(stmt, 2, be.value.c_str(), be.value.size(),
                                  SQLITE_TRANSIENT) != SQLITE_OK)
                throw dbwrapper_error("DB cannot bind blob value");

        // exec INSERT/DELETE
        src = sqlite3_step(stmt);
        rrc = sqlite3_reset(stmt);

        if ((src != SQLITE_DONE) || (rrc != SQLITE_OK))
            throw dbwrapper_error("DB failed update");
    }

    src = sqlite3_step(stmts[DBW_COMMIT]);
    rrc = sqlite3_reset(stmts[DBW_COMMIT]);

    if ((src != SQLITE_DONE) || (rrc != SQLITE_OK))
        throw dbwrapper_error("DB cannot commit transaction");

    return true;
}

// Prefixed with null character to avoid collisions with other keys
//
// We must use a string constructor which specifies length so that we copy
// past the null-terminator.
const std::string CDBWrapper::OBFUSCATE_KEY_KEY("\000obfuscate_key", 14);

const unsigned int CDBWrapper::OBFUSCATE_KEY_NUM_BYTES = 8;

/**
 * Returns a string (consisting of 8 random bytes) suitable for use as an
 * obfuscating XOR key.
 */
std::vector<unsigned char> CDBWrapper::CreateObfuscateKey() const
{
    unsigned char buff[OBFUSCATE_KEY_NUM_BYTES];
    GetRandBytes(buff, OBFUSCATE_KEY_NUM_BYTES);
    return std::vector<unsigned char>(&buff[0], &buff[OBFUSCATE_KEY_NUM_BYTES]);

}

bool CDBWrapper::IsEmpty()
{
    int src = sqlite3_step(stmts[DBW_GET_ALL_COUNT]);
    if (src != SQLITE_ROW) {
        sqlite3_reset(stmts[DBW_GET_ALL_COUNT]);
        throw dbwrapper_error("DB read count failure");
    }

    int64_t count = sqlite3_column_int64(stmts[DBW_GET_ALL_COUNT], 0);
    sqlite3_reset(stmts[DBW_GET_ALL_COUNT]);

    return (count == 0);
}

const std::vector<unsigned char>& CDBWrapper::GetObfuscateKey() const
{
    return obfuscate_key;
}

std::string CDBWrapper::GetObfuscateKeyHex() const
{
    return HexStr(obfuscate_key);
}

CDBIterator::~CDBIterator() { sqlite3_reset(pstmt); }

