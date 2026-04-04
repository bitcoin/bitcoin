// Copyright (c) present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <validation.h>


static TestChain100Setup* g_setup;

void initialize_validation_block_validity()
{
    static auto setup = MakeNoLogFileContext<TestChain100Setup>();
    g_setup = setup.get();
}

namespace {
CScript ConsumeVarietyScript(FuzzedDataProvider& fuzzed_data_provider)
{
    CScript script;
    CallOneOf(
        fuzzed_data_provider,
        [&] { script << OP_TRUE; },
        [&] {
            // P2WSH(OP_TRUE)
            script = GetScriptForDestination(WitnessV0ScriptHash(CScript() << OP_TRUE));
        },
        [&] {
            // P2SH(OP_TRUE)
            script = GetScriptForDestination(ScriptHash(CScript() << OP_TRUE));
        },
        [&] {
            // P2TR output (OP_1 + 32-byte program)
            const std::vector<unsigned char> bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(32);
            if (bytes.size() == 32) {
                script << OP_1 << bytes;
            } else {
                script << OP_TRUE;
            }
        },
        [&] {
            const std::vector<unsigned char> bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(10);
            script << bytes;
        });
    return script;
}

CBlock MakeBlock(const node::NodeContext& node, FuzzedDataProvider& fuzzed_data_provider)
{
    const Chainstate& chainstate = node.chainman->ActiveChainstate();
    const CBlockIndex* tip = chainstate.m_chain.Tip();
    assert(tip);
    if (fuzzed_data_provider.ConsumeBool()) {
        CBlock block;
        block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
        block.hashPrevBlock = *tip->phashBlock;
        block.nTime = tip->GetBlockTime() + 1;
        block.nBits = tip->nBits;
        block.nNonce = 0;
        CMutableTransaction coinbase_tx;
        coinbase_tx.vin.resize(1);
        // BIP34: block height must be the first item in the coinbase scriptSig.
        coinbase_tx.vin[0].scriptSig = CScript() << (tip->nHeight + 1) << OP_0;
        coinbase_tx.vout.resize(1);
        coinbase_tx.vout[0].nValue = GetBlockSubsidy(tip->nHeight + 1, node.chainman->GetParams().GetConsensus());
        coinbase_tx.vout[0].scriptPubKey = ConsumeVarietyScript(fuzzed_data_provider);
        block.vtx.push_back(MakeTransactionRef(std::move(coinbase_tx)));
        block.hashMerkleRoot = BlockMerkleRoot(block);
        return block;
    } else {
        node::BlockAssembler::Options assembler_options;
        assembler_options.coinbase_output_script = CScript() << OP_TRUE;
        assembler_options.include_dummy_extranonce = true;
        if (node.args) {
            node::ApplyArgsManOptions(*node.args, assembler_options);
        }
        auto block_ptr = PrepareBlock(node, assembler_options);
        assert(block_ptr);
        return *block_ptr;
    }
}

void MutateHeader(CBlock& block, FuzzedDataProvider& fuzzed_data_provider)
{
    if (fuzzed_data_provider.remaining_bytes() == 0) return;
    CallOneOf(
        fuzzed_data_provider,
        [&] { block.nVersion = fuzzed_data_provider.ConsumeIntegral<int32_t>(); },
        [&] { block.nTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>(); },
        [&] { block.nBits = fuzzed_data_provider.ConsumeIntegral<uint32_t>(); },
        [&] {
            // Force nonce to 0 or fuzz it.
            block.nNonce = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        },
        [&] { block.hashPrevBlock = ConsumeUInt256(fuzzed_data_provider); });
}

void MutateTransactionsStateless(CBlock& block, FuzzedDataProvider& fuzzed_data_provider)
{
    if (fuzzed_data_provider.remaining_bytes() == 0) return;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 5)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // Duplicate a random transaction.
                if (!block.vtx.empty()) {
                    const size_t tx_index = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, block.vtx.size() - 1);
                    block.vtx.push_back(block.vtx[tx_index]);
                }
            },
            [&] {
                // Swap two random transactions.
                if (block.vtx.size() >= 2) {
                    const size_t tx_index1 = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, block.vtx.size() - 1);
                    const size_t tx_index2 = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, block.vtx.size() - 1);
                    std::swap(block.vtx[tx_index1], block.vtx[tx_index2]);
                }
            },
            [&] {
                block.vtx.clear();
            },
            [&] {
                // Insert a transaction with fuzz-derived inputs and outputs at a random position.
                if (!block.vtx.empty()) {
                    CMutableTransaction malformed_tx;
                    malformed_tx.vin.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 5));
                    for (auto& vin : malformed_tx.vin) {
                        vin.prevout.hash = Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider));
                        vin.prevout.n = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 10);
                    }
                    malformed_tx.vout.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 5));
                    for (auto& vout : malformed_tx.vout) {
                        vout.nValue = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY + 1);
                        vout.scriptPubKey = ConsumeVarietyScript(fuzzed_data_provider);
                    }
                    const size_t insertion_pos = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, block.vtx.size() - 1);
                    block.vtx.insert(block.vtx.begin() + insertion_pos, MakeTransactionRef(std::move(malformed_tx)));
                }
            },
            [&] {
                // Remove a random transaction.
                if (!block.vtx.empty()) {
                    const size_t tx_index = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, block.vtx.size() - 1);
                    block.vtx.erase(block.vtx.begin() + tx_index);
                }
            });
    }
}

