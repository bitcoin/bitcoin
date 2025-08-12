// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chain.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <node/kernel_notifications.h>
#include <node/mining_types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <txmempool.h>
#include <uint256.h>
#include <validation.h>
#include <validationinterface.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>


namespace {

TestingSetup* g_setup;

/** Vector of blocks to keep references to blocks (to enable fuzzing input to pick one to build upon) */
static std::vector<std::shared_ptr<CBlock>> g_blocks;
/** CTxIns for spending outputs, which can be unspent, already spent, or an immature coinbase. */
static std::vector<CTxIn> g_spend_candidate_txins;
/** Static P2SH_OP_TRUE script */
static const CScript P2SH_OP_TRUE = CScript() << OP_HASH160 << ToByteVector(ScriptHash(CScript() << OP_TRUE)) << OP_EQUAL;
/** Static P2SH_OP_TRUE unlock script */
static const CScript P2SH_OP_TRUE_UNLOCK = CScript() << MakeUCharSpan(CScript() << OP_TRUE);
/** Static TAPROOT_OP_TRUE script and its witness */
static CScript TAPROOT_OP_TRUE;
static std::vector<std::vector<uint8_t>> TAPROOT_OP_TRUE_WITNESS;

/**
 * Initialize TAPROOT_OP_TRUE and TAPROOT_OP_TRUE_WITNESS static variables.
 */
static void InitTaprootScript()
{
    uint256 merkle_tree_hash = ComputeTapleafHash(TAPROOT_LEAF_TAPSCRIPT, MakeUCharSpan(CScript() << OP_TRUE));
    uint256 internal_key{std::vector<uint8_t>(32, 1)};
    auto res = XOnlyPubKey(internal_key).CreateTapTweak(&merkle_tree_hash);
    Assert(res.has_value());
    auto control = ToByteVector(internal_key);
    control.insert(control.begin(), TAPROOT_LEAF_TAPSCRIPT | (res->second ? 1 : 0));

    TAPROOT_OP_TRUE = CScript() << OP_1 << ToByteVector(res->first);
    TAPROOT_OP_TRUE_WITNESS.clear();
    TAPROOT_OP_TRUE_WITNESS.emplace_back(ToByteVector(CScript() << OP_TRUE));
    TAPROOT_OP_TRUE_WITNESS.emplace_back(std::move(control));
}

/**
 * Given a transaction and an output index, create a CTxIn that can be
 * used to spend it.
 */
static CTxIn GetSpendingScript(const CTransaction& tx, uint32_t vout_index)
{
    Assert(vout_index < tx.vout.size());
    const CTxOut& output = tx.vout[vout_index];

    CTxIn res{COutPoint(tx.GetHash(), vout_index)};
    if (output.scriptPubKey == P2WSH_OP_TRUE) {
        res.scriptSig = CScript();
        res.scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    } else if (output.scriptPubKey == P2SH_OP_TRUE) {
        res.scriptSig = P2SH_OP_TRUE_UNLOCK;
    } else if (output.scriptPubKey == CScript()) {
        res.scriptSig = CScript() << OP_TRUE;
    } else if (output.scriptPubKey == TAPROOT_OP_TRUE) {
        res.scriptSig = CScript();
        res.scriptWitness.stack = TAPROOT_OP_TRUE_WITNESS;
    }

    return res;
}

/**
 * Add a spend candidate CTxIn unless the output is unspendable.
 */
static void MaybeAddSpendCandidate(std::vector<CTxIn>& pool, const CTransaction& tx, uint32_t vout_index)
{
    Assert(vout_index < tx.vout.size());
    if (tx.vout[vout_index].scriptPubKey.IsUnspendable()) return;
    pool.push_back(GetSpendingScript(tx, vout_index));
}


/**
 * Read the block from the BlockManager and add it to g_blocks.
 */
static void LoadCurrentBlock(Chainstate& chainstate, CBlockIndex* current_block)
{
    // Read the block from the BlockManager.
    Assert(current_block->nHeight >= 0);
    // Resize g_blocks if needed.
    if (g_blocks.size() <= (size_t)current_block->nHeight) {
        g_blocks.resize(current_block->nHeight + 1);
    }

    g_blocks[current_block->nHeight] = std::make_shared<CBlock>();
    Assert(chainstate.m_blockman.ReadBlock(*g_blocks[current_block->nHeight], *current_block));

    // Iterate all transaction outputs.
    for (const auto& tx : g_blocks[current_block->nHeight]->vtx) {
        for (uint32_t vout_index{0}; vout_index < tx->vout.size(); ++vout_index) {
            MaybeAddSpendCandidate(g_spend_candidate_txins, *tx, vout_index);
        }
    }
}

/**
 * Read the Chainstate object into g_blocks.
 * Then fill g_spend_candidate_txins with inputs that can be tried by the target.
 */
static void LoadCurrentChain()
{
    // Clear existing data.
    g_blocks.clear();
    g_spend_candidate_txins.clear();

    {
        LOCK(::cs_main);
        // Retrieve the current chainstate.
        auto& chainstate = Assert(g_setup->m_node.chainman)->ActiveChainstate();
        // Make sure it contains a valid mempool.
        Assert(chainstate.GetMempool());

        // Traverse the chain from tip to genesis.
        auto current_block = chainstate.m_chain.Tip();

        while (current_block != nullptr) {
            LoadCurrentBlock(chainstate, current_block);
            // Move to previous block.
            current_block = current_block->pprev;
        }
    }

    // Reverse the order of g_spend_candidate_txins to have them in ascending order of
    // block height.
    std::reverse(g_spend_candidate_txins.begin(), g_spend_candidate_txins.end());
}


/**
 * Reset the chainman in the testing setup object.
 * Mine 2*COINBASE_MATURITY blocks to have spendable UTXOs.
 * It is called once in the initialization function.
 */
void ResetChainman(TestingSetup& setup)
{
    SetMockTime(setup.m_node.chainman->GetParams().GenesisBlock().Time());
    setup.m_node.chainman.reset();
    setup.m_node.notifications->m_shutdown_on_fatal_error = false;
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();

    for (int i = 0; i < 2 * COINBASE_MATURITY; i++) {
        node::BlockCreateOptions options;
        options.coinbase_output_script = P2WSH_OP_TRUE;
        MineBlock(setup.m_node, options);
    }
    setup.m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

/** Create additional transactions in the mempool that spend
 * coins from mature blocks. Otherwise the mined chain only contains
 * coinbase transactions.
 */
void AddExtraTxsToMempool(TestingSetup& setup)
{
    Assert(setup.m_node.chainman->ActiveChainstate().GetMempool()->size() == 0);
    for (size_t i = 1; i <= 10; i++) {
        CMutableTransaction ctx;
        ctx.version = CTransaction::CURRENT_VERSION;
        ctx.vin.resize(1);
        // CTxIn is spendable as g_spend_candidate_txins comes from early blocks whose
        // coinbases are mature.
        ctx.vin[0] = g_spend_candidate_txins[i];
        ctx.vout.resize(4);
        // Arbitrarily create various outputs of different kinds in the same tx.
        // P2WSH
        ctx.vout[0].nValue = CAmount(15 * COIN);
        ctx.vout[0].scriptPubKey = P2WSH_OP_TRUE;
        // P2SH
        ctx.vout[1].nValue = CAmount(15 * COIN);
        ctx.vout[1].scriptPubKey = P2SH_OP_TRUE;
        // Taproot
        ctx.vout[2].nValue = CAmount(10 * COIN);
        ctx.vout[2].scriptPubKey = TAPROOT_OP_TRUE;
        // Empty script
        ctx.vout[3].nValue = CAmount(10 * COIN);
        ctx.vout[3].scriptPubKey = CScript();

        LOCK(::cs_main);
        // Add transaction to the mempool.
        const MempoolAcceptResult ctx_result = setup.m_node.chainman->ProcessTransaction(MakeTransactionRef(ctx));
        Assert(ctx_result.m_result_type == MempoolAcceptResult::ResultType::VALID);

        Assert(setup.m_node.chainman->ActiveChainstate().GetMempool()->size() == i);
        // Force the mempool to select this transaction even though its fee is zero.
        setup.m_node.chainman->ActiveChainstate().GetMempool()->PrioritiseTransaction(ctx.GetHash(), COIN);
    }
}

/** Initialize the chain for this target. */
static void initialize_connect_block()
{
    // Instantiate REGTEST chain.
    static auto testing_setup = MakeNoLogFileContext<TestingSetup>(
        /*chain_type=*/ChainType::REGTEST, TestOpts{
                                               .extra_args = {
                                                   "-minrelaytxfee=0",
                                                   "-acceptnonstdtxn",
                                               },
                                           });
    g_setup = testing_setup.get();

    // Reset the chainman in the testing setup object.
    ResetChainman(*g_setup);

    // Initialize Taproot script declared as static variables.
    InitTaprootScript();

    // Load the chain mined in ResetChainman in global variables g_blocks and
    // g_spend_candidate_txins, to make them available to pick by the target.
    LoadCurrentChain();

    // Prepare multiple transactions for block 201. They spend coins
    // from various coinbases that are now mature enough.
    AddExtraTxsToMempool(*g_setup);
    // Mine block 201, which contains the transactions added to the mempool.
    node::BlockCreateOptions options;
    options.coinbase_output_script = P2WSH_OP_TRUE;
    MineBlock(g_setup->m_node, options);
    Assert(g_setup->m_node.chainman->ActiveChainstate().GetMempool()->size() == 0);

    // Load the 201st block into g_blocks.
    LOCK(::cs_main);
    auto& chainstate = Assert(g_setup->m_node.chainman)->ActiveChainstate();
    auto current_block = chainstate.m_chain.Tip();
    LoadCurrentBlock(chainstate, current_block);
}

/**
 * Read one transaction from the fuzzing input through the FuzzedDataProvider.
 * It is intended to leave more space to craft complex transactions, especially
 * with various script types (P2SH, P2WSH, TAPROOT, NOSCRIPT).
 * It is exclusively used by ConsumeBlock to read transactions inside a block.
 */
CTransactionRef ConsumeTransaction(FuzzedDataProvider& fuzzed_data_provider,
                                   std::vector<CTxIn>& additional_txins,
                                   bool coinbase = false,
                                   int target_height = 0)
{
    CMutableTransaction tx;
    tx.version = fuzzed_data_provider.ConsumeBool() ?
                     CTransaction::CURRENT_VERSION :
                     fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    tx.nLockTime = fuzzed_data_provider.ConsumeBool() ?
                       0 :
                       fuzzed_data_provider.ConsumeIntegral<uint32_t>();

    // Some harnesses want to explicitly read coinbase transactions from input.
    if (coinbase) {
        // vin size is hardcoded.
        tx.vin.resize(1);
        tx.vin[0].prevout.SetNull();
        if (fuzzed_data_provider.ConsumeBool()) {
            // 1/2 probability of a valid vin.
            tx.vin[0].scriptSig = CScript() << target_height;
        } else {
            // Read arbitrary data from input as scriptSig.
            auto script_sig = ConsumeRandomLengthByteVector<unsigned char>(fuzzed_data_provider, 100);
            tx.vin[0].scriptSig.assign(script_sig.begin(), script_sig.end());
        }
    } else {
        // Read a normal transaction, with up to 10 inputs.
        int num_inputs = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10);
        tx.vin.resize(num_inputs);
        for (int i = 0; i < num_inputs; i++) {
            // Read an integer to choose a CTxIn or reuse one generated by the
            // input. The content of the CTxIn is not read from the input per se.
            uint32_t input_index = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, g_spend_candidate_txins.size() + additional_txins.size() - 1);
            if (input_index < g_spend_candidate_txins.size()) {
                // Pick it from the spend candidates.
                tx.vin[i] = g_spend_candidate_txins[input_index];
            } else {
                // Pick it in the additional_txins set.
                Assert((input_index - g_spend_candidate_txins.size()) < additional_txins.size());
                tx.vin[i] = additional_txins[input_index - g_spend_candidate_txins.size()];
            }

            // Enable the fuzzer to mutate every CTxIn field after it is taken
            // from the spend candidates.
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].nSequence = ConsumeSequence(fuzzed_data_provider);
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].prevout.n = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].prevout.hash = Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider));
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].scriptSig = ConsumeScript(fuzzed_data_provider);
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].scriptWitness.stack.clear();
                int num_wit = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10);
                for (int j = 0; j < num_wit; j++) {
                    tx.vin[i].scriptWitness.stack.push_back(ConsumeRandomLengthByteVector<unsigned char>(fuzzed_data_provider, 100));
                }
            }
        }
    }

    // Read outputs.
    int num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 10);
    tx.vout.resize(num_outputs);
    for (int i = 0; i < num_outputs; i++) {
        // Read CAmount to spend.
        tx.vout[i].nValue = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-10, 50 * COIN + 10);

        // Read scriptPubKey type into one of the valid types.
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // P2WSH
                tx.vout[i].scriptPubKey = P2WSH_OP_TRUE;
            },
            [&] {
                // P2SH
                tx.vout[i].scriptPubKey = P2SH_OP_TRUE;
            },
            [&] {
                // Taproot
                tx.vout[i].scriptPubKey = TAPROOT_OP_TRUE;
            },
            [&] {
                // Empty script
                tx.vout[i].scriptPubKey = CScript();
            },
            [&] {
                // Read arbitrary scriptPubKey.
                tx.vout[i].scriptPubKey = ConsumeScript(fuzzed_data_provider);
            });
    }

    // Create the shared pointer to the CTransaction object.
    auto res = MakeTransactionRef(tx);

    if (!coinbase) {
        // Create spending scripts for CTxOuts so they can be spent in later
        // transactions. Do it here as the transaction hash is definitive.
        for (int i = 0; i < num_outputs; i++) {
            MaybeAddSpendCandidate(additional_txins, *res, i);
        }
    }

    return res;
}

