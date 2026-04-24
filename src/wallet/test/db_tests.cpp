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

// ---------------------------------------------------------------------------
// Regression tests for cyclic page references in the legacy BDB parser.
//
// The read-only BDB parser (src/wallet/migrate.cpp) is fed an attacker-supplied
// file whenever a user runs `migratewallet` or `bitcoin-wallet -wallet=X dump`.
// Its B-tree DFS and overflow-chain traversal both follow page numbers that
// are read directly from the file. Without visited-page tracking, a crafted
// file containing a cyclic page reference causes BerkeleyRODatabase::Open()
// to loop forever (and, for overflow chains, grow memory without bound).
//
// The two tests below build minimal hand-crafted BDB files that exercise each
// of the two cyclic code paths, and assert that MakeBerkeleyRODatabase()
// rejects them with DatabaseStatus::FAILED_LOAD rather than hanging.
// ---------------------------------------------------------------------------

namespace {
constexpr uint32_t kPageSize = 512;
constexpr uint32_t kBtreeMagic = 0x00053162;

class BdbFileBuilder
{
    std::vector<uint8_t> m_buf;

public:
    explicit BdbFileBuilder(uint32_t num_pages) : m_buf(kPageSize * num_pages, 0) {}

    void W8(size_t off, uint8_t v) { m_buf.at(off) = v; }
    void W16(size_t off, uint16_t v)
    {
        m_buf.at(off)     = v & 0xFF;
        m_buf.at(off + 1) = (v >> 8) & 0xFF;
    }
    void W32(size_t off, uint32_t v)
    {
        m_buf.at(off)     = v & 0xFF;
        m_buf.at(off + 1) = (v >> 8) & 0xFF;
        m_buf.at(off + 2) = (v >> 16) & 0xFF;
        m_buf.at(off + 3) = (v >> 24) & 0xFF;
    }
    void W32BE(size_t off, uint32_t v)
    {
        m_buf.at(off)     = (v >> 24) & 0xFF;
        m_buf.at(off + 1) = (v >> 16) & 0xFF;
        m_buf.at(off + 2) = (v >> 8) & 0xFF;
        m_buf.at(off + 3) = v & 0xFF;
    }

    // Writes a BTree meta page (BDB page type 9) at the given page index.
    void WriteMetaPage(uint32_t page_idx, uint32_t last_page, uint32_t root)
    {
        const size_t p = page_idx * kPageSize;
        W32(p +  0, 0);              // lsn_file   = 0
        W32(p +  4, 1);              // lsn_offset = 1 (LSNs-reset check)
        W32(p +  8, page_idx);       // page_num
        W32(p + 12, kBtreeMagic);    // magic
        W32(p + 16, 9);              // version
        W32(p + 20, kPageSize);      // pagesize
        W8 (p + 24, 0);              // encrypt_algo
        W8 (p + 25, 9);              // type = BTREE_META
        W32(p + 32, last_page);      // last_page
        W32(p + 48, 0x20);           // flags = SUBDB
        W32(p + 76, 2);              // minkey
        W32(p + 88, root);           // root
    }

    // Writes the outer BTREE_LEAF that names the subdatabase "main" and
    // points at the inner meta page.
    void WriteOuterLeaf(uint32_t page_idx, uint32_t inner_meta_page)
    {
        const size_t p = page_idx * kPageSize;
        W32(p +  0, 0);              // lsn_file
        W32(p +  4, 1);              // lsn_offset = 1
        W32(p +  8, page_idx);       // page_num
        W16(p + 20, 2);              // entries = 2
        W16(p + 22, 498);            // hf_offset
        W8 (p + 24, 1);              // level
        W8 (p + 25, 5);              // type = BTREE_LEAF
        W16(p + 26, 498);            // index[0] → "main"
        W16(p + 28, 505);            // index[1] → BE(inner_meta_page)
        // Record 0 at offset 498: key "main"
        W16(p + 498, 4);             // RecordHeader.len
        W8 (p + 500, 1);             // RecordHeader.type = KEYDATA
        W8 (p + 501, 'm'); W8(p + 502, 'a'); W8(p + 503, 'i'); W8(p + 504, 'n');
        // Record 1 at offset 505: big-endian inner meta page number
        W16(p + 505, 4);             // RecordHeader.len
        W8 (p + 507, 1);             // RecordHeader.type = KEYDATA
        W32BE(p + 508, inner_meta_page);
    }

    fs::path WriteTo(const fs::path& dir, const std::string& name) const
    {
        fs::path path = dir / fs::PathFromString(name);
        std::ofstream f(path.std_path(), std::ios::binary);
        BOOST_REQUIRE(f.good());
        f.write(reinterpret_cast<const char*>(m_buf.data()),
                static_cast<std::streamsize>(m_buf.size()));
        BOOST_REQUIRE(f.good());
        return path;
    }
};
} // namespace

