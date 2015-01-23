// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletdb.h"

#include "base58.h"
#include "protocol.h"
#include "serialize.h"
#include "sync.h"
#include "util.h"
#include "utiltime.h"
#include "wallet.h"

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

using namespace std;

static uint64_t nAccountingEntryNumber = 0;

//
// CWalletDB
//

bool CWalletDB::WriteName(const string& strAddress, const string& strName)
{
    return Write(make_pair(string("name"), strAddress), strName);
}

bool CWalletDB::EraseName(const string& strAddress)
{
    // This should only be used for sending addresses, never for receiving addresses,
    // receiving addresses must always have an address book entry if they're not change return.
    return Erase(make_pair(string("name"), strAddress));
}

bool CWalletDB::WritePurpose(const string& strAddress, const string& strPurpose)
{
    return Write(make_pair(string("purpose"), strAddress), strPurpose);
}

bool CWalletDB::ErasePurpose(const string& strPurpose)
{
    return Erase(make_pair(string("purpose"), strPurpose));
}

bool CWalletDB::WriteTx(uint256 hash, const CWalletTx& wtx)
{
    return Write(std::make_pair(std::string("tx"), hash), wtx);
}

bool CWalletDB::EraseTx(uint256 hash)
{
    return Erase(std::make_pair(std::string("tx"), hash));
}

bool CWalletDB::WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta)
{
    if (!Write(std::make_pair(std::string("keymeta"), vchPubKey),
               keyMeta, false))
        return false;

    // hash pubkey/privkey to accelerate wallet load
    std::vector<unsigned char> vchKey;
    vchKey.reserve(vchPubKey.size() + vchPrivKey.size());
    vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
    vchKey.insert(vchKey.end(), vchPrivKey.begin(), vchPrivKey.end());

    return Write(std::make_pair(std::string("key"), vchPubKey), std::make_pair(vchPrivKey, Hash(vchKey.begin(), vchKey.end())), false);
}

bool CWalletDB::WriteCryptedKey(const CPubKey& vchPubKey,
                                const std::vector<unsigned char>& vchCryptedSecret,
                                const CKeyMetadata &keyMeta)
{
    const bool fEraseUnencryptedKey = true;

    if (!Write(std::make_pair(std::string("keymeta"), vchPubKey),
            keyMeta))
        return false;

    if (!Write(std::make_pair(std::string("ckey"), vchPubKey), vchCryptedSecret, false))
        return false;
    if (fEraseUnencryptedKey)
    {
        Erase(std::make_pair(std::string("key"), vchPubKey));
        Erase(std::make_pair(std::string("wkey"), vchPubKey));
    }
    return true;
}

bool CWalletDB::WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey)
{
    return Write(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
}

bool CWalletDB::WriteCScript(const uint160& hash, const CScript& redeemScript)
{
    return Write(std::make_pair(std::string("cscript"), hash), redeemScript, false);
}

bool CWalletDB::WriteWatchOnly(const CScript &dest)
{
    return Write(std::make_pair(std::string("watchs"), dest), '1');
}

bool CWalletDB::EraseWatchOnly(const CScript &dest)
{
    return Erase(std::make_pair(std::string("watchs"), dest));
}

bool CWalletDB::WriteBestBlock(const CBlockLocator& locator)
{
    return Write(std::string("bestblock"), locator);
}

bool CWalletDB::ReadBestBlock(CBlockLocator& locator)
{
    return Read(std::string("bestblock"), locator);
}

bool CWalletDB::WriteOrderPosNext(int64_t nOrderPosNext)
{
    return Write(std::string("orderposnext"), nOrderPosNext);
}

bool CWalletDB::WriteDefaultKey(const CPubKey& vchPubKey)
{
    return Write(std::string("defaultkey"), vchPubKey);
}

bool CWalletDB::ReadPool(int64_t nPool, CKeyPool& keypool)
{
    return Read(std::make_pair(std::string("pool"), nPool), keypool);
}

bool CWalletDB::WritePool(int64_t nPool, const CKeyPool& keypool)
{
    return Write(std::make_pair(std::string("pool"), nPool), keypool);
}

bool CWalletDB::ErasePool(int64_t nPool)
{
    return Erase(std::make_pair(std::string("pool"), nPool));
}

bool CWalletDB::ReadAccount(const string& strAccount, CAccount& account)
{
    account.SetNull();
    return Read(make_pair(string("acc"), strAccount), account);
}

bool CWalletDB::WriteAccount(const string& strAccount, const CAccount& account)
{
    return Write(make_pair(string("acc"), strAccount), account);
}

bool CWalletDB::WriteAccountingEntry(const uint64_t nAccEntryNum, const CAccountingEntry& acentry)
{
    return Write(std::make_pair(std::string("acentry"), std::make_pair(acentry.strAccount, nAccEntryNum)), acentry);
}

bool CWalletDB::WriteAccountingEntry(const CAccountingEntry& acentry)
{
    return WriteAccountingEntry(++nAccountingEntryNumber, acentry);
}

void CWalletDB::ListAccountCreditDebit(const string& strAccount, list<CAccountingEntry>& entries)
{
    bool fAllAccounts = (strAccount == "*");

    for (CWalletDB::const_iterator it = begin(); it != end(); it++)
    {
        // Read next record
        CDataStream ssKey((*it).first, SER_DISK, CLIENT_VERSION);
        CDataStream ssValue((*it).second, SER_DISK, CLIENT_VERSION);

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "acentry")
            continue;
        
        CAccountingEntry acentry;
        ssKey >> acentry.strAccount;
        if (!fAllAccounts && acentry.strAccount != strAccount)
            continue;

        ssValue >> acentry;
        ssKey >> acentry.nEntryNo;
        entries.push_back(acentry);
    }
}

