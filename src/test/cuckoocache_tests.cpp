// Copyright (c) 2012-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <boost/test/unit_test.hpp>
#include <cuckoocache.h>
#include <script/sigcache.h>
#include <test/setup_common.h>
#include <random.h>
#include <thread>

/** Test Suite for CuckooCache
 *
 *  1) All tests should have a deterministic result (using insecure rand
 *  with deterministic seeds)
 *  2) Some test methods are templated to allow for easier testing
 *  against new versions / comparing
 *  3) Results should be treated as a regression test, i.e., did the behavior
 *  change significantly from what was expected. This can be OK, depending on
 *  the nature of the change, but requires updating the tests to reflect the new
 *  expected behavior. For example improving the hit rate may cause some tests
 *  using BOOST_CHECK_CLOSE to fail.
 *
 */
BOOST_AUTO_TEST_SUITE(cuckoocache_tests);

/* Test that no values not inserted into the cache are read out of it.
 *
 * There are no repeats in the first 200000 insecure_GetRandHash calls
 */
BOOST_AUTO_TEST_CASE(test_cuckoocache_no_fakes)
{
    SeedInsecureRand(SeedRand::ZEROS);
    CuckooCache::cache<uint256, SignatureCacheHasher> cc{};
    size_t megabytes = 4;
    cc.setup_bytes(megabytes << 20);
    for (int x = 0; x < 100000; ++x) {
        cc.insert(InsecureRand256());
    }
    for (int x = 0; x < 100000; ++x) {
        BOOST_CHECK(!cc.contains(InsecureRand256(), false));
    }
};

/** This helper returns the hit rate when megabytes*load worth of entries are
 * inserted into a megabytes sized cache
 */
template <typename Cache>
static double test_cache(size_t megabytes, double load)
{
    SeedInsecureRand(SeedRand::ZEROS);
    std::vector<uint256> hashes;
    Cache set{};
    size_t bytes = megabytes * (1 << 20);
    set.setup_bytes(bytes);
    uint32_t n_insert = static_cast<uint32_t>(load * (bytes / sizeof(uint256)));
    hashes.resize(n_insert);
    for (uint32_t i = 0; i < n_insert; ++i) {
        uint32_t* ptr = (uint32_t*)hashes[i].begin();
        for (uint8_t j = 0; j < 8; ++j)
            *(ptr++) = InsecureRand32();
    }
    /** We make a copy of the hashes because future optimizations of the
     * cuckoocache may overwrite the inserted element, so the test is
     * "future proofed".
     */
    std::vector<uint256> hashes_insert_copy = hashes;
    /** Do the insert */
    for (const uint256& h : hashes_insert_copy)
        set.insert(h);
    /** Count the hits */
    uint32_t count = 0;
    for (const uint256& h : hashes)
        count += set.contains(h, false);
    double hit_rate = ((double)count) / ((double)n_insert);
    return hit_rate;
}

/** The normalized hit rate for a given load.
 *
 * The semantics are a little confusing, so please see the below
 * explanation.
 *
 * Examples:
 *
 * 1) at load 0.5, we expect a perfect hit rate, so we multiply by
 * 1.0
 * 2) at load 2.0, we expect to see half the entries, so a perfect hit rate
 * would be 0.5. Therefore, if we see a hit rate of 0.4, 0.4*2.0 = 0.8 is the
 * normalized hit rate.
 *
 * This is basically the right semantics, but has a bit of a glitch depending on
 * how you measure around load 1.0 as after load 1.0 your normalized hit rate
 * becomes effectively perfect, ignoring freshness.
 */
static double normalize_hit_rate(double hits, double load)
{
    return hits * std::max(load, 1.0);
}

