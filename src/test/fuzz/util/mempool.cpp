// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <kernel/mempool_entry.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>

#include <cassert>
#include <cstdint>
#include <limits>

CTxMemPoolEntry ConsumeTxMemPoolEntry(FuzzedDataProvider& fuzzed_data_provider, const CTransaction& tx, uint32_t max_height) noexcept
{
    // Avoid:
    // policy/feerate.cpp:28:34: runtime error: signed integer overflow: 34873208148477500 * 1000 cannot be represented in type 'long'
    //
    // Reproduce using CFeeRate(348732081484775, 10).GetFeePerK()
    const CAmount fee{ConsumeMoney(fuzzed_data_provider, /*max=*/std::numeric_limits<CAmount>::max() / CAmount{100'000})};
    assert(MoneyRange(fee));
    const int64_t time = fuzzed_data_provider.ConsumeIntegral<int64_t>();
    const uint64_t entry_sequence{fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
    const auto entry_height{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, max_height)};
    const bool spends_coinbase = fuzzed_data_provider.ConsumeBool();
    const unsigned int sig_op_cost = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, MAX_BLOCK_SIGOPS_COST);
    return CTxMemPoolEntry{TxGraph::Ref(), MakeTransactionRef(tx), fee, time, entry_height, entry_sequence, spends_coinbase, sig_op_cost, {}};
}
