// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/stdio_bus_sdk_hooks.h>

#include <logging.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <chrono>
#include <sstream>

namespace node {

// ============================================================================
// SDK Implementation (placeholder for real stdio_bus SDK integration)
// ============================================================================

struct StdioBusSdkHooks::SdkImpl {
    std::string config_path;
    bool connected{false};
    
    // In real implementation, this would be:
    // std::unique_ptr<stdiobus::Bus> bus;
    
    bool Connect(const std::string& path) {
        config_path = path;
        // TODO: Real SDK integration
        // bus = std::make_unique<stdiobus::Bus>(path);
        // if (auto err = bus->start(); err) {
        //     LogDebug(BCLog::NET, "stdio_bus SDK failed to start: %s\n", err.message());
        //     return false;
        // }
        connected = true;
        LogDebug(BCLog::NET, "stdio_bus SDK connected (config: %s)\n", path);
        return true;
    }
    
    bool Send(const std::string& message) {
        if (!connected) return false;
        // TODO: Real SDK integration
        // if (auto err = bus->send(message); err) {
        //     return false;
        // }
        LogDebug(BCLog::NET, "stdio_bus SDK send: %s\n", message.substr(0, 100));
        return true;
    }
    
    void Disconnect() {
        if (connected) {
            // TODO: Real SDK integration
            // bus->stop(std::chrono::seconds(5));
            // bus.reset();
            connected = false;
            LogDebug(BCLog::NET, "stdio_bus SDK disconnected\n");
        }
    }
};

// ============================================================================
// StdioBusSdkHooks Implementation
// ============================================================================

StdioBusSdkHooks::StdioBusSdkHooks(Config config)
    : m_config(std::move(config))
    , m_shadow_mode(m_config.shadow_mode)
    , m_queue_capacity(m_config.queue_capacity)
    , m_sdk(std::make_unique<SdkImpl>())
{
}

StdioBusSdkHooks::~StdioBusSdkHooks()
{
    Stop();
}

bool StdioBusSdkHooks::Start()
{
    if (m_running.load(std::memory_order_relaxed)) {
        return true; // Already running
    }

    // Connect SDK
    if (!m_sdk->Connect(m_config.config_path)) {
        LogError("stdio_bus: Failed to connect SDK\n");
        return false;
    }

    // Start worker thread
    m_stop_requested.store(false, std::memory_order_relaxed);
    m_worker_thread = std::thread(&StdioBusSdkHooks::WorkerLoop, this);
    
    m_running.store(true, std::memory_order_release);
    m_enabled.store(true, std::memory_order_release);
    
    LogInfo("stdio_bus: Started (shadow_mode=%s, queue_capacity=%zu)\n",
            m_shadow_mode ? "true" : "false", m_queue_capacity);
    return true;
}

void StdioBusSdkHooks::Stop()
{
    if (!m_running.load(std::memory_order_relaxed)) {
        return; // Not running
    }

    m_enabled.store(false, std::memory_order_release);
    m_stop_requested.store(true, std::memory_order_release);
    
    // Wake up worker
    m_queue_cv.notify_all();
    
    // Wait for worker to finish
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
    
    // Disconnect SDK
    m_sdk->Disconnect();
    
    m_running.store(false, std::memory_order_release);
    LogInfo("stdio_bus: Stopped\n");
}

// ============================================================================
// Hook Implementations (fast path - just enqueue)
// ============================================================================

void StdioBusSdkHooks::OnMessage(const MessageEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnHeaders(const HeadersEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnBlockReceived(const BlockReceivedEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnBlockValidated(const BlockValidatedEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnTxAdmission(const TxAdmissionEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnMsgHandlerLoop(const MsgHandlerLoopEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnRpcCall(const RpcCallEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

// ============================================================================
// Phase 2: Block Processing Delay Events (#21803)
// ============================================================================

void StdioBusSdkHooks::OnBlockAnnounce(const BlockAnnounceEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnBlockRequestDecision(const BlockRequestDecisionEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnBlockInFlight(const BlockInFlightEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnStallerDetected(const StallerDetectedEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnCompactBlockDecision(const CompactBlockDecisionEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

void StdioBusSdkHooks::OnBlockSourceResolved(const BlockSourceResolvedEvent& ev)
{
    if (!Enabled()) return;
    
    int64_t start = GetMonotonicTimeUs();
    bool enqueued = TryEnqueue(ev);
    int64_t latency = GetMonotonicTimeUs() - start;
    
    m_stats.last_hook_latency_us.store(latency, std::memory_order_relaxed);
    m_stats.events_total.fetch_add(1, std::memory_order_relaxed);
    if (!enqueued) {
        m_stats.events_dropped.fetch_add(1, std::memory_order_relaxed);
    }
}

// ============================================================================
// Queue Operations
// ============================================================================

bool StdioBusSdkHooks::TryEnqueue(Event ev)
{
    std::unique_lock<std::mutex> lock(m_queue_mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        // Could not acquire lock immediately - drop event (fail-open)
        return false;
    }
    
    if (m_queue.size() >= m_queue_capacity) {
        // Queue full - drop event (fail-open)
        return false;
    }
    
    m_queue.push(std::move(ev));
    m_stats.queue_depth.store(m_queue.size(), std::memory_order_relaxed);
    
    lock.unlock();
    m_queue_cv.notify_one();
    return true;
}

// ============================================================================
// Background Worker
// ============================================================================

void StdioBusSdkHooks::WorkerLoop()
{
    LogDebug(BCLog::NET, "stdio_bus: Worker thread started\n");
    
    while (!m_stop_requested.load(std::memory_order_relaxed)) {
        Event ev;
        
        // Wait for event
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !m_queue.empty() || m_stop_requested.load(std::memory_order_relaxed);
            });
            
            if (m_queue.empty()) {
                continue;
            }
            
            ev = std::move(m_queue.front());
            m_queue.pop();
            m_stats.queue_depth.store(m_queue.size(), std::memory_order_relaxed);
        }
        
        // Serialize and send (outside lock)
        std::string json = SerializeEvent(ev);
        if (SendToSdk(json)) {
            m_stats.events_sent.fetch_add(1, std::memory_order_relaxed);
        } else {
            m_stats.errors.fetch_add(1, std::memory_order_relaxed);
        }
    }
    
    // Drain remaining events on shutdown
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        while (!m_queue.empty()) {
            Event ev = std::move(m_queue.front());
            m_queue.pop();
            std::string json = SerializeEvent(ev);
            SendToSdk(json);
        }
    }
    
    LogDebug(BCLog::NET, "stdio_bus: Worker thread stopped\n");
}

// ============================================================================
// Serialization
// ============================================================================

std::string StdioBusSdkHooks::SerializeEvent(const Event& ev) const
{
    std::ostringstream ss;
    ss << "{\"jsonrpc\":\"2.0\",\"method\":\"stdio_bus.event\",\"params\":{";
    
    std::visit([&ss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        
        if constexpr (std::is_same_v<T, MessageEvent>) {
            ss << "\"type\":\"message\","
               << "\"peer_id\":" << arg.peer_id << ","
               << "\"msg_type\":\"" << arg.msg_type << "\","
               << "\"size_bytes\":" << arg.size_bytes << ","
               << "\"received_us\":" << arg.received_us;
        }
        else if constexpr (std::is_same_v<T, HeadersEvent>) {
            ss << "\"type\":\"headers\","
               << "\"peer_id\":" << arg.peer_id << ","
               << "\"count\":" << arg.count << ","
               << "\"first_prev_hash\":\"" << arg.first_prev_hash.GetHex() << "\","
               << "\"received_us\":" << arg.received_us;
        }
        else if constexpr (std::is_same_v<T, BlockReceivedEvent>) {
            ss << "\"type\":\"block_received\","
               << "\"peer_id\":" << arg.peer_id << ","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"height\":" << arg.height << ","
               << "\"size_bytes\":" << arg.size_bytes << ","
               << "\"tx_count\":" << arg.tx_count << ","
               << "\"received_us\":" << arg.received_us;
        }
        else if constexpr (std::is_same_v<T, BlockValidatedEvent>) {
            ss << "\"type\":\"block_validated\","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"height\":" << arg.height << ","
               << "\"tx_count\":" << arg.tx_count << ","
               << "\"received_us\":" << arg.received_us << ","
               << "\"validated_us\":" << arg.validated_us << ","
               << "\"accepted\":" << (arg.accepted ? "true" : "false");
            if (!arg.reject_reason.empty()) {
                ss << ",\"reject_reason\":\"" << arg.reject_reason << "\"";
            }
        }
        else if constexpr (std::is_same_v<T, TxAdmissionEvent>) {
            ss << "\"type\":\"tx_admission\","
               << "\"txid\":\"" << arg.txid.GetHex() << "\","
               << "\"wtxid\":\"" << arg.wtxid.GetHex() << "\","
               << "\"size_bytes\":" << arg.size_bytes << ","
               << "\"received_us\":" << arg.received_us << ","
               << "\"processed_us\":" << arg.processed_us << ","
               << "\"accepted\":" << (arg.accepted ? "true" : "false");
            if (!arg.reject_reason.empty()) {
                ss << ",\"reject_reason\":\"" << arg.reject_reason << "\"";
            }
        }
        else if constexpr (std::is_same_v<T, MsgHandlerLoopEvent>) {
            ss << "\"type\":\"msg_handler_loop\","
               << "\"iteration\":" << arg.iteration << ","
               << "\"start_us\":" << arg.start_us << ","
               << "\"end_us\":" << arg.end_us << ","
               << "\"messages_processed\":" << arg.messages_processed << ","
               << "\"had_work\":" << (arg.had_work ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, RpcCallEvent>) {
            ss << "\"type\":\"rpc_call\","
               << "\"method\":\"" << arg.method << "\","
               << "\"start_us\":" << arg.start_us << ","
               << "\"end_us\":" << arg.end_us << ","
               << "\"success\":" << (arg.success ? "true" : "false");
        }
        // Phase 2: Block Processing Delay Events
        else if constexpr (std::is_same_v<T, BlockAnnounceEvent>) {
            const char* via_str = "unknown";
            switch (arg.via) {
                case BlockAnnounceVia::Headers: via_str = "headers"; break;
                case BlockAnnounceVia::CompactBlock: via_str = "compact_block"; break;
                case BlockAnnounceVia::Inv: via_str = "inv"; break;
            }
            ss << "\"type\":\"block_announce\","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"peer_id\":" << arg.peer_id << ","
               << "\"via\":\"" << via_str << "\","
               << "\"chainwork_delta\":" << arg.chainwork_delta << ","
               << "\"height\":" << arg.height << ","
               << "\"timestamp_us\":" << arg.timestamp_us;
        }
        else if constexpr (std::is_same_v<T, BlockRequestDecisionEvent>) {
            const char* reason_str = "unknown";
            switch (arg.reason) {
                case BlockRequestReason::NewBlock: reason_str = "new_block"; break;
                case BlockRequestReason::Retry: reason_str = "retry"; break;
                case BlockRequestReason::Hedge: reason_str = "hedge"; break;
                case BlockRequestReason::CompactFallback: reason_str = "compact_fallback"; break;
                case BlockRequestReason::ParallelDownload: reason_str = "parallel_download"; break;
            }
            ss << "\"type\":\"block_request_decision\","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"peer_id\":" << arg.peer_id << ","
               << "\"reason\":\"" << reason_str << "\","
               << "\"is_preferred_peer\":" << (arg.is_preferred_peer ? "true" : "false") << ","
               << "\"first_in_flight\":" << (arg.first_in_flight ? "true" : "false") << ","
               << "\"already_in_flight\":" << arg.already_in_flight << ","
               << "\"can_direct_fetch\":" << (arg.can_direct_fetch ? "true" : "false") << ","
               << "\"is_limited_peer\":" << (arg.is_limited_peer ? "true" : "false") << ","
               << "\"timestamp_us\":" << arg.timestamp_us;
        }
        else if constexpr (std::is_same_v<T, BlockInFlightEvent>) {
            const char* action_str = "unknown";
            switch (arg.action) {
                case InFlightAction::Add: action_str = "add"; break;
                case InFlightAction::Remove: action_str = "remove"; break;
                case InFlightAction::Timeout: action_str = "timeout"; break;
            }
            ss << "\"type\":\"block_in_flight\","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"peer_id\":" << arg.peer_id << ","
               << "\"action\":\"" << action_str << "\","
               << "\"inflight_count\":" << arg.inflight_count << ","
               << "\"peer_inflight_count\":" << arg.peer_inflight_count << ","
               << "\"timestamp_us\":" << arg.timestamp_us;
        }
        else if constexpr (std::is_same_v<T, StallerDetectedEvent>) {
            ss << "\"type\":\"staller_detected\","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"staller_peer_id\":" << arg.staller_peer_id << ","
               << "\"waiting_peer_id\":" << arg.waiting_peer_id << ","
               << "\"window_end_height\":" << arg.window_end_height << ","
               << "\"stall_duration_us\":" << arg.stall_duration_us << ","
               << "\"timestamp_us\":" << arg.timestamp_us;
        }
        else if constexpr (std::is_same_v<T, CompactBlockDecisionEvent>) {
            const char* action_str = "unknown";
            switch (arg.action) {
                case CompactBlockAction::Reconstruct: action_str = "reconstruct"; break;
                case CompactBlockAction::GetBlockTxn: action_str = "get_block_txn"; break;
                case CompactBlockAction::GetData: action_str = "get_data"; break;
                case CompactBlockAction::Wait: action_str = "wait"; break;
                case CompactBlockAction::Drop: action_str = "drop"; break;
            }
            ss << "\"type\":\"compact_block_decision\","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"peer_id\":" << arg.peer_id << ","
               << "\"action\":\"" << action_str << "\","
               << "\"missing_tx_count\":" << arg.missing_tx_count << ","
               << "\"first_in_flight\":" << (arg.first_in_flight ? "true" : "false") << ","
               << "\"is_highbandwidth\":" << (arg.is_highbandwidth ? "true" : "false") << ","
               << "\"timestamp_us\":" << arg.timestamp_us;
        }
        else if constexpr (std::is_same_v<T, BlockSourceResolvedEvent>) {
            ss << "\"type\":\"block_source_resolved\","
               << "\"hash\":\"" << arg.hash.GetHex() << "\","
               << "\"source_peer_id\":" << arg.source_peer_id << ","
               << "\"first_requested_peer_id\":" << arg.first_requested_peer_id << ","
               << "\"announce_to_receive_us\":" << arg.announce_to_receive_us << ","
               << "\"request_to_receive_us\":" << arg.request_to_receive_us << ","
               << "\"total_requests\":" << arg.total_requests << ","
               << "\"timestamp_us\":" << arg.timestamp_us;
        }
    }, ev);
    
    ss << "}}";
    return ss.str();
}

bool StdioBusSdkHooks::SendToSdk(const std::string& json)
{
    try {
        return m_sdk->Send(json);
    } catch (...) {
        // Fail silently - hooks must not affect consensus
        LogDebug(BCLog::NET, "stdio_bus: SDK send threw exception\n");
        return false;
    }
}

// ============================================================================
// Statistics
// ============================================================================

StdioBusStatsSnapshot StdioBusSdkHooks::GetStats() const
{
    return m_stats.Snapshot();
}

void StdioBusSdkHooks::ResetStats()
{
    m_stats.events_total.store(0, std::memory_order_relaxed);
    m_stats.events_dropped.store(0, std::memory_order_relaxed);
    m_stats.events_sent.store(0, std::memory_order_relaxed);
    m_stats.errors.store(0, std::memory_order_relaxed);
    m_stats.last_hook_latency_us.store(0, std::memory_order_relaxed);
}

// ============================================================================
// Factory
// ============================================================================

std::shared_ptr<StdioBusSdkHooks> MakeStdioBusSdkHooks(
    const std::string& config_path,
    bool shadow_mode)
{
    StdioBusSdkHooks::Config config;
    config.config_path = config_path;
    config.shadow_mode = shadow_mode;
    config.queue_capacity = 4096;
    
    auto hooks = std::make_shared<StdioBusSdkHooks>(std::move(config));
    if (!hooks->Start()) {
        return nullptr;
    }
    return hooks;
}

} // namespace node
