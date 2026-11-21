// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <common/args.h>
#include <consensus/validation.h>
#include <crypto/hex_base.h>
#include <dbwrapper.h>
#include <index/block_seq.h>
#include <index/disktxpos.h>
#include <index/txospenderindex.h>
#include <index/txospenderindex_key.h>
#include <streams.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <util/byte_units.h>
#include <util/strencodings.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txospenderindex_tests)

// Grants tests access to the otherwise non-public txospenderindex database handle.
class TxoSpenderIndexTest
{
public:
    static CDBWrapper& GetDB(const TxoSpenderIndex& index) { return index.GetDB(); }
};

namespace {

fs::path DBPath() { return gArgs.GetDataDirNet() / "indexes" / "txospenderindex" / "db"; }

SipHasher13UJ ReadHasher(const CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    BOOST_REQUIRE(db.Read(txospenderindex::DB_OUTPOINT_HASH_SALT, salt));
    return SipHasher13UJ{salt.first, salt.second};
}

std::vector<block_seq::BlockTxPosition> BucketPositions(CDBWrapper& db, block_seq::HashKeyPrefix prefix)
{
    std::vector<block_seq::BlockTxPosition> positions;
    std::unique_ptr<CDBIterator> it{db.NewIterator()};
    txospenderindex::DBKey key{prefix, {}};
    for (it->Seek(key); it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
        positions.push_back(key.pos);
    }
    return positions;
}

void InvalidateBlock(ChainstateManager& chainman, const uint256& block_hash)
{
    CBlockIndex* block_index{WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(block_hash))};
    BOOST_REQUIRE(block_index);
    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, block_index));
}

} // namespace

BOOST_FIXTURE_TEST_CASE(txospenderindex_initial_sync, TestChain100Setup)
{
    // Setup phase:
    // Mine blocks for coinbase maturity, so we can spend some coinbase outputs in the test.
    const CScript& coinbase_script = m_coinbase_txns[0]->vout[0].scriptPubKey;
    for (int i = 0; i < 10; i++) CreateAndProcessBlock({}, coinbase_script);

    // Spend 10 outputs
    std::vector<COutPoint> spent(10);
    std::vector<CMutableTransaction> spender(spent.size());
    for (size_t i = 0; i < spent.size(); i++) {
        // Outpoint
        auto coinbase_tx = m_coinbase_txns[i];
        spent[i] = COutPoint(coinbase_tx->GetHash(), 0);

        // Spending tx
        spender[i].version = 1;
        spender[i].vin.resize(1);
        spender[i].vin[0].prevout.hash = spent[i].hash;
        spender[i].vin[0].prevout.n = spent[i].n;
        spender[i].vout.resize(1);
        spender[i].vout[0].nValue = coinbase_tx->GetValueOut();
        spender[i].vout[0].scriptPubKey = coinbase_script;

        // Sign
        std::vector<unsigned char> vchSig;
        const uint256 hash = SignatureHash(coinbase_script, spender[i], 0, SIGHASH_ALL, 0, SigVersion::BASE);
        BOOST_REQUIRE(coinbaseKey.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        spender[i].vin[0].scriptSig << vchSig;
    }

    // Generate and ensure block has been fully processed
    const uint256 tip_hash = CreateAndProcessBlock(spender, coinbase_script).GetHash();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return m_node.chainman->ActiveTip()->GetBlockHash()), tip_hash);

    // Now we concluded the setup phase, run index
    TxoSpenderIndex txospenderindex(interfaces::MakeChain(m_node), 1 << 20, true);
    BOOST_REQUIRE(txospenderindex.Init());
    BOOST_CHECK(!txospenderindex.BlockUntilSyncedToCurrentChain()); // false when not synced
    BOOST_CHECK_NE(txospenderindex.GetSummary().best_block_hash, tip_hash);

    // Transaction should not be found in the index before it is synced.
    for (const auto& outpoint : spent) {
        BOOST_CHECK(!txospenderindex.FindSpender(outpoint));
    }

    txospenderindex.Sync();
    BOOST_CHECK_EQUAL(txospenderindex.GetSummary().best_block_hash, tip_hash);

    for (size_t i = 0; i < spent.size(); i++) {
        const auto tx_spender{txospenderindex.FindSpender(spent[i])};
        BOOST_REQUIRE(tx_spender);
        BOOST_CHECK_EQUAL(tx_spender->tx->GetHash(), spender[i].GetHash());
        BOOST_CHECK_EQUAL(tx_spender->block_hash, tip_hash);
    }

    StopIndex(txospenderindex, m_node);
}

