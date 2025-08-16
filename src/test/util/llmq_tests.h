// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_LLMQ_TESTS_H
#define BITCOIN_TEST_UTIL_LLMQ_TESTS_H

#include <test/util/setup_common.h>

#include <arith_uint256.h>
#include <bls/bls.h>
#include <consensus/params.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>

#include <chainlock/clsig.h>
#include <llmq/commitment.h>
#include <llmq/params.h>

#include <vector>

namespace llmq {
namespace testutils {

// Helper function to get LLMQ params from available_llmqs
inline const Consensus::LLMQParams& GetLLMQParams(Consensus::LLMQType type)
{
    for (const auto& params : Consensus::available_llmqs) {
        if (params.type == type) {
            return params;
        }
    }
    throw std::runtime_error("LLMQ type not found");
}

// Helper functions to create test data
inline CBLSPublicKey CreateRandomBLSPublicKey()
{
    CBLSSecretKey sk;
    sk.MakeNewKey();
    return sk.GetPublicKey();
}

inline CBLSSignature CreateRandomBLSSignature()
{
    CBLSSecretKey sk;
    sk.MakeNewKey();
    uint256 hash = InsecureRand256();
    return sk.Sign(hash, false);
}

inline CFinalCommitment CreateValidCommitment(const Consensus::LLMQParams& params, const uint256& quorumHash)
{
    CFinalCommitment commitment;
    commitment.llmqType = params.type;
    commitment.quorumHash = quorumHash;
    commitment.validMembers.resize(params.size, true);
    commitment.signers.resize(params.size, true);
    commitment.quorumVvecHash = InsecureRand256();
    commitment.quorumPublicKey = CreateRandomBLSPublicKey();
    commitment.quorumSig = CreateRandomBLSSignature();
    commitment.membersSig = CreateRandomBLSSignature();
    return commitment;
}

inline chainlock::ChainLockSig CreateChainLock(int32_t height, const uint256& blockHash)
{
    CBLSSignature sig = CreateRandomBLSSignature();
    return chainlock::ChainLockSig(height, blockHash, sig);
}

// Helper to create bit vectors with specific patterns
inline std::vector<bool> CreateBitVector(size_t size, const std::vector<size_t>& trueBits)
{
    std::vector<bool> result(size, false);
    for (size_t idx : trueBits) {
        if (idx < size) {
            result[idx] = true;
        }
    }
    return result;
}

// Serialization round-trip test helper
template <typename T>
inline bool TestSerializationRoundtrip(const T& obj)
{
    // Datastreams we will compare at the end
    CDataStream ss_obj(SER_NETWORK, PROTOCOL_VERSION);
    CDataStream ss_obj_rt(SER_NETWORK, PROTOCOL_VERSION);
    // A temporary datastream to perform serialization round-trip
    CDataStream ss_tmp(SER_NETWORK, PROTOCOL_VERSION);
    T obj_rt;

    ss_obj << obj;
    ss_tmp << obj;
    ss_tmp >> obj_rt;
    ss_obj_rt << obj_rt;

    return ss_obj.str() == ss_obj_rt.str();
}

// Helper to create deterministic test data
inline uint256 GetTestQuorumHash(uint32_t n) { return ArithToUint256(arith_uint256(n)); }

inline uint256 GetTestBlockHash(uint32_t n) { return ArithToUint256(arith_uint256(n) << 32); }

} // namespace testutils
} // namespace llmq

#endif // BITCOIN_TEST_UTIL_LLMQ_TESTS_H
