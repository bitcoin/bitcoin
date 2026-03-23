// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_STDIO_BUS_HOOKS_H
#define BITCOIN_NODE_STDIO_BUS_HOOKS_H

#include <cstdint>
#include <string>
#include <string_view>
#include <chrono>
#include <functional>
#include <memory>

#include <uint256.h>

namespace node {

/**
 * @brief Event types for stdio_bus hooks
 * 
 * These events are fired at key points in the P2P message processing
 * and validation pipeline. All hooks are read-only observers and
 * MUST NOT modify any state or affect consensus decisions.
 */

/** Message received event */
struct MessageEvent {
    int64_t peer_id;
    std::string msg_type;  // Owning copy for async safety
    size_t size_bytes;
    int64_t received_us;  // Monotonic timestamp in microseconds
};

/** Headers received event */
struct HeadersEvent {
    int64_t peer_id;
    size_t count;
    uint256 first_prev_hash;  // Owning copy for async safety
    int64_t received_us;
};

/** Block received event */
struct BlockReceivedEvent {
    int64_t peer_id;
    uint256 hash;  // Owning copy for async safety
    int height;  // -1 if unknown
    size_t size_bytes;
    size_t tx_count;
    int64_t received_us;
};

/** Block validated event */
struct BlockValidatedEvent {
    uint256 hash;  // Owning copy for async safety
    int height;
    size_t tx_count;
    int64_t received_us;
    int64_t validated_us;
    bool accepted;
    std::string reject_reason;  // Owning copy, empty if accepted
};

/** Transaction admission event */
struct TxAdmissionEvent {
    uint256 txid;   // Owning copy for async safety
    uint256 wtxid;  // Owning copy for async safety
    size_t size_bytes;
    int64_t received_us;
    int64_t processed_us;
    bool accepted;
    std::string reject_reason;  // Owning copy, empty if accepted
};

/** Message handler loop event */
struct MsgHandlerLoopEvent {
    int64_t iteration;
    int64_t start_us;
    int64_t end_us;
    int messages_processed;
    bool had_work;
};

/** RPC call event */
struct RpcCallEvent {
    std::string method;  // Owning copy for async safety
    int64_t start_us;
    int64_t end_us;
    bool success;
};

// ============================================================================
// Phase 2: Block Processing Delay Diagnostic Events (#21803)
// ============================================================================

/** Block announcement source */
enum class BlockAnnounceVia {
    Headers = 0,
    CompactBlock = 1,
    Inv = 2
};

/** Block announce event - when we learn about a new block */
struct BlockAnnounceEvent {
    uint256 hash;
    int64_t peer_id;
    BlockAnnounceVia via;
    int64_t chainwork_delta;  // Relative to our tip
    int height;
    int64_t timestamp_us;
};

/** Block request decision reason */
enum class BlockRequestReason {
    NewBlock = 0,           // First request for this block
    Retry = 1,              // Retry after timeout
    Hedge = 2,              // Hedging request to additional peer
    CompactFallback = 3,    // Fallback from compact block
    ParallelDownload = 4    // Parallel download slot
};

/** Block request decision event - when we decide to request a block */
struct BlockRequestDecisionEvent {
    uint256 hash;
    int64_t peer_id;
    BlockRequestReason reason;
    bool is_preferred_peer;
    bool first_in_flight;
    size_t already_in_flight;
    bool can_direct_fetch;
    bool is_limited_peer;
    int64_t timestamp_us;
};

/** Block in-flight change action */
enum class InFlightAction {
    Add = 0,
    Remove = 1,
    Timeout = 2
};

/** Block in-flight state change event */
struct BlockInFlightEvent {
    uint256 hash;
    int64_t peer_id;
    InFlightAction action;
    size_t inflight_count;      // Total in-flight for this block
    size_t peer_inflight_count; // In-flight for this peer
    int64_t timestamp_us;
};

/** Staller detected event - when download window is blocked */
struct StallerDetectedEvent {
    uint256 hash;
    int64_t staller_peer_id;
    int64_t waiting_peer_id;
    int window_end_height;
    int64_t stall_duration_us;
    int64_t timestamp_us;
};

/** Compact block decision action */
enum class CompactBlockAction {
    Reconstruct = 0,    // Successfully reconstructed from mempool
    GetBlockTxn = 1,    // Requesting missing transactions
    GetData = 2,        // Falling back to full block
    Wait = 3,           // Waiting for other peer
    Drop = 4            // Dropping (already have or invalid)
};

/** Compact block decision event */
struct CompactBlockDecisionEvent {
    uint256 hash;
    int64_t peer_id;
    CompactBlockAction action;
    size_t missing_tx_count;    // Transactions not in mempool
    bool first_in_flight;
    bool is_highbandwidth;
    int64_t timestamp_us;
};

/** Block source resolved event - when we finally get the block */
struct BlockSourceResolvedEvent {
    uint256 hash;
    int64_t source_peer_id;     // Peer that provided the block
    int64_t first_requested_peer_id;  // First peer we requested from
    int64_t announce_to_receive_us;   // Time from first announce to receive
    int64_t request_to_receive_us;    // Time from first request to receive
    size_t total_requests;      // Total requests made for this block
    int64_t timestamp_us;
};

/**
 * @brief stdio_bus mode enum
 */
enum class StdioBusMode {
    Off = 0,     ///< Disabled (default)
    Shadow = 1,  ///< Observe only, no behavior change
    Active = 2   ///< Enable optimizations (future)
};

/**
 * @brief Parse stdio_bus mode from string
 * @param str Mode string ("off", "shadow", "active")
 * @return Parsed mode, or Off if invalid
 */
inline StdioBusMode ParseStdioBusMode(std::string_view str) {
    if (str == "shadow") return StdioBusMode::Shadow;
    if (str == "active") return StdioBusMode::Active;
    return StdioBusMode::Off;
}

/**
 * @brief Convert stdio_bus mode to string
 */
inline std::string_view StdioBusModeToString(StdioBusMode mode) {
    switch (mode) {
        case StdioBusMode::Shadow: return "shadow";
        case StdioBusMode::Active: return "active";
        default: return "off";
    }
}

/**
 * @brief Abstract interface for stdio_bus hooks
 * 
 * This interface allows external observers to receive events from
 * the Bitcoin Core P2P and validation subsystems without modifying
 * their behavior.
 * 
 * CRITICAL REQUIREMENTS:
 * 1. All callbacks MUST be non-blocking (use bounded queue internally)
 * 2. All callbacks MUST NOT throw exceptions
 * 3. All callbacks MUST NOT modify any Bitcoin Core state
 * 4. All callbacks MUST NOT affect consensus decisions
 * 5. On any error, fail silently (fail-open)
 * 
 * Thread safety: Callbacks may be invoked from multiple threads.
 * Implementations must be thread-safe.
 */
class StdioBusHooks {
public:
    virtual ~StdioBusHooks() = default;

