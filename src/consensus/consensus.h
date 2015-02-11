// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include "consensus/params.h"
#include "consensus/types.h"

#include <stdint.h>

class CBlockHeader;
class CCoinsViewCache;
class CTransaction;
class CValidationState;

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 1000000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;
/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int LOCKTIME_THRESHOLD = 500000000; // Tue Nov  5 00:53:20 1985 UTC

/**
 * Consensus validations:
 * Check_ means checking everything possible with the data provided.
 * Verify_ means all data provided was enough for this level and its "consensus-verified".
 */
namespace Consensus {

/** Transaction validation functions */

bool CheckTx(const CTransaction&, CValidationState&);
/**
 * Check whether all inputs of this transaction are valid (no double spends and amounts)
 * This does not modify the UTXO set. This does not check scripts and sigs.
 * Preconditions: tx.IsCoinBase() is false.
 */
bool CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight);
/**
 * Preconditions: tx.IsCoinBase() is false.
 * Check whether all inputs of this transaction are valid (scripts and sigs)
 * This does not modify the UTXO set. This does not check double spends and amounts.
 * This is the more expensive consensus check for a transaction, do it last.
 */
bool CheckTxInputsScripts(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, bool cacheStore, unsigned int flags);

/** Block header validation functions */

bool VerifyBlockHeader(const CBlockHeader&, CValidationState&, const Consensus::Params&, int64_t nTime, CBlockIndexBase*, PrevIndexGetter);
bool CheckBlockHeader(const CBlockHeader&, CValidationState&, const Consensus::Params&, int64_t nTime, bool fCheckPOW = true);
bool ContextualCheckBlockHeader(const CBlockHeader&, CValidationState&, const Consensus::Params&, const CBlockIndexBase*, PrevIndexGetter);

} // namespace Consensus

/** Transaction validation utility functions */

bool CheckFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime);

/** Block header validation utility functions */

int64_t GetMedianTimePast(const CBlockIndexBase* pindex, PrevIndexGetter indexGetter);
unsigned int GetNextWorkRequired(const CBlockIndexBase*, const CBlockHeader*, const Consensus::Params&, PrevIndexGetter);
unsigned int CalculateNextWorkRequired(const CBlockIndexBase* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&);
/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);
/**
 * Returns true if there are nRequired or more blocks of minVersion or above
 * in the last Consensus::Params::nMajorityWindow blocks, starting at pstart and going backwards.
 */
bool IsSuperMajority(int minVersion, const CBlockIndexBase* pstart, unsigned nRequired, const Consensus::Params& consensusParams, PrevIndexGetter);

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
