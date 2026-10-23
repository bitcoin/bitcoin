// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <consensus/amount.h>
#include <crypto/hex_base.h>
#include <dbwrapper.h>
#include <index/txoscriptindex.h>
#include <index/txoscriptindex_key.h>
#include <interfaces/chain.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <validation.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txoscriptindex_tests)

class TxoScriptIndexTest : public TestChain100Setup
{
public:
    TxoScriptIndex index{interfaces::MakeChain(m_node), 1_MiB, /*memory=*/true};
    uint64_t remaining_bytes{50_MiB};

    TxoScriptIndexTest() { BOOST_REQUIRE(index.Init()); }
    ~TxoScriptIndexTest() { StopIndex(index, m_node); }

    CDBWrapper& GetDB() { return index.GetDB(); }

    txoscriptindex::HashPrefix GetKeyPrefix(const uint256& script_hash)
    {
        std::pair<uint64_t, uint64_t> salt;
        BOOST_REQUIRE(GetDB().Read(txoscriptindex::DB_SPK_HASH_SALT, salt));
        return txoscriptindex::CreateKeyPrefix(SipHasher13UJ{salt.first, salt.second}, script_hash);
    }
};

BOOST_AUTO_TEST_CASE(txoscriptindex_key_encoding)
{
    // Pin the complete persisted key format as well as the height encoding.
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txoscriptindex::DBKey{0x0102030405, 128, 81}), "7801020304058000000051");
}

BOOST_FIXTURE_TEST_CASE(txoscriptindex_initial_sync, TxoScriptIndexTest)
{
    const CScript& coinbase_script{m_coinbase_txns[0]->vout[0].scriptPubKey};
    const uint256 script_hash{txoscriptindex::Sha256ScriptPubKey(coinbase_script)};
    const int32_t tip_height{WITH_LOCK(::cs_main, return m_node.chainman->ActiveHeight())};

    BOOST_CHECK(index.Find(script_hash, /*start_height=*/0, tip_height, remaining_bytes).value().empty());

    index.Sync();
    const auto all{index.Find(script_hash, /*start_height=*/0, tip_height, remaining_bytes).value()};
    BOOST_CHECK_EQUAL(all.size(), m_coinbase_txns.size());
    for (const auto& match : all) {
        BOOST_REQUIRE(match.tx);
        BOOST_REQUIRE_EQUAL(match.outputs.size(), 1);
        BOOST_CHECK(match.tx->vout[match.outputs[0]].scriptPubKey == coinbase_script);
    }

    const auto ranged{index.Find(script_hash, /*start_height=*/90, /*end_height=*/95, remaining_bytes).value()};
    BOOST_REQUIRE_EQUAL(ranged.size(), 6);
    for (size_t i{0}; i < ranged.size(); ++i) {
        BOOST_CHECK_EQUAL(ranged[i].height, 90 + i);
    }

    BOOST_CHECK(index.Find(script_hash, -1, 95, remaining_bytes).value().empty());
    BOOST_CHECK(index.Find(script_hash, 95, 90, remaining_bytes).value().empty());
}

