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

#include <coinjoin/coinjoin-client.h>

#include <functional>

class WalletInit : public WalletInitInterface {
public:

    //! Return the wallets help message.
    std::string GetHelpString(bool showDebug) const override;

    //! Wallets parameter interaction
    bool ParameterInteraction() const override;

    //! Register wallet RPCs.
    void RegisterRPC(CRPCTable &tableRPC) const override;

    //! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
    //  This function will perform salvage on the wallet if requested, as long as only one wallet is
    //  being loaded (WalletParameterInteraction forbids -salvagewallet, -zapwallettxes or -upgradewallet with multiwallet).
    bool Verify() const override;

    //! Load wallet databases.
    bool Open() const override;

    //! Complete startup of wallets.
    void Start(CScheduler& scheduler) const override;

    //! Flush all wallets in preparation for shutdown.
    void Flush() const override;

    //! Stop all wallets. Wallets will be flushed first.
    void Stop() const override;

    //! Close all wallets.
    void Close() const override;

    // Dash Specific Wallet Init
    void AutoLockMasternodeCollaterals() const override;
    void InitCoinJoinSettings() const override;
    void InitKeePass() const override;
    bool InitAutoBackup() const override;
};

const WalletInitInterface& g_wallet_init_interface = WalletInit();

std::string WalletInit::GetHelpString(bool showDebug) const
{
    std::string strUsage = HelpMessageGroup(_("Wallet options:"));
    strUsage += HelpMessageOpt("-createwalletbackups=<n>", strprintf(_("Number of automatic wallet backups (default: %u)"), nWalletBackups));
    strUsage += HelpMessageOpt("-disablewallet", _("Do not load the wallet and disable wallet RPC calls"));
    strUsage += HelpMessageOpt("-keypool=<n>", strprintf(_("Set key pool size to <n> (default: %u)"), DEFAULT_KEYPOOL_SIZE));
    strUsage += HelpMessageOpt("-rescan=<mode>", _("Rescan the block chain for missing wallet transactions on startup") +
                                            " " + _("(1 = start from wallet creation time, 2 = start from genesis block)"));
    strUsage += HelpMessageOpt("-salvagewallet", _("Attempt to recover private keys from a corrupt wallet on startup"));
    strUsage += HelpMessageOpt("-spendzeroconfchange", strprintf(_("Spend unconfirmed change when sending transactions (default: %u)"), DEFAULT_SPEND_ZEROCONF_CHANGE));
    strUsage += HelpMessageOpt("-upgradewallet", _("Upgrade wallet to latest format on startup"));
    strUsage += HelpMessageOpt("-wallet=<path>", _("Specify wallet database path. Can be specified multiple times to load multiple wallets. Path is interpreted relative to <walletdir> if it is not absolute, and will be created if it does not exist (as a directory containing a wallet.dat file and log files). For backwards compatibility this will also accept names of existing data files in <walletdir>.)"));
    strUsage += HelpMessageOpt("-walletbroadcast", _("Make the wallet broadcast transactions") + " " + strprintf(_("(default: %u)"), DEFAULT_WALLETBROADCAST));
    strUsage += HelpMessageOpt("-walletdir=<dir>", _("Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)"));
    strUsage += HelpMessageOpt("-walletnotify=<cmd>", _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)"));
    strUsage += HelpMessageOpt("-zapwallettxes=<mode>", _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup") +
                                                        " " + _("(1 = keep tx meta data e.g. account owner and payment request information, 2 = drop tx meta data)"));
    strUsage += HelpMessageOpt("-walletbackupsdir=<dir>", _("Specify full path to directory for automatic wallet backups (must exist)"));

    strUsage += HelpMessageGroup(_("Wallet fee options:"));
    strUsage += HelpMessageOpt("-discardfee=<amt>", strprintf(_("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target"),
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)));
    strUsage += HelpMessageOpt("-fallbackfee=<amt>", strprintf(_("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)"),
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)));
    strUsage += HelpMessageOpt("-mintxfee=<amt>", strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)));
    strUsage += HelpMessageOpt("-paytxfee=<amt>", strprintf(_("Fee (in %s/kB) to add to transactions you send (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(payTxFee.GetFeePerK())));
    strUsage += HelpMessageOpt("-txconfirmtarget=<n>", strprintf(_("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)"), DEFAULT_TX_CONFIRM_TARGET));

    strUsage += HelpMessageGroup(_("HD wallet options:"));
    strUsage += HelpMessageOpt("-hdseed=<hex>", _("User defined seed for HD wallet (should be in hex). Only has effect during wallet creation/first start (default: randomly generated)"));
    strUsage += HelpMessageOpt("-mnemonic=<text>", _("User defined mnemonic for HD wallet (bip39). Only has effect during wallet creation/first start (default: randomly generated)"));
    strUsage += HelpMessageOpt("-mnemonicpassphrase=<text>", _("User defined mnemonic passphrase for HD wallet (BIP39). Only has effect during wallet creation/first start (default: empty string)"));
    strUsage += HelpMessageOpt("-usehd", _("Use hierarchical deterministic key generation (HD) after BIP39/BIP44. Only has effect during wallet creation/first start") + " " + strprintf(_("(default: %u)"), DEFAULT_USE_HD_WALLET));

    strUsage += HelpMessageGroup(_("KeePass options:"));
    strUsage += HelpMessageOpt("-keepass", strprintf(_("Use KeePass 2 integration using KeePassHttp plugin (default: %u)"), 0));
    strUsage += HelpMessageOpt("-keepassid=<id>", _("KeePassHttp id for the established association"));
    strUsage += HelpMessageOpt("-keepassname=<name>", _("Name to construct url for KeePass entry that stores the wallet passphrase"));
    strUsage += HelpMessageOpt("-keepassport=<port>", strprintf(_("Connect to KeePassHttp on port <port> (default: %u)"), DEFAULT_KEEPASS_HTTP_PORT));
    strUsage += HelpMessageOpt("-keepasskey=<key>", _("KeePassHttp key for AES encrypted communication with KeePass"));

    strUsage += HelpMessageGroup(_("CoinJoin options:"));
    strUsage += HelpMessageOpt("-enablecoinjoin", strprintf(_("Enable use of CoinJoin for funds stored in this wallet (0-1, default: %u)"), 0));
    strUsage += HelpMessageOpt("-coinjoinamount=<n>", strprintf(_("Target CoinJoin balance (%u-%u, default: %u)"), MIN_COINJOIN_AMOUNT, MAX_COINJOIN_AMOUNT, DEFAULT_COINJOIN_AMOUNT));
    strUsage += HelpMessageOpt("-coinjoinautostart", strprintf(_("Start CoinJoin automatically (0-1, default: %u)"), DEFAULT_COINJOIN_AUTOSTART));
    strUsage += HelpMessageOpt("-coinjoindenomsgoal=<n>", strprintf(_("Try to create at least N inputs of each denominated amount (%u-%u, default: %u)"), MIN_COINJOIN_DENOMS_GOAL, MAX_COINJOIN_DENOMS_GOAL, DEFAULT_COINJOIN_DENOMS_GOAL));
    strUsage += HelpMessageOpt("-coinjoindenomshardcap=<n>", strprintf(_("Create up to N inputs of each denominated amount (%u-%u, default: %u)"), MIN_COINJOIN_DENOMS_HARDCAP, MAX_COINJOIN_DENOMS_HARDCAP, DEFAULT_COINJOIN_DENOMS_HARDCAP));
    strUsage += HelpMessageOpt("-coinjoinmultisession", strprintf(_("Enable multiple CoinJoin mixing sessions per block, experimental (0-1, default: %u)"), DEFAULT_COINJOIN_MULTISESSION));
    strUsage += HelpMessageOpt("-coinjoinrounds=<n>", strprintf(_("Use N separate masternodes for each denominated input to mix funds (%u-%u, default: %u)"), MIN_COINJOIN_ROUNDS, MAX_COINJOIN_ROUNDS, DEFAULT_COINJOIN_ROUNDS));
    strUsage += HelpMessageOpt("-coinjoinsessions=<n>", strprintf(_("Use N separate masternodes in parallel to mix funds (%u-%u, default: %u)"), MIN_COINJOIN_SESSIONS, MAX_COINJOIN_SESSIONS, DEFAULT_COINJOIN_SESSIONS));

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

    bool zapwallettxes = gArgs.GetBoolArg("-zapwallettxes", false);
    // -zapwallettxes implies dropping the mempool on startup
    if (zapwallettxes && gArgs.SoftSetBoolArg("-persistmempool", false)) {
        LogPrintf("%s: parameter interaction: -zapwallettxes enabled -> setting -persistmempool=0\n", __func__);
    }

    // -zapwallettxes implies a rescan
    if (zapwallettxes) {
        if (is_multiwallet) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-zapwallettxes"));
        }
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -zapwallettxes enabled -> setting -rescan=1\n", __func__);
        }
    }

    int rescan_mode = gArgs.GetArg("-rescan", 0);
    if (rescan_mode < 0 || rescan_mode > 2) {
        LogPrintf("%s: Warning: incorrect -rescan mode, falling back to default value.\n", __func__);
        InitWarning(_("Incorrect -rescan mode, falling back to default value"));
        gArgs.ForceRemoveArg("-rescan");
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

    // begin PrivateSend -> CoinJoin migration
    if (gArgs.IsArgSet("-privatesendrounds")) {
        int nRoundsDeprecated = gArgs.GetArg("-privatesendrounds", DEFAULT_COINJOIN_ROUNDS);
        InitWarning("Warning: -privatesendrounds is deprecated, please use -coinjoinrounds.\n");
        if (gArgs.SoftSetArg("-coinjoinrounds", itostr(nRoundsDeprecated))) {
            LogPrintf("%s: parameter interaction: -privatesendrounds=%d -> setting -coinjoinrounds=%d\n", __func__, nRoundsDeprecated, nRoundsDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesendrounds");
    }
    if (gArgs.IsArgSet("-privatesendamount")) {
        int nAmountDeprecated = gArgs.GetArg("-privatesendamount", DEFAULT_COINJOIN_AMOUNT);
        InitWarning("Warning: -privatesendamount is deprecated, please use -coinjoinamount.\n");
        if (gArgs.SoftSetArg("-coinjoinamount", itostr(nAmountDeprecated))) {
            LogPrintf("%s: parameter interaction: -privatesendamount=%d -> setting -coinjoinamount=%d\n", __func__, nAmountDeprecated, nAmountDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesendamount");
    }
    if (gArgs.IsArgSet("-privatesenddenomsgoal")) {
        int nDenomsGoalDeprecated = gArgs.GetArg("-privatesenddenomsgoal", DEFAULT_COINJOIN_DENOMS_GOAL);
        InitWarning("Warning: -privatesenddenomsgoal is deprecated, please use -coinjoindenomsgoal.\n");
        if (gArgs.SoftSetArg("-coinjoindenomsgoal", itostr(nDenomsGoalDeprecated))) {
            LogPrintf("%s: parameter interaction: -privatesenddenomsgoal=%d -> setting -coinjoindenomsgoal=%d\n", __func__, nDenomsGoalDeprecated, nDenomsGoalDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesenddenomsgoal");
    }
    if (gArgs.IsArgSet("-privatesenddenomshardcap")) {
        int nDenomsHardcapDeprecated = gArgs.GetArg("-privatesenddenomshardcap", DEFAULT_COINJOIN_DENOMS_HARDCAP);
        InitWarning("Warning: -privatesenddenomshardcap is deprecated, please use -coinjoindenomshardcap.\n");
        if (gArgs.SoftSetArg("-coinjoindenomshardcap", itostr(nDenomsHardcapDeprecated))) {
            LogPrintf("%s: parameter interaction: -privatesenddenomshardcap=%d -> setting -coinjoindenomshardcap=%d\n", __func__, nDenomsHardcapDeprecated, nDenomsHardcapDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesenddenomshardcap");
    }
    if (gArgs.IsArgSet("-privatesendsessions")) {
        int nSessionsDeprecated = gArgs.GetArg("-privatesendsessions", DEFAULT_COINJOIN_SESSIONS);
        InitWarning("Warning: -privatesendsessions is deprecated, please use -coinjoinsessions.\n");
        if (gArgs.SoftSetArg("-coinjoinsessions", itostr(nSessionsDeprecated))) {
            LogPrintf("%s: parameter interaction: -privatesendsessions=%d -> setting -coinjoinsessions=%d\n", __func__, nSessionsDeprecated, nSessionsDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesendsessions");
    }
    if (gArgs.IsArgSet("-enableprivatesend")) {
        bool fEnablePSDeprecated = gArgs.GetBoolArg("-enableprivatesend", 0);
        InitWarning("Warning: -enableprivatesend is deprecated, please use -enablecoinjoin.\n");
        if (gArgs.SoftSetBoolArg("-enablecoinjoin", fEnablePSDeprecated)) {
            LogPrintf("%s: parameter interaction: -enableprivatesend=%d -> setting -enablecoinjoin=%d\n", __func__, fEnablePSDeprecated, fEnablePSDeprecated);
        }
        gArgs.ForceRemoveArg("-enableprivatesend");
    }
    if (gArgs.IsArgSet("-privatesendautostart")) {
        bool fAutoStartDeprecated = gArgs.GetBoolArg("-privatesendautostart", DEFAULT_COINJOIN_AUTOSTART);
        InitWarning("Warning: -privatesendautostart is deprecated, please use -coinjoinautostart.\n");
        if (gArgs.SoftSetBoolArg("-coinjoinautostart", fAutoStartDeprecated)) {
            LogPrintf("%s: parameter interaction: -privatesendautostart=%d -> setting -coinjoinautostart=%d\n", __func__, fAutoStartDeprecated, fAutoStartDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesendautostart");
    }
    if (gArgs.IsArgSet("-privatesendmultisession")) {
        bool fMultiSessionDeprecated = gArgs.GetBoolArg("-privatesendmultisession", DEFAULT_COINJOIN_MULTISESSION);
        InitWarning("Warning: -privatesendmultisession is deprecated, please use -coinjoinmultisession.\n");
        if (gArgs.SoftSetBoolArg("-coinjoinmultisession", fMultiSessionDeprecated)) {
            LogPrintf("%s: parameter interaction: -privatesendmultisession=%d -> setting -coinjoinmultisession=%d\n", __func__, fMultiSessionDeprecated, fMultiSessionDeprecated);
        }
        gArgs.ForceRemoveArg("-privatesendmultisession");
    }
    // end PrivateSend -> CoinJoin migration

    if (gArgs.GetArg("-coinjoindenomshardcap", DEFAULT_COINJOIN_DENOMS_HARDCAP) < gArgs.GetArg("-coinjoindenomsgoal", DEFAULT_COINJOIN_DENOMS_GOAL)) {
        return InitError("-coinjoindenomshardcap can't be lower than -coinjoindenomsgoal");
    }

    return true;
}

void WalletInit::RegisterRPC(CRPCTable &t) const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return;
    }

    RegisterWalletRPCCommands(t);
}

bool WalletInit::Verify() const
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

bool WalletInit::Open() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    for (const std::string& walletFile : gArgs.GetArgs("-wallet")) {
        if (!CWallet::CreateWalletFromFile(walletFile, fs::absolute(walletFile, GetWalletDir()))) {
            return false;
        }
    }

    return true;
}

void WalletInit::Start(CScheduler& scheduler) const
{
    for (CWallet* pwallet : GetWallets()) {
        pwallet->postInitProcess();
    }

    // Run a thread to flush wallet periodically
    scheduler.scheduleEvery(MaybeCompactWalletDB, 500);

    if (!fMasternodeMode && CCoinJoinClientOptions::IsEnabled()) {
        scheduler.scheduleEvery(std::bind(&DoCoinJoinMaintenance, std::ref(*g_connman)), 1 * 1000);
    }
}

void WalletInit::Flush() const
{
    for (CWallet* pwallet : GetWallets()) {
        if (CCoinJoinClientOptions::IsEnabled()) {
            // Stop CoinJoin, release keys
            auto it = coinJoinClientManagers.find(pwallet->GetName());
            it->second->ResetPool();
            it->second->StopMixing();
        }
        pwallet->Flush(false);
    }
}

void WalletInit::Stop() const
{
    for (CWallet* pwallet : GetWallets()) {
        pwallet->Flush(true);
    }
}

void WalletInit::Close() const
{
    for (CWallet* pwallet : GetWallets()) {
        RemoveWallet(pwallet);
        delete pwallet;
    }
}

void WalletInit::AutoLockMasternodeCollaterals() const
{
    // we can't do this before DIP3 is fully initialized
    for (CWallet* pwallet : GetWallets()) {
        pwallet->AutoLockMasternodeCollaterals();
    }
}

void WalletInit::InitCoinJoinSettings() const
{
    CCoinJoinClientOptions::SetEnabled(HasWallets() ? gArgs.GetBoolArg("-enablecoinjoin", true) : false);
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

void WalletInit::InitKeePass() const
{
    keePassInt.init();
}

bool WalletInit::InitAutoBackup() const
{
    return CWallet::InitAutoBackup();
}
