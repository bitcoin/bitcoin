// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <private_broadcast.h>

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <net.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/check.h>
#include <util/overflow.h>
#include <util/time.h>

#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

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
    const size_t max_send_attempts{fdp.ConsumeIntegralInRange<size_t>(1, 12)};
    PrivateBroadcast pb{cap, max_send_attempts};

    // Random transaction that the test generated and passed to Add(). Trimmed when Remove() is called.
    // The values are the number of times a transaction was picked for sending.
    std::unordered_map<CTransactionRef, size_t, CTransactionRefHash, CTransactionRefComp> transactions;

    // Transactions passed to PickTxForSend(), indexed by node id. Trimmed when
    // Remove() is called or a transaction is reset by Add().
    std::unordered_map<NodeId, CTransactionRef> nodes_sent_to;

    // A subset of `nodes_sent_to`, node ids passed to NodeConfirmedReception().
    // Trimmed when Remove() is called or a transaction is reset by Add().
    std::unordered_set<NodeId> nodes_that_confirmed_reception;

    NodeId next_nodeid{0}; // Generate unique node ids.

    const auto is_pending{[max_send_attempts](const auto& entry) {
        return entry.second < max_send_attempts;
    }};

    const auto ExistentOrNewNodeId = [&next_nodeid, &fdp](){
        if (next_nodeid == 0 || fdp.ConsumeBool()) {
            return next_nodeid++;
        }
        return fdp.ConsumeIntegralInRange<NodeId>(0, next_nodeid - 1);
    };

    LIMITED_WHILE (fdp.ConsumeBool(), 10000) {
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
                    auto tx_it{transactions.find(tx)};
                    Assert(tx_it != transactions.end());
                    if (is_pending(*tx_it)) {
                        Assert(res == PrivateBroadcast::AddResult::AlreadyPresent);
                    } else {
                        Assert(res == PrivateBroadcast::AddResult::Added);
                        tx_it->second = 0;
                        for (auto it = nodes_sent_to.begin(); it != nodes_sent_to.end();) {
                            if (CTransactionRefComp{}(it->second, tx)) {
                                nodes_that_confirmed_reception.erase(it->first);
                                it = nodes_sent_to.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                } else if (transactions.size() >= cap) {
                    Assert(res == PrivateBroadcast::AddResult::QueueFull);
                } else {
                    Assert(res == PrivateBroadcast::AddResult::Added);
                    transactions.emplace(tx, 0);
                }
            },
            [&] { // Remove()
                if (transactions.empty()) {
                    return;
                }
                const auto transactions_it{PickIterator(fdp, transactions)};
                const CTransactionRef& tx{transactions_it->first};

                size_t num_nodes_that_confirmed_tx{0};

                // Remove relevant entries from nodes_sent_to[] and nodes_that_confirmed_reception[] if any.
                for (auto it = nodes_sent_to.begin(); it != nodes_sent_to.end();) {
                    const NodeId nodeid{it->first};
                    if (CTransactionRefComp{}(it->second, tx)) {
                        it = nodes_sent_to.erase(it);
                        if (nodes_that_confirmed_reception.erase(nodeid) > 0) {
                            ++num_nodes_that_confirmed_tx;
                        }
                    } else {
                        ++it;
                    }
                }

                const auto opt_num_confirmed{pb.Remove(tx)};

                Assert(opt_num_confirmed.has_value());
                Assert(opt_num_confirmed.value() == num_nodes_that_confirmed_tx);
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
                    auto pending_transactions{transactions | std::views::filter(is_pending)};
                    const size_t min_picked{std::ranges::min_element(
                        pending_transactions, {}, [](const auto& el) { return el.second; })->second};
                    const auto picked_it{transactions.find(opt_tx.value())};
                    Assert(picked_it != transactions.end());
                    Assert(picked_it->second == min_picked); // picked the least-sent transaction
                    ++picked_it->second; // PickTxForSend() recorded exactly one send

                    const auto& [_, inserted]{nodes_sent_to.emplace(will_send_to_nodeid, opt_tx.value())};
                    Assert(inserted);
                } else {
                    Assert(std::ranges::none_of(transactions, is_pending));
                }
            },
            [&] { // GetTxForNode()
                const NodeId nodeid{ExistentOrNewNodeId()};

                const auto opt_tx{pb.GetTxForNode(nodeid)};

                if (nodes_sent_to.contains(nodeid)) {
                    Assert(opt_tx.has_value());
                    Assert(transactions.contains(opt_tx.value()));
                    Assert(opt_tx.value() == nodes_sent_to.at(nodeid));
                } else {
                    Assert(!opt_tx.has_value());
                }
            },
            [&] { // NodeConfirmedReception()
                const NodeId nodeid{ExistentOrNewNodeId()};

                pb.NodeConfirmedReception(nodeid);

                if (nodes_sent_to.contains(nodeid)) {
                    // nodeid was previously passed to PickTxForSend(), so NodeConfirmedReception()
                    // must have changed the internal state. Remember this to later check that
                    // DidNodeConfirmReception() works correctly.
                    nodes_that_confirmed_reception.emplace(nodeid);
                }
            },
            [&] { // DidNodeConfirmReception()
                const NodeId nodeid{ExistentOrNewNodeId()};

                const bool confirmed{pb.DidNodeConfirmReception(nodeid)};

                if (nodes_that_confirmed_reception.contains(nodeid)) {
                    Assert(confirmed);
                } else {
                    Assert(!confirmed);
                }
            },
            [&] { // HavePendingTransactions()
                if (std::ranges::any_of(transactions, is_pending)) {
                    Assert(pb.HavePendingTransactions());
                } else {
                    Assert(!pb.HavePendingTransactions());
                }
            },
            [&] { // GetStale()
                const auto stale{pb.GetStale()};

                Assert(stale.size() <= transactions.size());

                for (const auto& stale_tx : stale) {
                    const auto it{transactions.find(stale_tx)};
                    Assert(it != transactions.end());
                    Assert(is_pending(*it));
                }
            },
            [&] { // GetBroadcastInfo()
                const auto all_broadcast_info{pb.GetBroadcastInfo()};

                Assert(all_broadcast_info.size() == transactions.size());

                for (const auto& info : all_broadcast_info) {
                    const auto it{transactions.find(info.tx)};
                    Assert(it != transactions.end());
                    Assert(info.peers.size() == it->second); // exactly the sends we recorded
                    Assert(info.attempts_remaining == max_send_attempts - it->second);
                }
            },
            [&] {
                clock_ctx.set(ConsumeTime(fdp));
            });
    }
}
