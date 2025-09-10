#include <pos/difficulty.h>

#include <chain.h>

unsigned int GetPoSNextTargetRequired(const CBlockIndex* pindexLast,
                                      const CBlockHeader* pblock,
                                      const Consensus::Params& params)
{
    arith_uint256 bnLimit = UintToArith256(params.posLimit);

    int64_t target_spacing = params.nStakeTargetSpacing;
    int64_t interval = params.DifficultyAdjustmentInterval();

    int64_t actual_spacing = pblock->nTime - pindexLast->nTime;
    if (actual_spacing < 0) actual_spacing = target_spacing;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= ((interval - 1) * target_spacing + 2 * actual_spacing);
    bnNew /= ((interval + 1) * target_spacing);

    if (bnNew <= 0 || bnNew > bnLimit) {
        bnNew = bnLimit;
    }
    return bnNew.GetCompact();
}
