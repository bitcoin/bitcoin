// Copyright (c) 2020 The Bitcoin Core developers
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
    CScript script = ConsumeScript(fuzzed_data_provider);
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CScript s = ConsumeScript(fuzzed_data_provider);
                script = std::move(s);
            },
            [&] {
                const CScript& s = ConsumeScript(fuzzed_data_provider);
                script = s;
            },
            [&] {
                script << fuzzed_data_provider.ConsumeIntegral<int64_t>();
            },
            [&] {
                script << ConsumeOpcodeType(fuzzed_data_provider);
            },
            [&] {
                script << ConsumeScriptNum(fuzzed_data_provider);
            },
            [&] {
                script << ConsumeRandomLengthByteVector(fuzzed_data_provider);
            },
            [&] {
                script.clear();
            },
            [&] {
                (void)script.GetSigOpCount(false);
                (void)script.GetSigOpCount(true);
                (void)script.GetSigOpCount(script);
                (void)script.IsPayToScriptHash();
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
            });
    }
}
