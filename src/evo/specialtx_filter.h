// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTX_FILTER_H
#define BITCOIN_EVO_SPECIALTX_FILTER_H

#include <functional>
#include <span.h>

class CTransaction;

/**
 * Extract filterable elements from special transactions for use in compact block filters.
 * This function extracts the same fields that are included in bloom filters to ensure
 * SPV clients can detect special transactions using either filtering mechanism.
 *
 * @param tx The transaction to extract elements from
 * @param addElement Callback to add extracted elements to the filter. Uses
 *                   Span<const unsigned char> to avoid intermediate
 *                   allocations.
 */
void ExtractSpecialTxFilterElements(const CTransaction& tx,
                                   const std::function<void(Span<const unsigned char>)>& addElement);

#endif // BITCOIN_EVO_SPECIALTX_FILTER_H
