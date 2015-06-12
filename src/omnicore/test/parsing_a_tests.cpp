#include "omnicore/createpayload.h"
#include "omnicore/encoding.h"
#include "omnicore/omnicore.h"
#include "omnicore/script.h"
#include "omnicore/tx.h"

#include "base58.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "utilstrencodings.h"

#include <stdint.h>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(omnicore_parsing_a_tests)

static CTxOut PayToPubKey_Unrelated();
static CTxOut NonStandardOutput();
static CTxOut OpReturn_Unrelated();

/** Creates a dummy transaction with the given inputs and outputs. */
static CTransaction TxClassA(const std::vector<CTxOut>& txInputs, const std::vector<CTxOut>& txOuts)
{
    CMutableTransaction mutableTx;

    // Inputs:
    for (std::vector<CTxOut>::const_iterator it = txInputs.begin(); it != txInputs.end(); ++it)
    {
        const CTxOut& txOut = *it;

        // Create transaction for input:
        CMutableTransaction inputTx;
        unsigned int nOut = 0;
        inputTx.vout.push_back(txOut);
        CTransaction tx(inputTx);

        // Populate transaction cache:
        CCoinsModifier coins = view.ModifyCoins(tx.GetHash());

        if (nOut >= coins->vout.size()) {
            coins->vout.resize(nOut+1);
        }
        coins->vout[nOut].scriptPubKey = txOut.scriptPubKey;
        coins->vout[nOut].nValue = txOut.nValue;

        // Add input:
        CTxIn txIn(tx.GetHash(), nOut);
        mutableTx.vin.push_back(txIn);
    }

    for (std::vector<CTxOut>::const_iterator it = txOuts.begin(); it != txOuts.end(); ++it)
    {
        const CTxOut& txOut = *it;

        mutableTx.vout.push_back(txOut);
    }

    return CTransaction(mutableTx);
}

/** Helper to create a CTxOut object. */
static CTxOut createTxOut(int64_t amount, const std::string& dest)
{
    return CTxOut(amount, GetScriptForDestination(CBitcoinAddress(dest).Get()));
}

BOOST_AUTO_TEST_CASE(valid_class_a)
{
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3MkYxeAhUDVqWvaHrP98iUt"));
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000100000002540be400000000");
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3MsiMuLcMKryYWXHeMv9tBn"));
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000200000002540be400000000");
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;

        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(OpReturn_Unrelated());
        txOutputs.push_back(OpReturn_Unrelated());
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3MkYxeAhUDVqWvaHrP98iUt"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000100000002540be400000000");
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1HKTxLaqcbH5GcqpAbStfFwH2zvho6q8ZD"));
        txOutputs.push_back(createTxOut(6000, "1HEBH9TXCR3NkKaNd5ZLugxzmc3Feqkbns"));
        txOutputs.push_back(createTxOut(1747000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000100000002540be400000000");
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1HKTxLaqcbH5GcqpAbStfFwH2zvho6q8ZD"));
        txOutputs.push_back(createTxOut(6000, "1HQkdXiA2mWmoQkrknyxrHTfeBoKhthq23"));
        txOutputs.push_back(createTxOut(6001, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1HQkdXiA2mWmoQkrknyxrHTfeBoKhthq23");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000100000002540be400000000");
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(70000, "1e1jLtDvEetgwJ6jB3DuCf27sXcjA8qQ2"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(9001, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(9001, "1e1jLtDvEetgwJ6jB3DuCf27sXcjA8qQ2"));
        txOutputs.push_back(createTxOut(9001, "1e1jLtDvEethgGWJCWWZPVBdZWNM3zVYP"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1e1jLtDvEetgwJ6jB3DuCf27sXcjA8qQ2");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1e1jLtDvEethgGWJCWWZPVBdZWNM3zVYP");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "00000000000000010000000777777700000000");
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "3Kpeeo8MVoYnx7PeNb5FUus8bkJsZFPbw7"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6001, "344VVXmhPVTYnaNYNj3xgkcy3wasEEZtur"));
        txOutputs.push_back(createTxOut(6002, "34CxDzguRHnxA6fwccVBfC3wCZfvwmHAxV"));
        txOutputs.push_back(createTxOut(6003, "3J7F31dxvHXWqTse4rjzS7XayWJnr5fZqW"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3Kpeeo8MVoYnx7PeNb5FUus8bkJsZFPbw7");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "34CxDzguRHnxA6fwccVBfC3wCZfvwmHAxV");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000200000002540be400000000");
    }
}

BOOST_AUTO_TEST_CASE(invalid_class_a)
{
    // More than one data packet
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3MkYxeAhUDVqWvaHrP98iUt"));
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3MkYxeAhUDVqWvaHrP98iUt"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) != 0);
    }
    // Not MSC or TMSC
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3NVXNDCAksgfgSGGc1xs4Wi"));
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) != 0);
    }
    // Seq collision
    {
        int nBlock = std::numeric_limits<int>().max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1815000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1HKTxLaqcbH5GcqpAbStfFwH2zvho6q8ZD"));
        txOutputs.push_back(createTxOut(6000, "1HQkdXiA2mWmoQkrknyxrHTfeBoKhthq23"));
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) != 0);
    }
}

static CTxOut PayToPubKey_Unrelated()
{
    std::vector<unsigned char> vchPubKey = ParseHex(
        "04ad90e5b6bc86b3ec7fac2c5fbda7423fc8ef0d58df594c773fa05e2c281b2bfe"
        "877677c668bd13603944e34f4818ee03cadd81a88542b8b4d5431264180e2c28");

    CPubKey pubKey(vchPubKey.begin(), vchPubKey.end());

    CScript scriptPubKey;
    scriptPubKey << ToByteVector(pubKey) << OP_CHECKSIG;

    int64_t amount = GetDustThreshold(scriptPubKey);

    txnouttype outType;
    BOOST_CHECK(GetOutputType(scriptPubKey, outType));
    BOOST_CHECK(outType == TX_PUBKEY);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut OpReturn_Unrelated()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("7468697320697320646578782028323031352d30362d313129");

    return CTxOut(0, scriptPubKey);
}

static CTxOut NonStandardOutput()
{
    CScript scriptPubKey;
    scriptPubKey << ParseHex("decafbad") << OP_DROP << OP_TRUE;
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}


BOOST_AUTO_TEST_SUITE_END()

