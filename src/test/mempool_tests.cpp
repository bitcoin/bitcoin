// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <policy/policy.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/time.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(mempool_tests, TestingSetup)

static constexpr auto REMOVAL_REASON_DUMMY = MemPoolRemovalReason::REPLACED;

class MemPoolTest final : public CTxMemPool
{
public:
    using CTxMemPool::GetMinFee;
};

BOOST_AUTO_TEST_CASE(MempoolRemoveTest)
{
    // Test CTxMemPool::remove functionality

    TestMemPoolEntryHelper entry;
    // Parent transaction with three children,
    // and three grand-children:
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++)
    {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000LL;
    }
    CMutableTransaction txChild[3];
    for (int i = 0; i < 3; i++)
    {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout.hash = txParent.GetHash();
        txChild[i].vin[0].prevout.n = i;
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000LL;
    }
    CMutableTransaction txGrandChild[3];
    for (int i = 0; i < 3; i++)
    {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout.hash = txChild[i].GetHash();
        txGrandChild[i].vin[0].prevout.n = 0;
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000LL;
    }


    CTxMemPool& testPool = *Assert(m_node.mempool);
    LOCK2(::cs_main, testPool.cs);

    // Nothing in pool, remove should do nothing:
    unsigned int poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);

    // Just the parent:
    AddToMempool(testPool, entry.FromTx(txParent));
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 1);

    // Parent, children, grandchildren:
    AddToMempool(testPool, entry.FromTx(txParent));
    for (int i = 0; i < 3; i++)
    {
        AddToMempool(testPool, entry.FromTx(txChild[i]));
        AddToMempool(testPool, entry.FromTx(txGrandChild[i]));
    }
    // Remove Child[0], GrandChild[0] should be removed:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 2);
    // ... make sure grandchild and child are gone:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txGrandChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    // Remove parent, all children/grandchildren should go:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 5);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);

    // Add children and grandchildren, but NOT the parent (simulate the parent being in a block)
    for (int i = 0; i < 3; i++)
    {
        AddToMempool(testPool, entry.FromTx(txChild[i]));
        AddToMempool(testPool, entry.FromTx(txGrandChild[i]));
    }
    // Now remove the parent, as might happen if a block-re-org occurs but the parent cannot be
    // put into the mempool (maybe because it is non-standard):
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 6);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);
}

