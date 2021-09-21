// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees.h>
#include <policy/policy.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/time.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(policyestimator_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(BlockPolicyEstimates)
{
    CBlockPolicyEstimator feeEst;
    CTxMemPool mpool(&feeEst);
    LOCK2(cs_main, mpool.cs);
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
    CFeeRate baseRate(basefee, GetVirtualTransactionSize(CTransaction(tx)));

    // Create a fake block
    std::vector<CTransactionRef> block;
    int blocknum = 0;

    // Loop through 200 blocks
    // At a decay .9952 and 4 fee transactions per block
    // This makes the tx count about 2.5 per bucket, well above the 0.1 threshold
    while (blocknum < 200) {
        for (int j = 0; j < 10; j++) { // For each fee
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k; // make transaction unique
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(entry.Fee(feeV[j]).Time(GetTime()).Height(blocknum).FromTx(tx));
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
        // Check after just a few txs that combining buckets works as expected
        if (blocknum == 3) {
            // At this point we should need to combine 3 buckets to get enough data points
            // So estimateFee(1) should fail and estimateFee(2) should return somewhere around
            // 9*baserate.  estimateFee(2) %'s are 100,100,90 = average 97%
            BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
            BOOST_CHECK(feeEst.estimateFee(2).GetFeePerK() < 9*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(feeEst.estimateFee(2).GetFeePerK() > 9*baseRate.GetFeePerK() - deltaFee);
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
        origFeeEst.push_back(feeEst.estimateFee(i).GetFeePerK());
        if (i > 2) { // Fee estimates should be monotonically decreasing
            BOOST_CHECK(origFeeEst[i-1] <= origFeeEst[i-2]);
        }
        int mult = 11-i;
        if (i % 2 == 0) { //At scale 2, test logic is only correct for even targets
            BOOST_CHECK(origFeeEst[i-1] < mult*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(origFeeEst[i-1] > mult*baseRate.GetFeePerK() - deltaFee);
        }
    }
    // Fill out rest of the original estimates
    for (int i = 10; i <= 48; i++) {
        origFeeEst.push_back(feeEst.estimateFee(i).GetFeePerK());
    }

    // Mine 50 more blocks with no transactions happening, estimates shouldn't change
    // We haven't decayed the moving average enough so we still have enough data points in every bucket
    while (blocknum < 250)
        mpool.removeForBlock(block, ++blocknum);

    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() < origFeeEst[i-1] + deltaFee);
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }


    // Mine 15 more blocks with lots of transactions happening and not getting mined
    // Estimates should go up
    while (blocknum < 265) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(entry.Fee(feeV[j]).Time(GetTime()).Height(blocknum).FromTx(tx));
                txHashes[j].push_back(hash);
            }
        }
        mpool.removeForBlock(block, ++blocknum);
    }

    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i) == CFeeRate(0) || feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
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
    mpool.removeForBlock(block, 266);
    block.clear();
    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i) == CFeeRate(0) || feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }

    // Mine 400 more blocks where everything is mined every block
    // Estimates should be below original estimates
    while (blocknum < 665) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                uint256 hash = tx.GetHash();
                mpool.addUnchecked(entry.Fee(feeV[j]).Time(GetTime()).Height(blocknum).FromTx(tx));
                CTransactionRef ptx = mpool.get(hash);
                if (ptx)
                    block.push_back(ptx);

            }
        }
        mpool.removeForBlock(block, ++blocknum);
        block.clear();
    }
    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 9; i++) { // At 9, the original estimate was already at the bottom (b/c scale = 2)
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() < origFeeEst[i-1] - deltaFee);
    }
}

