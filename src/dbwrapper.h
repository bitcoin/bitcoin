// Copyright (c) 2012-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DBWRAPPER_H
#define BITCOIN_DBWRAPPER_H

#include "clientversion.h"
#include "serialize.h"
#include "streams.h"
#include "util.h"
#include "utilstrencodings.h"
#include "version.h"

#include <boost/filesystem/path.hpp>

#include <sqlite3.h>

enum {
        DBW_PUT,
        DBW_GET,
        DBW_DELETE,
        DBW_BEGIN,
        DBW_COMMIT,
        DBW_GET_ALL_COUNT,
        DBW_SEEK_SORTED,
};

class dbwrapper_error : public std::runtime_error
{
public:
    dbwrapper_error(const std::string& msg) : std::runtime_error(msg) {}
};

class BatchEntry {
public:
    bool           isErase;
    std::string    key;
    std::string    value;

    BatchEntry(bool isErase_, const std::string& key_,
               const std::string& value_) : isErase(isErase_), key(key_), value(value_) { }
    BatchEntry(bool isErase_, const std::string& key_) : isErase(isErase_), key(key_) { }
};

/** Batch of changes queued to be written to a CDBWrapper */
class CDBBatch
{
    friend class CDBWrapper;

private:
    std::vector<BatchEntry> entries;
    const std::vector<unsigned char> *obfuscate_key;

public:
    /**
     * @param[in] obfuscate_key    If passed, XOR data with this key.
     */
    CDBBatch(const std::vector<unsigned char> *obfuscate_key) : obfuscate_key(obfuscate_key) { };

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;

        std::string rawKey(&ssKey[0], ssKey.size());

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(ssValue.GetSerializeSize(value));
        ssValue << value;
        ssValue.Xor(*obfuscate_key);

        std::string rawValue(&ssValue[0], ssValue.size());

        BatchEntry be(false, rawKey, rawValue);
        entries.push_back(be);
    }

    template <typename K>
    void Erase(const K& key)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;

        std::string rawKey(&ssKey[0], ssKey.size());

        BatchEntry be(true, rawKey);
        entries.push_back(be);
    }
};

class CDBIterator
{
private:
    sqlite3_stmt *pstmt;
    const std::vector<unsigned char> *obfuscate_key;
    bool valid;

public:

    /**
     * @param[in] piterIn          The original leveldb iterator.
     * @param[in] obfuscate_key    If passed, XOR data with this key.
     */
    CDBIterator(sqlite3_stmt *pstmt_, const std::vector<unsigned char>* obfuscate_key) :
        pstmt(pstmt_), obfuscate_key(obfuscate_key), valid(false) { };
    ~CDBIterator();

    bool Valid() { return valid; };

    template<typename K> void Seek(const K& key) {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;

        valid = false;

        int rrc = sqlite3_reset(pstmt);
        if (rrc != SQLITE_OK)
            throw dbwrapper_error("DB reset failure, err " + itostr(rrc));

        std::string rawKey(&ssKey[0], ssKey.size());

        if (sqlite3_bind_blob(pstmt, 1, rawKey.c_str(), rawKey.size(),
                              SQLITE_TRANSIENT) != SQLITE_OK)
            throw dbwrapper_error("DB seek failure");

        int src = sqlite3_step(pstmt);
        if (src == SQLITE_ROW)
            valid = true;
    }

    void Next() {
        if (!valid)
            return;

        int src = sqlite3_step(pstmt);
        if (src == SQLITE_ROW)
            valid = true;
        else
            valid = false;
    }

    template<typename K> bool GetKey(K& key) {
        if (!valid)
            return false;

        std::string strCol((const char *) sqlite3_column_blob(pstmt, 0),
                           (size_t) sqlite3_column_bytes(pstmt, 0));
        try {
            CDataStream ssKey(&strCol[0], &strCol[0] + strCol.size(), SER_DISK, CLIENT_VERSION);
            ssKey >> key;
        } catch(std::exception &e) {
            return false;
        }
        return true;
    }

    unsigned int GetKeySize() {
        if (!valid)
            return 0;
        return sqlite3_column_bytes(pstmt, 0);
    }

    template<typename V> bool GetValue(V& value) {
        if (!valid)
            return false;

        std::string strCol((const char *) sqlite3_column_blob(pstmt, 1),
                           (size_t) sqlite3_column_bytes(pstmt, 1));
        try {
            CDataStream ssValue(&strCol[0], &strCol[0] + strCol.size(), SER_DISK, CLIENT_VERSION);
            ssValue.Xor(*obfuscate_key);
            ssValue >> value;
        } catch(std::exception &e) {
            return false;
        }
        return true;
    }

