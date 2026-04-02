// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <test/util/block_validity.h>

#include <test/util/mining.h>

#include <boost/test/unit_test.hpp>

namespace {

//! Default number of trials for AddCoin to find a unique random outpoint
static constexpr int MAX_ADDCOIN_TRIALS = 5;
} // namespace

CBlock ValidationBlockValidityTestingSetup::MakeBlock(BlockAssembler::Options options)
{
    auto block_ptr = PrepareBlock(m_node, options);
    Assert(block_ptr);
    return *block_ptr;
}

BlockValidationState ValidationBlockValidityTestingSetup::TestValidity(const CBlock& block, bool check_pow, bool check_merkle)
{
    LOCK(cs_main);
    return TestBlockValidity(m_chainstate, block, check_pow, check_merkle);
}

std::optional<COutPoint> ValidationBlockValidityTestingSetup::AddCoin(const CScript& script_pub_key, CAmount amount)
{
    for (int trial_idx = 0; trial_idx < MAX_ADDCOIN_TRIALS; ++trial_idx) {
        COutPoint outpoint{Txid::FromUint256(m_rng.rand256()), 0};
        LOCK(cs_main);
        if (m_chainstate.CoinsTip().AccessCoin(outpoint).IsSpent()) {
            m_chainstate.CoinsTip().AddCoin(outpoint, Coin(CTxOut(amount, script_pub_key), 1, false), true);
            return outpoint;
        }
    }
    return std::nullopt;
}

void CheckBlockValid(const BlockValidationState& state)
{
    BOOST_CHECK_MESSAGE(state.IsValid(), strprintf("Expected block to be valid but got reject reason: %s", state.GetRejectReason()));
    BOOST_CHECK(state.GetResult() == BlockValidationResult::BLOCK_RESULT_UNSET);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "");
    BOOST_CHECK_EQUAL(state.GetDebugMessage(), "");
}

void CheckBlockInvalid(const BlockValidationState& state, BlockValidationResult expected_result, const std::string& expected_reason, const std::string& expected_debug_message)
{
    BOOST_CHECK_MESSAGE(state.IsInvalid(), "Expected block to be invalid but it is valid");
    BOOST_CHECK(state.GetResult() == expected_result);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), expected_reason);
    BOOST_CHECK_EQUAL(state.GetDebugMessage(), expected_debug_message);
}
