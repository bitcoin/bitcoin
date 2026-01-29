// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <chain.h>
#include <chainparams.h>
#include <flatfile.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <validation.h>

#include <ranges>
#include <vector>

const TestingSetup* g_setup;

CBlockHeader ConsumeBlockHeader(FuzzedDataProvider& provider, uint256 prev_hash, int& nonce_counter)
{
    CBlockHeader header;
    header.nVersion = provider.ConsumeIntegral<decltype(header.nVersion)>();
    header.hashPrevBlock = prev_hash;
    header.hashMerkleRoot = uint256{}; // never used
    header.nTime = provider.ConsumeIntegral<decltype(header.nTime)>();
    header.nBits = Params().GenesisBlock().nBits; // not fuzzed because not used (validation is mocked).
    header.nNonce = nonce_counter++;              // prevent creating multiple block headers with the same hash
    return header;
}

void initialize_block_index_tree()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(block_index_tree, .init = initialize_block_index_tree)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    auto& chainman = static_cast<TestChainstateManager&>(*g_setup->m_node.chainman);
    auto& blockman = static_cast<TestBlockManager&>(chainman.m_blockman);
    CBlockIndex* genesis = chainman.ActiveChainstate().m_chain[0];
    int nonce_counter = 0;
    std::vector<CBlockIndex*> blocks;
    blocks.push_back(genesis);
    bool abort_run{false};

    std::vector<CBlockIndex*> pruned_blocks;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 1000)
    {
        if (abort_run) break;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // Receive a header building on an existing valid one. This assumes headers are valid, so PoW is not relevant here.
                LOCK(cs_main);
                CBlockIndex* prev_block = PickValue(fuzzed_data_provider, blocks);
                if (!(prev_block->nStatus & BLOCK_FAILED_MASK)) {
                    CBlockHeader header = ConsumeBlockHeader(fuzzed_data_provider, prev_block->GetBlockHash(), nonce_counter);
                    CBlockIndex* index = blockman.AddToBlockIndex(header, chainman.m_best_header);
                    assert(index->nStatus & BLOCK_VALID_TREE);
                    assert(index->pprev == prev_block);
                    blocks.push_back(index);
                }
            },
            [&] {
                // Receive a full block (valid or invalid) for an existing header, but don't attempt to connect it yet
                LOCK(cs_main);
                CBlockIndex* index = PickValue(fuzzed_data_provider, blocks);
                // Must be new to us and not known to be invalid (e.g. because of an invalid ancestor).
                if (index->nTx == 0 && !(index->nStatus & BLOCK_FAILED_MASK)) {
                    if (fuzzed_data_provider.ConsumeBool()) { // Invalid
                        BlockValidationState state;
                        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "consensus-invalid");
                        chainman.InvalidBlockFound(index, state);
                    } else {
                        size_t nTx = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 1000);
                        CBlock block; // Dummy block, so that ReceivedBlockTransactions can infer a nTx value.
                        block.vtx = std::vector<CTransactionRef>(nTx);
                        FlatFilePos pos(0, fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 1000));
                        chainman.ReceivedBlockTransactions(block, index, pos);
                        assert(index->nStatus & BLOCK_VALID_TRANSACTIONS);
                        assert(index->nStatus & BLOCK_HAVE_DATA);
                    }
                }
            },
            [&] {
                // Simplified ActivateBestChain(): Try to move to a chain with more work - with the possibility of finding blocks to be invalid on the way
                LOCK(cs_main);
                auto& chain = chainman.ActiveChain();
                CBlockIndex* old_tip = chain.Tip();
                assert(old_tip);
                do {
                    CBlockIndex* best_tip = chainman.FindMostWorkChain();
                    assert(best_tip);                   // Should at least return current tip
                    if (best_tip == chain.Tip()) break; // Nothing to do
                    // Rewind chain to forking point
                    const CBlockIndex* fork = chain.FindFork(best_tip);
                    // If we can't go back to the fork point due to pruned data, abort this run. In reality, a pruned node would also currently just crash in this scenario.
                    // This is very unlikely to happen due to the minimum pruning threshold of 550MiB.
                    CBlockIndex* it = chain.Tip();
                    while (it && it->nHeight != fork->nHeight) {
                        if (!(it->nStatus & BLOCK_HAVE_UNDO)) {
                            assert(blockman.m_have_pruned);
                            abort_run = true;
                            return;
                        }
                        it = it->pprev;
                    }
                    chain.SetTip(*chain[fork->nHeight]);

                    // Prepare new blocks to connect
                    std::vector<CBlockIndex*> to_connect;
                    it = best_tip;
                    while (it && it->nHeight != fork->nHeight) {
                        to_connect.push_back(it);
                        it = it->pprev;
                    }
                    // Connect blocks, possibly fail
                    for (CBlockIndex* block : to_connect | std::views::reverse) {
                        assert(!(block->nStatus & BLOCK_FAILED_MASK));
                        assert(block->nStatus & BLOCK_HAVE_DATA);
                        if (!block->IsValid(BLOCK_VALID_SCRIPTS)) {
                            if (fuzzed_data_provider.ConsumeBool()) { // Invalid
                                BlockValidationState state;
                                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "consensus-invalid");
                                chainman.InvalidBlockFound(block, state);
                                // This results in duplicate calls to InvalidChainFound, but mirrors the behavior in validation
                                chainman.InvalidChainFound(to_connect.front());
                                break;
                            } else {
                                block->RaiseValidity(BLOCK_VALID_SCRIPTS);
                                block->nStatus |= BLOCK_HAVE_UNDO;
                            }
                        }
                        chain.SetTip(*block);
                        chainman.ActiveChainstate().PruneBlockIndexCandidates();
                        // ActivateBestChainStep may release cs_main / not connect all blocks in one go - but only if we have at least as much chain work as we had at the start.
                        if (block->nChainWork > old_tip->nChainWork && fuzzed_data_provider.ConsumeBool()) {
                            break;
                        }
                    }
                } while (node::CBlockIndexWorkComparator()(chain.Tip(), old_tip));
                assert(chain.Tip()->nChainWork >= old_tip->nChainWork);
            },
            [&] {
                // Prune chain - dealing with block files is beyond the scope of this test, so just prune random blocks, making no assumptions
                // about what blocks are pruned together because they are in the same block file.
                // Also don't prune blocks outside of the chain for now - this would make the fuzzer crash because of the problem described in
                // https://github.com/bitcoin/bitcoin/issues/31512
                LOCK(cs_main);
                auto& chain = chainman.ActiveChain();
                int prune_height = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, chain.Height());
                CBlockIndex* prune_block{chain[prune_height]};
                if (prune_block != chain.Tip() && (prune_block->nStatus & BLOCK_HAVE_DATA)) {
                    blockman.m_have_pruned = true;
                    prune_block->nStatus &= ~BLOCK_HAVE_DATA;
                    prune_block->nStatus &= ~BLOCK_HAVE_UNDO;
                    prune_block->nFile = 0;
                    prune_block->nDataPos = 0;
                    prune_block->nUndoPos = 0;
                    auto range = blockman.m_blocks_unlinked.equal_range(prune_block->pprev);
                    while (range.first != range.second) {
                        std::multimap<CBlockIndex*, CBlockIndex*>::iterator _it = range.first;
                        range.first++;
                        if (_it->second == prune_block) {
                            blockman.m_blocks_unlinked.erase(_it);
                        }
                    }
                    pruned_blocks.push_back(prune_block);
                }
            },
            [&] {
                // Download a previously pruned block
                LOCK(cs_main);
                size_t num_pruned = pruned_blocks.size();
                if (num_pruned == 0) return;
                size_t i = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, num_pruned - 1);
                CBlockIndex* index = pruned_blocks[i];
                assert(!(index->nStatus & BLOCK_HAVE_DATA));
                CBlock block;
                block.vtx = std::vector<CTransactionRef>(index->nTx); // Set the number of tx to the prior value.
                FlatFilePos pos(0, fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 1000));
                chainman.ReceivedBlockTransactions(block, index, pos);
                assert(index->nStatus & BLOCK_VALID_TRANSACTIONS);
                assert(index->nStatus & BLOCK_HAVE_DATA);
                pruned_blocks.erase(pruned_blocks.begin() + i);
            });
    }
    if (!abort_run) {
        chainman.CheckBlockIndex();
    }

    // clean up global state changed by last iteration and prepare for next iteration
    {
        LOCK(cs_main);
        genesis->nStatus |= BLOCK_HAVE_DATA;
        genesis->nStatus |= BLOCK_HAVE_UNDO;
        chainman.m_best_header = genesis;
        chainman.ResetBestInvalid();
        chainman.nBlockSequenceId = 2;
        chainman.ActiveChain().SetTip(*genesis);
        chainman.ActiveChainstate().setBlockIndexCandidates.clear();
        chainman.m_cached_is_ibd = true;
        blockman.m_blocks_unlinked.clear();
        blockman.m_have_pruned = false;
        blockman.CleanupForFuzzing();
        // Delete all blocks but Genesis from block index
        uint256 genesis_hash = genesis->GetBlockHash();
        for (auto it = blockman.m_block_index.begin(); it != blockman.m_block_index.end();) {
            if (it->first != genesis_hash) {
                it = blockman.m_block_index.erase(it);
            } else {
                ++it;
            }
        }
        chainman.ActiveChainstate().TryAddBlockIndexCandidate(genesis);
        assert(blockman.m_block_index.size() == 1);
        assert(chainman.ActiveChainstate().setBlockIndexCandidates.size() == 1);
        assert(chainman.ActiveChain().Height() == 0);
    }
}
