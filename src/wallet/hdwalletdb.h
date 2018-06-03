// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_WALLET_HDWALLETDB_H
#define PARTICL_WALLET_HDWALLETDB_H

#include <amount.h>
#include <primitives/transaction.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#include <key.h>
#include <key/types.h>
#include <key/stealth.h>
#include <key/extkey.h>

#include <list>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

/*
prefixes
    name

    abe                 - address book entry
    acc
    acentry

    aki                 - anon key image: CPubKey - COutpoint

    bestblock
    bestblockheader

    ckey
    cscript

    defaultkey

    eacc                - extended account
    ecpk                - extended account stealth child key pack
    ek32                - bip32 extended keypair
    eknm                - named extended key
    epak                - extended account key pack
    espk                - extended account stealth key pack

    flag                - named integer flag

    ine                 - extkey index
    ins                 - stealth index, key: uint32_t, value: raw stealth address bytes

    keymeta
    key

    lao                 - locked anon/blind output: COutpoint
    lastfilteredheight
    lns                 - stealth link, key: keyid, value uint32_t (stealth index)
    lne                 - extkey link key: keyid, value uint32_t (stealth index)
    luo                 - locked unspent output

    mkey                - CMasterKey
    minversion

    oal
    oao                 - owned anon output
    orderposnext

    pool

    ris                 - reverse stealth index key: hashed raw stealth address bytes, value: uint32_t
    rtx                 - CTransactionRecord

    stx                 - CStoredTransaction
    sxad                - loose stealth address
    sxkm                - key meta data for keys received on stealth while wallet locked

    tx

    version
    votes               - vector of vote tokens added by time added asc

    wkey
    wset                - wallet setting
*/

class CTransactionRecord;
class CStoredTransaction;


class CStealthKeyMetadata
{
// Used to get secret for keys created by stealth transaction with wallet locked
public:
    CStealthKeyMetadata() {}

    CStealthKeyMetadata(CPubKey pkEphem_, CPubKey pkScan_)
    {
        pkEphem = pkEphem_;
        pkScan = pkScan_;
    };

    CPubKey pkEphem;
    CPubKey pkScan;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(pkEphem);
        READWRITE(pkScan);
    };
};

class CLockedAnonOutput
{
// expand key for anon output received with wallet locked
// stored in walletdb, key is pubkey hash160
public:
    CLockedAnonOutput() {}

    CLockedAnonOutput(CPubKey pkEphem_, CPubKey pkScan_, COutPoint outpoint_)
    {
        pkEphem = pkEphem_;
        pkScan = pkScan_;
        outpoint = outpoint_;
    };

    CPubKey   pkEphem;
    CPubKey   pkScan;
    COutPoint outpoint;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(pkEphem);
        READWRITE(pkScan);
        READWRITE(outpoint);
    };
};

class COwnedAnonOutput
{
// stored in walletdb, key is keyimage
// TODO: store nValue?
public:
    COwnedAnonOutput() {};

    COwnedAnonOutput(COutPoint outpoint_, bool fSpent_)
    {
        outpoint = outpoint_;
        fSpent   = fSpent_;
    };

    ec_point vchImage;
    int64_t nValue;

    COutPoint outpoint;
    bool fSpent;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(outpoint);
        READWRITE(fSpent);
    };
};

class CStealthAddressIndexed
{
public:
    CStealthAddressIndexed() {};

    CStealthAddressIndexed(std::vector<uint8_t> &addrRaw_) : addrRaw(addrRaw_) {};
    std::vector<uint8_t> addrRaw;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(addrRaw);
    };
};

class CVoteToken
{
public:
    CVoteToken() {};
    CVoteToken(uint32_t nToken_, int nStart_, int nEnd_, int64_t nTimeAdded_) :
        nToken(nToken_), nStart(nStart_), nEnd(nEnd_), nTimeAdded(nTimeAdded_) {};

    uint32_t nToken;
    int nStart;
    int nEnd;
    int64_t nTimeAdded;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(nToken);
        READWRITE(nStart);
        READWRITE(nEnd);
        READWRITE(nTimeAdded);
    };
};

