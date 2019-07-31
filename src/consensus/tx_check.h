// Copyright (c) 2017-2018 The Bitcointalkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINTALKCOIN_CONSENSUS_TX_CHECK_H
#define BITCOINTALKCOIN_CONSENSUS_TX_CHECK_H

/**
 * Context-independent transaction checking code that can be called outside the
 * bitcointalkcoin server and doesn't depend on chain or mempool state. Transaction
 * verification code that does call server functions or depend on server state
 * belongs in tx_verify.h/cpp instead.
 */

class CTransaction;
class CValidationState;

bool CheckTransaction(const CTransaction& tx, CValidationState& state, bool fCheckDuplicateInputs=true);

#endif // BITCOINTALKCOIN_CONSENSUS_TX_CHECK_H
