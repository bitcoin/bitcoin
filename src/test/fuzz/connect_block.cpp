// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <consensus/merkle.h>
#include <kernel/disconnected_transactions.h>
#include <node/kernel_notifications.h>
#include <pow.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <validationinterface.h>
#include <validation.h>


#define DEBUGOUTPUT(x) //(x);


namespace {


/** Testing setup used by the three harnesses */
TestingSetup* test_setup{nullptr};
/** Vector of blocks to keep a references on blocks (to enable fuzzing input to pick one to build upon)  */
static std::vector<std::shared_ptr<CBlock>> listBlocks;
/** Set of block hashes in listBlocks */
static std::set<uint256> existingBlockHash;
/** Spending script for all UTXOs (excluding OP_RETURN) including not mature CoinBase and already spend one */
static std::vector<CTxIn> allUTXOCTxIn;
/** Static P2SH_OP_TRUE script */
static const CScript P2SH_OP_TRUE = CScript() << OP_HASH160 << ToByteVector(ScriptHash(CScript() << OP_TRUE)) << OP_EQUAL;
/** Static P2SH_OP_TRUE unlock script (used in getResolveUTXO) */
static const CScript P2SH_OP_TRUE_UNLOCK = CScript() << MakeUCharSpan(CScript() << OP_TRUE);
/** Static TAPROOT_OP_TRUE script and its witness */
static CScript TAPROOT_OP_TRUE;
static std::vector<std::vector<uint8_t>> TAPROOT_OP_TRUE_WITNESS;

/**
 * Initialize TAPROOT_OP_TRUE and TAPROOT_OP_TRUE_WITNESS static variable
 */
static void init_taproot_script() {
    uint256 merkleTreeHash = ComputeTapleafHash(0xc0, MakeUCharSpan(CScript() << OP_TRUE));
    uint256 internalKey {std::vector<uint8_t>(32, 1)};
    auto res = XOnlyPubKey(internalKey).CreateTapTweak(&merkleTreeHash);
    Assert(res.has_value());
    auto control = ToByteVector(internalKey);
    control.insert(control.begin(), 0xc0 | (res->second?1:0));

    TAPROOT_OP_TRUE = CScript() << OP_1 << ToByteVector(res->first);
    TAPROOT_OP_TRUE_WITNESS.clear();
    TAPROOT_OP_TRUE_WITNESS.emplace_back(ToByteVector(CScript() << OP_TRUE));
    TAPROOT_OP_TRUE_WITNESS.emplace_back(std::move(control));
}
/**
 * Given a transaction and an output index, create a CTxIn that can be used to
 * spend it (if possible).
 */
static CTxIn getSpendingScript(const CTransaction& tx, unsigned voutIndex) {
    Assert(voutIndex < tx.vout.size());
    const CTxOut& output = tx.vout[voutIndex];

    CTxIn res {COutPoint(tx.GetHash(), voutIndex)};
    if (output.scriptPubKey.size() >= 1 && output.scriptPubKey[0] == OP_RETURN)
        return res;

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
 * Read the block from the BlockManager and add it to listBlocks and existingBlockHash.
 */
static void loadCurrentBlock(Chainstate& CState, CBlockIndex* currentBlock) {
    /** Read the block from the BlockManager */
    Assert(currentBlock->nHeight >= 0);
    /** resize listBlocks vector if needed */
    if (listBlocks.size() <= (size_t) currentBlock->nHeight) {
        listBlocks.resize(currentBlock->nHeight + 1);
    }

    listBlocks[currentBlock->nHeight] = std::make_shared<CBlock>();
    Assert(CState.m_blockman.ReadBlock(*listBlocks[currentBlock->nHeight], *currentBlock));
    /** Update hash set */
    existingBlockHash.insert(listBlocks[currentBlock->nHeight]->GetHash());

    /** Iterate all transactions outputs */
    for (const auto& tx: listBlocks[currentBlock->nHeight]->vtx) {
        for (unsigned voutIndex = 0; voutIndex < tx->vout.size(); voutIndex++) {
            /** Get the CTxOut output object  */
            auto& vout = tx->vout[voutIndex];
            /** Do not keep OP_RETURN outputs as they are not spendable */
            if (vout.scriptPubKey.size() >= 1 && vout.scriptPubKey[0] == OP_RETURN) continue;
            /** Create the CTxIn that can be used to spend this output */
            DEBUGOUTPUT(std::cout << "Adding UTXO from block " << currentBlock->nHeight << " with txid " << tx->GetHash().ToString() << " and vout " << voutIndex << std::endl);
            allUTXOCTxIn.push_back(getSpendingScript(*tx, voutIndex));
        }
    }
}

/**
 * Read the ChainState object into listBlocks vectors.
 * Then fill the allUTXOCTxIn set to keep track of all UTXO available in the chain.
 */
static void loadCurrentChain() {
    /** Clear existing data */
    listBlocks.clear();
    existingBlockHash.clear();
    allUTXOCTxIn.clear();

    {
        LOCK(::cs_main);
        /** Retrieve the current chain (Chainstate object) */
        auto& CState = Assert(test_setup->m_node.chainman)->ActiveChainstate();
        /** Make sure it contains a valid mempool */
        Assert(CState.GetMempool());

        /** Traverse the chain from tip to genesis */
        auto currentBlock = CState.m_chain.Tip();

        while (currentBlock != nullptr) {
            loadCurrentBlock(CState, currentBlock);
            /** Move to previous block */
            currentBlock = currentBlock->pprev;
        }
    }

    /** Reverse the order of allUTXOCTxIn to have them in
     * ascending order of block height */
    std::reverse(allUTXOCTxIn.begin(), allUTXOCTxIn.end());
}



/**
 * Reset the chainman in the testing setup object.
 * Mine 2*COINBASE_MATURITY blocks to have spendable UTXOs.
 * It is called once in the initialization function;
 */
void ResetChainman(TestingSetup& setup)
{
    SetMockTime(setup.m_node.chainman->GetParams().GenesisBlock().Time());
    setup.m_node.chainman.reset();
    setup.m_node.notifications->m_shutdown_on_fatal_error = false;
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();

    for (int i = 0; i < 2 * COINBASE_MATURITY; i++) {
        node::BlockAssembler::Options options;
        options.coinbase_output_script = P2WSH_OP_TRUE;
        options.include_dummy_extranonce = true;
        MineBlock(setup.m_node, options);
    }
    setup.m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

/** Create additional transaction in the mempool that are spending
 * coins from mature blocks. Otherwise the mined chain only contains
 * coinbase transactions.
 */
void addExtraTxsInMempool(TestingSetup& setup)
{
    Assert(Assert(Assert(setup.m_node.chainman)->ActiveChainstate().GetMempool())->size() == 0);
    for (unsigned i = 1; i < 11; i++) {
        CMutableTransaction ctx;
        ctx.version = CTransaction::CURRENT_VERSION;
        ctx.vin.resize(1);
        /** CTxIn is sure to be spendable as allUTXOCTxIn comes from listBlocks which first TXs are mature */
        ctx.vin[0] = allUTXOCTxIn[i];
        ctx.vout.resize(4);
        /** Arbitrarily creates various outputs of different kinds in the same Tx */
        // P2WSH
        ctx.vout[0].nValue = CAmount(15 * COIN);
        ctx.vout[0].scriptPubKey = P2WSH_OP_TRUE;
        // P2SH
        ctx.vout[1].nValue = CAmount(15 * COIN);
        ctx.vout[1].scriptPubKey = P2SH_OP_TRUE;
        // TAPSCRIPT
        ctx.vout[2].nValue = CAmount(10 * COIN);
        ctx.vout[2].scriptPubKey = TAPROOT_OP_TRUE;
        // NoScript
        ctx.vout[3].nValue = CAmount(10 * COIN);
        ctx.vout[3].scriptPubKey = CScript();

        LOCK(::cs_main);
        // Add transaction in the mempool
        const MempoolAcceptResult ctx_result = setup.m_node.chainman->ProcessTransaction(MakeTransactionRef(ctx));
        if (ctx_result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
            DEBUGOUTPUT(std::cout << "Transaction rejected : " << ctx_result.m_state.GetRejectReason() << std::endl);
        }
        Assert(ctx_result.m_result_type == MempoolAcceptResult::ResultType::VALID);

        Assert(setup.m_node.chainman->ActiveChainstate().GetMempool()->size() == i);
        // Force the mempool to select this transaction (even if fees == 0)
        setup.m_node.chainman->ActiveChainstate().GetMempool()->PrioritiseTransaction(ctx.GetHash(), COIN);
    }
}

/**
 * Initialize the chain for the three harnesses.
 */
static void initialize_connect_block() {
    Assert(EnableFuzzDeterminism());

    /** Instantiate REGTEST chain */
    static auto testing_setup = MakeNoLogFileContext<TestingSetup>(
            /*chain_type=*/ChainType::REGTEST, TestOpts{
                .extra_args = {
                    "-minrelaytxfee=0",
                    "-acceptnonstdtxn",
                },
            });
    test_setup = testing_setup.get();

    /** Reset the chainman in the testing setup object */
    ResetChainman(*test_setup);

    /** Initialize taproot script declared as static variables. */
    init_taproot_script();

    /** Load the chain mined in ResetChainman in global variables
     * listBlocks and allUTXOCTxIn, to make them available to pick by
     * the harnesses.
     */
    loadCurrentChain();

    /** Prepare multiple transactions for the 201 blocks. They spend coins from
     * various coinbases that are now mature enough.
     */
    addExtraTxsInMempool(*test_setup);
    /** Mine block 201 that shall contain the transactions added in the mempool */
    node::BlockAssembler::Options options;
    options.coinbase_output_script = P2WSH_OP_TRUE;
    MineBlock(test_setup->m_node, options);
    Assert(test_setup->m_node.chainman->ActiveChainstate().GetMempool()->size() == 0);

    /** Load the 201th block into listBlocks */
    LOCK(::cs_main);
    auto& CState = Assert(test_setup->m_node.chainman)->ActiveChainstate();
    auto currentBlock = CState.m_chain.Tip();
    loadCurrentBlock(CState, currentBlock);

    test_setup->m_node.chainman->ActiveChainstate().ForceFlushStateToDisk();
}

/**
 * Read one transaction from the fuzzing input through the FuzzedDataProvider.
 * It intend to leave more space to craft complex transactions and especially
 * with various scripts types (P2SH, P2WSH, TAPROOT, NOSCRIPT).
 * It is exclusively used by ConsumeBlock to read transaction inside a block.
 */
CTransactionRef ConsumeTransaction(FuzzedDataProvider& fuzzed_data_provider,
                                   std::vector<CTxIn>& additionalUTXO,
                                   bool coinbase=false,
                                   unsigned targetHeight=0) {

    CMutableTransaction tx;

    /** Some harnesses want to explicitely read coinbase transactions in input */
    if (coinbase) {
        tx.vin.resize(1);                                   /** Size of vin hardcoded */
        tx.vin[0].prevout.SetNull();
        tx.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL;
        if (fuzzed_data_provider.ConsumeBool()) {           /** 1/2 probability to get a valid vin */
            tx.vin[0].scriptSig = CScript() << targetHeight << OP_0;
        } else {
            /** Read arbitrary data in input as scriptSig */
            auto scriptSig = ConsumeRandomLengthByteVector<unsigned char>(fuzzed_data_provider, 100);
            tx.vin[0].scriptSig << scriptSig;
        }
    } else {
        /** Read a normal transaction */
        int numInput = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10);  /** Read up-to 10 input coins */
        tx.vin.resize(numInput);
        for (int i = 0; i < numInput; i++) {
            /** Read integer to read input coins from pre-mined UTXOs or reusing one generated by the input */
            /** The content of the CTxIn content is not read from the input per-se */
            uint32_t targetUTXO = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, allUTXOCTxIn.size() + additionalUTXO.size() - 1);
            if (targetUTXO < allUTXOCTxIn.size()) {  /** Pick it in the UTXO set */
                tx.vin[i] = allUTXOCTxIn[targetUTXO];
            } else {  /** Pick it in the additionalUTXO set */
                Assert((targetUTXO - allUTXOCTxIn.size()) < additionalUTXO.size());
                tx.vin[i] = additionalUTXO[targetUTXO - allUTXOCTxIn.size()];
            }

            /** Enable fuzzer to mutate every CTxIn fields after it being taken from existing UTXOs */
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].nSequence = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].prevout.n = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].prevout.hash = Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider));
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                auto scriptSig = ConsumeRandomLengthByteVector<unsigned char>(fuzzed_data_provider, 100);
                tx.vin[i].scriptSig << scriptSig;
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                tx.vin[i].scriptWitness.stack.clear();
                int numWit = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10);
                for (int j = 0; j < numWit; j++) {
                    tx.vin[i].scriptWitness.stack.push_back(ConsumeRandomLengthByteVector<unsigned char>(fuzzed_data_provider, 100));
                }
            }
        }
    }

    /** Read outputs */
    int numOutput = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 10);
    tx.vout.resize(numOutput);
    for (int i = 0; i < numOutput; i++) {
        /** Read CAmount to spend */
        tx.vout[i].nValue = fuzzed_data_provider.ConsumeIntegral<int64_t>();

        /** Read scriptPubKey type into one of the valid types */
        switch(fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 4)) {
            case 0:
                // P2WSH
                tx.vout[i].scriptPubKey = P2WSH_OP_TRUE;
                break;
            case 1:
                // P2SH
                tx.vout[i].scriptPubKey = P2SH_OP_TRUE;
                break;
            case 2:
                // TAPSCRIPT
                tx.vout[i].scriptPubKey = TAPROOT_OP_TRUE;
                break;
            case 3:
                // NoScript
                tx.vout[i].scriptPubKey = CScript();
                break;
            default: {
                /** Read arbitrary scriptPubKey */
                auto scriptPubKey = ConsumeRandomLengthByteVector<unsigned char>(fuzzed_data_provider, 100);
                tx.vout[i].scriptPubKey << scriptPubKey;
                break;
            }
        }
    }

    /** Create the shared pointer to the CTransaction object */
    auto res = MakeTransactionRef(tx);

    /** For all CTxOut created, create the associated spending script to enable
     * to be spent in later transactions.
     * Do it here as the transaction hash is definitive.
     * */
    if (!coinbase) {
        /** do it only for non-coinbase txs. */
        for (int i = 0; i < numOutput; i++) {
            /** Create the spending script and keep it in additionalUTXO */
            additionalUTXO.emplace_back(getSpendingScript(*res, i));
        }
    }

    return res;
}