void MutateTransactionsContextual(CBlock& block, FuzzedDataProvider& fuzzed_data_provider)
{
    if (fuzzed_data_provider.remaining_bytes() == 0) return;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 5)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // Append a signed transaction spending a randomly selected matured coinbase output.
                if (g_setup->m_coinbase_txns.empty()) return;
                const int next_height = WITH_LOCK(g_setup->m_node.chainman->GetMutex(),
                                                  return g_setup->m_node.chainman->ActiveChain().Height()) +
                                        1;
                if (next_height <= COINBASE_MATURITY) return;
                const size_t n_matured = static_cast<size_t>(next_height - COINBASE_MATURITY);
                if (n_matured > g_setup->m_coinbase_txns.size()) return;
                const size_t coinbase_idx = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, n_matured - 1);
                const auto& matured_coinbase_tx = g_setup->m_coinbase_txns[coinbase_idx];
                // vout[0] must exist and have enough value for at least a 1-sat output after a fuzzed fee.
                if (matured_coinbase_tx->vout.empty() || matured_coinbase_tx->vout[0].nValue <= 1000) return;
                const CAmount fee = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1000, matured_coinbase_tx->vout[0].nValue - 1);
                auto [valid_tx, actual_fee] = g_setup->CreateValidTransaction(
                    /*input_transactions=*/{matured_coinbase_tx},
                    /*inputs=*/{COutPoint(matured_coinbase_tx->GetHash(), 0)},
                    /*input_height=*/static_cast<int>(coinbase_idx + 1),
                    /*input_signing_keys=*/{g_setup->coinbaseKey},
                    /*outputs=*/{CTxOut(matured_coinbase_tx->vout[0].nValue - fee, CScript() << OP_TRUE)},
                    /*feerate=*/std::nullopt,
                    /*fee_output=*/std::nullopt);
                block.vtx.push_back(MakeTransactionRef(std::move(valid_tx)));
            },
            [&] {
                // Mutate the coinbase (vtx[0]).
                if (block.vtx.empty() || block.vtx[0]->vin.empty()) return;
                CMutableTransaction mutable_coinbase_tx(*block.vtx[0]);
                CallOneOf(
                    fuzzed_data_provider,
                    [&] {
                        // Push the fuzzed block height.
                        mutable_coinbase_tx.vin[0].scriptSig = CScript() << fuzzed_data_provider.ConsumeIntegral<int>();
                    },
                    [&] {
                        // Exceed the 100-byte coinbase scriptSig consensus limit.
                        const size_t script_len = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(101, 128);
                        const std::vector<unsigned char> bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(script_len);
                        mutable_coinbase_tx.vin[0].scriptSig = CScript{bytes.begin(), bytes.end()};
                    },
                    [&] {
                        // Give the coinbase multiple outputs.
                        mutable_coinbase_tx.vout.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(2, 5));
                    });
                block.vtx[0] = MakeTransactionRef(std::move(mutable_coinbase_tx));
            });
    }
}

} // namespace

FUZZ_TARGET(validation_block_validity, .init = initialize_validation_block_validity)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SteadyClockContext steady_ctx{};
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(),
                          return g_setup->m_node.chainman->ActiveTip()->Time()));
    Chainstate& chainstate = g_setup->m_node.chainman->ActiveChainstate();
    CBlock block;
    {
        LOCK(cs_main);
        block = MakeBlock(g_setup->m_node, fuzzed_data_provider);
    }
    MutateHeader(block, fuzzed_data_provider);
    MutateTransactionsContextual(block, fuzzed_data_provider);
    MutateTransactionsStateless(block, fuzzed_data_provider);
    if (fuzzed_data_provider.ConsumeBool()) {
        block.hashMerkleRoot = ConsumeUInt256(fuzzed_data_provider);
    } else {
        block.hashMerkleRoot = BlockMerkleRoot(block);
    }
    const bool check_pow = fuzzed_data_provider.ConsumeBool();
    const bool check_merkle = fuzzed_data_provider.ConsumeBool();
    LOCK(cs_main);
    const int height_before = chainstate.m_chain.Height();
    const BlockValidationState state = TestBlockValidity(chainstate, block, check_pow, check_merkle);
    // TestBlockValidity must not alter chain state.
    assert(chainstate.m_chain.Height() == height_before);
    // Exactly one of IsValid/IsInvalid/IsError must be set.
    assert(state.IsValid() + state.IsInvalid() + state.IsError() == 1);
}
