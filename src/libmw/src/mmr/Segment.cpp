#include <mw/mmr/Segment.h>
#include <mw/mmr/LeafSet.h>
#include <mw/mmr/MMR.h>
#include <mw/mmr/MMRUtil.h>

using namespace mmr;

Segment SegmentFactory::Assemble(const IMMR& mmr, const ILeafSet& leafset, const LeafIndex& first_leaf_idx, const uint16_t num_leaves)
{
    if (!leafset.Contains(first_leaf_idx)) {
        return {};
    }

    Segment segment;
    segment.leaves.reserve(num_leaves);

    mmr::LeafIndex leaf_idx = first_leaf_idx;
    mmr::LeafIndex last_leaf_idx = first_leaf_idx;
    while (segment.leaves.size() < num_leaves && leaf_idx < leafset.GetNextLeafIdx()) {
        if (leafset.Contains(leaf_idx)) {
            last_leaf_idx = leaf_idx;
            segment.leaves.push_back(mmr.GetLeaf(leaf_idx));
        }

        ++leaf_idx;
    }
    
    std::vector<Index> peak_indices = MMRUtil::CalcPeakIndices(leafset.GetNumNodes());
    assert(!peak_indices.empty());

    // Populate hashes
    std::set<Index> hash_indices = CalcHashIndices(leafset, peak_indices, first_leaf_idx, last_leaf_idx);
    segment.hashes.reserve(hash_indices.size());
    for (const Index& idx : hash_indices) {
        segment.hashes.push_back(mmr.GetHash(idx));
    }

    // Determine the lowest peak that can be calculated using the hashes we've already provided
    auto peak_iter = std::find_if(
        peak_indices.begin(), peak_indices.end(),
        [&last_leaf_idx](const Index& peak_idx) { return peak_idx >= last_leaf_idx.GetNodeIndex(); });
    assert(peak_iter != peak_indices.end());

    // Bag the next lower peak (if there is one), so the root can still be calculated
    if (++peak_iter != peak_indices.end()) {
        segment.lower_peak = MMRUtil::CalcBaggedPeak(mmr, *peak_iter);
    }

    return segment;
}


std::set<Index> SegmentFactory::CalcHashIndices(
    const ILeafSet& leafset,
    const std::vector<Index>& peak_indices,
    const mmr::LeafIndex& first_leaf_idx,
    const mmr::LeafIndex& last_leaf_idx)
{
    std::set<Index> proof_indices;

    // 1. Add peaks of mountains to the left of first index
    boost::optional<Index> prev_peak = boost::make_optional(false, Index());
    for (const Index& peak_idx : peak_indices) {
        if (peak_idx < first_leaf_idx.GetNodeIndex()) {
            proof_indices.insert(peak_idx);
            prev_peak = peak_idx;
        } else {
            break;
        }
    }

    // 2. Add indices needed to reach left edge of mountain
    uint64_t adjustment = prev_peak ? prev_peak->GetPosition() + 1 : 0;
    auto on_mountain_left_edge = [adjustment](const Index& idx) -> bool {
        return (idx.GetPosition() + 2 - adjustment) == (uint64_t(2) << idx.GetHeight());
    };

    Index idx = first_leaf_idx.GetNodeIndex();
    while (!on_mountain_left_edge(idx)) {
        Index sibling_idx = idx.GetSibling();
        if (sibling_idx < idx) {
            proof_indices.insert(sibling_idx);
            idx = Index(idx.GetPosition() + 1, idx.GetHeight() + 1);
        } else {
            idx = Index(sibling_idx.GetPosition() + 1, sibling_idx.GetHeight() + 1);
        }
    }

    // 3. Add all pruned parents after first leaf and before last leaf
    BitSet pruned_parents = MMRUtil::CalcPrunedParents(leafset.ToBitSet());
    for (uint64_t pos = first_leaf_idx.GetPosition(); pos < last_leaf_idx.GetPosition(); pos++) {
        if (pruned_parents.test(pos)) {
            proof_indices.insert(Index::At(pos));
        }
    }

    // 4. Add indices needed to reach right edge of mountain containing the last leaf
    auto peak_iter = std::find_if(
        peak_indices.begin(), peak_indices.end(),
        [&last_leaf_idx](const Index& peak_idx) { return peak_idx >= last_leaf_idx.GetNodeIndex(); });
    assert(peak_iter != peak_indices.end());

    Index peak = *peak_iter;
    auto on_mountain_right_edge = [peak](const Index& idx) -> bool {
        return idx.GetPosition() >= peak.GetPosition() - peak.GetHeight();
    };

    idx = last_leaf_idx.GetNodeIndex();
    while (!on_mountain_right_edge(idx)) {
        Index sibling_idx = idx.GetSibling();
        if (sibling_idx > idx) {
            proof_indices.insert(sibling_idx);
            idx = Index(sibling_idx.GetPosition() + 1, idx.GetHeight() + 1);
        } else {
            idx = Index(idx.GetPosition() + 1, idx.GetHeight() + 1);
        }
    }

    return proof_indices;
}