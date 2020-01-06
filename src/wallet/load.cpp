// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/load.h>

#include <interfaces/chain.h>
#include <scheduler.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/wallet.h>

bool VerifyWallets(interfaces::Chain& chain, const std::vector<std::string>& wallet_files)
{
    if (gArgs.IsArgSet("-walletdir")) {
        fs::path wallet_dir = gArgs.GetArg("-walletdir", "");
        boost::system::error_code error;
        // The canonical path cleans the path, preventing >1 Berkeley environment instances for the same directory
        fs::path canonical_wallet_dir = fs::canonical(wallet_dir, error);
        if (error || !fs::exists(wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" does not exist").translated, wallet_dir.string()));
            return false;
        } else if (!fs::is_directory(wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is not a directory").translated, wallet_dir.string()));
            return false;
        // The canonical path transforms relative paths into absolute ones, so we check the non-canonical version
        } else if (!wallet_dir.is_absolute()) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is a relative path").translated, wallet_dir.string()));
            return false;
        }
        gArgs.ForceSetArg("-walletdir", canonical_wallet_dir.string());
    }

    LogPrintf("Using wallet directory %s\n", GetWalletDir().string());

    chain.initMessage(_("Verifying wallet(s)...").translated);

    // Parameter interaction code should have thrown an error if -salvagewallet
    // was enabled with more than wallet file, so the wallet_files size check
    // here should have no effect.
    bool salvage_wallet = gArgs.GetBoolArg("-salvagewallet", false) && wallet_files.size() <= 1;

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet_file : wallet_files) {
        WalletLocation location(wallet_file);

        if (!wallet_paths.insert(location.GetPath()).second) {
            chain.initError(strprintf(_("Error loading wallet %s. Duplicate -wallet filename specified.").translated, wallet_file));
            return false;
        }

        std::string error_string;
        std::string warning_string;
        bool verify_success = CWallet::Verify(chain, location, salvage_wallet, error_string, warning_string);
        if (!error_string.empty()) chain.initError(error_string);
        if (!warning_string.empty()) chain.initWarning(warning_string);
        if (!verify_success) return false;
    }

    return true;
}

bool LoadWallets(interfaces::Chain& chain, const std::vector<std::string>& wallet_files)
{
    for (const std::string& walletFile : wallet_files) {
        std::shared_ptr<CWallet> pwallet = CWallet::CreateWalletFromFile(chain, WalletLocation(walletFile));
        if (!pwallet) {
            return false;
        }
        AddWallet(pwallet);
    }

    return true;
}

void StartWallets(CScheduler& scheduler)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->postInitProcess();
    }

    // Schedule periodic wallet flushes and tx rebroadcasts
    scheduler.scheduleEvery(MaybeCompactWalletDB, 500);
    scheduler.scheduleEvery(MaybeResendWalletTxs, 1000);
}

void FlushWallets()
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->Flush(false);
    }
}

void StopWallets()
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->Flush(true);
    }
}

void UnloadWallets()
{
    auto wallets = GetWallets();
    while (!wallets.empty()) {
        auto wallet = wallets.back();
        wallets.pop_back();
        RemoveWallet(wallet);
        UnloadWallet(std::move(wallet));
    }
}
