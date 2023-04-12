#include <boost/test/unit_test.hpp>

#include <net.h>
#include <netaddress.h>
#include <node/connection_types.h>
#include <node/eviction.h>
#include <node/eviction_impl.h>
#include <util/time.h>

#include <chrono>

using namespace std::literals;

BOOST_AUTO_TEST_SUITE(evictionman_tests)

BOOST_AUTO_TEST_CASE(full_outbound_eviction)
{
    auto evictionman = std::make_unique<EvictionManager>(
        MAX_BLOCK_RELAY_ONLY_CONNECTIONS, MAX_OUTBOUND_FULL_RELAY_CONNECTIONS);
    auto start_time{GetTime<std::chrono::seconds>()};
    SetMockTime(start_time);

    NodeId id{0};
    for (; id < MAX_OUTBOUND_FULL_RELAY_CONNECTIONS; ++id) {
        evictionman->AddCandidate(
            /*id=*/id,
            /*connected=*/GetTime<std::chrono::seconds>(),
            /*keyed_net_group=*/0,
            /*prefer_evict=*/false,
            /*is_local=*/false,
            /*network=*/NET_IPV4,
            /*noban=*/false,
            /*conn_type=*/ConnectionType::OUTBOUND_FULL_RELAY);
        evictionman->UpdateSuccessfullyConnected(id);
    }

    {
        auto [block_relay_peer, full_relay_peer] = evictionman->SelectOutboundNodesToEvict(GetTime<std::chrono::seconds>());
        BOOST_CHECK(!block_relay_peer);
        BOOST_CHECK(!full_relay_peer);
    }

    NodeId last_added_peer{id++};
    evictionman->AddCandidate(
        /*id=*/last_added_peer,
        /*connected=*/GetTime<std::chrono::seconds>(),
        /*keyed_net_group=*/0,
        /*prefer_evict=*/false,
        /*is_local=*/false,
        /*network=*/NET_IPV4,
        /*noban=*/false,
        /*conn_type=*/ConnectionType::OUTBOUND_FULL_RELAY);
    evictionman->UpdateSuccessfullyConnected(last_added_peer);

    SetMockTime(start_time + MINIMUM_CONNECT_TIME + 1s);

    {
        auto [block_relay_peer, full_relay_peer] = evictionman->SelectOutboundNodesToEvict(GetTime<std::chrono::seconds>());
        BOOST_CHECK(!block_relay_peer);
        BOOST_CHECK_EQUAL(full_relay_peer.value_or(-1), last_added_peer);
    }

    evictionman->UpdateLastBlockAnnounceTime(last_added_peer, GetTime<std::chrono::seconds>());

    {
        auto [block_relay_peer, full_relay_peer] = evictionman->SelectOutboundNodesToEvict(GetTime<std::chrono::seconds>());
        BOOST_CHECK(!block_relay_peer);
        BOOST_CHECK_EQUAL(full_relay_peer.value_or(-1), last_added_peer - 1);
    }
}

BOOST_AUTO_TEST_CASE(block_relay_eviction)
{
    auto evictionman = std::make_unique<EvictionManager>(
        MAX_BLOCK_RELAY_ONLY_CONNECTIONS, MAX_OUTBOUND_FULL_RELAY_CONNECTIONS);
    auto start_time{GetTime<std::chrono::seconds>()};
    SetMockTime(start_time);

    NodeId id{0};
    for (; id < MAX_BLOCK_RELAY_ONLY_CONNECTIONS; ++id) {
        evictionman->AddCandidate(
            /*id=*/id,
            /*connected=*/GetTime<std::chrono::seconds>(),
            /*keyed_net_group=*/0,
            /*prefer_evict=*/false,
            /*is_local=*/false,
            /*network=*/NET_IPV4,
            /*noban=*/false,
            /*conn_type=*/ConnectionType::BLOCK_RELAY);
        evictionman->UpdateSuccessfullyConnected(id);
    }

    {
        auto [block_relay_peer, full_relay_peer] = evictionman->SelectOutboundNodesToEvict(GetTime<std::chrono::seconds>());
        BOOST_CHECK(!block_relay_peer);
        BOOST_CHECK(!full_relay_peer);
    }

    NodeId last_added_peer{id++};
    evictionman->AddCandidate(
        /*id=*/last_added_peer,
        /*connected=*/GetTime<std::chrono::seconds>(),
        /*keyed_net_group=*/0,
        /*prefer_evict=*/false,
        /*is_local=*/false,
        /*network=*/NET_IPV4,
        /*noban=*/false,
        /*conn_type=*/ConnectionType::BLOCK_RELAY);
    evictionman->UpdateSuccessfullyConnected(last_added_peer);

    SetMockTime(start_time + MINIMUM_CONNECT_TIME + 1s);

    {
        auto [block_relay_peer, full_relay_peer] = evictionman->SelectOutboundNodesToEvict(GetTime<std::chrono::seconds>());
        BOOST_CHECK_EQUAL(block_relay_peer.value_or(-1), last_added_peer);
        BOOST_CHECK(!full_relay_peer);
    }

    evictionman->UpdateLastBlockTime(last_added_peer, GetTime<std::chrono::seconds>());

    {
        auto [block_relay_peer, full_relay_peer] = evictionman->SelectOutboundNodesToEvict(GetTime<std::chrono::seconds>());
        BOOST_CHECK_EQUAL(block_relay_peer.value_or(-1), last_added_peer - 1);
        BOOST_CHECK(!full_relay_peer);
    }
}

BOOST_AUTO_TEST_SUITE_END()
