#include <map>
#include <string>
#include <boost/test/unit_test.hpp>
#include "json/json_spirit_writer_template.h"

#include "main.h"
#include "wallet.h"

using namespace std;
using namespace json_spirit;

// In script_tests.cpp
extern Array read_json(const std::string& filename);
extern CScript ParseScript(string s);

BOOST_AUTO_TEST_SUITE(transaction_tests)

BOOST_AUTO_TEST_CASE(tx_valid)
{
    // Read tests from test/data/tx_valid.json
    // Format is an array of arrays
    // Inner arrays are either [ "comment" ]
    // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, enforceP2SH
    // ... where all scripts are stringified scripts.
    Array tests = read_json("tx_valid.json");

    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        string strTest = write_string(tv, false);
        if (test[0].type() == array_type)
        {
            if (test.size() != 3 || test[1].type() != str_type || test[2].type() != bool_type)
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
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION);
            CTransaction tx;
            stream >> tx;

            CValidationState state;
            BOOST_CHECK_MESSAGE(tx.CheckTransaction(state), strTest);
            BOOST_CHECK(state.IsValid());

            for (unsigned int i = 0; i < tx.vin.size(); i++)
            {
                if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    break;
                }

                BOOST_CHECK_MESSAGE(VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout], tx, i, test[2].get_bool() ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE, 0), strTest);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(tx_invalid)
{
    // Read tests from test/data/tx_invalid.json
    // Format is an array of arrays
    // Inner arrays are either [ "comment" ]
    // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, enforceP2SH
    // ... where all scripts are stringified scripts.
    Array tests = read_json("tx_invalid.json");

    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        string strTest = write_string(tv, false);
        if (test[0].type() == array_type)
        {
            if (test.size() != 3 || test[1].type() != str_type || test[2].type() != bool_type)
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
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION);
            CTransaction tx;
            stream >> tx;

            CValidationState state;
            fValid = tx.CheckTransaction(state) && state.IsValid();

            for (unsigned int i = 0; i < tx.vin.size() && fValid; i++)
            {
                if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    break;
                }

                fValid = VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout], tx, i, test[2].get_bool() ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE, 0);
            }

            BOOST_CHECK_MESSAGE(!fValid, strTest);
        }
    }
}

BOOST_AUTO_TEST_CASE(basic_transaction_tests)
{
    // Random real transaction (0aad88ccf4a93e6e0c51a4b2c735b72b049a1b0d078dbe033a3ef0e9536483d3)
    unsigned char ch[] = {1, 0, 0, 0, 199, 164, 233, 86, 2, 171, 88, 83, 94, 35, 232, 126, 67, 179, 60, 10, 33, 147, 214, 124, 159, 158, 168, 5, 176, 215, 131, 102, 144, 176, 62, 55, 234, 104, 131, 227, 50, 0, 0, 0, 0, 106, 71, 48, 68, 2, 32, 88, 154, 50, 124, 193, 82, 175, 93, 51, 97, 189, 152, 199, 127, 180, 209, 71, 49, 47, 127, 50, 198, 163, 180, 64, 191, 246, 174, 84, 5, 212, 215, 2, 32, 18, 33, 141, 93, 247, 182, 40, 158, 113, 103, 131, 179, 96, 254, 95, 0, 216, 45, 248, 190, 253, 163, 66, 216, 74, 144, 254, 145, 223, 34, 197, 175, 1, 33, 3, 44, 11, 40, 58, 0, 195, 137, 189, 135, 164, 112, 10, 218, 234, 79, 108, 142, 191, 128, 216, 61, 169, 146, 235, 252, 42, 140, 235, 149, 30, 60, 123, 255, 255, 255, 255, 229, 149, 206, 86, 116, 48, 226, 74, 58, 240, 243, 222, 116, 172, 234, 175, 208, 114, 244, 83, 185, 107, 168, 161, 52, 212, 91, 57, 201, 238, 196, 79, 0, 0, 0, 0, 106, 71, 48, 68, 2, 32, 56, 191, 119, 175, 41, 39, 97, 9, 15, 84, 12, 14, 99, 15, 205, 67, 120, 159, 187, 33, 80, 201, 104, 81, 19, 136, 230, 248, 187, 124, 104, 188, 2, 32, 96, 31, 40, 246, 36, 40, 39, 65, 151, 172, 231, 201, 14, 12, 99, 74, 73, 37, 1, 92, 58, 192, 253, 14, 250, 249, 245, 11, 226, 232, 204, 137, 1, 33, 3, 246, 76, 253, 20, 111, 183, 124, 112, 238, 11, 28, 102, 137, 133, 107, 97, 47, 69, 144, 217, 121, 33, 38, 128, 210, 2, 43, 187, 217, 129, 245, 157, 255, 255, 255, 255, 1, 64, 167, 144, 0, 0, 0, 0, 0, 25, 118, 169, 20, 65, 1, 103, 158, 39, 245, 201, 210, 142, 205, 250, 28, 169, 18, 134, 215, 5, 108, 237, 91, 136, 172, 0, 0, 0, 0, 0, 0};
    vector<unsigned char> vch(ch, ch + sizeof(ch) -1);
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    CTransaction tx;
    stream >> tx;
    CValidationState state;
    BOOST_CHECK_MESSAGE(tx.CheckTransaction(state) && state.IsValid(), "Simple deserialized transaction should be valid.");

    // Check that duplicate txins fail
    tx.vin.push_back(tx.vin[0]);
    BOOST_CHECK_MESSAGE(!tx.CheckTransaction(state) || !state.IsValid(), "Transaction with duplicate txins should be invalid.");
}

