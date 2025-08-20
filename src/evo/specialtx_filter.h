// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTX_FILTER_H
#define BITCOIN_EVO_SPECIALTX_FILTER_H

#include <functional>
#include <vector>

class CTransaction;

/**
 * Extract filterable elements from special transactions for use in compact block filters.
 * This function extracts the same fields that are included in bloom filters to ensure
 * SPV clients can detect special transactions using either filtering mechanism.
 *
 * @param tx The transaction to extract elements from
 * @param addElement Callback function to add extracted elements to the filter
 */
void ExtractSpecialTxFilterElements(const CTransaction& tx,
                                   const std::function<void(const std::vector<unsigned char>&)>& addElement);

#endif // BITCOIN_EVO_SPECIALTX_FILTER_H
