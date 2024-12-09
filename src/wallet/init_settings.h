#ifndef BITCOIN_WALLET_INIT_SETTINGS_H
#define BITCOIN_WALLET_INIT_SETTINGS_H

#include <common/setting.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <util/moneystr.h>
#include <wallet/coincontrol.h>
#include <wallet/db.h>
#include <wallet/wallet.h>

#include <string>
#include <vector>

namespace wallet {

using WalletSetting = common::Setting<
    "-wallet=<path>", std::vector<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Specify wallet path to load at startup. Can be used multiple times to load multiple wallets. Path is to a directory containing wallet data and log files. If the path is not absolute, it is interpreted relative to <walletdir>. This only loads existing wallets and does not create new ones. For backwards compatibility this also accepts names of existing top-level data files in <walletdir>.">
    ::Category<OptionsCategory::WALLET>;

using WalletSettingStr = common::Setting<
    "-wallet=<path>", std::string, common::SettingOptions{.legacy = true, .network_only = true}>;

using AddresstypeSetting = common::Setting<
    "-addresstype", std::string, common::SettingOptions{.legacy = true},
    "What type of addresses to use (\"legacy\", \"p2sh-segwit\", \"bech32\", or \"bech32m\", default: \"%s\")">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, FormatOutputType(DEFAULT_ADDRESS_TYPE)); }>
    ::Category<OptionsCategory::WALLET>;

using AvoidpartialspendsSetting = common::Setting<
    "-avoidpartialspends", bool, common::SettingOptions{.legacy = true},
    "Group outputs by address, selecting many (possibly all) or none, instead of selecting on a per-output basis. Privacy is improved as addresses are mostly swept with fewer transactions and outputs are aggregated in clean change addresses. It may result in higher fees due to less optimal coin selection caused by this added limitation and possibly a larger-than-necessary number of inputs being used. Always enabled for wallets with \"avoid_reuse\" enabled, otherwise default: %u.">
    ::DefaultFn<[] { return DEFAULT_AVOIDPARTIALSPENDS; }>
    ::Category<OptionsCategory::WALLET>;

using ChangetypeSetting = common::Setting<
    "-changetype", std::string, common::SettingOptions{.legacy = true},
    "What type of change to use (\"legacy\", \"p2sh-segwit\", \"bech32\", or \"bech32m\"). Default is \"legacy\" when "
                   "-addresstype=legacy, else it is an implementation detail.">
    ::Category<OptionsCategory::WALLET>;

using ConsolidatefeerateSetting = common::Setting<
    "-consolidatefeerate=<amt>", std::string, common::SettingOptions{.legacy = true},
    "The maximum feerate (in %s/kvB) at which transaction building may use more inputs than strictly necessary so that the wallet's UTXO pool can be reduced (default: %s).">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_CONSOLIDATE_FEERATE)); }>
    ::Category<OptionsCategory::WALLET>;

using DisablewalletSetting = common::Setting<
    "-disablewallet", bool, common::SettingOptions{.legacy = true},
    "Do not load the wallet and disable wallet RPC calls">
    ::Default<DEFAULT_DISABLE_WALLET>
    ::HelpArgs<>
    ::Category<OptionsCategory::WALLET>;

using DiscardfeeSetting = common::Setting<
    "-discardfee=<amt>", std::string, common::SettingOptions{.legacy = true},
    "The fee rate (in %s/kvB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)); }>
    ::Category<OptionsCategory::WALLET>;

using FallbackfeeSetting = common::Setting<
    "-fallbackfee=<amt>", std::string, common::SettingOptions{.legacy = true},
    "A fee rate (in %s/kvB) that will be used when fee estimation has insufficient data. 0 to entirely disable the fallbackfee feature. (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)); }>
    ::Category<OptionsCategory::WALLET>;

using KeypoolSetting = common::Setting<
    "-keypool=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Set key pool size to <n> (default: %u). Warning: Smaller sizes may increase the risk of losing funds when restoring from an old backup, if none of the addresses in the original keypool have been used.">
    ::Default<DEFAULT_KEYPOOL_SIZE>
    ::Category<OptionsCategory::WALLET>;

using MaxapsfeeSetting = common::Setting<
    "-maxapsfee=<n>", std::string, common::SettingOptions{.legacy = true},
    "Spend up to this amount in additional (absolute) fees (in %s) if it allows the use of partial spend avoidance (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_MAX_AVOIDPARTIALSPEND_FEE)); }>
    ::Category<OptionsCategory::WALLET>;

using MaxtxfeeSetting = common::Setting<
    "-maxtxfee=<amt>", std::string, common::SettingOptions{.legacy = true},
    "Maximum total fees (in %s) to use in a single wallet transaction; setting this too low may abort large transactions (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using MintxfeeSetting = common::Setting<
    "-mintxfee=<amt>", std::string, common::SettingOptions{.legacy = true},
    "Fee rates (in %s/kvB) smaller than this are considered zero fee for transaction creation (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)); }>
    ::Category<OptionsCategory::WALLET>;

