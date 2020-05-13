// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/load.h>

#include <net.h>
#include <coinjoin/client.h>
#include <coinjoin/options.h>
#include <interfaces/chain.h>
#include <scheduler.h>
#include <util/string.h>
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

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet_file : wallet_files) {
        WalletLocation location(wallet_file);

        if (!wallet_paths.insert(location.GetPath()).second) {
            chain.initError(strprintf(_("Error loading wallet %s. Duplicate -wallet filename specified."), wallet_file));
            return false;
        }

        bilingual_str error_string;
        std::vector<bilingual_str> warnings;
        bool verify_success = CWallet::Verify(chain, location, error_string, warnings);
        if (!warnings.empty()) chain.initWarning(Join(warnings, Untranslated("\n")));
        if (!verify_success) {
            chain.initError(error_string);
            return false;
        }
    }

    return true;
}

bool LoadWallets(interfaces::Chain& chain, const std::vector<std::string>& wallet_files)
{
    for (const std::string& walletFile : wallet_files) {
        bilingual_str error_string;
        std::vector<bilingual_str> warnings;
        std::shared_ptr<CWallet> pwallet = CWallet::CreateWalletFromFile(chain, WalletLocation(walletFile), error_string, warnings);
        if (!warnings.empty()) chain.initWarning(Join(warnings, Untranslated("\n")));
        if (!pwallet) {
            chain.initError(error_string);
            return false;
        }
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

    if (!fMasternodeMode && CCoinJoinClientOptions::IsEnabled()) {
        scheduler.scheduleEvery(std::bind(&DoCoinJoinMaintenance, std::ref(*g_connman)), 1 * 1000);
    }
}

void FlushWallets()
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        if (CCoinJoinClientOptions::IsEnabled()) {
            // Stop CoinJoin, release keys
            auto it = coinJoinClientManagers.find(pwallet->GetName());
            it->second->ResetPool();
            it->second->StopMixing();
        }
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
        RemoveWallet(wallet);
        UnloadWallet(std::move(wallet));
    }
}
