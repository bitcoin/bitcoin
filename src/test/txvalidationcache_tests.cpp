// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/validation.h"
#include "key.h"
#include "main.h"
#include "miner.h"
#include "parallel.h"
#include "pubkey.h"
#include "random.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"
#include "txmempool.h"
#include "utiltime.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txvalidationcache_tests) // BU harmonize suite name with filename

static bool ToMemPool(CMutableTransaction &tx)
{
    LOCK(cs_main);

    CValidationState state;
    return AcceptToMemoryPool(mempool, state, tx, false, NULL, true, false);
}

BOOST_FIXTURE_TEST_CASE(tx_mempool_block_doublespend, TestChain100Setup)
{
    // Make sure skipping validation of transctions that were
    // validated going into the memory pool does not allow
    // double-spends in blocks to pass validation when they should not.

    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;


    unsigned int sighashType = SIGHASH_ALL;
    if (chainActive.Tip()->IsforkActiveOnNextBlock(miningForkTime.value))
        sighashType |= SIGHASH_FORKID;

    // Create a double-spend of mature coinbase txn:
    std::vector<CMutableTransaction> spends;
    spends.resize(2);
    for (int i = 0; i < 2; i++)
    {
        spends[i].vin.resize(1);
        spends[i].vin[0].prevout.hash = coinbaseTxns[0].GetHash();
        spends[i].vin[0].prevout.n = 0;
        spends[i].vout.resize(1);
        spends[i].vout[0].nValue = 11 * CENT;
        spends[i].vout[0].scriptPubKey = scriptPubKey;

        // Sign:
        std::vector<unsigned char> vchSig;
        uint256 hash = SignatureHash(scriptPubKey, spends[i], 0, sighashType, coinbaseTxns[0].vout[0].nValue, 0);
        BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)sighashType);
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
