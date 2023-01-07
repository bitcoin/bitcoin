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

struct ImportMultiData
{
    std::unique_ptr<Descriptor> parsed_desc;
    std::string scriptPubKey;
    std::string redeem_script;
    std::string witness_script;
    std::string label;
    std::vector<std::string> public_keys;
    std::vector<std::string> private_keys;
    int64_t range_start = 0;
    int64_t range_end = 0;
    int64_t timestamp = 0;
    bool internal = false;
    bool isScript = false;
    bool watch_only = false;
    bool keypool = false;
    FlatSigningProvider keys;
};

struct ImportDescriptorData
{
    std::shared_ptr<Descriptor> parsed_desc;
    std::string label;
    int64_t range_start = 0;
    int64_t range_end = 0;
    int64_t next_index = 0;
    int64_t timestamp = 0;
    bool active = false;
    bool internal = false;
};

bool ProcessDescriptorImport(CWallet& wallet, const ImportDescriptorData& data, std::vector<std::string>& warnings, FlatSigningProvider& keys, bool range_exists, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool ProcessImport(CWallet& wallet, const ImportMultiData& data, std::vector<std::string>& warnings, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool ProcessPublicKey(CWallet& wallet, std::string strLabel, CPubKey pubKey) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool ProcessPrivateKey(CWallet& wallet, std::string strLabel, CKey key) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
bool ProcessAddress(CWallet &wallet, std::string strAddress, std::string strLabel, bool fP2SH) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
}

#endif // BITCOIN_WALLET_IMPORTS_H
