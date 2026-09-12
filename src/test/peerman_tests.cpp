// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/consensus.h>
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
#include <primitives/transaction.h>
#include <protocol.h>
#include <scheduler.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/validation.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validation_queue.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
struct PendingBlockTestingSetup : RegTestingSetup {
    std::vector<std::unique_ptr<CNode>> m_nodes;

    ConnmanTestMsg& Connman() { return static_cast<ConnmanTestMsg&>(*m_node.connman); }
    PeerManager& Peerman() { return *m_node.peerman; }

    PendingBlockTestingSetup()
    {
        // Drain setup callbacks before tests install synthetic block sources.
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        m_node.validation_signals->RegisterValidationInterface(m_node.peerman.get());
    }

    CNode& AddPeer(NodeId id, ConnectionType connection_type = ConnectionType::INBOUND) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        auto& node{*m_nodes.emplace_back(std::make_unique<CNode>(id, /*sock=*/nullptr,
            CAddress(LookupNumeric("127.0.0.1", 18444), NODE_NONE), /*nKeyedNetGroupIn=*/0,
            /*nLocalHostNonceIn=*/0, CAddress(), /*addrNameIn=*/"", connection_type,
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
        m_node.chainman->StopBlockProcessing();
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

    void WaitForBlockProcessing()
    {
        // PeerManager owns the block futures. Wait for a job queued after them.
        BlockWorkerGate completion{*m_node.chainman};
        completion.Open();
    }

    void ProcessBlockCompletions() EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        // Ready futures need a second poll after their callback markers run.
        Peerman().ProcessPendingEvents();
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        Peerman().ProcessPendingEvents();
    }

    std::pair<std::shared_ptr<CBlock>, const CBlockIndex*> PrepareHeader(unsigned int nonce = 0)
    {
        auto block{PrepareBlock(m_node, {})};
        block->nNonce = nonce;
        while (!CheckProofOfWork(block->GetHash(), block->nBits, m_node.chainman->GetConsensus())) ++block->nNonce;
        BlockValidationState state;
        const CBlockIndex* index{nullptr};
        const std::array<CBlockHeader, 1> headers{*block};
        BOOST_REQUIRE(m_node.chainman->ProcessNewBlockHeaders(headers, /*min_pow_checked=*/true, state, &index));
        BOOST_REQUIRE(index);
        return {block, index};
    }

    std::vector<std::shared_ptr<CBlock>> PrepareStallingBlocks()
    {
        auto& chainman{*m_node.chainman};
        // Fill the 1024-block download window except for heights 1 and 501.
        auto blocks{CreateBlockChain(1025, chainman.GetParams())};
        std::vector<CBlockHeader> headers;
        headers.reserve(blocks.size());
        for (const auto& block : blocks) headers.push_back(*block);
        BlockValidationState header_state;
        BOOST_REQUIRE(chainman.ProcessNewBlockHeaders(headers, true, header_state));
        for (int height{2}; height <= 1024; ++height) {
            if (height == 501) continue;
            BlockValidationState state;
            BOOST_REQUIRE(chainman.ProcessNewBlock(blocks[height - 1], state, true, true).get().new_block);
        }
        return blocks;
    }

    void CheckStallingWithPendingBlock(bool pending_first)
    {
        LOCK(NetEventsInterface::g_msgproc_mutex);
        auto& chainman{*m_node.chainman};
        FakeNodeClock clock{chainman.GetParams().GenesisBlock().Time() + 24h};
        const auto blocks{PrepareStallingBlocks()};
        auto& source{AddPeer(0, ConnectionType::OUTBOUND_FULL_RELAY)};
        auto& staller{AddPeer(1, ConnectionType::OUTBOUND_FULL_RELAY)};
        auto& other{AddPeer(2, ConnectionType::OUTBOUND_FULL_RELAY)};
        const auto& pending{blocks[pending_first ? 0 : 500]};
        const auto& withheld{blocks[pending_first ? 500 : 0]};
        const auto* pending_index{WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(pending->GetHash()))};
        const auto* withheld_index{WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(withheld->GetHash()))};
        BOOST_REQUIRE(Peerman().FetchBlock(source.GetId(), *pending_index));
        BOOST_REQUIRE(Peerman().FetchBlock(staller.GetId(), *withheld_index));
        Connman().FlushSendBuffer(source);
        source.fPauseSend = false;

        BlockWorkerGate gate{chainman};
        gate.Wait();
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(*pending))));
        Connman().ProcessMessagesOnce(source);
        BOOST_REQUIRE(Stats(source.GetId()).vHeightInFlight.empty());
        BOOST_REQUIRE(WITH_LOCK(cs_main, return !(pending_index->nStatus & BLOCK_HAVE_DATA)));

        const std::vector<CBlock> headers{CBlock{static_cast<const CBlockHeader&>(*blocks.back())}};
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(headers))));
        Connman().ProcessMessagesOnce(other);
        Peerman().SendMessages(other);
        BOOST_CHECK(Stats(other.GetId()).vHeightInFlight.empty());

        // Only a remote block preceding the pending local block should time out.
        clock += 3s;
        Peerman().SendMessages(staller);
        BOOST_CHECK_EQUAL(staller.fDisconnect, !pending_first);

        gate.Open();
        WaitForBlockProcessing();
        ProcessBlockCompletions();
        if (pending_first) {
            BOOST_REQUIRE_EQUAL(WITH_LOCK(cs_main, return chainman.ActiveHeight()), 500);
            Peerman().SendMessages(other);
            const auto in_flight{Stats(other.GetId()).vHeightInFlight};
            BOOST_REQUIRE_EQUAL(in_flight.size(), 1);
            BOOST_CHECK_EQUAL(in_flight.front(), 1025);
        }
    }

    void CheckQueuedDuplicateBlock(bool invalid)
    {
        LOCK(NetEventsInterface::g_msgproc_mutex);
        auto& chainman{*m_node.chainman};
        FakeNodeClock clock{std::chrono::seconds{chainman.GetParams().GenesisBlock().nTime} + 1h};
        const auto funding_blocks{CreateBlockChain(COINBASE_MATURITY, chainman.GetParams())};
        for (const auto& block : funding_blocks) {
            BlockValidationState state;
            BOOST_REQUIRE(chainman.ProcessNewBlock(block, state, true, true).get().new_block);
        }

        CMutableTransaction spend;
        spend.vin.emplace_back(COutPoint{funding_blocks.front()->vtx[0]->GetHash(), 0});
        // A wrong witness script passes admission, but fails when the block connects.
        spend.vin[0].scriptWitness.stack.push_back(invalid ? std::vector<uint8_t>{OP_FALSE} : WITNESS_STACK_ELEM_OP_TRUE);
        spend.vout.push_back(funding_blocks.front()->vtx[0]->vout[0]);
        --spend.vout[0].nValue;
        auto block{PrepareBlock(m_node, {})};
        block->vtx.push_back(MakeTransactionRef(spend));
        node::RegenerateCommitments(*block, chainman);
        while (!CheckProofOfWork(block->GetHash(), block->nBits, chainman.GetConsensus())) ++block->nNonce;

        auto& first{AddPeer(0, ConnectionType::OUTBOUND_FULL_RELAY)};
        auto& second{AddPeer(1, ConnectionType::OUTBOUND_FULL_RELAY)};
        BlockWorkerGate gate{chainman};
        gate.Wait();
        for (CNode* peer : {&first, &second}) {
            BOOST_REQUIRE(Connman().ReceiveMsgFrom(*peer, NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(*block))));
            Connman().ProcessMessagesOnce(*peer);
            BOOST_REQUIRE(Connman().ReceiveMsgFrom(*peer, NetMsg::Make(NetMsgType::PING, uint64_t{42})));
            BOOST_CHECK(!Connman().ProcessMessagesOnce(*peer));
            BOOST_REQUIRE(!peer->fDisconnect);
            BOOST_CHECK(!HasSendData(*peer));
        }
        {
            LOCK(cs_main);
            const CBlockIndex* index{Assert(chainman.m_blockman.LookupBlockIndex(block->GetHash()))};
            BOOST_CHECK(!(index->nStatus & BLOCK_HAVE_DATA));
            BOOST_CHECK(!(index->nStatus & BLOCK_FAILED_VALID));
        }

        gate.Open();
        WaitForBlockProcessing();
        ProcessBlockCompletions();
        for (CNode* peer : {&first, &second}) {
            Connman().ProcessMessagesOnce(*peer);
            BOOST_CHECK_EQUAL(peer->fDisconnect, invalid);
            auto message{peer->PollMessage()};
            if (invalid) {
                // Punishment must take effect before the next message is dequeued.
                BOOST_REQUIRE(message);
                BOOST_CHECK_EQUAL(message->first.m_type, NetMsgType::PING);
            } else {
                BOOST_CHECK(!message);
            }
        }
        LOCK(cs_main);
        const CBlockIndex* index{Assert(chainman.m_blockman.LookupBlockIndex(block->GetHash()))};
        BOOST_CHECK(index->nStatus & BLOCK_HAVE_DATA);
        BOOST_CHECK_EQUAL((index->nStatus & BLOCK_FAILED_VALID) != 0, invalid);
        BOOST_CHECK(chainman.ActiveTip()->GetBlockHash() == (invalid ? funding_blocks.back()->GetHash() : block->GetHash()));
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
    struct PeerManagerGuard {
        ValidationSignals& signals;
        PeerManager& peerman;

        ~PeerManagerGuard()
        {
            // Block futures are consumed; finish queued callbacks before destroying peerman.
            signals.UnregisterValidationInterface(&peerman);
            signals.SyncWithValidationInterfaceQueue();
        }
    } guard{*m_node.validation_signals, *peerman};

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
    Peerman().UnitTestBlockProcessing(source.GetId(), m_node.chainman->GetParams().GenesisBlock().GetHash(), completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);

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

    ValidationCallbackGate gate{*m_node.validation_signals};
    gate.Wait();
    completion.set_value({.processing_success = true, .new_block = true});
    BOOST_CHECK(!Connman().ProcessMessagesOnce(source));
    BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);
    BOOST_CHECK(source.m_last_block_time.load() == 0s);
    Peerman().SendMessages(source);
    BOOST_CHECK(!HasSendData(source));

    // Other peers can still process ordinary messages while callbacks are held.
    auto& callback_other{AddPeer(2)};
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(callback_other, NetMsg::Make(NetMsgType::GETADDR)));
    Connman().ProcessMessagesOnce(callback_other);
    BOOST_CHECK(Stats(callback_other.GetId()).m_addr_relay_enabled);

    gate.Open();
    ProcessBlockCompletions();
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
    Peerman().UnitTestBlockProcessing(source.GetId(), hash, completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
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
    ProcessBlockCompletions();
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
    Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
    Peerman().ProcessPendingEvents();
    BOOST_CHECK_EQUAL(Stats(requested_from.GetId()).vHeightInFlight.size(), 1);

    // Destroy the source CNode before completion. Cleanup still belongs to the manager.
    RemovePeer(source);
    ValidationCallbackGate gate{*m_node.validation_signals};
    gate.Wait();
    completion.set_value({.processing_success = false, .new_block = true});
    Peerman().ProcessPendingEvents();
    BOOST_CHECK_EQUAL(Stats(requested_from.GetId()).vHeightInFlight.size(), 1);
    gate.Open();
    ProcessBlockCompletions();
    BOOST_CHECK(Stats(requested_from.GetId()).vHeightInFlight.empty());

    // The loop must also be able to drain completions with no connected peers left.
    std::promise<BlockProcessingResult> last_completion;
    Peerman().UnitTestBlockProcessing(requested_from.GetId(), block->GetHash(), last_completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
    RemovePeer(requested_from);
    last_completion.set_value({.processing_success = false, .new_block = false});
    ProcessBlockCompletions();
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
    Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), rejected.get_future(), /*via_compact_block=*/true, /*optimistic_reconstruction=*/true);
    rejected.set_value({.processing_success = false, .new_block = false});
    ProcessBlockCompletions();
    BOOST_CHECK_EQUAL(Stats(requested_from.GetId()).vHeightInFlight.size(), 1);

    std::promise<BlockProcessingResult> completion;
    Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), completion.get_future(), /*via_compact_block=*/true, /*optimistic_reconstruction=*/true);
    // Another submission stores the block before our optimistic completion is ready.
    BlockValidationState state;
    BOOST_REQUIRE(m_node.chainman->ProcessNewBlock(block, state, /*force_processing=*/true, /*min_pow_checked=*/true).get().processing_success);
    Peerman().ProcessPendingEvents();
    BOOST_CHECK_EQUAL(Stats(requested_from.GetId()).vHeightInFlight.size(), 1);
    completion.set_value({.processing_success = true, .new_block = false});
    ProcessBlockCompletions();
    BOOST_CHECK(Stats(requested_from.GetId()).vHeightInFlight.empty());
    BOOST_CHECK(source.m_last_block_time.load() == 0s);
}

