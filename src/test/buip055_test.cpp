// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "data/tx_invalid.json.h"
#include "data/tx_valid.json.h"
#include "test/test_bitcoin.h"

#include "clientversion.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "key.h"
#include "keystore.h"
#include "main.h" // For CheckTransaction
#include "policy/policy.h"
#include "script/script.h"
#include "script/script_error.h"
#include "utilstrencodings.h"
#include "buip055fork.h"
#include "script/sign.h"

#include <map>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>

#include <univalue.h>

BOOST_FIXTURE_TEST_SUITE(buip055_tests, BasicTestingSetup)

// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 CENT outputs
// paid to a TX_PUBKEY, the second 21 and 22 CENT outputs
// paid to a TX_PUBKEYHASH.
//
static std::vector<CMutableTransaction>
SetupDummyInputs(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsRet)
{
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(i % 2);
        keystoreRet.AddKey(key[i]);
    }

    // Create some dummy input transactions
    dummyTransactions[0].vout.resize(2);
    dummyTransactions[0].vout[0].nValue = 11*CENT;
    dummyTransactions[0].vout[0].scriptPubKey << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = 50*CENT;
    dummyTransactions[0].vout[1].scriptPubKey << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    coinsRet.ModifyCoins(dummyTransactions[0].GetHash())->FromTx(dummyTransactions[0], 0);

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 21*CENT;
    dummyTransactions[1].vout[0].scriptPubKey = GetScriptForDestination(key[2].GetPubKey().GetID());
    dummyTransactions[1].vout[1].nValue = 22*CENT;
    dummyTransactions[1].vout[1].scriptPubKey = GetScriptForDestination(key[3].GetPubKey().GetID());
    coinsRet.ModifyCoins(dummyTransactions[1].GetHash())->FromTx(dummyTransactions[1], 0);

    return dummyTransactions;
}

BOOST_AUTO_TEST_CASE(buip055_op_return)
{

    // Check that a transaction with the invalid OP_RETURN is seen as invalid
    CMutableTransaction tx;
    tx.vout.resize(1);
    tx.vout[0].nValue = 0;
    tx.vout[0].scriptPubKey = CScript() << OP_RETURN << invalidOpReturn;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == true);

    CKey key;
    key.MakeNewKey(true);

    // Check that an arbitrary OP_RETURN is not invalid
    tx.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("010203040506070809");
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

    // Check that a normal tx (without OP_RETURN) is not invalid
    tx.vout[0].nValue = 50000;
    tx.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

    // Check that a normal tx with the invalid OP_RETURN is invalid
    tx.vout.resize(2);
    tx.vout[1].scriptPubKey = CScript() << OP_RETURN << invalidOpReturn;
    tx.vout[1].nValue = 0;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == true);

    // OP_RETURN must be the first instruction for it to could as an invalid tx on the new fork
    tx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_RETURN << invalidOpReturn;
    tx.vout[1].nValue = 0;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

    tx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << invalidOpReturn;
    tx.vout[1].nValue = 0;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

    tx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_RETURN << ParseHex("0") << invalidOpReturn;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

}

BOOST_AUTO_TEST_CASE(buip055_sighash)
{
    LOCK(cs_main);
    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    CMutableTransaction t;
    t.vin.resize(1);
    t.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t.vin[0].prevout.n = 1;
    t.vout.resize(1);
    t.vout[0].nValue = 90*CENT;
    CKey key;
    key.MakeNewKey(true);
    t.vout[0].scriptPubKey = GetScriptForDestination(key.GetPubKey().GetID());

    CTransaction tx(t);

    {
    TransactionSignatureCreator tsc(&keystore,&tx, 0, 90*CENT, SIGHASH_ALL);
    const CScript& scriptPubKey = dummyTransactions[0].vout[0].scriptPubKey;
    CScript& scriptSigRes = t.vin[0].scriptSig;
    bool worked = ProduceSignature(tsc,scriptPubKey,scriptSigRes);
    BOOST_CHECK(worked);
    BOOST_CHECK(IsTxProbablyNewSigHash(t)==false);
    }

    {
    TransactionSignatureCreator tsc(&keystore,&tx, 0, 90*CENT, SIGHASH_ALL | SIGHASH_FORKID);
    const CScript& scriptPubKey = dummyTransactions[0].vout[0].scriptPubKey;
    CScript& scriptSigRes = t.vin[0].scriptSig;
    bool worked = ProduceSignature(tsc,scriptPubKey,scriptSigRes);
    BOOST_CHECK(worked);
    BOOST_CHECK(IsTxProbablyNewSigHash(t)==true);
    }
}

BOOST_AUTO_TEST_SUITE_END()
