// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_RPC_LOAD_MONITOR_H
#define BITCOIN_NODE_RPC_LOAD_MONITOR_H

#include <atomic>
#include <cstdint>
#include <memory>

namespace node {

/**
 * RPC load state for backpressure policy.
 * Used to throttle low-priority P2P messages when RPC queue is overloaded.
 */
enum class RpcLoadState : uint8_t {
    NORMAL = 0,    //!< Queue depth < 75% - process all messages normally
    ELEVATED = 1,  //!< Queue depth >= 75% - defer low-priority P2P messages
    CRITICAL = 2,  //!< Queue depth >= 90% - may drop tx announcements
};

/**
 * Interface for monitoring RPC queue load.
 * Read-side used by net_processing for backpressure decisions.
 * Write-side used by HTTP server to report queue depth.
 */
class RpcLoadMonitor
{
public:
    virtual ~RpcLoadMonitor() = default;

    // Read-side API (for net_processing)
    virtual RpcLoadState GetState() const = 0;
    virtual int GetQueueDepth() const = 0;
    virtual int GetQueueCapacity() const = 0;

    // Write-side API (for HTTP/RPC layer)
    virtual void OnQueueDepthSample(int queue_depth, int queue_capacity) = 0;
};

/**
 * Lock-free RpcLoadMonitor with hysteresis to prevent state oscillation.
 *
 * State transitions:
 *   NORMAL -> ELEVATED: queue_depth >= 75% capacity
 *   NORMAL -> CRITICAL: queue_depth >= 90% capacity
 *   ELEVATED -> CRITICAL: queue_depth >= 90% capacity
 *   ELEVATED -> NORMAL: queue_depth < 50% capacity
 *   CRITICAL -> ELEVATED: queue_depth < 70% capacity
 */
struct RpcLoadMonitorConfig {
    double elevated_ratio = 0.75;   //!< Enter ELEVATED when >= this ratio
    double critical_ratio = 0.90;   //!< Enter CRITICAL when >= this ratio
    double leave_elevated = 0.50;   //!< Leave ELEVATED -> NORMAL when < this ratio
    double leave_critical = 0.70;   //!< Leave CRITICAL -> ELEVATED when < this ratio
};

class AtomicRpcLoadMonitor final : public RpcLoadMonitor
{
public:
    using Config = RpcLoadMonitorConfig;

    AtomicRpcLoadMonitor() : m_cfg{} {}
    explicit AtomicRpcLoadMonitor(Config cfg) : m_cfg(cfg) {}

    RpcLoadState GetState() const override
    {
        return m_state.load(std::memory_order_relaxed);
    }

    int GetQueueDepth() const override
    {
        return m_depth.load(std::memory_order_relaxed);
    }

    int GetQueueCapacity() const override
    {
        return m_capacity.load(std::memory_order_relaxed);
    }

    void OnQueueDepthSample(int depth, int cap) override
    {
        m_depth.store(depth, std::memory_order_relaxed);
        m_capacity.store(cap, std::memory_order_relaxed);

        if (cap <= 0) return;

        const double ratio = static_cast<double>(depth) / static_cast<double>(cap);
        RpcLoadState cur = m_state.load(std::memory_order_relaxed);
        RpcLoadState next = cur;

        // State machine with hysteresis
        if (cur == RpcLoadState::NORMAL) {
            if (ratio >= m_cfg.critical_ratio) {
                next = RpcLoadState::CRITICAL;
            } else if (ratio >= m_cfg.elevated_ratio) {
                next = RpcLoadState::ELEVATED;
            }
        } else if (cur == RpcLoadState::ELEVATED) {
            if (ratio >= m_cfg.critical_ratio) {
                next = RpcLoadState::CRITICAL;
            } else if (ratio < m_cfg.leave_elevated) {
                next = RpcLoadState::NORMAL;
            }
        } else { // CRITICAL
            if (ratio < m_cfg.leave_critical) {
                next = RpcLoadState::ELEVATED;
            }
        }

        if (next != cur) {
            m_state.store(next, std::memory_order_relaxed);
        }
    }

private:
    Config m_cfg;
    std::atomic<RpcLoadState> m_state{RpcLoadState::NORMAL};
    std::atomic<int> m_depth{0};
    std::atomic<int> m_capacity{0};
};

} // namespace node

#endif // BITCOIN_NODE_RPC_LOAD_MONITOR_H
