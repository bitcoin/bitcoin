// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/byteswap.h>
#include <crypto/common.h>
#include <logging.h>
#include <streams.h>
#include <util/translation.h>
#include <wallet/migrate.h>

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

namespace wallet {
// Magic bytes in both endianness's
constexpr uint32_t BTREE_MAGIC = 0x00053162;    // If the file endianness matches our system, we see this magic
constexpr uint32_t BTREE_MAGIC_OE = 0x62310500; // If the file endianness is the other one, we will see this magic

// Subdatabase name
static const std::vector<std::byte> SUBDATABASE_NAME = {std::byte{'m'}, std::byte{'a'}, std::byte{'i'}, std::byte{'n'}};

enum class PageType : uint8_t {
    /*
     * BDB has several page types, most of which we do not use
     * They are listed here for completeness, but commented out
     * to avoid opening something unintended.
    INVALID = 0,         // Invalid page type
    DUPLICATE = 1,       // Duplicate. Deprecated and no longer used
    HASH_UNSORTED = 2,   // Hash pages. Deprecated.
    RECNO_INTERNAL = 4,  // Recno internal
    RECNO_LEAF = 6,      // Recno leaf
    HASH_META = 8,       // Hash metadata
    QUEUE_META = 10,     // Queue Metadata
    QUEUE_DATA = 11,     // Queue Data
    DUPLICATE_LEAF = 12, // Off-page duplicate leaf
    HASH_SORTED = 13,    // Sorted hash page
    */
    BTREE_INTERNAL = 3, // BTree internal
    BTREE_LEAF = 5,     // BTree leaf
    OVERFLOW_DATA = 7,  // Overflow
    BTREE_META = 9,     // BTree metadata
};

enum class RecordType : uint8_t {
    KEYDATA = 1,
    // DUPLICATE = 2,       Unused as our databases do not support duplicate records
    OVERFLOW_DATA = 3,
    DELETE = 0x80, // Indicate this record is deleted. This is OR'd with the real type.
};

enum class BTreeFlags : uint32_t {
    /*
     * BTree databases have feature flags, but we do not use them except for
     * subdatabases. The unused flags are included for completeness, but commented out
     * to avoid accidental use.
    DUP = 1,         // Duplicates
    RECNO = 2,       // Recno tree
    RECNUM = 4,      // BTree: Maintain record counts
    FIXEDLEN = 8,    // Recno: fixed length records
    RENUMBER = 0x10, // Recno: renumber on insert/delete
    DUPSORT = 0x40,  // Duplicates are sorted
    COMPRESS = 0x80, // Compressed
    */
    SUBDB = 0x20, // Subdatabases
};

/** Berkeley DB BTree metadata page layout */
class MetaPage
{
public:
    uint32_t lsn_file;             // Log Sequence Number file
    uint32_t lsn_offset;           // Log Sequence Number offset
    uint32_t page_num;             // Current page number
    uint32_t magic;                // Magic number
    uint32_t version;              // Version
    uint32_t pagesize;             // Page size
    uint8_t encrypt_algo;          // Encryption algorithm
    PageType type;                 // Page type
    uint8_t metaflags;             // Meta-only flags
    uint8_t unused1;               // Unused
    uint32_t free_list;            // Free list page number
    uint32_t last_page;            // Page number of last page in db
    uint32_t partitions;           // Number of partitions
    uint32_t key_count;            // Cached key count
    uint32_t record_count;         // Cached record count
    BTreeFlags flags;              // Flags
    std::array<std::byte, 20> uid; // 20 byte unique file ID
    uint32_t unused2;              // Unused
    uint32_t minkey;               // Minimum key
    uint32_t re_len;               // Recno: fixed length record length
    uint32_t re_pad;               // Recno: fixed length record pad
    uint32_t root;                 // Root page number
    char unused3[368];             // 92 * 4 bytes of unused space
    uint32_t crypto_magic;         // Crypto magic number
    char trash[12];                // 3 * 4 bytes of trash space
    unsigned char iv[20];          // Crypto IV
    unsigned char chksum[16];      // Checksum

    bool other_endian;
    uint32_t expected_page_num;

