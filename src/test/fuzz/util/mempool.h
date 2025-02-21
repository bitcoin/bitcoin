// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_MEMPOOL_H
#define BITCOIN_TEST_FUZZ_UTIL_MEMPOOL_H

#include <kernel/mempool_entry.h>
#include <validation.h>

class CTransaction;
class CTxMemPool;
class FuzzedDataProvider;

class DummyChainState final : public Chainstate
{
public:
    void SetMempool(CTxMemPool* mempool)
    {
        m_mempool = mempool;
    }
};

[[nodiscard]] bool SanityCheckForConsumeTxMemPoolEntry(const CTransaction& tx) noexcept;
[[nodiscard]] CTxMemPoolEntry ConsumeTxMemPoolEntry(FuzzedDataProvider& fuzzed_data_provider, const CTransaction& tx) noexcept;

#endif // BITCOIN_TEST_FUZZ_UTIL_MEMPOOL_H
