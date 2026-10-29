// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/consensus.h>
#include <hash.h>
#include <instantsend/instantsend.h>
#include <instantsend/lock.h>
#include <instantsend/net_instantsend.h>
#include <llmq/context.h>
#include <llmq/signhash.h>
#include <llmq/signing.h>
#include <primitives/transaction.h>
#include <spork.h>
#include <streams.h>
#include <test/util/llmq_tests.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <validation.h>

#include <string_view>

#include <boost/test/unit_test.hpp>

struct NetInstantSendTest : RegTestingSetup {
    static Uint256HashSet ProcessBatch(NetInstantSend& net, const Consensus::LLMQParams& params, int offset,
                                       const std::vector<instantsend::PendingISLockEntry>& locks)
    {
        return net.ProcessPendingInstantSendLocks(params, offset, /*ban=*/true, locks);
    }
};

BOOST_AUTO_TEST_SUITE(evo_islock_tests)

BOOST_FIXTURE_TEST_CASE(missing_quorum_does_not_drop_batch, NetInstantSendTest)
{
    constexpr const char* REGTEST_SPORK_PRIVKEY{"cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"};
    BOOST_REQUIRE(m_node.sporkman->SetSporkAddress(Params().SporkAddresses().front()));
    BOOST_REQUIRE(m_node.sporkman->SetMinSporkKeys(1));
    BOOST_REQUIRE(m_node.sporkman->SetPrivKey(REGTEST_SPORK_PRIVKEY));
    BOOST_REQUIRE(m_node.sporkman->UpdateSpork(SPORK_2_INSTANTSEND_ENABLED, 0).has_value());
    auto& sigman = *m_node.llmq_ctx->sigman;
    auto& isman = *m_node.llmq_ctx->isman;
    NetInstantSend net{m_node.peerman.get(), isman, *m_node.llmq_ctx->qman, m_node.chainman->ActiveChainstate()};
    const auto cycle_hash = WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Genesis()->GetBlockHash());
    CBLSSecretKey key;
    key.MakeNewKey();

    for (const auto type : {Consensus::LLMQType::LLMQ_TEST_DIP0024, Consensus::LLMQType::LLMQ_TEST_INSTANTSEND}) {
        const auto& params = llmq::testutils::GetLLMQParams(type);
        std::vector<instantsend::PendingISLockEntry> locks;
        for (int i = 0; i < 3; ++i) {
            auto islock = std::make_shared<instantsend::InstantSendLock>();
            islock->txid = GetRandHash();
            islock->inputs.emplace_back(GetRandHash(), 0);
            islock->cycleHash = cycle_hash;
            const auto sign_hash = llmq::SignHash{type, cycle_hash, islock->GetRequestId(), islock->txid}.Get();
            const bool legacy = bls::bls_legacy_scheme.load();
            islock->sig.Set(key.Sign(sign_hash, legacy), legacy);
            if (i != 1) {
                // Exercise the already-verified path on either side of an unavailable quorum.
                BOOST_REQUIRE(sigman.ProcessRecoveredSig(
                    std::make_shared<llmq::CRecoveredSig>(type, cycle_hash, islock->GetRequestId(), islock->txid,
                                                          islock->sig)));
            }
            locks.push_back({{i == 2 ? 2 : 1, islock}, ::SerializeHash(*islock)});
        }

        const auto failed = ProcessBatch(net, params, /*offset=*/0, locks);
        BOOST_CHECK_EQUAL(failed.size(), 1U);
        BOOST_CHECK(failed.contains(locks[1].islock_hash));
        BOOST_CHECK(isman.AlreadyHave(CInv{MSG_ISDLOCK, locks[0].islock_hash}));
        BOOST_CHECK(!isman.AlreadyHave(CInv{MSG_ISDLOCK, locks[1].islock_hash}));
        BOOST_CHECK(isman.AlreadyHave(CInv{MSG_ISDLOCK, locks[2].islock_hash}));

        const auto retried = ProcessBatch(net, params, params.dkgInterval, {locks[1]});
        BOOST_CHECK(retried.contains(locks[1].islock_hash));
        BOOST_CHECK(!isman.AlreadyHave(CInv{MSG_ISDLOCK, locks[1].islock_hash}));
        BOOST_CHECK(!sigman.HasRecoveredSig(type, locks[1].islock->GetRequestId(), locks[1].islock->txid));
        BOOST_CHECK(sigman.FetchPendingReconstructed().empty());
    }
}

uint256 CalculateRequestId(const std::vector<COutPoint>& inputs)
{
    CHashWriter hw(SER_GETHASH, 0);
    hw << std::string_view("islock");
    hw << inputs;
    return hw.GetHash();
}