BOOST_AUTO_TEST_CASE(txospenderindex_key_encoding)
{
    // Pin the full key encodings, including the type prefixes.
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txospenderindex::DBKey{0x0102030405, {1, 2}}),
                      "78010203040501000002");
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txospenderindex::BlockSeqKey{1}), "7101");
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txospenderindex::BlockHashKey{uint256::ONE}),
                      "680100000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txospenderindex::DB_OUTPOINT_HASH_SALT),
                      "126f7574706f696e745f686173685f73616c74");

    // Legacy keys must match what previous versions wrote: the siphash key was
    // written with a string literal, so it has no length prefix and keeps its
    // terminating null byte.
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txospenderindex::DB_LEGACY_SIPHASH_KEY), "736970686173685f6b657900");
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txospenderindex::LegacyDBKey{0x0102030405060708, CDiskTxPos{{1, 2}, 3}}),
                      "730807060504030201010203");
}

//! Fixture with a transaction spending the first coinbase output, not yet mined.
struct SpendSetup : public TestChain100Setup {
    const CScript coinbase_script{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};
    const COutPoint spent{m_coinbase_txns[0]->GetHash(), 0};
    const CMutableTransaction spend_mtx{CreateValidMempoolTransaction(
        /*input_transaction=*/m_coinbase_txns[0],
        /*input_vout=*/0,
        /*input_height=*/1,
        /*input_signing_key=*/coinbaseKey,
        /*output_destination=*/CScript() << OP_TRUE,
        /*output_amount=*/CAmount{1 * COIN},
        /*submit=*/false)};
    const Txid spend_txid{spend_mtx.GetHash()};
};

