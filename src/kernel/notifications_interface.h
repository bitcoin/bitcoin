// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_NOTIFICATIONS_INTERFACE_H
#define BITCOIN_KERNEL_NOTIFICATIONS_INTERFACE_H

#include <cstdint>

class CBlockIndex;
enum class SynchronizationState;

namespace kernel {

/**
 * A base class defining functions for notifying about certain kernel
 * events.
 */
class Notifications
{
public:
    virtual ~Notifications(){};

    virtual void blockTip(SynchronizationState state, CBlockIndex& index) {}
    virtual void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) {}
};
} // namespace kernel

#endif // BITCOIN_KERNEL_NOTIFICATIONS_INTERFACE_H
