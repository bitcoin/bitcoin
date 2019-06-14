#include "omnicore/test/utils_tx.h"

#include "omnicore/createpayload.h"
#include "omnicore/encoding.h"
#include "omnicore/omnicore.h"
#include "omnicore/parsing.h"
#include "omnicore/script.h"
#include "omnicore/tx.h"

#include "base58.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"

#include <stdint.h>
#include <algorithm>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_parsing_b_tests, BasicTestingSetup)

/** Creates a dummy transaction with the given inputs and outputs. */
static CTransaction TxClassB(const std::vector<CTxOut>& txInputs, const std::vector<CTxOut>& txOuts)
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

/** Helper to determine hex-encoded payload size. */
static size_t getPayloadSize(unsigned int nPackets)
{
    // multiply by 2 for hex conversion
    // subtract 1 byte for sequence number
    return 2 * (PACKET_SIZE - 1) * nPackets;
}

BOOST_AUTO_TEST_CASE(valid_common_class_b)
{
    int nBlock = 0;

    std::vector<CTxOut> txInputs;
    txInputs.push_back(createTxOut(1000000, "1BxtgEa8UcrMzVZaW32zVyJh4Sg4KGFzxA"));
    txInputs.push_back(createTxOut(1000000, "1HG3s4Ext3sTqBTHrgftyUzG3cvx5ZbPCj"));
    txInputs.push_back(createTxOut(2000001, "1Pa6zyqnhL6LDJtrkCMi9XmEDNHJ23ffEr"));

    std::vector<CTxOut> txOutputs;
    txOutputs.push_back(PayToPubKeyHash_Exodus());
    txOutputs.push_back(PayToBareMultisig_1of3());
    txOutputs.push_back(PayToBareMultisig_3of5());
    txOutputs.push_back(PayToBareMultisig_3of5());
    txOutputs.push_back(PayToPubKeyHash_Unrelated());

    CTransaction dummyTx = TxClassB(txInputs, txOutputs);

    CMPTransaction metaTx;
    BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
    BOOST_CHECK_EQUAL(metaTx.getSender(), "1Pa6zyqnhL6LDJtrkCMi9XmEDNHJ23ffEr");
    BOOST_CHECK_EQUAL(metaTx.getPayload().size(), getPayloadSize(10));
}

BOOST_AUTO_TEST_CASE(valid_arbitrary_output_number_class_b)
{
    int nBlock = std::numeric_limits<int>::max();

    int nOutputs = 3000 * 8; // due to the junk

    std::vector<CTxOut> txInputs;
    txInputs.push_back(createTxOut(5550000, "3QHw8qKf1vQkMnSVXarq7N4PYzz1G3mAK4"));

    std::vector<CTxOut> txOutputs;
    for (int i = 0; i < nOutputs / 8; ++i) {
        txOutputs.push_back(PayToBareMultisig_1of2());
        txOutputs.push_back(PayToBareMultisig_1of3());
        txOutputs.push_back(PayToBareMultisig_3of5());
        txOutputs.push_back(OpReturn_Unrelated());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(PayToScriptHash_Unrelated());
        txOutputs.push_back(PayToPubKeyHash_Exodus());
    }

    std::random_shuffle(txOutputs.begin(), txOutputs.end());

    CTransaction dummyTx = TxClassB(txInputs, txOutputs);
    BOOST_CHECK_EQUAL(dummyTx.vout.size(), nOutputs);

    CMPTransaction metaTx;
    BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
    BOOST_CHECK_EQUAL(metaTx.getSender(), "3QHw8qKf1vQkMnSVXarq7N4PYzz1G3mAK4");
    BOOST_CHECK_EQUAL(metaTx.getPayload().size(), getPayloadSize(MAX_PACKETS));
}


BOOST_AUTO_TEST_SUITE_END()
