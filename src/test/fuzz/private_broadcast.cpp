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
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/overflow.h>
#include <util/time.h>

#include <map>
#include <unordered_map>

struct CTransactionRefHash {
    size_t operator()(const CTransactionRef& tx) const
    {
        return static_cast<size_t>(tx->GetWitnessHash().ToUint256().GetUint64(0));
    }
};

struct CTransactionRefComp {
    bool operator()(const CTransactionRef& a, const CTransactionRef& b) const
    {
        return a->GetWitnessHash() == b->GetWitnessHash();
    }
};

FUZZ_TARGET(private_broadcast)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    FakeNodeClock clock_ctx{ConsumeTime(fdp)};

    const size_t cap{fdp.ConsumeIntegralInRange<size_t>(1, 12)};
    PrivateBroadcast pb{cap};

    // Cumulative per-transaction counters, mirroring what PrivateBroadcast retains.
    struct TxModel {
        size_t num_picked{0};
        size_t num_confirmed{0};
    };

    // Random transactions that the test generated and passed to Add(). Trimmed when Remove() is called.
    std::unordered_map<CTransactionRef, TxModel, CTransactionRefHash, CTransactionRefComp> transactions;

    // In-flight sends to currently-connected nodes, mirroring PrivateBroadcast's per-node
    // state. Entries are added by PickTxForSend() and trimmed when Remove() or
    // NodeDisconnected() is called.
    struct NodeSendModel {
        CTransactionRef tx;
        bool confirmed{false};
    };
    std::map<NodeId, NodeSendModel> node_sends;

    NodeId next_nodeid{0}; // Generate unique node ids.

    const auto ExistentOrNewNodeId = [&next_nodeid, &fdp](){
        if (next_nodeid == 0 || fdp.ConsumeBool()) {
            return next_nodeid++;
        }
        return fdp.ConsumeIntegralInRange<NodeId>(0, next_nodeid - 1);
    };

    const auto NumLiveSendsOf = [&node_sends](const CTransactionRef& tx) {
        size_t num{0};
        for (const auto& [_, node_send] : node_sends) {
            if (node_send.tx->GetWitnessHash() == tx->GetWitnessHash()) {
                ++num;
            }
        }
        return num;
    };

    LIMITED_WHILE(fdp.ConsumeBool(), 10000) {
        CallOneOf(
            fdp,
            [&] { // Add()
                CTransactionRef tx;
                if (transactions.empty() || fdp.ConsumeBool()) {
                    tx = MakeTransactionRef(ConsumeTransaction(fdp, std::nullopt));
                } else {
                    tx = PickIterator(fdp, transactions)->first;
                }

                const bool present_before{transactions.contains(tx)};
                const auto res{pb.Add(tx)};
                if (present_before) {
                    Assert(res == PrivateBroadcast::AddResult::AlreadyPresent);
                } else if (transactions.size() >= cap) {
                    Assert(res == PrivateBroadcast::AddResult::QueueFull);
                } else {
                    Assert(res == PrivateBroadcast::AddResult::Added);
                    transactions.emplace(tx, TxModel{});
                }
            },
            [&] { // Remove()
                if (transactions.empty()) {
                    return;
                }
                const auto transactions_it{PickIterator(fdp, transactions)};
                const CTransactionRef& tx{transactions_it->first};

                // Remove relevant in-flight entries from node_sends[] if any.
                std::erase_if(node_sends, [&tx](const auto& entry) {
                    return entry.second.tx->GetWitnessHash() == tx->GetWitnessHash();
                });

                const auto opt_num_confirmed{pb.Remove(tx)};

                Assert(opt_num_confirmed.has_value());
                Assert(opt_num_confirmed.value() == transactions_it->second.num_confirmed);
                Assert(!pb.Remove(tx).has_value());
                transactions.erase(transactions_it);
            },
            [&] { // PickTxForSend()
                // Only give pristine node ids to PickTxForSend() as required.
                const NodeId will_send_to_nodeid{next_nodeid++};
                const CService will_send_to_address{ConsumeService(fdp)};

                const auto opt_tx{pb.PickTxForSend(will_send_to_nodeid, will_send_to_address)};

                if (opt_tx.has_value()) {
                    Assert(transactions.contains(opt_tx.value()));

                    // "Number of times picked for sending" is the primary key in Priority's comparison
                    // (fewest sends = highest priority), so PickTxForSend() must return a transaction
                    // with the minimum send count of any in the queue. Ties are broken by state we
                    // don't model, so only check this key.
                    const size_t min_picked{std::ranges::min_element(
                        transactions, {}, [](const auto& el) { return el.second.num_picked; })->second.num_picked};
                    const auto picked_it{transactions.find(opt_tx.value())};
                    Assert(picked_it != transactions.end());
                    Assert(picked_it->second.num_picked == min_picked); // picked the least-sent transaction
                    ++picked_it->second.num_picked; // PickTxForSend() recorded exactly one send

                    const auto& [_, inserted]{node_sends.emplace(will_send_to_nodeid, NodeSendModel{.tx = opt_tx.value()})};
                    Assert(inserted);
                } else {
                    Assert(transactions.empty());
                }
            },
            [&] { // GetTxForNode()
                const NodeId nodeid{ExistentOrNewNodeId()};

                const auto opt_tx{pb.GetTxForNode(nodeid)};

                const auto it{node_sends.find(nodeid)};
                if (it != node_sends.end()) {
                    Assert(opt_tx.has_value());
                    Assert(opt_tx.value()->GetWitnessHash() == it->second.tx->GetWitnessHash());
                    Assert(transactions.contains(opt_tx.value()));
                } else {
                    Assert(!opt_tx.has_value());
                }
            },
            [&] { // NodeConfirmedReception()
                const NodeId nodeid{ExistentOrNewNodeId()};

                pb.NodeConfirmedReception(nodeid);

                const auto it{node_sends.find(nodeid)};
                if (it != node_sends.end() && !it->second.confirmed) {
                    // nodeid has an in-flight send, so NodeConfirmedReception() must have
                    // bumped the transaction's confirmation counter (exactly once per node).
                    it->second.confirmed = true;
                    const auto transactions_it{transactions.find(it->second.tx)};
                    Assert(transactions_it != transactions.end());
                    ++transactions_it->second.num_confirmed;
                }
            },
            [&] { // NodeDisconnected()
                const NodeId nodeid{ExistentOrNewNodeId()};

                const bool confirmed{pb.NodeDisconnected(nodeid)};

                const auto it{node_sends.find(nodeid)};
                if (it != node_sends.end()) {
                    Assert(confirmed == it->second.confirmed);
                    node_sends.erase(it);
                } else {
                    Assert(!confirmed);
                }

                // The per-node state is gone now.
                Assert(!pb.GetTxForNode(nodeid).has_value());
                Assert(!pb.NodeDisconnected(nodeid));
            },
            [&] { // HavePendingTransactions()
                if (pb.HavePendingTransactions()) {
                    Assert(!transactions.empty());
                } else {
                    Assert(transactions.empty());
                }
            },
            [&] { // GetStale()
                const auto stale{pb.GetStale()};

                Assert(stale.size() <= transactions.size());

                for (const auto& stale_tx : stale) {
                    Assert(transactions.contains(stale_tx));
                }
            },
            [&] { // GetBroadcastInfo()
                const auto all_broadcast_info{pb.GetBroadcastInfo()};

                Assert(all_broadcast_info.size() == transactions.size());

                for (const auto& info : all_broadcast_info) {
                    const auto it{transactions.find(info.tx)};
                    Assert(it != transactions.end());
                    Assert(info.num_sent == it->second.num_picked);
                    Assert(info.num_confirmed == it->second.num_confirmed);
                    // Per-peer entries exist only for in-flight sends to connected nodes,
                    // regardless of how many sends happened in total.
                    Assert(info.peers.size() == NumLiveSendsOf(info.tx));
                }
            },
            [&] {
                clock_ctx.set(ConsumeTime(fdp));
            });
    }
}
