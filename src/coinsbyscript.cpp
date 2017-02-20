// Copyright (c) 2014-2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coinsbyscript.h"
#include "txdb.h"
#include "hash.h"
#include "ui_interface.h"

#include <assert.h>

#include <boost/thread.hpp>

using namespace std;

static const char DB_COINS_BYSCRIPT = 'd';
static const char DB_FLAG = 'F';
static const char DB_BEST_BLOCK = 'B';

CCoinsViewByScript::CCoinsViewByScript(CCoinsViewByScriptDB* viewIn) : base(viewIn) { }

bool CCoinsViewByScript::GetCoinsByScript(const CScript &script, CCoinsByScript &coins) {
    const uint160 key = CCoinsViewByScript::getKey(script);
    if (cacheCoinsByScript.count(key)) {
        coins = cacheCoinsByScript[key];
        return true;
    }
    if (base->GetCoinsByHashOfScript(key, coins)) {
        cacheCoinsByScript[key] = coins;
        return true;
    }
    return false;
}

CCoinsMapByScript::iterator CCoinsViewByScript::FetchCoinsByScript(const CScript &script, bool fRequireExisting) {
    const uint160 key = CCoinsViewByScript::getKey(script);
    CCoinsMapByScript::iterator it = cacheCoinsByScript.find(key);
    if (it != cacheCoinsByScript.end())
        return it;
    CCoinsByScript tmp;
    if (!base->GetCoinsByHashOfScript(key, tmp))
    {
        if (fRequireExisting)
            return cacheCoinsByScript.end();
    }
    CCoinsMapByScript::iterator ret = cacheCoinsByScript.insert(it, std::make_pair(key, CCoinsByScript()));
    tmp.swap(ret->second);
    return ret;
}

CCoinsByScript &CCoinsViewByScript::GetCoinsByScript(const CScript &script, bool fRequireExisting) {
    CCoinsMapByScript::iterator it = FetchCoinsByScript(script, fRequireExisting);
    assert(it != cacheCoinsByScript.end());
    return it->second;
}

uint160 CCoinsViewByScript::getKey(const CScript &script) {
    return Hash160(script);
}

uint256 CCoinsViewByScript::GetBestBlock() const {
    return hashBlock;
}

void CCoinsViewByScript::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
}

bool CCoinsViewByScript::Flush() {
    bool fOk = base->BatchWrite(this, hashBlock);
    return fOk;
}

CCoinsViewByScriptDB::CCoinsViewByScriptDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "coinsbyscript", nCacheSize, fMemory, fWipe, true)
{
}

bool CCoinsViewByScriptDB::GetCoinsByHashOfScript(const uint160 &hash, CCoinsByScript &coins) const {
    return db.Read(make_pair(DB_COINS_BYSCRIPT, hash), coins);
}

bool CCoinsViewByScriptDB::BatchWrite(CCoinsViewByScript* pcoinsViewByScriptIn, const uint256 &hashBlock) {
    CDBBatch batch(db);
    size_t count = 0;
    for (CCoinsMapByScript::iterator it = pcoinsViewByScriptIn->cacheCoinsByScript.begin(); it != pcoinsViewByScriptIn->cacheCoinsByScript.end();) {
        if (it->second.IsEmpty())
            batch.Erase(make_pair(DB_COINS_BYSCRIPT, it->first));
        else
            batch.Write(make_pair(DB_COINS_BYSCRIPT, it->first), it->second);
        CCoinsMapByScript::iterator itOld = it++;
        pcoinsViewByScriptIn->cacheCoinsByScript.erase(itOld);
        count++;
    }
    pcoinsViewByScriptIn->cacheCoinsByScript.clear();

    if (!hashBlock.IsNull())
        batch.Write(DB_BEST_BLOCK, hashBlock);

    LogPrint(BCLog::COINDB, "Committing %u coin address indexes to coin database...\n", (unsigned int)count);
    return db.WriteBatch(batch);
}

bool CCoinsViewByScriptDB::WriteFlag(const std::string &name, bool fValue) {
    return db.Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
}

