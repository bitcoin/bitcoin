// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_TRACES_H
#define BITCOIN_KERNEL_TRACES_H

#include <interfaces/tracing.h>
#include <sync.h>

#include <list>

namespace kernel {
/**
 * Struct used by kernel code which emits traces to track which traces are
 * enabled and which listeners are receiving them.
 */
struct Traces {
    Mutex mutex;
    std::list<std::unique_ptr<interfaces::UtxoCacheTrace>> utxo_cache GUARDED_BY(mutex);
};
} // namespace kernel

#endif // BITCOIN_KERNEL_TRACES_H
