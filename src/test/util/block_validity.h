// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H
#define BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H

#include <node/mining_types.h>
#include <test/util/setup_common.h>
#include <validation.h>

/**
 * ValidationBlockValidityTestingSetup relies on TestChain100Setup, which
 * mines 100 blocks during initialization.
 */
struct ValidationBlockValidityTestingSetup : public TestChain100Setup {
    Chainstate& m_chainstate{m_node.chainman->ActiveChainstate()};
    const Consensus::Params& m_params{m_node.chainman->GetParams().GetConsensus()};

    /** Create a new block template using the provided options. */
    CBlock MakeBlock(node::BlockCreateOptions options = {});
    /** Reset all cached validity flags (fChecked, m_checked_merkle_root, m_checked_witness_commitment). */
    void ResetBlock(CBlock& block);

    /** Verify a block's validity using TestBlockValidity. */
    BlockValidationState TestValidity(CBlock& block, bool check_pow = false, bool check_merkle = true);

    /** Helper to add a spendable coin to the UTXO set for testing. */
    COutPoint AddCoin(const CScript& script_pub_key = CScript() << OP_TRUE, CAmount amount = 1 * COIN);
};

#endif // BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H
