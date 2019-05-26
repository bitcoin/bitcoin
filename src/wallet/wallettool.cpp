// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <util/system.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

namespace WalletTool {

// The standard wallet deleter function blocks on the validation interface
// queue, which doesn't exist for the bitcoin-wallet. Define our own
// deleter here.
static void WalletToolReleaseWallet(CWallet* wallet)
{
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->Flush(true);
    delete wallet;
}

static std::shared_ptr<CWallet> CreateWallet(const std::string& name, const fs::path& path)
{
    if (fs::exists(path)) {
        fprintf(stderr, "Error: File exists already\n");
        return nullptr;
    }
    // dummy chain interface
    auto chain = interfaces::MakeChain();
    std::shared_ptr<CWallet> wallet_instance(new CWallet(*chain, WalletLocation(name), WalletDatabase::Create(path)), WalletToolReleaseWallet);
    bool first_run = true;
    DBErrors load_wallet_ret = wallet_instance->LoadWallet(first_run);
    if (load_wallet_ret != DBErrors::LOAD_OK) {
        fprintf(stderr, "Error creating %s", name.c_str());
        return nullptr;
    }

    wallet_instance->SetMinVersion(FEATURE_HD_SPLIT);

    // generate a new HD seed
    CPubKey seed = wallet_instance->GenerateNewSeed();
    wallet_instance->SetHDSeed(seed);

    fprintf(stdout, "Topping up keypool...\n");
    wallet_instance->TopUpKeyPool();
    return wallet_instance;
}

static std::shared_ptr<CWallet> LoadWallet(const std::string& name, const fs::path& path)
{
    if (!fs::exists(path)) {
        fprintf(stderr, "Error: Wallet files does not exist\n");
        return nullptr;
    }

    // dummy chain interface
    auto chain = interfaces::MakeChain();
    std::shared_ptr<CWallet> wallet_instance(new CWallet(*chain, WalletLocation(name), WalletDatabase::Create(path)), WalletToolReleaseWallet);
    DBErrors load_wallet_ret;
    try {
        bool first_run;
        load_wallet_ret = wallet_instance->LoadWallet(first_run);
    } catch (const std::runtime_error&) {
        fprintf(stderr, "Error loading %s. Is wallet being used by another process?\n", name.c_str());
        return nullptr;
    }

    if (load_wallet_ret != DBErrors::LOAD_OK) {
        wallet_instance = nullptr;
        if (load_wallet_ret == DBErrors::CORRUPT) {
            fprintf(stderr, "Error loading %s: Wallet corrupted", name.c_str());
            return nullptr;
        } else if (load_wallet_ret == DBErrors::NONCRITICAL_ERROR) {
            fprintf(stderr, "Error reading %s! All keys read correctly, but transaction data"
                            " or address book entries might be missing or incorrect.",
                name.c_str());
        } else if (load_wallet_ret == DBErrors::TOO_NEW) {
            fprintf(stderr, "Error loading %s: Wallet requires newer version of %s",
                name.c_str(), PACKAGE_NAME);
            return nullptr;
        } else if (load_wallet_ret == DBErrors::NEED_REWRITE) {
            fprintf(stderr, "Wallet needed to be rewritten: restart %s to complete", PACKAGE_NAME);
            return nullptr;
        } else {
            fprintf(stderr, "Error loading %s", name.c_str());
            return nullptr;
        }
    }

    return wallet_instance;
}

static void WalletShowInfo(CWallet* wallet_instance)
{
    LOCK(wallet_instance->cs_wallet);

    fprintf(stdout, "Wallet info\n===========\n");
    fprintf(stdout, "Encrypted: %s\n", wallet_instance->IsCrypted() ? "yes" : "no");
    fprintf(stdout, "HD (hd seed available): %s\n", wallet_instance->GetHDChain().seed_id.IsNull() ? "no" : "yes");
    fprintf(stdout, "Keypool Size: %u\n", wallet_instance->GetKeyPoolSize());
    fprintf(stdout, "Transactions: %zu\n", wallet_instance->mapWallet.size());
    fprintf(stdout, "Address Book: %zu\n", wallet_instance->mapAddressBook.size());
}

bool ExecuteWalletToolFunc(const std::string& command, const std::string& name)
{
    fs::path path = fs::absolute(name, GetWalletDir());

    if (command == "create") {
        std::shared_ptr<CWallet> wallet_instance = CreateWallet(name, path);
        if (wallet_instance) {
            WalletShowInfo(wallet_instance.get());
            wallet_instance->Flush(true);
        }
    } else if (command == "info") {
        if (!fs::exists(path)) {
            fprintf(stderr, "Error: no wallet file at %s\n", name.c_str());
            return false;
        }
        std::string error;
        if (!WalletBatch::VerifyEnvironment(path, error)) {
            fprintf(stderr, "Error loading %s. Is wallet being used by other process?\n", name.c_str());
            return false;
        }
        std::shared_ptr<CWallet> wallet_instance = LoadWallet(name, path);
        if (!wallet_instance) return false;
        WalletShowInfo(wallet_instance.get());
        wallet_instance->Flush(true);
    } else {
        fprintf(stderr, "Invalid command: %s\n", command.c_str());
        return false;
    }

    return true;
}
} // namespace WalletTool
