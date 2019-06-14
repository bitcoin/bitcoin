#include "omnicore/test/utils_tx.h"

#include "omnicore/createpayload.h"
#include "omnicore/encoding.h"
#include "omnicore/omnicore.h"
#include "omnicore/parsing.h"
#include "omnicore/rules.h"
#include "omnicore/script.h"
#include "omnicore/tx.h"

#include "base58.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"

#include <stdint.h>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_parsing_a_tests, BasicTestingSetup)

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
        int nBlock = 0;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(1765000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));
        txInputs.push_back(createTxOut(50000, "1JcWjikLW5CFV5XVTNSvqssJctYTwEf7Pc"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3MkYxeAhUDVqWvaHrP98iUt"));
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 50000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000100000002540be400000000");
    }
    {
        int nBlock = 0;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(907500, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));
        txInputs.push_back(createTxOut(907500, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1CVE9Au1XEm3MsiMuLcMKryYWXHeMv9tBn"));
        txOutputs.push_back(createTxOut(6000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(1747000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 50000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000200000002540be400000000");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

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
        int nBlock = 0;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(87000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1HKTxLaqcbH5GcqpAbStfFwH2zvho6q8ZD"));
        txOutputs.push_back(createTxOut(6000, "1HEBH9TXCR3NkKaNd5ZLugxzmc3Feqkbns"));
        txOutputs.push_back(createTxOut(7000, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(7000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 55000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000100000002540be400000000");
    }
    {
        int nBlock = ConsensusParams().SCRIPTHASH_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "34xhWktMFEGRTRmWf1hdNn1SyDDWiXa18H"));
        txInputs.push_back(createTxOut(100000, "34xhWktMFEGRTRmWf1hdNn1SyDDWiXa18H"));
        txInputs.push_back(createTxOut(200000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));
        txInputs.push_back(createTxOut(100000, "34xhWktMFEGRTRmWf1hdNn1SyDDWiXa18H"));
        txInputs.push_back(createTxOut(200000, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));
        txOutputs.push_back(createTxOut(6000, "1HKTxLaqcbH5GcqpAbStfFwH2zvho6q8ZD"));
        txOutputs.push_back(createTxOut(6000, "1HQkdXiA2mWmoQkrknyxrHTfeBoKhthq23"));
        txOutputs.push_back(createTxOut(6001, "1CcJFxoEW5PUwesMVxGrq6kAPJ1TJsSVqq"));
        txOutputs.push_back(createTxOut(665999, "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk"));

        CTransaction dummyTx = TxClassA(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 10000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1HRE7U9XNPD8kJBCwm5Q1VAepz25GBXnVk");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1HQkdXiA2mWmoQkrknyxrHTfeBoKhthq23");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000000000000000100000002540be400000000");
    }
    {
        int nBlock = 0;

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
        int nBlock = ConsensusParams().SCRIPTHASH_BLOCK;

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
        int nBlock = 0;

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
        int nBlock = 0;

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
        int nBlock = 0;

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


BOOST_AUTO_TEST_SUITE_END()
