#include <mw/mmr/MMR.h>
#include <mw/mmr/MMRUtil.h>

using namespace mmr;

mw::Hash IMMR::Root() const
{
    const uint64_t num_nodes = mmr::LeafIndex::At(GetNumLeaves()).GetPosition();
    if (num_nodes == 0) {
        return mw::Hash{};
    }

    // Find the "peaks"
    std::vector<uint64_t> peakIndices;

    uint64_t peakSize = BitUtil::FillOnesToRight(num_nodes);
    uint64_t numLeft = num_nodes;
    uint64_t sumPrevPeaks = 0;
    while (peakSize != 0) {
        if (numLeft >= peakSize) {
            peakIndices.push_back(sumPrevPeaks + peakSize - 1);
            sumPrevPeaks += peakSize;
            numLeft -= peakSize;
        }

        peakSize >>= 1;
    }

    assert(numLeft == 0);

    // Bag 'em
    mw::Hash hash;
    for (auto iter = peakIndices.crbegin(); iter != peakIndices.crend(); iter++) {
        mw::Hash peakHash = GetHash(Index::At(*iter));
        if (hash.IsZero()) {
            hash = peakHash;
        } else {
            hash = MMRUtil::CalcParentHash(Index::At(num_nodes), peakHash, hash);
        }
    }

    return hash;
}