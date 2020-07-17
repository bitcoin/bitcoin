// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <keepass.h>
#include <net.h>
#include <scheduler.h>
#include <util.h>
#include <utilmoneystr.h>
#include <validation.h>
#include <walletinitinterface.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <privatesend/privatesend-client.h>

#include <functional>

class WalletInit : public WalletInitInterface {
public:

    //! Return the wallets help message.
    std::string GetHelpString(bool showDebug) override;

    //! Wallets parameter interaction
    bool ParameterInteraction() override;

    //! Register wallet RPCs.
    void RegisterRPC(CRPCTable &tableRPC) override;

    //! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
    //  This function will perform salvage on the wallet if requested, as long as only one wallet is
    //  being loaded (WalletParameterInteraction forbids -salvagewallet, -zapwallettxes or -upgradewallet with multiwallet).
    bool Verify() override;

    //! Load wallet databases.
    bool Open() override;

    //! Complete startup of wallets.
    void Start(CScheduler& scheduler) override;

    //! Flush all wallets in preparation for shutdown.
    void Flush() override;

    //! Stop all wallets. Wallets will be flushed first.
    void Stop() override;

    //! Close all wallets.
    void Close() override;

    // Dash Specific Wallet Init
    void AutoLockMasternodeCollaterals() override;
    void InitPrivateSendSettings() override;
    void InitKeePass() override;
    bool InitAutoBackup() override;
};

static WalletInit g_wallet_init;
WalletInitInterface* const g_wallet_init_interface = &g_wallet_init;

