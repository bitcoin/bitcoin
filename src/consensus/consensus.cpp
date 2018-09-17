// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus.h"
#include <validation.h>

unsigned int GetMaxBlockWeight()
{
    if (AreAssetsDeployed())
        return MAX_BLOCK_WEIGHT_RIP2;

    return MAX_BLOCK_WEIGHT;
}

unsigned int GetMaxBlockSerializedSize()
{
    if (AreAssetsDeployed())
        return MAX_BLOCK_SERIALIZED_SIZE_RIP2;

    return MAX_BLOCK_SERIALIZED_SIZE;
}