BOOST_FIXTURE_TEST_CASE(pending_block_source_cleanup_without_cs_main, PendingBlockTestingSetup)
{
    CNode* source;
    {
        LOCK(NetEventsInterface::g_msgproc_mutex);
        source = &AddPeer(0);
    }
    std::future<void> completion;
    {
        LOCK(cs_main);
        completion = std::async(std::launch::async, [&] {
            LOCK(NetEventsInterface::g_msgproc_mutex);
            std::promise<BlockProcessingResult> result;
            Peerman().UnitTestBlockProcessing(source->GetId(), m_node.chainman->GetParams().GenesisBlock().GetHash(), result.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
            result.set_value({.processing_success = false, .new_block = false});
            ProcessBlockCompletions();
        });
        // Release cs_main before joining even if the old lock dependency returns.
        BOOST_CHECK(completion.wait_for(10s) == std::future_status::ready);
    }
    completion.get();
    LOCK(NetEventsInterface::g_msgproc_mutex);
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(*source, NetMsg::Make(NetMsgType::GETADDR)));
    Connman().ProcessMessagesOnce(*source);
    BOOST_CHECK(Stats(source->GetId()).m_addr_relay_enabled);
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
        Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::GETADDR)));
        ValidationCallbackGate gate{*m_node.validation_signals};
        gate.Wait();
        m_node.validation_signals->BlockChecked(block, invalid);
        if (ready) completion.set_value({.processing_success = false, .new_block = false});
        // Global polling must not allow the peer to resume before punishment either.
        Peerman().ProcessPendingEvents();
        BOOST_CHECK(!Connman().ProcessMessagesOnce(source));
        BOOST_CHECK(!source.fDisconnect);
        BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);
        gate.Open();
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        BOOST_CHECK(!Connman().ProcessMessagesOnce(source));
        BOOST_CHECK(source.fDisconnect);
        BOOST_CHECK(!Stats(source.GetId()).m_addr_relay_enabled);
        if (!ready) completion.set_value({.processing_success = false, .new_block = false});
        ProcessBlockCompletions();
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
    Peerman().UnitTestBlockProcessing(first.GetId(), block->GetHash(), first_completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
    m_node.validation_signals->BlockChecked(block, BlockValidationState{});
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // A later submission now owns the source entry for the same hash.
    std::promise<BlockProcessingResult> second_completion;
    Peerman().UnitTestBlockProcessing(second.GetId(), block->GetHash(), second_completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
    first_completion.set_value({.processing_success = false, .new_block = false});
    ProcessBlockCompletions();
    BlockValidationState invalid;
    invalid.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "test-invalid-block");
    m_node.validation_signals->BlockChecked(block, invalid);
    second_completion.set_value({.processing_success = false, .new_block = false});
    ProcessBlockCompletions();
    Connman().ProcessMessagesOnce(second);
    BOOST_CHECK(second.fDisconnect);
    BOOST_CHECK(!first.fDisconnect);
}

