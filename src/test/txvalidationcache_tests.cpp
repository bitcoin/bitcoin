// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "txmempool.h"
#include "random.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"
#include "utiltime.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(tx_validationcache_tests)

BOOST_AUTO_TEST_CASE(tx_validationcache)
{
    // Start with an empty memory pool
    CTxMemPool pool(CFeeRate(0));

    // Create eleven transactions, add five of them to the pool:
    CMutableTransaction txs[11];
    for (int i = 0; i < 11; i++)
    {
        txs[i].vin.resize(1);
        txs[i].vin[0].scriptSig = CScript() << OP_11;
        txs[i].vin[0].prevout.hash = GetRandHash();
        txs[i].vin[0].prevout.n = 0;
        txs[i].vout.resize(1);
        txs[i].vout[0].nValue = 5000000000LL;
        txs[i].vout[0].scriptPubKey = CScript() << OP_EQUAL;
        if (i < 5)
            pool.addUnchecked(txs[i].GetHash(),
                                 CTxMemPoolEntry(txs[i], 11, GetTime(), 111.0, 11, STANDARD_SCRIPT_VERIFY_FLAGS));
    }

    unsigned int nHits[3] = { 0, 0, 0 }; // Test three different sets of flags
    for (int i = 0; i < 11; i++)
    {
        uint256 txid = txs[i].GetHash();
        if (pool.validated(txid, STANDARD_SCRIPT_VERIFY_FLAGS)) ++nHits[0];
        if (pool.validated(txid, SCRIPT_VERIFY_NONE)) ++nHits[1];
        if (pool.validated(txid, SCRIPT_VERIFY_P2SH)) ++nHits[2];
    }
    BOOST_CHECK_EQUAL(nHits[0], 5);
    BOOST_CHECK_EQUAL(nHits[1], 5);
    BOOST_CHECK_EQUAL(nHits[2], 5);

    // Add txs[5] with less-strict validation flag, and
    // make sure validation caching code Does The Right Thing:
    pool.addUnchecked(txs[5].GetHash(),
                         CTxMemPoolEntry(txs[5], 11, GetTime(), 111.0, 11, SCRIPT_VERIFY_P2SH));

    BOOST_CHECK(!pool.validated(txs[5].GetHash(), STANDARD_SCRIPT_VERIFY_FLAGS));
    BOOST_CHECK(pool.validated(txs[5].GetHash(), SCRIPT_VERIFY_NONE));
}

static bool
ToMemPool(CMutableTransaction& tx)
{
    LOCK(cs_main);

    CValidationState state;
    return AcceptToMemoryPool(mempool, state, tx, false, NULL, false);
}

BOOST_FIXTURE_TEST_CASE(tx_mempool_block_doublespend, TestChain100Setup)
{
    // Make sure skipping validation of transctions that were
    // validated going into the memory pool does not allow
    // double-spends in blocks to pass validation when they should not.
    unsigned int startingHeight = chainActive.Height();

    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    // Create a double-spend of mature coinbase txn:
    std::vector<CMutableTransaction> spends;
    spends.resize(2);
    for (int i = 0; i < 2; i++)
    {
        spends[i].vin.resize(1);
        spends[i].vin[0].prevout.hash = coinbaseTxns[0].GetHash();
        spends[i].vin[0].prevout.n = 0;
        spends[i].vout.resize(1);
        spends[i].vout[0].nValue = 11*CENT;
        spends[i].vout[0].scriptPubKey = scriptPubKey;

        // Sign:
        std::vector<unsigned char> vchSig;
        uint256 hash = SignatureHash(scriptPubKey, spends[i], 0, SIGHASH_ALL);
        BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        spends[i].vin[0].scriptSig << vchSig;
    }

    CBlock block;

    // Test 1: block with both of those transactions should be rejected.
    block = CreateAndProcessBlock(spends, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() != block.GetHash());

    // Test 2: ... and should be rejected if spend1 is in the memory pool
    BOOST_CHECK(ToMemPool(spends[0]));
    block = CreateAndProcessBlock(spends, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() != block.GetHash());
    mempool.clear();

    // Test 3: ... and should be rejected if spend2 is in the memory pool
    BOOST_CHECK(ToMemPool(spends[1]));
    block = CreateAndProcessBlock(spends, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() != block.GetHash());
    mempool.clear();

    // Final sanity test: first spend in mempool, second in block, that's OK:
    std::vector<CMutableTransaction> oneSpend;
    oneSpend.push_back(spends[0]);
    BOOST_CHECK(ToMemPool(spends[1]));
    block = CreateAndProcessBlock(oneSpend, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    // spends[1] should have been removed from the mempool when the
    // block with spends[0] is accepted:
    BOOST_CHECK_EQUAL(mempool.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
