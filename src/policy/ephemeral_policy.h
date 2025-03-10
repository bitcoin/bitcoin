// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_EPHEMERAL_POLICY_H
#define BITCOIN_POLICY_EPHEMERAL_POLICY_H

#include <consensus/amount.h>
#include <policy/packages.h>
#include <primitives/transaction.h>

#include <optional>

class CFeeRate;
class CTxMemPool;
class TxValidationState;

/** These utility functions ensure that ephemeral dust is safely
 * created and spent without unduly risking them entering the utxo
 * set.

 * This is ensured by requiring:
 * - PreCheckEphemeralTx checks are respected
 * - The parent has no child (and 0-fee as implied above to disincentivize mining)
 * - OR the parent transaction has exactly one child, and the dust is spent by that child
 *
 * Imagine three transactions:
 * TxA, 0-fee with two outputs, one non-dust, one dust
 * TxB, spends TxA's non-dust
 * TxC, spends TxA's dust
 *
 * All the dust is spent if TxA+TxB+TxC is accepted, but the mining template may just pick
 * up TxA+TxB rather than the three "legal configurations":
 * 1) None
 * 2) TxA+TxB+TxC
 * 3) TxA+TxC
 * By requiring the child transaction to sweep any dust from the parent txn, we ensure that
 * there is a single child only, and this child, or the child's descendants,
 * are the only way to bring fees.
 */

/* All the following checks are only called if standardness rules are being applied. */

/** Must be called for each transaction once transaction fees are known.
 * Does context-less checks about a single transaction.
 * @returns false if the fee is non-zero and dust exists, populating state. True otherwise.
 */
bool PreCheckEphemeralTx(const CTransaction& tx, CFeeRate dust_relay_rate, CAmount base_fee, CAmount mod_fee, TxValidationState& state);

/** Must be called for each transaction(package) if any dust is in the package.
 *  Checks that each transaction's parents have their dust spent by the child,
 *  where parents are either in the mempool or in the package itself.
 *  Sets out_child_state and out_child_wtxid on failure.
 *  @returns true if all dust is properly spent.
 */
bool CheckEphemeralSpends(const Package& package, CFeeRate dust_relay_rate, const CTxMemPool& tx_pool, TxValidationState& out_child_state, Wtxid& out_child_wtxid);

#endif // BITCOIN_POLICY_EPHEMERAL_POLICY_H