BOOST_FIXTURE_TEST_CASE(txoscriptindex_collisions_and_pruned_blocks, TxoScriptIndexTest)
{
    const CScript target_script{CScript() << OP_TRUE};
    const uint256 script_hash{txoscriptindex::Sha256ScriptPubKey(target_script)};
    const auto first_block{CreateAndProcessBlock({}, target_script)};
    const auto second_block{CreateAndProcessBlock({}, target_script)};
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    index.Sync();
    CDBWrapper& db{GetDB()};
    const auto prefix{GetKeyPrefix(script_hash)};

    // Inject prefix collisions pointing at unrelated coinbases before the real matches.
    for (uint32_t height{1}; height <= 3; ++height) {
        db.Write(txoscriptindex::DBKey{prefix, height, 81}, txoscriptindex::EMPTY_VALUE);
    }
    // Two candidate transactions in one pruned block must produce one warning.
    db.Write(txoscriptindex::DBKey{prefix, 3, 82}, txoscriptindex::EMPTY_VALUE);
    CBlockIndex* pruned_block{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain()[3])};
    const auto original_status{WITH_LOCK(cs_main, return pruned_block->nStatus)};
    WITH_LOCK(cs_main, pruned_block->nStatus &= ~BLOCK_HAVE_DATA);
    const auto matches{index.Find(script_hash, 0, 102, remaining_bytes).value()};
    BOOST_REQUIRE_EQUAL(matches.size(), 3);
    BOOST_CHECK(!matches[0].tx);
    BOOST_CHECK(matches[0].block_hash == pruned_block->GetBlockHash());
    BOOST_CHECK_EQUAL(matches[0].height, 3);
    BOOST_CHECK_EQUAL(matches[0].tx_offset, 0);
    BOOST_CHECK(matches[0].outputs.empty());
    BOOST_CHECK(matches[1].tx->GetHash() == first_block.vtx[0]->GetHash());
    BOOST_CHECK(matches[2].tx->GetHash() == second_block.vtx[0]->GetHash());
    BOOST_CHECK_EQUAL(index.Find(script_hash, 0, 100, remaining_bytes).value().size(), 1);
    WITH_LOCK(cs_main, pruned_block->nStatus = original_status);
}

BOOST_FIXTURE_TEST_CASE(txoscriptindex_read_failures, TxoScriptIndexTest)
{
    const uint256 script_hash{txoscriptindex::Sha256ScriptPubKey(m_coinbase_txns[0]->vout[0].scriptPubKey)};
    index.Sync();
    CBlockIndex* block{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain()[1])};
    const auto check_read_error{[&] {
        const auto result{index.Find(script_hash, 1, 1, remaining_bytes)};
        BOOST_REQUIRE(!result);
        BOOST_CHECK(result.error().starts_with("Unable to read txoscriptindex transaction in block " + block->GetBlockHash().ToString() + ": "));
    }};

    const auto original_file{WITH_LOCK(cs_main, return block->nFile)};
    WITH_LOCK(cs_main, block->nFile = std::numeric_limits<decltype(block->nFile)>::max());
    check_read_error();
    WITH_LOCK(cs_main, block->nFile = original_file);

    // Simulate a corrupt index row pointing beyond the block file.
    const auto original_pos{WITH_LOCK(cs_main, return block->nDataPos)};
    WITH_LOCK(cs_main, block->nDataPos = 1U << 30);
    check_read_error();
    WITH_LOCK(cs_main, block->nDataPos = original_pos);
    BOOST_CHECK_EQUAL(index.Find(script_hash, 1, 1, remaining_bytes).value().size(), 1);
}

BOOST_FIXTURE_TEST_CASE(txoscriptindex_multi_output, TxoScriptIndexTest)
{
    const CScript& coinbase_script{m_coinbase_txns[0]->vout[0].scriptPubKey};
    const CScript dest_script{CScript() << OP_2};

    const auto tx{CreateValidMempoolTransaction(
        {m_coinbase_txns[0]}, {{m_coinbase_txns[0]->GetHash(), 0}}, /*input_height=*/1, {coinbaseKey},
        {{25 * COIN, dest_script}, {24 * COIN, dest_script}, {0, CScript() << OP_TRUE}}, /*submit=*/false)};

    const auto block{CreateAndProcessBlock({tx}, coinbase_script)};
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    index.Sync();
    const int32_t tip_height{WITH_LOCK(::cs_main, return m_node.chainman->ActiveHeight())};
    const auto matches{index.Find(txoscriptindex::Sha256ScriptPubKey(dest_script), /*start_height=*/0, tip_height, remaining_bytes).value()};
    BOOST_REQUIRE_EQUAL(matches.size(), 1);
    BOOST_REQUIRE(matches[0].tx);
    BOOST_CHECK(matches[0].tx->GetHash() == tx.GetHash());
    BOOST_CHECK_EQUAL(matches[0].tx_offset, txoscriptindex::BLOCK_HEADER_SIZE + GetSizeOfCompactSize(block.vtx.size()) + block.vtx[0]->ComputeTotalSize());
    BOOST_CHECK((matches[0].outputs == std::vector<uint32_t>{0, 1}));

    // A shared work budget charges each read, even for repeated transactions
    // or a second script in the same transaction.
    const uint64_t tx_bytes{CTransaction{tx}.ComputeTotalSize()};
    for (const auto& expected_outputs : {std::vector<uint32_t>{0, 1}, std::vector<uint32_t>{2}}) {
        const uint64_t before{remaining_bytes};
        const auto result{index.Find(txoscriptindex::Sha256ScriptPubKey(tx.vout[expected_outputs.front()].scriptPubKey), 0, tip_height, remaining_bytes).value()};
        BOOST_REQUIRE_EQUAL(result.size(), 1);
        BOOST_CHECK(result[0].outputs == expected_outputs);
        BOOST_CHECK_GT(before - remaining_bytes, tx_bytes);
    }
}