BOOST_FIXTURE_TEST_CASE(pending_cached_rejection_preserves_source_and_exemptions, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto block{std::make_shared<const CBlock>(m_node.chainman->GetParams().GenesisBlock())};
    BlockValidationState cached_invalid;
    cached_invalid.Invalid(BlockValidationResult::BLOCK_CACHED_INVALID, "duplicate-invalid");
    BlockValidationState consensus_invalid;
    consensus_invalid.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "test-invalid-block");
    struct TestCase {
        ConnectionType connection;
        bool via_compact_block;
        bool optimistic_reconstruction;
        bool disconnect;
    };
    NodeId next_id{0};
    for (const auto& test : {
             TestCase{ConnectionType::OUTBOUND_FULL_RELAY, false, false, true},
             TestCase{ConnectionType::INBOUND, false, false, false},
             TestCase{ConnectionType::MANUAL, false, false, false},
             TestCase{ConnectionType::OUTBOUND_FULL_RELAY, true, false, false},
             TestCase{ConnectionType::OUTBOUND_FULL_RELAY, true, true, false},
         }) {
        auto& source{AddPeer(next_id++, ConnectionType::OUTBOUND_FULL_RELAY)};
        auto& duplicate{AddPeer(next_id++, test.connection)};
        std::promise<BlockProcessingResult> source_completion, duplicate_completion;
        Peerman().UnitTestBlockProcessing(source.GetId(), block->GetHash(), source_completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
        Peerman().UnitTestBlockProcessing(duplicate.GetId(), block->GetHash(), duplicate_completion.get_future(), test.via_compact_block, test.optimistic_reconstruction);

        // A cached-invalid callback must neither punish nor erase a different source.
        m_node.validation_signals->BlockChecked(block, cached_invalid);
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
        Connman().ProcessMessagesOnce(source);
        BOOST_CHECK(!source.fDisconnect);
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(duplicate, NetMsg::Make(NetMsgType::PING, uint64_t{42})));
        duplicate_completion.set_value({.cached_invalid = true});
        ProcessBlockCompletions();
        Connman().ProcessMessagesOnce(duplicate);
        BOOST_CHECK_EQUAL(duplicate.fDisconnect, test.disconnect);
        auto message{duplicate.PollMessage()};
        BOOST_CHECK_EQUAL(message.has_value(), test.disconnect);
        if (message) BOOST_CHECK_EQUAL(message->first.m_type, NetMsgType::PING);

        // Normal validation can still find the source after the duplicate completes.
        m_node.validation_signals->BlockChecked(block, consensus_invalid);
        source_completion.set_value({});
        ProcessBlockCompletions();
        Connman().ProcessMessagesOnce(source);
        BOOST_CHECK(source.fDisconnect);
        Peerman().ProcessPendingEvents();
        RemovePeer(duplicate);
        RemovePeer(source);
    }
}

