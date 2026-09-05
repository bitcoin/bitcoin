// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_BLOCK_VALIDITY_CHECKS_H
#define BITCOIN_TEST_BLOCK_VALIDITY_CHECKS_H

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <tinyformat.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

inline void CheckBlockValid(const BlockValidationState& state)
{
    BOOST_CHECK_MESSAGE(state.IsValid(),
                        strprintf("Expected block to be valid but got reject reason: %s", state.GetRejectReason()));
    BOOST_CHECK(state.GetResult() == BlockValidationResult::BLOCK_RESULT_UNSET);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "");
    BOOST_CHECK_EQUAL(state.GetDebugMessage(), "");
}

inline void CheckBlockInvalid(const BlockValidationState& state,
                              BlockValidationResult expected_result,
                              const std::string& expected_reason,
                              const std::string& expected_debug_message)
{
    BOOST_CHECK_MESSAGE(state.IsInvalid(), "Expected block to be invalid but it is valid");
    BOOST_CHECK(state.GetResult() == expected_result);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), expected_reason);
    BOOST_CHECK_EQUAL(state.GetDebugMessage(), expected_debug_message);
}

/** Assert that a block with a script violation has the expected error state. */
inline void CheckScriptViolation(const BlockValidationState& state,
                                 const CBlock& block,
                                 const COutPoint& outpoint,
                                 const std::string& expected_reason)
{
    Assert(!block.vtx.empty());
    const auto debug = strprintf("input 0 of %s (wtxid %s), spending %s:%u",
                                 block.vtx.back()->GetHash().ToString(),
                                 block.vtx.back()->GetWitnessHash().ToString(),
                                 outpoint.hash.ToString(),
                                 outpoint.n);
    CheckBlockInvalid(state, BlockValidationResult::BLOCK_CONSENSUS, expected_reason, debug);
}

/** Assert that a block with a transaction-level violation has the expected error state. */
inline void CheckTxViolation(const BlockValidationState& state,
                             const CBlock& block,
                             const std::string& expected_reason,
                             const std::string& prefix = "Transaction check failed")
{
    const auto debug = strprintf("%s (tx hash %s) ", prefix, block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(state, BlockValidationResult::BLOCK_CONSENSUS, expected_reason, debug);
}

#endif // BITCOIN_TEST_BLOCK_VALIDITY_CHECKS_H