BOOST_AUTO_TEST_CASE(BlockPolicyEstimatesSegwitCpfp)
{
    CBlockPolicyEstimator fee_est;
    CTxMemPool mpool(&fee_est);
    LOCK2(cs_main, mpool.cs);

    // A transaction template spending a P2WPKH and paying to a P2WPKH
    CMutableTransaction tx;
    tx.nVersion = 1;
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript();
    tx.vin[0].scriptWitness.stack.push_back(ParseHex("3045022100d743c23d7e5f8c9ee792533d333f60c03a5fda7b97d781dc042305e215e34d290220339ff8710fa0bc74e02b36cd960be8d42b0e542f1822659e46107054e243304601"));
    tx.vin[0].scriptWitness.stack.push_back(ParseHex("0214c20e48bb2c90d880aa0c1acb65a666607c7b3f72da399b60da9eb91bcf787d"));
    tx.vout.resize(1);
    tx.vout[0].nValue = 0LL;
    tx.vout[0].scriptPubKey = CScript() << OP_0 << ParseHex("ffffffffffffffffffffffffffffffffffffffff");
    // Another template for a child that will spend 4 P2PWPKH instead
    CMutableTransaction child_tx{tx};
    child_tx.vin.resize(4);
    for (CTxIn& txin : child_tx.vin) {
        txin = tx.vin[0];
    }

    // We'll use low fee parents, higher fee standalone txs and child transactions
    // bumping the low fee parents above the standalone tx feerate.
    TestMemPoolEntryHelper entry;
    CAmount fee_parents(1000), fee_childs(39000), fee_childless(10000);
    CFeeRate feerate_parents(fee_parents, GetVirtualTransactionSize(CTransaction(tx)));
    CFeeRate feerate_childs(fee_childs, GetVirtualTransactionSize(CTransaction(tx)));
    CFeeRate feerate_package(fee_parents + fee_childs, GetVirtualTransactionSize(CTransaction(tx)) * 2);
    CFeeRate feerate_childless(fee_childless, GetVirtualTransactionSize(CTransaction(tx)));

    std::vector<CTransactionRef> block;
    unsigned blocknum = 0;
    std::vector<uint256> parents_hashes;

    // Used to make parent and standalone txs uniques
    unsigned txuid = 0;

    // Mine 20 blocks where both low and higher fee txs are broadcast but only
    // higher fees are mined.
    while (blocknum < 20) {
        for (unsigned i = 0; i < 10; i++) {
            tx.vin[0].prevout.n = ++txuid;
            mpool.addUnchecked(entry.Fee(fee_childless).Time(GetTime()).Height(blocknum).FromTx(tx));
            block.push_back(mpool.get(tx.GetHash()));

            for (unsigned j = 0; j < 2; j++) {
                tx.vin[0].prevout.n = ++txuid;
                mpool.addUnchecked(entry.Fee(fee_parents).Time(GetTime()).Height(blocknum).FromTx(tx));
                parents_hashes.push_back(tx.GetHash());
            }
        }
        mpool.removeForBlock(block, ++blocknum);
        block.clear();
    }
    BOOST_CHECK(fee_est.estimateFee(10).GetFeePerK() == feerate_childless.GetFeePerK());

    // Now continue mining higher fee transactions, but get the low fee tx to be included
    // thanks to CPFP.
    // Our estimate should increase toward the package feerate value.
    while (blocknum < 60) {
        for (unsigned i = 0; i < 50; i++) {
            tx.vin[0].prevout.SetNull();
            tx.vin[0].prevout.n = ++txuid;
            mpool.addUnchecked(entry.Fee(fee_childless).Time(GetTime()).Height(blocknum).FromTx(tx));
            block.push_back(mpool.get(tx.GetHash()));

            for (unsigned j = 0; j < 2; j++) {
                // Bump the parents' fees until there is no more.
                uint256 parent_txid;
                if (parents_hashes.size() > 0) {
                    parent_txid = parents_hashes.back();
                    parents_hashes.pop_back();
                } else {
                    tx.vin[0].prevout.SetNull();
                    tx.vin[0].prevout.n = ++txuid;
                    mpool.addUnchecked(entry.Fee(fee_parents).Time(GetTime()).Height(blocknum).FromTx(tx));
                    block.push_back(mpool.get(tx.GetHash()));
                    parent_txid = tx.GetHash();
                }

                tx.vin[0].prevout.n = 0;
                tx.vin[0].prevout.hash = parent_txid;
                CTxMemPool::txiter parent_iter = *mpool.GetIter(parent_txid);
                CTxMemPool::setEntries parents_set{parent_iter};
                // false as it has an unconfirmed parents, see `validForFeeEstimation`
                // in validation.
                mpool.addUnchecked(entry.Fee(fee_childs).Time(GetTime()).Height(blocknum).FromTx(tx), false);
                block.push_back(mpool.get(tx.GetHash()));
                block.push_back(mpool.get(parent_txid));
            }
        }
        mpool.removeForBlock(block, ++blocknum);
        block.clear();
    }
    BOOST_CHECK(fee_est.estimateFee(10).GetFeePerK() == feerate_package.GetFeePerK());

    // Now do the same but with 4 parents spent by a low fee child (E) itself spent
    // by a high-fee child (F) paying for the entire package:
    //    F
    //    E
    // A B C D
    // Our estimates should be decreased down to the low feerate of parent transactions
    // as we'll account for a large feerate for AEF and 3 low feerate B, C, D that get
    // immediately confirmed.
    while (blocknum < 70) {
        for (unsigned i = 0; i < 10; i++) {
            tx.vin[0].prevout.SetNull();
            tx.vin[0].prevout.n = ++txuid;
            mpool.addUnchecked(entry.Fee(fee_childless).Time(GetTime()).Height(blocknum).FromTx(tx));
            block.push_back(mpool.get(tx.GetHash()));

            for (unsigned j = 0; j < 4; j++) {
                tx.vin[0].prevout.n = ++txuid;
                uint256 parent_txid = tx.GetHash();
                mpool.addUnchecked(entry.Fee(fee_parents).Time(GetTime()).Height(blocknum).FromTx(tx));
                block.push_back(mpool.get(parent_txid));

                child_tx.vin[j].prevout.n = j;
                child_tx.vin[j].prevout.hash = parent_txid;
            }
            uint256 first_child_txid{child_tx.GetHash()};
            mpool.addUnchecked(entry.Fee(fee_parents).Time(GetTime()).Height(blocknum).FromTx(child_tx), false);
            block.push_back(mpool.get(first_child_txid));

            tx.vin[0].prevout.n = 0;
            tx.vin[0].prevout.hash = first_child_txid;
            mpool.addUnchecked(entry.Fee(fee_childs * 5).Time(GetTime()).Height(blocknum).FromTx(tx), false);
            block.push_back(mpool.get(tx.GetHash()));
        }
        mpool.removeForBlock(block, ++blocknum);
        block.clear();
    }
    BOOST_CHECK(fee_est.estimateFee(10).GetFeePerK() == feerate_parents.GetFeePerK());
}

BOOST_AUTO_TEST_SUITE_END()
