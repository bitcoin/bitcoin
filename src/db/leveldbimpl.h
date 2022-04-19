#ifndef BITCOIN_DB_LEVELDBIMPL_H
#define BITCOIN_DB_LEVELDBIMPL_H

#include <db/idb.h>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

class CLevelDBImpl;

namespace dbwrapper_private {

/** Handle database error by throwing dbwrapper_error exception.
 */
void HandleError(const leveldb::Status& status);

};

class dbwrapper_error : public std::runtime_error
{
public:
    explicit dbwrapper_error(const std::string& msg) : std::runtime_error(msg) {}
};


class CLevelDBImpl : public IDB
{
    friend class CLevelDBBatch;
    friend class CLevelDBIterator;

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

    std::vector<unsigned char> CreateObfuscateKey() const;

public:
    explicit CLevelDBImpl(const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe);
    ~CLevelDBImpl();

    std::optional<CowBytes> Read(Index dbindex, const DBByteSpan &key, std::size_t offset = 0, const std::optional<std::size_t> &size = std::nullopt) const override;
    bool Write(Index dbindex, const DBByteSpan &key, const DBByteSpan &value, bool fSync = false) override;
    size_t DynamicMemoryUsage() const override;
    bool Erase(Index dbindex, const DBByteSpan &key, bool fSync) override;
    bool IsEmpty() const override;
    std::unique_ptr<IDBIterator> NewIterator() override;
    size_t EstimateSize(const DBByteSpan &key_begin, const DBByteSpan &key_end) const override;
    bool Exists(Index dbindex, const DBByteSpan &key) const override;
};

/** Batch of changes queued to be written to a CDBWrapper */
class CLevelDBBatch : public IDBBatch
{
    friend class CLevelDBImpl;

private:
    CLevelDBImpl &parent;
    leveldb::WriteBatch batch;

    size_t size_estimate;

public:
    /**
     * @param[in] _parent   CDBWrapper that this batch is to be submitted to
     */
    explicit CLevelDBBatch(CLevelDBImpl &_parent) : parent(_parent), size_estimate(0) { };
    virtual ~CLevelDBBatch() {}

    void Clear() override;
    void Write(const DBByteSpan& key, const DBByteSpan& value) override;
    void Erase(const DBByteSpan& key) override;
    bool FlushToParent(bool fSync) override;
    size_t SizeEstimate() const override;
};

class CLevelDBIterator : public IDBIterator
{
private:
    const CLevelDBImpl &parent;
    std::unique_ptr<leveldb::Iterator> piter;

public:

    /**
     * @param[in] _parent          Parent CLevelDBImpl instance.
     */
    explicit CLevelDBIterator(const CLevelDBImpl &_parent) : parent(_parent), piter(parent.pdb->NewIterator(parent.iteroptions)) { };
    virtual ~CLevelDBIterator();

    bool Valid() const override;
    void SeekToFirst() override;
    void Seek(const DBByteSpan& key) override;
    void Next() override;
    std::optional<CowBytes> GetKey() override;
    std::optional<CowBytes> GetValue() override;
    uint32_t GetValueSize() override;

};

#endif // BITCOIN_DB_LEVELDBIMPL_H
