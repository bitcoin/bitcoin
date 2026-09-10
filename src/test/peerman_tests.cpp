// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <interfaces/mining.h>
#include <net.h>
#include <net_processing.h>
#include <netbase.h>
#include <netmessagemaker.h>
#include <node/connection_types.h>
#include <node/miner.h>
#include <pow.h>
#include <primitives/block.h>
#include <protocol.h>
#include <sync.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <utility>
#include <vector>

namespace {
struct PendingBlockTestingSetup : RegTestingSetup {
    std::vector<std::unique_ptr<CNode>> m_nodes;

    ConnmanTestMsg& Connman() { return static_cast<ConnmanTestMsg&>(*m_node.connman); }
    PeerManager& Peerman() { return *m_node.peerman; }

    PendingBlockTestingSetup()
    {
        m_node.validation_signals->RegisterValidationInterface(m_node.peerman.get());
    }

    CNode& AddPeer(NodeId id) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        auto& node{*m_nodes.emplace_back(std::make_unique<CNode>(id, /*sock=*/nullptr,
            CAddress(LookupNumeric("127.0.0.1", 18444), NODE_NONE), /*nKeyedNetGroupIn=*/0,
            /*nLocalHostNonceIn=*/0, CAddress(), /*addrNameIn=*/"", ConnectionType::INBOUND,
            /*inbound_onion=*/false, /*network_key=*/0))};
        Connman().AddTestNode(node);
        Connman().Handshake(node, /*successfully_connected=*/true,
            /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
            /*local_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
            PROTOCOL_VERSION, /*relay_txs=*/true);
        Connman().FlushSendBuffer(node);
        node.fPauseSend = false;
        return node;
    }

    void RemovePeer(CNode& node)
    {
        node.fDisconnect = true;
        Connman().RemoveTestNode(node);
        Peerman().FinalizeNode(node);
        std::erase_if(m_nodes, [&](const auto& owned) { return owned.get() == &node; });
    }

    ~PendingBlockTestingSetup()
    {
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        m_node.validation_signals->UnregisterValidationInterface(m_node.peerman.get());
        while (!m_nodes.empty()) RemovePeer(*m_nodes.back());
    }

    CNodeStateStats Stats(NodeId id)
    {
        CNodeStateStats stats;
        BOOST_REQUIRE(Peerman().GetNodeStateStats(id, stats));
        return stats;
    }

    bool HasSendData(CNode& node)
    {
        LOCK(node.cs_vSend);
        const auto& [bytes, more, type] = node.m_transport->GetBytesToSend(false);
        return !bytes.empty() || !node.vSendMsg.empty();
    }

    std::pair<std::shared_ptr<CBlock>, const CBlockIndex*> PrepareHeader()
    {
        auto block{PrepareBlock(m_node, {})};
        while (!CheckProofOfWork(block->GetHash(), block->nBits, m_node.chainman->GetConsensus())) ++block->nNonce;
        BlockValidationState state;
        const CBlockIndex* index{nullptr};
        const std::array<CBlockHeader, 1> headers{*block};
        BOOST_REQUIRE(m_node.chainman->ProcessNewBlockHeaders(headers, /*min_pow_checked=*/true, state, &index));
        BOOST_REQUIRE(index);
        return {block, index};
    }
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(peerman_tests, RegTestingSetup)

/** Window, in blocks, for connecting to NODE_NETWORK_LIMITED peers */
static constexpr int64_t NODE_NETWORK_LIMITED_ALLOW_CONN_BLOCKS = 144;

static void mineBlock(node::NodeContext& node, FakeNodeClock& clock, std::chrono::seconds block_time)
{
    auto curr_time = GetTime<std::chrono::seconds>();
    clock.set(block_time); // update time so the block is created with it
    auto mining{interfaces::MakeMining(node)};
    auto block_template{mining->createNewBlock({}, /*cooldown=*/false)};
    BOOST_REQUIRE(block_template);
    CBlock block{block_template->getBlock()};
    while (!CheckProofOfWork(block.GetHash(), block.nBits, node.chainman->GetConsensus())) ++block.nNonce;
    block.m_validation_cache.m_checked.store(true); // little speedup
    clock.set(curr_time); // process block at current time
    BlockValidationState state;
    Assert(node.chainman->ProcessNewBlock(std::make_shared<const CBlock>(block), state, /*force_processing=*/true, /*min_pow_checked=*/true).get().processing_success);
    node.validation_signals->SyncWithValidationInterfaceQueue(); // drain events queue
}

// Verifying when network-limited peer connections are desirable based on the node's proximity to the tip
BOOST_AUTO_TEST_CASE(connections_desirable_service_flags)
{
    FakeNodeClock clock{};
    std::unique_ptr<PeerManager> peerman = PeerManager::make(*m_node.connman, *m_node.addrman, nullptr, *m_node.chainman, *m_node.mempool, *m_node.warnings, {});
    auto consensus = m_node.chainman->GetParams().GetConsensus();

    // Check we start connecting to full nodes
    ServiceFlags peer_flags{NODE_WITNESS | NODE_NETWORK_LIMITED};
    BOOST_CHECK(peerman->GetDesirableServiceFlags(peer_flags) == ServiceFlags(NODE_NETWORK | NODE_WITNESS));

    // Make peerman aware of the initial best block and verify we accept limited peers when we start close to the tip time.
    auto tip = WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip());
    uint64_t tip_block_time = tip->GetBlockTime();
    int tip_block_height = tip->nHeight;
    peerman->SetBestBlock(tip_block_height, std::chrono::seconds{tip_block_time});

