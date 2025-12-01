// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * Mock CCoinsView that simulates write failures
 */
class CCoinsViewWriteFailure : public CCoinsView
{
private:
    FastRandomContext& m_rng;
    uint256 m_hashBestBlock;
    std::map<COutPoint, Coin> m_map;
    
    // Failure simulation parameters
    bool m_fail_next_write{false};
    size_t m_fail_after_n_writes{0};
    size_t m_write_count{0};
    bool m_fail_on_partial_write{false};
    size_t m_partial_write_threshold{0};

public:
    explicit CCoinsViewWriteFailure(FastRandomContext& rng) : m_rng{rng} {}

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override
    {
        auto it = m_map.find(outpoint);
        if (it != m_map.end() && !it->second.IsSpent()) {
            return it->second;
        }
        return std::nullopt;
    }

    uint256 GetBestBlock() const override { return m_hashBestBlock; }

    bool BatchWrite(CoinsViewCacheCursor& cursor, const uint256& hashBlock) override
    {
        // Simulate immediate write failure
        if (m_fail_next_write) {
            m_fail_next_write = false;
            return false;
        }

        // Simulate failure after N successful writes
        if (m_fail_after_n_writes > 0 && m_write_count >= m_fail_after_n_writes) {
            return false;
        }

        size_t entries_written = 0;
        for (auto it{cursor.Begin()}; it != cursor.End(); it = cursor.NextAndMaybeErase(*it)) {
            if (it->second.IsDirty()) {
                // Simulate partial write failure
                if (m_fail_on_partial_write && entries_written >= m_partial_write_threshold) {
                    return false;
                }
                
                m_map[it->first] = it->second.coin;
                if (it->second.coin.IsSpent() && m_rng.randrange(3) == 0) {
                    m_map.erase(it->first);
                }
                entries_written++;
            }
        }

        if (!hashBlock.IsNull()) {
            m_hashBestBlock = hashBlock;
        }

        m_write_count++;
        return true;
    }

    // Test control methods
    void SetFailNextWrite(bool fail) { m_fail_next_write = fail; }
    void SetFailAfterNWrites(size_t n) { m_fail_after_n_writes = n; }
    void SetFailOnPartialWrite(size_t threshold) {
        m_fail_on_partial_write = true;
        m_partial_write_threshold = threshold;
    }
    void ResetWriteCount() { m_write_count = 0; }
    size_t GetWriteCount() const { return m_write_count; }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(coins_write_failure_tests, BasicTestingSetup)

/**
 * Test that Flush() properly handles BatchWrite failures
 */
BOOST_AUTO_TEST_CASE(flush_handles_write_failure)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache(&base);

    // Add some coins to the cache
    for (int i = 0; i < 10; i++) {
        COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
        Coin coin;
        coin.out.nValue = 1000 + i;
        coin.nHeight = 100;
        cache.AddCoin(outpoint, std::move(coin), false);
    }

    // Verify cache has coins
    BOOST_CHECK_GT(cache.GetCacheSize(), 0);
    size_t initial_cache_size = cache.GetCacheSize();

    // Simulate write failure
    base.SetFailNextWrite(true);
    
    // Flush should fail
    BOOST_CHECK(!cache.Flush());
    
    // Cache should still contain the coins (not cleared on failure)
    BOOST_CHECK_EQUAL(cache.GetCacheSize(), initial_cache_size);
}

/**
 * Test that Sync() properly handles BatchWrite failures
 */
BOOST_AUTO_TEST_CASE(sync_handles_write_failure)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache(&base);

    // Add some coins
    COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
    Coin coin;
    coin.out.nValue = 5000;
    coin.nHeight = 200;
    cache.AddCoin(outpoint, std::move(coin), false);

    size_t initial_cache_size = cache.GetCacheSize();

    // Simulate write failure
    base.SetFailNextWrite(true);
    
    // Sync should fail
    BOOST_CHECK(!cache.Sync());
    
    // Cache should still contain the coins
    BOOST_CHECK_EQUAL(cache.GetCacheSize(), initial_cache_size);
    
    // Coin should still be accessible
    BOOST_CHECK(cache.HaveCoin(outpoint));
}

/**
 * Test recovery after write failure
 */
BOOST_AUTO_TEST_CASE(recovery_after_write_failure)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache(&base);

    COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
    Coin coin;
    coin.out.nValue = 10000;
    coin.nHeight = 300;
    cache.AddCoin(outpoint, std::move(coin), false);

    // First flush fails
    base.SetFailNextWrite(true);
    BOOST_CHECK(!cache.Flush());
    
    // Coin should still be in cache
    BOOST_CHECK(cache.HaveCoin(outpoint));
    
    // Second flush succeeds
    base.SetFailNextWrite(false);
    BOOST_CHECK(cache.Flush());
    
    // Cache should be empty after successful flush
    BOOST_CHECK_EQUAL(cache.GetCacheSize(), 0);
    
    // Coin should now be in base view
    BOOST_CHECK(base.GetCoin(outpoint).has_value());
}

/**
 * Test partial write failure (failure in the middle of writing multiple coins)
 */
