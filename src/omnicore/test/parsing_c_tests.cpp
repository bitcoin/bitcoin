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
#include "utilstrencodings.h"

#include <stdint.h>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_parsing_c_tests, BasicTestingSetup)

/** Creates a dummy transaction with the given inputs and outputs. */
static CTransaction TxClassC(const std::vector<CTxOut>& txInputs, const std::vector<CTxOut>& txOuts)
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

BOOST_AUTO_TEST_CASE(reference_identification)
{
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(5000000, "1NNQKWM8mC35pBNPxV1noWFZEw7A5X6zXz"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(createTxOut(2700000, ExodusAddress().ToString()));

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK(metaTx.getReceiver().empty());
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 2300000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1NNQKWM8mC35pBNPxV1noWFZEw7A5X6zXz");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "00000000000000070000000006dac2c0");
    }
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK + 1000;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(6000, "3NfRfUekDSzgSyohRro9jXD1AqDALN321P"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(createTxOut(6000, "3QHw8qKf1vQkMnSVXarq7N4PYzz1G3mAK4"));

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3NfRfUekDSzgSyohRro9jXD1AqDALN321P");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "3QHw8qKf1vQkMnSVXarq7N4PYzz1G3mAK4");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "00000000000000070000000006dac2c0");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(80000, "1M773vkrQDtBpkorfHTdctRo6kHxb4fXuT"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(createTxOut(6000, "1M773vkrQDtBpkorfHTdctRo6kHxb4fXuT"));

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 74000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1M773vkrQDtBpkorfHTdctRo6kHxb4fXuT");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1M773vkrQDtBpkorfHTdctRo6kHxb4fXuT");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "00000000000000070000000006dac2c0");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(80000, "19NNUnwsKZK5dCnDCZ7pqeruL2syA8hSVh"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(createTxOut(6000, "3NjfEthPHg6GEH9j7o4j4BGGZafBX2yw8j"));
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(createTxOut(6000, "37pwWHk1oFaWxVsnYKfGs7Lyt5yJEVomTH"));
        txOutputs.push_back(PayToBareMultisig_1of3());
        txOutputs.push_back(createTxOut(6000, "19NNUnwsKZK5dCnDCZ7pqeruL2syA8hSVh"));

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "19NNUnwsKZK5dCnDCZ7pqeruL2syA8hSVh");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "37pwWHk1oFaWxVsnYKfGs7Lyt5yJEVomTH");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(55550, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "3NjfEthPHg6GEH9j7o4j4BGGZafBX2yw8j"));
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(createTxOut(6000, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));
        txOutputs.push_back(createTxOut(6000, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));
        txOutputs.push_back(PayToPubKeyHash_Exodus());
        txOutputs.push_back(OpReturn_SimpleSend());

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "35iqJySouevicrYzMhjKSsqokSGwGovGov");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "35iqJySouevicrYzMhjKSsqokSGwGovGov");
    }
}

BOOST_AUTO_TEST_CASE(empty_op_return)
{
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(900000, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_PlainMarker());
        txOutputs.push_back(PayToPubKeyHash_Unrelated());

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK(metaTx.getPayload().empty());
        BOOST_CHECK_EQUAL(metaTx.getSender(), "35iqJySouevicrYzMhjKSsqokSGwGovGov");
        // via PayToPubKeyHash_Unrelated:
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "1f2dj45pxYb8BCW5sSbCgJ5YvXBfSapeX");
    }
}


BOOST_AUTO_TEST_CASE(trimmed_op_return)
{
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;

        std::vector<unsigned char> vchFiller(MAX_PACKETS * PACKET_SIZE, 0x07);
        std::vector<unsigned char> vchPayload = GetOmMarker();
        vchPayload.insert(vchPayload.end(), vchFiller.begin(), vchFiller.end());

        // These will be trimmed:
        vchPayload.push_back(0x44);
        vchPayload.push_back(0x44);
        vchPayload.push_back(0x44);

        CScript scriptPubKey;
        scriptPubKey << OP_RETURN;
        scriptPubKey << vchPayload;
        CTxOut txOut = CTxOut(0, scriptPubKey);
        txOutputs.push_back(txOut);

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), HexStr(vchFiller.begin(), vchFiller.end()));
        BOOST_CHECK_EQUAL(metaTx.getPayload().size() / 2, MAX_PACKETS * PACKET_SIZE);
    }
}

BOOST_AUTO_TEST_CASE(multiple_op_return_short)
{
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "13ZXjcDDUY3cRTPFXVfwmFR9Mz2WpnF5PP"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d6e690000111122223333");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN;
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d6e690001000200030004");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d6e69");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "13ZXjcDDUY3cRTPFXVfwmFR9Mz2WpnF5PP");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "00001111222233330001000200030004");
    }
}

BOOST_AUTO_TEST_CASE(multiple_op_return)
{
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d6e691222222222222222222222222223");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d6e694555555555555555555555555556");
            CTxOut txOut = CTxOut(5, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d6e69788888888889");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey; // has no marker and will be ignored!
            scriptPubKey << OP_RETURN << ParseHex("4d756c686f6c6c616e64204472697665");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d6e69ffff11111111111111111111"
                    "11111111111111111111111111111111111111111111111111111111111111"
                    "11111111111111111111111111111111111111111111111111111111111111"
                    "111111111111111111111111111111111111111111111117");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "12222222222222222222222222234555555"
                "555555555555555555556788888888889ffff11111111111111111111111111111"
                "111111111111111111111111111111111111111111111111111111111111111111"
                "111111111111111111111111111111111111111111111111111111111111111111"
                "1111111111111111111111111111117");
    }
}

BOOST_AUTO_TEST_CASE(multiple_op_return_pushes)
{
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));
        txInputs.push_back(PayToBareMultisig_3of5());

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(PayToScriptHash_Unrelated());
        txOutputs.push_back(OpReturn_MultiSimpleSend());

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(),
                // OpReturn_SimpleSend (without marker):
                "00000000000000070000000006dac2c0"
                // OpReturn_MultiSimpleSend (without marker):
                "00000000000000070000000000002329"
                "0062e907b15cbf27d5425399ebf6f0fb50ebb88f18"
                "000000000000001f0000000001406f40"
                "05da59767e81f4b019fe9f5984dbaa4f61bf197967");
    }
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN;
            scriptPubKey << ParseHex("6f6d6e6900000000000000010000000006dac2c0");
            scriptPubKey << ParseHex("00000000000000030000000000000d48");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(),
                "00000000000000010000000006dac2c000000000000000030000000000000d48");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
          CScript scriptPubKey;
          scriptPubKey << OP_RETURN;
          scriptPubKey << ParseHex("6f6d6e69");
          scriptPubKey << ParseHex("00000000000000010000000006dac2c0");
          CTxOut txOut = CTxOut(0, scriptPubKey);
          txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "00000000000000010000000006dac2c0");
    }
    {
        /**
         * The following transaction is invalid, because the first pushed data
         * doesn't contain the class C marker.
         */
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN;
            scriptPubKey << ParseHex("6f6d");
            scriptPubKey << ParseHex("6e69");
            scriptPubKey << ParseHex("00000000000000010000000006dac2c0");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) != 0);
    }
}


BOOST_AUTO_TEST_SUITE_END()