BOOST_FIXTURE_TEST_CASE(pending_block_failed_future, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    for (const bool broken_promise : {true, false}) {
        auto& source{AddPeer(broken_promise ? 0 : 1)};
        {
            std::promise<BlockProcessingResult> completion;
            Peerman().UnitTestBlockProcessing(source.GetId(), m_node.chainman->GetParams().GenesisBlock().GetHash(), completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
            if (!broken_promise) {
                struct UnexpectedException {};
                completion.set_exception(std::make_exception_ptr(UnexpectedException{}));
            }
        }
        ProcessBlockCompletions();
        BOOST_CHECK(source.fDisconnect);
        BOOST_CHECK(source.m_last_block_time.load() == 0s);
        Peerman().ProcessPendingEvents();
        RemovePeer(source);
    }
}

BOOST_AUTO_TEST_CASE(pending_block_marker_after_teardown)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    CNode source{0, /*sock=*/nullptr, CAddress(LookupNumeric("127.0.0.1", 18444), NODE_NONE),
        /*nKeyedNetGroupIn=*/0, /*nLocalHostNonceIn=*/0, CAddress(), /*addrNameIn=*/"",
        ConnectionType::INBOUND, /*inbound_onion=*/false, /*network_key=*/0};
    m_node.peerman->InitializeNode(source, ServiceFlags(NODE_NETWORK | NODE_WITNESS));
    std::promise<BlockProcessingResult> completion;
    m_node.peerman->UnitTestBlockProcessing(source.GetId(), m_node.chainman->GetParams().GenesisBlock().GetHash(), completion.get_future(), /*via_compact_block=*/false, /*optimistic_reconstruction=*/false);
    completion.set_value({});

    // Shutdown may leave a queued marker to be flushed after both owners are gone.
    m_node.chainman->StopBlockProcessing();
    m_node.scheduler->stop();
    m_node.validation_signals->FlushBackgroundCallbacks();
    m_node.peerman->ProcessPendingEvents();
    m_node.peerman->ProcessPendingEvents();
    BOOST_CHECK_EQUAL(m_node.validation_signals->CallbacksPending(), 1);
    m_node.peerman->FinalizeNode(source);
    m_node.peerman.reset();
    m_node.connman.reset();
    m_node.validation_signals->FlushBackgroundCallbacks();
    BOOST_CHECK_EQUAL(m_node.validation_signals->CallbacksPending(), 0);
}

