#include "omnicore/omnicore.h"
#include "omnicore/rules.h"
#include "omnicore/tally.h"

#include "sync.h"
#include "uint256.h"

#include <stdint.h>
#include <string>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(omnicore_checkpoint_tests)

BOOST_AUTO_TEST_CASE(hash_generation)
{
    LOCK(cs_tally);
    mp_tally_map.clear();

    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "55b852781b9995a44c939b64e441ae2724b96f99c8f4fb9a141cfc9842c4b0e3");
    BOOST_CHECK(update_tally_map("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 1, 12345, BALANCE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "33c9eb05f8720c170d93cfcd835bd53bca879ab10990ff178ebe682fc6d6d05c");
    BOOST_CHECK(update_tally_map("1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R", 3,  3400, BALANCE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "53c82a68c4d53195853fde3f55fa86c0c29dc0f5e328dd05346d177012f4b8c4");
    BOOST_CHECK(update_tally_map("1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R", 3, -3400, BALANCE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "33c9eb05f8720c170d93cfcd835bd53bca879ab10990ff178ebe682fc6d6d05c");
    BOOST_CHECK(update_tally_map("1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb", 2, 50000, METADEX_RESERVE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "f7a031b7ab173531fa2d06a18b85d3158ccea2831097c12df2e389210c74a935");
    BOOST_CHECK(update_tally_map("1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb", 10, 1, ACCEPT_RESERVE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "19784f4446df274cca995935163694e49566c7a02e9f50ea871540129dd395af");
    BOOST_CHECK(update_tally_map("1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb", 1, 100, SELLOFFER_RESERVE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "1751f9ecd4ba1d14ad779ae8a2f33586312788573e72b1b99df65368b124fec7");
    BOOST_CHECK(update_tally_map("1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb", 1, 9223372036854775807, BALANCE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "b4dabf6d23ffe815d811b1ccee0dbf2357afaf0546300889ae1ad7d60210a27b");
    BOOST_CHECK(update_tally_map("1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb", 1, -9223372036854775807, BALANCE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "1751f9ecd4ba1d14ad779ae8a2f33586312788573e72b1b99df65368b124fec7");
    BOOST_CHECK(update_tally_map("1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R", 31, -8000, PENDING));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "1751f9ecd4ba1d14ad779ae8a2f33586312788573e72b1b99df65368b124fec7");
    BOOST_CHECK(update_tally_map("1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R", 31, 8000, BALANCE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "1321c7a7590f03c46706a1be64b9c58f9d49239fc3bdfdb0d238cb0fc2d80175");
    BOOST_CHECK(update_tally_map("1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R", 2147483657, 234235245, METADEX_RESERVE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "f02f133992121312244c355de3fbd3dd64523972e7129b77579061205e1b7dfc");
    BOOST_CHECK(update_tally_map("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 1, -50000, PENDING));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "f02f133992121312244c355de3fbd3dd64523972e7129b77579061205e1b7dfc");
    BOOST_CHECK(update_tally_map("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 1, 9000, METADEX_RESERVE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "dc2d2536556d55d872a65e11aead6475bf283cd44deaa375722e583780c34152");
    BOOST_CHECK(update_tally_map("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 1, 777, ACCEPT_RESERVE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "78bc5630403a25626b19bc6dd2ed6f93b5b84502a63d203efbd378897d56640c");
    BOOST_CHECK(update_tally_map("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 1, 5, SELLOFFER_RESERVE));
    BOOST_CHECK_EQUAL(GetConsensusHash().GetHex(), "3580e94167f75620dfa8c267862aa47af46164ed8edaec3a800d732050ec0607");

    // Cleanup
    mp_tally_map.clear();
}

BOOST_AUTO_TEST_CASE(get_checkpoints)
{
    // There are consensus checkpoints for mainnet:
    BOOST_CHECK(!ConsensusParams("main").GetCheckpoints().empty());
    // ... but no checkpoints for regtest mode are defined:
    BOOST_CHECK(ConsensusParams("regtest").GetCheckpoints().empty());
}

BOOST_AUTO_TEST_CASE(verify_checkpoints)
{
    LOCK(cs_tally);
    mp_tally_map.clear();

    // Not a checkpoint (so we assume it's valid)
    BOOST_CHECK(VerifyCheckpoint(1, uint256("00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048")));
    // Valid checkpoint
    BOOST_CHECK(VerifyCheckpoint(0, uint256("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f")));
    // Valid checkpoint, but block hash mismatch
    BOOST_CHECK(!VerifyCheckpoint(0, uint256("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce260")));

    // Pollute the state (and therefore hash)
    BOOST_CHECK(update_tally_map("3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b", 1, 12345, BALANCE));
    // Checkpoint mismatch, due to the state pollution
    BOOST_CHECK(!VerifyCheckpoint(0, uint256("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f")));

    // Cleanup
    mp_tally_map.clear();
    BOOST_CHECK(VerifyCheckpoint(0, uint256("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f")));
}


BOOST_AUTO_TEST_SUITE_END()
