#ifndef BITCOIN_TEST_ARGSMAN_TESTS_SETTINGS_H
#define BITCOIN_TEST_ARGSMAN_TESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using RegTestSetting = common::Setting<
    "-regtest", common::Unset, common::SettingOptions{.legacy = true},
    "regtest">;

using TestNet4Setting = common::Setting<
    "-testnet4", common::Unset, common::SettingOptions{.legacy = true},
    "testnet4">;

using HSetting = common::Setting<
    "-h", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using HSettingStr = common::Setting<
    "-h", std::string, common::SettingOptions{.legacy = true}>;

using HSettingBool = common::Setting<
    "-h", bool, common::SettingOptions{.legacy = true}>;

using ValueSetting = common::Setting<
    "-value", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using ValueSettingStr = common::Setting<
    "-value", std::string, common::SettingOptions{.legacy = true}>;

using ValueSettingInt = common::Setting<
    "-value", int64_t, common::SettingOptions{.legacy = true}>;

using ValueSettingBool = common::Setting<
    "-value", bool, common::SettingOptions{.legacy = true}>;

using ASetting = common::Setting<
    "-a", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using ASettingStr = common::Setting<
    "-a", std::string, common::SettingOptions{.legacy = true}>;

using ASettingBool = common::Setting<
    "-a", bool, common::SettingOptions{.legacy = true}>;

using BSetting = common::Setting<
    "-b", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using BSettingStr = common::Setting<
    "-b", std::string, common::SettingOptions{.legacy = true}>;

using BSettingBool = common::Setting<
    "-b", bool, common::SettingOptions{.legacy = true}>;

using CccSetting = common::Setting<
    "-ccc", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using CccSettingStr = common::Setting<
    "-ccc", std::string, common::SettingOptions{.legacy = true}>;

using CccSettingBool = common::Setting<
    "-ccc", bool, common::SettingOptions{.legacy = true}>;

using FSetting = common::Setting<
    "f", bool, common::SettingOptions{.legacy = true}>
    ::Default<true>
    ::HelpArgs<>;

using FSetting2 = common::Setting<
    "-f", bool, common::SettingOptions{.legacy = true}>
    ::Default<true>
    ::HelpArgs<>;

using DSetting = common::Setting<
    "-d", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using DSettingStr = common::Setting<
    "-d", std::string, common::SettingOptions{.legacy = true}>;

using DSettingBool = common::Setting<
    "-d", bool, common::SettingOptions{.legacy = true}>;

using NobSetting = common::Setting<
    "-nob", common::Unset, common::SettingOptions{.legacy = true}>;

using CSetting = common::Setting<
    "-c", bool, common::SettingOptions{.legacy = true}>
    ::Default<true>
    ::HelpArgs<>;

using ESetting = common::Setting<
    "-e", bool, common::SettingOptions{.legacy = true}>
    ::Default<true>
    ::HelpArgs<>;

using FooSetting = common::Setting<
    "-foo", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using FooSettingStr = common::Setting<
    "-foo", std::string, common::SettingOptions{.legacy = true}>;

using BarSetting = common::Setting<
    "-bar", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using BarSettingStr = common::Setting<
    "-bar", std::string, common::SettingOptions{.legacy = true}>;

using FffSetting = common::Setting<
    "-fff", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using FffSettingStr = common::Setting<
    "-fff", std::string, common::SettingOptions{.legacy = true}>;

using FffSettingBool = common::Setting<
    "-fff", bool, common::SettingOptions{.legacy = true}>;

using GggSetting = common::Setting<
    "-ggg", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using GggSettingStr = common::Setting<
    "-ggg", std::string, common::SettingOptions{.legacy = true}>;

using GggSettingBool = common::Setting<
    "-ggg", bool, common::SettingOptions{.legacy = true}>;

using ISetting = common::Setting<
    "-i", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using ISettingStr = common::Setting<
    "-i", std::string, common::SettingOptions{.legacy = true}>;

using ISettingBool = common::Setting<
    "-i", bool, common::SettingOptions{.legacy = true}>;

using ZzzSetting = common::Setting<
    "-zzz", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using ZzzSettingStr = common::Setting<
    "-zzz", std::string, common::SettingOptions{.legacy = true}>;

using ZzzSettingBool = common::Setting<
    "-zzz", bool, common::SettingOptions{.legacy = true}>;

using IiiSetting = common::Setting<
    "-iii", std::string, common::SettingOptions{.legacy = true}>;

using IiiSettingBool = common::Setting<
    "-iii", bool, common::SettingOptions{.legacy = true}>;

using NofffSetting = common::Setting<
    "-nofff", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using NogggSetting = common::Setting<
    "-noggg", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using NohSetting = common::Setting<
    "-noh", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using NoiSetting = common::Setting<
    "-noi", std::vector<std::string>, common::SettingOptions{.legacy = true}>;

using StrTest1Setting = common::Setting<
    "strtest1", std::string, common::SettingOptions{.legacy = true}>;

using StrTest2Setting = common::Setting<
    "strtest2", std::string, common::SettingOptions{.legacy = true}>;

using IntTest1Setting = common::Setting<
    "inttest1", int64_t, common::SettingOptions{.legacy = true}>
    ::Default<-1>
    ::HelpArgs<>;

using IntTest2Setting = common::Setting<
    "inttest2", int64_t, common::SettingOptions{.legacy = true}>
    ::Default<-1>
    ::HelpArgs<>;

using IntTest3Setting = common::Setting<
    "inttest3", int64_t, common::SettingOptions{.legacy = true}>
    ::Default<-1>
    ::HelpArgs<>;

using BoolTest1Setting = common::Setting<
    "booltest1", bool, common::SettingOptions{.legacy = true}>;

using BoolTest2Setting = common::Setting<
    "booltest2", bool, common::SettingOptions{.legacy = true}>;

using BoolTest3Setting = common::Setting<
    "booltest3", bool, common::SettingOptions{.legacy = true}>;

using BoolTest4Setting = common::Setting<
    "booltest4", bool, common::SettingOptions{.legacy = true}>;

using PriTest1Setting = common::Setting<
    "pritest1", std::string, common::SettingOptions{.legacy = true}>;

using PriTest2Setting = common::Setting<
    "pritest2", std::string, common::SettingOptions{.legacy = true}>;

using PriTest3Setting = common::Setting<
    "pritest3", std::string, common::SettingOptions{.legacy = true}>;

using PriTest4Setting = common::Setting<
    "pritest4", std::string, common::SettingOptions{.legacy = true}>;

#endif // BITCOIN_TEST_ARGSMAN_TESTS_SETTINGS_H
