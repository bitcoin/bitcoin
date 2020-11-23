// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletdb.h>

#include <fs.h>
#include <key_io.h>
#include <protocol.h>
#include <serialize.h>
#include <sync.h>
#include <util/system.h>
#include <util/time.h>
#include <wallet/wallet.h>

#include <atomic>
#include <string>

#include <boost/thread.hpp>

namespace DBKeys {
const std::string ACENTRY{"acentry"};
const std::string BESTBLOCK_NOMERKLE{"bestblock_nomerkle"};
const std::string BESTBLOCK{"bestblock"};
const std::string CRYPTED_KEY{"ckey"};
const std::string CSCRIPT{"cscript"};
const std::string DEFAULTKEY{"defaultkey"};
const std::string DESTDATA{"destdata"};
const std::string FLAGS{"flags"};
const std::string HDCHAIN{"hdchain"};
const std::string KEYMETA{"keymeta"};
const std::string KEY{"key"};
const std::string MASTER_KEY{"mkey"};
const std::string MINVERSION{"minversion"};
const std::string NAME{"name"};
const std::string OLD_KEY{"wkey"};
const std::string ORDERPOSNEXT{"orderposnext"};
const std::string POOL{"pool"};
const std::string PURPOSE{"purpose"};
const std::string SETTINGS{"settings"};
const std::string TX{"tx"};
const std::string VERSION{"version"};
const std::string WATCHMETA{"watchmeta"};
const std::string WATCHS{"watchs"};
} // namespace DBKeys

//
// WalletBatch
//

bool WalletBatch::WriteName(const std::string& strAddress, const std::string& strName)
{
    return WriteIC(std::make_pair(DBKeys::NAME, strAddress), strName);
}

bool WalletBatch::EraseName(const std::string& strAddress)
{
    // This should only be used for sending addresses, never for receiving addresses,
    // receiving addresses must always have an address book entry if they're not change return.
    return EraseIC(std::make_pair(DBKeys::NAME, strAddress));
}

bool WalletBatch::WritePurpose(const std::string& strAddress, const std::string& strPurpose)
{
    return WriteIC(std::make_pair(DBKeys::PURPOSE, strAddress), strPurpose);
}

bool WalletBatch::ErasePurpose(const std::string& strAddress)
{
    return EraseIC(std::make_pair(DBKeys::PURPOSE, strAddress));
}

bool WalletBatch::WriteTx(const CWalletTx& wtx)
{
    return WriteIC(std::make_pair(DBKeys::TX, wtx.GetHash()), wtx);
}

bool WalletBatch::EraseTx(uint256 hash)
{
    return EraseIC(std::make_pair(DBKeys::TX, hash));
}

bool WalletBatch::WriteKeyMetadata(const CKeyMetadata& meta, const CPubKey& pubkey, const bool overwrite)
{
    return WriteIC(std::make_pair(DBKeys::KEYMETA, pubkey), meta, overwrite);
}

bool WalletBatch::WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta)
{
    if (!WriteKeyMetadata(keyMeta, vchPubKey, false)) {
        return false;
    }

    // hash pubkey/privkey to accelerate wallet load
    std::vector<unsigned char> vchKey;
    vchKey.reserve(vchPubKey.size() + vchPrivKey.size());
    vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
    vchKey.insert(vchKey.end(), vchPrivKey.begin(), vchPrivKey.end());

    return WriteIC(std::make_pair(DBKeys::KEY, vchPubKey), std::make_pair(vchPrivKey, Hash(vchKey.begin(), vchKey.end())), false);
}

bool WalletBatch::WriteCryptedKey(const CPubKey& vchPubKey,
                                const std::vector<unsigned char>& vchCryptedSecret,
                                const CKeyMetadata &keyMeta)
{
    if (!WriteKeyMetadata(keyMeta, vchPubKey, true)) {
        return false;
    }

    if (!WriteIC(std::make_pair(DBKeys::CRYPTED_KEY, vchPubKey), vchCryptedSecret, false)) {
        return false;
    }
    EraseIC(std::make_pair(DBKeys::KEY, vchPubKey));
    return true;
}

