// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SIGNHASH_H
#define BITCOIN_LLMQ_SIGNHASH_H

#include <llmq/params.h>
#include <uint256.h>

#include <cstring>
#include <vector>

#include <saltedhasher.h>
#include <util/hash_type.h>

namespace llmq {

/**
 * SignHash is a strongly-typed wrapper for the hash used in LLMQ signing operations.
 * It encapsulates the hash calculation for quorum signatures, replacing the need for
 * BuildSignHash function and avoiding circular dependencies.
 */
class SignHash : public BaseHash<uint256>
{
public:
    SignHash() = default;
    using BaseHash<uint256>::BaseHash;

    /**
     * Constructs a SignHash from the given parameters.
     * This replaces the previous BuildSignHash function.
     */
    SignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash);

    /**
     * Get the underlying uint256 hash value.
     */
    const uint256& Get() const { return m_hash; }

    // Serialization support
    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_hash;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> m_hash;
    }
};

// Salted hasher for llmq::SignHash that reuses the salted hasher for uint256
struct SignHashSaltedHasher {
    std::size_t operator()(const SignHash& signHash) const noexcept { return StaticSaltedHasher{}(signHash.Get()); }
};

} // namespace llmq

// Make SignHash hashable for use in unordered_map
template <>
struct std::hash<llmq::SignHash> {
    std::size_t operator()(const llmq::SignHash& signHash) const noexcept
    {
        // Use the first 8 bytes of the hash as the hash value
        const unsigned char* data = signHash.data();
        std::size_t result;
        std::memcpy(&result, data, sizeof(result));
        return result;
    }
};


#endif // BITCOIN_LLMQ_SIGNHASH_H