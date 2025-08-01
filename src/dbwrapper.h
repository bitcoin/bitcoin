// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DBWRAPPER_H
#define BITCOIN_DBWRAPPER_H

#include <attributes.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <util/check.h>
#include <util/fs.h>

#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

static const size_t DBWRAPPER_PREALLOC_KEY_SIZE = 64;
static const size_t DBWRAPPER_PREALLOC_VALUE_SIZE = 1024;
static const size_t DBWRAPPER_MAX_FILE_SIZE = 32 << 20; // 32 MiB

//! User-controlled performance and debug options.
struct DBOptions {
    //! Compact database on startup.
    bool force_compact = false;
};

//! Application-specific storage settings.
struct DBParams {
    //! Location in the filesystem where leveldb data will be stored.
    fs::path path;
    //! Configures various leveldb cache settings.
    size_t cache_bytes;
    //! If true, use leveldb's memory environment.
    bool memory_only = false;
    //! If true, remove all existing data.
    bool wipe_data = false;
    //! If true, store data obfuscated via simple XOR. If false, XOR with a
    //! zero'd byte array.
    bool obfuscate = false;
    //! Passed-through options.
    DBOptions options{};
};

class dbwrapper_error : public std::runtime_error
{
public:
    explicit dbwrapper_error(const std::string& msg) : std::runtime_error(msg) {}
};

class CDBWrapper;

/** These should be considered an implementation detail of the specific database.
 */
namespace dbwrapper_private {

/** Work around circular dependency, as well as for testing in dbwrapper_tests.
 * Database obfuscation should be considered an implementation detail of the
 * specific database.
 */
const Obfuscation& GetObfuscation(const CDBWrapper&);
}; // namespace dbwrapper_private

bool DestroyDB(const std::string& path_str);

/** Batch of changes queued to be written to a CDBWrapper */
class CDBBatch
{
    friend class CDBWrapper;

private:
    const CDBWrapper &parent;

    struct WriteBatchImpl;
    const std::unique_ptr<WriteBatchImpl> m_impl_batch;

    DataStream ssKey{};
    DataStream ssValue{};

    void WriteImpl(std::span<const std::byte> key, DataStream& ssValue);
    void EraseImpl(std::span<const std::byte> key);

public:
    /**
     * @param[in] _parent   CDBWrapper that this batch is to be submitted to
     */
    explicit CDBBatch(const CDBWrapper& _parent);
    ~CDBBatch();
    void Clear();

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssValue.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
        ssKey << key;
        ssValue << value;
        WriteImpl(ssKey, ssValue);
        ssKey.clear();
        ssValue.clear();
    }

    template <typename K>
    void Erase(const K& key)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        EraseImpl(ssKey);
        ssKey.clear();
    }

    size_t ApproximateSize() const;
};

class CDBIterator
{
public:
    struct IteratorImpl;

private:
    const CDBWrapper &parent;
    const std::unique_ptr<IteratorImpl> m_impl_iter;

    void SeekImpl(std::span<const std::byte> key);
    std::span<const std::byte> GetKeyImpl() const;
    std::span<const std::byte> GetValueImpl() const;

public:

    /**
     * @param[in] _parent          Parent CDBWrapper instance.
     * @param[in] _piter           The original leveldb iterator.
     */
    CDBIterator(const CDBWrapper& _parent, std::unique_ptr<IteratorImpl> _piter);
    ~CDBIterator();

    bool Valid() const;

    void SeekToFirst();

    template<typename K> void Seek(const K& key) {
        DataStream ssKey{};
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        SeekImpl(ssKey);
    }

    void Next();

    template<typename K> bool GetKey(K& key) {
        try {
            DataStream ssKey{GetKeyImpl()};
            ssKey >> key;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template<typename V> bool GetValue(V& value) {
        try {
            DataStream ssValue{GetValueImpl()};
            dbwrapper_private::GetObfuscation(parent)(ssValue);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }
};

struct LevelDBContext;

class CDBWrapper
{
    friend const Obfuscation& dbwrapper_private::GetObfuscation(const CDBWrapper&);
private:
    //! holds all leveldb-specific fields of this class
    std::unique_ptr<LevelDBContext> m_db_context;

    //! the name of this database
    std::string m_name;

    //! optional XOR-obfuscation of the database
    Obfuscation m_obfuscation;

    //! obfuscation key storage key, null-prefixed to avoid collisions
    inline static const std::string OBFUSCATION_KEY{"\000obfuscate_key", 14}; // explicit size to avoid truncation at leading \0

    //! path to filesystem storage
    const fs::path m_path;

    //! whether or not the database resides in memory
    bool m_is_memory;

    std::optional<std::string> ReadImpl(std::span<const std::byte> key) const;
    bool ExistsImpl(std::span<const std::byte> key) const;
    size_t EstimateSizeImpl(std::span<const std::byte> key1, std::span<const std::byte> key2) const;
    auto& DBContext() const LIFETIMEBOUND { return *Assert(m_db_context); }

public:
    CDBWrapper(const DBParams& params);
    ~CDBWrapper();

    CDBWrapper(const CDBWrapper&) = delete;
    CDBWrapper& operator=(const CDBWrapper&) = delete;

    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        DataStream ssKey{};
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        std::optional<std::string> strValue{ReadImpl(ssKey)};
        if (!strValue) {
            return false;
        }
        try {
            DataStream ssValue{MakeByteSpan(*strValue)};
            m_obfuscation(ssValue);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template <typename K, typename V>
    bool Write(const K& key, const V& value, bool fSync = false)
    {
        CDBBatch batch(*this);
        batch.Write(key, value);
        return WriteBatch(batch, fSync);
    }

    //! @returns filesystem path to the on-disk data.
    std::optional<fs::path> StoragePath() {
        if (m_is_memory) {
            return {};
        }
        return m_path;
    }

    template <typename K>
    bool Exists(const K& key) const
    {
        DataStream ssKey{};
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        return ExistsImpl(ssKey);
    }

    template <typename K>
    bool Erase(const K& key, bool fSync = false)
    {
        CDBBatch batch(*this);
        batch.Erase(key);
        return WriteBatch(batch, fSync);
    }

    bool WriteBatch(CDBBatch& batch, bool fSync = false);

    // Get an estimate of LevelDB memory usage (in bytes).
    size_t DynamicMemoryUsage() const;

    CDBIterator* NewIterator();

    /**
     * Return true if the database managed by this class contains no entries.
     */
    bool IsEmpty();

    template<typename K>
    size_t EstimateSize(const K& key_begin, const K& key_end) const
    {
        DataStream ssKey1{}, ssKey2{};
        ssKey1.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey2.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey1 << key_begin;
        ssKey2 << key_end;
        return EstimateSizeImpl(ssKey1, ssKey2);
    }
};

#endif // BITCOIN_DBWRAPPER_H
