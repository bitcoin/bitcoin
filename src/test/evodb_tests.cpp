
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <evo/evodb.h>
#include <dbwrapper.h>
#include <saltedhasher.h>

BOOST_AUTO_TEST_SUITE(evodb_tests)
int one = 100;
int two = 200;
int three = 300;
int four = 400;
int five = 500;
BOOST_AUTO_TEST_CASE(TestWriteCache) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);

    int value;
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, one);

    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, two);

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);

    // Check internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);
}

BOOST_AUTO_TEST_CASE(TestCacheEviction) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);
    evoDB.WriteCache(4, four); // Should evict one

    int value;
    BOOST_CHECK(!evoDB.ReadCache(1, value));
    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, two);

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);

    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, four);

    // Check internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);
}

BOOST_AUTO_TEST_CASE(TestEraseCache) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();
    auto eraseCache = evoDB.GetEraseCacheCopy();
    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);
    BOOST_CHECK_EQUAL(eraseCache.size(), 0);

    evoDB.EraseCache(2);
    // try erasing again
    evoDB.EraseCache(2);
    mapCache = evoDB.GetMapCache();
    fifoList = evoDB.GetFifoList();
    eraseCache = evoDB.GetEraseCacheCopy();
    BOOST_CHECK_EQUAL(mapCache.size(), 2);
    BOOST_CHECK_EQUAL(fifoList.size(), 2);
    BOOST_CHECK_EQUAL(eraseCache.size(), 1);
    BOOST_CHECK(eraseCache.find(2) != eraseCache.end());

    int value;
    // should flush to disk
    BOOST_CHECK(!evoDB.ReadCache(2, value));
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, one);

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);

    // Check internal structures that are empty after read flush (after erase)
    mapCache = evoDB.GetMapCache();
    fifoList = evoDB.GetFifoList();
    eraseCache = evoDB.GetEraseCacheCopy();

    BOOST_CHECK_EQUAL(mapCache.size(), 0);
    BOOST_CHECK_EQUAL(fifoList.size(), 0);
    BOOST_CHECK_EQUAL(eraseCache.size(), 0);
    BOOST_CHECK(eraseCache.find(2) == eraseCache.end());
}

BOOST_AUTO_TEST_CASE(TestFlushCacheToDisk) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);

    BOOST_CHECK(evoDB.FlushCacheToDisk());

    int value;
    BOOST_CHECK(evoDB.Read(1, value));
    BOOST_CHECK_EQUAL(value, one);

    BOOST_CHECK(evoDB.Read(2, value));
    BOOST_CHECK_EQUAL(value, two);

    BOOST_CHECK(evoDB.Read(3, value));
    BOOST_CHECK_EQUAL(value, three);

    // Check internal structures after flush
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();
    auto eraseCache = evoDB.GetEraseCacheCopy();

    BOOST_CHECK_EQUAL(mapCache.size(), 0);
    BOOST_CHECK_EQUAL(fifoList.size(), 0);
    BOOST_CHECK_EQUAL(eraseCache.size(), 0);
}

BOOST_AUTO_TEST_CASE(TestMaxCacheSize) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);
    evoDB.WriteCache(4, four); // Should evict one

    int value;
    BOOST_CHECK(!evoDB.ReadCache(1, value));
    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, two);
    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);
    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, four);

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);
}


BOOST_AUTO_TEST_CASE(TestCacheVsDBDiscrepancy) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::vector<int>> evoDB(dbParams, 3);

    std::vector<int> value1 = {1, 1, 1};
    std::vector<int> value2 = {2, 2, 2};
    std::vector<int> value3 = {3, 3, 3};
    std::vector<int> value4_db = {4, 4, 4};
    std::vector<int> value4_cache = {5, 5, 5};

    evoDB.WriteCache(1, value1);
    evoDB.WriteCache(2, value2);
    evoDB.WriteCache(3, value3);

    // Directly write to the DB to create a discrepancy
    BOOST_CHECK(evoDB.Write(4, value4_db, true));

    // Check if cache reads correctly (should get from DB)
    std::vector<int> value;
    BOOST_CHECK(evoDB.ReadCache(4, value));

    // Check if DB reads directly
    BOOST_CHECK(evoDB.Read(4, value));
    BOOST_CHECK_EQUAL_COLLECTIONS(value.begin(), value.end(), value4_db.begin(), value4_db.end());

    // Write to cache and ensure DB still has old value
    evoDB.WriteCache(4, value4_cache);
    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL_COLLECTIONS(value.begin(), value.end(), value4_cache.begin(), value4_cache.end());

    BOOST_CHECK(evoDB.Read(4, value));
    BOOST_CHECK_EQUAL_COLLECTIONS(value.begin(), value.end(), value4_db.begin(), value4_db.end());

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);

    evoDB.FlushCacheToDisk();
    // after flushing DB should have new value
    BOOST_CHECK(evoDB.Read(4, value));
    BOOST_CHECK_EQUAL_COLLECTIONS(value.begin(), value.end(), value4_cache.begin(), value4_cache.end());
    // cache should also
    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL_COLLECTIONS(value.begin(), value.end(), value4_cache.begin(), value4_cache.end());

    // Verify internal structures
    mapCache = evoDB.GetMapCache();
    fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 0);
    BOOST_CHECK_EQUAL(fifoList.size(), 0);
    
    mapCache = evoDB.GetMapCache();
    fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 0);
    BOOST_CHECK_EQUAL(fifoList.size(), 0);

}

