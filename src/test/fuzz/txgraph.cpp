// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txgraph.h>
#include <cluster_linearize.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/util/random.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <algorithm>
#include <map>
#include <memory>
#include <stdint.h>
#include <utility>

using namespace cluster_linearize;

namespace {

/** Data type representing a naive simulated TxGraph, keeping all transactions (even from
 *  disconnected components) in a single DepGraph. */
struct SimTxGraph
{
    /** Maximum number of transactions to support simultaneously. Set this higher than txgraph's
     *  cluster count, so we can exercise situations with more transactions than fit in one
     *  cluster. */
    static constexpr unsigned MAX_TRANSACTIONS = CLUSTER_COUNT_LIMIT * 2;
    /** Set type to use in the simulation. */
    using SetType = BitSet<MAX_TRANSACTIONS>;
    /** Data type for representing positions within SimTxGraph::graph. */
    using Pos = DepGraphIndex;
    /** Constant to mean "missing in this graph". */
    static constexpr auto MISSING = Pos(-1);

    /** The dependency graph (for all transactions in the simulation, regardless of
     *  connectivity/clustering). */
    DepGraph<SetType> graph;
    /** For each position in graph, which TxGraph::Ref it corresponds with (if any). */
    std::array<std::unique_ptr<TxGraph::Ref>, MAX_TRANSACTIONS> simmap;
    /** For each TxGraph::Ref in graph, the position it corresponds with. */
    std::map<const TxGraph::Ref*, Pos> simrevmap;
    /** The set of TxGraph::Ref entries that have been removed, but not yet destroyed. */
    std::vector<std::unique_ptr<TxGraph::Ref>> removed;

    /** Determine the number of (non-removed) transactions in the graph. */
    DepGraphIndex GetTransactionCount() const { return graph.TxCount(); }

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
        assert(graph.Positions()[simpos]);
        simmap[simpos] = std::make_unique<TxGraph::Ref>();
        auto ptr = simmap[simpos].get();
        simrevmap[ptr] = simpos;
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
    }

    /** Modify the transaction fee of a ref, if it exists. */
    void SetTransactionFee(TxGraph::Ref* ref, int64_t fee)
    {
        auto pos = Find(ref);
        if (pos == MISSING) return;
        graph.FeeRate(pos).fee = fee;
    }

