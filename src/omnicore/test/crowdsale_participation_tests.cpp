#include "omnicore/sp.h"

#include <stdint.h>
#include <utility>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(omnicore_crowdsale_participation_tests)

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
    int8_t earlyBirdBonus = 6;
    int8_t issuerBonus = 10;

    //
    // txid: 8fbd96005aba5671daf8288f89df8026a7ce4782a0bb411937537933956b827b
    //
    int64_t timestamp = 1407877014LL;
    int64_t amountInvested = 3000000000LL;

    int64_t totalTokens = 0;
    std::pair<int64_t, int64_t> tokensCreated;
    bool fClosed = false;

    mastercore::calculateFundraiser(amountInvested, earlyBirdBonus, deadline,
            timestamp, amountPerUnitInvested, issuerBonus, totalTokens,
            tokensCreated, fClosed);

    BOOST_CHECK(fClosed);
    BOOST_CHECK_EQUAL(8384883669867978007LL, tokensCreated.first); // user
    BOOST_CHECK_EQUAL(838488366986797800LL, tokensCreated.second); // issuer
}


BOOST_AUTO_TEST_SUITE_END()