std::string WalletInit::GetHelpString(bool showDebug)
{
    std::string strUsage = HelpMessageGroup(_("Wallet options:"));
    strUsage += HelpMessageOpt("-disablewallet", _("Do not load the wallet and disable wallet RPC calls"));
    strUsage += HelpMessageOpt("-keypool=<n>", strprintf(_("Set key pool size to <n> (default: %u)"), DEFAULT_KEYPOOL_SIZE));
    strUsage += HelpMessageOpt("-fallbackfee=<amt>", strprintf(_("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)"),
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)));
    strUsage += HelpMessageOpt("-discardfee=<amt>", strprintf(_("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target"),
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)));
    strUsage += HelpMessageOpt("-mintxfee=<amt>", strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)));
    strUsage += HelpMessageOpt("-paytxfee=<amt>", strprintf(_("Fee (in %s/kB) to add to transactions you send (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(payTxFee.GetFeePerK())));
    strUsage += HelpMessageOpt("-rescan", _("Rescan the block chain for missing wallet transactions on startup"));
    strUsage += HelpMessageOpt("-salvagewallet", _("Attempt to recover private keys from a corrupt wallet on startup"));
    strUsage += HelpMessageOpt("-spendzeroconfchange", strprintf(_("Spend unconfirmed change when sending transactions (default: %u)"), DEFAULT_SPEND_ZEROCONF_CHANGE));
    strUsage += HelpMessageOpt("-txconfirmtarget=<n>", strprintf(_("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)"), DEFAULT_TX_CONFIRM_TARGET));
    strUsage += HelpMessageOpt("-usehd", _("Use hierarchical deterministic key generation (HD) after BIP39/BIP44. Only has effect during wallet creation/first start") + " " + strprintf(_("(default: %u)"), DEFAULT_USE_HD_WALLET));
    strUsage += HelpMessageOpt("-mnemonic=<text>", _("User defined mnemonic for HD wallet (bip39). Only has effect during wallet creation/first start (default: randomly generated)"));
    strUsage += HelpMessageOpt("-mnemonicpassphrase=<text>", _("User defined mnemonic passphrase for HD wallet (BIP39). Only has effect during wallet creation/first start (default: empty string)"));
    strUsage += HelpMessageOpt("-hdseed=<hex>", _("User defined seed for HD wallet (should be in hex). Only has effect during wallet creation/first start (default: randomly generated)"));
    strUsage += HelpMessageOpt("-upgradewallet", _("Upgrade wallet to latest format on startup"));
    strUsage += HelpMessageOpt("-wallet=<path>", _("Specify wallet database path. Can be specified multiple times to load multiple wallets. Path is interpreted relative to <walletdir> if it is not absolute, and will be created if it does not exist (as a directory containing a wallet.dat file and log files). For backwards compatibility this will also accept names of existing data files in <walletdir>.)"));
    strUsage += HelpMessageOpt("-walletbroadcast", _("Make the wallet broadcast transactions") + " " + strprintf(_("(default: %u)"), DEFAULT_WALLETBROADCAST));
    strUsage += HelpMessageOpt("-walletdir=<dir>", _("Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)"));
    strUsage += HelpMessageOpt("-walletnotify=<cmd>", _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)"));
    strUsage += HelpMessageOpt("-zapwallettxes=<mode>", _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup") +
                                                        " " + _("(1 = keep tx meta data e.g. account owner and payment request information, 2 = drop tx meta data)"));
    strUsage += HelpMessageOpt("-createwalletbackups=<n>", strprintf(_("Number of automatic wallet backups (default: %u)"), nWalletBackups));
    strUsage += HelpMessageOpt("-walletbackupsdir=<dir>", _("Specify full path to directory for automatic wallet backups (must exist)"));
    strUsage += HelpMessageOpt("-keepass", strprintf(_("Use KeePass 2 integration using KeePassHttp plugin (default: %u)"), 0));
    strUsage += HelpMessageOpt("-keepassport=<port>", strprintf(_("Connect to KeePassHttp on port <port> (default: %u)"), DEFAULT_KEEPASS_HTTP_PORT));
    strUsage += HelpMessageOpt("-keepasskey=<key>", _("KeePassHttp key for AES encrypted communication with KeePass"));
    strUsage += HelpMessageOpt("-keepassid=<id>", _("KeePassHttp id for the established association"));
    strUsage += HelpMessageOpt("-keepassname=<name>", _("Name to construct url for KeePass entry that stores the wallet passphrase"));

    strUsage += HelpMessageGroup(_("PrivateSend options:"));
    strUsage += HelpMessageOpt("-enableprivatesend", strprintf(_("Enable use of PrivateSend for funds stored in this wallet (0-1, default: %u)"), 0));
    strUsage += HelpMessageOpt("-privatesendautostart", strprintf(_("Start PrivateSend automatically (0-1, default: %u)"), DEFAULT_PRIVATESEND_AUTOSTART));
    strUsage += HelpMessageOpt("-privatesendmultisession", strprintf(_("Enable multiple PrivateSend mixing sessions per block, experimental (0-1, default: %u)"), DEFAULT_PRIVATESEND_MULTISESSION));
    strUsage += HelpMessageOpt("-privatesendsessions=<n>", strprintf(_("Use N separate masternodes in parallel to mix funds (%u-%u, default: %u)"), MIN_PRIVATESEND_SESSIONS, MAX_PRIVATESEND_SESSIONS, DEFAULT_PRIVATESEND_SESSIONS));
    strUsage += HelpMessageOpt("-privatesendrounds=<n>", strprintf(_("Use N separate masternodes for each denominated input to mix funds (%u-%u, default: %u)"), MIN_PRIVATESEND_ROUNDS, MAX_PRIVATESEND_ROUNDS, DEFAULT_PRIVATESEND_ROUNDS));
    strUsage += HelpMessageOpt("-privatesendamount=<n>", strprintf(_("Target PrivateSend balance (%u-%u, default: %u)"), MIN_PRIVATESEND_AMOUNT, MAX_PRIVATESEND_AMOUNT, DEFAULT_PRIVATESEND_AMOUNT));
    strUsage += HelpMessageOpt("-privatesenddenomsgoal=<n>", strprintf(_("Try to create at least N inputs of each denominated amount (%u-%u, default: %u)"), MIN_PRIVATESEND_DENOMS_GOAL, MAX_PRIVATESEND_DENOMS_GOAL, DEFAULT_PRIVATESEND_DENOMS_GOAL));
    strUsage += HelpMessageOpt("-privatesenddenomshardcap=<n>", strprintf(_("Create up to N inputs of each denominated amount (%u-%u, default: %u)"), MIN_PRIVATESEND_DENOMS_HARDCAP, MAX_PRIVATESEND_DENOMS_HARDCAP, DEFAULT_PRIVATESEND_DENOMS_HARDCAP));

    if (showDebug)
    {
        strUsage += HelpMessageGroup(_("Wallet debugging/testing options:"));

        strUsage += HelpMessageOpt("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DEFAULT_WALLET_DBLOGSIZE));
        strUsage += HelpMessageOpt("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET));
        strUsage += HelpMessageOpt("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB));
        strUsage += HelpMessageOpt("-walletrejectlongchains", strprintf(_("Wallet will not create transactions that violate mempool chain limits (default: %u)"), DEFAULT_WALLET_REJECT_LONG_CHAINS));
    }

    return strUsage;
}

bool WalletInit::ParameterInteraction()
{
    if (gArgs.IsArgSet("-masternodeblsprivkey") && gArgs.SoftSetBoolArg("-disablewallet", true)) {
        LogPrintf("%s: parameter interaction: -masternodeblsprivkey set -> setting -disablewallet=1\n", __func__);
    }

    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const std::string& wallet : gArgs.GetArgs("-wallet")) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
        }

        return true;
    } else if (gArgs.IsArgSet("-masternodeblsprivkey")) {
        return InitError(_("You can not start a masternode with wallet enabled."));
    }

    gArgs.SoftSetArg("-wallet", "");
    const bool is_multiwallet = gArgs.GetArgs("-wallet").size() > 1;

    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
    }

    if (gArgs.GetBoolArg("-salvagewallet", false)) {
        if (is_multiwallet) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-salvagewallet"));
        }
        // Rewrite just private keys: rescan to find transactions
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -salvagewallet=1 -> setting -rescan=1\n", __func__);
        }
    }

    int zapwallettxes = gArgs.GetArg("-zapwallettxes", 0);
    // -zapwallettxes implies dropping the mempool on startup
    if (zapwallettxes != 0 && gArgs.SoftSetBoolArg("-persistmempool", false)) {
        LogPrintf("%s: parameter interaction: -zapwallettxes=%s -> setting -persistmempool=0\n", __func__, zapwallettxes);
    }

    // -zapwallettxes implies a rescan
    if (zapwallettxes != 0) {
        if (is_multiwallet) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-zapwallettxes"));
        }
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -zapwallettxes=%s -> setting -rescan=1\n", __func__, zapwallettxes);
        }
    }

    if (is_multiwallet) {
        if (gArgs.GetBoolArg("-upgradewallet", false)) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-upgradewallet"));
        }
    }

    if (gArgs.GetBoolArg("-sysperms", false))
        return InitError("-sysperms is not allowed in combination with enabled wallet functionality");
    if (gArgs.GetArg("-prune", 0) && gArgs.GetBoolArg("-rescan", false))
        return InitError(_("Rescans are not possible in pruned mode. You will need to use -reindex which will download the whole blockchain again."));

    if (::minRelayTxFee.GetFeePerK() > HIGH_TX_FEE_PER_KB)
        InitWarning(AmountHighWarn("-minrelaytxfee") + " " +
                    _("The wallet will avoid paying less than the minimum relay fee."));

    if (gArgs.IsArgSet("-mintxfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-mintxfee", ""), n) || 0 == n)
            return InitError(AmountErrMsg("mintxfee", gArgs.GetArg("-mintxfee", "")));
        if (n > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-mintxfee") + " " +
                        _("This is the minimum transaction fee you pay on every transaction."));
        CWallet::minTxFee = CFeeRate(n);
    }
    if (gArgs.IsArgSet("-fallbackfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-fallbackfee", ""), nFeePerK))
            return InitError(strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), gArgs.GetArg("-fallbackfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-fallbackfee") + " " +
                        _("This is the transaction fee you may pay when fee estimates are not available."));
        CWallet::fallbackFee = CFeeRate(nFeePerK);
    }
    if (gArgs.IsArgSet("-discardfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-discardfee", ""), nFeePerK))
            return InitError(strprintf(_("Invalid amount for -discardfee=<amount>: '%s'"), gArgs.GetArg("-discardfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-discardfee") + " " +
                        _("This is the transaction fee you may discard if change is smaller than dust at this level"));
        CWallet::m_discard_rate = CFeeRate(nFeePerK);
    }
    if (gArgs.IsArgSet("-paytxfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-paytxfee", ""), nFeePerK))
            return InitError(AmountErrMsg("paytxfee", gArgs.GetArg("-paytxfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-paytxfee") + " " +
                        _("This is the transaction fee you will pay if you send a transaction."));

        payTxFee = CFeeRate(nFeePerK, 1000);
        if (payTxFee < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
                                       gArgs.GetArg("-paytxfee", ""), ::minRelayTxFee.ToString()));
        }
    }
    if (gArgs.IsArgSet("-maxtxfee"))
    {
        CAmount nMaxFee = 0;
        if (!ParseMoney(gArgs.GetArg("-maxtxfee", ""), nMaxFee))
            return InitError(AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", "")));
        if (nMaxFee > HIGH_MAX_TX_FEE)
            InitWarning(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        maxTxFee = nMaxFee;
        if (CFeeRate(maxTxFee, 1000) < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                                       gArgs.GetArg("-maxtxfee", ""), ::minRelayTxFee.ToString()));
        }
    }
    nTxConfirmTarget = gArgs.GetArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    bSpendZeroConfChange = gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);

    if (gArgs.IsArgSet("-walletbackupsdir")) {
        if (!fs::is_directory(gArgs.GetArg("-walletbackupsdir", ""))) {
            LogPrintf("%s: Warning: incorrect parameter -walletbackupsdir, path must exist! Using default path.\n", __func__);
            InitWarning("Warning: incorrect parameter -walletbackupsdir, path must exist! Using default path.\n");

            gArgs.ForceRemoveArg("-walletbackupsdir");
        }
    }

    if (gArgs.IsArgSet("-hdseed") && IsHex(gArgs.GetArg("-hdseed", "not hex")) && (gArgs.IsArgSet("-mnemonic") || gArgs.IsArgSet("-mnemonicpassphrase"))) {
        gArgs.ForceRemoveArg("-mnemonic");
        gArgs.ForceRemoveArg("-mnemonicpassphrase");
        LogPrintf("%s: parameter interaction: can't use -hdseed and -mnemonic/-mnemonicpassphrase together, will prefer -seed\n", __func__);
    }

    if (gArgs.IsArgSet("-privatesenddenoms")) {
        int nDenomsDeprecated = gArgs.GetArg("-privatesenddenoms", DEFAULT_PRIVATESEND_DENOMS_HARDCAP);
        InitWarning("Warning: -privatesenddenoms is deprecated, please use -privatesenddenomshardcap or -privatesenddenomsgoal.\n");
        if (gArgs.SoftSetArg("-privatesenddenomshardcap", itostr(nDenomsDeprecated))) {
            LogPrintf("%s: parameter interaction: -privatesenddenoms=%d -> setting -privatesenddenomshardcap=%d\n", __func__, nDenomsDeprecated, nDenomsDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesenddenoms");
    }

    if (gArgs.GetArg("-privatesenddenomshardcap", DEFAULT_PRIVATESEND_DENOMS_HARDCAP) < gArgs.GetArg("-privatesenddenomsgoal", DEFAULT_PRIVATESEND_DENOMS_GOAL)) {
        return InitError("-privatesenddenomshardcap can't be lower than -privatesenddenomsgoal");
    }

    return true;
}

void WalletInit::RegisterRPC(CRPCTable &t)
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return;
    }

    RegisterWalletRPCCommands(t);
}