BOOST_AUTO_TEST_CASE(MempoolSizeLimitTest)
{
    auto& pool = static_cast<MemPoolTest&>(*Assert(m_node.mempool));
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    AddToMempool(pool, entry.Fee(1000LL).FromTx(tx1));

    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_2;
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_2 << OP_EQUAL;
    tx2.vout[0].nValue = 10 * COIN;
    AddToMempool(pool, entry.Fee(500LL).FromTx(tx2));

    pool.TrimToSize(pool.DynamicMemoryUsage()); // should do nothing
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // should remove the lower-feerate transaction
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));

    AddToMempool(pool, entry.FromTx(tx2));
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vin.resize(1);
    tx3.vin[0].prevout = COutPoint(tx2.GetHash(), 0);
    tx3.vin[0].scriptSig = CScript() << OP_2;
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_3 << OP_EQUAL;
    tx3.vout[0].nValue = 10 * COIN;
    AddToMempool(pool, entry.Fee(2000LL).FromTx(tx3));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // tx3 should pay for tx2 (CPFP)
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));
    BOOST_CHECK(pool.exists(tx3.GetHash()));

    pool.TrimToSize(GetVirtualTransactionSize(CTransaction(tx1))); // mempool is limited to tx1's size in memory usage, so nothing fits
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));
    BOOST_CHECK(!pool.exists(tx3.GetHash()));

    CFeeRate maxFeeRateRemoved(2500, GetVirtualTransactionSize(CTransaction(tx3)) + GetVirtualTransactionSize(CTransaction(tx2)));
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE);

    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vin.resize(2);
    tx4.vin[0].prevout.SetNull();
    tx4.vin[0].scriptSig = CScript() << OP_4;
    tx4.vin[1].prevout.SetNull();
    tx4.vin[1].scriptSig = CScript() << OP_4;
    tx4.vout.resize(2);
    tx4.vout[0].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[0].nValue = 10 * COIN;
    tx4.vout[1].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vin.resize(2);
    tx5.vin[0].prevout = COutPoint(tx4.GetHash(), 0);
    tx5.vin[0].scriptSig = CScript() << OP_4;
    tx5.vin[1].prevout.SetNull();
    tx5.vin[1].scriptSig = CScript() << OP_5;
    tx5.vout.resize(2);
    tx5.vout[0].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[0].nValue = 10 * COIN;
    tx5.vout[1].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx6 = CMutableTransaction();
    tx6.vin.resize(2);
    tx6.vin[0].prevout = COutPoint(tx4.GetHash(), 1);
    tx6.vin[0].scriptSig = CScript() << OP_4;
    tx6.vin[1].prevout.SetNull();
    tx6.vin[1].scriptSig = CScript() << OP_6;
    tx6.vout.resize(2);
    tx6.vout[0].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[0].nValue = 10 * COIN;
    tx6.vout[1].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx7 = CMutableTransaction();
    tx7.vin.resize(2);
    tx7.vin[0].prevout = COutPoint(tx5.GetHash(), 0);
    tx7.vin[0].scriptSig = CScript() << OP_5;
    tx7.vin[1].prevout = COutPoint(tx6.GetHash(), 0);
    tx7.vin[1].scriptSig = CScript() << OP_6;
    tx7.vout.resize(2);
    tx7.vout[0].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[0].nValue = 10 * COIN;
    tx7.vout[1].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[1].nValue = 10 * COIN;

    AddToMempool(pool, entry.Fee(700LL).FromTx(tx4));
    auto usage_with_tx4_only = pool.DynamicMemoryUsage();
    AddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    AddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    AddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    // From the topology above, tx7 must be sorted last, so it should
    // definitely evicted first if we must trim. tx4 should definitely remain
    // in the mempool since it has a higher feerate than its descendants and
    // should be in its own chunk.
    pool.TrimToSize(pool.DynamicMemoryUsage() - 1);
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    // Tx5 and Tx6 may be removed as well because they're in the same chunk as
    // tx7, but this behavior need not be guaranteed.

    if (!pool.exists(tx5.GetHash()))
        AddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    if (!pool.exists(tx6.GetHash()))
        AddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    AddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    // If we trim sufficiently, everything but tx4 should be removed.
    pool.TrimToSize(usage_with_tx4_only + 1);
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(!pool.exists(tx5.GetHash()));
    BOOST_CHECK(!pool.exists(tx6.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    AddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    AddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    AddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    std::vector<CTransactionRef> vtx;
    SetMockTime(42);
    SetMockTime(42 + CTxMemPool::ROLLING_FEE_HALFLIFE);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE);
    // ... we should keep the same min fee until we get a block
    pool.removeForBlock(vtx, 1);
    SetMockTime(42 + 2*CTxMemPool::ROLLING_FEE_HALFLIFE);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/2.0));
    // ... then feerate should drop 1/2 each halflife

    SetMockTime(42 + 2*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2);
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 5 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/4.0));
    // ... with a 1/2 halflife when mempool is < 1/2 its target size

    SetMockTime(42 + 2*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2 + CTxMemPool::ROLLING_FEE_HALFLIFE/4);
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 9 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/8.0));
    // ... with a 1/4 halflife when mempool is < 1/4 its target size

    SetMockTime(42 + 7*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2 + CTxMemPool::ROLLING_FEE_HALFLIFE/4);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), DEFAULT_INCREMENTAL_RELAY_FEE);
    // ... but feerate should never drop below DEFAULT_INCREMENTAL_RELAY_FEE

    SetMockTime(42 + 8*CTxMemPool::ROLLING_FEE_HALFLIFE + CTxMemPool::ROLLING_FEE_HALFLIFE/2 + CTxMemPool::ROLLING_FEE_HALFLIFE/4);
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), 0);
    // ... unless it has gone all the way to 0 (after getting past DEFAULT_INCREMENTAL_RELAY_FEE/2)
}

inline CTransactionRef make_tx(std::vector<CAmount>&& output_values, std::vector<CTransactionRef>&& inputs=std::vector<CTransactionRef>(), std::vector<uint32_t>&& input_indices=std::vector<uint32_t>())
{
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(output_values.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout.hash = inputs[i]->GetHash();
        tx.vin[i].prevout.n = input_indices.size() > i ? input_indices[i] : 0;
    }
    for (size_t i = 0; i < output_values.size(); ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = output_values[i];
    }
    return MakeTransactionRef(tx);
}


