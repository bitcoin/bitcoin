#include <boost/test/unit_test.hpp>

#include "init.h"
#include "main.h"
#include "uint256.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(mempool_tests)

BOOST_AUTO_TEST_CASE(CTxMemPool_add_remove)
{
    BOOST_CHECK_EQUAL(mempool.size(), 0);

    CTransaction tx1,tx2;

    tx1.vin.resize(1);
    tx1.vout.resize(1);
    tx1.vout[0].nValue = 2LL * COIN;

    // Adding and removing a single transaction
    BOOST_CHECK(!mempool.exists(tx1.GetHash()));

    mempool.addUnchecked(tx1, 1);

    BOOST_CHECK(mempool.exists(tx1.GetHash()));
    BOOST_CHECK_EQUAL(mempool.size(), 1);
    BOOST_CHECK(mempool.lookup(tx1.GetHash()) == tx1);

    mempool.remove(tx1.GetHash());
    BOOST_CHECK(!mempool.exists(tx1.GetHash()));
    BOOST_CHECK_EQUAL(mempool.size(), 0);


    // Adding and removing a transaction with dependencies, tx2 depends on tx1
    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].prevout.hash = tx1.GetHash();
    tx2.vin[0].prevout.n = 0;
    tx2.vout.resize(1);
    tx2.vout[0].nValue = 1LL * COIN;

    mempool.addUnchecked(tx1, 0);
    mempool.addUnchecked(tx2, 1LL * COIN);

    BOOST_CHECK(mempool.exists(tx1.GetHash()));
    BOOST_CHECK(mempool.exists(tx2.GetHash()));
    BOOST_CHECK_EQUAL(mempool.size(), 2);


    // tx2 depends on tx1, so both should be removed with recursive flag set
    mempool.remove(tx1.GetHash());
    BOOST_CHECK(!mempool.exists(tx1.GetHash()));
    BOOST_CHECK(!mempool.exists(tx2.GetHash()));

    BOOST_CHECK_EQUAL(mempool.size(), 0);
    BOOST_CHECK_EQUAL(mempool.heapTx.size(), 0);
    BOOST_CHECK_EQUAL(mempool.mapTx.size(), 0);
    BOOST_CHECK_EQUAL(mempool.mapNextTx.size(), 0);

    mempool.clear();
}