/**
 * Consume a block from the fuzzing input.
 * It builds a block on top of the given prev_block.
 */
CBlock ConsumeBlock(FuzzedDataProvider& fuzzed_data_provider, const CBlock& prev_block, int target_height,
                    std::vector<CTxIn>& additional_txins)
{
    CBlock block;

    // Initialize header fields.
    block.nVersion = g_blocks.back()->nVersion;
    block.hashPrevBlock = prev_block.GetHash();
    block.nTime = g_blocks.back()->nTime + 2;
    block.nBits = g_blocks.back()->nBits;

    // Give the fuzzer input the ability to mutate block header fields.
    if (fuzzed_data_provider.ConsumeBool()) {
        block.nVersion = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        block.hashPrevBlock = ConsumeUInt256(fuzzed_data_provider);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        block.nTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        block.nBits = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    }

    // Read the coinbase transaction from the input.
    block.vtx.push_back(ConsumeTransaction(fuzzed_data_provider, additional_txins, true, target_height));

    // Read up to num_tx transactions from the input.
    int num_tx = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 5);
    for (int i = 0; i < num_tx; i++) {
        block.vtx.push_back(ConsumeTransaction(fuzzed_data_provider, additional_txins));
    }

    // Commit witness.
    if (fuzzed_data_provider.ConsumeBool()) {
        g_setup->m_node.chainman->GenerateCoinbaseCommitment(block, nullptr);
    }

    // Set hashMerkleRoot to expected value.
    block.hashMerkleRoot = BlockMerkleRoot(block);
    // Let the fuzzer mutate hashMerkleRoot.
    if (fuzzed_data_provider.ConsumeBool()) {
        block.hashMerkleRoot = ConsumeUInt256(fuzzed_data_provider);
    }

    // Read the nonce from the input.
    block.nNonce = fuzzed_data_provider.ConsumeIntegral<uint32_t>();

    return block;
}


