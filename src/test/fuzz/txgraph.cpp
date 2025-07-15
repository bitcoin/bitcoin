// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cluster_linearize.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/random.h>
#include <txgraph.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <utility>

using namespace cluster_linearize;

namespace {

/** Data type representing a naive simulated TxGraph, keeping all transactions (even from
 *  disconnected components) in a single DepGraph. Unlike the real TxGraph, this only models
 *  a single graph, and multiple instances are used to simulate main/staging. */
struct SimTxGraph
{
    /** Maximum number of transactions to support simultaneously. Set this higher than txgraph's
     *  cluster count, so we can exercise situations with more transactions than fit in one
     *  cluster. */
    static constexpr unsigned MAX_TRANSACTIONS = MAX_CLUSTER_COUNT_LIMIT * 2;
    /** Set type to use in the simulation. */
    using SetType = BitSet<MAX_TRANSACTIONS>;
    /** Data type for representing positions within SimTxGraph::graph. */
    using Pos = DepGraphIndex;
    /** Constant to mean "missing in this graph". */
    static constexpr auto MISSING = Pos(-1);

    /** The dependency graph (for all transactions in the simulation, regardless of
     *  connectivity/clustering). */
    DepGraph<SetType> graph;
    /** For each position in graph, which TxGraph::Ref it corresponds with (if any). Use shared_ptr
     *  so that a SimTxGraph can be copied to create a staging one, while sharing Refs with
     *  the main graph. */
    std::array<std::shared_ptr<TxGraph::Ref>, MAX_TRANSACTIONS> simmap;
    /** For each TxGraph::Ref in graph, the position it corresponds with. */
    std::map<const TxGraph::Ref*, Pos> simrevmap;
    /** The set of TxGraph::Ref entries that have been removed, but not yet destroyed. */
    std::vector<std::shared_ptr<TxGraph::Ref>> removed;
    /** Whether the graph is oversized (true = yes, false = no, std::nullopt = unknown). */
    std::optional<bool> oversized;
    /** The configured maximum number of transactions per cluster. */
    DepGraphIndex max_cluster_count;
    /** Which transactions have been modified in the graph since creation, either directly or by
     *  being in a cluster which includes modifications. Only relevant for the staging graph. */
    SetType modified;
    /** The configured maximum total size of transactions per cluster. */
    uint64_t max_cluster_size;

    /** Construct a new SimTxGraph with the specified maximum cluster count and size. */
    explicit SimTxGraph(DepGraphIndex cluster_count, uint64_t cluster_size) :
        max_cluster_count(cluster_count), max_cluster_size(cluster_size) {}

    // Permit copying and moving.
    SimTxGraph(const SimTxGraph&) noexcept = default;
    SimTxGraph& operator=(const SimTxGraph&) noexcept = default;
    SimTxGraph(SimTxGraph&&) noexcept = default;
    SimTxGraph& operator=(SimTxGraph&&) noexcept = default;

    /** Get the connected components within this simulated transaction graph. */
    std::vector<SetType> GetComponents()
    {
        auto todo = graph.Positions();
        std::vector<SetType> ret;
        // Iterate over all connected components of the graph.
        while (todo.Any()) {
            auto component = graph.FindConnectedComponent(todo);
            ret.push_back(component);
            todo -= component;
        }
        return ret;
    }

    /** Check whether this graph is oversized (contains a connected component whose number of
     *  transactions exceeds max_cluster_count. */
    bool IsOversized()
    {
        if (!oversized.has_value()) {
            // Only recompute when oversized isn't already known.
            oversized = false;
            for (auto component : GetComponents()) {
                if (component.Count() > max_cluster_count) oversized = true;
                uint64_t component_size{0};
                for (auto i : component) component_size += graph.FeeRate(i).size;
                if (component_size > max_cluster_size) oversized = true;
            }
        }
        return *oversized;
    }

    void MakeModified(DepGraphIndex index)
    {
        modified |= graph.GetConnectedComponent(graph.Positions(), index);
    }

    /** Determine the number of (non-removed) transactions in the graph. */
    DepGraphIndex GetTransactionCount() const { return graph.TxCount(); }

    /** Get the sum of all fees/sizes in the graph. */
    FeePerWeight SumAll() const
    {
        FeePerWeight ret;
        for (auto i : graph.Positions()) {
            ret += graph.FeeRate(i);
        }
        return ret;
    }

    /** Get the position where ref occurs in this simulated graph, or -1 if it does not. */
    Pos Find(const TxGraph::Ref* ref) const
    {
        auto it = simrevmap.find(ref);
        if (it != simrevmap.end()) return it->second;
        return MISSING;
    }

    /** Given a position in this simulated graph, get the corresponding TxGraph::Ref. */
    TxGraph::Ref* GetRef(Pos pos)
    {
        assert(graph.Positions()[pos]);
        assert(simmap[pos]);
        return simmap[pos].get();
    }

    /** Add a new transaction to the simulation. */
    TxGraph::Ref* AddTransaction(const FeePerWeight& feerate)
    {
        assert(graph.TxCount() < MAX_TRANSACTIONS);
        auto simpos = graph.AddTransaction(feerate);
        MakeModified(simpos);
        assert(graph.Positions()[simpos]);
        simmap[simpos] = std::make_shared<TxGraph::Ref>();
        auto ptr = simmap[simpos].get();
        simrevmap[ptr] = simpos;
        // This may invalidate our cached oversized value.
        if (oversized.has_value() && !*oversized) oversized = std::nullopt;
        return ptr;
    }

    /** Add a dependency between two positions in this graph. */
    void AddDependency(TxGraph::Ref* parent, TxGraph::Ref* child)
    {
        auto par_pos = Find(parent);
        if (par_pos == MISSING) return;
        auto chl_pos = Find(child);
        if (chl_pos == MISSING) return;
        graph.AddDependencies(SetType::Singleton(par_pos), chl_pos);
        MakeModified(par_pos);
        // This may invalidate our cached oversized value.
        if (oversized.has_value() && !*oversized) oversized = std::nullopt;
    }

    /** Modify the transaction fee of a ref, if it exists. */
    void SetTransactionFee(TxGraph::Ref* ref, int64_t fee)
    {
        auto pos = Find(ref);
        if (pos == MISSING) return;
        // No need to invoke MakeModified, because this equally affects main and staging.
        graph.FeeRate(pos).fee = fee;
    }

