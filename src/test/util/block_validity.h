// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H
#define BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H

#include <node/miner.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <optional>

using node::BlockAssembler;

/**
 * ValidationBlockValidityTestingSetup relies on TestChain100Setup, which
 * Mines 100 blocks during initialization.
 *
 */
struct ValidationBlockValidityTestingSetup : public TestChain100Setup {
    Chainstate& m_chainstate{m_node.chainman->ActiveChainstate()};
    const Consensus::Params& m_params{m_node.chainman->GetParams().GetConsensus()};

    /** Create a new block template using the provided options. */
    CBlock MakeBlock(BlockAssembler::Options options = {});

    /** Verify a block's validity using TestBlockValidity. */
    BlockValidationState TestValidity(const CBlock& block, bool check_pow = false, bool check_merkle = true);

    /** Helper to add a spendable coin to the UTXO set for testing. */
    std::optional<COutPoint> AddCoin(const CScript& script_pub_key = CScript() << OP_TRUE, CAmount amount = 1 * COIN);
};

#endif // BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H
