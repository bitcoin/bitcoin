// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WARNINGS_H
#define BITCOIN_WARNINGS_H

#include <optional>
#include <string>
#include <vector>

struct bilingual_str;

void SetMiscWarning(const bilingual_str& warning);
void SetfLargeWorkInvalidChainFound(bool flag);
/** Pass std::nullopt to disable the warning */
void SetMedianTimeOffsetWarning(std::optional<bilingual_str> warning);
/** Return potential problems detected by the node. */
std::vector<bilingual_str> GetWarnings();

#endif // BITCOIN_WARNINGS_H
