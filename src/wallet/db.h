// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DB_H
#define BITCOIN_WALLET_DB_H

#include <clientversion.h>
#include <fs.h>
#include <streams.h>

#include <atomic>
#include <memory>
#include <string>

struct bilingual_str;

/** Given a wallet directory path or legacy file path, return path to main data file in the wallet database. */
fs::path WalletDataFilePath(const fs::path& wallet_path);
void SplitWalletPath(const fs::path& wallet_path, fs::path& env_directory, std::string& database_filename);

/** RAII class that provides access to a WalletDatabase */
class DatabaseBatch
{
private:
    virtual bool ReadKey(CDataStream& key, CDataStream& value) = 0;
    virtual bool WriteKey(CDataStream& key, CDataStream& value, bool overwrite=true) = 0;
    virtual bool EraseKey(CDataStream& key) = 0;
    virtual bool HasKey(CDataStream& key) = 0;

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
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        bool success = false;
        bool ret = ReadKey(ssKey, ssValue);
        if (ret) {
            // Unserialize value
            try {
                ssValue >> value;
                success = true;
            } catch (const std::exception&) {
                // In this case success remains 'false'
            }
        }
        return ret && success;
    }

    template <typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite = true)
    {
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        // Value
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(10000);
        ssValue << value;

        // Write
        return WriteKey(ssKey, ssValue, fOverwrite);
    }

    template <typename K>
    bool Erase(const K& key)
    {
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        // Erase
        return EraseKey(ssKey);
    }

    template <typename K>
    bool Exists(const K& key)
    {
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        // Exists
        return HasKey(ssKey);
    }

    virtual bool CreateCursor() = 0;
    virtual bool ReadAtCursor(CDataStream& ssKey, CDataStream& ssValue, bool& complete) = 0;
    virtual void CloseCursor() = 0;
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
    WalletDatabase() : nUpdateCounter(0), nLastSeen(0), nLastFlushed(0), nLastWalletUpdate(0) {}
    virtual ~WalletDatabase() {};

    /** Open the database if it is not already opened. */
    virtual void Open(const char* mode) = 0;

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

    std::atomic<unsigned int> nUpdateCounter;
    unsigned int nLastSeen;
    unsigned int nLastFlushed;
    int64_t nLastWalletUpdate;

    /** Verifies that the database file is not in use */
    virtual bool VerifyNotInUse(bilingual_str& error) = 0;
    /** Verifies the environment and database file */
    virtual bool Verify(bilingual_str& error) = 0;

    std::string m_file_path;

    /** Make a BerkeleyBatch connected to this database */
    virtual std::unique_ptr<DatabaseBatch> MakeBatch(const char* mode = "r+", bool flush_on_close = true) = 0;
};

#endif // BITCOIN_WALLET_DB_H
