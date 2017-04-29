// Copyright (c) 2014-2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSBYSCRIPT_H
#define BITCOIN_COINSBYSCRIPT_H

#include "coins.h"
#include "dbwrapper.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"
#include "script/standard.h"

class CCoinsViewDB;
class CCoinsViewByScriptDB;
class CScript;

class CCoinsByScript
{
public:
    // unspent transaction outputs
    std::set<COutPoint> setCoins;

    // empty constructor
    CCoinsByScript() { }

    bool IsEmpty() const {
        return (setCoins.empty());
    }

    void swap(CCoinsByScript &to) {
        to.setCoins.swap(setCoins);
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(setCoins);
    }
};

typedef std::map<CScriptID, CCoinsByScript> CCoinsMapByScript;

/** Adds a memory cache for coins by address */
class CCoinsViewByScript
{
private:
    CCoinsViewByScriptDB *base;

    mutable uint256 hashBlock;

public:
    CCoinsMapByScript cacheCoinsByScript; // accessed also from CCoinsViewByScriptDB
    CCoinsViewByScript(CCoinsViewByScriptDB* baseIn);

    bool GetCoinsByScript(const CScript &script, CCoinsByScript &coins);

    // Return a modifiable reference to a CCoinsByScript.
    CCoinsByScript &GetCoinsByScript(const CScript &script, bool fRequireExisting = true);

    void SetBestBlock(const uint256 &hashBlock);
    uint256 GetBestBlock() const;

    /**
     * Push the modifications applied to this cache to its base.
     * Failure to call this method before destruction will cause the changes to be forgotten.
     * If false is returned, the state of this cache (and its backing view) will be undefined.
     */
    bool Flush();

private:
    CCoinsMapByScript::iterator FetchCoinsByScript(const CScript &script, bool fRequireExisting);
};

/** Cursor for iterating over a CCoinsViewByScriptDB */
class CCoinsViewByScriptDBCursor 
{
public:
    ~CCoinsViewByScriptDBCursor() {}

    bool GetKey(CScriptID &key) const;
    bool GetValue(CCoinsByScript &coins) const;
    unsigned int GetValueSize() const;

    bool Valid() const;
    void Next();

private:
    CCoinsViewByScriptDBCursor(CDBIterator* pcursorIn):
        pcursor(pcursorIn) {}
    uint256 hashBlock;
    std::unique_ptr<CDBIterator> pcursor;
    std::pair<char, CScriptID> keyTmp;

    friend class CCoinsViewByScriptDB;
};

/** coinsbyscript database (coinsbyscript/) */
class CCoinsViewByScriptDB 
{
protected:
    CDBWrapper db;
public:
    CCoinsViewByScriptDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetCoinsByScriptID(const CScriptID &scriptID, CCoinsByScript &coins) const;
    bool BatchWrite(CCoinsViewByScript* pcoinsViewByScriptIn, const uint256 &hashBlock);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool DeleteAllCoinsByScript();   // removes txoutindex
    bool GenerateAllCoinsByScript(CCoinsViewDB* coinsIn); // creates txoutindex
    CCoinsViewByScriptDBCursor *Cursor() const;
};

#endif // BITCOIN_COINSBYSCRIPT_H