    MetaPage(uint32_t expected_page_num) : expected_page_num(expected_page_num) {}
    MetaPage() = delete;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> lsn_file;
        s >> lsn_offset;
        s >> page_num;
        s >> magic;
        s >> version;
        s >> pagesize;
        s >> encrypt_algo;

        other_endian = magic == BTREE_MAGIC_OE;

        uint8_t uint8_type;
        s >> uint8_type;
        type = static_cast<PageType>(uint8_type);

        s >> metaflags;
        s >> unused1;
        s >> free_list;
        s >> last_page;
        s >> partitions;
        s >> key_count;
        s >> record_count;

        uint32_t uint32_flags;
        s >> uint32_flags;
        if (other_endian) {
            uint32_flags = internal_bswap_32(uint32_flags);
        }
        flags = static_cast<BTreeFlags>(uint32_flags);

        s >> uid;
        s >> unused2;
        s >> minkey;
        s >> re_len;
        s >> re_pad;
        s >> root;
        s >> unused3;
        s >> crypto_magic;
        s >> trash;
        s >> iv;
        s >> chksum;

        if (other_endian) {
            lsn_file = internal_bswap_32(lsn_file);
            lsn_offset = internal_bswap_32(lsn_offset);
            page_num = internal_bswap_32(page_num);
            magic = internal_bswap_32(magic);
            version = internal_bswap_32(version);
            pagesize = internal_bswap_32(pagesize);
            free_list = internal_bswap_32(free_list);
            last_page = internal_bswap_32(last_page);
            partitions = internal_bswap_32(partitions);
            key_count = internal_bswap_32(key_count);
            record_count = internal_bswap_32(record_count);
            unused2 = internal_bswap_32(unused2);
            minkey = internal_bswap_32(minkey);
            re_len = internal_bswap_32(re_len);
            re_pad = internal_bswap_32(re_pad);
            root = internal_bswap_32(root);
            crypto_magic = internal_bswap_32(crypto_magic);
        }

        // Page number must match
        if (page_num != expected_page_num) {
            throw std::runtime_error("Meta page number mismatch");
        }

        // Check magic
        if (magic != BTREE_MAGIC) {
            throw std::runtime_error("Not a BDB file");
        }

        // Only version 9 is supported
        if (version != 9) {
            throw std::runtime_error("Unsupported BDB data file version number");
        }

        // Page size must be 512 <= pagesize <= 64k, and be a power of 2
        if (pagesize < 512 || pagesize > 65536 || (pagesize & (pagesize - 1)) != 0) {
            throw std::runtime_error("Bad page size");
        }

        // Page type must be the btree type
        if (type != PageType::BTREE_META) {
            throw std::runtime_error("Unexpected page type, should be 9 (BTree Metadata)");
        }

        // Only supported meta-flag is subdatabase
        if (flags != BTreeFlags::SUBDB) {
            throw std::runtime_error("Unexpected database flags, should only be 0x20 (subdatabases)");
        }
    }
};

/** General class for records in a BDB BTree database. Contains common fields. */
class RecordHeader
{
public:
    uint16_t len;    // Key/data item length
    RecordType type; // Page type (BDB has this include a DELETE FLAG that we track separately)
    bool deleted;    // Whether the DELETE flag was set on type

    static constexpr size_t SIZE = 3; // The record header is 3 bytes

    bool other_endian;

    RecordHeader(bool other_endian) : other_endian(other_endian) {}
    RecordHeader() = delete;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> len;

        uint8_t uint8_type;
        s >> uint8_type;
        type = static_cast<RecordType>(uint8_type & ~static_cast<uint8_t>(RecordType::DELETE));
        deleted = uint8_type & static_cast<uint8_t>(RecordType::DELETE);

        if (other_endian) {
            len = internal_bswap_16(len);
        }
    }
};

/** Class for data in the record directly */
class DataRecord
{
public:
    DataRecord(const RecordHeader& header) : m_header(header) {}
    DataRecord() = delete;

    RecordHeader m_header;

    std::vector<std::byte> data; // Variable length key/data item

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        data.resize(m_header.len);
        s.read(std::as_writable_bytes(std::span(data.data(), data.size())));
    }
};

/** Class for records representing internal nodes of the BTree. */
class InternalRecord
{
public:
    InternalRecord(const RecordHeader& header) : m_header(header) {}
    InternalRecord() = delete;

