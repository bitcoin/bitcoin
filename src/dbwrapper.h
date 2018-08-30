// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DBWRAPPER_H
#define BITCOIN_DBWRAPPER_H

#include "clientversion.h"
#include "serialize.h"
#include "streams.h"
#include "util.h"
#include "utilstrencodings.h"
#include "version.h"

#include <typeindex>

#include <boost/filesystem/path.hpp>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

static const size_t DBWRAPPER_PREALLOC_KEY_SIZE = 64;
static const size_t DBWRAPPER_PREALLOC_VALUE_SIZE = 1024;

class dbwrapper_error : public std::runtime_error
{
public:
    dbwrapper_error(const std::string& msg) : std::runtime_error(msg) {}
};

class CDBWrapper;

/** These should be considered an implementation detail of the specific database.
 */
namespace dbwrapper_private {

/** Handle database error by throwing dbwrapper_error exception.
 */
void HandleError(const leveldb::Status& status);

/** Work around circular dependency, as well as for testing in dbwrapper_tests.
 * Database obfuscation should be considered an implementation detail of the
 * specific database.
 */
const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w);

};

/** Batch of changes queued to be written to a CDBWrapper */
class CDBBatch
{
    friend class CDBWrapper;

private:
    const CDBWrapper &parent;
    leveldb::WriteBatch batch;

    CDataStream ssKey;
    CDataStream ssValue;

    size_t size_estimate;

public:
    /**
     * @param[in] parent    CDBWrapper that this batch is to be submitted to
     */
    CDBBatch(const CDBWrapper &_parent) : parent(_parent), ssKey(SER_DISK, CLIENT_VERSION), ssValue(SER_DISK, CLIENT_VERSION), size_estimate(0) { };

    void Clear()
    {
        batch.Clear();
        size_estimate = 0;
    }

    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey(ssKey.data(), ssKey.size());

        ssValue.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
        ssValue << value;
        ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
        leveldb::Slice slValue(ssValue.data(), ssValue.size());

        batch.Put(slKey, slValue);
        // - varint: key length (1 byte up to 127B, 2 bytes up to 16383B, ...)
        // - byte[]: key
        // - varint: value length
        // - byte[]: value
        // The formula below assumes the key and value are both less than 16k.
        size_estimate += 3 + (slKey.size() > 127) + slKey.size() + (slValue.size() > 127) + slValue.size();
        ssKey.clear();
        ssValue.clear();
    }

    template <typename K>
    void Erase(const K& key)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey(ssKey.data(), ssKey.size());

        batch.Delete(slKey);
        // - byte: header
        // - varint: key length
        // - byte[]: key
        // The formula below assumes the key is less than 16kB.
        size_estimate += 2 + (slKey.size() > 127) + slKey.size();
        ssKey.clear();
    }

    size_t SizeEstimate() const { return size_estimate; }
};

class CDBIterator
{
private:
    const CDBWrapper &parent;
    leveldb::Iterator *piter;

public:

    /**
     * @param[in] _parent          Parent CDBWrapper instance.
     * @param[in] _piter           The original leveldb iterator.
     */
    CDBIterator(const CDBWrapper &_parent, leveldb::Iterator *_piter) :
        parent(_parent), piter(_piter) { };
    ~CDBIterator();

    bool Valid();

    void SeekToFirst();

    template<typename K> void Seek(const K& key) {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey(ssKey.data(), ssKey.size());
        piter->Seek(slKey);
    }

    void Next();

