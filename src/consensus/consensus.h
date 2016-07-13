// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include <stdint.h>

class CCoinsViewCache;
class CTransaction;
class CValidationState;

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;
/** The maximum allowed cost for a block, see BIP 141 (network rule) */
static const unsigned int MAX_BLOCK_COST = 4000000;
/** The maximum allowed size for a block excluding witness data, in bytes (network rule) */
static const unsigned int MAX_BLOCK_BASE_SIZE = 1000000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

/** Transaction validation functions */

namespace Consensus {

/**
 * Checks specific to coinbase transactions.
 * Preconditions: tx.IsCoinBase() is true.
 */
bool CheckTxCoinbase(const CTransaction& tx, CValidationState& state, const int64_t flags, const int64_t nHeight);

/**
 * Fully verify a CTransaction.
 * @TODO this is incomplete, among other things, the scripts are not checked yet.
 */
bool VerifyTx(const CTransaction& tx, CValidationState& state, const int64_t flags, const int64_t nHeight, const CCoinsViewCache& inputs);

} // namespace Consensus

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
