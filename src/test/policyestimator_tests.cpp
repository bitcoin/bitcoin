// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "policy/policy.h"
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
    TestMemPoolEntryHelper entry;
    CAmount basefee(2000);
    CAmount deltaFee(100);
    std::vector<CAmount> feeV;

    // Populate vectors of increasing fees
    for (int j = 0; j < 10; j++) {
        feeV.push_back(basefee * (j+1));
    }

    // Store the hashes of transactions that have been
    // added to the mempool by their associate fee
    // txHashes[j] is populated with transactions either of
    // fee = basefee * (j+1)
    std::vector<uint256> txHashes[10];

    // Create a transaction template
    CScript garbage;
    for (unsigned int i = 0; i < 128; i++)
        garbage.push_back('X');
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = garbage;
    tx.vout.resize(1);
    tx.vout[0].nValue=0LL;
    CFeeRate baseRate(basefee, GetVirtualTransactionSize(tx));

    // Create a fake block
    std::vector<CTransactionRef> block;
    int blocknum = 0;

    // Loop through 200 blocks
    // At a decay .998 and 4 fee transactions per block
    // This makes the tx count about 1.33 per bucket, above the 1 threshold
    while (blocknum < 200) {
        for (int j = 0; j < 10; j++) { // For each fee
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k; // make transaction unique
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(hash, entry.Fee(feeV[j]).Time(GetTime()).Priority(0).Height(blocknum).FromTx(tx, &mpool));
                txHashes[j].push_back(hash);
            }
        }
        //Create blocks where higher fee txs are included more often
        for (int h = 0; h <= blocknum%10; h++) {
            // 10/10 blocks add highest fee transactions
            // 9/10 blocks add 2nd highest and so on until ...
            // 1/10 blocks add lowest fee transactions
            while (txHashes[9-h].size()) {
                CTransactionRef ptx = mpool.get(txHashes[9-h].back());
                if (ptx)
                    block.push_back(ptx);
                txHashes[9-h].pop_back();
            }
        }
        mpool.removeForBlock(block, ++blocknum);
        block.clear();
        if (blocknum == 30) {
            // At this point we should need to combine 5 buckets to get enough data points
            // So estimateFee(1,2,3) should fail and estimateFee(4) should return somewhere around
            // 8*baserate.  estimateFee(4) %'s are 100,100,100,100,90 = average 98%
            BOOST_CHECK(mpool.estimateFee(1) == CFeeRate(0));
            BOOST_CHECK(mpool.estimateFee(2) == CFeeRate(0));
            BOOST_CHECK(mpool.estimateFee(3) == CFeeRate(0));
            BOOST_CHECK(mpool.estimateFee(4).GetFeePerK() < 8*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(mpool.estimateFee(4).GetFeePerK() > 8*baseRate.GetFeePerK() - deltaFee);
            int answerFound;
            BOOST_CHECK(mpool.estimateSmartFee(1, &answerFound) == mpool.estimateFee(4) && answerFound == 4);
            BOOST_CHECK(mpool.estimateSmartFee(3, &answerFound) == mpool.estimateFee(4) && answerFound == 4);
            BOOST_CHECK(mpool.estimateSmartFee(4, &answerFound) == mpool.estimateFee(4) && answerFound == 4);
            BOOST_CHECK(mpool.estimateSmartFee(8, &answerFound) == mpool.estimateFee(8) && answerFound == 8);
        }
    }

    std::vector<CAmount> origFeeEst;
    // Highest feerate is 10*baseRate and gets in all blocks,
    // second highest feerate is 9*baseRate and gets in 9/10 blocks = 90%,
    // third highest feerate is 8*base rate, and gets in 8/10 blocks = 80%,
    // so estimateFee(1) would return 10*baseRate but is hardcoded to return failure
    // Second highest feerate has 100% chance of being included by 2 blocks,
    // so estimateFee(2) should return 9*baseRate etc...
    for (int i = 1; i < 10;i++) {
        origFeeEst.push_back(mpool.estimateFee(i).GetFeePerK());
        if (i > 2) { // Fee estimates should be monotonically decreasing
            BOOST_CHECK(origFeeEst[i-1] <= origFeeEst[i-2]);
        }
        int mult = 11-i;
        if (i > 1) {
            BOOST_CHECK(origFeeEst[i-1] < mult*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(origFeeEst[i-1] > mult*baseRate.GetFeePerK() - deltaFee);
        }
        else {
            BOOST_CHECK(origFeeEst[i-1] == CFeeRate(0).GetFeePerK());
        }
    }

    // Mine 50 more blocks with no transactions happening, estimates shouldn't change
    // We haven't decayed the moving average enough so we still have enough data points in every bucket
    while (blocknum < 250)
        mpool.removeForBlock(block, ++blocknum);

    BOOST_CHECK(mpool.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() < origFeeEst[i-1] + deltaFee);
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }


    // Mine 15 more blocks with lots of transactions happening and not getting mined
    // Estimates should go up
    while (blocknum < 265) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(hash, entry.Fee(feeV[j]).Time(GetTime()).Priority(0).Height(blocknum).FromTx(tx, &mpool));
                txHashes[j].push_back(hash);
            }
        }
        mpool.removeForBlock(block, ++blocknum);
    }

    int answerFound;
    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(mpool.estimateFee(i) == CFeeRate(0) || mpool.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
        BOOST_CHECK(mpool.estimateSmartFee(i, &answerFound).GetFeePerK() > origFeeEst[answerFound-1] - deltaFee);
    }

    // Mine all those transactions
    // Estimates should still not be below original
    for (int j = 0; j < 10; j++) {
        while(txHashes[j].size()) {
            CTransactionRef ptx = mpool.get(txHashes[j].back());
            if (ptx)
                block.push_back(ptx);
            txHashes[j].pop_back();
        }
    }
    mpool.removeForBlock(block, 265);
    block.clear();
    BOOST_CHECK(mpool.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }

    // Mine 200 more blocks where everything is mined every block
    // Estimates should be below original estimates
    while (blocknum < 465) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(hash, entry.Fee(feeV[j]).Time(GetTime()).Priority(0).Height(blocknum).FromTx(tx, &mpool));
                CTransactionRef ptx = mpool.get(hash);
                if (ptx)
                    block.push_back(ptx);

            }
        }
        mpool.removeForBlock(block, ++blocknum);
        block.clear();
    }
    BOOST_CHECK(mpool.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10; i++) {
        BOOST_CHECK(mpool.estimateFee(i).GetFeePerK() < origFeeEst[i-1] - deltaFee);
    }

    // Test that if the mempool is limited, estimateSmartFee won't return a value below the mempool min fee
    // and that estimateSmartPriority returns essentially an infinite value
    mpool.addUnchecked(tx.GetHash(),  entry.Fee(feeV[5]).Time(GetTime()).Priority(0).Height(blocknum).FromTx(tx, &mpool));
    // evict that transaction which should set a mempool min fee of minRelayTxFee + feeV[5]
    mpool.TrimToSize(1);
    BOOST_CHECK(mpool.GetMinFee(1).GetFeePerK() > feeV[5]);
    for (int i = 1; i < 10; i++) {
        BOOST_CHECK(mpool.estimateSmartFee(i).GetFeePerK() >= mpool.estimateFee(i).GetFeePerK());
        BOOST_CHECK(mpool.estimateSmartFee(i).GetFeePerK() >= mpool.GetMinFee(1).GetFeePerK());
        BOOST_CHECK(mpool.estimateSmartPriority(i) == INF_PRIORITY);
    }
}

BOOST_AUTO_TEST_SUITE_END()
