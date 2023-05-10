// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
#define BITCOIN_NODE_KERNEL_NOTIFICATIONS_H

#include <kernel/notifications_interface.h>

#include <cstdint>

class CBlockIndex;
enum class SynchronizationState;

namespace node {
class KernelNotifications : public kernel::Notifications
{
public:
    void blockTip(SynchronizationState state, CBlockIndex& index) override;

    void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override;
};
} // namespace node

#endif // BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
