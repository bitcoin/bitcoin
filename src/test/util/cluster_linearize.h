// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_CLUSTER_LINEARIZE_H
#define BITCOIN_TEST_UTIL_CLUSTER_LINEARIZE_H

#include <cluster_linearize.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <stdint.h>
#include <numeric>
#include <vector>
#include <utility>

namespace {

using namespace cluster_linearize;

using TestBitSet = BitSet<32>;

/** Check if a graph is acyclic. */
template<typename SetType>
bool IsAcyclic(const DepGraph<SetType>& depgraph) noexcept
{
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        if ((depgraph.Ancestors(i) & depgraph.Descendants(i)) != SetType::Singleton(i)) {
            return false;
        }
    }
    return true;
}

/** A formatter for a bespoke serialization for acyclic DepGraph objects.
 *
 * The serialization format outputs information about transactions in a topological order (parents
 * before children), together with position information so transactions can be moved back to their
 * correct position on deserialization.
 *
 * - For each transaction t in the DepGraph (in some topological order);
 *   - The size: VARINT(t.size), which cannot be 0.
 *   - The fee: VARINT(SignedToUnsigned(t.fee)), see below for SignedToUnsigned.
 *   - For each direct dependency:
 *     - VARINT(skip)
 *   - The position of t in the cluster: VARINT(skip)
 * - The end of the graph: VARINT(0)
 *
 * The list of skip values encodes the dependencies of t, as well as its position in the cluster.
 * Each skip value is the number of possibilities that were available, but were not taken. These
 * possibilities are, in order:
 * - For each previous transaction in the graph, in reverse serialization order, whether it is a
 *   direct parent of t (but excluding transactions which are already implied to be dependencies
 *   by parent relations that were serialized before it).
 * - The various insertion positions in the cluster, from the very end of the cluster, to the
 *   front.
 *
 * Let's say you have a 7-transaction cluster, consisting of transactions F,A,C,B,G,E,D, but
 * serialized in order A,B,C,D,E,F,G, because that happens to be a topological ordering. By the
 * time G gets serialized, what has been serialized already represents the cluster F,A,C,B,E,D (in
 * that order). G has B and E as direct parents, and E depends on C.
 *
 * In this case, the possibilities are, in order:
 * - [ ] the dependency G->F
 * - [X] the dependency G->E
 * - [ ] the dependency G->D
 * - [X] the dependency G->B
 * - [ ] the dependency G->A
 * - [ ] put G at the end of the cluster
 * - [ ] put G before D
 * - [X] put G before E
 * - [ ] put G before B
 * - [ ] put G before C
 * - [ ] put G before A
 * - [ ] put G before F
 *
 * The skip values in this case are 1 (G->F), 1 (G->D), 3 (G->A, G at end, G before D). No skip
 * after 3 is needed (or permitted), because there can only be one position for G. Also note that
 * G->C is not included in the list of possibilities, as it is implied by the included G->E and
 * E->C that came before it. On deserialization, if the last skip value was 8 or larger (putting
 * G before the beginning of the cluster), it is interpreted as wrapping around back to the end.
 *
 *
 * Rationale:
 * - Why VARINTs? They are flexible enough to represent large numbers where needed, but more
 *   compact for smaller numbers. The serialization format is designed so that simple structures
 *   involve smaller numbers, so smaller size maps to simpler graphs.
 * - Why use SignedToUnsigned? It results in small unsigned values for signed values with small
 *   absolute value. This way we can encode negative fees in graphs, but still let small negative
 *   numbers have small encodings.
 * - Why are the parents emitted in reverse order compared to the transactions themselves? This
 *   naturally lets us skip parents-of-parents, as they will be reflected as implied dependencies.
 * - Why encode skip values and not a bitmask to convey the list positions? It turns out that the
 *   most complex graphs (in terms of linearization complexity) are ones with ~1 dependency per
 *   transaction. The current encoding uses ~1 byte per transaction for dependencies in this case,
 *   while a bitmask would require ~N/2 bits per transaction.
 */

struct DepGraphFormatter
{
    /** Convert x>=0 to 2x (even), x<0 to -2x-1 (odd). */
    static uint64_t SignedToUnsigned(int64_t x) noexcept
    {
        if (x < 0) {
            return 2 * uint64_t(-(x + 1)) + 1;
        } else {
            return 2 * uint64_t(x);
        }
    }

    /** Convert even x to x/2 (>=0), odd x to -(x/2)-1 (<0). */
    static int64_t UnsignedToSigned(uint64_t x) noexcept
    {
        if (x & 1) {
            return -int64_t(x / 2) - 1;
        } else {
            return int64_t(x / 2);
        }
    }