BOOST_FIXTURE_TEST_CASE(worker_processes_peers_and_disconnected_sources, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& chainman{*m_node.chainman};
    auto& first{AddPeer(0)};
    auto& second{AddPeer(1)};
    auto& other{AddPeer(2)};
    const auto [primer, primer_index]{PrepareHeader()};
    BlockValidationState primer_state;
    BOOST_REQUIRE(chainman.ProcessNewBlock(primer, primer_state, true, true).get().new_block);
    BlockWorkerGate gate{chainman};
    gate.Wait();

    const auto [first_block, first_index]{PrepareHeader()};
    const auto [second_block, second_index]{PrepareHeader(first_block->nNonce + 1)};
    BOOST_REQUIRE(first_block->GetHash() != second_block->GetHash());
    for (const auto& [peer, block] : {std::pair{&first, first_block}, std::pair{&second, second_block}}) {
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(*peer, NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(*block))));
        Connman().ProcessMessagesOnce(*peer);
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(*peer, NetMsg::Make(NetMsgType::GETADDR)));
        BOOST_CHECK(!Connman().ProcessMessagesOnce(*peer));
        BOOST_CHECK(!Stats(peer->GetId()).m_addr_relay_enabled);
    }
    BOOST_CHECK(WITH_LOCK(cs_main, return !(first_index->nStatus & BLOCK_HAVE_DATA)));
    BOOST_CHECK(WITH_LOCK(cs_main, return !(second_index->nStatus & BLOCK_HAVE_DATA)));
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::GETADDR)));
    Connman().ProcessMessagesOnce(other);
    BOOST_CHECK(Stats(other.GetId()).m_addr_relay_enabled);

    // The submitting peer's CNode can disappear while its admitted work remains.
    BOOST_REQUIRE(Peerman().FetchBlock(other.GetId(), *second_index));
    RemovePeer(second);
    BOOST_CHECK_EQUAL(Stats(other.GetId()).vHeightInFlight.size(), 1);
    BlockValidationState duplicate_state;
    auto duplicate{chainman.ProcessNewBlock(second_block, duplicate_state, true, true)};
    BOOST_CHECK(duplicate.wait_for(0s) == std::future_status::timeout);
    gate.Open();
    BOOST_REQUIRE(duplicate.wait_for(30s) == std::future_status::ready);
    const auto result{duplicate.get()};
    BOOST_CHECK(result.processing_success && !result.new_block);
    ProcessBlockCompletions();
    BOOST_CHECK(Stats(other.GetId()).vHeightInFlight.empty());
    BOOST_CHECK(WITH_LOCK(cs_main, return first_index->nStatus & BLOCK_HAVE_DATA));
    BOOST_CHECK(WITH_LOCK(cs_main, return second_index->nStatus & BLOCK_HAVE_DATA));
    Connman().ProcessMessagesOnce(first);
    BOOST_CHECK(Stats(first.GetId()).m_addr_relay_enabled);
}

