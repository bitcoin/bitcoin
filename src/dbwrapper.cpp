// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>

#include <random.h>
#include <streams.h>

#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <leveldb/write_batch.h>
#include <memenv.h>

#include <algorithm>
#include <memory>
#include <stdint.h>

/** These should be considered an implementation detail of the specific database.
 */
namespace dbwrapper_private {

/** Handle database error by throwing dbwrapper_error exception.
 */
void HandleError(const leveldb::Status& status);
};

class DBBatchImpl
{
private:
    const CDBWrapper& parent;

    leveldb::WriteBatch batch;

    size_t size_estimate{0};
public:
    DBBatchImpl(const CDBWrapper& _parent);

    void Clear();

    void doWrite(CDataStream& key, CDataStream& value);
    void doErase(CDataStream& key);

    size_t SizeEstimate() const;

    leveldb::WriteBatch& GetWriteBatch();
};

CDBBatch::CDBBatch(const CDBWrapper& _parent)
    : m_impl(std::make_unique<DBBatchImpl>(_parent)) {}

DBBatchImpl::DBBatchImpl(const CDBWrapper& _parent)
    : parent(_parent) {}

CDBBatch::~CDBBatch() = default;

void CDBBatch::Clear()
{
    return m_impl->Clear();
}

void DBBatchImpl::Clear()
{
    batch.Clear();
    size_estimate = 0;
}

void CDBBatch::doWrite(CDataStream& key, CDataStream& value)
{
    m_impl->doWrite(key, value);
}

void DBBatchImpl::doWrite(CDataStream& key, CDataStream& value)
{
    leveldb::Slice slKey((const char*)key.data(), key.size());

    value.Xor(dbwrapper_private::GetObfuscateKey(parent));
    leveldb::Slice slValue((const char*)value.data(), value.size());

    batch.Put(slKey, slValue);

    // LevelDB serializes writes as:
    // - byte: header
    // - varint: key length (1 byte up to 127B, 2 bytes up to 16383B, ...)
    // - byte[]: key
    // - varint: value length
    // - byte[]: value
    // The formula below assumes the key and value are both less than 16k.
    size_estimate += 3 + (slKey.size() > 127) + slKey.size() + (slValue.size() > 127) + slValue.size();
}

void CDBBatch::doErase(CDataStream& key)
{
    m_impl->doErase(key);
}

void DBBatchImpl::doErase(CDataStream& key)
{
    leveldb::Slice slKey((const char*)key.data(), key.size());
    batch.Delete(slKey);

    // LevelDB serializes erases as:
    // - byte: header
    // - varint: key length
    // - byte[]: key
    // The formula below assumes the key is less than 16kB.
    size_estimate += 2 + (slKey.size() > 127) + slKey.size();
}

size_t CDBBatch::SizeEstimate() const {
    return m_impl->SizeEstimate();
}

size_t DBBatchImpl::SizeEstimate() const { return size_estimate; }

class DBIteratorImpl
{
private:
    const CDBWrapper& parent;
    std::unique_ptr<leveldb::Iterator> piter;

public:
    explicit DBIteratorImpl(const CDBWrapper& _parent, leveldb::Iterator* _piter);

    void doSeek(const CDataStream& key);
    CDataStream doGetKey();
    CDataStream doGetValue();

    unsigned int GetValueSize();

    bool Valid() const;
    void SeekToFirst();
    void Next();
};

leveldb::WriteBatch& DBBatchImpl::GetWriteBatch()
{
    return batch;
}

CDBIterator::CDBIterator(std::unique_ptr<DBIteratorImpl> impl)
    : m_impl(std::move(impl)) {};

DBIteratorImpl::DBIteratorImpl(const CDBWrapper& _parent, leveldb::Iterator* _piter)
    : parent(_parent), piter(_piter){};

CDBIterator::~CDBIterator() = default;

