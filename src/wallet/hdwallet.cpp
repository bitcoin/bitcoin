// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/hdwallet.h"

#include "crypto/hmac_sha256.h"
#include "crypto/hmac_sha512.h"
#include "crypto/sha256.h"

#include "random.h"
#include "base58.h"
#include "validation.h"
#include "consensus/validation.h"
#include "consensus/merkle.h"
#include "smsg/smessage.h"
#include "pos/kernel.h"
#include "pos/miner.h"
#include "utilmoneystr.h"
#include "script/script.h"
#include "script/standard.h"
#include "script/sign.h"
#include "policy/policy.h"
#include "wallet/coincontrol.h"
#include "blind.h"

#include <algorithm>
#include <random>


int CHDWallet::Finalise()
{
    if (fDebug)
        LogPrint("hdwallet", "%s\n", __func__);
    
    FreeExtKeyMaps();
    
    mapAddressBook.clear();
    
    return 0;
};

int CHDWallet::FreeExtKeyMaps()
{
    if (fDebug)
        LogPrint("hdwallet", "%s\n", __func__);
    
    ExtKeyAccountMap::iterator it = mapExtAccounts.begin();
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
        if (it->second)
            delete it->second;
    mapExtAccounts.clear();

    ExtKeyMap::iterator itl = mapExtKeys.begin();
    for (itl = mapExtKeys.begin(); itl != mapExtKeys.end(); ++itl)
        if (itl->second)
            delete itl->second;
    mapExtKeys.clear();
    
    return 0;
};

bool CHDWallet::InitLoadWallet()
{
    if (GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        pwalletMain = NULL;
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    std::string walletFile = GetArg("-wallet", DEFAULT_WALLET_DAT);

    CHDWallet *walletInstance = new CHDWallet(walletFile);

    bool fLegacyWallet = GetBoolArg("-genfirstkey", false); // qa tests
    CHDWallet *const pwallet = (CHDWallet*) CreateWalletFromFile(walletFile, walletInstance, fLegacyWallet);

    if (!pwallet)
        return false;

    if (!ParseMoney(GetArg("-reservebalance", ""),  pwallet->nReserveBalance))
    {
        delete pwallet;
        return InitError(_("Invalid amount for -reservebalance=<amount>"));
    };

    if (pwallet->mapMasterKeys.size() > 0
        && !pwallet->SetCrypted())
    {
        delete pwallet;
        return errorN(0, "SetCrypted failed.");
    };

    pwalletMain = pwallet;

    if (!fLegacyWallet)
    {
        // - Prepare extended keys
        pwallet->ExtKeyLoadMaster();
        pwallet->ExtKeyLoadAccounts();
        pwallet->ExtKeyLoadAccountPacks();
        
        pwallet->LoadStealthAddresses();
        
        fParticlWallet = true;
    };

    {
        LOCK(pwallet->cs_wallet);
        CHDWalletDB wdb(pwallet->strWalletFile);
        
        pwallet->LoadAddressBook(&wdb);
        pwallet->LoadTxRecords(&wdb);
        pwallet->LoadVoteTokens(&wdb);
    }

    return true;
}

bool CHDWallet::LoadAddressBook(CHDWalletDB *pwdb)
{
    LogPrint("hdwallet","Loading address book.\n");
    
    assert(pwdb);
    LOCK(cs_wallet);

    Dbc *pcursor;
    if (!(pcursor = pwdb->GetCursor()))
        throw std::runtime_error(strprintf("%s: cannot create DB cursor", __func__).c_str());

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CBitcoinAddress addr;
    CKeyID idAccount;
    
    std::string sPrefix = "abe";
    std::string strType;
    std::string strAddress;
    
    size_t nCount = 0;
    unsigned int fFlags = DB_SET_RANGE;
    ssKey << sPrefix;
    while (pwdb->ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;
        ssKey >> strType;
        if (strType != sPrefix)
            break;

        ssKey >> strAddress;
        
        CAddressBookData data;
        ssValue >> data;

        std::pair<std::map<CTxDestination, CAddressBookData>::iterator, bool> ret;
        ret = mapAddressBook.insert(std::pair<CTxDestination, CAddressBookData>
            (CBitcoinAddress(strAddress).Get(), data));
        if (!ret.second)
        {
            // update existing record
            CAddressBookData &entry = ret.first->second;
            entry.name = data.name;
            entry.purpose = data.purpose;
            entry.vPath = data.vPath;
        } else
        {
            nCount++;
        }
    };
    
    LogPrint("hdwallet", "Loaded %d addresses.\n", nCount);
    pcursor->close();

    return true;
};

bool CHDWallet::LoadVoteTokens(CHDWalletDB *pwdb)
{
    LogPrint("hdwallet","Loading vote tokens.\n");
    
    LOCK(cs_wallet);
    
    vVoteTokens.clear();
    
    std::vector<CVoteToken> vVoteTokensRead;
    
    if (!pwdb->ReadVoteTokens(vVoteTokensRead))
        return false;
    
    int nBestHeight = chainActive.Height();
    
    for (auto &v : vVoteTokensRead)
    {
        if (v.nEnd > nBestHeight - 1000) // 1000 block buffer incase of reorg etc
        {
            vVoteTokens.push_back(v);
            if (fDebug)
            {
                if ((v.nToken >> 16) < 1
                    || (v.nToken & 0xFFFF) < 1)
                    LogPrint("hdwallet", _("Clearing vote from block %d to %d."),
                        v.nStart, v.nEnd);
                else
                    LogPrint("hdwallet", _("Voting for option %u on proposal %u from block %d to %d."),
                        v.nToken >> 16, v.nToken & 0xFFFF, v.nStart, v.nEnd);
            };
        };
    };
    
    return true;
};

bool CHDWallet::GetVote(int nHeight, uint32_t &token)
{
    for (auto i = vVoteTokens.size(); i-- > 0; )
    {
        auto &v = vVoteTokens[i];
        
        if (v.nEnd < nHeight
            || v.nStart > nHeight)
            continue;
        
        if ((v.nToken >> 16) < 1
            || (v.nToken & 0xFFFF) < 1)
            continue;
        
        token = v.nToken;
        return true;
    };
    
    return false;
};

bool CHDWallet::LoadTxRecords(CHDWalletDB *pwdb)
{
    LogPrint("hdwallet","Loading transaction records.\n");
    
    assert(pwdb);
    LOCK(cs_wallet);
    
    Dbc *pcursor;
    if (!(pcursor = pwdb->GetCursor()))
        throw std::runtime_error(strprintf("%s: cannot create DB cursor", __func__).c_str());

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    
    std::string sPrefix = "rtx";
    std::string strType;
    uint256 txhash;
    
    size_t nCount = 0;
    unsigned int fFlags = DB_SET_RANGE;
    ssKey << sPrefix;
    while (pwdb->ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;
        ssKey >> strType;
        if (strType != sPrefix)
            break;

        ssKey >> txhash;
        
        CTransactionRecord data;
        ssValue >> data;
        LoadToWallet(txhash, data);
        nCount++;
    };
    
    pcursor->close();
    
    LogPrint("hdwallet", "Loaded %d records.\n", nCount);
    
    return true;
};

bool CHDWallet::EncryptWallet(const SecureString &strWalletPassphrase)
{
    if (fDebug)
        LogPrint("hdwallet", "%s\n", __func__);
    
    if (IsCrypted())
        return false;

    CKeyingMaterial vMasterKey;
    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    LogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK2(cs_main, cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        
        if (fFileBacked)
        {
            assert(!pwalletdbEncryption);
            pwalletdbEncryption = new CHDWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin())
            {
                delete pwalletdbEncryption;
                pwalletdbEncryption = NULL;
                return false;
            };
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        }

        if (!EncryptKeys(vMasterKey))
        {
            if (fFileBacked) {
                pwalletdbEncryption->TxnAbort();
                delete pwalletdbEncryption;
            }
            // We now probably have half of our keys encrypted in memory, and half not...
            // die and let the user reload the unencrypted wallet.
            assert(false);
        }
        
        if (0 != ExtKeyEncryptAll((CHDWalletDB*)pwalletdbEncryption, vMasterKey))
        {
            LogPrintf("Terminating - Error: ExtKeyEncryptAll failed.\n");
            if (fFileBacked) {
                pwalletdbEncryption->TxnAbort();
                delete pwalletdbEncryption;
            }
            assert(false); // die and let the user reload the unencrypted wallet.
        };

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, pwalletdbEncryption, true);

        if (fFileBacked)
        {
            if (!pwalletdbEncryption->TxnCommit()) {
                delete pwalletdbEncryption;
                // We now have keys encrypted in memory, but not on disk...
                // die to avoid confusion and let the user reload the unencrypted wallet.
                assert(false);
            }

            delete pwalletdbEncryption;
            pwalletdbEncryption = NULL;
        }

        if (!Lock())
            LogPrintf("%s: ERROR: Lock wallet failed!\n", __func__);
        if (!Unlock(strWalletPassphrase))
            LogPrintf("%s: ERROR: Unlock wallet failed!\n", __func__);
        if (!Lock())
            LogPrintf("%s: ERROR: Lock wallet failed!\n", __func__);

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        CDB::Rewrite(strWalletFile);

    }
    NotifyStatusChanged(this);

    return true;
};

bool CHDWallet::Lock()
{
    if (fDebug)
        LogPrint("hdwallet", "CHDWallet::Lock()\n");

    if (IsLocked())
        return true;

    LogPrintf("Locking wallet.\n");

    {
        LOCK(cs_wallet);
        ExtKeyLock();
    }

    return CCryptoKeyStore::Lock();
};

bool CHDWallet::Unlock(const SecureString &strWalletPassphrase)
{
    if (fDebug)
        LogPrint("hdwallet", "CHDWallet::Unlock()\n");
    
    if (!IsLocked()
        || !IsCrypted())
        return false;

    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    LogPrintf("Unlocking wallet.\n");

    {
        LOCK2(cs_main, cs_wallet);
        BOOST_FOREACH(const MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                continue; // try another master key
            if (!CCryptoKeyStore::Unlock(vMasterKey))
                return false;
            break;
        };

        ExtKeyUnlock(vMasterKey);
        ProcessLockedStealthOutputs();
        //ProcessLockedAnonOutputs();
        SecureMsgWalletUnlocked();
        
    } // cs_main, cs_wallet
    
    return true;
};

bool CHDWallet::HaveAddress(const CBitcoinAddress &address)
{
    LOCK(cs_wallet);
    
    CTxDestination dest = address.Get();
    if (dest.type() == typeid(CKeyID))
    {
        CKeyID id = boost::get<CKeyID>(dest);
        return HaveKey(id);
    };
    
    if (dest.type() == typeid(CExtKeyPair))
    {
        CExtKeyPair ek = boost::get<CExtKeyPair>(dest);
        CKeyID id = ek.GetID();
        return HaveExtKey(id);
    };
     
    if (dest.type() == typeid(CStealthAddress))
    {
        CStealthAddress sx = boost::get<CStealthAddress>(dest);
        return HaveStealthAddress(sx);
    };
    
    return false;
};

bool CHDWallet::HaveKey(const CKeyID &address, CEKAKey &ak, CExtKeyAccount *pa) const
{
    if (fDebug)
        AssertLockHeld(cs_wallet);
    //LOCK(cs_wallet);

    int rv;
    ExtKeyAccountMap::const_iterator it;
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
    {
        pa = it->second;
        rv = pa->HaveKey(address, true, ak);
        if (rv == 1)
            return true;
        if (rv == 3)
        {
            if (0 != ExtKeySaveKey(it->second, address, ak))
                return error("%s: ExtKeySaveKey failed.", __func__);
            return true;
        };
    };
    
    pa = NULL;
    return CCryptoKeyStore::HaveKey(address);
};

bool CHDWallet::HaveKey(const CKeyID &address) const
{
    //AssertLockHeld(cs_wallet);
    LOCK(cs_wallet);
    
    CEKAKey ak;
    CExtKeyAccount *pa = NULL;
    return HaveKey(address, ak, pa);
};

bool CHDWallet::HaveExtKey(const CKeyID &keyID) const
{
    LOCK(cs_wallet);

    // NOTE: This only checks keys currently in memory (mapExtKeys)
    //       There may be other extkeys in the db.

    ExtKeyMap::const_iterator it = mapExtKeys.find(keyID);
    if (it != mapExtKeys.end())
        return true;

    return false;
};

bool CHDWallet::GetKey(const CKeyID &address, CKey &keyOut) const
{
    LOCK(cs_wallet);

    ExtKeyAccountMap::const_iterator it;
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
    {
        if (it->second->GetKey(address, keyOut))
            return true;
    };
    return CCryptoKeyStore::GetKey(address, keyOut);
};

bool CHDWallet::GetPubKey(const CKeyID &address, CPubKey& pkOut) const
{
    LOCK(cs_wallet);
    ExtKeyAccountMap::const_iterator it;
    for (it = mapExtAccounts.begin(); it != mapExtAccounts.end(); ++it)
    {
        if (it->second->GetPubKey(address, pkOut))
            return true;
    };

    return CCryptoKeyStore::GetPubKey(address, pkOut);
};

bool CHDWallet::HaveStealthAddress(const CStealthAddress &sxAddr) const
{
    if (fDebug)
    {
        AssertLockHeld(cs_wallet);
    };

    if (stealthAddresses.count(sxAddr))
        return true;

    CKeyID sxId = CPubKey(sxAddr.scan_pubkey).GetID();

    ExtKeyAccountMap::const_iterator mi;
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *ea = mi->second;

        if (ea->mapStealthKeys.size() < 1)
            continue;

        AccStealthKeyMap::iterator it = ea->mapStealthKeys.find(sxId);
        if (it != ea->mapStealthKeys.end())
            return true;
    };

    return false;
};

bool CHDWallet::ImportStealthAddress(const CStealthAddress &sxAddr, const CKey &skSpend)
{
    if (fDebug)
        LogPrint("hdwallet", "%s: %s.\n", __func__, sxAddr.Encoded());

    LOCK(cs_wallet);

    // - must add before changing spend_secret
    stealthAddresses.insert(sxAddr);

    bool fOwned = skSpend.IsValid();

    if (fOwned)
    {
        // -- owned addresses can only be added when wallet is unlocked
        if (IsLocked())
        {
            stealthAddresses.erase(sxAddr);
            return error("%s: Wallet must be unlocked.", __func__);
        };

        CPubKey pk = skSpend.GetPubKey();
        if (!AddKeyPubKey(skSpend, pk))
        {
            stealthAddresses.erase(sxAddr);
            return error("%s: AddKeyPubKey failed.", __func__);
        };
    };

    if (!CHDWalletDB(strWalletFile).WriteStealthAddress(sxAddr))
    {
        stealthAddresses.erase(sxAddr);
        return error("%s: WriteStealthAddress failed.", __func__);
    };

    return true;
};

bool CHDWallet::AddressBookChangedNotify(const CTxDestination &address, ChangeType nMode)
{
    // Must run without cs_wallet locked
    
    CAddressBookData entry;
    isminetype tIsMine;
    
    {
        LOCK(cs_wallet);
       
        std::map<CTxDestination, CAddressBookData>::const_iterator mi = mapAddressBook.find(address);
        if (mi == mapAddressBook.end())
            return false;
        entry = mi->second;
        
        tIsMine = ::IsMine(*this, address);
    }
    
    NotifyAddressBookChanged(this, address, entry.name, tIsMine != ISMINE_NO, entry.purpose, nMode);

    if (tIsMine == ISMINE_SPENDABLE
        && address.type() == typeid(CKeyID))
    {
        CKeyID id = boost::get<CKeyID>(address);
        SecureMsgWalletKeyChanged(id, entry.name, nMode);
    };
    
    return true;
};

bool CHDWallet::SetAddressBook(CHDWalletDB *pwdb, const CTxDestination &address, const std::string &strName,
    const std::string &strPurpose, const std::vector<uint32_t> &vPath, bool fNotifyChanged)
{
    ChangeType nMode;
    isminetype tIsMine;
    
    CAddressBookData *entry;
    {
        LOCK(cs_wallet); // mapAddressBook
        
        CAddressBookData emptyEntry;
        std::pair<std::map<CTxDestination, CAddressBookData>::iterator, bool> ret;
        ret = mapAddressBook.insert(std::pair<CTxDestination, CAddressBookData>(address, emptyEntry));
        nMode = (ret.second) ? CT_NEW : CT_UPDATED;
        
        entry = &ret.first->second;
        
        entry->name = strName;
        entry->vPath = vPath;
        
        tIsMine = ::IsMine(*this, address);
        if (!strPurpose.empty()) /* update purpose only if requested */
            entry->purpose = strPurpose;
        
        if (fFileBacked)
        {
            if (pwdb)
            {
                if (!pwdb->WriteAddressBookEntry(CBitcoinAddress(address).ToString(), *entry))
                    return false;
            } else
            {
                if (!CHDWalletDB(strWalletFile).WriteAddressBookEntry(CBitcoinAddress(address).ToString(), *entry))
                    return false;
            };
        };
    }
    
    if (fNotifyChanged)
    {
        // Must run without cs_wallet locked
        NotifyAddressBookChanged(this, address, strName, tIsMine != ISMINE_NO, strPurpose, nMode);

        if (tIsMine == ISMINE_SPENDABLE
            && address.type() == typeid(CKeyID))
        {
            CKeyID id = boost::get<CKeyID>(address);
            SecureMsgWalletKeyChanged(id, strName, nMode);
        };
    };
    
    return true;
};

bool CHDWallet::SetAddressBook(const CTxDestination &address, const std::string &strName, const std::string &purpose)
{
    bool fOwned;
    ChangeType nMode;
    
    {
        LOCK(cs_wallet); // mapAddressBook
        std::map<CTxDestination, CAddressBookData>::iterator mi = mapAddressBook.find(address);
        nMode = (mi == mapAddressBook.end()) ? CT_NEW : CT_UPDATED;
        fOwned = ::IsMine(*this, address) == ISMINE_SPENDABLE;
    }

    if (fOwned
        && address.type() == typeid(CKeyID))
    {
        CKeyID id = boost::get<CKeyID>(address);
        SecureMsgWalletKeyChanged(id, strName, nMode);
    };
    
    return CWallet::SetAddressBook(address, strName, purpose);
};

bool CHDWallet::DelAddressBook(const CTxDestination &address)
{
    if (address.type() == typeid(CStealthAddress))
    {
        const CStealthAddress &sxAddr = boost::get<CStealthAddress>(address);
        bool fOwned; // must check on copy from wallet

        {
            LOCK(cs_wallet);

            std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxAddr);
            if (si == stealthAddresses.end())
            {
                LogPrintf("%s: Error: Stealth address not found in wallet.\n", __func__);
                return false;
            };

            fOwned = si->scan_secret.size() < 32 ? false : true;

            if (stealthAddresses.erase(sxAddr) < 1
                || !CHDWalletDB(strWalletFile).EraseStealthAddress(sxAddr))
            {
                LogPrintf("%s: Error: Remove stealthAddresses failed.\n", __func__);
                return false;
            };
        }
        
        NotifyAddressBookChanged(this, address, "", fOwned, "", CT_DELETED);
        return true;
    };
    
    if (::IsMine(*this, address) == ISMINE_SPENDABLE
        && address.type() == typeid(CKeyID))
    {
        CKeyID id = boost::get<CKeyID>(address);
        SecureMsgWalletKeyChanged(id, "", CT_DELETED);
    };
    
    return CWallet::DelAddressBook(address);
};

isminetype CHDWallet::IsMine(const CScript &scriptPubKey, CKeyID &keyID,
    CEKAKey &ak, CExtKeyAccount *pa, bool &isInvalid, SigVersion sigversion)
{
    
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    
    if (!Solver(scriptPubKey, whichType, vSolutions)) {
        if (HaveWatchOnly(scriptPubKey))
            return ISMINE_WATCH_UNSOLVABLE;
        return ISMINE_NO;
    }

    switch (whichType)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
        break;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if (sigversion != SIGVERSION_BASE && vSolutions[0].size() != 33) {
            isInvalid = true;
            return ISMINE_NO;
        }
        if (HaveKey(keyID, ak, pa))
            return ISMINE_SPENDABLE;
        break;
    case TX_PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (sigversion != SIGVERSION_BASE) {
            CPubKey pubkey;
            if (GetPubKey(keyID, pubkey) && !pubkey.IsCompressed()) {
                isInvalid = true;
                return ISMINE_NO;
            }
        }
        if (HaveKey(keyID, ak, pa))
            return ISMINE_SPENDABLE;
        break;
    case TX_SCRIPTHASH:
    {
        CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
        CScript subscript;
        if (GetCScript(scriptID, subscript)) {
            isminetype ret = ::IsMine(*((CKeyStore*)this), subscript, isInvalid);
            if (ret == ISMINE_SPENDABLE || ret == ISMINE_WATCH_SOLVABLE || (ret == ISMINE_NO && isInvalid))
                return ret;
        }
        break;
    }
    case TX_WITNESS_V0_KEYHASH:
    case TX_WITNESS_V0_SCRIPTHASH:
        LogPrintf("%s: Ignoring TX_WITNESS script type.\n"); // shouldn't happen
        return ISMINE_NO;

    case TX_MULTISIG:
    {
        // Only consider transactions "mine" if we own ALL the
        // keys involved. Multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        std::vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        if (sigversion != SIGVERSION_BASE) {
            for (size_t i = 0; i < keys.size(); i++) {
                if (keys[i].size() != 33) {
                    isInvalid = true;
                    return ISMINE_NO;
                }
            }
        }
        if (HaveKeys(keys, *((CKeyStore*)this)) == keys.size())
            return ISMINE_SPENDABLE;
        break;
    }
    }

    if (HaveWatchOnly(scriptPubKey)) {
        // TODO: This could be optimized some by doing some work after the above solver
        SignatureData sigs;
        return ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigs) ? ISMINE_WATCH_SOLVABLE : ISMINE_WATCH_UNSOLVABLE;
    }
    return ISMINE_NO;
};

isminetype CHDWallet::IsMine(const CTxOutBase *txout) const
{
    switch (txout->nVersion)
    {
        case OUTPUT_STANDARD:
            return ::IsMine(*this, ((CTxOutStandard*)txout)->scriptPubKey);
        case OUTPUT_CT:
            return ::IsMine(*this, ((CTxOutCT*)txout)->scriptPubKey);
        case OUTPUT_RINGCT:
        default:
            return ISMINE_NO;
    };
    
    return ISMINE_NO;
};

bool CHDWallet::IsMine(const CTransaction &tx) const
{
    for (const auto txout : tx.vpout)
        if (IsMine(txout.get()))
            return true;
    
    for (const auto &txout : tx.vout)
        if (CWallet::IsMine(txout))
            return true;
    return false;
}

bool CHDWallet::IsFromMe(const CTransaction& tx) const
{
    return (GetDebit(tx, ISMINE_ALL) > 0);
}

// Note that this function doesn't distinguish between a 0-valued input,
// and a not-"is mine" (according to the filter) input.
CAmount CHDWallet::GetDebit(const CTxIn &txin, const isminefilter &filter) const
{
    {
        LOCK(cs_wallet);
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx &prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vpout.size())
                if (IsMine(prev.tx->vpout[txin.prevout.n].get()) & filter)
                    return prev.tx->vpout[txin.prevout.n]->GetValue();
        };
    } // cs_wallet
    return 0;
};

CAmount CHDWallet::GetDebit(const CTransaction& tx, const isminefilter& filter) const
{
    if (!tx.IsParticlVersion())
        return CWallet::GetDebit(tx, filter);
    
    CAmount nDebit = 0;
    for (auto &txin : tx.vin)
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    };
    return nDebit;
};

