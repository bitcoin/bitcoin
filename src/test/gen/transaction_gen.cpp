// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <test/gen/transaction_gen.h>

#include <test/gen/crypto_gen.h>
#include <test/gen/script_gen.h>

#include <core_io.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <wallet/wallet.h>

#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/gen/Predicate.h>
#include <rapidcheck/gen/Select.h>

CMutableTransaction BuildCreditingTransaction(const CScript& scriptPubKey, int nValue = 0)
{
    CMutableTransaction txCredit;
    txCredit.nVersion = 1;
    txCredit.nLockTime = 0;
    txCredit.vin.resize(1);
    txCredit.vout.resize(1);
    txCredit.vin[0].prevout.SetNull();
    txCredit.vin[0].scriptSig = CScript() << CScriptNum(0) << CScriptNum(0);
    txCredit.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    txCredit.vout[0].scriptPubKey = scriptPubKey;
    txCredit.vout[0].nValue = nValue;

    return txCredit;
}

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, const CMutableTransaction& txCredit)
{
    CMutableTransaction txSpend;
    txSpend.nVersion = 1;
    txSpend.nLockTime = 0;
    txSpend.vin.resize(1);
    txSpend.vin[0].scriptWitness = scriptWitness;
    txSpend.vin[0].prevout.hash = txCredit.GetHash();
    txSpend.vin[0].prevout.n = 0;
    txSpend.vin[0].scriptSig = scriptSig;
    txSpend.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    txSpend.vout.resize(1);
    txSpend.vout[0].scriptPubKey = CScript();
    txSpend.vout[0].nValue = txCredit.vout[0].nValue;

    return txSpend;
}

/** Helper function to generate a tx that spends a spk */
SpendingInfo sign(const SPKCKeyPair& spk_keys, const CScript& redeemScript = CScript())
{
    const unsigned int inputIndex = 0;
    const CAmount nValue = 0;
    const CScript& spk = spk_keys.first;
    const std::vector<CKey>& keys = spk_keys.second;
    FillableSigningProvider store;
    for (const auto k : keys) {
        store.AddKey(k);
    }
    //add redeem script
    store.AddCScript(redeemScript);
    CMutableTransaction creditingTx = BuildCreditingTransaction(spk, nValue);
    CMutableTransaction spendingTx = BuildSpendingTransaction(CScript(), CScriptWitness(), creditingTx);
    SignatureData sigdata;
    MutableTransactionSignatureCreator creator(&spendingTx, inputIndex, 0);
    const SigningProvider& sp = store;
    assert(ProduceSignature(sp, creator, spk, sigdata));
    UpdateInput(spendingTx.vin[0], sigdata);
    const CTxOut& output = creditingTx.vout[0];
    const CTransaction finalTx = CTransaction(spendingTx);
    SpendingInfo tup = std::make_tuple(output, finalTx, inputIndex);
    return tup;
}

/** A signed tx that validly spends a P2PKSPK */
rc::Gen<SpendingInfo> SignedP2PKTx()
{
    return rc::gen::map(P2PKSPK(), [](const SPKCKeyPair& spk_key) {
        return sign(spk_key);
    });
}

rc::Gen<SpendingInfo> SignedP2PKHTx()
{
    return rc::gen::map(P2PKHSPK(), [](const SPKCKeyPair& spk_key) {
        return sign(spk_key);
    });
}

rc::Gen<SpendingInfo> SignedMultisigTx()
{
    return rc::gen::map(MultisigSPK(), [](const SPKCKeyPair& spk_key) {
        return sign(spk_key);
    });
}

rc::Gen<SpendingInfo> SignedP2SHTx()
{
    return rc::gen::map(RawSPK(), [](const SPKCKeyPair& spk_keys) {
        const CScript& redeemScript = spk_keys.first;
        const std::vector<CKey>& keys = spk_keys.second;
        //hash the spk
        const CScript& p2sh = GetScriptForDestination(ScriptHash(redeemScript));
        return sign(std::make_pair(p2sh, keys), redeemScript);
    });
}

rc::Gen<SpendingInfo> SignedP2WPKHTx()
{
    return rc::gen::map(P2WPKHSPK(), [](const SPKCKeyPair& spk_keys) {
        return sign(spk_keys);
    });
}

rc::Gen<SpendingInfo> SignedP2WSHTx()
{
    return rc::gen::map(MultisigSPK(), [](const SPKCKeyPair& spk_keys) {
        const CScript& redeemScript = spk_keys.first;
        const std::vector<CKey>& keys = spk_keys.second;
        const CScript p2wsh = GetScriptForWitness(redeemScript);
        return sign(std::make_pair(p2wsh, keys), redeemScript);
    });
}

/** Generates an arbitrary validly signed tx */
rc::Gen<SpendingInfo> SignedTx()
{
    return rc::gen::oneOf(SignedP2PKTx(), SignedP2PKHTx(),
        SignedMultisigTx(), SignedP2SHTx(), SignedP2WPKHTx(),
        SignedP2WSHTx());
}
