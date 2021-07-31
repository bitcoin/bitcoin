// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_EVODB_H
#define BITCOIN_EVO_EVODB_H

#include <dbwrapper.h>
#include <sync.h>
#include <uint256.h>

// "b_b" was used in the initial version of deterministic MN storage
// "b_b2" was used after compact diffs were introduced
static const std::string EVODB_BEST_BLOCK = "b_b2";

class CEvoDB;

class CEvoDBScopedCommitter
{
private:
    CEvoDB& evoDB;
    bool didCommitOrRollback{false};

public:
    explicit CEvoDBScopedCommitter(CEvoDB& _evoDB);
    ~CEvoDBScopedCommitter();

    void Commit();
    void Rollback();
};

class CEvoDB
{
public:
    CCriticalSection cs;
private:
    CDBWrapper db;

    typedef CDBTransaction<CDBWrapper, CDBBatch> RootTransaction;
    typedef CDBTransaction<RootTransaction, RootTransaction> CurTransaction;

    CDBBatch rootBatch;
    RootTransaction rootDBTransaction;
    CurTransaction curDBTransaction;

public:
    explicit CEvoDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    std::unique_ptr<CEvoDBScopedCommitter> BeginTransaction()
    {
        LOCK(cs);
        return std::make_unique<CEvoDBScopedCommitter>(*this);
    }

    CurTransaction& GetCurTransaction()
    {
        AssertLockHeld(cs); // lock must be held from outside as long as the DB transaction is used
        return curDBTransaction;
    }

    template <typename K, typename V>
    bool Read(const K& key, V& value)
    {
        LOCK(cs);
        return curDBTransaction.Read(key, value);
    }

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        LOCK(cs);
        curDBTransaction.Write(key, value);
    }

    template <typename K>
    bool Exists(const K& key)
    {
        LOCK(cs);
        return curDBTransaction.Exists(key);
    }

    template <typename K>
    void Erase(const K& key)
    {
        LOCK(cs);
        curDBTransaction.Erase(key);
    }

    CDBWrapper& GetRawDB()
    {
        return db;
    }

    size_t GetMemoryUsage() const
    {
        return rootDBTransaction.GetMemoryUsage();
    }

    bool CommitRootTransaction();

    bool IsEmpty() { return db.IsEmpty(); }

    bool VerifyBestBlock(const uint256& hash);
    void WriteBestBlock(const uint256& hash);

private:
    // only CEvoDBScopedCommitter is allowed to invoke these
    friend class CEvoDBScopedCommitter;
    void CommitCurTransaction();
    void RollbackCurTransaction();
};

extern std::unique_ptr<CEvoDB> evoDb;

#endif // BITCOIN_EVO_EVODB_H
