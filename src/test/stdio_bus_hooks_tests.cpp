// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <node/stdio_bus_hooks.h>
#include <node/stdio_bus_observer.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/time.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

using namespace node;

BOOST_FIXTURE_TEST_SUITE(stdio_bus_hooks_tests, BasicTestingSetup)

// ============================================================================
// Test: NoOpStdioBusHooks does nothing and is always safe
// ============================================================================

BOOST_AUTO_TEST_CASE(noop_hooks_disabled)
{
    NoOpStdioBusHooks hooks;
    BOOST_CHECK(!hooks.Enabled());
    BOOST_CHECK(hooks.ShadowMode());
}

BOOST_AUTO_TEST_CASE(noop_hooks_safe_to_call)
{
    NoOpStdioBusHooks hooks;
    
    // All hook calls should be safe no-ops
    MessageEvent msg_ev{.peer_id = 1, .msg_type = "headers", .size_bytes = 100, .received_us = 0};
    hooks.OnMessage(msg_ev);
    
    HeadersEvent hdr_ev{.peer_id = 1, .count = 10, .first_prev_hash = uint256::ZERO, .received_us = 0};
    hooks.OnHeaders(hdr_ev);
    
    BlockReceivedEvent blk_recv_ev{.peer_id = 1, .hash = uint256::ZERO, .height = 100, .size_bytes = 1000, .tx_count = 5, .received_us = 0};
    hooks.OnBlockReceived(blk_recv_ev);
    
    BlockValidatedEvent blk_val_ev{.hash = uint256::ZERO, .height = 100, .tx_count = 5, .received_us = 0, .validated_us = 1000, .accepted = true, .reject_reason = {}};
    hooks.OnBlockValidated(blk_val_ev);
    
    TxAdmissionEvent tx_ev{.txid = uint256::ZERO, .wtxid = uint256::ZERO, .size_bytes = 250, .received_us = 0, .processed_us = 100, .accepted = true, .reject_reason = {}};
    hooks.OnTxAdmission(tx_ev);
    
    MsgHandlerLoopEvent loop_ev{.iteration = 1, .start_us = 0, .end_us = 100, .messages_processed = 5, .had_work = true};
    hooks.OnMsgHandlerLoop(loop_ev);
    
    RpcCallEvent rpc_ev{.method = "getblockchaininfo", .start_us = 0, .end_us = 500, .success = true};
    hooks.OnRpcCall(rpc_ev);
    
    // If we get here without crash/exception, test passes
    BOOST_CHECK(true);
}

// ============================================================================
// Test: StdioBusMode parsing
// ============================================================================

BOOST_AUTO_TEST_CASE(parse_stdio_bus_mode)
{
    BOOST_CHECK(ParseStdioBusMode("off") == StdioBusMode::Off);
    BOOST_CHECK(ParseStdioBusMode("shadow") == StdioBusMode::Shadow);
    BOOST_CHECK(ParseStdioBusMode("active") == StdioBusMode::Active);
    
    // Invalid values default to Off
    BOOST_CHECK(ParseStdioBusMode("") == StdioBusMode::Off);
    BOOST_CHECK(ParseStdioBusMode("invalid") == StdioBusMode::Off);
    BOOST_CHECK(ParseStdioBusMode("SHADOW") == StdioBusMode::Off); // case sensitive
}

BOOST_AUTO_TEST_CASE(stdio_bus_mode_to_string)
{
    BOOST_CHECK(StdioBusModeToString(StdioBusMode::Off) == "off");
    BOOST_CHECK(StdioBusModeToString(StdioBusMode::Shadow) == "shadow");
    BOOST_CHECK(StdioBusModeToString(StdioBusMode::Active) == "active");
}

// ============================================================================
// Test: GetMonotonicTimeUs returns increasing values
// ============================================================================

BOOST_AUTO_TEST_CASE(monotonic_time_increases)
{
    int64_t t1 = GetMonotonicTimeUs();
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    int64_t t2 = GetMonotonicTimeUs();
    
    BOOST_CHECK(t2 > t1);
    BOOST_CHECK(t2 - t1 >= 100); // At least 100us passed
}

// ============================================================================
// Test: Recording hooks for verification
// ============================================================================

