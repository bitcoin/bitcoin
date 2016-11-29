#include "omnicore/test/utils_tx.h"

#include "omnicore/omnicore.h"
#include "omnicore/script.h"
#include "omnicore/tx.h"

#include "base58.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "random.h"
#include "script/script.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"

#include <stdint.h>
#include <algorithm>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_sender_bycontribution_tests, BasicTestingSetup)

// Forward declarations
static CTransaction TxClassB(const std::vector<CTxOut>& txInputs);
static bool GetSenderByContribution(const std::vector<CTxOut>& vouts, std::string& strSender);
static CTxOut createTxOut(int64_t amount, const std::string& dest);
static CKeyID createRandomKeyId();
static CScriptID createRandomScriptId();
void shuffleAndCheck(std::vector<CTxOut>& vouts, unsigned nRounds);

// Test settings
static const unsigned nOutputs = 256;
static const unsigned nAllRounds = 2;
static const unsigned nShuffleRounds = 16;

/**
 * Tests the invalidation of the transaction, when there are not allowed inputs.
 */
BOOST_AUTO_TEST_CASE(invalid_inputs)
{
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(PayToPubKey_Unrelated());
        vouts.push_back(PayToPubKeyHash_Unrelated());
        std::string strSender;
        BOOST_CHECK(!GetSenderByContribution(vouts, strSender));
    }
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(PayToPubKeyHash_Unrelated());
        vouts.push_back(PayToBareMultisig_1of3());
        std::string strSender;
        BOOST_CHECK(!GetSenderByContribution(vouts, strSender));
    }
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(PayToScriptHash_Unrelated());
        vouts.push_back(PayToPubKeyHash_Exodus());
        vouts.push_back(NonStandardOutput());
        std::string strSender;
        BOOST_CHECK(!GetSenderByContribution(vouts, strSender));
    }
}

/**
 * Tests sender selection "by sum" with pay-to-pubkey-hash outputs, where a single
 * candidate has the highest output value.
 */
BOOST_AUTO_TEST_CASE(p2pkh_contribution_by_sum_test)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(100, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(100, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(100, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(100, "168F7az8ACWxntNqL1FathoKTbgK17GVft"));
    vouts.push_back(createTxOut(100, "168F7az8ACWxntNqL1FathoKTbgK17GVft"));
    vouts.push_back(createTxOut(999, "16X6UDz6dMkVAAkWdY6HKe85o6EVAbzDtn")); // Winner
    vouts.push_back(createTxOut(100, "1BYk8d1fWy1JLcNqHyNTEg2Jxu9EDb1BmY"));
    vouts.push_back(createTxOut(100, "1BYk8d1fWy1JLcNqHyNTEg2Jxu9EDb1BmY"));
    vouts.push_back(createTxOut(100, "1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB"));

    std::string strExpected("16X6UDz6dMkVAAkWdY6HKe85o6EVAbzDtn");

    for (int i = 0; i < 10; ++i) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strExpected, strSender);
    }
}

/**
 * Tests sender selection "by sum" with pay-to-pubkey-hash outputs, where a candidate
 * with the highest output value by sum, with more than one output, is chosen.
 */
BOOST_AUTO_TEST_CASE(p2pkh_contribution_by_total_sum_test)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(499, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(501, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(295, "1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB")); // Winner
    vouts.push_back(createTxOut(310, "1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB")); // Winner
    vouts.push_back(createTxOut(400, "1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB")); // Winner
    vouts.push_back(createTxOut(500, "1Pr75FNvtoWHeocNfc4zTQCfK5kMVakWcn"));
    vouts.push_back(createTxOut(500, "1Pr75FNvtoWHeocNfc4zTQCfK5kMVakWcn"));

    std::string strExpected("1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB");

    for (int i = 0; i < 10; ++i) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strExpected, strSender);
    }
}