bool WalletBatch::WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey)
{
    return WriteIC(std::make_pair(DBKeys::MASTER_KEY, nID), kMasterKey, true);
}

bool WalletBatch::WriteCScript(const uint160& hash, const CScript& redeemScript)
{
    return WriteIC(std::make_pair(DBKeys::CSCRIPT, hash), redeemScript, false);
}

bool WalletBatch::WriteWatchOnly(const CScript &dest, const CKeyMetadata& keyMeta)
{
    if (!WriteIC(std::make_pair(DBKeys::WATCHMETA, dest), keyMeta)) {
        return false;
    }
    return WriteIC(std::make_pair(DBKeys::WATCHS, dest), '1');
}

bool WalletBatch::EraseWatchOnly(const CScript &dest)
{
    if (!EraseIC(std::make_pair(DBKeys::WATCHMETA, dest))) {
        return false;
    }
    return EraseIC(std::make_pair(DBKeys::WATCHS, dest));
}

bool WalletBatch::WriteBestBlock(const CBlockLocator& locator)
{
    WriteIC(DBKeys::BESTBLOCK, CBlockLocator()); // Write empty block locator so versions that require a merkle branch automatically rescan
    return WriteIC(DBKeys::BESTBLOCK_NOMERKLE, locator);
}

bool WalletBatch::ReadBestBlock(CBlockLocator& locator)
{
    if (m_batch.Read(DBKeys::BESTBLOCK, locator) && !locator.vHave.empty()) return true;
    return m_batch.Read(DBKeys::BESTBLOCK_NOMERKLE, locator);
}

bool WalletBatch::WriteOrderPosNext(int64_t nOrderPosNext)
{
    return WriteIC(DBKeys::ORDERPOSNEXT, nOrderPosNext);
}

bool WalletBatch::ReadPool(int64_t nPool, CKeyPool& keypool)
{
    return m_batch.Read(std::make_pair(DBKeys::POOL, nPool), keypool);
}

bool WalletBatch::WritePool(int64_t nPool, const CKeyPool& keypool)
{
    return WriteIC(std::make_pair(DBKeys::POOL, nPool), keypool);
}

bool WalletBatch::ErasePool(int64_t nPool)
{
    return EraseIC(std::make_pair(DBKeys::POOL, nPool));
}

bool WalletBatch::WriteMinVersion(int nVersion)
{
    return WriteIC(DBKeys::MINVERSION, nVersion);
}

class CWalletScanState {
public:
    unsigned int nKeys{0};
    unsigned int nCKeys{0};
    unsigned int nWatchKeys{0};
    unsigned int nKeyMeta{0};
    unsigned int m_unknown_records{0};
    bool fIsEncrypted{false};
    bool fAnyUnordered{false};
    std::vector<uint256> vWalletUpgrade;

    CWalletScanState() {
    }
};

