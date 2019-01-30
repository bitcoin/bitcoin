// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/psbtwallet.h>

bool FillPSBT(const CWallet* pwallet, PartiallySignedTransaction& psbtx, TransactionError& error, bool& complete, int sighash_type, bool sign, bool bip32derivs)
{
    LOCK(pwallet->cs_wallet);
    // Get all of the previous transactions
    complete = true;
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        const uint256& txhash = txin.prevout.hash;
        const auto it = pwallet->mapWallet.find(txhash);
        if (it != pwallet->mapWallet.end()) {
            const CWalletTx& wtx = it->second;
            // We only need the non_witness_utxo, which is a superset of the witness_utxo.
            //   The signing code will switch to the smaller witness_utxo if this is ok.
            input.non_witness_utxo = wtx.tx;
        }

        // Get the Sighash type
        if (sign && input.sighash_type > 0 && input.sighash_type != sighash_type) {
            error = TransactionError::SIGHASH_MISMATCH;
            return false;
        }

        complete &= SignPSBTInput(HidingSigningProvider(pwallet, !sign, !bip32derivs), *psbtx.tx, input, i, sighash_type);
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

    return true;
}

