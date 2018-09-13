// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletutil.h>
#include <util.h>

fs::path GetWalletDir()
{
    fs::path path;

    if (gArgs.IsArgSet("-walletdir")) {
        path = gArgs.GetArg("-walletdir", "");
        if (!fs::is_directory(path)) {
            // If the path specified doesn't exist, we return the deliberately
            // invalid empty string.
            path = "";
        }
    } else {
        path = GetDataDir();
        // If a wallets directory exists, use that, otherwise default to GetDataDir
        if (fs::is_directory(path / "wallets")) {
            path /= "wallets";
        }
    }

    return path;
}

CKeyHolder::CKeyHolder(CWallet* pwallet) :
    reserveKey(pwallet)
{
    reserveKey.GetReservedKey(pubKey);
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
    std::vector<std::unique_ptr<CKeyHolder>> tmp;
    {
        // don't hold cs_storage while calling KeepKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (tmp.size() > 0) {
        for (auto &key : tmp) {
            key->KeepKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys kept\n", __func__, tmp.size());
    }
}

void CKeyHolderStorage::ReturnAll()
{
    std::vector<std::unique_ptr<CKeyHolder>> tmp;
    {
        // don't hold cs_storage while calling ReturnKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (tmp.size() > 0) {
        for (auto &key : tmp) {
            key->ReturnKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys returned\n", __func__, tmp.size());
    }
}

