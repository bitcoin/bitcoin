#ifndef BITCOIN_BITCOIN_TX_SETTINGS_H
#define BITCOIN_BITCOIN_TX_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using VersionSetting = common::Setting<
    "-version", bool, common::SettingOptions{.legacy = true},
    "Print version and exit">;

using CreateSetting = common::Setting<
    "-create", bool, common::SettingOptions{.legacy = true},
    "Create new, empty TX.">;

using JsonSetting = common::Setting<
    "-json", bool, common::SettingOptions{.legacy = true},
    "Select JSON output">;

using TxidSetting = common::Setting<
    "-txid", bool, common::SettingOptions{.legacy = true},
    "Output only the hex-encoded transaction id of the resultant transaction.">;

using DelinSetting = common::Setting<
    "delin=N", common::Unset, common::SettingOptions{.legacy = true},
    "Delete input N from TX">
    ::Category<OptionsCategory::COMMANDS>;

using DeloutSetting = common::Setting<
    "delout=N", common::Unset, common::SettingOptions{.legacy = true},
    "Delete output N from TX">
    ::Category<OptionsCategory::COMMANDS>;

using InSetting = common::Setting<
    "in=TXID:VOUT(:SEQUENCE_NUMBER)", common::Unset, common::SettingOptions{.legacy = true},
    "Add input to TX">
    ::Category<OptionsCategory::COMMANDS>;

using LocktimeSetting = common::Setting<
    "locktime=N", common::Unset, common::SettingOptions{.legacy = true},
    "Set TX lock time to N">
    ::Category<OptionsCategory::COMMANDS>;

using NversionSetting = common::Setting<
    "nversion=N", common::Unset, common::SettingOptions{.legacy = true},
    "Set TX version to N">
    ::Category<OptionsCategory::COMMANDS>;

using OutaddrSetting = common::Setting<
    "outaddr=VALUE:ADDRESS", common::Unset, common::SettingOptions{.legacy = true},
    "Add address-based output to TX">
    ::Category<OptionsCategory::COMMANDS>;

using OutdataSetting = common::Setting<
    "outdata=[VALUE:]DATA", common::Unset, common::SettingOptions{.legacy = true},
    "Add data-based output to TX">
    ::Category<OptionsCategory::COMMANDS>;

using OutmultisigSetting = common::Setting<
    "outmultisig=VALUE:REQUIRED:PUBKEYS:PUBKEY1:PUBKEY2:....[:FLAGS]", common::Unset, common::SettingOptions{.legacy = true},
    "Add Pay To n-of-m Multi-sig output to TX. n = REQUIRED, m = PUBKEYS. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.">
    ::Category<OptionsCategory::COMMANDS>;

using OutpubkeySetting = common::Setting<
    "outpubkey=VALUE:PUBKEY[:FLAGS]", common::Unset, common::SettingOptions{.legacy = true},
    "Add pay-to-pubkey output to TX. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-pubkey-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.">
    ::Category<OptionsCategory::COMMANDS>;

using OutscriptSetting = common::Setting<
    "outscript=VALUE:SCRIPT[:FLAGS]", common::Unset, common::SettingOptions{.legacy = true},
    "Add raw script output to TX. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.">
    ::Category<OptionsCategory::COMMANDS>;

using ReplaceableSetting = common::Setting<
    "replaceable(=N)", common::Unset, common::SettingOptions{.legacy = true},
    "Sets Replace-By-Fee (RBF) opt-in sequence number for input N. "
        "If N is not provided, the command attempts to opt-in all available inputs for RBF. "
        "If the transaction has no inputs, this option is ignored.">
    ::Category<OptionsCategory::COMMANDS>;

using SignSetting = common::Setting<
    "sign=SIGHASH-FLAGS", common::Unset, common::SettingOptions{.legacy = true},
    "Add zero or more signatures to transaction. "
        "This command requires JSON registers:"
        "prevtxs=JSON object, "
        "privatekeys=JSON object. "
        "See signrawtransactionwithkey docs for format of sighash flags, JSON objects.">
    ::Category<OptionsCategory::COMMANDS>;

using LoadSetting = common::Setting<
    "load=NAME:FILENAME", common::Unset, common::SettingOptions{.legacy = true},
    "Load JSON file FILENAME into register NAME">
    ::Category<OptionsCategory::REGISTER_COMMANDS>;

using SetSetting = common::Setting<
    "set=NAME:JSON-STRING", common::Unset, common::SettingOptions{.legacy = true},
    "Set register NAME to given JSON-STRING">
    ::Category<OptionsCategory::REGISTER_COMMANDS>;

#endif // BITCOIN_BITCOIN_TX_SETTINGS_H