DBErrors CWalletDB::ReorderTransactions(CWallet* pwallet)
{
    LOCK(pwallet->cs_wallet);
    // Old wallets didn't have any defined order for transactions
    // Probably a bad idea to change the output of this

    // First: get all CWalletTx and CAccountingEntry into a sorted-by-time multimap.
    typedef pair<CWalletTx*, CAccountingEntry*> TxPair;
    typedef multimap<int64_t, TxPair > TxItems;
    TxItems txByTime;

    for (map<uint256, CWalletTx>::iterator it = pwallet->mapWallet.begin(); it != pwallet->mapWallet.end(); ++it)
    {
        CWalletTx* wtx = &((*it).second);
        txByTime.insert(make_pair(wtx->nTimeReceived, TxPair(wtx, (CAccountingEntry*)0)));
    }
    list<CAccountingEntry> acentries;
    ListAccountCreditDebit("", acentries);
    BOOST_FOREACH(CAccountingEntry& entry, acentries)
    {
        txByTime.insert(make_pair(entry.nTime, TxPair((CWalletTx*)0, &entry)));
    }

    int64_t& nOrderPosNext = pwallet->nOrderPosNext;
    nOrderPosNext = 0;
    std::vector<int64_t> nOrderPosOffsets;
    for (TxItems::iterator it = txByTime.begin(); it != txByTime.end(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        CAccountingEntry *const pacentry = (*it).second.second;
        int64_t& nOrderPos = (pwtx != 0) ? pwtx->nOrderPos : pacentry->nOrderPos;

        if (nOrderPos == -1)
        {
            nOrderPos = nOrderPosNext++;
            nOrderPosOffsets.push_back(nOrderPos);

            if (pwtx)
            {
                if (!WriteTx(pwtx->GetHash(), *pwtx))
                    return DB_LOAD_FAIL;
            }
            else
                if (!WriteAccountingEntry(pacentry->nEntryNo, *pacentry))
                    return DB_LOAD_FAIL;
        }
        else
        {
            int64_t nOrderPosOff = 0;
            BOOST_FOREACH(const int64_t& nOffsetStart, nOrderPosOffsets)
            {
                if (nOrderPos >= nOffsetStart)
                    ++nOrderPosOff;
            }
            nOrderPos += nOrderPosOff;
            nOrderPosNext = std::max(nOrderPosNext, nOrderPos + 1);

            if (!nOrderPosOff)
                continue;

            // Since we're changing the order, write it back
            if (pwtx)
            {
                if (!WriteTx(pwtx->GetHash(), *pwtx))
                    return DB_LOAD_FAIL;
            }
            else
                if (!WriteAccountingEntry(pacentry->nEntryNo, *pacentry))
                    return DB_LOAD_FAIL;
        }
    }
    WriteOrderPosNext(nOrderPosNext);

    return DB_LOAD_OK;
}

class CWalletScanState {
public:
    unsigned int nKeys;
    unsigned int nCKeys;
    unsigned int nKeyMeta;
    bool fIsEncrypted;
    bool fAnyUnordered;
    int nFileVersion;
    vector<uint256> vWalletUpgrade;

    CWalletScanState() {
        nKeys = nCKeys = nKeyMeta = 0;
        fIsEncrypted = false;
        fAnyUnordered = false;
        nFileVersion = 0;
    }
};

bool
ReadKeyValue(CWallet* pwallet, CDataStream& ssKey, CDataStream& ssValue,
             CWalletScanState &wss, string& strType, string& strErr)
{
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other
        ssKey >> strType;
        if (strType == "name")
        {
            string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet->mapAddressBook[CBitcoinAddress(strAddress).Get()].name;
        }
        else if (strType == "purpose")
        {
            string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet->mapAddressBook[CBitcoinAddress(strAddress).Get()].purpose;
        }
        else if (strType == "tx")
        {
            uint256 hash;
            ssKey >> hash;
            CWalletTx wtx;
            ssValue >> wtx;
            CValidationState state;
            if (!(CheckTransaction(wtx, state) && (wtx.GetHash() == hash) && state.IsValid()))
                return false;

            // Undo serialize changes in 31600
            if (31404 <= wtx.fTimeReceivedIsTxTime && wtx.fTimeReceivedIsTxTime <= 31703)
            {
                if (!ssValue.empty())
                {
                    char fTmp;
                    char fUnused;
                    ssValue >> fTmp >> fUnused >> wtx.strFromAccount;
                    strErr = strprintf("LoadWallet() upgrading tx ver=%d %d '%s' %s",
                                       wtx.fTimeReceivedIsTxTime, fTmp, wtx.strFromAccount, hash.ToString());
                    wtx.fTimeReceivedIsTxTime = fTmp;
                }
                else
                {
                    strErr = strprintf("LoadWallet() repairing tx ver=%d %s", wtx.fTimeReceivedIsTxTime, hash.ToString());
                    wtx.fTimeReceivedIsTxTime = 0;
                }
                wss.vWalletUpgrade.push_back(hash);
            }

            if (wtx.nOrderPos == -1)
                wss.fAnyUnordered = true;

            pwallet->AddToWallet(wtx, true, NULL);
        }
        else if (strType == "acentry")
        {
            string strAccount;
            ssKey >> strAccount;
            uint64_t nNumber;
            ssKey >> nNumber;
            if (nNumber > nAccountingEntryNumber)
                nAccountingEntryNumber = nNumber;

            if (!wss.fAnyUnordered)
            {
                CAccountingEntry acentry;
                ssValue >> acentry;
                if (acentry.nOrderPos == -1)
                    wss.fAnyUnordered = true;
            }
        }
        else if (strType == "watchs")
        {
            CScript script;
            ssKey >> script;
            char fYes;
            ssValue >> fYes;
            if (fYes == '1')
                pwallet->LoadWatchOnly(script);

            // Watch-only addresses have no birthday information for now,
            // so set the wallet birthday to the beginning of time.
            pwallet->nTimeFirstKey = 1;
        }
        else if (strType == "key" || strType == "wkey")
        {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            if (!vchPubKey.IsValid())
            {
                strErr = "Error reading wallet database: CPubKey corrupt";
                return false;
            }
            CKey key;
            CPrivKey pkey;
            uint256 hash;

            if (strType == "key")
            {
                wss.nKeys++;
                ssValue >> pkey;
            } else {
                CWalletKey wkey;
                ssValue >> wkey;
                pkey = wkey.vchPrivKey;
            }

            // Old wallets store keys as "key" [pubkey] => [privkey]
            // ... which was slow for wallets with lots of keys, because the public key is re-derived from the private key
            // using EC operations as a checksum.
            // Newer wallets store keys as "key"[pubkey] => [privkey][hash(pubkey,privkey)], which is much faster while
            // remaining backwards-compatible.
            try
            {
                ssValue >> hash;
            }
            catch (...) {}

            bool fSkipCheck = false;

            if (!hash.IsNull())
            {
                // hash pubkey/privkey to accelerate wallet load
                std::vector<unsigned char> vchKey;
                vchKey.reserve(vchPubKey.size() + pkey.size());
                vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
                vchKey.insert(vchKey.end(), pkey.begin(), pkey.end());

                if (Hash(vchKey.begin(), vchKey.end()) != hash)
                {
                    strErr = "Error reading wallet database: CPubKey/CPrivKey corrupt";
                    return false;
                }

                fSkipCheck = true;
            }

            if (!key.Load(pkey, vchPubKey, fSkipCheck))
            {
                strErr = "Error reading wallet database: CPrivKey corrupt";
                return false;
            }
            if (!pwallet->LoadKey(key, vchPubKey))
            {
                strErr = "Error reading wallet database: LoadKey failed";
                return false;
            }
        }
        else if (strType == "mkey")
        {
            unsigned int nID;
            ssKey >> nID;
            CMasterKey kMasterKey;
            ssValue >> kMasterKey;
            if(pwallet->mapMasterKeys.count(nID) != 0)
            {
                strErr = strprintf("Error reading wallet database: duplicate CMasterKey id %u", nID);
                return false;
            }
            pwallet->mapMasterKeys[nID] = kMasterKey;
            if (pwallet->nMasterKeyMaxID < nID)
                pwallet->nMasterKeyMaxID = nID;
        }
        else if (strType == "ckey")
        {
            vector<unsigned char> vchPubKey;
            ssKey >> vchPubKey;
            vector<unsigned char> vchPrivKey;
            ssValue >> vchPrivKey;
            wss.nCKeys++;

            if (!pwallet->LoadCryptedKey(vchPubKey, vchPrivKey))
            {
                strErr = "Error reading wallet database: LoadCryptedKey failed";
                return false;
            }
            wss.fIsEncrypted = true;
        }
        else if (strType == "keymeta")
        {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;

            pwallet->LoadKeyMetadata(vchPubKey, keyMeta);

            // find earliest key creation time, as wallet birthday
            if (!pwallet->nTimeFirstKey ||
                (keyMeta.nCreateTime < pwallet->nTimeFirstKey))
                pwallet->nTimeFirstKey = keyMeta.nCreateTime;
        }
        else if (strType == "defaultkey")
        {
            ssValue >> pwallet->vchDefaultKey;
        }
        else if (strType == "pool")
        {
            int64_t nIndex;
            ssKey >> nIndex;
            CKeyPool keypool;
            ssValue >> keypool;
            pwallet->setKeyPool.insert(nIndex);

            // If no metadata exists yet, create a default with the pool key's
            // creation time. Note that this may be overwritten by actually
            // stored metadata for that key later, which is fine.
            CKeyID keyid = keypool.vchPubKey.GetID();
            if (pwallet->mapKeyMetadata.count(keyid) == 0)
                pwallet->mapKeyMetadata[keyid] = CKeyMetadata(keypool.nTime);
        }
        else if (strType == "version")
        {
            ssValue >> wss.nFileVersion;
            if (wss.nFileVersion == 10300)
                wss.nFileVersion = 300;
        }
        else if (strType == "cscript")
        {
            uint160 hash;
            ssKey >> hash;
            CScript script;
            ssValue >> script;
            if (!pwallet->LoadCScript(script))
            {
                strErr = "Error reading wallet database: LoadCScript failed";
                return false;
            }
        }
        else if (strType == "orderposnext")
        {
            ssValue >> pwallet->nOrderPosNext;
        }
        else if (strType == "destdata")
        {
            std::string strAddress, strKey, strValue;
            ssKey >> strAddress;
            ssKey >> strKey;
            ssValue >> strValue;
            if (!pwallet->LoadDestData(CBitcoinAddress(strAddress).Get(), strKey, strValue))
            {
                strErr = "Error reading wallet database: LoadDestData failed";
                return false;
            }
        }
    } catch (...)
    {
        return false;
    }
    return true;
}

