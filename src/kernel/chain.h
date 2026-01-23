// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CHAIN_H
#define BITCOIN_KERNEL_CHAIN_H

#include <iostream>

namespace kernel {
struct ChainstateRole;
std::ostream& operator<<(std::ostream& os, const ChainstateRole& role);
} // namespace kernel

#endif // BITCOIN_KERNEL_CHAIN_H
