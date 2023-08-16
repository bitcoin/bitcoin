// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <util/translation.h>
#include <util/system.h>
#include <wallet/salvage.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

namespace WalletTool {

// The standard wallet deleter function blocks on the validation interface
// queue, which doesn't exist for the dash-wallet. Define our own
// deleter here.
static void WalletToolReleaseWallet(CWallet* wallet)
{
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->Close();
    delete wallet;
}

static void WalletCreate(CWallet* wallet_instance)
{
    LOCK(wallet_instance->cs_wallet);
    wallet_instance->SetMinVersion(FEATURE_COMPRPUBKEY);

    // generate a new HD seed
    wallet_instance->SetupLegacyScriptPubKeyMan();
    // NOTE: we do not yet create HD wallets by default
    // auto spk_man = wallet_instance->GetLegacyScriptPubKeyMan();
    // spk_man->GenerateNewHDChain("", "");

    tfm::format(std::cout, "Topping up keypool...\n");
    wallet_instance->TopUpKeyPool();
}

static std::shared_ptr<CWallet> MakeWallet(const std::string& name, const fs::path& path, bool create)
{
    DatabaseOptions options;
    DatabaseStatus status;
    if (create) {
        options.require_create = true;
    } else {
        options.require_existing = true;
    }
    bilingual_str error;
    std::unique_ptr<WalletDatabase> database = MakeDatabase(path, options, status, error);
    if (!database) {
        tfm::format(std::cerr, "%s\n", error.original);
        return nullptr;
    }

    // dummy chain interface
    std::shared_ptr<CWallet> wallet_instance{new CWallet(nullptr /* chain */, name, std::move(database)), WalletToolReleaseWallet};
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

    if (create) WalletCreate(wallet_instance.get());

    return wallet_instance;
}

static void WalletShowInfo(CWallet* wallet_instance)
{
    // lock required because of some AssertLockHeld()
    LOCK(wallet_instance->cs_wallet);

    CHDChain hdChainTmp;
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
        std::shared_ptr<CWallet> wallet_instance = MakeWallet(name, path, /* create= */ true);
        if (wallet_instance) {
            WalletShowInfo(wallet_instance.get());
            wallet_instance->Close();
        }
    } else if (command == "info" || command == "salvage" || command == "wipetxes") {
        if (command == "info") {
            std::shared_ptr<CWallet> wallet_instance = MakeWallet(name, path, /* create= */ false);
            if (!wallet_instance) return false;
            WalletShowInfo(wallet_instance.get());
            wallet_instance->Close();
        } else if (command == "salvage") {
#ifdef USE_BDB
            bilingual_str error;
            std::vector<bilingual_str> warnings;
            bool ret = RecoverDatabaseFile(path, error, warnings);
            if (!ret) {
                for (const auto& warning : warnings) {
                    tfm::format(std::cerr, "%s\n", warning.original);
                }
                if (!error.empty()) {
                    tfm::format(std::cerr, "%s\n", error.original);
                }
            }
            return ret;
#else
            tfm::format(std::cerr, "Salvage command is not available as BDB support is not compiled");
            return false;
#endif
        } else if (command == "wipetxes") {
#ifdef USE_BDB
            std::shared_ptr<CWallet> wallet_instance = MakeWallet(name, path, /* create= */ false);
            if (wallet_instance == nullptr) return false;

            std::vector<uint256> vHash;
            std::vector<uint256> vHashOut;

            LOCK(wallet_instance->cs_wallet);

            for (auto& [txid, _] : wallet_instance->mapWallet) {
                vHash.push_back(txid);
            }

            if (wallet_instance->ZapSelectTx(vHash, vHashOut) != DBErrors::LOAD_OK) {
                tfm::format(std::cerr, "Could not properly delete transactions");
                wallet_instance->Close();
                return false;
            }

            wallet_instance->Close();
            return vHashOut.size() == vHash.size();
#else
            tfm::format(std::cerr, "Wipetxes command is not available as BDB support is not compiled");
            return false;
#endif
        }
    } else {
        tfm::format(std::cerr, "Invalid command: %s\n", command);
        return false;
    }

    return true;
}
} // namespace WalletTool
