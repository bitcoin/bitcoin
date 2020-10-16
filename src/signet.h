// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SIGNET_H
#define BITCOIN_SIGNET_H

#include <consensus/params.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

#include <optional.h>

/**
 * Extract signature and check whether a block has a valid solution
 */
bool CheckSignetBlockSolution(const CBlock& block, const Consensus::Params& consensusParams);

typedef uint8_t TransactionProofResult;
constexpr TransactionProofResult TransactionProofInvalid       = 1;
constexpr TransactionProofResult TransactionProofInconclusive  = 2;
constexpr TransactionProofResult TransactionProofValid         = 3;
constexpr TransactionProofResult TransactionProofInFutureFlag  = 0x80;

TransactionProofResult CheckTransactionProof(const std::string& message, const CScript& challenge, const CTransaction& to_spend, const CTransaction& to_sign);

std::vector<uint8_t> GetMessageCommitment(const std::string& message);

/**
 * Generate the signet tx corresponding to the given block
 *
 * The signet tx commits to everything in the block except:
 * 1. It hashes a modified merkle root with the signet signature removed.
 * 2. It skips the nonce.
 */
class SignetTxs {
    template<class T1, class T2>
    SignetTxs(const T1& to_spend, const T2& to_sign) : m_to_spend{to_spend}, m_to_sign{to_sign} { }

public:
    static Optional<SignetTxs> Create(const CScript& signature, const std::vector<std::vector<uint8_t>>& witnessStack, const std::vector<uint8_t>& commitment, const CScript& challenge, uint32_t locktime = 0, uint32_t sequence = 0);
    static Optional<SignetTxs> Create(const CBlock& block, const CScript& challenge);

    const CTransaction m_to_spend;
    const CTransaction m_to_sign;
};

#endif // BITCOIN_SIGNET_H
