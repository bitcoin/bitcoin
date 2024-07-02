
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <evo/evodb.h>
#include <dbwrapper.h>

BOOST_AUTO_TEST_SUITE(EvoDBTestSuite)

BOOST_AUTO_TEST_CASE(TestWriteCache) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");

    std::string value;
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, "one");

    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, "two");

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, "three");

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
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");
    evoDB.WriteCache(4, "four"); // Should evict "one"

    std::string value;
    BOOST_CHECK(!evoDB.ReadCache(1, value));
    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, "two");

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, "three");

    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, "four");

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
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");

    evoDB.EraseCache(2);

    std::string value;
    BOOST_CHECK(!evoDB.ReadCache(2, value));
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, "one");

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, "three");

    // Check internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();
    auto eraseCache = evoDB.GetEraseCacheCopy();

    BOOST_CHECK_EQUAL(mapCache.size(), 2);
    BOOST_CHECK_EQUAL(fifoList.size(), 2);
    BOOST_CHECK_EQUAL(eraseCache.size(), 1);
    BOOST_CHECK(eraseCache.find(2) != eraseCache.end());
}

BOOST_AUTO_TEST_CASE(TestGetAllEntriesReverse) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");

    auto entries = evoDB.GetAllEntriesReverse();
    BOOST_CHECK_EQUAL(entries.size(), 3);
    BOOST_CHECK_EQUAL(entries[0].second, "three");
    BOOST_CHECK_EQUAL(entries[1].second, "two");
    BOOST_CHECK_EQUAL(entries[2].second, "one");

    evoDB.WriteCache(4, "four");

    entries = evoDB.GetAllEntriesReverse();
    BOOST_CHECK_EQUAL(entries.size(), 3);
    BOOST_CHECK_EQUAL(entries[0].second, "four");
    BOOST_CHECK_EQUAL(entries[1].second, "three");
    BOOST_CHECK_EQUAL(entries[2].second, "two");

    // Check internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);

    // Ensure internal structures match the output of GetAllEntriesReverse
    auto it = fifoList.rbegin();
    for (const auto& entry : entries) {
        BOOST_CHECK_EQUAL(entry.first, it->first);
        BOOST_CHECK_EQUAL(entry.second, it->second);
        ++it;
    }
}

BOOST_AUTO_TEST_CASE(TestFlushCacheToDisk) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");

    BOOST_CHECK(evoDB.FlushCacheToDisk());

    std::string value;
    BOOST_CHECK(evoDB.Read(1, value));
    BOOST_CHECK_EQUAL(value, "one");

    BOOST_CHECK(evoDB.Read(2, value));
    BOOST_CHECK_EQUAL(value, "two");

    BOOST_CHECK(evoDB.Read(3, value));
    BOOST_CHECK_EQUAL(value, "three");

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
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");
    evoDB.WriteCache(4, "four"); // Should evict "one"

    std::string value;
    BOOST_CHECK(!evoDB.ReadCache(1, value));
    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, "two");
    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, "three");
    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, "four");

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
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");

    // Directly write to the DB to create a discrepancy
    {
        CDBBatch batch(evoDB);
        batch.Write(4, "four");
        evoDB.WriteBatch(batch, true);
    }

    // Check if cache reads correctly
    std::string value;
    BOOST_CHECK(!evoDB.ReadCache(4, value));

    // Check if DB reads correctly
    BOOST_CHECK(evoDB.Read(4, value));
    BOOST_CHECK_EQUAL(value, "four");

    // Write to cache and ensure it doesn't read from DB
    evoDB.WriteCache(4, "four_cache");
    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, "four_cache");

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 4);
    BOOST_CHECK_EQUAL(fifoList.size(), 4);
}


BOOST_AUTO_TEST_CASE(TestGetAllEntriesReverseBetweenFlushes) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    evoDB.WriteCache(3, "three");
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    evoDB.WriteCache(4, "four");
    evoDB.WriteCache(5, "five");

    auto entries = evoDB.GetAllEntriesReverse();
    BOOST_CHECK_EQUAL(entries.size(), 5);
    BOOST_CHECK_EQUAL(entries[0].second, "five");
    BOOST_CHECK_EQUAL(entries[1].second, "four");
    BOOST_CHECK_EQUAL(entries[2].second, "three");
    BOOST_CHECK_EQUAL(entries[3].second, "two");
    BOOST_CHECK_EQUAL(entries[4].second, "one");

    // Verify cache has not reached max size and gets filled from disk
    std::string value;
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, "one");

    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, "two");

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, "three");

    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, "four");

    BOOST_CHECK(evoDB.ReadCache(5, value));
    BOOST_CHECK_EQUAL(value, "five");

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);

    // Ensure internal structures match the output of GetAllEntriesReverse
    auto it = fifoList.rbegin();
    for (const auto& entry : entries) {
        if (it == fifoList.rend()) break;
        BOOST_CHECK_EQUAL(entry.first, it->first);
        BOOST_CHECK_EQUAL(entry.second, it->second);
        ++it;
    }
}