    template<typename K> bool GetKey(K& key) {
        leveldb::Slice slKey = piter->key();
        try {
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            ssKey >> key;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    unsigned int GetKeySize() {
        return piter->key().size();
    }

    template<typename V> bool GetValue(V& value) {
        leveldb::Slice slValue = piter->value();
        try {
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    unsigned int GetValueSize() {
        return piter->value().size();
    }

};

class CDBWrapper
{
    friend const std::vector<unsigned char>& dbwrapper_private::GetObfuscateKey(const CDBWrapper &w);
private:
    //! custom environment this database is using (may be NULL in case of default environment)
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
    CDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory = false, bool fWipe = false, bool obfuscate = false);
    ~CDBWrapper();

    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey(ssKey.data(), ssKey.size());

        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", status.ToString());
            dbwrapper_private::HandleError(status);
        }
        try {
            CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
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
        leveldb::Slice slKey(ssKey.data(), ssKey.size());

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

    template <typename K>
    bool Erase(const K& key, bool fSync = false)
    {
        CDBBatch batch(*this);
        batch.Erase(key);
        return WriteBatch(batch, fSync);
    }

    bool WriteBatch(CDBBatch& batch, bool fSync = false);

    // not available for LevelDB; provide for compatibility with BDB
    bool Flush()
    {
        return true;
    }

    bool Sync()
    {
        CDBBatch batch(*this);
        return WriteBatch(batch, true);
    }

    CDBIterator *NewIterator()
    {
        return new CDBIterator(*this, pdb->NewIterator(iteroptions));
    }

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
        leveldb::Slice slKey1(ssKey1.data(), ssKey1.size());
        leveldb::Slice slKey2(ssKey2.data(), ssKey2.size());
        uint64_t size = 0;
        leveldb::Range range(slKey1, slKey2);
        pdb->GetApproximateSizes(&range, 1, &size);
        return size;
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
        leveldb::Slice slKey1(ssKey1.data(), ssKey1.size());
        leveldb::Slice slKey2(ssKey2.data(), ssKey2.size());
        pdb->CompactRange(&slKey1, &slKey2);
    }

};

class CDBTransaction {
private:
    CDBWrapper &db;

    struct KeyHolder {
        virtual ~KeyHolder() = default;
        virtual bool Less(const KeyHolder &b) const = 0;
        virtual void Erase(CDBBatch &batch) = 0;
    };
    typedef std::unique_ptr<KeyHolder> KeyHolderPtr;

    template <typename K>
    struct KeyHolderImpl : KeyHolder {
        KeyHolderImpl(const K &_key)
                : key(_key) {
        }
        virtual bool Less(const KeyHolder &b) const {
            auto *b2 = dynamic_cast<const KeyHolderImpl<K>*>(&b);
            return key < b2->key;
        }
        virtual void Erase(CDBBatch &batch) {
            batch.Erase(key);
        }
        K key;
    };

    struct KeyValueHolder {
        virtual void Write(CDBBatch &batch) = 0;
    };
    typedef std::unique_ptr<KeyValueHolder> KeyValueHolderPtr;

    template <typename K, typename V>
    struct KeyValueHolderImpl : KeyValueHolder {
        KeyValueHolderImpl(const KeyHolderImpl<K> &_key, const V &_value)
                : key(_key),
                  value(_value) { }
        virtual void Write(CDBBatch &batch) {
            batch.Write(key.key, value);
        }
        const KeyHolderImpl<K> &key;
        V value;
    };

    struct keyCmp {
        bool operator()(const KeyHolderPtr &a, const KeyHolderPtr &b) const {
            return a->Less(*b);
        }
    };

    typedef std::map<KeyHolderPtr, KeyValueHolderPtr, keyCmp> KeyValueMap;
    typedef std::map<std::type_index, KeyValueMap> TypeKeyValueMap;

    TypeKeyValueMap writes;
    TypeKeyValueMap deletes;

    template <typename K>
    KeyValueMap *getMapForType(TypeKeyValueMap &m, bool create) {
        auto it = m.find(typeid(K));
        if (it != m.end()) {
            return &it->second;
        }
        if (!create)
            return nullptr;
        auto it2 = m.emplace(typeid(K), KeyValueMap());
        return &it2.first->second;
    }

    template <typename K>
    KeyValueMap *getWritesMap(bool create) {
        return getMapForType<K>(writes, create);
    }

    template <typename K>
    KeyValueMap *getDeletesMap(bool create) {
        return getMapForType<K>(deletes, create);
    }

public:
    CDBTransaction(CDBWrapper &_db) : db(_db) {}

    template <typename K, typename V>
    void Write(const K& key, const V& value) {
        KeyHolderPtr k(new KeyHolderImpl<K>(key));
        KeyHolderImpl<K>* k2 = dynamic_cast<KeyHolderImpl<K>*>(k.get());
        KeyValueHolderPtr kv(new KeyValueHolderImpl<K,V>(*k2, value));

        KeyValueMap *ds = getDeletesMap<K>(false);
        if (ds)
            ds->erase(k);

        KeyValueMap *ws = getWritesMap<K>(true);
        ws->erase(k);
        ws->emplace(std::make_pair(std::move(k), std::move(kv)));
    }

    template <typename K, typename V>
    bool Read(const K& key, V& value) {
        KeyHolderPtr k(new KeyHolderImpl<K>(key));

        KeyValueMap *ds = getDeletesMap<K>(false);
        if (ds && ds->count(k))
            return false;

        KeyValueMap *ws = getWritesMap<K>(false);
        if (ws) {
            KeyValueMap::iterator it = ws->find(k);
            if (it != ws->end()) {
                auto *impl = dynamic_cast<KeyValueHolderImpl<K, V> *>(it->second.get());
                if (!impl)
                    return false;
                value = impl->value;
                return true;
            }
        }

        return db.Read(key, value);
    }

    template <typename K>
    bool Exists(const K& key) {
        KeyHolderPtr k(new KeyHolderImpl<K>(key));

        KeyValueMap *ds = getDeletesMap<K>(false);
        if (ds && ds->count(k))
            return false;

        KeyValueMap *ws = getWritesMap<K>(false);
        if (ws && ws->count(k))
            return true;

        return db.Exists(key);
    }

    template <typename K>
    void Erase(const K& key) {
        KeyHolderPtr k(new KeyHolderImpl<K>(key));

        KeyValueMap *ws = getWritesMap<K>(false);
        if (ws)
            ws->erase(k);
        KeyValueMap *ds = getDeletesMap<K>(true);
        ds->emplace(std::move(k), nullptr);
    }

    void Clear() {
        writes.clear();
        deletes.clear();
    }

    bool Commit() {
        CDBBatch batch(db);
        for (auto &p : deletes) {
            for (auto &p2 : p.second) {
                p2.first->Erase(batch);
            }
        }
        for (auto &p : writes) {
            for (auto &p2 : p.second) {
                p2.second->Write(batch);
            }
        }
        bool ret = db.WriteBatch(batch, true);
        Clear();
        return ret;
    }

    bool IsClean() {
        return writes.empty() && deletes.empty();
    }
};

class CScopedDBTransaction {
private:
    CDBTransaction &dbTransaction;
    std::function<void ()> commitHandler;
    std::function<void ()> rollbackHandler;
    bool didCommitOrRollback{};

public:
    CScopedDBTransaction(CDBTransaction &dbTx) : dbTransaction(dbTx) {}
    ~CScopedDBTransaction() {
        if (!didCommitOrRollback)
            Rollback();
    }
    bool Commit() {
        assert(!didCommitOrRollback);
        didCommitOrRollback = true;
        bool result = dbTransaction.Commit();
        if (commitHandler)
            commitHandler();
        return result;
    }
    void Rollback() {
        assert(!didCommitOrRollback);
        didCommitOrRollback = true;
        dbTransaction.Clear();
        if (rollbackHandler)
            rollbackHandler();
    }

    static std::unique_ptr<CScopedDBTransaction> Begin(CDBTransaction &dbTx) {
        assert(dbTx.IsClean());
        return std::unique_ptr<CScopedDBTransaction>(new CScopedDBTransaction(dbTx));
    }

    void SetCommitHandler(const std::function<void ()> &h) {
        commitHandler = h;
    }
    void SetRollbackHandler(const std::function<void ()> &h) {
        rollbackHandler = h;
    }
};

#endif // BITCOIN_DBWRAPPER_H