CAmount CHDWallet::GetCredit(const CTxOutBase *txout, const isminefilter &filter) const
{
    CAmount nValue = 0;
    switch (txout->nVersion)
    {
        case OUTPUT_STANDARD:
            nValue = ((CTxOutStandard*)txout)->nValue;
            break;
        case OUTPUT_CT:
        case OUTPUT_RINGCT:
        default:
            return 0;
    };
    
    if (!MoneyRange(nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return ((IsMine(txout) & filter) ? nValue : 0);
}

CAmount CHDWallet::GetCredit(const CTransaction &tx, const isminefilter &filter) const
{
    CAmount nCredit = 0;
    
    for (const auto txout : tx.vpout)
    {
        nCredit += GetCredit(txout.get(), filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    };
    
    // - legacy txns
    for (const auto &txout : tx.vout)
    {
        nCredit += CWallet::GetCredit(txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nCredit;
}

bool CHDWallet::InMempool(const uint256 &hash) const
{
    
    LOCK(mempool.cs);
    return mempool.exists(hash);
};

bool hashUnset(const uint256 &hash)
{
    return (hash.IsNull() || hash == ABANDON_HASH);
}

int CHDWallet::GetDepthInMainChain(const uint256 &blockhash) const
{
     
    if (hashUnset(blockhash))
        return 0;

    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    BlockMap::iterator mi = mapBlockIndex.find(blockhash);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex *pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    //pindexRet = pindex;
    return (chainActive.Height() - pindex->nHeight + 1);
    //return ((nIndex == -1) ? (-1) : 1) * (chainActive.Height() - pindex->nHeight + 1);
};

bool CHDWallet::IsTrusted(const uint256 &txhash, const uint256 &blockhash) const
{
    
    //if (!CheckFinalTx(*this))
    //    return false;
    //if (tx->IsCoinStake() && hashUnset()) // ignore failed stakes
    //    return false;
    int nDepth = GetDepthInMainChain(blockhash);
    if (nDepth >= 1)
        return true;
    if (nDepth < 0)
        return false;
    //if (!bSpendZeroConfChange || !IsFromMe(ISMINE_ALL)) // using wtx's cached debit
    //    return false;
    
    // Don't trust unconfirmed transactions from us unless they are in the mempool.
    CTransactionRef ptx = mempool.get(txhash);
    if (!ptx)
        return false;
    
    // Trusted if all inputs are from us and are in the mempool:
    for (const auto &txin : ptx->vin)
    {
        // Transactions not sent by us: not trusted
        MapRecords_t::const_iterator rit = mapRecords.find(txin.prevout.hash);
        if (rit != mapRecords.end())
        {
            const COutputRecord *oR = rit->second.GetOutput(txin.prevout.n);
            
            if (!oR
                || !(oR->nFlags & ORF_OWNED))
                return false;
            continue;
        };
        
        const CWalletTx *parent = GetWalletTx(txin.prevout.hash);
        if (parent == NULL)
            return false;
        
        const CTxOutBase *parentOut = parent->tx->vpout[txin.prevout.n].get();
        if (IsMine(parentOut) != ISMINE_SPENDABLE)
            return false;
    }
    
    return true;
};


CAmount CHDWallet::GetBalance() const
{
    
    CAmount nBalance = 0;
    
    LOCK2(cs_main, cs_wallet);
    
    for (const auto &ri : mapRecords)
    {
        const auto &txhash = ri.first;
        const auto &rtx = ri.second;
        if (!IsTrusted(txhash, rtx.blockHash))
            continue;
        
        for (const auto &r : rtx.vout)
        {
            if (r.nFlags & ORF_OWNED && !IsSpent(txhash, r.n))
                nBalance += r.nValue;
        };
        
        if (!MoneyRange(nBalance))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    };
    
    nBalance += CWallet::GetBalance();
    return nBalance;
};

CAmount CHDWallet::GetUnconfirmedBalance() const
{
    
    CAmount nBalance = 0;
    
    LOCK2(cs_main, cs_wallet);
    
    for (const auto &ri : mapRecords)
    {
        const auto &txhash = ri.first;
        const auto &rtx = ri.second;
        
        if (IsTrusted(txhash, rtx.blockHash))
            continue;
        
        for (const auto &r : rtx.vout)
        {
            if (r.nFlags & ORF_OWNED && !IsSpent(txhash, r.n))
                nBalance += r.nValue;
        };
        
        if (!MoneyRange(nBalance))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    };
    
    nBalance += CWallet::GetUnconfirmedBalance();
    return nBalance;
};

CAmount CHDWallet::GetBlindBalance()
{
    LogPrintf("%s: TODO\n", __func__);
    return 0;
};

CAmount CHDWallet::GetAnonBalance()
{
    LogPrintf("%s: TODO\n", __func__);
    return 0;
};

/**
 * total coins staked (non-spendable until maturity)
 */
CAmount CHDWallet::GetStaked()
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (WalletTxMap::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        CWalletTx *pcoin = &(*it).second;
        
        if (pcoin->IsCoinStake()
            && pcoin->GetDepthInMainChainCached() > 0 // checks for hashunset
            && pcoin->GetBlocksToMaturity() > 0)
        {
            nTotal += CHDWallet::GetCredit(*pcoin, ISMINE_SPENDABLE);
        }
    };
    return nTotal;
};

bool CHDWallet::IsChange(const CTxOutBase *txout) const
{
    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    if (txout->IsStandardOutput())
    {
        const CScript &scriptPubKey = ((CTxOutStandard*)txout)->scriptPubKey;
        if (::IsMine(*this, scriptPubKey))
        {
            CTxDestination address;
            if (!ExtractDestination(scriptPubKey, address))
                return true;

            LOCK(cs_wallet);
            if (!mapAddressBook.count(address))
                return true;
        };
    };
    return false;
    
    
};

int CHDWallet::GetChangeAddress(CPubKey &pk)
{
    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idDefaultAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s Unknown account.", __func__);

    // - Return a key from the lookahead of the internal chain
    //   Don't promote the key to the main map, that will happen when the transaction is processed.

    CStoredExtKey *pc;
    if ((pc = mi->second->ChainInternal()) == NULL)
        return errorN(1, "%s Unknown chain.", __func__);

    uint32_t nChild;
    if (0 != pc->DeriveNextKey(pk, nChild, false, false))
        return errorN(1, "%s TryDeriveNext failed.", __func__);

    if (fDebug)
    {
        CBitcoinAddress addr(pk.GetID());
        LogPrint("hdwallet", "Change Address: %s\n", addr.ToString().c_str());
    };

    return 0;
};

static void AddOutputRecordMetaData(CTransactionRecord &rtx, std::vector<CTempRecipient> &vecSend)
{
    for (auto &r : vecSend)
    {
        if (r.nType == OUTPUT_CT)
        {
            COutputRecord rec;
            
            rec.n = r.n;
            rec.nType = r.nType;
            rec.nValue = r.nAmount;
            rec.nFlags |= ORF_FROM;
            rec.sNarration = r.sNarration;
            
            if (r.address.type() == typeid(CStealthAddress))
            {
                CStealthAddress sx = boost::get<CStealthAddress>(r.address);
                //ORA_STEALTH
                
            } else
            if (r.address.type() == typeid(CExtKeyPair))
            {
                CExtKeyPair ek = boost::get<CExtKeyPair>(r.address);
                //ORA_EXTKEY
                
            } else
            if (r.address.type() == typeid(CKeyID))
            {
                //ORA_STANDARD
                
                
            };
            rtx.InsertOutput(rec);
        };
    };
};

int CHDWallet::ExpandTempRecipients(std::vector<CTempRecipient> &vecSend, CStoredExtKey *pc, std::string &sError)
{
    LOCK(cs_wallet);
    uint32_t nChild;
    for (size_t i = 0; i < vecSend.size(); ++i)
    {
        CTempRecipient &r = vecSend[i];
        
        if (r.nType == OUTPUT_STANDARD)
        {
            if (r.address.type() == typeid(CStealthAddress))
            {
                CStealthAddress sx = boost::get<CStealthAddress>(r.address);
                
                CKey sShared;
                ec_point pkSendTo;
                
                int k, nTries = 24;
                for (k = 0; k < nTries; ++k) // if StealthSecret fails try again with new ephem key
                {
                    r.sEphem.MakeNewKey(true);
                    if (StealthSecret(r.sEphem, sx.scan_pubkey, sx.spend_pubkey, sShared, pkSendTo) == 0)
                        break;
                };
                if (k >= nTries)
                    return errorN(1, sError, __func__, "Could not generate receiving public key.");
                
                r.sEphem.GetPubKey();
                CPubKey pkEphem = r.sEphem.GetPubKey();
                r.pkTo = CPubKey(pkSendTo);
                CKeyID idTo = r.pkTo.GetID();
                r.scriptPubKey = GetScriptForDestination(idTo);
                
                if (fDebug)
                    LogPrint("hdwallet", "Stealth send to generated address: %s\n", CBitcoinAddress(idTo).ToString());
                
                
                CTempRecipient rd;
                rd.nType = OUTPUT_DATA;
                
                std::vector<uint8_t> vchNarr;
                if (r.sNarration.length() > 0)
                {
                    SecMsgCrypter crypter;
                    crypter.SetKey(sShared.begin(), pkEphem.begin());
                    
                    if (!crypter.Encrypt((uint8_t*)r.sNarration.data(), r.sNarration.length(), vchNarr))
                        return errorN(1, sError, __func__, "Narration encryption failed.");
                    
                    if (vchNarr.size() > MAX_STEALTH_NARRATION_SIZE)
                        return errorN(1, sError, __func__, "Encrypted narration is too long.");
                };
                
                rd.vData.resize(34
                    + (sx.prefix.number_bits > 0 ? 5 : 0)
                    + (vchNarr.size() + (vchNarr.size() > 0 ? 1 : 0)));
                
                size_t o = 0;
                rd.vData[o++] = DO_STEALTH;
                memcpy(&rd.vData[o], pkEphem.begin(), 33);
                o += 33;
                
                if (sx.prefix.number_bits > 0)
                {
                    rd.vData[o++] = DO_STEALTH_PREFIX;
                    uint32_t prefix, mask = SetStealthMask(sx.prefix.number_bits);
                    GetStrongRandBytes((uint8_t*) &prefix, 4);
                    
                    prefix = prefix & (~mask);
                    prefix |= sx.prefix.bitfield & mask;
                    
                    memcpy(&rd.vData[o], &prefix, 4);
                    o+=4;
                };
                
                if (vchNarr.size() > 0)
                {
                    rd.vData[o++] = DO_NARR_CRYPT;
                    memcpy(&rd.vData[o], &vchNarr[0], vchNarr.size());
                    o += vchNarr.size();
                };
                
                vecSend.insert(vecSend.begin() + (i+1), rd);
                i++; // skip over inserted output
            } else
            {
                if (r.address.type() == typeid(CExtKeyPair))
                {
                    CExtKeyPair ek = boost::get<CExtKeyPair>(r.address);
                    CExtKey58 ek58;
                    ek58.SetKeyP(ek);
                    CPubKey pkDest;
                    uint32_t nChildKey;
                    if (0 != ((CHDWallet*)pwalletMain)->ExtKeyGetDestination(ek, pkDest, nChildKey))
                        return errorN(1, sError, __func__, "ExtKeyGetDestination failed.");
                    
                    r.nChildKey = nChildKey;
                    r.pkTo = pkDest;
                    r.scriptPubKey = GetScriptForDestination(pkDest.GetID());
                } else
                {
                    r.scriptPubKey = GetScriptForDestination(r.address);
                };
                
                if (r.sNarration.length() > 0)
                {
                    CTempRecipient rd;
                    rd.nType = OUTPUT_DATA;
                    
                    std::vector<uint8_t> vNarr();
                    rd.vData.push_back(DO_NARR_PLAIN);
                    std::copy(r.sNarration.begin(), r.sNarration.end(), std::back_inserter(rd.vData));
                    
                    vecSend.insert(vecSend.begin() + (i+1), rd);
                    i++; // skip over inserted output
                };
            };
        } else
        if (r.nType == OUTPUT_CT)
        {
            CKey sEphem;
            
            if (0 != pc->DeriveNextKey(sEphem, nChild, true))
                return errorN(1, sError, __func__, "TryDeriveNext failed.");
            
            if (r.address.type() == typeid(CStealthAddress))
            {
                CStealthAddress sx = boost::get<CStealthAddress>(r.address);
                
                CKey sShared;
                ec_point pkSendTo;
                
                int k, nTries = 24;
                for (k = 0; k < nTries; ++k)
                {
                    if (StealthSecret(sEphem, sx.scan_pubkey, sx.spend_pubkey, sShared, pkSendTo) == 0)
                        break;
                    // if StealthSecret fails try again with new ephem key
                    if (0 != pc->DeriveNextKey(sEphem, nChild, true))
                        return errorN(1, sError, __func__, "DeriveNextKey failed.");
                };
                if (k >= nTries)
                    return errorN(1, sError, __func__, "Could not generate receiving public key.");
                
                r.pkTo = CPubKey(pkSendTo);
                CKeyID idTo = r.pkTo.GetID();
                
                if (fDebug)
                    LogPrint("hdwallet", "Creating output to stealth generated address: %s\n", CBitcoinAddress(idTo).ToString()); // TODO: add output type
                
                r.scriptPubKey = GetScriptForDestination(idTo);
            } else
            if (r.address.type() == typeid(CExtKeyPair))
            {
                CExtKeyPair ek = boost::get<CExtKeyPair>(r.address);
                CExtKey58 ek58;
                ek58.SetKeyP(ek);
                uint32_t nDestChildKey;
                if (0 != ((CHDWallet*)pwalletMain)->ExtKeyGetDestination(ek, r.pkTo, nDestChildKey))
                    return errorN(1, sError, __func__, "ExtKeyGetDestination failed.");
                
                r.nChildKey = nDestChildKey;
                r.scriptPubKey = GetScriptForDestination(r.pkTo.GetID());
            } else
            if (r.address.type() == typeid(CKeyID))
            {
                // Need a matching public key
                CKeyID idTo = boost::get<CKeyID>(r.address);
                r.scriptPubKey = GetScriptForDestination(idTo);
                
                if (!GetPubKey(idTo, r.pkTo))
                {
                    if (0 != SecureMsgGetStoredKey(idTo, r.pkTo))
                        return errorN(1, sError, __func__, _("No public key found for address %s.").c_str(), CBitcoinAddress(idTo).ToString());
                };
            } else
            {
                return errorN(1, sError, __func__, _("Unknown address type.").c_str());
            };
            
            r.sEphem = sEphem;
        };
    };
    
    return 0;
};

/** Update wallet after successfull transaction */
int CHDWallet::PostProcessTempRecipients(std::vector<CTempRecipient> &vecSend)
{
    LOCK(cs_wallet);
    for (size_t i = 0; i < vecSend.size(); ++i)
    {
        CTempRecipient &r = vecSend[i];
        
        if (r.address.type() == typeid(CExtKeyPair))
        {
            CExtKeyPair ek = boost::get<CExtKeyPair>(r.address);
            r.nChildKey+=1;
            ExtKeyUpdateLooseKey(ek, r.nChildKey, true);
        };
    };
    
    return 0;
};

int CHDWallet::AddStandardInputs(CWalletTx &wtx, CTransactionRecord &rtx,
    std::vector<CTempRecipient> &vecSend,
    CExtKeyAccount *sea, CStoredExtKey *pc,
    bool sign, std::string &sError)
{
    CAmount nFeeRet = 0;
    CAmount nValue = 0;
    for (auto &r : vecSend)
        nValue += r.nAmount;
    
    
    if (0 != ExpandTempRecipients(vecSend, pc, sError))
        return 0; // sError is set
    
    wtx.fTimeReceivedIsTxTime = true;
    wtx.BindWallet(this);
    wtx.fFromMe = true;
    CMutableTransaction txNew;
    txNew.nVersion = PARTICL_TXN_VERSION;
    txNew.vout.clear();
    
    // Discourage fee sniping. See CWallet::CreateTransaction
    txNew.nLockTime = chainActive.Height();
    
    // 1/10 chance of random time further back to increase privacy
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));
    
    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
    
    {
        std::set<std::pair<const CWalletTx*,unsigned int> > setCoins;
        LOCK2(cs_main, cs_wallet);
        std::vector<COutput> vAvailableCoins;
        AvailableCoins(vAvailableCoins, true);
        
        // Start with no fee and loop until there is enough fee
        for (;;)
        {
            txNew.vin.clear();
            txNew.vpout.clear();
            wtx.fFromMe = true;
            
            CAmount nValueToSelect = nValue;
            
            double dPriority = 0;
            
            // Choose coins to use
            CAmount nValueIn = 0;
            setCoins.clear();
            if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn))
                return errorN(1, sError, __func__, _("Insufficient funds.").c_str());
            
            for (const auto &pcoin : setCoins)
            {
                CAmount nCredit = pcoin.first->tx->vpout[pcoin.second]->GetValue();
                //The coin age after the next block (depth+1) is used instead of the current,
                //reflecting an assumption the user would accept a bit more delay for
                //a chance at a free transaction.
                //But mempool inputs might still be in the mempool, so their age stays 0
                int age = pcoin.first->GetDepthInMainChain();
                assert(age >= 0);
                if (age != 0)
                    age += 1;
                dPriority += (double)nCredit * age;
            };
            
            const CAmount nChange = nValueIn - nValueToSelect;
            
            // Remove last fee output
            for (size_t i = 0; i < vecSend.size(); ++i)
            {
                if (vecSend[i].fChange)
                {
                    vecSend.erase(vecSend.begin() + i);
                    break;
                };
            };
            
            if (nChange > 0)
            {
                // Fill an output to ourself
                CTxOutStandard tempOut;
                CPubKey pkChange;
                if (0 != GetChangeAddress(pkChange))
                    return errorN(1, sError, __func__, "GetChangeAddress failed.");
                
                CTempRecipient r;
                r.nType = OUTPUT_STANDARD;
                r.nAmount = nChange;
                
                CKeyID idChange = pkChange.GetID();
                r.address = idChange;
                r.scriptPubKey = GetScriptForDestination(idChange);
                tempOut.scriptPubKey = r.scriptPubKey;
                
                tempOut.nValue = nChange;
                
                // Never create dust outputs; if we would, just
                // add the dust to the fee.
                if (tempOut.IsDust(dustRelayFee))
                {
                    //nChangePosInOut = -1;
                    nFeeRet += nChange;
                } else
                {
                    r.fChange = true;
                    vecSend.push_back(r);
                };
            };
            
            // Fill vin
            //
            // Note how the sequence number is set to non-maxint so that
            // the nLockTime set above actually works.
            //
            // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
            // we use the highest possible value in that range (maxint-2)
            // to avoid conflicting with other possible uses of nSequence,
            // and in the spirit of "smallest possible change from prior
            // behavior."
            for (const auto &coin : setCoins)
                txNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second,CScript(),
                    std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
            
            // The fee will come out of the change
            CAmount nValueOutPlain = 0;
            
            int nLastBlindedOutput = -1;
            int nChangePosInOut = -1;
            
            
            OUTPUT_PTR<CTxOutData> outFee = MAKE_OUTPUT<CTxOutData>();
            outFee->vData.push_back(DO_FEE);
            outFee->vData.resize(9); // resize to more bytes than varint fee could take
            txNew.vpout.push_back(outFee);
            
            for (size_t i = 0; i < vecSend.size(); ++i)
            {
                auto &r = vecSend[i];
                
                if (r.nType == OUTPUT_DATA)
                {
                    OUTPUT_PTR<CTxOutData> txout = MAKE_OUTPUT<CTxOutData>();
                    txout->vData = r.vData;
                    
                    r.n = txNew.vpout.size();
                    txNew.vpout.push_back(txout);
                } else
                if (r.nType == OUTPUT_STANDARD)
                {
                    if (r.fChange)
                        nChangePosInOut = i;
                    
                    nValueOutPlain += r.nAmount;
                    OUTPUT_PTR<CTxOutStandard> txout = MAKE_OUTPUT<CTxOutStandard>(r.nAmount, r.scriptPubKey);
                    
                    r.n = txNew.vpout.size();
                    txNew.vpout.push_back(txout);
                } else
                if (r.nType == OUTPUT_CT)
                {
                    nLastBlindedOutput = i;
                    //vBlindedOutputOffsets.push_back(i);
                    OUTPUT_PTR<CTxOutCT> txout = MAKE_OUTPUT<CTxOutCT>();
                    
                    txout->vData.resize(33);
                    CPubKey pkEphem = r.sEphem.GetPubKey();
                    memcpy(&txout->vData[0], pkEphem.begin(), 33);
                    
                    txout->scriptPubKey = r.scriptPubKey;
                    
                    r.n = txNew.vpout.size();
                    txNew.vpout.push_back(txout);
                };
            };
            
            
            std::vector<uint8_t*> vpBlinds;
            std::vector<uint8_t> vBlindPlain;
            
            size_t nBlindedInputs = 1;
            secp256k1_pedersen_commitment plainCommitment;
            secp256k1_pedersen_commitment plainInputCommitment;
            
            
            
            vBlindPlain.resize(32);
            memset(&vBlindPlain[0], 0, 32);
            vpBlinds.push_back(&vBlindPlain[0]);
            if (!secp256k1_pedersen_commit(secp256k1_ctx_blind, &plainInputCommitment, &vBlindPlain[0], (uint64_t) nValueIn, secp256k1_generator_h))
                return errorN(1, sError, __func__, "secp256k1_pedersen_commit failed for plain in.");
            
            
            if (nValueOutPlain > 0)
            {
                vpBlinds.push_back(&vBlindPlain[0]);
                
                if (!secp256k1_pedersen_commit(secp256k1_ctx_blind, &plainCommitment, &vBlindPlain[0], (uint64_t) nValueOutPlain, secp256k1_generator_h))
                    return errorN(1, sError, __func__, "secp256k1_pedersen_commit failed for plain out.");
            };
            
            for (size_t i = 0; i < vecSend.size(); ++i)
            {
                auto &r = vecSend[i];
                
                if (r.nType == OUTPUT_CT)
                {
                    r.vBlind.resize(32);
                    
                    if ((int)i == nLastBlindedOutput)
                    {
                        
                        // Last to-be-blinded value: compute from all other blinding factors.
                        // sum of output blinding values must equal sum of input blinding values
                        if (!secp256k1_pedersen_blind_sum(secp256k1_ctx_blind, &r.vBlind[0], &vpBlinds[0], vpBlinds.size(), nBlindedInputs))
                            return errorN(1, sError, __func__, "secp256k1_pedersen_blind_sum failed.");
                    } else
                    {
                        GetStrongRandBytes(&r.vBlind[0], 32);
                        vpBlinds.push_back(&r.vBlind[0]);
                    };
                    
                    
                    
                    assert(r.n < (int)txNew.vpout.size());
                    CTxOutCT *pout = (CTxOutCT*) txNew.vpout[r.n].get();
                    
                    uint64_t nValue = r.nAmount;
                    if (!secp256k1_pedersen_commit(secp256k1_ctx_blind,
                        &pout->commitment, (uint8_t*)&r.vBlind[0],
                        nValue, secp256k1_generator_h))
                        return errorN(1, sError, __func__, "secp256k1_pedersen_commit failed.");
                    
                    
                    uint256 nonce = r.sEphem.ECDH(r.pkTo);
                    CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());
                    
                    const char *message = r.sNarration.c_str();
                    size_t mlen = strlen(message);
                    
                    size_t nRangeProofLen = 5134;
                    pout->vRangeproof.resize(nRangeProofLen);
                    
                    // TODO: rangeproof parameter selection
                    
                    uint64_t min_value = 0;
                    int ct_exponent = 2;
                    int ct_bits = 32;
                    
                    if (1 != secp256k1_rangeproof_sign(secp256k1_ctx_blind,
                        &pout->vRangeproof[0], &nRangeProofLen,
                        min_value, &pout->commitment,
                        &r.vBlind[0], nonce.begin(),
                        ct_exponent, ct_bits, 
                        nValue,
                        (const unsigned char*) message, mlen,
                        NULL, 0,
                        secp256k1_generator_h))
                        return errorN(1, sError, __func__, "secp256k1_rangeproof_sign failed.");
                    
                    pout->vRangeproof.resize(nRangeProofLen);
                };
            };
            
            
            // Fill in dummy signatures for fee calculation.
            int nIn = 0;
            for (const auto &coin : setCoins)
            {
                const CScript& scriptPubKey = coin.first->tx->vpout[coin.second]->GetStandardOutput()->scriptPubKey;
                SignatureData sigdata;

                if (!ProduceSignature(DummySignatureCreatorParticl(this), scriptPubKey, sigdata))
                    return errorN(1, sError, __func__, "Dummy signature failed.");
                UpdateTransaction(txNew, nIn, sigdata);
                nIn++;
            };
            
            unsigned int nBytes = GetVirtualTransactionSize(txNew);

            CTransaction txNewConst(txNew);
            dPriority = txNewConst.ComputePriority(dPriority, nBytes);
            
            // Remove scriptSigs to eliminate the fee calculation dummy signatures
            for (auto &vin : txNew.vin) {
                vin.scriptSig = CScript();
                vin.scriptWitness.SetNull();
            }
            
            int currentConfirmationTarget = nTxConfirmTarget;
            
            CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
            
            // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
            // because we must be at the maximum allowed fee.
            if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
                return errorN(1, sError, __func__, _("Transaction too large for fee policy.").c_str());
            
            if (nFeeRet >= nFeeNeeded)
            {
                // Reduce fee to only the needed amount if we have change
                // output to increase.  This prevents potential overpayment
                // in fees if the coins selected to meet nFeeNeeded result
                // in a transaction that requires less fee than the prior
                // iteration.
                // TODO: The case where nSubtractFeeFromAmount > 0 remains
                // to be addressed because it requires returning the fee to
                // the payees and not the change output.
                // TODO: The case where there is no change output remains
                // to be addressed so we avoid creating too small an output.
                if (nFeeRet > nFeeNeeded && nChangePosInOut != -1)
                {
                    auto &r = vecSend[nChangePosInOut];
                    
                    CAmount extraFeePaid = nFeeRet - nFeeNeeded;
                    CTxOutBaseRef c = txNew.vpout[r.n];
                    c->SetValue(c->GetValue() + extraFeePaid);
                    r.nAmount = c->GetValue();
                    
                    nFeeRet -= extraFeePaid;
                };
                break; // Done, enough fee included.
            };
            
            // Try to reduce change to include necessary fee
            if (nChangePosInOut != -1)
            {
                auto &r = vecSend[nChangePosInOut];
                
                CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                
                CTxOutBaseRef c = txNew.vpout[r.n];
                // Only reduce change if remaining amount is still a large enough output.
                if (c->GetValue() >= MIN_FINAL_CHANGE + additionalFeeNeeded)
                {
                    c->SetValue(c->GetValue() - additionalFeeNeeded);
                    r.nAmount = c->GetValue();
                    nFeeRet += additionalFeeNeeded;
                    break; // Done, able to increase fee from change
                };
            };

            // Include more fee and try again.
            nFeeRet = nFeeNeeded;
            continue;
        };
        
        std::vector<uint8_t> &vData = ((CTxOutData*)txNew.vpout[0].get())->vData;
        vData.resize(1);
        if (0 != PutVarInt(vData, nFeeRet))
            return errorN(1, "%s: PutVarInt %ld failed\n", __func__, nFeeRet);
        
        if (sign)
        {
            CTransaction txNewConst(txNew);
            int nIn = 0;
            for (const auto &coin : setCoins)
            {
                const CTxOutStandard *prevOut = coin.first->tx->vpout[coin.second]->GetStandardOutput();
                const CScript& scriptPubKey = prevOut->scriptPubKey;
                //CAmount nCoinValue = prevOut->GetValue();
                std::vector<uint8_t> vchAmount;
                prevOut->PutValue(vchAmount);
                
                SignatureData sigdata;
                
                if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, vchAmount, SIGHASH_ALL), scriptPubKey, sigdata))
                {
                    return errorN(1, sError, __func__, _("Signing transaction failed").c_str());
                } else
                {
                    UpdateTransaction(txNew, nIn, sigdata);
                };

                nIn++;
            };
        };
        
        rtx.nFee = nFeeRet;
        AddOutputRecordMetaData(rtx, vecSend);
        
        // Embed the constructed transaction data in wtxNew.
        wtx.SetTx(MakeTransactionRef(std::move(txNew)));
        
    } // cs_main, cs_wallet
    
    return 0;
}

int CHDWallet::AddStandardInputs(CWalletTx &wtx, CTransactionRecord &rtx,
    std::vector<CTempRecipient> &vecSend, bool sign, std::string &sError)
{
    if (vecSend.size() < 1)
        return errorN(1, sError, __func__, _("Transaction must have at least one recipient.").c_str());
    
    CAmount nValue = 0;
    bool fCT = false;
    for (auto &r : vecSend)
    {
        nValue += r.nAmount;
        if (nValue < 0 || r.nAmount < 0)
            return errorN(1, sError, __func__, _("Transaction amounts must not be negative.").c_str());
        
        if (r.nType == OUTPUT_CT)
            fCT = true;
    };
    
    // No point hiding the amount in one output
    // If one of the outputs was always 0 it would be easy to track amounts,
    // the output that gets spent would be = plain input.
    if (fCT
        && vecSend.size() < 2)
    {
        CTempRecipient &r0 = vecSend[0];
        CTempRecipient rN;
        rN.nType = r0.nType;
        rN.nAmount = r0.nAmount * ((float)GetRandInt(100) / 100.0);
        rN.address = r0.address;
        
        r0.nAmount -= rN.nAmount;
        vecSend.push_back(rN);
    };
    
    CExtKeyAccount *sea;
    CStoredExtKey *pcC;
    if (0 != GetDefaultConfidentialChain(NULL, sea, pcC))
        return errorN(1, sError, __func__, _("Could not get confidential chain from account.").c_str());
    
    uint32_t nLastHardened = pcC->nHGenerated;
    
    if (0 != AddStandardInputs(wtx, rtx, vecSend, sea, pcC, sign, sError))
    {
        // sError will be set
        pcC->nHGenerated = nLastHardened; // reset
        return 1;
    };
    
    
    {
        LOCK(cs_wallet);
        CHDWalletDB wdb(strWalletFile, "r+");
        
        std::vector<uint8_t> vEphemPath;
        uint32_t idIndex;
        bool requireUpdateDB;
        if (0 == ExtKeyGetIndex(&wdb, sea, idIndex, requireUpdateDB))
        {
            PushUInt32(vEphemPath, idIndex);
            
            if (0 == AppendChainPath(pcC, vEphemPath))
            {
                uint32_t nChild = nLastHardened;
                PushUInt32(vEphemPath, SetHardenedBit(nChild));
                rtx.mapValue[RTXVT_EPHEM_PATH] = vEphemPath;
                
            } else
            {
                LogPrintf("Warning: %s - missing path value.\n", __func__);
                vEphemPath.clear();
            };
        } else
        {
            LogPrintf("Warning: %s - ExtKeyGetIndex failed %s.\n", __func__, pcC->GetIDString58());
        };
        
        CKeyID idChain = pcC->GetID();
        if (!wdb.WriteExtKey(idChain, *pcC))
        {
            pcC->nHGenerated = nLastHardened;
            return errorN(1, sError, __func__, "WriteExtKey failed.");
        };
    }
    
    return 0;
};

