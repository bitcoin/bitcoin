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

#include <db/leveldbimpl.h>

static const size_t DBWRAPPER_PREALLOC_KEY_SIZE = 64;
static const size_t DBWRAPPER_PREALLOC_VALUE_SIZE = 1024;

static const char* LEVELDB_SELECTOR = "leveldb";

class CDBWrapper;
class CDBBatch;
class CDBIterator;

/** These should be considered an implementation detail of the specific database.
 */
namespace dbwrapper_private {

/** Work around circular dependency, as well as for testing in dbwrapper_tests.
 * Database obfuscation should be considered an implementation detail of the
 * specific database.
 */
const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w);
};

enum class DatabaseBackend {
    LevelDB,
};

static DatabaseBackend GetSelectedDBType()
{
    static std::string selectedDatabaseStr = gArgs.GetArg("-dbbackend", LEVELDB_SELECTOR);
    static const auto selectorFunc = [&]() {
        if (selectedDatabaseStr == LEVELDB_SELECTOR) {
            return DatabaseBackend::LevelDB;
        }
        throw std::runtime_error("Unrecognized database selector: " + selectedDatabaseStr);
    };
    static DatabaseBackend selectedDatabase = selectorFunc();
    return selectedDatabase;
}

class InternalDBFactories {
    static std::unique_ptr<IDB> MakeDBImpl(DatabaseBackend tp, const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe);
    static std::unique_ptr<IDBBatch> MakeDBBatchImpl(DatabaseBackend tp, CDBWrapper& parent);
    static std::unique_ptr<IDBIterator> MakeDBIteratorImpl(DatabaseBackend tp, CDBWrapper& parent);
    friend CDBWrapper;
    friend CDBBatch;
    friend CDBIterator;
};

class CDBWrapper
{
    friend const std::vector<unsigned char>& dbwrapper_private::GetObfuscateKey(const CDBWrapper &w);
private:
    std::unique_ptr<IDB> impl;

    //! a key used for optional XOR-obfuscation of the database
    std::vector<unsigned char> obfuscate_key;

    //! the key under which the obfuscation key is stored
    static const std::string OBFUSCATE_KEY_KEY;

    //! the length of the obfuscate key in number of bytes
    static const unsigned int OBFUSCATE_KEY_NUM_BYTES;

    std::vector<unsigned char> CreateObfuscateKey() const;

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
    virtual ~CDBWrapper();

    CDBWrapper(const CDBWrapper&) = delete;
    CDBWrapper& operator=(const CDBWrapper&) = delete;

    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        std::optional<CowBytes> result = impl->Read(IDB::Index::DB_MAIN_INDEX, DBByteSpan(ssKey.data(), ssKey.size()));
        if (!result) {
            return false;
        }

        try {
            CDataStream ssValue{result->GetSpan(), SER_DISK, CLIENT_VERSION};
            ssValue.Xor(obfuscate_key);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template <typename K, typename V>
    bool Write(const K& key, const V& value, bool fSync = false)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
        ssValue << value;
        ssValue.Xor(obfuscate_key);

        return impl->Write(IDB::Index::DB_MAIN_INDEX, DBByteSpan(ssKey.data(), ssKey.size()), DBByteSpan(ssValue.data(), ssValue.size()), fSync);
    }

    template <typename K>
    bool Exists(const K& key) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        return impl->Exists(IDB::Index::DB_MAIN_INDEX, Span(ssKey.data(), ssKey.size()));
    }

    template <typename K>
    bool Erase(const K& key, bool fSync = false)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        return impl->Erase(IDB::Index::DB_MAIN_INDEX, DBByteSpan(ssKey.data(), ssKey.size()), fSync);
    }

    bool WriteBatch(CDBBatch &batch, bool fSync = false);

    // Get an estimate of LevelDB memory usage (in bytes).
    size_t DynamicMemoryUsage() const;

    CDBIterator* NewIterator();

    /**
     * Return true if the database managed by this class contains no entries.
     */
    bool IsEmpty() {
        return impl->IsEmpty();
    }

    template<typename K>
    size_t EstimateSize(const K& key_begin, const K& key_end) const
    {
        CDataStream ssKey1(SER_DISK, CLIENT_VERSION), ssKey2(SER_DISK, CLIENT_VERSION);
        ssKey1.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey2.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey1 << key_begin;
        ssKey2 << key_end;
        return impl->EstimateSize(DBByteSpan(ssKey1.data(), ssKey1.size()), DBByteSpan(ssKey2.data(), ssKey2.size()));
    }

    IDB* GetRawDB();
};

///** Batch of changes queued to be written to a CDBWrapper */
class CDBBatch
{
    CDBWrapper &parent;

    CDataStream ssKey;
    CDataStream ssValue;

    std::unique_ptr<IDBBatch> batch_impl;

public:
    /**
         * @param[in] _parent   CDBWrapper that this batch is to be submitted to
         */
    explicit CDBBatch(CDBWrapper &_parent);

    void Clear();

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;

        ssValue.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
        ssValue << value;
        ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));

        batch_impl->Write(DBByteSpan(ssKey.data(), ssKey.size()), DBByteSpan(ssValue.data(), ssValue.size()));

        ssKey.clear();
        ssValue.clear();
    }

    template <typename K>
    void Erase(const K& key)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());

        batch_impl->Erase(DBByteSpan(ssKey.data(), ssKey.size()));

        ssKey.clear();
    }

    size_t SizeEstimate() const;

    bool FlushToParent(bool fSync);
};

class CDBIterator
{
private:
    CDBWrapper &parent;
    std::unique_ptr<IDBIterator> piter;

public:

    /**
         * @param[in] _parent          Parent CDBWrapper instance.
         */
    explicit CDBIterator(CDBWrapper &_parent) :
        parent(_parent), piter(InternalDBFactories::MakeDBIteratorImpl(GetSelectedDBType(), parent)) { };
    ~CDBIterator();

    bool Valid() const;

    void SeekToFirst();

    template<typename K> void Seek(const K& key) {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());
        piter->Seek(DBByteSpan(ssKey.data(), ssKey.size()));
    }

    void Next();

    template<typename K> bool GetKey(K& key) {
        std::optional<CowBytes> slKey = piter->GetKey();
        try {
            CDataStream ssKey{slKey->GetSpan(), SER_DISK, CLIENT_VERSION};
            ssKey >> key;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template<typename V> bool GetValue(V& value) {
        std::optional<CowBytes> slValue = piter->GetValue();
        try {
            CDataStream ssValue{slValue->GetSpan(), SER_DISK, CLIENT_VERSION};
            ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    uint32_t GetValueSize();
};

#endif // BITCOIN_DBWRAPPER_H