    RecordHeader m_header;

    uint8_t unused;              // Padding, unused
    uint32_t page_num;           // Page number of referenced page
    uint32_t records;            // Subtree record count
    std::vector<std::byte> data; // Variable length key item

    static constexpr size_t FIXED_SIZE = 9; // Size of fixed data is 9 bytes

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> unused;
        s >> page_num;
        s >> records;

        data.resize(m_header.len);
        s.read(std::as_writable_bytes(std::span(data.data(), data.size())));

        if (m_header.other_endian) {
            page_num = internal_bswap_32(page_num);
            records = internal_bswap_32(records);
        }
    }
};

/** Class for records representing overflow records of the BTree.
 * Overflow records point to a page which contains the data in the record.
 * Those pages may point to further pages with the rest of the data if it does not fit
 * in one page */
class OverflowRecord
{
public:
    OverflowRecord(const RecordHeader& header) : m_header(header) {}
    OverflowRecord() = delete;

    RecordHeader m_header;

    uint8_t unused2;      // Padding, unused
    uint32_t page_number; // Page number where data begins
    uint32_t item_len;    // Total length of item

    static constexpr size_t SIZE = 9; // Overflow record is always 9 bytes

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> unused2;
        s >> page_number;
        s >> item_len;

        if (m_header.other_endian) {
            page_number = internal_bswap_32(page_number);
            item_len = internal_bswap_32(item_len);
        }
    }
};

/** A generic data page in the database. Contains fields common to all data pages. */
class PageHeader
{
public:
    uint32_t lsn_file;   // Log Sequence Number file
    uint32_t lsn_offset; // Log Sequence Number offset
    uint32_t page_num;   // Current page number
    uint32_t prev_page;  // Previous page number
    uint32_t next_page;  // Next page number
    uint16_t entries;    // Number of items on the page
    uint16_t hf_offset;  // High free byte page offset
    uint8_t level;       // Btree page level
    PageType type;       // Page type

    static constexpr int64_t SIZE = 26; // The header is 26 bytes

    uint32_t expected_page_num;
    bool other_endian;

    PageHeader(uint32_t page_num, bool other_endian) : expected_page_num(page_num), other_endian(other_endian) {}
    PageHeader() = delete;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> lsn_file;
        s >> lsn_offset;
        s >> page_num;
        s >> prev_page;
        s >> next_page;
        s >> entries;
        s >> hf_offset;
        s >> level;

        uint8_t uint8_type;
        s >> uint8_type;
        type = static_cast<PageType>(uint8_type);

        if (other_endian) {
            lsn_file = internal_bswap_32(lsn_file);
            lsn_offset = internal_bswap_32(lsn_offset);
            page_num = internal_bswap_32(page_num);
            prev_page = internal_bswap_32(prev_page);
            next_page = internal_bswap_32(next_page);
            entries = internal_bswap_16(entries);
            hf_offset = internal_bswap_16(hf_offset);
        }

        if (expected_page_num != page_num) {
            throw std::runtime_error("Page number mismatch");
        }
        if ((type != PageType::OVERFLOW_DATA && level < 1) || (type == PageType::OVERFLOW_DATA && level != 0)) {
            throw std::runtime_error("Bad btree level");
        }
    }
};

/** A page of records in the database */
class RecordsPage
{
public:
    RecordsPage(const PageHeader& header) : m_header(header) {}
    RecordsPage() = delete;

    PageHeader m_header;

    std::vector<uint16_t> indexes;
    std::vector<std::variant<DataRecord, OverflowRecord>> records;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        // Current position within the page
        int64_t pos = PageHeader::SIZE;