    template <typename Stream, typename SetType>
    static void Ser(Stream& s, const DepGraph<SetType>& depgraph)
    {
        /** Construct a topological order to serialize the transactions in. */
        std::vector<ClusterIndex> topo_order(depgraph.TxCount());
        std::iota(topo_order.begin(), topo_order.end(), ClusterIndex{0});
        std::sort(topo_order.begin(), topo_order.end(), [&](ClusterIndex a, ClusterIndex b) {
            auto anc_a = depgraph.Ancestors(a).Count(), anc_b = depgraph.Ancestors(b).Count();
            if (anc_a != anc_b) return anc_a < anc_b;
            return a < b;
        });

        /** Which transactions the deserializer already knows when it has deserialized what has
         *  been serialized here so far, and in what order. */
        std::vector<ClusterIndex> rebuilt_order;
        rebuilt_order.reserve(depgraph.TxCount());

        // Loop over the transactions in topological order.
        for (ClusterIndex topo_idx = 0; topo_idx < topo_order.size(); ++topo_idx) {
            /** Which depgraph index we are currently writing. */
            ClusterIndex idx = topo_order[topo_idx];
            // Write size, which must be larger than 0.
            s << VARINT_MODE(depgraph.FeeRate(idx).size, VarIntMode::NONNEGATIVE_SIGNED);
            // Write fee, encoded as an unsigned varint (odd=negative, even=non-negative).
            s << VARINT(SignedToUnsigned(depgraph.FeeRate(idx).fee));
            // Write dependency information.
            SetType written_parents;
            uint64_t diff = 0; //!< How many potential parent/child relations we have skipped over.
            for (ClusterIndex dep_dist = 0; dep_dist < topo_idx; ++dep_dist) {
                /** Which depgraph index we are currently considering as parent of idx. */
                ClusterIndex dep_idx = topo_order[topo_idx - 1 - dep_dist];
                // Ignore transactions which are already known to be ancestors.
                if (depgraph.Descendants(dep_idx).Overlaps(written_parents)) continue;
                if (depgraph.Ancestors(idx)[dep_idx]) {
                    // When an actual parent is encounted, encode how many non-parents were skipped
                    // before it.
                    s << VARINT(diff);
                    diff = 0;
                    written_parents.Set(dep_idx);
                } else {
                    // When a non-parent is encountered, increment the skip counter.
                    ++diff;
                }
            }
            // Write position information.
            ClusterIndex insert_distance = 0;
            while (insert_distance < rebuilt_order.size()) {
                // Loop to find how far from the end in rebuilt_order to insert.
                if (idx > *(rebuilt_order.end() - 1 - insert_distance)) break;
                ++insert_distance;
            }
            rebuilt_order.insert(rebuilt_order.end() - insert_distance, idx);
            s << VARINT(diff + insert_distance);
        }

        // Output a final 0 to denote the end of the graph.
        s << uint8_t{0};
    }

    template <typename Stream, typename SetType>
    void Unser(Stream& s, DepGraph<SetType>& depgraph)
    {
        /** The dependency graph which we deserialize into first, with transactions in
         *  topological serialization order, not original cluster order. */
        DepGraph<SetType> topo_depgraph;
        /** Mapping from cluster order to serialization order, used later to reconstruct the
         *  cluster order. */
        std::vector<ClusterIndex> reordering;

        // Read transactions in topological order.
        try {
            while (true) {
                // Read size. Size 0 signifies the end of the DepGraph.
                int32_t size;
                s >> VARINT_MODE(size, VarIntMode::NONNEGATIVE_SIGNED);
                size &= 0x3FFFFF; // Enough for size up to 4M.
                static_assert(0x3FFFFF >= 4000000);
                if (size == 0 || topo_depgraph.TxCount() == SetType::Size()) break;
                // Read fee, encoded as an unsigned varint (odd=negative, even=non-negative).
                uint64_t coded_fee;
                s >> VARINT(coded_fee);
                coded_fee &= 0xFFFFFFFFFFFFF; // Enough for fee between -21M...21M BTC.
                static_assert(0xFFFFFFFFFFFFF > uint64_t{2} * 21000000 * 100000000);
                auto fee = UnsignedToSigned(coded_fee);
                // Extend topo_depgraph with the new transaction (at the end).
                auto topo_idx = topo_depgraph.AddTransaction({fee, size});
                reordering.push_back(topo_idx);
                // Read dependency information.
                uint64_t diff = 0; //!< How many potential parents we have to skip.
                s >> VARINT(diff);
                for (ClusterIndex dep_dist = 0; dep_dist < topo_idx; ++dep_dist) {
                    /** Which topo_depgraph index we are currently considering as parent of topo_idx. */
                    ClusterIndex dep_topo_idx = topo_idx - 1 - dep_dist;
                    // Ignore transactions which are already known ancestors of topo_idx.
                    if (topo_depgraph.Descendants(dep_topo_idx)[topo_idx]) continue;
                    if (diff == 0) {
                        // When the skip counter has reached 0, add an actual dependency.
                        topo_depgraph.AddDependency(dep_topo_idx, topo_idx);
                        // And read the number of skips after it.
                        s >> VARINT(diff);
                    } else {
                        // Otherwise, dep_topo_idx is not a parent. Decrement and continue.
                        --diff;
                    }
                }
                // If we reach this point, we can interpret the remaining skip value as how far from the
                // end of reordering topo_idx should be placed (wrapping around), so move it to its
                // correct location. The preliminary reordering.push_back(topo_idx) above was to make
                // sure that if a deserialization exception occurs, topo_idx still appears somewhere.
                reordering.pop_back();
                reordering.insert(reordering.end() - (diff % (reordering.size() + 1)), topo_idx);
            }
        } catch (const std::ios_base::failure&) {}

        // Construct the original cluster order depgraph.
        depgraph = {};
        // Add transactions to depgraph in the original cluster order.
        for (auto topo_idx : reordering) {
            depgraph.AddTransaction(topo_depgraph.FeeRate(topo_idx));
        }
        // Translate dependencies from topological to cluster order.
        for (ClusterIndex idx = 0; idx < reordering.size(); ++idx) {
            ClusterIndex topo_idx = reordering[idx];
            for (ClusterIndex dep_idx = 0; dep_idx < reordering.size(); ++dep_idx) {
                ClusterIndex dep_topo_idx = reordering[dep_idx];
                if (topo_depgraph.Ancestors(topo_idx)[dep_topo_idx]) {
                    depgraph.AddDependency(dep_idx, idx);
                }
            }
        }
    }
};

