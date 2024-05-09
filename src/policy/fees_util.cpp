// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <policy/fees_util.h>
#include <primitives/transaction.h>

TxAncestorsAndDescendants GetTxAncestorsAndDescendants(const std::vector<RemovedMempoolTransactionInfo>& transactions)
{
    TxAncestorsAndDescendants visited_txs;
    for (const auto& tx_info : transactions) {
        const auto& txid = tx_info.info.m_tx->GetHash();
        visited_txs.emplace(txid, std::make_tuple(std::set<Txid>{txid}, std::set<Txid>{txid}));
        for (const auto& input : tx_info.info.m_tx->vin) {
            // If a parent has been visited add all the parent ancestors to the set of transaction ancestor
            // Also add the transaction into each ancestor descendant set
            if (visited_txs.find(input.prevout.hash) != visited_txs.end()) {
                auto& parent_ancestors = std::get<0>(visited_txs[input.prevout.hash]);
                auto& tx_ancestors = std::get<0>(visited_txs[txid]);
                for (auto& ancestor : parent_ancestors) {
                    tx_ancestors.insert(ancestor);
                    auto& ancestor_descendants = std::get<1>(visited_txs[ancestor]);
                    ancestor_descendants.insert(txid);
                }
            }
        }
    }
    return visited_txs;
}