class RecordingStdioBusHooks : public StdioBusHooks {
public:
    bool Enabled() const override { return m_enabled; }
    bool ShadowMode() const override { return true; }
    
    void OnMessage(const MessageEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push_back(ev);
    }
    
    void OnHeaders(const HeadersEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_headers.push_back(ev);
    }
    
    void OnBlockReceived(const BlockReceivedEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_blocks_received.push_back(ev);
    }
    
    void OnBlockValidated(const BlockValidatedEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_blocks_validated.push_back(ev);
    }
    
    void OnTxAdmission(const TxAdmissionEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tx_admissions.push_back(ev);
    }
    
    void OnMsgHandlerLoop(const MsgHandlerLoopEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loop_events.push_back(ev);
    }
    
    void OnRpcCall(const RpcCallEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rpc_calls.push_back(ev);
    }
    
    // Phase 2: Block Processing Delay Events
    void OnBlockAnnounce(const BlockAnnounceEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_block_announces.push_back(ev);
    }
    
    void OnBlockRequestDecision(const BlockRequestDecisionEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_block_request_decisions.push_back(ev);
    }
    
    void OnBlockInFlight(const BlockInFlightEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_block_in_flights.push_back(ev);
    }
    
    void OnStallerDetected(const StallerDetectedEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_staller_events.push_back(ev);
    }
    
    void OnCompactBlockDecision(const CompactBlockDecisionEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_compact_block_decisions.push_back(ev);
    }
    
    void OnBlockSourceResolved(const BlockSourceResolvedEvent& ev) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_block_source_resolved.push_back(ev);
    }
    
    // Accessors
    size_t MessageCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_messages.size(); }
    size_t HeadersCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_headers.size(); }
    size_t BlocksReceivedCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_blocks_received.size(); }
    size_t BlocksValidatedCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_blocks_validated.size(); }
    size_t TxAdmissionsCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_tx_admissions.size(); }
    size_t BlockAnnouncesCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_block_announces.size(); }
    size_t StallerEventsCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_staller_events.size(); }
    
    MessageEvent GetMessage(size_t i) const { std::lock_guard<std::mutex> lock(m_mutex); return m_messages.at(i); }
    HeadersEvent GetHeaders(size_t i) const { std::lock_guard<std::mutex> lock(m_mutex); return m_headers.at(i); }
    BlockValidatedEvent GetBlockValidated(size_t i) const { std::lock_guard<std::mutex> lock(m_mutex); return m_blocks_validated.at(i); }
    BlockAnnounceEvent GetBlockAnnounce(size_t i) const { std::lock_guard<std::mutex> lock(m_mutex); return m_block_announces.at(i); }
    
    bool m_enabled{true};
    
private:
    mutable std::mutex m_mutex;
    std::vector<MessageEvent> m_messages;
    std::vector<HeadersEvent> m_headers;
    std::vector<BlockReceivedEvent> m_blocks_received;
    std::vector<BlockValidatedEvent> m_blocks_validated;
    std::vector<TxAdmissionEvent> m_tx_admissions;
    std::vector<MsgHandlerLoopEvent> m_loop_events;
    std::vector<RpcCallEvent> m_rpc_calls;
    // Phase 2
    std::vector<BlockAnnounceEvent> m_block_announces;
    std::vector<BlockRequestDecisionEvent> m_block_request_decisions;
    std::vector<BlockInFlightEvent> m_block_in_flights;
    std::vector<StallerDetectedEvent> m_staller_events;
    std::vector<CompactBlockDecisionEvent> m_compact_block_decisions;
    std::vector<BlockSourceResolvedEvent> m_block_source_resolved;
};

