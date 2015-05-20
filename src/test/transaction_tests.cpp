// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "data/tx_invalid.json.h"
#include "data/tx_valid.json.h"

#include "key.h"
#include "keystore.h"
#include "main.h"
#include "script.h"

#include <map>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/test/unit_test.hpp>
#include "json/json_spirit_writer_template.h"

using namespace std;
using namespace json_spirit;
using namespace boost::algorithm;

// In script_tests.cpp
extern Array read_json(const std::string& jsondata);
extern CScript ParseScript(string s);

unsigned int ParseFlags(string strFlags){
    unsigned int flags = 0;
    vector<string> words;
    split(words, strFlags, is_any_of(","));

    // Note how NOCACHE is not included as it is a runtime-only flag.
    static map<string, unsigned int> mapFlagNames;
    if (mapFlagNames.size() == 0)
    {
        mapFlagNames["NONE"] = SCRIPT_VERIFY_NONE;
        mapFlagNames["P2SH"] = SCRIPT_VERIFY_P2SH;
        mapFlagNames["STRICTENC"] = SCRIPT_VERIFY_STRICTENC;
        mapFlagNames["LOW_S"] = SCRIPT_VERIFY_LOW_S;
        mapFlagNames["NULLDUMMY"] = SCRIPT_VERIFY_NULLDUMMY;
    }

    BOOST_FOREACH(string word, words)
    {
        if (!mapFlagNames.count(word))
            BOOST_ERROR("Bad test: unknown verification flag '" << word << "'");
        flags |= mapFlagNames[word];
    }

    return flags;
}

BOOST_AUTO_TEST_SUITE(transaction_tests)

BOOST_AUTO_TEST_CASE(tx_valid)
{
    // Read tests from test/data/tx_valid.json
    // Format is an array of arrays
    // Inner arrays are either [ "comment" ]
    // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, verifyFlags
    // ... where all scripts are stringified scripts.
    //
    // verifyFlags is a comma separated list of script verification flags to apply, or "NONE"
    Array tests = read_json(std::string(json_tests::tx_valid, json_tests::tx_valid + sizeof(json_tests::tx_valid)));

    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        string strTest = write_string(tv, false);
        if (test[0].type() == array_type)
        {
            if (test.size() != 3 || test[1].type() != str_type || test[2].type() != str_type)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            map<COutPoint, CScript> mapprevOutScriptPubKeys;
            Array inputs = test[0].get_array();
            bool fValid = true;
            BOOST_FOREACH(Value& input, inputs)
            {
                if (input.type() != array_type)
                {
                    fValid = false;
                    break;
                }
                Array vinput = input.get_array();
                if (vinput.size() != 3)
                {
                    fValid = false;
                    break;
                }

                mapprevOutScriptPubKeys[COutPoint(uint256(vinput[0].get_str()), vinput[1].get_int())] = ParseScript(vinput[2].get_str());
            }
            if (!fValid)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            string transaction = test[1].get_str();
            CDataStream stream(ParseHex(transaction), SER_NETWORK, CREDITS_PROTOCOL_VERSION);
            Credits_CTransaction tx;
            stream >> tx;

            CValidationState state;
            BOOST_CHECK_MESSAGE(Bitcredit_CheckTransaction(tx, state), strTest);
            BOOST_REQUIRE_MESSAGE(state.IsValid(), state.GetRejectReason());

            for (unsigned int i = 0; i < tx.vin.size(); i++)
            {
                if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    break;
                }

                unsigned int verify_flags = ParseFlags(test[2].get_str());
                //TODO - Enable this, most important part of test
//                BOOST_CHECK_MESSAGE(Bitcredit_VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout],
//                                                 tx, i, verify_flags, 0),
//                                    strTest);
            }
        }
    }
}

