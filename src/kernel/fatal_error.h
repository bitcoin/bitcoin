// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_FATAL_ERROR_H
#define BITCOIN_KERNEL_FATAL_ERROR_H

#include <consensus/validation.h>
#include <kernel/notifications_interface.h>
#include <util/translation.h>

inline bool FatalError(kernel::Notifications& notifications, BlockValidationState& state, const bilingual_str& message)
{
    notifications.fatalError(message);
    return state.Error(message.original);
}

#endif // BITCOIN_KERNEL_FATAL_ERROR_H
