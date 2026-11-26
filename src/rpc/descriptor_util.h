// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_RPC_DESCRIPTOR_UTIL_H
#define BITCOIN_RPC_DESCRIPTOR_UTIL_H

#include <cstddef>
#include <string>
#include <vector>

class UniValue;
struct DescriptorAnalysis;
struct DescriptorKeyInventory;
struct RPCResult;

/** JSON helpers shared by the RPCs that return a descriptor's keys and multisig analysis. */

//! Description of the object returned by DescriptorAnalysisToUniv().
extern const std::string HELP_DESCRIPTOR_ANALYSIS;

//! Fields of each entry of the array returned by DescriptorKeysToUniv().
std::vector<RPCResult> DescriptorKeyResultFields();
//! Fields of the object returned by DescriptorAnalysisToUniv().
std::vector<RPCResult> DescriptorAnalysisResultFields();

UniValue IdsToUniv(const std::vector<size_t>& ids);
UniValue DescriptorKeysToUniv(const DescriptorKeyInventory& inventory);
UniValue DescriptorAnalysisToUniv(const DescriptorAnalysis& analysis);

#endif // BITCOIN_RPC_DESCRIPTOR_UTIL_H