//TODO - Enable this
BOOST_AUTO_TEST_CASE(tx_invalid)
{
//    // Read tests from test/data/tx_invalid.json
//    // Format is an array of arrays
//    // Inner arrays are either [ "comment" ]
//    // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, verifyFlags
//    // ... where all scripts are stringified scripts.
//    //
//    // verifyFlags is a comma separated list of script verification flags to apply, or "NONE"
//    Array tests = read_json(std::string(json_tests::tx_invalid, json_tests::tx_invalid + sizeof(json_tests::tx_invalid)));
//
//    BOOST_FOREACH(Value& tv, tests)
//    {
//        Array test = tv.get_array();
//        string strTest = write_string(tv, false);
//        if (test[0].type() == array_type)
//        {
//            if (test.size() != 3 || test[1].type() != str_type || test[2].type() != str_type)
//            {
//                BOOST_ERROR("Bad test: " << strTest);
//                continue;
//            }
//
//            map<COutPoint, CScript> mapprevOutScriptPubKeys;
//            Array inputs = test[0].get_array();
//            bool fValid = true;
//            BOOST_FOREACH(Value& input, inputs)
//            {
//                if (input.type() != array_type)
//                {
//                    fValid = false;
//                    break;
//                }
//                Array vinput = input.get_array();
//                if (vinput.size() != 3)
//                {
//                    fValid = false;
//                    break;
//                }
//
//                mapprevOutScriptPubKeys[COutPoint(uint256(vinput[0].get_str()), vinput[1].get_int())] = ParseScript(vinput[2].get_str());
//            }
//            if (!fValid)
//            {
//                BOOST_ERROR("Bad test: " << strTest);
//                continue;
//            }
//
//            string transaction = test[1].get_str();
//            CDataStream stream(ParseHex(transaction), SER_NETWORK, CREDITS_PROTOCOL_VERSION);
//            Credits_CTransaction tx;
//            stream >> tx;
//
//            CValidationState state;
//            fValid = Bitcredit_CheckTransaction(tx, state) && state.IsValid();
//
//            for (unsigned int i = 0; i < tx.vin.size() && fValid; i++)
//            {
//                if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
//                {
//                    BOOST_ERROR("Bad test: " << strTest);
//                    break;
//                }
//
//                unsigned int verify_flags = ParseFlags(test[2].get_str());
//                fValid = Bitcredit_VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout],
//                                      tx, i, verify_flags, 0);
//            }
//
//            BOOST_CHECK_MESSAGE(!fValid, strTest);
//        }
//    }
}

BOOST_AUTO_TEST_CASE(basic_transaction_tests)
{
    // Random real transaction
    string transaction = "010000000100000001b14bdcbc3e01bdaad36cc08e81e69c82e1060bc14e518db2b49aa43ad90ba26000000000490047304402203f16c6f40162ab686621ef3000b04e75418a0c0cb2d8aebeac894ae360ac1e780220ddc15ecdfc3507ac48e1681a33eb60996631bf6bf5bc0a0682c4db743ce7ca2b010140420f00000000001976a914660d4ef3a743e3e696ad990364e555c271ad504b88ac00000000";
    CDataStream stream(ParseHex(transaction), SER_DISK, CREDITS_CLIENT_VERSION);
    Credits_CTransaction tx;
    stream >> tx;
    CValidationState state;
    BOOST_CHECK_MESSAGE(Bitcredit_CheckTransaction(tx, state) && state.IsValid(), "Simple deserialized transaction should be valid.");

    // Check that duplicate txins fail
    tx.vin.push_back(tx.vin[0]);
    BOOST_CHECK_MESSAGE(!Bitcredit_CheckTransaction(tx, state) || !state.IsValid(), "Transaction with duplicate txins should be invalid.");
}

//
// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 CENT outputs
// paid to a TX_PUBKEY, the second 21 and 22 CENT outputs
// paid to a TX_PUBKEYHASH.
//
static std::vector<Credits_CTransaction>
SetupDummyInputs(CBasicKeyStore& keystoreRet, Credits_CCoinsView & coinsRet)
{
    std::vector<Credits_CTransaction> dummyTransactions;
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
    dummyTransactions[0].vout[0].scriptPubKey << key[0].GetPubKey() << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = 50*CENT;
    dummyTransactions[0].vout[1].scriptPubKey << key[1].GetPubKey() << OP_CHECKSIG;
    coinsRet.SetCoins(dummyTransactions[0].GetHash(), Credits_CCoins(dummyTransactions[0], 0));

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 21*CENT;
    dummyTransactions[1].vout[0].scriptPubKey.SetDestination(key[2].GetPubKey().GetID());
    dummyTransactions[1].vout[1].nValue = 22*CENT;
    dummyTransactions[1].vout[1].scriptPubKey.SetDestination(key[3].GetPubKey().GetID());
    coinsRet.SetCoins(dummyTransactions[1].GetHash(), Credits_CCoins(dummyTransactions[1], 0));

    return dummyTransactions;
}