bool CCoinsViewByScriptDB::ReadFlag(const std::string &name, bool &fValue) {
    char ch;
    if (!db.Read(std::make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

CCoinsViewByScriptDBCursor *CCoinsViewByScriptDB::Cursor() const
{
    CCoinsViewByScriptDBCursor *i = new CCoinsViewByScriptDBCursor(const_cast<CDBWrapper*>(&db)->NewIterator());
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    i->pcursor->Seek(DB_COINS_BYSCRIPT);
    if (!i->pcursor->Valid())
        // If db empty then set this cursor invalid
        i->keyTmp.first = 0;
    else
        // Cache key of first record
        i->pcursor->GetKey(i->keyTmp);
    return i;
}

bool CCoinsViewByScriptDBCursor::GetKey(uint160 &key) const
{
    // Return cached key
    if (keyTmp.first == DB_COINS_BYSCRIPT) {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool CCoinsViewByScriptDBCursor::GetValue(CCoinsByScript &coins) const
{
    return pcursor->GetValue(coins);
}

unsigned int CCoinsViewByScriptDBCursor::GetValueSize() const
{
    return pcursor->GetValueSize();
}

bool CCoinsViewByScriptDBCursor::Valid() const
{
    return keyTmp.first == DB_COINS_BYSCRIPT;
}

void CCoinsViewByScriptDBCursor::Next()
{
    pcursor->Next();
    if (!pcursor->Valid() || !pcursor->GetKey(keyTmp))
        keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
}

bool CCoinsViewByScriptDB::DeleteAllCoinsByScript()
{
    std::unique_ptr<CCoinsViewByScriptDBCursor> pcursor(Cursor());

    std::vector<uint160> v;
    int64_t i = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            uint160 hash;
            if (!pcursor->GetKey(hash))
                break;
            v.push_back(hash);
            if (v.size() >= 10000)
            {
                i += v.size();
                CDBBatch batch(db);
                BOOST_FOREACH(const uint160& _hash, v)
                    batch.Erase(make_pair(DB_COINS_BYSCRIPT, _hash)); // delete
                db.WriteBatch(batch);
                v.clear();
            }

            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    if (!v.empty())
    {
        i += v.size();
        CDBBatch batch(db);
        BOOST_FOREACH(const uint160& hash, v)
            batch.Erase(make_pair(DB_COINS_BYSCRIPT, hash)); // delete
        db.WriteBatch(batch);
    }
    if (i > 0)
        LogPrintf("Address index with %d addresses successfully deleted.\n", i);

    return true;
}

bool CCoinsViewByScriptDB::GenerateAllCoinsByScript(CCoinsViewDB* coinsIn)
{
    LogPrintf("Building address index for -txoutindex. Be patient...\n");
    int64_t nTxCount = coinsIn->CountCoins();

    std::unique_ptr<CCoinsViewCursor> pcursor(coinsIn->Cursor());

    CCoinsMapByScript mapCoinsByScript;
    int64_t i = 0;
    int64_t progress = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            if (progress % 1000 == 0 && nTxCount > 0)
                uiInterface.ShowProgress(_("Building address index..."), (int)(((double)progress / (double)nTxCount) * (double)100));
            progress++;

            uint256 txhash;
            CCoins coins;
            if (!pcursor->GetKey(txhash) || !pcursor->GetValue(coins))
                break;

            for (unsigned int j = 0; j < coins.vout.size(); j++)
            {
                if (coins.vout[j].IsNull() || coins.vout[j].scriptPubKey.IsUnspendable())
                    continue;

                const uint160 key = CCoinsViewByScript::getKey(coins.vout[j].scriptPubKey);
                if (!mapCoinsByScript.count(key))
                {
                    CCoinsByScript coinsByScript;
                    GetCoinsByHashOfScript(key, coinsByScript);
                    mapCoinsByScript.insert(make_pair(key, coinsByScript));
                }
                mapCoinsByScript[key].setCoins.insert(COutPoint(txhash, (uint32_t)j));
                i++;
            }

            if (mapCoinsByScript.size() >= 10000)
            {
                CDBBatch batch(db);
                for (CCoinsMapByScript::iterator it = mapCoinsByScript.begin(); it != mapCoinsByScript.end();) {
                    if (it->second.IsEmpty())
                        batch.Erase(make_pair(DB_COINS_BYSCRIPT, it->first));
                    else
                        batch.Write(make_pair(DB_COINS_BYSCRIPT, it->first), it->second);
                    CCoinsMapByScript::iterator itOld = it++;
                    mapCoinsByScript.erase(itOld);
                }
                db.WriteBatch(batch);
                mapCoinsByScript.clear();
            }

            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    if (!mapCoinsByScript.empty())
    {
       CDBBatch batch(db);
       for (CCoinsMapByScript::iterator it = mapCoinsByScript.begin(); it != mapCoinsByScript.end();) {
           if (it->second.IsEmpty())
               batch.Erase(make_pair(DB_COINS_BYSCRIPT, it->first));
           else
               batch.Write(make_pair(DB_COINS_BYSCRIPT, it->first), it->second);
           CCoinsMapByScript::iterator itOld = it++;
           mapCoinsByScript.erase(itOld);
       }
       db.WriteBatch(batch);
    }
    LogPrintf("Address index with %d outputs successfully built.\n", i);
    return true;
}
