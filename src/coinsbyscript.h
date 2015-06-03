// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSBYSCRIPT_H
#define BITCOIN_COINSBYSCRIPT_H

#include "coins.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

class CCoinsViewDB;
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
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(setCoins);
    }
};

typedef std::map<uint160, CCoinsByScript> CCoinsMapByScript; // uint160 = hash of script

/** Adds a memory cache for coins by address */
class CCoinsViewByScript
{
private:
    CCoinsViewDB *base;

public:
    CCoinsMapByScript cacheCoinsByScript; // accessed also from CCoinsViewDB in txdb.cpp
    CCoinsViewByScript(CCoinsViewDB* baseIn);

    bool GetCoinsByScript(const CScript &script, CCoinsByScript &coins);

    // Return a modifiable reference to a CCoinsByScript.
    CCoinsByScript &GetCoinsByScript(const CScript &script, bool fRequireExisting = true);

    static uint160 getKey(const CScript &script); // we use the hash of the script as key in the database

private:
    CCoinsMapByScript::iterator FetchCoinsByScript(const CScript &script, bool fRequireExisting);
};

#endif // BITCOIN_COINSBYSCRIPT_H
