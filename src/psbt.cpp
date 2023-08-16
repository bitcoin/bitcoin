// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <consensus/tx_verify.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <psbt.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <numeric>

PartiallySignedTransaction::PartiallySignedTransaction(const CMutableTransaction& tx) : tx(tx)
{
    inputs.resize(tx.vin.size());
    outputs.resize(tx.vout.size());
}

bool PartiallySignedTransaction::IsNull() const
{
    return !tx && inputs.empty() && outputs.empty() && unknown.empty();
}

bool PartiallySignedTransaction::Merge(const PartiallySignedTransaction& psbt)
{
    // Prohibited to merge two PSBTs over different transactions
    if (tx->GetHash() != psbt.tx->GetHash()) {
        return false;
    }

    for (unsigned int i = 0; i < inputs.size(); ++i) {
        inputs[i].Merge(psbt.inputs[i]);
    }
    for (unsigned int i = 0; i < outputs.size(); ++i) {
        outputs[i].Merge(psbt.outputs[i]);
    }
    unknown.insert(psbt.unknown.begin(), psbt.unknown.end());

    return true;
}

bool PartiallySignedTransaction::IsSane() const
{
    for (PSBTInput input : inputs) {
        if (!input.IsSane()) return false;
    }
    return true;
}

bool PartiallySignedTransaction::AddInput(const CTxIn& txin, PSBTInput& psbtin)
{
    if (std::find(tx->vin.begin(), tx->vin.end(), txin) != tx->vin.end()) {
        return false;
    }
    tx->vin.push_back(txin);
    psbtin.partial_sigs.clear();
    psbtin.final_script_sig.clear();
    inputs.push_back(psbtin);
    return true;
}

bool PartiallySignedTransaction::AddOutput(const CTxOut& txout, const PSBTOutput& psbtout)
{
    tx->vout.push_back(txout);
    outputs.push_back(psbtout);
    return true;
}

bool PartiallySignedTransaction::GetInputUTXO(CTxOut& utxo, int input_index) const
{
    PSBTInput input = inputs[input_index];
    uint32_t prevout_index = tx->vin[input_index].prevout.n;
    if (input.non_witness_utxo) {
        if (prevout_index >= input.non_witness_utxo->vout.size()) {
            return false;
        }
        utxo = input.non_witness_utxo->vout[prevout_index];
    } else {
        return false;
    }
    return true;
}

bool PSBTInput::IsNull() const
{
    return !non_witness_utxo && partial_sigs.empty() && unknown.empty() && hd_keypaths.empty() && redeem_script.empty();
}

void PSBTInput::FillSignatureData(SignatureData& sigdata) const
{
    if (!final_script_sig.empty()) {
        sigdata.scriptSig = final_script_sig;
        sigdata.complete = true;
    }
    if (sigdata.complete) {
        return;
    }

    sigdata.signatures.insert(partial_sigs.begin(), partial_sigs.end());
    if (!redeem_script.empty()) {
        sigdata.redeem_script = redeem_script;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair);
    }
}

void PSBTInput::FromSignatureData(const SignatureData& sigdata)
{
    if (sigdata.complete) {
        partial_sigs.clear();
        hd_keypaths.clear();
        redeem_script.clear();

        if (!sigdata.scriptSig.empty()) {
            final_script_sig = sigdata.scriptSig;
        }
        return;
    }

    partial_sigs.insert(sigdata.signatures.begin(), sigdata.signatures.end());
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    for (const auto& entry : sigdata.misc_pubkeys) {
        hd_keypaths.emplace(entry.second);
    }
}

void PSBTInput::Merge(const PSBTInput& input)
{
    if (!non_witness_utxo && input.non_witness_utxo) non_witness_utxo = input.non_witness_utxo;

    partial_sigs.insert(input.partial_sigs.begin(), input.partial_sigs.end());
    hd_keypaths.insert(input.hd_keypaths.begin(), input.hd_keypaths.end());
    unknown.insert(input.unknown.begin(), input.unknown.end());

    if (redeem_script.empty() && !input.redeem_script.empty()) redeem_script = input.redeem_script;
    if (final_script_sig.empty() && !input.final_script_sig.empty()) final_script_sig = input.final_script_sig;
}

bool PSBTInput::IsSane() const
{
    return true;
}

void PSBTOutput::FillSignatureData(SignatureData& sigdata) const
{
    if (!redeem_script.empty()) {
        sigdata.redeem_script = redeem_script;
    }
    for (const auto& key_pair : hd_keypaths) {
        sigdata.misc_pubkeys.emplace(key_pair.first.GetID(), key_pair);
    }
}

void PSBTOutput::FromSignatureData(const SignatureData& sigdata)
{
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    for (const auto& entry : sigdata.misc_pubkeys) {
        hd_keypaths.emplace(entry.second);
    }
}

bool PSBTOutput::IsNull() const
{
    return redeem_script.empty() && hd_keypaths.empty() && unknown.empty();
}

