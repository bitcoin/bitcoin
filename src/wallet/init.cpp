// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <init.h>
#include <interfaces/moduleinterface.h>
#include <net.h>
#include <util.h>
#include <utilmoneystr.h>
#include <validation.h>
#include <wallet/keepass.h>
#include <wallet/privatesend-client.h>
#include <walletinitinterface.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

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
    void Start(CScheduler& scheduler, CConnman* connman) const override;

    //! Flush all wallets in preparation for shutdown.
    void Flush() const override;

    //! Stop all wallets. Wallets will be flushed first.
    void Stop() const override;

    //! Close all wallets.
    void Close() const override;
};

const WalletInitInterface& g_wallet_init_interface = WalletInit();

std::string WalletInit::GetHelpString(bool showDebug) const
{
    std::string strUsage = HelpMessageGroup(_("Wallet options:"));
    strUsage += HelpMessageOpt("-addresstype", strprintf(_("What type of addresses to use (\"legacy\", \"p2sh-segwit\", or \"bech32\", default: \"%s\")"), FormatOutputType(DEFAULT_ADDRESS_TYPE)));
    strUsage += HelpMessageOpt("-changetype", _("What type of change to use (\"legacy\", \"p2sh-segwit\", or \"bech32\"). Default is same as -addresstype, except when -addresstype=p2sh-segwit a native segwit output is used when sending to a native segwit address)"));
    strUsage += HelpMessageOpt("-disablewallet", _("Do not load the wallet and disable wallet RPC calls"));
    strUsage += HelpMessageOpt("-discardfee=<amt>", strprintf(_("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target"),
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)));
    strUsage += HelpMessageOpt("-fallbackfee=<amt>", strprintf(_("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)"),
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)));
    strUsage += HelpMessageOpt("-keypool=<n>", strprintf(_("Set key pool size to <n> (default: %u)"), DEFAULT_KEYPOOL_SIZE));
    strUsage += HelpMessageOpt("-mintxfee=<amt>", strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)));
    strUsage += HelpMessageOpt("-paytxfee=<amt>", strprintf(_("Fee (in %s/kB) to add to transactions you send (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(payTxFee.GetFeePerK())));
    strUsage += HelpMessageOpt("-rescan", _("Rescan the block chain for missing wallet transactions on startup"));
    strUsage += HelpMessageOpt("-salvagewallet", _("Attempt to recover private keys from a corrupt wallet on startup"));
    strUsage += HelpMessageOpt("-spendzeroconfchange", strprintf(_("Spend unconfirmed change when sending transactions (default: %u)"), DEFAULT_SPEND_ZEROCONF_CHANGE));
    strUsage += HelpMessageOpt("-txconfirmtarget=<n>", strprintf(_("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)"), DEFAULT_TX_CONFIRM_TARGET));
    strUsage += HelpMessageOpt("-upgradewallet", _("Upgrade wallet to latest format on startup"));
    strUsage += HelpMessageOpt("-wallet=<path>", _("Specify wallet database path. Can be specified multiple times to load multiple wallets. Path is interpreted relative to <walletdir> if it is not absolute, and will be created if it does not exist (as a directory containing a wallet.dat file and log files). For backwards compatibility this will also accept names of existing data files in <walletdir>.)"));
    strUsage += HelpMessageOpt("-walletbroadcast", _("Make the wallet broadcast transactions") + " " + strprintf(_("(default: %u)"), DEFAULT_WALLETBROADCAST));
    strUsage += HelpMessageOpt("-walletdir=<dir>", _("Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)"));
    strUsage += HelpMessageOpt("-walletnotify=<cmd>", _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)"));
    strUsage += HelpMessageOpt("-walletrbf", strprintf(_("Send transactions with full-RBF opt-in enabled (RPC only, default: %u)"), DEFAULT_WALLET_RBF));
    strUsage += HelpMessageOpt("-zapwallettxes=<mode>", _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup") +
                               " " + _("(1 = keep tx meta data e.g. account owner and payment request information, 2 = drop tx meta data)"));

    strUsage += HelpMessageGroup(_("PrivateSend options:"));
    strUsage += HelpMessageOpt("-enableprivatesend=<n>", strprintf(_("Enable use of automated PrivateSend for funds stored in this wallet (0-1, default: %u)"), 0));
    strUsage += HelpMessageOpt("-privatesendmultisession=<n>", strprintf(_("Enable multiple PrivateSend mixing sessions per block, experimental (0-1, default: %u)"), DEFAULT_PRIVATESEND_MULTISESSION));
    strUsage += HelpMessageOpt("-privatesendrounds=<n>", strprintf(_("Use N separate masternodes for each denominated input to mix funds (2-16, default: %u)"), DEFAULT_PRIVATESEND_ROUNDS));
    strUsage += HelpMessageOpt("-privatesendamount=<n>", strprintf(_("Keep N CHC anonymized (default: %u)"), DEFAULT_PRIVATESEND_AMOUNT));
    strUsage += HelpMessageOpt("-liquidityprovider=<n>", strprintf(_("Provide liquidity to PrivateSend by infrequently mixing coins on a continual basis (0-100, default: %u, 1=very frequent, high fees, 100=very infrequent, low fees)"), DEFAULT_PRIVATESEND_LIQUIDITY));

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

    g_wallet_allow_fallback_fee = Params().IsFallbackFeeEnabled();
    if (gArgs.IsArgSet("-fallbackfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-fallbackfee", ""), nFeePerK))
            return InitError(strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), gArgs.GetArg("-fallbackfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-fallbackfee") + " " +
                        _("This is the transaction fee you may pay when fee estimates are not available."));
        CWallet::fallbackFee = CFeeRate(nFeePerK);
        g_wallet_allow_fallback_fee = nFeePerK != 0; //disable fallback fee in case value was set to 0, enable if non-null value
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
    fWalletRbf = gArgs.GetBoolArg("-walletrbf", DEFAULT_WALLET_RBF);

    if (gArgs.IsArgSet("-keepass")) {
        keePassInt.init(); // Initialize KeePass Integration
    }

    privateSendClient.nLiquidityProvider = std::min(std::max((int)gArgs.GetArg("-liquidityprovider", DEFAULT_PRIVATESEND_LIQUIDITY), MIN_PRIVATESEND_LIQUIDITY), MAX_PRIVATESEND_LIQUIDITY);
    int nMaxRounds = MAX_PRIVATESEND_ROUNDS;
    if(privateSendClient.nLiquidityProvider) {
        // special case for liquidity providers only, normal clients should use default value
        privateSendClient.SetMinBlocksToWait(privateSendClient.nLiquidityProvider);
        nMaxRounds = std::numeric_limits<int>::max();
    }

    privateSendClient.fEnablePrivateSend = gArgs.GetBoolArg("-enableprivatesend", false);
    privateSendClient.fPrivateSendMultiSession = gArgs.GetBoolArg("-privatesendmultisession", DEFAULT_PRIVATESEND_MULTISESSION);
    privateSendClient.nPrivateSendRounds = std::min(std::max((int)gArgs.GetArg("-privatesendrounds", DEFAULT_PRIVATESEND_ROUNDS), MIN_PRIVATESEND_ROUNDS), nMaxRounds);
    privateSendClient.nPrivateSendAmount = std::min(std::max((int)gArgs.GetArg("-privatesendamount", DEFAULT_PRIVATESEND_AMOUNT), MIN_PRIVATESEND_AMOUNT), MAX_PRIVATESEND_AMOUNT);

    int nLiqProvTmp = gArgs.GetArg("-liquidityprovider", DEFAULT_PRIVATESEND_LIQUIDITY);
    if (nLiqProvTmp > 0) {
        gArgs.ForceSetArg("-enableprivatesend", "1");
        LogPrintf("%s: parameter interaction: -liquidityprovider=%d -> setting -enableprivatesend=1\n", __func__, nLiqProvTmp);
        gArgs.ForceSetArg("-privatesendrounds", itostr(std::numeric_limits<int>::max()));
        LogPrintf("%s: parameter interaction: -liquidityprovider=%d -> setting -privatesendrounds=%d\n", __func__, nLiqProvTmp, itostr(std::numeric_limits<int>::max()));
        gArgs.ForceSetArg("-privatesendamount", itostr(MAX_PRIVATESEND_AMOUNT));
        LogPrintf("%s: parameter interaction: -liquidityprovider=%d -> setting -privatesendamount=%d\n", __func__, nLiqProvTmp, MAX_PRIVATESEND_AMOUNT);
    }

    LogPrintf("PrivateSend liquidityprovider: %d\n", privateSendClient.nLiquidityProvider);
    LogPrintf("PrivateSend rounds: %d\n", privateSendClient.nPrivateSendRounds);
    LogPrintf("PrivateSend amount: %d\n", privateSendClient.nPrivateSendAmount);

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

    uiInterface.InitMessage(_("Verifying and backing up wallet(s)..."));

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const std::string& walletFile : gArgs.GetArgs("-wallet")) {
        // Do some checking on wallet path. It should be either a:
        //
        // 1. Path where a directory can be created.
        // 2. Path to an existing directory.
        // 3. Path to a symlink to a directory.
        // 4. For backwards compatibility, the name of a data file in -walletdir.
        fs::path wallet_path = fs::absolute(walletFile, GetWalletDir());
        fs::file_type path_type = fs::symlink_status(wallet_path).type();
        if (!(path_type == fs::file_not_found || path_type == fs::directory_file ||
              (path_type == fs::symlink_file && fs::is_directory(wallet_path)) ||
              (path_type == fs::regular_file && fs::path(walletFile).filename() == walletFile))) {
            return InitError(strprintf(
                _("Invalid -wallet path '%s'. -wallet path should point to a directory where wallet.dat and "
                  "database/log.?????????? files can be stored, a location where such a directory could be created, "
                  "or (for backwards compatibility) the name of an existing data file in -walletdir (%s)"),
                walletFile, GetWalletDir()));
        }

        if (!wallet_paths.insert(wallet_path).second) {
            return InitError(strprintf(_("Error loading wallet %s. Duplicate -wallet filename specified."), walletFile));
        }

        std::string strError;
        if (!WalletBatch::VerifyEnvironment(wallet_path, strError)) {
            return InitError(strError);
        }

        if (gArgs.GetBoolArg("-salvagewallet", false)) {
            // Recover readable keypairs:
            CWallet dummyWallet("dummy", WalletDatabase::CreateDummy());
            std::string backup_filename;
            if (!WalletBatch::Recover(wallet_path, (void *)&dummyWallet, WalletBatch::RecoverKeysOnlyFilter, backup_filename)) {
                return false;
            }
        }

        std::string strWarning;
        bool dbV = WalletBatch::VerifyDatabaseFile(wallet_path, strWarning, strError);
        if (!strWarning.empty()) {
            InitWarning(strWarning);
        }
        if (!dbV) {
            InitError(strError);
            return false;
        }

        if(!AutoBackupWallet(nullptr, walletFile, strWarning, strError)) {
            if (!strWarning.empty())
                InitWarning(strWarning);
            if (!strError.empty())
                return InitError(strError);
        }
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
        CWallet * const pwallet = CWallet::CreateWalletFromFile(walletFile, fs::absolute(walletFile, GetWalletDir()));
        if (!pwallet) {
            return false;
        }
        vpwallets.push_back(pwallet);
    }

    return true;
}

