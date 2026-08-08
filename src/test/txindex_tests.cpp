// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <crypto/hex_base.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/disktxpos.h>
#include <index/txindex.h>
#include <index/txindex_key.h>
#include <interfaces/chain.h>
#include <key.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <script/script.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <validation.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txindex_tests)

// Grants tests access to the otherwise non-public txindex database handle.
class TxIndexTest
{
public:
    static CDBWrapper& GetDB(const TxIndex& txindex) { return txindex.GetDB(); }
    static CBlockLocator ReadBestBlock(const TxIndex& txindex) { return txindex.GetDB().ReadBestBlock(); }
    static void WriteBestBlock(const TxIndex& txindex, const CBlockLocator& locator)
    {
        auto& db{txindex.GetDB()};
        CDBBatch batch{db};
        db.WriteBestBlock(batch, locator);
        db.WriteBatch(batch);
    }
};

namespace {

SipHasher13UJ ReadHasher(const CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    BOOST_REQUIRE(db.Read(txindex::DB_TXID_HASH_SALT, salt));
    return SipHasher13UJ{salt.first, salt.second};
}

std::vector<txindex::BlockTxPosition> BucketPositions(CDBWrapper& db, txindex::TxHashKeyPrefix prefix)
{
    std::vector<txindex::BlockTxPosition> positions;
    std::unique_ptr<CDBIterator> it{db.NewIterator()};
    txindex::DBKey key{prefix, {}};
    for (it->Seek(key); it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
        positions.push_back(key.pos);
    }
    return positions;
}

FlatFilePos BlockFilePos(const ChainstateManager& chainman, uint32_t height)
{
    LOCK(cs_main);
    const CBlockIndex* block_index{chainman.ActiveChain()[height]};
    BOOST_REQUIRE(block_index);
    return {block_index->nFile, block_index->nDataPos};
}

//! Look up a transaction, requiring success, and return the containing block's hash.
uint256 LookupTx(const TxIndex& txindex, const Txid& txid)
{
    CTransactionRef tx;
    uint256 block_hash;
    BOOST_REQUIRE(txindex.FindTx(txid, block_hash, tx));
    BOOST_CHECK(Assert(tx)->GetHash() == txid);
    return block_hash;
}

void InvalidateBlock(ChainstateManager& chainman, const uint256& block_hash)
{
    CBlockIndex* block_index{WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(block_hash))};
    BOOST_REQUIRE(block_index);
    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, block_index));
}

} // namespace

BOOST_AUTO_TEST_CASE(txindex_position_encoding)
{
    constexpr struct { txindex::BlockTxPosition position; std::string_view encoded; } test_vectors[]{
        {{0, 0}, "000000000000"},
        {{1, 2}, "000001000002"},
        {{10'000'000, 123}, "98968000007b"},
        {{456, 3'999'999}, "0001c83d08ff"},
    };

    for (const auto& [position, encoded] : test_vectors) {
        BOOST_CHECK_EQUAL(HexStr(DataStream{} << position), encoded);

        txindex::BlockTxPosition decoded;
        BOOST_CHECK((DataStream{ParseHex(encoded)} >> decoded).empty());
        BOOST_CHECK(decoded == position);
    }

    // Pin the full key encodings, including the type prefixes.
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txindex::BlockSeqKey{1}), "73000001");
    BOOST_CHECK_EQUAL(HexStr(DataStream{} << txindex::DBKey{0x0102030405, {1, 2}}),
                      "780102030405000001000002");

    BOOST_CHECK_EQUAL(txindex::BLOCK_HEADER_SIZE, GetSerializeSize(CBlockHeader{}));
}

BOOST_AUTO_TEST_CASE(txindex_hash_prefix)
{
    BOOST_CHECK_EQUAL(
        txindex::CreateKeyPrefix(
            SipHasher13UJ{0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL},
            Txid{"1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100"}),
        0xc67d87b08cULL);
}