void PSBTOutput::Merge(const PSBTOutput& output)
{
    hd_keypaths.insert(output.hd_keypaths.begin(), output.hd_keypaths.end());
    unknown.insert(output.unknown.begin(), output.unknown.end());

    if (redeem_script.empty() && !output.redeem_script.empty()) redeem_script = output.redeem_script;
}

void UpdatePSBTOutput(const SigningProvider& provider, PartiallySignedTransaction& psbt, int index)
{
    CMutableTransaction& tx = *Assert(psbt.tx);
    const CTxOut& out = tx.vout.at(index);
    PSBTOutput& psbt_out = psbt.outputs.at(index);

    // Fill a SignatureData with output info
    SignatureData sigdata;
    psbt_out.FillSignatureData(sigdata);

    // Construct a would-be spend of this output, to update sigdata with.
    // Note that ProduceSignature is used to fill in metadata (not actual signatures),
    // so provider does not need to provide any private keys (it can be a HidingSigningProvider).
    MutableTransactionSignatureCreator creator(&tx, /* index */ 0, out.nValue, SIGHASH_ALL);
    ProduceSignature(provider, creator, out.scriptPubKey, sigdata);

    // Put redeem_script, key paths, into PSBTOutput.
    psbt_out.FromSignatureData(sigdata);
}
bool PSBTInputSigned(const PSBTInput& input)
{
    return !input.final_script_sig.empty();
}

bool SignPSBTInput(const SigningProvider& provider, PartiallySignedTransaction& psbt, int index, int sighash, SignatureData* out_sigdata, bool use_dummy)
{
    PSBTInput& input = psbt.inputs.at(index);
    const CMutableTransaction& tx = *psbt.tx;

    if (PSBTInputSigned(input)) {
        return true;
    }

    // Fill SignatureData with input info
    SignatureData sigdata;
    input.FillSignatureData(sigdata);

    // Get UTXO
    CTxOut utxo;

    // Verify input sanity, which checks that at most one of witness or non-witness utxos is provided.
    if (!input.IsSane()) {
        return false;
    }

    if (input.non_witness_utxo) {
        // If we're taking our information from a non-witness UTXO, verify that it matches the prevout.
        COutPoint prevout = tx.vin[index].prevout;
        if (prevout.n >= input.non_witness_utxo->vout.size()) {
            return false;
        }
        if (input.non_witness_utxo->GetHash() != prevout.hash) {
            return false;
        }
        utxo = input.non_witness_utxo->vout[prevout.n];
    } else {
        return false;
    }

    bool sig_complete;
    if (use_dummy) {
        sig_complete = ProduceSignature(provider, DUMMY_SIGNATURE_CREATOR, utxo.scriptPubKey, sigdata);
    } else {
        MutableTransactionSignatureCreator creator(&tx, index, utxo.nValue, sighash);
        sig_complete = ProduceSignature(provider, creator, utxo.scriptPubKey, sigdata);
    }
    input.FromSignatureData(sigdata);

    // Fill in the missing info
    if (out_sigdata) {
        out_sigdata->missing_pubkeys = sigdata.missing_pubkeys;
        out_sigdata->missing_sigs = sigdata.missing_sigs;
        out_sigdata->missing_redeem_script = sigdata.missing_redeem_script;
    }

    return sig_complete;
}

bool FinalizePSBT(PartiallySignedTransaction& psbtx)
{
    // Finalize input signatures -- in case we have partial signatures that add up to a complete
    //   signature, but have not combined them yet (e.g. because the combiner that created this
    //   PartiallySignedTransaction did not understand them), this will combine them into a final
    //   script.
    bool complete = true;
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        complete &= SignPSBTInput(DUMMY_SIGNING_PROVIDER, psbtx, i, SIGHASH_ALL);
    }

    return complete;
}

bool FinalizeAndExtractPSBT(PartiallySignedTransaction& psbtx, CMutableTransaction& result)
{
    // It's not safe to extract a PSBT that isn't finalized, and there's no easy way to check
    //   whether a PSBT is finalized without finalizing it, so we just do this.
    if (!FinalizePSBT(psbtx)) {
        return false;
    }

    result = *psbtx.tx;
    for (unsigned int i = 0; i < result.vin.size(); ++i) {
        result.vin[i].scriptSig = psbtx.inputs[i].final_script_sig;
    }
    return true;
}

TransactionError CombinePSBTs(PartiallySignedTransaction& out, const std::vector<PartiallySignedTransaction>& psbtxs)
{
    out = psbtxs[0]; // Copy the first one

    // Merge
    for (auto it = std::next(psbtxs.begin()); it != psbtxs.end(); ++it) {
        if (!out.Merge(*it)) {
            return TransactionError::PSBT_MISMATCH;
        }
    }
    if (!out.IsSane()) {
        return TransactionError::INVALID_PSBT;
    }

    return TransactionError::OK;
}

