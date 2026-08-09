// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <node/connection_types.h>
#include <protocol.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <memory>

static_assert(MAX_VERIFIED_INBOUND_PER_PROTX >= 2,
              "cap must tolerate a reconnect racing a not-yet-reaped socket");

BOOST_FIXTURE_TEST_SUITE(evo_mnauth_tests, BasicTestingSetup)

namespace {

//! Ownership passes to ConnmanTestMsg, which deletes the nodes in ClearTestNodes().
CNode& AddNode(ConnmanTestMsg& connman, NodeId id, ConnectionType conn_type)
{
    auto* node = new CNode{id,
                           /*sock=*/nullptr,
                           CAddress{},
                           /*nKeyedNetGroupIn=*/0,
                           /*nLocalHostNonceIn=*/0,
                           CAddress{},
                           /*addrNameIn=*/"",
                           conn_type,
                           /*inbound_onion=*/false};
    connman.AddTestNode(*node);
    return *node;
}

} // namespace

/**
 * CConnman::TryMarkVerified is the sole gate to verified-masternode status, which exempts an
 * inbound peer from the inbound connection limit and protects it from eviction. It must refuse the
 * grant once MAX_VERIFIED_INBOUND_PER_PROTX verified inbound connections already carry the same
 * proRegTxHash, else one operator key could stack limit-exempt, eviction-immune slots without end.
 */
BOOST_AUTO_TEST_CASE(verified_inbound_capped_per_protx)
{
    auto connman = std::make_unique<ConnmanTestMsg>(0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman);

    const uint256 attacker{1};
    const uint256 other{2};
    const uint256 pubkey_hash{3};
    NodeId id{0};

    // An unknown peer id cannot be granted anything.
    BOOST_CHECK(!connman->TryMarkVerified(/*id=*/1337, attacker, pubkey_hash));

    // Inbound legs sharing one identity accumulate against that identity's budget; grants below
    // the cap succeed, the first grant beyond it is refused and leaves the peer unmarked.
    CNode* first_leg{nullptr};
    for (size_t i = 0; i < MAX_VERIFIED_INBOUND_PER_PROTX; ++i) {
        CNode& node = AddNode(*connman, id++, ConnectionType::INBOUND);
        if (i == 0) first_leg = &node;
        BOOST_CHECK(connman->TryMarkVerified(node.GetId(), attacker, pubkey_hash));
        BOOST_CHECK_EQUAL(node.GetVerifiedProRegTxHash(), attacker);
        BOOST_CHECK_EQUAL(node.GetVerifiedPubKeyHash(), pubkey_hash);
    }
    CNode& refused = AddNode(*connman, id++, ConnectionType::INBOUND);
    BOOST_CHECK(!connman->TryMarkVerified(refused.GetId(), attacker, pubkey_hash));
    BOOST_CHECK(refused.GetVerifiedProRegTxHash().IsNull());
    BOOST_CHECK(refused.GetVerifiedPubKeyHash().IsNull());

    // The budget is per identity, and unverified inbound legs such as `refused` do not consume it:
    // they already count against nInbound and remain ordinary eviction candidates.
    CNode& other_node = AddNode(*connman, id++, ConnectionType::INBOUND);
    BOOST_CHECK(connman->TryMarkVerified(other_node.GetId(), other, pubkey_hash));

    // Outbound legs are not part of the resource being bounded: neither the limit exemption nor
    // eviction protection applies to them, so they are granted beyond the cap.
    CNode& outbound = AddNode(*connman, id++, ConnectionType::OUTBOUND_FULL_RELAY);
    BOOST_CHECK(connman->TryMarkVerified(outbound.GetId(), attacker, pubkey_hash));

    // A verified leg already flagged for disconnect no longer holds its slot, so a reconnect
    // racing a stale socket is accepted as soon as the stale leg is being torn down.
    first_leg->fDisconnect = true;
    BOOST_CHECK(connman->TryMarkVerified(refused.GetId(), attacker, pubkey_hash));
    BOOST_CHECK_EQUAL(refused.GetVerifiedProRegTxHash(), attacker);

    connman->ClearTestNodes();
}

BOOST_AUTO_TEST_SUITE_END()
