// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <core_io.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <univalue.h>

void initialize_script_format()
{
    SelectParams(CBaseChainParams::REGTEST);
}

FUZZ_TARGET_INIT(script_format, initialize_script_format)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CScript script{ConsumeScript(fuzzed_data_provider)};

    (void)FormatScript(script);
    (void)ScriptToAsmStr(script, /*fAttemptSighashDecode=*/fuzzed_data_provider.ConsumeBool());

    UniValue o1(UniValue::VOBJ);
    ScriptPubKeyToUniv(script, o1, /*include_hex=*/fuzzed_data_provider.ConsumeBool());
    UniValue o3(UniValue::VOBJ);
    ScriptToUniv(script, o3);
}
