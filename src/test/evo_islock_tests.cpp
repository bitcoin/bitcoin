// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/consensus.h>
#include <hash.h>
#include <instantsend/lock.h>
#include <llmq/signing.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <string_view>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(evo_islock_tests)

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
    auto signHash = llmq::BuildSignHash(Consensus::LLMQType::LLMQ_60_75, uint256S(quorumHashStr), islock.GetRequestId(),
                                        islock.txid);
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

BOOST_AUTO_TEST_SUITE_END()
