// Copyright (c) 2015 Eric Lombrozo
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/softforks.h"

#include "chain.h"
#include "consensus/blockruleindex.h"
#include "consensus/params.h"

using namespace Consensus::SoftForks;

/**
 * Returns true if there are nRequired or more blocks of minVersion or above
 * in the last Consensus::Params::nMajorityWindow blocks, starting at pstart and going backwards.
 */
static bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned nRequired, const Consensus::Params& consensusParams)
{
    unsigned int nFound = 0;
    for (int i = 0; i < consensusParams.nMajorityWindow && nFound < nRequired && pstart != NULL; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    }
    return (nFound >= nRequired);
}

VersionStatus Consensus::SoftForks::CheckVersion(const CBlockIndex& blockIndex, const Consensus::VersionBits::BlockRuleIndex& blockRuleIndex, const Consensus::Params& consensusParams, CBlockIndex* pindexPrev)
{
    using namespace Consensus::VersionBits;

    if (!pindexPrev)
        pindexPrev = blockIndex.pprev;

    // Handle versionbits cases first
    if (UsesVersionBits(blockIndex.nVersion))
        return blockRuleIndex.AreVersionBitsRecognized(&blockIndex, pindexPrev) ? VALID : UNRECOGNIZED;

    // Reject blockIndex.nVersion=1 blocks when 95% (75% on testnet) of the network has upgraded:
    if (blockIndex.nVersion < 2 && IsSuperMajority(2, pindexPrev, consensusParams.nMajorityRejectBlockOutdated, consensusParams))
        return INVALID;

    // Reject blockIndex.nVersion=2 blocks when 95% (75% on testnet) of the network has upgraded:
    if (blockIndex.nVersion < 3 && IsSuperMajority(3, pindexPrev, consensusParams.nMajorityRejectBlockOutdated, consensusParams))
        return INVALID;

    // Reject blockIndex.nVersion=3 blocks when 95% (75% on testnet) of the network has upgraded:
    if (blockIndex.nVersion < 4 && IsSuperMajority(4, pindexPrev, consensusParams.nMajorityRejectBlockOutdated, consensusParams))
        return INVALID;

    if (blockIndex.nVersion > 4)
        return UNRECOGNIZED;

    return VALID;
}

bool Consensus::SoftForks::IsActive(int bip, const CBlockIndex& blockIndex, const Consensus::VersionBits::BlockRuleIndex& blockRuleIndex, const Consensus::Params& consensusParams, CBlockIndex* pindexPrev)
{
    if (!pindexPrev)
        pindexPrev = blockIndex.pprev;

    switch (bip)
    {
    case BIP16:
        // BIP16 didn't become active until Apr 1 2012
        return blockIndex.GetBlockTime() >= 1333238400;

    case BIP30:
        {
            // Do not allow blocks that contain transactions which 'overwrite' older transactions,
            // unless those are already completely spent.
            // If such overwrites are allowed, coinbases and transactions depending upon those
            // can be duplicated to remove the ability to spend the first instance -- even after
            // being sent to another address.
            // See BIP30 and http://r6.ca/blog/20120206T005236Z.html for more information.
            // This logic is not necessary for memory pool transactions, as AcceptToMemoryPool
            // already refuses previously-known transaction ids entirely.
            // This rule was originally applied to all blocks with a timestamp after March 15, 2012, 0:00 UTC.
            // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
            // two in the chain that violate it. This prevents exploiting the issue against nodes during their
            // initial block download.
            bool fEnforceBIP30 = (!blockIndex.phashBlock) || // Enforce on CreateNewBlock invocations which don't have a hash.
                                !((blockIndex.nHeight==91842 && blockIndex.GetBlockHash() == uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
                                 (blockIndex.nHeight==91880 && blockIndex.GetBlockHash() == uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")));

            // Once BIP34 activated it was not possible to create new duplicate coinbases and thus other than starting
            // with the 2 existing duplicate coinbase pairs, not possible to create overwriting txs.  But by the
            // time BIP34 activated, in each of the existing pairs the duplicate coinbase had overwritten the first
            // before the first had been spent.  Since those coinbases are sufficiently buried its no longer possible to create further
            // duplicate transactions descending from the known pairs either.
            // If we're on the known chain at height greater than where BIP34 activated, we can save the db accesses needed for the BIP30 check.
            CBlockIndex *pindexBIP34height = pindexPrev->GetAncestor(consensusParams.BIP34Height);
            //Only continue to enforce if we're below BIP34 activation height or the block hash at that height doesn't correspond.
            return fEnforceBIP30 && (!pindexBIP34height || !(pindexBIP34height->GetBlockHash() == consensusParams.BIP34Hash));
        }

    case BIP34:
        // Enforce block.nVersion=2 rule that the coinbase starts with serialized block height
        // if 750 of the last 1,000 blocks are version 2 or greater (51/100 if testnet):
        return blockIndex.nVersion >= 2 && IsSuperMajority(2, pindexPrev, consensusParams.nMajorityEnforceBlockUpgrade, consensusParams);
        
    case BIP65:
        // Start enforcing CHECKLOCKTIMEVERIFY, (BIP65) for blockIndex.nVersion=4, when 75% of the network has upgraded:
        return blockIndex.nVersion >= 4 && IsSuperMajority(4, pindexPrev, consensusParams.nMajorityEnforceBlockUpgrade, consensusParams);

    case BIP66:
        // Start enforcing the DERSIG (BIP66) rules, for block.nVersion=3 blocks, when 75% of the network has upgraded:
        return blockIndex.nVersion >= 3 && IsSuperMajority(3, pindexPrev, consensusParams.nMajorityEnforceBlockUpgrade, consensusParams);

    default:
        // Handle versionbits cases
        return (blockRuleIndex.GetRuleState(bip, &blockIndex) == Consensus::VersionBits::ACTIVE);
    }

}

const char* Consensus::SoftForks::GetBipName(int bip)
{
    switch (bip)
    {
    case BIP16:
        return "BIP16";

    case BIP30:
        return "BIP30";

    case BIP34:
        return "BIP34";

    case BIP65:
        return "BIP65";

    case BIP66:
        return "BIP66";

    default:
        return "N/A";
    }
}
