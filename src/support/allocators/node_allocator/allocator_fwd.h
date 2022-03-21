// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_ALLOCATOR_FWD_H
#define BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_ALLOCATOR_FWD_H

#include <cstddef>

namespace node_allocator {

template <typename T, size_t ALLOCATION_SIZE_BYTES>
class Allocator;

} // namespace node_allocator

#endif // BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_ALLOCATOR_FWD_H