//
// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 CENT outputs
// paid to a TX_PUBKEY, the second 21 and 22 CENT outputs
// paid to a TX_PUBKEYHASH.
//
static std::vector<CTransaction>
SetupDummyInputs(CBasicKeyStore& keystoreRet, CCoinsView & coinsRet)
{
    std::vector<CTransaction> dummyTransactions;
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
    coinsRet.SetCoins(dummyTransactions[0].GetHash(), CCoins(dummyTransactions[0], 0));

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 21*CENT;
    dummyTransactions[1].vout[0].scriptPubKey.SetDestination(key[2].GetPubKey().GetID());
    dummyTransactions[1].vout[1].nValue = 22*CENT;
    dummyTransactions[1].vout[1].scriptPubKey.SetDestination(key[3].GetPubKey().GetID());
    coinsRet.SetCoins(dummyTransactions[1].GetHash(), CCoins(dummyTransactions[1], 0));

    return dummyTransactions;
}

BOOST_AUTO_TEST_CASE(test_Get)
{
    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(coinsDummy);
    std::vector<CTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    CTransaction t1;
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

    BOOST_CHECK(t1.AreInputsStandard(coins));
    BOOST_CHECK_EQUAL(t1.GetValueIn(coins), (50+21+22)*CENT);

    // Adding extra junk to the scriptSig should make it non-standard:
    t1.vin[0].scriptSig << OP_11;
    BOOST_CHECK(!t1.AreInputsStandard(coins));

    // ... as should not having enough:
    t1.vin[0].scriptSig = CScript();
    BOOST_CHECK(!t1.AreInputsStandard(coins));
}

BOOST_AUTO_TEST_CASE(test_IsStandard)
{
    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(coinsDummy);
    std::vector<CTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    CTransaction t;
    t.vin.resize(1);
    t.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t.vin[0].prevout.n = 1;
    t.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t.vout.resize(1);
    t.vout[0].nValue = 90*CENT;
    CKey key;
    key.MakeNewKey(true);
    t.vout[0].scriptPubKey.SetDestination(key.GetPubKey().GetID());
    BOOST_CHECK(t.IsStandard());

    t.vout[0].scriptPubKey = CScript() << OP_1;
    BOOST_CHECK(!t.IsStandard());

    // 80-byte TX_NULL_DATA (standard)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    BOOST_CHECK(t.IsStandard());

    // 81-byte TX_NULL_DATA (non-standard)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3800");
    BOOST_CHECK(!t.IsStandard());

    // TX_NULL_DATA w/o PUSHDATA
    t.vout.resize(1);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(t.IsStandard());

    // Only one TX_NULL_DATA permitted in all cases
    t.vout.resize(2);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    BOOST_CHECK(!t.IsStandard());

    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(!t.IsStandard());

    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    BOOST_CHECK(!t.IsStandard());
}

BOOST_AUTO_TEST_SUITE_END()