int CHDWallet::AddBlindedInputs(CWalletTx &wtx, CTransactionRecord &rtx,
    std::vector<CTempRecipient> &vecSend,
    CExtKeyAccount *sea, CStoredExtKey *pc,
    bool sign, std::string &sError)
{
    CAmount nFeeRet = 0;
    CAmount nValue = 0;
    for (auto &r : vecSend)
        nValue += r.nAmount;
    
    if (0 != ExpandTempRecipients(vecSend, pc, sError))
        return 0; // sError is set
    
    wtx.fTimeReceivedIsTxTime = true;
    wtx.BindWallet(this);
    wtx.fFromMe = true;
    CMutableTransaction txNew;
    txNew.nVersion = PARTICL_TXN_VERSION;
    txNew.vout.clear();
    
    // Discourage fee sniping. See CWallet::CreateTransaction
    txNew.nLockTime = chainActive.Height();
    
    // 1/10 chance of random time further back to increase privacy
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));
    
    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
    
    {
        std::vector<std::pair<MapRecords_t::const_iterator, unsigned int> > setCoins;
        LOCK2(cs_main, cs_wallet);
        std::vector<COutputR> vAvailableCoins;
        AvailableBlindedCoins(vAvailableCoins, true);
        
        CAmount nValueOutPlain = 0;
        int nChangePosInOut = -1;
        
        // Start with no fee and loop until there is enough fee
        for (;;)
        {
            txNew.vin.clear();
            txNew.vpout.clear();
            wtx.fFromMe = true;
            
            CAmount nValueToSelect = nValue;
            
            double dPriority = 0;
            
            // Choose coins to use
            CAmount nValueIn = 0;
            setCoins.clear();
            if (!SelectBlindedCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn))
                return errorN(1, sError, __func__, _("Insufficient funds.").c_str());
            
            for (const auto &pcoin : setCoins)
            {
                int age = GetDepthInMainChain(pcoin.first->second.blockHash);
                if (age != 0)
                    age += 1;
                
                dPriority += age;
            };
            
            const CAmount nChange = nValueIn - nValueToSelect;
            
            // Remove fee output added during last iteration
            for (size_t i = 0; i < vecSend.size(); ++i)
            {
                if (vecSend[i].fChange)
                {
                    vecSend.erase(vecSend.begin() + i);
                    break;
                };
            };
            
            nChangePosInOut = -1;
            // Insert a sender-owned 0 value output that becomes the change output if needed
            {
                // Fill an output to ourself
                CPubKey pkChange;
                if (0 != GetChangeAddress(pkChange))
                    return errorN(1, sError, __func__, "GetChangeAddress failed.");
                
                CKey sEphem;
                uint32_t nDiscard; // Derive the same ephemeral secret each iteration
                if (0 != pc->DeriveNextKey(sEphem, nDiscard, true))
                    return errorN(1, sError, __func__, "TryDeriveNext failed.");
                
                CTempRecipient r;
                r.nType = OUTPUT_CT;
                r.nAmount = 0;
                
                CKeyID idChange = pkChange.GetID();
                r.address = idChange;
                r.pkTo = pkChange;
                r.scriptPubKey = GetScriptForDestination(idChange);
                r.fChange = true;
                r.sEphem = sEphem;
                
                nChangePosInOut = GetRandInt(vecSend.size()+1);
                if (nChangePosInOut < (int)vecSend.size()
                    && vecSend[nChangePosInOut].nType == OUTPUT_DATA)
                    nChangePosInOut++;
                vecSend.insert(vecSend.begin()+nChangePosInOut, r);
            };
            
            if (nChange > ::minRelayTxFee.GetFee(2048)) // TODO: better output size estimate
            {
                // Fill an output to ourself
                CPubKey pkChange;
                if (0 != GetChangeAddress(pkChange))
                    return errorN(1, sError, __func__, "GetChangeAddress failed.");
                
                CKey sEphem;
                uint32_t nDiscard; // Derive the same ephemeral secret each iteration
                if (0 != pc->DeriveNextKey(sEphem, nDiscard, true))
                    return errorN(1, sError, __func__, "TryDeriveNext failed.");
                
                CTempRecipient r;
                r.nType = OUTPUT_CT;
                r.nAmount = nChange;
                
                CKeyID idChange = pkChange.GetID();
                r.address = idChange;
                r.pkTo = pkChange;
                r.scriptPubKey = GetScriptForDestination(idChange);
                r.fChange = true;
                r.sEphem = sEphem;
                
                if (nChangePosInOut > -1)
                {
                    // Update existing change output
                    vecSend[nChangePosInOut] = r;
                } else
                {
                    // Insert change txn at random position:
                    nChangePosInOut = GetRandInt(vecSend.size()+1);
                    // Don't displace data outputs
                    if (nChangePosInOut < (int)vecSend.size()
                        && vecSend[nChangePosInOut].nType == OUTPUT_DATA)
                        nChangePosInOut++;
                    vecSend.insert(vecSend.begin()+nChangePosInOut, r);
                };
            } else
            {
                nFeeRet += nChange;
            };
            
            // Fill vin
            //
            // Note how the sequence number is set to non-maxint so that
            // the nLockTime set above actually works.
            //
            // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
            // we use the highest possible value in that range (maxint-2)
            // to avoid conflicting with other possible uses of nSequence,
            // and in the spirit of "smallest possible change from prior
            // behavior."
            for (const auto &coin : setCoins)
            {
                // coin.first->first is txhash
                txNew.vin.push_back(CTxIn(coin.first->first,coin.second,CScript(),
                    std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
            };
            
            std::vector<uint8_t*> vpBlinds;
            nValueOutPlain = 0;
            // The fee will come out of the change
            
            int nLastBlindedOutput = -1;
            nChangePosInOut = -1;
            
            OUTPUT_PTR<CTxOutData> outFee = MAKE_OUTPUT<CTxOutData>();
            outFee->vData.push_back(DO_FEE);
            outFee->vData.resize(9); // resize to more bytes than varint fee could take
            txNew.vpout.push_back(outFee);
            
            for (size_t i = 0; i < vecSend.size(); ++i)
            {
                auto &r = vecSend[i];
                
                if (r.nType == OUTPUT_DATA)
                {
                    OUTPUT_PTR<CTxOutData> txout = MAKE_OUTPUT<CTxOutData>();
                    txout->vData = r.vData;
                    
                    r.n = txNew.vpout.size();
                    txNew.vpout.push_back(txout);
                } else
                if (r.nType == OUTPUT_STANDARD)
                {
                    nValueOutPlain += r.nAmount;
                    OUTPUT_PTR<CTxOutStandard> txout = MAKE_OUTPUT<CTxOutStandard>(r.nAmount, r.scriptPubKey);
                    
                    r.n = txNew.vpout.size();
                    txNew.vpout.push_back(txout);
                } else
                if (r.nType == OUTPUT_CT)
                {
                    if (r.fChange)
                        nChangePosInOut = i;
                    
                    nLastBlindedOutput = i;
                    OUTPUT_PTR<CTxOutCT> txout = MAKE_OUTPUT<CTxOutCT>();
                    
                    txout->vData.resize(33);
                    CPubKey pkEphem = r.sEphem.GetPubKey();
                    memcpy(&txout->vData[0], pkEphem.begin(), 33);
                    
                    txout->scriptPubKey = r.scriptPubKey;
                    
                    r.n = txNew.vpout.size();
                    txNew.vpout.push_back(txout);
                    
                    r.vBlind.resize(32);
                    // Need to know the fee before calulating the blind sum
                    GetStrongRandBytes(&r.vBlind[0], 32);
                    
                    CTxOutCT *pout = txout.get();
                    uint64_t nValue = r.nAmount;
                    if (!secp256k1_pedersen_commit(secp256k1_ctx_blind,
                        &pout->commitment, (uint8_t*)&r.vBlind[0],
                        nValue, secp256k1_generator_h))
                        return errorN(1, sError, __func__, "secp256k1_pedersen_commit failed.");
                    
                    
                    uint256 nonce = r.sEphem.ECDH(r.pkTo);
                    CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());
                    
                    const char *message = r.sNarration.c_str();
                    size_t mlen = strlen(message);
                    
                    size_t nRangeProofLen = 5134;
                    pout->vRangeproof.resize(nRangeProofLen);
                    
                    // TODO: rangeproof parameter selection
                    
                    uint64_t min_value = 0;
                    int ct_exponent = 2;
                    int ct_bits = 32;
                    
                    SelectRangeProofParameters(nValue, min_value, ct_exponent, ct_bits);
                    
                    
                    if (1 != secp256k1_rangeproof_sign(secp256k1_ctx_blind,
                        &pout->vRangeproof[0], &nRangeProofLen,
                        min_value, &pout->commitment,
                        &r.vBlind[0], nonce.begin(),
                        ct_exponent, ct_bits, 
                        nValue,
                        (const unsigned char*) message, mlen,
                        NULL, 0,
                        secp256k1_generator_h))
                        return errorN(1, sError, __func__, "secp256k1_rangeproof_sign failed.");
                    
                    pout->vRangeproof.resize(nRangeProofLen);
                };
            };
            
            // Fill in dummy signatures for fee calculation.
            int nIn = 0;
            for (const auto &coin : setCoins)
            {
                const COutputRecord *oR = coin.first->second.GetOutput(coin.second);
                const CScript &scriptPubKey = oR->scriptPubKey;
                SignatureData sigdata;
                
                

                if (!ProduceSignature(DummySignatureCreatorParticl(this), scriptPubKey, sigdata))
                    return errorN(1, sError, __func__, "Dummy signature failed.");
                UpdateTransaction(txNew, nIn, sigdata);
                nIn++;
            };
            
            unsigned int nBytes = GetVirtualTransactionSize(txNew);

            CTransaction txNewConst(txNew);
            dPriority = txNewConst.ComputePriority(dPriority, nBytes);
            
            
            int currentConfirmationTarget = nTxConfirmTarget;
            
            CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
            
            
            // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
            // because we must be at the maximum allowed fee.
            if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
                return errorN(1, sError, __func__, _("Transaction too large for fee policy.").c_str());
            
            if (nFeeRet >= nFeeNeeded)
            {
                // Reduce fee to only the needed amount if we have change
                // output to increase.  This prevents potential overpayment
                // in fees if the coins selected to meet nFeeNeeded result
                // in a transaction that requires less fee than the prior
                // iteration.
                // TODO: The case where nSubtractFeeFromAmount > 0 remains
                // to be addressed because it requires returning the fee to
                // the payees and not the change output.
                // TODO: The case where there is no change output remains
                // to be addressed so we avoid creating too small an output.
                if (nFeeRet > nFeeNeeded && nChangePosInOut != -1)
                {
                    auto &r = vecSend[nChangePosInOut];
                    
                    CAmount extraFeePaid = nFeeRet - nFeeNeeded;
                    
                    r.nAmount += extraFeePaid;
                    nFeeRet -= extraFeePaid;
                };
                break; // Done, enough fee included.
            };
            
            // Try to reduce change to include necessary fee
            if (nChangePosInOut != -1)
            {
                auto &r = vecSend[nChangePosInOut];
                
                CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                
                if (r.nAmount >= MIN_FINAL_CHANGE + additionalFeeNeeded)
                {
                    r.nAmount -= additionalFeeNeeded;
                    nFeeRet += additionalFeeNeeded;
                    nValueOutPlain += nFeeRet;
                    break; // Done, able to increase fee from change
                }
            };
            
            
            // Include more fee and try again.
            nFeeRet = nFeeNeeded;
            continue;
        };
        
        std::vector<uint8_t> vInputBlinds;
        std::vector<uint8_t*> vpBlinds;
        
        vInputBlinds.resize(32 * setCoins.size());
        
        int nIn = 0;
        for (const auto &coin : setCoins)
        {
            auto &vin = txNew.vin[nIn];
            const uint256 &txhash = coin.first->first;
            
            CStoredTransaction stx;
            if (!CHDWalletDB(strWalletFile).ReadStoredTx(txhash, stx))
                return errorN(1, "%s: ReadStoredTx failed for %s.\n", __func__, txhash.ToString().c_str());
            
            if (!stx.GetBlind(coin.second, &vInputBlinds[nIn * 32]))
                return errorN(1, "%s: GetBlind failed for %s.\n", __func__, txhash.ToString().c_str());
            
            vpBlinds.push_back(&vInputBlinds[nIn * 32]);
            
            
            // Remove scriptSigs to eliminate the fee calculation dummy signatures
            vin.scriptSig = CScript();
            vin.scriptWitness.SetNull();
            nIn++;
        };
        
        
        size_t nBlindedInputs = vpBlinds.size();
        
        
        
        std::vector<uint8_t> vBlindPlain;
        vBlindPlain.resize(32);
        memset(&vBlindPlain[0], 0, 32);
        
        secp256k1_pedersen_commitment plainCommitment;
        
        if (nValueOutPlain > 0)
        {
            vpBlinds.push_back(&vBlindPlain[0]);
            //if (!secp256k1_pedersen_commit(secp256k1_ctx_blind, &plainCommitment, &vBlindPlain[0], (uint64_t) nValueOutPlain, secp256k1_generator_h))
            //    return errorN(1, sError, __func__, "secp256k1_pedersen_commit failed for plain out.");
        };
        
        // Update the change output commitment
        for (size_t i = 0; i < vecSend.size(); ++i)
        {
            auto &r = vecSend[i];
            
            if (r.nType == OUTPUT_CT)
            {
                //CTxOutCT *txout = (CTxOutCT*)txNew.vpout[r.n].get();
                
                if ((int)i != nChangePosInOut)
                {
                    vpBlinds.push_back(&r.vBlind[0]);
                };
            };
        };
        // Change output may not be last
        if (nChangePosInOut != -1)
        {
            auto &r = vecSend[nChangePosInOut];
            if (r.nType != OUTPUT_CT)
                return errorN(1, sError, __func__, "nChangePosInOut not blind.");
            
            CTxOutCT *pout = (CTxOutCT*)txNew.vpout[r.n].get();
            // Last to-be-blinded value: compute from all other blinding factors.
            // sum of output blinding values must equal sum of input blinding values
            if (!secp256k1_pedersen_blind_sum(secp256k1_ctx_blind, &r.vBlind[0], &vpBlinds[0], vpBlinds.size(), nBlindedInputs))
                return errorN(1, sError, __func__, "secp256k1_pedersen_blind_sum failed.");
            
            
            uint64_t nValue = r.nAmount;
            if (!secp256k1_pedersen_commit(secp256k1_ctx_blind,
                &pout->commitment, (uint8_t*)&r.vBlind[0],
                nValue, secp256k1_generator_h))
                return errorN(1, sError, __func__, "secp256k1_pedersen_commit failed.");
            
            uint256 nonce = r.sEphem.ECDH(r.pkTo);
            CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());
            
            const char *message = r.sNarration.c_str();
            size_t mlen = strlen(message);
            
            size_t nRangeProofLen = 5134;
            pout->vRangeproof.resize(nRangeProofLen);
            
            // TODO: rangeproof parameter selection
            
            uint64_t min_value = 0;
            int ct_exponent = 2;
            int ct_bits = 32;
            
            if (1 != secp256k1_rangeproof_sign(secp256k1_ctx_blind,
                &pout->vRangeproof[0], &nRangeProofLen,
                min_value, &pout->commitment,
                &r.vBlind[0], nonce.begin(),
                ct_exponent, ct_bits, 
                nValue,
                (const unsigned char*) message, mlen,
                NULL, 0,
                secp256k1_generator_h))
                return errorN(1, sError, __func__, "secp256k1_rangeproof_sign failed.");
            
            pout->vRangeproof.resize(nRangeProofLen);
        };
        
        
        
        std::vector<uint8_t> &vData = ((CTxOutData*)txNew.vpout[0].get())->vData;
        vData.resize(1);
        if (0 != PutVarInt(vData, nFeeRet))
            return errorN(1, "%s: PutVarInt %ld failed\n", __func__, nFeeRet);
        
        if (sign)
        {
            CTransaction txNewConst(txNew);
            int nIn = 0;
            for (const auto &coin : setCoins)
            {
                const uint256 &txhash = coin.first->first;
                const COutputRecord *oR = coin.first->second.GetOutput(coin.second);
                if (!oR)
                    return errorN(1, "%s: GetOutput %s failed\n", __func__, txhash.ToString());
                
                const CScript &scriptPubKey = oR->scriptPubKey;
                
                CStoredTransaction stx;
                if (!CHDWalletDB(strWalletFile).ReadStoredTx(txhash, stx))
                    return errorN(1, "%s: ReadStoredTx failed for %s.\n", __func__, txhash.ToString().c_str());
                std::vector<uint8_t> vchAmount;
                stx.tx->vpout[coin.second]->PutValue(vchAmount);
                
                SignatureData sigdata;
                
                if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, vchAmount, SIGHASH_ALL), scriptPubKey, sigdata))
                {
                    return errorN(1, sError, __func__, _("Signing transaction failed").c_str());
                } else
                {
                    UpdateTransaction(txNew, nIn, sigdata);
                };
                
                nIn++;
            };
        };
        
        
        rtx.nFee = nFeeRet;
        AddOutputRecordMetaData(rtx, vecSend);
        
        // Embed the constructed transaction data in wtxNew.
        wtx.SetTx(MakeTransactionRef(std::move(txNew)));
        
    } // cs_main, cs_wallet
    
    
    return 0;
};

int CHDWallet::AddBlindedInputs(CWalletTx &wtx, CTransactionRecord &rtx,
    std::vector<CTempRecipient> &vecSend, bool sign, std::string &sError)
{
    if (vecSend.size() < 1)
        return errorN(1, sError, __func__, _("Transaction must have at least one recipient.").c_str());
    
    
    CExtKeyAccount *sea;
    CStoredExtKey *pcC;
    if (0 != GetDefaultConfidentialChain(NULL, sea, pcC))
        return errorN(1, sError, __func__, _("Could not get confidential chain from account.").c_str());
    
    uint32_t nLastHardened = pcC->nHGenerated;
    
    if (0 != AddBlindedInputs(wtx, rtx, vecSend, sea, pcC, sign, sError))
    {
        // sError will be set
        pcC->nHGenerated = nLastHardened; // reset
        return 1;
    };
    
    {
        LOCK(cs_wallet);
        CHDWalletDB wdb(strWalletFile, "r+");
        
        std::vector<uint8_t> vEphemPath;
        uint32_t idIndex;
        bool requireUpdateDB;
        if (0 == ExtKeyGetIndex(&wdb, sea, idIndex, requireUpdateDB))
        {
            PushUInt32(vEphemPath, idIndex);
            
            if (0 == AppendChainPath(pcC, vEphemPath))
            {
                uint32_t nChild = nLastHardened;
                PushUInt32(vEphemPath, SetHardenedBit(nChild));
                rtx.mapValue[RTXVT_EPHEM_PATH] = vEphemPath;
                
            } else
            {
                LogPrintf("Warning: %s - missing path value.\n", __func__);
                vEphemPath.clear();
            };
        } else
        {
            LogPrintf("Warning: %s - ExtKeyGetIndex failed %s.\n", __func__, pcC->GetIDString58());
        };
        
        CKeyID idChain = pcC->GetID();
        if (!wdb.WriteExtKey(idChain, *pcC))
        {
            pcC->nHGenerated = nLastHardened;
            return errorN(1, sError, __func__, "WriteExtKey failed.");
        };
    }
    
    return 0;
};

bool CHDWallet::LoadToWallet(const CWalletTx& wtxIn)
{
    uint256 hash = wtxIn.GetHash();

    mapWallet[hash] = wtxIn;
    CWalletTx& wtx = mapWallet[hash];
    wtx.BindWallet(this);
    wtxOrdered.insert(make_pair(wtx.nOrderPos, TxPair(&wtx, (CAccountingEntry*)0)));
    AddToSpends(hash);
    BOOST_FOREACH(const CTxIn& txin, wtx.tx->vin) {
        if (mapWallet.count(txin.prevout.hash)) {
            CWalletTx& prevtx = mapWallet[txin.prevout.hash];
            if (prevtx.nIndex == -1 && !prevtx.hashUnset()) {
                MarkConflicted(prevtx.hashBlock, wtx.GetHash());
            }
        }
    }
    
    int nBestHeight = chainActive.Height();
    if (wtx.IsCoinStake() && wtx.isAbandoned())
    {
        int csHeight;
        if (wtx.tx->GetCoinStakeHeight(csHeight)
            && csHeight > nBestHeight - 1000)
        {
            // Add to MapStakeSeen to prevent node submitting a block that would be rejected.
            const COutPoint &kernel = wtx.tx->vin[0].prevout;
            AddToMapStakeSeen(kernel, hash);
        };
    };
    
    return true;
};

bool CHDWallet::LoadToWallet(const uint256 &hash, const CTransactionRecord &rtx)
{
    
    std::pair<std::map<uint256, CTransactionRecord>::iterator, bool> ret = mapRecords.insert(std::make_pair(hash, rtx));
    
    std::map<uint256, CTransactionRecord>::iterator mri = ret.first;
    rtxOrdered.insert(std::make_pair(rtx.nTimeReceived, mri));
    
    return true;
};


int CHDWallet::UnloadTransaction(const uint256 &hash)
{
    // remove from TxSpends
    
    std::map<uint256, CWalletTx>::iterator itw;
    if ((itw = mapWallet.find(hash)) == mapWallet.end())
    {
        LogPrintf("Warning: %s - tx not found in mapwallet! %s.\n", __func__, hash.ToString());
        return 1;
    };
    
    CWalletTx *pcoin = &itw->second;
    
    for (auto &txin : pcoin->tx->vin)
    {
        std::pair<TxSpends::iterator, TxSpends::iterator> ip = mapTxSpends.equal_range(txin.prevout);
        for (auto it = ip.first; it != ip.second; ++it)
        {
            if (it->second == hash)
            { 
                mapTxSpends.erase(it);
                break;
            };
        };
    };
    
    mapWallet.erase(itw);
    return 0;
};

int CHDWallet::GetDefaultConfidentialChain(CHDWalletDB *pwdb, CExtKeyAccount *&sea, CStoredExtKey *&pc)
{
    LOCK(cs_wallet);
    
    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idDefaultAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s: %s.", __func__, _("Default account not found"));
    
    sea = mi->second;
    
    mapEKValue_t::iterator mvi = sea->mapValue.find(EKVT_CONFIDENTIAL_CHAIN);
    if (mvi != sea->mapValue.end())
    {
        uint64_t n;
        GetCompressedInt64(mvi->second, n);
        
        if (n < sea->vExtKeys.size())
        {
            pc = sea->vExtKeys[n];
            return 0;
        };
        
        return errorN(1, "%s: %s.", __func__, _("Confidential chain set but not found"));
    };
    
    LogPrint("hdwallet", "Adding confidential chain to account: %s.\n", sea->GetIDString58());
    
    size_t nConfidential = sea->vExtKeys.size();
    
    CStoredExtKey *sekAccount = sea->ChainAccount();
    if (!sekAccount)
        return errorN(1, "%s: %s.", __func__, _("Account chain not found"));
    
    std::vector<uint8_t> vAccountPath, vSubKeyPath, v;
    mvi = sekAccount->mapValue.find(EKVT_PATH);
    if (mvi != sekAccount->mapValue.end())
        vAccountPath = mvi->second;
    
    CExtKey evConfidential;
    uint32_t nChild;
    if (0 != sekAccount->DeriveNextKey(evConfidential, nChild, true))
        return errorN(1, "%s: %s.", __func__, _("DeriveNextKey failed"));
    
    
    CStoredExtKey *sekConfidential = new CStoredExtKey();
    sekConfidential->kp = evConfidential;
    vSubKeyPath = vAccountPath;
    SetHardenedBit(nChild);
    sekConfidential->mapValue[EKVT_PATH] = PushUInt32(vSubKeyPath, nChild);
    sekConfidential->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT;
    
    sea->vExtKeyIDs.push_back(sekConfidential->GetID());
    sea->vExtKeys.push_back(sekConfidential);
    
    sea->mapValue[EKVT_CONFIDENTIAL_CHAIN] = SetCompressedInt64(v, nConfidential);
    
    if (!pwdb)
    {
        CHDWalletDB wdb(strWalletFile, "r+");
        if (0 != ExtKeySaveAccountToDB(&wdb, idDefaultAccount, sea))
            return errorN(1,"%s: %s.", __func__, _("ExtKeySaveAccountToDB failed"));
    } else
    {
        if (0 != ExtKeySaveAccountToDB(pwdb, idDefaultAccount, sea))
            return errorN(1, "%s: %s.", __func__, _("ExtKeySaveAccountToDB failed"));
    };
    
    pc = sekConfidential;
    return 0;
};

int CHDWallet::ExtKeyNew32(CExtKey &out)
{
    if (fDebug)
        LogPrint("hdwallet", "ExtKeyNew32 from random.\n");

    uint8_t data[32];

    for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
    {
        GetStrongRandBytes(data, 32);
        if (ExtKeyNew32(out, data, 32) == 0)
            break;
    };

    return out.IsValid() ? 0 : 1;
};

int CHDWallet::ExtKeyNew32(CExtKey &out, const char *sPassPhrase, int32_t nHash, const char *sSeed)
{
    // - match keys from http://bip32.org/
    if (fDebug)
        LogPrint("hdwallet", "ExtKeyNew32 from pass phrase.\n");

    uint8_t data[64];
    int nPhraseLen = strlen(sPassPhrase);
    int nSeedLen = strlen(sSeed);

    memset(data, 0, 64);

    CHMAC_SHA256 ctx256((const uint8_t*)sPassPhrase, nPhraseLen);
    for (int i = 0; i < nHash; ++i)
    {
        CHMAC_SHA256 tmp = ctx256;
        tmp.Write(data, 32).Finalize(data);
    };
    
    CHMAC_SHA512((const uint8_t*)sSeed, nSeedLen).Write(data, 32).Finalize(data);

    if (out.SetKeyCode(data, &data[32]) != 0)
        return errorN(1, "SetKeyCode failed.");

    return out.IsValid() ? 0 : 1;
};

int CHDWallet::ExtKeyNew32(CExtKey &out, uint8_t *data, uint32_t lenData)
{
    if (fDebug)
        LogPrint("hdwallet", "%s\n", __func__);

    out.SetMaster(data, lenData);

    return out.IsValid() ? 0 : 1;
};

int CHDWallet::ExtKeyImportLoose(CHDWalletDB *pwdb, CStoredExtKey &sekIn, CKeyID &idDerived, bool fBip44, bool fSaveBip44)
{
    // - if fBip44, the bip44 keyid is returned in idDerived
    
    if (fDebug)
    {
        LogPrint("hdwallet", "%s.\n", __func__);
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CKeyID id = sekIn.GetID();

    bool fsekInExist = true;
    // - it's possible for a public ext key to be added first
    CStoredExtKey sekExist;
    CStoredExtKey sek = sekIn;
    if (pwdb->ReadExtKey(id, sekExist))
    {
        if (IsCrypted()
            && 0 != ExtKeyUnlock(&sekExist))
            return errorN(13, "%s: %s", __func__, ExtKeyGetString(13));

        sek = sekExist;
        if (!sek.kp.IsValidV()
            && sekIn.kp.IsValidV())
        {
            sek.kp = sekIn.kp;
            std::vector<uint8_t> v;
            sek.mapValue[EKVT_ADDED_SECRET_AT] = SetCompressedInt64(v, GetTime());
        };
    } else
    {
        // - new key
        sek.nFlags |= EAF_ACTIVE;

        fsekInExist = false;
    };

    if (fBip44)
    {
        // import key as bip44 root and derive a new master key
        // NOTE: can't know created at time of derived key here

        std::vector<uint8_t> v;
        sek.mapValue[EKVT_KEY_TYPE] = SetChar(v, EKT_BIP44_MASTER);

        CExtKey evDerivedKey, evPurposeKey;
        sek.kp.Derive(evPurposeKey, BIP44_PURPOSE);
        evPurposeKey.Derive(evDerivedKey, Params().BIP44ID());

        v.resize(0);
        PushUInt32(v, BIP44_PURPOSE);
        PushUInt32(v, Params().BIP44ID());

        CStoredExtKey sekDerived;
        sekDerived.nFlags |= EAF_ACTIVE;
        sekDerived.kp = evDerivedKey;
        sekDerived.mapValue[EKVT_PATH] = v;
        sekDerived.mapValue[EKVT_ROOT_ID] = SetCKeyID(v, id);
        sekDerived.sLabel = sek.sLabel + " - bip44 derived.";

        idDerived = sekDerived.GetID();

        if (pwdb->ReadExtKey(idDerived, sekExist))
        {
            if (fSaveBip44
                && !fsekInExist)
            {
                // - assume the user wants to save the bip44 key, drop down
            } else
            {
                return errorN(12, "%s: %s", __func__, ExtKeyGetString(12));
            };
        } else
        {
            if (IsCrypted()
                && (ExtKeyEncrypt(&sekDerived, vMasterKey, false) != 0))
                return errorN(1, "%s: ExtKeyEncrypt failed.", __func__);

            if (!pwdb->WriteExtKey(idDerived, sekDerived))
                return errorN(1, "%s: DB Write failed.", __func__);
        };
    };

    if (!fBip44 || fSaveBip44)
    {
        if (IsCrypted()
            && ExtKeyEncrypt(&sek, vMasterKey, false) != 0)
            return errorN(1, "%s: ExtKeyEncrypt failed.", __func__);

        if (!pwdb->WriteExtKey(id, sek))
            return errorN(1, "%s: DB Write failed.", __func__);
    };

    return 0;
};

int CHDWallet::ExtKeyImportAccount(CHDWalletDB *pwdb, CStoredExtKey &sekIn, int64_t nTimeStartScan, const std::string &sLabel)
{
    // rv: 0 success, 1 fail, 2 existing key, 3 updated key
    // It's not possible to import an account using only a public key as internal keys are derived hardened

    if (fDebug)
    {
        LogPrint("hdwallet", "%s.\n", __func__);
        AssertLockHeld(cs_wallet);

        if (nTimeStartScan == 0)
            LogPrintf("No blockchain scanning.\n");
        else
            LogPrintf("Scan blockchain from %d.\n", nTimeStartScan);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CKeyID idAccount = sekIn.GetID();

    CStoredExtKey *sek = new CStoredExtKey();
    *sek = sekIn;

    // NOTE: is this confusing behaviour?
    CStoredExtKey sekExist;
    if (pwdb->ReadExtKey(idAccount, sekExist))
    {
        // add secret if exists in db
        *sek = sekExist;
        if (!sek->kp.IsValidV()
            && sekIn.kp.IsValidV())
        {
            sek->kp = sekIn.kp;
            std::vector<uint8_t> v;
            sek->mapValue[EKVT_ADDED_SECRET_AT] = SetCompressedInt64(v, GetTime());
        };
    };

    // TODO: before allowing import of 'watch only' accounts
    //       txns must be linked to accounts.

    if (!sek->kp.IsValidV())
    {
        delete sek;
        return errorN(1, "Accounts must be derived from a valid private key.");
    };

    CExtKeyAccount *sea = new CExtKeyAccount();
    if (pwdb->ReadExtAccount(idAccount, *sea))
    {
        if (0 != ExtKeyUnlock(sea))
        {
            delete sek;
            delete sea;
            return errorN(1, "Error unlocking existing account.");
        };
        CStoredExtKey *sekAccount = sea->ChainAccount();
        if (!sekAccount)
        {
            delete sek;
            delete sea;
            return errorN(1, "ChainAccount failed.");
        };
        // - account exists, update secret if necessary
        if (!sek->kp.IsValidV()
            && sekAccount->kp.IsValidV())
        {
            sekAccount->kp = sek->kp;
            std::vector<uint8_t> v;
            sekAccount->mapValue[EKVT_ADDED_SECRET_AT] = SetCompressedInt64(v, GetTime());

             if (IsCrypted()
                && ExtKeyEncrypt(sekAccount, vMasterKey, false) != 0)
            {
                delete sek;
                delete sea;
                return errorN(1, "ExtKeyEncrypt failed.");
            };

            if (!pwdb->WriteExtKey(idAccount, *sekAccount))
            {
                delete sek;
                delete sea;
                return errorN(1, "WriteExtKey failed.");
            };
            if (nTimeStartScan)
                ScanChainFromTime(nTimeStartScan);

            delete sek;
            delete sea;
            return 3;
        };
        delete sek;
        delete sea;
        return 2;
    };

    CKeyID idMaster; // inits to 0
    if (0 != ExtKeyCreateAccount(sek, idMaster, *sea, sLabel))
    {
        delete sek;
        delete sea;
        return errorN(1, "ExtKeyCreateAccount failed.");
    };

    std::vector<uint8_t> v;
    sea->mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, nTimeStartScan);

    if (0 != ExtKeySaveAccountToDB(pwdb, idAccount, sea))
    {
        sea->FreeChains();
        delete sea;
        return errorN(1, "DB Write failed.");
    };

    if (0 != ExtKeyAddAccountToMaps(idAccount, sea))
    {
        sea->FreeChains();
        delete sea;
        return errorN(1, "ExtKeyAddAccountToMap() failed.");
    };

    if (nTimeStartScan)
        ScanChainFromTime(nTimeStartScan);

    return 0;
};

int CHDWallet::ExtKeySetMaster(CHDWalletDB *pwdb, CKeyID &idNewMaster)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idNewMaster, CChainParams::EXT_KEY_HASH);
        LogPrint("hdwallet", "ExtKeySetMaster %s.\n", addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CKeyID idOldMaster;
    bool fOldMaster = pwdb->ReadNamedExtKeyId("master", idOldMaster);

    if (idNewMaster == idOldMaster)
        return errorN(11, ExtKeyGetString(11));

    ExtKeyMap::iterator mi;
    CStoredExtKey ekOldMaster, *pEKOldMaster, *pEKNewMaster;
    bool fNew = false;
    mi = mapExtKeys.find(idNewMaster);
    if (mi != mapExtKeys.end())
    {
        pEKNewMaster = mi->second;
    } else
    {
        pEKNewMaster = new CStoredExtKey();
        fNew = true;
        if (!pwdb->ReadExtKey(idNewMaster, *pEKNewMaster))
        {
            delete pEKNewMaster;
            return errorN(10, ExtKeyGetString(10));
        };
    };

    // - prevent setting bip44 root key as a master key.
    mapEKValue_t::iterator mvi = pEKNewMaster->mapValue.find(EKVT_KEY_TYPE);
    if (mvi != pEKNewMaster->mapValue.end()
        && mvi->second.size() == 1
        && mvi->second[0] == EKT_BIP44_MASTER)
    {
        if (fNew) delete pEKNewMaster;
        return errorN(9, ExtKeyGetString(9));
    };

    if (ExtKeyUnlock(pEKNewMaster) != 0
        || !pEKNewMaster->kp.IsValidV())
    {
        if (fNew) delete pEKNewMaster;
        return errorN(1, "New master ext key has no secret.");
    };

    std::vector<uint8_t> v;
    pEKNewMaster->mapValue[EKVT_KEY_TYPE] = SetChar(v, EKT_MASTER);

    if (!pwdb->WriteExtKey(idNewMaster, *pEKNewMaster)
        || !pwdb->WriteNamedExtKeyId("master", idNewMaster))
    {
        if (fNew) delete pEKNewMaster;
        return errorN(1, "DB Write failed.");
    };

    // -- unset old master ext key
    if (fOldMaster)
    {
        mi = mapExtKeys.find(idOldMaster);
        if (mi != mapExtKeys.end())
        {
            pEKOldMaster = mi->second;
        } else
        {
            if (!pwdb->ReadExtKey(idOldMaster, ekOldMaster))
            {
                if (fNew) delete pEKNewMaster;
                return errorN(1, "ReadExtKey failed.");
            };

            pEKOldMaster = &ekOldMaster;
        };

        mapEKValue_t::iterator it = pEKOldMaster->mapValue.find(EKVT_KEY_TYPE);
        if (it != pEKOldMaster->mapValue.end())
        {
            if (fDebug)
                LogPrint("hdwallet", "Removing tag from old master key %s.\n", pEKOldMaster->GetIDString58().c_str());
            pEKOldMaster->mapValue.erase(it);
            if (!pwdb->WriteExtKey(idOldMaster, *pEKOldMaster))
            {
                if (fNew) delete pEKNewMaster;
                return errorN(1, "WriteExtKey failed.");
            };
        };
    };

    mapExtKeys[idNewMaster] = pEKNewMaster;
    pEKMaster = pEKNewMaster;

    return 0;
};

