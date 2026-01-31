// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/load.h>

#include <common/args.h>
#include <interfaces/chain.h>
#include <scheduler.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/string.h>
#include <util/translation.h>
#include <wallet/context.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <univalue.h>

#include <system_error>

using util::Join;

namespace wallet {
bool VerifyWallets(WalletContext& context)
{
    interfaces::Chain& chain = *context.chain;
    ArgsManager& args = *Assert(context.args);

    if (args.IsArgSet("-walletdir")) {
        const fs::path wallet_dir{args.GetPathArg("-walletdir")};
        std::error_code error;
        // The canonical path cleans the path, preventing >1 Berkeley environment instances for the same directory
        // It also lets the fs::exists and fs::is_directory checks below pass on windows, since they return false
        // if a path has trailing slashes, and it strips trailing slashes.
        fs::path canonical_wallet_dir = fs::canonical(wallet_dir, error);
        if (error || !fs::exists(canonical_wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" does not exist"), fs::PathToString(wallet_dir)));
            return false;
        } else if (!fs::is_directory(canonical_wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is not a directory"), fs::PathToString(wallet_dir)));
            return false;
        // The canonical path transforms relative paths into absolute ones, so we check the non-canonical version
        } else if (!wallet_dir.is_absolute()) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is a relative path"), fs::PathToString(wallet_dir)));
            return false;
        }
        args.ForceSetArg("-walletdir", fs::PathToString(canonical_wallet_dir));
    }

    LogInfo("Using wallet directory %s", fs::PathToString(GetWalletDir()));
    // Print general DB information
    LogDBInfo();

    chain.initMessage(_("Verifying wallet(s)…"));

    // For backwards compatibility if an unnamed top level wallet exists in the
    // wallets directory, include it in the default list of wallets to load.
    if (!args.IsArgSet("wallet")) {
        DatabaseOptions options;
        ReadDatabaseArgs(args, options);
        options.require_existing = true;
        options.verify = false;
        if (MakeWalletDatabase("", options)) {
            common::SettingsValue wallets(common::SettingsValue::VARR);
            wallets.push_back(""); // Default wallet name is ""
            // Pass write=false because no need to write file and probably
            // better not to. If unnamed wallet needs to be added next startup
            // and the setting is empty, this code will just run again.
            chain.overwriteRwSetting("wallet", std::move(wallets), interfaces::SettingsAction::SKIP_WRITE);
        }
    }

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet : chain.getSettingsList("wallet")) {
        if (!wallet.isStr()) {
            chain.initError(_("Invalid value detected for '-wallet' or '-nowallet'. "
                              "'-wallet' requires a string value, while '-nowallet' accepts only '1' to disable all wallets"));
            return false;
        }
        const auto& wallet_file = wallet.get_str();
        const fs::path path = fsbridge::AbsPathJoin(GetWalletDir(), fs::PathFromString(wallet_file));

        if (!wallet_paths.insert(path).second) {
            chain.initWarning(strprintf(_("Ignoring duplicate -wallet %s."), wallet_file));
            continue;
        }

        DatabaseOptions options;
        ReadDatabaseArgs(args, options);
        options.require_existing = true;
        options.verify = true;
        auto result{MakeWalletDatabase(wallet_file, options)};
        if (!result) {
            if (result.error() == DatabaseError::FAILED_NOT_FOUND) {
                chain.initWarning(Untranslated(strprintf("Skipping -wallet path that doesn't exist. %s", util::ErrorString(result).original)));
            } else if (result.error() == DatabaseError::FAILED_LEGACY_DISABLED) {
                // Skipping legacy wallets as they will not be loaded.
                // This will be properly communicated to the user during the loading process.
                continue;
            } else {
                chain.initError(util::ErrorString(result));
                return false;
            }
        }
    }

    return true;
}

bool LoadWallets(WalletContext& context)
{
    interfaces::Chain& chain = *context.chain;
    try {
        std::set<fs::path> wallet_paths;
        for (const auto& wallet : chain.getSettingsList("wallet")) {
            if (!wallet.isStr()) {
                chain.initError(_("Invalid value detected for '-wallet' or '-nowallet'. "
                                  "'-wallet' requires a string value, while '-nowallet' accepts only '1' to disable all wallets"));
                return false;
            }
            const auto& name = wallet.get_str();
            if (!wallet_paths.insert(fs::PathFromString(name)).second) {
                continue;
            }
            DatabaseOptions options;
            ReadDatabaseArgs(*context.args, options);
            options.require_existing = true;
            options.verify = false; // No need to verify, assuming verified earlier in VerifyWallets()
            util::Result<void> result;
            auto database{MakeWalletDatabase(name, options) >> result};
            if (!database) {
                if (database.error() == DatabaseError::FAILED_NOT_FOUND) continue;
                if (database.error() == DatabaseError::FAILED_LEGACY_DISABLED) {
                    // Inform user that legacy wallet is not loaded and suggest upgrade options
                    chain.initWarning(util::ErrorString(result));
                    continue;
                }
            }
            chain.initMessage(_("Loading wallet…"));
            auto pwallet{database ? CWallet::Create(context, name, std::move(database.value()), options.create_flags) >> result : nullptr};
            if (result.messages_ptr() && !result.messages_ptr()->warnings.empty()) chain.initWarning(Join(result.messages_ptr()->warnings, Untranslated("\n")));
            if (!pwallet) {
                chain.initError(Join(result.messages_ptr()->errors, Untranslated("\n")));
                return false;
            }

            NotifyWalletLoaded(context, pwallet.value());
            AddWallet(context, pwallet.value());
        }
        return true;
    } catch (const std::runtime_error& e) {
        chain.initError(Untranslated(e.what()));
        return false;
    }
}

void StartWallets(WalletContext& context)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets(context)) {
        pwallet->postInitProcess();
    }

    context.scheduler->scheduleEvery([&context] { MaybeResendWalletTxs(context); }, 1min);
}

void UnloadWallets(WalletContext& context)
{
    auto wallets = GetWallets(context);
    while (!wallets.empty()) {
        auto wallet = wallets.back();
        wallets.pop_back();
        // Note: Warnings returned from RemoveWallet are silently discarded.
        Assume(RemoveWallet(context, wallet, /* load_on_start= */ std::nullopt));
        WaitForDeleteWallet(std::move(wallet));
    }
}
} // namespace wallet
