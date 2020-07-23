// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/salvage.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

namespace WalletTool {

// The standard wallet deleter function blocks on the validation interface
// queue, which doesn't exist for the bitcoin-wallet. Define our own
// deleter here.
static void WalletToolReleaseWallet(CWallet* wallet)
{
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->Close();
    delete wallet;
}

static std::shared_ptr<CWallet> CreateWallet(const std::string& name, const fs::path& path)
{
    if (fs::exists(path)) {
        tfm::format(std::cerr, "Error: File exists already\n");
        return nullptr;
    }
    // dummy chain interface
    std::shared_ptr<CWallet> wallet_instance(new CWallet(nullptr /* chain */, WalletLocation(name), CreateWalletDatabase(path)), WalletToolReleaseWallet);
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
    std::shared_ptr<CWallet> wallet_instance(new CWallet(nullptr /* chain */, WalletLocation(name), CreateWalletDatabase(path)), WalletToolReleaseWallet);
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

static bool SalvageWallet(const fs::path& path)
{
    // Create a Database handle to allow for the db to be initialized before recovery
    std::unique_ptr<WalletDatabase> database = CreateWalletDatabase(path);

    // Initialize the environment before recovery
    bilingual_str error_string;
    try {
        database->Verify(error_string);
    } catch (const fs::filesystem_error& e) {
        error_string = Untranslated(strprintf("Error loading wallet. %s", fsbridge::get_filesystem_error_message(e)));
    }
    if (!error_string.original.empty()) {
        tfm::format(std::cerr, "Failed to open wallet for salvage :%s\n", error_string.original);
        return false;
    }

    // Perform the recovery
    return RecoverDatabaseFile(path);
}

bool ExecuteWalletToolFunc(const std::string& command, const std::string& name)
{
    fs::path path = fs::absolute(name, GetWalletDir());

    if (command == "create") {
        std::shared_ptr<CWallet> wallet_instance = CreateWallet(name, path);
        if (wallet_instance) {
            WalletShowInfo(wallet_instance.get());
            wallet_instance->Close();
        }
    } else if (command == "info" || command == "salvage") {
        if (!fs::exists(path)) {
            tfm::format(std::cerr, "Error: no wallet file at %s\n", name);
            return false;
        }

        if (command == "info") {
            std::shared_ptr<CWallet> wallet_instance = LoadWallet(name, path);
            if (!wallet_instance) return false;
            WalletShowInfo(wallet_instance.get());
            wallet_instance->Close();
        } else if (command == "salvage") {
            return SalvageWallet(path);
        }
    } else {
        tfm::format(std::cerr, "Invalid command: %s\n", command);
        return false;
    }

    return true;
}
} // namespace WalletTool
