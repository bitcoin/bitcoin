// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_WARNINGS_H
#define BITCOIN_NODE_WARNINGS_H

#include <optional>
#include <string>
#include <vector>

class UniValue;
struct bilingual_str;

namespace node {
void SetMiscWarning(const bilingual_str& warning);
void SetfLargeWorkInvalidChainFound(bool flag);
/** Pass std::nullopt to disable the warning */
void SetMedianTimeOffsetWarning(std::optional<bilingual_str> warning);
/** Return potential problems detected by the node. */
std::vector<bilingual_str> GetWarnings();
/**
 * RPC helper function that wraps GetWarnings. Returns a UniValue::VSTR
 * with the latest warning if use_deprecated is set to true, or a
 * UniValue::VARR with all warnings otherwise.
 */
UniValue GetWarningsForRpc(bool use_deprecated);
} // namespace node

#endif // BITCOIN_NODE_WARNINGS_H