BOOST_AUTO_TEST_CASE(CTxMemPool_priority_tracking)
{
    BOOST_CHECK(mempool.size() == 0);

    CTransaction tx1,tx2,tx3,tx4,tx5,tx6;

    // First transaction, 1BTC fee
    tx1.vin.resize(1);
    tx1.vout.resize(4);
    tx1.vout[0].nValue = 10LL * COIN;
    tx1.vout[1].nValue = 10LL * COIN;
    tx1.vout[2].nValue = 10LL * COIN;
    tx1.vout[3].nValue = 10LL * COIN;
    BOOST_CHECK(mempool.addUnchecked(tx1, 1LL * COIN));

    // Exactly one tx in the heap
    BOOST_CHECK(mempool.heapTx.top() == tx1);
    BOOST_CHECK_EQUAL(mempool.heapTx.top().nSumTxFees, 1LL * COIN);
    BOOST_CHECK_EQUAL(mempool.heapTx.top().nSumTxSize, 87);
    BOOST_CHECK_EQUAL(mempool.heapTx.top().nDepth, 1);


    // Second tx, 2BTC fee, replaces tx1 as top of the heap
    tx2.vin.resize(1);
    tx2.vin[0].prevout.n = 1; // need to make tx1 != tx2
    tx2.vout.resize(2);
    tx2.vout[0].nValue = 10LL * COIN;
    tx2.vout[1].nValue = 10LL * COIN;
    BOOST_CHECK(mempool.addUnchecked(tx2, 2LL * COIN));

    BOOST_CHECK(mempool.heapTx.top() == tx2);
    BOOST_CHECK_EQUAL(mempool.heapTx.top().nSumTxFees, 2LL * COIN);
    BOOST_CHECK_EQUAL(mempool.heapTx.top().nSumTxSize, 69);
    BOOST_CHECK_EQUAL(mempool.heapTx.top().nDepth, 1);


    // Third tx, 1BTC fee, top of heap remains unchanged
    tx3.vin.resize(1);
    tx3.vin[0].prevout.n = 2;
    tx3.vout.resize(1);
    tx3.vout[0].nValue = 10LL * COIN;
    BOOST_CHECK(mempool.addUnchecked(tx3, 1LL * COIN));

    BOOST_CHECK(mempool.heapTx.top() == tx2);
    BOOST_CHECK_EQUAL(mempool.lookup(tx3.GetHash()).nSumTxFees, 1LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx3.GetHash()).nSumTxSize, 60);
    BOOST_CHECK_EQUAL(mempool.lookup(tx3.GetHash()).nDepth, 1);


    // Spend tx1 output 0 with 4BTC fee
    tx4.vin.resize(1);
    tx4.vin[0].scriptSig = CScript() << OP_1;
    tx4.vin[0].prevout.hash = tx1.GetHash();
    tx4.vin[0].prevout.n = 0;
    tx4.vout.resize(1);
    tx4.vout[0].nValue = 6LL * COIN;
    BOOST_CHECK(mempool.addUnchecked(tx4, 4LL * COIN));

    // tx4 becomes new heap top
    BOOST_CHECK(mempool.heapTx.top() == tx4);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxFees, 5LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxSize, 87+61);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nDepth, 2);


    // Spend tx1 output 1, 2, and tx2 output 1 with 1BTC fee
    tx5.vin.resize(3);
    tx5.vin[0].scriptSig = CScript() << OP_1;
    tx5.vin[0].prevout.hash = tx1.GetHash();
    tx5.vin[0].prevout.n = 1;
    tx5.vin[1].scriptSig = CScript() << OP_1;
    tx5.vin[1].prevout.hash = tx1.GetHash();
    tx5.vin[1].prevout.n = 2;
    tx5.vin[2].scriptSig = CScript() << OP_1;
    tx5.vin[2].prevout.hash = tx2.GetHash();
    tx5.vin[2].prevout.n = 1;
    tx5.vout.resize(1);
    tx5.vout[0].nValue = 29LL * COIN;
    BOOST_CHECK(mempool.addUnchecked(tx5, 1LL * COIN));

    // Fees counted pessimisticly, only one fee counted out of all three
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxFees, (2LL + 1LL) * COIN);

    // Size also counted pessimisticly, every txin sumed together, which even
    // double-counted tx1
    BOOST_CHECK_EQUAL(::GetSerializeSize(tx5, SER_NETWORK, PROTOCOL_VERSION), 145);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxSize, 87+87+69+145);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nDepth, 2);


    // Spend tx1.3 and tx5 with 1BTC fee
    tx6.vin.resize(2);
    tx6.vin[0].scriptSig = CScript() << OP_1;
    tx6.vin[0].prevout.hash = tx1.GetHash();
    tx6.vin[0].prevout.n = 3;
    tx6.vin[1].scriptSig = CScript() << OP_1;
    tx6.vin[1].prevout.hash = tx5.GetHash();
    tx6.vin[1].prevout.n = 0;
    tx6.vout.resize(1);
    tx6.vout[0].nValue = 39LL * COIN;
    BOOST_CHECK(mempool.addUnchecked(tx6, 1LL * COIN));

    // tx6 is not the heap top
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxFees, 4LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxSize, 87+(87+87+69+145)+103);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nDepth, 3);


    // Remove tx2 and update priorities
    std::set<uint256> changed;
    changed.insert(tx2.GetHash());
    mempool.remove(tx2.GetHash(), false);
    mempool.updatePriorities(changed);

    // Fees no longer include tx2, but they do include tx1
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxFees, (1LL + 1LL) * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxSize, 87+87+145);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nDepth, 2);

    // tx6 updated correctly
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxFees, (1LL + 1LL + 1LL) * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxSize, 87+(87+87+145)+103);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nDepth, 3);


    // Remove tx1
    changed.clear();
    changed.insert(tx1.GetHash());
    mempool.remove(tx1.GetHash(), false);
    mempool.updatePriorities(changed);

    BOOST_CHECK(mempool.heapTx.top() == tx4);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxFees, 4LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxSize, 61);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nDepth, 1);

    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxFees, 1LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxSize, 145);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nDepth, 1);

    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxFees, (1LL + 1LL) * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxSize, 145+103);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nDepth, 2);

    // Remove tx5
    changed.clear();
    changed.insert(tx5.GetHash());
    mempool.remove(tx5.GetHash(), false);
    mempool.updatePriorities(changed);

    BOOST_CHECK(mempool.heapTx.top() == tx4);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxFees, 4LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxSize, 61);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nDepth, 1);

    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxFees, 1LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxSize, 103);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nDepth, 1);


    // Reinstate tx1, tx2 and tx5 as though a re-org happened and they got put
    // back in the mempool. Note how the order ensures the sums won't be
    // correct.
    BOOST_CHECK(mempool.addUnchecked(tx5, 1LL * COIN));
    BOOST_CHECK(mempool.addUnchecked(tx2, 2LL * COIN));
    BOOST_CHECK(mempool.addUnchecked(tx1, 1LL * COIN));

    changed.clear();
    changed.insert(tx1.GetHash());
    changed.insert(tx2.GetHash());
    changed.insert(tx5.GetHash());
    mempool.updatePriorities(changed);

    BOOST_CHECK_EQUAL(mempool.lookup(tx1.GetHash()).nSumTxFees, 1LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx1.GetHash()).nSumTxSize, 87);
    BOOST_CHECK_EQUAL(mempool.lookup(tx1.GetHash()).nDepth, 1);

    BOOST_CHECK_EQUAL(mempool.lookup(tx2.GetHash()).nSumTxFees, 2LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx2.GetHash()).nSumTxSize, 69);
    BOOST_CHECK_EQUAL(mempool.lookup(tx2.GetHash()).nDepth, 1);

    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxFees, 5LL * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nSumTxSize, 87+61);
    BOOST_CHECK_EQUAL(mempool.lookup(tx4.GetHash()).nDepth, 2);

    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxFees, (2LL + 1LL) * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nSumTxSize, 87+87+69+145);
    BOOST_CHECK_EQUAL(mempool.lookup(tx5.GetHash()).nDepth, 2);

    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxFees, (2LL + 1LL + 1LL) * COIN);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nSumTxSize, (87+87+69+145)+87+103);
    BOOST_CHECK_EQUAL(mempool.lookup(tx6.GetHash()).nDepth, 3);


    // Remove everything
    mempool.remove(tx1.GetHash());
    mempool.remove(tx2.GetHash());
    mempool.remove(tx3.GetHash());

    BOOST_CHECK_EQUAL(mempool.size(), 0);
    BOOST_CHECK_EQUAL(mempool.heapTx.size(), 0);
    BOOST_CHECK_EQUAL(mempool.mapTx.size(), 0);
    BOOST_CHECK_EQUAL(mempool.mapNextTx.size(), 0);

    mempool.clear();
}

BOOST_AUTO_TEST_SUITE_END()