BOOST_AUTO_TEST_CASE(TestCachePersistenceAcrossFlushes) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    evoDB.WriteCache(3, three);
    evoDB.WriteCache(4, four);
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    evoDB.WriteCache(5, five);

    // Verify cache persistence across flushes
    int value;
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, one);

    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, two);

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);

    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, four);

    BOOST_CHECK(evoDB.ReadCache(5, value));
    BOOST_CHECK_EQUAL(value, five);

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 1);
    BOOST_CHECK_EQUAL(fifoList.size(), 1);
}

BOOST_AUTO_TEST_CASE(TestSimultaneousCacheAndDiskReads) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    // Write some entries to cache and disk
    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    // Directly write to the DB to create a discrepancy
    {
        CDBBatch batch(evoDB);
        batch.Write(3, three);
        evoDB.WriteBatch(batch, true);
    }

    // Write to cache
    evoDB.WriteCache(4, four);

    // Verify cache reads correctly
    int value;
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, one);

    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, two);

    BOOST_CHECK(evoDB.ReadCache(3, value)); // Should read from DB
    BOOST_CHECK_EQUAL(value, three);

    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, four);

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 1);
    BOOST_CHECK_EQUAL(fifoList.size(), 1);

}


BOOST_AUTO_TEST_CASE(TestStressTest) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 1000); // Large cache size for stress test

    // Write the first batch of entries to the cache
    for (int i = 0; i < 1000; ++i) {
        evoDB.WriteCache(i, i);
    }

    // Verify internal structures and contents before flush
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 1000);
    BOOST_CHECK_EQUAL(fifoList.size(), 1000);

    // Check the contents of the cache before flush
    for (int i = 0; i < 1000; ++i) {
        int value;
        BOOST_CHECK(evoDB.ReadCache(i, value));
        BOOST_CHECK_EQUAL(value, i);
        BOOST_CHECK_EQUAL(mapCache[i]->second, i);
    }

    // Verify FIFO list contents before flush
    int index = 0;
    for (const auto& entry : fifoList) {
        BOOST_CHECK_EQUAL(entry.first, index);
        BOOST_CHECK_EQUAL(entry.second, index);
        index++;
    }

    // Flush the cache to disk
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    // Ensure cache is cleared after flush
    mapCache = evoDB.GetMapCache();
    fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 0);
    BOOST_CHECK_EQUAL(fifoList.size(), 0);

    // Write the second batch of entries to the cache
    for (int i = 1000; i < 2000; ++i) {
        evoDB.WriteCache(i, i);
    }

    // Verify internal structures and contents after second batch
    mapCache = evoDB.GetMapCache();
    fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 1000);
    BOOST_CHECK_EQUAL(fifoList.size(), 1000);

    // Check the contents of the cache after flush
    for (int i = 1000; i < 2000; ++i) {
        int value;
        BOOST_CHECK(evoDB.ReadCache(i, value));
        BOOST_CHECK_EQUAL(value, i);
        BOOST_CHECK_EQUAL(mapCache[i]->second, i);
    }

    // Verify FIFO list contents after second batch
    index = 1000;
    for (const auto& entry : fifoList) {
        BOOST_CHECK_EQUAL(entry.first, index);
        BOOST_CHECK_EQUAL(entry.second, index);
        index++;
    }

    // Ensure that entries from the first batch are in the database but not in the cache
    for (int i = 0; i < 1000; ++i) {
        int value;
        BOOST_CHECK(evoDB.ReadCache(i, value)); // Should not be in cache but in DB
        BOOST_CHECK(evoDB.Read(i, value)); // Should be in DB
        BOOST_CHECK_EQUAL(value, i);
    }

    // Flush the cache to disk again
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    // Ensure all entries are now in the database
    for (int i = 0; i < 2000; ++i) {
        int value;
        BOOST_CHECK(evoDB.Read(i, value));
        BOOST_CHECK_EQUAL(value, i);
    }
}
BOOST_AUTO_TEST_CASE(TestUint256KeyUniqueness)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true
    };

    CEvoDB<uint256, int, StaticSaltedHasher> evoDB(dbParams, 10);

    uint256 key1 = uint256S("0x01");
    uint256 key2 = uint256S("0x02");
    uint256 key3 = uint256S("0x03");

    evoDB.WriteCache(key1, 100);
    evoDB.WriteCache(key2, 200);
    evoDB.WriteCache(key3, 300);

    int value;

    BOOST_CHECK(evoDB.ReadCache(key1, value));
    BOOST_CHECK_EQUAL(value, 100);

    BOOST_CHECK(evoDB.ReadCache(key2, value));
    BOOST_CHECK_EQUAL(value, 200);

    BOOST_CHECK(evoDB.ReadCache(key3, value));
    BOOST_CHECK_EQUAL(value, 300);

    // Check internal structures explicitly
    auto mapCache = evoDB.GetMapCache();
    BOOST_CHECK_EQUAL(mapCache.size(), 3);

    // Confirm that keys are treated as unique and not overwritten
    BOOST_CHECK(mapCache.find(key1) != mapCache.end());
    BOOST_CHECK(mapCache.find(key2) != mapCache.end());
    BOOST_CHECK(mapCache.find(key3) != mapCache.end());
}

