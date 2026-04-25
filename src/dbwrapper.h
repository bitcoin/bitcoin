// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DBWRAPPER_H
#define BITCOIN_DBWRAPPER_H

#include <attributes.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <util/byte_units.h>
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
static const size_t DBWRAPPER_MAX_FILE_SIZE{32_MiB};

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
            SpanReader ssKey{GetKeyImpl()};
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

    std::optional<std::string> ReadImpl(std::span<const std::byte> key) const;
    bool ExistsImpl(std::span<const std::byte> key) const;
    size_t EstimateSizeImpl(std::span<const std::byte> key1, std::span<const std::byte> key2) const;
    auto& DBContext() const LIFETIMEBOUND { return *Assert(m_db_context); }

public:
    CDBWrapper(const DBParams& params);
    ~CDBWrapper();

    CDBWrapper(const CDBWrapper&) = delete;
    CDBWrapper& operator=(const CDBWrapper&) = delete;

    struct ReadStatus {
        enum class Code {
            OK,                      //!< Key found and value deserialized successfully.
            NOT_FOUND,               //!< Key does not exist in the database.
            DESERIALIZATION_ERROR,   //!< Key exists but value could not be deserialized.
            DB_INTERNAL_ERROR,       //!< Unexpected internal DB error.
        };

        const Code status;
        const std::optional<std::string> op_error{std::nullopt};

        ReadStatus(const Code status) : status(status) {}
        ReadStatus(const Code status, const std::string& error) : status(status), op_error(error) {}
    };

    /**
     * Read and deserialize a value from the database, with explicit error discrimination.
     *
     * Unlike Read(), this method distinguishes between a missing key (NOT_FOUND), a
     * deserialization failure (DESERIALIZATION_ERROR), and an internal DB error
     * (DB_INTERNAL_ERROR) enabling callers to treat data corruption differently from
     * an absent entry.
     *
     * @note Callers are expected to provide well-formed keys; key serialization
     *       is the only operation that may throw.
     *
     * @param[in]  key    The key to look up.
     * @param[out] value  Populated with the deserialized value on ReadStatus::OK
     *                    and indeterminate on any other status.
     * @return ReadStatus::OK on success,
     *         ReadStatus::NOT_FOUND if the key does not exist,
     *         ReadStatus::DESERIALIZATION_ERROR if value deserialization fails,
     *         ReadStatus::DB_INTERNAL_ERROR if DB record read fails.
     */
    template <typename K, typename V>
    [[nodiscard]] ReadStatus TryRead(const K& key, V& value) const
    {
        DataStream ssKey{};
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        // Key serialization is the only operation that may throw.
        // Callers are expected to provide well-formed keys.
        ssKey << key;

        std::optional<std::string> strValue;
        try {
            strValue = ReadImpl(ssKey);
            if (!strValue) {
                return {ReadStatus::Code::NOT_FOUND};
            }
        } catch (const std::exception& e) {
            return {ReadStatus::Code::DB_INTERNAL_ERROR, e.what()};
        }

        try {
            std::span ssValue{MakeWritableByteSpan(*strValue)};
            m_obfuscation(ssValue);
            SpanReader{ssValue} >> value;
        } catch (const std::exception& e) {
            return {ReadStatus::Code::DESERIALIZATION_ERROR, e.what()};
        }

        return {ReadStatus::Code::OK};
    }

    /**
     * Wrapper around TryRead() that preserves the original Read() semantics:
     * returns true on success, false if the key is absent or deserialization
     * fails, and throws dbwrapper_error on an internal DB error.
     *
     * Prefer TryRead() when the caller needs to distinguish between a missing
     * key and a corrupt value.
     */
    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        auto read_status = TryRead(key, value);
        switch (read_status.status) {
            case ReadStatus::Code::OK: return true;
            case ReadStatus::Code::NOT_FOUND: return false;
            case ReadStatus::Code::DESERIALIZATION_ERROR: return false;
            case ReadStatus::Code::DB_INTERNAL_ERROR: throw dbwrapper_error(read_status.op_error.value_or("unknown database error"));
        } // no default case, so the compiler can warn about missing cases
        std::abort(); // unreachable
    }

    template <typename K, typename V>
    void Write(const K& key, const V& value, bool fSync = false)
    {
        CDBBatch batch(*this);
        batch.Write(key, value);
        WriteBatch(batch, fSync);
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
    void Erase(const K& key, bool fSync = false)
    {
        CDBBatch batch(*this);
        batch.Erase(key);
        WriteBatch(batch, fSync);
    }

    void WriteBatch(CDBBatch& batch, bool fSync = false);

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
