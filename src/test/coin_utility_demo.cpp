// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Coin Creation and Management Utility Demo
 *
 * This test utility demonstrates how to create and manage coins (UTXOs)
 * in Bitcoin Core. It provides practical examples of:
 * - Creating coins with different properties
 * - Adding coins to the cache
 * - Spending coins
 * - Verifying coin states
 * - Working with coinbase vs regular transaction outputs
 */

#include <coins.h>
#include <consensus/amount.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coin_utility_demo, BasicTestingSetup)

/**
 * Utility function: Create a simple coin with specified parameters
 */
static Coin CreateTestCoin(CAmount value, int height, bool is_coinbase = false)
{
    CTxOut txout;
    txout.nValue = value;
    // Create a simple P2PKH-like script (OP_DUP OP_HASH160 <20 bytes> OP_EQUALVERIFY OP_CHECKSIG)
    txout.scriptPubKey = CScript() << OP_DUP << OP_HASH160
                                   << std::vector<uint8_t>(20, 0x01)
                                   << OP_EQUALVERIFY << OP_CHECKSIG;

    return Coin(std::move(txout), height, is_coinbase);
}

/**
 * Utility function: Create a coin with a custom script
 */
static Coin CreateCoinWithScript(CAmount value, int height, const CScript& script, bool is_coinbase = false)
{
    CTxOut txout;
    txout.nValue = value;
    txout.scriptPubKey = script;

    return Coin(std::move(txout), height, is_coinbase);
}

/**
 * Utility function: Print coin information (for debugging)
 */
static std::string CoinInfo(const Coin& coin, const COutPoint& outpoint)
{
    std::string info = "Coin at " + outpoint.ToString() + "\n";
    info += "  Value: " + std::to_string(coin.out.nValue) + " satoshis\n";
    info += "  Height: " + std::to_string(coin.nHeight) + "\n";
    info += "  IsCoinBase: " + std::string(coin.IsCoinBase() ? "true" : "false") + "\n";
    info += "  IsSpent: " + std::string(coin.IsSpent() ? "true" : "false") + "\n";
    info += "  ScriptPubKey size: " + std::to_string(coin.out.scriptPubKey.size()) + " bytes\n";
    info += "  Dynamic memory: " + std::to_string(coin.DynamicMemoryUsage()) + " bytes\n";
    return info;
}

/**
 * Demo 1: Basic coin creation
 * Shows how to create coins with different properties
 */
BOOST_AUTO_TEST_CASE(demo_basic_coin_creation)
{
    // Create a regular transaction output coin
    Coin regular_coin = CreateTestCoin(
        /* value */ COIN,           // 1 BTC = 100,000,000 satoshis
        /* height */ 100,            // Created at block height 100
        /* is_coinbase */ false
    );

    BOOST_CHECK_EQUAL(regular_coin.out.nValue, COIN);
    BOOST_CHECK_EQUAL(regular_coin.nHeight, 100);
    BOOST_CHECK_EQUAL(regular_coin.IsCoinBase(), false);
    BOOST_CHECK_EQUAL(regular_coin.IsSpent(), false);

    // Create a coinbase coin (block reward)
    Coin coinbase_coin = CreateTestCoin(
        /* value */ 50 * COIN,       // 50 BTC coinbase reward
        /* height */ 1,               // Created at block height 1
        /* is_coinbase */ true
    );

    BOOST_CHECK_EQUAL(coinbase_coin.out.nValue, 50 * COIN);
    BOOST_CHECK_EQUAL(coinbase_coin.nHeight, 1);
    BOOST_CHECK_EQUAL(coinbase_coin.IsCoinBase(), true);
    BOOST_CHECK_EQUAL(coinbase_coin.IsSpent(), false);

    // Create a small dust-like coin
    Coin dust_coin = CreateTestCoin(
        /* value */ 546,              // Dust threshold (546 satoshis)
        /* height */ 200,
        /* is_coinbase */ false
    );

    BOOST_CHECK_EQUAL(dust_coin.out.nValue, 546);
    BOOST_CHECK_GE(dust_coin.out.nValue, GetDustThreshold(dust_coin.out, DUST_RELAY_TX_FEE));
}

/**
 * Demo 2: Adding coins to cache
 * Shows how to add coins to CCoinsViewCache
 */
