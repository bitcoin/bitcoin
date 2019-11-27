// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <chainparams.h>
#include <index/addrindex.h>
#include <index/txindex.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(addr_index_tests)

BOOST_FIXTURE_TEST_CASE(addr_index_initial_sync_and_spends, TestChain100Setup)
{
    AddrIndex addr_index(1 << 20, true);
    CScript coinbase_script_pub_key = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    const int max_count = 500; // Use a max count of 500, which is much higher than the number of results in the test.
    const int skip = 0;

    // Transactions should not be found in the index before it is started.
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> spends;
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> creations;
    BOOST_CHECK(!addr_index.FindTxsByScript(max_count, skip, coinbase_script_pub_key, spends, creations));

    // BlockUntilSyncedToCurrentChain should return false before addr_index is started.
    BOOST_CHECK(!addr_index.BlockUntilSyncedToCurrentChain());
    addr_index.Start();

    // Mine blocks for coinbase maturity, so we can spend some coinbase outputs in the test.
    for (int i = 0; i < 20; i++) {
        std::vector<CMutableTransaction> no_txns;
        CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
    }

    // Allow addr_index to catch up with the block index.
    constexpr int64_t timeout_ms = 10 * 1000;
    int64_t time_start = GetTimeMillis();
    while (!addr_index.BlockUntilSyncedToCurrentChain()) {
        BOOST_REQUIRE(time_start + timeout_ms > GetTimeMillis());
        UninterruptibleSleep(std::chrono::milliseconds{100});
    }

    // Check that addr_index has all coinbase outputs indexed.
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> spends2;
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> creations2;
    if (!addr_index.FindTxsByScript(max_count, skip, coinbase_script_pub_key, spends2, creations2)) {
            BOOST_ERROR("FindTxsByScript failed");
    }
    // The coinbase transactions have the same scriptPubKey in their output.
    BOOST_CHECK_EQUAL(spends2.size(), 0u);
    BOOST_CHECK_EQUAL(creations2.size(), 120u);

    // Create several new key pairs to test sending to many different addresses in the same block.
    std::vector<CKey> priv_keys(10);
    std::vector<CScript> script_pub_keys(10);
    for (int i = 0; i < 10; i++) {
        priv_keys[i].MakeNewKey(true);
        script_pub_keys[i] = CScript() <<  ToByteVector(priv_keys[i].GetPubKey()) << OP_CHECKSIG;
    }

    // Create a transaction sending to each of the new addresses.
    std::vector<CMutableTransaction> spend_txns(10);
    CreateSpendingTxs(0, script_pub_keys, spend_txns, coinbase_script_pub_key);

    const CBlock& block = CreateAndProcessBlock(spend_txns, coinbase_script_pub_key);
    const uint256 block_hash = block.GetHash();
    BOOST_CHECK(addr_index.BlockUntilSyncedToCurrentChain()); // Let the address index catch up.
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block_hash); // Sanity check to make sure this block is actually being used.

    // Now check that all the addresses we sent to are present in the index.
    for (int i = 0; i < 10; i++) {
        std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> spends3;
        std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> creations3;
        if (!addr_index.FindTxsByScript(max_count, skip, script_pub_keys[i], spends3, creations3)) {
            BOOST_ERROR("FindTxsByScript failed");
        }

        // The coinbase transactions have the same scriptPubKey in their output.
        BOOST_CHECK_EQUAL(spends3.size(), 0u);
        BOOST_CHECK_EQUAL(creations3.size(), 1u);

        // Confirm that the transaction's destination is in the index.
        bool found_tx = false;
        for (const auto& creation : creations3) {
            if (creation.second.first->GetHash() == spend_txns[i].GetHash()) {
                found_tx = true;
                break;
            }
        }

        if (!found_tx) BOOST_ERROR("Transaction not found by destination");
    }

    // Now we'll create transaction that only send to the first 5 addresses we made.
    // Then we can check that the number of txs for those addresses increases, while
    // the number of txs for the other address remains the same.
    std::vector<CMutableTransaction> spend_txns2(5);
    CreateSpendingTxs(10, script_pub_keys, spend_txns2, coinbase_script_pub_key);

    const CBlock& block2 = CreateAndProcessBlock(spend_txns2, coinbase_script_pub_key);
    const uint256 block_hash2 = block2.GetHash();
    BOOST_CHECK(addr_index.BlockUntilSyncedToCurrentChain());
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block_hash2);

    for (int i = 0; i < 10; i++) {
        std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> spends4;
        std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> creations4;

        if (!addr_index.FindTxsByScript(max_count, skip, script_pub_keys[i], spends4, creations4)) {
            BOOST_ERROR("FindTxsByScript failed");
        }

        // The coinbase transactions have the same scriptPubKey in their output.
        BOOST_CHECK_EQUAL(spends4.size(), 0u);

        // Expect 2 transactions for those sent to twice, 1 for the rest.
        if (i >= 5) {
            BOOST_CHECK_EQUAL(creations4.size(), 1u);
        } else {
            BOOST_CHECK_EQUAL(creations4.size(), 2u);
        }

        // Confirm that the transaction's destination is in the index.
        bool found_tx = false;
        for (const auto& creation :creations4) {
            if (i >= 5) {
                if (creation.second.first->GetHash() == spend_txns[i].GetHash()) {
                    found_tx = true;
                    break;
                }
            } else {
                if (creation.second.first->GetHash() == spend_txns2[i].GetHash()) {
                    found_tx = true;
                    break;
                }
            }
        }
        if (!found_tx) BOOST_ERROR("Transaction not found by destination");
    }

    // Check the results for the coinbase_script_pub_key again.
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> spends5;
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> creations5;
    if (!addr_index.FindTxsByScript(max_count, skip, coinbase_script_pub_key, spends5, creations5)) {
        BOOST_ERROR("FindTxsByScript failed");
    }
    BOOST_CHECK_EQUAL(spends5.size(), 10u + 5u);
    BOOST_CHECK_EQUAL(creations5.size(), 100u + 20u + 2u);

    // Check that max_count is respected by setting a low max_count of 22.
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> spends6;
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> creations6;
    if (!addr_index.FindTxsByScript(22, skip, coinbase_script_pub_key, spends6, creations6)) {
            BOOST_ERROR("FindTxsByScript failed");
    }
    BOOST_CHECK_EQUAL(spends6.size()+creations6.size(), 22u);

    // Check that the skip parameter is respected by setting a skip of 40.
    const unsigned int skip_test_val = 40;
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> spends7;
    std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> creations7;
    if (!addr_index.FindTxsByScript(max_count, skip_test_val, coinbase_script_pub_key, spends7, creations7)) {
            BOOST_ERROR("FindTxsByScript failed");
    }
    BOOST_CHECK_EQUAL(spends7.size()+creations7.size(), spends5.size()+creations5.size()-skip_test_val);

    addr_index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