BOOST_AUTO_TEST_CASE(test_Get)
{
    CBasicKeyStore keystore;
    Credits_CCoinsView coinsDummy;
    Credits_CCoinsViewCache bitcredit_coins(coinsDummy);
    Bitcoin_CClaimCoinsView bitcoin_coinsDummy;
    Bitcoin_CClaimCoinsViewCache bitcoin_coins(bitcoin_coinsDummy, bitcoin_nClaimCoinCacheFlushSize);
    std::vector<Credits_CTransaction> dummyTransactions = SetupDummyInputs(keystore, bitcredit_coins);

    Credits_CTransaction t1;
    t1.vin.resize(3);
    t1.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t1.vin[0].prevout.n = 1;
    t1.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t1.vin[1].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[1].prevout.n = 0;
    t1.vin[1].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vin[2].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[2].prevout.n = 1;
    t1.vin[2].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vout.resize(2);
    t1.vout[0].nValue = 90*CENT;
    t1.vout[0].scriptPubKey << OP_1;

    BOOST_CHECK(Bitcredit_AreInputsStandard(t1, bitcredit_coins, bitcoin_coins));
    BOOST_CHECK_EQUAL(bitcredit_coins.GetValueIn(t1), (50+21+22)*CENT);

    // Adding extra junk to the scriptSig should make it non-standard:
    t1.vin[0].scriptSig << OP_11;
    BOOST_CHECK(!Bitcredit_AreInputsStandard(t1, bitcredit_coins, bitcoin_coins));

    // ... as should not having enough:
    t1.vin[0].scriptSig = CScript();
    BOOST_CHECK(!Bitcredit_AreInputsStandard(t1, bitcredit_coins, bitcoin_coins));
}

BOOST_AUTO_TEST_CASE(test_IsStandard)
{
    LOCK(credits_mainState.cs_main);
    CBasicKeyStore keystore;
    Credits_CCoinsView coinsDummy;
    Credits_CCoinsViewCache coins(coinsDummy);
    std::vector<Credits_CTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    Credits_CTransaction t;
    t.vin.resize(1);
    t.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t.vin[0].prevout.n = 1;
    t.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t.vout.resize(1);
    t.vout[0].nValue = 90*CENT;
    CKey key;
    key.MakeNewKey(true);
    t.vout[0].scriptPubKey.SetDestination(key.GetPubKey().GetID());

    string reason;
    BOOST_CHECK(Bitcredit_IsStandardTx(t, reason));

    t.vout[0].nValue = 501; // dust
    BOOST_CHECK(!Bitcredit_IsStandardTx(t, reason));

    t.vout[0].nValue = 601; // not dust
    BOOST_CHECK(Bitcredit_IsStandardTx(t, reason));

    t.vout[0].scriptPubKey = CScript() << OP_1;
    BOOST_CHECK(!Bitcredit_IsStandardTx(t, reason));

    // 40-byte TX_NULL_DATA (standard)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    BOOST_CHECK(Bitcredit_IsStandardTx(t, reason));

    // 41-byte TX_NULL_DATA (non-standard)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3800");
    BOOST_CHECK(!Bitcredit_IsStandardTx(t, reason));

    // TX_NULL_DATA w/o PUSHDATA
    t.vout.resize(1);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(Bitcredit_IsStandardTx(t, reason));

    // Only one TX_NULL_DATA permitted in all cases
    t.vout.resize(2);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    BOOST_CHECK(!Bitcredit_IsStandardTx(t, reason));

    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(!Bitcredit_IsStandardTx(t, reason));

    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(!Bitcredit_IsStandardTx(t, reason));
}

BOOST_AUTO_TEST_SUITE_END()