int CHDWallet::ExtKeyNewMaster(CHDWalletDB *pwdb, CKeyID &idMaster, bool fAutoGenerated)
{
    // - Must pair with ExtKeySetMaster

    //  This creates two keys, a root key and a master key derived according
    //  to BIP44 (path 44'/22'), The root (bip44) key only stored in the system
    //  and the derived key is set as the system master key.

    LogPrintf("ExtKeyNewMaster.\n");
    AssertLockHeld(cs_wallet);
    assert(pwdb);

    if (IsLocked())
        return errorN(1, "Wallet must be unlocked.");

    CExtKey evRootKey;
    CStoredExtKey sekRoot;
    if (ExtKeyNew32(evRootKey) != 0)
        return errorN(1, "ExtKeyNew32 failed.");

    std::vector<uint8_t> v;
    sekRoot.nFlags |= EAF_ACTIVE;
    sekRoot.mapValue[EKVT_KEY_TYPE] = SetChar(v, EKT_BIP44_MASTER);
    sekRoot.kp = evRootKey;
    sekRoot.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());
    sekRoot.sLabel = "Initial BIP44 Master";
    CKeyID idRoot = sekRoot.GetID();

    CExtKey evMasterKey;
    evRootKey.Derive(evMasterKey, BIP44_PURPOSE);
    evMasterKey.Derive(evMasterKey, Params().BIP44ID());

    std::vector<uint8_t> vPath;
    PushUInt32(vPath, BIP44_PURPOSE);
    PushUInt32(vPath, Params().BIP44ID());

    CStoredExtKey sekMaster;
    sekMaster.nFlags |= EAF_ACTIVE;
    sekMaster.kp = evMasterKey;
    sekMaster.mapValue[EKVT_PATH] = vPath;
    sekMaster.mapValue[EKVT_ROOT_ID] = SetCKeyID(v, idRoot);
    sekMaster.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());
    sekMaster.sLabel = "Initial Master";

    idMaster = sekMaster.GetID();

    if (IsCrypted()
        && (ExtKeyEncrypt(&sekRoot, vMasterKey, false) != 0
            || ExtKeyEncrypt(&sekMaster, vMasterKey, false) != 0))
    {
        return errorN(1, "ExtKeyEncrypt failed.");
    };

    if (!pwdb->WriteExtKey(idRoot, sekRoot)
        || !pwdb->WriteExtKey(idMaster, sekMaster)
        || (fAutoGenerated && !pwdb->WriteFlag("madeDefaultEKey", 1)))
    {
        return errorN(1, "DB Write failed.");
    };

    return 0;
};

int CHDWallet::ExtKeyCreateAccount(CStoredExtKey *sekAccount, CKeyID &idMaster, CExtKeyAccount &ekaOut, const std::string &sLabel)
{
    if (fDebug)
    {
        LogPrint("hdwallet", "%s.\n", __func__);
        AssertLockHeld(cs_wallet);
    };

    std::vector<uint8_t> vAccountPath, vSubKeyPath, v;
    mapEKValue_t::iterator mi = sekAccount->mapValue.find(EKVT_PATH);

    if (mi != sekAccount->mapValue.end())
    {
        vAccountPath = mi->second;
    };

    ekaOut.idMaster = idMaster;
    ekaOut.sLabel = sLabel;
    ekaOut.nFlags |= EAF_ACTIVE;
    ekaOut.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());

    if (sekAccount->kp.IsValidV())
        ekaOut.nFlags |= EAF_HAVE_SECRET;

    CExtKey evExternal, evInternal, evStealth;
    uint32_t nExternal, nInternal, nStealth;
    if (sekAccount->DeriveNextKey(evExternal, nExternal, false) != 0
        || sekAccount->DeriveNextKey(evInternal, nInternal, false) != 0
        || sekAccount->DeriveNextKey(evStealth, nStealth, true) != 0)
    {
        return errorN(1, "Could not derive account chain keys.");
    };

    CStoredExtKey *sekExternal = new CStoredExtKey();
    sekExternal->kp = evExternal;
    vSubKeyPath = vAccountPath;
    sekExternal->mapValue[EKVT_PATH] = PushUInt32(vSubKeyPath, nExternal);
    sekExternal->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT;
    sekExternal->mapValue[EKVT_N_LOOKAHEAD] = SetCompressedInt64(v, N_DEFAULT_EKVT_LOOKAHEAD);

    CStoredExtKey *sekInternal = new CStoredExtKey();
    sekInternal->kp = evInternal;
    vSubKeyPath = vAccountPath;
    sekInternal->mapValue[EKVT_PATH] = PushUInt32(vSubKeyPath, nInternal);
    sekInternal->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT;

    CStoredExtKey *sekStealth = new CStoredExtKey();
    sekStealth->kp = evStealth;
    vSubKeyPath = vAccountPath;
    sekStealth->mapValue[EKVT_PATH] = PushUInt32(vSubKeyPath, nStealth);
    sekStealth->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT;

    ekaOut.vExtKeyIDs.push_back(sekAccount->GetID());
    ekaOut.vExtKeyIDs.push_back(sekExternal->GetID());
    ekaOut.vExtKeyIDs.push_back(sekInternal->GetID());
    ekaOut.vExtKeyIDs.push_back(sekStealth->GetID());

    ekaOut.vExtKeys.push_back(sekAccount);
    ekaOut.vExtKeys.push_back(sekExternal);
    ekaOut.vExtKeys.push_back(sekInternal);
    ekaOut.vExtKeys.push_back(sekStealth);

    ekaOut.nActiveExternal = 1;
    ekaOut.nActiveInternal = 2;
    ekaOut.nActiveStealth = 3;

    if (IsCrypted()
        && ExtKeyEncrypt(&ekaOut, vMasterKey, false) != 0)
    {
        delete sekExternal;
        delete sekInternal;
        delete sekStealth;
        // - sekAccount should be freed in calling function
        return errorN(1, "ExtKeyEncrypt failed.");
    };

    return 0;
};

int CHDWallet::ExtKeyDeriveNewAccount(CHDWalletDB *pwdb, CExtKeyAccount *sea, const std::string &sLabel, const std::string &sPath)
{
    // - derive a new account from the master extkey and save to wallet
    LogPrintf("%s\n", __func__);
    AssertLockHeld(cs_wallet);
    assert(pwdb);
    assert(sea);

    if (IsLocked())
        return errorN(1, "%s: Wallet must be unlocked.", __func__);

    if (!pEKMaster || !pEKMaster->kp.IsValidV())
        return errorN(1, "%s: Master ext key is invalid.", __func__);

    CKeyID idMaster = pEKMaster->GetID();

    CStoredExtKey *sekAccount = new CStoredExtKey();
    CExtKey evAccountKey;
    uint32_t nOldHGen = pEKMaster->GetCounter(true);
    uint32_t nAccount;
    std::vector<uint8_t> vAccountPath, vSubKeyPath;

    if (sPath.length() == 0)
    {
        if (pEKMaster->DeriveNextKey(evAccountKey, nAccount, true, true) != 0)
        {
            delete sekAccount;
            return errorN(1, "%s: Could not derive account key from master.", __func__);
        };
        sekAccount->kp = evAccountKey;
        sekAccount->mapValue[EKVT_PATH] = PushUInt32(vAccountPath, nAccount);
    } else
    {
        std::vector<uint32_t> vPath;
        int rv;
        if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
        {
            delete sekAccount;
            return errorN(1, "%s: ExtractExtKeyPath failed %s.", __func__, ExtKeyGetString(rv));
        };

        CExtKey vkOut;
        CExtKey vkWork = pEKMaster->kp.GetExtKey();
        for (std::vector<uint32_t>::iterator it = vPath.begin(); it != vPath.end(); ++it)
        {
            if (!vkWork.Derive(vkOut, *it))
            {
                delete sekAccount;
                return errorN(1, "%s: CExtKey Derive failed %s, %d.", __func__, sPath.c_str(), *it);
            };
            PushUInt32(vAccountPath, *it);

            vkWork = vkOut;
        };

        sekAccount->kp = vkOut;
        sekAccount->mapValue[EKVT_PATH] = vAccountPath;
    };

    if (!sekAccount->kp.IsValidV()
        || !sekAccount->kp.IsValidP())
    {
        delete sekAccount;
        pEKMaster->SetCounter(nOldHGen, true);
        return errorN(1, "%s: Invalid key.", __func__);
    };

    sekAccount->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT;
    if (0 != ExtKeyCreateAccount(sekAccount, idMaster, *sea, sLabel))
    {
        delete sekAccount;
        pEKMaster->SetCounter(nOldHGen, true);
        return errorN(1, "%s: ExtKeyCreateAccount failed.", __func__);
    };

    CKeyID idAccount = sea->GetID();
    
    CStoredExtKey checkSEA;
    if (pwdb->ReadExtKey(idAccount, checkSEA))
    {
        sea->FreeChains();
        pEKMaster->SetCounter(nOldHGen, true);
        return errorN(14, "%s: Account already exists in db.", __func__);
    };

    if (!pwdb->WriteExtKey(idMaster, *pEKMaster)
        || 0 != ExtKeySaveAccountToDB(pwdb, idAccount, sea))
    {
        sea->FreeChains();
        pEKMaster->SetCounter(nOldHGen, true);
        return errorN(1, "%s: DB Write failed.", __func__);
    };

    if (0 != ExtKeyAddAccountToMaps(idAccount, sea))
    {
        sea->FreeChains();
        return errorN(1, "%s: ExtKeyAddAccountToMaps() failed.", __func__);
    };

    return 0;
};

int CHDWallet::ExtKeySetDefaultAccount(CHDWalletDB *pwdb, CKeyID &idNewDefault)
{
    // Set an existing account as the new master, ensure EAF_ACTIVE is set
    // add account to maps if not already there
    // run in a db transaction
    
    if (fDebug)
        LogPrint("hdwallet", "%s\n", __func__);
    AssertLockHeld(cs_wallet);
    assert(pwdb);
    
    CExtKeyAccount *sea = new CExtKeyAccount();
    
    // - read account from db, inactive accounts are not loaded into maps
    if (!pwdb->ReadExtAccount(idNewDefault, *sea))
    {
        delete sea;
        return errorN(15, "%s: Account not in wallet.", __func__);
    };
    
    // - if !EAF_ACTIVE, rewrite with EAF_ACTIVE set
    if (!(sea->nFlags & EAF_ACTIVE))
    {
        sea->nFlags |= EAF_ACTIVE;
        if (!pwdb->WriteExtAccount(idNewDefault, *sea))
        {
            delete sea;
            return errorN(1, "%s: WriteExtAccount() failed.", __func__);
        };
    };
    
    if (!pwdb->WriteNamedExtKeyId("defaultAccount", idNewDefault))
    {
        delete sea;
        return errorN(1, "%s: WriteNamedExtKeyId() failed.", __func__);
    };
    
    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idNewDefault);
    if (mi == mapExtAccounts.end())
    {
        if (0 != ExtKeyAddAccountToMaps(idNewDefault, sea))
        {
            delete sea;
            return errorN(1, "%s: ExtKeyAddAccountToMaps() failed.", __func__);
        };
        // - sea will be freed in FreeExtKeyMaps
    } else
    {
        delete sea;
    };
    
    // - set idDefaultAccount last, incase something fails.
    idDefaultAccount = idNewDefault;
    
    return 0;
};

int CHDWallet::ExtKeyEncrypt(CStoredExtKey *sek, const CKeyingMaterial &vMKey, bool fLockKey)
{
    if (!sek->kp.IsValidV())
    {
        if (fDebug)
            LogPrint("hdwallet", "%s: sek %s has no secret, encryption skipped.", __func__, sek->GetIDString58());
        return 0;
        //return errorN(1, "Invalid secret.");
    };

    std::vector<uint8_t> vchCryptedSecret;
    CPubKey pubkey = sek->kp.pubkey;
    CKeyingMaterial vchSecret(sek->kp.key.begin(), sek->kp.key.end());
    if (!EncryptSecret(vMKey, vchSecret, pubkey.GetHash(), vchCryptedSecret))
        return errorN(1, "EncryptSecret failed.");

    sek->nFlags |= EAF_IS_CRYPTED;

    sek->vchCryptedSecret = vchCryptedSecret;

    // - CStoredExtKey serialise will never save key when vchCryptedSecret is set
    //   thus key can be left intact here
    if (fLockKey)
    {
        sek->fLocked = 1;
        sek->kp.key.Clear();
    } else
    {
        sek->fLocked = 0;
    };

    return 0;
};

int CHDWallet::ExtKeyEncrypt(CExtKeyAccount *sea, const CKeyingMaterial &vMKey, bool fLockKey)
{
    assert(sea);

    std::vector<CStoredExtKey*>::iterator it;
    for (it = sea->vExtKeys.begin(); it != sea->vExtKeys.end(); ++it)
    {
        CStoredExtKey *sek = *it;
        if (sek->nFlags & EAF_IS_CRYPTED)
            continue;

        if (!sek->kp.IsValidV()
            && fDebug)
        {
            LogPrintf("%s : Skipping account %s chain, no secret.", __func__, sea->GetIDString58().c_str());
            continue;
        };

        if (sek->kp.IsValidV()
            && ExtKeyEncrypt(sek, vMKey, fLockKey) != 0)
            return 1;
    };

    return 0;
};

int CHDWallet::ExtKeyEncryptAll(CHDWalletDB *pwdb, const CKeyingMaterial &vMKey)
{
    LogPrintf("%s\n", __func__);

    // Encrypt loose and account extkeys stored in wallet
    // skip invalid private keys

    Dbc *pcursor = pwdb->GetTxnCursor();

    if (!pcursor)
        return errorN(1, "%s : cannot create DB cursor.", __func__);

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CKeyID ckeyId;
    CBitcoinAddress addr;
    CStoredExtKey sek;
    CExtKeyAccount sea;
    CExtKey58 eKey58;
    std::string strType;

    size_t nKeys = 0;
    size_t nAccounts = 0;

    uint32_t fFlags = DB_SET_RANGE;
    ssKey << std::string("ek32");
    while (pwdb->ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "ek32")
            break;

        ssKey >> ckeyId;
        ssValue >> sek;

        if (!sek.kp.IsValidV())
        {
            if (fDebug)
            {
                addr.Set(ckeyId, CChainParams::EXT_KEY_HASH);
                LogPrint("hdwallet", "%s : Skipping key %s, no secret.", __func__, sek.GetIDString58().c_str());
            };
            continue;
        };

        if (ExtKeyEncrypt(&sek, vMKey, true) != 0)
            return errorN(1, "%s : ExtKeyEncrypt failed.", __func__);

        nKeys++;

        if (!pwdb->Replace(pcursor, sek))
            return errorN(1, "%s : Replace failed.", __func__);
    };

    pcursor->close();

    if (fDebug)
        LogPrint("hdwallet", "%s : Encrypted %u keys, %u accounts.", __func__, nKeys, nAccounts);

    return 0;
};

int CHDWallet::ExtKeyLock()
{
    if (fDebug)
        LogPrint("hdwallet", "ExtKeyLock.\n");

    if (pEKMaster)
    {
        pEKMaster->kp.key.Clear();
        pEKMaster->fLocked = 1;
    };

    // TODO: iterate over mapExtKeys instead?
    ExtKeyAccountMap::iterator mi;
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *sea = mi->second;
        std::vector<CStoredExtKey*>::iterator it;
        for (it = sea->vExtKeys.begin(); it != sea->vExtKeys.end(); ++it)
        {
            CStoredExtKey *sek = *it;
            if (!(sek->nFlags & EAF_IS_CRYPTED))
                continue;
            sek->kp.key.Clear();
            sek->fLocked = 1;
        };
    };

    return 0;
};




int CHDWallet::ExtKeyUnlock(CExtKeyAccount *sea)
{
    return ExtKeyUnlock(sea, vMasterKey);
};

int CHDWallet::ExtKeyUnlock(CExtKeyAccount *sea, const CKeyingMaterial &vMKey)
{
    std::vector<CStoredExtKey*>::iterator it;
    for (it = sea->vExtKeys.begin(); it != sea->vExtKeys.end(); ++it)
    {
        CStoredExtKey *sek = *it;
        if (!(sek->nFlags & EAF_IS_CRYPTED))
            continue;
        if (ExtKeyUnlock(sek, vMKey) != 0)
            return 1;
    };

    return 0;
};

int CHDWallet::ExtKeyUnlock(CStoredExtKey *sek)
{
    return ExtKeyUnlock(sek, vMasterKey);
};

int CHDWallet::ExtKeyUnlock(CStoredExtKey *sek, const CKeyingMaterial &vMKey)
{
    if (!(sek->nFlags & EAF_IS_CRYPTED)) // is not necessary to unlock
        return 0;

    CKeyingMaterial vchSecret;
    uint256 iv = Hash(sek->kp.pubkey.begin(), sek->kp.pubkey.end());
    if (!DecryptSecret(vMKey, sek->vchCryptedSecret, iv, vchSecret)
        || vchSecret.size() != 32)
    {
        return errorN(1, "Failed decrypting ext key %s", sek->GetIDString58().c_str());
    };

    sek->kp.key.Set(vchSecret.begin(), vchSecret.end(), true);

    if (!sek->kp.IsValidV())
        return errorN(1, "Failed decrypting ext key %s", sek->GetIDString58().c_str());

    // - check, necessary?
    if (sek->kp.key.GetPubKey() != sek->kp.pubkey)
        return errorN(1, "Decrypted ext key mismatch %s", sek->GetIDString58().c_str());

    sek->fLocked = 0;
    return 0;
};

int CHDWallet::ExtKeyUnlock(const CKeyingMaterial &vMKey)
{
    if (fDebug)
        LogPrint("hdwallet", "ExtKeyUnlock.\n");

    if (pEKMaster
        && pEKMaster->nFlags & EAF_IS_CRYPTED)
    {
        if (ExtKeyUnlock(pEKMaster, vMKey) != 0)
            return 1;
    };

    ExtKeyAccountMap::iterator mi;
    mi = mapExtAccounts.begin();
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *sea = mi->second;

        if (ExtKeyUnlock(sea, vMKey) != 0)
            return errorN(1, "ExtKeyUnlock() account failed.");
    };

    return 0;
};


int CHDWallet::ExtKeyCreateInitial(CHDWalletDB *pwdb)
{
    LogPrintf("Creating intital extended master key and account.\n");

    CKeyID idMaster;

    if (!pwdb->TxnBegin())
        return errorN(1, "TxnBegin failed.");

    if (ExtKeyNewMaster(pwdb, idMaster, true) != 0
        || ExtKeySetMaster(pwdb, idMaster) != 0)
    {
        pwdb->TxnAbort();
        return errorN(1, "Make or SetNewMasterKey failed.");
    };

    CExtKeyAccount *seaDefault = new CExtKeyAccount();

    if (ExtKeyDeriveNewAccount(pwdb, seaDefault, "default") != 0)
    {
        delete seaDefault;
        pwdb->TxnAbort();
        return errorN(1, "DeriveNewExtAccount failed.");
    };

    idDefaultAccount = seaDefault->GetID();
    if (!pwdb->WriteNamedExtKeyId("defaultAccount", idDefaultAccount))
    {
        pwdb->TxnAbort();
        return errorN(1, "WriteNamedExtKeyId failed.");
    };

    CPubKey newKey;
    if (0 != NewKeyFromAccount(pwdb, idDefaultAccount, newKey, false, false))
    {
        pwdb->TxnAbort();
        return errorN(1, "NewKeyFromAccount failed.");
    };

    CEKAStealthKey aks;
    std::string strLbl = "Default Stealth Address";
    if (0 != NewStealthKeyFromAccount(pwdb, idDefaultAccount, strLbl, aks, 0, NULL))
    {
        pwdb->TxnAbort();
        return errorN(1, "NewStealthKeyFromAccount failed.");
    };

    if (!pwdb->TxnCommit())
    {
        // --TxnCommit destroys activeTxn
        return errorN(1, "TxnCommit failed.");
    };

    SetAddressBook(CBitcoinAddress(newKey.GetID()).Get(), "Default Address", "receive");

    return 0;
}

int CHDWallet::ExtKeyLoadMaster()
{
    LogPrintf("Loading master ext key.\n");

    LOCK(cs_wallet);

    CKeyID idMaster;

    CHDWalletDB wdb(strWalletFile, "r+");
    if (!wdb.ReadNamedExtKeyId("master", idMaster))
    {
        int nValue;
        if (!wdb.ReadFlag("madeDefaultEKey", nValue)
            || nValue == 0)
        {
            /*
            if (IsLocked())
            {
                fMakeExtKeyInitials = true; // set flag for unlock
                LogPrintf("Wallet locked, master key will be created when unlocked.\n");
                return 0;
            };

            if (ExtKeyCreateInitial(&wdb) != 0)
                return errorN(1, "ExtKeyCreateDefaultMaster failed.");
            
            return 0;
            */
        };
        LogPrintf("Warning: No master ext key has been set.\n");
        return 1;
    };

    pEKMaster = new CStoredExtKey();
    if (!wdb.ReadExtKey(idMaster, *pEKMaster))
    {
        delete pEKMaster;
        pEKMaster = NULL;
        return errorN(1, "ReadExtKey failed to read master ext key.");
    };

    if (!pEKMaster->kp.IsValidP()) // wallet could be locked, check pk
    {
        delete pEKMaster;
        pEKMaster = NULL;
        return errorN(1, " Loaded master ext key is invalid %s.", pEKMaster->GetIDString58().c_str());
    };

    if (pEKMaster->nFlags & EAF_IS_CRYPTED)
        pEKMaster->fLocked = 1;

    // - add to key map
    mapExtKeys[idMaster] = pEKMaster;

    // find earliest key creation time, as wallet birthday
    int64_t nCreatedAt;
    GetCompressedInt64(pEKMaster->mapValue[EKVT_CREATED_AT], (uint64_t&)nCreatedAt);

    if (!nTimeFirstKey || (nCreatedAt && nCreatedAt < nTimeFirstKey))
        nTimeFirstKey = nCreatedAt;

    return 0;
};

int CHDWallet::ExtKeyLoadAccounts()
{
    LogPrintf("Loading ext accounts.\n");

    LOCK(cs_wallet);

    CHDWalletDB wdb(strWalletFile);

    if (!wdb.ReadNamedExtKeyId("defaultAccount", idDefaultAccount))
    {
        LogPrintf("Warning: No default ext account set.\n");
    };

    Dbc *pcursor;
    if (!(pcursor = wdb.GetCursor()))
        throw std::runtime_error(strprintf("%s: cannot create DB cursor", __func__).c_str());

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CBitcoinAddress addr;
    CKeyID idAccount;
    std::string strType;

    unsigned int fFlags = DB_SET_RANGE;
    ssKey << std::string("eacc");
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "eacc")
            break;

        ssKey >> idAccount;

        if (fDebug)
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrint("hdwallet", "Loading account %s\n", addr.ToString().c_str());
        };

        CExtKeyAccount *sea = new CExtKeyAccount();
        ssValue >> *sea;

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi != mapExtAccounts.end())
        {
            // - account already loaded, skip, can be caused by ExtKeyCreateInitial()
            if (fDebug)
                LogPrint("hdwallet", "Account already loaded.\n");
            continue;
        };

        if (!(sea->nFlags & EAF_ACTIVE))
        {
            if (fDebug)
            {
                addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
                LogPrint("hdwallet", "Skipping inactive %s\n", addr.ToString().c_str());
            };
            delete sea;
            continue;
        };

        // - find earliest key creation time, as wallet birthday
        int64_t nCreatedAt;
        GetCompressedInt64(sea->mapValue[EKVT_CREATED_AT], (uint64_t&)nCreatedAt);

        if (!nTimeFirstKey || (nCreatedAt && nCreatedAt < nTimeFirstKey))
            nTimeFirstKey = nCreatedAt;

        sea->vExtKeys.resize(sea->vExtKeyIDs.size());
        for (size_t i = 0; i < sea->vExtKeyIDs.size(); ++i)
        {
            CKeyID &id = sea->vExtKeyIDs[i];
            CStoredExtKey *sek = new CStoredExtKey();

            if (wdb.ReadExtKey(id, *sek))
            {
                sea->vExtKeys[i] = sek;
            } else
            {
                addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
                LogPrintf("WARNING: Could not read key %d of account %s\n", i, addr.ToString().c_str());
                sea->vExtKeys[i] = NULL;
                delete sek;
            };
        };

        if (0 != ExtKeyAddAccountToMaps(idAccount, sea))
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("ExtKeyAddAccountToMaps() failed: %s\n", addr.ToString().c_str());
            sea->FreeChains();
            delete sea;
        };
    };

    pcursor->close();

    return 0;
};

