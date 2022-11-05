#include <mw/mmr/MMR.h>
#include <mw/mmr/MMRUtil.h>

using namespace mmr;

mw::Hash IMMR::Root() const
{
    const uint64_t num_nodes = GetNumNodes();

    // Find the "peaks"
    std::vector<mmr::Index> peak_indices = MMRUtil::CalcPeakIndices(num_nodes);

    // Bag 'em
    mw::Hash hash;
    for (auto iter = peak_indices.crbegin(); iter != peak_indices.crend(); iter++) {
        mw::Hash peakHash = GetHash(*iter);
        if (hash.IsZero()) {
            hash = peakHash;
        } else {
            hash = MMRUtil::CalcParentHash(Index::At(num_nodes), peakHash, hash);
        }
    }

    return hash;
}