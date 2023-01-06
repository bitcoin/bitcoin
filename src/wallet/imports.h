// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_IMPORTS_H
#define BITCOIN_WALLET_IMPORTS_H

#include <string>
#include <vector>
#include <threadsafety.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <wallet/wallet.h>

namespace wallet {
struct MiscError {
    std::string error;
    MiscError(std::string error) : error(error) {}
};

struct WalletError {
    std::string error;
    WalletError(std::string error) : error(error) {}
};

struct InvalidAddressOrKey {
    std::string error;
    InvalidAddressOrKey(std::string error) : error(error) {}
};

struct InvalidParameter {
    std::string error;
    InvalidParameter(std::string error) : error(error) {}
};

bool ProcessPublicKey(CWallet& wallet, std::string strLabel, CPubKey pubKey) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool ProcessPrivateKey(CWallet& wallet, std::string strLabel, CKey key) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool ProcessAddress(CWallet &wallet, std::string strAddress, std::string strLabel, bool fP2SH) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
}

#endif // BITCOIN_WALLET_IMPORTS_H