    clock.set(std::chrono::seconds{tip_block_time + 1}); // Set node time to tip time
    BOOST_CHECK(peerman->GetDesirableServiceFlags(peer_flags) == ServiceFlags(NODE_NETWORK_LIMITED | NODE_WITNESS));

    // Check we don't disallow limited peers connections when we are behind but still recoverable (below the connection safety window)
    clock += std::chrono::seconds{consensus.nPowTargetSpacing * (NODE_NETWORK_LIMITED_ALLOW_CONN_BLOCKS - 1)};
    BOOST_CHECK(peerman->GetDesirableServiceFlags(peer_flags) == ServiceFlags(NODE_NETWORK_LIMITED | NODE_WITNESS));

    // Check we disallow limited peers connections when we are further than the limited peers safety window
    clock += std::chrono::seconds{consensus.nPowTargetSpacing * 2};
    BOOST_CHECK(peerman->GetDesirableServiceFlags(peer_flags) == ServiceFlags(NODE_NETWORK | NODE_WITNESS));

    // By now, we tested that the connections desirable services flags change based on the node's time proximity to the tip.
    // Now, perform the same tests for when the node receives a block.
    m_node.validation_signals->RegisterValidationInterface(peerman.get());

    // First, verify a block in the past doesn't enable limited peers connections
    // At this point, our time is (NODE_NETWORK_LIMITED_ALLOW_CONN_BLOCKS + 1) * 10 minutes ahead the tip's time.
    mineBlock(m_node, clock, /*block_time=*/std::chrono::seconds{tip_block_time + 1});
    BOOST_CHECK(peerman->GetDesirableServiceFlags(peer_flags) == ServiceFlags(NODE_NETWORK | NODE_WITNESS));

    // Verify a block close to the tip enables limited peers connections
    mineBlock(m_node, clock, /*block_time=*/GetTime<std::chrono::seconds>());
    BOOST_CHECK(peerman->GetDesirableServiceFlags(peer_flags) == ServiceFlags(NODE_NETWORK_LIMITED | NODE_WITNESS));

    // Lastly, verify the stale tip checks can disallow limited peers connections after not receiving blocks for a prolonged period.
    clock += std::chrono::seconds{consensus.nPowTargetSpacing * NODE_NETWORK_LIMITED_ALLOW_CONN_BLOCKS + 1};
    BOOST_CHECK(peerman->GetDesirableServiceFlags(peer_flags) == ServiceFlags(NODE_NETWORK | NODE_WITNESS));
}