BOOST_FIXTURE_TEST_CASE(txindex_initial_sync, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/true);
    BOOST_REQUIRE(txindex.Init());

    CTransactionRef tx_disk;
    uint256 block_hash;

    // Transaction should not be found in the index before it is started.
    for (const auto& txn : m_coinbase_txns) {
        BOOST_CHECK(!txindex.FindTx(txn->GetHash(), block_hash, tx_disk));
    }

    // BlockUntilSyncedToCurrentChain should return false before txindex is started.
    BOOST_CHECK(!txindex.BlockUntilSyncedToCurrentChain());

    txindex.Sync();

    // Check that txindex excludes genesis block transactions.
    const CBlock& genesis_block = Params().GenesisBlock();
    for (const auto& txn : genesis_block.vtx) {
        BOOST_CHECK(!txindex.FindTx(txn->GetHash(), block_hash, tx_disk));
    }

    // Check that txindex has all txs that were in the chain before it started.
    for (const auto& txn : m_coinbase_txns) {
        LookupTx(txindex, txn->GetHash());
    }

    // Check that new transactions in new blocks make it into the index.
    for (int i = 0; i < 10; i++) {
        CScript coinbase_script_pub_key = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
        std::vector<CMutableTransaction> no_txns;
        const CBlock& block = CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
        const CTransaction& txn = *block.vtx[0];

        BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
        LookupTx(txindex, txn.GetHash());
    }

    // shutdown sequence (c.f. Shutdown() in init.cpp)
    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_collision_scan_path, TestChain100Setup)
{
    // On-disk, so the legacy-entry probe at construction runs against a fresh
    // database, as it would on a node whose index was created by this version.
    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const SipHasher13UJ hasher{ReadHasher(db)};

    // Lookups scan candidates in descending sequence order, so entries of
    // later-connected blocks are tried first. Forge a colliding entry under the
    // first coinbase's prefix pointing at the last coinbase, so looking up the
    // first tx must scan that false positive first.
    const Txid fake_txid{m_coinbase_txns.back()->GetHash()};
    const Txid target_txid{m_coinbase_txns.front()->GetHash()};
    const auto fake_prefix{txindex::CreateKeyPrefix(hasher, fake_txid)};
    const auto target_prefix{txindex::CreateKeyPrefix(hasher, target_txid)};
    // Distinct prefixes guarantee the target's bucket initially holds only the target.
    BOOST_REQUIRE(fake_prefix != target_prefix);

    // Read the last coinbase's encoded position straight from its bucket.
    const auto fake_bucket{BucketPositions(db, fake_prefix)};
    BOOST_REQUIRE_EQUAL(fake_bucket.size(), 1U);
    const txindex::BlockTxPosition fake_pos{fake_bucket.front()};

    db.Write(txindex::DBKey{target_prefix, fake_pos}, txindex::EMPTY_VALUE);

    // The target's bucket now holds the real target first (lower sequence
    // number), then the forged false positive, which the descending scan tries first.
    const auto target_bucket{BucketPositions(db, target_prefix)};
    BOOST_REQUIRE_EQUAL(target_bucket.size(), 2U);
    BOOST_CHECK(target_bucket[0] != fake_pos);
    BOOST_CHECK(target_bucket[1] == fake_pos);

    LookupTx(txindex, target_txid);

    // A database created fresh by this version cannot contain legacy entries, so
    // lookups skip the legacy fallback: drop the last coinbase's hashed entry and
    // re-add it under the old 't' + txid schema (a physical CDiskTxPos), then
    // confirm the lookup misses even though the legacy row exists.
    // BlockTxPosition offsets are from the block start (header included), while
    // the legacy CDiskTxPos.nTxOffset is measured after the header.
    const CDiskTxPos fake_physical{BlockFilePos(*m_node.chainman, fake_pos.block_seq + 1), fake_pos.tx_offset_in_block - txindex::BLOCK_HEADER_SIZE};
    db.Erase(txindex::DBKey{fake_prefix, fake_pos});
    db.Write(txindex::LegacyTxKey(fake_txid), fake_physical);
    CTransactionRef legacy_tx;
    uint256 block_hash;
    BOOST_CHECK(!txindex.FindTx(fake_txid, block_hash, legacy_tx));

    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_legacy_fallback, TestChain100Setup)
{
    // Seed the on-disk database with a legacy ('t' + txid) entry before the index
    // is opened, as if it had been written by a pre-hashing version.
    const Txid legacy_txid{m_coinbase_txns.front()->GetHash()};
    // The block at height 1 holds only the coinbase, so the tx starts right after
    // the header and the 1-byte tx count.
    const CDiskTxPos legacy_pos{BlockFilePos(*m_node.chainman, 1), 1};
    {
        CDBWrapper db{DBParams{.path = gArgs.GetDataDirNet() / "indexes" / "txindex", .cache_bytes = 1_MiB}};
        db.Write(txindex::LegacyTxKey(legacy_txid), legacy_pos);
    }

    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    // Drop the hashed entries so only the legacy row remains, then confirm the
    // lookup succeeds through the fallback.
    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const auto prefix{txindex::CreateKeyPrefix(ReadHasher(db), legacy_txid)};
    const auto bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE(!bucket.empty());
    for (const auto& pos : bucket) db.Erase(txindex::DBKey{prefix, pos});

    LookupTx(txindex, legacy_txid);

    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_locator_upgrade, TestChain100Setup)
{
    uint256 legacy_hash, new_hash;
    {
        LOCK(cs_main);
        legacy_hash = Assert(m_node.chainman->ActiveChain()[1])->GetBlockHash();
        new_hash = Assert(m_node.chainman->ActiveChain().Tip())->GetBlockHash();
    }
    CBlockLocator legacy_locator{{legacy_hash}}, new_locator{{new_hash}};
    { CDBWrapper{DBParams{.path = gArgs.GetDataDirNet() / "indexes" / "txindex", .cache_bytes = 1_MiB}}.Write(uint8_t{'B'}, legacy_locator); }

    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/false);
    BOOST_CHECK(TxIndexTest::ReadBestBlock(txindex).vHave == legacy_locator.vHave);

    TxIndexTest::WriteBestBlock(txindex, new_locator);
    BOOST_CHECK(TxIndexTest::ReadBestBlock(txindex).vHave == new_locator.vHave);

    CBlockLocator stored_legacy_locator;
    BOOST_REQUIRE(TxIndexTest::GetDB(txindex).Read(uint8_t{'B'}, stored_legacy_locator));
    BOOST_CHECK(stored_legacy_locator.vHave == legacy_locator.vHave);
}