BOOST_AUTO_TEST_CASE(recording_hooks_capture_events)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    
    // Fire some events
    MessageEvent msg1{.peer_id = 1, .msg_type = "headers", .size_bytes = 100, .received_us = 1000};
    MessageEvent msg2{.peer_id = 2, .msg_type = "block", .size_bytes = 500000, .received_us = 2000};
    hooks->OnMessage(msg1);
    hooks->OnMessage(msg2);
    
    HeadersEvent hdr{.peer_id = 1, .count = 2000, .first_prev_hash = uint256::ONE, .received_us = 1500};
    hooks->OnHeaders(hdr);
    
    BOOST_CHECK_EQUAL(hooks->MessageCount(), 2);
    BOOST_CHECK_EQUAL(hooks->HeadersCount(), 1);
    
    // Verify captured data
    auto captured_msg1 = hooks->GetMessage(0);
    BOOST_CHECK_EQUAL(captured_msg1.peer_id, 1);
    BOOST_CHECK_EQUAL(captured_msg1.msg_type, "headers");
    BOOST_CHECK_EQUAL(captured_msg1.size_bytes, 100);
    
    auto captured_hdr = hooks->GetHeaders(0);
    BOOST_CHECK_EQUAL(captured_hdr.count, 2000);
    BOOST_CHECK(captured_hdr.first_prev_hash == uint256::ONE);
}

BOOST_AUTO_TEST_CASE(disabled_hooks_not_called)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    hooks->m_enabled = false;
    
    BOOST_CHECK(!hooks->Enabled());
    
    // Even if we call hooks, they should check Enabled() first
    // This test verifies the pattern used in net_processing.cpp
    if (hooks->Enabled()) {
        MessageEvent msg{.peer_id = 1, .msg_type = "test", .size_bytes = 10, .received_us = 0};
        hooks->OnMessage(msg);
    }
    
    BOOST_CHECK_EQUAL(hooks->MessageCount(), 0);
}

// ============================================================================
// Test: Thread safety of hooks
// ============================================================================

BOOST_AUTO_TEST_CASE(hooks_thread_safe)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    constexpr int NUM_THREADS = 4;
    constexpr int EVENTS_PER_THREAD = 1000;
    
    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&hooks, t]() {
            for (int i = 0; i < EVENTS_PER_THREAD; ++i) {
                MessageEvent ev{
                    .peer_id = t,
                    .msg_type = "test",
                    .size_bytes = static_cast<size_t>(i),
                    .received_us = GetMonotonicTimeUs()
                };
                hooks->OnMessage(ev);
            }
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    BOOST_CHECK_EQUAL(hooks->MessageCount(), NUM_THREADS * EVENTS_PER_THREAD);
}

// ============================================================================
// Test: StdioBusValidationObserver
// ============================================================================

// Test wrapper to expose protected methods for testing
class TestableStdioBusValidationObserver : public StdioBusValidationObserver {
public:
    using StdioBusValidationObserver::StdioBusValidationObserver;
    using StdioBusValidationObserver::BlockChecked;
};

BOOST_FIXTURE_TEST_CASE(validation_observer_disabled_when_hooks_disabled, ChainTestingSetup)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    hooks->m_enabled = false;
    
    TestableStdioBusValidationObserver observer(hooks);
    BOOST_CHECK(!observer.Enabled());
}

BOOST_FIXTURE_TEST_CASE(validation_observer_enabled_when_hooks_enabled, ChainTestingSetup)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    hooks->m_enabled = true;
    
    TestableStdioBusValidationObserver observer(hooks);
    BOOST_CHECK(observer.Enabled());
}

BOOST_FIXTURE_TEST_CASE(validation_observer_block_checked_accepted, ChainTestingSetup)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    TestableStdioBusValidationObserver observer(hooks);
    
    // Create a simple block
    CBlock block;
    block.nVersion = 1;
    block.hashPrevBlock = uint256::ZERO;
    block.nTime = 1234567890;
    block.nBits = 0x1d00ffff;
    block.nNonce = 0;
    
    // Simulate accepted block
    BlockValidationState state;
    observer.BlockChecked(std::make_shared<const CBlock>(block), state);
    
    BOOST_CHECK_EQUAL(hooks->BlocksValidatedCount(), 1);
    auto ev = hooks->GetBlockValidated(0);
    BOOST_CHECK(ev.accepted);
    BOOST_CHECK(ev.reject_reason.empty());
    BOOST_CHECK(ev.hash == block.GetHash());
}

