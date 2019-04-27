// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <init.h>
#include <interfaces/chain.h>
#include <net.h>
#include <scheduler.h>
#include <outputtype.h>
#include <util/error.h>
#include <util/system.h>
#include <util/moneystr.h>
#include <validation.h>
#include <walletinitinterface.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

class WalletInit : public WalletInitInterface {
public:

    //! Was the wallet component compiled in.
    bool HasWalletSupport() const override {return true;}

    //! Return the wallets help message.
    void AddWalletOptions() const override;

    //! Wallets parameter interaction
    bool ParameterInteraction() const override;

    //! Add wallets that should be opened to list of init interfaces.
    void Construct(InitInterfaces& interfaces) const override;
};

const WalletInitInterface& g_wallet_init_interface = WalletInit();

void WalletInit::AddWalletOptions() const
{
    gArgs.AddArg("-addresstype", strprintf("What type of addresses to use (\"legacy\", \"p2sh-segwit\", or \"bech32\", default: \"%s\")", FormatOutputType(DEFAULT_ADDRESS_TYPE)), false, OptionsCategory::WALLET);
    gArgs.AddArg("-avoidpartialspends", strprintf("Group outputs by address, selecting all or none, instead of selecting on a per-output basis. Privacy is improved as an address is only used once (unless someone sends to it after spending from it), but may result in slightly higher fees as suboptimal coin selection may result due to the added limitation (default: %u)", DEFAULT_AVOIDPARTIALSPENDS), false, OptionsCategory::WALLET);
    gArgs.AddArg("-changetype", "What type of change to use (\"legacy\", \"p2sh-segwit\", or \"bech32\"). Default is same as -addresstype, except when -addresstype=p2sh-segwit a native segwit output is used when sending to a native segwit address)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-disablewallet", "Do not load the wallet and disable wallet RPC calls", false, OptionsCategory::WALLET);
    gArgs.AddArg("-discardfee=<amt>", strprintf("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target",
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)), false, OptionsCategory::WALLET);
    gArgs.AddArg("-fallbackfee=<amt>", strprintf("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)",
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)), false, OptionsCategory::WALLET);
    gArgs.AddArg("-keypool=<n>", strprintf("Set key pool size to <n> (default: %u)", DEFAULT_KEYPOOL_SIZE), false, OptionsCategory::WALLET);
    gArgs.AddArg("-maxtxfee=<amt>", strprintf("Maximum total fees (in %s) to use in a single wallet transaction; setting this too low may abort large transactions (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-mintxfee=<amt>", strprintf("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)), false, OptionsCategory::WALLET);
    gArgs.AddArg("-paytxfee=<amt>", strprintf("Fee (in %s/kB) to add to transactions you send (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(CFeeRate{DEFAULT_PAY_TX_FEE}.GetFeePerK())), false, OptionsCategory::WALLET);
    gArgs.AddArg("-rescan", "Rescan the block chain for missing wallet transactions on startup", false, OptionsCategory::WALLET);
    gArgs.AddArg("-salvagewallet", "Attempt to recover private keys from a corrupt wallet on startup", false, OptionsCategory::WALLET);
    gArgs.AddArg("-spendzeroconfchange", strprintf("Spend unconfirmed change when sending transactions (default: %u)", DEFAULT_SPEND_ZEROCONF_CHANGE), false, OptionsCategory::WALLET);
    gArgs.AddArg("-txconfirmtarget=<n>", strprintf("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)", DEFAULT_TX_CONFIRM_TARGET), false, OptionsCategory::WALLET);
    gArgs.AddArg("-upgradewallet", "Upgrade wallet to latest format on startup", false, OptionsCategory::WALLET);
    gArgs.AddArg("-wallet=<path>", "Specify wallet database path. Can be specified multiple times to load multiple wallets. Path is interpreted relative to <walletdir> if it is not absolute, and will be created if it does not exist (as a directory containing a wallet.dat file and log files). For backwards compatibility this will also accept names of existing data files in <walletdir>.)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletbroadcast",  strprintf("Make the wallet broadcast transactions (default: %u)", DEFAULT_WALLETBROADCAST), false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletdir=<dir>", "Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletnotify=<cmd>", "Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletrbf", strprintf("Send transactions with full-RBF opt-in enabled (RPC only, default: %u)", DEFAULT_WALLET_RBF), false, OptionsCategory::WALLET);
    gArgs.AddArg("-zapwallettxes=<mode>", "Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup"
                               " (1 = keep tx meta data e.g. payment request information, 2 = drop tx meta data)", false, OptionsCategory::WALLET);

    gArgs.AddArg("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DEFAULT_WALLET_DBLOGSIZE), true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET), true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB), true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-walletrejectlongchains", strprintf("Wallet will not create transactions that violate mempool chain limits (default: %u)", DEFAULT_WALLET_REJECT_LONG_CHAINS), true, OptionsCategory::WALLET_DEBUG_TEST);
}

bool WalletInit::ParameterInteraction() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const std::string& wallet : gArgs.GetArgs("-wallet")) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
        }

        return true;
    }

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

    return true;
}

void WalletInit::Construct(InitInterfaces& interfaces) const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return;
    }
    gArgs.SoftSetArg("-wallet", "");
    interfaces.chain_clients.emplace_back(interfaces::MakeWalletClient(*interfaces.chain, gArgs.GetArgs("-wallet")));
}