BOOST_FIXTURE_TEST_CASE(txindex_reorg_keeps_stale_entries, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), /*n_cache_size=*/1_MiB, /*f_memory=*/true);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    const CScript coinbase_script{CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG};

    // Mine a unique (non-coinbase) transaction into a new block at height 101.
    CMutableTransaction unique_mtx{CreateValidMempoolTransaction(
        /*input_transaction=*/m_coinbase_txns[0],
        /*input_vout=*/0,
        /*input_height=*/1,
        /*input_signing_key=*/coinbaseKey,
        /*output_destination=*/CScript() << OP_TRUE,
        /*output_amount=*/CAmount{1 * COIN},
        /*submit=*/false)};
    const Txid unique_txid{MakeTransactionRef(unique_mtx)->GetHash()};
    const uint256 stale_block_hash{CreateAndProcessBlock({unique_mtx}, coinbase_script).GetHash()};
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());

    BOOST_CHECK(LookupTx(txindex, unique_txid) == stale_block_hash);

    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const auto prefix{txindex::CreateKeyPrefix(ReadHasher(db), unique_txid)};
    const auto original_bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE_EQUAL(original_bucket.size(), 1U);

    ChainstateManager& chainman{*m_node.chainman};

    // Invalidate the block holding the unique transaction.
    InvalidateBlock(chainman, stale_block_hash);
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());

    // The disconnected transaction is still found, in the now-stale block.
    BOOST_CHECK(LookupTx(txindex, unique_txid) == stale_block_hash);
    {
        LOCK(cs_main);
        const CBlockIndex* stale_index{chainman.m_blockman.LookupBlockIndex(stale_block_hash)};
        BOOST_REQUIRE(stale_index);
        BOOST_CHECK(!chainman.ActiveChain().Contains(*stale_index));
    }

    // Mine the same transaction into a replacement branch, which gets a later
    // sequence number. The lookup must now return the branch block in the active chain.
    const uint256 branch_block_hash{CreateAndProcessBlock({unique_mtx}, CScript() << OP_TRUE).GetHash()};
    CreateAndProcessBlock({}, coinbase_script);
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());
    BOOST_CHECK(LookupTx(txindex, unique_txid) == branch_block_hash);

    // Reorg back to the original branch. The original branch block must be
    // now be preferred even though the replacement branch has a later sequence.
    {
        LOCK(cs_main);
        chainman.ActiveChainstate().ResetBlockFailureFlags(chainman.m_blockman.LookupBlockIndex(stale_block_hash));
    }
    InvalidateBlock(chainman, branch_block_hash);
    {
        BlockValidationState state;
        BOOST_REQUIRE(chainman.ActiveChainstate().ActivateBestChain(state));
    }
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());
    BOOST_CHECK(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip()->GetBlockHash()) == stale_block_hash);

    BOOST_CHECK(LookupTx(txindex, unique_txid) == stale_block_hash);

    // Reconnecting the original block must not create duplicate entries.
    const auto reorg_bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE_EQUAL(reorg_bucket.size(), 2U);
    BOOST_CHECK(reorg_bucket.front() == original_bucket.front());

    txindex.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
