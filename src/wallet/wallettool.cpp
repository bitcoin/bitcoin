// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
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
        tfm::format(std::cerr, "Error: File exists already\n");
        return nullptr;
    }
    // dummy chain interface
    std::shared_ptr<CWallet> wallet_instance(new CWallet(nullptr /* chain */, WalletLocation(name), WalletDatabase::Create(path)), WalletToolReleaseWallet);
    LOCK(wallet_instance->cs_wallet);
    bool first_run = true;
    DBErrors load_wallet_ret = wallet_instance->LoadWallet(first_run);
    if (load_wallet_ret != DBErrors::LOAD_OK) {
        tfm::format(std::cerr, "Error creating %s", name);
        return nullptr;
    }

    wallet_instance->SetMinVersion(FEATURE_HD_SPLIT);

    // generate a new HD seed
    auto spk_man = wallet_instance->GetOrCreateLegacyScriptPubKeyMan();
    CPubKey seed = spk_man->GenerateNewSeed();
    spk_man->SetHDSeed(seed);

    tfm::format(std::cout, "Topping up keypool...\n");
    wallet_instance->TopUpKeyPool();
    return wallet_instance;
}

static std::shared_ptr<CWallet> LoadWallet(const std::string& name, const fs::path& path)
{
    if (!fs::exists(path)) {
        tfm::format(std::cerr, "Error: Wallet files does not exist\n");
        return nullptr;
    }

    // dummy chain interface
    std::shared_ptr<CWallet> wallet_instance(new CWallet(nullptr /* chain */, WalletLocation(name), WalletDatabase::Create(path)), WalletToolReleaseWallet);
    DBErrors load_wallet_ret;
    try {
        bool first_run;
        load_wallet_ret = wallet_instance->LoadWallet(first_run);
    } catch (const std::runtime_error&) {
        tfm::format(std::cerr, "Error loading %s. Is wallet being used by another process?\n", name);
        return nullptr;
    }

    if (load_wallet_ret != DBErrors::LOAD_OK) {
        wallet_instance = nullptr;
        if (load_wallet_ret == DBErrors::CORRUPT) {
            tfm::format(std::cerr, "Error loading %s: Wallet corrupted", name);
            return nullptr;
        } else if (load_wallet_ret == DBErrors::NONCRITICAL_ERROR) {
            tfm::format(std::cerr, "Error reading %s! All keys read correctly, but transaction data"
                            " or address book entries might be missing or incorrect.",
                name);
        } else if (load_wallet_ret == DBErrors::TOO_NEW) {
            tfm::format(std::cerr, "Error loading %s: Wallet requires newer version of %s",
                name, PACKAGE_NAME);
            return nullptr;
        } else if (load_wallet_ret == DBErrors::NEED_REWRITE) {
            tfm::format(std::cerr, "Wallet needed to be rewritten: restart %s to complete", PACKAGE_NAME);
            return nullptr;
        } else {
            tfm::format(std::cerr, "Error loading %s", name);
            return nullptr;
        }
    }

    return wallet_instance;
}

static void WalletShowInfo(CWallet* wallet_instance)
{
    LOCK(wallet_instance->cs_wallet);

    tfm::format(std::cout, "Wallet info\n===========\n");
    tfm::format(std::cout, "Encrypted: %s\n", wallet_instance->IsCrypted() ? "yes" : "no");
    tfm::format(std::cout, "HD (hd seed available): %s\n", wallet_instance->IsHDEnabled() ? "yes" : "no");
    tfm::format(std::cout, "Keypool Size: %u\n", wallet_instance->GetKeyPoolSize());
    tfm::format(std::cout, "Transactions: %zu\n", wallet_instance->mapWallet.size());
    tfm::format(std::cout, "Address Book: %zu\n", wallet_instance->m_address_book.size());
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
            tfm::format(std::cerr, "Error: no wallet file at %s\n", name);
            return false;
        }
        std::string error;
        if (!WalletBatch::VerifyEnvironment(path, error)) {
            tfm::format(std::cerr, "Error loading %s. Is wallet being used by other process?\n", name);
            return false;
        }
        std::shared_ptr<CWallet> wallet_instance = LoadWallet(name, path);
        if (!wallet_instance) return false;
        WalletShowInfo(wallet_instance.get());
        wallet_instance->Flush(true);
    } else {
        tfm::format(std::cerr, "Invalid command: %s\n", command);
        return false;
    }

    return true;
}
} // namespace WalletTool