BOOST_FIXTURE_TEST_CASE(txoscriptindex_false_positive_budget, TxoScriptIndexTest)
{
    const uint256 script_hash{txoscriptindex::Sha256ScriptPubKey(CScript() << OP_TRUE)};
    index.Sync();
    CDBWrapper& db{GetDB()};
    const auto prefix{GetKeyPrefix(script_hash)};
    for (uint32_t height{1}; height <= 2; ++height) {
        db.Write(txoscriptindex::DBKey{prefix, height, 81}, txoscriptindex::EMPTY_VALUE);
    }

    const uint64_t key_bytes{GetSerializeSize(txoscriptindex::DBKey{prefix, 1, 81})};
    const uint64_t tx_bytes{m_coinbase_txns[0]->ComputeTotalSize()};
    BOOST_CHECK(index.Find(script_hash, 1, 2, remaining_bytes).value().empty());
    CBlockIndex* unreadable{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain()[2])};
    const auto original_pos{WITH_LOCK(cs_main, return unreadable->nDataPos)};
    WITH_LOCK(cs_main, unreadable->nDataPos = 1U << 30);
    // Exhaust the budget on a false positive before reaching the unreadable block.
    remaining_bytes = key_bytes + tx_bytes - 1;
    auto result{index.Find(script_hash, 1, 2, remaining_bytes)};
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().starts_with("Script history index lookup limit exceeded while scanning block 1."));
    BOOST_CHECK_EQUAL(remaining_bytes, tx_bytes - 1);

    // Pruned candidates still consume the key-visit budget.
    CBlockIndex* pruned{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain()[1])};
    const auto original_status{WITH_LOCK(cs_main, return pruned->nStatus)};
    WITH_LOCK(cs_main, pruned->nStatus &= ~BLOCK_HAVE_DATA);
    remaining_bytes = key_bytes;
    result = index.Find(script_hash, 1, 2, remaining_bytes);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().starts_with("Script history index lookup limit exceeded while scanning block 2."));
    BOOST_CHECK_EQUAL(remaining_bytes, 0);
    WITH_LOCK(cs_main, pruned->nStatus = original_status);
    WITH_LOCK(cs_main, unreadable->nDataPos = original_pos);

    // A boundary key outside the requested range must not produce a height hint.
    remaining_bytes = key_bytes + tx_bytes;
    result = index.Find(script_hash, 1, 1, remaining_bytes);
    BOOST_REQUIRE(!result);
    BOOST_CHECK_EQUAL(result.error(), "Script history index lookup limit exceeded. Narrow the range with start_height/end_height or the descriptor range.");

    // An exact budget succeeds, including the boundary key past the requested range.
    remaining_bytes = 2 * key_bytes + tx_bytes;
    BOOST_CHECK(index.Find(script_hash, 1, 1, remaining_bytes).value().empty());
    BOOST_CHECK_EQUAL(remaining_bytes, 0);
}

BOOST_AUTO_TEST_SUITE_END()