void CDBIterator::doSeek(const CDataStream& key)
{
    return m_impl->doSeek(key);
}

void DBIteratorImpl::doSeek(const CDataStream& key)
{
    leveldb::Slice slKey((const char*)key.data(), key.size());
    piter->Seek(slKey);
}

CDataStream CDBIterator::doGetKey()
{
    return m_impl->doGetKey();
}

CDataStream DBIteratorImpl::doGetKey()
{
    leveldb::Slice slKey = piter->key();
    return CDataStream{MakeByteSpan(slKey), SER_DISK, CLIENT_VERSION};
}

CDataStream CDBIterator::doGetValue()
{
    return m_impl->doGetValue();
}

CDataStream DBIteratorImpl::doGetValue()
{
    leveldb::Slice slValue = piter->value();
    CDataStream ssValue{MakeByteSpan(slValue), SER_DISK, CLIENT_VERSION};
    ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
    return ssValue;
}

unsigned int CDBIterator::GetValueSize()
{
    return m_impl->GetValueSize();
}

unsigned int DBIteratorImpl::GetValueSize()
{
    return piter->value().size();
}

bool CDBIterator::Valid() const { return m_impl->Valid(); }
bool DBIteratorImpl::Valid() const { return piter->Valid(); }

void CDBIterator::SeekToFirst() { m_impl->SeekToFirst(); }
void DBIteratorImpl::SeekToFirst() { piter->SeekToFirst(); }

void CDBIterator::Next() { m_impl->Next(); }
void DBIteratorImpl::Next() { piter->Next(); }

class CBitcoinLevelDBLogger : public leveldb::Logger
{
public:
    // This code is adapted from posix_logger.h, which is why it is using vsprintf.
    // Please do not do this in normal code
    void Logv(const char * format, va_list ap) override {
            if (!LogAcceptCategory(BCLog::LEVELDB)) {
                return;
            }
            char buffer[500];
            for (int iter = 0; iter < 2; iter++) {
                char* base;
                int bufsize;
                if (iter == 0) {
                    bufsize = sizeof(buffer);
                    base = buffer;
                }
                else {
                    bufsize = 30000;
                    base = new char[bufsize];
                }
                char* p = base;
                char* limit = base + bufsize;

                // Print the message
                if (p < limit) {
                    va_list backup_ap;
                    va_copy(backup_ap, ap);
                    // Do not use vsnprintf elsewhere in bitcoin source code, see above.
                    p += vsnprintf(p, limit - p, format, backup_ap);
                    va_end(backup_ap);
                }

                // Truncate to available space if necessary
                if (p >= limit) {
                    if (iter == 0) {
                        continue;       // Try again with larger buffer
                    }
                    else {
                        p = limit - 1;
                    }
                }

                // Add newline if necessary
                if (p == base || p[-1] != '\n') {
                    *p++ = '\n';
                }

                assert(p <= limit);
                base[std::min(bufsize - 1, (int)(p - base))] = '\0';
                LogPrintf("leveldb: %s", base);  /* Continued */
                if (base != buffer) {
                    delete[] base;
                }
                break;
            }
    }
};

static void SetMaxOpenFiles(leveldb::Options *options) {
    // On most platforms the default setting of max_open_files (which is 1000)
    // is optimal. On Windows using a large file count is OK because the handles
    // do not interfere with select() loops. On 64-bit Unix hosts this value is
    // also OK, because up to that amount LevelDB will use an mmap
    // implementation that does not use extra file descriptors (the fds are
    // closed after being mmap'ed).
    //
    // Increasing the value beyond the default is dangerous because LevelDB will
    // fall back to a non-mmap implementation when the file count is too large.
    // On 32-bit Unix host we should decrease the value because the handles use
    // up real fds, and we want to avoid fd exhaustion issues.
    //
    // See PR #12495 for further discussion.

    int default_open_files = options->max_open_files;
#ifndef WIN32
    if (sizeof(void*) < 8) {
        options->max_open_files = 64;
    }
#endif
    LogPrint(BCLog::LEVELDB, "LevelDB using max_open_files=%d (default=%d)\n",
             options->max_open_files, default_open_files);
}