        // Get the items
        for (uint32_t i = 0; i < m_header.entries; ++i) {
            // Get the index
            uint16_t index;
            s >> index;
            if (m_header.other_endian) {
                index = internal_bswap_16(index);
            }
            indexes.push_back(index);
            pos += sizeof(uint16_t);

            // Go to the offset from the index
            int64_t to_jump = index - pos;
            if (to_jump < 0) {
                throw std::runtime_error("Data record position not in page");
            }
            s.ignore(to_jump);

            // Read the record
            RecordHeader rec_hdr(m_header.other_endian);
            s >> rec_hdr;
            to_jump += RecordHeader::SIZE;

            switch (rec_hdr.type) {
            case RecordType::KEYDATA: {
                DataRecord record(rec_hdr);
                s >> record;
                records.emplace_back(record);
                to_jump += rec_hdr.len;
                break;
            }
            case RecordType::OVERFLOW_DATA: {
                OverflowRecord record(rec_hdr);
                s >> record;
                records.emplace_back(record);
                to_jump += OverflowRecord::SIZE;
                break;
            }
            default:
                throw std::runtime_error("Unknown record type in records page");
            }

            // Go back to the indexes
            s.seek(-to_jump, SEEK_CUR);
        }
    }
};

/** A page containing overflow data */
class OverflowPage
{
public:
    OverflowPage(const PageHeader& header) : m_header(header) {}
    OverflowPage() = delete;

    PageHeader m_header;

    // BDB overloads some page fields to store overflow page data
    // hf_offset contains the length of the overflow data stored on this page
    // entries contains a reference count for references to this item

    // The overflow data itself. Begins immediately following header
    std::vector<std::byte> data;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        data.resize(m_header.hf_offset);
        s.read(std::as_writable_bytes(std::span(data.data(), data.size())));
    }
};

/** A page of records in the database */
class InternalPage
{
public:
    InternalPage(const PageHeader& header) : m_header(header) {}
    InternalPage() = delete;

    PageHeader m_header;

    std::vector<uint16_t> indexes;
    std::vector<InternalRecord> records;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        // Current position within the page
        int64_t pos = PageHeader::SIZE;

        // Get the items
        for (uint32_t i = 0; i < m_header.entries; ++i) {
            // Get the index
            uint16_t index;
            s >> index;
            if (m_header.other_endian) {
                index = internal_bswap_16(index);
            }
            indexes.push_back(index);
            pos += sizeof(uint16_t);

            // Go to the offset from the index
            int64_t to_jump = index - pos;
            if (to_jump < 0) {
                throw std::runtime_error("Internal record position not in page");
            }
            s.ignore(to_jump);

            // Read the record
            RecordHeader rec_hdr(m_header.other_endian);
            s >> rec_hdr;
            to_jump += RecordHeader::SIZE;

            if (rec_hdr.type != RecordType::KEYDATA) {
                throw std::runtime_error("Unknown record type in internal page");
            }
            InternalRecord record(rec_hdr);
            s >> record;
            records.emplace_back(record);
            to_jump += InternalRecord::FIXED_SIZE + rec_hdr.len;

            // Go back to the indexes
            s.seek(-to_jump, SEEK_CUR);
        }
    }
};

static void SeekToPage(AutoFile& s, uint32_t page_num, uint32_t page_size)
{
    int64_t pos = int64_t{page_num} * page_size;
    s.seek(pos, SEEK_SET);
}

