#pragma once

#include <mw/common/Macros.h>
#include <mw/models/crypto/Hash.h>
#include <mw/mmr/LeafIndex.h>
#include <set>

// Forward Declarations
class IMMR;
class ILeafSet;

MMR_NAMESPACE

/// <summary>
/// Represents a collection of contiguous, unpruned leaves,
/// and all hashes necessary to prove membership in the PMMR.
/// </summary>
struct Segment
{
    // The hashes of the requested unspent leaves, in ascending order
    std::vector<mw::Hash> leaves;

    // The MMR node hashes needed to verify the integrity of the MMR & the provided leaves
    std::vector<mw::Hash> hashes;

    // The "bagged" hash of the next lower peak (if there is one),
    // which is necessary to compute the PMMR root
    boost::optional<mw::Hash> lower_peak;
};

/// <summary>
/// Builds Segments for a provided MMR and segment.
/// </summary>
class SegmentFactory
{
public:
    static Segment Assemble(
        const IMMR& mmr,
        const ILeafSet& leafset,
        const LeafIndex& first_leaf_idx,
        const uint16_t num_leaves
    );

private:
    static std::set<Index> CalcHashIndices(
        const ILeafSet& leafset,
        const std::vector<Index>& peak_indices,
        const mmr::LeafIndex& first_leaf_idx,
        const mmr::LeafIndex& last_leaf_idx
    );

    static boost::optional<mw::Hash> BagNextLowerPeak(
        const IMMR& mmr,
        const std::vector<Index>& peak_indices,
        const mmr::Index& peak_idx,
        const uint64_t num_nodes
    );
};

END_NAMESPACE