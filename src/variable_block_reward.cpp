// Copyright (c) 2018 The BitcoinV Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <iostream>

#include "variable_block_reward.h"
#include <amount.h>
#include <consensus/params.h>
#include <primitives/block.h>

#include <arith_uint256.h>

static uint32_t get_num_consecutive_ones(uint32_t a, uint32_t b);
static uint32_t get_max_extra_subsidy_allowed(uint32_t num_consecutive_ones);

CAmount GetBlockSubsidyVBR(int nHeight, const Consensus::Params& consensusParams, const CBlock &block, bool print)
{
    int halvings = nHeight / consensusParams.nSubsidyHalvingInterval;
    // Force block reward to zero when right shift is undefined.
    if (halvings >= 64)
        return 0;


    uint32_t prev_hash_0 = get_32bit_word_from_uint256(block.hashPrevBlock);

    uint32_t merkle_root_0 = get_32bit_word_from_uint256(block.hashMerkleRoot);

    // count num of matching bits starting from right.
    uint32_t consec_ones = get_num_consecutive_ones(prev_hash_0, merkle_root_0);

    uint32_t extra_subsidy = get_max_extra_subsidy_allowed(consec_ones);

    if (print)
    {
      std::cout << "block.hashPrevBlock: " << block.hashPrevBlock.ToString() << std::endl;
      std::cout << "prev_hash_0: " << std::hex << prev_hash_0 << std::endl;   

      std::cout << "block.hashMerkleRoot: " << block.hashMerkleRoot.ToString() << std::endl;
      std::cout << "merkle_root_0: " << std::hex << merkle_root_0 << std::endl; 

      std::cout << "# bits match: " << consec_ones << std::endl;
    }


    CAmount nSubsidy = (50 * extra_subsidy) * COIN;
    // Subsidy is cut in half every XXXX blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;
    return nSubsidy;
}

// count num of matching bits starting from right.
uint32_t get_num_consecutive_ones(uint32_t a, uint32_t b)
{
    //std::cout << "a: " << std::hex << a << std::endl; 
    //std::cout << "b: " << std::hex << b << std::endl; 
    // xnor gives a '1' if they match
    uint32_t reg = ~(a ^ b);
    //std::cout << "---------------reg: " << reg << std::endl;
    uint32_t num = 0;
    for (int n=0; n<32; n++)
    {
        if ( (reg & (1<<n)) > 0 )
        {
            num++;
        }
        else
        {
            break;
        }
    }

    return num;
}

// The more the consecutive bits, the bigger the BlockReward
uint32_t get_max_extra_subsidy_allowed(uint32_t num_consecutive_ones)
{
    if ( num_consecutive_ones <= 0)
    {
        return 0;
    }

    uint64_t max_allowed = 1 << (num_consecutive_ones);

    // max extra reward is about 1 million times regular block reward
    if ( max_allowed >= (1 << 20) )
    {
        max_allowed = (1 << 20);
    }

    return max_allowed;
}
