// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_CLUSTER_LINEARIZE_H
#define BITCOIN_TEST_UTIL_CLUSTER_LINEARIZE_H

#include <cluster_linearize.h>
#include <serialize.h>
#include <streams.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <stdint.h>
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

/** Get the minimal set of parents a transaction has (parents which are not parents
 *  of ancestors). */
template<typename SetType>
SetType GetReducedParents(const DepGraph<SetType>& depgraph, ClusterIndex i) noexcept
{
    SetType ret = depgraph.Ancestors(i);
    ret.Reset(i);
    for (auto a : ret) {
        if (ret[a]) {
            ret -= depgraph.Ancestors(a);
            ret.Set(a);
        }
    }
    return ret;
}

/** Get the minimal set of children a transaction has (children which are not children
 *  of descendants). */
template<typename SetType>
SetType GetReducedChildren(const DepGraph<SetType>& depgraph, ClusterIndex i) noexcept
{
    SetType ret = depgraph.Descendants(i);
    ret.Reset(i);
    for (auto a : ret) {
        if (ret[a]) {
            ret -= depgraph.Descendants(a);
            ret.Set(a);
        }
    }
    return ret;
}


/** A formatter for a bespoke serialization for acyclic DepGraph objects.
 *
 * The serialization format consists of:
 * - For each transaction t in the DepGraph:
 *   - The size: VARINT(tx[t].size), which cannot be 0.
 *   - The fee: VARINT(SignedToUnsigned(tx[t].fee)), see below for SignedToUnsigned.
 *   - The dependencies: for each minimized parent and minimized child of t among tx[0..t-1]:
 *     - VARINT(delta), which cannot be 0.
 *       To determine these values, consider the list of all potential parents and children tx t
 *       has among tx[0..t-1]. First the parents, in order from t-1 back to 0, and then the
 *       children in the same order. For these, we only consider ones that satisyfy
 *       CanAddDependency, based on all (actual) dependencies emitted before it, so it excludes
 *       parents/children that would be redundant, ones which would imply a cyclic dependency, or
 *       ones which would make an earlier dependency redundant.
 *       Now find in this list the positions that correspond to actual parents/children. The delta
 *       value for the first is 1 + its position in the list. The delta value for all further ones
 *       is the distance between its position and the previous ones' position.
 *   - The end of the dependencies: VARINT(0)
 * - The end of the graph: VARINT(0)
 *
 * On deserialization, if a read delta value results in a position outside the list of potential
 * parents/children, it is treated as 0 (i.e., the end of the encodings of dependences of t).
 *
 * Rationale:
 * - Why VARINTs? They are flexible enough to represent large numbers where needed, but more
 *   compact for smaller numbers. The serialization format is designed so that simple structures
 *   involve smaller numbers, so smaller size maps to simpler graphs.
 * - Why use SignedToUnsigned? It results in small unsigned values for signed values with small
 *   absolute value. This way we can encode negative fees in graphs, but still let small negative
 *   numbers have small encodings.
 * - Why are the parents/children emitted in order from t-1 back to 0? This means that if E is the
 *   encoding of a subgraph with no outside dependencies, copies of E in the serialization (in the
 *   right places) will result in copies of that subgraph.
 * - Why use CanAddDependency in the serialization definition? This makes sure that every variation
 *   (as produced by a fuzzer) of a graph will result in another, meaningful, and very likely
 *   distinct graph.
 * - Why use delta encoding and not a bitmask to convey the list positions? It turns out that
 *   the most complex graphs (in terms of linearization complexity) are ones with ~1 dependency per
 *   transaction. Delta encoding means just 2 bytes per transaction in this case (1 delta, 1 zero),
 *   while a bitmask would require 1 bit per potential transaction (= linear in the graph size).
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

    /** Test whether adding a dependency between parent and child is valid and meaningful. */
    template<typename SetType>
    static bool CanAddDependency(const DepGraph<SetType>& depgraph, ClusterIndex parent, ClusterIndex child) noexcept
    {
        // If child is already an ancestor of parent, the dependency would cause a cycle. Without
        // this condition, it would be possible for DepGraphFormatter to deserialize to a cyclic
        // graph.
        if (depgraph.Ancestors(parent)[child]) return false;
        // If child is already a descendant of parent, the dependency would be redundant. This is
        // an optimization whose goal is maximizing the probability to changes to the encoding map
        // to semantically distinct graphs.
        if (depgraph.Descendants(parent)[child]) return false;
        // If there is an ancestor of parent which is a direct parent of a descendant of child,
        // that dependency will have been redundant if a dependency between parent and child is
        // added. This is also just an optimization.
        const auto& descendants = depgraph.Descendants(child);
        for (auto i : depgraph.Ancestors(parent)) {
            if (descendants.Overlaps(depgraph.Descendants(i))) {
                if (descendants.Overlaps(GetReducedChildren(depgraph, i))) return false;
            }
        }
        return true;
    }

    template <typename Stream, typename SetType>
    static void Ser(Stream& s, const DepGraph<SetType>& depgraph)
    {
        /** The graph corresponding to what the deserializer already knows. */
        DepGraph<SetType> rebuild(depgraph.TxCount());
        for (ClusterIndex idx = 0; idx < depgraph.TxCount(); ++idx) {
            // Write size.
            s << VARINT_MODE(depgraph.FeeRate(idx).size, VarIntMode::NONNEGATIVE_SIGNED);
            // Write fee.
            s << VARINT(SignedToUnsigned(depgraph.FeeRate(idx).fee));
            // Write dependency information.
            uint64_t counter = 0; //!< How many potential parent/child relations we've iterated over.
            uint64_t offset = 0; //!< The counter value at the last actually written relation.
            for (unsigned loop = 0; loop < 2; ++loop) {
                // In loop 0 store parents among tx 0..idx-1; in loop 1 store children among those.
                SetType towrite = loop ? GetReducedChildren(depgraph, idx) : GetReducedParents(depgraph, idx);
                for (ClusterIndex i = 0; i < idx; ++i) {
                    ClusterIndex parent = loop ? idx : idx - 1 - i;
                    ClusterIndex child = loop ? idx - 1 - i : idx;
                    if (CanAddDependency(rebuild, parent, child)) {
                        ++counter;
                        if (towrite[idx - 1 - i]) {
                            rebuild.AddDependency(parent, child);
                            // The actually emitted values are differentially encoded (one value
                            // per parent/child relation).
                            s << VARINT(counter - offset);
                            offset = counter;
                        }
                    }
                }
            }
            if (counter > offset) s << uint8_t{0};
        }
        // Output a final 0 to denote the end of the graph.
        s << uint8_t{0};
    }

    template <typename Stream, typename SetType>
    void Unser(Stream& s, DepGraph<SetType>& depgraph)
    {
        depgraph = {};
        while (true) {
            // Read size. Size 0 signifies the end of the DepGraph.
            int32_t size;
            s >> VARINT_MODE(size, VarIntMode::NONNEGATIVE_SIGNED);
            size &= 0x3FFFFF; // Enough for size up to 4M.
            static_assert(0x3FFFFF >= 4000000);
            if (size == 0 || depgraph.TxCount() == SetType::Size()) break;
            // Read fee, encoded as a signed varint (odd means negative, even means non-negative).
            uint64_t coded_fee;
            s >> VARINT(coded_fee);
            coded_fee &= 0xFFFFFFFFFFFFF; // Enough for fee between -21M...21M BTC.
            static_assert(0xFFFFFFFFFFFFF > uint64_t{2} * 21000000 * 100000000);
            auto fee = UnsignedToSigned(coded_fee);
            // Extend resulting graph with new transaction.
            auto idx = depgraph.AddTransaction({fee, size});
            // Read dependency information.
            uint64_t offset = 0; //!< The next encoded value.
            uint64_t counter = 0; //!< How many potential parent/child relations we've iterated over.
            for (unsigned loop = 0; loop < 2; ++loop) {
                // In loop 0 read parents among tx 0..idx-1; in loop 1 read children.
                bool done = false;
                for (ClusterIndex i = 0; i < idx; ++i) {
                    ClusterIndex parent = loop ? idx : idx - 1 - i;
                    ClusterIndex child = loop ? idx - 1 - i : idx;
                    if (CanAddDependency(depgraph, parent, child)) {
                        ++counter;
                        // If counter passes offset, read & decode the next differentially encoded
                        // value. If a 0 is read, this signifies the end of this transaction's
                        // dependency information.
                        if (offset < counter) {
                            uint64_t diff;
                            s >> VARINT(diff);
                            offset += diff;
                            if (diff == 0 || offset < diff) {
                                done = true;
                                break;
                            }
                        }
                        // On a match, actually add the relation.
                        if (offset == counter) depgraph.AddDependency(parent, child);
                    }
                }
                if (done) break;
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
    // Consistency check between reduced parents/children and ancestors/descendants.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        SetType parents = GetReducedParents(depgraph, i);
        SetType combined_anc = SetType::Singleton(i);
        for (auto j : parents) {
            // Transactions cannot be a parent of themselves.
            assert(j != i);
            // Parents cannot have other parents as ancestors.
            assert((depgraph.Ancestors(j) & parents) == SetType::Singleton(j));
            combined_anc |= depgraph.Ancestors(j);
        }
        // The ancestors of all parents combined must equal the ancestors.
        assert(combined_anc == depgraph.Ancestors(i));

        SetType children = GetReducedChildren(depgraph, i);
        SetType combined_desc = SetType::Singleton(i);
        for (auto j : children) {
            // Transactions cannot be a child of themselves.
            assert(j != i);
            // Children cannot have other children as descendants.
            assert((depgraph.Descendants(j) & children) == SetType::Singleton(j));
            combined_desc |= depgraph.Descendants(j);
        }
        // The descendants of all children combined must equal the descendants.
        assert(combined_desc == depgraph.Descendants(i));
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
        // Check topology and lack of duplicates.
        assert((depgraph.Ancestors(i) - done) == TestBitSet::Singleton(i));
        done.Set(i);
    }
}

} // namespace

#endif // BITCOIN_TEST_UTIL_CLUSTER_LINEARIZE_H