BOOST_FIXTURE_TEST_CASE(txospenderindex_collision_scan_path, SpendSetup)
{
    const uint256 block_hash{CreateAndProcessBlock({spend_mtx}, coinbase_script).GetHash()};

    TxoSpenderIndex index(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(index.Init());
    index.Sync();

    CDBWrapper& db{TxoSpenderIndexTest::GetDB(index)};
    const SipHasher13UJ hasher{ReadHasher(db)};
    const auto spent_prefix{txospenderindex::CreateKeyPrefix(hasher, spent)};
    const auto spent_bucket{BucketPositions(db, spent_prefix)};
    BOOST_REQUIRE_EQUAL(spent_bucket.size(), 1U);

    // Forge a colliding entry under the prefix of an unspent outpoint, pointing at
    // the spending transaction, which spends a different outpoint.
    const COutPoint unspent{m_coinbase_txns[1]->GetHash(), 0};
    const auto unspent_prefix{txospenderindex::CreateKeyPrefix(hasher, unspent)};
    BOOST_REQUIRE(unspent_prefix != spent_prefix);
    BOOST_REQUIRE(BucketPositions(db, unspent_prefix).empty());
    db.Write(txospenderindex::DBKey{unspent_prefix, spent_bucket.front()}, block_seq::EMPTY_VALUE);

    // The false positive is read and rejected.
    BOOST_CHECK(!index.FindSpender(unspent));

    // A false positive in the bucket of a spent outpoint does not hide its spender.
    // Point it at the coinbase of the same block, which sorts before the spender.
    db.Write(txospenderindex::DBKey{spent_prefix, {spent_bucket.front().block_seq, block_seq::BLOCK_HEADER_SIZE + 1}}, block_seq::EMPTY_VALUE);
    BOOST_REQUIRE_EQUAL(BucketPositions(db, spent_prefix).size(), 2U);
    const auto spender{index.FindSpender(spent)};
    BOOST_REQUIRE(spender);
    BOOST_CHECK(spender->tx->GetHash() == spend_txid);
    BOOST_CHECK(spender->block_hash == block_hash);

    StopIndex(index, m_node);
}

BOOST_FIXTURE_TEST_CASE(txospenderindex_unreadable_candidate, SpendSetup)
{
    const uint256 block_hash{CreateAndProcessBlock({spend_mtx}, coinbase_script).GetHash()};

    TxoSpenderIndex index(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/true);
    BOOST_REQUIRE(index.Init());
    index.Sync();

    CDBWrapper& db{TxoSpenderIndexTest::GetDB(index)};
    const auto prefix{txospenderindex::CreateKeyPrefix(ReadHasher(db), spent)};
    BOOST_REQUIRE_EQUAL(BucketPositions(db, prefix).size(), 1U);

    // Forge a false positive pointing at the block at height 1, which sorts
    // before the real spender, and make that block's data unavailable.
    CBlockIndex* unreadable{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain()[1])};
    BOOST_REQUIRE(unreadable);
    uint32_t unreadable_seq;
    BOOST_REQUIRE(db.Read(txospenderindex::BlockHashKey{unreadable->GetBlockHash()}, unreadable_seq));
    const block_seq::BlockTxPosition unreadable_pos{unreadable_seq, block_seq::BLOCK_HEADER_SIZE + 1};
    db.Write(txospenderindex::DBKey{prefix, unreadable_pos}, block_seq::EMPTY_VALUE);
    // The lookup visits the unreadable candidate first.
    const auto bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE_EQUAL(bucket.size(), 2U);
    BOOST_CHECK(bucket.front() == unreadable_pos);
    WITH_LOCK(cs_main, unreadable->nStatus &= ~BLOCK_HAVE_DATA);

    // The unreadable candidate is skipped and the real spender is still found.
    const auto spender{index.FindSpender(spent)};
    WITH_LOCK(cs_main, unreadable->nStatus |= BLOCK_HAVE_DATA);
    BOOST_REQUIRE(spender);
    BOOST_CHECK(spender->tx->GetHash() == spend_txid);
    BOOST_CHECK(spender->block_hash == block_hash);

    StopIndex(index, m_node);
}

BOOST_FIXTURE_TEST_CASE(txospenderindex_reorg_keeps_stale_entries, SpendSetup)
{
    TxoSpenderIndex index(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/true);
    BOOST_REQUIRE(index.Init());
    index.Sync();

    const uint256 stale_block_hash{CreateAndProcessBlock({spend_mtx}, coinbase_script).GetHash()};
    BOOST_REQUIRE(index.BlockUntilSyncedToCurrentChain());
    {
        const auto spender{index.FindSpender(spent)};
        BOOST_REQUIRE(spender);
        BOOST_CHECK(spender->tx->GetHash() == spend_txid);
        BOOST_CHECK(spender->block_hash == stale_block_hash);
    }

    CDBWrapper& db{TxoSpenderIndexTest::GetDB(index)};
    const auto prefix{txospenderindex::CreateKeyPrefix(ReadHasher(db), spent)};
    const auto original_bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE_EQUAL(original_bucket.size(), 1U);

    ChainstateManager& chainman{*m_node.chainman};

    // Once its block leaves the active chain the spend is no longer returned,
    // even before the index processes the reorg, but its entry is kept.
    InvalidateBlock(chainman, stale_block_hash);
    BOOST_CHECK(!index.FindSpender(spent));
    const uint256 empty_block_hash{CreateAndProcessBlock({}, CScript() << OP_TRUE).GetHash()};
    BOOST_REQUIRE(index.BlockUntilSyncedToCurrentChain());
    BOOST_CHECK(!index.FindSpender(spent));
    BOOST_CHECK_EQUAL(BucketPositions(db, prefix).size(), 1U);

    // Mine the same spend into the replacement branch.
    const uint256 branch_block_hash{CreateAndProcessBlock({spend_mtx}, CScript() << OP_TRUE).GetHash()};
    BOOST_REQUIRE(index.BlockUntilSyncedToCurrentChain());
    {
        const auto spender{index.FindSpender(spent)};
        BOOST_REQUIRE(spender);
        BOOST_CHECK(spender->block_hash == branch_block_hash);
    }
    BOOST_CHECK_EQUAL(BucketPositions(db, prefix).size(), 2U);

    // Reorg back to the original branch, which must be extended to become the most-work chain.
    {
        LOCK(cs_main);
        chainman.ActiveChainstate().ResetBlockFailureFlags(chainman.m_blockman.LookupBlockIndex(stale_block_hash));
    }
    InvalidateBlock(chainman, empty_block_hash);
    {
        BlockValidationState state;
        BOOST_REQUIRE(chainman.ActiveChainstate().ActivateBestChain(state));
    }
    BOOST_CHECK(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip()->GetBlockHash()) == stale_block_hash);
    CreateAndProcessBlock({}, coinbase_script);
    BOOST_REQUIRE(index.BlockUntilSyncedToCurrentChain());
    {
        const auto spender{index.FindSpender(spent)};
        BOOST_REQUIRE(spender);
        BOOST_CHECK(spender->block_hash == stale_block_hash);
    }

    // Reconnecting the original block keeps its sequence and creates no duplicate entries.
    const auto reorg_bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE_EQUAL(reorg_bucket.size(), 2U);
    BOOST_CHECK(reorg_bucket.front() == original_bucket.front());

    StopIndex(index, m_node);
}