/**
 * Tests sender selection "by sum" with pay-to-pubkey-hash outputs, where all outputs
 * have equal values, and a candidate is chosen based on the lexicographical order of
 * the base58 string representation (!) of the candidate.
 *
 * Note: it reflects the behavior of Omni Core, but this edge case is not specified.
 */
BOOST_AUTO_TEST_CASE(p2pkh_contribution_by_sum_order_test)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(1000, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe")); // Winner
    vouts.push_back(createTxOut(1000, "168F7az8ACWxntNqL1FathoKTbgK17GVft"));
    vouts.push_back(createTxOut(1000, "16X6UDz6dMkVAAkWdY6HKe85o6EVAbzDtn"));
    vouts.push_back(createTxOut(1000, "1BYk8d1fWy1JLcNqHyNTEg2Jxu9EDb1BmY"));
    vouts.push_back(createTxOut(1000, "1CE8bBr1dYZRMnpmyYsFEoexa1YoPz2mfB"));    
    vouts.push_back(createTxOut(1000, "1HG3s4Ext3sTqBTHrgftyUzG3cvx5ZbPCj"));
    vouts.push_back(createTxOut(1000, "1K6JtSvrHtyFmxdtGZyZEF7ydytTGqasNc"));    
    vouts.push_back(createTxOut(1000, "1Pa6zyqnhL6LDJtrkCMi9XmEDNHJ23ffEr"));
    vouts.push_back(createTxOut(1000, "1Pr75FNvtoWHeocNfc4zTQCfK5kMVakWcn"));

    std::string strExpected("1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe");

    for (int i = 0; i < 10; ++i) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strExpected, strSender);
    }
}

/**
 * Tests sender selection "by sum" with pay-to-script-hash outputs, where a single
 * candidate has the highest output value.
 */
BOOST_AUTO_TEST_CASE(p2sh_contribution_by_sum_test)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(100, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(150, "32LUY4obZmFGFVtJTs4o1qk4dxo1bYq9s8"));
    vouts.push_back(createTxOut(400, "34To5z7h35gQ54212gFRJSkwJUxGREr3SZ"));
    vouts.push_back(createTxOut(100, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(400, "3HsMqdiWqaziUe9VExna46NbWBAFaioMrK"));
    vouts.push_back(createTxOut(100, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(777, "3KYen723gbjhJU69j8jRhZU6fDw8iVVWKy")); // Winner
    vouts.push_back(createTxOut(100, "3KuUrmqdM7BWfHTsyyLyYnJ57HgnKurvkK"));

    std::string strExpected("3KYen723gbjhJU69j8jRhZU6fDw8iVVWKy");

    for (int i = 0; i < 10; ++i) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strExpected, strSender);
    }
}

/**
 * Tests sender selection "by sum" with pay-to-pubkey-hash and pay-to-script-hash 
 * outputs mixed, where a candidate with the highest output value by sum, with more 
 * than one output, is chosen.
 */
BOOST_AUTO_TEST_CASE(p2sh_contribution_by_total_sum_test)
{
    std::vector<CTxOut> vouts;

    vouts.push_back(createTxOut(100, "34To5z7h35gQ54212gFRJSkwJUxGREr3SZ"));
    vouts.push_back(createTxOut(500, "34To5z7h35gQ54212gFRJSkwJUxGREr3SZ"));
    vouts.push_back(createTxOut(600, "3CD1QW6fjgTwKq3Pj97nty28WZAVkziNom")); // Winner
    vouts.push_back(createTxOut(500, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(100, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));
    vouts.push_back(createTxOut(350, "3CD1QW6fjgTwKq3Pj97nty28WZAVkziNom")); // Winner
    vouts.push_back(createTxOut(110, "1471EHpnJ62MDxLw96dKcNT8sWPEbHrAUe"));

    std::string strExpected("3CD1QW6fjgTwKq3Pj97nty28WZAVkziNom");

    for (int i = 0; i < 10; ++i) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strExpected, strSender);
    }
}

