// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/factories/transactions.h>

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, int nValue, const uint256& prevout_hash)
{
    CMutableTransaction txSpend;
    txSpend.nVersion = 1;
    txSpend.nLockTime = 0;
    txSpend.vin.resize(1);
    txSpend.vout.resize(1);
    txSpend.vin[0].prevout.hash = prevout_hash;
    txSpend.vin[0].prevout.n = 0;
    txSpend.vin[0].scriptSig = scriptSig;
    txSpend.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    txSpend.vout[0].scriptPubKey = CScript();
    txSpend.vout[0].nValue = nValue;

    return txSpend;
}

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, int nValue, const uint256& prevout_hash)
{
    CMutableTransaction txSpend{BuildSpendingTransaction(scriptSig, nValue, prevout_hash)};
    txSpend.vin[0].scriptWitness = scriptWitness;

    return txSpend;
}

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CMutableTransaction& txCredit)
{
    return BuildSpendingTransaction(scriptSig, txCredit.vout[0].nValue, txCredit.GetHash());
}

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, const CMutableTransaction& txCredit)
{
    CMutableTransaction txSpend{BuildSpendingTransaction(scriptSig, txCredit)};
    txSpend.vin[0].scriptWitness = scriptWitness;

    return txSpend;
}

CMutableTransaction BuildCreditingTransaction(const CScript& scriptPubKey, int nValue)
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
