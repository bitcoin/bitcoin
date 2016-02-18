// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_VERSIONBITS_H
#define BITCOIN_CONSENSUS_VERSIONBITS_H

#include <stdint.h>
#include <string>

class CBlockIndex; // TODO decouple from chain.o

/**
 * Implementation of BIP9, see https://github.com/bitcoin/bips/blob/master/bip-0009.mediawiki
 */
namespace Consensus {

class CVersionBitsCacheInterface;
class Params;

/** What block version to use for new blocks (pre versionbits) */
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 4;
/**
 * The version bit reserved for signaling hardfork activation to all types of nodes (previously "sign bit").
 * See https://github.com/bitcoin/bips/pull/317 (TODO wait for BIP number)
 */
static const uint32_t HARDFORK_BIT = 1 << 31; // 1000...0
static const uint32_t UNUSED_RESERVED_BIT = 1 << 30; // 0100...0
static const uint32_t VERSIONBIT_BIT = 1 << 29; // 0010...0
static const uint32_t RESERVED_BITS_MASK = HARDFORK_BIT | UNUSED_RESERVED_BIT | VERSIONBIT_BIT; // 1110...0

/**
 * Get the consensus flags to be enforced according to the block.nVersion history. 
 */
unsigned int GetFlags(const CBlockIndex* pindexPrev, const Params& consensusParams, CVersionBitsCacheInterface& versionBitsCache);

} // namespace Consensus

// Non-consensus versionbits utility functions:

/**
 * Determine what nVersion a new block should use. This is useful for mining.
 */
int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams, Consensus::CVersionBitsCacheInterface& versionBitsCache);

/**
 * If there was a deployment confirmation counting round and any unkown deployment is locked in.
 */
std::string GetLastUnknownDeploymentWarning(const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams, Consensus::CVersionBitsCacheInterface& versionBitsCache);

#endif // BITCOIN_CONSENSUS_VERSIONBITS_H
