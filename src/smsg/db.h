// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_SMSG_DB_H
#define PARTICL_SMSG_DB_H

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "sync.h"
#include "serialize.h"
#include "streams.h"

class SecMsgStored;
class CKeyID;
class CPubKey;

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

    bool ReadPK(CKeyID &addr, CPubKey &pubkey);
    bool WritePK(CKeyID &addr, CPubKey &pubkey);
    bool ExistsPK(CKeyID &addr);

    bool NextSmesg(leveldb::Iterator *it, std::string &prefix, uint8_t *vchKey, SecMsgStored &smsgStored);
    bool NextSmesgKey(leveldb::Iterator *it, std::string &prefix, uint8_t *vchKey);
    bool ReadSmesg(uint8_t *chKey, SecMsgStored &smsgStored);
    bool WriteSmesg(uint8_t *chKey, SecMsgStored &smsgStored);
    bool ExistsSmesg(uint8_t *chKey);
    bool EraseSmesg(uint8_t *chKey);

    leveldb::DB *pdb;       // points to the global instance
    leveldb::WriteBatch *activeBatch;
};

#endif // PARTICL_SMSG_DB_H