static bool IsKeyType(string strType)
{
    return (strType== "key" || strType == "wkey" ||
            strType == "mkey" || strType == "ckey");
}

DBErrors CWalletDB::LoadWallet(CWallet* pwallet)
{
    if(!Load())
        return DB_CORRUPT;
    
    pwallet->vchDefaultKey = CPubKey();
    CWalletScanState wss;
    bool fNoncriticalErrors = false;
    DBErrors result = DB_LOAD_OK;

    bool fAutoTransaction = TxnBegin();
    
    try {
        LOCK(pwallet->cs_wallet);
        for (CWalletDB::const_iterator it = begin(); it != end(); it++)
        {
            // Read next record
            CDataStream ssKey((*it).first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue((*it).second, SER_DISK, CLIENT_VERSION);

            // Try to be tolerant of single corrupt records:
            string strType, strErr;
            if (!ReadKeyValue(pwallet, ssKey, ssValue, wss, strType, strErr))
            {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (IsKeyType(strType))
                    result = DB_CORRUPT;
                else
                {
                    // Leave other errors alone, if we try to fix them we might make things worse.
                    fNoncriticalErrors = true; // ... but do warn the user there is something wrong.
                    if (strType == "tx")
                        // Rescan if there is a bad transaction record:
                        SoftSetBoolArg("-rescan", true);
                }
            }
            if (!strErr.empty())
                LogPrintf("%s\n", strErr);
        }
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (...) {
        result = DB_CORRUPT;
    }

    if (fNoncriticalErrors && result == DB_LOAD_OK)
        result = DB_NONCRITICAL_ERROR;

    // Any wallet corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != DB_LOAD_OK)
        return result;

    LogPrintf("nFileVersion = %d\n", wss.nFileVersion);

    LogPrintf("Keys: %u plaintext, %u encrypted, %u w/ metadata, %u total\n",
           wss.nKeys, wss.nCKeys, wss.nKeyMeta, wss.nKeys + wss.nCKeys);

    // nTimeFirstKey is only reliable if all keys have metadata
    if ((wss.nKeys + wss.nCKeys) != wss.nKeyMeta)
        pwallet->nTimeFirstKey = 1; // 0 would be considered 'no value'

    BOOST_FOREACH(uint256 hash, wss.vWalletUpgrade)
        WriteTx(hash, pwallet->mapWallet[hash]);

    if (wss.fAnyUnordered)
        result = ReorderTransactions(pwallet);

    if (fAutoTransaction)
        TxnCommit();
        
    return result;
}

DBErrors CWalletDB::FindWalletTx(CWallet* pwallet, vector<uint256>& vTxHash, vector<CWalletTx>& vWtx)
{
    pwallet->vchDefaultKey = CPubKey();
    bool fNoncriticalErrors = false;
    DBErrors result = DB_LOAD_OK;

    try {
        LOCK(pwallet->cs_wallet);
        for (CWalletDB::const_iterator it = begin(); it != end(); it++)
        {
            // Read next record
            CDataStream ssKey((*it).first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue((*it).second, SER_DISK, CLIENT_VERSION);
            
            string strType;
            ssKey >> strType;
            if (strType == "tx") {
                uint256 hash;
                ssKey >> hash;

                CWalletTx wtx;
                ssValue >> wtx;

                vTxHash.push_back(hash);
                vWtx.push_back(wtx);
            }
        }
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (...) {
        result = DB_CORRUPT;
    }

    if (fNoncriticalErrors && result == DB_LOAD_OK)
        result = DB_NONCRITICAL_ERROR;

    return result;
}

bool CWalletDB::ZapWalletTx(CWallet* pwallet, vector<CWalletTx>& vWtx)
{
    // build list of wallet TXs
    vector<uint256> vTxHash;
    DBErrors err = FindWalletTx(pwallet, vTxHash, vWtx);
    if (err != DB_LOAD_OK)
        return false;

    // erase each wallet TX
    BOOST_FOREACH (uint256& hash, vTxHash) {
        if (!EraseTx(hash))
            return false;
    }

    return true;
}

DBErrors CWalletDB::RewriteAndReplace(const string& walletFile)
{
    std::string newFile = walletFile+".tmp";
    Rewrite(newFile);
    
    //simple verify
    CLogDBFile *newDB = new CLogDBFile();
    bool dbLoadRet = newDB->Open(newFile.c_str(), false);
    delete newDB;
    
    if(dbLoadRet)
    {
        Close();
        
        
        boost::filesystem::remove_all(walletFile);
        RenameOver(newFile, walletFile);
        
        //reopen the db
        if(!ReloadDB(walletFile))
        {
            return DB_CORRUPT;
        }
    }
    
    return DB_LOAD_OK;
}

//
// Try to (very carefully!) recover wallet.dat if there is a problem.
//
bool CWalletDB::Recover(std::string filename, bool fOnlyKeys)
{
    //TODO
    return false;
}

bool CWalletDB::Recover(std::string filename)
{
    //TODO
    return CWalletDB::Recover(filename, false);
}

bool CWalletDB::Verify(std::string filename, bool salvage)
{
    // simple verifying by loading the db keystore but not load the keys itself
    CWalletDB *newDB = new CWalletDB(filename);
    bool loadRet = newDB->Load();
    delete newDB;
    
    return loadRet;
}

bool CWalletDB::WriteDestData(const std::string &address, const std::string &key, const std::string &value)
{
    return Write(std::make_pair(std::string("destdata"), std::make_pair(address, key)), value);
}

bool CWalletDB::EraseDestData(const std::string &address, const std::string &key)
{
    return Erase(std::make_pair(std::string("destdata"), std::make_pair(address, key)));
}
