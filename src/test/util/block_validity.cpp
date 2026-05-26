// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/block_validity.h>

#include <pow.h>
#include <test/util/mining.h>
#include <util/check.h>

#include <boost/test/unit_test.hpp>

CBlock ValidationBlockValidityTestingSetup::MakeBlock(node::BlockCreateOptions options)
{
    auto block_ptr = PrepareBlock(m_node, options);
    Assert(block_ptr);
    return *block_ptr;
}

void ValidationBlockValidityTestingSetup::ResetBlock(CBlock& block)
{
    block.fChecked = block.m_checked_merkle_root = block.m_checked_witness_commitment = false;
}

BlockValidationState ValidationBlockValidityTestingSetup::TestValidity(CBlock& block, bool check_pow, bool check_merkle)
{
    ResetBlock(block);
    LOCK(cs_main);
    return TestBlockValidity(m_chainstate, block, check_pow, check_merkle);
}

COutPoint ValidationBlockValidityTestingSetup::AddCoin(const CScript& script_pub_key, CAmount amount)
{
    // Max trials to find a unique random outpoint
    const int max_trials{5};
    for (int trial_idx = 0; trial_idx < max_trials; ++trial_idx) {
        COutPoint outpoint{Txid::FromUint256(m_rng.rand256()), 0};
        LOCK(cs_main);
        if (m_chainstate.CoinsTip().AccessCoin(outpoint).IsSpent()) {
            m_chainstate.CoinsTip().AddCoin(outpoint, Coin(CTxOut(amount, script_pub_key), 1, false), true);
            return outpoint;
        }
    }
    BOOST_REQUIRE_MESSAGE(false, "AddCoin: failed to find an unused outpoint");
    return {};
}

void ValidationBlockValidityTestingSetup::SolveBlockPoW(CBlock& block)
{
    ResetBlock(block);
    const uint32_t start_nonce{block.nNonce};
    while (!CheckProofOfWork(block.GetHash(), block.nBits, m_params)) {
        ++block.nNonce;
        BOOST_REQUIRE_MESSAGE(block.nNonce != start_nonce, "SolvePoW: exhausted all nonces without finding valid PoW");
    }
}

BlockValidationState ValidationBlockValidityTestingSetup::ProcessNewBlock(CBlock& block, bool force_processing, bool min_pow_checked)
{
    ResetBlock(block);
    bool new_block{false};
    m_node.chainman->ProcessNewBlock(std::make_shared<const CBlock>(block), force_processing, min_pow_checked, &new_block);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    Assert(m_subscriber.m_checked_block_states.size() == 1);
    BlockValidationState state{m_subscriber.m_checked_block_states.front()};
    m_subscriber.ClearCheckedBlockStates();
    return state;
}
