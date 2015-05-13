// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "policy/fees.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(policyestimator_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(BlockPolicyEstimates)
{
    CTxMemPool mpool(CFeeRate(1000));
    CAmount basefee(2000);
    double basepri = 1e6;
    CAmount deltaFee(100);
    double deltaPri=5e5;
    std::vector<CAmount> feeV[2];
    std::vector<double> priV[2];

    // Populate vectors of increasing fees or priorities
    for (int j = 0; j < 10; j++) {
        //V[0] is for fee transactions
        feeV[0].push_back(basefee * (j+1));
        priV[0].push_back(0);
        //V[1] is for priority transactions
        feeV[1].push_back(CAmount(0));
        priV[1].push_back(basepri * pow(10, j+1));
    }

    // Store the hashes of transactions that have been
    // added to the mempool by their associate fee/pri
    // txHashes[j] is populated with transactions either of
    // fee = basefee * (j+1)  OR  pri = 10^6 * 10^(j+1)
    std::vector<uint256> txHashes[10];

    // Create a transaction template
    CScript garbage;
    for (unsigned int i = 0; i < 128; i++)
        garbage.push_back('X');
    CMutableTransaction tx;
    std::list<CTransaction> dummyConflicted;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = garbage;
    tx.vout.resize(1);
    tx.vout[0].nValue=0LL;
    CFeeRate baseRate(basefee, ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION));

    // Create a fake block
    std::vector<CTransaction> block;
    int blocknum = 0;

    // Loop through 200 blocks
    // At a decay .998 and 4 fee transactions per block
    // This makes the tx count about 1.33 per bucket, above the 1 threshold
    while (blocknum < 200) {
        for (int j = 0; j < 10; j++) { // For each fee/pri multiple
            for (int k = 0; k < 5; k++) { // add 4 fee txs for every priority tx
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k; // make transaction unique
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(hash, CTxMemPoolEntry(tx, feeV[k/4][j], GetTime(), priV[k/4][j], blocknum, mpool.HasNoInputsOf(tx)));
                txHashes[j].push_back(hash);
            }
        }
        //Create blocks where higher fee/pri txs are included more often
        for (int h = 0; h <= blocknum%10; h++) {
            // 10/10 blocks add highest fee/pri transactions
            // 9/10 blocks add 2nd highest and so on until ...
            // 1/10 blocks add lowest fee/pri transactions
            while (txHashes[9-h].size()) {
                CTransaction btx;
                if (mpool.lookup(txHashes[9-h].back(), btx))
                    block.push_back(btx);
                txHashes[9-h].pop_back();
            }
        }
        mpool.removeForBlock(block, ++blocknum, dummyConflicted);
        block.clear();
        if (blocknum == 30) {
            // At this point we should need to combine 5 buckets to get enough data points
            // So estimateFee(1) should fail and estimateFee(2) should return somewhere around
            // 8*baserate
            BOOST_CHECK(mpool.estimateFee(1) == CFeeRate(0));
            BOOST_CHECK(mpool.estimateFee(2).GetFeePerK() < 8*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(mpool.estimateFee(2).GetFeePerK() > 8*baseRate.GetFeePerK() - deltaFee);
        }
    }

    std::vector<CAmount> origFeeEst;
    std::vector<double> origPriEst;
    // Highest feerate is 10*baseRate and gets in all blocks,
    // second highest feerate is 9*baseRate and gets in 9/10 blocks = 90%,
    // third highest feerate is 8*base rate, and gets in 8/10 blocks = 80%,
    // so estimateFee(1) should return 9*baseRate.
    // Third highest feerate has 90% chance of being included by 2 blocks,
    // so estimateFee(2) should return 8*baseRate etc...
    for (int i = 1; i < 10;i++) {
        origFeeEst.push_back(mpool.estimateFee(i).GetFeePerK());
        origPriEst.push_back(mpool.estimatePriority(i));
        if (i > 1) { // Fee estimates should be monotonically decreasing
            BOOST_CHECK(origFeeEst[i-1] <= origFeeEst[i-2]);
            BOOST_CHECK(origPriEst[i-1] <= origPriEst[i-2]);
        }
        BOOST_CHECK(origFeeEst[i-1] < (10-i)*baseRate.GetFeePerK() + deltaFee);
        BOOST_CHECK(origFeeEst[i-1] > (10-i)*baseRate.GetFeePerK() - deltaFee);
        BOOST_CHECK(origPriEst[i-1] < pow(10,10-i) * basepri + deltaPri);
        BOOST_CHECK(origPriEst[i-1] > pow(10,10-i) * basepri - deltaPri);
    }

    // Mine 50 more blocks with no transactions happening, estimates shouldn't change
    // We haven't decayed the moving average enough so we still have enough data points in every bucket
    while (blocknum < 250)
        mpool.removeForBlock(block, ++blocknum, dummyConflicted);

    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() < origFeeEst[i-1] + deltaFee);
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
        BOOST_CHECK(mpool.estimatePriority(i) < origPriEst[i-1] + deltaPri);
        BOOST_CHECK(mpool.estimatePriority(i) > origPriEst[i-1] - deltaPri);
    }


    // Mine 15 more blocks with lots of transactions happening and not getting mined
    // Estimates should go up
    while (blocknum < 265) {
        for (int j = 0; j < 10; j++) { // For each fee/pri multiple
            for (int k = 0; k < 5; k++) { // add 4 fee txs for every priority tx
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(hash, CTxMemPoolEntry(tx, feeV[k/4][j], GetTime(), priV[k/4][j], blocknum, mpool.HasNoInputsOf(tx)));
                txHashes[j].push_back(hash);
            }
        }
        mpool.removeForBlock(block, ++blocknum, dummyConflicted);
    }

    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
        BOOST_CHECK(mpool.estimatePriority(i) > origPriEst[i-1] - deltaPri);
    }

    // Mine all those transactions
    // Estimates should still not be below original
    for (int j = 0; j < 10; j++) {
        while(txHashes[j].size()) {
            CTransaction btx;
            if (mpool.lookup(txHashes[j].back(), btx))
                block.push_back(btx);
            txHashes[j].pop_back();
        }
    }
    mpool.removeForBlock(block, 265, dummyConflicted);
    block.clear();
    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
        BOOST_CHECK(mpool.estimatePriority(i) > origPriEst[i-1] - deltaPri);
    }

    // Mine 100 more blocks where everything is mined every block
    // Estimates should be below original estimates (not possible for last estimate)
    while (blocknum < 365) {
        for (int j = 0; j < 10; j++) { // For each fee/pri multiple
            for (int k = 0; k < 5; k++) { // add 4 fee txs for every priority tx
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(hash, CTxMemPoolEntry(tx, feeV[k/4][j], GetTime(), priV[k/4][j], blocknum, mpool.HasNoInputsOf(tx)));
                CTransaction btx;
                if (mpool.lookup(hash, btx))
                    block.push_back(btx);
            }
        }
        mpool.removeForBlock(block, ++blocknum, dummyConflicted);
        block.clear();
    }
    for (int i = 1; i < 9; i++) {
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() < origFeeEst[i-1] - deltaFee);
        BOOST_CHECK(mpool.estimatePriority(i) < origPriEst[i-1] - deltaPri);
    }
}

BOOST_AUTO_TEST_SUITE_END()
