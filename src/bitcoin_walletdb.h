// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BITCOIN_WALLETDB_H
#define BITCOIN_BITCOIN_WALLETDB_H

#include "bitcoin_db.h"
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
class Bitcoin_CWallet;
class Bitcoin_CWalletTx;
class uint160;
class uint256;

/** Error statuses for the wallet database */
enum Bitcoin_DBErrors
{
    BITCOIN_DB_LOAD_OK,
    BITCOIN_DB_CORRUPT,
    BITCOIN_DB_NONCRITICAL_ERROR,
    BITCOIN_DB_TOO_NEW,
    BITCOIN_DB_LOAD_FAIL,
    BITCOIN_DB_NEED_REWRITE
};

class Bitcoin_CKeyMetadata
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown

    Bitcoin_CKeyMetadata()
    {
        SetNull();
    }
    Bitcoin_CKeyMetadata(int64_t nCreateTime_)
    {
        nVersion = Bitcoin_CKeyMetadata::CURRENT_VERSION;
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
        nVersion = Bitcoin_CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
    }
};

/** Access to the wallet database (wallet.dat) */
class Bitcoin_CWalletDB : public Bitcoin_CDB
{
public:
    Bitcoin_CWalletDB(std::string strFilename, const char* pszMode="r+") : Bitcoin_CDB(strFilename.c_str(), pszMode)
    {
    }
private:
    Bitcoin_CWalletDB(const Bitcoin_CWalletDB&);
    void operator=(const Bitcoin_CWalletDB&);
public:
    bool WriteName(const std::string& strAddress, const std::string& strName);
    bool EraseName(const std::string& strAddress);

    bool WritePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(const std::string& strAddress);

    bool WriteTx(uint256 hash, const Bitcoin_CWalletTx& wtx);
    bool EraseTx(uint256 hash);

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const Bitcoin_CKeyMetadata &keyMeta);
    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const Bitcoin_CKeyMetadata &keyMeta);
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
    bool WriteAccountingEntry(const CAccountingEntry& acentry);
    int64_t GetAccountCreditDebit(const std::string& strAccount);
    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

    Bitcoin_DBErrors ReorderTransactions(Bitcoin_CWallet*);
    Bitcoin_DBErrors LoadWallet(Bitcoin_CWallet* pwallet);
    Bitcoin_DBErrors FindWalletTx(Bitcoin_CWallet* pwallet, std::vector<uint256>& vTxHash);
    Bitcoin_DBErrors ZapWalletTx(Bitcoin_CWallet* pwallet);
    static bool Recover(Bitcoin_CDBEnv& dbenv, std::string filename, bool fOnlyKeys);
    static bool Recover(Bitcoin_CDBEnv& dbenv, std::string filename);
};

bool Bitcoin_BackupWallet(const Bitcoin_CWallet& wallet, const std::string& strDest);

#endif // BITCOIN_WALLETDB_H
