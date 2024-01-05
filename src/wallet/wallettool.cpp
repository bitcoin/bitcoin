// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <wallet/wallettool.h>

#include <common/args.h>
#include <util/fs.h>
#include <util/translation.h>
#include <wallet/dump.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

namespace wallet {
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

static void WalletCreate(CWallet* wallet_instance, uint64_t wallet_creation_flags)
{
    LOCK(wallet_instance->cs_wallet);

    wallet_instance->SetMinVersion(FEATURE_LATEST);
    wallet_instance->InitWalletFlags(wallet_creation_flags);

    if (!wallet_instance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        assert(false);
    } else {
        wallet_instance->SetupDescriptorScriptPubKeyMans();
    }

    tfm::format(std::cout, "Topping up keypool...\n");
    wallet_instance->TopUpKeyPool();
}

static std::shared_ptr<CWallet> MakeWallet(const std::string& name, const fs::path& path, DatabaseOptions options)
{
    DatabaseStatus status;
    bilingual_str error;
    std::unique_ptr<WalletDatabase> database = MakeDatabase(path, options, status, error);
    if (!database) {
        tfm::format(std::cerr, "%s\n", error.original);
        return nullptr;
    }

    // dummy chain interface
    std::shared_ptr<CWallet> wallet_instance{new CWallet(/*chain=*/nullptr, name, std::move(database)), WalletToolReleaseWallet};
    DBErrors load_wallet_ret;
    try {
        load_wallet_ret = wallet_instance->LoadWallet();
    } catch (const std::runtime_error&) {
        tfm::format(std::cerr, "Error loading %s. Is wallet being used by another process?\n", name);
        return nullptr;
    }

    if (load_wallet_ret != DBErrors::LOAD_OK) {
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
        } else if (load_wallet_ret == DBErrors::NEED_RESCAN) {
            tfm::format(std::cerr, "Error reading %s! Some transaction data might be missing or"
                           " incorrect. Wallet requires a rescan.",
                name);
        } else {
            tfm::format(std::cerr, "Error loading %s", name);
            return nullptr;
        }
    }

    if (options.require_create) WalletCreate(wallet_instance.get(), options.create_flags);

    return wallet_instance;
}

static void WalletShowInfo(CWallet* wallet_instance)
{
    LOCK(wallet_instance->cs_wallet);

    tfm::format(std::cout, "Wallet info\n===========\n");
    tfm::format(std::cout, "Name: %s\n", wallet_instance->GetName());
    tfm::format(std::cout, "Format: %s\n", wallet_instance->GetDatabase().Format());
    tfm::format(std::cout, "Descriptors: %s\n", wallet_instance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) ? "yes" : "no");
    tfm::format(std::cout, "Encrypted: %s\n", wallet_instance->IsCrypted() ? "yes" : "no");
    tfm::format(std::cout, "HD (hd seed available): %s\n", wallet_instance->IsHDEnabled() ? "yes" : "no");
    tfm::format(std::cout, "Keypool Size: %u\n", wallet_instance->GetKeyPoolSize());
    tfm::format(std::cout, "Transactions: %zu\n", wallet_instance->mapWallet.size());
    tfm::format(std::cout, "Address Book: %zu\n", wallet_instance->m_address_book.size());
}

bool ExecuteWalletToolFunc(const ArgsManager& args, const std::string& command)
{
    if (args.IsArgSet("-dumpfile") && command != "dump" && command != "createfromdump") {
        tfm::format(std::cerr, "The -dumpfile option can only be used with the \"dump\" and \"createfromdump\" commands.\n");
        return false;
    }
    if (args.IsArgSet("-descriptors") && command != "create") {
        tfm::format(std::cerr, "The -descriptors option can only be used with the 'create' command.\n");
        return false;
    }
    if (args.IsArgSet("-legacy") && command != "create") {
        tfm::format(std::cerr, "The -legacy option can only be used with the 'create' command.\n");
        return false;
    }
    if (command == "create" && !args.IsArgSet("-wallet")) {
        tfm::format(std::cerr, "Wallet name must be provided when creating a new wallet.\n");
        return false;
    }
    const std::string name = args.GetArg("-wallet", "");
    const fs::path path = fsbridge::AbsPathJoin(GetWalletDir(), fs::PathFromString(name));

    if (command == "create") {
        DatabaseOptions options;
        ReadDatabaseArgs(args, options);
        options.require_create = true;
        // If -legacy is set, use it. Otherwise default to false.
        bool make_legacy = args.GetBoolArg("-legacy", false);
        // If neither -legacy nor -descriptors is set, default to true. If -descriptors is set, use its value.
        bool make_descriptors = (!args.IsArgSet("-descriptors") && !args.IsArgSet("-legacy")) || (args.IsArgSet("-descriptors") && args.GetBoolArg("-descriptors", true));
        if (make_legacy && make_descriptors) {
            tfm::format(std::cerr, "Only one of -legacy or -descriptors can be set to true, not both\n");
            return false;
        }
        if (!make_legacy && !make_descriptors) {
            tfm::format(std::cerr, "One of -legacy or -descriptors must be set to true (or omitted)\n");
            return false;
        }
        if (make_descriptors) {
            options.create_flags |= WALLET_FLAG_DESCRIPTORS;
            options.require_format = DatabaseFormat::SQLITE;
        }

        const std::shared_ptr<CWallet> wallet_instance = MakeWallet(name, path, options);
        if (wallet_instance) {
            WalletShowInfo(wallet_instance.get());
            wallet_instance->Close();
        }
    } else if (command == "info") {
        DatabaseOptions options;
        ReadDatabaseArgs(args, options);
        options.require_existing = true;
        const std::shared_ptr<CWallet> wallet_instance = MakeWallet(name, path, options);
        if (!wallet_instance) return false;
        WalletShowInfo(wallet_instance.get());
        wallet_instance->Close();
    } else if (command == "dump") {
        DatabaseOptions options;
        ReadDatabaseArgs(args, options);
        options.require_existing = true;
        DatabaseStatus status;

        if (IsBDBFile(BDBDataFile(path))) {
            options.require_format = DatabaseFormat::BERKELEY_RO;
        }

        bilingual_str error;
        std::unique_ptr<WalletDatabase> database = MakeDatabase(path, options, status, error);
        if (!database) {
            tfm::format(std::cerr, "%s\n", error.original);
            return false;
        }

        bool ret = DumpWallet(args, *database, error);
        if (!ret && !error.empty()) {
            tfm::format(std::cerr, "%s\n", error.original);
            return ret;
        }
        tfm::format(std::cout, "The dumpfile may contain private keys. To ensure the safety of your Bitcoin, do not share the dumpfile.\n");
        return ret;
    } else if (command == "createfromdump") {
        bilingual_str error;
        std::vector<bilingual_str> warnings;
        bool ret = CreateFromDump(args, name, path, error, warnings);
        for (const auto& warning : warnings) {
            tfm::format(std::cout, "%s\n", warning.original);
        }
        if (!ret && !error.empty()) {
            tfm::format(std::cerr, "%s\n", error.original);
        }
        return ret;
    } else {
        tfm::format(std::cerr, "Invalid command: %s\n", command);
        return false;
    }

    return true;
}
} // namespace WalletTool
} // namespace wallet
