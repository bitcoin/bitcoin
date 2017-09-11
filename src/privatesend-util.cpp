// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "privatesend-util.h"

CKeyHolder::CKeyHolder(CWallet* pwallet) :
    reserveKey(pwallet)
{
    reserveKey.GetReservedKey(pubKey, false);
}

void CKeyHolder::KeepKey()
{
    reserveKey.KeepKey();
}

void CKeyHolder::ReturnKey()
{
    reserveKey.ReturnKey();
}

CScript CKeyHolder::GetScriptForDestination() const
{
    return ::GetScriptForDestination(pubKey.GetID());
}


const CKeyHolder& CKeyHolderStorage::AddKey(CWallet* pwallet)
{
    storage.emplace_back(std::unique_ptr<CKeyHolder>(new CKeyHolder(pwallet)));
    LogPrintf("CKeyHolderStorage::%s -- storage size %lld\n", __func__, storage.size());
    return *storage.back();
}

void CKeyHolderStorage::KeepAll(){
    if (storage.size() > 0) {
        for (auto &key : storage) {
            key->KeepKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys kept\n", __func__, storage.size());
        storage.clear();
    }
}

void CKeyHolderStorage::ReturnAll()
{
    if (storage.size() > 0) {
        for (auto &key : storage) {
            key->ReturnKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys returned\n", __func__, storage.size());
        storage.clear();
    }
}
