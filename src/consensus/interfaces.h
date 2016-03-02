// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_INTERFACES_H
#define BITCOIN_CONSENSUS_INTERFACES_H

#include "params.h"

namespace Consensus {

typedef void (*VersionBitsCacheSetter)(const void* blockIndex, CVersionBitsState* versionBitsState);
typedef const CVersionBitsState* (*VersionBitsCacheGetter)(const void* blockIndex);

/**
 * Collection of function pointers to interface with the versionbits cache.
 */
struct CVersionBitsCacheInterface
{
    VersionBitsCacheSetter Set;
    VersionBitsCacheGetter Get;
};

} // namespace Consensus

#endif // BITCOIN_CONSENSUS_INTERFACES_H
