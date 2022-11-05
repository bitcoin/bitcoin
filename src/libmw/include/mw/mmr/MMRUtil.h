#pragma once

#include <mw/common/BitSet.h>
#include <mw/mmr/Index.h>
#include <mw/mmr/LeafIndex.h>

class IMMR;

class MMRUtil
{
public:
    static mw::Hash CalcParentHash(const mmr::Index& index, const mw::Hash& left_hash, const mw::Hash& right_hash);
    static std::vector<mmr::Index> CalcPeakIndices(const uint64_t num_nodes);
    static boost::optional<mw::Hash> CalcBaggedPeak(const IMMR& mmr, const mmr::Index& peak_idx);

    static BitSet BuildCompactBitSet(const uint64_t num_leaves, const BitSet& unspent_leaf_indices);
    static BitSet DiffCompactBitSet(const BitSet& prev_compact, const BitSet& new_compact);

    /// <summary>
    /// An MMR can be rebuilt from the leaves and a small set of carefully chosen parent hashes.
    /// This calculates the positions of those parent hashes.
    /// </summary>
    /// <param name="unspent_leaf_indices">The unspent leaf indices.</param>
    /// <returns>The pruned parent positions.</returns>
    static BitSet CalcPrunedParents(const BitSet& unspent_leaf_indices);
};

/// <summary>
/// Iterates through MMR index positions at a single height, starting from left to right.
/// </summary>
struct SiblingIter
{
public:
    SiblingIter(const uint64_t height, const mmr::Index& last_node);

    bool Next();

    const mmr::Index& Get() const noexcept { return m_next; }
    uint64_t GetPosition() const noexcept { return m_next.GetPosition(); }

private:
    uint64_t m_height;
    mmr::Index m_lastNode;
    uint64_t m_baseInc;

    uint64_t m_siblingNum;
    mmr::Index m_next;
};