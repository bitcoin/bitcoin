#include <pos/stake.h>
#include <chain.h>
#include <consensus/amount.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <hash.h>
#include <script/script.h>
#include <validation.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <memory>

BOOST_FIXTURE_TEST_SUITE(stake_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(valid_kernel)
{
    // Construct a dummy previous block index
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 100 * COIN;
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x207fffff; // very low difficulty
    unsigned int nTimeTx = nTimeBlockFrom + MIN_STAKE_AGE; // exactly minimum age

    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, hash_proof, false));
}

BOOST_AUTO_TEST_CASE(invalid_kernel_time)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 100 * COIN;
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE - 16; // not old enough and masked

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, nTimeTx, hash_proof, false));
}

BOOST_AUTO_TEST_CASE(invalid_kernel_target)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 1 * COIN; // minimum stake amount
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x1;     // extremely high difficulty
    unsigned int nTimeTx = MIN_STAKE_AGE;    // minimal age

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, nTimeTx, hash_proof, false));
}

BOOST_AUTO_TEST_CASE(kernel_hash_matches_expectation)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 100 * COIN;
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE;

    uint256 expected_modifier;
    {
        HashWriter ss_mod;
        ss_mod << prev_index.GetBlockHash() << prevout.hash << prevout.n;
        expected_modifier = ss_mod.GetHash();
    }

    uint256 expected_hash;
    {
        HashWriter ss_kernel;
        ss_kernel << expected_modifier << nTimeBlockFrom << prevout.hash << prevout.n
                  << nTimeTx;
        expected_hash = ss_kernel.GetHash();
    }

    uint256 hash_proof;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, hash_proof, false));
    BOOST_CHECK_EQUAL(hash_proof, expected_hash);
}

BOOST_AUTO_TEST_CASE(stake_modifier_differs_per_input)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE;

    COutPoint prevout1{Txid::FromUint256(uint256{3}), 0};
    COutPoint prevout2{Txid::FromUint256(uint256{4}), 1};

    uint256 proof1;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     100 * COIN, prevout1, nTimeTx, proof1, false));
    uint256 proof2;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     100 * COIN, prevout2, nTimeTx, proof2, false));
    BOOST_CHECK(proof1 != proof2);
}

BOOST_AUTO_TEST_CASE(height1_requires_coinstake)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 0;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);
    Consensus::Params params;

    CBlock block;
    block.nTime = 16;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));

    BOOST_CHECK(!ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_AUTO_TEST_CASE(valid_height1_coinstake)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 0;
    prev_index.nBits = 0x207fffff;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);

    // Add a mature coin to the view
    COutPoint prevout{Txid::FromUint256(uint256{2}), 0};
    Coin coin;
    coin.out.nValue = 1 * COIN;
    coin.nHeight = 0;
    view.AddCoin(prevout, std::move(coin), false);

    CBlock block;
    block.nTime = MIN_STAKE_AGE + 16;
    block.nBits = 0x207fffff;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(coinbase));

    CMutableTransaction coinstake;
    coinstake.nLockTime = 1;
    coinstake.vin.emplace_back(prevout);
    coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = 1 * COIN;
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));

    Consensus::Params params;
    BOOST_CHECK(ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_AUTO_TEST_CASE(reject_low_stake_amount)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 0;
    prev_index.nBits = 0x207fffff;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);

    COutPoint prevout{Txid::FromUint256(uint256{2}), 0};
    Coin coin;
    coin.out.nValue = COIN / 2; // below minimum stake amount
    coin.nHeight = 0;
    view.AddCoin(prevout, std::move(coin), false);

    CBlock block;
    block.nTime = MIN_STAKE_AGE + 16;
    block.nBits = 0x207fffff;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(coinbase));

    CMutableTransaction coinstake;
    coinstake.nLockTime = 1;
    coinstake.vin.emplace_back(prevout);
    coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = COIN / 2;
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));

    Consensus::Params params;
    BOOST_CHECK(!ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_AUTO_TEST_CASE(height1_allows_young_coinstake)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 16; // young genesis time
    prev_index.nBits = 0x207fffff;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);

    // Add a coin from the genesis block
    COutPoint prevout{Txid::FromUint256(uint256{2}), 0};
    Coin coin;
    coin.out.nValue = 1 * COIN;
    coin.nHeight = 0;
    view.AddCoin(prevout, std::move(coin), false);

    CBlock block;
    block.nTime = MIN_STAKE_AGE; // younger than required age relative to prev_index
    block.nBits = 0x207fffff;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(coinbase));

    CMutableTransaction coinstake;
    coinstake.nLockTime = 1;
    coinstake.vin.emplace_back(prevout);
    coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = 1 * COIN;
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));

    Consensus::Params params;
    BOOST_CHECK(ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_FIXTURE_TEST_CASE(reject_pow_after_height1, ChainTestingSetup)
{
    g_chainman = m_node.chainman.get();

    uint256 prev_hash{1};
    auto prev_index = std::make_unique<CBlockIndex>();
    prev_index->nHeight = 1;
    prev_index->nBits = 0x207fffff;
    prev_index->phashBlock = &prev_hash;
    {
        LOCK(cs_main);
        g_chainman->BlockIndex().emplace(prev_hash, prev_index.get());
    }

    CBlock block;
    block.hashPrevBlock = prev_hash;
    block.nBits = prev_index->nBits;
    block.nTime = 2;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;
    BOOST_CHECK(!CheckBlock(block, state, Params().GetConsensus()));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-pow");

    {
        LOCK(cs_main);
        g_chainman->BlockIndex().erase(prev_hash);
    }
    g_chainman = nullptr;
}

BOOST_FIXTURE_TEST_CASE(process_new_block_rejects_pow_height2, ChainTestingSetup)
{
    g_chainman = m_node.chainman.get();

    uint256 prev_hash{1};
    auto prev_index = std::make_unique<CBlockIndex>();
    prev_index->nHeight = 1;
    prev_index->nBits = 0x207fffff;
    prev_index->phashBlock = &prev_hash;
    {
        LOCK(cs_main);
        g_chainman->BlockIndex().emplace(prev_hash, prev_index.get());
    }

    CBlock block;
    block.hashPrevBlock = prev_hash;
    block.nBits = prev_index->nBits;
    block.nTime = 2;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
    block.hashMerkleRoot = BlockMerkleRoot(block);

    bool new_block{false};
    BOOST_CHECK(!g_chainman->ProcessNewBlock(std::make_shared<const CBlock>(block),
                                             /*force_processing=*/true, /*min_pow_checked=*/true, &new_block));

    {
        LOCK(cs_main);
        g_chainman->BlockIndex().erase(prev_hash);
    }
    g_chainman = nullptr;
}

BOOST_AUTO_TEST_SUITE_END()