std::string PSBTRoleName(PSBTRole role) {
    switch (role) {
    case PSBTRole::CREATOR: return "creator";
    case PSBTRole::UPDATER: return "updater";
    case PSBTRole::SIGNER: return "signer";
    case PSBTRole::FINALIZER: return "finalizer";
    case PSBTRole::EXTRACTOR: return "extractor";
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

PSBTAnalysis AnalyzePSBT(PartiallySignedTransaction psbtx)
{
    // Go through each input and build status
    PSBTAnalysis result;

    bool calc_fee = true;

    CAmount in_amt = 0;

    result.inputs.resize(psbtx.tx->vin.size());

    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        PSBTInput& input = psbtx.inputs[i];
        PSBTInputAnalysis& input_analysis = result.inputs[i];

        // We set next role here and ratchet backwards as required
        input_analysis.next = PSBTRole::EXTRACTOR;

        // Check for a UTXO
        CTxOut utxo;
        if (psbtx.GetInputUTXO(utxo, i)) {
            if (!MoneyRange(utxo.nValue) || !MoneyRange(in_amt + utxo.nValue)) {
                result.SetInvalid(strprintf("PSBT is not valid. Input %u has invalid value", i));
                return result;
            }
            in_amt += utxo.nValue;
            input_analysis.has_utxo = true;
        } else {
            if (input.non_witness_utxo && psbtx.tx->vin[i].prevout.n >= input.non_witness_utxo->vout.size()) {
                result.SetInvalid(strprintf("PSBT is not valid. Input %u specifies invalid prevout", i));
                return result;
            }
            input_analysis.has_utxo = false;
            input_analysis.is_final = false;
            input_analysis.next = PSBTRole::UPDATER;
            calc_fee = false;
        }

        if (!utxo.IsNull() && utxo.scriptPubKey.IsUnspendable()) {
            result.SetInvalid(strprintf("PSBT is not valid. Input %u spends unspendable output", i));
            return result;
        }

        // Check if it is final
        if (!utxo.IsNull() && !PSBTInputSigned(input)) {
            input_analysis.is_final = false;

            // Figure out what is missing
            SignatureData outdata;
            bool complete = SignPSBTInput(DUMMY_SIGNING_PROVIDER, psbtx, i, 1, &outdata);

            // Things are missing
            if (!complete) {
                input_analysis.missing_pubkeys = outdata.missing_pubkeys;
                input_analysis.missing_redeem_script = outdata.missing_redeem_script;
                input_analysis.missing_sigs = outdata.missing_sigs;

                // If we are only missing signatures and nothing else, then next is signer
                if (outdata.missing_pubkeys.empty() && outdata.missing_redeem_script.IsNull() && !outdata.missing_sigs.empty()) {
                    input_analysis.next = PSBTRole::SIGNER;
                } else {
                    input_analysis.next = PSBTRole::UPDATER;
                }
            } else {
                input_analysis.next = PSBTRole::FINALIZER;
            }
        } else if (!utxo.IsNull()){
            input_analysis.is_final = true;
        }
    }

    // Calculate next role for PSBT by grabbing "minumum" PSBTInput next role
    result.next = PSBTRole::EXTRACTOR;
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        PSBTInputAnalysis& input_analysis = result.inputs[i];
        result.next = std::min(result.next, input_analysis.next);
    }
    assert(result.next > PSBTRole::CREATOR);

    if (calc_fee) {
        // Get the output amount
        CAmount out_amt = std::accumulate(psbtx.tx->vout.begin(), psbtx.tx->vout.end(), CAmount(0),
            [](CAmount a, const CTxOut& b) {
            if (!MoneyRange(a) || !MoneyRange(b.nValue) || !MoneyRange(a + b.nValue)) {
                    return CAmount(-1);
                }
                return a += b.nValue;
            }
        );
        if (!MoneyRange(out_amt)) {
            result.SetInvalid(strprintf("PSBT is not valid. Output amount invalid"));
            return result;
        }

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
                newcoin.nHeight = 1;
                view.AddCoin(psbtx.tx->vin[i].prevout, std::move(newcoin), true);
            }
        }

        if (success) {
            CTransaction ctx = CTransaction(mtx);
            size_t size = GetVirtualTransactionSize(ctx, GetTransactionSigOpCount(ctx, view, STANDARD_SCRIPT_VERIFY_FLAGS));
            result.estimated_vsize = size;
            // Estimate fee rate
            CFeeRate feerate(fee, size);
            result.estimated_feerate = feerate;
        }

    }

    return result;
}

bool DecodeBase64PSBT(PartiallySignedTransaction& psbt, const std::string& base64_tx, std::string& error)
{
    bool invalid;
    std::string tx_data = DecodeBase64(base64_tx, &invalid);
    if (invalid) {
        error = "invalid base64";
        return false;
    }
    return DecodeRawPSBT(psbt, tx_data, error);
}

bool DecodeRawPSBT(PartiallySignedTransaction& psbt, const std::string& tx_data, std::string& error)
{
    CDataStream ss_data(tx_data.data(), tx_data.data() + tx_data.size(), SER_NETWORK, PROTOCOL_VERSION);
    try {
        ss_data >> psbt;
        if (!ss_data.empty()) {
            error = "extra data after PSBT";
            return false;
        }
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
    return true;
}
