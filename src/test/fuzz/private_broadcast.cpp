// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <net.h>
#include <primitives/transaction.h>
#include <private_broadcast.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/overflow.h>
#include <util/time.h>

#include <unordered_map>
#include <unordered_set>

FUZZ_TARGET(private_broadcast)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fdp));

    PrivateBroadcast pb;

    struct CTransactionRefHasher {
        size_t operator()(const CTransactionRef& t) const { return ReadLE64(t->GetHash().begin()); }
    };

    std::unordered_map<CTransactionRef, /*pushed_to_nodes=*/std::unordered_set<NodeId>, CTransactionRefHasher>
        transactions;

    NodeId next_nodeid{0}; // Generate unique node ids.

    while (fdp.remaining_bytes() > 0) {
        CallOneOf(
            fdp,
            [&] { // Add()
                std::optional<CMutableTransaction> opt_tx{ConsumeDeserializable<CMutableTransaction>(fdp, fdp.ConsumeBool() ? TX_WITH_WITNESS : TX_NO_WITNESS)};
                if (!opt_tx.has_value()) {
                    return;
                }
                TxValidationState state_with_dupe_check;
                CTransactionRef tx{MakeTransactionRef(std::move(opt_tx.value()))};
                const bool res{CheckTransaction(*tx, state_with_dupe_check)};
                if (!res || !state_with_dupe_check.IsValid()) {
                    return;
                }
                if (pb.Add(tx)) {
                    transactions.emplace(tx, /*pushed_to_nodes=*/std::unordered_set<NodeId>{});
                }
            },
            [&] { // Remove()
                if (transactions.empty()) {
                    return;
                }
                const auto it{PickIterator(fdp, transactions)};
                const CTransactionRef tx{it->first}; // Will still be valid after erase() below.
                Assert(pb.Remove(tx).has_value());
                transactions.erase(it);
                Assert(!pb.Remove(tx).has_value());
            },
            [&] { // GetTxForBroadcast()
                const auto opt_tx{pb.GetTxForBroadcast()};
                if (opt_tx.has_value()) {
                    Assert(transactions.contains(opt_tx.value()));
                } else {
                    Assert(transactions.empty());
                }
            },
            [&] { // PushedToNode()
                if (transactions.empty()) {
                    return;
                }
                const auto new_nodeid{next_nodeid++};
                auto& [tx, pushed_to_nodes] = *PickIterator(fdp, transactions);
                pb.PushedToNode(new_nodeid, tx->GetHash());
                const auto [_, inserted] = pushed_to_nodes.emplace(new_nodeid);
                Assert(inserted);
            },
            [&] { // GetTxPushedToNode()
                if (transactions.empty()) {
                    return;
                }
                const auto& [pushed_tx, pushed_to_nodes] = *PickIterator(fdp, transactions);
                if (pushed_to_nodes.empty()) {
                    return;
                }
                const auto nodeid{PickValue(fdp, pushed_to_nodes)};

                const auto opt_tx{pb.GetTxPushedToNode(nodeid)};
                Assert(opt_tx.has_value());
                Assert(opt_tx.value()->GetHash() == pushed_tx->GetHash());
            },
            [&] { // FinishBroadcast()
                if (transactions.empty()) {
                    return;
                }
                auto& [_, pushed_to_nodes] = *PickIterator(fdp, transactions);
                if (pushed_to_nodes.empty()) {
                    return;
                }
                const auto it_pushed_to_nodes{PickIterator(fdp, pushed_to_nodes)};
                const auto nodeid{*it_pushed_to_nodes};
                const bool confirmed{fdp.ConsumeBool()};
                pb.FinishBroadcast(nodeid, confirmed);
                pushed_to_nodes.erase(it_pushed_to_nodes);
                Assert(!pb.FinishBroadcast(nodeid, confirmed));
            },
            [&] { // GetStale()
                for (const auto& stale_tx : pb.GetStale()) {
                    Assert(transactions.contains(stale_tx));
                }
            }
        );

        // Advance the time by 0 to 1h.
        const auto t{Now<NodeSeconds>().time_since_epoch().count()};
        SetMockTime(ConsumeTime(fdp, /*min=*/t, /*max=*/t + 3600));
    }
}