FUZZ_TARGET(connect_block, .init = initialize_connect_block)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{g_blocks.back()->Time() + 2s};

    LOCK(::cs_main);
    g_setup->m_node.chainman->m_validation_cache.m_script_execution_cache.TestOnlyReset();
    Chainstate& active_chainstate = g_setup->m_node.chainman->ActiveChainstate();
    CBlockIndex* active_tip = active_chainstate.m_chain.Tip();
    Assert(active_tip->GetBlockHash() == g_blocks.back()->GetHash());
    CCoinsViewCache active_coins(&active_chainstate.CoinsTip());

    // Read a new block from the data provider.
    std::vector<CTxIn> additional_txins;
    CBlock block = ConsumeBlock(fuzzed_data_provider, *g_blocks.back(), active_tip->nHeight + 1, additional_txins);

    // Duplicate a transaction (not the coinbase) from the previous block
    // to hit the BIP30 check.
    if (fuzzed_data_provider.ConsumeBool()) {
        const auto& duplicates = g_blocks.back()->vtx;
        block.vtx.push_back(duplicates[fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, duplicates.size() - 1)]);
    }

    // Compute new CBlockIndex object.
    uint256 current_hash = block.GetHash();
    CBlockIndex new_index(block);
    new_index.pprev = active_tip;
    new_index.nHeight = active_tip->nHeight + 1;
    new_index.phashBlock = &current_hash;

    // Try to connect the block.
    BlockValidationState state;
    bool connected = active_chainstate.ConnectBlock(block,
                                                    state,
                                                    &new_index,
                                                    active_coins,
                                                    /*fJustCheck=*/true);
    Assert(connected == state.IsValid());
}

} // namespace
