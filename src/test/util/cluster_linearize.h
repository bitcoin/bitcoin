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
    for (ClusterIndex i : depgraph.Positions()) {
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
 * - The appending of 1, 2, 3, ... holes at the end of the cluster, followed by appending the new
 *   transaction.
 *
 * Let's say you have a 7-transaction cluster, consisting of transactions F,A,C,B,_,G,E,_,D
 * (where _ represent holes; unused positions within the DepGraph) but serialized in order
 * A,B,C,D,E,F,G, because that happens to be a topological ordering. By the time G gets serialized,
 * what has been serialized already represents the cluster F,A,C,B,_,E,_,D (in that order). G has B
 * and E as direct parents, and E depends on C.
 *
 * In this case, the possibilities are, in order:
 * - [ ] the dependency G->F
 * - [X] the dependency G->E
 * - [ ] the dependency G->D
 * - [X] the dependency G->B
 * - [ ] the dependency G->A
 * - [ ] put G at the end of the cluster
 * - [ ] put G before D
 * - [ ] put G before the hole before D
 * - [X] put G before E
 * - [ ] put G before the hole before E
 * - [ ] put G before B
 * - [ ] put G before C
 * - [ ] put G before A
 * - [ ] put G before F
 * - [ ] add 1 hole at the end of the cluster, followed by G
 * - [ ] add 2 holes at the end of the cluster, followed by G
 * - [ ] add ...
 *
 * The skip values in this case are 1 (G->F), 1 (G->D), 4 (G->A, G at end, G before D, G before
 * hole). No skip after 4 is needed (or permitted), because there can only be one position for G.
 * Also note that G->C is not included in the list of possibilities, as it is implied by the
 * included G->E and E->C that came before it. On deserialization, if the last skip value was 8 or
 * larger (putting G before the beginning of the cluster), it is interpreted as wrapping around
 * back to the end.
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
    [[maybe_unused]] static uint64_t SignedToUnsigned(int64_t x) noexcept
    {
        if (x < 0) {
            return 2 * uint64_t(-(x + 1)) + 1;
        } else {
            return 2 * uint64_t(x);
        }
    }

    /** Convert even x to x/2 (>=0), odd x to -(x/2)-1 (<0). */
    [[maybe_unused]] static int64_t UnsignedToSigned(uint64_t x) noexcept
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
        std::vector<ClusterIndex> topo_order;
        topo_order.reserve(depgraph.TxCount());
        for (auto i : depgraph.Positions()) topo_order.push_back(i);
        std::sort(topo_order.begin(), topo_order.end(), [&](ClusterIndex a, ClusterIndex b) {
            auto anc_a = depgraph.Ancestors(a).Count(), anc_b = depgraph.Ancestors(b).Count();
            if (anc_a != anc_b) return anc_a < anc_b;
            return a < b;
        });

        /** Which positions (incl. holes) the deserializer already knows when it has deserialized
         *  what has been serialized here so far. */
        SetType done;

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
                    // When an actual parent is encountered, encode how many non-parents were skipped
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
            auto add_holes = SetType::Fill(idx) - done - depgraph.Positions();
            if (add_holes.None()) {
                // The new transaction is to be inserted N positions back from the end of the
                // cluster. Emit N to indicate that that many insertion choices are skipped.
                auto skips = (done - SetType::Fill(idx)).Count();
                s << VARINT(diff + skips);
            } else {
                // The new transaction is to be appended at the end of the cluster, after N holes.
                // Emit current_cluster_size + N, to indicate all insertion choices are skipped,
                // plus N possibilities for the number of holes.
                s << VARINT(diff + done.Count() + add_holes.Count());
                done |= add_holes;
            }
            done.Set(idx);
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
        /** Mapping from serialization order to cluster order, used later to reconstruct the
         *  cluster order. */
        std::vector<ClusterIndex> reordering;
        /** How big the entries vector in the reconstructed depgraph will be (including holes). */
        ClusterIndex total_size{0};

        // Read transactions in topological order.
        while (true) {
            FeeFrac new_feerate; //!< The new transaction's fee and size.
            SetType new_ancestors; //!< The new transaction's ancestors (excluding itself).
            uint64_t diff{0}; //!< How many potential parents/insertions we have to skip.
            bool read_error{false};
            try {
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
                new_feerate = {UnsignedToSigned(coded_fee), size};
                // Read dependency information.
                auto topo_idx = reordering.size();
                s >> VARINT(diff);
                for (ClusterIndex dep_dist = 0; dep_dist < topo_idx; ++dep_dist) {
                    /** Which topo_depgraph index we are currently considering as parent of topo_idx. */
                    ClusterIndex dep_topo_idx = topo_idx - 1 - dep_dist;
                    // Ignore transactions which are already known ancestors of topo_idx.
                    if (new_ancestors[dep_topo_idx]) continue;
                    if (diff == 0) {
                        // When the skip counter has reached 0, add an actual dependency.
                        new_ancestors |= topo_depgraph.Ancestors(dep_topo_idx);
                        // And read the number of skips after it.
                        s >> VARINT(diff);
                    } else {
                        // Otherwise, dep_topo_idx is not a parent. Decrement and continue.
                        --diff;
                    }
                }
            } catch (const std::ios_base::failure&) {
                // Continue even if a read error was encountered.
                read_error = true;
            }
            // Construct a new transaction whenever we made it past the new_feerate construction.
            if (new_feerate.IsEmpty()) break;
            assert(reordering.size() < SetType::Size());
            auto topo_idx = topo_depgraph.AddTransaction(new_feerate);
            topo_depgraph.AddDependencies(new_ancestors, topo_idx);
            if (total_size < SetType::Size()) {
                // Normal case.
                diff %= SetType::Size();
                if (diff <= total_size) {
                    // Insert the new transaction at distance diff back from the end.
                    for (auto& pos : reordering) {
                        pos += (pos >= total_size - diff);
                    }
                    reordering.push_back(total_size++ - diff);
                } else {
                    // Append diff - total_size holes at the end, plus the new transaction.
                    total_size = diff;
                    reordering.push_back(total_size++);
                }
            } else {
                // In case total_size == SetType::Size, it is not possible to insert the new
                // transaction without exceeding SetType's size. Instead, interpret diff as an
                // index into the holes, and overwrite a position there. This branch is never used
                // when deserializing the output of the serializer, but gives meaning to otherwise
                // invalid input.
                diff %= (SetType::Size() - reordering.size());
                SetType holes = SetType::Fill(SetType::Size());
                for (auto pos : reordering) holes.Reset(pos);
                for (auto pos : holes) {
                    if (diff == 0) {
                        reordering.push_back(pos);
                        break;
                    }
                    --diff;
                }
            }
            // Stop if a read error was encountered during deserialization.
            if (read_error) break;
        }

        // Construct the original cluster order depgraph.
        depgraph = DepGraph(topo_depgraph, reordering, total_size);
    }
};

