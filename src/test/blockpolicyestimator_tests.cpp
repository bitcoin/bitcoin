// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/estimator_args.h>
#include <policy/policy.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/readwritefile.h>
#include <util/serfloat.h>
#include <util/time.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>
#include <string>

BOOST_FIXTURE_TEST_SUITE(blockpolicyestimator_tests, ChainTestingSetup)

BOOST_AUTO_TEST_CASE(BlockPolicyEstimates)
{
    CBlockPolicyEstimator feeEst{BlockPolicyFeeEstPath(*m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    TestMemPoolEntryHelper entry;
    CAmount basefee(2000);
    CAmount deltaFee(100);
    std::vector<CAmount> feeV;
    feeV.reserve(10);

    // Populate vectors of increasing fees
    for (int j = 0; j < 10; j++) {
        feeV.push_back(basefee * (j+1));
    }

    // Store the hashes of transactions that have been
    // added to the mempool by their associate fee
    // mempool_txs[j] is populated with transactions either of
    // fee = basefee * (j+1)
    std::list<CTxMemPoolEntry> mempool_txs[10];

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
    std::vector<RemovedMempoolTransactionInfo> block_txs;
    int blocknum = 0;

    // Loop through 200 blocks
    // At a decay .9952 and 4 fee transactions per block
    // This makes the tx count about 2.5 per bucket, well above the 0.1 threshold
    while (blocknum < 200) {
        for (int j = 0; j < 10; j++) { // For each fee
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k; // make transaction unique
                // Simulate the tx being added to the mempool by calling processTransaction(tx_info)
                mempool_txs[j].emplace_back(entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                  feeV[j],
                                                                                  virtual_size,
                                                                                  entry.nHeight,
                                                                                  /*mempool_limit_bypassed=*/false,
                                                                                  /*submitted_in_package=*/false,
                                                                                  /*chainstate_is_current=*/true,
                                                                                  /*has_no_mempool_parents=*/true)};
                feeEst.processTransaction(tx_info);
            }
        }
        //Create blocks where higher fee txs are included more often
        for (int h = 0; h <= blocknum%10; h++) {
            // 10/10 blocks add highest fee transactions
            // 9/10 blocks add 2nd highest and so on until ...
            // 1/10 blocks add lowest fee transactions
            while (mempool_txs[9 - h].size()) {
                auto& tx_entry = mempool_txs[9 - h].back();
                block_txs.emplace_back(tx_entry);
                mempool_txs[9 - h].pop_back();
            }
        }

        feeEst.processBlock(block_txs, ++blocknum);
        block_txs.clear();
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
    while (blocknum < 250) {
        feeEst.processBlock(block_txs, ++blocknum);
    }

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
                // Simulate the tx being added to the mempool by calling processTransaction(tx_info)
                mempool_txs[j].emplace_back(entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                  feeV[j],
                                                                                  virtual_size,
                                                                                  entry.nHeight,
                                                                                  /*mempool_limit_bypassed=*/false,
                                                                                  /*submitted_in_package=*/false,
                                                                                  /*chainstate_is_current=*/true,
                                                                                  /*has_no_mempool_parents=*/true)};
                feeEst.processTransaction(tx_info);
            }
        }
        feeEst.processBlock(block_txs, ++blocknum);
    }

    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i) == CFeeRate(0) || feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }

    // Mine all those transactions
    // Estimates should still not be below original
    for (int j = 0; j < 10; j++) {
        while (mempool_txs[j].size()) {
            auto& tx_entry = mempool_txs[j].back();
            block_txs.emplace_back(tx_entry);
            mempool_txs[j].pop_back();
        }
    }

    feeEst.processBlock(block_txs, ++blocknum);
    block_txs.clear();

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
                // These txs are mined in the same block, so there is no need to
                // retain them in mempool_txs; use a local entry to build block_txs.
                const CTxMemPoolEntry tx_entry{entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx)};
                const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                  feeV[j],
                                                                                  virtual_size,
                                                                                  entry.nHeight,
                                                                                  /*mempool_limit_bypassed=*/false,
                                                                                  /*submitted_in_package=*/false,
                                                                                  /*chainstate_is_current=*/true,
                                                                                  /*has_no_mempool_parents=*/true)};

                feeEst.processTransaction(tx_info);
                block_txs.emplace_back(tx_entry);
            }
        }

        feeEst.processBlock(block_txs, ++blocknum);
        block_txs.clear();
    }
    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 9; i++) { // At 9, the original estimate was already at the bottom (b/c scale = 2)
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() < origFeeEst[i-1] - deltaFee);
    }
}

BOOST_AUTO_TEST_CASE(ReadRejectsInvalidBuckets)
{
    const fs::path path{m_args.GetDataDirBase() / "fee_estimates_buckets.dat"};
    CBlockPolicyEstimator feeEst{BlockPolicyFeeEstPath(m_args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    {
        AutoFile file{fsbridge::fopen(path, "wb")};
        BOOST_REQUIRE(feeEst.Write(file));
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }
    const auto [ok, data]{ReadBinaryFile(path)};
    BOOST_REQUIRE(ok);

    // Layout: version, best height, historical first and best (4 bytes each),
    // then the bucket count as a compact size and the buckets as 8-byte doubles.
    constexpr size_t buckets_pos{16};
    SpanReader reader{std::span{reinterpret_cast<const unsigned char*>(data.data()) + buckets_pos, data.size() - buckets_pos}};
    const uint64_t num_buckets{ReadCompactSize(reader)};
    const size_t first_pos{data.size() - reader.size()};
    BOOST_REQUIRE_GT(num_buckets, 2U);

    const auto read_with_bucket = [&](size_t index, double value) {
        std::string mutated{data};
        const uint64_t encoded{EncodeDouble(value)};
        for (int i = 0; i < 8; ++i) mutated[first_pos + 8 * index + i] = static_cast<char>(encoded >> (8 * i));
        BOOST_REQUIRE(WriteBinaryFile(path, mutated));
        AutoFile file{fsbridge::fopen(path, "rb")};
        return feeEst.Read(file);
    };

    // The unchanged file is accepted.
    {
        BOOST_REQUIRE(WriteBinaryFile(path, data));
        AutoFile file{fsbridge::fopen(path, "rb")};
        BOOST_CHECK(feeEst.Read(file));
    }
    // No infinite last bucket.
    BOOST_CHECK(!read_with_bucket(num_buckets - 1, 1e98));
    // A bucket that is not positive.
    BOOST_CHECK(!read_with_bucket(0, 0.0));
    BOOST_CHECK(!read_with_bucket(0, -1.0));
    // Buckets that do not increase.
    BOOST_CHECK(!read_with_bucket(1, 0.5));
    // A NaN bucket.
    BOOST_CHECK(!read_with_bucket(1, std::numeric_limits<double>::quiet_NaN()));
}

BOOST_AUTO_TEST_SUITE_END()
