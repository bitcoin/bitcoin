#include <pos/stake.h>
#include <chain.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(stake_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(valid_kernel)
{
    // Construct a dummy previous block index
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    // Previous block containing the stake input
    CBlock block_from;
    block_from.nTime = 50;

    // Transaction being staked
    CMutableTransaction mtx_prev;
    mtx_prev.nTime = 50;
    mtx_prev.vout.emplace_back(CAmount(1000), CScript());
    CTransactionRef tx_prev = MakeTransactionRef(mtx_prev);

    COutPoint prevout{tx_prev->GetHash(), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x207fffff; // very low difficulty
    unsigned int nTimeTx = 150;      // coin age 100 seconds

    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, block_from, 0, tx_prev,
                                     prevout, nTimeTx, hash_proof, false));
}

BOOST_AUTO_TEST_CASE(invalid_kernel_time)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    CBlock block_from;
    block_from.nTime = 200;

    CMutableTransaction mtx_prev;
    mtx_prev.nTime = 200;
    mtx_prev.vout.emplace_back(CAmount(1000), CScript());
    CTransactionRef tx_prev = MakeTransactionRef(mtx_prev);

    COutPoint prevout{tx_prev->GetHash(), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = 150; // earlier than block time

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, block_from, 0, tx_prev,
                                      prevout, nTimeTx, hash_proof, false));
}

BOOST_AUTO_TEST_CASE(invalid_kernel_target)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    CBlock block_from;
    block_from.nTime = 50;

    CMutableTransaction mtx_prev;
    mtx_prev.nTime = 50;
    mtx_prev.vout.emplace_back(CAmount(1), CScript());
    CTransactionRef tx_prev = MakeTransactionRef(mtx_prev);

    COutPoint prevout{tx_prev->GetHash(), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x1;     // extremely high difficulty
    unsigned int nTimeTx = 60;    // minimal age

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, block_from, 0, tx_prev,
                                      prevout, nTimeTx, hash_proof, false));
}

BOOST_AUTO_TEST_SUITE_END()