/** Perform a sanity/consistency check on a DepGraph. */
template<typename SetType>
void SanityCheck(const DepGraph<SetType>& depgraph)
{
    // Verify Positions and PositionRange consistency.
    ClusterIndex num_positions{0};
    ClusterIndex position_range{0};
    for (ClusterIndex i : depgraph.Positions()) {
        ++num_positions;
        position_range = i + 1;
    }
    assert(num_positions == depgraph.TxCount());
    assert(position_range == depgraph.PositionRange());
    assert(position_range >= num_positions);
    assert(position_range <= SetType::Size());
    // Consistency check between ancestors internally.
    for (ClusterIndex i : depgraph.Positions()) {
        // Transactions include themselves as ancestors.
        assert(depgraph.Ancestors(i)[i]);
        // If a is an ancestor of b, then b's ancestors must include all of a's ancestors.
        for (auto a : depgraph.Ancestors(i)) {
            assert(depgraph.Ancestors(i).IsSupersetOf(depgraph.Ancestors(a)));
        }
    }
    // Consistency check between ancestors and descendants.
    for (ClusterIndex i : depgraph.Positions()) {
        for (ClusterIndex j : depgraph.Positions()) {
            assert(depgraph.Ancestors(i)[j] == depgraph.Descendants(j)[i]);
        }
        // No transaction is a parent or child of itself.
        auto parents = depgraph.GetReducedParents(i);
        auto children = depgraph.GetReducedChildren(i);
        assert(!parents[i]);
        assert(!children[i]);
        // Parents of a transaction do not have ancestors inside those parents (except itself).
        // Note that even the transaction itself may be missing (if it is part of a cycle).
        for (auto parent : parents) {
            assert((depgraph.Ancestors(parent) & parents).IsSubsetOf(SetType::Singleton(parent)));
        }
        // Similar for children and descendants.
        for (auto child : children) {
            assert((depgraph.Descendants(child) & children).IsSubsetOf(SetType::Singleton(child)));
        }
    }
    if (IsAcyclic(depgraph)) {
        // If DepGraph is acyclic, serialize + deserialize must roundtrip.
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

        // In acyclic graphs, the union of parents with parents of parents etc. yields the
        // full ancestor set (and similar for children and descendants).
        std::vector<SetType> parents(depgraph.PositionRange()), children(depgraph.PositionRange());
        for (ClusterIndex i : depgraph.Positions()) {
            parents[i] = depgraph.GetReducedParents(i);
            children[i] = depgraph.GetReducedChildren(i);
        }
        for (auto i : depgraph.Positions()) {
            // Initialize the set of ancestors with just the current transaction itself.
            SetType ancestors = SetType::Singleton(i);
            // Iteratively add parents of all transactions in the ancestor set to itself.
            while (true) {
                const auto old_ancestors = ancestors;
                for (auto j : ancestors) ancestors |= parents[j];
                // Stop when no more changes are being made.
                if (old_ancestors == ancestors) break;
            }
            assert(ancestors == depgraph.Ancestors(i));

            // Initialize the set of descendants with just the current transaction itself.
            SetType descendants = SetType::Singleton(i);
            // Iteratively add children of all transactions in the descendant set to itself.
            while (true) {
                const auto old_descendants = descendants;
                for (auto j : descendants) descendants |= children[j];
                // Stop when no more changes are being made.
                if (old_descendants == descendants) break;
            }
            assert(descendants == depgraph.Descendants(i));
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
        assert(depgraph.Positions()[i]);
        // Check topology and lack of duplicates.
        assert((depgraph.Ancestors(i) - done) == TestBitSet::Singleton(i));
        done.Set(i);
    }
}

} // namespace

#endif // BITCOIN_TEST_UTIL_CLUSTER_LINEARIZE_H
