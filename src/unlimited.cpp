// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>

#include "util.h"
#include "unlimited.h"

uint64_t maxGeneratedBlock = DEFAULT_MAX_GENERATED_BLOCK_SIZE;


void UnlimitedSetup(void)
{
   maxGeneratedBlock = GetArg("-blockmaxsize", DEFAULT_MAX_GENERATED_BLOCK_SIZE);
}
