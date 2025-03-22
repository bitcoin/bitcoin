// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_MEMPOOL_H
#define BITCOIN_TEST_FUZZ_UTIL_MEMPOOL_H

#include <kernel/mempool_entry.h>

class CTransaction;
class FuzzedDataProvider;

[[nodiscard]] CTxMemPoolEntry ConsumeTxMemPoolEntry(FuzzedDataProvider& fuzzed_data_provider, const CTransaction& tx) noexcept;

#endif // BITCOIN_TEST_FUZZ_UTIL_MEMPOOL_H
