#ifndef BITCOIN_DB_IDB_H
#define BITCOIN_DB_IDB_H

#include <cowbytes.h>
#include <fs.h>
#include <map>
#include <optional>
#include <vector>

using DBByteSpan = Span<const std::byte>;

class IDBBatch
{
public:
    /**
     * @brief Clear cleans all the contents of the batch and resets the size estimates, if any
     */
    virtual void Clear() = 0;

    /**
     * @brief Write data to the batch; the operation has no effect on the database until flushed
     */
    virtual void Write(const DBByteSpan& key, const DBByteSpan& value) = 0;

    /**
     * @brief Erase data from the database with a given key; the operation has no effect on the database until flushed
     */
    virtual void Erase(const DBByteSpan& key) = 0;

    /**
     * @return an estimate of the size of the data held in the batch
     */
    virtual size_t SizeEstimate() const = 0;

    /**
     * @brief FlushToParent apply the changes of the batch to the database
     */
    virtual bool FlushToParent(bool fSync) = 0;

    virtual ~IDBBatch() {}
};

class IDBIterator
{
public:
    /**
     * @return true if the iterator points to a value in the database, false otherwise
     */
    virtual bool Valid() const = 0;

    /**
     * @brief SeekToFirst moves the iterator pointer to the very first value in the key/value store/database
     */
    virtual void SeekToFirst() = 0;

    /**
     * @brief Seek moves the iterator pointer to the first entry with prefix `key`
     */
    virtual void Seek(const DBByteSpan& key) = 0;

    /**
     * @brief Next moves the iterator to the next key/value pair, assuming the iterator is valid
     */
    virtual void Next() = 0;

    /**
     * @return the key under the current iterator position, if the iterator is valid, otherwise std::nullopt
     */
    virtual std::optional<CowBytes> GetKey() = 0;

    /**
     * @return the value under the current iterator position, if the iterator is valid, otherwise std::nullopt
     */
    virtual std::optional<CowBytes> GetValue() = 0;

    /**
     * @return the size of the value under the current iterator position
     */
    virtual uint32_t GetValueSize() = 0;

    virtual ~IDBIterator() {}
};

class IDB
{
public:
    enum class Index : int {
        DB_MAIN_INDEX = 0,
    };

    /**
     * @return the approximate memory usage of the database; 0 can be either an error or that feature is not supported
     */
    virtual size_t DynamicMemoryUsage() const = 0;

    /**
     * @return Copy-on-write bytes of the value read from the database if reading the value is a success, otherwise std::nullopt
     * Under a database operation batch/transaction, the unowned data is valid only until the transaction is over
     */
    virtual std::optional<CowBytes>
    Read(IDB::Index dbindex, const DBByteSpan& key, std::size_t offset = 0,
         const std::optional<std::size_t>& size = std::nullopt) const = 0;

    /**
     * @return true if writing the key/value pair to the database is a success, false otherwise
     */
    virtual bool Write(IDB::Index dbindex, const DBByteSpan& key, const DBByteSpan& value, bool fSync = false) = 0;

    /**
     * @return true if the key doesn't exist or was deleted, false otherwiseCowBytes (db access failed, etc)
     */
    virtual bool Erase(IDB::Index dbindex, const DBByteSpan& key, bool fSync) = 0;

    /**
     * @return true if the key exists in the database, false otherwise
     */
    virtual bool Exists(IDB::Index dbindex, const DBByteSpan& key) const = 0;

    /**
     * @return true if the database has zero key/value pairs
     */
    virtual bool IsEmpty() const = 0;

    /**
     * @return the estimated size of the keys in the range [key_begin, key_end), a left-closed interval
     */
    virtual size_t EstimateSize(const DBByteSpan& key_begin, const DBByteSpan& key_end) const = 0;

    /**
     * @return a new uninitialized iterator; must use Seek() or similar methods to initialize it
     */
    virtual std::unique_ptr<IDBIterator> NewIterator() = 0;

    virtual ~IDB() {}
};

#endif // BITCOIN_DB_IDB_H