/** Check the hit rate on loads ranging from 0.1 to 1.6 */
BOOST_AUTO_TEST_CASE(cuckoocache_hit_rate_ok)
{
    /** Arbitrarily selected Hit Rate threshold that happens to work for this test
     * as a lower bound on performance.
     */
    double HitRateThresh = 0.98;
    size_t megabytes = 4;
    for (double load = 0.1; load < 2; load *= 2) {
        double hits = test_cache<CuckooCache::cache<uint256, SignatureCacheHasher>>(megabytes, load);
        BOOST_CHECK(normalize_hit_rate(hits, load) > HitRateThresh);
    }
}


/** This helper checks that erased elements are preferentially inserted onto and
 * that the hit rate of "fresher" keys is reasonable*/
template <typename Cache>
static void test_cache_erase(size_t megabytes)
{
    double load = 1;
    SeedInsecureRand(SeedRand::ZEROS);
    std::vector<uint256> hashes;
    Cache set{};
    size_t bytes = megabytes * (1 << 20);
    set.setup_bytes(bytes);
    uint32_t n_insert = static_cast<uint32_t>(load * (bytes / sizeof(uint256)));
    hashes.resize(n_insert);
    for (uint32_t i = 0; i < n_insert; ++i) {
        uint32_t* ptr = (uint32_t*)hashes[i].begin();
        for (uint8_t j = 0; j < 8; ++j)
            *(ptr++) = InsecureRand32();
    }
    /** We make a copy of the hashes because future optimizations of the
     * cuckoocache may overwrite the inserted element, so the test is
     * "future proofed".
     */
    std::vector<uint256> hashes_insert_copy = hashes;

    /** Insert the first half */
    for (uint32_t i = 0; i < (n_insert / 2); ++i)
        set.insert(hashes_insert_copy[i]);
    /** Erase the first quarter */
    for (uint32_t i = 0; i < (n_insert / 4); ++i)
        BOOST_CHECK(set.contains(hashes[i], true));
    /** Insert the second half */
    for (uint32_t i = (n_insert / 2); i < n_insert; ++i)
        set.insert(hashes_insert_copy[i]);

    /** elements that we marked as erased but are still there */
    size_t count_erased_but_contained = 0;
    /** elements that we did not erase but are older */
    size_t count_stale = 0;
    /** elements that were most recently inserted */
    size_t count_fresh = 0;

    for (uint32_t i = 0; i < (n_insert / 4); ++i)
        count_erased_but_contained += set.contains(hashes[i], false);
    for (uint32_t i = (n_insert / 4); i < (n_insert / 2); ++i)
        count_stale += set.contains(hashes[i], false);
    for (uint32_t i = (n_insert / 2); i < n_insert; ++i)
        count_fresh += set.contains(hashes[i], false);

    double hit_rate_erased_but_contained = double(count_erased_but_contained) / (double(n_insert) / 4.0);
    double hit_rate_stale = double(count_stale) / (double(n_insert) / 4.0);
    double hit_rate_fresh = double(count_fresh) / (double(n_insert) / 2.0);

    // Check that our hit_rate_fresh is perfect
    BOOST_CHECK_EQUAL(hit_rate_fresh, 1.0);
    // Check that we have a more than 2x better hit rate on stale elements than
    // erased elements.
    BOOST_CHECK(hit_rate_stale > 2 * hit_rate_erased_but_contained);
}

BOOST_AUTO_TEST_CASE(cuckoocache_erase_ok)
{
    size_t megabytes = 4;
    test_cache_erase<CuckooCache::cache<uint256, SignatureCacheHasher>>(megabytes);
}

