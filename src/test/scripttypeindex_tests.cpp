#include <index/scripttypeindex.h>
#include <interfaces/chain.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <validation.h>
#include <addresstype.h> 

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(scripttypeindex_tests)

// Basic initial sync test
BOOST_FIXTURE_TEST_CASE(scripttypeindex_initial_sync, TestChain100Setup)
{
    ScriptTypeIndex script_type_index{interfaces::MakeChain(m_node), 0, true};
    BOOST_REQUIRE(script_type_index.Init());

    const CBlockIndex* block_index;
    {
        LOCK(cs_main);
        block_index = m_node.chainman->ActiveChain().Tip();
    }

    // ScriptTypeIndex should not be found before it is started.
    ScriptTypeBlockStats stats;
    BOOST_CHECK(!script_type_index.LookupStats(block_index->GetBlockHash(), stats));

    // BlockUntilSyncedToCurrentChain should return false before index is started.
    BOOST_CHECK(!script_type_index.BlockUntilSyncedToCurrentChain());

    // Sync the index
    script_type_index.Sync();

    // Check that ScriptTypeIndex works for genesis block.
    const CBlockIndex* genesis_block_index;
    {
        LOCK(cs_main);
        genesis_block_index = m_node.chainman->ActiveChain().Genesis();
    }
    BOOST_CHECK(script_type_index.LookupStats(genesis_block_index->GetBlockHash(), stats));

    // Check that ScriptTypeIndex updates with new blocks.
    BOOST_CHECK(script_type_index.LookupStats(block_index->GetBlockHash(), stats));

    // Create a new block with only a coinbase tx
    const CScript script_pub_key{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
    std::vector<CMutableTransaction> noTxns;
    CreateAndProcessBlock(noTxns, script_pub_key);

    // Let the ScriptTypeIndex catch up again.
    BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());

    // get new tip
    const CBlockIndex* new_block_index;
    {
        LOCK(cs_main);
        new_block_index = m_node.chainman->ActiveChain().Tip();
    }
    BOOST_CHECK(script_type_index.LookupStats(new_block_index->GetBlockHash(), stats));

    // Check that the tip actually changed
    BOOST_CHECK(block_index != new_block_index);


    // It is not safe to stop and destroy the index until it finishes handling
    // the last BlockConnected notification. The BlockUntilSyncedToCurrentChain()
    // call above is sufficient to ensure this, but the
    // SyncWithValidationInterfaceQueue() call below is also needed to ensure
    // TSAN always sees the test thread waiting for the notification thread, and
    // avoid potential false positive reports.
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    script_type_index.Stop();
}

