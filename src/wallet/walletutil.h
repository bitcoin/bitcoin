// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETUTIL_H
#define BITCOIN_WALLET_WALLETUTIL_H

#include <chainparamsbase.h>
#include <fs.h>
#include <util/system.h>
#include <wallet/wallet.h>

#include <vector>

//! Get the path of the wallet directory.
fs::path GetWalletDir();

/**
 * Figures out what wallet, if any, to use for a Private Send.
 *
 * @param[in]
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */

class CKeyHolder
{
private:
    CReserveKey reserveKey;
    CPubKey pubKey;
public:
    CKeyHolder(CWallet* pwalletIn);
    CKeyHolder(CKeyHolder&&) = default;
    CKeyHolder& operator=(CKeyHolder&&) = default;
    void KeepKey();
    void ReturnKey();

    CScript GetScriptForDestination() const;

};

class CKeyHolderStorage
{
private:
    std::vector<std::unique_ptr<CKeyHolder> > storage;
    mutable CCriticalSection cs_storage;

public:
    CScript AddKey(CWallet* pwalletIn);
    void KeepAll();
    void ReturnAll();

};

//! Get wallets in wallet directory.
std::vector<fs::path> ListWalletDir();

#endif // BITCOIN_WALLET_WALLETUTIL_H