    /** Remove the transaction in the specified position from the graph. */
    void RemoveTransaction(TxGraph::Ref* ref)
    {
        auto pos = Find(ref);
        if (pos == MISSING) return;
        graph.RemoveTransactions(SetType::Singleton(pos));
        simrevmap.erase(simmap[pos].get());
        // Retain the TxGraph::Ref corresponding to this position, so the Ref destruction isn't
        // invoked until the simulation explicitly decided to do so.
        removed.push_back(std::move(simmap[pos]));
        simmap[pos].reset();
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
        // Deduplicate.
        std::sort(ret.begin(), ret.end());
        ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
        // Replace input.
        arg = std::move(ret);
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
     *  to be read from the provider, without realistically impacting test sensitivity. */
    InsecureRandomContext rng(0xdecade2009added + buffer.size());

    /** Variable used whenever an empty TxGraph::Ref is needed. */
    TxGraph::Ref empty_ref;

    // Construct a real and a simulated graph.
    auto real = MakeTxGraph();
    SimTxGraph sim;

    /** Function to pick any Ref (from sim.simmap or sim.removed, or the empty Ref). */
    auto pick_fn = [&]() noexcept -> TxGraph::Ref* {
        auto tx_count = sim.GetTransactionCount();
        /** The number of possible choices. */
        size_t choices = tx_count + sim.removed.size() + 1;
        /** Pick one of them. */
        auto choice = provider.ConsumeIntegralInRange<size_t>(0, choices - 1);
        if (choice < tx_count) {
            // Return from real.
            for (auto i : sim.graph.Positions()) {
                if (choice == 0) return sim.GetRef(i);
                --choice;
            }
            assert(false);
        } else {
            choice -= tx_count;
        }
        if (choice < sim.removed.size()) {
            // Return from removed.
            return sim.removed[choice].get();
        } else {
            choice -= sim.removed.size();
        }
        // Return empty.
        assert(choice == 0);
        return &empty_ref;
    };

    LIMITED_WHILE(provider.remaining_bytes() > 0, 200) {
        // Read a one-byte command.
        int command = provider.ConsumeIntegral<uint8_t>();
        // Treat it lowest bit as a flag (which selects a variant of some of the operations), and
        // leave the rest of the bits in command.
        bool alt = command & 1;
        command >>= 1;

        // Keep decrementing command for each applicable operation, until one is hit. Multiple
        // iterations may be necessary.
        while (true) {
            if (sim.GetTransactionCount() < SimTxGraph::MAX_TRANSACTIONS && command-- == 0) {
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
                    size = provider.ConsumeIntegral<uint8_t>() + 1;
                }
                FeePerWeight feerate{fee, size};
                // Create a real TxGraph::Ref.
                auto ref = real->AddTransaction(feerate);
                // Create a unique_ptr place in the simulation to put the Ref in.
                auto ref_loc = sim.AddTransaction(feerate);
                // Move it in place.
                *ref_loc = std::move(ref);
                break;
            } else if (sim.GetTransactionCount() + sim.removed.size() > 1 && command-- == 0) {
                // AddDependency.
                auto par = pick_fn();
                auto chl = pick_fn();
                auto pos_par = sim.Find(par);
                auto pos_chl = sim.Find(chl);
                if (pos_par != SimTxGraph::MISSING && pos_chl != SimTxGraph::MISSING) {
                    // Determine if adding this would introduce a cycle (not allowed by TxGraph),
                    // and if so, skip.
                    if (sim.graph.Ancestors(pos_par)[pos_chl]) break;
                    // Determine if adding this would violate CLUSTER_COUNT_LIMIT, and if so, skip.
                    auto temp_depgraph = sim.graph;
                    temp_depgraph.AddDependencies(SimTxGraph::SetType::Singleton(pos_par), pos_chl);
                    auto todo = temp_depgraph.Positions();
                    bool oversize{false};
                    while (todo.Any()) {
                        auto component = temp_depgraph.FindConnectedComponent(todo);
                        if (component.Count() > CLUSTER_COUNT_LIMIT) oversize = true;
                        todo -= component;
                    }
                    if (oversize) break;
                }
                sim.AddDependency(par, chl);
                real->AddDependency(*par, *chl);
                break;
            } else if (sim.removed.size() < 100 && command-- == 0) {
                // RemoveTransaction. Either all its ancestors or all its descendants are also
                // removed (if any), to make sure TxGraph's reordering of removals and dependencies
                // has no effect.
                std::vector<TxGraph::Ref*> to_remove;
                to_remove.push_back(pick_fn());
                sim.IncludeAncDesc(to_remove, alt);
                // The order in which these ancestors/descendants are removed should not matter;
                // randomly shuffle them.
                std::shuffle(to_remove.begin(), to_remove.end(), rng);
                for (TxGraph::Ref* ptr : to_remove) {
                    real->RemoveTransaction(*ptr);
                    sim.RemoveTransaction(ptr);
                }
                break;
            } else if (sim.removed.size() > 0 && command-- == 0) {
                // ~Ref. Destroying a TxGraph::Ref has an observable effect on the TxGraph it
                // refers to, so this simulation permits doing so separately from other actions on
                // TxGraph.

                // Pick a Ref of sim.removed to destroy.
                auto removed_pos = provider.ConsumeIntegralInRange<size_t>(0, sim.removed.size() - 1);
                if (removed_pos != sim.removed.size() - 1) {
                    std::swap(sim.removed[removed_pos], sim.removed.back());
                }
                sim.removed.pop_back();
                break;
            } else if (command-- == 0) {
                // SetTransactionFee.
                int64_t fee;
                if (alt) {
                    fee = provider.ConsumeIntegralInRange<int64_t>(-0x8000000000000, 0x7ffffffffffff);
                } else {
                    fee = provider.ConsumeIntegral<uint8_t>();
                }
                auto ref = pick_fn();
                real->SetTransactionFee(*ref, fee);
                sim.SetTransactionFee(ref, fee);
                break;
            } else if (command-- == 0) {
                // GetTransactionCount.
                assert(real->GetTransactionCount() == sim.GetTransactionCount());
                break;
            } else if (command-- == 0) {
                // Exists.
                auto ref = pick_fn();
                bool exists = real->Exists(*ref);
                bool should_exist = sim.Find(ref) != SimTxGraph::MISSING;
                assert(exists == should_exist);
                break;
            } else if (command-- == 0) {
                // GetIndividualFeerate.
                auto ref = pick_fn();
                auto feerate = real->GetIndividualFeerate(*ref);
                auto simpos = sim.Find(ref);
                if (simpos == SimTxGraph::MISSING) {
                    assert(feerate.IsEmpty());
                } else {
                    assert(feerate == sim.graph.FeeRate(simpos));
                }
                break;
            } else if (command-- == 0) {
                // GetAncestors/GetDescendants.
                auto ref = pick_fn();
                auto result_set = sim.MakeSet(alt ? real->GetDescendants(*ref) :
                                                    real->GetAncestors(*ref));
                auto expect_set = sim.GetAncDesc(ref, alt);
                assert(result_set == expect_set);
                break;
            } else if (command-- == 0) {
                // GetCluster.
                auto ref = pick_fn();
                auto result = real->GetCluster(*ref);
                // Check cluster count limit.
                assert(result.size() <= CLUSTER_COUNT_LIMIT);
                // Require the result to be topologically valid and not contain duplicates.
                auto left = sim.graph.Positions();
                for (auto refptr : result) {
                    auto simpos = sim.Find(refptr);
                    assert(simpos != SimTxGraph::MISSING);
                    assert(left[simpos]);
                    left.Reset(simpos);
                    assert(!sim.graph.Ancestors(simpos).Overlaps(left));
                }
                // Require the set to be connected.
                auto result_set = sim.MakeSet(result);
                assert(sim.graph.IsConnected(result_set));
                // If ref exists, the result must contain it. If not, it must be empty.
                auto simpos = sim.Find(ref);
                if (simpos != SimTxGraph::MISSING) {
                    assert(result_set[simpos]);
                } else {
                    assert(result_set.None());
                }
                // Require the set not to have ancestors or descendants outside of it.
                for (auto i : result_set) {
                    assert(sim.graph.Ancestors(i).IsSubsetOf(result_set));
                    assert(sim.graph.Descendants(i).IsSubsetOf(result_set));
                }
                break;
            }
        }
    }

