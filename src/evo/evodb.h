// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_EVODB_H
#define DASH_EVODB_H

#include "dbwrapper.h"
#include "sync.h"
#include "uint256.h"

static const std::string EVODB_BEST_BLOCK = "b_b";

class CEvoDB
{
private:
    CCriticalSection cs;
    CDBWrapper db;
    CDBTransaction dbTransaction;

public:
    CEvoDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    std::unique_ptr<CScopedDBTransaction> BeginTransaction()
    {
        LOCK(cs);
        auto t = CScopedDBTransaction::Begin(dbTransaction);
        return t;
    }

    template <typename K, typename V>
    bool Read(const K& key, V& value)
    {
        LOCK(cs);
        return dbTransaction.Read(key, value);
    }

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        LOCK(cs);
        dbTransaction.Write(key, value);
    }

    template <typename K>
    bool Exists(const K& key)
    {
        LOCK(cs);
        return dbTransaction.Exists(key);
    }

    template <typename K>
    void Erase(const K& key)
    {
        LOCK(cs);
        dbTransaction.Erase(key);
    }

    CDBWrapper& GetRawDB()
    {
        return db;
    }

    bool VerifyBestBlock(const uint256& hash);
    void WriteBestBlock(const uint256& hash);
};

extern CEvoDB* evoDb;

#endif //DASH_EVODB_H