int CHDWallet::ExtKeySaveAccountToDB(CHDWalletDB *pwdb, CKeyID &idAccount, CExtKeyAccount *sea)
{
    if (fDebug)
    {
        LogPrint("hdwallet", "ExtKeySaveAccountToDB()\n");
        AssertLockHeld(cs_wallet);
    };
    assert(sea);

    for (size_t i = 0; i < sea->vExtKeys.size(); ++i)
    {
        CStoredExtKey *sek = sea->vExtKeys[i];
        if (!pwdb->WriteExtKey(sea->vExtKeyIDs[i], *sek))
            return errorN(1, "ExtKeySaveAccountToDB(): WriteExtKey failed.");
    };

    if (!pwdb->WriteExtAccount(idAccount, *sea))
        return errorN(1, "ExtKeySaveAccountToDB() WriteExtAccount failed.");

    return 0;
};

int CHDWallet::ExtKeyAddAccountToMaps(CKeyID &idAccount, CExtKeyAccount *sea)
{
    // - open/activate account in wallet
    //   add to mapExtAccounts and mapExtKeys

    if (fDebug)
    {
        LogPrint("hdwallet", "ExtKeyAddAccountToMap()\n");
        AssertLockHeld(cs_wallet);
    };
    assert(sea);

    for (size_t i = 0; i < sea->vExtKeys.size(); ++i)
    {
        CStoredExtKey *sek = sea->vExtKeys[i];

        if (sek->nFlags & EAF_IS_CRYPTED)
            sek->fLocked = 1;

        if (sek->nFlags & EAF_ACTIVE
            && sek->nFlags & EAF_RECEIVE_ON)
        {
            uint64_t nLookAhead = N_DEFAULT_LOOKAHEAD;

            mapEKValue_t::iterator itV = sek->mapValue.find(EKVT_N_LOOKAHEAD);
            if (itV != sek->mapValue.end())
                nLookAhead = GetCompressedInt64(itV->second, nLookAhead);

            sea->AddLookAhead(i, (uint32_t)nLookAhead);
            //sea->AddLookBehind(i, (uint32_t)nLookAhead/2);
        };

        mapExtKeys[sea->vExtKeyIDs[i]] = sek;
    };

    mapExtAccounts[idAccount] = sea;
    return 0;
};

int CHDWallet::ExtKeyRemoveAccountFromMapsAndFree(CExtKeyAccount *sea)
{
    // - remove account keys from wallet maps and free
    if (!sea)
        return 0;
    
    CKeyID idAccount = sea->GetID();
    
    for (size_t i = 0; i < sea->vExtKeys.size(); ++i)
        mapExtKeys.erase(sea->vExtKeyIDs[i]);
    
    mapExtAccounts.erase(idAccount);
    sea->FreeChains();
    delete sea;
    return 0;
};

int CHDWallet::ExtKeyLoadAccountPacks()
{
    LogPrintf("Loading ext account packs.\n");

    LOCK(cs_wallet);

    CHDWalletDB wdb(strWalletFile);

    Dbc *pcursor;
    if (!(pcursor = wdb.GetCursor()))
        throw std::runtime_error(strprintf("%s : cannot create DB cursor", __func__).c_str());

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CKeyID idAccount;
    CBitcoinAddress addr;
    uint32_t nPack;
    std::string strType;
    std::vector<CEKAKeyPack> ekPak;
    std::vector<CEKAStealthKeyPack> aksPak;
    std::vector<CEKASCKeyPack> asckPak;

    unsigned int fFlags = DB_SET_RANGE;
    ssKey << std::string("epak");
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        ekPak.clear();
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "epak")
            break;

        ssKey >> idAccount;
        ssKey >> nPack;

        if (fDebug)
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrint("hdwallet", "Loading account key pack %s %u\n", addr.ToString().c_str(), nPack);
        };

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi == mapExtAccounts.end())
        {
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Warning: Unknown account %s.\n", addr.ToString().c_str());
            continue;
        };

        CExtKeyAccount *sea = mi->second;

        ssValue >> ekPak;

        std::vector<CEKAKeyPack>::iterator it;
        for (it = ekPak.begin(); it != ekPak.end(); ++it)
        {
            sea->mapKeys[it->id] = it->ak;
        };
    };
    
    size_t nStealthKeys = 0;
    ssKey.clear();
    ssKey << std::string("espk");
    fFlags = DB_SET_RANGE;
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        aksPak.clear();
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "espk")
            break;

        ssKey >> idAccount;
        ssKey >> nPack;

        if (fDebug)
            LogPrint("hdwallet", "Loading account stealth key pack %s %u\n", idAccount.ToString().c_str(), nPack);

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi == mapExtAccounts.end())
        {
            CBitcoinAddress addr;
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Warning: Unknown account %s.\n", addr.ToString().c_str());
            continue;
        };

        CExtKeyAccount *sea = mi->second;

        ssValue >> aksPak;
        
        
        std::vector<CEKAStealthKeyPack>::iterator it;
        for (it = aksPak.begin(); it != aksPak.end(); ++it)
        {
            nStealthKeys++;
            sea->mapStealthKeys[it->id] = it->aks;
        };
    };
    
    if (fDebug)
        LogPrint("hdwallet", "Loaded %d stealthkey%s.\n", nStealthKeys, nStealthKeys == 1 ? "" : "s");

    ssKey.clear();
    ssKey << std::string("ecpk");
    fFlags = DB_SET_RANGE;
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        aksPak.clear();
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "ecpk")
            break;

        ssKey >> idAccount;
        ssKey >> nPack;

        if (fDebug)
            LogPrint("hdwallet", "Loading account stealth child key pack %s %u\n", idAccount.ToString().c_str(), nPack);

        ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
        if (mi == mapExtAccounts.end())
        {
            CBitcoinAddress addr;
            addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
            LogPrintf("Warning: Unknown account %s.\n", addr.ToString().c_str());
            continue;
        };

        CExtKeyAccount *sea = mi->second;

        ssValue >> asckPak;

        std::vector<CEKASCKeyPack>::iterator it;
        for (it = asckPak.begin(); it != asckPak.end(); ++it)
        {
            sea->mapStealthChildKeys[it->id] = it->asck;
        };
    };

    pcursor->close();

    return 0;
};

int CHDWallet::ExtKeyAppendToPack(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &idKey, CEKAKey &ak, bool &fUpdateAcc) const
{
    // - must call WriteExtAccount after


    CKeyID idAccount = sea->GetID();
    std::vector<CEKAKeyPack> ekPak;
    if (!pwdb->ReadExtKeyPack(idAccount, sea->nPack, ekPak))
    {
        // -- new pack
        ekPak.clear();
        if (fDebug)
            LogPrint("hdwallet", "Account %s, starting new keypack %u.\n", idAccount.ToString(), sea->nPack);
    };

    try { ekPak.push_back(CEKAKeyPack(idKey, ak)); } catch (std::exception& e)
    {
        return errorN(1, "%s push_back failed.", __func__, sea->nPack);
    };

    if (!pwdb->WriteExtKeyPack(idAccount, sea->nPack, ekPak))
    {
        return errorN(1, "%s Save key pack %u failed.", __func__, sea->nPack);
    };

    fUpdateAcc = false;
    if ((uint32_t)ekPak.size() >= MAX_KEY_PACK_SIZE-1)
    {
        fUpdateAcc = true;
        sea->nPack++;
    };
    return 0;
};

int CHDWallet::ExtKeyAppendToPack(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &idKey, CEKASCKey &asck, bool &fUpdateAcc) const
{

    // - must call WriteExtAccount after

    CKeyID idAccount = sea->GetID();
    std::vector<CEKASCKeyPack> asckPak;
    if (!pwdb->ReadExtStealthKeyChildPack(idAccount, sea->nPackStealthKeys, asckPak))
    {
        // -- new pack
        asckPak.clear();
        if (fDebug)
            LogPrint("hdwallet", "Account %s, starting new stealth child keypack %u.\n", idAccount.ToString(), sea->nPackStealthKeys);
    };

    try { asckPak.push_back(CEKASCKeyPack(idKey, asck)); } catch (std::exception& e)
    {
        return errorN(1, "%s push_back failed.", __func__);
    };

    if (!pwdb->WriteExtStealthKeyChildPack(idAccount, sea->nPackStealthKeys, asckPak))
        return errorN(1, "%s Save key pack %u failed.", __func__, sea->nPackStealthKeys);

    fUpdateAcc = false;
    if ((uint32_t)asckPak.size() >= MAX_KEY_PACK_SIZE-1)
    {
        sea->nPackStealthKeys++;
        fUpdateAcc = true;
    };

    return 0;
};

int CHDWallet::ExtKeySaveKey(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &keyId, CEKAKey &ak) const
{
    if (fDebug)
    {
        CBitcoinAddress addr(keyId);
        LogPrint("hdwallet", "%s %s %s.\n", __func__, sea->GetIDString58().c_str(), addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    if (!sea->SaveKey(keyId, ak))
        return errorN(1, "%s SaveKey failed.", __func__);

    bool fUpdateAcc;
    if (0 != ExtKeyAppendToPack(pwdb, sea, keyId, ak, fUpdateAcc))
        return errorN(1, "%s ExtKeyAppendToPack failed.", __func__);

    CStoredExtKey *pc = sea->GetChain(ak.nParent);
    if (!pc)
        return errorN(1, "%s GetChain failed.", __func__);

    CKeyID idChain = sea->vExtKeyIDs[ak.nParent];
    if (!pwdb->WriteExtKey(idChain, *pc))
        return errorN(1, "%s WriteExtKey failed.", __func__);

    if (fUpdateAcc) // only neccessary if nPack has changed
    {
        CKeyID idAccount = sea->GetID();
        if (!pwdb->WriteExtAccount(idAccount, *sea))
            return errorN(1, "%s WriteExtAccount failed.", __func__);
    };

    return 0;
};

int CHDWallet::ExtKeySaveKey(CExtKeyAccount *sea, const CKeyID &keyId, CEKAKey &ak) const
{
    //LOCK(cs_wallet);
    if (fDebug)
        AssertLockHeld(cs_wallet);

    CHDWalletDB wdb(strWalletFile, "r+");

    if (!wdb.TxnBegin())
        return errorN(1, "%s TxnBegin failed.", __func__);

    if (0 != ExtKeySaveKey(&wdb, sea, keyId, ak))
    {
        wdb.TxnAbort();
        return 1;
    };

    if (!wdb.TxnCommit())
        return errorN(1, "%s TxnCommit failed.", __func__);
    return 0;
};

int CHDWallet::ExtKeySaveKey(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &keyId, CEKASCKey &asck) const
{
    if (fDebug)
    {
        CBitcoinAddress addr(keyId);
        LogPrint("hdwallet", "%s: %s %s.\n", __func__, sea->GetIDString58().c_str(), addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    if (!sea->SaveKey(keyId, asck))
        return errorN(1, "%s SaveKey failed.", __func__);

    bool fUpdateAcc;
    if (0 != ExtKeyAppendToPack(pwdb, sea, keyId, asck, fUpdateAcc))
        return errorN(1, "%s ExtKeyAppendToPack failed.", __func__);

    if (fUpdateAcc) // only neccessary if nPackStealth has changed
    {
        CKeyID idAccount = sea->GetID();
        if (!pwdb->WriteExtAccount(idAccount, *sea))
            return errorN(1, "%s WriteExtAccount failed.", __func__);
    };

    return 0;
};

int CHDWallet::ExtKeySaveKey(CExtKeyAccount *sea, const CKeyID &keyId, CEKASCKey &asck) const
{
    if (fDebug)
        AssertLockHeld(cs_wallet);

    CHDWalletDB wdb(strWalletFile, "r+");

    if (!wdb.TxnBegin())
        return errorN(1, "%s TxnBegin failed.", __func__);

    if (0 != ExtKeySaveKey(&wdb, sea, keyId, asck))
    {
        wdb.TxnAbort();
        return 1;
    };

    if (!wdb.TxnCommit())
        return errorN(1, "%s TxnCommit failed.", __func__);
    return 0;
};

int CHDWallet::ExtKeyUpdateStealthAddress(CHDWalletDB *pwdb, CExtKeyAccount *sea, CKeyID &sxId, std::string &sLabel)
{
    if (fDebug)
    {
        LogPrint("hdwallet", "%s.\n", __func__);
        AssertLockHeld(cs_wallet);
    };

    AccStealthKeyMap::iterator it = sea->mapStealthKeys.find(sxId);
    if (it == sea->mapStealthKeys.end())
        return errorN(1, "%s: Stealth key not in account.", __func__);


    if (it->second.sLabel == sLabel)
        return 0; // no change

    CKeyID accId = sea->GetID();
    std::vector<CEKAStealthKeyPack> aksPak;
    for (uint32_t i = 0; i <= sea->nPackStealth; ++i)
    {
        if (!pwdb->ReadExtStealthKeyPack(accId, i, aksPak))
            return errorN(1, "%s: ReadExtStealthKeyPack %d failed.", __func__, i);

        std::vector<CEKAStealthKeyPack>::iterator itp;
        for (itp = aksPak.begin(); itp != aksPak.end(); ++itp)
        {
            if (itp->id == sxId)
            {
                itp->aks.sLabel = sLabel;
                if (!pwdb->WriteExtStealthKeyPack(accId, i, aksPak))
                    return errorN(1, "%s: WriteExtStealthKeyPack %d failed.", __func__, i);

                it->second.sLabel = sLabel;

                return 0;
            };
        };
    };

    return errorN(1, "%s: Stealth key not in db.", __func__);
};

int CHDWallet::ExtKeyNewIndex(CHDWalletDB *pwdb, const CKeyID &idKey, uint32_t &index)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idKey, CChainParams::EXT_ACC_HASH); // could be a loose key also
        LogPrint("hdwallet", "%s %s.\n", __func__, addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };
    
    std::string sPrefix = "ine";
    uint32_t lastId = 0xFFFFFFFF;
    bool fInsertLast = false;
    
    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey << std::make_pair(sPrefix, lastId); // set to last possible key
    
    Dbc *pcursor = pwdb->InTxn() ? pwdb->GetTxnCursor() : pwdb->GetCursor();
    if (!pcursor)
        return errorN(1, "%s: Could not get wallet database %s cursor, inTxn.\n", __func__, pwdb->InTxn() ? "txn" : "");
    
    index = 1;
    if (pwdb->ReadPrevKeyAtCursor(pcursor, ssKey, fInsertLast) == 0)
    {
        std::string strType;
        ssKey >> strType;
        if (strType == sPrefix)
        {
            uint32_t prevId;
            ssKey >> prevId;
            index = prevId+1;
        };
    } else
    {
        // ReadPrevKeyAtCursor is not expected to fail if fInsertLast is false
        if (!fInsertLast)
        {
            pcursor->close();
            return errorN(1, "%s: ReadPrevKeyAtCursor failed.\n", __func__);
        };
    };
    
    pcursor->close();
    
    if (index == lastId)
        return errorN(1, "%s: Wallet extkey index is full!\n", __func__); // expect multiple wallets per node before anyone hits this
    
    LogPrint("hdwallet", "%s: New index %u.\n", __func__, index);
    
    if (fInsertLast)
    {
        CKeyID dummy;
        if (!pwdb->WriteExtKeyIndex(lastId, dummy))
            return errorN(1, "%s: WriteExtKeyIndex failed.\n", __func__);
    };
    
    if (!pwdb->WriteExtKeyIndex(index, idKey))
        return errorN(1, "%s: WriteExtKeyIndex failed.\n", __func__);
    
    return 0;
};

int CHDWallet::ExtKeyGetIndex(CHDWalletDB *pwdb, CExtKeyAccount *sea, uint32_t &index, bool &fUpdate)
{
    mapEKValue_t::iterator mi = sea->mapValue.find(EKVT_INDEX);
    if (mi != sea->mapValue.end())
    {
        assert(mi->second.size() == 4);
        memcpy(&index, &mi->second[0], 4);
        return 0;
    };
    
    CKeyID idAccount = sea->GetID();
    if (0 != ExtKeyNewIndex(pwdb, idAccount, index))
        return errorN(1, "%s ExtKeyNewIndex failed.", __func__);
    std::vector<uint8_t> vTmp;
    sea->mapValue[EKVT_INDEX] = PushUInt32(vTmp, index);
    fUpdate = true;
    
    return 0;
};

int CHDWallet::NewKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount, CPubKey &pkOut, bool fInternal, bool fHardened, const char *plabel)
{
    // - if plabel is not null, add to mapAddressBook
    
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
        LogPrint("hdwallet", "%s %s.\n", __func__, addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (fHardened
        && IsLocked())
        return errorN(1, "%s Wallet must be unlocked to derive hardened keys.", __func__);

    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
    if (mi == mapExtAccounts.end())
        return errorN(2, "%s Unknown account.", __func__);

    CExtKeyAccount *sea = mi->second;
    CStoredExtKey *sek = NULL;

    uint32_t nExtKey = fInternal ? sea->nActiveInternal : sea->nActiveExternal;

    if (nExtKey < sea->vExtKeys.size())
        sek = sea->vExtKeys[nExtKey];

    if (!sek)
        return errorN(3, "%s Unknown chain.", __func__);

    uint32_t nChildBkp = fHardened ? sek->nHGenerated : sek->nGenerated;
    uint32_t nChildOut;
    if (0 != sek->DeriveNextKey(pkOut, nChildOut, fHardened))
        return errorN(4, "%s Derive failed.", __func__);

    CEKAKey ks(nExtKey, nChildOut);
    CKeyID idKey = pkOut.GetID();

    bool fUpdateAcc;
    if (0 != ExtKeyAppendToPack(pwdb, sea, idKey, ks, fUpdateAcc))
    {
        sek->SetCounter(nChildBkp, fHardened);
        return errorN(5, "%s ExtKeyAppendToPack failed.", __func__);
    };

    if (!pwdb->WriteExtKey(sea->vExtKeyIDs[nExtKey], *sek))
    {
        sek->SetCounter(nChildBkp, fHardened);
        return errorN(6, "%s Save account chain failed.", __func__);
    };

    std::vector<uint32_t> vPath;
    uint32_t idIndex;
    if (plabel)
    {
        bool requireUpdateDB;
        if (0 == ExtKeyGetIndex(pwdb, sea, idIndex, requireUpdateDB))
            vPath.push_back(idIndex); // first entry is the index to the account / master key
        fUpdateAcc = requireUpdateDB ? true : fUpdateAcc;
    };

    if (fUpdateAcc)
    {
        CKeyID idAccount = sea->GetID();
        if (!pwdb->WriteExtAccount(idAccount, *sea))
        {
            sek->SetCounter(nChildBkp, fHardened);
            return errorN(7, "%s Save account chain failed.", __func__);
        };
    };

    sea->SaveKey(idKey, ks); // remove from lookahead, add to pool, add new lookahead

    if (plabel)
    {
        if (0 == AppendChainPath(sek, vPath))
        {
            vPath.push_back(ks.nKey);
        } else
        {
            LogPrintf("Warning: %s - missing path value.\n", __func__);
            vPath.clear();
        };
        SetAddressBook(pwdb, idKey, plabel, "receive", vPath, false);
    };

    return 0;
};

int CHDWallet::NewKeyFromAccount(CPubKey &pkOut, bool fInternal, bool fHardened, const char *plabel)
{
    {
        LOCK(cs_wallet);
        CHDWalletDB wdb(strWalletFile, "r+");

        if (!wdb.TxnBegin())
            return errorN(1, "%s TxnBegin failed.", __func__);

        if (0 != NewKeyFromAccount(&wdb, idDefaultAccount, pkOut, fInternal, fHardened, plabel))
        {
            wdb.TxnAbort();
            return 1;
        };

        if (!wdb.TxnCommit())
            return errorN(1, "%s TxnCommit failed.", __func__);
    }
    
    AddressBookChangedNotify(pkOut.GetID(), CT_NEW);
    return 0;
};

int CHDWallet::NewStealthKeyFromAccount(
    CHDWalletDB *pwdb, const CKeyID &idAccount, std::string &sLabel,
    CEKAStealthKey &akStealthOut, uint32_t nPrefixBits, const char *pPrefix)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
        LogPrint("hdwallet","%s %s.\n", __func__, addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "%s Wallet must be unlocked to derive hardened keys.", __func__);

    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s Unknown account.", __func__);

    CExtKeyAccount *sea = mi->second;
    uint32_t nChain = sea->nActiveStealth;
    if (nChain >= sea->vExtKeys.size())
        return errorN(1, "%s Stealth chain unknown %d.", __func__, nChain);

    CStoredExtKey *sek = sea->vExtKeys[nChain];


    // - scan secrets must be stored uncrypted - always derive hardened keys

    uint32_t nChildBkp = sek->nHGenerated;

    CKey kScan, kSpend;
    uint32_t nScanOut, nSpendOut;
    if (0 != sek->DeriveNextKey(kScan, nScanOut, true))
        return errorN(1, "%s Derive failed.", __func__);

    if (0 != sek->DeriveNextKey(kSpend, nSpendOut, true))
    {
        sek->SetCounter(nChildBkp, true);
        return errorN(1, "%s Derive failed.", __func__);
    };
    
    uint32_t nPrefix = 0;
    if (pPrefix)
    {
        if (!ExtractStealthPrefix(pPrefix, nPrefix))
            return errorN(1, "%s ExtractStealthPrefix.", __func__);
    } else
    if (nPrefixBits > 0)
    {
        // if pPrefix is null, set nPrefix from the hash of kSpend
        
        uint8_t tmp32[32];
        CSHA256().Write(kSpend.begin(), 32).Finalize(tmp32);
        memcpy(&nPrefix, tmp32, 4);
    };
    uint32_t nMask = SetStealthMask(nPrefixBits);
    
    nPrefix = nPrefix & nMask;

    CEKAStealthKey aks(nChain, nScanOut, kScan, nChain, nSpendOut, kSpend, nPrefixBits, nPrefix);
    aks.sLabel = sLabel;
    
    CStealthAddress sxAddr;
    if (0 != aks.SetSxAddr(sxAddr))
        return errorN(1, "%s SetSxAddr failed.", __func__);
    
    // Set path for address book
    std::vector<uint32_t> vPath;
    uint32_t idIndex;
    bool requireUpdateDB;
    if (0 == ExtKeyGetIndex(pwdb, sea, idIndex, requireUpdateDB))
        vPath.push_back(idIndex); // first entry is the index to the account / master key
    
    if (0 == AppendChainPath(sek, vPath))
    {
        uint32_t nChild = nScanOut;
        vPath.push_back(SetHardenedBit(nChild));
    } else
    {
        LogPrintf("Warning: %s - missing path value.\n", __func__);
        vPath.clear();
    };
    
    std::vector<CEKAStealthKeyPack> aksPak;
    
    CKeyID idKey = aks.GetID();
    sea->mapStealthKeys[idKey] = aks;
    
    if (!pwdb->ReadExtStealthKeyPack(idAccount, sea->nPackStealth, aksPak))
    {
        // -- new pack
        aksPak.clear();
        if (fDebug)
            LogPrint("hdwallet", "Account %s, starting new stealth keypack %u.\n", idAccount.ToString(), sea->nPackStealth);
    };
    
    aksPak.push_back(CEKAStealthKeyPack(idKey, aks));
    
    if (!pwdb->WriteExtStealthKeyPack(idAccount, sea->nPackStealth, aksPak))
    {
        sea->mapStealthKeys.erase(idKey);
        sek->SetCounter(nChildBkp, true);
        return errorN(1, "%s Save key pack %u failed.", __func__, sea->nPackStealth);
    };
    
    if ((uint32_t)aksPak.size() >= MAX_KEY_PACK_SIZE-1)
        sea->nPackStealth++;

    if (!pwdb->WriteExtKey(sea->vExtKeyIDs[nChain], *sek))
    {
        sea->mapStealthKeys.erase(idKey);
        sek->SetCounter(nChildBkp, true);
        return errorN(1, "%s Save account chain failed.", __func__);
    };
    
    SetAddressBook(pwdb, sxAddr, sLabel, "receive", vPath, false);
    
    akStealthOut = aks;
    return 0;
};

int CHDWallet::NewStealthKeyFromAccount(std::string &sLabel, CEKAStealthKey &akStealthOut, uint32_t nPrefixBits, const char *pPrefix)
{
    {
        LOCK(cs_wallet);
        CHDWalletDB wdb(strWalletFile, "r+");

        if (!wdb.TxnBegin())
            return errorN(1, "%s TxnBegin failed.", __func__);

        if (0 != NewStealthKeyFromAccount(&wdb, idDefaultAccount, sLabel, akStealthOut, nPrefixBits, pPrefix))
        {
            wdb.TxnAbort();
            return 1;
        };

        if (!wdb.TxnCommit())
            return errorN(1, "%s TxnCommit failed.", __func__);
    }
    CStealthAddress sxAddr;
    akStealthOut.SetSxAddr(sxAddr);
    AddressBookChangedNotify(sxAddr, CT_NEW);
    return 0;
};

int CHDWallet::NewExtKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount,
    std::string &sLabel, CStoredExtKey *sekOut, const char *plabel, uint32_t *childNo)
{
    if (fDebug)
    {
        CBitcoinAddress addr;
        addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
        LogPrint("hdwallet", "%s %s.\n", __func__, addr.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    assert(pwdb);

    if (IsLocked())
        return errorN(1, "%s Wallet must be unlocked to derive hardened keys.", __func__);

    bool fHardened = false; // TODO: make option

    ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
    if (mi == mapExtAccounts.end())
        return errorN(1, "%s Unknown account.", __func__);

    CExtKeyAccount *sea = mi->second;

    CStoredExtKey *sekAccount = sea->ChainAccount();
    if (!sekAccount)
        return errorN(1, "%s Unknown chain.", __func__);

    std::vector<uint8_t> vAccountPath, v;
    mapEKValue_t::iterator mvi = sekAccount->mapValue.find(EKVT_PATH);
    if (mvi != sekAccount->mapValue.end())
        vAccountPath = mvi->second;

    CExtKey evNewKey;

    uint32_t nOldGen = sekAccount->GetCounter(fHardened);
    uint32_t nNewChildNo;
    
    if (childNo)
    {
        if ((0 != sekAccount->DeriveKey(evNewKey, *childNo, nNewChildNo, fHardened)) != 0)
            return errorN(1, "DeriveKey failed.");
    } else
    {
        if (sekAccount->DeriveNextKey(evNewKey, nNewChildNo, fHardened) != 0)
            return errorN(1, "DeriveNextKey failed.");
    };
    
    sekOut->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT;
    sekOut->kp = evNewKey;
    sekOut->mapValue[EKVT_PATH] = PushUInt32(vAccountPath, nNewChildNo);
    sekOut->mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());
    sekOut->sLabel = sLabel;

    if (IsCrypted()
        && ExtKeyEncrypt(sekOut, vMasterKey, false) != 0)
    {
        sekAccount->SetCounter(nOldGen, fHardened);
        return errorN(1, "ExtKeyEncrypt failed.");
    };

    size_t chainNo = sea->vExtKeyIDs.size();
    CKeyID idNewChain = sekOut->GetID();
    sea->vExtKeyIDs.push_back(idNewChain);
    sea->vExtKeys.push_back(sekOut);
    
    if (plabel)
    {
        std::vector<uint32_t> vPath;
        uint32_t idIndex;
        bool requireUpdateDB;
        if (0 == ExtKeyGetIndex(pwdb, sea, idIndex, requireUpdateDB))
            vPath.push_back(idIndex); // first entry is the index to the account / master key
            
        vPath.push_back(nNewChildNo);
        SetAddressBook(pwdb, sekOut->kp, plabel, "receive", vPath, false);
    };

    if (!pwdb->WriteExtAccount(idAccount, *sea)
        || !pwdb->WriteExtKey(idAccount, *sekAccount)
        || !pwdb->WriteExtKey(idNewChain, *sekOut))
    {
        sekAccount->SetCounter(nOldGen, fHardened);
        return errorN(1, "DB Write failed.");
    };

    uint64_t nLookAhead = N_DEFAULT_LOOKAHEAD;
    mvi = sekOut->mapValue.find(EKVT_N_LOOKAHEAD);
    if (mvi != sekOut->mapValue.end())
        nLookAhead = GetCompressedInt64(mvi->second, nLookAhead);
    sea->AddLookAhead(chainNo, nLookAhead);
    
    mapExtKeys[idNewChain] = sekOut;

    return 0;
};

int CHDWallet::NewExtKeyFromAccount(std::string &sLabel, CStoredExtKey *sekOut,
    const char *plabel, uint32_t *childNo)
{
    {
        LOCK(cs_wallet);
        CHDWalletDB wdb(strWalletFile, "r+");

        if (!wdb.TxnBegin())
            return errorN(1, "%s TxnBegin failed.", __func__);

        if (0 != NewExtKeyFromAccount(&wdb, idDefaultAccount, sLabel, sekOut, plabel, childNo))
        {
            wdb.TxnAbort();
            return 1;
        };

        if (!wdb.TxnCommit())
            return errorN(1, "%s TxnCommit failed.", __func__);
    }
    AddressBookChangedNotify(sekOut->kp, CT_NEW);
    return 0;
};

int CHDWallet::ExtKeyGetDestination(const CExtKeyPair &ek, CPubKey &pkDest, uint32_t &nKey)
{
    if (fDebug)
    {
        CExtKey58 ek58;
        ek58.SetKeyP(ek);
        LogPrint("hdwallet", "%s: %s.\n", __func__, ek58.ToString().c_str());
        AssertLockHeld(cs_wallet);
    };

    /*
        get the next destination,
        if key is not saved yet, return 1st key
        don't save key here, save after derived key has been sucessfully used
    */

    CKeyID keyId = ek.GetID();

    CHDWalletDB wdb(strWalletFile, "r+");

    CStoredExtKey sek;
    if (wdb.ReadExtKey(keyId, sek))
    {
        if (0 != sek.DeriveNextKey(pkDest, nKey))
            return errorN(1, "%s: DeriveNextKey failed.", __func__);
        return 0;
    } else
    {
        nKey = 0; // AddLookAhead starts from 0
        for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
        {
            if (ek.Derive(pkDest, nKey))
            {
                return 0;
            };
            nKey++;
        };
    };

    return errorN(1, "%s: Could not derive key.", __func__);
};

int CHDWallet::ExtKeyUpdateLooseKey(const CExtKeyPair &ek, uint32_t nKey, bool fAddToAddressBook)
{
    if (fDebug)
    {
        CExtKey58 ek58;
        ek58.SetKeyP(ek);
        LogPrint("hdwallet", "%s %s, nKey %d.\n", __func__, ek58.ToString().c_str(), nKey);
        AssertLockHeld(cs_wallet);
    };

    CKeyID keyId = ek.GetID();

    CHDWalletDB wdb(strWalletFile, "r+");

    CStoredExtKey sek;
    if (wdb.ReadExtKey(keyId, sek))
    {
        sek.nGenerated = nKey;
        if (!wdb.WriteExtKey(keyId, sek))
            return errorN(1, "%s: WriteExtKey failed.", __func__);
    } else
    {
        sek.kp = ek;
        sek.nGenerated = nKey;
        CKeyID idDerived;
        if (0 != ExtKeyImportLoose(&wdb, sek, idDerived, false, false))
            return errorN(1, "%s: ExtKeyImportLoose failed.", __func__);
    };

    if (fAddToAddressBook
        && !mapAddressBook.count(CTxDestination(ek)))
    {
        SetAddressBook(ek, "", "");
    };
    return 0;
};

int CHDWallet::ScanChainFromTime(int64_t nTimeStartScan)
{
    LogPrintf("%s: %d\n", __func__, nTimeStartScan);

    CBlockIndex *pnext, *pindex = chainActive.Genesis();

    if (pindex == NULL)
        return errorN(1, "%s: Genesis Block is not set.", __func__);

    while (pindex && pindex->nTime < nTimeStartScan
        && (pnext = chainActive.Next(pindex)))
        pindex = pnext;

    LogPrintf("%s: Starting from height %d.\n", __func__, pindex->nHeight);

    {
        LOCK2(cs_main, cs_wallet);

        MarkDirty();

        ScanForWalletTransactions(pindex, true);
        ReacceptWalletTransactions();
    } // cs_main, cs_wallet

    return 0;
};

int CHDWallet::ScanChainFromHeight(int nHeight)
{
    LogPrintf("%s: %d\n", __func__, nHeight);

    CBlockIndex *pnext, *pindex = chainActive.Genesis();

    if (pindex == NULL)
        return errorN(1, "%s: Genesis Block is not set.", __func__);

    while (pindex && pindex->nHeight < nHeight
        && (pnext = chainActive.Next(pindex)))
        pindex = pnext;

    LogPrintf("%s: Starting from height %d.\n", __func__, pindex->nHeight);

    {
        LOCK2(cs_main, cs_wallet);

        MarkDirty();

        ScanForWalletTransactions(pindex, true);
        ReacceptWalletTransactions();
    } // cs_main, cs_wallet

    return 0;
};

bool CHDWallet::CreateTransaction(const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
    std::string& strFailReason, const CCoinControl *coinControl, bool sign)
{
    LogPrintf("CHDWallet %s\n", __func__);
    
    if (!fParticlWallet)
        return CWallet::CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet, nChangePosInOut, strFailReason, coinControl, sign);
    
    CAmount nValue = 0;
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto &recipient : vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            strFailReason = _("Transaction amounts must not be negative");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty())
    {
        strFailReason = _("Transaction must have at least one recipient");
        return false;
    }
    
    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.BindWallet(this);
    CMutableTransaction txNew;
    txNew.nVersion = PARTICL_TXN_VERSION;
    txNew.vout.clear();
    txNew.nLockTime = 0;
    
    {
        std::set<std::pair<const CWalletTx*,unsigned int> > setCoins;
        LOCK2(cs_main, cs_wallet);
        {
            std::vector<COutput> vAvailableCoins;
            AvailableCoins(vAvailableCoins, true, coinControl);
            
            nFeeRet = 0;
            // Start with no fee and loop until there is enough fee
            while (true)
            {
                nChangePosInOut = nChangePosRequest;
                txNew.vin.clear();
                txNew.vpout.clear();
                wtxNew.fFromMe = true;
                bool fFirst = true;
                
                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nValueToSelect += nFeeRet;
                double dPriority = 0;
                
                // vpouts to the payees
                for (const auto &recipient : vecSend)
                {
                    if (recipient.vData.size() > 0)
                    {
                        OUTPUT_PTR<CTxOutData> txout = MAKE_OUTPUT<CTxOutData>();
                        
                        txout->vData = recipient.vData;
                        txNew.vpout.push_back(txout);
                        continue;
                    };
                    
                    OUTPUT_PTR<CTxOutStandard> txout = MAKE_OUTPUT<CTxOutStandard>(recipient.nAmount, recipient.scriptPubKey);
                    if (recipient.fSubtractFeeFromAmount)
                    {
                        txout->nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            txout->nValue -= nFeeRet % nSubtractFeeFromAmount;
                        };
                    };
                    
                    if (txout->IsDust(dustRelayFee))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout->nValue < 0)
                                strFailReason = _("The transaction amount is too small to pay the fee");
                            else
                                strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                        } else
                        {
                            strFailReason = _("Transaction amount too small");
                        };
                        return false;
                    };
                    txNew.vpout.push_back(txout);
                };
                
                // Choose coins to use
                CAmount nValueIn = 0;
                setCoins.clear();
                
                if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
                {
                    strFailReason = _("Insufficient funds");
                    return false;
                };
                
                for (const auto &pcoin : setCoins)
                {
                    CAmount nCredit = pcoin.first->tx->vpout[pcoin.second]->GetValue();
                    //The coin age after the next block (depth+1) is used instead of the current,
                    //reflecting an assumption the user would accept a bit more delay for
                    //a chance at a free transaction.
                    //But mempool inputs might still be in the mempool, so their age stays 0
                    int age = pcoin.first->GetDepthInMainChain();
                    assert(age >= 0);
                    if (age != 0)
                        age += 1;
                    dPriority += (double)nCredit * age;
                };
                
                const CAmount nChange = nValueIn - nValueToSelect;
                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-bitcoin-address
                    CScript scriptChange;

                    // coin control: send change to custom address
                    if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                    {
                        scriptChange = GetScriptForDestination(coinControl->destChange);
                    } else
                    {
                        // no coin control: send change to new address from internal chain
                        
                        CPubKey pkChange;
                        if (0 != GetChangeAddress(pkChange))
                        {
                            strFailReason = _("GetChangeAddress failed.");
                            return false;
                        };

                        scriptChange = GetScriptForDestination(pkChange.GetID());
                    };
                    
                    OUTPUT_PTR<CTxOutStandard> newTxOut = MAKE_OUTPUT<CTxOutStandard>(nChange, scriptChange);
                    
                    // We do not move dust-change to fees, because the sender would end up paying more than requested.
                    // This would be against the purpose of the all-inclusive feature.
                    // So instead we raise the change and deduct from the recipient.
                    if (nSubtractFeeFromAmount > 0 && newTxOut->IsDust(dustRelayFee))
                    {
                        CAmount nDust = newTxOut->GetDustThreshold(dustRelayFee) - newTxOut->nValue;
                        newTxOut->nValue += nDust; // raise change until no more dust
                        for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
                        {
                            CTxOutStandard *pOut = (CTxOutStandard*) txNew.vpout[i].get();
                            if (vecSend[i].fSubtractFeeFromAmount)
                            {
                                pOut->nValue -= nDust;
                                if (pOut->IsDust(dustRelayFee))
                                {
                                    strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                                    return false;
                                };
                                break;
                            };
                        };
                    };
                    
                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    if (newTxOut->IsDust(dustRelayFee))
                    {
                        nChangePosInOut = -1;
                        nFeeRet += nChange;
                        
                    } else
                    {
                        if (nChangePosInOut == -1)
                        {
                            // Insert change txn at random position:
                            nChangePosInOut = GetRandInt(txNew.vpout.size()+1);
                            // Don't displace dataoutputs
                            if (nChangePosInOut < (int)txNew.vpout.size()
                                && !txNew.vpout[nChangePosInOut]->IsStandardOutput())
                                nChangePosInOut++;
                        }
                        else if ((unsigned int)nChangePosInOut > txNew.vpout.size())
                        {
                            strFailReason = _("Change index out of range");
                            return false;
                        }
                        
                        std::vector<CTxOutBaseRef>::iterator position = txNew.vpout.begin()+nChangePosInOut;
                        txNew.vpout.insert(position, newTxOut);
                    };
                }
                
                // Fill vin
                //
                // Note how the sequence number is set to non-maxint so that
                // the nLockTime set above actually works.
                //
                // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
                // we use the highest possible value in that range (maxint-2)
                // to avoid conflicting with other possible uses of nSequence,
                // and in the spirit of "smallest possible change from prior
                // behavior."
                for (const auto& coin : setCoins)
                    txNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second,CScript(),
                                              std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
                
                // Fill in dummy signatures for fee calculation.
                int nIn = 0;
                for (const auto& coin : setCoins)
                {
                    const CScript& scriptPubKey = coin.first->tx->vpout[coin.second]->GetStandardOutput()->scriptPubKey;
                    SignatureData sigdata;

                    if (!ProduceSignature(DummySignatureCreatorParticl(this), scriptPubKey, sigdata))
                    {
                        strFailReason = _("Signing transaction failed");
                        return false;
                    } else {
                        UpdateTransaction(txNew, nIn, sigdata);
                    }

                    nIn++;
                }
                
                unsigned int nBytes = GetVirtualTransactionSize(txNew);

                CTransaction txNewConst(txNew);
                dPriority = txNewConst.ComputePriority(dPriority, nBytes);
                
                // Remove scriptSigs to eliminate the fee calculation dummy signatures
                for (auto& vin : txNew.vin) {
                    vin.scriptSig = CScript();
                    vin.scriptWitness.SetNull();
                }
                
                // Allow to override the default confirmation target over the CoinControl instance
                int currentConfirmationTarget = nTxConfirmTarget;
                if (coinControl && coinControl->nConfirmTarget > 0)
                    currentConfirmationTarget = coinControl->nConfirmTarget;

                // Can we complete this as a free transaction?
                if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
                {
                    // Not enough fee: enough priority?
                    double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
                    // Require at least hard-coded AllowFree.
                    if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
                        break;
                }
                
                CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
                if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
                    nFeeNeeded = coinControl->nMinimumTotalFee;
                }
                if (coinControl && coinControl->fOverrideFeeRate)
                    nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

                // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
                // because we must be at the maximum allowed fee.
                if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
                {
                    strFailReason = _("Transaction too large for fee policy");
                    return false;
                }
                
                if (nFeeRet >= nFeeNeeded) {
                    // Reduce fee to only the needed amount if we have change
                    // output to increase.  This prevents potential overpayment
                    // in fees if the coins selected to meet nFeeNeeded result
                    // in a transaction that requires less fee than the prior
                    // iteration.
                    // TODO: The case where nSubtractFeeFromAmount > 0 remains
                    // to be addressed because it requires returning the fee to
                    // the payees and not the change output.
                    // TODO: The case where there is no change output remains
                    // to be addressed so we avoid creating too small an output.
                    if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                        CAmount extraFeePaid = nFeeRet - nFeeNeeded;
                        CTxOutBaseRef c = txNew.vpout[nChangePosInOut];
                        c->SetValue(c->GetValue() + extraFeePaid);
                        nFeeRet -= extraFeePaid;
                    }
                    break; // Done, enough fee included.
                };
                
                // Try to reduce change to include necessary fee
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                    
                    CTxOutBaseRef c = txNew.vpout[nChangePosInOut];
                    // Only reduce change if remaining amount is still a large enough output.
                    if (c->GetValue() >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
                        c->SetValue(c->GetValue() - additionalFeeNeeded);
                        nFeeRet += additionalFeeNeeded;
                        break; // Done, able to increase fee from change
                    }
                }

                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                continue;
            } // while (true)
        }
        
        if (sign)
        {
            CTransaction txNewConst(txNew);
            int nIn = 0;
            for (const auto& coin : setCoins)
            {
                const CTxOutStandard *prevOut = coin.first->tx->vpout[coin.second]->GetStandardOutput();
                const CScript& scriptPubKey = prevOut->scriptPubKey;
                
                std::vector<uint8_t> vchAmount;
                prevOut->PutValue(vchAmount);
                SignatureData sigdata;
                
                if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, vchAmount, SIGHASH_ALL), scriptPubKey, sigdata))
                {
                    strFailReason = _("Signing transaction failed");
                    return false;
                } else {
                    UpdateTransaction(txNew, nIn, sigdata);
                }

                nIn++;
            }
        }
        
        // Embed the constructed transaction data in wtxNew.
        wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

        // Limit size
        if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
        {
            strFailReason = _("Transaction too large");
            return false;
        }
        
    } // cs_main, cs_wallet
    
    
    if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        LockPoints lp;
        CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
        CTxMemPool::setEntries setAncestors;
        size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT)*1000;
        size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT)*1000;
        std::string errString;
        if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
            strFailReason = _("Transaction has too long of a mempool chain");
            return false;
        }
    }
    
    assert(txNew.vout.empty());
    return true;
};