BOOST_FIXTURE_TEST_CASE(txospenderindex_legacy_fallback, SpendSetup)
{
    const CBlock block{CreateAndProcessBlock({spend_mtx}, coinbase_script)};
    const uint256 block_hash{block.GetHash()};

    // Seed the on-disk database with a legacy entry and a legacy locator at the
    // tip before the index is opened, as if written by a previous version.
    const std::pair<uint64_t, uint64_t> legacy_key{1, 2};
    CDiskTxPos legacy_pos;
    CBlockLocator locator;
    {
        LOCK(cs_main);
        const CBlockIndex* tip{m_node.chainman->ActiveChain().Tip()};
        BOOST_REQUIRE(tip->GetBlockHash() == block_hash);
        // The spending tx follows the 1-byte tx count and the coinbase.
        legacy_pos = CDiskTxPos{{tip->nFile, tip->nDataPos}, 1 + static_cast<uint32_t>(GetSerializeSize(TX_WITH_WITNESS(*block.vtx[0])))};
        locator = GetLocator(tip);
    }
    const txospenderindex::LegacyDBKey legacy_db_key{
        txospenderindex::CreateLegacyKeyPrefix(PresaltedSipHasher{legacy_key.first, legacy_key.second}, spent), legacy_pos};
    {
        CDBWrapper db{DBParams{.path = DBPath(), .cache_bytes = 1_MiB}};
        // Written exactly as previous versions did.
        db.Write("siphash_key", legacy_key);
        db.Write(legacy_db_key, block_seq::EMPTY_VALUE);
        db.Write(uint8_t{'B'}, locator);
    }

    TxoSpenderIndex index(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(index.Init());
    index.Sync();

    // The index resumes from the legacy locator, so the spend is only in a legacy entry.
    CDBWrapper& db{TxoSpenderIndexTest::GetDB(index)};
    BOOST_CHECK(BucketPositions(db, txospenderindex::CreateKeyPrefix(ReadHasher(db), spent)).empty());
    {
        const auto spender{index.FindSpender(spent)};
        BOOST_REQUIRE(spender);
        BOOST_CHECK(spender->tx->GetHash() == spend_txid);
        BOOST_CHECK(spender->block_hash == block_hash);
    }

    // Once its block leaves the active chain, the legacy entry is kept but no
    // longer returned, before and after the index processes the reorg.
    InvalidateBlock(*m_node.chainman, block_hash);
    BOOST_CHECK(!index.FindSpender(spent));
    CreateAndProcessBlock({}, CScript() << OP_TRUE);
    BOOST_REQUIRE(index.BlockUntilSyncedToCurrentChain());
    BOOST_CHECK(db.Exists(legacy_db_key));
    BOOST_CHECK(!index.FindSpender(spent));

    StopIndex(index, m_node);
}

BOOST_AUTO_TEST_SUITE_END()
