// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_MCL_STATIC_MCL_INIT_H
#define NAVCOIN_BLSCT_ARITH_MCL_STATIC_MCL_INIT_H

#define BLS_ETH 1
#include <bls/bls384_256.h>
#include <stdexcept>
#include <blsct/arith/mcl/init/mcl_init.h>

/**
 * This class should be instantiated in static context only e.g.
 *  
 * static volatile StaticMclInit for_side_effect_only;
*/
class StaticMclInit: MclInit
{
public:
    StaticMclInit()
    {
        Initialize();
    }
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_STATIC_MCL_INIT_H