BOOST_AUTO_TEST_CASE(getrequestid)
{
    // Create an empty InstantSendLock
    instantsend::InstantSendLock islock;

    // Compute expected hash for an empty inputs vector.
    // Note: InstantSendLock::GetRequestId() serializes the prefix "islock"
    // followed by the 'inputs' vector.
    {
        const uint256 expected = CalculateRequestId(islock.inputs);

        BOOST_CHECK(islock.GetRequestId() == expected);
    }

    // Now add two dummy inputs to the lock
    islock.inputs.clear();
    // Construct two dummy outpoints (using uint256S for a dummy hash)
    COutPoint op1(uint256::ONE, 0);
    COutPoint op2(uint256::TWO, 1);
    islock.inputs.push_back(op1);
    islock.inputs.push_back(op2);

    const uint256 expected = CalculateRequestId(islock.inputs);

    BOOST_CHECK(islock.GetRequestId() == expected);
}

BOOST_AUTO_TEST_CASE(deserialize_instantlock_from_realdata2)
{
    // Expected values from the provided getislocks output:
    const std::string_view expectedTxidStr = "7b33968effa613e8ea9c1b5734c9bbbe467ff4650f8060caf8a5c213c6059d5b";
    const std::string_view expectedCycleHashStr = "000000000000000bbd0b1bb95540351e7ee99c5b08efde076b3d712a57ea74d6";
    const std::string_view expectedSignatureStr =
        "997d0b36738a9eef46ceeb4405998ff7235317708f277402799ffe05258015cae9b6bae"
        "43683f992b2f50f70f8f0cb9c0f26af340b00903e93995c1345d1b2c5b697ebecdbe581"
        "1dd112e11889101dcb4553b2bc206ab304026b96c07dec4f24";
    const std::string quorumHashStr = "0000000000000019756ecc9c9c5f476d3f66876b1dcfa5dde1ea82f0d99334a2";
    const std::string_view expectedSignHashStr = "6a3c37bc610c4efd5babd8941068a8eca9e7bec942fe175b8ca9cae31b67e838";
    // The serialized InstantSend lock from the "hex" field of getislocks:
    const std::string_view islockHex =
        "0101497915895c30eebfad0c5fcfb9e0e72308c7e92cd3749be2fd49c8320c4c58b6010000005b9d05c613c2a5f8ca60800f65f47f46be"
        "bbc934571b9ceae813a6ff8e96337bd674ea572a713d6b07deef085b9ce97e1e354055b91b0bbd0b00000000000000997d0b36738a9eef"
        "46ceeb4405998ff7235317708f277402799ffe05258015cae9b6bae43683f992b2f50f70f8f0cb9c0f26af340b00903e93995c1345d1b2"
        "c5b697ebecdbe5811dd112e11889101dcb4553b2bc206ab304026b96c07dec4f24";

    // This islock was created with non-legacy. Using legacy will result in the signature being all zeros.
    bls::bls_legacy_scheme.store(false);

    // Convert hex string to a byte vector and deserialize.
    std::vector<unsigned char> islockData = ParseHex(islockHex);
    CDataStream ss(islockData, SER_NETWORK, PROTOCOL_VERSION);
    instantsend::InstantSendLock islock;
    ss >> islock;

    // Verify the calculated signHash
    auto signHash =
        llmq::SignHash(Consensus::LLMQType::LLMQ_60_75, uint256S(quorumHashStr), islock.GetRequestId(), islock.txid).Get();
    BOOST_CHECK_EQUAL(signHash.ToString(), expectedSignHashStr);

    // Verify the txid field.
    BOOST_CHECK_EQUAL(islock.txid.ToString(), expectedTxidStr);

    // Verify the cycleHash field.
    BOOST_CHECK_EQUAL(islock.cycleHash.ToString(), expectedCycleHashStr);

    // Verify the inputs vector has exactly one element.
    BOOST_REQUIRE_EQUAL(islock.inputs.size(), 1U);
    const COutPoint& input = islock.inputs.front();
    const std::string expectedInputTxid = "b6584c0c32c849fde29b74d32ce9c70823e7e0b9cf5f0cadbfee305c89157949";
    const unsigned int expectedInputN = 1;
    BOOST_CHECK_EQUAL(input.hash.ToString(), expectedInputTxid);
    BOOST_CHECK_EQUAL(input.n, expectedInputN);

    // Compute the expected request ID: it is the hash of the constant prefix "islock" followed by the inputs.
    uint256 expectedRequestId = CalculateRequestId(islock.inputs);
    BOOST_CHECK_EQUAL(islock.GetRequestId().ToString(), expectedRequestId.ToString());

    // Verify the signature field.
    BOOST_CHECK_EQUAL(islock.sig.Get().ToString(), expectedSignatureStr);
}

