// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <policy/fees/forecaster_util.h>
#include <policy/policy.h>
#include <random.h>

#include <boost/test/unit_test.hpp>
#include <vector>

static inline std::vector<CTransactionRef> make_random_txs(size_t count)
{
    std::vector<CTransactionRef> txs;
    for (size_t i = 0; i < count; ++i) {
        auto rng = FastRandomContext();
        auto tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vout.resize(1);
        tx.vin[0].prevout.hash = Txid::FromUint256(rng.rand256());
        tx.vin[0].prevout.n = 0;
        tx.vin[0].scriptSig << OP_TRUE;
        tx.vout[0].scriptPubKey = CScript() << OP_TRUE;
        tx.vout[0].nValue = COIN;
        txs.emplace_back(MakeTransactionRef(tx));
    }
    return txs;
}

BOOST_AUTO_TEST_SUITE(forecaster_util_tests)

BOOST_AUTO_TEST_CASE(calculate_percentile_test)
{
    // Test that CalculatePercentiles returns empty when vector is empty
    BOOST_CHECK(CalculatePercentiles({}, DEFAULT_BLOCK_MAX_WEIGHT).empty());

    const int32_t package_size{10};
    const int32_t individual_tx_vsize = static_cast<int32_t>(DEFAULT_BLOCK_MAX_WEIGHT / WITNESS_SCALE_FACTOR) / package_size;

    // Define fee rate categories with corresponding fee rates in satoshis per kvB
    const FeeFrac superHighFeerate{500 * individual_tx_vsize, individual_tx_vsize}; // Super High fee: 500 sat/kvB
    const FeeFrac highFeerate{100 * individual_tx_vsize, individual_tx_vsize};      // High fee: 100 sat/kvB
    const FeeFrac mediumFeerate{50 * individual_tx_vsize, individual_tx_vsize};     // Medium fee: 50 sat/kvB
    const FeeFrac lowFeerate{10 * individual_tx_vsize, individual_tx_vsize};        // Low fee: 10 sat/kvB

    std::vector<FeeFrac> package_feerates;
    package_feerates.reserve(package_size);

    // Populate the feerate histogram based on specified index ranges.
    for (int i = 0; i < package_size; ++i) {
        if (i < 3) {
            package_feerates.emplace_back(superHighFeerate); // Super High fee rate for top 3
        } else if (i < 5) {
            package_feerates.emplace_back(highFeerate); // High fee rate for next 2
        } else if (i < 8) {
            package_feerates.emplace_back(mediumFeerate);                                          // Medium fee rate for next 3
            BOOST_CHECK(CalculatePercentiles(package_feerates, DEFAULT_BLOCK_MAX_WEIGHT).empty()); // CalculatePercentiles should return empty until reaching the 95th percentile
        } else {
            package_feerates.emplace_back(lowFeerate); // Low fee rate for remaining 2
        }
    }

    // Test percentile calculation on a complete histogram
    {
        const auto percentiles = CalculatePercentiles(package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
        BOOST_CHECK(percentiles.p25 == superHighFeerate);
        BOOST_CHECK(percentiles.p50 == highFeerate);
        BOOST_CHECK(percentiles.p75 == mediumFeerate);
        BOOST_CHECK(percentiles.p95 == lowFeerate);
    }

    // Test that CalculatePercentiles maintains monotonicity across all percentiles
    {
        package_feerates[7] = superHighFeerate; // Increase 8th index to a high fee rate
        const auto percentiles = CalculatePercentiles(package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
        BOOST_CHECK(percentiles.p25 == superHighFeerate);
        BOOST_CHECK(percentiles.p50 == highFeerate);
        BOOST_CHECK(percentiles.p75 == mediumFeerate); // Should still reflect the previous medium rate
        BOOST_CHECK(percentiles.p95 == lowFeerate);
    }
}

BOOST_AUTO_TEST_CASE(filter_packages)
{
    // Create 10 test transactions with equal size.
    std::vector<CTransactionRef> block_txs = make_random_txs(10);
    const size_t single_tx_vsize = (*block_txs.begin())->GetTotalWeight() / WITNESS_SCALE_FACTOR;

    // Define packages based on transaction fee and vsizes
    std::vector<FeeFrac> package_feerates = {
        FeeFrac(2000, single_tx_vsize),     // Package 0 (tx1)
        FeeFrac(3900, single_tx_vsize * 2), // Package 1 (tx2, tx3)
        FeeFrac(1900, single_tx_vsize),     // Package 2 (tx4)
        FeeFrac(5100, single_tx_vsize * 3), // Package 3 (tx5, tx6, tx7)
        FeeFrac(1500, single_tx_vsize),     // Package 4 (tx8)
        FeeFrac(1400, single_tx_vsize),     // Package 5 (tx9)
        FeeFrac(1000, single_tx_vsize),     // Package 6 (tx10)
    };

    {
        // Case 1: Ignore tx1 (Package 0)
        std::set<Txid> ignore_txs = {block_txs[0]->GetHash()};
        auto filtered_packages = FilterPackages(package_feerates, ignore_txs, block_txs);
        BOOST_CHECK(filtered_packages.size() == package_feerates.size() - 1);
        BOOST_CHECK(std::find_if(filtered_packages.begin(), filtered_packages.end(),
                                 [&](const FeeFrac& p) { return p == package_feerates[0]; }) == filtered_packages.end());
    }

    {
        // Case 2: Ignore only tx3 (Part of Package 1)
        std::set<Txid> ignore_txs = {block_txs[2]->GetHash()};
        auto filtered_packages = FilterPackages(package_feerates, ignore_txs, block_txs);
        BOOST_CHECK(filtered_packages.size() == package_feerates.size() - 1);

        BOOST_CHECK(std::find_if(filtered_packages.begin(), filtered_packages.end(),
                                 [&](const FeeFrac& p) { return p == package_feerates[1]; }) == filtered_packages.end());
    }

    {
        // Case 3: Ignore all of Package 3 (tx5, tx6, tx7)
        std::set<Txid> ignore_txs = {block_txs[4]->GetHash(), block_txs[5]->GetHash(), block_txs[6]->GetHash()};
        auto filtered_packages = FilterPackages(package_feerates, ignore_txs, block_txs);
        BOOST_CHECK(filtered_packages.size() == package_feerates.size() - 1);
        BOOST_CHECK(std::find_if(filtered_packages.begin(), filtered_packages.end(),
                                 [&](const FeeFrac& p) { return p == package_feerates[3]; }) == filtered_packages.end());
    }

    {
        // Case 4: Ignore multiple transactions across different packages (tx1, tx2, tx6, tx10)
        std::set<Txid> ignore_txs = {block_txs[0]->GetHash(), block_txs[1]->GetHash(), block_txs[5]->GetHash(), block_txs[9]->GetHash()};
        auto filtered_packages = FilterPackages(package_feerates, ignore_txs, block_txs);
        BOOST_CHECK(filtered_packages.size() == package_feerates.size() - 4);
        BOOST_CHECK(std::find_if(filtered_packages.begin(), filtered_packages.end(),
                                 [&](const FeeFrac& p) { return p == package_feerates[0]; }) == filtered_packages.end());
        BOOST_CHECK(std::find_if(filtered_packages.begin(), filtered_packages.end(),
                                 [&](const FeeFrac& p) { return p == package_feerates[1]; }) == filtered_packages.end());
        BOOST_CHECK(std::find_if(filtered_packages.begin(), filtered_packages.end(),
                                 [&](const FeeFrac& p) { return p == package_feerates[3]; }) == filtered_packages.end());
        BOOST_CHECK(std::find_if(filtered_packages.begin(), filtered_packages.end(),
                                 [&](const FeeFrac& p) { return p == package_feerates[6]; }) == filtered_packages.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()
