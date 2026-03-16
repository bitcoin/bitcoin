// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
#define BITCOIN_NODE_KERNEL_NOTIFICATIONS_H

#include <kernel/notifications_interface.h>

#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>

#include <atomic>
#include <cstdint>
#include <functional>

class ArgsManager;
class CBlockIndex;
enum class SynchronizationState;
struct bilingual_str;

namespace kernel {
enum class Warning;
} // namespace kernel

namespace node {

class Warnings;
static constexpr int DEFAULT_STOPATHEIGHT{0};

//! State tracked by the KernelNotifications interface meant to be used by
//! mining code, index code, RPCs, and other code sitting above the validation
//! layer.
//!
//! Currently just tracks the chain tip, but could be used to hold other
//! information in the future, like the last flushed block, pruning
//! information, etc.
struct KernelState {
    bool chainstate_loaded{false};
    std::optional<uint256> tip_block;
};

class KernelNotifications : public kernel::Notifications
{
public:
    KernelNotifications(const std::function<bool()>& shutdown_request, std::atomic<int>& exit_status, node::Warnings& warnings)
        : m_shutdown_request(shutdown_request), m_exit_status{exit_status}, m_warnings{warnings} {}

    [[nodiscard]] kernel::InterruptResult blockTip(SynchronizationState state, const CBlockIndex& index, double verification_progress) override EXCLUSIVE_LOCKS_REQUIRED(!m_tip_block_mutex);

    void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override;

    void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override;

    void warningSet(kernel::Warning id, const bilingual_str& message) override;

    void warningUnset(kernel::Warning id) override;

    void flushError(const bilingual_str& message) override;

    void fatalError(const bilingual_str& message) override;

    void setChainstateLoaded(bool chainstate_loaded) EXCLUSIVE_LOCKS_REQUIRED(!m_tip_block_mutex) {
        LOCK(m_tip_block_mutex);
        if (!chainstate_loaded) m_state = {};
        m_state.chainstate_loaded = chainstate_loaded;
        m_tip_block_cv.notify_all();
    }

    //! Block height after which blockTip notification will return Interrupted{}, if >0.
    int m_stop_at_height{DEFAULT_STOPATHEIGHT};
    //! Useful for tests, can be set to false to avoid shutdown on fatal error.
    bool m_shutdown_on_fatal_error{true};

    Mutex m_tip_block_mutex;
    std::condition_variable m_tip_block_cv GUARDED_BY(m_tip_block_mutex);
    KernelState m_state GUARDED_BY(m_tip_block_mutex);
    //! The block for which the last blockTip notification was received.
    //! It's first set when the tip is connected during node initialization.
    //! Might be unset during an early shutdown.
    std::optional<uint256> TipBlock() EXCLUSIVE_LOCKS_REQUIRED(m_tip_block_mutex);

private:
    const std::function<bool()>& m_shutdown_request;
    std::atomic<int>& m_exit_status;
    node::Warnings& m_warnings;
};

void ReadNotificationArgs(const ArgsManager& args, KernelNotifications& notifications);

} // namespace node

#endif // BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