/** Perform a sanity/consistency check on a DepGraph. */
template<typename SetType>
void SanityCheck(const DepGraph<SetType>& depgraph)
{
    // Consistency check between ancestors internally.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        // Transactions include themselves as ancestors.
        assert(depgraph.Ancestors(i)[i]);
        // If a is an ancestor of b, then b's ancestors must include all of a's ancestors.
        for (auto a : depgraph.Ancestors(i)) {
            assert(depgraph.Ancestors(i).IsSupersetOf(depgraph.Ancestors(a)));
        }
    }
    // Consistency check between ancestors and descendants.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        for (ClusterIndex j = 0; j < depgraph.TxCount(); ++j) {
            assert(depgraph.Ancestors(i)[j] == depgraph.Descendants(j)[i]);
        }
    }
    // If DepGraph is acyclic, serialize + deserialize must roundtrip.
    if (IsAcyclic(depgraph)) {
        std::vector<unsigned char> ser;
        VectorWriter writer(ser, 0);
        writer << Using<DepGraphFormatter>(depgraph);
        SpanReader reader(ser);
        DepGraph<TestBitSet> decoded_depgraph;
        reader >> Using<DepGraphFormatter>(decoded_depgraph);
        assert(depgraph == decoded_depgraph);
        assert(reader.empty());
        // It must also deserialize correctly without the terminal 0 byte (as the deserializer
        // will upon EOF still return what it read so far).
        assert(ser.size() >= 1 && ser.back() == 0);
        ser.pop_back();
        reader = SpanReader{ser};
        decoded_depgraph = {};
        reader >> Using<DepGraphFormatter>(decoded_depgraph);
        assert(depgraph == decoded_depgraph);
        assert(reader.empty());
    }
}

/** Verify that a DepGraph corresponds to the information in a cluster. */
template<typename SetType>
void VerifyDepGraphFromCluster(const Cluster<SetType>& cluster, const DepGraph<SetType>& depgraph)
{
    // Sanity check the depgraph, which includes a check for correspondence between ancestors and
    // descendants, so it suffices to check just ancestors below.
    SanityCheck(depgraph);
    // Verify transaction count.
    assert(cluster.size() == depgraph.TxCount());
    // Verify feerates.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        assert(depgraph.FeeRate(i) == cluster[i].first);
    }
    // Verify ancestors.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        // Start with the transaction having itself as ancestor.
        auto ancestors = SetType::Singleton(i);
        // Add parents of ancestors to the set of ancestors until it stops changing.
        while (true) {
            const auto old_ancestors = ancestors;
            for (auto ancestor : ancestors) {
                ancestors |= cluster[ancestor].second;
            }
            if (old_ancestors == ancestors) break;
        }
        // Compare against depgraph.
        assert(depgraph.Ancestors(i) == ancestors);
        // Some additional sanity tests:
        // - Every transaction has itself as ancestor.
        assert(ancestors[i]);
        // - Every transaction has its direct parents as ancestors.
        for (auto parent : cluster[i].second) {
            assert(ancestors[parent]);
        }
    }
}

/** Perform a sanity check on a linearization. */
template<typename SetType>
void SanityCheck(const DepGraph<SetType>& depgraph, Span<const ClusterIndex> linearization)
{
    // Check completeness.
    assert(linearization.size() == depgraph.TxCount());
    TestBitSet done;
    for (auto i : linearization) {
        // Check transaction position is in range.
        assert(i < depgraph.TxCount());
        // Check topology and lack of duplicates.
        assert((depgraph.Ancestors(i) - done) == TestBitSet::Singleton(i));
        done.Set(i);
    }
}

} // namespace

#endif // BITCOIN_TEST_UTIL_CLUSTER_LINEARIZE_H