/**
 * Tests sender selection "by sum" with pay-to-script-hash outputs, where all outputs
 * have equal values, and a candidate is chosen based on the lexicographical order of
 * the base58 string representation (!) of the candidate.
 *
 * Note: it reflects the behavior of Omni Core, but this edge case is not specified.
 */
BOOST_AUTO_TEST_CASE(p2sh_contribution_by_sum_order_test)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(1000, "32LUY4obZmFGFVtJTs4o1qk4dxo1bYq9s8")); // Winner
    vouts.push_back(createTxOut(1000, "34To5z7h35gQ54212gFRJSkwJUxGREr3SZ"));
    vouts.push_back(createTxOut(1000, "3CD1QW6fjgTwKq3Pj97nty28WZAVkziNom"));
    vouts.push_back(createTxOut(1000, "3GLZeQfjqFpULBzS12TjraZziNewnCE7bd"));
    vouts.push_back(createTxOut(1000, "3HsMqdiWqaziUe9VExna46NbWBAFaioMrK"));    
    vouts.push_back(createTxOut(1000, "3KYen723gbjhJU69j8jRhZU6fDw8iVVWKy"));
    vouts.push_back(createTxOut(1000, "3KuUrmqdM7BWfHTsyyLyYnJ57HgnKurvkK"));    
    vouts.push_back(createTxOut(1000, "3NukJ6fYZJ5Kk8bPjycAnruZkE5Q7UW7i8"));
    vouts.push_back(createTxOut(1000, "3PeDornjuSraASTqyN7WDMexTogz29QnXu"));    
    
    std::string strExpected("32LUY4obZmFGFVtJTs4o1qk4dxo1bYq9s8");

    for (int i = 0; i < 10; ++i) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strExpected, strSender);
    }
}

/**
 * Tests sender selection "by sum", where the lexicographical order of the base58 
 * representation as string (instead of uint160) determines the chosen candidate.
 *
 * In practise this implies selecting the sender "by sum" via a comparison of 
 * CBitcoinAddress objects would yield faulty results.
 *
 * Note: it reflects the behavior of Omni Core, but this edge case is not specified.
 */
BOOST_AUTO_TEST_CASE(sender_selection_string_based_test)
{
    std::vector<CTxOut> vouts;
    // Hash 160: 539a7a290034e0107f27cb36cb2d1095b1768d10
    vouts.push_back(createTxOut(1000, "18d49BjkbBCoK659fuktKjB3HugxK9NbpW")); // Winner
    // Hash 160: 70161ebf72f894fbee554e88cf7889035899827f
    vouts.push_back(createTxOut(1000, "1BDfBRCA3WBuHYHcaAdy4vojgmWZQncc16"));
    // Hash 160: b94b91c9e0423f6e5279de68989dda9032d67e87
    vouts.push_back(createTxOut(1000, "1HtkY9cvSBEQugk1R3XT5wjJZuyvSyzT8W"));
    // Hash 160: 06569c8f59f428db748c94bf1d5d9f5f9d0db116
    vouts.push_back(createTxOut(1000, "1aWp2ceCw95EVBwAv8HwX4Gofx9ksthjv"));  // Not!

    std::string strExpected("18d49BjkbBCoK659fuktKjB3HugxK9NbpW");

    for (int i = 0; i < 24; ++i) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strExpected, strSender);
    }
}

/**
 * Tests order independence of the sender selection "by sum" for pay-to-pubkey-hash
 * outputs, where all output values are equal.
 */
BOOST_AUTO_TEST_CASE(sender_selection_same_amount_test)
{
    for (unsigned i = 0; i < nAllRounds; ++i) {
        std::vector<CTxOut> vouts;
        for (unsigned n = 0; n < nOutputs; ++n) {
            CTxOut output(static_cast<int64_t>(1000),
                    GetScriptForDestination(createRandomKeyId()));
            vouts.push_back(output);
        }
        shuffleAndCheck(vouts, nShuffleRounds);
    }
}

/**
 * Tests order independence of the sender selection "by sum" for pay-to-pubkey-hash
 * outputs, where output values are different for each output.
 */
