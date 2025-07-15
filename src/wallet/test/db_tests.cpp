// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/translation.h>
#include <wallet/sqlite.h>
#include <wallet/migrate.h>
#include <wallet/test/util.h>
#include <wallet/walletutil.h>

#include <cstddef>
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

BOOST_AUTO_TEST_CASE(database_readonly_comprehensive_test)
{
    // Test read-only database behavior - write operations should fail
    // and database state should remain unchanged after failed writes

    // Test MockableDatabase read-only enforcement
    // First create a regular (writable) database and populate it
    auto writable_db = CreateMockableWalletDatabase();
    std::unique_ptr<DatabaseBatch> write_batch = writable_db->MakeBatch();

    // Write some initial data
    BOOST_CHECK(write_batch->Write(std::string("key1"), std::string("value1")));
    BOOST_CHECK(write_batch->Write(std::string("key2"), std::string("value2")));

    // Copy the data to create a read-only database
    MockableData copied_data = dynamic_cast<MockableDatabase&>(*writable_db).m_records;
    auto readonly_db = CreateMockableWalletDatabase(copied_data, true);
    BOOST_CHECK(readonly_db->IsReadOnly());
    std::unique_ptr<DatabaseBatch> readonly_batch = readonly_db->MakeBatch();

    // Verify initial state - read operations should work
    std::string read_value;
    BOOST_CHECK(readonly_batch->Read(std::string("key1"), read_value));
    BOOST_CHECK_EQUAL(read_value, "value1");
    BOOST_CHECK(readonly_batch->Read(std::string("key2"), read_value));
    BOOST_CHECK_EQUAL(read_value, "value2");
    BOOST_CHECK(readonly_batch->Exists(std::string("key1")));
    BOOST_CHECK(readonly_batch->Exists(std::string("key2")));

    // Critical test: Write operations must fail and return false
    BOOST_CHECK(!readonly_batch->Write(std::string("key3"), std::string("value3")));
    BOOST_CHECK(!readonly_batch->Write(std::string("key1"), std::string("newvalue"), true));
    BOOST_CHECK(!readonly_batch->Erase(std::string("key1")));

    // Verify database state is unchanged after failed write attempts
    BOOST_CHECK(!readonly_batch->Exists(std::string("key3"))); // New key should not exist
    BOOST_CHECK(readonly_batch->Read(std::string("key1"), read_value));
    BOOST_CHECK_EQUAL(read_value, "value1"); // Original value should be unchanged
    BOOST_CHECK(readonly_batch->Read(std::string("key2"), read_value));
    BOOST_CHECK_EQUAL(read_value, "value2"); // Original value should be unchanged

    // Test that ErasePrefix also fails on read-only database
    DataStream prefix_stream;
    prefix_stream << std::string("key");
    BOOST_CHECK(!readonly_batch->ErasePrefix(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(prefix_stream.data()), prefix_stream.size())));

    // Verify keys still exist after failed ErasePrefix
    BOOST_CHECK(readonly_batch->Exists(std::string("key1")));
    BOOST_CHECK(readonly_batch->Exists(std::string("key2")));

    // Test SQLite database read-only behavior if possible
    DatabaseOptions read_write_options;
    DatabaseStatus status;
    bilingual_str error;
    auto rw_database = MakeSQLiteDatabase(m_path_root / "sqlite_rw", read_write_options, status, error);
    if (rw_database) {
        BOOST_CHECK(!rw_database->IsReadOnly());

        // Create and populate the database first
        rw_database->Open();
        {
            std::unique_ptr<DatabaseBatch> batch = rw_database->MakeBatch();
            BOOST_CHECK(batch->Write(std::string("key1"), std::string("value1")));
            BOOST_CHECK(batch->Write(std::string("key2"), std::string("value2")));
        }
        rw_database->Close();

        // Now test read-only access to existing database
        DatabaseOptions read_only_options;
        read_only_options.read_only = true;
        auto ro_database = MakeSQLiteDatabase(m_path_root / "sqlite_rw", read_only_options, status, error);
        if (ro_database && ro_database->IsReadOnly()) {
            std::unique_ptr<DatabaseBatch> ro_batch = ro_database->MakeBatch();

            // Read operations should work
            BOOST_CHECK(ro_batch->Read(std::string("key1"), read_value));
            BOOST_CHECK_EQUAL(read_value, "value1");

            // Write operations should fail on SQLite read-only database
            BOOST_CHECK(!ro_batch->Write(std::string("key3"), std::string("value3")));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
