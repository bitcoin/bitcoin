// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SIGNHASH_H
#define BITCOIN_LLMQ_SIGNHASH_H

#include <llmq/params.h>
#include <uint256.h>

#include <vector>

#include <util/hash_type.h>

namespace llmq
{

/**
 * SignHash is a strongly-typed wrapper for the hash used in LLMQ signing operations.
 * It encapsulates the hash calculation for quorum signatures, replacing the need for
 * BuildSignHash function and avoiding circular dependencies.
 */
class SignHash : public BaseHash<uint256> {
public:
    SignHash() = default;

    /**
     * Constructs a SignHash from the given parameters.
     * This replaces the previous BuildSignHash function.
     */
    SignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash);

    /**
     * Get the underlying uint256 hash value.
     */
    const uint256& Get() const { return m_hash; }
};

} // namespace llmq

#endif // BITCOIN_LLMQ_SIGNHASH_H