// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/coinjoin.h>
#include <interfaces/init.h>
#include <interfaces/wallet.h>
#include <net.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <univalue.h>
#include <util/check.h>
#include <util/error.h>
#include <util/system.h>
#include <util/moneystr.h>
#include <util/translation.h>
#include <validation.h>
#ifdef USE_BDB
#include <wallet/bdb.h>
#endif
#include <wallet/bip39.h>
#include <wallet/coincontrol.h>
#include <wallet/hdchain.h>
#include <wallet/wallet.h>
#include <walletinitinterface.h>

#include <coinjoin/client.h>
#include <coinjoin/options.h>

using node::NodeContext;

namespace wallet {
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
    void AutoLockMasternodeCollaterals(interfaces::WalletLoader& wallet_loader) const override;
    void InitCoinJoinSettings(interfaces::CoinJoin::Loader& coinjoin_loader, interfaces::WalletLoader& wallet_loader) const override;
    void InitAutoBackup() const override;
};

void WalletInit::AddWalletOptions(ArgsManager& argsman) const
{
    argsman.AddArg("-avoidpartialspends", strprintf("Group outputs by address, selecting many (possibly all) or none, instead of selecting on a per-output basis. Privacy is improved as addresses are mostly swept with fewer transactions and outputs are aggregated in clean change addresses. It may result in higher fees due to less optimal coin selection caused by this added limitation and possibly a larger-than-necessary number of inputs being used. Always enabled for wallets with \"avoid_reuse\" enabled, otherwise default: %u.", DEFAULT_AVOIDPARTIALSPENDS), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-consolidatefeerate=<amt>", strprintf("The maximum feerate (in %s/kvB) at which transaction building may use more inputs than strictly necessary so that the wallet's UTXO pool can be reduced (default: %s).", CURRENCY_UNIT, FormatMoney(DEFAULT_CONSOLIDATE_FEERATE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-createwalletbackups=<n>", strprintf("Number of automatic wallet backups (default: %u)", nWalletBackups), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-disablewallet", "Do not load the wallet and disable wallet RPC calls", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
#if HAVE_SYSTEM
    argsman.AddArg("-instantsendnotify=<cmd>", "Execute command when a wallet InstantSend transaction is successfully locked. %s in cmd is replaced by TxID and %w is replaced by wallet name. %w is not currently implemented on Windows. On systems where %w is supported, it should NOT be quoted because this would break shell escaping used to invoke the command.", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
#endif
    argsman.AddArg("-keypool=<n>", strprintf("Set key pool size to <n> (default: %u). Warning: Smaller sizes may increase the risk of losing funds when restoring from an old backup, if none of the addresses in the original keypool have been used.", DEFAULT_KEYPOOL_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-rescan=<mode>", "Rescan the block chain for missing wallet transactions on startup"
                                            " (1 = start from wallet creation time, 2 = start from genesis block)", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-spendzeroconfchange", strprintf("Spend unconfirmed change when sending transactions (default: %u)", DEFAULT_SPEND_ZEROCONF_CHANGE), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-wallet=<path>", "Specify wallet path to load at startup. Can be used multiple times to load multiple wallets. Path is to a directory containing wallet data and log files. If the path is not absolute, it is interpreted relative to <walletdir>. This only loads existing wallets and does not create new ones. For backwards compatibility this also accepts names of existing top-level data files in <walletdir>.", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::WALLET);
    argsman.AddArg("-walletbackupsdir=<dir>", "Specify full path to directory for automatic wallet backups (must exist)", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-walletbroadcast", strprintf("Make the wallet broadcast transactions (default: %u)", DEFAULT_WALLETBROADCAST), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-walletdir=<dir>", "Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::WALLET);
#if HAVE_SYSTEM
    argsman.AddArg("-walletnotify=<cmd>", "Execute command when a wallet transaction changes. %s in cmd is replaced by TxID, %w is replaced by wallet name, %b is replaced by the hash of the block including the transaction (set to 'unconfirmed' if the transaction is not included) and %h is replaced by the block height (-1 if not included). %w is not currently implemented on windows. On systems where %w is supported, it should NOT be quoted because this would break shell escaping used to invoke the command.", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
#endif

    argsman.AddArg("-discardfee=<amt>", strprintf("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target",
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-fallbackfee=<amt>", strprintf("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data. 0 to entirely disable the fallbackfee feature. (default: %s)",
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-maxapsfee=<n>", strprintf("Spend up to this amount in additional (absolute) fees (in %s) if it allows the use of partial spend avoidance (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_MAX_AVOIDPARTIALSPEND_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);

    argsman.AddArg("-maxtxfee=<amt>", strprintf("Maximum total fees (in %s) to use in a single wallet transaction; setting this too low may abort large transactions (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-mintxfee=<amt>", strprintf("Fee rates (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-paytxfee=<amt>", strprintf("Fee rate (in %s/kB) to add to transactions you send (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(CFeeRate{DEFAULT_PAY_TX_FEE}.GetFeePerK())), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);
    argsman.AddArg("-txconfirmtarget=<n>", strprintf("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)", DEFAULT_TX_CONFIRM_TARGET), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_FEE);

    argsman.AddArg("-hdseed=<hex>", "User defined seed for HD wallet (should be in hex). Only has effect during wallet creation/first start (default: randomly generated)", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::WALLET_HD);
    argsman.AddArg("-mnemonic=<text>", "User defined mnemonic for HD wallet (bip39). Only has effect during wallet creation/first start (default: randomly generated)", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::WALLET_HD);
    argsman.AddArg("-mnemonicbits=<n>", strprintf("User defined mnemonic security for HD wallet in bits (BIP39). Only has effect during wallet creation/first start (allowed values: 128, 160, 192, 224, 256; default: %u)", CHDChain::DEFAULT_MNEMONIC_BITS), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET_HD);
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

#ifdef USE_BDB
    argsman.AddArg("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DatabaseOptions().max_log_mb), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", !DatabaseOptions().use_shared_memory), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
#else
    argsman.AddHiddenArgs({"-dblogsize", "-flushwallet", "-privdb"});
#endif

#ifdef USE_SQLITE
    argsman.AddArg("-unsafesqlitesync", "Set SQLite synchronous=OFF to disable waiting for the database to sync to disk. This is unsafe and can cause data loss and corruption. This option is only used by tests to improve their performance (default: false)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
#else
    argsman.AddHiddenArgs({"-unsafesqlitesync"});
#endif

    argsman.AddArg("-walletrejectlongchains", strprintf("Wallet will not create transactions that violate mempool chain limits (default: %u)", DEFAULT_WALLET_REJECT_LONG_CHAINS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);

    argsman.AddHiddenArgs({"-zapwallettxes"});
}

bool WalletInit::ParameterInteraction() const
{
#ifdef USE_BDB
     if (!BerkeleyDatabaseSanityCheck()) {
         return InitError(Untranslated("A version conflict was detected between the run-time BerkeleyDB library and the one used during compilation."));
     }
#endif
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const std::string& wallet : gArgs.GetArgs("-wallet")) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
        }

        return true;
    } else if (gArgs.IsArgSet("-masternodeblsprivkey")) {
        return InitError(_("You can not start a masternode with wallet enabled."));
    }

    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
    }

    if (gArgs.IsArgSet("-zapwallettxes")) {
        return InitError(Untranslated("-zapwallettxes has been removed. If you are attempting to remove a stuck transaction from your wallet, please use abandontransaction instead."));
    }

    int rescan_mode = gArgs.GetIntArg("-rescan", 0);
    if (rescan_mode < 0 || rescan_mode > 2) {
        LogPrintf("%s: Warning: incorrect -rescan mode, falling back to default value.\n", __func__);
        InitWarning(_("Incorrect -rescan mode, falling back to default value"));
        gArgs.ForceRemoveArg("rescan");
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

    if (gArgs.GetIntArg("-coinjoindenomshardcap", DEFAULT_COINJOIN_DENOMS_HARDCAP) < gArgs.GetIntArg("-coinjoindenomsgoal", DEFAULT_COINJOIN_DENOMS_GOAL)) {
        return InitError(strprintf(_("%s can't be lower than %s"), "-coinjoindenomshardcap", "-coinjoindenomsgoal"));
    }

    if (CMnemonic::Generate(gArgs.GetIntArg("-mnemonicbits", CHDChain::DEFAULT_MNEMONIC_BITS)) == SecureString()) {
        return InitError(strprintf(_("Invalid '%s'. Allowed values: 128, 160, 192, 224, 256."), "-mnemonicbits"));
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
    node.coinjoin_loader = node.init->makeCoinJoinLoader();
    auto wallet_loader = node.init->makeWalletLoader(*node.chain, *node.coinjoin_loader);
    node.wallet_loader = wallet_loader.get();
    node.chain_clients.emplace_back(std::move(wallet_loader));
}


void WalletInit::AutoLockMasternodeCollaterals(interfaces::WalletLoader& wallet_loader) const
{
    // we can't do this before DIP3 is fully initialized
    for (const auto& wallet : wallet_loader.getWallets()) {
        wallet->autoLockMasternodeCollaterals();
    }
}

void WalletInit::InitCoinJoinSettings(interfaces::CoinJoin::Loader& coinjoin_loader, interfaces::WalletLoader& wallet_loader) const
{
    const auto& wallets{wallet_loader.getWallets()};
    CCoinJoinClientOptions::SetEnabled(!wallets.empty() ? gArgs.GetBoolArg("-enablecoinjoin", true) : false);
    if (!CCoinJoinClientOptions::IsEnabled()) {
        return;
    }
    bool fAutoStart = gArgs.GetBoolArg("-coinjoinautostart", DEFAULT_COINJOIN_AUTOSTART);
    for (auto& wallet : wallets) {
        auto manager = Assert(coinjoin_loader.GetClient(wallet->getWalletName()));
        if (wallet->isLocked(/*fForMixing=*/false)) {
            manager->stopMixing();
            LogPrintf("CoinJoin: Mixing stopped for locked wallet \"%s\"\n", wallet->getWalletName());
        } else if (fAutoStart) {
            manager->startMixing();
            LogPrintf("CoinJoin: Automatic mixing started for wallet \"%s\"\n", wallet->getWalletName());
        }
    }
    LogPrintf("CoinJoin: autostart=%d, multisession=%d," /* Continued */
              "sessions=%d, rounds=%d, amount=%d, denoms_goal=%d, denoms_hardcap=%d\n",
              fAutoStart, CCoinJoinClientOptions::IsMultiSessionEnabled(),
              CCoinJoinClientOptions::GetSessions(), CCoinJoinClientOptions::GetRounds(),
              CCoinJoinClientOptions::GetAmount(), CCoinJoinClientOptions::GetDenomsGoal(),
              CCoinJoinClientOptions::GetDenomsHardCap());
}

void WalletInit::InitAutoBackup() const
{
    CWallet::InitAutoBackup();
}
} // namespace wallet

const WalletInitInterface& g_wallet_init_interface = wallet::WalletInit();