BOOST_FIXTURE_TEST_CASE(validation_observer_block_checked_rejected, ChainTestingSetup)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    TestableStdioBusValidationObserver observer(hooks);
    
    CBlock block;
    block.nVersion = 1;
    
    // Simulate rejected block
    BlockValidationState state;
    state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-duplicate");
    observer.BlockChecked(std::make_shared<const CBlock>(block), state);
    
    BOOST_CHECK_EQUAL(hooks->BlocksValidatedCount(), 1);
    auto ev = hooks->GetBlockValidated(0);
    BOOST_CHECK(!ev.accepted);
    BOOST_CHECK(!ev.reject_reason.empty());
}

// ============================================================================
// Test: Event struct field validation
// ============================================================================

BOOST_AUTO_TEST_CASE(message_event_fields)
{
    MessageEvent ev{
        .peer_id = 42,
        .msg_type = "headers",
        .size_bytes = 162000,
        .received_us = 1234567890123
    };
    
    BOOST_CHECK_EQUAL(ev.peer_id, 42);
    BOOST_CHECK_EQUAL(ev.msg_type, "headers");
    BOOST_CHECK_EQUAL(ev.size_bytes, 162000);
    BOOST_CHECK_EQUAL(ev.received_us, 1234567890123);
}

BOOST_AUTO_TEST_CASE(block_validated_event_fields)
{
    uint256 hash = uint256::FromHex("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f").value();
    
    BlockValidatedEvent ev{
        .hash = hash,
        .height = 800000,
        .tx_count = 3500,
        .received_us = 1000000,
        .validated_us = 1500000,
        .accepted = true,
        .reject_reason = {}
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.height, 800000);
    BOOST_CHECK_EQUAL(ev.tx_count, 3500);
    BOOST_CHECK_EQUAL(ev.validated_us - ev.received_us, 500000); // 500ms validation time
    BOOST_CHECK(ev.accepted);
}

BOOST_AUTO_TEST_CASE(tx_admission_event_fields)
{
    uint256 txid = uint256::FromHex("0000000000000000000000000000000000000000000000000000000000abc123").value();
    uint256 wtxid = uint256::FromHex("0000000000000000000000000000000000000000000000000000000000def456").value();
    
    TxAdmissionEvent ev{
        .txid = txid,
        .wtxid = wtxid,
        .size_bytes = 250,
        .received_us = 1000,
        .processed_us = 1100,
        .accepted = false,
        .reject_reason = "insufficient fee"
    };
    
    BOOST_CHECK(ev.txid == txid);
    BOOST_CHECK(ev.wtxid == wtxid);
    BOOST_CHECK_EQUAL(ev.size_bytes, 250);
    BOOST_CHECK(!ev.accepted);
    BOOST_CHECK_EQUAL(ev.reject_reason, "insufficient fee");
}

// ============================================================================
// Test: Latency measurement helper
// ============================================================================

BOOST_AUTO_TEST_CASE(latency_measurement)
{
    int64_t start = GetMonotonicTimeUs();
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    int64_t end = GetMonotonicTimeUs();
    int64_t latency_us = end - start;
    
    // Should be at least 1ms (1000us)
    BOOST_CHECK(latency_us >= 1000);
    // Should be less than 100ms (sanity check)
    BOOST_CHECK(latency_us < 100000);
}

// ============================================================================
// Phase 2: Block Processing Delay Events Tests (#21803)
// ============================================================================

BOOST_AUTO_TEST_CASE(block_announce_event_fields)
{
    uint256 hash = uint256::FromHex("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f").value();
    
    BlockAnnounceEvent ev{
        .hash = hash,
        .peer_id = 42,
        .via = BlockAnnounceVia::Headers,
        .chainwork_delta = 12345678,
        .height = 800000,
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.peer_id, 42);
    BOOST_CHECK(ev.via == BlockAnnounceVia::Headers);
    BOOST_CHECK_EQUAL(ev.chainwork_delta, 12345678);
    BOOST_CHECK_EQUAL(ev.height, 800000);
}

BOOST_AUTO_TEST_CASE(block_announce_via_enum)
{
    BOOST_CHECK(static_cast<int>(BlockAnnounceVia::Headers) == 0);
    BOOST_CHECK(static_cast<int>(BlockAnnounceVia::CompactBlock) == 1);
    BOOST_CHECK(static_cast<int>(BlockAnnounceVia::Inv) == 2);
}

