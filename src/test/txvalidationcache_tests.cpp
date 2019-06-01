// Copyright (c) 2011-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <key.h>
#include <validation.h>
#include <miner.h>
#include <pubkey.h>
#include <txmempool.h>
#include <random.h>
#include <script/standard.h>
#include <script/sign.h>
#include <test/setup_common.h>
#include <util/time.h>
#include <keystore.h>

#include <boost/test/unit_test.hpp>
// SYSCOIN
bool CheckInputs(const CTransaction& tx, CValidationState &state, const CCoinsViewCache &inputs, bool fScriptChecks, unsigned int flags, bool cacheSigStore, bool cacheFullScriptStore, PrecomputedTransactionData& txdata, std::vector<CScriptCheck> *pvChecks = nullptr, std::vector<CScriptCheckConcurrent> *pvChecksConcurrent = nullptr, uint256* hashCacheEntry = nullptr);

BOOST_AUTO_TEST_SUITE(tx_validationcache_tests)

static bool
ToMemPool(const CMutableTransaction& tx)
{
    LOCK(cs_main);

    CValidationState state;
    return AcceptToMemoryPool(mempool, state, MakeTransactionRef(tx), nullptr /* pfMissingInputs */,
                              nullptr /* plTxnReplaced */, true /* bypass_limits */, 0 /* nAbsurdFee */);
}

BOOST_FIXTURE_TEST_CASE(tx_mempool_block_doublespend, TestChain100Setup)
{
    // Make sure skipping validation of transactions that were
    // validated going into the memory pool does not allow
    // double-spends in blocks to pass validation when they should not.

    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    // Create a double-spend of mature coinbase txn:
    std::vector<CMutableTransaction> spends;
    spends.resize(2);
    for (int i = 0; i < 2; i++)
    {
        spends[i].nVersion = 1;
        spends[i].vin.resize(1);
        spends[i].vin[0].prevout.hash = m_coinbase_txns[0]->GetHash();
        spends[i].vin[0].prevout.n = 0;
        spends[i].vout.resize(1);
        spends[i].vout[0].nValue = 11*CENT;
        spends[i].vout[0].scriptPubKey = scriptPubKey;

        // Sign:
        std::vector<unsigned char> vchSig;
        uint256 hash = SignatureHash(scriptPubKey, spends[i], 0, SIGHASH_ALL, 0, SigVersion::BASE);
        BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        spends[i].vin[0].scriptSig << vchSig;
    }

    CBlock block;

    // Test 1: block with both of those transactions should be rejected.
    block = CreateAndProcessBlock(spends, scriptPubKey);
    {
        LOCK(cs_main);
        BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() != block.GetHash());
    }

    // Test 2: ... and should be rejected if spend1 is in the memory pool
    BOOST_CHECK(ToMemPool(spends[0]));
    block = CreateAndProcessBlock(spends, scriptPubKey);
    {
        LOCK(cs_main);
        BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() != block.GetHash());
    }
    mempool.clear();

    // Test 3: ... and should be rejected if spend2 is in the memory pool
    BOOST_CHECK(ToMemPool(spends[1]));
    block = CreateAndProcessBlock(spends, scriptPubKey);
    {
        LOCK(cs_main);
        BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() != block.GetHash());
    }
    mempool.clear();

    // Final sanity test: first spend in mempool, second in block, that's OK:
    std::vector<CMutableTransaction> oneSpend;
    oneSpend.push_back(spends[0]);
    BOOST_CHECK(ToMemPool(spends[1]));
    block = CreateAndProcessBlock(oneSpend, scriptPubKey);
    {
        LOCK(cs_main);
        BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());
    }
    // spends[1] should have been removed from the mempool when the
    // block with spends[0] is accepted:
    BOOST_CHECK_EQUAL(mempool.size(), 0U);
}


BOOST_AUTO_TEST_SUITE_END()
