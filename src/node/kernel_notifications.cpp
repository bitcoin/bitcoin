// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/kernel_notifications.h>

#include <node/interface_ui.h>

namespace node {

void KernelNotifications::blockTip(SynchronizationState state, CBlockIndex& index)
{
    uiInterface.NotifyBlockTip(state, &index);
}

void KernelNotifications::headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync)
{
    uiInterface.NotifyHeaderTip(state, height, timestamp, presync);
}

} // namespace node
