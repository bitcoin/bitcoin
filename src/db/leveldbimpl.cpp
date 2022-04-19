#include <db/leveldbimpl.h>

#include <logging.h>
#include <util/strencodings.h>
#include <memenv.h>
#include <leveldb/cache.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <util/system.h>

class CBitcoinLevelDBLogger : public leveldb::Logger {
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

CLevelDBImpl::CLevelDBImpl(const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe)
    : m_name{fs::PathToString(path.stem())}
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
}

CLevelDBImpl::~CLevelDBImpl()
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

} // namespace dbwrapper_private

std::optional<CowBytes> CLevelDBImpl::Read(Index /*dbindex*/, const DBByteSpan &key, std::size_t offset, const std::optional<std::size_t> &size) const
{
    leveldb::Slice slKey((const char*)key.data(), key.size());

    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
    if (!status.ok()) {
        if (status.IsNotFound())
            return std::nullopt;
        LogPrintf("LevelDB read failure: %s\n", status.ToString());
        dbwrapper_private::HandleError(status);
    }
    if(size) {
        return std::make_optional(CowBytes(strValue.substr(offset, *size)));
    } else {
        return std::make_optional(CowBytes(strValue.substr(offset)));
    }
}

bool CLevelDBImpl::Write(Index /*dbindex*/, const DBByteSpan &key, const DBByteSpan &value, bool fSync)
{
    CLevelDBBatch batch(*this);
    batch.Write(key, value);
    return batch.FlushToParent(fSync);
}

size_t CLevelDBImpl::DynamicMemoryUsage() const
{
    std::string memory;
    std::optional<size_t> parsed;
    if (!pdb->GetProperty("leveldb.approximate-memory-usage", &memory) || !(parsed = ToIntegral<size_t>(memory))) {
        LogPrint(BCLog::LEVELDB, "Failed to get approximate-memory-usage property\n");
        return 0;
    }
    return parsed.value();
}

bool CLevelDBImpl::Erase(Index /*dbindex*/, const DBByteSpan &key, bool fSync)
{
    CLevelDBBatch batch(*this);
    batch.Erase(key);
    return batch.FlushToParent(fSync);
}

bool CLevelDBImpl::IsEmpty() const
{
    std::unique_ptr<IDBIterator> it(const_cast<CLevelDBImpl*>(this)->NewIterator());
    it->SeekToFirst();
    return !(it->Valid());
}

std::unique_ptr<IDBIterator> CLevelDBImpl::NewIterator()
{
    return std::make_unique<CLevelDBIterator>(*this);
}

size_t CLevelDBImpl::EstimateSize(const DBByteSpan &key_begin, const DBByteSpan &key_end) const
{
    leveldb::Slice slKey1((const char*)key_begin.data(), key_begin.size());
    leveldb::Slice slKey2((const char*)key_end.data(), key_end.size());
    uint64_t size = 0;
    leveldb::Range range(slKey1, slKey2);
    pdb->GetApproximateSizes(&range, 1, &size);
    return size;
}

bool CLevelDBImpl::Exists(Index /*dbindex*/, const DBByteSpan &key) const
{
    leveldb::Slice slKey((const char*)key.data(), key.size());

    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
    if (!status.ok()) {
        if (status.IsNotFound())
            return false;
        LogPrintf("LevelDB read failure: %s\n", status.ToString());
        dbwrapper_private::HandleError(status);
    }
    return true;
}

void CLevelDBBatch::Clear()
{
    batch.Clear();
    size_estimate = 0;
}

void CLevelDBBatch::Write(const DBByteSpan &key, const DBByteSpan &value)
{
    leveldb::Slice slKey((const char*)key.data(), key.size());
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

void CLevelDBBatch::Erase(const DBByteSpan &key)
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

bool CLevelDBBatch::FlushToParent(bool fSync)
{
    const bool log_memory = LogAcceptCategory(BCLog::LEVELDB);
    double mem_before = 0;
    if (log_memory) {
        mem_before = parent.DynamicMemoryUsage() / 1024.0 / 1024;
    }
    leveldb::Status status = parent.pdb->Write(fSync ? parent.syncoptions : parent.writeoptions, &this->batch);
    dbwrapper_private::HandleError(status);
    if (log_memory) {
        double mem_after = parent.DynamicMemoryUsage() / 1024.0 / 1024;
                    LogPrint(BCLog::LEVELDB, "WriteBatch memory usage: db=%s, before=%.1fMiB, after=%.1fMiB\n",
                             parent.m_name, mem_before, mem_after);
    }
    return true;
}

size_t CLevelDBBatch::SizeEstimate() const { return size_estimate; }



uint32_t CLevelDBIterator::GetValueSize() {
    return piter->value().size();
}

CLevelDBIterator::~CLevelDBIterator()
{
}

bool CLevelDBIterator::Valid() const
{
    return piter->Valid();
}

void CLevelDBIterator::SeekToFirst()
{
    piter->SeekToFirst();
}

void CLevelDBIterator::Seek(const DBByteSpan &key) {
    leveldb::Slice slKey((const char*)key.data(), key.size());
    piter->Seek(slKey);
}

void CLevelDBIterator::Next()
{
    return piter->Next();
}

std::optional<CowBytes> CLevelDBIterator::GetKey() {
    // TODO(Sam): Maybe check Valid() first? Let's see what other devs think about this
    leveldb::Slice slKey = piter->key();
    return std::make_optional(CowBytes(MakeByteSpan(slKey)));
}

std::optional<CowBytes> CLevelDBIterator::GetValue() {
    leveldb::Slice slValue = piter->value();
    return std::make_optional(CowBytes(MakeByteSpan(slValue)));
}
