// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/notifications_interface.h>

#include <cassert>
#include <string>

namespace kernel {

std::string InterruptReasonToString(InterruptReason reason)
{
    switch (reason) {
    case InterruptReason::StopAfterBlockImport:
        return "Stopping after block import.";
    case InterruptReason::StopAtHeight:
        return "Reached block height specified by stop at height.";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

} // namespace kernel
