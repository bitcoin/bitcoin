// Copyright (c) 2012-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LEVELDBWRAPPER_H
#define BITCOIN_LEVELDBWRAPPER_H

#include "clientversion.h"
#include "serialize.h"
#include "streams.h"
#include "util.h"
#include "version.h"
#include "sync.h"

#include <typeindex>

#include <boost/filesystem/path.hpp>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

class leveldb_error : public std::runtime_error
{
public:
    leveldb_error(const std::string& msg) : std::runtime_error(msg) {}
};

void HandleError(const leveldb::Status& status); // throw(leveldb_error)

/** Batch of changes queued to be written to a CLevelDBWrapper */
class CLevelDBBatch
{
    friend class CLevelDBWrapper;

private:
    leveldb::WriteBatch batch;

public:
    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(ssValue.GetSerializeSize(value));
        ssValue << value;
        leveldb::Slice slValue(&ssValue[0], ssValue.size());

        batch.Put(slKey, slValue);
    }

    template <typename K>
    void Erase(const K& key)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        batch.Delete(slKey);
    }
};

class CLevelDBWrapper
{
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

public:
    CLevelDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    ~CLevelDBWrapper();

    template <typename K, typename V>
    bool Read(const K& key, V& value) const // throw(leveldb_error)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", status.ToString());
            HandleError(status);
        }
        try {
            CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    template <typename K, typename V>
    bool Write(const K& key, const V& value, bool fSync = false) // throw(leveldb_error)
    {
        CLevelDBBatch batch;
        batch.Write(key, value);
        return WriteBatch(batch, fSync);
    }

    template <typename K>
    bool Exists(const K& key) const // throw(leveldb_error)
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(ssKey.GetSerializeSize(key));
        ssKey << key;
        leveldb::Slice slKey(&ssKey[0], ssKey.size());

        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", status.ToString());
            HandleError(status);
        }
        return true;
    }

    template <typename K>
    bool Erase(const K& key, bool fSync = false) // throw(leveldb_error)
    {
        CLevelDBBatch batch;
        batch.Erase(key);
        return WriteBatch(batch, fSync);
    }

    bool WriteBatch(CLevelDBBatch& batch, bool fSync = false); // throw(leveldb_error)

    // not available for LevelDB; provide for compatibility with BDB
    bool Flush()
    {
        return true;
    }

    bool Sync() // throw(leveldb_error)
    {
        CLevelDBBatch batch;
        return WriteBatch(batch, true);
    }

    // not exactly clean encapsulation, but it's easiest for now
    leveldb::Iterator* NewIterator()
    {
        return pdb->NewIterator(iteroptions);
    }
};

class CDBTransaction {
private:
    CLevelDBWrapper &db;

    struct KeyHolder {
        virtual ~KeyHolder() = default;
        virtual bool Less(const KeyHolder &b) const = 0;
        virtual void Erase(CLevelDBBatch &batch) = 0;
    };
    typedef std::unique_ptr<KeyHolder> KeyHolderPtr;

    template <typename K>
    struct KeyHolderImpl : KeyHolder {
        KeyHolderImpl(const K &_key)
                : key(_key) {
        }
        virtual bool Less(const KeyHolder &b) const {
            auto *b2 = dynamic_cast<const KeyHolderImpl<K>*>(&b);
            assert(b2 != nullptr);
            return key < b2->key;
        }
        virtual void Erase(CLevelDBBatch &batch) {
            batch.Erase(key);
        }
        K key;
    };

    struct KeyValueHolder {
        virtual ~KeyValueHolder() = default;
        virtual void Write(CLevelDBBatch &batch) = 0;
    };
    typedef std::unique_ptr<KeyValueHolder> KeyValueHolderPtr;

    template <typename K, typename V>
    struct KeyValueHolderImpl : KeyValueHolder {
        KeyValueHolderImpl(const KeyHolderImpl<K> &_key, const V &_value)
                : key(_key),
                  value(_value) { }
        virtual void Write(CLevelDBBatch &batch) {
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
    CDBTransaction(CLevelDBWrapper &_db) : db(_db) {}

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
            auto it = ws->find(k);
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
        CLevelDBBatch batch;
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

class TransactionLevelDBWrapper
{
public:
    TransactionLevelDBWrapper(const std::string & dbName, size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    std::unique_ptr<CScopedDBTransaction> BeginTransaction()
    {
        LOCK(m_cs);
        auto t = CScopedDBTransaction::Begin(m_dbTransaction);
        return t;
    }

    template<typename K, typename V>
    bool Read(const K& key, V& value)
    {
        LOCK(m_cs);
        return m_dbTransaction.Read(key, value);
    }


    template<typename K, typename V>
    void Write(const K& key, const V& value)
    {
        LOCK(m_cs);
        m_dbTransaction.Write(key, value);
    }

    template <typename K>
    bool Exists(const K& key)
    {
        LOCK(m_cs);
        return m_dbTransaction.Exists(key);
    }

    template <typename K>
    void Erase(const K& key)
    {
        LOCK(m_cs);
        m_dbTransaction.Erase(key);
    }

    CLevelDBWrapper& GetRawDB() { return m_db; }

protected:
    CCriticalSection m_cs;
    CLevelDBWrapper m_db;
    CDBTransaction m_dbTransaction;
};

#endif // BITCOIN_LEVELDBWRAPPER_H
