// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include "amount.h"
#include "consensus/params.h"

class CBlock;
class CBlockHeader;
class CBlockIndex;
class CCoinsViewEfficient;
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
/** No amount larger than this (in satoshi) is valid */
static const CAmount MAX_MONEY = 21000000 * COIN;

/**
 * Consensus validations:
 * Check_ means checking everything possible with the data provided.
 * Verify_ means all data provided was enough for this level and its "consensus-verified".
 */
namespace Consensus {

/** Full verification functions */
bool VerifyBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& params, int64_t nTime, CBlockIndex* pindexPrev);

/** Context-independent validity checks */
inline bool VerifyAmount(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
bool CheckTx(const CTransaction& tx, CValidationState &state);
bool CheckBlockHeader(const CBlockHeader& block, int64_t nTime, CValidationState& state, const Consensus::Params& params, bool fCheckPOW = true);
bool CheckBlock(const CBlock& block, int64_t nTime, CValidationState& state, const Consensus::Params& params, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

/** Context-dependent validity checks */
bool ContextualCheckBlockHeader(const CBlockHeader& block, CValidationState& state, CBlockIndex* pindexPrev, const Consensus::Params& params);
bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime);
/**
 * Check whether all inputs of this transaction are valid (no double spends and amounts)
 * This does not modify the UTXO set. This does not check scripts and sigs.
 */
bool CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewEfficient& inputs, int nSpendHeight);
/**
 * Preconditions: tx.IsCoinBase() is false.
 * Check whether all inputs of this transaction are valid (scripts and sigs)
 * This does not modify the UTXO set. This does not check double spends and amounts.
 * This is the more expensive consensus check for a transaction, do it last.
 */
bool CheckTxInputsScripts(const CTransaction& tx, CValidationState& state, const CCoinsViewEfficient& inputs, bool cacheStore, unsigned int flags);
bool ContextualCheckBlock(const CBlock& block, CValidationState& state, const Consensus::Params& params, const CBlockIndex* pindexPrev);

/** Utility functions */
/**
 * BIP16 didn't become active until Apr 1 2012
 * Starts enforcing the DERSIG (BIP66) rules, for block.nVersion=3 blocks, when 75% of the network has upgraded
 */
unsigned int GetFlags(const CBlock& block, CBlockIndex* pindex, const Consensus::Params& params);
/**
 * Count ECDSA signature operations the old-fashioned (pre-0.6) way
 * @return number of sigops this transaction's outputs will produce when spent
 * @see CTransaction::FetchInputs
 */
unsigned int GetLegacySigOpCount(const CTransaction& tx);
/**
 * Count ECDSA signature operations in pay-to-script-hash inputs.
 * @param[in] mapInputs Map of previous transactions that have outputs we're spending
 * @return maximum number of sigops required to validate this transaction's inputs
 * @see CTransaction::FetchInputs
 */
unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewEfficient& mapInputs);
/**
 * Count ECDSA signature operations.
 * @see Consensus::GetLegacySigOpCount and Consensus::GetP2SHSigOpCount
 */
unsigned int GetSigOpCount(const CTransaction&, const CCoinsViewEfficient&);
/**
 * Return sum of txouts for a given transaction.
 * GetValueIn() is a method on CCoinsViewCache, because
 * inputs must be known to compute value in.
 */
CAmount GetValueOut(const CTransaction& tx);

} // namespace Consensus

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