BOOST_FIXTURE_TEST_CASE(worker_pending_block_not_downloaded, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& chainman{*m_node.chainman};
    FakeNodeClock clock{std::chrono::seconds{chainman.GetParams().GenesisBlock().nTime} + 1h};
    auto& source{AddPeer(0)};
    auto& other{AddPeer(1)};
    const auto [primer, primer_index]{PrepareHeader()};
    BlockValidationState primer_state;
    BOOST_REQUIRE(chainman.ProcessNewBlock(primer, primer_state, true, true).get().new_block);
    BlockWorkerGate gate{chainman};
    gate.Wait();

    const auto [block, index]{PrepareHeader()};
    BOOST_REQUIRE(Peerman().FetchBlock(source.GetId(), *index));
    Connman().FlushSendBuffer(source);
    source.fPauseSend = false;
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(*block))));
    Connman().ProcessMessagesOnce(source);
    BOOST_CHECK(Stats(source.GetId()).vHeightInFlight.empty());
    BOOST_CHECK(WITH_LOCK(cs_main, return !(index->nStatus & BLOCK_HAVE_DATA)));

    const std::vector<CBlock> headers{CBlock{static_cast<const CBlockHeader&>(*block)}};
    for (const bool disconnect : {false, true}) {
        if (disconnect) RemovePeer(source);
        // Neither headers direct fetch nor the send loop should request admitted work.
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(headers))));
        Connman().ProcessMessagesOnce(other);
        BOOST_REQUIRE(Stats(other.GetId()).vHeightInFlight.empty());
        Peerman().SendMessages(other);
        BOOST_REQUIRE(Stats(other.GetId()).vHeightInFlight.empty());
        Connman().FlushSendBuffer(other);
        other.fPauseSend = false;
    }

    gate.Open();
    WaitForBlockProcessing();
    ProcessBlockCompletions();
    BOOST_CHECK(WITH_LOCK(cs_main, return index->nStatus & BLOCK_HAVE_DATA));

    // A new block remains eligible for automatic download.
    const auto [next_block, next_index]{PrepareHeader()};
    const std::vector<CBlock> next_headers{CBlock{static_cast<const CBlockHeader&>(*next_block)}};
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(next_headers))));
    Connman().ProcessMessagesOnce(other);
    BOOST_CHECK_EQUAL(Stats(other.GetId()).vHeightInFlight.size(), 1);
}

BOOST_FIXTURE_TEST_CASE(worker_pending_block_does_not_start_stall, PendingBlockTestingSetup)
{
    CheckStallingWithPendingBlock(/*pending_first=*/true);
}

BOOST_FIXTURE_TEST_CASE(worker_pending_block_preserves_remote_stall, PendingBlockTestingSetup)
{
    CheckStallingWithPendingBlock(/*pending_first=*/false);
}

BOOST_FIXTURE_TEST_CASE(download_window_advance_does_not_start_stall, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& chainman{*m_node.chainman};
    FakeNodeClock clock{chainman.GetParams().GenesisBlock().Time() + 24h};
    const auto blocks{PrepareStallingBlocks()};
    auto& staller{AddPeer(0, ConnectionType::OUTBOUND_FULL_RELAY)};
    auto& other{AddPeer(1, ConnectionType::OUTBOUND_FULL_RELAY)};
    const auto* withheld_index{WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(blocks[500]->GetHash()))};
    BOOST_REQUIRE(Peerman().FetchBlock(staller.GetId(), *withheld_index));

    // Stop between storage and activation, when downloaded blocks can advance
    // the peer's last common block during the scan, ahead of the active tip.
    {
        LOCK(cs_main);
        BlockValidationState state;
        BOOST_REQUIRE(chainman.AcceptBlock(blocks.front(), state, nullptr, true, nullptr, nullptr, true));
        BOOST_REQUIRE_EQUAL(chainman.ActiveHeight(), 0);
        BOOST_REQUIRE(withheld_index->pprev->HaveNumChainTxs());
    }

    const std::vector<CBlock> headers{CBlock{static_cast<const CBlockHeader&>(*blocks.back())}};
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(headers))));
    Connman().ProcessMessagesOnce(other);
    Peerman().SendMessages(other);
    BOOST_CHECK_EQUAL(Stats(other.GetId()).nCommonHeight, 500);

    clock += 3s;
    Peerman().SendMessages(staller);
    BOOST_CHECK(!staller.fDisconnect);

    // The next scan uses the advanced window and requests the next block.
    Peerman().SendMessages(other);
    const auto in_flight{Stats(other.GetId()).vHeightInFlight};
    BOOST_REQUIRE_EQUAL(in_flight.size(), 1);
    BOOST_CHECK_EQUAL(in_flight.front(), 1025);
}