/**
 * Call after CreateTransaction unless you want to abort
 */
bool CHDWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, CConnman* connman, CValidationState& state)
{
    
    {
        LOCK2(cs_main, cs_wallet);
        
        mapValue_t mapNarr;
        FindStealthTransactions(wtxNew, mapNarr);
        
        if (!mapNarr.empty())
        {
            for (auto &item : mapNarr)
                wtxNew.mapValue[item.first] = item.second;
        };
        
        LogPrintf("CommitTransaction:\n%s", wtxNew.tx->ToString());
        {
            // Take key pair from key pool so it won't be used again
            reservekey.KeepKey();

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            
            AddToWallet(wtxNew);

            // Notify that old coins are spent
            BOOST_FOREACH(const CTxIn& txin, wtxNew.tx->vin)
            {
                const uint256 &txhash = txin.prevout.hash;
                std::map<uint256, CWalletTx>::iterator it = mapWallet.find(txhash);
                if (it != mapWallet.end())
                    it->second.BindWallet(this);
                
                NotifyTransactionChanged(this, txhash, CT_UPDATED);
            }
        }
        
        // Track how many getdata requests our transaction gets
        mapRequestCount[wtxNew.GetHash()] = 0;

        if (fBroadcastTransactions)
        {
            CValidationState state;
            // Broadcast
            if (!wtxNew.AcceptToMemoryPool(maxTxFee, state)) {
                LogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", state.GetRejectReason());
                // TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.
            } else {
                wtxNew.RelayWalletTransaction(connman);
            }
        }
    }
    return true;
}


bool CHDWallet::CommitTransaction(CWalletTx &wtxNew, CTransactionRecord &rtx,
    CReserveKey &reservekey, CConnman *connman, CValidationState &state)
{
    
    {
        LOCK2(cs_main, cs_wallet);
        
        AddToRecord(rtx, *wtxNew.tx, NULL, -1);
        
        // Track how many getdata requests our transaction gets
        mapRequestCount[wtxNew.GetHash()] = 0;

        if (fBroadcastTransactions)
        {
            CValidationState state;
            // Broadcast
            if (!wtxNew.AcceptToMemoryPool(maxTxFee, state))
            {
                LogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", state.GetRejectReason());
                // TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.
                
                
            } else
            {
                wtxNew.BindWallet(this);
                wtxNew.RelayWalletTransaction(connman);
            }
        }
    }
    return true;
};

int CHDWallet::LoadStealthAddresses()
{
    if (fDebug)
        LogPrint("hdwallet", "%s\n", __func__);
    
    LOCK(cs_wallet);

    CHDWalletDB wdb(strWalletFile);

    Dbc *pcursor;
    if (!(pcursor = wdb.GetCursor()))
        return errorN(1, "%s: cannot create DB cursor", __func__);

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    CBitcoinAddress addr;
    CStealthAddress sx;
    std::string strType;

    unsigned int fFlags = DB_SET_RANGE;
    ssKey << std::string("sxad");
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;

        ssKey >> strType;
        if (strType != "sxad")
            break;
        
        ssValue >> sx;
        if (fDebug)
            LogPrint("hdwallet", "Loading stealth address %s\n", sx.Encoded());
        
        stealthAddresses.insert(sx);
    };
    pcursor->close();
    
    if (fDebug)
        LogPrint("hdwallet", "Loaded %u stealth address.\n", stealthAddresses.size());
    
    return 0;
};

bool CHDWallet::UpdateStealthAddressIndex(const CKeyID &idK, const CStealthAddressIndexed &sxi, uint32_t &id)
{
    LOCK(cs_wallet);
    
    uint160 hash = Hash160(sxi.addrRaw.begin(), sxi.addrRaw.end());
    
    uint32_t lastId = 0xFFFFFFFF;
    bool fInsertLast = false;
    
    CHDWalletDB wdb(strWalletFile, "r+");
    
    if (wdb.ReadStealthAddressIndexReverse(hash, id))
    {
        
        if (!wdb.WriteStealthAddressLink(idK, id))
            return error("%s: WriteStealthAddressLink failed.\n", __func__);
        return true;
    };
    
    if (fDebug)
        LogPrint("hdwallet", "%s: Indexing new stealth key.\n", __func__);
    
    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    ssKey << std::make_pair(std::string("ins"), lastId); // set to last possible key
    
    Dbc *pcursor = wdb.GetCursor();
    if (!pcursor)
        return error("%s: Could not get wallet database cursor\n", __func__);
    
    id = 1;
    if (wdb.ReadPrevKeyAtCursor(pcursor, ssKey, fInsertLast) == 0)
    {
        std::string strType;
        ssKey >> strType;
        if (strType == "ins")
        {
            uint32_t prevId;
            ssKey >> prevId;
            id = prevId+1;
        };
    } else
    {
        // ReadPrevKeyAtCursor is not expected to fail if fInsertLast is false
        if (!fInsertLast)
        {
            pcursor->close();
            return error("%s: ReadPrevKeyAtCursor failed.\n", __func__);
        };
    };
    
    pcursor->close();
    
    if (id == lastId)
        return error("%s: Wallet stealth index is full!\n", __func__); // expect multiple wallets per node before anyone hits this
    
    if (fDebug)
        LogPrint("hdwallet", "%s: New index %u.\n", __func__, id);
    
    if (fInsertLast)
    {
        CStealthAddressIndexed dummy;
        if (!wdb.WriteStealthAddressIndex(lastId, dummy))
            return error("%s: WriteStealthAddressIndex failed.\n", __func__);
    };
    
    if (!wdb.WriteStealthAddressIndex(id, sxi)
        || !wdb.WriteStealthAddressIndexReverse(hash, id)
        || !wdb.WriteStealthAddressLink(idK, id))
        return error("%s: Write failed.\n", __func__);
    
    return true;
};

bool CHDWallet::GetStealthLinked(const CKeyID &idK, CStealthAddress &sx)
{
    LOCK(cs_wallet);
    
    CHDWalletDB wdb(strWalletFile);
    
    uint32_t sxId;
    if (!wdb.ReadStealthAddressLink(idK, sxId))
        return false;
    
    CStealthAddressIndexed sxi;
    if (!wdb.ReadStealthAddressIndex(sxId, sxi))
        return false;
    
    if (sxi.addrRaw.size() < MIN_STEALTH_RAW_SIZE)
        return error("%s: Incorrect size for stealthId: %u", __func__, sxId);
    
    if (0 != sx.FromRaw(&sxi.addrRaw[0], sxi.addrRaw.size()))
        return error("%s: FromRaw failed for stealthId: %u", __func__, sxId);
    
    return true;
};

bool CHDWallet::ProcessLockedStealthOutputs()
{
    if (fDebug)
    {
        AssertLockHeld(cs_wallet);
        LogPrint("hdwallet", "%s\n", __func__);
    };
    
    CHDWalletDB wdb(strWalletFile);
    
    if (!wdb.TxnBegin())
        return error("%s: TxnBegin failed.", __func__);

    Dbc *pcursor;
    if (!(pcursor = wdb.GetTxnCursor()))
        return error("%s: Cannot create DB cursor.", __func__);

    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    
    CPubKey pk;
    
    std::string strType;
    CKeyID idk;
    CStealthKeyMetadata sxKeyMeta;
    
    size_t nProcessed = 0; // incl any failed attempts
    size_t nExpanded = 0;
    unsigned int fFlags = DB_SET_RANGE;
    ssKey << std::string("sxkm");
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;
        ssKey >> strType;
        if (strType != "sxkm")
            break;
        
        nProcessed++;
        
        ssKey >> idk;
        ssValue >> sxKeyMeta;
        
        if (!GetPubKey(idk, pk))
        {
            LogPrintf("%s Error: GetPubKey failed %s.\n", __func__, CBitcoinAddress(idk).ToString());
            continue;
        };
        
        CStealthAddress sxFind;
        sxFind.SetScanPubKey(sxKeyMeta.pkScan);

        std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxFind);
        if (si == stealthAddresses.end())
        {
            LogPrintf("%s Error: No stealth key found to add secret for %s.\n", __func__, CBitcoinAddress(idk).ToString());
            continue;
        };
        
        if (fDebug)
            LogPrint("hdwallet", "Expanding secret for %s\n", CBitcoinAddress(idk).ToString());
        
        CKey sSpendR, sSpend;
        
        if (!GetKey(si->spend_secret_id, sSpend))
        {
            LogPrintf("%s Error: Stealth address has no spend_secret_id key for %s\n", __func__, CBitcoinAddress(idk).ToString());
            continue;
        };
        
        if (si->scan_secret.size() != EC_SECRET_SIZE)
        {
            LogPrintf("%s Error: Stealth address has no scan_secret key for %s\n", __func__, CBitcoinAddress(idk).ToString());
            continue;
        };
        
        if (sxKeyMeta.pkEphem.size() != EC_COMPRESSED_SIZE)
        {
            LogPrintf("%s Error: Incorrect Ephemeral point size (%d) for %s\n", __func__, sxKeyMeta.pkEphem.size(), CBitcoinAddress(idk).ToString());
            continue;
        };
        
        ec_point pkEphem;;
        pkEphem.resize(EC_COMPRESSED_SIZE);
        memcpy(&pkEphem[0], sxKeyMeta.pkEphem.begin(), sxKeyMeta.pkEphem.size());
        
        if (StealthSecretSpend(si->scan_secret, pkEphem, sSpend, sSpendR) != 0)
        {
            LogPrintf("%s Error: StealthSecretSpend() failed for %s\n", __func__, CBitcoinAddress(idk).ToString());
            continue;
        };
        
        if (!sSpendR.IsValid())
        {
            LogPrintf("%s Error: Reconstructed key is invalid for %s\n", __func__, CBitcoinAddress(idk).ToString());
            continue;
        };
        
        CPubKey cpkT = sSpendR.GetPubKey();
        if (cpkT != pk)
        {
            LogPrintf("%s: Error: Generated secret does not match.\n", __func__);
            if (fDebug)
            {
                LogPrintf("cpkT   %s\n", HexStr(cpkT).c_str());
                LogPrintf("pubKey %s\n", HexStr(pk).c_str());
            };
            continue;
        };
        
        if (fDebug)
            LogPrint("hdwallet", "%s: Adding secret to key %s.\n", __func__, CBitcoinAddress(idk).ToString());
        
        
        pwalletdbEncryption = &wdb; // HACK: use wdb in CWallet::AddCryptedKey
        if (!AddKeyPubKey(sSpendR, pk))
        {
            LogPrintf("%s: Error: AddKeyPubKey failed.\n", __func__);
            pwalletdbEncryption = NULL;
            continue;
        };
        pwalletdbEncryption = NULL;
        
        nExpanded++;
        
        int rv = pcursor->del(0);
        if (rv != 0)
            LogPrintf("%s: Error: EraseStealthKeyMeta failed for %s, %d\n", __func__, CBitcoinAddress(idk).ToString(), rv);
        
    };
    
    pcursor->close();
    
    wdb.TxnCommit();
    
    if (fDebug)
        LogPrint("hdwallet", "%s: Expanded %u/%u key%s.\n", __func__, nExpanded, nProcessed, nProcessed == 1 ? "" : "s");
    
    return true;
};

inline bool MatchPrefix(uint32_t nAddrBits, uint32_t addrPrefix, uint32_t outputPrefix, bool fHavePrefix)
{
    if (nAddrBits < 1) // addresses without prefixes scan all incoming stealth outputs
        return true;
    if (!fHavePrefix) // don't check when address has a prefix and no prefix on output
        return false;
    
    uint32_t mask = SetStealthMask(nAddrBits);
    
    return (addrPrefix & mask) == (outputPrefix & mask);
};