BOOST_AUTO_TEST_CASE(geninputlockrequestid_basic)
{
    // Test that GenInputLockRequestId generates consistent hashes for the same outpoint
    const uint256 txHash = uint256S("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    const uint32_t outputIndex = 5;

    COutPoint outpoint1(txHash, outputIndex);
    COutPoint outpoint2(txHash, outputIndex);

    // Same outpoints should produce identical request IDs
    const uint256 requestId1 = instantsend::GenInputLockRequestId(outpoint1);
    const uint256 requestId2 = instantsend::GenInputLockRequestId(outpoint2);

    BOOST_CHECK(requestId1 == requestId2);
    BOOST_CHECK(!requestId1.IsNull());
}

BOOST_AUTO_TEST_CASE(geninputlockrequestid_different_outpoints)
{
    // Test that different outpoints produce different request IDs
    const uint256 txHash1 = uint256S("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    const uint256 txHash2 = uint256S("0xfedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321");

    COutPoint outpoint1(txHash1, 0);
    COutPoint outpoint2(txHash2, 0);
    COutPoint outpoint3(txHash1, 1); // Same hash, different index

    const uint256 requestId1 = instantsend::GenInputLockRequestId(outpoint1);
    const uint256 requestId2 = instantsend::GenInputLockRequestId(outpoint2);
    const uint256 requestId3 = instantsend::GenInputLockRequestId(outpoint3);

    // All should be different
    BOOST_CHECK(requestId1 != requestId2);
    BOOST_CHECK(requestId1 != requestId3);
    BOOST_CHECK(requestId2 != requestId3);
}

BOOST_AUTO_TEST_CASE(geninputlockrequestid_only_outpoint_matters)
{
    // Critical test: Verify that only the COutPoint is hashed, not scriptSig or nSequence
    // This validates the fix where CTxIn was incorrectly used before
    const uint256 txHash = uint256S("0xabcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
    const uint32_t outputIndex = 3;

    COutPoint outpoint(txHash, outputIndex);

    // Create two CTxIn objects with the same prevout but different scriptSig and nSequence
    CTxIn txin1;
    txin1.prevout = outpoint;
    txin1.scriptSig = CScript() << OP_1 << OP_2;
    txin1.nSequence = 0xFFFFFFFF;

    CTxIn txin2;
    txin2.prevout = outpoint;
    txin2.scriptSig = CScript() << OP_3 << OP_4 << OP_5; // Different scriptSig
    txin2.nSequence = 0x12345678;                        // Different nSequence

    // The request IDs should be identical because they share the same prevout (COutPoint)
    const uint256 requestId1 = instantsend::GenInputLockRequestId(txin1.prevout);
    const uint256 requestId2 = instantsend::GenInputLockRequestId(txin2.prevout);

    BOOST_CHECK(requestId1 == requestId2);

    // Also verify against the direct outpoint
    const uint256 requestId3 = instantsend::GenInputLockRequestId(outpoint);
    BOOST_CHECK(requestId1 == requestId3);
}

BOOST_AUTO_TEST_CASE(geninputlockrequestid_serialization_format)
{
    // Test that the serialization format is: SerializeHash(pair("inlock", outpoint))
    const uint256 txHash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000001");
    const uint32_t outputIndex = 0;

    COutPoint outpoint(txHash, outputIndex);

    // Calculate the expected hash manually
    const uint256 expectedHash = ::SerializeHash(std::make_pair(std::string_view("inlock"), outpoint));

    // Get the actual hash from the function
    const uint256 actualHash = instantsend::GenInputLockRequestId(outpoint);

    BOOST_CHECK(actualHash == expectedHash);
}

BOOST_AUTO_TEST_CASE(geninputlockrequestid_edge_cases)
{
    // Test edge cases: null hash, max index
    COutPoint nullOutpoint(uint256(), 0);
    COutPoint maxIndexOutpoint(uint256::ONE, COutPoint::NULL_INDEX);

    const uint256 nullRequestId = instantsend::GenInputLockRequestId(nullOutpoint);
    const uint256 maxIndexRequestId = instantsend::GenInputLockRequestId(maxIndexOutpoint);

    // Both should produce valid (non-null) request IDs
    BOOST_CHECK(!nullRequestId.IsNull());
    BOOST_CHECK(!maxIndexRequestId.IsNull());

    // And they should be different from each other
    BOOST_CHECK(nullRequestId != maxIndexRequestId);
}

// Regression test for the islock input cap: an oversized input vector must be
// rejected by TriviallyValid(), while the cap is derived so it can never reject a valid lock.
BOOST_AUTO_TEST_CASE(trivially_valid_input_cap)
{
    // A lock with a non-null txid and a few unique inputs is trivially valid.
    instantsend::InstantSendLock islock;
    islock.txid = uint256::ONE;
    islock.inputs = {COutPoint(uint256::ONE, 0), COutPoint(uint256::ONE, 1)};
    BOOST_CHECK(islock.TriviallyValid());

    // MAX_INPUTS is derived from MaxBlockSize(): a consensus-valid transaction must fit in a
    // block and each input is >=41 bytes on the wire, so it can never carry more than
    // MaxBlockSize() / 41 inputs. The cap thus cannot reject a legitimate islock, and it
    // tracks any future block-size change.
    BOOST_CHECK_EQUAL(instantsend::InstantSendLock::MAX_INPUTS, MaxBlockSize() / 41);

    // A lock carrying more than MAX_INPUTS inputs is rejected up front, before any O(n)
    // hashing/dedup work, so a peer cannot pin an oversized input vector.
    instantsend::InstantSendLock oversized;
    oversized.txid = uint256::ONE;
    oversized.inputs.assign(instantsend::InstantSendLock::MAX_INPUTS + 1, COutPoint(uint256::ONE, 0));
    BOOST_CHECK(!oversized.TriviallyValid());
}

BOOST_AUTO_TEST_SUITE_END()
