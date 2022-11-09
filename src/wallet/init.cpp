// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <net.h>
#include <node/context.h>
#include <univalue.h>
#include <util/check.h>
#include <util/error.h>
#include <util/system.h>
#include <util/moneystr.h>
#include <util/translation.h>
#include <validation.h>
#include <walletinitinterface.h>
#include <wallet/wallet.h>

#include <coinjoin/client.h>
#include <coinjoin/options.h>

class WalletInit : public WalletInitInterface
{
public:
    //! Was the wallet component compiled in.
    bool HasWalletSupport() const override {return true;}

    //! Return the wallets help message.
    void AddWalletOptions(ArgsManager& argsman) const override;

    //! Wallets parameter interaction
    bool ParameterInteraction() const override;

    //! Add wallets that should be opened to list of chain clients.
    void Construct(NodeContext& node) const override;

    // Dash Specific Wallet Init
    void AutoLockMasternodeCollaterals() const override;
    void InitCoinJoinSettings() const override;
    bool InitAutoBackup() const override;
};

const WalletInitInterface& g_wallet_init_interface = WalletInit();

void WalletInit::AddWalletOptions(ArgsManager& argsman) const
{
    argsman.AddArg("-avoidpartialspends", strprintf("Group outputs by address, selecting all or none, instead of selecting on a per-output basis. Privacy is improved as an address is only used once (unless someone sends to it after spending from it), but may result in slightly higher fees as suboptimal coin selection may result due to the added limitation (default: %u (always enabled for wallets with \"avoid_reuse\" enabled))", DEFAULT_AVOIDPARTIALSPENDS), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-createwalletbackups=<n>", strprintf("Number of automatic wallet backups (default: %u)", nWalletBackups), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-disablewallet", "Do not load the wallet and disable wallet RPC calls", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
#if HAVE_SYSTEM
    argsman.AddArg("-instantsendnotify=<cmd>", "Execute command when a wallet InstantSend transaction is successfully locked (%s in cmd is replaced by TxID)", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
#endif
    argsman.AddArg("-keypool=<n>", strprintf("Set key pool size to <n> (default: %u). Warning: Smaller sizes may increase the risk of losing funds when restoring from an old backup, if none of the addresses in the original keypool have been used.", DEFAULT_KEYPOOL_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-rescan=<mode>", "Rescan the block chain for missing wallet transactions on startup"
                                            " (1 = start from wallet creation time, 2 = start from genesis block)", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-spendzeroconfchange", strprintf("Spend unconfirmed change when sending transactions (default: %u)", DEFAULT_SPEND_ZEROCONF_CHANGE), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-upgradewallet", "Upgrade wallet to latest format on startup", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-wallet=<path>", "Specify wallet database path. Can be specified multiple times to load multiple wallets. Path is interpreted relative to <walletdir> if it is not absolute, and will be created if it does not exist (as a directory containing a wallet.dat file and log files). For backwards compatibility this will also accept names of existing data files in <walletdir>.)", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::WALLET);
    argsman.AddArg("-walletbackupsdir=<dir>", "Specify full path to directory for automatic wallet backups (must exist)", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-walletbroadcast", strprintf("Make the wallet broadcast transactions (default: %u)", DEFAULT_WALLETBROADCAST), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-walletdir=<dir>", "Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::WALLET);
#if HAVE_SYSTEM
    argsman.AddArg("-walletnotify=<cmd>", "Execute command when a wallet transaction changes. %s in cmd is replaced by TxID and %w is replaced by wallet name. %w is not currently implemented on windows. On systems where %w is supported, it should NOT be quoted because this would break shell escaping used to invoke the command.", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
#endif

    argsman.AddArg("-discardfee=<amt>", strprintf("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target",
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-fallbackfee=<amt>", strprintf("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)",
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-maxtxfee=<amt>", strprintf("Maximum total fees (in %s) to use in a single wallet transaction; setting this too low may abort large transactions (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-mintxfee=<amt>", strprintf("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-paytxfee=<amt>", strprintf("Fee (in %s/kB) to add to transactions you send (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(CFeeRate{DEFAULT_PAY_TX_FEE}.GetFeePerK())), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-txconfirmtarget=<n>", strprintf("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)", DEFAULT_TX_CONFIRM_TARGET), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);

    argsman.AddArg("-hdseed=<hex>", "User defined seed for HD wallet (should be in hex). Only has effect during wallet creation/first start (default: randomly generated)", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::WALLET_HD);
    argsman.AddArg("-mnemonic=<text>", "User defined mnemonic for HD wallet (bip39). Only has effect during wallet creation/first start (default: randomly generated)", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::WALLET_HD);
    argsman.AddArg("-mnemonicpassphrase=<text>", "User defined mnemonic passphrase for HD wallet (BIP39). Only has effect during wallet creation/first start (default: empty string)", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::WALLET_HD);
    argsman.AddArg("-usehd", strprintf("Use hierarchical deterministic key generation (HD) after BIP39/BIP44. Only has effect during wallet creation/first start (default: %u)", DEFAULT_USE_HD_WALLET), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_HD);

    argsman.AddArg("-enablecoinjoin", strprintf("Enable use of CoinJoin for funds stored in this wallet (0-1, default: %u)", 0), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);
    argsman.AddArg("-coinjoinamount=<n>", strprintf("Target CoinJoin balance (%u-%u, default: %u)", MIN_COINJOIN_AMOUNT, MAX_COINJOIN_AMOUNT, DEFAULT_COINJOIN_AMOUNT), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);
    argsman.AddArg("-coinjoinautostart", strprintf("Start CoinJoin automatically (0-1, default: %u)", DEFAULT_COINJOIN_AUTOSTART), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);
    argsman.AddArg("-coinjoindenomsgoal=<n>", strprintf("Try to create at least N inputs of each denominated amount (%u-%u, default: %u)", MIN_COINJOIN_DENOMS_GOAL, MAX_COINJOIN_DENOMS_GOAL, DEFAULT_COINJOIN_DENOMS_GOAL), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);
    argsman.AddArg("-coinjoindenomshardcap=<n>", strprintf("Create up to N inputs of each denominated amount (%u-%u, default: %u)", MIN_COINJOIN_DENOMS_HARDCAP, MAX_COINJOIN_DENOMS_HARDCAP, DEFAULT_COINJOIN_DENOMS_HARDCAP), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);
    argsman.AddArg("-coinjoinmultisession", strprintf("Enable multiple CoinJoin mixing sessions per block, experimental (0-1, default: %u)", DEFAULT_COINJOIN_MULTISESSION), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);
    argsman.AddArg("-coinjoinrounds=<n>", strprintf("Use N separate masternodes for each denominated input to mix funds (%u-%u, default: %u)", MIN_COINJOIN_ROUNDS, MAX_COINJOIN_ROUNDS, DEFAULT_COINJOIN_ROUNDS), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);
    argsman.AddArg("-coinjoinsessions=<n>", strprintf("Use N separate masternodes in parallel to mix funds (%u-%u, default: %u)", MIN_COINJOIN_SESSIONS, MAX_COINJOIN_SESSIONS, DEFAULT_COINJOIN_SESSIONS), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_COINJOIN);

    argsman.AddArg("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DEFAULT_WALLET_DBLOGSIZE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-walletrejectlongchains", strprintf("Wallet will not create transactions that violate mempool chain limits (default: %u)", DEFAULT_WALLET_REJECT_LONG_CHAINS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);

    argsman.AddHiddenArgs({"-zapwallettxes"});
}

bool WalletInit::ParameterInteraction() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const std::string& wallet : gArgs.GetArgs("-wallet")) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
        }

        return true;
    } else if (gArgs.IsArgSet("-masternodeblsprivkey")) {
        return InitError(_("You can not start a masternode with wallet enabled."));
    }

    const bool is_multiwallet = gArgs.GetArgs("-wallet").size() > 1;

    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
    }

    if (gArgs.IsArgSet("-zapwallettxes")) {
        return InitError(Untranslated("-zapwallettxes has been removed. If you are attempting to remove a stuck transaction from your wallet, please use abandontransaction instead."));
    }

    int rescan_mode = gArgs.GetArg("-rescan", 0);
    if (rescan_mode < 0 || rescan_mode > 2) {
        LogPrintf("%s: Warning: incorrect -rescan mode, falling back to default value.\n", __func__);
        InitWarning(_("Incorrect -rescan mode, falling back to default value"));
        gArgs.ForceRemoveArg("rescan");
    }

    if (is_multiwallet) {
        if (gArgs.GetBoolArg("-upgradewallet", false)) {
            return InitError(strprintf(_("%s is only allowed with a single wallet file"), "-upgradewallet"));
        }
    }

    if (gArgs.GetBoolArg("-sysperms", false))
        return InitError(Untranslated("-sysperms is not allowed in combination with enabled wallet functionality"));

    if (gArgs.IsArgSet("-walletbackupsdir")) {
        if (!fs::is_directory(gArgs.GetArg("-walletbackupsdir", ""))) {
            InitWarning(strprintf(_("Warning: incorrect parameter %s, path must exist! Using default path."), "-walletbackupsdir"));
            gArgs.ForceRemoveArg("walletbackupsdir");
        }
    }

    if (gArgs.IsArgSet("-hdseed") && IsHex(gArgs.GetArg("-hdseed", "not hex")) && (gArgs.IsArgSet("-mnemonic") || gArgs.IsArgSet("-mnemonicpassphrase"))) {
        InitWarning(strprintf(_("Warning: can't use %s and %s together, will prefer %s"), "-hdseed", "-mnemonic/-mnemonicpassphrase", "-hdseed"));
        gArgs.ForceRemoveArg("mnemonic");
        gArgs.ForceRemoveArg("mnemonicpassphrase");
    }

    if (gArgs.GetArg("-coinjoindenomshardcap", DEFAULT_COINJOIN_DENOMS_HARDCAP) < gArgs.GetArg("-coinjoindenomsgoal", DEFAULT_COINJOIN_DENOMS_GOAL)) {
        return InitError(strprintf(_("%s can't be lower than %s"), "-coinjoindenomshardcap", "-coinjoindenomsgoal"));
    }

    return true;
}

void WalletInit::Construct(NodeContext& node) const
{
    ArgsManager& args = *Assert(node.args);
    if (args.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return;
    }
    // If there's no -wallet setting with a list of wallets to load, set it to
    // load the default "" wallet.
    if (!args.IsArgSet("wallet")) {
        args.LockSettings([&](util::Settings& settings) {
            util::SettingsValue wallets(util::SettingsValue::VARR);
            wallets.push_back(""); // Default wallet name is ""
            settings.rw_settings["wallet"] = wallets;
        });
    }
    auto wallet_client = interfaces::MakeWalletClient(*node.chain, args, args.GetArgs("-wallet"));
    node.wallet_client = wallet_client.get();
    node.chain_clients.emplace_back(std::move(wallet_client));
}


void WalletInit::AutoLockMasternodeCollaterals() const
{
    // we can't do this before DIP3 is fully initialized
    for (const auto& pwallet : GetWallets()) {
        pwallet->AutoLockMasternodeCollaterals();
    }
}

void WalletInit::InitCoinJoinSettings() const
{
    CCoinJoinClientOptions::SetEnabled(!GetWallets().empty() ? gArgs.GetBoolArg("-enablecoinjoin", true) : false);
    if (!CCoinJoinClientOptions::IsEnabled()) {
        return;
    }
    bool fAutoStart = gArgs.GetBoolArg("-coinjoinautostart", DEFAULT_COINJOIN_AUTOSTART);
    for (auto& pwallet : GetWallets()) {
        if (pwallet->IsLocked()) {
            coinJoinClientManagers.at(pwallet->GetName())->StopMixing();
        } else if (fAutoStart) {
            coinJoinClientManagers.at(pwallet->GetName())->StartMixing();
        }
    }
    LogPrintf("CoinJoin: autostart=%d, multisession=%d," /* Continued */
              "sessions=%d, rounds=%d, amount=%d, denoms_goal=%d, denoms_hardcap=%d\n",
              fAutoStart, CCoinJoinClientOptions::IsMultiSessionEnabled(),
              CCoinJoinClientOptions::GetSessions(), CCoinJoinClientOptions::GetRounds(),
              CCoinJoinClientOptions::GetAmount(), CCoinJoinClientOptions::GetDenomsGoal(),
              CCoinJoinClientOptions::GetDenomsHardCap());
}

bool WalletInit::InitAutoBackup() const
{
    return CWallet::InitAutoBackup();
}
