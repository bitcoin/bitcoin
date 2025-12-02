// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <blockencodings.h>
#include <consensus/amount.h>
#include <kernel/cs_main.h>
#include <net_processing.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>

#include <memory>
#include <vector>


static void AddTx(const CTransactionRef& tx, const CAmount& fee, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    LockPoints lp;
    TryAddToMempool(pool, CTxMemPoolEntry(TxGraph::Ref(), tx, fee, /*time=*/0, /*entry_height=*/1, /*entry_sequence=*/0, /*spends_coinbase=*/false, /*sigops_cost=*/4, lp));
}

namespace {
class BenchCBHAST : public CBlockHeaderAndShortTxIDs
{
private:
    static CBlock DummyBlock()
    {
        CBlock block;
        block.nVersion = 5;
        block.hashPrevBlock.SetNull();
        block.hashMerkleRoot.SetNull();
        block.nTime = 1231006505;
        block.nBits = 0x1d00ffff;
        block.nNonce = 2083236893;
        block.fChecked = false;
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vout.resize(1);
        block.vtx.emplace_back(MakeTransactionRef(tx)); // dummy coinbase
        return block;
    }

public:
    BenchCBHAST(InsecureRandomContext& rng, int txs) : CBlockHeaderAndShortTxIDs(DummyBlock(), rng.rand64())
    {
        shorttxids.reserve(txs);
        while (txs-- > 0) {
            shorttxids.push_back(rng.randbits<SHORTTXIDS_LENGTH*8>());
        }
    }
};
} // anon namespace

static void BlockEncodingBench(benchmark::Bench& bench, size_t n_pool, size_t n_extra)
{
    const auto testing_setup = MakeNoLogFileContext<const ChainTestingSetup>(ChainType::MAIN);
    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    InsecureRandomContext rng(11);

    LOCK2(cs_main, pool.cs);

    std::vector<std::pair<Wtxid, CTransactionRef>> extratxn;
    extratxn.reserve(n_extra);

    // bump up the size of txs
    std::array<std::byte,200> sigspam;
    sigspam.fill(std::byte(42));

    // a reasonably large mempool of 50k txs, ~10MB total
    std::vector<CTransactionRef> refs;
    refs.reserve(n_pool + n_extra);
    for (size_t i = 0; i < n_pool + n_extra; ++i) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vin[0].scriptSig = CScript() << sigspam;
        tx.vin[0].scriptWitness.stack.push_back({1});
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
        tx.vout[0].nValue = i;
        refs.push_back(MakeTransactionRef(tx));
    }

    // ensure mempool ordering is different to memory ordering of transactions,
    // to simulate a mempool that has changed over time
    std::shuffle(refs.begin(), refs.end(), rng);

    for (size_t i = 0; i < n_pool; ++i) {
        AddTx(refs[i], /*fee=*/refs[i]->vout[0].nValue, pool);
    }
    for (size_t i = n_pool; i < n_pool + n_extra; ++i) {
        extratxn.emplace_back(refs[i]->GetWitnessHash(), refs[i]);
    }

    BenchCBHAST cmpctblock{rng, 3000};

    bench.run([&] {
        PartiallyDownloadedBlock pdb{&pool};
        auto res = pdb.InitData(cmpctblock, extratxn);

        // if there were duplicates the benchmark will be invalid
        // (eg, extra txns will be skipped) and we will receive
        // READ_STATUS_FAILED
        assert(res == READ_STATUS_OK);
    });
}

static void BlockEncodingNoExtra(benchmark::Bench& bench)
{
    BlockEncodingBench(bench, 50000, 0);
}

static void BlockEncodingStdExtra(benchmark::Bench& bench)
{
    static_assert(DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN == 100);
    BlockEncodingBench(bench, 50000, 100);
}

static void BlockEncodingLargeExtra(benchmark::Bench& bench)
{
    BlockEncodingBench(bench, 50000, 5000);
}

BENCHMARK(BlockEncodingNoExtra, benchmark::PriorityLevel::HIGH);
BENCHMARK(BlockEncodingStdExtra, benchmark::PriorityLevel::HIGH);
BENCHMARK(BlockEncodingLargeExtra, benchmark::PriorityLevel::HIGH);