bool WalletInit::Verify()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return true;
    }

    if (gArgs.IsArgSet("-walletdir")) {
        fs::path wallet_dir = gArgs.GetArg("-walletdir", "");
        if (!fs::exists(wallet_dir)) {
            return InitError(strprintf(_("Specified -walletdir \"%s\" does not exist"), wallet_dir.string()));
        } else if (!fs::is_directory(wallet_dir)) {
            return InitError(strprintf(_("Specified -walletdir \"%s\" is not a directory"), wallet_dir.string()));
        } else if (!wallet_dir.is_absolute()) {
            return InitError(strprintf(_("Specified -walletdir \"%s\" is a relative path"), wallet_dir.string()));
        }
    }

    LogPrintf("Using wallet directory %s\n", GetWalletDir().string());

    uiInterface.InitMessage(_("Verifying wallet(s)..."));

    std::vector<std::string> wallet_files = gArgs.GetArgs("-wallet");

    // Parameter interaction code should have thrown an error if -salvagewallet
    // was enabled with more than wallet file, so the wallet_files size check
    // here should have no effect.
    bool salvage_wallet = gArgs.GetBoolArg("-salvagewallet", false) && wallet_files.size() <= 1;

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet_file : wallet_files) {
        fs::path wallet_path = fs::absolute(wallet_file, GetWalletDir());

        if (!wallet_paths.insert(wallet_path).second) {
            return InitError(strprintf(_("Error loading wallet %s. Duplicate -wallet filename specified."), wallet_file));
        }

        std::string error_string;
        std::string warning_string;
        bool verify_success = CWallet::Verify(wallet_file, salvage_wallet, error_string, warning_string);
        if (!error_string.empty()) InitError(error_string);
        if (!warning_string.empty()) InitWarning(warning_string);
        if (!verify_success) return false;
    }

    return true;
}

