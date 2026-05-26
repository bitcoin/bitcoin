// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H
#define BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H

#include <node/mining_types.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <validationinterface.h>

/**
 * A CValidationInterface subscriber that records the BlockValidationState from
 * each BlockChecked notification. Use ClearCheckedBlockStates() to reset
 * between calls.
 */
class ValidationInterfaceSubscriber : public CValidationInterface
{
public:
    std::vector<BlockValidationState> m_checked_block_states;
    void ClearCheckedBlockStates() { m_checked_block_states.clear(); }

protected:
    void BlockChecked(const std::shared_ptr<const CBlock>&, const BlockValidationState& state) override
    {
        m_checked_block_states.push_back(state);
    }
};

/**
 * ValidationBlockValidityTestingSetup relies on TestChain100Setup, which
 * mines 100 blocks during initialization.
 */
struct ValidationBlockValidityTestingSetup : public TestChain100Setup {
    Chainstate& m_chainstate{m_node.chainman->ActiveChainstate()};
    const Consensus::Params& m_params{m_node.chainman->GetParams().GetConsensus()};
    ValidationInterfaceSubscriber m_subscriber;

    ValidationBlockValidityTestingSetup()
    {
        m_node.validation_signals->RegisterValidationInterface(&m_subscriber);
    }
    ~ValidationBlockValidityTestingSetup()
    {
        m_node.validation_signals->UnregisterValidationInterface(&m_subscriber);
    }
    /** Create a new block template using the provided options. */
    CBlock MakeBlock(node::BlockCreateOptions options = {});
    /** Reset all cached validity flags (fChecked, m_checked_merkle_root, m_checked_witness_commitment). */
    void ResetBlock(CBlock& block);
    /** Reset all cached validity flags then find a valid nonce. Modifies the block in-place. */
    void SolveBlockPoW(CBlock& block);

    /** Verify a block's validity using TestBlockValidity. */
    BlockValidationState TestValidity(CBlock& block, bool check_pow = false, bool check_merkle = true);
    /**
     * Validate a block against the UTXO set via ConnectBlock (fJustCheck=true).
     * Does not modify chain state. Only catches errors in UTXO/script validation.
     */
    BlockValidationState ConnectBlock(CBlock& block);

    /** Helper to add a spendable coin to the UTXO set for testing. */
    COutPoint AddCoin(const CScript& script_pub_key = CScript() << OP_TRUE, CAmount amount = 1 * COIN);
    /**
     * Submit a block via ProcessNewBlock, drain the validation signal queue,
     * and return the single BlockChecked notification state.
     */
    BlockValidationState ProcessNewBlock(CBlock& block, bool force_processing = true, bool min_pow_checked = true);
};

#endif // BITCOIN_TEST_UTIL_BLOCK_VALIDITY_H
