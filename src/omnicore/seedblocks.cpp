#include <omnicore/seedblocks.h>

#include <omnicore/log.h>

#include <chainparams.h>
#include <util/time.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>
#include <set>
#include <string>
#include <vector>

const int MAX_SEED_BLOCK = 0;

static std::set<int> GetBlockList()
{
    int blocks[] = {};

    PrintToLog("Seed block filter active - %d blocks will be parsed during initial scan.\n", sizeof(blocks)/sizeof(blocks[0]));

    return std::set<int>(blocks, blocks + sizeof(blocks)/sizeof(blocks[0]));
}

bool SkipBlock(int nBlock)
{
    static std::set<int> blockList = GetBlockList();

    // Scan all non mainnet blocks:
    if (Params().NetworkIDString() != "main") {
        return false;
    }
    // Scan all blocks, which are not in the list:
    if (nBlock > MAX_SEED_BLOCK) {
        return false;
    }
    // Otherwise check, if the block is in the list:
    return (blockList.find(nBlock) == blockList.end());
}
