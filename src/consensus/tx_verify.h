// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_TX_VERIFY_H
#define BITCOIN_CONSENSUS_TX_VERIFY_H

#include <coins.h>
#include <consensus/amount.h>
#include <script/verify_flags.h>

#include <concepts>
#include <cstdint>
#include <span>
#include <vector>

class CBlockIndex;
class CCoinsViewCache;
class CTransaction;
class TxValidationState;

/** Transaction validation functions */

namespace Consensus {

template <typename T>
concept CoinRef = std::convertible_to<T, const Coin&>;

/**
 * Check whether all inputs of this transaction are valid (amounts and maturity)
 * This does not modify the UTXO set. This does not check scripts and sigs.
 * @param[in] coins Sorted span of Coins containing previous transaction outputs tx is spending
 * @param[out] txfee Set to the transaction fee if successful.
 * Preconditions: tx.IsCoinBase() is false.
 */
template <Consensus::CoinRef T>
[[nodiscard]] bool CheckTxInputs(const CTransaction& tx, TxValidationState& state, const std::span<T> coins, int nSpendHeight, CAmount& txfee);
} // namespace Consensus

/** Auxiliary functions for transaction validation (ideally should not be exposed) */

/**
 * Count ECDSA signature operations the old-fashioned (pre-0.6) way
 * @return number of sigops this transaction's outputs will produce when spent
 * @see CTransaction::FetchInputs
 */
unsigned int GetLegacySigOpCount(const CTransaction& tx);

/**
 * Count ECDSA signature operations in pay-to-script-hash inputs.
 *
 * @param[in] coins Sorted span of Coins containing previous transaction outputs we're spending
 * @return maximum number of sigops required to validate this transaction's inputs
 * @see CTransaction::FetchInputs
 */
template <Consensus::CoinRef T>
unsigned int GetP2SHSigOpCount(const CTransaction& tx, const std::span<T> coins);

/**
 * Compute total signature operation cost of a transaction.
 * @param[in] tx    Transaction for which we are computing the cost
 * @param[in] coins Sorted span of Coins containing previous transaction outputs we're spending
 * @param[in] flags Script verification flags
 * @return Total signature operation cost of tx
 */
template <Consensus::CoinRef T>
int64_t GetTransactionSigOpCost(const CTransaction& tx, const std::span<T> coins, script_verify_flags flags);

/**
 * Check if transaction is final and can be included in a block with the
 * specified height and time. Consensus critical.
 */
bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime);

/**
 * Calculates the block height and previous block's median time past at
 * which the transaction will be considered final in the context of BIP 68.
 * For each input that is not sequence locked, the corresponding entries in
 * prevHeights are set to 0 as they do not affect the calculation.
 */
std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block);

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair);
/**
 * Check if transaction is final per BIP 68 sequence numbers and can be included in a block.
 * Consensus critical. Takes as input a list of heights at which tx's inputs (in order) confirmed.
 */
bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block);

#endif // BITCOIN_CONSENSUS_TX_VERIFY_H
