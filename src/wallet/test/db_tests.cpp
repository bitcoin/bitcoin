// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/translation.h>
#include <wallet/sqlite.h>
#include <wallet/migrate.h>
#include <wallet/test/util.h>
#include <wallet/walletutil.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

inline std::ostream& operator<<(std::ostream& os, const std::pair<const SerializeData, SerializeData>& kv)
{
    std::span key{kv.first}, value{kv.second};
    os << "(\"" << std::string_view{reinterpret_cast<const char*>(key.data()), key.size()} << "\", \""
       << std::string_view{reinterpret_cast<const char*>(value.data()), value.size()} << "\")";
    return os;
}

namespace wallet {

inline std::span<const std::byte> StringBytes(std::string_view str)
{
    return std::as_bytes(std::span{str});
}

static SerializeData StringData(std::string_view str)
{
    auto bytes = StringBytes(str);
    return SerializeData{bytes.begin(), bytes.end()};
}

static void CheckPrefix(DatabaseBatch& batch, std::span<const std::byte> prefix, MockableData expected)
{
    std::unique_ptr<DatabaseCursor> cursor = batch.GetNewPrefixCursor(prefix);
    MockableData actual;
    while (true) {
        DataStream key, value;
        DatabaseCursor::Status status = cursor->Next(key, value);
        if (status == DatabaseCursor::Status::DONE) break;
        BOOST_CHECK(status == DatabaseCursor::Status::MORE);
        BOOST_CHECK(
            actual.emplace(SerializeData(key.begin(), key.end()), SerializeData(value.begin(), value.end())).second);
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_SUITE(db_tests, BasicTestingSetup)

static std::vector<std::unique_ptr<WalletDatabase>> TestDatabases(const fs::path& path_root)
{
    std::vector<std::unique_ptr<WalletDatabase>> dbs;
    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    // Unable to test BerkeleyRO since we cannot create a new BDB database to open
    dbs.emplace_back(MakeSQLiteDatabase(path_root / "sqlite", options, status, error));
    dbs.emplace_back(CreateMockableWalletDatabase());
    return dbs;
}

BOOST_AUTO_TEST_CASE(db_cursor_prefix_range_test)
{
    // Test each supported db
    for (const auto& database : TestDatabases(m_path_root)) {
        std::vector<std::string> prefixes = {"", "FIRST", "SECOND", "P\xfe\xff", "P\xff\x01", "\xff\xff"};

        std::unique_ptr<DatabaseBatch> handler = Assert(database)->MakeBatch();
        // Write elements to it
        for (unsigned int i = 0; i < 10; i++) {
            for (const auto& prefix : prefixes) {
                BOOST_CHECK(handler->Write(std::make_pair(prefix, i), i));
            }
        }

        // Now read all the items by prefix and verify that each element gets parsed correctly
        for (const auto& prefix : prefixes) {
            DataStream s_prefix;
            s_prefix << prefix;
            std::unique_ptr<DatabaseCursor> cursor = handler->GetNewPrefixCursor(s_prefix);
            DataStream key;
            DataStream value;
            for (int i = 0; i < 10; i++) {
                DatabaseCursor::Status status = cursor->Next(key, value);
                BOOST_CHECK_EQUAL(status, DatabaseCursor::Status::MORE);

                std::string key_back;
                unsigned int i_back;
                key >> key_back >> i_back;
                BOOST_CHECK_EQUAL(key_back, prefix);

                unsigned int value_back;
                value >> value_back;
                BOOST_CHECK_EQUAL(value_back, i_back);
            }

            // Let's now read it once more, it should return DONE
            BOOST_CHECK(cursor->Next(key, value) == DatabaseCursor::Status::DONE);
        }
        handler.reset();
        database->Close();
    }
}

// Lower level DatabaseBase::GetNewPrefixCursor test, to cover cases that aren't
// covered in the higher level test above. The higher level test uses
// serialized strings which are prefixed with string length, so it doesn't test
// truly empty prefixes or prefixes that begin with \xff
BOOST_AUTO_TEST_CASE(db_cursor_prefix_byte_test)
{
    const MockableData::value_type
        e{StringData(""), StringData("e")},
        p{StringData("prefix"), StringData("p")},
        ps{StringData("prefixsuffix"), StringData("ps")},
        f{StringData("\xff"), StringData("f")},
        fs{StringData("\xffsuffix"), StringData("fs")},
        ff{StringData("\xff\xff"), StringData("ff")},
        ffs{StringData("\xff\xffsuffix"), StringData("ffs")};
    for (const auto& database : TestDatabases(m_path_root)) {
        std::unique_ptr<DatabaseBatch> batch = database->MakeBatch();

        // Write elements to it if not berkeleyro
        for (const auto& [k, v] : {e, p, ps, f, fs, ff, ffs}) {
            batch->Write(std::span{k}, std::span{v});
        }

        CheckPrefix(*batch, StringBytes(""), {e, p, ps, f, fs, ff, ffs});
        CheckPrefix(*batch, StringBytes("prefix"), {p, ps});
        CheckPrefix(*batch, StringBytes("\xff"), {f, fs, ff, ffs});
        CheckPrefix(*batch, StringBytes("\xff\xff"), {ff, ffs});
        batch.reset();
        database->Close();
    }
}

BOOST_AUTO_TEST_CASE(db_availability_after_write_error)
{
    // Ensures the database remains accessible without deadlocking after a write error.
    // To simulate the behavior, record overwrites are disallowed, and the test verifies
    // that the database remains active after failing to store an existing record.
    for (const auto& database : TestDatabases(m_path_root)) {
        // Write original record
        std::unique_ptr<DatabaseBatch> batch = database->MakeBatch();
        std::string key = "key";
        std::string value = "value";
        std::string value2 = "value_2";
        BOOST_CHECK(batch->Write(key, value));
        // Attempt to overwrite the record (expect failure)
        BOOST_CHECK(!batch->Write(key, value2, /*fOverwrite=*/false));
        // Successfully overwrite the record
        BOOST_CHECK(batch->Write(key, value2, /*fOverwrite=*/true));
        // Sanity-check; read and verify the overwritten value
        std::string read_value;
        BOOST_CHECK(batch->Read(key, read_value));
        BOOST_CHECK_EQUAL(read_value, value2);
    }
}

// Verify 'ErasePrefix' functionality using db keys similar to the ones used by the wallet.
// Keys are in the form of std::pair<TYPE, ENTRY_ID>
BOOST_AUTO_TEST_CASE(erase_prefix)
{
    const std::string key = "key";
    const std::string key2 = "key2";
    const std::string value = "value";
    const std::string value2 = "value_2";
    auto make_key = [](std::string type, std::string id) { return std::make_pair(type, id); };

    for (const auto& database : TestDatabases(m_path_root)) {
        if (dynamic_cast<BerkeleyRODatabase*>(database.get())) {
            // Skip this test if BerkeleyRO
            continue;
        }
        std::unique_ptr<DatabaseBatch> batch = database->MakeBatch();

        // Write two entries with the same key type prefix, a third one with a different prefix
        // and a fourth one with the type-id values inverted
        BOOST_CHECK(batch->Write(make_key(key, value), value));
        BOOST_CHECK(batch->Write(make_key(key, value2), value2));
        BOOST_CHECK(batch->Write(make_key(key2, value), value));
        BOOST_CHECK(batch->Write(make_key(value, key), value));

        // Erase the ones with the same prefix and verify result
        BOOST_CHECK(batch->TxnBegin());
        BOOST_CHECK(batch->ErasePrefix(DataStream() << key));
        BOOST_CHECK(batch->TxnCommit());

        BOOST_CHECK(!batch->Exists(make_key(key, value)));
        BOOST_CHECK(!batch->Exists(make_key(key, value2)));
        // Also verify that entries with a different prefix were not erased
        BOOST_CHECK(batch->Exists(make_key(key2, value)));
        BOOST_CHECK(batch->Exists(make_key(value, key)));
    }
}

// Test-only statement execution error
constexpr int TEST_SQLITE_ERROR = -999;

class DbExecBlocker : public SQliteExecHandler
{
private:
    SQliteExecHandler m_base_exec;
    std::set<std::string> m_blocked_statements;
public:
    DbExecBlocker(std::set<std::string> blocked_statements) : m_blocked_statements(blocked_statements) {}
    int Exec(SQLiteDatabase& database, const std::string& statement) override {
        if (m_blocked_statements.contains(statement)) return TEST_SQLITE_ERROR;
        return m_base_exec.Exec(database, statement);
    }
};

BOOST_AUTO_TEST_CASE(txn_close_failure_dangling_txn)
{
    // Verifies that there is no active dangling, to-be-reversed db txn
    // after the batch object that initiated it is destroyed.
    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    std::unique_ptr<SQLiteDatabase> database = MakeSQLiteDatabase(m_path_root / "sqlite", options, status, error);

    std::string key = "key";
    std::string value = "value";

    std::unique_ptr<SQLiteBatch> batch = std::make_unique<SQLiteBatch>(*database);
    BOOST_CHECK(batch->TxnBegin());
    BOOST_CHECK(batch->Write(key, value));
    // Set a handler to prevent txn abortion during destruction.
    // Mimicking a db statement execution failure.
    batch->SetExecHandler(std::make_unique<DbExecBlocker>(std::set<std::string>{"ROLLBACK TRANSACTION"}));
    // Destroy batch
    batch.reset();

    // Ensure there is no dangling, to-be-reversed db txn
    BOOST_CHECK(!database->HasActiveTxn());

    // And, just as a sanity check; verify that new batchs only write what they suppose to write
    // and nothing else.
    std::string key2 = "key2";
    std::unique_ptr<SQLiteBatch> batch2 = std::make_unique<SQLiteBatch>(*database);
    BOOST_CHECK(batch2->Write(key2, value));
    // The first key must not exist
    BOOST_CHECK(!batch2->Exists(key));
}

BOOST_AUTO_TEST_CASE(concurrent_txn_dont_interfere)
{
    std::string key = "key";
    std::string value = "value";
    std::string value2 = "value_2";

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    const auto& database = MakeSQLiteDatabase(m_path_root / "sqlite", options, status, error);

    std::unique_ptr<DatabaseBatch> handler = Assert(database)->MakeBatch();

    // Verify concurrent db transactions does not interfere between each other.
    // Start db txn, write key and check the key does exist within the db txn.
    BOOST_CHECK(handler->TxnBegin());
    BOOST_CHECK(handler->Write(key, value));
    BOOST_CHECK(handler->Exists(key));

    // But, the same key, does not exist in another handler
    std::unique_ptr<DatabaseBatch> handler2 = Assert(database)->MakeBatch();
    BOOST_CHECK(handler2->Exists(key));

    // Attempt to commit the handler txn calling the handler2 methods.
    // Which, must not be possible.
    BOOST_CHECK(!handler2->TxnCommit());
    BOOST_CHECK(!handler2->TxnAbort());

    // Only the first handler can commit the changes.
    BOOST_CHECK(handler->TxnCommit());
    // And, once commit is completed, handler2 can read the record
    std::string read_value;
    BOOST_CHECK(handler2->Read(key, read_value));
    BOOST_CHECK_EQUAL(read_value, value);

    // Also, once txn is committed, single write statements are re-enabled.
    // Which means that handler2 can read the record changes directly.
    BOOST_CHECK(handler->Write(key, value2, /*fOverwrite=*/true));
    BOOST_CHECK(handler2->Read(key, read_value));
    BOOST_CHECK_EQUAL(read_value, value2);
}

BOOST_AUTO_TEST_CASE(in_memory_database_cannot_reopen)
{
    // Reopening an in-memory database would create a fresh empty connection,
    // silently losing all data. Open() must throw instead.
    InMemoryWalletDatabase database;
    database.Close();
    BOOST_CHECK_THROW(database.Open(), std::runtime_error);
}

namespace {
//! Hand-crafts a legacy BDB file. Core has no BDB write support, so the read-only
//! parser can only be tested against bytes laid out here in the format it reads.
class BDBFileBuilder
{
    static constexpr uint32_t PAGE_BYTES{512};
    std::vector<std::byte> m_bytes;

    size_t Offset(uint32_t page, size_t off) const { return size_t{page} * PAGE_BYTES + off; }

public:
    explicit BDBFileBuilder(uint32_t num_pages) : m_bytes(size_t{num_pages} * PAGE_BYTES, std::byte{0}) {}

    void W8(uint32_t page, size_t off, uint8_t v) { m_bytes.at(Offset(page, off)) = std::byte{v}; }
    void WLE(uint32_t page, size_t off, uint32_t v, size_t n) { for (size_t i = 0; i < n; ++i) W8(page, off + i, static_cast<uint8_t>(v >> (8 * i))); }
    void WBE(uint32_t page, size_t off, uint32_t v, size_t n) { for (size_t i = 0; i < n; ++i) W8(page, off + i, static_cast<uint8_t>(v >> (8 * (n - 1 - i)))); }
    void WStr(uint32_t page, size_t off, std::string_view s) { for (char c : s) W8(page, off++, static_cast<uint8_t>(c)); }

    //! A BTREE_META page (type 9) pointing at a subdatabase root.
    void MetaPage(uint32_t page, uint32_t last_page, uint32_t root)
    {
        WLE(page, 4, 1, 4);            // lsn_offset = 1 (LSNs must read as reset)
        WLE(page, 8, page, 4);         // page_num
        WLE(page, 12, 0x00053162, 4);  // btree magic
        WLE(page, 16, 9, 4);           // version
        WLE(page, 20, PAGE_BYTES, 4);   // pagesize
        W8(page, 25, 9);               // type = BTREE_META
        WLE(page, 32, last_page, 4);   // last_page
        WLE(page, 48, 0x20, 4);        // flags = SUBDB
        WLE(page, 88, root, 4);        // root
    }

    //! The outer root leaf that names the "main" subdatabase and its meta page.
    void OuterLeaf(uint32_t page, uint32_t inner_meta_page)
    {
        WLE(page, 4, 1, 4);            // lsn_offset = 1
        WLE(page, 8, page, 4);         // page_num
        WLE(page, 20, 2, 2);           // entries = 2
        W8(page, 24, 1);               // level = 1
        W8(page, 25, 5);               // type = BTREE_LEAF
        WLE(page, 26, 30, 2);          // index[0] -> "main" record
        WLE(page, 28, 37, 2);          // index[1] -> inner meta page record
        WLE(page, 30, 4, 2); W8(page, 32, 1); WStr(page, 33, "main");
        WLE(page, 37, 4, 2); W8(page, 39, 1); WBE(page, 40, inner_meta_page, 4);
    }

    fs::path WriteTo(const fs::path& dir, const std::string& name) const
    {
        const fs::path path{dir / fs::PathFromString(name)};
        std::ofstream file{path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(m_bytes.data()), static_cast<std::streamsize>(m_bytes.size()));
        return path;
    }
};
} // namespace

BOOST_AUTO_TEST_CASE(bdbro_btree_page_referenced_twice)
{
    // An internal page with two entries pointing at the same child stays level
    // consistent, so the level check passes, but the child (and its subtree) would
    // be parsed once per parent edge. Check the parser rejects the second visit.
    BDBFileBuilder builder(/*num_pages=*/5);
    builder.MetaPage(/*page=*/0, /*last_page=*/4, /*root=*/1);
    builder.OuterLeaf(/*page=*/1, /*inner_meta_page=*/2);
    builder.MetaPage(/*page=*/2, /*last_page=*/4, /*root=*/3);

    // Page 3: an internal page (level 2) whose two entries point at one record
    // whose child page is 4.
    builder.WLE(3, 4, 1, 4);          // lsn_offset = 1
    builder.WLE(3, 8, 3, 4);          // page_num
    builder.WLE(3, 20, 2, 2);         // entries = 2
    builder.W8(3, 24, 2);             // level = 2
    builder.W8(3, 25, 3);             // type = BTREE_INTERNAL
    builder.WLE(3, 26, 30, 2);        // index[0] -> record
    builder.WLE(3, 28, 30, 2);        // index[1] -> same record
    builder.WLE(3, 30, 1, 2);         // RecordHeader.len = 1
    builder.W8(3, 32, 1);             // RecordHeader.type = KEYDATA
    builder.WLE(3, 34, 4, 4);         // InternalRecord.page_num = 4

    // Page 4: an empty leaf, parsed once and then reached a second time.
    builder.WLE(4, 4, 1, 4);          // lsn_offset = 1
    builder.WLE(4, 8, 4, 4);          // page_num
    builder.W8(4, 24, 1);             // level = 1
    builder.W8(4, 25, 5);             // type = BTREE_LEAF

    const fs::path path{builder.WriteTo(m_path_root, "bdbro_btree.dat")};

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    auto db{MakeBerkeleyRODatabase(path, options, status, error)};
    BOOST_CHECK(!db);
    BOOST_CHECK_EQUAL(status, DatabaseStatus::FAILED_LOAD);
    BOOST_CHECK_EQUAL(error.original, "BTree page referenced more than once");
}

BOOST_AUTO_TEST_CASE(bdbro_overflow_page_referenced_twice)
{
    // A record's overflow page points back at itself. It carries no data, so the
    // length check cannot bound the chain; check the repeated page is rejected.
    BDBFileBuilder builder(/*num_pages=*/5);
    builder.MetaPage(/*page=*/0, /*last_page=*/4, /*root=*/1);
    builder.OuterLeaf(/*page=*/1, /*inner_meta_page=*/2);
    builder.MetaPage(/*page=*/2, /*last_page=*/4, /*root=*/3);

    // Page 3: a leaf whose two records are a key and an overflow record -> page 4.
    builder.WLE(3, 4, 1, 4);          // lsn_offset = 1
    builder.WLE(3, 8, 3, 4);          // page_num
    builder.WLE(3, 20, 2, 2);         // entries = 2
    builder.W8(3, 24, 1);             // level = 1
    builder.W8(3, 25, 5);             // type = BTREE_LEAF
    builder.WLE(3, 26, 30, 2);        // index[0] -> key record
    builder.WLE(3, 28, 34, 2);        // index[1] -> overflow record
    builder.WLE(3, 30, 1, 2); builder.W8(3, 32, 1); builder.WStr(3, 33, "k");
    builder.WLE(3, 34, 0, 2);         // RecordHeader.len = 0
    builder.W8(3, 36, 3);             // RecordHeader.type = OVERFLOW_DATA
    builder.WLE(3, 38, 4, 4);         // OverflowRecord.page_number = 4
    builder.WLE(3, 42, 1, 4);         // OverflowRecord.item_len = 1

    // Page 4: an overflow page with no data whose next page is itself.
    builder.WLE(4, 4, 1, 4);          // lsn_offset = 1
    builder.WLE(4, 8, 4, 4);          // page_num
    builder.WLE(4, 16, 4, 4);         // next_page = 4
    builder.W8(4, 25, 7);             // type = OVERFLOW_DATA

    const fs::path path{builder.WriteTo(m_path_root, "bdbro_overflow.dat")};

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    auto db{MakeBerkeleyRODatabase(path, options, status, error)};
    BOOST_CHECK(!db);
    BOOST_CHECK_EQUAL(status, DatabaseStatus::FAILED_LOAD);
    BOOST_CHECK_EQUAL(error.original, "Overflow page referenced more than once");
}

BOOST_AUTO_TEST_CASE(bdbro_page_records_do_not_fit)
{
    // A page whose index entries all point at the same record makes the parser read
    // and keep that record once per entry. Craft one and check the parser rejects it
    // rather than reading a single page into entries * record_len bytes of memory.
    const uint32_t entries{40};
    const uint32_t record_off{26 + 2 * entries}; // just past the index array
    const uint32_t record_len{20};

    BDBFileBuilder builder(/*num_pages=*/4);
    builder.MetaPage(/*page=*/0, /*last_page=*/3, /*root=*/1);
    builder.OuterLeaf(/*page=*/1, /*inner_meta_page=*/2);
    builder.MetaPage(/*page=*/2, /*last_page=*/3, /*root=*/3);

    // Page 3: the inner root leaf, with every index pointing at one record.
    builder.WLE(3, 4, 1, 4);          // lsn_offset = 1
    builder.WLE(3, 8, 3, 4);          // page_num
    builder.WLE(3, 20, entries, 2);   // entries
    builder.W8(3, 24, 1);             // level = 1
    builder.W8(3, 25, 5);             // type = BTREE_LEAF
    for (uint32_t i = 0; i < entries; ++i) {
        builder.WLE(3, 26 + 2 * i, record_off, 2);
    }
    builder.WLE(3, record_off, record_len, 2); // RecordHeader.len
    builder.W8(3, record_off + 2, 1);          // RecordHeader.type = KEYDATA

    const fs::path path{builder.WriteTo(m_path_root, "bdbro_records.dat")};

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    auto db{MakeBerkeleyRODatabase(path, options, status, error)};
    BOOST_CHECK(!db);
    BOOST_CHECK_EQUAL(status, DatabaseStatus::FAILED_LOAD);
    BOOST_CHECK_EQUAL(error.original, "Data records exceed page size");
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