bool WalletInit::Open()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    for (const std::string& walletFile : gArgs.GetArgs("-wallet")) {
        CWallet * const pwallet = CWallet::CreateWalletFromFile(walletFile, fs::absolute(walletFile, GetWalletDir()));
        if (!pwallet) {
            return false;
        }
        AddWallet(pwallet);
    }

    return true;
}

void WalletInit::Start(CScheduler& scheduler)
{
    for (CWallet* pwallet : GetWallets()) {
        pwallet->postInitProcess();
    }

    // Run a thread to flush wallet periodically
    scheduler.scheduleEvery(MaybeCompactWalletDB, 500);

    if (!fMasternodeMode && CPrivateSendClientOptions::IsEnabled()) {
        scheduler.scheduleEvery(std::bind(&DoPrivateSendMaintenance, std::ref(*g_connman)), 1 * 1000);
    }
}

void WalletInit::Flush()
{
    for (CWallet* pwallet : GetWallets()) {
        if (CPrivateSendClientOptions::IsEnabled()) {
            // Stop PrivateSend, release keys
            auto it = privateSendClientManagers.find(pwallet->GetName());
            it->second->ResetPool();
            it->second->StopMixing();
        }
        pwallet->Flush(false);
    }
}

void WalletInit::Stop()
{
    for (CWallet* pwallet : GetWallets()) {
        pwallet->Flush(true);
    }
}

