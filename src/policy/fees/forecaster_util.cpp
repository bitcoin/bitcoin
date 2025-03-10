// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/forecaster_util.h>
#include <policy/policy.h>

#include <algorithm>

Percentiles CalculatePercentiles(const std::vector<FeeFrac>& package_feerates, const int32_t total_weight)
{
    if (package_feerates.empty()) return Percentiles{};
    int32_t accumulated_weight{0};
    const int32_t p25_weight = 0.25 * total_weight;
    const int32_t p50_weight = 0.50 * total_weight;
    const int32_t p75_weight = 0.75 * total_weight;
    const int32_t p95_weight = 0.95 * total_weight;
    auto last_tracked_feerate = package_feerates.front();
    auto percentiles = Percentiles{};

    // Process histogram entries while maintaining monotonicity
    for (const auto& curr_feerate : package_feerates) {
        accumulated_weight += curr_feerate.size * WITNESS_SCALE_FACTOR;
        // Maintain monotonicity by taking the minimum between the current and last tracked fee rate
        last_tracked_feerate = std::min(last_tracked_feerate, curr_feerate, [](const FeeFrac& a, const FeeFrac& b) {
            return std::is_lt(FeeRateCompare(a, b));
        });
        if (accumulated_weight >= p25_weight && percentiles.p25.IsEmpty()) {
            percentiles.p25 = last_tracked_feerate;
        }
        if (accumulated_weight >= p50_weight && percentiles.p50.IsEmpty()) {
            percentiles.p50 = last_tracked_feerate;
        }
        if (accumulated_weight >= p75_weight && percentiles.p75.IsEmpty()) {
            percentiles.p75 = last_tracked_feerate;
        }
        if (accumulated_weight >= p95_weight && percentiles.p95.IsEmpty()) {
            percentiles.p95 = last_tracked_feerate;
            break; // Early exit once all percentiles are calculated
        }
    }

    // Return empty percentiles if we couldn't calculate the 95th percentile.
    return percentiles.p95.IsEmpty() ? Percentiles{} : percentiles;
}

std::string forecastTypeToString(ForecastType forecastType)
{
    switch (forecastType) {
    case ForecastType::MEMPOOL_FORECAST:
        return std::string("Mempool Forecast");
    case ForecastType::BLOCK_POLICY:
        return std::string("Block Policy Estimator");
    }
    // no default case, so the compiler can warn about missing cases
    assert(false);
}

std::vector<FeeFrac> FilterPackages(std::vector<FeeFrac>& package_feerates,
                                    std::set<Txid>& transactions_to_ignore,
                                    const std::vector<CTransactionRef>& block_txs)
{
    if (package_feerates.empty()) return {};
    Assume(!block_txs.empty());

    // Tracks which package should be removed
    std::vector<bool> indexed_to_filter(package_feerates.size(), false);

    size_t index = 0;                 // Current package index
    size_t current_package_vsize = 0; // Tracks accumulated vsize in the current package
    std::set<Txid> curr_package_txs;  // Stores transactions in the current package

    // Identify packages that contain transactions marked for removal
    for (const auto& tx : block_txs) {
        size_t tx_vsize = tx->GetTotalWeight() / WITNESS_SCALE_FACTOR;

        // Ensure `index` is within valid bounds before accessing `package_feerates`
        Assume(index < package_feerates.size());

        /* Move to the next package if:
           - Adding this transaction would exceed the package's vsize
            (Only when we've already added at least one transaction to the current package
            This prevents unintended package shifts due to slight size discrepancies)
        */
        if (!curr_package_txs.empty() && (current_package_vsize + tx_vsize > static_cast<size_t>(package_feerates[index].size))) {
            ++index; // Move to the next package
            current_package_vsize = 0;
            curr_package_txs.clear();
        }

        // Add the current transaction to the package
        curr_package_txs.insert(tx->GetHash());
        current_package_vsize += tx_vsize;

        // Mark the package for removal if it contains a flagged transaction
        if (transactions_to_ignore.count(tx->GetHash())) {
            indexed_to_filter[index] = true;
        }
    }

    // Build a new vector excluding flagged packages
    std::vector<FeeFrac> filtered_packages;
    filtered_packages.reserve(package_feerates.size());

    for (size_t i = 0; i < package_feerates.size(); ++i) {
        if (!indexed_to_filter[i]) {                          // Skip flagged packages
            filtered_packages.push_back(package_feerates[i]); // Implicit move
        }
    }
    return filtered_packages;
}