void WalletInit::Start(CScheduler& scheduler, CConnman* connman) const
{
    for (CWallet* pwallet : vpwallets) {
        pwallet->postInitProcess(scheduler, gArgs.GetBoolArg("-mnconflock", true) ? true : false);
    }
    if (privateSendClient.getWallet(vpwallets[0]->GetName())) {
        privateSendClient.Controller(scheduler, connman);
    }
}

void WalletInit::Flush() const
{
    for (CWallet* pwallet : vpwallets) {
        pwallet->Flush(false);
    }
}

void WalletInit::Stop() const
{
    for (CWallet* pwallet : vpwallets) {
        pwallet->Flush(true);
    }
    // Stop PrivateSend, release keys
    privateSendClient.fEnablePrivateSend = false;
    privateSendClient.ResetPool();
}

void WalletInit::Close() const
{
    for (CWallet* pwallet : vpwallets) {
        delete pwallet;
    }
    vpwallets.clear();
}

class CWalletInterface : public WalletInterface {
public:
    /** Check MN Collateral */
    bool CheckMNCollateral(COutPoint& outpointRet, CTxDestination &destRet, CPubKey& pubKeyRet, CKey& keyRet, const std::string& strTxHash, const std::string& strOutputIndex) override;
    /** Return MN mixing state */
    bool IsMixingMasternode(const CNode* pnode) override;
};

static CWalletInterface g_wallet;
WalletInterface* const g_wallet_interface = &g_wallet;

bool CWalletInterface::CheckMNCollateral(COutPoint& outpointRet, CTxDestination &destRet, CPubKey& pubKeyRet, CKey& keyRet, const std::string& strTxHash, const std::string& strOutputIndex)
{
    bool foundmnout = false;
    for (CWallet* pwallet : vpwallets) {
        if (pwallet->GetMasternodeOutpointAndKeys(outpointRet, destRet, pubKeyRet, keyRet, strTxHash, strOutputIndex))
            foundmnout = true;
    }
    return foundmnout;
}

bool CWalletInterface::IsMixingMasternode(const CNode* pnode)
{
    return privateSendClient.IsMixingMasternode(pnode);
}