    /** Remove the transaction in the specified position from the graph. */
    void RemoveTransaction(TxGraph::Ref* ref)
    {
        auto pos = Find(ref);
        if (pos == MISSING) return;
        MakeModified(pos);
        graph.RemoveTransactions(SetType::Singleton(pos));
        simrevmap.erase(simmap[pos].get());
        // Retain the TxGraph::Ref corresponding to this position, so the Ref destruction isn't
        // invoked until the simulation explicitly decided to do so.
        removed.push_back(std::move(simmap[pos]));
        simmap[pos].reset();
        // This may invalidate our cached oversized value.
        if (oversized.has_value() && *oversized) oversized = std::nullopt;
    }

    /** Destroy the transaction from the graph, including from the removed set. This will
     *  trigger TxGraph::Ref::~Ref. reset_oversize controls whether the cached oversized
     *  value is cleared (destroying does not clear oversizedness in TxGraph of the main
     *  graph while staging exists). */
    void DestroyTransaction(TxGraph::Ref* ref, bool reset_oversize)
    {
        auto pos = Find(ref);
        if (pos == MISSING) {
            // Wipe the ref, if it exists, from the removed vector. Use std::partition rather
            // than std::erase because we don't care about the order of the entries that
            // remain.
            auto remove = std::partition(removed.begin(), removed.end(), [&](auto& arg) { return arg.get() != ref; });
            removed.erase(remove, removed.end());
        } else {
            MakeModified(pos);
            graph.RemoveTransactions(SetType::Singleton(pos));
            simrevmap.erase(simmap[pos].get());
            simmap[pos].reset();
            // This may invalidate our cached oversized value.
            if (reset_oversize && oversized.has_value() && *oversized) {
                oversized = std::nullopt;
            }
        }
    }

    /** Construct the set with all positions in this graph corresponding to the specified
     *  TxGraph::Refs. All of them must occur in this graph and not be removed. */
    SetType MakeSet(std::span<TxGraph::Ref* const> arg)
    {
        SetType ret;
        for (TxGraph::Ref* ptr : arg) {
            auto pos = Find(ptr);
            assert(pos != Pos(-1));
            ret.Set(pos);
        }
        return ret;
    }

    /** Get the set of ancestors (desc=false) or descendants (desc=true) in this graph. */
    SetType GetAncDesc(TxGraph::Ref* arg, bool desc)
    {
        auto pos = Find(arg);
        if (pos == MISSING) return {};
        return desc ? graph.Descendants(pos) : graph.Ancestors(pos);
    }

    /** Given a set of Refs (given as a vector of pointers), expand the set to include all its
     *  ancestors (desc=false) or all its descendants (desc=true) in this graph. */
    void IncludeAncDesc(std::vector<TxGraph::Ref*>& arg, bool desc)
    {
        std::vector<TxGraph::Ref*> ret;
        for (auto ptr : arg) {
            auto simpos = Find(ptr);
            if (simpos != MISSING) {
                for (auto i : desc ? graph.Descendants(simpos) : graph.Ancestors(simpos)) {
                    ret.push_back(simmap[i].get());
                }
            } else {
                ret.push_back(ptr);
            }
        }
        // Construct deduplicated version in input (do not use std::sort/std::unique for
        // deduplication as it'd rely on non-deterministic pointer comparison).
        arg.clear();
        for (auto ptr : ret) {
            if (std::find(arg.begin(), arg.end(), ptr) == arg.end()) {
                arg.push_back(ptr);
            }
        }
    }


    /** Verify that set contains transactions from every oversized cluster, and nothing from
     *  non-oversized ones. */
    bool MatchesOversizedClusters(const SetType& set)
    {
        if (set.Any() && !IsOversized()) return false;

        auto todo = graph.Positions();
        if (!set.IsSubsetOf(todo)) return false;

        // Walk all clusters, and make sure all of set doesn't come from non-oversized clusters
        while (todo.Any()) {
            auto component = graph.FindConnectedComponent(todo);
            // Determine whether component is oversized, due to either the size or count limit.
            bool is_oversized = component.Count() > max_cluster_count;
            uint64_t component_size{0};
            for (auto i : component) component_size += graph.FeeRate(i).size;
            is_oversized |= component_size > max_cluster_size;
            // Check whether overlap with set matches is_oversized.
            if (is_oversized != set.Overlaps(component)) return false;
            todo -= component;
        }
        return true;
    }
};

} // namespace

