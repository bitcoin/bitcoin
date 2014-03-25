// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_WALLETDB_H
#define BITCOIN_WALLETDB_H

#include "db.h"
#include "key.h"

#include <list>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class CAccount;
class CAccountingEntry;
struct CBlockLocator;
class CKeyPool;
class CMasterKey;
class CScript;
class Bitcredit_CWallet;
class Bitcredit_CWalletTx;
class uint160;
class uint256;

/** Error statuses for the wallet database */
enum Bitcredit_DBErrors
{
    BITCREDIT_DB_LOAD_OK,
    BITCREDIT_DB_CORRUPT,
    BITCREDIT_DB_NONCRITICAL_ERROR,
    BITCREDIT_DB_TOO_NEW,
    BITCREDIT_DB_LOAD_FAIL,
    BITCREDIT_DB_NEED_REWRITE
};

class Bitcredit_CKeyMetadata
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown

    Bitcredit_CKeyMetadata()
    {
        SetNull();
    }
    Bitcredit_CKeyMetadata(int64_t nCreateTime_)
    {
        nVersion = Bitcredit_CKeyMetadata::CURRENT_VERSION;
        nCreateTime = nCreateTime_;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
    )

    void SetNull()
    {
        nVersion = Bitcredit_CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
    }
};

/** Access to the wallet database (wallet.dat) */
class Bitcredit_CWalletDB : public Bitcredit_CDB
{
public:
    Bitcredit_CDBEnv *pbitDb;

    Bitcredit_CWalletDB(std::string strFilename, Bitcredit_CDBEnv *bitDbIn, const char* pszMode="r+") : Bitcredit_CDB(strFilename.c_str(), bitDbIn, pszMode)
    {
    	pbitDb = bitDbIn;
    }
private:
    Bitcredit_CWalletDB(const Bitcredit_CWalletDB&);
    void operator=(const Bitcredit_CWalletDB&);
public:
    bool WriteName(const std::string& strAddress, const std::string& strName);
    bool EraseName(const std::string& strAddress);

    bool WritePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(const std::string& strAddress);

    bool WriteTx(uint256 hash, const Bitcredit_CWalletTx& wtx);
    bool EraseTx(uint256 hash);

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const Bitcredit_CKeyMetadata &keyMeta);
    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const Bitcredit_CKeyMetadata &keyMeta);
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);

    bool WriteCScript(const uint160& hash, const CScript& redeemScript);

    bool WriteBestBlock(const CBlockLocator& locator);
    bool ReadBestBlock(CBlockLocator& locator);

    bool WriteOrderPosNext(int64_t nOrderPosNext);

    bool WriteDefaultKey(const CPubKey& vchPubKey);

    bool ReadPool(int64_t nPool, CKeyPool& keypool);
    bool WritePool(int64_t nPool, const CKeyPool& keypool);
    bool ErasePool(int64_t nPool);

    bool WriteMinVersion(int nVersion);

    bool ReadAccount(const std::string& strAccount, CAccount& account);
    bool WriteAccount(const std::string& strAccount, const CAccount& account);

    /// Write destination data key,value tuple to database
    bool WriteDestData(const std::string &address, const std::string &key, const std::string &value);
    /// Erase destination data tuple from wallet database
    bool EraseDestData(const std::string &address, const std::string &key);
private:
    bool WriteAccountingEntry(const uint64_t nAccEntryNum, const CAccountingEntry& acentry);
public:
    bool WriteAccountingEntry(const CAccountingEntry& acentry, uint64_t &nAccountingEntryNumber);
    int64_t GetAccountCreditDebit(const std::string& strAccount);
    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

    Bitcredit_DBErrors ReorderTransactions(Bitcredit_CWallet*);
    Bitcredit_DBErrors LoadWallet(Bitcredit_CWallet* pwallet, uint64_t &nAccountingEntryNumber);
    Bitcredit_DBErrors FindWalletTx(Bitcredit_CWallet* pwallet, std::vector<uint256>& vTxHash);
    Bitcredit_DBErrors ZapWalletTx(Bitcredit_CWallet* pwallet);
    static bool Recover(Bitcredit_CDBEnv& dbenv, std::string filename, bool fOnlyKeys, uint64_t &nAccountingEntryNumber);
    static bool Recover(Bitcredit_CDBEnv& dbenv, std::string filename, uint64_t &nAccountingEntryNumber);
};

bool Bitcredit_BackupWallet(const Bitcredit_CWallet& wallet, const std::string& strDest);

#endif // BITCOIN_WALLETDB_H