    /** Check if hooks are enabled */
    virtual bool Enabled() const = 0;

    /** Check if shadow mode (observe-only) is active */
    virtual bool ShadowMode() const { return true; }

    // ========== P2P Events ==========

    /** Called when any P2P message is received (before processing) */
    virtual void OnMessage(const MessageEvent& event) = 0;

    /** Called when HEADERS message is received */
    virtual void OnHeaders(const HeadersEvent& event) = 0;

    /** Called when block data is received (BLOCK or reconstructed from CMPCTBLOCK) */
    virtual void OnBlockReceived(const BlockReceivedEvent& event) = 0;

    // ========== Validation Events ==========

    /** Called after block validation completes */
    virtual void OnBlockValidated(const BlockValidatedEvent& event) = 0;

    /** Called after transaction admission attempt */
    virtual void OnTxAdmission(const TxAdmissionEvent& event) = 0;

    // ========== Performance Events ==========

    /** Called at end of each message handler loop iteration */
    virtual void OnMsgHandlerLoop(const MsgHandlerLoopEvent& event) = 0;

    /** Called after RPC call completes */
    virtual void OnRpcCall(const RpcCallEvent& event) = 0;

    // ========== Phase 2: Block Processing Delay Events (#21803) ==========

    /** Called when a new block is announced */
    virtual void OnBlockAnnounce(const BlockAnnounceEvent& event) = 0;

    /** Called when a block request decision is made */
    virtual void OnBlockRequestDecision(const BlockRequestDecisionEvent& event) = 0;

    /** Called when block in-flight state changes */
    virtual void OnBlockInFlight(const BlockInFlightEvent& event) = 0;

    /** Called when a staller is detected in download window */
    virtual void OnStallerDetected(const StallerDetectedEvent& event) = 0;

    /** Called when compact block processing decision is made */
    virtual void OnCompactBlockDecision(const CompactBlockDecisionEvent& event) = 0;

    /** Called when block source is resolved (block received) */
    virtual void OnBlockSourceResolved(const BlockSourceResolvedEvent& event) = 0;
};

/**
 * @brief No-op implementation of StdioBusHooks
 * 
 * Used when stdio_bus is disabled. All methods are empty and inline.
 */
class NoOpStdioBusHooks final : public StdioBusHooks {
public:
    bool Enabled() const override { return false; }
    bool ShadowMode() const override { return true; }
    
    void OnMessage(const MessageEvent&) override {}
    void OnHeaders(const HeadersEvent&) override {}
    void OnBlockReceived(const BlockReceivedEvent&) override {}
    void OnBlockValidated(const BlockValidatedEvent&) override {}
    void OnTxAdmission(const TxAdmissionEvent&) override {}
    void OnMsgHandlerLoop(const MsgHandlerLoopEvent&) override {}
    void OnRpcCall(const RpcCallEvent&) override {}
    
    // Phase 2: Block Processing Delay Events
    void OnBlockAnnounce(const BlockAnnounceEvent&) override {}
    void OnBlockRequestDecision(const BlockRequestDecisionEvent&) override {}
    void OnBlockInFlight(const BlockInFlightEvent&) override {}
    void OnStallerDetected(const StallerDetectedEvent&) override {}
    void OnCompactBlockDecision(const CompactBlockDecisionEvent&) override {}
    void OnBlockSourceResolved(const BlockSourceResolvedEvent&) override {}
};

/**
 * @brief Get current monotonic time in microseconds
 * 
 * Uses std::chrono::steady_clock for monotonic timestamps.
 */
inline int64_t GetMonotonicTimeUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

} // namespace node

#endif // BITCOIN_NODE_STDIO_BUS_HOOKS_H