BOOST_AUTO_TEST_CASE(demo_add_coins_to_cache)
{
    CCoinsView base_view;
    CCoinsViewCache cache(&base_view);

    // Create an outpoint (transaction hash + output index)
    COutPoint outpoint1(Txid::FromUint256(InsecureRand256()), 0);

    // Create and add a coin
    Coin coin1 = CreateTestCoin(10 * COIN, 150, false);
    cache.AddCoin(outpoint1, std::move(coin1), false);

    // Verify the coin exists in cache
    BOOST_CHECK(cache.HaveCoin(outpoint1));

    // Retrieve the coin
    const Coin& retrieved = cache.AccessCoin(outpoint1);
    BOOST_CHECK_EQUAL(retrieved.out.nValue, 10 * COIN);
    BOOST_CHECK_EQUAL(retrieved.nHeight, 150);

    // Add multiple coins
    for (int i = 0; i < 5; i++) {
        COutPoint outpoint(Txid::FromUint256(InsecureRand256()), i);
        Coin coin = CreateTestCoin((i + 1) * COIN, 100 + i, false);
        cache.AddCoin(outpoint, std::move(coin), false);
    }

    // Cache should now have 6 coins
    size_t coin_count = 0;
    for (auto it = cache.Begin(); it != cache.End(); it = cache.NextAndMaybeErase(*it)) {
        coin_count++;
    }
    BOOST_CHECK_EQUAL(coin_count, 6);
}

/**
 * Demo 3: Spending coins
 * Shows how to spend coins from the cache
 */
BOOST_AUTO_TEST_CASE(demo_spend_coins)
{
    CCoinsView base_view;
    CCoinsViewCache cache(&base_view);

    // Create and add a coin
    COutPoint outpoint(Txid::FromUint256(InsecureRand256()), 0);
    Coin original_coin = CreateTestCoin(5 * COIN, 100, false);
    cache.AddCoin(outpoint, Coin(original_coin), false);

    // Verify coin exists and is unspent
    BOOST_CHECK(cache.HaveCoin(outpoint));
    const Coin& before_spend = cache.AccessCoin(outpoint);
    BOOST_CHECK(!before_spend.IsSpent());

    // Spend the coin (save it for undo data)
    Coin spent_coin;
    bool spent_success = cache.SpendCoin(outpoint, &spent_coin);
    BOOST_CHECK(spent_success);

    // Verify the spent coin has the same properties as original
    BOOST_CHECK_EQUAL(spent_coin.out.nValue, 5 * COIN);
    BOOST_CHECK_EQUAL(spent_coin.nHeight, 100);

    // After spending, the coin is marked as spent in cache
    // Note: The coin might still exist in cache as a spent entry or be removed
    // depending on the FRESH flag
}

/**
 * Demo 4: Coin states and properties
 * Shows different coin states and their properties
 */
BOOST_AUTO_TEST_CASE(demo_coin_states)
{
    // Test 1: Fresh coin (newly created)
    Coin fresh_coin = CreateTestCoin(COIN, 1, false);
    BOOST_CHECK(!fresh_coin.IsSpent());

    // Test 2: Spent coin (cleared)
    Coin spent_coin = CreateTestCoin(COIN, 1, false);
    spent_coin.Clear();
    BOOST_CHECK(spent_coin.IsSpent());
    BOOST_CHECK_EQUAL(spent_coin.out.nValue, -1);  // Null value
    BOOST_CHECK_EQUAL(spent_coin.nHeight, 0);
    BOOST_CHECK_EQUAL(spent_coin.IsCoinBase(), false);

    // Test 3: Coinbase maturity (coinbase coins need 100 confirmations)
    const int COINBASE_MATURITY = 100;
    Coin coinbase = CreateTestCoin(50 * COIN, 50, true);
    int current_height = 149;

    // Not mature yet (need height 150)
    BOOST_CHECK_LT(current_height - coinbase.nHeight, COINBASE_MATURITY);

    current_height = 150;
    // Now mature
    BOOST_CHECK_GE(current_height - coinbase.nHeight, COINBASE_MATURITY);
}

/**
 * Demo 5: Different script types
 * Shows coins with different locking scripts
 */
BOOST_AUTO_TEST_CASE(demo_different_scripts)
{
    // P2PKH (Pay to Public Key Hash)
    CScript p2pkh_script = CScript() << OP_DUP << OP_HASH160
                                     << std::vector<uint8_t>(20, 0x01)
                                     << OP_EQUALVERIFY << OP_CHECKSIG;
    Coin p2pkh_coin = CreateCoinWithScript(COIN, 100, p2pkh_script, false);
    BOOST_CHECK_EQUAL(p2pkh_coin.out.scriptPubKey.size(), 25);

    // P2SH (Pay to Script Hash)
    CScript p2sh_script = CScript() << OP_HASH160
                                    << std::vector<uint8_t>(20, 0x02)
                                    << OP_EQUAL;
    Coin p2sh_coin = CreateCoinWithScript(COIN, 100, p2sh_script, false);
    BOOST_CHECK_EQUAL(p2sh_coin.out.scriptPubKey.size(), 23);

    // P2WPKH (Pay to Witness Public Key Hash) - SegWit
    CScript p2wpkh_script = CScript() << OP_0 << std::vector<uint8_t>(20, 0x03);
    Coin p2wpkh_coin = CreateCoinWithScript(COIN, 100, p2wpkh_script, false);
    BOOST_CHECK_EQUAL(p2wpkh_coin.out.scriptPubKey.size(), 22);

    // Each coin type has different memory usage
    BOOST_CHECK_GT(p2pkh_coin.DynamicMemoryUsage(), 0);
}