void BerkeleyRODatabase::Open()
{
    // Open the file
    FILE* file = fsbridge::fopen(m_filepath, "rb");
    AutoFile db_file(file);
    if (db_file.IsNull()) {
        throw std::runtime_error("BerkeleyRODatabase: Failed to open database file");
    }

    uint32_t page_size = 4096; // Default page size

    // Read the outer metapage
    // Expected page number is 0
    MetaPage outer_meta(0);
    db_file >> outer_meta;
    page_size = outer_meta.pagesize;

    // Verify the size of the file is a multiple of the page size
    db_file.seek(0, SEEK_END);
    int64_t size = db_file.tell();

    // Since BDB stores everything in a page, the file size should be a multiple of the page size;
    // However, BDB doesn't actually check that this is the case, and enforcing this check results
    // in us rejecting a database that BDB would not, so this check needs to be excluded.
    // This is left commented out as a reminder to not accidentally implement this in the future.
    // if (size % page_size != 0) {
    //     throw std::runtime_error("File size is not a multiple of page size");
    // }

    // Check the last page number
    uint32_t expected_last_page{uint32_t((size / page_size) - 1)};
    if (outer_meta.last_page != expected_last_page) {
        throw std::runtime_error("Last page number could not fit in file");
    }

    // Make sure encryption is disabled
    if (outer_meta.encrypt_algo != 0) {
        throw std::runtime_error("BDB builtin encryption is not supported");
    }

    // Check all Log Sequence Numbers (LSN) point to file 0 and offset 1 which indicates that the LSNs were
    // reset and that the log files are not necessary to get all of the data in the database.
    for (uint32_t i = 0; i < outer_meta.last_page; ++i) {
        // The LSN is composed of 2 32-bit ints, the first is a file id, the second an offset
        // It will always be the first 8 bytes of a page, so we deserialize it directly for every page
        uint32_t file;
        uint32_t offset;
        SeekToPage(db_file, i, page_size);
        db_file >> file >> offset;
        if (outer_meta.other_endian) {
            file = internal_bswap_32(file);
            offset = internal_bswap_32(offset);
        }
        if (file != 0 || offset != 1) {
            throw std::runtime_error("LSNs are not reset, this database is not completely flushed. Please reopen then close the database with a version that has BDB support");
        }
    }

    // Read the root page
    SeekToPage(db_file, outer_meta.root, page_size);
    PageHeader header(outer_meta.root, outer_meta.other_endian);
    db_file >> header;
    if (header.type != PageType::BTREE_LEAF) {
        throw std::runtime_error("Unexpected outer database root page type");
    }
    if (header.entries != 2) {
        throw std::runtime_error("Unexpected number of entries in outer database root page");
    }
    RecordsPage page(header);
    db_file >> page;

    // First record should be the string "main"
    if (!std::holds_alternative<DataRecord>(page.records.at(0)) || std::get<DataRecord>(page.records.at(0)).data != SUBDATABASE_NAME) {
        throw std::runtime_error("Subdatabase has an unexpected name");
    }
    // Check length of page number for subdatabase location
    if (!std::holds_alternative<DataRecord>(page.records.at(1)) || std::get<DataRecord>(page.records.at(1)).m_header.len != 4) {
        throw std::runtime_error("Subdatabase page number has unexpected length");
    }

    // Read subdatabase page number
    // It is written as a big endian 32 bit number
    uint32_t main_db_page = ReadBE32(std::get<DataRecord>(page.records.at(1)).data.data());

    // The main database is in a page that doesn't exist
    if (main_db_page > outer_meta.last_page) {
        throw std::runtime_error("Page number is greater than database last page");
    }

    // Read the inner metapage
    SeekToPage(db_file, main_db_page, page_size);
    MetaPage inner_meta(main_db_page);
    db_file >> inner_meta;

    if (inner_meta.pagesize != page_size) {
        throw std::runtime_error("Unexpected page size");
    }

    if (inner_meta.last_page > outer_meta.last_page) {
        throw std::runtime_error("Subdatabase last page is greater than database last page");
    }

    // Make sure encryption is disabled
    if (inner_meta.encrypt_algo != 0) {
        throw std::runtime_error("BDB builtin encryption is not supported");
    }

    // Do a DFS through the BTree, starting at root
    std::vector<uint32_t> pages{inner_meta.root};
    while (pages.size() > 0) {
        uint32_t curr_page = pages.back();
        // It turns out BDB completely ignores this last_page field and doesn't actually update it to the correct
        // last page. While we should be checking this, we can't.
        // This is left commented out as a reminder to not accidentally implement this in the future.
        // if (curr_page > inner_meta.last_page) {
        //     throw std::runtime_error("Page number is greater than subdatabase last page");
        // }
        pages.pop_back();
        SeekToPage(db_file, curr_page, page_size);
        PageHeader header(curr_page, inner_meta.other_endian);
        db_file >> header;
        switch (header.type) {
        case PageType::BTREE_INTERNAL: {
            InternalPage int_page(header);
            db_file >> int_page;
            for (const InternalRecord& rec : int_page.records) {
                if (rec.m_header.deleted) continue;
                pages.push_back(rec.page_num);
            }
            break;
        }
        case PageType::BTREE_LEAF: {
            RecordsPage rec_page(header);
            db_file >> rec_page;
            if (rec_page.records.size() % 2 != 0) {
                // BDB stores key value pairs in consecutive records, thus an odd number of records is unexpected
                throw std::runtime_error("Records page has odd number of records");
            }
            bool is_key = true;
            std::vector<std::byte> key;
            for (const std::variant<DataRecord, OverflowRecord>& rec : rec_page.records) {
                std::vector<std::byte> data;
                if (const DataRecord* drec = std::get_if<DataRecord>(&rec)) {
                    if (drec->m_header.deleted) continue;
                    data = drec->data;
                } else if (const OverflowRecord* orec = std::get_if<OverflowRecord>(&rec)) {
                    if (orec->m_header.deleted) continue;
                    uint32_t next_page = orec->page_number;
                    while (next_page != 0) {
                        SeekToPage(db_file, next_page, page_size);
                        PageHeader opage_header(next_page, inner_meta.other_endian);
                        db_file >> opage_header;
                        if (opage_header.type != PageType::OVERFLOW_DATA) {
                            throw std::runtime_error("Bad overflow record page type");
                        }
                        OverflowPage opage(opage_header);
                        db_file >> opage;
                        data.insert(data.end(), opage.data.begin(), opage.data.end());
                        next_page = opage_header.next_page;
                    }
                }

                if (is_key) {
                    key = data;
                } else {
                    m_records.emplace(SerializeData{key.begin(), key.end()}, SerializeData{data.begin(), data.end()});
                    key.clear();
                }
                is_key = !is_key;
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected page type");
        }
    }
}

std::unique_ptr<DatabaseBatch> BerkeleyRODatabase::MakeBatch()
{
    return std::make_unique<BerkeleyROBatch>(*this);
}

bool BerkeleyRODatabase::Backup(const std::string& dest) const
{
    fs::path src(m_filepath);
    fs::path dst(fs::PathFromString(dest));

    if (fs::is_directory(dst)) {
        dst = BDBDataFile(dst);
    }
    try {
        if (fs::exists(dst) && fs::equivalent(src, dst)) {
            LogPrintf("cannot backup to wallet source file %s\n", fs::PathToString(dst));
            return false;
        }

        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
        LogPrintf("copied %s to %s\n", fs::PathToString(m_filepath), fs::PathToString(dst));
        return true;
    } catch (const fs::filesystem_error& e) {
        LogWarning("error copying %s to %s - %s\n", fs::PathToString(m_filepath), fs::PathToString(dst), e.code().message());
        return false;
    }
}

bool BerkeleyROBatch::ReadKey(DataStream&& key, DataStream& value)
{
    SerializeData key_data{key.begin(), key.end()};
    const auto it{m_database.m_records.find(key_data)};
    if (it == m_database.m_records.end()) {
        return false;
    }
    auto val = it->second;
    value.clear();
    value.write(std::span(val));
    return true;
}

bool BerkeleyROBatch::HasKey(DataStream&& key)
{
    SerializeData key_data{key.begin(), key.end()};
    return m_database.m_records.count(key_data) > 0;
}

BerkeleyROCursor::BerkeleyROCursor(const BerkeleyRODatabase& database, std::span<const std::byte> prefix)
    : m_database(database)
{
    std::tie(m_cursor, m_cursor_end) = m_database.m_records.equal_range(BytePrefix{prefix});
}

DatabaseCursor::Status BerkeleyROCursor::Next(DataStream& ssKey, DataStream& ssValue)
{
    if (m_cursor == m_cursor_end) {
        return DatabaseCursor::Status::DONE;
    }
    ssKey.write(std::span(m_cursor->first));
    ssValue.write(std::span(m_cursor->second));
    m_cursor++;
    return DatabaseCursor::Status::MORE;
}

std::unique_ptr<DatabaseCursor> BerkeleyROBatch::GetNewPrefixCursor(std::span<const std::byte> prefix)
{
    return std::make_unique<BerkeleyROCursor>(m_database, prefix);
}

std::unique_ptr<BerkeleyRODatabase> MakeBerkeleyRODatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    fs::path data_file = BDBDataFile(path);
    try {
        std::unique_ptr<BerkeleyRODatabase> db = std::make_unique<BerkeleyRODatabase>(data_file);
        status = DatabaseStatus::SUCCESS;
        return db;
    } catch (const std::runtime_error& e) {
        error.original = e.what();
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
}
} // namespace wallet
