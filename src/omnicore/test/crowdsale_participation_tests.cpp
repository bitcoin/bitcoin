#include "omnicore/sp.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <limits>
#include <utility>

BOOST_FIXTURE_TEST_SUITE(omnicore_crowdsale_participation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(overpayment_close)
{
    //
    // txid: eda3d2bbb8125397f4d4909ea25d845dc451e8a3206035bf0d736bb3ece5d758
    //
    // http://builder.bitwatch.co/?version=1&type=51&ecosystem=2&property_type=2&
    // currency_identifier=0&text=436f696e436f696e00&text=436f696e436f696e00&
    // text=43726f7764436f696e00&text=687474703a2f2f616c6c746865636f696e2e757300&
    // text=69646b00&currency_identifier=2&number_of_coins=3133700000000&
    // timestamp=1407064860000&percentage=6&percentage=10
    // 
    int64_t amountPerUnitInvested = 3133700000000LL;
    int64_t deadline = 1407064860000LL;
    uint8_t earlyBirdBonus = 6;
    uint8_t issuerBonus = 10;

    //
    // txid: 8fbd96005aba5671daf8288f89df8026a7ce4782a0bb411937537933956b827b
    //
    int64_t timestamp = 1407877014LL;
    int64_t amountInvested = 3000000000LL;

    int64_t totalTokens = 0;
    std::pair<int64_t, int64_t> tokensCreated;
    bool fClosed = false;

    mastercore::calculateFundraiser(true, amountInvested, earlyBirdBonus, deadline,
            timestamp, amountPerUnitInvested, issuerBonus, totalTokens,
            tokensCreated, fClosed);

    BOOST_CHECK(fClosed);
    BOOST_CHECK_EQUAL(8384883669867978007LL, tokensCreated.first); // user
    BOOST_CHECK_EQUAL(838488366986797800LL, tokensCreated.second); // issuer
}

BOOST_AUTO_TEST_CASE(max_limits)
{
    int64_t amountPerUnitInvested = std::numeric_limits<int64_t>::max();
    int64_t deadline = std::numeric_limits<int64_t>::max();
    uint8_t earlyBirdBonus = std::numeric_limits<uint8_t>::max();
    uint8_t issuerBonus = std::numeric_limits<uint8_t>::max();

    int64_t timestamp = 0;
    int64_t amountInvested = std::numeric_limits<int64_t>::max();

    int64_t totalTokens = std::numeric_limits<int64_t>::max() - 1LL;
    std::pair<int64_t, int64_t> tokensCreated;
    bool fClosed = false;

    mastercore::calculateFundraiser(true, amountInvested, earlyBirdBonus, deadline,
            timestamp, amountPerUnitInvested, issuerBonus, totalTokens,
            tokensCreated, fClosed);

    BOOST_CHECK(fClosed);
    BOOST_CHECK_EQUAL(1, tokensCreated.first);  // user
    BOOST_CHECK_EQUAL(0, tokensCreated.second); // issuer
}

BOOST_AUTO_TEST_CASE(negative_time)
{
    int64_t amountPerUnitInvested = 50;
    int64_t deadline = 500000000;
    uint8_t earlyBirdBonus = 255;
    uint8_t issuerBonus = 19;

    int64_t timestamp = 500007119;
    int64_t amountInvested = 1000000000L;

    int64_t totalTokens = 0;
    std::pair<int64_t, int64_t> tokensCreated;
    bool fClosed = false;

    mastercore::calculateFundraiser(false, amountInvested, earlyBirdBonus, deadline,
            timestamp, amountPerUnitInvested, issuerBonus, totalTokens,
            tokensCreated, fClosed);

    BOOST_CHECK(!fClosed);
    BOOST_CHECK_EQUAL(500, tokensCreated.first); // user
    BOOST_CHECK_EQUAL(95, tokensCreated.second); // issuer
}

BOOST_AUTO_TEST_SUITE_END()