static bool
ReadKeyValue(CWallet* pwallet, CDataStream& ssKey, CDataStream& ssValue,
             CWalletScanState &wss, std::string& strType, std::string& strErr) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other
        ssKey >> strType;
        if (strType == DBKeys::NAME) {
            std::string strAddress;
            ssKey >> strAddress;
            std::string label;
            ssValue >> label;
            pwallet->m_address_book[DecodeDestination(strAddress)].SetLabel(label);
        } else if (strType == DBKeys::PURPOSE) {
            std::string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet->m_address_book[DecodeDestination(strAddress)].purpose;
        } else if (strType == DBKeys::TX) {
            uint256 hash;
            ssKey >> hash;
            CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
            ssValue >> wtx;
            if (wtx.GetHash() != hash)
                return false;

            // Undo serialize changes in 31600
            if (31404 <= wtx.fTimeReceivedIsTxTime && wtx.fTimeReceivedIsTxTime <= 31703)
            {
                if (!ssValue.empty())
                {
                    char fTmp;
                    char fUnused;
                    std::string unused_string;
                    ssValue >> fTmp >> fUnused >> unused_string;
                    strErr = strprintf("LoadWallet() upgrading tx ver=%d %d %s",
                                       wtx.fTimeReceivedIsTxTime, fTmp, hash.ToString());
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

            pwallet->LoadToWallet(wtx);
        } else if (strType == DBKeys::WATCHS) {
            wss.nWatchKeys++;
            CScript script;
            ssKey >> script;
            char fYes;
            ssValue >> fYes;
            if (fYes == '1') {
                pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadWatchOnly(script);
            }
        } else if (strType == DBKeys::KEY) {
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

            wss.nKeys++;
            ssValue >> pkey;

            // Old wallets store keys as DBKeys::KEY [pubkey] => [privkey]
            // ... which was slow for wallets with lots of keys, because the public key is re-derived from the private key
            // using EC operations as a checksum.
            // Newer wallets store keys as DBKeys::KEY [pubkey] => [privkey][hash(pubkey,privkey)], which is much faster while
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
            if (!pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadKey(key, vchPubKey))
            {
                strErr = "Error reading wallet database: LegacyScriptPubKeyMan::LoadKey failed";
                return false;
            }
        } else if (strType == DBKeys::MASTER_KEY) {
            // Master encryption key is loaded into only the wallet and not any of the ScriptPubKeyMans.
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
        } else if (strType == DBKeys::CRYPTED_KEY) {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            if (!vchPubKey.IsValid())
            {
                strErr = "Error reading wallet database: CPubKey corrupt";
                return false;
            }
            std::vector<unsigned char> vchPrivKey;
            ssValue >> vchPrivKey;
            wss.nCKeys++;

            if (!pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadCryptedKey(vchPubKey, vchPrivKey))
            {
                strErr = "Error reading wallet database: LegacyScriptPubKeyMan::LoadCryptedKey failed";
                return false;
            }
            wss.fIsEncrypted = true;
        } else if (strType == DBKeys::KEYMETA) {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;
            pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadKeyMetadata(vchPubKey.GetID(), keyMeta);
        } else if (strType == DBKeys::WATCHMETA) {
            CScript script;
            ssKey >> script;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;
            pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadScriptMetadata(CScriptID(script), keyMeta);
        } else if (strType == DBKeys::DEFAULTKEY) {
            // We don't want or need the default key, but if there is one set,
            // we want to make sure that it is valid so that we can detect corruption
            CPubKey vchPubKey;
            ssValue >> vchPubKey;
            if (!vchPubKey.IsValid()) {
                strErr = "Error reading wallet database: Default Key corrupt";
                return false;
            }
        } else if (strType == DBKeys::POOL) {
            int64_t nIndex;
            ssKey >> nIndex;
            CKeyPool keypool;
            ssValue >> keypool;

            pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadKeyPool(nIndex, keypool);
        } else if (strType == DBKeys::CSCRIPT) {
            uint160 hash;
            ssKey >> hash;
            CScript script;
            ssValue >> script;
            if (!pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadCScript(script))
            {
                strErr = "Error reading wallet database: LegacyScriptPubKeyMan::LoadCScript failed";
                return false;
            }
        } else if (strType == DBKeys::ORDERPOSNEXT) {
            ssValue >> pwallet->nOrderPosNext;
        } else if (strType == DBKeys::DESTDATA) {
            std::string strAddress, strKey, strValue;
            ssKey >> strAddress;
            ssKey >> strKey;
            ssValue >> strValue;
            pwallet->LoadDestData(DecodeDestination(strAddress), strKey, strValue);
        } else if (strType == DBKeys::HDCHAIN) {
            CHDChain chain;
            ssValue >> chain;
            pwallet->GetOrCreateLegacyScriptPubKeyMan()->SetHDChain(chain, true);
        } else if (strType == DBKeys::FLAGS) {
            uint64_t flags;
            ssValue >> flags;
            if (!pwallet->SetWalletFlags(flags, true)) {
                strErr = "Error reading wallet database: Unknown non-tolerable wallet flags found";
                return false;
            }
        } else if (strType == DBKeys::OLD_KEY) {
            strErr = "Found unsupported 'wkey' record, try loading with version 0.18";
            return false;
        } else if (strType != DBKeys::BESTBLOCK && strType != DBKeys::BESTBLOCK_NOMERKLE &&
                   strType != DBKeys::MINVERSION && strType != DBKeys::ACENTRY &&
                   strType != DBKeys::VERSION && strType != DBKeys::SETTINGS) {
            wss.m_unknown_records++;
        }
    } catch (const std::exception& e) {
        if (strErr.empty()) {
            strErr = e.what();
        }
        return false;
    } catch (...) {
        if (strErr.empty()) {
            strErr = "Caught unknown exception in ReadKeyValue";
        }
        return false;
    }
    return true;
}

