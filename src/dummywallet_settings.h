#ifndef BITCOIN_DUMMYWALLET_SETTINGS_H
#define BITCOIN_DUMMYWALLET_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using WalletSettingHidden = common::Setting<
    "-wallet=<path>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using AddressTypeSettingHidden = common::Setting<
    "-addresstype", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using AvoidPartialSpendsSettingHidden = common::Setting<
    "-avoidpartialspends", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ChangeTypeSettingHidden = common::Setting<
    "-changetype", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ConsolidateFeeRateSettingHidden = common::Setting<
    "-consolidatefeerate=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using DisableWalletSettingHidden = common::Setting<
    "-disablewallet", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using DiscardFeeSettingHidden = common::Setting<
    "-discardfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using FallbackFeeSettingHidden = common::Setting<
    "-fallbackfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using KeyPoolSettingHidden = common::Setting<
    "-keypool=<n>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MaxApsFeeSettingHidden = common::Setting<
    "-maxapsfee=<n>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MaxTxFeeSettingHidden = common::Setting<
    "-maxtxfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MinTxFeeSettingHidden = common::Setting<
    "-mintxfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using PayTxFeeSettingHidden = common::Setting<
    "-paytxfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using SignerSettingHidden = common::Setting<
    "-signer=<cmd>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using SpendZeroConfChangeSettingHidden = common::Setting<
    "-spendzeroconfchange", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using TxConfirmTargetSettingHidden = common::Setting<
    "-txconfirmtarget=<n>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletBroadcastSettingHidden = common::Setting<
    "-walletbroadcast", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletDirSettingHidden = common::Setting<
    "-walletdir=<dir>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletNotifySettingHidden = common::Setting<
    "-walletnotify=<cmd>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletRbfSettingHidden = common::Setting<
    "-walletrbf", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletRejectLongChainsSettingHidden = common::Setting<
    "-walletrejectlongchains", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletCrossChainSettingHidden = common::Setting<
    "-walletcrosschain", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using UnsafeSqliteSyncSettingHidden = common::Setting<
    "-unsafesqlitesync", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

#endif // BITCOIN_DUMMYWALLET_SETTINGS_H
