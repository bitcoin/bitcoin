#ifndef BITCOIN_DUMMYWALLET_SETTINGS_H
#define BITCOIN_DUMMYWALLET_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using WalletSettingHidden = common::Setting<
    "-wallet=<path>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using AddresstypeSettingHidden = common::Setting<
    "-addresstype", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using AvoidpartialspendsSettingHidden = common::Setting<
    "-avoidpartialspends", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ChangetypeSettingHidden = common::Setting<
    "-changetype", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using ConsolidatefeerateSettingHidden = common::Setting<
    "-consolidatefeerate=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using DisablewalletSettingHidden = common::Setting<
    "-disablewallet", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using DiscardfeeSettingHidden = common::Setting<
    "-discardfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using FallbackfeeSettingHidden = common::Setting<
    "-fallbackfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using KeypoolSettingHidden = common::Setting<
    "-keypool=<n>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MaxapsfeeSettingHidden = common::Setting<
    "-maxapsfee=<n>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MaxtxfeeSettingHidden = common::Setting<
    "-maxtxfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using MintxfeeSettingHidden = common::Setting<
    "-mintxfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using PaytxfeeSettingHidden = common::Setting<
    "-paytxfee=<amt>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using SignerSettingHidden = common::Setting<
    "-signer=<cmd>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using SpendzeroconfchangeSettingHidden = common::Setting<
    "-spendzeroconfchange", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using TxconfirmtargetSettingHidden = common::Setting<
    "-txconfirmtarget=<n>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletbroadcastSettingHidden = common::Setting<
    "-walletbroadcast", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletdirSettingHidden = common::Setting<
    "-walletdir=<dir>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletnotifySettingHidden = common::Setting<
    "-walletnotify=<cmd>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletrbfSettingHidden = common::Setting<
    "-walletrbf", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using DblogsizeSettingHidden = common::Setting<
    "-dblogsize=<n>", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using FlushwalletSettingHidden = common::Setting<
    "-flushwallet", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using PrivdbSettingHidden = common::Setting<
    "-privdb", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletrejectlongchainsSettingHidden = common::Setting<
    "-walletrejectlongchains", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using WalletcrosschainSettingHidden = common::Setting<
    "-walletcrosschain", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using UnsafesqlitesyncSettingHidden = common::Setting<
    "-unsafesqlitesync", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

using SwapbdbendianSettingHidden = common::Setting<
    "-swapbdbendian", common::Unset, common::SettingOptions{.legacy = true}>
    ::Category<OptionsCategory::HIDDEN>;

#endif // BITCOIN_DUMMYWALLET_SETTINGS_H
