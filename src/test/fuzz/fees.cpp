// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/messages.h>
#include <consensus/amount.h>
#include <policy/fees/block_policy_estimator.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/fees.h>

#include <cstdint>
#include <string>
#include <vector>

using common::StringForFeeReason;
using common::StringForFeeSource;

FUZZ_TARGET(fees)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CFeeRate minimal_incremental_fee{ConsumeMoney(fuzzed_data_provider)};
    FastRandomContext rng{/*fDeterministic=*/true};
    FeeFilterRounder fee_filter_rounder{minimal_incremental_fee, rng};
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const CAmount current_minimum_fee = ConsumeMoney(fuzzed_data_provider);
        const CAmount rounded_fee = fee_filter_rounder.round(current_minimum_fee);
        assert(MoneyRange(rounded_fee));
    }
    const FeeReason fee_reason = fuzzed_data_provider.PickValueInArray({FeeReason::NONE, FeeReason::HALF_ESTIMATE, FeeReason::FULL_ESTIMATE, FeeReason::DOUBLE_ESTIMATE, FeeReason::CONSERVATIVE});
    (void)StringForFeeReason(fee_reason);

    const FeeSource fee_source = fuzzed_data_provider.PickValueInArray({FeeSource::FEE_RATE_ESTIMATOR, FeeSource::MEMPOOL_MIN, FeeSource::PAYTXFEE, FeeSource::FALLBACK, FeeSource::REQUIRED});
    (void)StringForFeeSource(fee_source);
}
