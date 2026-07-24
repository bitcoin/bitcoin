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
#include <util/strencodings.h>
#include <validation.h>

#include <cstdint>
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
    static CDBWrapper& GetDB(const TxIndex& txindex) { return static_cast<CDBWrapper&>(txindex.GetDB()); }
};

namespace {

PresaltedSipHasher ReadHasher(const CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    BOOST_REQUIRE(db.Read(std::string{"txid_hash_salt"}, salt));
    return PresaltedSipHasher{salt.first, salt.second};
}

std::vector<txindex::BlockTxPosition> BucketPositions(CDBWrapper& db, const txindex::TxHashKeyPrefix& prefix)
{
    std::vector<txindex::BlockTxPosition> positions;
    std::unique_ptr<CDBIterator> it{db.NewIterator()};
    txindex::DBKey key{prefix, {}};
    for (it->Seek(std::pair{txindex::DB_TXINDEX_HASHED, prefix}); it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
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
}

BOOST_FIXTURE_TEST_CASE(txindex_initial_sync, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
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
        if (!txindex.FindTx(txn->GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn->GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // Check that new transactions in new blocks make it into the index.
    for (int i = 0; i < 10; i++) {
        CScript coinbase_script_pub_key = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
        std::vector<CMutableTransaction> no_txns;
        const CBlock& block = CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
        const CTransaction& txn = *block.vtx[0];

        BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
        if (!txindex.FindTx(txn.GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn.GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // shutdown sequence (c.f. Shutdown() in init.cpp)
    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_collision_scan_path, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const PresaltedSipHasher hasher{ReadHasher(db)};

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

    db.Write(txindex::DBKey{target_prefix, fake_pos}, "");

    // The target's bucket now holds the real target first (lower sequence
    // number), then the forged false positive, which the descending scan tries first.
    const auto target_bucket{BucketPositions(db, target_prefix)};
    BOOST_REQUIRE_EQUAL(target_bucket.size(), 2U);
    BOOST_CHECK(target_bucket[0] != fake_pos);
    BOOST_CHECK(target_bucket[1] == fake_pos);

    CTransactionRef tx_disk;
    uint256 block_hash;
    BOOST_REQUIRE(txindex.FindTx(target_txid, block_hash, tx_disk));
    BOOST_REQUIRE(tx_disk);
    BOOST_CHECK(tx_disk->GetHash() == target_txid);

    // A database created fresh by this version cannot contain legacy entries, so
    // lookups skip the legacy fallback: drop the last coinbase's hashed entry and
    // re-add it under the old 't' + txid schema (a physical CDiskTxPos), then
    // confirm the lookup misses even though the legacy row exists.
    // BlockTxPosition offsets are from the block start (header included), while
    // the legacy CDiskTxPos.nTxOffset is measured after the header.
    const CDiskTxPos fake_physical{BlockFilePos(*m_node.chainman, fake_pos.block_seq + 1), fake_pos.tx_offset_in_block - static_cast<uint32_t>(GetSerializeSize(CBlockHeader{}))};
    db.Erase(txindex::DBKey{fake_prefix, fake_pos});
    db.Write(std::make_pair(static_cast<uint8_t>('t'), fake_txid.ToUint256()), fake_physical);
    CTransactionRef legacy_tx;
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
        db.Write(std::make_pair(static_cast<uint8_t>('t'), legacy_txid.ToUint256()), legacy_pos);
    }

    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, /*f_memory=*/false);
    BOOST_REQUIRE(txindex.Init());
    txindex.Sync();

    // Drop the hashed entries so only the legacy row remains, then confirm the
    // lookup succeeds through the fallback.
    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const auto prefix{txindex::CreateKeyPrefix(ReadHasher(db), legacy_txid)};
    const auto bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE(!bucket.empty());
    for (const auto& pos : bucket) db.Erase(txindex::DBKey{prefix, pos});

    CTransactionRef tx_disk;
    uint256 block_hash;
    BOOST_REQUIRE(txindex.FindTx(legacy_txid, block_hash, tx_disk));
    BOOST_REQUIRE(tx_disk);
    BOOST_CHECK(tx_disk->GetHash() == legacy_txid);

    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_reorg_keeps_stale_entries, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), 1_MiB, true);
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

    CTransactionRef tx_disk;
    uint256 block_hash;
    BOOST_REQUIRE(txindex.FindTx(unique_txid, block_hash, tx_disk));
    BOOST_CHECK(tx_disk->GetHash() == unique_txid);
    BOOST_CHECK(block_hash == stale_block_hash);

    CDBWrapper& db{TxIndexTest::GetDB(txindex)};
    const auto prefix{txindex::CreateKeyPrefix(ReadHasher(db), unique_txid)};
    const auto original_bucket{BucketPositions(db, prefix)};
    BOOST_REQUIRE_EQUAL(original_bucket.size(), 1U);

    ChainstateManager& chainman{*m_node.chainman};

    // Invalidate the block holding the unique transaction, then mine a longer branch.
    {
        CBlockIndex* tip{WITH_LOCK(cs_main, return chainman.ActiveChain().Tip())};
        BlockValidationState state;
        BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, tip));
    }
    const uint256 branch_block_hash{CreateAndProcessBlock({}, coinbase_script).GetHash()};
    CreateAndProcessBlock({}, coinbase_script);
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());

    // The disconnected transaction is still found, in the now-stale block.
    BOOST_REQUIRE(txindex.FindTx(unique_txid, block_hash, tx_disk));
    BOOST_CHECK(tx_disk->GetHash() == unique_txid);
    BOOST_CHECK(block_hash == stale_block_hash);
    {
        LOCK(cs_main);
        const CBlockIndex* stale_index{chainman.m_blockman.LookupBlockIndex(block_hash)};
        BOOST_REQUIRE(stale_index);
        BOOST_CHECK(!chainman.ActiveChain().Contains(*stale_index));
    }

    // Reorg back to the original branch by reconsidering the stale block and
    // invalidating the replacement branch. Reconnecting the block must not
    // create duplicate entries, since it keeps its original sequence number.
    {
        LOCK(cs_main);
        chainman.ActiveChainstate().ResetBlockFailureFlags(chainman.m_blockman.LookupBlockIndex(stale_block_hash));
    }
    {
        CBlockIndex* branch_index{WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(branch_block_hash))};
        BlockValidationState state;
        BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, branch_index));
        BOOST_REQUIRE(chainman.ActiveChainstate().ActivateBestChain(state));
    }
    BOOST_REQUIRE(txindex.BlockUntilSyncedToCurrentChain());
    BOOST_CHECK(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip()->GetBlockHash()) == stale_block_hash);

    // The transaction is found in the reconnected (again active) block, and its
    // bucket keeps the original position.
    BOOST_REQUIRE(txindex.FindTx(unique_txid, block_hash, tx_disk));
    BOOST_CHECK(tx_disk->GetHash() == unique_txid);
    BOOST_CHECK(block_hash == stale_block_hash);

    BOOST_CHECK(BucketPositions(db, prefix) == original_bucket);

    txindex.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