bool CHDWallet::ProcessStealthOutput(const CTxDestination &address,
    std::vector<uint8_t> &vchEphemPK, uint32_t prefix, bool fHavePrefix, CKey &sShared)
{
    ec_point pkExtracted;
    CKey sSpend;
    
    CKeyID ckidMatch = boost::get<CKeyID>(address);
    if (HaveKey(ckidMatch)) // no point checking if already have key
        return false;
    
    std::set<CStealthAddress>::iterator it;
    for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
    {
        if (!MatchPrefix(it->prefix.number_bits, it->prefix.bitfield, prefix, fHavePrefix))
            continue;
        
         if (!it->scan_secret.IsValid())
            continue; // stealth address is not owned
        
        if (StealthSecret(it->scan_secret, vchEphemPK, it->spend_pubkey, sShared, pkExtracted) != 0)
        {
            LogPrintf("%s: StealthSecret failed.\n", __func__);
            continue;
        };
        
        CPubKey pkE(pkExtracted);
        if (!pkE.IsValid())
            continue;
        
        CKeyID idExtracted = pkE.GetID();
        if (ckidMatch != idExtracted)
            continue;

        if (fDebug)
            LogPrint("hdwallet", "Found stealth txn to address %s\n", it->Encoded());
        
        CStealthAddressIndexed sxi;
        it->ToRaw(sxi.addrRaw);
        uint32_t sxId;
        if (!UpdateStealthAddressIndex(ckidMatch, sxi, sxId))
            return error("%s: UpdateStealthAddressIndex failed.\n", __func__);
        
        if (IsLocked())
        {
            if (fDebug)
                LogPrint("hdwallet", "Wallet locked, adding key without secret.\n");
            
            // -- add key without secret
            std::vector<uint8_t> vchEmpty;
            AddCryptedKey(pkE, vchEmpty);
            CBitcoinAddress coinAddress(idExtracted);
            
            CPubKey cpkEphem(vchEphemPK);
            CPubKey cpkScan(it->scan_pubkey);
            CStealthKeyMetadata lockedSkMeta(cpkEphem, cpkScan);

            if (!CHDWalletDB(strWalletFile).WriteStealthKeyMeta(idExtracted, lockedSkMeta))
                LogPrintf("WriteStealthKeyMeta failed for %s.\n", coinAddress.ToString().c_str());

            //mapStealthKeyMeta[idExtracted] = lockedSkMeta;
            nFoundStealth++;
            return true;
        };
        
        if (!GetKey(it->spend_secret_id, sSpend))
        {
            // silently fail?
            if (fDebug)
                LogPrint("hdwallet", "GetKey() stealth spend failed.\n");
            continue;
        };
        
        CKey sSpendR;
        if (StealthSharedToSecretSpend(sShared, sSpend, sSpendR) != 0)
        {
            LogPrintf("%s: StealthSharedToSecretSpend() failed.\n", __func__);
            continue;
        };
        
        CPubKey pkT = sSpendR.GetPubKey();
        if (!pkT.IsValid())
        {
            LogPrintf("%s: pkT is invalid.\n", __func__);
            continue;
        };
        
        CKeyID keyID = pkT.GetID();
        if (keyID != ckidMatch)
        {
            LogPrintf("%s: Spend key mismatch!\n", __func__);
            continue;
        };
        
        if (fDebug)
            LogPrint("hdwallet", "%s: Adding key %s.\n", __func__, CBitcoinAddress(keyID).ToString().c_str());
        
        if (!AddKeyPubKey(sSpendR, pkT))
        {
            LogPrintf("%s: AddKeyPubKey failed.\n", __func__);
            continue;
        };
        
        nFoundStealth++;
        return true;
    };
    
    // - ext account stealth keys
    ExtKeyAccountMap::const_iterator mi;
    for (mi = mapExtAccounts.begin(); mi != mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *ea = mi->second;

        for (AccStealthKeyMap::iterator it = ea->mapStealthKeys.begin(); it != ea->mapStealthKeys.end(); ++it)
        {
            const CEKAStealthKey &aks = it->second;
            
            if (!MatchPrefix(aks.nPrefixBits, aks.nPrefix, prefix, fHavePrefix))
                continue;
            
            if (!aks.skScan.IsValid())
                continue;

            if (StealthSecret(aks.skScan, vchEphemPK, aks.pkSpend, sShared, pkExtracted) != 0)
            {
                LogPrintf("%s: StealthSecret failed.\n", __func__);
                continue;
            };

            CPubKey pkE(pkExtracted);
            if (!pkE.IsValid())
                continue;
            
            CKeyID idExtracted = pkE.GetID();
            if (ckidMatch != idExtracted)
                continue;
            
            if (fDebug)
            {
                LogPrint("hdwallet", "Found stealth txn to address %s\n", aks.ToStealthAddress().c_str());

                // - check key if not locked
                if (!IsLocked())
                {
                    CKey kTest;
                    if (0 != ea->ExpandStealthChildKey(&aks, sShared, kTest))
                    {
                        LogPrintf("%s: Error: ExpandStealthChildKey failed! %s.\n", __func__, aks.ToStealthAddress().c_str());
                        continue;
                    };

                    CKeyID kTestId = kTest.GetPubKey().GetID();
                    if (kTestId != ckidMatch)
                    {
                        LogPrintf("%s: Error: Spend key mismatch!\n", __func__);
                        continue;
                    };
                    CBitcoinAddress coinAddress(kTestId);
                    LogPrintf("Debug: ExpandStealthChildKey matches! %s, %s.\n", aks.ToStealthAddress().c_str(), coinAddress.ToString().c_str());
                };
            };

            // - don't need to extract key now, wallet may be locked
            CKeyID idStealthKey = aks.GetID();
            CEKASCKey kNew(idStealthKey, sShared);
            if (0 != ExtKeySaveKey(ea, ckidMatch, kNew))
            {
                LogPrintf("%s: Error: ExtKeySaveKey failed!\n", __func__);
                continue;
            };
            
            CStealthAddressIndexed sxi;
            aks.ToRaw(sxi.addrRaw);
            uint32_t sxId;
            if (!UpdateStealthAddressIndex(ckidMatch, sxi, sxId))
                return error("%s: UpdateStealthAddressIndex failed.\n", __func__);
            
            return true;
        };
    };
    
    return false;
};

int CHDWallet::CheckForStealthAndNarration(const CTxOutBase *pb, const CTxOutData *pdata, std::string &sNarr)
{
    // returns: -1 error, 0 nothing found, 1 narration, 2 stealth
    
    CKey sShared;
    std::vector<uint8_t> vchEphemPK, vchENarr;
    const std::vector<uint8_t> &vData = pdata->vData;
    
    if (vData.size() < 1)
        return -1;
    
    if (vData[0] == DO_NARR_PLAIN)
    {
        if (vData.size() < 2)
            return -1; // error
        
        sNarr = std::string(vData.begin()+1, vData.end());
        return 1;
    };
    
    if (vData[0] == DO_STEALTH)
    {
        if (vData.size() < 34
            || !pb->IsStandardOutput())
            return -1; // error
        
        vchEphemPK.resize(33);
        memcpy(&vchEphemPK[0], &vData[1], 33);
        
        uint32_t prefix = 0;
        bool fHavePrefix = false;
        
        if (vData.size() >= 34 + 5
            && vData[34] == DO_STEALTH_PREFIX)
        {
            fHavePrefix = true;
            memcpy(&prefix, &vData[35], 4);
        };
        
        const CTxOutStandard *so = (CTxOutStandard*)pb;
        CTxDestination address;
        if (!ExtractDestination(so->scriptPubKey, address)
            || address.type() != typeid(CKeyID))
        {
            LogPrintf("%s: ExtractDestination failed.\n",  __func__);
            return -1;
        };
        
        if (!ProcessStealthOutput(address, vchEphemPK, prefix, fHavePrefix, sShared))
        {
            // TODO: check all other outputs?
            return 0;
        };
        
        int nNarrOffset = -1;
        if (vData.size() > 40 && vData[39] == DO_NARR_CRYPT)
            nNarrOffset = 40;
        else
        if (vData.size() > 35 && vData[34] == DO_NARR_CRYPT)
            nNarrOffset = 35;
        
        if (nNarrOffset > -1)
        {
            size_t lenNarr = vData.size() - nNarrOffset;
            if (lenNarr < 1 || lenNarr > 32) // min block size 8?
            {
                LogPrintf("%s: Invalid narration data length: %d\n", __func__, lenNarr);
                return 2; // still found
            };
            vchENarr.resize(lenNarr);
            memcpy(&vchENarr[0], &vData[nNarrOffset], lenNarr);
            
            SecMsgCrypter crypter;
            crypter.SetKey(sShared.begin(), &vchEphemPK[0]);
            
            std::vector<uint8_t> vchNarr;
            if (!crypter.Decrypt(&vchENarr[0], vchENarr.size(), vchNarr))
            {
                LogPrintf("%s: Decrypt narration failed.\n", __func__);
                return 2; // still found
            };
            sNarr = std::string(vchNarr.begin(), vchNarr.end());
        };
        
        return 2;
    };
    
    LogPrintf("%s: Unknown data output type %d.\n",  __func__, vData[0]);
    return -1;
};

bool CHDWallet::FindStealthTransactions(const CTransaction& tx, mapValue_t& mapNarr)
{
    if (fDebug)
        LogPrint("hdwallet", "%s: tx: %s.\n", __func__, tx.GetHash().GetHex());

    mapNarr.clear();

    LOCK(cs_wallet);
    
    // A data output always applies to the preceding output
    int32_t nOutputId = -1;
    for (const auto &txout : tx.vpout)
    {
        nOutputId++;
        if (txout->nVersion != OUTPUT_DATA)
            continue;
        
        CTxOutData *txd = (CTxOutData*) txout.get();
        
        if (nOutputId < 1)
        {
            LogPrint("hdwallet", "%s: Warning, ignoring data output in pos 0, tx: %s.\n", __func__, tx.GetHash().GetHex());
            continue;
        };
        
        std::string sNarr;
        if (CheckForStealthAndNarration(tx.vpout[nOutputId-1].get(), txd, sNarr) < 0)
            LogPrintf("%s: txn %s, malformed data output %d.\n",  __func__, tx.GetHash().ToString(), nOutputId);
        
        if (sNarr.length() > 0)
        {
            std::string sKey = strprintf("n%d", nOutputId-1);
            mapNarr[sKey] = sNarr;
        };
        
        //const CTxOutStandard *so = (CTxOutStandard*)tx.vpout[nOutputId-1].get();
        
    };
    
    return true;
};

bool CHDWallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlockIndex* pIndex, int posInBlock, bool fUpdate)
{
    //if (!fParticlWallet)
    //    return CWallet::AddToWalletIfInvolvingMe(tx, pIndex, posInBlock, fUpdate);
    
    {
        AssertLockHeld(cs_wallet);
        if (posInBlock != -1) {
            BOOST_FOREACH(const CTxIn& txin, tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        LogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), pIndex->GetBlockHash().ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                        MarkConflicted(pIndex->GetBlockHash(), range.first->second);
                    }
                    range.first++;
                }
            }
        }
        
        
        
        mapValue_t mapNarr;
        bool fIsMine = false;
        if (!tx.IsCoinBase() && !tx.IsCoinStake()) // Skip transactions that can't be stealth.
        {
            FindStealthTransactions(tx, mapNarr);
            /*
            if (tx.nVersion == ANON_TXN_VERSION)
            {
                LOCK(cs_main); // cs_wallet is already locked
                CWalletDB walletdb(strWalletFile, "cr+");
                CTxDB txdb("cr+");

                uint256 blockHash = (pblock ? (nNodeMode == NT_FULL ? ((CBlock*)pblock)->GetHash() : *(uint256*)pblock) : 0);

                walletdb.TxnBegin();
                txdb.TxnBegin();
                std::vector<WalletTxMap::iterator> vUpdatedTxns;
                if (!ProcessAnonTransaction(&walletdb, &txdb, tx, blockHash, fIsMine, mapNarr, vUpdatedTxns))
                {
                    LogPrintf("ProcessAnonTransaction failed %s\n", hash.ToString().c_str());
                    walletdb.TxnAbort();
                    txdb.TxnAbort();
                    return false;
                } else
                {
                    walletdb.TxnCommit();
                    txdb.TxnCommit();
                    for (std::vector<WalletTxMap::iterator>::iterator it = vUpdatedTxns.begin();
                        it != vUpdatedTxns.end(); ++it)
                        NotifyTransactionChanged(this, (*it)->first, CT_UPDATED);
                };
            };
            */
        }
        
        bool fIsFromMe = false;
        size_t nCT = 0, nRingCT = 0;
        
        std::map<uint256, CWalletTx>::const_iterator miw;
        std::map<uint256, CTransactionRecord>::const_iterator mir;
        for (const auto &txin : tx.vin)
        {
            miw = mapWallet.find(txin.prevout.hash);
            if (miw != mapWallet.end())
            {
                const CWalletTx &prev = miw->second;
                if (txin.prevout.n < prev.tx->vpout.size()
                    && IsMine(prev.tx->vpout[txin.prevout.n].get()) & ISMINE_ALL)
                {
                    fIsFromMe = true;
                    break; // only need one match
                };
                
                continue; // a txn in mapWallet shouldn't be in mapRecords too
            };
            
            mir = mapRecords.find(txin.prevout.hash);
            if (mir != mapRecords.end())
            {
                const COutputRecord *r = mir->second.GetOutput(txin.prevout.n);
                if (r && r->nFlags & ORF_OWNED)
                {
                    fIsFromMe = true;
                    break; // only need one match
                }; 
            };
        };
        
        for (const auto &txout : tx.vpout)
        {
            if (txout->IsType(OUTPUT_CT))
            {
                nCT++;
                
                // Find stealth
                const CTxOutCT *ctout = (CTxOutCT*) txout.get();
                
                uint32_t prefix = 0;
                bool fHavePrefix = false;
                CTxDestination address;
                if (!ExtractDestination(ctout->scriptPubKey, address)
                    || address.type() != typeid(CKeyID))
                {
                    LogPrintf("%s: ExtractDestination failed.\n", __func__);
                    continue;
                };
                
                if (ctout->vData.size() != 33) // TODO: prefix
                {
                    LogPrint("hdwallet", "bad blind output data size.\n");
                    continue;
                };
                
                CKey sShared;
                std::vector<uint8_t> vchEphemPK;
                vchEphemPK.resize(33);
                memcpy(&vchEphemPK[0], &ctout->vData[0], 33);
                
                if (ProcessStealthOutput(address, vchEphemPK, prefix, fHavePrefix, sShared))
                    fIsMine = true;
                else
                    continue;
                
            } else
            if (txout->IsType(OUTPUT_RINGCT))
            {
                nRingCT++;
                
                const CTxOutRingCT *rctout = (CTxOutRingCT*) txout.get();
                
                
                
            } else
            {
                if (IsMine(txout.get()))
                    fIsMine = true;
            };
        };
        
        if (nCT > 0 || nRingCT > 0)
        {
            bool fExisted = mapRecords.count(tx.GetHash()) != 0;
            if (fExisted && !fUpdate) return false;
            
            if (fExisted || fIsMine || fIsFromMe)
            {
                CTransactionRecord rtx;
                bool rv = AddToRecord(rtx, tx, pIndex, posInBlock, false);
                WakeThreadStakeMiner(); // wallet balance may have changed
                return rv;
            }
            
            return false;
        };
        
        bool fExisted = mapWallet.count(tx.GetHash()) != 0;
        if (fExisted && !fUpdate) return false;
        if (fExisted || fIsMine || fIsFromMe)
        {
            // A coinstake txn not linked to a block is being orphaned
            if (fExisted && tx.IsCoinStake() && posInBlock < 0)
            {
                uint256 hashTx = tx.GetHash();
                LogPrint("pos", "Orphaning stake txn: %s\n", hashTx.ToString());
                
                // if block is later reconnected tx will be unabandoned by AddToWallet
                if (!AbandonTransaction(hashTx))
                    LogPrintf("ERROR: %s - Orphaning stake, AbandonTransaction failed for %s\n", __func__, hashTx.ToString());
            };
            
            CWalletTx wtx(this, MakeTransactionRef(tx));
            
            if (!mapNarr.empty())
                wtx.mapValue.insert(mapNarr.begin(), mapNarr.end());
            
            // Get merkle branch if transaction was found in a block
            //if (posInBlock != -1)
            if (posInBlock > -1)
                wtx.SetMerkleBranch(pIndex, posInBlock);
            bool rv = AddToWallet(wtx, false);
            WakeThreadStakeMiner(); // wallet balance may have changed
            return rv;
        }
    }
    
    return false;
};

const CWalletTx *CHDWallet::GetWalletTx(const uint256 &hash) const
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::const_iterator it = mapTempWallet.find(hash);
    if (it != mapTempWallet.end())
        return &(it->second);
    
    return CWallet::GetWalletTx(hash);
}

CWalletTx *CHDWallet::GetWalletTx(const uint256& hash)
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::iterator itr = mapTempWallet.find(hash);
    if (itr != mapTempWallet.end())
        return &(itr->second);
    
    std::map<uint256, CWalletTx>::iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return NULL;
    return &(it->second);
};

int CHDWallet::InsertTempTxn(const uint256 &txid, const uint256 &blockHash, int nIndex) const
{
    LOCK(cs_wallet);
    
    CWalletTx wtx;
    CStoredTransaction stx;
    
    /*
    uint256 hashBlock;
    CTransactionRef txRef;
    if (!GetTransaction(txid, txRef, Params().GetConsensus(), hashBlock, false))
        return errorN(1, "%s: GetTransaction failed, %s.\n", __func__, txid.ToString());
    */
    if (!CHDWalletDB(strWalletFile).ReadStoredTx(txid, stx))
        return errorN(1, "%s: ReadStoredTx failed for %s.\n", __func__, txid.ToString().c_str());
    
    wtx.BindWallet(pwalletMain);
    wtx.tx = stx.tx;
    wtx.hashBlock = blockHash;
    wtx.nIndex = nIndex;
    
    mapTempWallet[txid] = wtx;
    
    return 0;
};

int CHDWallet::OwnStandardOut(const CTxOutStandard *pout, const CTxOutData *pdata, COutputRecord &rout)
{
    
    if (pdata)
    {
        std::string sNarr;
        if (CheckForStealthAndNarration((CTxOutBase*)pout, pdata, sNarr) < 0)
            LogPrintf("%s: malformed data output %d.\n",  __func__, rout.n);
        
        if (sNarr.length() > 0)
            rout.sNarration = sNarr;
    };
    
    CKeyID idk;
    CEKAKey ak;
    CExtKeyAccount *pa = NULL;
    bool isInvalid;
    if (IsMine(pout->scriptPubKey, idk, ak, pa, isInvalid) != ISMINE_SPENDABLE)
    {
        return 0;
    };
    
    rout.nType = OUTPUT_STANDARD;
    rout.nFlags |= ORF_OWNED;
    rout.nValue = pout->nValue;
    rout.scriptPubKey = pout->scriptPubKey;
    
    return 1;
};

int CHDWallet::OwnBlindOut(const CTxOutCT *pout, const CStoredExtKey *pc, uint32_t &nLastChild,
    COutputRecord &rout, CStoredTransaction &stx)
{
    
    bool fDecoded = false;
    
    if (pc && !IsLocked()) // check if output is from this wallet
    {
        CKey keyEphem;
        uint32_t nChildOut;
        if (0 == pc->DeriveKey(keyEphem, nLastChild, nChildOut, true))
        {
            // regenerate nonce
            //uint256 nonce = keyEphem.ECDH(pkto_outs[k]);
            //CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());
        };
    };
    
    CKeyID idk;
    CEKAKey ak;
    CExtKeyAccount *pa = NULL;
    bool isInvalid;
    if (IsMine(pout->scriptPubKey, idk, ak, pa, isInvalid) != ISMINE_SPENDABLE)
    {
        return 0;
    };
    
    CKey key;
    if (!GetKey(idk, key))
        return errorN(0, "%s: GetKey failed.", __func__);
    
    if (pout->vData.size() < 33)
        return errorN(0, "%s: vData.size() < 33.", __func__);
    
    
    CPubKey pkEphem;
    pkEphem.Set(pout->vData.begin(), pout->vData.begin() + 33);
    
    // regenerate nonce
    uint256 nonce = key.ECDH(pkEphem);
    CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());
    
    uint64_t min_value, max_value;
    uint8_t blindOut[32];
    unsigned char msg[4096];
    size_t msg_size;
    uint64_t amountOut;
    if (1 != secp256k1_rangeproof_rewind(secp256k1_ctx_blind,
        blindOut, &amountOut, msg, &msg_size, nonce.begin(),
        &min_value, &max_value, 
        &pout->commitment, pout->vRangeproof.data(), pout->vRangeproof.size(),
        NULL, 0,
        secp256k1_generator_h))
        return errorN(0, "%s: secp256k1_rangeproof_rewind failed.", __func__);
    
    //msg[msg_size] = '\0';
    //if (msg_size)
    if (strlen((const char*)msg))
        rout.sNarration = std::string((const char*)msg);
    
    rout.nType = OUTPUT_CT;
    rout.nFlags |= ORF_OWNED;
    rout.nValue = amountOut;
    rout.scriptPubKey = pout->scriptPubKey;
    
    stx.InsertBlind(rout.n, blindOut);
    
    return 1;
};

int CHDWallet::OwnAnonOut(const CTxOutRingCT *pout, const CStoredExtKey *pc, uint32_t &nLastChild,
    COutputRecord &rout, CStoredTransaction &stx)
{
    
    
    return 0;
};

bool CHDWallet::AddToRecord(CTransactionRecord &rtxIn, const CTransaction &tx,
    const CBlockIndex *pIndex, int posInBlock, bool fFlushOnClose)
{
    LogPrint("hdwallet","%s: %s, %p, %d\n", __func__, tx.GetHash().ToString(), pIndex, posInBlock);
    
    AssertLockHeld(cs_wallet);
    CHDWalletDB wdb(strWalletFile, "r+", fFlushOnClose);
    
    uint256 txhash = tx.GetHash();
    
    //CTransactionRecord newRecord;
    // Inserts only if not already there, returns tx inserted or tx found
    std::pair<std::map<uint256, CTransactionRecord>::iterator, bool> ret = mapRecords.insert(std::make_pair(txhash, rtxIn));
    CTransactionRecord &rtx = ret.first->second;
    
    if (pIndex)
    {
        rtx.blockHash = pIndex->GetBlockHash();
        rtx.nIndex = posInBlock;
    };
    
    bool fInsertedNew = ret.second;
    if (fInsertedNew)
    {
        rtx.nTimeReceived = GetAdjustedTime();
        
        std::map<uint256, CTransactionRecord>::iterator mri = ret.first;
        rtxOrdered.insert(std::make_pair(rtx.nTimeReceived, mri));
        
        for (auto &txin : tx.vin)
        {
            AddToSpends(txin.prevout, txhash);
        };
    };
    
    CExtKeyAccount *seaC = NULL;
    CStoredExtKey *pcC = NULL;
    std::vector<uint8_t> vEphemPath;
    uint32_t nCTStart = 0;
    mapRTxValue_t::iterator mvi = rtx.mapValue.find(RTXVT_EPHEM_PATH);
    if (mvi != rtx.mapValue.end())
    {
        vEphemPath = mvi->second;
        
        if (vEphemPath.size() != 12) // accid / chainchild / start
            return false;
        
        uint32_t accIndex, nChainChild;
        memcpy(&accIndex, &vEphemPath[0], 4);
        memcpy(&nChainChild, &vEphemPath[4], 4);
        memcpy(&nCTStart, &vEphemPath[8], 4);
        
        CKeyID idAccount;
        if (wdb.ReadExtKeyIndex(accIndex, idAccount))
        {
            // TODO: load from db if not found
            ExtKeyAccountMap::iterator mi = mapExtAccounts.find(idAccount);
            if (mi != mapExtAccounts.end())
            {
                seaC = mi->second;
                for (auto pchain : seaC->vExtKeys)
                {
                    if (pchain->kp.nChild == nChainChild)
                        pcC = pchain;
                };
            };
        };
    };
    
    
    
    
    
    CStoredTransaction stx;
    
    for (size_t i = 0; i < tx.vpout.size(); ++i)
    {
        const auto &txout = tx.vpout[i];
        
        COutputRecord rout;
        rout.n = i;
        
        
        COutputRecord *pout = rtx.GetOutput(i);
        
        bool fHave = false;
        if (pout) // have output recorded already, still need to check if owned
        {
            fHave = true;
        } else
        {
            pout = &rout;
        }
        
        
        switch (txout->nVersion)
        {
            case OUTPUT_STANDARD:
                {
                CTxOutData *pdata = NULL;
                if (i < tx.vpout.size()-1)
                {
                    if (tx.vpout[i+1]->nVersion == OUTPUT_DATA)
                        pdata = (CTxOutData*)tx.vpout[i+1].get();
                };
                
                if (OwnStandardOut((CTxOutStandard*)txout.get(), pdata, *pout))
                {
                    if (!fHave)
                        rtx.InsertOutput(*pout);
                }
                //rtx.vout.push_back(rout);
                }
                break;
            case OUTPUT_CT:
                if (OwnBlindOut((CTxOutCT*)txout.get(), pcC, nCTStart, *pout, stx))
                {
                    if (!fHave)
                        rtx.InsertOutput(*pout);
                }
                break;
            case OUTPUT_RINGCT:
                if (OwnAnonOut((CTxOutRingCT*)txout.get(), pcC, nCTStart, *pout, stx))
                {
                    if (!fHave)
                        rtx.InsertOutput(*pout);
                }
                break;
        };
    };
    
    bool fUpdated = false;
    
    if (fInsertedNew || fUpdated)
    {
        stx.tx = MakeTransactionRef(tx);
        if (!wdb.WriteTxRecord(txhash, rtx)
            || !wdb.WriteStoredTx(txhash, stx))
            return false;
    };
    
    return true;
};

void CHDWallet::AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
    vCoins.clear();

    LOCK2(cs_main, cs_wallet);
    for (std::map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const uint256 &wtxid = it->first;
        const CWalletTx *pcoin = &it->second;

        if (!CheckFinalTx(*pcoin))
            continue;

        if (fOnlyConfirmed && !pcoin->IsTrusted())
            continue;

        if (pcoin->GetBlocksToMaturity() > 0)
            continue;

        int nDepth = pcoin->GetDepthInMainChain();
        if (nDepth < 0)
            continue;

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !pcoin->InMempool())
            continue;

        // We should not consider coins from transactions that are replacing
        // other transactions.
        //
        // Example: There is a transaction A which is replaced by bumpfee
        // transaction B. In this case, we want to prevent creation of
        // a transaction B' which spends an output of B.
        //
        // Reason: If transaction A were initially confirmed, transactions B
        // and B' would no longer be valid, so the user would have to create
        // a new transaction C to replace B'. However, in the case of a
        // one-block reorg, transactions B' and C might BOTH be accepted,
        // when the user only wanted one of them. Specifically, there could
        // be a 1-block reorg away from the chain where transactions A and C
        // were accepted to another chain where B, B', and C were all
        // accepted.
        if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
            continue;
        }

        // Similarly, we should not consider coins from transactions that
        // have been replaced. In the example above, we would want to prevent
        // creation of a transaction A' spending an output of A, because if
        // transaction B were initially confirmed, conflicting with A and
        // A', we wouldn't want to the user to create a transaction D
        // intending to replace A', but potentially resulting in a scenario
        // where A, A', and D could all be accepted (instead of just B and
        // D, or just A and A' like the user would want).
        if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
            continue;
        }
        
        for (unsigned int i = 0; i < pcoin->tx->vpout.size(); i++)
        {
            if (!pcoin->tx->vpout[i]->IsStandardOutput())
                continue;
                
            const CTxOutStandard *txout = pcoin->tx->vpout[i]->GetStandardOutput();
            isminetype mine = IsMine(txout);
            
            if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
                !IsLockedCoin((*it).first, i) && (txout->nValue > 0 || fIncludeZeroValue) &&
                (!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i))))
            {
                vCoins.push_back(COutput(pcoin, i, nDepth,
                                         ((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
                                          (coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO),
                                         (mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) != ISMINE_NO));
            };
        };
    };
    
    for (std::map<uint256, CTransactionRecord>::const_iterator it = mapRecords.begin(); it != mapRecords.end(); ++it)
    {
        const uint256 &txid = it->first;
        const CTransactionRecord &rtx = it->second;
        
        if (fOnlyConfirmed && !IsTrusted(txid, rtx.blockHash))
            continue;
        
        // TODO: implement when moving coinbase and coinstake txns to mapRecords
        //if (pcoin->GetBlocksToMaturity() > 0)
        //    continue;

        int nDepth = GetDepthInMainChain(rtx.blockHash);
        if (nDepth < 0)
            continue;
        

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !InMempool(txid))
            continue;
        
        
        if (nDepth == 0 && fOnlyConfirmed
             && (rtx.mapValue.count(RTXVT_REPLACES_TXID)
                || rtx.mapValue.count(RTXVT_REPLACED_BY_TXID)))
            continue;
        
        std::map<uint256, CWalletTx>::const_iterator twi = mapTempWallet.find(txid);
        for (auto &r : rtx.vout)
        {
            if (r.nType != OUTPUT_STANDARD)
                continue;
            
            if (r.nFlags & ORF_OWNED && !(IsSpent(txid, r.n)))
            {
                if (twi == mapTempWallet.end()
                    && (twi = mapTempWallet.find(txid)) == mapTempWallet.end())
                {
                    if (0 != InsertTempTxn(txid, rtx.blockHash, rtx.nIndex)
                        || (twi = mapTempWallet.find(txid)) == mapTempWallet.end())
                    {
                        LogPrintf("ERROR: %s - InsertTempTxn failed %s.", __func__, txid.ToString());
                        return;
                    };
                };
                vCoins.push_back(COutput(&twi->second, r.n, nDepth, true, true));
            };
            
        };
    };
    
}

