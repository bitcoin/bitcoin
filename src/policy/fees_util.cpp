// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <node/mini_miner.h>
#include <policy/feerate.h>
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

MiniMinerInput GetMiniMinerInput(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block)
{
    // Cache all the transactions for efficient lookup
    std::map<Txid, TransactionInfo> tx_caches;
    for (const auto& tx : txs_removed_for_block) {
        tx_caches.emplace(tx.info.m_tx->GetHash(), TransactionInfo(tx.info.m_tx, tx.info.m_fee, tx.info.m_virtual_transaction_size, tx.info.txHeight));
    }
    const auto& txAncestorsAndDescendants = GetTxAncestorsAndDescendants(txs_removed_for_block);
    std::vector<node::MiniMinerMempoolEntry> transactions;
    std::map<Txid, std::set<Txid>> descendant_caches;
    transactions.reserve(txAncestorsAndDescendants.size());
    for (const auto& transaction : txAncestorsAndDescendants) {
        const auto& txid = transaction.first;
        const auto& [ancestors, descendants] = transaction.second;
        int64_t vsize_ancestor = 0;
        CAmount fee_with_ancestors = 0;
        for (auto& ancestor_id : ancestors) {
            const auto& ancestor = tx_caches.find(ancestor_id)->second;
            vsize_ancestor += ancestor.m_virtual_transaction_size;
            fee_with_ancestors += ancestor.m_fee;
        }
        descendant_caches.emplace(txid, descendants);
        auto tx_info = tx_caches.find(txid)->second;
        transactions.emplace_back(tx_info.m_tx, tx_info.m_virtual_transaction_size, vsize_ancestor, tx_info.m_fee, fee_with_ancestors);
    }
    return std::make_tuple(transactions, descendant_caches);
}