bool WalletBatch::IsKeyType(const std::string& strType)
{
    return (strType == DBKeys::KEY ||
            strType == DBKeys::MASTER_KEY || strType == DBKeys::CRYPTED_KEY);
}

DBErrors WalletBatch::LoadWallet(CWallet* pwallet)
{
    CWalletScanState wss;
    bool fNoncriticalErrors = false;
    DBErrors result = DBErrors::LOAD_OK;

    LOCK(pwallet->cs_wallet);
    try {
        int nMinVersion = 0;
        if (m_batch.Read(DBKeys::MINVERSION, nMinVersion)) {
            if (nMinVersion > FEATURE_LATEST)
                return DBErrors::TOO_NEW;
            pwallet->LoadMinVersion(nMinVersion);
        }

        // Get cursor
        Dbc* pcursor = m_batch.GetCursor();
        if (!pcursor)
        {
            pwallet->WalletLogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true)
        {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                pwallet->WalletLogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            std::string strType, strErr;
            if (!ReadKeyValue(pwallet, ssKey, ssValue, wss, strType, strErr))
            {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (IsKeyType(strType) || strType == DBKeys::DEFAULTKEY) {
                    result = DBErrors::CORRUPT;
                } else if (strType == DBKeys::FLAGS) {
                    // reading the wallet flags can only fail if unknown flags are present
                    result = DBErrors::TOO_NEW;
                } else {
                    // Leave other errors alone, if we try to fix them we might make things worse.
                    fNoncriticalErrors = true; // ... but do warn the user there is something wrong.
                    if (strType == DBKeys::TX)
                        // Rescan if there is a bad transaction record:
                        gArgs.SoftSetBoolArg("-rescan", true);
                }
            }
            if (!strErr.empty())
                pwallet->WalletLogPrintf("%s\n", strErr);
        }
        pcursor->close();
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (...) {
        result = DBErrors::CORRUPT;
    }

    if (fNoncriticalErrors && result == DBErrors::LOAD_OK)
        result = DBErrors::NONCRITICAL_ERROR;

    // Any wallet corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != DBErrors::LOAD_OK)
        return result;

    // Last client version to open this wallet, was previously the file version number
    int last_client = CLIENT_VERSION;
    m_batch.Read(DBKeys::VERSION, last_client);

    int wallet_version = pwallet->GetVersion();
    pwallet->WalletLogPrintf("Wallet File Version = %d\n", wallet_version > 0 ? wallet_version : last_client);

    pwallet->WalletLogPrintf("Keys: %u plaintext, %u encrypted, %u w/ metadata, %u total. Unknown wallet records: %u\n",
           wss.nKeys, wss.nCKeys, wss.nKeyMeta, wss.nKeys + wss.nCKeys, wss.m_unknown_records);

    // nTimeFirstKey is only reliable if all keys have metadata
    if ((wss.nKeys + wss.nCKeys + wss.nWatchKeys) != wss.nKeyMeta) {
        auto spk_man = pwallet->GetOrCreateLegacyScriptPubKeyMan();
        if (spk_man) {
            LOCK(spk_man->cs_KeyStore);
            spk_man->UpdateTimeFirstKey(1);
        }
    }

    for (const uint256& hash : wss.vWalletUpgrade)
        WriteTx(pwallet->mapWallet.at(hash));

    // Rewrite encrypted wallets of versions 0.4.0 and 0.5.0rc:
    if (wss.fIsEncrypted && (last_client == 40000 || last_client == 50000))
        return DBErrors::NEED_REWRITE;

    if (last_client < CLIENT_VERSION) // Update
        m_batch.Write(DBKeys::VERSION, CLIENT_VERSION);

    if (wss.fAnyUnordered)
        result = pwallet->ReorderTransactions();

    // Upgrade all of the wallet keymetadata to have the hd master key id
    // This operation is not atomic, but if it fails, updated entries are still backwards compatible with older software
    try {
        pwallet->UpgradeKeyMetadata();
    } catch (...) {
        result = DBErrors::CORRUPT;
    }

    return result;
}

DBErrors WalletBatch::FindWalletTx(std::vector<uint256>& vTxHash, std::vector<CWalletTx>& vWtx)
{
    DBErrors result = DBErrors::LOAD_OK;

    try {
        int nMinVersion = 0;
        if (m_batch.Read(DBKeys::MINVERSION, nMinVersion)) {
            if (nMinVersion > FEATURE_LATEST)
                return DBErrors::TOO_NEW;
        }

        // Get cursor
        Dbc* pcursor = m_batch.GetCursor();
        if (!pcursor)
        {
            LogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true)
        {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                LogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            std::string strType;
            ssKey >> strType;
            if (strType == DBKeys::TX) {
                uint256 hash;
                ssKey >> hash;

                CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
                ssValue >> wtx;

                vTxHash.push_back(hash);
                vWtx.push_back(wtx);
            }
        }
        pcursor->close();
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (...) {
        result = DBErrors::CORRUPT;
    }

    return result;
}

DBErrors WalletBatch::ZapSelectTx(std::vector<uint256>& vTxHashIn, std::vector<uint256>& vTxHashOut)
{
    // build list of wallet TXs and hashes
    std::vector<uint256> vTxHash;
    std::vector<CWalletTx> vWtx;
    DBErrors err = FindWalletTx(vTxHash, vWtx);
    if (err != DBErrors::LOAD_OK) {
        return err;
    }

    std::sort(vTxHash.begin(), vTxHash.end());
    std::sort(vTxHashIn.begin(), vTxHashIn.end());

    // erase each matching wallet TX
    bool delerror = false;
    std::vector<uint256>::iterator it = vTxHashIn.begin();
    for (const uint256& hash : vTxHash) {
        while (it < vTxHashIn.end() && (*it) < hash) {
            it++;
        }
        if (it == vTxHashIn.end()) {
            break;
        }
        else if ((*it) == hash) {
            if(!EraseTx(hash)) {
                LogPrint(BCLog::WALLETDB, "Transaction was found for deletion but returned database error: %s\n", hash.GetHex());
                delerror = true;
            }
            vTxHashOut.push_back(hash);
        }
    }

    if (delerror) {
        return DBErrors::CORRUPT;
    }
    return DBErrors::LOAD_OK;
}

DBErrors WalletBatch::ZapWalletTx(std::vector<CWalletTx>& vWtx)
{
    // build list of wallet TXs
    std::vector<uint256> vTxHash;
    DBErrors err = FindWalletTx(vTxHash, vWtx);
    if (err != DBErrors::LOAD_OK)
        return err;

    // erase each wallet TX
    for (const uint256& hash : vTxHash) {
        if (!EraseTx(hash))
            return DBErrors::CORRUPT;
    }

    return DBErrors::LOAD_OK;
}

void MaybeCompactWalletDB()
{
    static std::atomic<bool> fOneThread(false);
    if (fOneThread.exchange(true)) {
        return;
    }
    if (!gArgs.GetBoolArg("-flushwallet", DEFAULT_FLUSHWALLET)) {
        return;
    }

    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        WalletDatabase& dbh = pwallet->GetDBHandle();

        unsigned int nUpdateCounter = dbh.nUpdateCounter;

        if (dbh.nLastSeen != nUpdateCounter) {
            dbh.nLastSeen = nUpdateCounter;
            dbh.nLastWalletUpdate = GetTime();
        }

        if (dbh.nLastFlushed != nUpdateCounter && GetTime() - dbh.nLastWalletUpdate >= 2) {
            if (BerkeleyBatch::PeriodicFlush(dbh)) {
                dbh.nLastFlushed = nUpdateCounter;
            }
        }
    }

    fOneThread = false;
}

//
// Try to (very carefully!) recover wallet file if there is a problem.
//
bool WalletBatch::Recover(const fs::path& wallet_path, void *callbackDataIn, bool (*recoverKVcallback)(void* callbackData, CDataStream ssKey, CDataStream ssValue), std::string& out_backup_filename)
{
    return BerkeleyBatch::Recover(wallet_path, callbackDataIn, recoverKVcallback, out_backup_filename);
}

bool WalletBatch::Recover(const fs::path& wallet_path, std::string& out_backup_filename)
{
    // recover without a key filter callback
    // results in recovering all record types
    return WalletBatch::Recover(wallet_path, nullptr, nullptr, out_backup_filename);
}

bool WalletBatch::RecoverKeysOnlyFilter(void *callbackData, CDataStream ssKey, CDataStream ssValue)
{
    CWallet *dummyWallet = reinterpret_cast<CWallet*>(callbackData);
    CWalletScanState dummyWss;
    std::string strType, strErr;
    bool fReadOK;
    {
        // Required in LoadKeyMetadata():
        LOCK(dummyWallet->cs_wallet);
        fReadOK = ReadKeyValue(dummyWallet, ssKey, ssValue,
                               dummyWss, strType, strErr);
    }
    if (!IsKeyType(strType) && strType != DBKeys::HDCHAIN) {
        return false;
    }
    if (!fReadOK)
    {
        LogPrintf("WARNING: WalletBatch::Recover skipping %s: %s\n", strType, strErr);
        return false;
    }

    return true;
}

bool WalletBatch::VerifyEnvironment(const fs::path& wallet_path, std::string& errorStr)
{
    return BerkeleyBatch::VerifyEnvironment(wallet_path, errorStr);
}

bool WalletBatch::VerifyDatabaseFile(const fs::path& wallet_path, std::vector<std::string>& warnings, std::string& errorStr)
{
    return BerkeleyBatch::VerifyDatabaseFile(wallet_path, warnings, errorStr, WalletBatch::Recover);
}

bool WalletBatch::WriteDestData(const std::string &address, const std::string &key, const std::string &value)
{
    return WriteIC(std::make_pair(DBKeys::DESTDATA, std::make_pair(address, key)), value);
}

bool WalletBatch::EraseDestData(const std::string &address, const std::string &key)
{
    return EraseIC(std::make_pair(DBKeys::DESTDATA, std::make_pair(address, key)));
}


bool WalletBatch::WriteHDChain(const CHDChain& chain)
{
    return WriteIC(DBKeys::HDCHAIN, chain);
}

bool WalletBatch::WriteWalletFlags(const uint64_t flags)
{
    return WriteIC(DBKeys::FLAGS, flags);
}

bool WalletBatch::TxnBegin()
{
    return m_batch.TxnBegin();
}

bool WalletBatch::TxnCommit()
{
    return m_batch.TxnCommit();
}

bool WalletBatch::TxnAbort()
{
    return m_batch.TxnAbort();
}