template <typename Cache>
static void test_cache_erase_parallel(size_t megabytes)
{
    double load = 1;
    SeedInsecureRand(SeedRand::ZEROS);
    std::vector<uint256> hashes;
    Cache set{};
    size_t bytes = megabytes * (1 << 20);
    set.setup_bytes(bytes);
    uint32_t n_insert = static_cast<uint32_t>(load * (bytes / sizeof(uint256)));
    hashes.resize(n_insert);
    for (uint32_t i = 0; i < n_insert; ++i) {
        uint32_t* ptr = (uint32_t*)hashes[i].begin();
        for (uint8_t j = 0; j < 8; ++j)
            *(ptr++) = InsecureRand32();
    }
    /** We make a copy of the hashes because future optimizations of the
     * cuckoocache may overwrite the inserted element, so the test is
     * "future proofed".
     */
    std::vector<uint256> hashes_insert_copy = hashes;
    boost::shared_mutex mtx;

    {
        /** Grab lock to make sure we release inserts */
        boost::unique_lock<boost::shared_mutex> l(mtx);
        /** Insert the first half */
        for (uint32_t i = 0; i < (n_insert / 2); ++i)
            set.insert(hashes_insert_copy[i]);
    }

    /** Spin up 3 threads to run contains with erase.
     */
    std::vector<std::thread> threads;
    /** Erase the first quarter */
    for (uint32_t x = 0; x < 3; ++x)
        /** Each thread is emplaced with x copy-by-value
        */
        threads.emplace_back([&, x] {
            boost::shared_lock<boost::shared_mutex> l(mtx);
            size_t ntodo = (n_insert/4)/3;
            size_t start = ntodo*x;
            size_t end = ntodo*(x+1);
            for (uint32_t i = start; i < end; ++i) {
                bool contains = set.contains(hashes[i], true);
                assert(contains);
            }
        });

    /** Wait for all threads to finish
     */
    for (std::thread& t : threads)
        t.join();
    /** Grab lock to make sure we observe erases */
    boost::unique_lock<boost::shared_mutex> l(mtx);
    /** Insert the second half */
    for (uint32_t i = (n_insert / 2); i < n_insert; ++i)
        set.insert(hashes_insert_copy[i]);

    /** elements that we marked erased but that are still there */
    size_t count_erased_but_contained = 0;
    /** elements that we did not erase but are older */
    size_t count_stale = 0;
    /** elements that were most recently inserted */
    size_t count_fresh = 0;

    for (uint32_t i = 0; i < (n_insert / 4); ++i)
        count_erased_but_contained += set.contains(hashes[i], false);
    for (uint32_t i = (n_insert / 4); i < (n_insert / 2); ++i)
        count_stale += set.contains(hashes[i], false);
    for (uint32_t i = (n_insert / 2); i < n_insert; ++i)
        count_fresh += set.contains(hashes[i], false);

    double hit_rate_erased_but_contained = double(count_erased_but_contained) / (double(n_insert) / 4.0);
    double hit_rate_stale = double(count_stale) / (double(n_insert) / 4.0);
    double hit_rate_fresh = double(count_fresh) / (double(n_insert) / 2.0);

    // Check that our hit_rate_fresh is perfect
    BOOST_CHECK_EQUAL(hit_rate_fresh, 1.0);
    // Check that we have a more than 2x better hit rate on stale elements than
    // erased elements.
    BOOST_CHECK(hit_rate_stale > 2 * hit_rate_erased_but_contained);
}
BOOST_AUTO_TEST_CASE(cuckoocache_erase_parallel_ok)
{
    size_t megabytes = 4;
    test_cache_erase_parallel<CuckooCache::cache<uint256, SignatureCacheHasher>>(megabytes);
}