/**
 * Consume a block from the fuzzing input.
 * It builds a block on top of the given currentBlock.
 */
CBlock ConsumeBlock(FuzzedDataProvider& fuzzed_data_provider, const CBlock& prevBlock, unsigned targetHeight,
                    std::vector<CTxIn>& additionalUTXO, bool forceValidBlock=false) {
    CBlock block;

    /** First create a valid block header */
    block.nVersion = listBlocks.back()->nVersion;
    block.hashPrevBlock = prevBlock.GetHash();
    block.nTime = listBlocks.back()->nTime + 2;
    block.nBits = listBlocks.back()->nBits;

    /** Give the fuzzer input the ability to mutate block header fields */
    if (fuzzed_data_provider.ConsumeBool()) {
        block.nVersion = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    }
    if (fuzzed_data_provider.ConsumeBool() && !forceValidBlock) {
        block.hashPrevBlock = ConsumeUInt256(fuzzed_data_provider);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        block.nTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        block.nBits = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    }

    /** Read the coinbase transaction from the input */
    block.vtx.push_back(ConsumeTransaction(fuzzed_data_provider, additionalUTXO, true, targetHeight));

    /** Read up to numTx transactions from the input */
    int numTx = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 5);
    for (int i = 0; i < numTx; i++) {
        block.vtx.push_back(ConsumeTransaction(fuzzed_data_provider, additionalUTXO));
    }

    /** Commit witness. */
    if (numTx > 0) {
        test_setup->m_node.chainman->GenerateCoinbaseCommitment(block, nullptr);
    }
    for (unsigned i = 0; i < block.vtx[0]->vout.size(); i++) {
        additionalUTXO.emplace_back(getSpendingScript(*block.vtx[0], i));
    }

    /** Set hashMerkleRoot to expected value */
    block.hashMerkleRoot = BlockMerkleRoot(block);
    /** Let fuzzer mutate hashMerkleRoot if not forced to create a valid block */
    if (fuzzed_data_provider.ConsumeBool() && !forceValidBlock) {
        block.hashMerkleRoot = ConsumeUInt256(fuzzed_data_provider);
    }

    /** Adjust nonce if decided to generate a valid block or we already generated a block with the same hash */
    auto read_valid_nonce = fuzzed_data_provider.ConsumeBool();
    if (read_valid_nonce || forceValidBlock || existingBlockHash.contains(block.GetHash())) {
        const auto& consensus = test_setup->m_node.chainman->GetConsensus();
        block.nNonce = 0;
        // do not check against current nBits (as it may be a huge value)
        while (!CheckProofOfWork(block.GetHash(), listBlocks.back()->nBits, consensus) || existingBlockHash.contains(block.GetHash())) {
            ++block.nNonce;
            if (block.nNonce == 0) break;
        }
    } else {  /** Read the content from the input */
        block.nNonce = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    }

    return block;
}