/** Access to the wallet database */
class CHDWalletDB : public WalletBatch
{
public:
    CHDWalletDB(WalletDatabase& dbw, const char* pszMode = "r+", bool _fFlushOnClose = true) : WalletBatch(dbw, pszMode, _fFlushOnClose)
    {
    };

    bool InTxn()
    {
        return m_batch.pdb && m_batch.activeTxn;
    };

    Dbc *GetTxnCursor()
    {
        if (!m_batch.pdb || !m_batch.activeTxn)
            return nullptr;

        DbTxn *ptxnid = m_batch.activeTxn; // call TxnBegin first

        Dbc *pcursor = nullptr;
        int ret = m_batch.pdb->cursor(ptxnid, &pcursor, 0);
        if (ret != 0)
            return nullptr;
        return pcursor;
    };

    Dbc *GetCursor()
    {
        return m_batch.GetCursor();
    };

    template< typename T>
    bool Replace(Dbc *pcursor, const T &value)
    {
        if (!pcursor)
            return false;

        if (m_batch.fReadOnly)
            assert(!"Replace called on database in read-only mode");

        // Value
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(10000);
        ssValue << value;
        Dbt datValue(&ssValue[0], ssValue.size());

        // Write
        int ret = pcursor->put(nullptr, &datValue, DB_CURRENT);

        if (ret != 0)
        {
            LogPrintf("CursorPut ret %d - %s\n", ret, DbEnv::strerror(ret));
        }
        // Clear memory in case it was a private key
        memset(datValue.get_data(), 0, datValue.get_size());

        return (ret == 0);
    }

    int ReadAtCursor(Dbc *pcursor, CDataStream &ssKey, CDataStream &ssValue, unsigned int fFlags=DB_NEXT)
    {
        // Read at cursor
        Dbt datKey;
        if (fFlags == DB_SET || fFlags == DB_SET_RANGE || fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datKey.set_data(&ssKey[0]);
            datKey.set_size(ssKey.size());
        };

        Dbt datValue;
        if (fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datValue.set_data(&ssValue[0]);
            datValue.set_size(ssValue.size());
        };

        datKey.set_flags(DB_DBT_MALLOC);
        datValue.set_flags(DB_DBT_MALLOC);
        int ret = pcursor->get(&datKey, &datValue, fFlags);
        if (ret != 0)
            return ret;
        else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
            return 99999;

        // Convert to streams
        ssKey.SetType(SER_DISK);
        ssKey.clear();
        ssKey.write((char*)datKey.get_data(), datKey.get_size());

        ssValue.SetType(SER_DISK);
        ssValue.clear();
        ssValue.write((char*)datValue.get_data(), datValue.get_size());

        // Clear and free memory
        memset(datKey.get_data(), 0, datKey.get_size());
        memset(datValue.get_data(), 0, datValue.get_size());
        free(datKey.get_data());
        free(datValue.get_data());
        return 0;
    }

    int ReadKeyAtCursor(Dbc *pcursor, CDataStream &ssKey, unsigned int fFlags=DB_NEXT)
    {
        // Read key at cursor
        Dbt datKey;
        if (fFlags == DB_SET || fFlags == DB_SET_RANGE)
        {
            datKey.set_data(&ssKey[0]);
            datKey.set_size(ssKey.size());
        }
        datKey.set_flags(DB_DBT_MALLOC);

        Dbt datValue;
        datValue.set_flags(DB_DBT_PARTIAL); // don't read data, dlen and doff are 0 after memset

        int ret = pcursor->get(&datKey, &datValue, fFlags);
        if (ret != 0)
            return ret;
        if (datKey.get_data() == nullptr)
            return 99999;

        // Convert to streams
        ssKey.SetType(SER_DISK);
        ssKey.clear();
        ssKey.write((char*)datKey.get_data(), datKey.get_size());

        // Clear and free memory
        memset(datKey.get_data(), 0, datKey.get_size());
        free(datKey.get_data());
        return 0;
    }


    bool WriteStealthKeyMeta(const CKeyID &keyId, const CStealthKeyMetadata &sxKeyMeta);
    bool EraseStealthKeyMeta(const CKeyID &keyId);

