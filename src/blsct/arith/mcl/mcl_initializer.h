// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_MCL_MCL_INITIALIZER_H
#define NAVCOIN_BLSCT_ARITH_MCL_MCL_INITIALIZER_H

#define BLS_ETH 1
#include <bls/bls384_256.h>
#include <stdexcept>

/**
 * Should instantiate this class in static context only e.g.
 *  
 * static volatile MclStaticInitializer for_side_effect_only;
*/
class MclInitializer
{
public:
    MclInitializer()
    {
        if (blsInit(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR) != 0) {
            throw std::runtime_error("blsInit failed");
        }
        mclBn_setETHserialization(1);
    }
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_MCL_INITIALIZER_H
