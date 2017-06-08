#include "omnicore/consensushash.h"
#include "omnicore/dex.h"
#include "omnicore/mdex.h"
#include "omnicore/sp.h"
#include "omnicore/omnicore.h"
#include "omnicore/rules.h"
#include "omnicore/tally.h"

#include "arith_uint256.h"
#include "sync.h"
#include "test/test_bitcoin.h"
#include "uint256.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <string>

namespace mastercore
{
extern std::string GenerateConsensusString(const CMPTally& tallyObj, const std::string& address, const uint32_t propertyId); // done
extern std::string GenerateConsensusString(const CMPOffer& offerObj, const std::string& address); // half
extern std::string GenerateConsensusString(const CMPAccept& acceptObj, const std::string& address);
extern std::string GenerateConsensusString(const CMPMetaDEx& tradeObj);
extern std::string GenerateConsensusString(const CMPCrowd& crowdObj);
extern std::string GenerateConsensusString(const uint32_t propertyId, const std::string& address);
}

extern void clear_all_state();

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_checkpoint_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(consensus_string_tally)
{
    CMPTally tally;
    BOOST_CHECK_EQUAL("", GenerateConsensusString(tally, "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 1));
    BOOST_CHECK_EQUAL("", GenerateConsensusString(tally, "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 3));

    BOOST_CHECK(tally.updateMoney(3, 7, BALANCE));
    BOOST_CHECK_EQUAL("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b|3|7|0|0|0",
            GenerateConsensusString(tally, "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 3));

    BOOST_CHECK(tally.updateMoney(3, 7, BALANCE));
    BOOST_CHECK(tally.updateMoney(3, 100, SELLOFFER_RESERVE));
    BOOST_CHECK(tally.updateMoney(3, int64_t(9223372036854775807LL), ACCEPT_RESERVE));
    BOOST_CHECK(tally.updateMoney(3, (-int64_t(9223372036854775807LL)-1), PENDING)); // ignored
    BOOST_CHECK(tally.updateMoney(3, int64_t(4294967296L), METADEX_RESERVE));
    BOOST_CHECK_EQUAL("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b|3|14|100|9223372036854775807|4294967296",
            GenerateConsensusString(tally, "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 3));
}

BOOST_AUTO_TEST_CASE(consensus_string_offer)
{
    CMPOffer offerA;
    BOOST_CHECK_EQUAL("0000000000000000000000000000000000000000000000000000000000000000|3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b|0|0|0|0|0",
            GenerateConsensusString(offerA, "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b"));

    CMPOffer offerB(340000, 3000, 2, 100000, 1000, 10, uint256S("3c9a055899147b03b2c5240a020c1f94d243a834ecc06ab8cfa504ee29d07b7d"));
    BOOST_CHECK_EQUAL("3c9a055899147b03b2c5240a020c1f94d243a834ecc06ab8cfa504ee29d07b7d|1HG3s4Ext3sTqBTHrgftyUzG3cvx5ZbPCj|2|3000|100000|1000|10",
            GenerateConsensusString(offerB, "1HG3s4Ext3sTqBTHrgftyUzG3cvx5ZbPCj"));
}

BOOST_AUTO_TEST_CASE(consensus_string_accept)
{
    CMPAccept accept(1234, 1000, 350000, 10, 2, 2000, 4000, uint256S("2c9a055899147b03b2c5240a020c1f94d243a834ecc06ab8cfa504ee29d07b7b"));
    BOOST_CHECK_EQUAL("2c9a055899147b03b2c5240a020c1f94d243a834ecc06ab8cfa504ee29d07b7b|3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b|1234|1000|350000",
            GenerateConsensusString(accept, "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b"));
}

BOOST_AUTO_TEST_CASE(consensus_string_mdex)
{
    CMPMetaDEx tradeA;
    BOOST_CHECK_EQUAL("0000000000000000000000000000000000000000000000000000000000000000||0|0|0|0|0",
            GenerateConsensusString(tradeA));

    CMPMetaDEx tradeB("1PxejjeWZc9ZHph7A3SYDo2sk2Up4AcysH", 395000, 31, 1000000, 1, 2000000,
            uint256S("2c9a055899147b03b2c5240a020c1f94d243a834ecc06ab8cfa504ee29d07b7d"), 1, 1, 900000);
    BOOST_CHECK_EQUAL("2c9a055899147b03b2c5240a020c1f94d243a834ecc06ab8cfa504ee29d07b7d|1PxejjeWZc9ZHph7A3SYDo2sk2Up4AcysH|31|1000000|1|2000000|900000",
            GenerateConsensusString(tradeB));
}

BOOST_AUTO_TEST_CASE(consensus_string_crowdsale)
{
    CMPCrowd crowdsaleA;
    BOOST_CHECK_EQUAL("0|0|0|0|0",
            GenerateConsensusString(crowdsaleA));

    CMPCrowd crowdsaleB(77, 500000, 3, 1514764800, 10, 255, 10000, 25500);
    BOOST_CHECK_EQUAL("77|3|1514764800|10000|25500",
            GenerateConsensusString(crowdsaleB));
}

BOOST_AUTO_TEST_CASE(consensus_string_property_issuer)
{
    BOOST_CHECK_EQUAL("5|3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b",
            GenerateConsensusString(5, "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b"));
}

BOOST_AUTO_TEST_CASE(get_checkpoints)
{
    // There are consensus checkpoints for mainnet:
    BOOST_CHECK(!ConsensusParams("main").GetCheckpoints().empty());
    // ... but no checkpoints for regtest mode are defined:
    BOOST_CHECK(ConsensusParams("regtest").GetCheckpoints().empty());
}


BOOST_AUTO_TEST_SUITE_END()