BOOST_AUTO_TEST_CASE(TestForEachCachedEntryEnumeratesCurrentCacheValues)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(2, four);
    evoDB.WriteCache(3, three);

    std::unordered_map<int, int> visited;
    evoDB.ForEachCachedEntry([&](int key, int value) {
        visited[key] = value;
    });

    BOOST_CHECK_EQUAL(visited.size(), 3U);
    BOOST_CHECK_EQUAL(visited[1], one);
    BOOST_CHECK_EQUAL(visited[2], four);
    BOOST_CHECK_EQUAL(visited[3], three);
}

BOOST_AUTO_TEST_CASE(TestForEachCachedEntryFlushesPendingEraseBeforeIteration)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    BOOST_REQUIRE(evoDB.FlushCacheToDisk());

    evoDB.WriteCache(3, three);
    evoDB.EraseCache(1);

    std::unordered_map<int, int> visited;
    evoDB.ForEachCachedEntry([&](int key, int value) {
        visited[key] = value;
    });

    BOOST_CHECK(visited.empty());
    BOOST_CHECK_EQUAL(evoDB.GetMapCache().size(), 0U);
    BOOST_CHECK_EQUAL(evoDB.GetEraseCacheCopy().size(), 0U);

    int value;
    BOOST_CHECK(!evoDB.Read(1, value));
    BOOST_CHECK(evoDB.Read(2, value));
    BOOST_CHECK_EQUAL(value, two);
    BOOST_CHECK(evoDB.Read(3, value));
    BOOST_CHECK_EQUAL(value, three);
}

BOOST_AUTO_TEST_CASE(TestForEachEraseEntryEnumeratesPendingEraseKeys)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 3);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);
    evoDB.EraseCache(1);
    evoDB.EraseCache(3);

    std::unordered_set<int> visited;
    evoDB.ForEachEraseEntry([&](int key) {
        visited.insert(key);
    });

    BOOST_CHECK_EQUAL(visited.size(), 2U);
    BOOST_CHECK(visited.count(1) == 1);
    BOOST_CHECK(visited.count(3) == 1);
    BOOST_CHECK(visited.count(2) == 0);
}

