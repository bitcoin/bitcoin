#include "omnicore/omnicore.h"

#include "base58.h"
#include "chainparams.h"

#include <limits>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(omnicore_exodus_tests)

BOOST_AUTO_TEST_CASE(exodus_address_mainnet)
{
    SelectParams(CBaseChainParams::MAIN);

    BOOST_CHECK(CBitcoinAddress("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P") ==
                ExodusAddress());
    BOOST_CHECK(!(CBitcoinAddress("1rDQWR9yZLJY7ciyghAaF7XKD9tGzQuP6") ==
                ExodusAddress()));

    SelectParams(CBaseChainParams::UNITTEST);
}

BOOST_AUTO_TEST_CASE(exodus_crowdsale_address_mainnet)
{
    SelectParams(CBaseChainParams::MAIN);

    BOOST_CHECK(CBitcoinAddress("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P") ==
                ExodusCrowdsaleAddress(0));
    BOOST_CHECK(CBitcoinAddress("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>().max()));
    BOOST_CHECK(!(CBitcoinAddress("1rDQWR9yZLJY7ciyghAaF7XKD9tGzQuP6") ==
                ExodusCrowdsaleAddress(0)));
    BOOST_CHECK(!(CBitcoinAddress("1rDQWR9yZLJY7ciyghAaF7XKD9tGzQuP6") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>().max())));

    SelectParams(CBaseChainParams::UNITTEST);
}

BOOST_AUTO_TEST_CASE(exodus_address_testnet)
{
    SelectParams(CBaseChainParams::TESTNET);

    BOOST_CHECK(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusAddress());
    BOOST_CHECK(!(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusAddress()));

    SelectParams(CBaseChainParams::UNITTEST);
}

BOOST_AUTO_TEST_CASE(exodus_address_regtest)
{
    SelectParams(CBaseChainParams::REGTEST);

    BOOST_CHECK(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusAddress());
    BOOST_CHECK(!(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusAddress()));

    SelectParams(CBaseChainParams::UNITTEST);
}

BOOST_AUTO_TEST_CASE(exodus_crowdsale_address_testnet)
{
    SelectParams(CBaseChainParams::TESTNET);

    BOOST_CHECK(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(0));
    BOOST_CHECK(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK-1));
    BOOST_CHECK(!(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(0)));
    BOOST_CHECK(!(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK-1)));
    BOOST_CHECK(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK));
    BOOST_CHECK(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>().max()));
    BOOST_CHECK(!(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(MONEYMAN_TESTNET_BLOCK)));
    BOOST_CHECK(!(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>().max())));

    SelectParams(CBaseChainParams::UNITTEST);
}

BOOST_AUTO_TEST_CASE(exodus_crowdsale_address_regtest)
{
    SelectParams(CBaseChainParams::REGTEST);

    BOOST_CHECK(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(0));
    BOOST_CHECK(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK-1));
    BOOST_CHECK(!(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(0)));
    BOOST_CHECK(!(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK-1)));
    BOOST_CHECK(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK));
    BOOST_CHECK(CBitcoinAddress("moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>().max()));
    BOOST_CHECK(!(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(MONEYMAN_REGTEST_BLOCK)));
    BOOST_CHECK(!(CBitcoinAddress("mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv") ==
                ExodusCrowdsaleAddress(std::numeric_limits<int>().max())));

    SelectParams(CBaseChainParams::UNITTEST);
}


BOOST_AUTO_TEST_SUITE_END()
