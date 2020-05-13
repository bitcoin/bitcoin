// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
#include "vbk/test/util/e2e_fixture.hpp"
#include "vbk/test/util/tx.hpp"

BOOST_AUTO_TEST_SUITE(vbk_merkle_tests)

struct MerkleFixture : public E2eFixture {
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
    auto block = endorseAltBlockAndMine(ChainActive().Tip()->GetBlockHash());
    LOCK(cs_main);
    auto index = LookupBlockIndex(block.GetHash());


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
    BOOST_CHECK(state.GetRejectReason() == "bad-txnmrklroot");

    // erase commitment
    tx.vout.erase(tx.vout.begin() + commitpos);
    block.vtx[0] = MakeTransactionRef(tx);

    BOOST_CHECK(!VeriBlock::VerifyTopLevelMerkleRoot(block, state, index->pprev));
    BOOST_CHECK(state.GetRejectReason() == "bad-txnmrklroot");
}

BOOST_FIXTURE_TEST_CASE(Regression, MerkleFixture){
    auto block = endorseAltBlockAndMine(ChainActive().Tip()->GetBlockHash());
    LOCK(cs_main);
    auto index = LookupBlockIndex(block.GetHash());
    BlockValidationState state;
    BOOST_CHECK(VeriBlock::VerifyTopLevelMerkleRoot(block, state, index->pprev));

    using namespace VeriBlock;

    auto topLevel = VeriBlock::TopLevelMerkleRoot(ChainActive()[0], block, nullptr);
    auto keystones = VeriBlock::getKeystoneHashesForTheNextBlock(ChainActive()[0]);
    auto mroot = BlockMerkleRoot(block, nullptr);
    auto height = 1;
    auto context = VeriBlock::ContextInfoContainer(height, keystones, mroot);

    std::cout << "topLevel1 : " << topLevel.GetHex() << "\n";
    std::cout << "topLevel2 : " << context.getTopLevelMerkleRoot().GetHex() << "\n";
    std::cout << "keystone1 : " << keystones[0].GetHex() << "\n";
    std::cout << "keystone2 : " << keystones[1].GetHex() << "\n";
    std::cout << "mroot     : " << mroot.GetHex() << "\n";
    std::cout << "height    : " << height << "\n";
    std::cout << "unauthd   : " << HexStr(context.getUnauthenticated()) << "\n";
    std::cout << "unauthdh  : " << context.getUnauthenticatedHash().GetHex() << "\n";

}

BOOST_AUTO_TEST_SUITE_END()