BOOST_AUTO_TEST_CASE(TestReadCacheDoesNotResurrectPendingEraseBeforeReadLock)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 10);

    evoDB.WriteCache(1, one);
    BOOST_REQUIRE(evoDB.FlushCacheToDisk());

    bool injected{false};
    evoDB.SetBeforeReadLockHookForTesting([&]() {
        if (injected) return;
        injected = true;
        evoDB.EraseCache(1);
    });

    int value;
    BOOST_CHECK(!evoDB.ReadCache(1, value));
    BOOST_CHECK(injected);
    BOOST_CHECK_EQUAL(evoDB.GetMapCache().size(), 0U);
    BOOST_CHECK_EQUAL(evoDB.GetEraseCacheCopy().size(), 0U);
    BOOST_CHECK(!evoDB.Read(1, value));

    evoDB.SetBeforeReadLockHookForTesting({});
}

BOOST_AUTO_TEST_CASE(TestExistsCacheDoesNotResurrectPendingEraseBeforeReadLock)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 10);

    evoDB.WriteCache(1, one);
    BOOST_REQUIRE(evoDB.FlushCacheToDisk());

    bool injected{false};
    evoDB.SetBeforeReadLockHookForTesting([&]() {
        if (injected) return;
        injected = true;
        evoDB.EraseCache(1);
    });

    BOOST_CHECK(!evoDB.ExistsCache(1));
    BOOST_CHECK(injected);
    BOOST_CHECK_EQUAL(evoDB.GetMapCache().size(), 0U);
    BOOST_CHECK_EQUAL(evoDB.GetEraseCacheCopy().size(), 0U);

    int value;
    BOOST_CHECK(!evoDB.Read(1, value));

    evoDB.SetBeforeReadLockHookForTesting({});
}

BOOST_AUTO_TEST_CASE(TestConcurrentReadCacheWaitsForEraseTriggeredFlush)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 10);

    evoDB.WriteCache(1, one);
    BOOST_REQUIRE(evoDB.FlushCacheToDisk());
    evoDB.EraseCache(1);

    std::promise<void> flush_started;
    std::shared_future<void> allow_flush = std::async(std::launch::deferred, [] {}).share();
    std::promise<void> release_flush;
    allow_flush = release_flush.get_future().share();
    bool started{false};

    evoDB.SetWriteBatchHookForTesting([&](CDBBatch& batch) {
        if (!started) {
            started = true;
            flush_started.set_value();
        }
        allow_flush.wait();
        return evoDB.WriteBatch(batch, /*fSync=*/true);
    });

    auto first_reader = std::async(std::launch::async, [&]() {
        int value;
        return evoDB.ReadCache(1, value);
    });

    flush_started.get_future().wait();

    auto second_reader = std::async(std::launch::async, [&]() {
        int value;
        return evoDB.ReadCache(1, value);
    });

    BOOST_CHECK(second_reader.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout);

    release_flush.set_value();
    BOOST_CHECK(!first_reader.get());
    BOOST_CHECK(!second_reader.get());

    int value;
    BOOST_CHECK(!evoDB.Read(1, value));
    evoDB.SetWriteBatchHookForTesting({});
}

BOOST_AUTO_TEST_CASE(TestFlushCacheToDiskRestoresUncommittedWritesAfterPartialFailure)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 10);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);

    int batch_calls{0};
    evoDB.SetWriteBatchHookForTesting([&](CDBBatch& batch) {
        ++batch_calls;
        if (batch_calls == 1) {
            return evoDB.WriteBatch(batch, /*fSync=*/true);
        }
        return false;
    });

    BOOST_CHECK(!evoDB.FlushCacheToDisk(/*CHUNK_ITEMS=*/2));
    BOOST_CHECK_EQUAL(batch_calls, 2);

    int value;
    BOOST_CHECK(evoDB.Read(1, value));
    BOOST_CHECK_EQUAL(value, one);
    BOOST_CHECK(evoDB.Read(2, value));
    BOOST_CHECK_EQUAL(value, two);
    BOOST_CHECK(!evoDB.Read(3, value));

    BOOST_CHECK_EQUAL(evoDB.GetMapCache().size(), 1U);
    BOOST_CHECK_EQUAL(evoDB.GetFifoList().size(), 1U);
    BOOST_CHECK_EQUAL(evoDB.GetEraseCacheCopy().size(), 0U);
    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);

    evoDB.SetWriteBatchHookForTesting({});
    BOOST_CHECK(evoDB.FlushCacheToDisk(/*CHUNK_ITEMS=*/2));
    BOOST_CHECK(evoDB.Read(3, value));
    BOOST_CHECK_EQUAL(value, three);
}