/**
 * Demo 6: Cache hierarchy and flushing
 * Shows how coins flow through the cache hierarchy
 */
BOOST_AUTO_TEST_CASE(demo_cache_hierarchy)
{
    // Create a three-layer hierarchy: base -> cache1 -> cache2
    CCoinsView base_view;
    CCoinsViewCache cache1(&base_view);
    CCoinsViewCache cache2(&cache1);

    // Add coin to top-level cache (cache2)
    COutPoint outpoint(Txid::FromUint256(InsecureRand256()), 0);
    Coin coin = CreateTestCoin(10 * COIN, 100, false);
    cache2.AddCoin(outpoint, std::move(coin), false);

    // Coin is in cache2 but not yet in cache1 or base
    BOOST_CHECK(cache2.HaveCoin(outpoint));
    BOOST_CHECK(!cache1.HaveCoin(outpoint));

    // Flush cache2 to cache1
    BOOST_CHECK(cache2.Flush());

    // Now coin is in cache1
    BOOST_CHECK(cache1.HaveCoin(outpoint));
    BOOST_CHECK(cache2.HaveCoin(outpoint));  // Still accessible from cache2

    // Flush cache1 to base
    BOOST_CHECK(cache1.Flush());

    // Coin has propagated through all levels
    BOOST_CHECK(base_view.HaveCoin(outpoint));
}

/**
 * Demo 7: Bulk coin operations
 * Shows how to work with multiple coins efficiently
 */
BOOST_AUTO_TEST_CASE(demo_bulk_operations)
{
    CCoinsView base_view;
    CCoinsViewCache cache(&base_view);

    // Create multiple coins simulating transaction outputs
    const int NUM_OUTPUTS = 10;
    std::vector<COutPoint> outpoints;

    // Simulate a transaction with multiple outputs
    Txid txid = Txid::FromUint256(InsecureRand256());
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        COutPoint outpoint(txid, i);
        outpoints.push_back(outpoint);

        // Each output has different value
        Coin coin = CreateTestCoin((i + 1) * COIN / 10, 100, false);
        cache.AddCoin(outpoint, std::move(coin), false);
    }

    // Verify all coins exist
    for (const auto& outpoint : outpoints) {
        BOOST_CHECK(cache.HaveCoin(outpoint));
    }

    // Spend some of the outputs (simulating a new transaction)
    std::vector<Coin> spent_coins;
    for (int i = 0; i < 5; i++) {
        Coin spent;
        cache.SpendCoin(outpoints[i], &spent);
        spent_coins.push_back(spent);
    }

    // Verify we saved the spent coins correctly
    BOOST_CHECK_EQUAL(spent_coins.size(), 5);
    for (size_t i = 0; i < spent_coins.size(); i++) {
        BOOST_CHECK_EQUAL(spent_coins[i].out.nValue, (i + 1) * COIN / 10);
    }
}

/**
 * Demo 8: Memory usage tracking
 * Shows how to track dynamic memory usage of coins
 */
BOOST_AUTO_TEST_CASE(demo_memory_usage)
{
    // Small script
    CScript small_script = CScript() << OP_TRUE;
    Coin small_coin = CreateCoinWithScript(COIN, 100, small_script, false);
    size_t small_memory = small_coin.DynamicMemoryUsage();

    // Large script (100 bytes)
    CScript large_script;
    large_script.resize(100, 0x01);
    Coin large_coin = CreateCoinWithScript(COIN, 100, large_script, false);
    size_t large_memory = large_coin.DynamicMemoryUsage();

    // Larger script uses more memory
    BOOST_CHECK_GT(large_memory, small_memory);

    // Test cache memory tracking
    CCoinsView base_view;
    CCoinsViewCache cache(&base_view);

    size_t initial_usage = cache.DynamicMemoryUsage();

    // Add coins and track memory growth
    for (int i = 0; i < 100; i++) {
        COutPoint outpoint(Txid::FromUint256(InsecureRand256()), 0);
        Coin coin = CreateTestCoin(COIN, 100 + i, false);
        cache.AddCoin(outpoint, std::move(coin), false);
    }

    size_t after_add_usage = cache.DynamicMemoryUsage();
    BOOST_CHECK_GT(after_add_usage, initial_usage);
}

BOOST_AUTO_TEST_SUITE_END()