BOOST_AUTO_TEST_CASE(MempoolAncestryTests)
{
    size_t ancestors, clustersize;

    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /* Base transaction */
    //
    // [tx1]
    //
    CTransactionRef tx1 = make_tx(/*output_values=*/{10 * COIN});
    AddToMempool(pool, entry.Fee(10000LL).FromTx(tx1));

    // Ancestors / clustersize should be 1 / 1 (itself / itself)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 1ULL);

    /* Child transaction */
    //
    // [tx1].0 <- [tx2]
    //
    CTransactionRef tx2 = make_tx(/*output_values=*/{495 * CENT, 5 * COIN}, /*inputs=*/{tx1});
    AddToMempool(pool, entry.Fee(10000LL).FromTx(tx2));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     2 (tx1,2)
    // tx2          2 (tx1,2)   2 (tx1,2)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 2ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 2ULL);

    /* Grand-child 1 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //
    CTransactionRef tx3 = make_tx(/*output_values=*/{290 * CENT, 200 * CENT}, /*inputs=*/{tx2});
    AddToMempool(pool, entry.Fee(10000LL).FromTx(tx3));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     3 (tx1,2,3)
    // tx2          2 (tx1,2)   3 (tx1,2,3)
    // tx3          3 (tx1,2,3) 3 (tx1,2,3)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);

    /* Grand-child 2 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef tx4 = make_tx(/*output_values=*/{290 * CENT, 250 * CENT}, /*inputs=*/{tx2}, /*input_indices=*/{1});
    AddToMempool(pool, entry.Fee(10000LL).FromTx(tx4));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     4 (tx1,2,3,4)
    // tx2          2 (tx1,2)   4 (tx1,2,3,4)
    // tx3          3 (tx1,2,3) 4 (tx1,2,3,4)
    // tx4          3 (tx1,2,4) 4 (tx1,2,3,4)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);

    /* Make an alternate branch that is longer and connect it to tx3 */
    //
    // [ty1].0 <- [ty2].0 <- [ty3].0 <- [ty4].0 <- [ty5].0
    //                                              |
    // [tx1].0 <- [tx2].0 <- [tx3].0 <- [ty6] --->--/
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef ty1, ty2, ty3, ty4, ty5;
    CTransactionRef* ty[5] = {&ty1, &ty2, &ty3, &ty4, &ty5};
    CAmount v = 5 * COIN;
    for (uint64_t i = 0; i < 5; i++) {
        CTransactionRef& tyi = *ty[i];
        tyi = make_tx(/*output_values=*/{v}, /*inputs=*/i > 0 ? std::vector<CTransactionRef>{*ty[i - 1]} : std::vector<CTransactionRef>{});
        v -= 50 * CENT;
        AddToMempool(pool, entry.Fee(10000LL).FromTx(tyi));
        pool.GetTransactionAncestry(tyi->GetHash(), ancestors, clustersize);
        BOOST_CHECK_EQUAL(ancestors, i+1);
        BOOST_CHECK_EQUAL(clustersize, i+1);
    }
    CTransactionRef ty6 = make_tx(/*output_values=*/{5 * COIN}, /*inputs=*/{tx3, ty5});
    AddToMempool(pool, entry.Fee(10000LL).FromTx(ty6));

    // Ancestors / clustersize should be:
    // transaction  ancestors           clustersize
    // ============ =================== ===========
    // tx1          1 (tx1)             10 (tx1-5, ty1-5)
    // tx2          2 (tx1,2)           10
    // tx3          3 (tx1,2,3)         10
    // tx4          3 (tx1,2,4)         10
    // ty1          1 (ty1)             10
    // ty2          2 (ty1,2)           10
    // ty3          3 (ty1,2,3)         10
    // ty4          4 (y1234)           10
    // ty5          5 (y12345)          10
    // ty6          9 (tx123, ty123456) 10
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty5->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 5ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty6->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 9ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
}

BOOST_AUTO_TEST_CASE(MempoolAncestryTestsDiamond)
{
    size_t ancestors, descendants;

    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /* Ancestors represented more than once ("diamond") */
    //
    // [ta].0 <- [tb].0 -----<------- [td].0
    //            |                    |
    //            \---1 <- [tc].0 --<--/
    //
    CTransactionRef ta, tb, tc, td;
    ta = make_tx(/*output_values=*/{10 * COIN});
    tb = make_tx(/*output_values=*/{5 * COIN, 3 * COIN}, /*inputs=*/ {ta});
    tc = make_tx(/*output_values=*/{2 * COIN}, /*inputs=*/{tb}, /*input_indices=*/{1});
    td = make_tx(/*output_values=*/{6 * COIN}, /*inputs=*/{tb, tc}, /*input_indices=*/{0, 0});
    AddToMempool(pool, entry.Fee(10000LL).FromTx(ta));
    AddToMempool(pool, entry.Fee(10000LL).FromTx(tb));
    AddToMempool(pool, entry.Fee(10000LL).FromTx(tc));
    AddToMempool(pool, entry.Fee(10000LL).FromTx(td));

    // Ancestors / descendants should be:
    // transaction  ancestors           descendants
    // ============ =================== ===========
    // ta           1 (ta               4 (ta,tb,tc,td)
    // tb           2 (ta,tb)           4 (ta,tb,tc,td)
    // tc           3 (ta,tb,tc)        4 (ta,tb,tc,td)
    // td           4 (ta,tb,tc,td)     4 (ta,tb,tc,td)
    pool.GetTransactionAncestry(ta->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(tb->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(tc->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(td->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
}

BOOST_AUTO_TEST_SUITE_END()
