// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <consensus/validation.h>
#include <script/signingprovider.h>
#include <test/util/transaction_utils.h>

CMutableTransaction BuildCreditingTransaction(const CScript& scriptPubKey, CAmount nValue)
{
    CMutableTransaction txCredit;
    txCredit.version = 1;
    txCredit.nLockTime = 0;
    txCredit.vin = {
        CTxIn{COutPoint{}, CScript{} << CScriptNum(0) << CScriptNum(0), CTxIn::SEQUENCE_FINAL},
    };
    txCredit.vout = {
        CTxOut{nValue, scriptPubKey},
    };

    return txCredit;
}

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, const CTransaction& txCredit)
{
    CMutableTransaction txSpend;
    txSpend.version = 1;
    txSpend.nLockTime = 0;
    txSpend.vin = {
        CTxIn{COutPoint{txCredit.GetHash(), 0}, scriptSig, CTxIn::SEQUENCE_FINAL},
    };
    txSpend.vin[0].scriptWitness = scriptWitness;
    txSpend.vout = {
        CTxOut{txCredit.vout[0].nValue, CScript{}},
    };

    return txSpend;
}

std::vector<CMutableTransaction> SetupDummyInputs(FillableSigningProvider& keystoreRet, CCoinsViewCache& coinsRet, const std::array<CAmount,4>& nValues)
{
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    CKey key[4];
    for (int i = 0; i < 4; i++) {
        key[i].MakeNewKey(i % 2);
        keystoreRet.AddKey(key[i]);
    }

    // Create some dummy input transactions
    dummyTransactions[0].vout = {
        CTxOut{nValues[0], CScript{} << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG},
        CTxOut{nValues[1], CScript{} << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG},
    };
    AddCoins(coinsRet, CTransaction(dummyTransactions[0]), 0);

    dummyTransactions[1].vout = {
        CTxOut{nValues[2], GetScriptForDestination(PKHash(key[2].GetPubKey()))},
        CTxOut{nValues[3], GetScriptForDestination(PKHash(key[3].GetPubKey()))},
    };
    AddCoins(coinsRet, CTransaction(dummyTransactions[1]), 0);

    return dummyTransactions;
}

void BulkTransaction(CMutableTransaction& tx, int32_t target_weight)
{
    tx.vout.emplace_back(0, CScript() << OP_RETURN);
    auto unpadded_weight{GetTransactionWeight(CTransaction(tx))};
    assert(target_weight >= unpadded_weight);

    // determine number of needed padding bytes by converting weight difference to vbytes
    auto dummy_vbytes = (target_weight - unpadded_weight + (WITNESS_SCALE_FACTOR - 1)) / WITNESS_SCALE_FACTOR;
    // compensate for the increase of the compact-size encoded script length
    // (note that the length encoding of the unpadded output script needs one byte)
    dummy_vbytes -= GetSizeOfCompactSize(dummy_vbytes) - 1;

    // pad transaction by repeatedly appending a dummy opcode to the output script
    tx.vout[0].scriptPubKey.insert(tx.vout[0].scriptPubKey.end(), dummy_vbytes, OP_1);

    // actual weight should be at most 3 higher than target weight
    assert(GetTransactionWeight(CTransaction(tx)) >= target_weight);
    assert(GetTransactionWeight(CTransaction(tx)) <= target_weight + 3);
}

bool SignSignature(const SigningProvider &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType, SignatureData& sig_data)
{
    assert(nIn < txTo.vin.size());

    MutableTransactionSignatureCreator creator(txTo, nIn, amount, {.sighash_type = nHashType});

    bool ret = ProduceSignature(provider, creator, fromPubKey, sig_data);
    UpdateInput(txTo.vin.at(nIn), sig_data);
    return ret;
}

bool SignSignature(const SigningProvider &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType, SignatureData& sig_data)
{
    assert(nIn < txTo.vin.size());
    const CTxIn& txin = txTo.vin[nIn];
    assert(txin.prevout.n < txFrom.vout.size());
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    return SignSignature(provider, txout.scriptPubKey, txTo, nIn, txout.nValue, nHashType, sig_data);
}