static leveldb::Options GetOptions(size_t nCacheSize)
{
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.info_log = new CBitcoinLevelDBLogger();
    if (leveldb::kMajorVersion > 1 || (leveldb::kMajorVersion == 1 && leveldb::kMinorVersion >= 16)) {
        // LevelDB versions before 1.16 consider short writes to be corruption. Only trigger error
        // on corruption in later versions.
        options.paranoid_checks = true;
    }
    SetMaxOpenFiles(&options);
    return options;
}

class DBWrapperImpl
{
    friend const std::vector<unsigned char>& dbwrapper_private::GetObfuscateKey(const CDBWrapper &w);
private:
   CDBWrapper& m_parent;

    //! custom environment this database is using (may be nullptr in case of default environment)
    leveldb::Env* penv;

    //! database options used
    leveldb::Options options;

    //! options used when reading from the database
    leveldb::ReadOptions readoptions;

    //! options used when iterating over values of the database
    leveldb::ReadOptions iteroptions;

    //! options used when writing to the database
    leveldb::WriteOptions writeoptions;

    //! options used when sync writing to the database
    leveldb::WriteOptions syncoptions;

    //! the database itself
    leveldb::DB* pdb;

    //! the name of this database
    std::string m_name;

    //! a key used for optional XOR-obfuscation of the database
    std::vector<unsigned char> obfuscate_key;

    //! the key under which the obfuscation key is stored
    static const std::string OBFUSCATE_KEY_KEY;

    //! the length of the obfuscate key in number of bytes
    static const unsigned int OBFUSCATE_KEY_NUM_BYTES;

public:
    DBWrapperImpl(CDBWrapper& parent, const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate);
    ~DBWrapperImpl();

    bool WriteBatch(CDBBatch& batch, bool fSync);

    size_t DynamicMemoryUsage() const;

    std::unique_ptr<DBIteratorImpl> NewIterator();

    bool IsEmpty();

    /**
     * Returns a string (consisting of 8 random bytes) suitable for use as an
     * obfuscating XOR key.
     */
    std::vector<unsigned char> CreateObfuscateKey() const;

    // Removed from interface later when pimpl'd
    bool doRawGet(const CDataStream& ssKey, std::string& strValue) const;

    std::optional<CDataStream> doGet(const CDataStream& ssKey) const;

    bool doExists(const CDataStream& ssKey) const;

    size_t doEstimateSizes(const CDataStream& begin_key, const CDataStream& end_key) const;

    void doCompactRange(const CDataStream& begin_key, const CDataStream& end_key) const;
};

