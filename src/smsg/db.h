// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_SMSG_DB_H
#define PARTICL_SMSG_DB_H

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include <sync.h>
#include <smsg/keystore.h>

class CDataStream;

namespace smsg
{

class SecMsgStored;

extern CCriticalSection cs_smsgDB;
extern leveldb::DB *smsgDB;

class SecMsgDB
{
public:
    SecMsgDB()
    {
        activeBatch = nullptr;
    };

    ~SecMsgDB()
    {
        // Deletes only data scoped to this SecMsgDB object.
        if (activeBatch)
            delete activeBatch;
    };

    bool Open(const char *pszMode="r+");

    bool ScanBatch(const CDataStream &key, std::string *value, bool *deleted) const;

    bool TxnBegin();
    bool TxnCommit();
    bool TxnAbort();

    bool ReadPK(const CKeyID &addr, CPubKey &pubkey);
    bool WritePK(const CKeyID &addr, CPubKey &pubkey);
    bool ExistsPK(const CKeyID &addr);

    bool ReadKey(const CKeyID &idk, SecMsgKey &key);
    bool WriteKey(const CKeyID &idk, const SecMsgKey &key);

    bool NextSmesg(leveldb::Iterator *it, const std::string &prefix, uint8_t *vchKey, SecMsgStored &smsgStored);
    bool NextSmesgKey(leveldb::Iterator *it, const std::string &prefix, uint8_t *vchKey);
    bool ReadSmesg(const uint8_t *chKey, SecMsgStored &smsgStored);
    bool WriteSmesg(const uint8_t *chKey, SecMsgStored &smsgStored);
    bool ExistsSmesg(const uint8_t *chKey);
    bool EraseSmesg(const uint8_t *chKey);

    bool NextPrivKey(leveldb::Iterator *it, const std::string &prefix, CKeyID &idk, SecMsgKey &key);

    leveldb::DB *pdb;       // points to the global instance
    leveldb::WriteBatch *activeBatch;
};

} // namespace smsg

#endif // PARTICL_SMSG_DB_H