BOOST_AUTO_TEST_CASE(block_request_decision_event_fields)
{
    uint256 hash = uint256::FromHex("0000000000000000000000000000000000000000000000000000000000abc123").value();
    
    BlockRequestDecisionEvent ev{
        .hash = hash,
        .peer_id = 10,
        .reason = BlockRequestReason::NewBlock,
        .is_preferred_peer = true,
        .first_in_flight = true,
        .already_in_flight = 0,
        .can_direct_fetch = true,
        .is_limited_peer = false,
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.peer_id, 10);
    BOOST_CHECK(ev.reason == BlockRequestReason::NewBlock);
    BOOST_CHECK(ev.is_preferred_peer);
    BOOST_CHECK(ev.first_in_flight);
    BOOST_CHECK_EQUAL(ev.already_in_flight, 0);
}

BOOST_AUTO_TEST_CASE(block_in_flight_event_fields)
{
    uint256 hash = uint256::FromHex("0000000000000000000000000000000000000000000000000000000000def456").value();
    
    BlockInFlightEvent ev{
        .hash = hash,
        .peer_id = 5,
        .action = InFlightAction::Add,
        .inflight_count = 1,
        .peer_inflight_count = 3,
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.peer_id, 5);
    BOOST_CHECK(ev.action == InFlightAction::Add);
    BOOST_CHECK_EQUAL(ev.inflight_count, 1);
    BOOST_CHECK_EQUAL(ev.peer_inflight_count, 3);
}

BOOST_AUTO_TEST_CASE(staller_detected_event_fields)
{
    uint256 hash = uint256::FromHex("0000000000000000000000000000000000000000000000000000000000789abc").value();
    
    StallerDetectedEvent ev{
        .hash = hash,
        .staller_peer_id = 3,
        .waiting_peer_id = 7,
        .window_end_height = 800010,
        .stall_duration_us = 5000000, // 5 seconds
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.staller_peer_id, 3);
    BOOST_CHECK_EQUAL(ev.waiting_peer_id, 7);
    BOOST_CHECK_EQUAL(ev.window_end_height, 800010);
    BOOST_CHECK_EQUAL(ev.stall_duration_us, 5000000);
}

BOOST_AUTO_TEST_CASE(compact_block_decision_event_fields)
{
    uint256 hash = uint256::FromHex("00000000000000000000000000000000000000000000000000000000c0ac7123").value();
    
    CompactBlockDecisionEvent ev{
        .hash = hash,
        .peer_id = 15,
        .action = CompactBlockAction::GetBlockTxn,
        .missing_tx_count = 5,
        .first_in_flight = true,
        .is_highbandwidth = true,
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.peer_id, 15);
    BOOST_CHECK(ev.action == CompactBlockAction::GetBlockTxn);
    BOOST_CHECK_EQUAL(ev.missing_tx_count, 5);
    BOOST_CHECK(ev.first_in_flight);
    BOOST_CHECK(ev.is_highbandwidth);
}

BOOST_AUTO_TEST_CASE(block_source_resolved_event_fields)
{
    uint256 hash = uint256::FromHex("000000000000000000000000000000000000000000000000000000000e501ed9").value();
    
    BlockSourceResolvedEvent ev{
        .hash = hash,
        .source_peer_id = 20,
        .first_requested_peer_id = 15,
        .announce_to_receive_us = 2500000, // 2.5 seconds
        .request_to_receive_us = 1500000,  // 1.5 seconds
        .total_requests = 3,
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.source_peer_id, 20);
    BOOST_CHECK_EQUAL(ev.first_requested_peer_id, 15);
    BOOST_CHECK_EQUAL(ev.announce_to_receive_us, 2500000);
    BOOST_CHECK_EQUAL(ev.request_to_receive_us, 1500000);
    BOOST_CHECK_EQUAL(ev.total_requests, 3);
}

