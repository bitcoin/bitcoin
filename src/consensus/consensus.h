// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include "consensus/params.h"

#include <stdint.h>

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

} // namespace Consensus

/** Block validation utility functions */

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

/** Flags for LockTime() */
enum {
    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
