// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/index_sync_util.h>

#include <addresstype.h>
#include <consensus/amount.h>
#include <key.h>
#include <policy/feerate.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/check.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

std::vector<CBlock> ExtendChainWithSpends(TestChain100Setup& test_setup)
{
    // Building the chain is not timed, but it is most of the wall time of a run
    constexpr uint32_t num_blocks{50};
    constexpr uint32_t num_txs_per_block{50};

    // Block filter elements are deduplicated, so only distinct scripts add to a
    // filter. Key pairs are reused across blocks, distinct within one.
    const size_t num_keys{2 * num_txs_per_block};
    std::vector<CKey> keys;
    std::vector<CTxOut> outs;
    keys.reserve(num_keys);
    outs.reserve(num_keys);
    for (size_t i{0}; i < num_keys; ++i) {
        keys.push_back(GenerateRandomKey());
        outs.emplace_back(COIN, GetScriptForDestination(WitnessV0KeyHash{keys.back().GetPubKey()}));
    }

    const CScript coinbase_spk{GetScriptForDestination(WitnessV0KeyHash{test_setup.coinbaseKey.GetPubKey()})};

    // The coinbase of block 1 matures exactly at the first height added here.
    CTransactionRef tx_to_spend{test_setup.m_coinbase_txns[0]};
    std::vector<COutPoint> inputs{{tx_to_spend->GetHash(), 0}};
    std::vector<CKey> signing_keys{test_setup.coinbaseKey};
    size_t output_pair{0};

    std::vector<CBlock> blocks;
    blocks.reserve(num_blocks);
    for (uint32_t b{0}; b < num_blocks; ++b) {
        std::vector<CMutableTransaction> txs;
        txs.reserve(num_txs_per_block);
        for (uint32_t i{0}; i < num_txs_per_block; ++i) {
            auto tx{test_setup.CreateValidTransaction(
                                  /*input_transactions=*/{tx_to_spend},
                                  /*inputs=*/inputs,
                                  /*input_height=*/0, // Height is unused outside the scratch signing view
                                  /*input_signing_keys=*/signing_keys,
                                  /*outputs=*/{outs[2 * output_pair], outs[2 * output_pair + 1]},
                                  /*feerate=*/{},
                                  /*fee_output=*/{})
                        .first};
            tx_to_spend = MakeTransactionRef(tx);
            txs.push_back(std::move(tx));
            inputs = {{tx_to_spend->GetHash(), 0}, {tx_to_spend->GetHash(), 1}};
            signing_keys = {keys[2 * output_pair], keys[2 * output_pair + 1]};
            output_pair = (output_pair + 1) % num_txs_per_block;
        }
        auto& block{blocks.emplace_back(test_setup.CreateBlock(txs, coinbase_spk))};
        Assert(!ProcessBlock(test_setup.m_node, std::make_shared<CBlock>(block)).IsNull());
        // UpdateTime() clamps block times to at least MTP+1, so advance the mock clock like mineBlocks()
        test_setup.m_clock += 1s;
    }
    return blocks;
}