BOOST_AUTO_TEST_CASE(recording_hooks_capture_phase2_events)
{
    auto hooks = std::make_shared<RecordingStdioBusHooks>();
    
    // Fire Phase 2 events
    BlockAnnounceEvent announce_ev{
        .hash = uint256::ONE,
        .peer_id = 1,
        .via = BlockAnnounceVia::CompactBlock,
        .chainwork_delta = 1000,
        .height = 100,
        .timestamp_us = GetMonotonicTimeUs()
    };
    hooks->OnBlockAnnounce(announce_ev);
    
    StallerDetectedEvent staller_ev{
        .hash = uint256::ONE,
        .staller_peer_id = 2,
        .waiting_peer_id = 1,
        .window_end_height = 110,
        .stall_duration_us = 0,
        .timestamp_us = GetMonotonicTimeUs()
    };
    hooks->OnStallerDetected(staller_ev);
    
    BOOST_CHECK_EQUAL(hooks->BlockAnnouncesCount(), 1);
    BOOST_CHECK_EQUAL(hooks->StallerEventsCount(), 1);
    
    auto captured_announce = hooks->GetBlockAnnounce(0);
    BOOST_CHECK(captured_announce.hash == uint256::ONE);
    BOOST_CHECK(captured_announce.via == BlockAnnounceVia::CompactBlock);
}

BOOST_AUTO_TEST_CASE(noop_hooks_phase2_safe_to_call)
{
    NoOpStdioBusHooks hooks;
    
    // All Phase 2 hook calls should be safe no-ops
    BlockAnnounceEvent announce_ev{.hash = uint256::ZERO, .peer_id = 1, .via = BlockAnnounceVia::Headers, .chainwork_delta = 0, .height = 0, .timestamp_us = 0};
    hooks.OnBlockAnnounce(announce_ev);
    
    BlockRequestDecisionEvent request_ev{.hash = uint256::ZERO, .peer_id = 1, .reason = BlockRequestReason::NewBlock, .is_preferred_peer = false, .first_in_flight = true, .already_in_flight = 0, .can_direct_fetch = true, .is_limited_peer = false, .timestamp_us = 0};
    hooks.OnBlockRequestDecision(request_ev);
    
    BlockInFlightEvent inflight_ev{.hash = uint256::ZERO, .peer_id = 1, .action = InFlightAction::Add, .inflight_count = 1, .peer_inflight_count = 1, .timestamp_us = 0};
    hooks.OnBlockInFlight(inflight_ev);
    
    StallerDetectedEvent staller_ev{.hash = uint256::ZERO, .staller_peer_id = 1, .waiting_peer_id = 2, .window_end_height = 100, .stall_duration_us = 0, .timestamp_us = 0};
    hooks.OnStallerDetected(staller_ev);
    
    CompactBlockDecisionEvent compact_ev{.hash = uint256::ZERO, .peer_id = 1, .action = CompactBlockAction::Reconstruct, .missing_tx_count = 0, .first_in_flight = true, .is_highbandwidth = false, .timestamp_us = 0};
    hooks.OnCompactBlockDecision(compact_ev);
    
    BlockSourceResolvedEvent resolved_ev{.hash = uint256::ZERO, .source_peer_id = 1, .first_requested_peer_id = 1, .announce_to_receive_us = 1000, .request_to_receive_us = 500, .total_requests = 1, .timestamp_us = 0};
    hooks.OnBlockSourceResolved(resolved_ev);
    
    // If we get here without crash/exception, test passes
    BOOST_CHECK(true);
}

// ============================================================================
// Phase 2: SendMessages hook tests
// ============================================================================

BOOST_AUTO_TEST_CASE(block_request_reason_enum)
{
    BOOST_CHECK(static_cast<int>(BlockRequestReason::NewBlock) == 0);
    BOOST_CHECK(static_cast<int>(BlockRequestReason::Retry) == 1);
    BOOST_CHECK(static_cast<int>(BlockRequestReason::Hedge) == 2);
    BOOST_CHECK(static_cast<int>(BlockRequestReason::CompactFallback) == 3);
    BOOST_CHECK(static_cast<int>(BlockRequestReason::ParallelDownload) == 4);
}

BOOST_AUTO_TEST_CASE(inflight_action_enum)
{
    BOOST_CHECK(static_cast<int>(InFlightAction::Add) == 0);
    BOOST_CHECK(static_cast<int>(InFlightAction::Remove) == 1);
    BOOST_CHECK(static_cast<int>(InFlightAction::Timeout) == 2);
}