BOOST_AUTO_TEST_CASE(TestCachePersistenceAcrossFlushes) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    evoDB.WriteCache(3, "three");
    evoDB.WriteCache(4, "four");
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    evoDB.WriteCache(5, "five");

    auto entries = evoDB.GetAllEntriesReverse();
    BOOST_CHECK_EQUAL(entries.size(), 5);
    BOOST_CHECK_EQUAL(entries[0].second, "five");
    BOOST_CHECK_EQUAL(entries[1].second, "four");
    BOOST_CHECK_EQUAL(entries[2].second, "three");
    BOOST_CHECK_EQUAL(entries[3].second, "two");
    BOOST_CHECK_EQUAL(entries[4].second, "one");

    // Verify cache persistence across flushes
    std::string value;
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, "one");

    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, "two");

    BOOST_CHECK(evoDB.ReadCache(3, value));
    BOOST_CHECK_EQUAL(value, "three");

    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, "four");

    BOOST_CHECK(evoDB.ReadCache(5, value));
    BOOST_CHECK_EQUAL(value, "five");

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);

    // Ensure internal structures match the output of GetAllEntriesReverse
    auto it = fifoList.rbegin();
    for (const auto& entry : entries) {
        if (it == fifoList.rend()) break;
        BOOST_CHECK_EQUAL(entry.first, it->first);
        BOOST_CHECK_EQUAL(entry.second, it->second);
        ++it;
    }
}

BOOST_AUTO_TEST_CASE(TestSimultaneousCacheAndDiskReads) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::string> evoDB(dbParams, 3);

    // Write some entries to cache and disk
    evoDB.WriteCache(1, "one");
    evoDB.WriteCache(2, "two");
    BOOST_CHECK(evoDB.FlushCacheToDisk());

    // Directly write to the DB to create a discrepancy
    {
        CDBBatch batch(evoDB);
        batch.Write(3, "three_db");
        evoDB.WriteBatch(batch, true);
    }

    // Write to cache
    evoDB.WriteCache(4, "four");

    auto entries = evoDB.GetAllEntriesReverse();
    BOOST_CHECK_EQUAL(entries.size(), 4);
    BOOST_CHECK_EQUAL(entries[0].second, "four");
    BOOST_CHECK_EQUAL(entries[1].second, "three_db");
    BOOST_CHECK_EQUAL(entries[2].second, "two");
    BOOST_CHECK_EQUAL(entries[3].second, "one");

    // Verify cache reads correctly
    std::string value;
    BOOST_CHECK(evoDB.ReadCache(1, value));
    BOOST_CHECK_EQUAL(value, "one");

    BOOST_CHECK(evoDB.ReadCache(2, value));
    BOOST_CHECK_EQUAL(value, "two");

    BOOST_CHECK(evoDB.ReadCache(3, value)); // Should read from DB
    BOOST_CHECK_EQUAL(value, "three_db");

    BOOST_CHECK(evoDB.ReadCache(4, value));
    BOOST_CHECK_EQUAL(value, "four");

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 3);
    BOOST_CHECK_EQUAL(fifoList.size(), 3);

    // Ensure internal structures match the output of GetAllEntriesReverse
    auto it = fifoList.rbegin();
    for (const auto& entry : entries) {
        if (it == fifoList.rend()) break;
        BOOST_CHECK_EQUAL(entry.first, it->first);
        BOOST_CHECK_EQUAL(entry.second, it->second);
        ++it;
    }
}
/*
BOOST_AUTO_TEST_CASE(TestStressTest) {
    auto dbParams = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true};
    CEvoDB<int, std::string> evoDB(dbParams, 1000); // Large cache size for stress test

    for (int i = 0; i < 1000; ++i) {
        evoDB.WriteCache(i, "value" + std::to_string(i));
    }

    BOOST_CHECK(evoDB.FlushCacheToDisk());

    for (int i = 1000; i < 2000; ++i) {
        evoDB.WriteCache(i, "value" + std::to_string(i));
    }

    auto entries = evoDB.GetAllEntriesReverse();
    BOOST_CHECK_EQUAL(entries.size(), 2000);

    for (int i = 0; i < 2000; ++i) {
        BOOST_CHECK_EQUAL(entries[1999 - i].second, "value" + std::to_string(i));
    }

    // Verify internal structures
    auto mapCache = evoDB.GetMapCache();
    auto fifoList = evoDB.GetFifoList();

    BOOST_CHECK_EQUAL(mapCache.size(), 1000);
    BOOST_CHECK_EQUAL(fifoList.size(), 1000);

    // Ensure internal structures match the output of GetAllEntriesReverse
    auto it = fifoList.rbegin();
    for (int i = 0; i < 1000; ++i) {
        BOOST_CHECK_EQUAL(entries[1999 - i].first, it->first);
        BOOST_CHECK_EQUAL(entries[1999 - i].second, it->second);
        ++it;
    }
}*/
BOOST_AUTO_TEST_SUITE_END()

