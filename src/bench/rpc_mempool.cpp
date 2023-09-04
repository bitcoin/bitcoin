// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <kernel/cs_main.h>
#include <kernel/mempool_entry.h>
#include <rpc/mempool.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <util/chaintype.h>
#include <validation.h>

#include <univalue.h>


static void AddTx(const CTransactionRef& tx, const CAmount& fee, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    LockPoints lp;
    pool.addUnchecked(CTxMemPoolEntry(tx, fee, /*time=*/0, /*entry_height=*/0, /*entry_sequence=*/0, /*entry_tx_inputs_coin_age=*/0.0, /*in_chain_input_value=*/0, /*spends_coinbase=*/false, /*extra_weight=*/0, /*sigops_cost=*/4, lp));
}

static void RpcMempool(benchmark::Bench& bench)
{
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN);
    auto& chainman = *testing_setup->m_node.chainman;
    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    LOCK2(cs_main, pool.cs);

    for (int i = 0; i < 1000; ++i) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vin[0].scriptWitness.stack.push_back({1});
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
        tx.vout[0].nValue = i;
        const CTransactionRef tx_r{MakeTransactionRef(tx)};
        AddTx(tx_r, /*fee=*/i, pool);
    }

    bench.run([&] {
        (void)MempoolToJSON(chainman, pool, /*verbose=*/true);
    });
}

BENCHMARK(RpcMempool, benchmark::PriorityLevel::HIGH);