BOOST_AUTO_TEST_CASE(compact_block_action_enum)
{
    BOOST_CHECK(static_cast<int>(CompactBlockAction::Reconstruct) == 0);
    BOOST_CHECK(static_cast<int>(CompactBlockAction::GetBlockTxn) == 1);
    BOOST_CHECK(static_cast<int>(CompactBlockAction::GetData) == 2);
    BOOST_CHECK(static_cast<int>(CompactBlockAction::Wait) == 3);
    BOOST_CHECK(static_cast<int>(CompactBlockAction::Drop) == 4);
}

BOOST_AUTO_TEST_CASE(block_request_decision_parallel_download)
{
    // Test BlockRequestDecisionEvent with ParallelDownload reason (used in SendMessages)
    uint256 hash = uint256::FromHex("000000000000000000000000000000000000000000000000000000005e9d5123").value();
    
    BlockRequestDecisionEvent ev{
        .hash = hash,
        .peer_id = 25,
        .reason = BlockRequestReason::ParallelDownload,
        .is_preferred_peer = true,
        .first_in_flight = false,
        .already_in_flight = 2,
        .can_direct_fetch = true,
        .is_limited_peer = false,
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.peer_id, 25);
    BOOST_CHECK(ev.reason == BlockRequestReason::ParallelDownload);
    BOOST_CHECK(ev.is_preferred_peer);
    BOOST_CHECK(!ev.first_in_flight);
    BOOST_CHECK_EQUAL(ev.already_in_flight, 2);
    BOOST_CHECK(ev.can_direct_fetch);
    BOOST_CHECK(!ev.is_limited_peer);
}

BOOST_AUTO_TEST_CASE(block_inflight_timeout_event)
{
    // Test BlockInFlightEvent with Timeout action (used in SendMessages timeout branch)
    uint256 hash = uint256::FromHex("00000000000000000000000000000000000000000000000000000000710e0456").value();
    
    BlockInFlightEvent ev{
        .hash = hash,
        .peer_id = 30,
        .action = InFlightAction::Timeout,
        .inflight_count = 1,
        .peer_inflight_count = 5,
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.peer_id, 30);
    BOOST_CHECK(ev.action == InFlightAction::Timeout);
    BOOST_CHECK_EQUAL(ev.inflight_count, 1);
    BOOST_CHECK_EQUAL(ev.peer_inflight_count, 5);
}

BOOST_AUTO_TEST_CASE(staller_detected_timeout_disconnect)
{
    // Test StallerDetectedEvent for timeout disconnect (used in SendMessages stall timeout)
    uint256 hash = uint256::FromHex("00000000000000000000000000000000000000000000000057a11710e0000789").value();
    
    StallerDetectedEvent ev{
        .hash = hash,
        .staller_peer_id = 35,
        .waiting_peer_id = -1, // We are the staller being disconnected
        .window_end_height = 800050,
        .stall_duration_us = 10000000, // 10 seconds stall
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.staller_peer_id, 35);
    BOOST_CHECK_EQUAL(ev.waiting_peer_id, -1);
    BOOST_CHECK_EQUAL(ev.window_end_height, 800050);
    BOOST_CHECK_EQUAL(ev.stall_duration_us, 10000000);
}

BOOST_AUTO_TEST_CASE(staller_detected_stall_started)
{
    // Test StallerDetectedEvent for stall started (used in SendMessages stall start)
    uint256 hash = uint256::FromHex("0000000000000000000000000000000000000000000000057a11574700000123").value();
    
    StallerDetectedEvent ev{
        .hash = hash,
        .staller_peer_id = 40,
        .waiting_peer_id = 45, // Peer waiting for staller
        .window_end_height = 800060,
        .stall_duration_us = 0, // Just started
        .timestamp_us = GetMonotonicTimeUs()
    };
    
    BOOST_CHECK(ev.hash == hash);
    BOOST_CHECK_EQUAL(ev.staller_peer_id, 40);
    BOOST_CHECK_EQUAL(ev.waiting_peer_id, 45);
    BOOST_CHECK_EQUAL(ev.window_end_height, 800060);
    BOOST_CHECK_EQUAL(ev.stall_duration_us, 0);
}

BOOST_AUTO_TEST_SUITE_END()