BOOST_AUTO_TEST_CASE(partial_write_failure)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache(&base);

    std::vector<COutPoint> outpoints;
    
    // Add 10 coins
    for (int i = 0; i < 10; i++) {
        COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
        outpoints.push_back(outpoint);
        Coin coin;
        coin.out.nValue = 1000 * (i + 1);
        coin.nHeight = 100 + i;
        cache.AddCoin(outpoint, std::move(coin), false);
    }

    // Simulate failure after writing 5 coins
    base.SetFailOnPartialWrite(5);
    
    // Flush should fail
    BOOST_CHECK(!cache.Flush());
    
    // Cache should still have all coins since flush failed
    BOOST_CHECK_EQUAL(cache.GetCacheSize(), 10);
}

/**
 * Test write failure after multiple successful writes
 */
BOOST_AUTO_TEST_CASE(write_failure_after_n_writes)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache(&base);

    // Allow 3 successful writes, then fail
    base.SetFailAfterNWrites(3);

    // Perform multiple flush operations
    for (int round = 0; round < 5; round++) {
        COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
        Coin coin;
        coin.out.nValue = 1000 * round;
        coin.nHeight = 100 * round;
        cache.AddCoin(outpoint, std::move(coin), false);
        
        bool flush_result = cache.Flush();
        
        if (round < 3) {
            // First 3 should succeed
            BOOST_CHECK(flush_result);
            BOOST_CHECK_EQUAL(cache.GetCacheSize(), 0);
        } else {
            // After 3 writes, should fail
            BOOST_CHECK(!flush_result);
            BOOST_CHECK_GT(cache.GetCacheSize(), 0);
        }
    }
}

/**
 * Test that dirty flags are preserved on write failure
 */
BOOST_AUTO_TEST_CASE(dirty_flags_preserved_on_failure)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache(&base);

    COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
    Coin coin;
    coin.out.nValue = 50000;
    coin.nHeight = 500;
    cache.AddCoin(outpoint, std::move(coin), false);

    // Verify coin is in cache and dirty
    BOOST_CHECK(cache.HaveCoinInCache(outpoint));
    
    // Fail the write
    base.SetFailNextWrite(true);
    BOOST_CHECK(!cache.Flush());
    
    // Coin should still be in cache
    BOOST_CHECK(cache.HaveCoinInCache(outpoint));
    
    // Now succeed the write
    base.SetFailNextWrite(false);
    BOOST_CHECK(cache.Flush());
    
    // Coin should be written to base
    BOOST_CHECK(base.GetCoin(outpoint).has_value());
    BOOST_CHECK_EQUAL(base.GetCoin(outpoint)->out.nValue, 50000);
}

/**
 * Test multiple cache layers with write failure
 */
BOOST_AUTO_TEST_CASE(multi_layer_cache_write_failure)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache1(&base);
    CCoinsViewCache cache2(&cache1);

    COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
    Coin coin;
    coin.out.nValue = 75000;
    coin.nHeight = 750;
    cache2.AddCoin(outpoint, std::move(coin), false);

    // Flush cache2 to cache1 (should succeed - no base write yet)
    BOOST_CHECK(cache2.Flush());
    BOOST_CHECK_EQUAL(cache2.GetCacheSize(), 0);
    BOOST_CHECK_GT(cache1.GetCacheSize(), 0);

    // Now try to flush cache1 to base with failure
    base.SetFailNextWrite(true);
    BOOST_CHECK(!cache1.Flush());
    
    // cache1 should still have the coin
    BOOST_CHECK_GT(cache1.GetCacheSize(), 0);
    
    // Retry flush (should succeed)
    base.SetFailNextWrite(false);
    BOOST_CHECK(cache1.Flush());
    
    // Now base should have the coin
    BOOST_CHECK(base.GetCoin(outpoint).has_value());
}

/**
 * Test that spent coins are handled correctly on write failure
 */
BOOST_AUTO_TEST_CASE(spent_coins_write_failure)
{
    CCoinsViewWriteFailure base{m_rng};
    CCoinsViewCache cache(&base);

    COutPoint outpoint(Txid::FromUint256(m_rng.rand256()), 0);
    
    // First, add and successfully write a coin
    Coin coin;
    coin.out.nValue = 25000;
    coin.nHeight = 250;
    cache.AddCoin(outpoint, std::move(coin), false);
    BOOST_CHECK(cache.Flush());
    
    // Verify it's in base
    BOOST_CHECK(base.GetCoin(outpoint).has_value());
    
    // Now spend it
    BOOST_CHECK(cache.SpendCoin(outpoint));
    
    // Try to flush the spend with failure
    base.SetFailNextWrite(true);
    BOOST_CHECK(!cache.Flush());
    
    // The spend should still be in cache
    BOOST_CHECK_GT(cache.GetCacheSize(), 0);
    
    // Base should still have the unspent coin
    BOOST_CHECK(base.GetCoin(outpoint).has_value());
    
    // Retry flush (should succeed)
    base.SetFailNextWrite(false);
    BOOST_CHECK(cache.Flush());
    
    // Now the coin should be spent in base
    BOOST_CHECK(!base.GetCoin(outpoint).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
