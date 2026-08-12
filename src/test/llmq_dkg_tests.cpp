// Copyright (c) 2022-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <llmq/dkgsession.h>
#include <llmq/dkgsessionhandler.h>
#include <protocol.h>
#include <span.h>
#include <streams.h>
#include <util/irange.h>
#include <util/underlying.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(llmq_dkg_tests)

BOOST_AUTO_TEST_CASE(llmq_dkgerror)
{
    using namespace llmq;
    for (auto i : irange::range(ToUnderlying(llmq::DKGError::type::_COUNT))) {
        BOOST_REQUIRE(GetSimulatedErrorRate(llmq::DKGError::type(i)) == 0.0);
        SetSimulatedDKGErrorRate(llmq::DKGError::type(i), 1.0);
        BOOST_REQUIRE(GetSimulatedErrorRate(llmq::DKGError::type(i)) == 1.0);
    }
    BOOST_REQUIRE(GetSimulatedErrorRate(llmq::DKGError::type::_COUNT) == 0.0);
    SetSimulatedDKGErrorRate(llmq::DKGError::type::_COUNT, 1.0);
    BOOST_REQUIRE(GetSimulatedErrorRate(llmq::DKGError::type::_COUNT) == 0.0);
}

namespace {
CDataStream MakeDKGMessage(uint8_t value)
{
    CDataStream ds{SER_NETWORK, PROTOCOL_VERSION};
    ds << value;
    return ds;
}

//! The inventory hash CDKGPendingMessages records for MakeDKGMessage(value)
uint256 MessageHash(uint8_t value)
{
    const CDataStream ds{MakeDKGMessage(value)};
    CHashWriter hw(SER_GETHASH, 0);
    hw.write(AsBytes(Span{ds}));
    return hw.GetHash();
}

void Push(llmq::CDKGPendingMessages& pending, NodeId from, const uint256& sender_protx, uint8_t value)
{
    CDataStream ds{MakeDKGMessage(value)};
    (void)pending.PushPendingMessage(from, sender_protx, ds);
}

uint256 MakeProTx(uint8_t value)
{
    uint256 hash;
    hash.begin()[0] = value;
    return hash;
}
} // namespace

BOOST_AUTO_TEST_CASE(pending_messages_own_messages_share_quota_path)
{
    using namespace llmq;

    const uint256 own_protx = MakeProTx(0xee);

    // Own messages (from=-1) are enqueued under this node's proTxHash and
    // charged like any other sender's.
    CDKGPendingMessages pending{/*_maxMessagesPerProTx=*/2, MSG_QUORUM_CONTRIB};
    Push(pending, /*from=*/-1, own_protx, 1);
    Push(pending, /*from=*/-1, own_protx, 2);
    Push(pending, /*from=*/-1, own_protx, 3);
    BOOST_CHECK(pending.HasSeen(MessageHash(2)));
    BOOST_CHECK(!pending.HasSeen(MessageHash(3)));

    BOOST_CHECK_EQUAL(pending.PopPendingMessages(3).size(), 2U);
}

BOOST_AUTO_TEST_CASE(pending_messages_quota_survives_reconnect)
{
    using namespace llmq;

    const uint256 protx_a = MakeProTx(0xa1);

    CDKGPendingMessages pending{/*_maxMessagesPerProTx=*/2, MSG_QUORUM_CONTRIB};
    Push(pending, /*from=*/1, protx_a, 1);
    Push(pending, /*from=*/1, protx_a, 2);
    BOOST_CHECK(pending.HasSeen(MessageHash(1)));
    BOOST_CHECK(pending.HasSeen(MessageHash(2)));

    // Reconnecting mints a fresh NodeId but keeps the proTxHash, so the quota
    // is already spent.
    Push(pending, /*from=*/2, protx_a, 3);
    Push(pending, /*from=*/3, protx_a, 4);
    BOOST_CHECK(!pending.HasSeen(MessageHash(3)));
    BOOST_CHECK(!pending.HasSeen(MessageHash(4)));

    // Draining frees queue slots but does not refund the per-proTx quota.
    BOOST_CHECK_EQUAL(pending.PopPendingMessages(5).size(), 2U);
    Push(pending, /*from=*/4, protx_a, 5);
    BOOST_CHECK(!pending.HasSeen(MessageHash(5)));

    // A new round resets everything.
    pending.Clear();
    Push(pending, /*from=*/4, protx_a, 5);
    BOOST_CHECK(pending.HasSeen(MessageHash(5)));
}

BOOST_AUTO_TEST_CASE(pending_messages_quota_is_per_protx)
{
    using namespace llmq;

    const uint256 protx_a = MakeProTx(0xa1);
    const uint256 protx_b = MakeProTx(0xb1);

    CDKGPendingMessages pending{/*_maxMessagesPerProTx=*/2, MSG_QUORUM_CONTRIB};
    Push(pending, /*from=*/1, protx_a, 1);
    Push(pending, /*from=*/1, protx_a, 2);

    // Duplicates are rejected before charging the quota.
    Push(pending, /*from=*/2, protx_b, 1);
    Push(pending, /*from=*/2, protx_b, 2);

    // One proTx's spent quota has no effect on another's.
    Push(pending, /*from=*/2, protx_b, 3);
    Push(pending, /*from=*/2, protx_b, 4);
    Push(pending, /*from=*/2, protx_b, 5);
    BOOST_CHECK(pending.HasSeen(MessageHash(3)));
    BOOST_CHECK(pending.HasSeen(MessageHash(4)));
    BOOST_CHECK(!pending.HasSeen(MessageHash(5)));

    // A quota-dropped hash is not marked seen, so a sender with remaining
    // budget can still deliver it.
    const uint256 protx_c = MakeProTx(0xc1);
    Push(pending, /*from=*/3, protx_c, 5);
    BOOST_CHECK(pending.HasSeen(MessageHash(5)));

    BOOST_CHECK_EQUAL(pending.PopPendingMessages(6).size(), 5U);
}

BOOST_AUTO_TEST_SUITE_END()
