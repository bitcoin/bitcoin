// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"
#include "random.h"
#include "script/standard.h"
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

BOOST_AUTO_TEST_SUITE_END()
