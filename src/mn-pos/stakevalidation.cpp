#include "stakevalidation.h"

#include <primitives/block.h>
#include <pubkey.h>

bool CheckBlockSignature(const CBlock& block, const CPubKey& pubkeyMasternode)
{
    uint256 hashBlock = block.GetHash();
    return pubkeyMasternode.Verify(hashBlock, block.vchBlockSig);
}

