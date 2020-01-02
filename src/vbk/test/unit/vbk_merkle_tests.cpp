#include <boost/test/unit_test.hpp>

#include <algorithm>

#include <chain.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <wallet/wallet.h>

#include "vbk/genesis.hpp"
#include "vbk/init.hpp"
#include "vbk/merkle.hpp"
#include "vbk/service_locator.hpp"
#include "vbk/test/util/mock.hpp"
#include "vbk/test/util/tx.hpp"
#include "vbk/util_service/util_service_impl.hpp"

BOOST_AUTO_TEST_SUITE(vbk_merkle_tests)

struct MerkleFixture {
    // this inits veriblock services
    TestChain100Setup blockchain;
};

BOOST_FIXTURE_TEST_CASE(genesis_block_hash_is_valid, MerkleFixture)
{
    CBlock block = VeriBlock::CreateGenesisBlock(
        1337, 36282504, 0x1d0fffff, 1, 50 * COIN,
        "047c62bbf7f5aa4dd5c16bad99ac621b857fac4e93de86e45f5ada73404eeb44dedcf377b03c14a24e9d51605d9dd2d8ddaef58760d9c4bb82d9c8f06d96e79488",
        "VeriBlock");
    BlockValidationState state;

    bool result = VeriBlock::VerifyTopLevelMerkleRoot(block, state, nullptr);
    BOOST_CHECK(result);
    BOOST_CHECK(state.IsValid());
}

BOOST_FIXTURE_TEST_CASE(TestChain100Setup_has_valid_merkle_roots, MerkleFixture)
{
    SelectParams("regtest");
    BlockValidationState state;
    CBlock block;

    for (int i = 0; i <= 100; i++) {
        CBlockIndex* index = ChainActive()[i];
        BOOST_REQUIRE_MESSAGE(index != nullptr, "can not find block at given height");
        BOOST_REQUIRE_MESSAGE(ReadBlockFromDisk(block, index, Params().GetConsensus()), "can not read block");
        BOOST_CHECK_MESSAGE(VeriBlock::VerifyTopLevelMerkleRoot(block, state, index->pprev), strprintf("merkle root of block %d is invalid", i));
    }
}

BOOST_FIXTURE_TEST_CASE(addPopTransactionRootIntoCoinbaseCommitment_test, MerkleFixture)
{
    VeriBlockTest::ServicesFixture service_fixture;
    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {std::vector<uint8_t>(100, 2)});

    {
        LOCK(cs_main);

        TxValidationState state;
        BOOST_CHECK(AcceptToMemoryPool(mempool, state, MakeTransactionRef(popTx), nullptr, false, DEFAULT_TRANSACTION_MAXFEE));
    }

    CScript scriptPubKey = CScript() << ToByteVector(blockchain.coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    CBlock block = blockchain.CreateAndProcessBlock({popTx}, scriptPubKey);
    CBlockIndex* index = ChainActive().Tip();

    BlockValidationState state;
    BOOST_CHECK(VeriBlock::VerifyTopLevelMerkleRoot(block, state, index->pprev));

    // change pop merkle root
    int commitpos = VeriBlock::GetPopMerkleRootCommitmentIndex(block);
    BOOST_CHECK(commitpos != -1);
    CMutableTransaction tx(*block.vtx[0]);
    tx.vout[0].scriptPubKey[4] = 0xff;
    tx.vout[0].scriptPubKey[5] = 0xff;
    tx.vout[0].scriptPubKey[6] = 0xff;
    tx.vout[0].scriptPubKey[7] = 0xff;
    tx.vout[0].scriptPubKey[8] = 0xff;
    tx.vout[0].scriptPubKey[9] = 0xff;
    tx.vout[0].scriptPubKey[10] = 0xff;
    block.vtx[0] = MakeTransactionRef(tx);

    BOOST_CHECK(!VeriBlock::VerifyTopLevelMerkleRoot(block, state, index->pprev));

    // erase commitment
    tx.vout.erase(tx.vout.begin() + commitpos);
    block.vtx[0] = MakeTransactionRef(tx);

    BOOST_CHECK(!VeriBlock::VerifyTopLevelMerkleRoot(block, state, index->pprev));
}

BOOST_AUTO_TEST_SUITE_END()