void WalletInit::Close()
{
    for (CWallet* pwallet : GetWallets()) {
        RemoveWallet(pwallet);
        delete pwallet;
    }
}

void WalletInit::AutoLockMasternodeCollaterals()
{
    // we can't do this before DIP3 is fully initialized
    for (CWallet* pwallet : GetWallets()) {
        pwallet->AutoLockMasternodeCollaterals();
    }
}

void WalletInit::InitPrivateSendSettings()
{
    CPrivateSendClientOptions::SetEnabled(HasWallets() ? gArgs.GetBoolArg("-enableprivatesend", true) : false);
    if (!CPrivateSendClientOptions::IsEnabled()) {
        return;
    }
    bool fAutoStart = gArgs.GetBoolArg("-privatesendautostart", DEFAULT_PRIVATESEND_AUTOSTART);
    for (auto& pwallet : GetWallets()) {
        if (pwallet->IsLocked()) {
            privateSendClientManagers.at(pwallet->GetName())->StopMixing();
        } else if (fAutoStart) {
            privateSendClientManagers.at(pwallet->GetName())->StartMixing(pwallet);
        }
    }
    LogPrintf("PrivateSend: autostart=%d, multisession=%d," /* Continued */
              "sessions=%d, rounds=%d, amount=%d, denoms_goal=%d, denoms_hardcap=%d\n",
              fAutoStart, CPrivateSendClientOptions::IsMultiSessionEnabled(),
              CPrivateSendClientOptions::GetSessions(), CPrivateSendClientOptions::GetRounds(),
              CPrivateSendClientOptions::GetAmount(), CPrivateSendClientOptions::GetDenomsGoal(),
              CPrivateSendClientOptions::GetDenomsHardCap());
}

void WalletInit::InitKeePass()
{
    keePassInt.init();
}

bool WalletInit::InitAutoBackup()
{
    return CWallet::InitAutoBackup();
}