    unsigned int GetValueSize() {
        if (!valid)
            return 0;
        return sqlite3_column_bytes(pstmt, 1);
    }

};

class CDBWrapper
{
private:
    //! the database itself
    sqlite3* psql;

    std::vector<sqlite3_stmt*> stmts;

    //! a key used for optional XOR-obfuscation of the database
    std::vector<unsigned char> obfuscate_key;

    //! the key under which the obfuscation key is stored
    static const std::string OBFUSCATE_KEY_KEY;

    //! the length of the obfuscate key in number of bytes
    static const unsigned int OBFUSCATE_KEY_NUM_BYTES;

    std::vector<unsigned char> CreateObfuscateKey() const;

public:
    /**
     * @param[in] path        Location in the filesystem where leveldb data will be stored.
     * @param[in] nCacheSize  Configures various leveldb cache settings.
     * @param[in] fMemory     If true, use leveldb's memory environment.
     * @param[in] fWipe       If true, remove all existing data.
     * @param[in] obfuscate   If true, store data obfuscated via simple XOR. If false, XOR
     *                        with a zero'd byte array.
     */
    CDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory = false, bool fWipe = false, bool obfuscate = false);
    ~CDBWrapper();

    template <typename K, typename V>
    bool Read(const K& key, V& value) const throw(dbwrapper_error)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;

        int brc = sqlite3_bind_blob(stmts[DBW_GET], 1, &ssKey[0], ssKey.size(),
                                    SQLITE_TRANSIENT);
        if (brc != SQLITE_OK)
            throw dbwrapper_error("DB find-read failure, err " + itostr(brc));

        int src = sqlite3_step(stmts[DBW_GET]);
        if (src != SQLITE_ROW) {
            sqlite3_reset(stmts[DBW_GET]);

            if (src == SQLITE_DONE)                // success; 0 results
                return false;

            // exceptional error
            throw dbwrapper_error("DB read failure, err " + itostr(src));
        }

        // SQLITE_ROW - we have a result
        int col_size = sqlite3_column_bytes(stmts[DBW_GET], 0);
        std::string strValue((const char *) sqlite3_column_blob(stmts[DBW_GET], 0), col_size);

        sqlite3_reset(stmts[DBW_GET]);

        try {
            CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue.Xor(obfuscate_key);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template <typename K, typename V>
    bool Write(const K& key, const V& value, bool fSync = false) throw(dbwrapper_error)
    {
        CDBBatch batch(&obfuscate_key);
        batch.Write(key, value);
        return WriteBatch(batch, fSync);
    }

    template <typename K>
    bool Exists(const K& key) const throw(dbwrapper_error)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;

        if (sqlite3_bind_blob(stmts[DBW_GET], 1, &ssKey[0], ssKey.size(),
                              SQLITE_TRANSIENT) != SQLITE_OK)
            throw dbwrapper_error("DB find-exists failure");

        int src = sqlite3_step(stmts[DBW_GET]);
        sqlite3_reset(stmts[DBW_GET]);
        if (src == SQLITE_DONE)                // zero results; not found
            return false;
        else if (src == SQLITE_ROW)
            return true;
        else
            throw dbwrapper_error("DB read failure, err " + itostr(src));
    }

    template <typename K>
    bool Erase(const K& key, bool fSync = false) throw(dbwrapper_error)
    {
        CDBBatch batch(&obfuscate_key);
        batch.Erase(key);
        return WriteBatch(batch, fSync);
    }

    bool WriteBatch(CDBBatch& batch, bool fSync = false) throw(dbwrapper_error);

    // not available for LevelDB; provide for compatibility with BDB
    bool Flush()
    {
        return true;
    }

    bool Sync() throw(dbwrapper_error)
    {
        CDBBatch batch(&obfuscate_key);
        return WriteBatch(batch, true);
    }

    CDBIterator *NewIterator()
    {
        unsigned int stmt_idx = DBW_SEEK_SORTED;
        return new CDBIterator(stmts[stmt_idx], &obfuscate_key);
    }

    /**
     * Return true if the database managed by this class contains no entries.
     */
    bool IsEmpty();

    /**
     * Accessor for obfuscate_key.
     */
    const std::vector<unsigned char>& GetObfuscateKey() const;

    /**
     * Return the obfuscate_key as a hex-formatted string.
     */
    std::string GetObfuscateKeyHex() const;

};

#endif // BITCOIN_DBWRAPPER_H

