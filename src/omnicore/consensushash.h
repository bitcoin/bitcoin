#ifndef OMNICORE_CONSENSUSHASH_H
#define OMNICORE_CONSENSUSHASH_H

#include "uint256.h"

namespace mastercore
{
/** Obtains a hash of all balances to use for consensus verification and checkpointing. */
uint256 GetConsensusHash();

/** Obtains a hash of the overall MetaDEx state (default) or a specific orderbook (supply a property ID). */
uint256 GetMetaDExHash(const uint32_t propertyId = 0);

}

#endif // OMNICORE_CONSENSUSHASH_H
