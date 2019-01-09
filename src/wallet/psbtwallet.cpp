// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/protocol.h>
#include <wallet/psbtwallet.h>

bool FillPSBT(const CWallet* pwallet, PartiallySignedTransaction& psbtx, int sighash_type, bool sign, bool bip32derivs)
{
    LOCK(pwallet->cs_wallet);
    // Get all of the previous transactions
    bool complete = true;
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Verify input looks sane. This will check that we have at most one uxto, witness or non-witness.
        if (!input.IsSane()) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "PSBT input is not sane.");
        }

        // If we have no utxo, grab it from the wallet.
        if (!input.non_witness_utxo && input.witness_utxo.IsNull()) {
            const uint256& txhash = txin.prevout.hash;
            const auto it = pwallet->mapWallet.find(txhash);
            if (it != pwallet->mapWallet.end()) {
                const CWalletTx& wtx = it->second;
                // We only need the non_witness_utxo, which is a superset of the witness_utxo.
                //   The signing code will switch to the smaller witness_utxo if this is ok.
                input.non_witness_utxo = wtx.tx;
            }
        }

        // Get the Sighash type
        if (sign && input.sighash_type > 0 && input.sighash_type != sighash_type) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Specified Sighash and sighash in PSBT do not match.");
        }

        complete &= SignPSBTInput(HidingSigningProvider(pwallet, !sign, !bip32derivs), psbtx, i, sighash_type);
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that hardware wallets can identify change
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        const CTxOut& out = psbtx.tx->vout.at(i);
        PSBTOutput& psbt_out = psbtx.outputs.at(i);

        // Fill a SignatureData with output info
        SignatureData sigdata;
        psbt_out.FillSignatureData(sigdata);

        MutableTransactionSignatureCreator creator(psbtx.tx.get_ptr(), 0, out.nValue, 1);
        ProduceSignature(HidingSigningProvider(pwallet, true, !bip32derivs), creator, out.scriptPubKey, sigdata);
        psbt_out.FromSignatureData(sigdata);
    }
    return complete;
}