BOOST_FIXTURE_TEST_CASE(worker_pending_compact_block_not_downloaded, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& chainman{*m_node.chainman};
    FakeNodeClock clock{std::chrono::seconds{chainman.GetParams().GenesisBlock().nTime} + 1h};

    // Mature a coinbase, then park the worker before submitting the next block.
    const auto funding_blocks{CreateBlockChain(COINBASE_MATURITY, chainman.GetParams())};
    for (const auto& funding_block : funding_blocks) {
        BlockValidationState state;
        BOOST_REQUIRE(chainman.ProcessNewBlock(funding_block, state, true, true).get().new_block);
    }
    BlockWorkerGate gate{chainman};
    gate.Wait();
    BOOST_REQUIRE_EQUAL(WITH_LOCK(cs_main, return chainman.ActiveHeight()), COINBASE_MATURITY);

    auto& source{AddPeer(0)};
    auto& other{AddPeer(1)};
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::SENDCMPCT, /*high_bandwidth=*/false, /*version=*/CMPCTBLOCKS_VERSION)));
    Connman().ProcessMessagesOnce(other);
    other.m_bip152_highbandwidth_to = true;
    Connman().FlushSendBuffer(other);
    other.fPauseSend = false;

    // Include a valid spend that is absent from our mempool and compact-block cache.
    CMutableTransaction spend;
    spend.vin.emplace_back(COutPoint{funding_blocks.front()->vtx[0]->GetHash(), 0});
    spend.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    spend.vout.push_back(funding_blocks.front()->vtx[0]->vout[0]);
    --spend.vout[0].nValue;
    auto block{PrepareBlock(m_node, {})};
    block->vtx.push_back(MakeTransactionRef(spend));
    node::RegenerateCommitments(*block, chainman);
    while (!CheckProofOfWork(block->GetHash(), block->nBits, chainman.GetConsensus())) ++block->nNonce;
    BlockValidationState header_state;
    const CBlockIndex* index{nullptr};
    const std::array<CBlockHeader, 1> headers{*block};
    BOOST_REQUIRE(chainman.ProcessNewBlockHeaders(headers, true, header_state, &index));
    BOOST_REQUIRE(index);

    const CBlockHeaderAndShortTxIDs compact{*block, /*nonce=*/0};
    PartiallyDownloadedBlock partial{m_node.mempool.get()};
    BOOST_REQUIRE(partial.InitData(compact, {}) == READ_STATUS_OK);
    BOOST_REQUIRE(!partial.IsTxAvailable(1));

    BOOST_REQUIRE(Peerman().FetchBlock(source.GetId(), *index));
    Connman().FlushSendBuffer(source);
    source.fPauseSend = false;
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(source, NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(*block))));
    Connman().ProcessMessagesOnce(source);
    BOOST_CHECK(Stats(source.GetId()).vHeightInFlight.empty());
    BOOST_REQUIRE(WITH_LOCK(cs_main, return !(index->nStatus & BLOCK_HAVE_DATA)));

    for (const bool disconnect : {false, true}) {
        if (disconnect) RemovePeer(source);
        BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::CMPCTBLOCK, compact)));
        Connman().ProcessMessagesOnce(other);
        BOOST_CHECK(Stats(other.GetId()).vHeightInFlight.empty());
        BOOST_CHECK(!HasSendData(other));
    }

    // A different, unadmitted block can still request its missing transaction while the worker is parked.
    CBlock alternative{*block};
    ++alternative.nNonce;
    while (!CheckProofOfWork(alternative.GetHash(), alternative.nBits, chainman.GetConsensus())) ++alternative.nNonce;
    const CBlockHeaderAndShortTxIDs alternative_compact{alternative, /*nonce=*/0};
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(other, NetMsg::Make(NetMsgType::CMPCTBLOCK, alternative_compact)));
    Connman().ProcessMessagesOnce(other);
    BOOST_CHECK_EQUAL(Stats(other.GetId()).vHeightInFlight.size(), 1);
    {
        LOCK(other.cs_vSend);
        const auto& [bytes, more, type] = other.m_transport->GetBytesToSend(false);
        if (!bytes.empty()) {
            BOOST_CHECK_EQUAL(type, NetMsgType::GETBLOCKTXN);
        } else {
            BOOST_REQUIRE_EQUAL(other.vSendMsg.size(), 1);
            BOOST_CHECK_EQUAL(other.vSendMsg.front().m_type, NetMsgType::GETBLOCKTXN);
        }
    }

    gate.Open();
    WaitForBlockProcessing();
    ProcessBlockCompletions();
    BOOST_CHECK(WITH_LOCK(cs_main, return index->nStatus & BLOCK_HAVE_DATA));
    BOOST_CHECK(WITH_LOCK(cs_main, return chainman.ActiveTip()->GetBlockHash()) == block->GetHash());
}