template <typename Cache>
static void test_cache_generations()
{
    // This test checks that for a simulation of network activity, the fresh hit
    // rate is never below 99%, and the number of times that it is worse than
    // 99.9% are less than 1% of the time.
    double min_hit_rate = 0.99;
    double tight_hit_rate = 0.999;
    double max_rate_less_than_tight_hit_rate = 0.01;
    // A cache that meets this specification is therefore shown to have a hit
    // rate of at least tight_hit_rate * (1 - max_rate_less_than_tight_hit_rate) +
    // min_hit_rate*max_rate_less_than_tight_hit_rate = 0.999*99%+0.99*1% == 99.89%
    // hit rate with low variance.

    // We use deterministic values, but this test has also passed on many
    // iterations with non-deterministic values, so it isn't "overfit" to the
    // specific entropy in FastRandomContext(true) and implementation of the
    // cache.
    SeedInsecureRand(SeedRand::ZEROS);

    // block_activity models a chunk of network activity. n_insert elements are
    // added to the cache. The first and last n/4 are stored for removal later
    // and the middle n/2 are not stored. This models a network which uses half
    // the signatures of recently (since the last block) added transactions
    // immediately and never uses the other half.
    struct block_activity {
        std::vector<uint256> reads;
        block_activity(uint32_t n_insert, Cache& c) : reads()
        {
            std::vector<uint256> inserts;
            inserts.resize(n_insert);
            reads.reserve(n_insert / 2);
            for (uint32_t i = 0; i < n_insert; ++i) {
                uint32_t* ptr = (uint32_t*)inserts[i].begin();
                for (uint8_t j = 0; j < 8; ++j)
                    *(ptr++) = InsecureRand32();
            }
            for (uint32_t i = 0; i < n_insert / 4; ++i)
                reads.push_back(inserts[i]);
            for (uint32_t i = n_insert - (n_insert / 4); i < n_insert; ++i)
                reads.push_back(inserts[i]);
            for (const auto& h : inserts)
                c.insert(h);
        }
    };

    const uint32_t BLOCK_SIZE = 1000;
    // We expect window size 60 to perform reasonably given that each epoch
    // stores 45% of the cache size (~472k).
    const uint32_t WINDOW_SIZE = 60;
    const uint32_t POP_AMOUNT = (BLOCK_SIZE / WINDOW_SIZE) / 2;
    const double load = 10;
    const size_t megabytes = 4;
    const size_t bytes = megabytes * (1 << 20);
    const uint32_t n_insert = static_cast<uint32_t>(load * (bytes / sizeof(uint256)));

    std::vector<block_activity> hashes;
    Cache set{};
    set.setup_bytes(bytes);
    hashes.reserve(n_insert / BLOCK_SIZE);
    std::deque<block_activity> last_few;
    uint32_t out_of_tight_tolerance = 0;
    uint32_t total = n_insert / BLOCK_SIZE;
    // we use the deque last_few to model a sliding window of blocks. at each
    // step, each of the last WINDOW_SIZE block_activities checks the cache for
    // POP_AMOUNT of the hashes that they inserted, and marks these erased.
    for (uint32_t i = 0; i < total; ++i) {
        if (last_few.size() == WINDOW_SIZE)
            last_few.pop_front();
        last_few.emplace_back(BLOCK_SIZE, set);
        uint32_t count = 0;
        for (auto& act : last_few)
            for (uint32_t k = 0; k < POP_AMOUNT; ++k) {
                count += set.contains(act.reads.back(), true);
                act.reads.pop_back();
            }
        // We use last_few.size() rather than WINDOW_SIZE for the correct
        // behavior on the first WINDOW_SIZE iterations where the deque is not
        // full yet.
        double hit = (double(count)) / (last_few.size() * POP_AMOUNT);
        // Loose Check that hit rate is above min_hit_rate
        BOOST_CHECK(hit > min_hit_rate);
        // Tighter check, count number of times we are less than tight_hit_rate
        // (and implicitly, greater than min_hit_rate)
        out_of_tight_tolerance += hit < tight_hit_rate;
    }
    // Check that being out of tolerance happens less than
    // max_rate_less_than_tight_hit_rate of the time
    BOOST_CHECK(double(out_of_tight_tolerance) / double(total) < max_rate_less_than_tight_hit_rate);
}
BOOST_AUTO_TEST_CASE(cuckoocache_generations)
{
    test_cache_generations<CuckooCache::cache<uint256, SignatureCacheHasher>>();
}

BOOST_AUTO_TEST_SUITE_END();