// Test with multiple script types: P2PK (coinbase), P2PKH, P2SH, MULTISIG, NULL_DATA, P2WPKH, P2WSH, P2TR
// We create separate blocks for each script type to ensure they're all processed correctly
BOOST_FIXTURE_TEST_CASE(scripttypeindex_two_script_types, TestChain100Setup)
{
    ScriptTypeIndex script_type_index{interfaces::MakeChain(m_node), 0, true};
    BOOST_REQUIRE(script_type_index.Init());
    script_type_index.Sync();

    // Generate keys for different script types
    CKey key1 = GenerateRandomKey();
    CKey key2 = GenerateRandomKey();
    CKey key3 = GenerateRandomKey();
    CPubKey pubkey1 = key1.GetPubKey();
    CPubKey pubkey2 = key2.GetPubKey();
    CPubKey pubkey3 = key3.GetPubKey();

    CScript coinbase_script = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG; // P2PK
    CAmount coinbase_value = m_coinbase_txns[0]->vout[0].nValue;

    // Create blocks with different script types and verify each one
    std::map<TxoutType, uint256> block_hashes;

    // Block 1: P2PKH
    {
        std::vector<CTxOut> outputs;
        outputs.push_back(CTxOut(coinbase_value - 10000, GetScriptForDestination(PKHash(pubkey1))));
        auto [tx, fee] = CreateValidTransaction(
            {m_coinbase_txns[0]},
            {COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
            0,
            {coinbaseKey},
            outputs,
            std::nullopt,
            std::nullopt
        );
        const CBlock& block = CreateAndProcessBlock({tx}, coinbase_script);
        BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());
        {
            LOCK(cs_main);
            block_hashes[TxoutType::PUBKEYHASH] = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        }
    }

    // Block 2: P2SH
    {
        CScript redeem_script = CScript() << OP_TRUE;
        std::vector<CTxOut> outputs;
        outputs.push_back(CTxOut(coinbase_value - 10000, GetScriptForDestination(ScriptHash(redeem_script))));
        auto [tx, fee] = CreateValidTransaction(
            {m_coinbase_txns[1]},
            {COutPoint(m_coinbase_txns[1]->GetHash(), 0)},
            0,
            {coinbaseKey},
            outputs,
            std::nullopt,
            std::nullopt
        );
        const CBlock& block = CreateAndProcessBlock({tx}, coinbase_script);
        BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());
        {
            LOCK(cs_main);
            block_hashes[TxoutType::SCRIPTHASH] = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        }
    }

    // Block 3: MULTISIG
    {
        std::vector<CPubKey> multisig_keys = {pubkey1, pubkey2, pubkey3};
        CScript multisig_script = GetScriptForMultisig(2, multisig_keys);
        std::vector<CTxOut> outputs;
        outputs.push_back(CTxOut(coinbase_value - 10000, multisig_script));
        auto [tx, fee] = CreateValidTransaction(
            {m_coinbase_txns[2]},
            {COutPoint(m_coinbase_txns[2]->GetHash(), 0)},
            0,
            {coinbaseKey},
            outputs,
            std::nullopt,
            std::nullopt
        );
        const CBlock& block = CreateAndProcessBlock({tx}, coinbase_script);
        BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());
        {
            LOCK(cs_main);
            block_hashes[TxoutType::MULTISIG] = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        }
    }

    // Block 4: NULL_DATA
    {
        std::vector<CTxOut> outputs;
        outputs.push_back(CTxOut(0, CScript() << OP_RETURN << std::vector<unsigned char>(20, 0x42)));
        auto [tx, fee] = CreateValidTransaction(
            {m_coinbase_txns[3]},
            {COutPoint(m_coinbase_txns[3]->GetHash(), 0)},
            0,
            {coinbaseKey},
            outputs,
            std::nullopt,
            std::nullopt
        );
        const CBlock& block = CreateAndProcessBlock({tx}, coinbase_script);
        BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());
        {
            LOCK(cs_main);
            block_hashes[TxoutType::NULL_DATA] = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        }
    }

    // Block 5: P2WPKH
    {
        std::vector<CTxOut> outputs;
        outputs.push_back(CTxOut(coinbase_value - 10000, GetScriptForDestination(WitnessV0KeyHash(pubkey1.GetID()))));
        auto [tx, fee] = CreateValidTransaction(
            {m_coinbase_txns[4]},
            {COutPoint(m_coinbase_txns[4]->GetHash(), 0)},
            0,
            {coinbaseKey},
            outputs,
            std::nullopt,
            std::nullopt
        );
        const CBlock& block = CreateAndProcessBlock({tx}, coinbase_script);
        BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());
        {
            LOCK(cs_main);
            block_hashes[TxoutType::WITNESS_V0_KEYHASH] = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        }
    }

    // Block 6: P2WSH
    {
        CScript witness_script = CScript() << OP_1 << ToByteVector(pubkey1) << OP_1 << OP_CHECKMULTISIG;
        WitnessV0ScriptHash witness_script_hash(witness_script);
        std::vector<CTxOut> outputs;
        outputs.push_back(CTxOut(coinbase_value - 10000, GetScriptForDestination(witness_script_hash)));
        auto [tx, fee] = CreateValidTransaction(
            {m_coinbase_txns[5]},
            {COutPoint(m_coinbase_txns[5]->GetHash(), 0)},
            0,
            {coinbaseKey},
            outputs,
            std::nullopt,
            std::nullopt
        );
        const CBlock& block = CreateAndProcessBlock({tx}, coinbase_script);
        BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());
        {
            LOCK(cs_main);
            block_hashes[TxoutType::WITNESS_V0_SCRIPTHASH] = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        }
    }

    // Block 7: P2TR (Taproot)
    {
        XOnlyPubKey xonly_pubkey(pubkey1);
        std::vector<CTxOut> outputs;
        outputs.push_back(CTxOut(coinbase_value - 10000, GetScriptForDestination(WitnessV1Taproot(xonly_pubkey))));
        auto [tx, fee] = CreateValidTransaction(
            {m_coinbase_txns[6]},
            {COutPoint(m_coinbase_txns[6]->GetHash(), 0)},
            0,
            {coinbaseKey},
            outputs,
            std::nullopt,
            std::nullopt
        );
        const CBlock& block = CreateAndProcessBlock({tx}, coinbase_script);
        BOOST_CHECK(script_type_index.BlockUntilSyncedToCurrentChain());
        {
            LOCK(cs_main);
            block_hashes[TxoutType::WITNESS_V1_TAPROOT] = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        }
    }

    // Verify each block's stats
    for (const auto& [type, block_hash] : block_hashes) {
        ScriptTypeBlockStats stats;
        BOOST_REQUIRE(script_type_index.LookupStats(block_hash, stats));
        
        size_t type_idx = static_cast<size_t>(type);
        
        // Each block should have: at least 1 P2PK (coinbase) + at least 1 of the specific script type
        BOOST_CHECK_GE(stats.output_counts[static_cast<size_t>(TxoutType::PUBKEY)], 1U);
        BOOST_CHECK_GE(stats.output_counts[type_idx], 1U);
        
        if (type == TxoutType::NULL_DATA) {
            BOOST_CHECK_EQUAL(stats.output_values[type_idx], 0);
        } else {
            BOOST_CHECK_GT(stats.output_values[type_idx], 0);
        }
        
        // Verify NONSTANDARD is 0 for each block
        BOOST_CHECK_EQUAL(stats.output_counts[static_cast<size_t>(TxoutType::NONSTANDARD)], 0U);
    }

    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    script_type_index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()