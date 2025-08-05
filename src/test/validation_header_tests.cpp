#include <boost/test/unit_test.hpp>

#include <arith_uint256.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <pow.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <util/time.h>
#include <validation.h>

BOOST_FIXTURE_TEST_SUITE(validation_header_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(reject_future_timestamp)
{
    const auto params = CreateChainParams(*m_node.args, ChainType::REGTEST)->GetConsensus();
    SetMockTime(NodeSeconds{0});

    CBlockHeader header;
    header.nBits = UintToArith256(params.powLimit).GetCompact();
    header.nTime = MAX_FUTURE_BLOCK_TIME + 1; // far in future relative to mock time
    BlockValidationState state;
    BOOST_CHECK(!CheckBlockHeader(header, state, params));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "time-too-new");
}

BOOST_AUTO_TEST_CASE(reject_invalid_difficulty)
{
    const auto params = CreateChainParams(*m_node.args, ChainType::REGTEST)->GetConsensus();
    SetMockTime(NodeSeconds{0});

    CBlockHeader header;
    header.nTime = 1;
    header.nBits = 0x1f111111; // target higher than powLimit
    BlockValidationState state;
    BOOST_CHECK(!CheckBlockHeader(header, state, params, /*fCheckPOW=*/false));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-diffbits");
}

BOOST_AUTO_TEST_CASE(reject_invalid_pow)
{
    const auto params = CreateChainParams(*m_node.args, ChainType::REGTEST)->GetConsensus();
    SetMockTime(NodeSeconds{0});

    CBlockHeader header;
    header.nTime = 1;
    arith_uint256 target = UintToArith256(params.powLimit) >> 20;
    header.nBits = target.GetCompact();
    BlockValidationState state;
    uint256 hash = header.GetHash();
    BOOST_CHECK(UintToArith256(hash) > target); // ensure hash exceeds target
    BOOST_CHECK(!CheckBlockHeader(header, state, params));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "high-hash");
}

BOOST_AUTO_TEST_SUITE_END()
