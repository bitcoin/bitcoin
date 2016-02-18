// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include <stdint.h>

class CTransaction;
class CValidationState;

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 1000000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

/**
 * Consensus validations (for Script, Tx, Header and Block):
 * Check_ means checking everything possible with the data provided.
 * Verify_ means all data provided was enough for this level and its "consensus-verified".
 */
namespace Consensus {

/** Transaction validation functions */

/**
 * Performs all tx the checks that are common for coinbase and regular transactions.
 * It doesn't require knowledge of the inputs (utxo).
 */
bool CheckTxPreInputs(const CTransaction& tx, CValidationState& state, const int nHeight, int64_t nLockTimeCutoff, int64_t& nSigOps);

/**
 * Fully verify a CTransaction.
 * @TODO this is incomplete, among other things, the scripts are not checked yet.
 */
bool VerifyTx(const CTransaction& tx, CValidationState& state, const unsigned int flags, const int nHeight, const int64_t nMedianTime, const int64_t nBlockTime, int64_t& nSigOps);

} // namespace Consensus

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