using PaytxfeeSetting = common::Setting<
    "-paytxfee=<amt>", std::string, common::SettingOptions{.legacy = true},
    "Fee rate (in %s/kvB) to add to transactions you send (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, CURRENCY_UNIT, FormatMoney(CFeeRate{DEFAULT_PAY_TX_FEE}.GetFeePerK())); }>
    ::Category<OptionsCategory::WALLET>;

using SignerSetting = common::Setting<
    "-signer=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "External signing tool, see doc/external-signer.md">
    ::Category<OptionsCategory::WALLET>;

using SpendzeroconfchangeSetting = common::Setting<
    "-spendzeroconfchange", bool, common::SettingOptions{.legacy = true},
    "Spend unconfirmed change when sending transactions (default: %u)">
    ::Default<DEFAULT_SPEND_ZEROCONF_CHANGE>
    ::Category<OptionsCategory::WALLET>;

using TxconfirmtargetSetting = common::Setting<
    "-txconfirmtarget=<n>", int64_t, common::SettingOptions{.legacy = true},
    "If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)">
    ::Default<DEFAULT_TX_CONFIRM_TARGET>
    ::Category<OptionsCategory::WALLET>;

using WalletbroadcastSetting = common::Setting<
    "-walletbroadcast", bool, common::SettingOptions{.legacy = true},
    "Make the wallet broadcast transactions (default: %u)">
    ::Default<DEFAULT_WALLETBROADCAST>
    ::Category<OptionsCategory::WALLET>;

using WalletdirSetting = common::Setting<
    "-walletdir=<dir>", fs::path, common::SettingOptions{.legacy = true, .network_only = true},
    "Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)">
    ::Category<OptionsCategory::WALLET>;

using WalletnotifySetting = common::Setting<
    "-walletnotify=<cmd>", std::string, common::SettingOptions{.legacy = true},
    "Execute command when a wallet transaction changes. %s in cmd is replaced by TxID, %w is replaced by wallet name, %b is replaced by the hash of the block including the transaction (set to 'unconfirmed' if the transaction is not included) and %h is replaced by the block height (-1 if not included). %w is not currently implemented on windows. On systems where %w is supported, it should NOT be quoted because this would break shell escaping used to invoke the command.">
    ::Category<OptionsCategory::WALLET>;

using WalletrbfSetting = common::Setting<
    "-walletrbf", bool, common::SettingOptions{.legacy = true},
    "Send transactions with full-RBF opt-in enabled (RPC only, default: %u)">
    ::Default<DEFAULT_WALLET_RBF>
    ::Category<OptionsCategory::WALLET>;

using DblogsizeSetting = common::Setting<
    "-dblogsize=<n>", int64_t, common::SettingOptions{.legacy = true, .debug_only = true},
    "Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, DatabaseOptions().max_log_mb); }>
    ::Category<OptionsCategory::WALLET_DEBUG_TEST>;

using FlushwalletSetting = common::Setting<
    "-flushwallet", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Run a thread to flush wallet periodically (default: %u)">
    ::Default<DEFAULT_FLUSHWALLET>
    ::Category<OptionsCategory::WALLET_DEBUG_TEST>;

using PrivdbSetting = common::Setting<
    "-privdb", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Sets the DB_PRIVATE flag in the wallet db environment (default: %u)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, !DatabaseOptions().use_shared_memory); }>
    ::Category<OptionsCategory::WALLET_DEBUG_TEST>;

using WalletrejectlongchainsSetting = common::Setting<
    "-walletrejectlongchains", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Wallet will not create transactions that violate mempool chain limits (default: %u)">
    ::Default<DEFAULT_WALLET_REJECT_LONG_CHAINS>
    ::Category<OptionsCategory::WALLET_DEBUG_TEST>;

using WalletcrosschainSetting = common::Setting<
    "-walletcrosschain", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Allow reusing wallet files across chains (default: %u)">
    ::Default<DEFAULT_WALLETCROSSCHAIN>
    ::Category<OptionsCategory::WALLET_DEBUG_TEST>;

using UnsafesqlitesyncSetting = common::Setting<
    "-unsafesqlitesync", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Set SQLite synchronous=OFF to disable waiting for the database to sync to disk. This is unsafe and can cause data loss and corruption. This option is only used by tests to improve their performance (default: false)">
    ::Category<OptionsCategory::WALLET_DEBUG_TEST>;

using SwapbdbendianSetting = common::Setting<
    "-swapbdbendian", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Swaps the internal endianness of BDB wallet databases (default: false)">
    ::Category<OptionsCategory::WALLET_DEBUG_TEST>;
} // namespace wallet

#endif // BITCOIN_WALLET_INIT_SETTINGS_H
