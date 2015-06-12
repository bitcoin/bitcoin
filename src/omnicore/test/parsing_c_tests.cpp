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

BOOST_AUTO_TEST_SUITE(omnicore_parsing_c_tests)

static CTxOut PayToPubKeyHash_Exodus();
static CTxOut PayToPubKey_Unrelated();
static CTxOut NonStandardOutput();
static CTxOut PayToBareMultisig_1of3();
static CTxOut OpReturn_SimpleSend();
static CTxOut OpReturn_Unrelated();

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
        int nBlock = OP_RETURN_BLOCK;

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
        int nBlock = OP_RETURN_BLOCK + 1000;

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
        int nBlock = std::numeric_limits<int>().max();

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
        int nBlock = std::numeric_limits<int>().max();

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
        int nBlock = std::numeric_limits<int>().max();

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

BOOST_AUTO_TEST_CASE(op_return_payload_too_small)
{
    {
        int nBlock = OP_RETURN_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "1MV8MySYueghUDMsjNV67CfeRXmQZAxjSW"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d00000304");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) != 0);
    }
}

BOOST_AUTO_TEST_CASE(op_return_payload_min_size)
{
    {
        int nBlock = OP_RETURN_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "1KsXJ5XuoXHHkevNm3bLpqWPP4PtGEjfuE"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6dffffffff05");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "1KsXJ5XuoXHHkevNm3bLpqWPP4PtGEjfuE");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "ffffffff05");
    }
}

BOOST_AUTO_TEST_CASE(multiple_op_return_short)
{
    {
        int nBlock = OP_RETURN_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "13ZXjcDDUY3cRTPFXVfwmFR9Mz2WpnF5PP"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d0000111122223333");
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
            scriptPubKey << OP_RETURN << ParseHex("6f6d0001000200030004");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d");
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
        int nBlock = OP_RETURN_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d1222222222222222222222222223");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d4555555555555555555555555556");
            CTxOut txOut = CTxOut(5, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6d788888888889");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey; // has no marker!
            scriptPubKey << OP_RETURN << ParseHex("4d756c686f6c6c616e64204472697665");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6dffff111111111111111111111111"
                    "11111111111111111111111111111111111111111111111111111111111111"
                    "11111111111111111111111111111111111111111111111111111111111111"
                    "11111111111111111111111111111111111111111117");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassC(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "12222222222222222222222222234555555"
                "5555555555555555555567888888888896c686f6c6c616e64204472697665ffff1"
                "111111111111111111111111111111111111111111111111111111111111111111"
                "111111111111111111111111111111111111111111111111111111111111111111"
                "11111111111111111111111111111111111111111111111111111111117");
    }
}

static CTxOut PayToPubKeyHash_Exodus()
{
    CBitcoinAddress address = ExodusAddress();
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
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

static CTxOut PayToBareMultisig_1of3()
{
    std::vector<unsigned char> vchPayload1 = ParseHex(
        "04ad90e5b6bc86b3ec7fac2c5fbda7423fc8ef0d58df594c773fa05e2c281b2bfe"
        "877677c668bd13603944e34f4818ee03cadd81a88542b8b4d5431264180e2c28");
    std::vector<unsigned char> vchPayload2 = ParseHex(
        "026766a63686d2cc5d82c929d339b7975010872aa6bf76f6fac69f28f8e293a914");
    std::vector<unsigned char> vchPayload3 = ParseHex(
        "02959b8e2f2e4fb67952cda291b467a1781641c94c37feaa0733a12782977da23b");

    CPubKey pubKey1(vchPayload1.begin(), vchPayload1.end());
    CPubKey pubKey2(vchPayload2.begin(), vchPayload2.end());
    CPubKey pubKey3(vchPayload3.begin(), vchPayload3.end());

    // 1-of-3 bare multisig script
    CScript scriptPubKey;
    scriptPubKey << CScript::EncodeOP_N(1);
    scriptPubKey << ToByteVector(pubKey1);
    scriptPubKey << ToByteVector(pubKey2);
    scriptPubKey << ToByteVector(pubKey3);
    scriptPubKey << CScript::EncodeOP_N(3);
    scriptPubKey << OP_CHECKMULTISIG;

    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut OpReturn_SimpleSend()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("6f6d00000000000000070000000006dac2c0");

    return CTxOut(0, scriptPubKey);
}

static CTxOut OpReturn_Unrelated()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("7468697320697320646578782028323031352d30362d313229");

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