BOOST_FIXTURE_TEST_CASE(pending_block_pauses_only_source_peer, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    FakeNodeClock clock{};
    auto& source{AddPeer(0)};
    auto& other{AddPeer(1)};
    std::promise<BlockProcessingResult> completion;
    Peerman().UnitTestBlockProcessing(source.GetId(), m_node.chainman->GetParams().GenesisBlock().GetHash(), completion.get_future(), /*optimistic_reconstruction=*/false);

    BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::GETADDR)));
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::GETADDR)));
    BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);
    BOOST_CHECK(!Connman().ProcessMessagesOnce(source));
    BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);
    Connman().ProcessMessagesOnce(other);
    BOOST_CHECK(Stats(other.GetId()).m_addr_relay_enabled);

    // Pending validation pauses the source's send loop, while other peers can send.
    Peerman().SendPings();
    Peerman().SendMessages(source);
    BOOST_CHECK(!HasSendData(source));
    Peerman().SendMessages(other);
    BOOST_CHECK(HasSendData(other));

    completion.set_value({.processing_success = true, .new_block = true});
    Connman().ProcessMessagesOnce(source);
    BOOST_CHECK(Stats(source.GetId()).m_addr_relay_enabled);
    BOOST_CHECK(source.m_last_block_time.load() == GetTime<std::chrono::seconds>());
    Peerman().SendMessages(source);
    BOOST_CHECK(HasSendData(source));

    // Repeated polling must not repeat completion's timestamp update.
    const auto completed_at{source.m_last_block_time.load()};
    clock += 1s;
    Peerman().ProcessPendingEvents();
    Peerman().SendMessages(source);
    BOOST_CHECK(source.m_last_block_time.load() == completed_at);
}

BOOST_FIXTURE_TEST_CASE(pending_block_allows_queued_getdata_work, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& source{AddPeer(0)};
    const auto hash{m_node.chainman->GetParams().GenesisBlock().GetHash()};
    // The block request is processed first, leaving the transaction request queued.
    const std::vector<CInv> requests{{MSG_WITNESS_BLOCK, hash}, {MSG_TX, hash}};
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::GETDATA, requests)));
    Connman().ProcessMessagesOnce(source);
    Connman().FlushSendBuffer(source);
    source.fPauseSend = false;

    std::promise<BlockProcessingResult> completion;
    Peerman().UnitTestBlockProcessing(source.GetId(), hash, completion.get_future(), /*optimistic_reconstruction=*/false);
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::GETADDR)));

    // Answer the queued request while validation is pending, without dequeuing GETADDR.
    BOOST_CHECK(!Connman().ProcessMessagesOnce(source));
    BOOST_CHECK(HasSendData(source));
    BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);
    Connman().FlushSendBuffer(source);
    source.fPauseSend = false;
    BOOST_CHECK(!Connman().ProcessMessagesOnce(source));
    BOOST_CHECK(!HasSendData(source));
    BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);

    completion.set_value({.processing_success = true, .new_block = false});
    Connman().ProcessMessagesOnce(source);
    BOOST_CHECK(Stats(source.GetId()).m_addr_relay_enabled);
}

BOOST_FIXTURE_TEST_CASE(pending_block_survives_disconnection, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& source{AddPeer(0)};
    auto& requested_from{AddPeer(1)};
    const auto [block, index]{PrepareHeader()};
    BOOST_REQUIRE(Peerman().FetchBlock(requested_from.GetId(), *index));
    std::promise<BlockProcessingResult> completion;
    Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), completion.get_future(), /*optimistic_reconstruction=*/false);
    Peerman().ProcessPendingEvents();
    BOOST_CHECK_EQUAL(Stats(requested_from.GetId()).vHeightInFlight.size(), 1);

    // Destroy the source CNode before completion. Cleanup still belongs to the manager.
    RemovePeer(source);
    completion.set_value({.processing_success = false, .new_block = true});
    Peerman().ProcessPendingEvents();
    BOOST_CHECK(Stats(requested_from.GetId()).vHeightInFlight.empty());

    // The loop must also be able to drain completions with no connected peers left.
    std::promise<BlockProcessingResult> last_completion;
    Peerman().UnitTestBlockProcessing(requested_from.GetId(), block->GetHash(), last_completion.get_future(), /*optimistic_reconstruction=*/false);
    RemovePeer(requested_from);
    last_completion.set_value({.processing_success = false, .new_block = false});
    Peerman().ProcessPendingEvents();
    Peerman().ProcessPendingEvents();
}