BOOST_FIXTURE_TEST_CASE(worker_admission_returns_immediate_outcomes, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& chainman{*m_node.chainman};
    const auto [primer, primer_index]{PrepareHeader()};
    BlockValidationState primer_state;
    BOOST_REQUIRE(chainman.ProcessNewBlock(primer, primer_state, true, true).get().new_block);
    BlockWorkerGate gate{chainman};
    gate.Wait();

    BlockValidationState duplicate_state;
    auto duplicate{chainman.ProcessNewBlock(primer, duplicate_state, true, true)};
    BOOST_REQUIRE(duplicate.wait_for(0s) == std::future_status::ready);
    const auto duplicate_result{duplicate.get()};
    BOOST_CHECK(duplicate_state.IsValid());
    BOOST_CHECK(duplicate_result.processing_success && !duplicate_result.new_block);

    const auto [block, index]{PrepareHeader()};
    auto mutated{std::make_shared<CBlock>(*block)};
    mutated->m_validation_cache = {};
    CMutableTransaction coinbase{*mutated->vtx[0]};
    ++coinbase.vout[0].nValue;
    mutated->vtx[0] = MakeTransactionRef(std::move(coinbase));
    BlockValidationState invalid_state;
    auto invalid{chainman.ProcessNewBlock(mutated, invalid_state, true, true)};
    BOOST_REQUIRE(invalid.wait_for(0s) == std::future_status::ready);
    BOOST_CHECK(invalid_state.GetResult() == BlockValidationResult::BLOCK_MUTATED);
    BOOST_CHECK(!invalid.get().processing_success);

    BlockValidationState admitted_state;
    auto admitted{chainman.ProcessNewBlock(block, admitted_state, true, true)};
    BOOST_CHECK(admitted_state.IsValid());
    BOOST_CHECK(admitted.wait_for(0s) == std::future_status::timeout);
    chainman.InterruptBlockProcessing();
    BlockValidationState stopped_state;
    auto stopped{chainman.ProcessNewBlock(block, stopped_state, true, true)};
    BOOST_REQUIRE(stopped.wait_for(0s) == std::future_status::ready);
    BOOST_CHECK(stopped_state.IsError());
    BOOST_CHECK(!stopped.get().processing_success);
    gate.Open();
    BOOST_REQUIRE(admitted.wait_for(30s) == std::future_status::ready);
    BOOST_CHECK(admitted.get().new_block);
    BOOST_CHECK(admitted_state.IsValid());
}

BOOST_FIXTURE_TEST_CASE(worker_initial_failure_punishes_its_sender, PendingBlockTestingSetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    auto& chainman{*m_node.chainman};
    auto& good_peer{AddPeer(0)};
    auto& bad_peer{AddPeer(1)};
    const auto [primer, primer_index]{PrepareHeader()};
    BlockValidationState primer_state;
    BOOST_REQUIRE(chainman.ProcessNewBlock(primer, primer_state, true, true).get().new_block);
    BlockWorkerGate gate{chainman};
    gate.Wait();

    const auto [block, index]{PrepareHeader()};
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(good_peer, NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(*block))));
    Connman().ProcessMessagesOnce(good_peer);
    auto mutated{std::make_shared<CBlock>(*block)};
    mutated->m_validation_cache = {};
    CMutableTransaction coinbase{*mutated->vtx[0]};
    ++coinbase.vout[0].nValue;
    mutated->vtx[0] = MakeTransactionRef(std::move(coinbase));
    BOOST_REQUIRE(Connman().ReceiveMsgFrom(bad_peer, NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(*mutated))));
    Connman().ProcessMessagesOnce(bad_peer);
    Connman().ProcessMessagesOnce(bad_peer);
    BOOST_CHECK(bad_peer.fDisconnect);
    BOOST_CHECK(!good_peer.fDisconnect);

    BlockValidationState state;
    auto completion{chainman.ProcessNewBlock(block, state, true, true)};
    gate.Open();
    BOOST_REQUIRE(completion.wait_for(30s) == std::future_status::ready);
    BOOST_CHECK(completion.get().processing_success);
    ProcessBlockCompletions();
    Connman().ProcessMessagesOnce(good_peer);
    BOOST_CHECK(!good_peer.fDisconnect);
    BOOST_CHECK(WITH_LOCK(cs_main, return index->nStatus & BLOCK_HAVE_DATA));
}

BOOST_FIXTURE_TEST_CASE(worker_queued_duplicate_rejection_punishes_each_sender, PendingBlockTestingSetup)
{
    CheckQueuedDuplicateBlock(/*invalid=*/true);
}

BOOST_FIXTURE_TEST_CASE(worker_queued_valid_duplicate_resumes_both_senders, PendingBlockTestingSetup)
{
    CheckQueuedDuplicateBlock(/*invalid=*/false);
}

BOOST_AUTO_TEST_SUITE_END()
