/**
 * @file utilsbitcoin.cpp
 *
 * This file contains certain helpers to access information about Bitcoin.
 */

#include <chain.h>
#include <chainparams.h>
#include <validation.h>
#include <sync.h>

#include <stdint.h>
#include <string>

namespace mastercore
{
/**
 * @return The current chain length.
 */
int GetHeight()
{
    LOCK(cs_main);
    return ::ChainActive().Height();
}

/**
 * @return The timestamp of the latest block.
 */
uint32_t GetLatestBlockTime()
{
    LOCK(cs_main);
    if (::ChainActive().Tip())
        return ::ChainActive().Tip()->GetBlockTime();
    else
        return Params().GenesisBlock().nTime;
}

/**
 * @return The CBlockIndex, or nullptr, if the block isn't available.
 */
CBlockIndex* GetBlockIndex(const uint256& hash)
{
    CBlockIndex* pBlockIndex = nullptr;
    LOCK(cs_main);
    BlockMap::const_iterator it = ::BlockIndex().find(hash);
    if (it != ::BlockIndex().end()) {
        pBlockIndex = it->second;
    }

    return pBlockIndex;
}

bool MainNet()
{
    return Params().NetworkIDString() == "main";
}

bool TestNet()
{
    return Params().NetworkIDString() == "test";
}

bool RegTest()
{
    return Params().NetworkIDString() == "regtest";
}

bool UnitTest()
{
    return Params().NetworkIDString() == "unittest";
}

bool isNonMainNet()
{
    return !MainNet() && !UnitTest();
}


} // namespace mastercore