BOOST_FIXTURE_TEST_CASE(pending_optimistic_block_checks_validity_at_completion, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& source{AddPeer(0)};
    auto& requested_from{AddPeer(1)};
    const auto [block, index]{PrepareHeader()};
    BOOST_REQUIRE(Peerman().FetchBlock(requested_from.GetId(), *index));

    std::promise<BlockProcessingResult> rejected;
    Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), rejected.get_future(), /*optimistic_reconstruction=*/true);
    rejected.set_value({.processing_success = false, .new_block = false});
    Peerman().ProcessPendingEvents();
    BOOST_CHECK_EQUAL(Stats(requested_from.GetId()).vHeightInFlight.size(), 1);

    std::promise<BlockProcessingResult> completion;
    Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), completion.get_future(), /*optimistic_reconstruction=*/true);
    // Another submission stores the block before our optimistic completion is ready.
    BlockValidationState state;
    BOOST_REQUIRE(m_node.chainman->ProcessNewBlock(block, state, /*force_processing=*/true, /*min_pow_checked=*/true).get().processing_success);
    Peerman().ProcessPendingEvents();
    BOOST_CHECK_EQUAL(Stats(requested_from.GetId()).vHeightInFlight.size(), 1);
    completion.set_value({.processing_success = true, .new_block = false});
    Peerman().ProcessPendingEvents();
    BOOST_CHECK(Stats(requested_from.GetId()).vHeightInFlight.empty());
    BOOST_CHECK(source.m_last_block_time.load() == 0s);
}

BOOST_FIXTURE_TEST_CASE(pending_block_punishes_before_resuming, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto block{std::make_shared<const CBlock>(m_node.chainman->GetParams().GenesisBlock())};
    BlockValidationState invalid;
    invalid.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "test-invalid-block");

    for (const bool ready : {false, true}) {
        auto& source{AddPeer(ready ? 1 : 0)};
        std::promise<BlockProcessingResult> completion;
        Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), completion.get_future(), /*optimistic_reconstruction=*/false);
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::GETADDR)));
        m_node.validation_signals->BlockChecked(block, invalid);
        if (ready) completion.set_value({.processing_success = false, .new_block = false});
        // Global polling must not allow the peer to resume before punishment either.
        Peerman().ProcessPendingEvents();
        BOOST_CHECK(!Connman().ProcessMessagesOnce(source));
        BOOST_CHECK(source.fDisconnect);
        BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);
        if (!ready) completion.set_value({.processing_success = false, .new_block = false});
        Peerman().ProcessPendingEvents();
        RemovePeer(source);
    }
}

BOOST_FIXTURE_TEST_CASE(pending_block_preserves_newer_source, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& first{AddPeer(0)};
    auto& second{AddPeer(1)};
    auto block{std::make_shared<const CBlock>(m_node.chainman->GetParams().GenesisBlock())};
    std::promise<BlockProcessingResult> first_completion;
    Peerman().UnitTestBlockProcessing(first.GetId(), block->GetHash(), first_completion.get_future(), /*optimistic_reconstruction=*/false);
    m_node.validation_signals->BlockChecked(block, BlockValidationState{});

    // A later submission now owns the source entry for the same hash.
    std::promise<BlockProcessingResult> second_completion;
    Peerman().UnitTestBlockProcessing(second.GetId(), block->GetHash(), second_completion.get_future(), /*optimistic_reconstruction=*/false);
    first_completion.set_value({.processing_success = false, .new_block = false});
    Peerman().ProcessPendingEvents();
    BlockValidationState invalid;
    invalid.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "test-invalid-block");
    m_node.validation_signals->BlockChecked(block, invalid);
    second_completion.set_value({.processing_success = false, .new_block = false});
    Connman().ProcessMessagesOnce(second);
    BOOST_CHECK(second.fDisconnect);
    BOOST_CHECK(!first.fDisconnect);
}

BOOST_FIXTURE_TEST_CASE(pending_block_failed_future, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    for (const bool broken_promise : {true, false}) {
        auto& source{AddPeer(broken_promise ? 0 : 1)};
        {
            std::promise<BlockProcessingResult> completion;
            Peerman().UnitTestBlockProcessing(source.GetId(), m_node.chainman->GetParams().GenesisBlock().GetHash(), completion.get_future(), /*optimistic_reconstruction=*/false);
            if (!broken_promise) {
                struct UnexpectedException {};
                completion.set_exception(std::make_exception_ptr(UnexpectedException{}));
            }
        }
        Peerman().ProcessPendingEvents();
        BOOST_CHECK(source.fDisconnect);
        BOOST_CHECK(source.m_last_block_time.load() == 0s);
        Peerman().ProcessPendingEvents();
        RemovePeer(source);
    }
}

BOOST_AUTO_TEST_SUITE_END()
