// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <consensus/tx_verify.h>
#include <node/psbt.h>
#include <policy/policy.h>
#include <policy/settings.h>

#include <numeric>

PSBTAnalysis AnalyzePSBT(PartiallySignedTransaction psbtx)
{
    // Go through each input and build status
    PSBTAnalysis result;

    bool calc_fee = true;
    bool all_final = true;
    bool only_missing_sigs = true;
    bool only_missing_final = false;
    CAmount in_amt = 0;

    result.inputs.resize(psbtx.tx->vin.size());

    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        PSBTInput& input = psbtx.inputs[i];
        PSBTInputAnalysis& input_analysis = result.inputs[i];

        // Check for a UTXO
        CTxOut utxo;
        if (psbtx.GetInputUTXO(utxo, i)) {
            in_amt += utxo.nValue;
            input_analysis.has_utxo = true;
        } else {
            input_analysis.has_utxo = false;
            input_analysis.is_final = false;
            input_analysis.next = PSBTRole::UPDATER;
            calc_fee = false;
        }

        // Check if it is final
        if (!utxo.IsNull() && !PSBTInputSigned(input)) {
            input_analysis.is_final = false;
            all_final = false;

            // Figure out what is missing
            SignatureData outdata;
            bool complete = SignPSBTInput(DUMMY_SIGNING_PROVIDER, psbtx, i, 1, &outdata);

            // Things are missing
            if (!complete) {
                input_analysis.missing_pubkeys = outdata.missing_pubkeys;
                input_analysis.missing_redeem_script = outdata.missing_redeem_script;
                input_analysis.missing_witness_script = outdata.missing_witness_script;
                input_analysis.missing_sigs = outdata.missing_sigs;

                // If we are only missing signatures and nothing else, then next is signer
                if (outdata.missing_pubkeys.empty() && outdata.missing_redeem_script.IsNull() && outdata.missing_witness_script.IsNull() && !outdata.missing_sigs.empty()) {
                    input_analysis.next = PSBTRole::SIGNER;
                } else {
                    only_missing_sigs = false;
                    input_analysis.next = PSBTRole::UPDATER;
                }
            } else {
                only_missing_final = true;
                input_analysis.next = PSBTRole::FINALIZER;
            }
        } else if (!utxo.IsNull()){
            input_analysis.is_final = true;
        }
    }

    if (all_final) {
        only_missing_sigs = false;
        result.next = PSBTRole::EXTRACTOR;
    }
    if (calc_fee) {
        // Get the output amount
        CAmount out_amt = std::accumulate(psbtx.tx->vout.begin(), psbtx.tx->vout.end(), CAmount(0),
            [](CAmount a, const CTxOut& b) {
                return a += b.nValue;
            }
        );

        // Get the fee
        CAmount fee = in_amt - out_amt;
        result.fee = fee;

        // Estimate the size
        CMutableTransaction mtx(*psbtx.tx);
        CCoinsView view_dummy;
        CCoinsViewCache view(&view_dummy);
        bool success = true;

        for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
            PSBTInput& input = psbtx.inputs[i];
            Coin newcoin;

            if (!SignPSBTInput(DUMMY_SIGNING_PROVIDER, psbtx, i, 1, nullptr, true) || !psbtx.GetInputUTXO(newcoin.out, i)) {
                success = false;
                break;
            } else {
                mtx.vin[i].scriptSig = input.final_script_sig;
                mtx.vin[i].scriptWitness = input.final_script_witness;
                newcoin.nHeight = 1;
                view.AddCoin(psbtx.tx->vin[i].prevout, std::move(newcoin), true);
            }
        }

        if (success) {
            CTransaction ctx = CTransaction(mtx);
            size_t size = GetVirtualTransactionSize(ctx, GetTransactionSigOpCost(ctx, view, STANDARD_SCRIPT_VERIFY_FLAGS));
            result.estimated_vsize = size;
            // Estimate fee rate
            CFeeRate feerate(fee, size);
            result.estimated_feerate = feerate;
        }

        if (only_missing_sigs) {
            result.next = PSBTRole::SIGNER;
        } else if (only_missing_final) {
            result.next = PSBTRole::FINALIZER;
        } else if (all_final) {
            result.next = PSBTRole::EXTRACTOR;
        } else {
            result.next = PSBTRole::UPDATER;
        }
    } else {
        result.next = PSBTRole::UPDATER;
    }

    return result;
}
