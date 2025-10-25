// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <index/txospenderindex.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txospenderindex_tests)

BOOST_FIXTURE_TEST_CASE(txospenderindex_initial_sync, TestChain100Setup)
{
    TxoSpenderIndex txospenderindex(interfaces::MakeChain(m_node), 1 << 20, true);
    BOOST_REQUIRE(txospenderindex.Init());

    // Mine blocks for coinbase maturity, so we can spend some coinbase outputs in the test.
    for (int i = 0; i < 50; i++) {
        std::vector<CMutableTransaction> no_txns;
        CreateAndProcessBlock(no_txns, this->m_coinbase_txns[i]->vout[0].scriptPubKey);
    }
    std::vector<COutPoint> spent(10);
    std::vector<CMutableTransaction> spender(spent.size());

    for (size_t i = 0; i < spent.size(); i++) {
        spent[i] = COutPoint(this->m_coinbase_txns[i]->GetHash(), 0);
        spender[i].version = 1;
        spender[i].vin.resize(1);
        spender[i].vin[0].prevout.hash = spent[i].hash;
        spender[i].vin[0].prevout.n = spent[i].n;
        spender[i].vout.resize(1);
        spender[i].vout[0].nValue = this->m_coinbase_txns[i]->GetValueOut();
        spender[i].vout[0].scriptPubKey = this->m_coinbase_txns[i]->vout[0].scriptPubKey;

        // Sign:
        std::vector<unsigned char> vchSig;
        const uint256 hash = SignatureHash(this->m_coinbase_txns[i]->vout[0].scriptPubKey, spender[i], 0, SIGHASH_ALL, 0, SigVersion::BASE);
        coinbaseKey.Sign(hash, vchSig);
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        spender[i].vin[0].scriptSig << vchSig;
    }

    CreateAndProcessBlock(spender, this->m_coinbase_txns[0]->vout[0].scriptPubKey);

    // Transaction should not be found in the index before it is started.
    for (const auto& outpoint : spent) {
        BOOST_CHECK(!txospenderindex.FindSpender(outpoint));
    }

    // BlockUntilSyncedToCurrentChain should return false before txospenderindex is started.
    BOOST_CHECK(!txospenderindex.BlockUntilSyncedToCurrentChain());

    txospenderindex.Sync();

    for (size_t i = 0; i < spent.size(); i++) {
        BOOST_CHECK_EQUAL(txospenderindex.FindSpender(spent[i])->GetHash(), spender[i].GetHash());
    }

    // It is not safe to stop and destroy the index until it finishes handling
    // the last BlockConnected notification. The BlockUntilSyncedToCurrentChain()
    // call above is sufficient to ensure this, but the
    // SyncWithValidationInterfaceQueue() call below is also needed to ensure
    // TSAN always sees the test thread waiting for the notification thread, and
    // avoid potential false positive reports.
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // shutdown sequence (c.f. Shutdown() in init.cpp)
    txospenderindex.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