FUZZ_TARGET(connect_block, .init = initialize_connect_block)
{
    LOCK(::cs_main);
    /** Initialize Cleanup class that will automatically restore the environment when exiting the scope */
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(listBlocks.back()->GetBlockTime() + 2);

    /** Initialize data provider */
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());


    Chainstate& active_chainstate = test_setup->m_node.chainman->ActiveChainstate();
    CBlockIndex* active_tip = active_chainstate.m_chain.Tip();
    CCoinsViewCache& active_coins = active_chainstate.CoinsTip();

    DEBUGOUTPUT(std::cout << "Current Height: " << active_tip->nHeight << std::endl);

    /** Read the new block from the input */
    std::vector<CTxIn> additionalUTXO;
    CBlock block = ConsumeBlock(fuzzed_data_provider, *listBlocks.back(), active_tip->nHeight + 1, additionalUTXO);
    CBlockHeader curr_header = static_cast<const CBlockHeader&>(block);

    BlockValidationState state;
    /** Perform basic checks on the block to early reject it */
    const auto& consensus = test_setup->m_node.chainman->GetConsensus();
    if (!CheckBlock(block, state, consensus)) {
        DEBUGOUTPUT(std::cout << "Block invalid: " << state.GetRejectReason() << std::endl);
        return;
    }

    /** Compute new CBlockIndex object */
    uint256 currentHash = curr_header.GetHash();
    CBlockIndex new_index(curr_header);
    new_index.pprev = active_tip;
    new_index.nHeight = active_tip->nHeight + 1;
    new_index.phashBlock = &currentHash;

    /** Try to connect the block */
    bool success = active_chainstate.ConnectBlock(block,
                                                  state,
                                                  &new_index,
                                                  active_coins,
                                                  /* justCheck*/ true);

    if (success) {
        DEBUGOUTPUT(std::cout << "Block connected successfully: " << curr_header.GetHash().ToString() << std::endl);
        DEBUGOUTPUT(std::cout << "State: " << state.ToString() << std::endl);
        //Assert(active_chainstate.DisconnectBlock(block, &new_index, active_coins) == DISCONNECT_OK);
    }
    else {
        DEBUGOUTPUT(std::cout << "Block connection failed: " << state.GetRejectReason() << std::endl);
        // If the connection failed, we can still try to disconnect the block
        // to ensure that the disconnect logic is robust.
    }
}

}