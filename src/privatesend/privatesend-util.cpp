// Copyright (c) 2014-2018 The Dash Core developers
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


CScript CKeyHolderStorage::AddKey(CWallet* pwallet)
{
    auto keyHolder = std::unique_ptr<CKeyHolder>(new CKeyHolder(pwallet));
    auto script = keyHolder->GetScriptForDestination();

    LOCK(cs_storage);
    storage.emplace_back(std::move(keyHolder));
    LogPrintf("CKeyHolderStorage::%s -- storage size %lld\n", __func__, storage.size());
    return script;
}

void CKeyHolderStorage::KeepAll()
{
    std::vector<std::unique_ptr<CKeyHolder> > tmp;
    {
        // don't hold cs_storage while calling KeepKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (tmp.size() > 0) {
        for (auto& key : tmp) {
            key->KeepKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys kept\n", __func__, tmp.size());
    }
}

void CKeyHolderStorage::ReturnAll()
{
    std::vector<std::unique_ptr<CKeyHolder> > tmp;
    {
        // don't hold cs_storage while calling ReturnKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (tmp.size() > 0) {
        for (auto& key : tmp) {
            key->ReturnKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys returned\n", __func__, tmp.size());
    }
}
