
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <evo/evodb.h>
#include <dbwrapper.h>

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

    evoDB.EraseCache(2);

    int value;
    BOOST_CHECK(!evoDB.ReadCache(2, value));
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, one);

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, three);

    // Check internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();
    auto eraseCache = evoDB.GetEraseCacheCopy();

    BOOST_CHECK_EQUAL(mapCache.size(), 2);
    BOOST_CHECK_EQUAL(fifoList.size(), 2);
    BOOST_CHECK_EQUAL(eraseCache.size(), 1);
    BOOST_CHECK(eraseCache.find(2) != eraseCache.end());
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

BOOST_AUTO_TEST_SUITE_END()

