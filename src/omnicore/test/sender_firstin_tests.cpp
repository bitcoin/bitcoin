#include "omnicore/omnicore.h"
#include "omnicore/script.h"
#include "omnicore/tx.h"
#include "omnicore/encoding.h"
#include "omnicore/createpayload.h"

#include "base58.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "random.h"
#include "script/script.h"
#include "script/standard.h"
#include "utilstrencodings.h"

#include <stdint.h>
#include <algorithm>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(omnicore_sender_firstin_tests)

static CTxOut PayToPubKey_Unrelated();
static CTxOut PayToBareMultisig_1of3();
static CTxOut NonStandardOutput();

/** Creates a dummy class C transaction with the given inputs. */
static CTransaction TxClassC(const std::vector<CTxOut>& txInputs)
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

    // Outputs:
    std::vector<std::pair<CScript, int64_t> > txOutputs;
    std::vector<unsigned char> vchPayload = CreatePayload_SimpleSend(1, 1000);
    BOOST_CHECK(OmniCore_Encode_ClassC(vchPayload, txOutputs));

    for (std::vector<std::pair<CScript, int64_t> >::const_iterator it = txOutputs.begin(); it != txOutputs.end(); ++it)
    {
        CTxOut txOut(it->second, it->first);
        mutableTx.vout.push_back(txOut);
    }

    return CTransaction(mutableTx);
}

/** Helper to create a CTxOut object. */
static CTxOut createTxOut(int64_t amount, const std::string& dest)
{
    return CTxOut(amount, GetScriptForDestination(CBitcoinAddress(dest).Get()));
}

/** Extracts the "first" sender. */
static bool GetFirstSender(const std::vector<CTxOut>& txInputs, std::string& strSender)
{
    int nBlock = std::numeric_limits<int>().max();

    CMPTransaction metaTx;
    CTransaction dummyTx = TxClassC(txInputs);

    if (ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0) {
        strSender = metaTx.getSender();
        return true;
    }

    return false;
}

BOOST_AUTO_TEST_CASE(first_vin_is_sender)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(100, "1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB")); // Winner
    vouts.push_back(createTxOut(999, "3KYen723gbjhJU69j8jRhZU6fDw8iVVWKy"));
    vouts.push_back(createTxOut(200, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));

    std::string strExpected("1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB");

    std::string strSender;
    BOOST_CHECK(GetFirstSender(vouts, strSender));
    BOOST_CHECK_EQUAL(strExpected, strSender);
}

BOOST_AUTO_TEST_CASE(less_input_restrictions)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(555, "3CD1QW6fjgTwKq3Pj97nty28WZAVkziNom")); // Winner
    vouts.push_back(PayToPubKey_Unrelated());
    vouts.push_back(PayToBareMultisig_1of3());
    vouts.push_back(NonStandardOutput());

    std::string strExpected("3CD1QW6fjgTwKq3Pj97nty28WZAVkziNom");

    std::string strSender;
    BOOST_CHECK(GetFirstSender(vouts, strSender));
    BOOST_CHECK_EQUAL(strExpected, strSender);
}

BOOST_AUTO_TEST_CASE(invalid_inputs)
{
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(PayToPubKey_Unrelated());
        std::string strSender;
        BOOST_CHECK(!GetFirstSender(vouts, strSender));
    }
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(PayToBareMultisig_1of3());
        std::string strSender;
        BOOST_CHECK(!GetFirstSender(vouts, strSender));
    }
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(NonStandardOutput());
        std::string strSender;
        BOOST_CHECK(!GetFirstSender(vouts, strSender));
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

static CTxOut NonStandardOutput()
{
    CScript scriptPubKey;
    scriptPubKey << ParseHex("decafbad") << OP_DROP << OP_TRUE;
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}


BOOST_AUTO_TEST_SUITE_END()