BOOST_AUTO_TEST_CASE(bdb_parser_btree_cycle_detection)
{
    // 4 pages: outer meta, outer leaf, inner meta, inner BTREE_INTERNAL with
    // a self-referential InternalRecord (page_num = own page).
    BdbFileBuilder b(/*num_pages=*/4);
    b.WriteMetaPage(/*page_idx=*/0, /*last_page=*/3, /*root=*/1);
    b.WriteOuterLeaf(/*page_idx=*/1, /*inner_meta_page=*/2);
    b.WriteMetaPage(/*page_idx=*/2, /*last_page=*/3, /*root=*/3);

    // Page 3: BTREE_INTERNAL with one record whose page_num points back to 3.
    // The LSN-reset loop runs for i < outer_meta.last_page (= 3), so page 3
    // is not subject to that check; only the DFS will touch it.
    constexpr size_t P3 = 3 * kPageSize;
    b.W32(P3 +  8, 3);               // page_num
    b.W16(P3 + 20, 1);               // entries = 1
    b.W16(P3 + 22, 499);             // hf_offset
    b.W8 (P3 + 24, 1);               // level
    b.W8 (P3 + 25, 3);               // type = BTREE_INTERNAL
    b.W16(P3 + 26, 499);             // index[0] → offset 499
    b.W16(P3 + 499, 1);              // RecordHeader.len = 1
    b.W8 (P3 + 501, 1);              // RecordHeader.type = KEYDATA
    b.W8 (P3 + 502, 0);              // unused
    b.W32(P3 + 503, 3);              // InternalRecord.page_num = 3  ← CYCLE
    b.W32(P3 + 507, 0);              // records
    b.W8 (P3 + 511, 0);              // data[0]

    const fs::path wallet_path = b.WriteTo(m_path_root, "cycle_btree.dat");

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    auto db = MakeBerkeleyRODatabase(wallet_path, options, status, error);
    BOOST_CHECK(!db);
    BOOST_CHECK_EQUAL(status, DatabaseStatus::FAILED_LOAD);
    BOOST_CHECK_EQUAL(error.original, "Cyclic page reference in BDB database");
}

BOOST_AUTO_TEST_CASE(bdb_parser_overflow_cycle_detection)
{
    // 5 pages: outer meta, outer leaf, inner meta, inner BTREE_LEAF with a
    // key + OverflowRecord pointing at an OVERFLOW_DATA page whose next_page
    // points back to itself.
    BdbFileBuilder b(/*num_pages=*/5);
    b.WriteMetaPage(/*page_idx=*/0, /*last_page=*/4, /*root=*/1);
    b.WriteOuterLeaf(/*page_idx=*/1, /*inner_meta_page=*/2);
    b.WriteMetaPage(/*page_idx=*/2, /*last_page=*/4, /*root=*/3);

    // Page 3: BTREE_LEAF with 2 records (key + overflow value).
    //   index[0] → offset 496: DataRecord(len=1, "k")
    //   index[1] → offset 500: OverflowRecord(page_number = 4)
    constexpr size_t P3 = 3 * kPageSize;
    b.W32(P3 +  0, 0);               // lsn_file   = 0
    b.W32(P3 +  4, 1);               // lsn_offset = 1 (LSNs-reset check covers this page)
    b.W32(P3 +  8, 3);               // page_num
    b.W16(P3 + 20, 2);               // entries = 2
    b.W16(P3 + 22, 496);             // hf_offset
    b.W8 (P3 + 24, 1);               // level
    b.W8 (P3 + 25, 5);               // type = BTREE_LEAF
    b.W16(P3 + 26, 496);             // index[0]
    b.W16(P3 + 28, 500);             // index[1]
    // Record 0 at 496: key "k"
    b.W16(P3 + 496, 1);              // RecordHeader.len
    b.W8 (P3 + 498, 1);              // RecordHeader.type = KEYDATA
    b.W8 (P3 + 499, 'k');
    // Record 1 at 500: OverflowRecord pointing to page 4
    b.W16(P3 + 500, 0);              // RecordHeader.len (informational)
    b.W8 (P3 + 502, 3);              // RecordHeader.type = OVERFLOW_DATA
    b.W8 (P3 + 503, 0);              // unused2
    b.W32(P3 + 504, 4);              // OverflowRecord.page_number = 4
    b.W32(P3 + 508, 0);              // item_len

    // Page 4: OVERFLOW_DATA page with next_page = 4  ← CYCLE.
    // Not covered by the LSN-reset loop (runs only for i < last_page = 4).
    constexpr size_t P4 = 4 * kPageSize;
    b.W32(P4 +  8, 4);               // page_num
    b.W32(P4 + 16, 4);               // next_page = 4  ← CYCLE
    b.W16(P4 + 20, 0);               // entries (reference count; unused here)
    b.W16(P4 + 22, 0);               // hf_offset = 0 (overflow data length)
    b.W8 (P4 + 24, 0);               // level = 0 (required for OVERFLOW_DATA)
    b.W8 (P4 + 25, 7);               // type = OVERFLOW_DATA

    const fs::path wallet_path = b.WriteTo(m_path_root, "cycle_overflow.dat");

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    auto db = MakeBerkeleyRODatabase(wallet_path, options, status, error);
    BOOST_CHECK(!db);
    BOOST_CHECK_EQUAL(status, DatabaseStatus::FAILED_LOAD);
    BOOST_CHECK_EQUAL(error.original, "Cyclic page reference in BDB database");
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
