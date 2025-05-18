// Copyright (c) 2020-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(script_ops)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CScript script_mut = ConsumeScript(fuzzed_data_provider);
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 1000000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CScript s = ConsumeScript(fuzzed_data_provider);
                script_mut = std::move(s);
            },
            [&] {
                const CScript& s = ConsumeScript(fuzzed_data_provider);
                script_mut = s;
            },
            [&] {
                script_mut << fuzzed_data_provider.ConsumeIntegral<int64_t>();
            },
            [&] {
                script_mut << ConsumeOpcodeType(fuzzed_data_provider);
            },
            [&] {
                script_mut << ConsumeScriptNum(fuzzed_data_provider);
            },
            [&] {
                script_mut << ConsumeRandomLengthByteVector(fuzzed_data_provider);
            },
            [&] {
                script_mut.clear();
            });
    }
    const CScript& script = script_mut;
    (void)script.GetSigOpCount(false);
    (void)script.GetSigOpCount(true);
    (void)script.GetSigOpCount(script);
    (void)script.HasValidOps();
    (void)script.IsPayToScriptHash();
    (void)script.IsPayToWitnessScriptHash();
    (void)script.IsPushOnly();
    (void)script.IsUnspendable();
    {
        CScript::const_iterator pc = script.begin();
        opcodetype opcode;
        (void)script.GetOp(pc, opcode);
        std::vector<uint8_t> data;
        (void)script.GetOp(pc, opcode, data);
        (void)script.IsPushOnly(pc);
    }
    {
        int version;
        std::vector<uint8_t> program;
        (void)script.IsWitnessProgram(version, program);
    }
}
