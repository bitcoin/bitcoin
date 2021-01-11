// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <node/psbt.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/script.h>
#include <streams.h>
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
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    PartiallySignedTransaction psbt_mut;
    std::string error;
    auto str = fuzzed_data_provider.ConsumeRandomLengthString();
    if (!DecodeRawPSBT(psbt_mut, MakeByteSpan(str), error)) {
        return;
    }
    const PartiallySignedTransaction psbt = psbt_mut;

    const PSBTAnalysis analysis = AnalyzePSBT(psbt);
    (void)PSBTRoleName(analysis.next);
    for (const PSBTInputAnalysis& input_analysis : analysis.inputs) {
        (void)PSBTRoleName(input_analysis.next);
    }

    (void)psbt.IsNull();

    std::optional<CMutableTransaction> tx = psbt.tx;
    if (tx) {
        const CMutableTransaction& mtx = *tx;
        const PartiallySignedTransaction psbt_from_tx{mtx};
    }

    for (const PSBTInput& input : psbt.inputs) {
        (void)PSBTInputSigned(input);
        (void)input.IsNull();
    }
    (void)CountPSBTUnsignedInputs(psbt);

    for (const PSBTOutput& output : psbt.outputs) {
        (void)output.IsNull();
    }

    for (size_t i = 0; i < psbt.tx->vin.size(); ++i) {
        CTxOut tx_out;
        if (psbt.inputs.at(i).GetUTXO(tx_out)) {
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

    PartiallySignedTransaction psbt_merge;
    str = fuzzed_data_provider.ConsumeRandomLengthString();
    if (!DecodeRawPSBT(psbt_merge, MakeByteSpan(str), error)) {
        psbt_merge = psbt;
    }
    psbt_mut = psbt;
    (void)psbt_mut.Merge(psbt_merge);
    psbt_mut = psbt;
    (void)CombinePSBTs(psbt_mut, {psbt_mut, psbt_merge});
    psbt_mut = psbt;
    for (auto& psbt_in : psbt_merge.inputs) {
        (void)psbt_mut.AddInput(psbt_in);
    }
    for (unsigned int i = 0; i < psbt_merge.tx->vout.size(); ++i) {
        Assert(psbt_mut.AddOutput(psbt_merge.tx->vout[i], psbt_merge.outputs[i]));
    }
    psbt_mut.unknown.insert(psbt_merge.unknown.begin(), psbt_merge.unknown.end());
}
