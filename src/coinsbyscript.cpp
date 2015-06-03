// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coinsbyscript.h"
#include "txdb.h"
#include "hash.h"

#include <assert.h>

CCoinsViewByScript::CCoinsViewByScript(CCoinsViewDB* viewIn) : base(viewIn) { }

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