DBWrapperImpl::DBWrapperImpl(CDBWrapper& parent, const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate)
    : m_parent(parent),
      m_name{fs::PathToString(path.stem())}
{
    penv = nullptr;
    readoptions.verify_checksums = true;
    iteroptions.verify_checksums = true;
    iteroptions.fill_cache = false;
    syncoptions.sync = true;
    options = GetOptions(nCacheSize);
    options.create_if_missing = true;
    if (fMemory) {
        penv = leveldb::NewMemEnv(leveldb::Env::Default());
        options.env = penv;
    } else {
        if (fWipe) {
            LogPrintf("Wiping LevelDB in %s\n", fs::PathToString(path));
            leveldb::Status result = leveldb::DestroyDB(fs::PathToString(path), options);
            dbwrapper_private::HandleError(result);
        }
        TryCreateDirectories(path);
        LogPrintf("Opening LevelDB in %s\n", fs::PathToString(path));
    }
    // PathToString() return value is safe to pass to leveldb open function,
    // because on POSIX leveldb passes the byte string directly to ::open(), and
    // on Windows it converts from UTF-8 to UTF-16 before calling ::CreateFileW
    // (see env_posix.cc and env_windows.cc).
    leveldb::Status status = leveldb::DB::Open(options, fs::PathToString(path), &pdb);
    dbwrapper_private::HandleError(status);
    LogPrintf("Opened LevelDB successfully\n");

    if (gArgs.GetBoolArg("-forcecompactdb", false)) {
        LogPrintf("Starting database compaction of %s\n", fs::PathToString(path));
        pdb->CompactRange(nullptr, nullptr);
        LogPrintf("Finished database compaction of %s\n", fs::PathToString(path));
    }

    // The base-case obfuscation key, which is a noop.
    obfuscate_key = std::vector<unsigned char>(OBFUSCATE_KEY_NUM_BYTES, '\000');

    bool key_exists = m_parent.Read(OBFUSCATE_KEY_KEY, obfuscate_key);

    if (!key_exists && obfuscate && IsEmpty()) {
        // Initialize non-degenerate obfuscation if it won't upset
        // existing, non-obfuscated data.
        std::vector<unsigned char> new_key = CreateObfuscateKey();

        // Write `new_key` so we don't obfuscate the key with itself
        m_parent.Write(OBFUSCATE_KEY_KEY, new_key);
        obfuscate_key = new_key;

        LogPrintf("Wrote new obfuscate key for %s: %s\n", fs::PathToString(path), HexStr(obfuscate_key));
    }

    LogPrintf("Using obfuscation key for %s: %s\n", fs::PathToString(path), HexStr(obfuscate_key));
}

CDBWrapper::CDBWrapper(const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate)
    : m_impl(std::make_unique<DBWrapperImpl>(*this, path, nCacheSize, fMemory, fWipe, obfuscate))
{}

DBWrapperImpl::~DBWrapperImpl()
{
    delete pdb;
    pdb = nullptr;
    delete options.filter_policy;
    options.filter_policy = nullptr;
    delete options.info_log;
    options.info_log = nullptr;
    delete options.block_cache;
    options.block_cache = nullptr;
    delete penv;
    options.env = nullptr;
}

CDBWrapper::~CDBWrapper() = default;

bool DBWrapperImpl::doRawGet(const CDataStream& ssKey, std::string& strValue) const
{
    leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());

    leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
    if (!status.ok()) {
        if (status.IsNotFound())
            return false;
        LogPrintf("LevelDB read failure: %s\n", status.ToString());
        dbwrapper_private::HandleError(status);
    }

    return true;
}

std::optional<CDataStream> CDBWrapper::doGet(const CDataStream& ssKey) const
{
    return m_impl->doGet(ssKey);
}