BOOST_AUTO_TEST_CASE(sender_selection_increasing_amount_test)
{
    for (unsigned i = 0; i < nAllRounds; ++i) {
        std::vector<CTxOut> vouts;
        for (unsigned n = 0; n < nOutputs; ++n) {
            CTxOut output(static_cast<int64_t>(1000 + n),
                    GetScriptForDestination(createRandomKeyId()));
            vouts.push_back(output);
        }
        shuffleAndCheck(vouts, nShuffleRounds);
    }
}

/**
 * Tests order independence of the sender selection "by sum" for pay-to-pubkey-hash
 * and pay-to-script-hash outputs mixed together, where output values are equal for 
 * every second output.
 */
BOOST_AUTO_TEST_CASE(sender_selection_mixed_test)
{
    for (unsigned i = 0; i < nAllRounds; ++i) {
        std::vector<CTxOut> vouts;
        for (unsigned n = 0; n < nOutputs; ++n) {
            CScript scriptPubKey;
            if (GetRandInt(2) == 0) {
                scriptPubKey = GetScriptForDestination(createRandomKeyId());
            } else {
                scriptPubKey = GetScriptForDestination(createRandomScriptId());
            };
            int64_t nAmount = static_cast<int64_t>(1000 - n * (n % 2 == 0));
            vouts.push_back(CTxOut(nAmount, scriptPubKey));
        }
        shuffleAndCheck(vouts, nShuffleRounds);
    }
}

/** Creates a dummy class B transaction with the given inputs. */
static CTransaction TxClassB(const std::vector<CTxOut>& txInputs)
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
    mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
    mutableTx.vout.push_back(PayToBareMultisig_1of3());
    mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());

    return CTransaction(mutableTx);
}

/** Extracts the sender "by contribution". */
static bool GetSenderByContribution(const std::vector<CTxOut>& vouts, std::string& strSender)
{
    int nBlock = std::numeric_limits<int>::max();

    CMPTransaction metaTx;
    CTransaction dummyTx = TxClassB(vouts);

    if (ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0) {
        strSender = metaTx.getSender();
        return true;
    }

    return false;
}

/** Helper to create a CTxOut object. */
static CTxOut createTxOut(int64_t amount, const std::string& dest)
{
    return CTxOut(amount, GetScriptForDestination(CBitcoinAddress(dest).Get()));
}

/** Helper to create a CKeyID object with random value.*/
static CKeyID createRandomKeyId()
{
    std::vector<unsigned char> vch;
    vch.reserve(20);
    for (int i = 0; i < 20; ++i) {
        vch.push_back(static_cast<unsigned char>(GetRandInt(256)));
    }
    return CKeyID(uint160(vch));
}

/** Helper to create a CScriptID object with random value.*/
static CScriptID createRandomScriptId()
{
    std::vector<unsigned char> vch;
    vch.reserve(20);
    for (int i = 0; i < 20; ++i) {
        vch.push_back(static_cast<unsigned char>(GetRandInt(256)));
    }
    return CScriptID(uint160(vch));
}

/**
 * Identifies the sender of a transaction, based on the list of provided transaction
 * outputs, and then shuffles the list n times, while checking, if this produces the
 * same result. The "contribution by sum" sender selection doesn't require specific
 * positions or order of outputs, and should work in all cases.
 */
void shuffleAndCheck(std::vector<CTxOut>& vouts, unsigned nRounds)
{
    std::string strSenderFirst;
    BOOST_CHECK(GetSenderByContribution(vouts, strSenderFirst));

    for (unsigned j = 0; j < nRounds; ++j) {
        std::random_shuffle(vouts.begin(), vouts.end(), GetRandInt);

        std::string strSender;
        BOOST_CHECK(GetSenderByContribution(vouts, strSender));
        BOOST_CHECK_EQUAL(strSenderFirst, strSender);
    }
}


BOOST_AUTO_TEST_SUITE_END()