    bool WriteStealthAddress(const CStealthAddress &sxAddr);
    bool ReadStealthAddress(CStealthAddress &sxAddr);
    bool EraseStealthAddress(const CStealthAddress &sxAddr);



    bool ReadNamedExtKeyId(const std::string &name, CKeyID &identifier, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteNamedExtKeyId(const std::string &name, const CKeyID &identifier);

    bool ReadExtKey(const CKeyID &identifier, CStoredExtKey &ek32, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteExtKey(const CKeyID &identifier, const CStoredExtKey &ek32);

    bool ReadExtAccount(const CKeyID &identifier, CExtKeyAccount &ekAcc, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteExtAccount(const CKeyID &identifier, const CExtKeyAccount &ekAcc);

    bool ReadExtKeyPack(const CKeyID &identifier, const uint32_t nPack, std::vector<CEKAKeyPack> &ekPak, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteExtKeyPack(const CKeyID &identifier, const uint32_t nPack, const std::vector<CEKAKeyPack> &ekPak);

    bool ReadExtStealthKeyPack(const CKeyID &identifier, const uint32_t nPack, std::vector<CEKAStealthKeyPack> &aksPak, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteExtStealthKeyPack(const CKeyID &identifier, const uint32_t nPack, const std::vector<CEKAStealthKeyPack> &aksPak);

    bool ReadExtStealthKeyChildPack(const CKeyID &identifier, const uint32_t nPack, std::vector<CEKASCKeyPack> &asckPak, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteExtStealthKeyChildPack(const CKeyID &identifier, const uint32_t nPack, const std::vector<CEKASCKeyPack> &asckPak);

    bool ReadFlag(const std::string &name, int32_t &nValue, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteFlag(const std::string &name, int32_t nValue);


    bool ReadExtKeyIndex(uint32_t id, CKeyID &identifier, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteExtKeyIndex(uint32_t id, const CKeyID &identifier);


    bool ReadStealthAddressIndex(uint32_t id, CStealthAddressIndexed &sxi, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteStealthAddressIndex(uint32_t id, const CStealthAddressIndexed &sxi);

    bool ReadStealthAddressIndexReverse(const uint160 &hash, uint32_t &id, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteStealthAddressIndexReverse(const uint160 &hash, uint32_t id);

    //bool GetStealthAddressIndex(const CStealthAddressIndexed &sxi, uint32_t &id); // Get stealth index or create new index if none found

    bool ReadStealthAddressLink(const CKeyID &keyId, uint32_t &id, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteStealthAddressLink(const CKeyID &keyId, uint32_t id);

    bool WriteAddressBookEntry(const std::string &sKey, const CAddressBookData &data);
    bool EraseAddressBookEntry(const std::string &sKey);

    bool ReadVoteTokens(std::vector<CVoteToken> &vVoteTokens, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteVoteTokens(const std::vector<CVoteToken> &vVoteTokens);

    bool WriteTxRecord(const uint256 &hash, const CTransactionRecord &rtx);
    bool EraseTxRecord(const uint256 &hash);


    bool ReadStoredTx(const uint256 &hash, CStoredTransaction &stx, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteStoredTx(const uint256 &hash, const CStoredTransaction &stx);
    bool EraseStoredTx(const uint256 &hash);

    bool ReadAnonKeyImage(const CCmpPubKey &ki, COutPoint &op, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteAnonKeyImage(const CCmpPubKey &ki, const COutPoint &op);
    bool EraseAnonKeyImage(const CCmpPubKey &ki);


    bool HaveLockedAnonOut(const COutPoint &op, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteLockedAnonOut(const COutPoint &op);
    bool EraseLockedAnonOut(const COutPoint &op);


    bool ReadWalletSetting(const std::string &setting, std::string &json, uint32_t nFlags=DB_READ_UNCOMMITTED);
    bool WriteWalletSetting(const std::string &setting, const std::string &json);
    bool EraseWalletSetting(const std::string &setting);
};

//void ThreadFlushHDWalletDB();

#endif // PARTICL_WALLET_HDWALLETDB_H