FUZZ_TARGET(txgraph)
{
    // This is a big simulation test for TxGraph, which performs a fuzz-derived sequence of valid
    // operations on a TxGraph instance, as well as on a simpler (mostly) reimplementation (see
    // SimTxGraph above), comparing the outcome of functions that return a result, and finally
    // performing a full comparison between the two.

    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    /** Internal test RNG, used only for decisions which would require significant amount of data
     *  to be read from the provider, without realistically impacting test sensitivity, and for
     *  specialized test cases that are hard to perform more generically. */
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>());

    /** Variable used whenever an empty TxGraph::Ref is needed. */
    TxGraph::Ref empty_ref;

    /** The maximum number of transactions per (non-oversized) cluster we will use in this
     *  simulation. */
    auto max_cluster_count = provider.ConsumeIntegralInRange<DepGraphIndex>(1, MAX_CLUSTER_COUNT_LIMIT);
    /** The maximum total size of transactions in a (non-oversized) cluster. */
    auto max_cluster_size = provider.ConsumeIntegralInRange<uint64_t>(1, 0x3fffff * MAX_CLUSTER_COUNT_LIMIT);

    // Construct a real graph, and a vector of simulated graphs (main, and possibly staging).
    auto real = MakeTxGraph(max_cluster_count, max_cluster_size);
    std::vector<SimTxGraph> sims;
    sims.reserve(2);
    sims.emplace_back(max_cluster_count, max_cluster_size);

    /** Struct encapsulating information about a BlockBuilder that's currently live. */
    struct BlockBuilderData
    {
        /** BlockBuilder object from real. */
        std::unique_ptr<TxGraph::BlockBuilder> builder;
        /** The set of transactions marked as included in *builder. */
        SimTxGraph::SetType included;
        /** The set of transactions marked as included or skipped in *builder. */
        SimTxGraph::SetType done;
        /** The last chunk feerate returned by *builder. IsEmpty() if none yet. */
        FeePerWeight last_feerate;

        BlockBuilderData(std::unique_ptr<TxGraph::BlockBuilder> builder_in) : builder(std::move(builder_in)) {}
    };

    /** Currently active block builders. */
    std::vector<BlockBuilderData> block_builders;

    /** Function to pick any Ref (for either sim in sims: from sim.simmap or sim.removed, or the
     *  empty Ref). */
    auto pick_fn = [&]() noexcept -> TxGraph::Ref* {
        size_t tx_count[2] = {sims[0].GetTransactionCount(), 0};
        /** The number of possible choices. */
        size_t choices = tx_count[0] + sims[0].removed.size() + 1;
        if (sims.size() == 2) {
            tx_count[1] = sims[1].GetTransactionCount();
            choices += tx_count[1] + sims[1].removed.size();
        }
        /** Pick one of them. */
        auto choice = provider.ConsumeIntegralInRange<size_t>(0, choices - 1);
        // Consider both main and (if it exists) staging.
        for (size_t level = 0; level < sims.size(); ++level) {
            auto& sim = sims[level];
            if (choice < tx_count[level]) {
                // Return from graph.
                for (auto i : sim.graph.Positions()) {
                    if (choice == 0) return sim.GetRef(i);
                    --choice;
                }
                assert(false);
            } else {
                choice -= tx_count[level];
            }
            if (choice < sim.removed.size()) {
                // Return from removed.
                return sim.removed[choice].get();
            } else {
                choice -= sim.removed.size();
            }
        }
        // Return empty.
        assert(choice == 0);
        return &empty_ref;
    };

    /** Function to construct the correct fee-size diagram a real graph has based on its graph
     *  order (as reported by GetCluster(), so it works for both main and staging). */
    auto get_diagram_fn = [&](bool main_only) -> std::vector<FeeFrac> {
        int level = main_only ? 0 : sims.size() - 1;
        auto& sim = sims[level];
        // For every transaction in the graph, request its cluster, and throw them into a set.
        std::set<std::vector<TxGraph::Ref*>> clusters;
        for (auto i : sim.graph.Positions()) {
            auto ref = sim.GetRef(i);
            clusters.insert(real->GetCluster(*ref, main_only));
        }
        // Compute the chunkings of each (deduplicated) cluster.
        size_t num_tx{0};
        std::vector<FeeFrac> chunk_feerates;
        for (const auto& cluster : clusters) {
            num_tx += cluster.size();
            std::vector<SimTxGraph::Pos> linearization;
            linearization.reserve(cluster.size());
            for (auto refptr : cluster) linearization.push_back(sim.Find(refptr));
            for (const FeeFrac& chunk_feerate : ChunkLinearization(sim.graph, linearization)) {
                chunk_feerates.push_back(chunk_feerate);
            }
        }
        // Verify the number of transactions after deduplicating clusters. This implicitly verifies
        // that GetCluster on each element of a cluster reports the cluster transactions in the same
        // order.
        assert(num_tx == sim.GetTransactionCount());
        // Sort by feerate only, since violating topological constraints within same-feerate
        // chunks won't affect diagram comparisons.
        std::sort(chunk_feerates.begin(), chunk_feerates.end(), std::greater{});
        return chunk_feerates;
    };

    LIMITED_WHILE(provider.remaining_bytes() > 0, 200) {
        // Read a one-byte command.
        int command = provider.ConsumeIntegral<uint8_t>();
        int orig_command = command;

        // Treat the lowest bit of a command as a flag (which selects a variant of some of the
        // operations), and the second-lowest bit as a way of selecting main vs. staging, and leave
        // the rest of the bits in command.
        bool alt = command & 1;
        bool use_main = command & 2;
        command >>= 2;

        /** Use the bottom 2 bits of command to select an entry in the block_builders vector (if
         *  any). These use the same bits as alt/use_main, so don't use those in actions below
         *  where builder_idx is used as well. */
        int builder_idx = block_builders.empty() ? -1 : int((orig_command & 3) % block_builders.size());

        // Provide convenient aliases for the top simulated graph (main, or staging if it exists),
        // one for the simulated graph selected based on use_main (for operations that can operate
        // on both graphs), and one that always refers to the main graph.
        auto& top_sim = sims.back();
        auto& sel_sim = use_main ? sims[0] : top_sim;
        auto& main_sim = sims[0];

        // Keep decrementing command for each applicable operation, until one is hit. Multiple
        // iterations may be necessary.
        while (true) {
            if ((block_builders.empty() || sims.size() > 1) && top_sim.GetTransactionCount() < SimTxGraph::MAX_TRANSACTIONS && command-- == 0) {
                // AddTransaction.
                int64_t fee;
                int32_t size;
                if (alt) {
                    // If alt is true, pick fee and size from the entire range.
                    fee = provider.ConsumeIntegralInRange<int64_t>(-0x8000000000000, 0x7ffffffffffff);
                    size = provider.ConsumeIntegralInRange<int32_t>(1, 0x3fffff);
                } else {
                    // Otherwise, use smaller range which consume fewer fuzz input bytes, as just
                    // these are likely sufficient to trigger all interesting code paths already.
                    fee = provider.ConsumeIntegral<uint8_t>();
                    size = provider.ConsumeIntegralInRange<uint32_t>(1, 0xff);
                }
                FeePerWeight feerate{fee, size};
                // Create a real TxGraph::Ref.
                auto ref = real->AddTransaction(feerate);
                // Create a shared_ptr place in the simulation to put the Ref in.
                auto ref_loc = top_sim.AddTransaction(feerate);
                // Move it in place.
                *ref_loc = std::move(ref);
                break;
            } else if ((block_builders.empty() || sims.size() > 1) && top_sim.GetTransactionCount() + top_sim.removed.size() > 1 && command-- == 0) {
                // AddDependency.
                auto par = pick_fn();
                auto chl = pick_fn();
                auto pos_par = top_sim.Find(par);
                auto pos_chl = top_sim.Find(chl);
                if (pos_par != SimTxGraph::MISSING && pos_chl != SimTxGraph::MISSING) {
                    // Determine if adding this would introduce a cycle (not allowed by TxGraph),
                    // and if so, skip.
                    if (top_sim.graph.Ancestors(pos_par)[pos_chl]) break;
                }
                top_sim.AddDependency(par, chl);
                real->AddDependency(*par, *chl);
                break;
            } else if ((block_builders.empty() || sims.size() > 1) && top_sim.removed.size() < 100 && command-- == 0) {
                // RemoveTransaction. Either all its ancestors or all its descendants are also
                // removed (if any), to make sure TxGraph's reordering of removals and dependencies
                // has no effect.
                std::vector<TxGraph::Ref*> to_remove;
                to_remove.push_back(pick_fn());
                top_sim.IncludeAncDesc(to_remove, alt);
                // The order in which these ancestors/descendants are removed should not matter;
                // randomly shuffle them.
                std::shuffle(to_remove.begin(), to_remove.end(), rng);
                for (TxGraph::Ref* ptr : to_remove) {
                    real->RemoveTransaction(*ptr);
                    top_sim.RemoveTransaction(ptr);
                }
                break;
            } else if (sel_sim.removed.size() > 0 && command-- == 0) {
                // ~Ref (of an already-removed transaction). Destroying a TxGraph::Ref has an
                // observable effect on the TxGraph it refers to, so this simulation permits doing
                // so separately from other actions on TxGraph.

                // Pick a Ref of sel_sim.removed to destroy. Note that the same Ref may still occur
                // in the other graph, and thus not actually trigger ~Ref yet (which is exactly
                // what we want, as destroying Refs is only allowed when it does not refer to an
                // existing transaction in either graph).
                auto removed_pos = provider.ConsumeIntegralInRange<size_t>(0, sel_sim.removed.size() - 1);
                if (removed_pos != sel_sim.removed.size() - 1) {
                    std::swap(sel_sim.removed[removed_pos], sel_sim.removed.back());
                }
                sel_sim.removed.pop_back();
                break;
            } else if (block_builders.empty() && command-- == 0) {
                // ~Ref (of any transaction).
                std::vector<TxGraph::Ref*> to_destroy;
                to_destroy.push_back(pick_fn());
                while (true) {
                    // Keep adding either the ancestors or descendants the already picked
                    // transactions have in both graphs (main and staging) combined. Destroying
                    // will trigger deletions in both, so to have consistent TxGraph behavior, the
                    // set must be closed under ancestors, or descendants, in both graphs.
                    auto old_size = to_destroy.size();
                    for (auto& sim : sims) sim.IncludeAncDesc(to_destroy, alt);
                    if (to_destroy.size() == old_size) break;
                }
                // The order in which these ancestors/descendants are destroyed should not matter;
                // randomly shuffle them.
                std::shuffle(to_destroy.begin(), to_destroy.end(), rng);
                for (TxGraph::Ref* ptr : to_destroy) {
                    for (size_t level = 0; level < sims.size(); ++level) {
                        sims[level].DestroyTransaction(ptr, level == sims.size() - 1);
                    }
                }
                break;
            } else if (block_builders.empty() && command-- == 0) {
                // SetTransactionFee.
                int64_t fee;
                if (alt) {
                    fee = provider.ConsumeIntegralInRange<int64_t>(-0x8000000000000, 0x7ffffffffffff);
                } else {
                    fee = provider.ConsumeIntegral<uint8_t>();
                }
                auto ref = pick_fn();
                real->SetTransactionFee(*ref, fee);
                for (auto& sim : sims) {
                    sim.SetTransactionFee(ref, fee);
                }
                break;
            } else if (command-- == 0) {
                // GetTransactionCount.
                assert(real->GetTransactionCount(use_main) == sel_sim.GetTransactionCount());
                break;
            } else if (command-- == 0) {
                // Exists.
                auto ref = pick_fn();
                bool exists = real->Exists(*ref, use_main);
                bool should_exist = sel_sim.Find(ref) != SimTxGraph::MISSING;
                assert(exists == should_exist);
                break;
            } else if (command-- == 0) {
                // IsOversized.
                assert(sel_sim.IsOversized() == real->IsOversized(use_main));
                break;
            } else if (command-- == 0) {
                // GetIndividualFeerate.
                auto ref = pick_fn();
                auto feerate = real->GetIndividualFeerate(*ref);
                bool found{false};
                for (auto& sim : sims) {
                    auto simpos = sim.Find(ref);
                    if (simpos != SimTxGraph::MISSING) {
                        found = true;
                        assert(feerate == sim.graph.FeeRate(simpos));
                    }
                }
                if (!found) assert(feerate.IsEmpty());
                break;
            } else if (!main_sim.IsOversized() && command-- == 0) {
                // GetMainChunkFeerate.
                auto ref = pick_fn();
                auto feerate = real->GetMainChunkFeerate(*ref);
                auto simpos = main_sim.Find(ref);
                if (simpos == SimTxGraph::MISSING) {
                    assert(feerate.IsEmpty());
                } else {
                    // Just do some quick checks that the reported value is in range. A full
                    // recomputation of expected chunk feerates is done at the end.
                    assert(feerate.size >= main_sim.graph.FeeRate(simpos).size);
                    assert(feerate.size <= main_sim.SumAll().size);
                }
                break;
            } else if (!sel_sim.IsOversized() && command-- == 0) {
                // GetAncestors/GetDescendants.
                auto ref = pick_fn();
                auto result = alt ? real->GetDescendants(*ref, use_main)
                                  : real->GetAncestors(*ref, use_main);
                assert(result.size() <= max_cluster_count);
                auto result_set = sel_sim.MakeSet(result);
                assert(result.size() == result_set.Count());
                auto expect_set = sel_sim.GetAncDesc(ref, alt);
                assert(result_set == expect_set);
                break;
            } else if (!sel_sim.IsOversized() && command-- == 0) {
                // GetAncestorsUnion/GetDescendantsUnion.
                std::vector<TxGraph::Ref*> refs;
                // Gather a list of up to 15 Ref pointers.
                auto count = provider.ConsumeIntegralInRange<size_t>(0, 15);
                refs.resize(count);
                for (size_t i = 0; i < count; ++i) {
                    refs[i] = pick_fn();
                }
                // Their order should not matter, shuffle them.
                std::shuffle(refs.begin(), refs.end(), rng);
                // Invoke the real function, and convert to SimPos set.
                auto result = alt ? real->GetDescendantsUnion(refs, use_main)
                                  : real->GetAncestorsUnion(refs, use_main);
                auto result_set = sel_sim.MakeSet(result);
                assert(result.size() == result_set.Count());
                // Compute the expected result.
                SimTxGraph::SetType expect_set;
                for (TxGraph::Ref* ref : refs) expect_set |= sel_sim.GetAncDesc(ref, alt);
                // Compare.
                assert(result_set == expect_set);
                break;
            } else if (!sel_sim.IsOversized() && command-- == 0) {
                // GetCluster.
                auto ref = pick_fn();
                auto result = real->GetCluster(*ref, use_main);
                // Check cluster count limit.
                assert(result.size() <= max_cluster_count);
                // Require the result to be topologically valid and not contain duplicates.
                auto left = sel_sim.graph.Positions();
                uint64_t total_size{0};
                for (auto refptr : result) {
                    auto simpos = sel_sim.Find(refptr);
                    total_size += sel_sim.graph.FeeRate(simpos).size;
                    assert(simpos != SimTxGraph::MISSING);
                    assert(left[simpos]);
                    left.Reset(simpos);
                    assert(!sel_sim.graph.Ancestors(simpos).Overlaps(left));
                }
                // Check cluster size limit.
                assert(total_size <= max_cluster_size);
                // Require the set to be connected.
                auto result_set = sel_sim.MakeSet(result);
                assert(sel_sim.graph.IsConnected(result_set));
                // If ref exists, the result must contain it. If not, it must be empty.
                auto simpos = sel_sim.Find(ref);
                if (simpos != SimTxGraph::MISSING) {
                    assert(result_set[simpos]);
                } else {
                    assert(result_set.None());
                }
                // Require the set not to have ancestors or descendants outside of it.
                for (auto i : result_set) {
                    assert(sel_sim.graph.Ancestors(i).IsSubsetOf(result_set));
                    assert(sel_sim.graph.Descendants(i).IsSubsetOf(result_set));
                }
                break;
            } else if (command-- == 0) {
                // HaveStaging.
                assert((sims.size() == 2) == real->HaveStaging());
                break;
            } else if (sims.size() < 2 && command-- == 0) {
                // StartStaging.
                sims.emplace_back(sims.back());
                sims.back().modified = SimTxGraph::SetType{};
                real->StartStaging();
                break;
            } else if (block_builders.empty() && sims.size() > 1 && command-- == 0) {
                // CommitStaging.
                real->CommitStaging();
                sims.erase(sims.begin());
                break;
            } else if (sims.size() > 1 && command-- == 0) {
                // AbortStaging.
                real->AbortStaging();
                sims.pop_back();
                // Reset the cached oversized value (if TxGraph::Ref destructions triggered
                // removals of main transactions while staging was active, then aborting will
                // cause it to be re-evaluated in TxGraph).
                sims.back().oversized = std::nullopt;
                break;
            } else if (!main_sim.IsOversized() && command-- == 0) {
                // CompareMainOrder.
                auto ref_a = pick_fn();
                auto ref_b = pick_fn();
                auto sim_a = main_sim.Find(ref_a);
                auto sim_b = main_sim.Find(ref_b);
                // Both transactions must exist in the main graph.
                if (sim_a == SimTxGraph::MISSING || sim_b == SimTxGraph::MISSING) break;
                auto cmp = real->CompareMainOrder(*ref_a, *ref_b);
                // Distinct transactions have distinct places.
                if (sim_a != sim_b) assert(cmp != 0);
                // Ancestors go before descendants.
                if (main_sim.graph.Ancestors(sim_a)[sim_b]) assert(cmp >= 0);
                if (main_sim.graph.Descendants(sim_a)[sim_b]) assert(cmp <= 0);
                // Do not verify consistency with chunk feerates, as we cannot easily determine
                // these here without making more calls to real, which could affect its internal
                // state. A full comparison is done at the end.
                break;
            } else if (!sel_sim.IsOversized() && command-- == 0) {
                // CountDistinctClusters.
                std::vector<TxGraph::Ref*> refs;
                // Gather a list of up to 15 (or up to 255) Ref pointers.
                auto count = provider.ConsumeIntegralInRange<size_t>(0, alt ? 255 : 15);
                refs.resize(count);
                for (size_t i = 0; i < count; ++i) {
                    refs[i] = pick_fn();
                }
                // Their order should not matter, shuffle them.
                std::shuffle(refs.begin(), refs.end(), rng);
                // Invoke the real function.
                auto result = real->CountDistinctClusters(refs, use_main);
                // Build a set with representatives of the clusters the Refs occur in in the
                // simulated graph. For each, remember the lowest-index transaction SimPos in the
                // cluster.
                SimTxGraph::SetType sim_reps;
                for (auto ref : refs) {
                    // Skip Refs that do not occur in the simulated graph.
                    auto simpos = sel_sim.Find(ref);
                    if (simpos == SimTxGraph::MISSING) continue;
                    // Find the component that includes ref.
                    auto component = sel_sim.graph.GetConnectedComponent(sel_sim.graph.Positions(), simpos);
                    // Remember the lowest-index SimPos in component, as a representative for it.
                    assert(component.Any());
                    sim_reps.Set(component.First());
                }
                // Compare the number of deduplicated representatives with the value returned by
                // the real function.
                assert(result == sim_reps.Count());
                break;
            } else if (command-- == 0) {
                // DoWork.
                real->DoWork();
                break;
            } else if (sims.size() == 2 && !sims[0].IsOversized() && !sims[1].IsOversized() && command-- == 0) {
                // GetMainStagingDiagrams()
                auto [real_main_diagram, real_staged_diagram] = real->GetMainStagingDiagrams();
                auto real_sum_main = std::accumulate(real_main_diagram.begin(), real_main_diagram.end(), FeeFrac{});
                auto real_sum_staged = std::accumulate(real_staged_diagram.begin(), real_staged_diagram.end(), FeeFrac{});
                auto real_gain = real_sum_staged - real_sum_main;
                auto sim_gain = sims[1].SumAll() - sims[0].SumAll();
                // Just check that the total fee gained/lost and size gained/lost according to the
                // diagram matches the difference in these values in the simulated graph. A more
                // complete check of the GetMainStagingDiagrams result is performed at the end.
                assert(sim_gain == real_gain);
                // Check that the feerates in each diagram are monotonically decreasing.
                for (size_t i = 1; i < real_main_diagram.size(); ++i) {
                    assert(FeeRateCompare(real_main_diagram[i], real_main_diagram[i - 1]) <= 0);
                }
                for (size_t i = 1; i < real_staged_diagram.size(); ++i) {
                    assert(FeeRateCompare(real_staged_diagram[i], real_staged_diagram[i - 1]) <= 0);
                }
                break;
            } else if (block_builders.size() < 4 && !main_sim.IsOversized() && command-- == 0) {
                // GetBlockBuilder.
                block_builders.emplace_back(real->GetBlockBuilder());
                break;
            } else if (!block_builders.empty() && command-- == 0) {
                // ~BlockBuilder.
                block_builders.erase(block_builders.begin() + builder_idx);
                break;
            } else if (!block_builders.empty() && command-- == 0) {
                // BlockBuilder::GetCurrentChunk, followed by Include/Skip.
                auto& builder_data = block_builders[builder_idx];
                auto new_included = builder_data.included;
                auto new_done = builder_data.done;
                auto chunk = builder_data.builder->GetCurrentChunk();
                if (chunk) {
                    // Chunk feerates must be monotonously decreasing.
                    if (!builder_data.last_feerate.IsEmpty()) {
                        assert(!(chunk->second >> builder_data.last_feerate));
                    }
                    builder_data.last_feerate = chunk->second;
                    // Verify the contents of GetCurrentChunk.
                    FeePerWeight sum_feerate;
                    for (TxGraph::Ref* ref : chunk->first) {
                        // Each transaction in the chunk must exist in the main graph.
                        auto simpos = main_sim.Find(ref);
                        assert(simpos != SimTxGraph::MISSING);
                        // Verify the claimed chunk feerate.
                        sum_feerate += main_sim.graph.FeeRate(simpos);
                        // Make sure no transaction is reported twice.
                        assert(!new_done[simpos]);
                        new_done.Set(simpos);
                        // The concatenation of all included transactions must be topologically valid.
                        new_included.Set(simpos);
                        assert(main_sim.graph.Ancestors(simpos).IsSubsetOf(new_included));
                    }
                    assert(sum_feerate == chunk->second);
                } else {
                    // When we reach the end, if nothing was skipped, the entire graph should have
                    // been reported.
                    if (builder_data.done == builder_data.included) {
                        assert(builder_data.done.Count() == main_sim.GetTransactionCount());
                    }
                }
                // Possibly invoke GetCurrentChunk() again, which should give the same result.
                if ((orig_command % 7) >= 5) {
                    auto chunk2 = builder_data.builder->GetCurrentChunk();
                    assert(chunk == chunk2);
                }
                // Skip or include.
                if ((orig_command % 5) >= 3) {
                    // Skip.
                    builder_data.builder->Skip();
                } else {
                    // Include.
                    builder_data.builder->Include();
                    builder_data.included = new_included;
                }
                builder_data.done = new_done;
                break;
            } else if (!main_sim.IsOversized() && command-- == 0) {
                // GetWorstMainChunk.
                auto [worst_chunk, worst_chunk_feerate] = real->GetWorstMainChunk();
                // Just do some sanity checks here. Consistency with GetBlockBuilder is checked
                // below.
                if (main_sim.GetTransactionCount() == 0) {
                    assert(worst_chunk.empty());
                    assert(worst_chunk_feerate.IsEmpty());
                } else {
                    assert(!worst_chunk.empty());
                    SimTxGraph::SetType done;
                    FeePerWeight sum;
                    for (TxGraph::Ref* ref : worst_chunk) {
                        // Each transaction in the chunk must exist in the main graph.
                        auto simpos = main_sim.Find(ref);
                        assert(simpos != SimTxGraph::MISSING);
                        sum += main_sim.graph.FeeRate(simpos);
                        // Make sure the chunk contains no duplicate transactions.
                        assert(!done[simpos]);
                        done.Set(simpos);
                        // All elements are preceded by all their descendants.
                        assert(main_sim.graph.Descendants(simpos).IsSubsetOf(done));
                    }
                    assert(sum == worst_chunk_feerate);
                }
                break;
            } else if ((block_builders.empty() || sims.size() > 1) && command-- == 0) {
                // Trim.
                bool was_oversized = top_sim.IsOversized();
                auto removed = real->Trim();
                // Verify that something was removed if and only if there was an oversized cluster.
                assert(was_oversized == !removed.empty());
                if (!was_oversized) break;
                auto removed_set = top_sim.MakeSet(removed);
                // The removed set must contain all its own descendants.
                for (auto simpos : removed_set) {
                    assert(top_sim.graph.Descendants(simpos).IsSubsetOf(removed_set));
                }
                // Something from every oversized cluster should have been removed, and nothing
                // else.
                assert(top_sim.MatchesOversizedClusters(removed_set));

                // Apply all removals to the simulation, and verify the result is no longer
                // oversized. Don't query the real graph for oversizedness; it is compared
                // against the simulation anyway later.
                for (auto simpos : removed_set) {
                    top_sim.RemoveTransaction(top_sim.GetRef(simpos));
                }
                assert(!top_sim.IsOversized());
                break;
            } else if ((block_builders.empty() || sims.size() > 1) &&
                       top_sim.GetTransactionCount() > max_cluster_count && !top_sim.IsOversized() && command-- == 0) {
                // Trim (special case which avoids apparent cycles in the implicit approximate
                // dependency graph constructed inside the Trim() implementation). This is worth
                // testing separately, because such cycles cannot occur in realistic scenarios,
                // but this is hard to replicate in general in this fuzz test.

                // First, we need to have dependencies applied and linearizations fixed to avoid
                // circular dependencies in implied graph; trigger it via whatever means.
                real->CountDistinctClusters({}, false);

                // Gather the current clusters.
                auto clusters = top_sim.GetComponents();

                // Merge clusters randomly until at least one oversized one appears.
                bool made_oversized = false;
                auto merges_left = clusters.size() - 1;
                while (merges_left > 0) {
                    --merges_left;
                    // Find positions of clusters in the clusters vector to merge together.
                    auto par_cl = rng.randrange(clusters.size());
                    auto chl_cl = rng.randrange(clusters.size() - 1);
                    chl_cl += (chl_cl >= par_cl);
                    Assume(chl_cl != par_cl);
                    // Add between 1 and 3 dependencies between them. As all are in the same
                    // direction (from the child cluster to parent cluster), no cycles are possible,
                    // regardless of what internal topology Trim() uses as approximation within the
                    // clusters.
                    int num_deps = rng.randrange(3) + 1;
                    for (int i = 0; i < num_deps; ++i) {
                        // Find a parent transaction in the parent cluster.
                        auto par_idx = rng.randrange(clusters[par_cl].Count());
                        SimTxGraph::Pos par_pos = 0;
                        for (auto j : clusters[par_cl]) {
                            if (par_idx == 0) {
                                par_pos = j;
                                break;
                            }
                            --par_idx;
                        }
                        // Find a child transaction in the child cluster.
                        auto chl_idx = rng.randrange(clusters[chl_cl].Count());
                        SimTxGraph::Pos chl_pos = 0;
                        for (auto j : clusters[chl_cl]) {
                            if (chl_idx == 0) {
                                chl_pos = j;
                                break;
                            }
                            --chl_idx;
                        }
                        // Add dependency to both simulation and real TxGraph.
                        auto par_ref = top_sim.GetRef(par_pos);
                        auto chl_ref = top_sim.GetRef(chl_pos);
                        top_sim.AddDependency(par_ref, chl_ref);
                        real->AddDependency(*par_ref, *chl_ref);
                    }
                    // Compute the combined cluster.
                    auto par_cluster = clusters[par_cl];
                    auto chl_cluster = clusters[chl_cl];
                    auto new_cluster = par_cluster | chl_cluster;
                    // Remove the parent and child cluster from clusters.
                    std::erase_if(clusters, [&](const auto& cl) noexcept { return cl == par_cluster || cl == chl_cluster; });
                    // Add the combined cluster.
                    clusters.push_back(new_cluster);
                    // If this is the first merge that causes an oversized cluster to appear, pick
                    // a random number of further merges to appear.
                    if (!made_oversized) {
                        made_oversized = new_cluster.Count() > max_cluster_count;
                        if (!made_oversized) {
                            FeeFrac total;
                            for (auto i : new_cluster) total += top_sim.graph.FeeRate(i);
                            if (uint32_t(total.size) > max_cluster_size) made_oversized = true;
                        }
                        if (made_oversized) merges_left = rng.randrange(clusters.size());
                    }
                }

                // Determine an upper bound on how many transactions are removed.
                uint32_t max_removed = 0;
                for (auto& cluster : clusters) {
                    // Gather all transaction sizes in the to-be-combined cluster.
                    std::vector<uint32_t> sizes;
                    for (auto i : cluster) sizes.push_back(top_sim.graph.FeeRate(i).size);
                    auto sum_sizes = std::accumulate(sizes.begin(), sizes.end(), uint64_t{0});
                    // Sort from large to small.
                    std::sort(sizes.begin(), sizes.end(), std::greater{});
                    // In the worst case, only the smallest transactions are removed.
                    while (sizes.size() > max_cluster_count || sum_sizes > max_cluster_size) {
                        sum_sizes -= sizes.back();
                        sizes.pop_back();
                        ++max_removed;
                    }
                }

                // Invoke Trim now on the definitely-oversized txgraph.
                auto removed = real->Trim();
                // Verify that the number of removals is within range.
                assert(removed.size() >= 1);
                assert(removed.size() <= max_removed);
                // The removed set must contain all its own descendants.
                auto removed_set = top_sim.MakeSet(removed);
                for (auto simpos : removed_set) {
                    assert(top_sim.graph.Descendants(simpos).IsSubsetOf(removed_set));
                }
                // Something from every oversized cluster should have been removed, and nothing
                // else.
                assert(top_sim.MatchesOversizedClusters(removed_set));

                // Apply all removals to the simulation, and verify the result is no longer
                // oversized. Don't query the real graph for oversizedness; it is compared
                // against the simulation anyway later.
                for (auto simpos : removed_set) {
                    top_sim.RemoveTransaction(top_sim.GetRef(simpos));
                }
                assert(!top_sim.IsOversized());
                break;
            }
        }
    }

    // After running all modifications, perform an internal sanity check (before invoking
    // inspectors that may modify the internal state).
    real->SanityCheck();

    if (!sims[0].IsOversized()) {
        // If the main graph is not oversized, verify the total ordering implied by
        // CompareMainOrder.
        // First construct two distinct randomized permutations of the positions in sims[0].
        std::vector<SimTxGraph::Pos> vec1;
        for (auto i : sims[0].graph.Positions()) vec1.push_back(i);
        std::shuffle(vec1.begin(), vec1.end(), rng);
        auto vec2 = vec1;
        std::shuffle(vec2.begin(), vec2.end(), rng);
        if (vec1 == vec2) std::next_permutation(vec2.begin(), vec2.end());
        // Sort both according to CompareMainOrder. By having randomized starting points, the order
        // of CompareMainOrder invocations is somewhat randomized as well.
        auto cmp = [&](SimTxGraph::Pos a, SimTxGraph::Pos b) noexcept {
            return real->CompareMainOrder(*sims[0].GetRef(a), *sims[0].GetRef(b)) < 0;
        };
        std::sort(vec1.begin(), vec1.end(), cmp);
        std::sort(vec2.begin(), vec2.end(), cmp);

        // Verify the resulting orderings are identical. This could only fail if the ordering was
        // not total.
        assert(vec1 == vec2);

        // Verify that the ordering is topological.
        auto todo = sims[0].graph.Positions();
        for (auto i : vec1) {
            todo.Reset(i);
            assert(!sims[0].graph.Ancestors(i).Overlaps(todo));
        }
        assert(todo.None());

        // For every transaction in the total ordering, find a random one before it and after it,
        // and compare their chunk feerates, which must be consistent with the ordering.
        for (size_t pos = 0; pos < vec1.size(); ++pos) {
            auto pos_feerate = real->GetMainChunkFeerate(*sims[0].GetRef(vec1[pos]));
            if (pos > 0) {
                size_t before = rng.randrange<size_t>(pos);
                auto before_feerate = real->GetMainChunkFeerate(*sims[0].GetRef(vec1[before]));
                assert(FeeRateCompare(before_feerate, pos_feerate) >= 0);
            }
            if (pos + 1 < vec1.size()) {
                size_t after = pos + 1 + rng.randrange<size_t>(vec1.size() - 1 - pos);
                auto after_feerate = real->GetMainChunkFeerate(*sims[0].GetRef(vec1[after]));
                assert(FeeRateCompare(after_feerate, pos_feerate) <= 0);
            }
        }

        // The same order should be obtained through a BlockBuilder as implied by CompareMainOrder,
        // if nothing is skipped.
        auto builder = real->GetBlockBuilder();
        std::vector<SimTxGraph::Pos> vec_builder;
        std::vector<TxGraph::Ref*> last_chunk;
        FeePerWeight last_chunk_feerate;
        while (auto chunk = builder->GetCurrentChunk()) {
            FeePerWeight sum;
            for (TxGraph::Ref* ref : chunk->first) {
                // The reported chunk feerate must match the chunk feerate obtained by asking
                // it for each of the chunk's transactions individually.
                assert(real->GetMainChunkFeerate(*ref) == chunk->second);
                // Verify the chunk feerate matches the sum of the reported individual feerates.
                sum += real->GetIndividualFeerate(*ref);
                // Chunks must contain transactions that exist in the graph.
                auto simpos = sims[0].Find(ref);
                assert(simpos != SimTxGraph::MISSING);
                vec_builder.push_back(simpos);
            }
            assert(sum == chunk->second);
            last_chunk = std::move(chunk->first);
            last_chunk_feerate = chunk->second;
            builder->Include();
        }
        assert(vec_builder == vec1);

        // The last chunk returned by the BlockBuilder must match GetWorstMainChunk, in reverse.
        std::reverse(last_chunk.begin(), last_chunk.end());
        auto [worst_chunk, worst_chunk_feerate] = real->GetWorstMainChunk();
        assert(last_chunk == worst_chunk);
        assert(last_chunk_feerate == worst_chunk_feerate);

        // Check that the implied ordering gives rise to a combined diagram that matches the
        // diagram constructed from the individual cluster linearization chunkings.
        auto main_real_diagram = get_diagram_fn(/*main_only=*/true);
        auto main_implied_diagram = ChunkLinearization(sims[0].graph, vec1);
        assert(CompareChunks(main_real_diagram, main_implied_diagram) == 0);

        if (sims.size() >= 2 && !sims[1].IsOversized()) {
            // When the staging graph is not oversized as well, call GetMainStagingDiagrams, and
            // fully verify the result.
            auto [main_cmp_diagram, stage_cmp_diagram] = real->GetMainStagingDiagrams();
            // Check that the feerates in each diagram are monotonically decreasing.
            for (size_t i = 1; i < main_cmp_diagram.size(); ++i) {
                assert(FeeRateCompare(main_cmp_diagram[i], main_cmp_diagram[i - 1]) <= 0);
            }
            for (size_t i = 1; i < stage_cmp_diagram.size(); ++i) {
                assert(FeeRateCompare(stage_cmp_diagram[i], stage_cmp_diagram[i - 1]) <= 0);
            }
            // Treat the diagrams as sets of chunk feerates, and sort them in the same way so that
            // std::set_difference can be used on them below. The exact ordering does not matter
            // here, but it has to be consistent with the one used in main_real_diagram and
            // stage_real_diagram).
            std::sort(main_cmp_diagram.begin(), main_cmp_diagram.end(), std::greater{});
            std::sort(stage_cmp_diagram.begin(), stage_cmp_diagram.end(), std::greater{});
            // Find the chunks that appear in main_diagram but are missing from main_cmp_diagram.
            // This is allowed, because GetMainStagingDiagrams omits clusters in main unaffected
            // by staging.
            std::vector<FeeFrac> missing_main_cmp;
            std::set_difference(main_real_diagram.begin(), main_real_diagram.end(),
                                main_cmp_diagram.begin(), main_cmp_diagram.end(),
                                std::inserter(missing_main_cmp, missing_main_cmp.end()),
                                std::greater{});
            assert(main_cmp_diagram.size() + missing_main_cmp.size() == main_real_diagram.size());
            // Do the same for chunks in stage_diagram missing from stage_cmp_diagram.
            auto stage_real_diagram = get_diagram_fn(/*main_only=*/false);
            std::vector<FeeFrac> missing_stage_cmp;
            std::set_difference(stage_real_diagram.begin(), stage_real_diagram.end(),
                                stage_cmp_diagram.begin(), stage_cmp_diagram.end(),
                                std::inserter(missing_stage_cmp, missing_stage_cmp.end()),
                                std::greater{});
            assert(stage_cmp_diagram.size() + missing_stage_cmp.size() == stage_real_diagram.size());
            // The missing chunks must be equal across main & staging (otherwise they couldn't have
            // been omitted).
            assert(missing_main_cmp == missing_stage_cmp);

            // The missing part must include at least all transactions in staging which have not been
            // modified, or been in a cluster together with modified transactions, since they were
            // copied from main. Note that due to the reordering of removals w.r.t. dependency
            // additions, it is possible that the real implementation found more unaffected things.
            FeeFrac missing_real;
            for (const auto& feerate : missing_main_cmp) missing_real += feerate;
            FeeFrac missing_expected = sims[1].graph.FeeRate(sims[1].graph.Positions() - sims[1].modified);
            // Note that missing_real.fee < missing_expected.fee is possible to due the presence of
            // negative-fee transactions.
            assert(missing_real.size >= missing_expected.size);
        }
    }

    assert(real->HaveStaging() == (sims.size() > 1));

    // Try to run a full comparison, for both main_only=false and main_only=true in TxGraph
    // inspector functions that support both.
    for (int main_only = 0; main_only < 2; ++main_only) {
        auto& sim = main_only ? sims[0] : sims.back();
        // Compare simple properties of the graph with the simulation.
        assert(real->IsOversized(main_only) == sim.IsOversized());
        assert(real->GetTransactionCount(main_only) == sim.GetTransactionCount());
        // If the graph (and the simulation) are not oversized, perform a full comparison.
        if (!sim.IsOversized()) {
            auto todo = sim.graph.Positions();
            // Iterate over all connected components of the resulting (simulated) graph, each of which
            // should correspond to a cluster in the real one.
            while (todo.Any()) {
                auto component = sim.graph.FindConnectedComponent(todo);
                todo -= component;
                // Iterate over the transactions in that component.
                for (auto i : component) {
                    // Check its individual feerate against simulation.
                    assert(sim.graph.FeeRate(i) == real->GetIndividualFeerate(*sim.GetRef(i)));
                    // Check its ancestors against simulation.
                    auto expect_anc = sim.graph.Ancestors(i);
                    auto anc = sim.MakeSet(real->GetAncestors(*sim.GetRef(i), main_only));
                    assert(anc.Count() <= max_cluster_count);
                    assert(anc == expect_anc);
                    // Check its descendants against simulation.
                    auto expect_desc = sim.graph.Descendants(i);
                    auto desc = sim.MakeSet(real->GetDescendants(*sim.GetRef(i), main_only));
                    assert(desc.Count() <= max_cluster_count);
                    assert(desc == expect_desc);
                    // Check the cluster the transaction is part of.
                    auto cluster = real->GetCluster(*sim.GetRef(i), main_only);
                    assert(cluster.size() <= max_cluster_count);
                    assert(sim.MakeSet(cluster) == component);
                    // Check that the cluster is reported in a valid topological order (its
                    // linearization).
                    std::vector<DepGraphIndex> simlin;
                    SimTxGraph::SetType done;
                    uint64_t total_size{0};
                    for (TxGraph::Ref* ptr : cluster) {
                        auto simpos = sim.Find(ptr);
                        assert(sim.graph.Descendants(simpos).IsSubsetOf(component - done));
                        done.Set(simpos);
                        assert(sim.graph.Ancestors(simpos).IsSubsetOf(done));
                        simlin.push_back(simpos);
                        total_size += sim.graph.FeeRate(simpos).size;
                    }
                    // Check cluster size.
                    assert(total_size <= max_cluster_size);
                    // Construct a chunking object for the simulated graph, using the reported cluster
                    // linearization as ordering, and compare it against the reported chunk feerates.
                    if (sims.size() == 1 || main_only) {
                        cluster_linearize::LinearizationChunking simlinchunk(sim.graph, simlin);
                        DepGraphIndex idx{0};
                        for (unsigned chunknum = 0; chunknum < simlinchunk.NumChunksLeft(); ++chunknum) {
                            auto chunk = simlinchunk.GetChunk(chunknum);
                            // Require that the chunks of cluster linearizations are connected (this must
                            // be the case as all linearizations inside are PostLinearized).
                            assert(sim.graph.IsConnected(chunk.transactions));
                            // Check the chunk feerates of all transactions in the cluster.
                            while (chunk.transactions.Any()) {
                                assert(chunk.transactions[simlin[idx]]);
                                chunk.transactions.Reset(simlin[idx]);
                                assert(chunk.feerate == real->GetMainChunkFeerate(*cluster[idx]));
                                ++idx;
                            }
                        }
                    }
                }
            }
        }
    }

    // Sanity check again (because invoking inspectors may modify internal unobservable state).
    real->SanityCheck();

    // Kill the block builders.
    block_builders.clear();
    // Kill the TxGraph object.
    real.reset();
    // Kill the simulated graphs, with all remaining Refs in it. If any, this verifies that Refs
    // can outlive the TxGraph that created them.
    sims.clear();
}
