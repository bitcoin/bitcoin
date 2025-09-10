#ifndef BITCOIN_POS_STAKEMODIFIER_H
#define BITCOIN_POS_STAKEMODIFIER_H

#include <consensus/params.h>
#include <uint256.h>

class CBlockIndex;

/**
 * Compute a new stake modifier using the previous modifier and block data.
 */
uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& prev_modifier);

/**
 * Return the current stake modifier. A new modifier is generated when the
 * refresh interval defined by consensus parameters has elapsed.
 */
uint256 GetStakeModifier(const CBlockIndex* pindexPrev, unsigned int nTime, const Consensus::Params& params);

#endif // BITCOIN_POS_STAKEMODIFIER_H
