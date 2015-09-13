// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include "consensus/params.h"

#include <stdint.h>

class CBlock;
class CBlockHeader;
class CBlockIndex;
class CCoinsViewCache;
class CTransaction;
class CValidationState;

/**
 * Consensus validations:
 * Check_ means checking everything possible with the data provided.
 * Verify_ means all data provided was enough for this level and its "consensus-verified".
 */
namespace Consensus {

/** Transaction validation functions */

/**
 * Context-independent CTransaction validity checks
 */
bool CheckTx(const CTransaction& tx, CValidationState& state, const Params& consensusParams);
/**
 * Check whether all inputs of this transaction are valid (no double spends and amounts)
 * This does not modify the UTXO set. This does not check scripts and sigs.
 * Preconditions: tx.IsCoinBase() is false.
 */
bool CheckTxInputs(const CTransaction& tx, CValidationState& state, const Params& consensusParams, const CCoinsViewCache& inputs, int nSpendHeight);

/** Block header validation functions */

/**
 * Context-independent CBlockHeader validity checks
 */
bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Params& consensusParams, int64_t nTime, bool fCheckPOW = true);
/**
 * Context-dependent CBlockHeader validity checks
 */
bool ContextualCheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Params& consensusParams, const CBlockIndex* pindexPrev);

/** Block validation functions */

/**
 * Context-independent CBlock validity checks
 */
bool CheckBlock(const CBlock& block, CValidationState& state, const Params& consensusParams, int64_t nTime, bool fCheckPOW = true, bool fCheckMerkleRoot = true);
/**
 * Context-dependent CBlock validity checks
 */
bool ContextualCheckBlock(const CBlock& block, CValidationState& state, const Params& consensusParams, const CBlockIndex* pindexPrev);

} // namespace Consensus

/** Block header validation utility functions */

int64_t GetMedianTimePast(const CBlockIndex* pindex, const Consensus::Params& consensusParams);

/** Block validation utilities */

/** The maximum allowed size for a serialized block, in bytes (network rule) */
inline uint64_t MaxBlockSize(const Consensus::Params& consensusParams)
{
    return consensusParams.nMaxBlockSize;
}
/** The maximum allowed number of signature check operations in a block (network rule) */
inline uint64_t MaxBlockSigops(const Consensus::Params& consensusParams)
{
    return consensusParams.nMaxBlockSigops;
}

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