    // After running all modifications, perform an internal sanity check (before invoking
    // inspectors that may modify the internal state).
    real->SanityCheck();

    // Compare simple properties of the graph with the simulation.
    assert(real->GetTransactionCount() == sim.GetTransactionCount());

    // Perform a full comparison.
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
            auto anc = sim.MakeSet(real->GetAncestors(*sim.GetRef(i)));
            assert(anc == expect_anc);
            // Check its descendants against simulation.
            auto expect_desc = sim.graph.Descendants(i);
            auto desc = sim.MakeSet(real->GetDescendants(*sim.GetRef(i)));
            assert(desc == expect_desc);
            // Check the cluster the transaction is part of.
            auto cluster = real->GetCluster(*sim.GetRef(i));
            assert(sim.MakeSet(cluster) == component);
            // Check that the cluster is reported in a valid topological order (its
            // linearization).
            std::vector<DepGraphIndex> simlin;
            SimTxGraph::SetType done;
            for (TxGraph::Ref* ptr : cluster) {
                auto simpos = sim.Find(ptr);
                assert(sim.graph.Descendants(simpos).IsSubsetOf(component - done));
                done.Set(simpos);
                assert(sim.graph.Ancestors(simpos).IsSubsetOf(done));
                simlin.push_back(simpos);
            }
            // Construct a chunking object for the simulated graph, using the reported cluster
            // linearization as ordering.
            cluster_linearize::LinearizationChunking simlinchunk(sim.graph, simlin);
            for (unsigned chunknum = 0; chunknum < simlinchunk.NumChunksLeft(); ++chunknum) {
                auto chunk = simlinchunk.GetChunk(chunknum);
                // Require that the chunks of cluster linearizations are connected (this must
                // be the case as all linearizations inside are PostLinearized).
                assert(sim.graph.IsConnected(chunk.transactions));
            }
        }
    }

    // Sanity check again (because invoking inspectors may modify internal unobservable state).
    real->SanityCheck();

    // Remove all remaining transactions, because Refs cannot be destroyed otherwise (this will be
    // addressed in a follow-up commit).
    for (auto i : sim.graph.Positions()) {
        auto ref = sim.GetRef(i);
        real->RemoveTransaction(*ref);
    }
}
