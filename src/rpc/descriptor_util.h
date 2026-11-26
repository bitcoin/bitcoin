// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_RPC_DESCRIPTOR_UTIL_H
#define BITCOIN_RPC_DESCRIPTOR_UTIL_H

#include <cstddef>
#include <vector>

class UniValue;
struct DescriptorKeyInventory;
struct RPCResult;

/** JSON helpers shared by the RPCs that return a descriptor's keys. */

//! Fields of each entry of the array returned by DescriptorKeysToUniv().
std::vector<RPCResult> DescriptorKeyResultFields();

UniValue IdsToUniv(const std::vector<size_t>& ids);
UniValue DescriptorKeysToUniv(const DescriptorKeyInventory& inventory);

#endif // BITCOIN_RPC_DESCRIPTOR_UTIL_H
