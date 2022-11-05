#include <mw/mmr/MMRUtil.h>
#include <mw/mmr/MMR.h>
#include <mw/crypto/Hasher.h>
#include <mw/util/BitUtil.h>

#include <boost/dynamic_bitset.hpp>
#include <cmath>

using namespace mmr;

mw::Hash MMRUtil::CalcParentHash(const Index& index, const mw::Hash& left_hash, const mw::Hash& right_hash)
{
    return Hasher()
        .Append<uint64_t>(index.GetPosition())
        .Append(left_hash)
        .Append(right_hash)
        .hash();
}

std::vector<mmr::Index> MMRUtil::CalcPeakIndices(const uint64_t num_nodes)
{
    if (num_nodes == 0) {
        return {};
    }

    // Find the "peaks"
    std::vector<mmr::Index> peak_indices;

    uint64_t peakSize = BitUtil::FillOnesToRight(num_nodes);
    uint64_t numLeft = num_nodes;
    uint64_t sumPrevPeaks = 0;
    while (peakSize != 0) {
        if (numLeft >= peakSize) {
            peak_indices.push_back(mmr::Index::At(sumPrevPeaks + peakSize - 1));
            sumPrevPeaks += peakSize;
            numLeft -= peakSize;
        }

        peakSize >>= 1;
    }

    assert(numLeft == 0);
    return peak_indices;
}

boost::optional<mw::Hash> MMRUtil::CalcBaggedPeak(const IMMR& mmr, const mmr::Index& peak_idx)
{
    mmr::Index next_node = mmr.GetNextLeafIdx().GetNodeIndex();

    // Find the "peaks"
    std::vector<mmr::Index> peak_indices = MMRUtil::CalcPeakIndices(mmr.GetNumNodes());

    // Bag 'em
    boost::optional<mw::Hash> bagged_peak;
    for (auto iter = peak_indices.crbegin(); iter != peak_indices.crend(); iter++) {
        mw::Hash peakHash = mmr.GetHash(*iter);
        if (bagged_peak) {
            bagged_peak = MMRUtil::CalcParentHash(next_node, peakHash, *bagged_peak);
        } else {
            bagged_peak = peakHash;
        }

        if (*iter == peak_idx) {
            return bagged_peak;
        }
    }

    return boost::none;
}

BitSet MMRUtil::BuildCompactBitSet(const uint64_t num_leaves, const BitSet& unspent_leaf_indices)
{
    BitSet compactable_node_indices(num_leaves * 2);

    boost::dynamic_bitset<> prunable_nodes(num_leaves * 2);

    LeafIndex leaf_idx = LeafIndex::At(0);
    while (leaf_idx.Get() < num_leaves) {
        if (unspent_leaf_indices.size() > leaf_idx.Get() && !unspent_leaf_indices.test(leaf_idx.Get())) {
            prunable_nodes.set(leaf_idx.GetPosition());
        }

        leaf_idx = leaf_idx.Next();
    }

    LeafIndex next_leaf = LeafIndex::At(num_leaves);
    Index last_node = Index::At(next_leaf.GetPosition() - 1);

    uint64_t height = 1;
    while ((uint64_t(2) << height) - 2 <= next_leaf.GetPosition()) {
        SiblingIter iter(height, last_node);
        while (iter.Next()) {
            Index right_child = iter.Get().GetRightChild();
            if (prunable_nodes.test(right_child.GetPosition())) {
                Index left_child = iter.Get().GetLeftChild();
                if (prunable_nodes.test(left_child.GetPosition())) {
                    compactable_node_indices.set(right_child.GetPosition());
                    compactable_node_indices.set(left_child.GetPosition());
                    prunable_nodes.set(iter.Get().GetPosition());
                }
            }
        }

        ++height;
    }

    return compactable_node_indices;
}

BitSet MMRUtil::DiffCompactBitSet(const BitSet& prev_compact, const BitSet& new_compact)
{
    BitSet diff;

    for (size_t i = 0; i < new_compact.size(); i++) {
        if (prev_compact.size() > i && prev_compact.test(i)) {
            assert(new_compact.test(i));
            continue;
        }

        diff.bitset.push_back(new_compact.test(i));
    }

    return diff;
}

/// <summary>
/// An MMR can be rebuilt from the leaves and a small set of carefully chosen parent hashes.
/// This calculates the positions of those parent hashes.
/// </summary>
/// <param name="unspent_leaf_indices">The unspent leaf indices.</param>
/// <returns>The pruned parent positions.</returns>
BitSet MMRUtil::CalcPrunedParents(const BitSet& unspent_leaf_indices)
{
    BitSet ret(unspent_leaf_indices.size() * 2);

    LeafIndex leaf_idx = LeafIndex::At(0);
    while (leaf_idx.Get() < unspent_leaf_indices.size()) {
        if (!unspent_leaf_indices.test(leaf_idx.Get())) {
            ret.set(leaf_idx.GetPosition());
        }

        leaf_idx = leaf_idx.Next();
    }

    Index last_node = LeafIndex::At(unspent_leaf_indices.size()).GetNodeIndex();

    uint64_t height = 1;
    while ((uint64_t(2) << height) - 2 <= last_node.GetPosition()) {
        SiblingIter iter(height, last_node);
        while (iter.Next()) {
            Index right_child = iter.Get().GetRightChild();
            if (ret.test(right_child.GetPosition())) {
                Index left_child = iter.Get().GetLeftChild();
                if (ret.test(left_child.GetPosition())) {
                    ret.set(right_child.GetPosition(), false);
                    ret.set(left_child.GetPosition(), false);
                    ret.set(iter.Get().GetPosition());
                }
            }
        }

        ++height;
    }


    return ret;
}

SiblingIter::SiblingIter(const uint64_t height, const Index& last_node)
    : m_height(height),
    m_lastNode(last_node),
    m_baseInc((uint64_t(2) << height) - 1),
    m_siblingNum(0),
    m_next()
{
}

bool SiblingIter::Next()
{
    if (m_siblingNum == 0) {
        m_next = Index(m_baseInc - 1, m_height);
    } else {
        uint64_t increment = m_baseInc + BitUtil::CountRightmostZeros(m_siblingNum);
        m_next = Index(m_next.GetPosition() + increment, m_height);
    }

    ++m_siblingNum;
    return m_next <= m_lastNode;
};