std::optional<CDataStream> DBWrapperImpl::doGet(const CDataStream& ssKey) const
{
    std::string strValue;
    if (!doRawGet(ssKey, strValue)) {
        return std::nullopt;
    }

    try {
        CDataStream ssValue{MakeByteSpan(strValue), SER_DISK, CLIENT_VERSION};
        ssValue.Xor(obfuscate_key);
        return std::move(ssValue);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool CDBWrapper::doExists(const CDataStream& ssKey) const
{
    return m_impl->doExists(ssKey);
}

bool DBWrapperImpl::doExists(const CDataStream& ssKey) const
{
    std::string strValue;
    return doRawGet(ssKey, strValue);
}

size_t CDBWrapper::doEstimateSizes(const CDataStream& begin_key, const CDataStream& end_key) const
{
    return m_impl->doEstimateSizes(begin_key, end_key);
}

size_t DBWrapperImpl::doEstimateSizes(const CDataStream& begin_key, const CDataStream& end_key) const
{
    leveldb::Slice slKey1((const char*)begin_key.data(), begin_key.size());
    leveldb::Slice slKey2((const char*)end_key.data(), end_key.size());

    uint64_t size = 0;
    leveldb::Range range(slKey1, slKey2);
    pdb->GetApproximateSizes(&range, 1, &size);
    return size;
}

void CDBWrapper::doCompactRange(const CDataStream& begin_key, const CDataStream& end_key) const
{
    return m_impl->doCompactRange(begin_key, end_key);
}

void DBWrapperImpl::doCompactRange(const CDataStream& begin_key, const CDataStream& end_key) const
{
    leveldb::Slice slKey1((const char*)begin_key.data(), begin_key.size());
    leveldb::Slice slKey2((const char*)end_key.data(), end_key.size());

    pdb->CompactRange(&slKey1, &slKey2);
}

bool CDBWrapper::WriteBatch(CDBBatch& batch, bool fSync)
{
    return m_impl->WriteBatch(batch, fSync);
}

bool DBWrapperImpl::WriteBatch(CDBBatch& batch, bool fSync)
{
    const bool log_memory = LogAcceptCategory(BCLog::LEVELDB);
    double mem_before = 0;
    if (log_memory) {
        mem_before = DynamicMemoryUsage() / 1024.0 / 1024;
    }
    leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.m_impl->GetWriteBatch());
    dbwrapper_private::HandleError(status);
    if (log_memory) {
        double mem_after = DynamicMemoryUsage() / 1024.0 / 1024;
        LogPrint(BCLog::LEVELDB, "WriteBatch memory usage: db=%s, before=%.1fMiB, after=%.1fMiB\n",
                 m_name, mem_before, mem_after);
    }
    return true;
}

size_t CDBWrapper::DynamicMemoryUsage() const
{
    return m_impl->DynamicMemoryUsage();
}

size_t DBWrapperImpl::DynamicMemoryUsage() const
{
    std::string memory;
    std::optional<size_t> parsed;
    if (!pdb->GetProperty("leveldb.approximate-memory-usage", &memory) || !(parsed = ToIntegral<size_t>(memory))) {
        LogPrint(BCLog::LEVELDB, "Failed to get approximate-memory-usage property\n");
        return 0;
    }
    return parsed.value();
}

std::unique_ptr<CDBIterator> CDBWrapper::NewIterator()
{
    return std::make_unique<CDBIterator>(m_impl->NewIterator());
}

std::unique_ptr<DBIteratorImpl> DBWrapperImpl::NewIterator()
{
    return std::make_unique<DBIteratorImpl>(m_parent, pdb->NewIterator(iteroptions));
}

// Prefixed with null character to avoid collisions with other keys
//
// We must use a string constructor which specifies length so that we copy
// past the null-terminator.
const std::string DBWrapperImpl::OBFUSCATE_KEY_KEY("\000obfuscate_key", 14);

const unsigned int DBWrapperImpl::OBFUSCATE_KEY_NUM_BYTES = 8;

std::vector<unsigned char> DBWrapperImpl::CreateObfuscateKey() const
{
    std::vector<uint8_t> ret(OBFUSCATE_KEY_NUM_BYTES);
    GetRandBytes(ret.data(), OBFUSCATE_KEY_NUM_BYTES);
    return ret;
}

bool CDBWrapper::IsEmpty()
{
    return m_impl->IsEmpty();
}

bool DBWrapperImpl::IsEmpty()
{
    std::unique_ptr<DBIteratorImpl> it(NewIterator());
    it->SeekToFirst();
    return !(it->Valid());
}

namespace dbwrapper_private {

void HandleError(const leveldb::Status& status)
{
    if (status.ok())
        return;
    const std::string errmsg = "Fatal LevelDB error: " + status.ToString();
    LogPrintf("%s\n", errmsg);
    LogPrintf("You can use -debug=leveldb to get more complete diagnostic messages\n");
    throw dbwrapper_error(errmsg);
}

const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w)
{
    return w.m_impl->obfuscate_key;
}

} // namespace dbwrapper_private
