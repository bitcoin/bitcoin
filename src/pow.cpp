// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <ldpc/LDPC.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        /*In the mainnetmode, fPoWAllowMinDiffcultyBlocks is set to be negative.
         *Thus, we don't care the following routines
         */
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    printf("nActualTimespan = %d\tnPoWTargetTimespan = %d\n",nActualTimespan,params.nPowTargetTimespan);
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    int cur_level = pindexLast->nBits;
    double cur_difficulty = static_cast<double>(1)/ldpc_level_table[cur_level].prob;
    double ret_difficulty = cur_difficulty/((double)nActualTimespan/(double)params.nPowTargetTimespan);

    /*cur_difficulty is the number of expected iterations to solving the last 2016 problems for a given level.
     *ret_difficulty is the number of expected iterations to solving next 2016 problems.
     *We have to find a level in which the number of expected iterations is firstly greather (less) than ret_difficulty.
     *To prevent dramastically change in level, we only consider +- 10 levels at the current level.
     *In addition, note that ldpc_level_table.prob is alrady sorted, i.e.,
     *lpdc_table.prob[1] >  lpdc_table.prob[2] > lpdc_table.prob[3] > ... > lpdc_table.prob[MAX_LEVEL_DIFF]
     *Thus, the inverse of thems are also sorted, i.e.,
     *1./ldpc_table.prob[1] < 1./ldpc_table.prob[2] < 1./ldpc_table.prob[3] < ... < 1./ldpc_table.prob[MAX_LEVEL_DIFF]
     *
     *  consider a example in which we assume that cur_level = 10 and cur_difficulty is 187.
     * 1./lpdc_level_table[10] = 100;
     * 1./lpdc_level_table[11] = 150;
     * 1./lpdc_level_table[12] = 175;
     * 1./lpdc_level_table[13] = 190;
     * 1./lpdc_level_table[14] = 195;
     * 1./lpdc_level_table[20] = 1288;
     *
     * Then, new level is 12..
     *
     * consider a different example in which we assume that cur_level = 10 and cur_difficulty is 187, but
     * 1./lpdc_level_table[10] = 100;
     * 1./lpdc_level_table[11] = 120;
     * 1./lpdc_level_table[12] = 135;
     * 1./lpdc_level_table[13] = 140;
     *             ...
     *             ...
     * 1./lpdc_level_table[20] = 185;
     * In this case, new_level is 20 because 20 is the maximum level among the possible levels.
     */

    int from_level = cur_level - 10, to_level = cur_level + 10, new_level;

    // prevent the memory violation
    if (from_level <= 1)
        from_level = 1;
    if ( to_level >= MAX_DIFF_LEVEL )
        to_level = MAX_DIFF_LEVEL;
    if (cur_difficulty - ret_difficulty < static_cast<double>(0))
    {
        if (cur_level == MAX_DIFF_LEVEL)
            new_level = cur_level;
        else
        {
            // since the difficulty increases, we have to increase level as well.
            for ( new_level = cur_level + 1 ; new_level <= to_level ; new_level++)
            {
                double tmp = (static_cast<double>(1)/ldpc_level_table[new_level].prob - ret_difficulty);
                if ( tmp > static_cast<double>(0) )
                    break;
            }
        }
    }
    else if (cur_difficulty - ret_difficulty > static_cast<double>(0))
    {
        if (cur_level == 1 )
            new_level = cur_level;
        else
        {
            // since the difficulty decreases, we have to decrease level as well.
            for ( new_level = cur_level - 1 ; new_level >= from_level ; new_level--)
            {
                double tmp = (static_cast<double>(1)/ldpc_level_table[new_level].prob - ret_difficulty);
                if ( tmp < static_cast<double>(0) )
                    break;
            }
        }
    }
    else   //no changes in difficulty.
        new_level = cur_level;
    printf("cur_diff = %lf\t ret_diff = %lf\n",cur_difficulty,ret_difficulty);
    printf("curt_level = %d\t curt_expected_num = %lf\n",cur_level,1/ldpc_level_table[cur_level].prob);
    printf(" new_level = %d\t  new_expected_num = %lf\n\n",new_level,1/ldpc_level_table[new_level].prob);
    return new_level;
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

#if LDPC_POW
bool CheckProofOfWork(CBlockHeader block, const Consensus::Params& params)
{
    /*
    printf("\nnonce : %d\n",block.nNonce);
    std::cout  << block.GetHash().ToString() << std::endl;
    std::cout  << block.hashPrevBlock.ToString() << std::endl;
    std::cout  << block.hashMerkleRoot.ToString() << std::endl;
    */
    bool result = false;
    LDPC *ldpc = new LDPC;
    ldpc->set_difficulty(block.nBits);
    //ldpc->set_difficulty(1);
    ldpc->initialization();
    ldpc->generate_seeds(UintToArith256(block.hashPrevBlock).GetLow64());
    ldpc->generate_H();
    ldpc->generate_Q();
    ldpc->generate_hv((unsigned char*)block.GetHash().ToString().c_str());
    ldpc->decoding();
    if (ldpc->decision())
        result = true;

    delete ldpc;
    return result;
}
#endif
