// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <protocol.h>
#include <script/standard.h>
#include <spork.h>
#include <timedata.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>
#include <utility>

BOOST_FIXTURE_TEST_SUITE(spork_tests, RegTestingSetup)

BOOST_AUTO_TEST_CASE(extreme_timestamps_are_handled_without_overflow)
{
    CSporkManager sporkman;
    CKey key;
    key.MakeNewKey(/*fCompressed=*/true);
    BOOST_REQUIRE(sporkman.SetSporkAddress(EncodeDestination(PKHash{key.GetPubKey()})));

    CSporkMessage future_spork;
    future_spork.nTimeSigned = std::numeric_limits<int64_t>::max();
    BOOST_REQUIRE(future_spork.Sign(key));
    BOOST_CHECK(!sporkman.IsValidSpork(future_spork));

    CSporkMessage inactive_spork{SPORK_2_INSTANTSEND_ENABLED, std::numeric_limits<int64_t>::max(),
                                 GetAdjustedTime()};
    BOOST_REQUIRE(inactive_spork.Sign(key));
    BOOST_REQUIRE(sporkman.IsValidSpork(inactive_spork));
    BOOST_REQUIRE(sporkman.ProcessSpork(inactive_spork));
    BOOST_CHECK(!sporkman.IsSporkActive(SPORK_2_INSTANTSEND_ENABLED));
}

BOOST_AUTO_TEST_CASE(sporks_signed_by_an_unknown_key_are_rejected)
{
    CSporkManager sporkman;
    CKey spork_key;
    spork_key.MakeNewKey(/*fCompressed=*/true);
    BOOST_REQUIRE(sporkman.SetSporkAddress(EncodeDestination(PKHash{spork_key.GetPubKey()})));

    CKey other_key;
    other_key.MakeNewKey(/*fCompressed=*/true);

    CSporkMessage spork{SPORK_2_INSTANTSEND_ENABLED, 1, GetAdjustedTime()};
    BOOST_REQUIRE(spork.Sign(other_key));
    BOOST_CHECK(!sporkman.IsValidSpork(spork));

    BOOST_REQUIRE(spork.Sign(spork_key));
    BOOST_CHECK(sporkman.IsValidSpork(spork));
}

BOOST_AUTO_TEST_CASE(check_and_remove_drops_sporks_not_signed_by_the_spork_key)
{
    CSporkManager sporkman;
    CKey key;
    key.MakeNewKey(/*fCompressed=*/true);
    BOOST_REQUIRE(sporkman.SetSporkAddress(EncodeDestination(PKHash{key.GetPubKey()})));

    const SporkValue default_value{sporkman.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED)};
    BOOST_REQUIRE(default_value != 1);

    CSporkMessage spork{SPORK_2_INSTANTSEND_ENABLED, 1, GetAdjustedTime()};
    BOOST_REQUIRE(spork.Sign(key));
    BOOST_REQUIRE(sporkman.ProcessSpork(spork));
    BOOST_REQUIRE_EQUAL(sporkman.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED), 1);
    BOOST_REQUIRE(sporkman.GetSporkByHash(spork.GetHash()).has_value());

    // Mimic a restart with a spork address the cached sporks were not signed by.
    CKey other_key;
    other_key.MakeNewKey(/*fCompressed=*/true);
    BOOST_REQUIRE(sporkman.SetSporkAddress(EncodeDestination(PKHash{other_key.GetPubKey()})));
    sporkman.CheckAndRemove();

    BOOST_CHECK_EQUAL(sporkman.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED), default_value);
    BOOST_CHECK(!sporkman.GetSporkByHash(spork.GetHash()).has_value());
}

BOOST_AUTO_TEST_CASE(rapid_updates_are_accepted_by_peers)
{
    CSporkManager signer;
    CSporkManager peer;
    CKey key;
    key.MakeNewKey(/*fCompressed=*/true);
    const auto address = EncodeDestination(PKHash{key.GetPubKey()});
    BOOST_REQUIRE(signer.SetSporkAddress(address));
    BOOST_REQUIRE(signer.SetPrivKey(EncodeSecret(key)));
    BOOST_REQUIRE(peer.SetSporkAddress(address));

    constexpr int64_t now{1791370200};
    const std::pair<int64_t, int64_t> times[] = {
        {now, now},
        {now, now + 1},
        {now - 10, now + 2},
        {now + 100, now + 100},
    };
    SporkValue value{0};
    for (const auto& [mock_time, signed_time] : times) {
        SetMockTime(mock_time);
        ++value;
        const auto inv = signer.UpdateSpork(SPORK_2_INSTANTSEND_ENABLED, value);
        BOOST_REQUIRE(inv.has_value());
        const auto spork = signer.GetSporkByHash(inv->hash);
        BOOST_REQUIRE(spork.has_value());
        BOOST_CHECK_EQUAL(spork->nTimeSigned, signed_time);
        BOOST_REQUIRE(peer.IsValidSpork(*spork));
        BOOST_CHECK(peer.ProcessSpork(*spork));
        BOOST_CHECK_EQUAL(peer.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED), value);
        BOOST_CHECK_EQUAL(signer.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED), value);
        BOOST_CHECK(!peer.ProcessSpork(*spork));
    }

    // a successor to the active spork would be too far in the future for peers to accept
    SetMockTime(now - 3 * 60 * 60);
    BOOST_CHECK(!signer.UpdateSpork(SPORK_2_INSTANTSEND_ENABLED, value + 1).has_value());
    BOOST_CHECK_EQUAL(signer.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED), value);
}

BOOST_AUTO_TEST_SUITE_END()
