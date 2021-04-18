// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/load.h>

#include <fs.h>
#include <interfaces/chain.h>
#include <scheduler.h>
#include <util/string.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <univalue.h>

bool VerifyWallets(interfaces::Chain& chain)
{
    if (gArgs.IsArgSet("-walletdir")) {
        fs::path wallet_dir = gArgs.GetArg("-walletdir", "");
        boost::system::error_code error;
        // The canonical path cleans the path, preventing >1 Berkeley environment instances for the same directory
        fs::path canonical_wallet_dir = fs::canonical(wallet_dir, error);
        if (error || !fs::exists(wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" does not exist"), wallet_dir.string()));
            return false;
        } else if (!fs::is_directory(wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is not a directory"), wallet_dir.string()));
            return false;
        // The canonical path transforms relative paths into absolute ones, so we check the non-canonical version
        } else if (!wallet_dir.is_absolute()) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is a relative path"), wallet_dir.string()));
            return false;
        }
        gArgs.ForceSetArg("-walletdir", canonical_wallet_dir.string());
    }

    LogPrintf("Using wallet directory %s\n", GetWalletDir().string());

    chain.initMessage(_("Verifying wallet(s)...").translated);

    // For backwards compatibility if an unnamed top level wallet exists in the
    // wallets directory, include it in the default list of wallets to load.
    if (!gArgs.IsArgSet("wallet")) {
        DatabaseOptions options;
        DatabaseStatus status;
        bilingual_str error_string;
        options.require_existing = true;
        options.verify = false;
        if (MakeWalletDatabase("", options, status, error_string)) {
            gArgs.LockSettings([&](util::Settings& settings) {
                util::SettingsValue wallets(util::SettingsValue::VARR);
                wallets.push_back(""); // Default wallet name is ""
                settings.rw_settings["wallet"] = wallets;
            });
        }
    }

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet_file : gArgs.GetArgs("-wallet")) {
        const fs::path path = fs::absolute(wallet_file, GetWalletDir());

        if (!wallet_paths.insert(path).second) {
            chain.initWarning(strprintf(_("Ignoring duplicate -wallet %s."), wallet_file));
            continue;
        }

        DatabaseOptions options;
        DatabaseStatus status;
        options.require_existing = true;
        options.verify = true;
        bilingual_str error_string;
        if (!MakeWalletDatabase(wallet_file, options, status, error_string)) {
            if (status == DatabaseStatus::FAILED_NOT_FOUND) {
                chain.initWarning(Untranslated(strprintf("Skipping -wallet path that doesn't exist. %s\n", error_string.original)));
            } else {
                chain.initError(error_string);
                return false;
            }
        }
    }

    return true;
}

bool LoadWallets(interfaces::Chain& chain)
{
    try {
        std::set<fs::path> wallet_paths;
        for (const std::string& name : gArgs.GetArgs("-wallet")) {
            if (!wallet_paths.insert(name).second) {
                continue;
            }
            DatabaseOptions options;
            DatabaseStatus status;
            options.require_existing = true;
            options.verify = false; // No need to verify, assuming verified earlier in VerifyWallets()
            bilingual_str error;
            std::vector<bilingual_str> warnings;
            std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(name, options, status, error);
            if (!database && status == DatabaseStatus::FAILED_NOT_FOUND) {
                continue;
            }
            std::shared_ptr<CWallet> pwallet = database ? CWallet::Create(chain, name, std::move(database), options.create_flags, error, warnings) : nullptr;
            if (!warnings.empty()) chain.initWarning(Join(warnings, Untranslated("\n")));
            if (!pwallet) {
                chain.initError(error);
                return false;
            }
            AddWallet(pwallet);
        }
        return true;
    } catch (const std::runtime_error& e) {
        chain.initError(Untranslated(e.what()));
        return false;
    }
}

void StartWallets(CScheduler& scheduler, const ArgsManager& args)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->postInitProcess();
    }

    // Schedule periodic wallet flushes and tx rebroadcasts
    if (args.GetBoolArg("-flushwallet", DEFAULT_FLUSHWALLET)) {
        scheduler.scheduleEvery(MaybeCompactWalletDB, std::chrono::milliseconds{500});
    }
    scheduler.scheduleEvery(MaybeResendWalletTxs, std::chrono::milliseconds{1000});
}

void FlushWallets()
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->Flush();
    }
}

void StopWallets()
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->Close();
    }
}

void UnloadWallets()
{
    auto wallets = GetWallets();
    while (!wallets.empty()) {
        auto wallet = wallets.back();
        wallets.pop_back();
        std::vector<bilingual_str> warnings;
        RemoveWallet(wallet, nullopt, warnings);
        UnloadWallet(std::move(wallet));
    }
}
