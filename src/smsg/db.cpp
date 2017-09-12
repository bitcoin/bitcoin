// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "smsg/db.h"
#include "smsg/smessage.h"
#include "sync.h"
#include "serialize.h"
#include "clientversion.h"


CCriticalSection cs_smsgDB;
leveldb::DB *smsgDB = nullptr;

bool SecMsgDB::Open(const char *pszMode)
{
    if (smsgDB)
    {
        pdb = smsgDB;
        return true;
    };

    bool fCreate = strchr(pszMode, 'c');

    fs::path fullpath = GetDataDir() / "smsgdb";

    if (!fCreate
        && (!fs::exists(fullpath)
            || !fs::is_directory(fullpath)))
    {
        LogPrintf("%s: DB does not exist.\n", __func__);
        return false;
    };

    leveldb::Options options;
    options.create_if_missing = fCreate;
    leveldb::Status s = leveldb::DB::Open(options, fullpath.string(), &smsgDB);

    if (!s.ok())
    {
        LogPrintf("%s: Error opening db: %s.\n", __func__, s.ToString());
        return false;
    };

    pdb = smsgDB;

    return true;
};


class SecMsgBatchScanner : public leveldb::WriteBatch::Handler
{
public:
    std::string needle;
    bool *deleted;
    std::string* foundValue;
    bool foundEntry;

    SecMsgBatchScanner() : foundEntry(false) {}

    virtual void Put(const leveldb::Slice &key, const leveldb::Slice &value)
    {
        if (key.ToString() == needle)
        {
            foundEntry = true;
            *deleted = false;
            *foundValue = value.ToString();
        };
    };

    virtual void Delete(const leveldb::Slice &key)
    {
        if (key.ToString() == needle)
        {
            foundEntry = true;
            *deleted = true;
        };
    };
};

// When performing a read, if we have an active batch we need to check it first
// before reading from the database, as the rest of the code assumes that once
// a database transaction begins reads are consistent with it. It would be good
// to change that assumption in future and avoid the performance hit, though in
// practice it does not appear to be large.
bool SecMsgDB::ScanBatch(const CDataStream &key, std::string *value, bool *deleted) const
{
    if (!activeBatch)
        return false;

    *deleted = false;
    SecMsgBatchScanner scanner;
    scanner.needle = key.str();
    scanner.deleted = deleted;
    scanner.foundValue = value;
    leveldb::Status s = activeBatch->Iterate(&scanner);
    if (!s.ok())
        return error("SecMsgDB ScanBatch error: %s\n", s.ToString());

    return scanner.foundEntry;
}

bool SecMsgDB::TxnBegin()
{
    if (activeBatch)
        return true;
    activeBatch = new leveldb::WriteBatch();
    return true;
};

bool SecMsgDB::TxnCommit()
{
    if (!activeBatch)
        return false;

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status status = pdb->Write(writeOptions, activeBatch);
    delete activeBatch;
    activeBatch = nullptr;

    if (!status.ok())
        return error("SecMsgDB batch commit failure: %s\n", status.ToString());

    return true;
};

bool SecMsgDB::TxnAbort()
{
    delete activeBatch;
    activeBatch = nullptr;
    return true;
};

bool SecMsgDB::ReadPK(CKeyID &addr, CPubKey &pubkey)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.reserve(sizeof(addr) + 2);
    ssKey << 'p';
    ssKey << 'k';
    ssKey << addr;
    std::string strValue;

    bool readFromDb = true;
    if (activeBatch)
    {
        // Check activeBatch first
        bool deleted = false;
        readFromDb = ScanBatch(ssKey, &strValue, &deleted) == false;
        if (deleted)
            return false;
    };

    if (readFromDb)
    {
        leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &strValue);
        if (!s.ok())
        {
            if (s.IsNotFound())
                return false;
            return error("LevelDB read failure: %s\n", s.ToString());
        };
    };

    try {
        CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
        ssValue >> pubkey;
    } catch (std::exception &e) {
        LogPrintf("SecMsgDB::ReadPK() unserialize threw: %s.\n", e.what());
        return false;
    };

    return true;
};

