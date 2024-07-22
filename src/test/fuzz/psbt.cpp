// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/psbt.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/script.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/random.h>
#include <util/check.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using node::AnalyzePSBT;
using node::PSBTAnalysis;
using node::PSBTInputAnalysis;

FUZZ_TARGET(psbt)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    auto str = fuzzed_data_provider.ConsumeRandomLengthString();
    util::Result<PartiallySignedTransaction> psbt_res = DecodeRawPSBT(MakeByteSpan(str));
    if (!psbt_res) {
        return;
    }
    PartiallySignedTransaction psbt_mut = *psbt_res;
    const PartiallySignedTransaction psbt = psbt_mut;

    // A PSBT must roundtrip.
    std::vector<uint8_t> psbt_ser;
    VectorWriter{psbt_ser, 0, psbt};
    SpanReader reader{psbt_ser};
    PartiallySignedTransaction psbt_roundtrip(deserialize, reader);

    // And be stable across roundtrips.
    std::vector<uint8_t> roundtrip_ser;
    VectorWriter{roundtrip_ser, 0, psbt_roundtrip};
    Assert(psbt_ser == roundtrip_ser);

    const PSBTAnalysis analysis = AnalyzePSBT(psbt);
    (void)PSBTRoleName(analysis.next);
    for (const PSBTInputAnalysis& input_analysis : analysis.inputs) {
        (void)PSBTRoleName(input_analysis.next);
    }

    (void)psbt.IsNull();

    (void)psbt.GetUnsignedTx();

    for (const PSBTInput& input : psbt.inputs) {
        (void)PSBTInputSigned(input);
        (void)input.IsNull();
    }
    (void)CountPSBTUnsignedInputs(psbt);

    for (const PSBTOutput& output : psbt.outputs) {
        (void)output.IsNull();
    }

    for (const PSBTInput& input : psbt.inputs) {
        CTxOut tx_out;
        if (input.GetUTXO(tx_out)) {
            (void)tx_out.IsNull();
            (void)tx_out.ToString();
        }
    }

    psbt_mut = psbt;
    (void)FinalizePSBT(psbt_mut);

    psbt_mut = psbt;
    CMutableTransaction result;
    if (FinalizeAndExtractPSBT(psbt_mut, result)) {
        const PartiallySignedTransaction psbt_from_tx{result};
    }

    PartiallySignedTransaction psbt_merge = psbt;
    str = fuzzed_data_provider.ConsumeRandomLengthString();
    util::Result<PartiallySignedTransaction> psbt_merge_res = DecodeRawPSBT(MakeByteSpan(str));
    if (psbt_merge_res) {
        psbt_merge = *psbt_merge_res;
    }
    psbt_mut = psbt;
    (void)psbt_mut.Merge(psbt_merge);
    psbt_mut = psbt;
    std::optional<PartiallySignedTransaction> comb_res = CombinePSBTs({psbt_mut, psbt_merge});
    if (comb_res) {
        psbt_mut = *comb_res;
    }
    for (const auto& psbt_in : psbt_merge.inputs) {
        (void)psbt_mut.AddInput(psbt_in);
    }
    for (const auto& psbt_out : psbt_merge.outputs) {
        Assert(psbt_mut.AddOutput(psbt_out));
    }
    psbt_mut.unknown.insert(psbt_merge.unknown.begin(), psbt_merge.unknown.end());
}