bool CHDWallet::SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl) const
{
    if (!fParticlWallet)
        return CWallet::SelectCoins(vAvailableCoins, nTargetValue, setCoinsRet, nValueRet, coinControl);
    
    std::vector<COutput> vCoins(vAvailableCoins);
    

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs)
    {
        for (auto &out : vCoins)
        {
            if (!out.fSpendable)
                 continue;
            nValueRet += out.tx->tx->vpout[out.i]->GetValue();
            setCoinsRet.insert(std::make_pair(out.tx, out.i));
        }
        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    std::set<std::pair<const CWalletTx*, uint32_t> > setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<COutPoint> vPresetInputs;
    if (coinControl)
        coinControl->ListSelected(vPresetInputs);
    
    for (auto &outpoint : vPresetInputs)
    {
        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx *pcoin = &it->second;
            // Clearly invalid input, fail
            if (pcoin->tx->vpout.size() <= outpoint.n)
                return false;
            nValueFromPresetInputs += pcoin->tx->vpout[outpoint.n]->GetValue();
            setPresetCoins.insert(std::make_pair(pcoin, outpoint.n));
        } else
            return false; // TODO: Allow non-wallet inputs
    }

    // remove preset inputs from vCoins
    for (std::vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coinControl && coinControl->HasSelected();)
    {
        if (setPresetCoins.count(std::make_pair(it->tx, it->i)))
            it = vCoins.erase(it);
        else
            ++it;
    }

    size_t nMaxChainLength = std::min(GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT), GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT));
    bool fRejectLongChains = GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    bool res = nTargetValue <= nValueFromPresetInputs ||
        CWallet::SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 6, 0, vCoins, setCoinsRet, nValueRet) ||
        CWallet::SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 1, 0, vCoins, setCoinsRet, nValueRet) ||
        (bSpendZeroConfChange && CWallet::SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, 2, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && CWallet::SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::min((size_t)4, nMaxChainLength/3), vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && CWallet::SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength/2, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && CWallet::SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && !fRejectLongChains && CWallet::SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::numeric_limits<uint64_t>::max(), vCoins, setCoinsRet, nValueRet));
    
    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

    return res;
}

void CHDWallet::AvailableBlindedCoins(std::vector<COutputR>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
    
    for (MapRecords_t::const_iterator it = mapRecords.begin(); it != mapRecords.end(); ++it)
    {
        const uint256 &txid = it->first;
        const CTransactionRecord &rtx = it->second;
        
        if (fOnlyConfirmed && !IsTrusted(txid, rtx.blockHash))
            continue;
        
        // TODO: implement when moving coinbase and coinstake txns to mapRecords
        //if (pcoin->GetBlocksToMaturity() > 0)
        //    continue;

        int nDepth = GetDepthInMainChain(rtx.blockHash);
        if (nDepth < 0)
            continue;
        

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !InMempool(txid))
            continue;
        
        
        if (nDepth == 0 && fOnlyConfirmed
             && (rtx.mapValue.count(RTXVT_REPLACES_TXID)
                || rtx.mapValue.count(RTXVT_REPLACED_BY_TXID)))
            continue;
        
        for (auto &r : rtx.vout)
        {
            if (r.nType != OUTPUT_CT)
                continue;
            
            if (r.nFlags & ORF_OWNED && !(IsSpent(txid, r.n)))
            {
                vCoins.push_back(COutputR(txid, it, r.n, nDepth));
            };
        };
    };
    
};

bool CHDWallet::SelectBlindedCoins(const std::vector<COutputR>& vAvailableCoins, const CAmount& nTargetValue, std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl) const
{
    
    std::vector<COutputR> vCoins(vAvailableCoins);
    
    
    CAmount nValueFromPresetInputs = 0;
    // TODO: Add coincontrol
    
    
    size_t nMaxChainLength = std::min(GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT), GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT));
    bool fRejectLongChains = GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    bool res = nTargetValue <= nValueFromPresetInputs ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 6, 0, vCoins, setCoinsRet, nValueRet) ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 1, 0, vCoins, setCoinsRet, nValueRet) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, 2, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::min((size_t)4, nMaxChainLength/3), vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength/2, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && !fRejectLongChains && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::numeric_limits<uint64_t>::max(), vCoins, setCoinsRet, nValueRet));
    
    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    //setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;
    
    std::shuffle(std::begin(setCoinsRet), std::end(setCoinsRet), std::default_random_engine(unsigned(time(NULL))));

    return res;
}

struct CompareValueOnly
{
    bool operator()(const std::pair<CAmount, std::pair<MapRecords_t::const_iterator, unsigned int> >& t1,
                    const std::pair<CAmount, std::pair<MapRecords_t::const_iterator, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

static void ApproximateBestSubset(std::vector<std::pair<CAmount, std::pair<MapRecords_t::const_iterator,unsigned int> > >vValue, const CAmount& nTotalLower, const CAmount& nTargetValue,
                                  std::vector<char>& vfBest, CAmount& nBest, int iterations = 1000)
{
    std::vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    FastRandomContext insecure_rand;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        CAmount nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                //The solver here uses a randomized algorithm,
                //the randomness serves no real security purpose but is just
                //needed to prevent degenerate behavior and it is important
                //that the rng is fast. We do not use a constant random sequence,
                //because there may be some privacy improvement by making
                //the selection random.
                if (nPass == 0 ? insecure_rand.rand32()&1 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

bool CHDWallet::SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs,
    uint64_t nMaxAncestors, std::vector<COutputR> vCoins, std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> >& setCoinsRet, CAmount& nValueRet) const
{
    
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    std::pair<CAmount, std::pair<MapRecords_t::const_iterator,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<CAmount>::max();
    coinLowestLarger.second.first = mapRecords.end();
    std::vector<std::pair<CAmount, std::pair<MapRecords_t::const_iterator,unsigned int> > > vValue;
    CAmount nTotalLower = 0;
    
    random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

    for (const auto &r : vCoins)
    {
        //if (!r.fSpendable)
        //    continue;
        MapRecords_t::const_iterator rtxi = r.rtx;
        const CTransactionRecord *rtx = &rtxi->second;
        
        const CWalletTx *pcoin = GetWalletTx(r.txhash);
        if (!pcoin)
        {
            if (0 != InsertTempTxn(r.txhash, rtx->blockHash, rtx->nIndex)
                || !(pcoin = GetWalletTx(r.txhash)))
                return error("%s: InsertTempTxn failed.\n", __func__);
        };
        
        
        if (r.nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? nConfMine : nConfTheirs))
            continue;
        
        if (!mempool.TransactionWithinChainLimit(r.txhash, nMaxAncestors))
            continue;
        
        const COutputRecord *oR = rtx->GetOutput(r.i);
        if (!oR)
            return error("%s: GetOutput failed, %s, %d.\n", r.txhash.ToString(), r.i);
        
        CAmount nV = oR->nValue;
        std::pair<CAmount,std::pair<MapRecords_t::const_iterator,unsigned int> > coin = std::make_pair(nV, std::make_pair(rtxi, r.i));
        
        if (nV == nTargetValue)
        {
            setCoinsRet.push_back(coin.second);
            nValueRet += coin.first;
            return true;
        } else
        if (nV < nTargetValue + MIN_CHANGE)
        {
            vValue.push_back(coin);
            nTotalLower += nV;
        } else
        if (nV < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    };
    
    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.push_back(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    };
    
    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == mapRecords.end())
            return false;
        setCoinsRet.push_back(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }
    
    // Solve subset sum by stochastic approximation
    std::sort(vValue.begin(), vValue.end(), CompareValueOnly());
    std::reverse(vValue.begin(), vValue.end());
    std::vector<char> vfBest;
    CAmount nBest;
    
    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + MIN_CHANGE)
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + MIN_CHANGE, vfBest, nBest);

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (coinLowestLarger.second.first != mapRecords.end() &&
        ((nBest != nTargetValue && nBest < nTargetValue + MIN_CHANGE) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.push_back(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    } else
    {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.push_back(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        LogPrint("selectcoins", "SelectCoins() best subset: ");
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                LogPrint("selectcoins", "%s ", FormatMoney(vValue[i].first));
        LogPrint("selectcoins", "total %s\n", FormatMoney(nBest));
    }

    return true;
}

/**
 * Outpoint is spent if any non-conflicted transaction
 * spends it:
 */
bool CHDWallet::IsSpent(const uint256& hash, unsigned int n) const
{
    const COutPoint outpoint(hash, n);
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    
    for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
    {
        const uint256& wtxid = it->second;
        std::map<uint256, CWalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end())
        {
            if (mit->second.isAbandoned())
                continue;
            
            int depth = mit->second.GetDepthInMainChain();
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned()))
                return true; // Spent
        };
        
        MapRecords_t::const_iterator rit = mapRecords.find(wtxid);
        if (rit != mapRecords.end())
        {
            if (rit->second.IsAbandoned())
                continue;
            
            int depth = GetDepthInMainChain(rit->second.blockHash);
            if (depth >= 0)
                return true; // Spent
        };
    };
    
    return false;
};

void CHDWallet::SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator> range)
{
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx* copyFrom = NULL;
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        CWalletTx *wtx = GetWalletTx(hash);
        if (!wtx)
            continue;
        
        int n = wtx->nOrderPos;
        if (n < nMinOrderPos)
        {
            nMinOrderPos = n;
            copyFrom = wtx;
        }
    }
    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        
        CWalletTx* copyTo = GetWalletTx(hash);
        if (!copyTo)
            continue;
        
        if (copyFrom == copyTo) continue;
        if (!copyFrom->IsEquivalentTo(*copyTo)) continue;
        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        // fTimeReceivedIsTxTime not copied on purpose
        // nTimeReceived not copied on purpose
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        copyTo->strFromAccount = copyFrom->strFromAccount;
        // nOrderPos not copied on purpose
        // cached members not copied on purpose
    }
}


void CHDWallet::AvailableCoinsForStaking(std::vector<COutput> &vCoins, int64_t nTime, int nHeight)
{
    vCoins.clear();
    
    deepestTxnDepth = 0;
    
    {
        LOCK2(cs_main, cs_wallet);
        
        int nHeight = chainActive.Tip()->nHeight;
        int nRequiredDepth = std::min((int)(Params().GetStakeMinConfirmations()-1), (int)(nHeight / 2));
        
        for (WalletTxMap::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            CWalletTx *pcoin = &it->second;
            CTransactionRef tx = pcoin->tx;
            
            int nDepth = pcoin->GetDepthInMainChainCached();
            
            if (nDepth > deepestTxnDepth)
                deepestTxnDepth = nDepth;
            
            if (nDepth < nRequiredDepth)
                continue;
            
            const uint256 &wtxid = it->first;
            for (size_t i = 0; i < tx->vpout.size(); ++i)
            {
                if (tx->vpout[i]->nVersion != OUTPUT_STANDARD)
                    continue;
                
                COutPoint kernel(wtxid, i);
                if (CheckStakeUnused(kernel)
                    && !(IsSpent(wtxid, i)) && IsMine(tx->vpout[i].get())) // TODO: [rm] && ((CTxOutStandard*)tx->vpout[i].get())->nValue >= nMinimumInputValue
                    vCoins.push_back(COutput(pcoin, i, nDepth, true, true));
            };
        };
        
        for (std::map<uint256, CTransactionRecord>::const_iterator it = mapRecords.begin(); it != mapRecords.end(); ++it)
        {
            const uint256 &txid = it->first;
            const CTransactionRecord &rtx = it->second;
            
            int nDepth = GetDepthInMainChain(rtx.blockHash);
            if (nDepth > deepestTxnDepth)
                deepestTxnDepth = nDepth;
            
            if (nDepth < nRequiredDepth)
                continue;
            
            std::map<uint256, CWalletTx>::const_iterator twi = mapTempWallet.end();
            for (auto &r : rtx.vout)
            {
                if (r.nType != OUTPUT_STANDARD)
                    continue;
                
                if (r.nFlags & ORF_OWNED && !(IsSpent(txid, r.n)))
                {
                    if (twi == mapTempWallet.end()
                        && (twi = mapTempWallet.find(txid)) == mapTempWallet.end())
                    {
                        if (0 != InsertTempTxn(txid, rtx.blockHash, rtx.nIndex)
                            || (twi = mapTempWallet.find(txid)) == mapTempWallet.end())
                        {
                            LogPrintf("ERROR: %s - InsertTempTxn failed %s.", __func__, txid.ToString());
                            return;
                        };
                    };
                    vCoins.push_back(COutput(&twi->second, r.n, nDepth, true, true));
                };
                
            };
        };
        
    }
    
    std::shuffle(std::begin(vCoins), std::end(vCoins), std::default_random_engine(unsigned(time(NULL))));
};

bool CHDWallet::SelectCoinsForStaking(int64_t nTargetValue, int64_t nTime, int nHeight, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet)
{
    std::vector<COutput> vCoins;
    AvailableCoinsForStaking(vCoins, nTime, nHeight);

    setCoinsRet.clear();
    nValueRet = 0;
    
    for (auto &output : vCoins)
    {
        const CWalletTx *pcoin = output.tx;
        int i = output.i;
        
        // Stop if we've chosen enough inputs
        if (nValueRet >= nTargetValue)
            break;
        
        int64_t n = pcoin->tx->vpout[i]->GetValue();
        
        std::pair<int64_t, std::pair<const CWalletTx*, unsigned int> > coin = std::make_pair(n, std::make_pair(pcoin, i));
        
        if (n >= nTargetValue)
        {
            // If input value is greater or equal to target then simply insert
            //    it into the current subset and exit
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            break;
        } else
        if (n < nTargetValue + CENT)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
        };
    };
    
    return true;
}

typedef std::vector<unsigned char> valtype;

bool CHDWallet::CreateCoinStake(unsigned int nBits, int64_t nTime, int nBlockHeight, int64_t nFees, CMutableTransaction &txNew, CKey &key)
{
    CBlockIndex *pindexPrev = chainActive.Tip();
    arith_uint256 bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    
    txNew.vin.clear();
    txNew.vout.clear();
    txNew.vpout.clear();
    
    // Mark coin stake transaction
    txNew.nVersion = PARTICL_TXN_VERSION;
    txNew.SetType(TXN_COINSTAKE);
    
    CAmount nBalance = GetBalance();
    
    if (nBalance <= nReserveBalance)
        return false;
    
    // Choose coins to use
    std::vector<const CWalletTx*> vwtxPrev;
    std::set<std::pair<const CWalletTx*,unsigned int> > setCoins;
    CAmount nValueIn = 0;

    // Select coins with suitable depth
    if (!SelectCoinsForStaking(nBalance - nReserveBalance, nTime, nBlockHeight, setCoins, nValueIn))
        return false;
    
    if (setCoins.empty())
        return false;

    CAmount nCredit = 0;
    CScript scriptPubKeyKernel;

    std::set<std::pair<const CWalletTx*,unsigned int> >::iterator it = setCoins.begin();

    for (; it != setCoins.end(); ++it)
    {
        auto pcoin = *it;
        if (ThreadStakeMinerStopped()) // interruption_point
            return false;

        COutPoint prevoutStake = COutPoint(pcoin.first->GetHash(), pcoin.second);

        int64_t nBlockTime;
        if (CheckKernel(pindexPrev, nBits, nTime, prevoutStake, &nBlockTime))
        {
            // Found a kernel
            LogPrint("pos", "%s: Kernel found.\n", __func__);
            
            if (!pcoin.first->tx->vpout[pcoin.second]->IsStandardOutput())
                continue;
            CTxOutStandard *kernelOut = (CTxOutStandard*)pcoin.first->tx->vpout[pcoin.second].get();
            
            std::vector<valtype> vSolutions;
            txnouttype whichType;

            if (!Solver(kernelOut->scriptPubKey, whichType, vSolutions))
            {
                LogPrint("pos", "%s: Failed to parse kernel.\n", __func__);
                break;
            };
            
            LogPrint("pos", "%s: Parsed kernel type=%d.\n", __func__, whichType);

            if (whichType != TX_PUBKEYHASH)
            {
                LogPrint("pos", "%s: No support for kernel type=%d.\n", __func__, whichType);
                break;  // only support pay to address (pay to pubkey hash)
            };
            
            CKeyID spendId = uint160(vSolutions[0]);
            if (!GetKey(spendId, key))
            {
                LogPrint("pos", "%s: Failed to get key for kernel type=%d.\n", __func__, whichType);
                break;  // unable to find corresponding key
            };
            
            //scriptPubKeyKernel << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
            scriptPubKeyKernel << OP_DUP << OP_HASH160 << ToByteVector(spendId) << OP_EQUALVERIFY << OP_CHECKSIG;
            
            txNew.nVersion = PARTICL_TXN_VERSION;
            txNew.SetType(TXN_COINSTAKE);
            txNew.vin.clear();
            txNew.vout.clear();
            
            
            txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
            
            nCredit += kernelOut->nValue;
            vwtxPrev.push_back(pcoin.first);
            
            
            std::shared_ptr<CTxOutData> out0 = MAKE_OUTPUT<CTxOutData>();
            out0->vData.resize(4);
            memcpy(&out0->vData[0], &nBlockHeight, 4);
            
            uint32_t voteToken = 0;
            if (GetVote(nBlockHeight, voteToken))
            {
                size_t origSize = out0->vData.size();
                out0->vData.resize(origSize + 5);
                out0->vData[origSize] = DO_VOTE;
                memcpy(&out0->vData[origSize+1], &voteToken, 4);
            };
            
            txNew.vpout.push_back(out0);
            
            std::shared_ptr<CTxOutStandard> out1 = MAKE_OUTPUT<CTxOutStandard>();
            out1->nValue = 0;
            out1->scriptPubKey = scriptPubKeyKernel;
            txNew.vpout.push_back(out1);
            
            LogPrint("pos", "%s: Added kernel.\n", __func__);
            
            setCoins.erase(it);
            break;
        };
    };
    
    if (nCredit == 0 || nCredit > nBalance - nReserveBalance)
    {
        return false;
    };
    
    const size_t nMaxStakeCombine = 3; // TODO: make option
    size_t nStakesCombined = 0;
    
    // Attempt to add more inputs
    // TODO: necessary with static reward?
    // only advantage here is to setup the next stake using this output as a kernel to have a higher chance of staking
    it = setCoins.begin();
    while (it != setCoins.end())
    {
        // Only add coins of the same key/address as kernel
        
        if (nStakesCombined >= nMaxStakeCombine)
            break;
        
        std::set<std::pair<const CWalletTx*,unsigned int> >::iterator itc = it++; // copy the current iterator then increment it
        auto pcoin = *itc;
        
        if (!pcoin.first->tx->vpout[pcoin.second]->IsStandardOutput())
            continue;
        CTxOutStandard *prevOut = (CTxOutStandard*)pcoin.first->tx->vpout[pcoin.second].get();
        
        if (prevOut->scriptPubKey != scriptPubKeyKernel)
            continue;
        
        if (pcoin.first->GetHash() != txNew.vin[0].prevout.hash)
            continue;
        
        //int64_t nTimeWeight = GetWeight((int64_t)pcoin.first->nTime, (int64_t)txNew.nTime);
        // Stop adding more inputs if already too many inputs
        if (txNew.vin.size() >= 100)
            break;
        
        // Stop adding more inputs if value is already pretty significant
        if (nCredit >= Params().GetStakeCombineThreshold())
            break;
        
        // Stop adding inputs if reached reserve limit
        if (nCredit + prevOut->nValue > nBalance - nReserveBalance)
            break;
    
        // Do not add additional significant input
        if (prevOut->nValue >= Params().GetStakeCombineThreshold())
            continue;
        
        txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
        nCredit += prevOut->nValue;
        vwtxPrev.push_back(pcoin.first);
        
        LogPrint("pos", "%s: Combining kernel %s, %d.\n", __func__, pcoin.first->GetHash().ToString(), pcoin.second);
        nStakesCombined++;
        setCoins.erase(itc);
    };
    
    CAmount nReward = Params().GetProofOfStakeReward(pindexPrev, nFees);
    if (nReward <= 0)
        return false;
    
    nCredit += nReward;
    
    // Set output amount, split outputs if > GetStakeSplitThreshold
    if (nCredit >= Params().GetStakeSplitThreshold())
    {
        std::shared_ptr<CTxOutStandard> outSplit = MAKE_OUTPUT<CTxOutStandard>();
        outSplit->nValue = 0;
        outSplit->scriptPubKey = scriptPubKeyKernel;
        
        txNew.vpout.back()->SetValue(nCredit / 2);
        outSplit->SetValue(nCredit - txNew.vpout.back()->GetValue());
        txNew.vpout.push_back(outSplit);
    } else
    {
        txNew.vpout.back()->SetValue(nCredit);
    };
    
    // Sign
    int nIn = 0;
    for (const auto &pcoin : vwtxPrev)
    {
        uint32_t nPrev = txNew.vin[nIn].prevout.n;
        
        CTxOutStandard *prevOut = (CTxOutStandard*)pcoin->tx->vpout[nPrev].get();
        CScript &scriptPubKeyOut = prevOut->scriptPubKey;
        std::vector<uint8_t> vchAmount;
        prevOut->PutValue(vchAmount);
        
        SignatureData sigdata;
        CTransaction txToConst(txNew);
        if (!ProduceSignature(TransactionSignatureCreator(this,&txToConst, nIn, vchAmount, SIGHASH_ALL), scriptPubKeyOut, sigdata))
        {
            LogPrint("pos", "%s: ProduceSignature failed.\n", __func__);
            return false;
        };
        
        UpdateTransaction(txNew, nIn, sigdata);
        nIn++;
    };
    
    // Limit size
    unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
    if (nBytes >= DEFAULT_BLOCK_MAX_SIZE/5)
        return error("CreateCoinStake : exceeded coinstake size limit");

    // Successfully generated coinstake
    return true;
};

bool CHDWallet::SignBlock(CBlockTemplate *pblocktemplate, int nHeight, int64_t nSearchTime)
{
    LogPrint("pos", "%s, nHeight %d\n", __func__, nHeight);
    
    CBlock *pblock = &pblocktemplate->block;
    if (pblock->vtx.size() < 1)
        return error("%s: Malformed block.", __func__);
    
    int64_t nFees = -pblocktemplate->vTxFees[0];
    
    CBlockIndex *pindexPrev = chainActive.Tip();
    
    CKey key;
    pblock->nVersion = PARTICL_BLOCK_VERSION;
    pblock->nBits = GetNextTargetRequired(pindexPrev);
    LogPrint("pos", "%s, nBits %d\n", __func__, pblock->nBits);
    
    CMutableTransaction txCoinStake;
    txCoinStake.nVersion = PARTICL_TXN_VERSION;
    txCoinStake.SetType(TXN_COINSTAKE);
    
    if (CreateCoinStake(pblock->nBits, nSearchTime, nHeight, nFees, txCoinStake, key))
    {
        LogPrint("pos", "%s: Kernel found.\n", __func__);
        
        //if (txCoinStake.nTime >= chainActive.Tip()->GetPastTimeLimit()+1)
        if (nSearchTime >= chainActive.Tip()->GetPastTimeLimit()+1)
        {
            // make sure coinstake would meet timestamp protocol
            //    as it would be the same as the block timestamp
            //pblock->nTime = txCoinStake.nTime;
            pblock->nTime = nSearchTime;
            
            // Remove coinbasetxn
            pblock->vtx[0].reset();
            pblock->vtx.erase(pblock->vtx.begin());
            
            
            // Insert coinstake as txn0
            pblock->vtx.insert(pblock->vtx.begin(), MakeTransactionRef(txCoinStake));
            
            bool mutated;
            pblock->hashMerkleRoot = BlockMerkleRoot(*pblock, &mutated);
            pblock->hashWitnessMerkleRoot = BlockWitnessMerkleRoot(*pblock, &mutated);

            // Append a signature to the block
            return key.Sign(pblock->GetHash(), pblock->vchBlockSig);
        };
    };
    
    nLastCoinStakeSearchTime = nSearchTime;
    
    return false;
};

int ToStealthRecipient(CStealthAddress &sx, CAmount nValue, bool fSubtractFeeFromAmount,
    std::vector<CRecipient> &vecSend, std::string &sNarr, std::string &strError)
{
    CScript scriptPubKey;
    CKey sEphem;
    CKey sShared;
    ec_point pkSendTo;
    
    int k, nTries = 24;
    for (k = 0; k < nTries; ++k) // if StealthSecret fails try again with new ephem key
    {
        sEphem.MakeNewKey(true);
        if (StealthSecret(sEphem, sx.scan_pubkey, sx.spend_pubkey, sShared, pkSendTo) == 0)
            break;
    };
    if (k >= nTries)
    {
        strError = "Could not generate receiving public key.";
        return 1;
    };
    
    CPubKey pkEphem = sEphem.GetPubKey();
    CPubKey pkTo(pkSendTo);
    CKeyID idTo = pkTo.GetID();
    
    if (fDebug)
    {
        LogPrint("hdwallet", "Stealth send to generated address: %s\n", CBitcoinAddress(idTo).ToString());
        //LogPrint("hdwallet", "ephem_pubkey: %s\n", HexStr(pkEphem.begin(), pkEphem.end()));
    };
    
    scriptPubKey = GetScriptForDestination(idTo);
    CRecipient recipient(scriptPubKey, nValue, fSubtractFeeFromAmount);
    vecSend.push_back(recipient);
    
    std::vector<uint8_t> vchNarr;
    if (sNarr.length() > 0)
    {
        SecMsgCrypter crypter;
        crypter.SetKey(sShared.begin(), pkEphem.begin());
        
        if (!crypter.Encrypt((uint8_t*)sNarr.data(), sNarr.length(), vchNarr))
        {
            strError = "Narration encryption failed.";
            return 1;
        };

        if (vchNarr.size() > MAX_STEALTH_NARRATION_SIZE)
        {
            strError = "Encrypted narration is too long.";
            return 1;
        };
    };
    
    vecSend.push_back(CRecipient());
    CRecipient &rData = vecSend.back();
    rData.vData.resize(34
        + (sx.prefix.number_bits > 0 ? 5 : 0)
        + (vchNarr.size() + (vchNarr.size() > 0 ? 1 : 0)));
    
    size_t o = 0;
    rData.vData[o++] = DO_STEALTH;
    memcpy(&rData.vData[o], pkEphem.begin(), 33);
    o += 33;
    
    if (sx.prefix.number_bits > 0)
    {
        rData.vData[o++] = DO_STEALTH_PREFIX;
        uint32_t prefix, mask = SetStealthMask(sx.prefix.number_bits);
        GetStrongRandBytes((uint8_t*) &prefix, 4);
        
        prefix = prefix & (~mask);
        prefix |= sx.prefix.bitfield & mask;
        
        memcpy(&rData.vData[o], &prefix, 4);
        o+=4;
    };
    
    if (vchNarr.size() > 0)
    {
        rData.vData[o++] = DO_NARR_CRYPT;
        memcpy(&rData.vData[o], &vchNarr[0], vchNarr.size());
        o += vchNarr.size();
    };
    
    return 0;
};


int LoopExtKeysInDB(bool fInactive, bool fInAccount, LoopExtKeyCallback &callback)
{
    AssertLockHeld(pwalletMain->cs_wallet);
    
    CHDWalletDB wdb(pwalletMain->strWalletFile);
    
    Dbc *pcursor;
    if (!(pcursor = wdb.GetCursor()))
        throw std::runtime_error(strprintf("%s : cannot create DB cursor", __func__).c_str());
    
    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    
    CKeyID ckeyId;
    CStoredExtKey sek;
    std::string strType;
    
    uint32_t fFlags = DB_SET_RANGE;
    ssKey << std::string("ek32");
    
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;
        
        ssKey >> strType;
        if (strType != "ek32")
            break;
        
        ssKey >> ckeyId;
        ssValue >> sek;
        if (!fInAccount
            && sek.nFlags & EAF_IN_ACCOUNT)
            continue;
        callback.ProcessKey(ckeyId, sek);
    };
    pcursor->close();
    
    return 0;
};


int LoopExtAccountsInDB(bool fInactive, LoopExtKeyCallback &callback)
{
    AssertLockHeld(pwalletMain->cs_wallet);
    CHDWalletDB wdb(pwalletMain->strWalletFile);
    // - list accounts
    
    Dbc *pcursor;
    if (!(pcursor = wdb.GetCursor()))
        throw std::runtime_error(strprintf("%s : cannot create DB cursor", __func__).c_str());
    
    CDataStream ssKey(SER_DISK, CLIENT_VERSION);
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);
    CKeyID idAccount;
    CExtKeyAccount sea;
    CBitcoinAddress addr;
    std::string strType, sError;
    
    uint32_t fFlags = DB_SET_RANGE;
    ssKey << std::string("eacc");
    
    while (wdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags) == 0)
    {
        fFlags = DB_NEXT;
        
        ssKey >> strType;
        if (strType != "eacc")
            break;
        
        ssKey >> idAccount;
        ssValue >> sea;
        
        sea.vExtKeys.resize(sea.vExtKeyIDs.size());
        for (size_t i = 0; i < sea.vExtKeyIDs.size(); ++i)
        {
            CKeyID &id = sea.vExtKeyIDs[i];
            CStoredExtKey *sek = new CStoredExtKey();
            
            if (wdb.ReadExtKey(id, *sek))
            {
                sea.vExtKeys[i] = sek;
            } else
            {
                addr.Set(idAccount, CChainParams::EXT_ACC_HASH);
                LogPrintf("WARNING: Could not read key %d of account %s\n", i, addr.ToString().c_str());
                sea.vExtKeys[i] = NULL;
                delete sek;
            };
        };
        callback.ProcessAccount(idAccount, sea);
        
        sea.FreeChains();
    };
    
    pcursor->close();
    
    return 0;
};