bool SecMsgDB::WritePK(CKeyID &addr, CPubKey &pubkey)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.reserve(sizeof(addr) + 2);
    ssKey << 'p';
    ssKey << 'k';
    ssKey << addr;
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    ssValue.reserve(sizeof(pubkey));
    ssValue << pubkey;

    if (activeBatch)
    {
        activeBatch->Put(ssKey.str(), ssValue.str());
        return true;
    };

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Put(writeOptions, ssKey.str(), ssValue.str());
    if (!s.ok())
        return error("SecMsgDB write failure: %s\n", s.ToString());

    return true;
};

bool SecMsgDB::ExistsPK(CKeyID &addr)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.reserve(sizeof(addr)+2);
    ssKey << 'p';
    ssKey << 'k';
    ssKey << addr;
    std::string unused;

    if (activeBatch)
    {
        bool deleted;
        if (ScanBatch(ssKey, &unused, &deleted) && !deleted)
            return true;
    };

    leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &unused);
    return s.IsNotFound() == false;
};


bool SecMsgDB::NextSmesg(leveldb::Iterator *it, std::string &prefix, uint8_t *chKey, SecMsgStored &smsgStored)
{
    if (!pdb)
        return false;

    if (!it->Valid()) // first run
        it->Seek(prefix);
    else
        it->Next();

    if (!(it->Valid()
        && it->key().size() == 18
        && memcmp(it->key().data(), prefix.data(), 2) == 0))
        return false;

    memcpy(chKey, it->key().data(), 18);

    try {
        CDataStream ssValue(it->value().data(), it->value().data() + it->value().size(), SER_DISK, CLIENT_VERSION);
        ssValue >> smsgStored;
    } catch (std::exception &e) {
        LogPrintf("SecMsgDB::NextSmesg() unserialize threw: %s.\n", e.what());
        return false;
    };

    return true;
};

bool SecMsgDB::NextSmesgKey(leveldb::Iterator *it, std::string &prefix, uint8_t *chKey)
{
    if (!pdb)
        return false;

    if (!it->Valid()) // first run
        it->Seek(prefix);
    else
        it->Next();

    if (!(it->Valid()
        && it->key().size() == 18
        && memcmp(it->key().data(), prefix.data(), 2) == 0))
        return false;

    memcpy(chKey, it->key().data(), 18);

    return true;
};

bool SecMsgDB::ReadSmesg(uint8_t *chKey, SecMsgStored &smsgStored)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);
    std::string strValue;

    bool readFromDb = true;
    if (activeBatch)
    {
        // Check activeBatch first
        bool deleted = false;
        readFromDb = ScanBatch(ssKey, &strValue, &deleted) == false;
        if (deleted)
            return false;
    };

    if (readFromDb)
    {
        leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &strValue);
        if (!s.ok())
        {
            if (s.IsNotFound())
                return false;
            return error("LevelDB read failure: %s\n", s.ToString());
        };
    };

    try {
        CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, CLIENT_VERSION);
        ssValue >> smsgStored;
    } catch (std::exception &e) {
        LogPrintf("SecMsgDB::ReadSmesg() unserialize threw: %s.\n", e.what());
        return false;
    };

    return true;
};

bool SecMsgDB::WriteSmesg(uint8_t *chKey, SecMsgStored &smsgStored)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    ssValue << smsgStored;

    if (activeBatch)
    {
        activeBatch->Put(ssKey.str(), ssValue.str());
        return true;
    };

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Put(writeOptions, ssKey.str(), ssValue.str());
    if (!s.ok())
        return error("SecMsgDB write failed: %s\n", s.ToString());

    return true;
};

bool SecMsgDB::ExistsSmesg(uint8_t *chKey)
{
    if (!pdb)
        return false;

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);
    std::string unused;

    if (activeBatch)
    {
        bool deleted;
        if (ScanBatch(ssKey, &unused, &deleted) && !deleted)
            return true;
    };

    leveldb::Status s = pdb->Get(leveldb::ReadOptions(), ssKey.str(), &unused);
    return s.IsNotFound() == false;
    return true;
};

bool SecMsgDB::EraseSmesg(uint8_t *chKey)
{
    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey.write((const char*)chKey, 18);

    if (activeBatch)
    {
        activeBatch->Delete(ssKey.str());
        return true;
    };

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    leveldb::Status s = pdb->Delete(writeOptions, ssKey.str());

    if (s.ok() || s.IsNotFound())
        return true;
    return error("SecMsgDB erase failed: %s\n", s.ToString());
};