BOOST_AUTO_TEST_CASE(TestFlushCacheToDiskPreservesFailingWriteChunkBoundary)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 10);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);
    evoDB.WriteCache(4, four);

    int batch_calls{0};
    evoDB.SetWriteBatchHookForTesting([&](CDBBatch& batch) {
        ++batch_calls;
        if (batch_calls == 1) {
            return evoDB.WriteBatch(batch, /*fSync=*/true);
        }
        return false;
    });

    BOOST_CHECK(!evoDB.FlushCacheToDisk(/*CHUNK_ITEMS=*/2));
    BOOST_CHECK_EQUAL(batch_calls, 2);

    int value;
    BOOST_CHECK(evoDB.Read(1, value));
    BOOST_CHECK_EQUAL(value, one);
    BOOST_CHECK(evoDB.Read(2, value));
    BOOST_CHECK_EQUAL(value, two);
    BOOST_CHECK(!evoDB.Read(3, value));
    BOOST_CHECK(!evoDB.Read(4, value));

    BOOST_CHECK_EQUAL(evoDB.GetMapCache().size(), 2U);
    BOOST_CHECK_EQUAL(evoDB.GetFifoList().size(), 2U);
    BOOST_CHECK_EQUAL(evoDB.GetEraseCacheCopy().size(), 0U);
    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);
    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, four);
}

BOOST_AUTO_TEST_CASE(TestFlushCacheToDiskRestoresUncommittedErasesAfterPartialFailure)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 10);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);
    BOOST_REQUIRE(evoDB.FlushCacheToDisk());

    evoDB.EraseCache(1);
    evoDB.EraseCache(2);
    evoDB.EraseCache(3);

    int batch_calls{0};
    evoDB.SetWriteBatchHookForTesting([&](CDBBatch& batch) {
        ++batch_calls;
        if (batch_calls == 1) {
            return evoDB.WriteBatch(batch, /*fSync=*/true);
        }
        return false;
    });

    BOOST_CHECK(!evoDB.FlushCacheToDisk(/*CHUNK_ITEMS=*/2));
    BOOST_CHECK_EQUAL(batch_calls, 2);

    auto eraseCache = evoDB.GetEraseCacheCopy();
    BOOST_CHECK_EQUAL(eraseCache.size(), 1U);

    int value;
    int remaining_on_disk{0};
    for (const auto key : {1, 2, 3}) {
        if (eraseCache.count(key) == 1) {
            BOOST_CHECK(evoDB.Read(key, value));
            remaining_on_disk = key;
        } else {
            BOOST_CHECK(!evoDB.Read(key, value));
        }
    }
    BOOST_CHECK(remaining_on_disk != 0);

    evoDB.SetWriteBatchHookForTesting({});
    BOOST_CHECK(evoDB.FlushCacheToDisk(/*CHUNK_ITEMS=*/2));
    BOOST_CHECK(!evoDB.Read(remaining_on_disk, value));
}

BOOST_AUTO_TEST_CASE(TestFlushCacheToDiskPreservesFailingEraseChunkBoundary)
{
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, int> evoDB(dbParams, 10);

    evoDB.WriteCache(1, one);
    evoDB.WriteCache(2, two);
    evoDB.WriteCache(3, three);
    evoDB.WriteCache(4, four);
    BOOST_REQUIRE(evoDB.FlushCacheToDisk());

    evoDB.EraseCache(1);
    evoDB.EraseCache(2);
    evoDB.EraseCache(3);
    evoDB.EraseCache(4);

    int batch_calls{0};
    evoDB.SetWriteBatchHookForTesting([&](CDBBatch& batch) {
        ++batch_calls;
        if (batch_calls == 1) {
            return evoDB.WriteBatch(batch, /*fSync=*/true);
        }
        return false;
    });

    BOOST_CHECK(!evoDB.FlushCacheToDisk(/*CHUNK_ITEMS=*/2));
    BOOST_CHECK_EQUAL(batch_calls, 2);

    auto eraseCache = evoDB.GetEraseCacheCopy();
    BOOST_CHECK_EQUAL(eraseCache.size(), 2U);

    int value;
    int keys_on_disk{0};
    for (const auto key : {1, 2, 3, 4}) {
        if (eraseCache.count(key) == 1) {
            BOOST_CHECK(evoDB.Read(key, value));
            ++keys_on_disk;
        } else {
            BOOST_CHECK(!evoDB.Read(key, value));
        }
    }
    BOOST_CHECK_EQUAL(keys_on_disk, 2);
}
BOOST_AUTO_TEST_SUITE_END()

