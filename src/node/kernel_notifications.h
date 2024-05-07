// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
#define BITCOIN_NODE_KERNEL_NOTIFICATIONS_H

#include <kernel/notifications_interface.h>

#include <atomic>
#include <cstdint>

class ArgsManager;
class CBlockIndex;
enum class SynchronizationState;
struct bilingual_str;

namespace kernel {
enum class Warning;
} // namespace kernel

namespace util {
class SignalInterrupt;
} // namespace util

namespace node {

class Warnings;
static constexpr int DEFAULT_STOPATHEIGHT{0};

class KernelNotifications : public kernel::Notifications
{
public:
    KernelNotifications(util::SignalInterrupt& shutdown, std::atomic<int>& exit_status, node::Warnings& warnings)
        : m_shutdown(shutdown), m_exit_status{exit_status}, m_warnings{warnings} {}

    [[nodiscard]] kernel::InterruptResult blockTip(SynchronizationState state, CBlockIndex& index) override;

    void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override;

    void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override;

    void warningSet(kernel::Warning id, const bilingual_str& message) override;

    void warningUnset(kernel::Warning id) override;

    void flushError(const bilingual_str& message) override;

    void fatalError(const bilingual_str& message) override;

    //! Block height after which blockTip notification will return Interrupted{}, if >0.
    int m_stop_at_height{DEFAULT_STOPATHEIGHT};
    //! Useful for tests, can be set to false to avoid shutdown on fatal error.
    bool m_shutdown_on_fatal_error{true};
private:
    util::SignalInterrupt& m_shutdown;
    std::atomic<int>& m_exit_status;
    node::Warnings& m_warnings;
};

void ReadNotificationArgs(const ArgsManager& args, KernelNotifications& notifications);

} // namespace node

#endif // BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
