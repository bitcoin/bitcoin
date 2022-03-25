// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DBWRAPPER_H
#define BITCOIN_DBWRAPPER_H

#include <clientversion.h>
#include <fs.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <util/strencodings.h>
#include <util/system.h>

static const size_t DBWRAPPER_PREALLOC_KEY_SIZE = 64;
static const size_t DBWRAPPER_PREALLOC_VALUE_SIZE = 1024;

class dbwrapper_error : public std::runtime_error
{
public:
    explicit dbwrapper_error(const std::string& msg) : std::runtime_error(msg) {}
};

class CDBWrapper;
class DBWrapperImpl;

/** These should be considered an implementation detail of the specific database.
 */
namespace dbwrapper_private {

/** Work around circular dependency, as well as for testing in dbwrapper_tests.
 * Database obfuscation should be considered an implementation detail of the
 * specific database.
 */
const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w);

};

class DBBatchImpl;

/** Batch of changes queued to be written to a CDBWrapper */
class CDBBatch
{
    friend class DBWrapperImpl;

protected:
    const std::unique_ptr<DBBatchImpl> m_impl;
private:
    void doWrite(CDataStream& key, CDataStream& value);
    void doErase(CDataStream& key);

public:
    /**
     * @param[in] _parent   CDBWrapper that this batch is to be submitted to
     */
    explicit CDBBatch(const CDBWrapper &_parent);
    ~CDBBatch();

    void Clear();

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
        ssValue << value;

        doWrite(ssKey, ssValue);
    }

    template <typename K>
    void Erase(const K& key)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        doErase(ssKey);
    }

    size_t SizeEstimate() const;
};

class DBIteratorImpl;

class CDBIterator
{
protected:
    const std::unique_ptr<DBIteratorImpl> m_impl;
private:
    void doSeek(const CDataStream& key);
    CDataStream doGetKey();
    CDataStream doGetValue();

public:
    /**
     * @param[in] impl          DBIteratorImpl instance.
     */
    CDBIterator(std::unique_ptr<DBIteratorImpl> impl);
    ~CDBIterator();

    bool Valid() const;

    void SeekToFirst();

    template<typename K> void Seek(const K& key) {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        doSeek(ssKey);
    }

    void Next();

    template<typename K> bool GetKey(K& key) {
        try {
            doGetKey() >> key;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template<typename V> bool GetValue(V& value) {
        try {
            doGetValue() >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    unsigned int GetValueSize();
};

class CDBWrapper
{
    friend const std::vector<unsigned char>& dbwrapper_private::GetObfuscateKey(const CDBWrapper &w);
private:
    std::optional<CDataStream> doGet(const CDataStream& ssKey) const;

    bool doExists(const CDataStream& ssKey) const;

    size_t doEstimateSizes(const CDataStream& begin_key, const CDataStream& end_key) const;

    void doCompactRange(const CDataStream& begin_key, const CDataStream& end_key) const;
protected:
    const std::unique_ptr<DBWrapperImpl> m_impl;
public:
    /**
     * @param[in] path        Location in the filesystem where leveldb data will be stored.
     * @param[in] nCacheSize  Configures various leveldb cache settings.
     * @param[in] fMemory     If true, use leveldb's memory environment.
     * @param[in] fWipe       If true, remove all existing data.
     * @param[in] obfuscate   If true, store data obfuscated via simple XOR. If false, XOR
     *                        with a zero'd byte array.
     */
    CDBWrapper(const fs::path& path, size_t nCacheSize, bool fMemory = false, bool fWipe = false, bool obfuscate = false);
    ~CDBWrapper();

    CDBWrapper(const CDBWrapper&) = delete;
    CDBWrapper& operator=(const CDBWrapper&) = delete;

    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        std::string strValue;

        auto maybe_data = doGet(ssKey);
        if (!maybe_data) {
            return false;
        }

        try {
            *maybe_data >> value;
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

    template <typename K>
    bool Exists(const K& key) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        return doExists(ssKey);
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

    std::unique_ptr<CDBIterator> NewIterator();

    /**
     * Return true if the database managed by this class contains no entries.
     */
    bool IsEmpty();

    template<typename K>
    size_t EstimateSize(const K& key_begin, const K& key_end) const
    {
        CDataStream ssKey1(SER_DISK, CLIENT_VERSION), ssKey2(SER_DISK, CLIENT_VERSION);
        ssKey1.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey2.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey1 << key_begin;
        ssKey2 << key_end;

        return doEstimateSizes(ssKey1, ssKey2);
     }

    /**
     * Compact a certain range of keys in the database.
     */
    template<typename K>
    void CompactRange(const K& key_begin, const K& key_end) const
    {
        CDataStream ssKey1(SER_DISK, CLIENT_VERSION), ssKey2(SER_DISK, CLIENT_VERSION);
        ssKey1.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey2.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey1 << key_begin;
        ssKey2 << key_end;

        return doCompactRange(ssKey1, ssKey2);
    }
};

#endif // BITCOIN_DBWRAPPER_H
