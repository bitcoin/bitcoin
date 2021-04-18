// Copyright (c) 2019-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <node/psbt.h>
#include <optional.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/script.h>
#include <streams.h>
#include <util/memory.h>
#include <version.h>

#include <cstdint>
#include <string>
#include <vector>

void initialize()
{
    static const ECCVerifyHandle verify_handle;
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    PartiallySignedTransaction psbt_mut;
    const std::string raw_psbt{buffer.begin(), buffer.end()};
    std::string error;
    if (!DecodeRawPSBT(psbt_mut, raw_psbt, error)) {
        return;
    }
    const PartiallySignedTransaction psbt = psbt_mut;

    const PSBTAnalysis analysis = AnalyzePSBT(psbt);
    (void)PSBTRoleName(analysis.next);
    for (const PSBTInputAnalysis& input_analysis : analysis.inputs) {
        (void)PSBTRoleName(input_analysis.next);
    }

    (void)psbt.IsNull();

    Optional<CMutableTransaction> tx = psbt.tx;
    if (tx) {
        const CMutableTransaction& mtx = *tx;
        const PartiallySignedTransaction psbt_from_tx{mtx};
    }

    for (const PSBTInput& input : psbt.inputs) {
        (void)PSBTInputSigned(input);
        (void)input.IsNull();
    }

    for (const PSBTOutput& output : psbt.outputs) {
        (void)output.IsNull();
    }

    for (size_t i = 0; i < psbt.tx->vin.size(); ++i) {
        CTxOut tx_out;
        if (psbt.GetInputUTXO(tx_out, i)) {
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

    psbt_mut = psbt;
    (void)psbt_mut.Merge(psbt);
}
