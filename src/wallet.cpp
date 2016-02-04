// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"
#include "wallet.h"
#include "walletdb.h"
#include "bloom.h"
#include "crypter.h"
#include "ui_interface.h"
#include "base58.h"
#include "kernel.h"
#include "coincontrol.h"
#include <boost/algorithm/string/replace.hpp>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// mapWallet
//

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t1,
                    const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

CPubKey CWallet::GenerateNewKey()
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    RandAddSeedPerfmon();
    CKey secret;
    secret.MakeNewKey(fCompressed);

    // Compressed public keys were introduced in version 0.6.0
    if (fCompressed)
        SetMinVersion(FEATURE_COMPRPUBKEY);

    CPubKey pubkey = secret.GetPubKey();

    // Create new metadata
    int64_t nCreationTime = GetTime();
    mapKeyMetadata[pubkey.GetID()] = CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;

    if (!AddKeyPubKey(secret, pubkey))
        throw std::runtime_error("CWallet::GenerateNewKey() : AddKeyPubKey failed");
    return pubkey;
}

bool CWallet::AddKey(const CKey& secret)
{
    return CWallet::AddKeyPubKey(secret, secret.GetPubKey());
}

bool CWallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    if (!fFileBacked)
        return true;
    if (!IsCrypted()) {
        return CWalletDB(strWalletFile).WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
    }
    return true;
}

bool CWallet::AddKeyInDBTxn(CWalletDB* pdb, const CKey& key)
{
    LOCK(cs_KeyStore);
    // -- can't use CWallet::AddKey(), as in a db transaction
    //    hack: pwalletdbEncryption CCryptoKeyStore::AddKey calls CWallet::AddCryptedKey
    //    DB Write() uses activeTxn
    CWalletDB *pwalletdbEncryptionOld = pwalletdbEncryption;
    pwalletdbEncryption = pdb;
    
    CPubKey pubkey = key.GetPubKey();
    bool rv = CCryptoKeyStore::AddKeyPubKey(key, pubkey);
    
    pwalletdbEncryption = pwalletdbEncryptionOld;
    
    if (!rv)
    {
        LogPrintf("CCryptoKeyStore::AddKeyPubKey failed.\n");
        return false;
    };

    if (fFileBacked
        && !IsCrypted())
    {
        if (!pdb->WriteKey(pubkey, key.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]))
        {
            LogPrintf("WriteKey() failed.\n");
            return false;
        };
    };
    return true;
};

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey, const vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;

    if (!fFileBacked)
        return true;

    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
    }

    return false;
}

bool CWallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

bool CWallet::AddCScript(const CScript& redeemScript)
{
    if (!CCryptoKeyStore::AddCScript(redeemScript))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteCScript(Hash160(redeemScript), redeemScript);
}

// optional setting to unlock wallet for staking only
// serves to disable the trivial sendmoney when OS account compromised
// provides no real security
bool fWalletUnlockStakingOnly = false;

bool CWallet::LoadCScript(const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = CBitcoinAddress(redeemScript.GetID()).ToString();
        LogPrintf("%s: Warning: This wallet contains a redeemScript of size %u which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n",
            __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr.c_str());
        return true;
    };

    return CCryptoKeyStore::AddCScript(redeemScript);
}

bool CWallet::Lock()
{
    if (fDebug)
        LogPrintf("CWallet::Lock()\n");
    
    if (IsLocked())
        return true;

    LogPrintf("Locking wallet.\n");

    {
        LOCK(cs_wallet);
        CWalletDB wdb(strWalletFile);

        // -- load encrypted spend_secret of stealth addresses
        CStealthAddress sxAddrTemp;
        std::set<CStealthAddress>::iterator it;
        for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
        {
            if (it->scan_secret.size() < 32)
                continue; // stealth address is not owned
            // -- CStealthAddress are only sorted on spend_pubkey
            CStealthAddress &sxAddr = const_cast<CStealthAddress&>(*it);
            if (fDebug)
                LogPrintf("Recrypting stealth key %s\n", sxAddr.Encoded().c_str());

            sxAddrTemp.scan_pubkey = sxAddr.scan_pubkey;
            if (!wdb.ReadStealthAddress(sxAddrTemp))
            {
                LogPrintf("Error: Failed to read stealth key from db %s\n", sxAddr.Encoded().c_str());
                continue;
            }
            sxAddr.spend_secret = sxAddrTemp.spend_secret;
        };
    }
    return LockKeyStore();
};

bool CWallet::Unlock(const SecureString& strWalletPassphrase)
{
    if (fDebug)
        LogPrintf("CWallet::Unlock()\n");

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
                return false;
            if (!CCryptoKeyStore::Unlock(vMasterKey))
                return false;
            break;
        };

        UnlockStealthAddresses(vMasterKey);
        ProcessLockedAnonOutputs();
        SecureMsgWalletUnlocked();
    }
    return true;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked();

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;
        BOOST_FOREACH(MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey)
                && UnlockStealthAddresses(vMasterKey))
            {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                LogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;

                CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();
                return true;
            }
        }
    }

    return false;
}

void CWallet::SetBestChain(const CBlockLocator& loc)
{
    CWalletDB walletdb(strWalletFile);
    walletdb.WriteBestBlock(loc);
}

void CWallet::SetBestThinChain(const CBlockThinLocator& loc)
{
    CWalletDB walletdb(strWalletFile);
    walletdb.WriteBestBlockThin(loc);
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn, bool fExplicit)
{
    LOCK(cs_wallet); // nWalletVersion
    if (nWalletVersion >= nVersion)
        return true;

    // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
    if (fExplicit && nVersion > nWalletMaxVersion)
            nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    if (fFileBacked)
    {
        CWalletDB* pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
        if (nWalletVersion > 40000)
            pwalletdb->WriteMinVersion(nWalletVersion);
        if (!pwalletdbIn)
            delete pwalletdb;
    };

    return true;
}

bool CWallet::SetMaxVersion(int nVersion)
{
    LOCK(cs_wallet); // nWalletVersion, nWalletMaxVersion
    // cannot downgrade below current version
    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial vMasterKey;
    RandAddSeedPerfmon();

    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    RAND_bytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey(nDerivationMethodIndex);

    RandAddSeedPerfmon();
    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    RAND_bytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

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
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        if (fFileBacked)
        {
            pwalletdbEncryption = new CWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin())
                return false;
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        };

        if (!EncryptKeys(vMasterKey))
        {
            if (fFileBacked)
                pwalletdbEncryption->TxnAbort();
            exit(1); //We now probably have half of our keys encrypted in memory, and half not...die and let the user reload their unencrypted wallet.
        };

        std::set<CStealthAddress>::iterator it;
        for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
        {
            if (it->scan_secret.size() < 32)
                continue; // stealth address is not owned
            // -- CStealthAddress is only sorted on spend_pubkey
            CStealthAddress &sxAddr = const_cast<CStealthAddress&>(*it);

            if (fDebug)
                LogPrintf("Encrypting stealth key %s\n", sxAddr.Encoded().c_str());

            std::vector<unsigned char> vchCryptedSecret;

            CSecret vchSecret;
            vchSecret.resize(32);
            memcpy(&vchSecret[0], &sxAddr.spend_secret[0], 32);

            uint256 iv = Hash(sxAddr.spend_pubkey.begin(), sxAddr.spend_pubkey.end());
            if (!EncryptSecret(vMasterKey, vchSecret, iv, vchCryptedSecret))
            {
                LogPrintf("Error: Failed encrypting stealth key %s\n", sxAddr.Encoded().c_str());
                continue;
            };

            sxAddr.spend_secret = vchCryptedSecret;
            pwalletdbEncryption->WriteStealthAddress(sxAddr);
        };

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, pwalletdbEncryption, true);

        if (fFileBacked)
        {
            if (!pwalletdbEncryption->TxnCommit())
                exit(1); //We now have keys encrypted in memory, but no on disk...die to avoid confusion and let the user reload their unencrypted wallet.

            delete pwalletdbEncryption;
            pwalletdbEncryption = NULL;
        };

        Lock();
        Unlock(strWalletPassphrase);
        NewKeyPool();
        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        CDB::Rewrite(strWalletFile);

    }
    NotifyStatusChanged(this);

    return true;
}

int64_t CWallet::IncOrderPosNext(CWalletDB *pwalletdb)
{
    AssertLockHeld(cs_wallet); // nOrderPosNext
    int64_t nRet = nOrderPosNext++;
    if (pwalletdb)
    {
        pwalletdb->WriteOrderPosNext(nOrderPosNext);
    } else
    {
        CWalletDB(strWalletFile).WriteOrderPosNext(nOrderPosNext);
    };

    return nRet;
}

CWallet::TxItems CWallet::OrderedTxItems(std::list<CAccountingEntry>& acentries, std::string strAccount, bool fShowCoinstake)
{
    AssertLockHeld(cs_wallet); // mapWallet
    CWalletDB walletdb(strWalletFile);

    // First: get all CWalletTx and CAccountingEntry into a sorted-by-order multimap.
    TxItems txOrdered;

    // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
    // would make this much faster for applications that do this a lot.
    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        CWalletTx* wtx = &((*it).second);

        if (!fShowCoinstake
            && wtx->IsCoinStake())
            continue;

        txOrdered.insert(make_pair(wtx->nOrderPos, TxPair(wtx, (CAccountingEntry*)0)));
    };

    acentries.clear();
    walletdb.ListAccountCreditDebit(strAccount, acentries);
    BOOST_FOREACH(CAccountingEntry& entry, acentries)
    {
        txOrdered.insert(make_pair(entry.nOrderPos, TxPair((CWalletTx*)0, &entry)));
    };

    return txOrdered;
}

void CWallet::WalletUpdateSpent(const CTransaction &tx, bool fBlock)
{
    // Anytime a signature is successfully verified, it's proof the outpoint is spent.
    // Update the wallet spent flag if it doesn't know due to wallet.dat being
    // restored from backup or the user making copies of wallet.dat.
    {
        LOCK(cs_wallet);
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            if (tx.nVersion == ANON_TXN_VERSION
                && txin.IsAnonInput())
            {
                // anon input
                // TODO
                continue;
            }

            std::map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi != mapWallet.end())
            {
                CWalletTx& wtx = (*mi).second;
                if (txin.prevout.n >= wtx.vout.size())
                {
                    LogPrintf("WalletUpdateSpent: bad wtx %s\n", wtx.GetHash().ToString().c_str());
                } else
                if (!wtx.IsSpent(txin.prevout.n) && IsMine(wtx.vout[txin.prevout.n]))
                {
                    LogPrintf("WalletUpdateSpent found spent coin %s GAMEUNITS %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkSpent(txin.prevout.n);
                    wtx.WriteToDisk();
                    NotifyTransactionChanged(this, txin.prevout.hash, CT_UPDATED);
                };
            };
        };

        if (fBlock)
        {
            uint256 hash = tx.GetHash();
            map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
            CWalletTx& wtx = (*mi).second;

            BOOST_FOREACH(const CTxOut& txout, tx.vout)
            {
                if (tx.nVersion == ANON_TXN_VERSION
                    && txout.IsAnonOutput())
                {
                    // anon output
                    // TODO
                    continue;
                }

                if (IsMine(txout))
                {
                    wtx.MarkUnspent(&txout - &tx.vout[0]);
                    wtx.WriteToDisk();
                    NotifyTransactionChanged(this, hash, CT_UPDATED);
                };
            };
        };

    }
}

void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
            item.second.MarkDirty();
    }
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn, const uint256& hashIn)
{
    //uint256 hashIn = wtxIn.GetHash();
    {
        LOCK(cs_wallet);

        // Inserts only if not already there, returns tx inserted or tx found
        pair<map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hashIn, wtxIn));
        CWalletTx& wtx = (*ret.first).second;
        wtx.BindWallet(this);
        bool fInsertedNew = ret.second;
        if (fInsertedNew)
        {
            wtx.nTimeReceived = GetAdjustedTime();
            wtx.nOrderPos = IncOrderPosNext();

            wtx.nTimeSmart = wtx.nTimeReceived;
            if (wtxIn.hashBlock != 0)
            {
                bool fInBlockIndex = false;
                unsigned int blocktime;

                if (nNodeMode == NT_FULL)
                {
                    fInBlockIndex = mapBlockIndex.count(wtxIn.hashBlock);
                    blocktime = mapBlockIndex[wtxIn.hashBlock]->nTime;
                } else
                {
                    //fInBlockIndex = mapBlockThinIndex.count(wtxIn.hashBlock);

                    std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(wtxIn.hashBlock);
                    if (mi == mapBlockThinIndex.end()
                        && !fThinFullIndex
                        && pindexRear)
                    {
                        CTxDB txdb("r");
                        CDiskBlockThinIndex diskindex;
                        if (txdb.ReadBlockThinIndex(wtxIn.hashBlock, diskindex)
                            || diskindex.hashNext != 0)
                        {
                            fInBlockIndex = true;
                            blocktime = diskindex.nTime;
                        };
                    } else
                    {
                        fInBlockIndex = true;
                        blocktime = mi->second->nTime;
                    };
                };

                if (fInBlockIndex)
                {
                    unsigned int latestNow = wtx.nTimeReceived;
                    unsigned int latestEntry = 0;

                    // Tolerate times up to the last timestamp in the wallet not more than 5 minutes into the future
                    int64_t latestTolerated = latestNow + 300;
                    std::list<CAccountingEntry> acentries;
                    TxItems txOrdered = OrderedTxItems(acentries);
                    for (TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
                    {
                        CWalletTx *const pwtx = (*it).second.first;
                        if (pwtx == &wtx)
                            continue;

                        CAccountingEntry *const pacentry = (*it).second.second;
                        int64_t nSmartTime;
                        if (pwtx)
                        {
                            nSmartTime = pwtx->nTimeSmart;
                            if (!nSmartTime)
                                nSmartTime = pwtx->nTimeReceived;
                        } else
                        {
                            nSmartTime = pacentry->nTime;
                        };

                        if (nSmartTime <= latestTolerated)
                        {
                            latestEntry = nSmartTime;
                            if (nSmartTime > latestNow)
                                latestNow = nSmartTime;
                            break;
                        };
                    };



                    wtx.nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
                } else
                {
                    LogPrintf("AddToWallet() : found %s in block %s not in index\n",
                        hashIn.ToString().substr(0,10).c_str(),
                        wtxIn.hashBlock.ToString().c_str());
                };
            };
        };

        bool fUpdated = false;

        if (!fInsertedNew)
        {
            // Merge
            if (wtxIn.hashBlock != 0 && wtxIn.hashBlock != wtx.hashBlock)
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            };

            if (wtxIn.nIndex != -1 && (wtxIn.vMerkleBranch != wtx.vMerkleBranch || wtxIn.nIndex != wtx.nIndex))
            {
                wtx.vMerkleBranch = wtxIn.vMerkleBranch;
                wtx.nIndex = wtxIn.nIndex;
                fUpdated = true;
            };

            if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
            {
                wtx.fFromMe = wtxIn.fFromMe;
                fUpdated = true;
            };

            fUpdated |= wtx.UpdateSpent(wtxIn.vfSpent);
        };

        //// debug print
        LogPrintf("AddToWallet %s  %s%s\n", hashIn.ToString().substr(0,10).c_str(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        // Write to disk
        if (fInsertedNew || fUpdated)
        {
            if (!wtx.WriteToDisk())
                return false;
        };

        if (!fHaveGUI)
        {
            // If default receiving address gets used, replace it with a new one
            if (vchDefaultKey.IsValid())
            {
                CScript scriptDefaultKey;
                scriptDefaultKey.SetDestination(vchDefaultKey.GetID());
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if (txout.scriptPubKey == scriptDefaultKey)
                    {
                        CPubKey newDefaultKey;
                        if (GetKeyFromPool(newDefaultKey, false))
                        {
                            SetDefaultKey(newDefaultKey);
                            SetAddressBookName(vchDefaultKey.GetID(), "");
                        };
                    };
                };
            };
        };
        
        // since AddToWallet is called directly for self-originating transactions, check for consumption of own coins
        WalletUpdateSpent(wtx, (wtxIn.hashBlock != 0));

        // Notify UI of new or updated transaction
        NotifyTransactionChanged(this, hashIn, fInsertedNew ? CT_NEW : CT_UPDATED);

        if (nNodeMode == NT_THIN
            && fInsertedNew == CT_NEW
            && pBloomFilter
            && (pBloomFilter->nFlags & BLOOM_UPDATE_MASK) == BLOOM_UPDATE_ALL)
        {
            uint32_t nAdded = 0;

            // -- add unspent outputs to bloom filters
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
            {
                if (wtx.nVersion == ANON_TXN_VERSION
                    && txin.IsAnonInput())
                    continue;

                map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
                if (mi == mapWallet.end())
                    continue;

                CWalletTx& wtxPrev = (*mi).second;

                if (txin.prevout.n >= wtxPrev.vout.size())
                {
                    LogPrintf("AddToWallet: bad wtx %s\n", wtxPrev.GetHash().ToString().c_str());
                } else
                if (!wtxPrev.IsSpent(txin.prevout.n) && IsMine(wtxPrev.vout[txin.prevout.n]))
                {

                    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
                    stream << txin.prevout;
                    std::vector<unsigned char> vData(stream.begin(), stream.end());
                    AddDataToMerkleFilters(vData);
                    nAdded++;
                };
            };

            if (fDebug)
                LogPrintf("AddToWallet() Added %u outputs to bloom filters.\n", nAdded);
        }

        // notify an external script when a wallet transaction comes in or is updated
        std::string strCmd = GetArg("-walletnotify", "");

        if (!strCmd.empty())
        {
            boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
            boost::thread t(runCommand, strCmd); // thread runs free
        };

    }
    return true;
}

// Add a transaction to the wallet, or update it.
// pblock is optional, but should be provided if the transaction is known to be in a block.
// If fUpdate is true, existing transactions will be updated.
bool CWallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const uint256& hash, const void* pblock, bool fUpdate, bool fFindBlock)
{
    //LogPrintf("AddToWalletIfInvolvingMe() %s\n", hash.ToString().c_str()); // happens often

    //uint256 hash = tx.GetHash();
    {
        LOCK(cs_wallet);
        bool fExisted = mapWallet.count(hash);
        if (fExisted && !fUpdate)
        {
            return false;
        };

        mapValue_t mapNarr;

        FindStealthTransactions(tx, mapNarr);

        bool fIsMine = false;
        if (tx.nVersion == ANON_TXN_VERSION)
        {
            LOCK(cs_main); // cs_wallet is already locked
            CWalletDB walletdb(strWalletFile, "cr+");
            CTxDB txdb("cr+");

            uint256 blockHash = 0;
            if (nNodeMode == NT_FULL)
                blockHash = pblock ? ((CBlock*)pblock)->GetHash() : 0;
            else
                blockHash = pblock ? *(uint256*)pblock : 0;

            walletdb.TxnBegin();
            txdb.TxnBegin();
            std::vector<std::map<uint256, CWalletTx>::iterator> vUpdatedTxns;
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
                for (std::vector<std::map<uint256, CWalletTx>::iterator>::iterator it = vUpdatedTxns.begin();
                    it != vUpdatedTxns.end(); ++it)
                    NotifyTransactionChanged(this, (*it)->first, CT_UPDATED);
            };
        };

        if (fExisted || fIsMine || IsMine(tx) || IsFromMe(tx))
        {
            CWalletTx wtx(this, tx);

            if (!mapNarr.empty())
                wtx.mapValue.insert(mapNarr.begin(), mapNarr.end());

            // Get merkle branch if transaction was found in a block
            if (nNodeMode == NT_FULL)
            {
                const CBlock* pcblock = (CBlock*)pblock;
                if (pcblock)
                    wtx.SetMerkleBranch(pcblock);
            } else
            {
                const uint256* pblockhash = (uint256*)pblock;

                if (pblockhash)
                    wtx.hashBlock = *pblockhash;
            };

            return AddToWallet(wtx, hash);
        } else
        {
            WalletUpdateSpent(tx);
        };
    }
    return false;
}

bool CWallet::EraseFromWallet(uint256 hash)
{
    if (!fFileBacked)
        return false;

    {
        LOCK(cs_wallet);
        if (mapWallet.erase(hash))
            CWalletDB(strWalletFile).EraseTx(hash);
    }
    return true;
}


bool CWallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return true;
        };
    }
    return false;
}

int64_t CWallet::GetDebit(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return prev.vout[txin.prevout.n].nValue;
        };
    }
    return 0;
}

int64_t CWallet::GetGameunitsXDebit(const CTxIn& txin) const
{
    if (!txin.IsAnonInput())
        return 0;

    // -- amount of owned gameunits decreased
    // TODO: store links in memory

    {
        LOCK(cs_wallet);

        CWalletDB walletdb(strWalletFile, "r");

        std::vector<uint8_t> vchImage;
        txin.ExtractKeyImage(vchImage);

        COwnedAnonOutput oao;
        if (!walletdb.ReadOwnedAnonOutput(vchImage, oao))
            return 0;
        //return oao.nValue

        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(oao.outpoint.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (oao.outpoint.n < prev.vout.size())
                return prev.vout[oao.outpoint.n].nValue;
        };

    }

    return 0;
};

int64_t CWallet::GetGameunitsXCredit(const CTxOut& txout) const
{
    if (!txout.IsAnonOutput())
        return 0;

    // TODO: store links in memory

    {
        LOCK(cs_wallet);

        CWalletDB walletdb(strWalletFile, "r");

        CPubKey pkCoin    = txout.ExtractAnonPk();

        std::vector<uint8_t> vchImage;
        if (!walletdb.ReadOwnedAnonOutputLink(pkCoin, vchImage))
            return 0;

        COwnedAnonOutput oao;
        if (!walletdb.ReadOwnedAnonOutput(vchImage, oao))
            return 0;

        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(oao.outpoint.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (oao.outpoint.n < prev.vout.size())
            {
                return prev.vout[oao.outpoint.n].nValue;
            }
        };

    }

    return 0;
};

bool CWallet::IsChange(const CTxOut& txout) const
{
    CTxDestination address;

    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a TX_PUBKEYHASH that is mine but isn't in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    if (ExtractDestination(txout.scriptPubKey, address) && ::IsMine(*this, address))
    {
        LOCK(cs_wallet);
        if (!mapAddressBook.count(address))
            return true;
    };

    return false;
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

int CWalletTx::GetRequestCount() const
{
    // Returns -1 if it wasn't being tracked
    int nRequests = -1;

    {
        LOCK(pwallet->cs_wallet);
        if (IsCoinBase() || IsCoinStake())
        {
            // Generated block
            if (hashBlock != 0)
            {
                std::map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                if (mi != pwallet->mapRequestCount.end())
                    nRequests = (*mi).second;
            };
        } else
        {
            // Did anyone request this transaction?
            map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(GetHash());
            if (mi != pwallet->mapRequestCount.end())
            {
                nRequests = (*mi).second;

                // How about the block it's in?
                if (nRequests == 0 && hashBlock != 0)
                {
                    map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                    if (mi != pwallet->mapRequestCount.end())
                        nRequests = (*mi).second;
                    else
                        nRequests = 1; // If it's in someone else's block it must have got out
                };
            };
        };
    }
    return nRequests;
}

void CWalletTx::GetAmounts(list<pair<CTxDestination, int64_t> >& listReceived,
                           list<pair<CTxDestination, int64_t> >& listSent, int64_t& nFee, string& strSentAccount) const
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    // Compute fee:
    int64_t nDebit = GetDebit();

    //if (nVersion == ANON_TXN_VERSION)
    //    nDebit += GetGameunitsXDebit();

    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        int64_t nValueOut = GetValueOut();
        nFee = nDebit - nValueOut;
    };

    // Sent/received.
    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        if (nVersion == ANON_TXN_VERSION
            && txout.IsAnonOutput())
        {
            const CScript &s = txout.scriptPubKey;
            CKeyID ckidD = CPubKey(&s[2+1], 33).GetID();

            bool fIsMine = pwallet->HaveKey(ckidD);

            CTxDestination address = ckidD;

            // If we are debited by the transaction, add the output as a "sent" entry
            if (nDebit > 0)
                listSent.push_back(make_pair(address, txout.nValue));

            // If we are receiving the output, add it as a "received" entry
            if (fIsMine)
                listReceived.push_back(make_pair(address, txout.nValue));

            continue;
        }

        // Skip special stake out
        if (txout.scriptPubKey.empty())
            continue;

        opcodetype firstOpCode;
        CScript::const_iterator pc = txout.scriptPubKey.begin();
        if (txout.scriptPubKey.GetOp(pc, firstOpCode)
            && firstOpCode == OP_RETURN)
            continue;


        bool fIsMine;
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > 0)
        {
            // Don't report 'change' txouts
            if (pwallet->IsChange(txout))
                continue;
            fIsMine = pwallet->IsMine(txout);
        } else
        if (!(fIsMine = pwallet->IsMine(txout)))
            continue;

        // In either case, we need to get the destination address
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address))
        {
            LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                this->GetHash().ToString().c_str());
            address = CNoDestination();
        };

        // If we are debited by the transaction, add the output as a "sent" entry
        if (nDebit > 0)
            listSent.push_back(make_pair(address, txout.nValue));

        // If we are receiving the output, add it as a "received" entry
        if (fIsMine)
            listReceived.push_back(make_pair(address, txout.nValue));
    };

}

void CWalletTx::GetAccountAmounts(const string& strAccount, int64_t& nReceived,
                                  int64_t& nSent, int64_t& nFee) const
{
    nReceived = nSent = nFee = 0;

    int64_t allFee;
    string strSentAccount;
    list<pair<CTxDestination, int64_t> > listReceived;
    list<pair<CTxDestination, int64_t> > listSent;
    GetAmounts(listReceived, listSent, allFee, strSentAccount);

    if (strAccount == strSentAccount)
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& s, listSent)
            nSent += s.second;
        nFee = allFee;
    };

    {
        LOCK(pwallet->cs_wallet);
        BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& r, listReceived)
        {
            if (pwallet->mapAddressBook.count(r.first))
            {
                map<CTxDestination, string>::const_iterator mi = pwallet->mapAddressBook.find(r.first);
                if (mi != pwallet->mapAddressBook.end() && (*mi).second == strAccount)
                    nReceived += r.second;
            } else
            if (strAccount.empty())
            {
                nReceived += r.second;
            }
        }
    }
}

void CWalletTx::AddSupportingTransactions(CTxDB& txdb)
{
    vtxPrev.clear();

    const int COPY_DEPTH = 3;
    if (SetMerkleBranch() < COPY_DEPTH)
    {
        vector<uint256> vWorkQueue;
        BOOST_FOREACH(const CTxIn& txin, vin)
            vWorkQueue.push_back(txin.prevout.hash);

        // This critsect is OK because txdb is already open
        {
            LOCK(pwallet->cs_wallet);
            map<uint256, const CMerkleTx*> mapWalletPrev;
            set<uint256> setAlreadyDone;
            for (unsigned int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hash = vWorkQueue[i];
                if (setAlreadyDone.count(hash))
                    continue;
                setAlreadyDone.insert(hash);

                CMerkleTx tx;
                map<uint256, CWalletTx>::const_iterator mi = pwallet->mapWallet.find(hash);
                if (mi != pwallet->mapWallet.end())
                {
                    tx = (*mi).second;
                    BOOST_FOREACH(const CMerkleTx& txWalletPrev, (*mi).second.vtxPrev)
                        mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;
                } else
                if (mapWalletPrev.count(hash))
                {
                    tx = *mapWalletPrev[hash];
                } else
                if (txdb.ReadDiskTx(hash, tx))
                {
                    ;
                } else
                {
                    LogPrintf("ERROR: AddSupportingTransactions() : unsupported transaction\n");
                    continue;
                };

                int nDepth = tx.SetMerkleBranch();
                vtxPrev.push_back(tx);

                if (nDepth < COPY_DEPTH)
                {
                    BOOST_FOREACH(const CTxIn& txin, tx.vin)
                        vWorkQueue.push_back(txin.prevout.hash);
                };
            }
        }
    }

    reverse(vtxPrev.begin(), vtxPrev.end());
}

bool CWalletTx::WriteToDisk()
{
    return CWalletDB(pwallet->strWalletFile).WriteTx(GetHash(), *this);
}

// Scan the block chain (starting in pindexStart) for transactions
// from or to us. If fUpdate is true, found transactions that already
// exist in the wallet will be updated.
int CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate)
{
    if (fDebug)
        LogPrintf("ScanForWalletTransactions()\n");
    
    if (nNodeMode != NT_FULL)
    {
        LogPrintf("Error: CWallet::ScanForWalletTransactions() must be run in full mode.\n");
        return 0;
    };

    int ret = 0;

    CBlockIndex* pindex = pindexStart;
    {
        LOCK2(cs_main, cs_wallet);
        while (pindex)
        {
            // no need to read and scan block, if block was created before
            // our wallet birthday (as adjusted for block time variability)
            if (nTimeFirstKey && (pindex->nTime < (nTimeFirstKey - 7200)))
            {
                pindex = pindex->pnext;
                continue;
            };

            CBlock block;
            block.ReadFromDisk(pindex, true);
            BOOST_FOREACH(CTransaction& tx, block.vtx)
            {
                uint256 hash = tx.GetHash();
                if (AddToWalletIfInvolvingMe(tx, hash, &block, fUpdate))
                    ret++;
            };
            pindex = pindex->pnext;
        };
    }
    return ret;
}

void CWallet::ReacceptWalletTransactions()
{
    if (fDebug)
        LogPrintf("ReacceptWalletTransactions()\n");

    CTxDB txdb("r");

    if (nNodeMode == NT_THIN)
    {
        for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            CWalletTx& wtx = (*it).second;

            if ((wtx.IsCoinBase() && wtx.IsSpent(0))
                || (wtx.IsCoinStake() && wtx.IsSpent(1)))
                continue;

            std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(wtx.hashBlock);
            if (mi == mapBlockThinIndex.end())
            {
                if (!fThinFullIndex)
                {
                    CDiskBlockThinIndex diskindex;
                    if (txdb.ReadBlockThinIndex(wtx.hashBlock, diskindex)
                        && diskindex.hashNext != 0)
                        continue; // block is in db and in main chain
                };

                // Re-accept any txes of ours that aren't already in a block
                if (!(wtx.IsCoinBase() || wtx.IsCoinStake()))
                    wtx.AcceptWalletTransaction(txdb);

                continue;
            };
        };
        return;
    };


    bool fRepeat = true;
    while (fRepeat)
    {
        LOCK2(cs_main, cs_wallet);

        fRepeat = false;
        vector<CDiskTxPos> vMissingTx;
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            if ((wtx.IsCoinBase() && wtx.IsSpent(0))
                || (wtx.IsCoinStake() && wtx.IsSpent(1)))
                continue;

            CTxIndex txindex;
            bool fUpdated = false;
            if (txdb.ReadTxIndex(wtx.GetHash(), txindex))
            {
                // Update fSpent if a tx got spent somewhere else by a copy of wallet.dat
                if (txindex.vSpent.size() != wtx.vout.size())
                {
                    LogPrintf("ERROR: ReacceptWalletTransactions() : txindex.vSpent.size() %u != wtx.vout.size() %u\n", txindex.vSpent.size(), wtx.vout.size());
                    continue;
                };

                for (unsigned int i = 0; i < txindex.vSpent.size(); i++)
                {
                    if (wtx.IsSpent(i))
                        continue;

                    if (!txindex.vSpent[i].IsNull() && IsMine(wtx.vout[i]))
                    {
                        wtx.MarkSpent(i);
                        fUpdated = true;
                        vMissingTx.push_back(txindex.vSpent[i]);
                    };
                };

                if (fUpdated)
                {
                    LogPrintf("ReacceptWalletTransactions found spent coin %s GAMEUNITS %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkDirty();
                    wtx.WriteToDisk();
                };
            } else
            {
                // Re-accept any txes of ours that aren't already in a block
                if (!(wtx.IsCoinBase() || wtx.IsCoinStake()))
                    wtx.AcceptWalletTransaction(txdb);
            };
        };

        if (!vMissingTx.empty())
        {
            // TODO: optimize this to scan just part of the block chain?
            if (ScanForWalletTransactions(pindexGenesisBlock))
                fRepeat = true;  // Found missing transactions: re-do re-accept.
        };
    };
}

void CWalletTx::RelayWalletTransaction(CTxDB& txdb)
{
    BOOST_FOREACH(const CMerkleTx& tx, vtxPrev)
    {
        if (!(tx.IsCoinBase() || tx.IsCoinStake()))
        {
            uint256 hash = tx.GetHash();
            if (!txdb.ContainsTx(hash))
                RelayTransaction((CTransaction)tx, hash);
        };
    };

    if (!(IsCoinBase() || IsCoinStake()))
    {
        uint256 hash = GetHash();
        if (!txdb.ContainsTx(hash))
        {
            LogPrintf("Relaying wtx %s\n", hash.ToString().substr(0,10).c_str());
            RelayTransaction((CTransaction)*this, hash);
        };
    };
}

void CWalletTx::RelayWalletTransaction()
{
   CTxDB txdb("r");
   RelayWalletTransaction(txdb);
}

void CWallet::ResendWalletTransactions(bool fForce)
{
    if (!fForce)
    {
        // Do this infrequently and randomly to avoid giving away
        // that these are our transactions.
        static int64_t nNextTime = 0;
        if (GetTime() < nNextTime)
            return;
        bool fFirst = (nNextTime == 0);
        nNextTime = GetTime() + GetRand(30 * 60);
        if (fFirst)
            return;

        // Only do it if there's been a new block since last time
        static int64_t nLastTime;
        if (nTimeBestReceived < nLastTime)
            return;
        nLastTime = GetTime();
    };

    // Rebroadcast any of our txes that aren't in a block yet
    LogPrintf("ResendWalletTransactions()\n");
    CTxDB txdb("r");
    
    multimap<unsigned int, CWalletTx*> mapSorted;
    {
        LOCK(cs_wallet);
        // Sort them in chronological order
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            // Don't rebroadcast until it's had plenty of time that
            // it should have gotten in already by now.
            if (fForce || nTimeBestReceived - (int64_t)wtx.nTimeReceived > 5 * 60)
                mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
        };
    } // cs_wallet

    BOOST_FOREACH(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
    {
        CWalletTx& wtx = *item.second;
        if (wtx.CheckTransaction())
            wtx.RelayWalletTransaction(txdb);
        else
            LogPrintf("ResendWalletTransactions() : CheckTransaction failed for transaction %s\n", wtx.GetHash().ToString().c_str());
    };
    
}






//////////////////////////////////////////////////////////////////////////////
//
// Actions
//


int64_t CWallet::GetBalance() const
{
    int64_t nTotal = 0;

    {
        LOCK2(cs_main, cs_wallet);
        for (std::map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsTrusted())
                nTotal += pcoin->GetAvailableCredit();
        };
    }

    return nTotal;
}

int64_t CWallet::GetGameunitsXBalance() const
{
    int64_t nTotal = 0;

    {
        LOCK2(cs_main, cs_wallet);
        for (std::map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsTrusted() && pcoin->nVersion == ANON_TXN_VERSION)
                nTotal += pcoin->GetAvailableGameunitsXCredit();
        };
    }

    return nTotal;
};


int64_t CWallet::GetUnconfirmedBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0))
                nTotal += pcoin->GetAvailableCredit();
        };
    }
    return nTotal;
}

int64_t CWallet::GetImmatureBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx& pcoin = (*it).second;
            if (pcoin.IsCoinBase() && pcoin.GetBlocksToMaturity() > 0 && pcoin.IsInMainChain())
                nTotal += GetCredit(pcoin);
        }
    }
    return nTotal;
}

// populate vCoins with vector of spendable COutputs
void CWallet::AvailableCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl) const
{
    vCoins.clear();

    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            if (!pcoin->IsFinal())
                continue;

            if (fOnlyConfirmed && !pcoin->IsTrusted())
                continue;

            if ((pcoin->IsCoinBase()||pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 0)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
                if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue >= nMinimumInputValue &&
                (!coinControl || !coinControl->HasSelected() || coinControl->IsSelected((*it).first, i)))
                    vCoins.push_back(COutput(pcoin, i, nDepth));

        }
    }
}

void CWallet::AvailableCoinsForStaking(vector<COutput>& vCoins, unsigned int nSpendTime) const
{
    vCoins.clear();

    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            // Filtering by tx timestamp instead of block timestamp may give false positives but never false negatives
            if (pcoin->nTime + nStakeMinAge > nSpendTime)
                continue;

            if (pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 1)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->nVersion == ANON_TXN_VERSION
                    && pcoin->vout[i].IsAnonOutput())
                    continue;
                if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue >= nMinimumInputValue)
                    vCoins.push_back(COutput(pcoin, i, nDepth));
            };
        };
    }
}

static void ApproximateBestSubset(vector<pair<int64_t, pair<const CWalletTx*,unsigned int> > >vValue, int64_t nTotalLower, int64_t nTargetValue,
                                  vector<char>& vfBest, int64_t& nBest, int iterations = 1000)
{
    vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        int64_t nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                if (nPass == 0 ? rand() % 2 : !vfIncluded[i])
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

// ppcoin: total coins staked (non-spendable until maturity)
int64_t CWallet::GetStake() const
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
            nTotal += CWallet::GetCredit(*pcoin);
    }
    return nTotal;
}

int64_t CWallet::GetNewMint() const
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
            nTotal += CWallet::GetCredit(*pcoin);
    }
    return nTotal;
}

bool CWallet::SelectCoinsMinConf(int64_t nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, vector<COutput> vCoins, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    pair<int64_t, pair<const CWalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<int64_t>::max();
    coinLowestLarger.second.first = NULL;
    vector<pair<int64_t, pair<const CWalletTx*,unsigned int> > > vValue;
    int64_t nTotalLower = 0;

    random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

    BOOST_FOREACH(COutput output, vCoins)
    {
        const CWalletTx *pcoin = output.tx;

        if (output.nDepth < (pcoin->IsFromMe() ? nConfMine : nConfTheirs))
            continue;

        int i = output.i;

        // Follow the timestamp rules
        if (pcoin->nTime > nSpendTime)
            continue;

        int64_t n = pcoin->vout[i].nValue;

        pair<int64_t,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n == nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            return true;
        } else
        if (n < nTargetValue + CENT)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        } else
        if (n < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }

    // Solve subset sum by stochastic approximation
    sort(vValue.rbegin(), vValue.rend(), CompareValueOnly());
    vector<char> vfBest;
    int64_t nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest, 1000);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + CENT)
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + CENT, vfBest, nBest, 1000);

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (coinLowestLarger.second.first &&
        ((nBest != nTargetValue && nBest < nTargetValue + CENT) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    } else
    {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        if (fDebug && GetBoolArg("-printpriority"))
        {
            //// debug print
            LogPrintf("SelectCoins() best subset: ");
            for (unsigned int i = 0; i < vValue.size(); i++)
                if (vfBest[i])
                    LogPrintf("%s ", FormatMoney(vValue[i].first).c_str());
            LogPrintf("total %s\n", FormatMoney(nBest).c_str());
        }
    }

    return true;
}

bool CWallet::SelectCoins(int64_t nTargetValue, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, const CCoinControl* coinControl) const
{
    vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coinControl && coinControl->HasSelected())
    {
        BOOST_FOREACH(const COutput& out, vCoins)
        {
            nValueRet += out.tx->vout[out.i].nValue;
            setCoinsRet.insert(make_pair(out.tx, out.i));
        };

        return (nValueRet >= nTargetValue);
    };

    return (SelectCoinsMinConf(nTargetValue, nSpendTime, 1, 10, vCoins, setCoinsRet, nValueRet) ||
            SelectCoinsMinConf(nTargetValue, nSpendTime, 1, 1, vCoins, setCoinsRet, nValueRet) ||
            SelectCoinsMinConf(nTargetValue, nSpendTime, 0, 1, vCoins, setCoinsRet, nValueRet));
}

// Select some coins without random shuffle or best subset approximation
bool CWallet::SelectCoinsForStaking(int64_t nTargetValue, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    vector<COutput> vCoins;
    AvailableCoinsForStaking(vCoins, nSpendTime);

    setCoinsRet.clear();
    nValueRet = 0;

    BOOST_FOREACH(COutput output, vCoins)
    {
        const CWalletTx *pcoin = output.tx;
        int i = output.i;

        // Stop if we've chosen enough inputs
        if (nValueRet >= nTargetValue)
            break;

        int64_t n = pcoin->vout[i].nValue;

        pair<int64_t, pair<const CWalletTx*, unsigned int> > coin = make_pair(n, make_pair(pcoin, i));

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
    }

    return true;
}


bool CWallet::CreateTransaction(const vector<pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, int32_t& nChangePos, const CCoinControl* coinControl)
{
    int64_t nValue = 0;
    BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
    {
        if (nValue < 0)
            return false;
        nValue += s.second;
    };

    if (vecSend.empty() || nValue < 0)
        return false;

    wtxNew.BindWallet(this);

    {
        LOCK2(cs_main, cs_wallet);
        // txdb must be opened before the mapWallet lock
        CTxDB txdb("r");
        {
            nFeeRet = nTransactionFee;
            while (true)
            {
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;

                int64_t nTotalValue = nValue + nFeeRet;
                double dPriority = 0;
                // vouts to the payees
                BOOST_FOREACH(const PAIRTYPE(CScript, int64_t)& s, vecSend)
                {
                    wtxNew.vout.push_back(CTxOut(s.second, s.first));
                };

                // Choose coins to use
                set<pair<const CWalletTx*,unsigned int> > setCoins;
                int64_t nValueIn = 0;
                if (!SelectCoins(nTotalValue, wtxNew.nTime, setCoins, nValueIn, coinControl))
                    return false;

                BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
                {
                    int64_t nCredit = pcoin.first->vout[pcoin.second].nValue;
                    dPriority += (double)nCredit * pcoin.first->GetDepthInMainChain();
                };

                int64_t nChange = nValueIn - nValue - nFeeRet;
                // if sub-cent change is required, the fee must be raised to at least MIN_TX_FEE
                // or until nChange becomes zero
                // NOTE: this depends on the exact behaviour of GetMinFee
                if (nFeeRet < MIN_TX_FEE && nChange > 0 && nChange < CENT)
                {
                    int64_t nMoveToFee = min(nChange, MIN_TX_FEE - nFeeRet);
                    nChange -= nMoveToFee;
                    nFeeRet += nMoveToFee;
                };

                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-bitcoin-address
                    CScript scriptChange;

                    // coin control: send change to custom address
                    if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                    {
                        scriptChange.SetDestination(coinControl->destChange);
                    } else
                    {
                        // no coin control: send change to newly generated address

                        // Note: We use a new key here to keep it from being obvious which side is the change.
                        //  The drawback is that by not reusing a previous key, the change may be lost if a
                        //  backup is restored, if the backup doesn't have the new private key for the change.
                        //  If we reused the old key, it would be possible to add code to look for and
                        //  rediscover unknown transactions that were written with keys of ours to recover
                        //  post-backup change.

                        // Reserve a new key pair from key pool
                        CPubKey vchPubKey;
                        assert(reservekey.GetReservedKey(vchPubKey)); // should never fail, as we just unlocked

                        scriptChange.SetDestination(vchPubKey.GetID());
                    };

                    // Insert change txn at random position:
                    vector<CTxOut>::iterator position = wtxNew.vout.begin()+GetRandInt(wtxNew.vout.size() + 1);

                    // -- don't put change output between value and narration outputs
                    if (position > wtxNew.vout.begin() && position < wtxNew.vout.end())
                    {
                        while (position > wtxNew.vout.begin())
                        {
                            if (position->nValue != 0)
                                break;
                            position--;
                        };
                    };
                    wtxNew.vout.insert(position, CTxOut(nChange, scriptChange));
                    nChangePos = std::distance(wtxNew.vout.begin(), position);
                } else
                {
                    reservekey.ReturnKey();
                };

                // Fill vin
                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    wtxNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second));

                // Sign
                int nIn = 0;
                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    if (!SignSignature(*this, *coin.first, wtxNew, nIn++))
                        return false;

                // Limit size
                unsigned int nBytes = ::GetSerializeSize(*(CTransaction*)&wtxNew, SER_NETWORK, PROTOCOL_VERSION);
                if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
                    return false;

                dPriority /= nBytes;

                // Check that enough fee is included
                int64_t nPayFee = nTransactionFee * (1 + (int64_t)nBytes / 1000);
                int64_t nMinFee = wtxNew.GetMinFee(1, GMF_SEND, nBytes);

                if (nFeeRet < max(nPayFee, nMinFee))
                {
                    nFeeRet = max(nPayFee, nMinFee);
                    continue;
                };

                // Fill vtxPrev by copying from previous transactions vtxPrev
                wtxNew.AddSupportingTransactions(txdb);
                wtxNew.fTimeReceivedIsTxTime = true;

                break;
            };
        }
    }
    return true;
}




bool CWallet::CreateTransaction(CScript scriptPubKey, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, const CCoinControl* coinControl)
{
    vector< pair<CScript, int64_t> > vecSend;
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    if (sNarr.length() > 0)
    {
        std::vector<uint8_t> vNarr(sNarr.c_str(), sNarr.c_str() + sNarr.length());
        std::vector<uint8_t> vNDesc;

        vNDesc.resize(2);
        vNDesc[0] = 'n';
        vNDesc[1] = 'p';

        CScript scriptN = CScript() << OP_RETURN << vNDesc << OP_RETURN << vNarr;

        vecSend.push_back(make_pair(scriptN, 0));
    };

    // -- CreateTransaction won't place change between value and narr output.
    //    narration output will be for preceding output

    int nChangePos;
    bool rv = CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet, nChangePos, coinControl);

    // -- narration will be added to mapValue later in FindStealthTransactions From CommitTransaction
    return rv;
}



bool CWallet::NewStealthAddress(std::string& sError, std::string& sLabel, CStealthAddress& sxAddr)
{
    ec_secret scan_secret;
    ec_secret spend_secret;

    if (GenerateRandomSecret(scan_secret) != 0
        || GenerateRandomSecret(spend_secret) != 0)
    {
        sError = "GenerateRandomSecret failed.";
        LogPrintf("Error CWallet::NewStealthAddress - %s\n", sError.c_str());
        return false;
    };

    ec_point scan_pubkey, spend_pubkey;
    if (SecretToPublicKey(scan_secret, scan_pubkey) != 0)
    {
        sError = "Could not get scan public key.";
        LogPrintf("Error CWallet::NewStealthAddress - %s\n", sError.c_str());
        return false;
    };
    
    if (SecretToPublicKey(spend_secret, spend_pubkey) != 0)
    {
        sError = "Could not get spend public key.";
        LogPrintf("Error CWallet::NewStealthAddress - %s\n", sError.c_str());
        return false;
    };
    
    if (fDebug)
    {
        LogPrintf("getnewstealthaddress: ");
        LogPrintf("scan_pubkey ");
        for (uint32_t i = 0; i < scan_pubkey.size(); ++i)
            LogPrintf("%02x", scan_pubkey[i]);
        LogPrintf("\n");
        
        LogPrintf("spend_pubkey ");
        for (uint32_t i = 0; i < spend_pubkey.size(); ++i)
            LogPrintf("%02x", spend_pubkey[i]);
        LogPrintf("\n");
    };


    sxAddr.label = sLabel;
    sxAddr.scan_pubkey = scan_pubkey;
    sxAddr.spend_pubkey = spend_pubkey;

    sxAddr.scan_secret.resize(32);
    memcpy(&sxAddr.scan_secret[0], &scan_secret.e[0], 32);
    sxAddr.spend_secret.resize(32);
    memcpy(&sxAddr.spend_secret[0], &spend_secret.e[0], 32);

    return true;
}

bool CWallet::AddStealthAddress(CStealthAddress& sxAddr)
{
    LOCK(cs_wallet);

    // must add before changing spend_secret
    stealthAddresses.insert(sxAddr);

    bool fOwned = sxAddr.scan_secret.size() == ec_secret_size;



    if (fOwned)
    {
        // -- owned addresses can only be added when wallet is unlocked
        if (IsLocked())
        {
            LogPrintf("Error: CWallet::AddStealthAddress wallet must be unlocked.\n");
            stealthAddresses.erase(sxAddr);
            return false;
        };

        if (IsCrypted())
        {
            std::vector<unsigned char> vchCryptedSecret;
            CSecret vchSecret;
            vchSecret.resize(32);
            memcpy(&vchSecret[0], &sxAddr.spend_secret[0], 32);

            uint256 iv = Hash(sxAddr.spend_pubkey.begin(), sxAddr.spend_pubkey.end());
            if (!EncryptSecret(vMasterKey, vchSecret, iv, vchCryptedSecret))
            {
                LogPrintf("Error: Failed encrypting stealth key %s\n", sxAddr.Encoded().c_str());
                stealthAddresses.erase(sxAddr);
                return false;
            };
            sxAddr.spend_secret = vchCryptedSecret;
        };
    };


    bool rv = CWalletDB(strWalletFile).WriteStealthAddress(sxAddr);

    if (rv)
        NotifyAddressBookChanged(this, sxAddr, sxAddr.label, fOwned, CT_NEW);

    return rv;
}

bool CWallet::UnlockStealthAddresses(const CKeyingMaterial& vMasterKeyIn)
{
    // -- decrypt spend_secret of stealth addresses
    std::set<CStealthAddress>::iterator it;
    for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
    {
        if (it->scan_secret.size() < 32)
            continue; // stealth address is not owned

        // -- CStealthAddress are only sorted on spend_pubkey
        CStealthAddress &sxAddr = const_cast<CStealthAddress&>(*it);

        if (fDebug)
            LogPrintf("Decrypting stealth key %s\n", sxAddr.Encoded().c_str());

        CSecret vchSecret;
        uint256 iv = Hash(sxAddr.spend_pubkey.begin(), sxAddr.spend_pubkey.end());
        if(!DecryptSecret(vMasterKeyIn, sxAddr.spend_secret, iv, vchSecret)
            || vchSecret.size() != 32)
        {
            LogPrintf("Error: Failed decrypting stealth key %s\n", sxAddr.Encoded().c_str());
            continue;
        };

        ec_secret testSecret;
        memcpy(&testSecret.e[0], &vchSecret[0], 32);
        ec_point pkSpendTest;

        if (SecretToPublicKey(testSecret, pkSpendTest) != 0
            || pkSpendTest != sxAddr.spend_pubkey)
        {
            LogPrintf("Error: Failed decrypting stealth key, public key mismatch %s\n", sxAddr.Encoded().c_str());
            continue;
        };

        sxAddr.spend_secret.resize(32);
        memcpy(&sxAddr.spend_secret[0], &vchSecret[0], 32);
    };

    CryptedKeyMap::iterator mi = mapCryptedKeys.begin();
    for (; mi != mapCryptedKeys.end(); ++mi)
    {
        CPubKey &pubKey = (*mi).second.first;
        std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
        if (vchCryptedSecret.size() != 0)
            continue;

        CKeyID ckid = pubKey.GetID();
        CBitcoinAddress addr(ckid);

        StealthKeyMetaMap::iterator mi = mapStealthKeyMeta.find(ckid);
        if (mi == mapStealthKeyMeta.end())
        {
            // -- could be an anon output
            if (fDebug)
                LogPrintf("Warning: No metadata found to add secret for %s\n", addr.ToString().c_str());
            continue;
        };

        CStealthKeyMetadata& sxKeyMeta = mi->second;

        CStealthAddress sxFind;
        sxFind.SetScanPubKey(sxKeyMeta.pkScan);

        std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxFind);
        if (si == stealthAddresses.end())
        {
            LogPrintf("No stealth key found to add secret for %s\n", addr.ToString().c_str());
            continue;
        };

        if (fDebug)
            LogPrintf("Expanding secret for %s\n", addr.ToString().c_str());

        ec_secret sSpendR;
        ec_secret sSpend;
        ec_secret sScan;

        if (si->spend_secret.size() != ec_secret_size
            || si->scan_secret.size() != ec_secret_size)
        {
            LogPrintf("Stealth address has no secret key for %s\n", addr.ToString().c_str());
            continue;
        };
        memcpy(&sScan.e[0], &si->scan_secret[0], ec_secret_size);
        memcpy(&sSpend.e[0], &si->spend_secret[0], ec_secret_size);

        ec_point pkEphem;;
        pkEphem.resize(sxKeyMeta.pkEphem.size());
        memcpy(&pkEphem[0], sxKeyMeta.pkEphem.begin(), sxKeyMeta.pkEphem.size());
        
        if (StealthSecretSpend(sScan, pkEphem, sSpend, sSpendR) != 0)
        {
            LogPrintf("StealthSecretSpend() failed.\n");
            continue;
        };

        ec_point pkTestSpendR;
        if (SecretToPublicKey(sSpendR, pkTestSpendR) != 0)
        {
            LogPrintf("SecretToPublicKey() failed.\n");
            continue;
        };
        
        CKey ckey;
        ckey.Set(&sSpendR.e[0], true);
        
        if (!ckey.IsValid())
        {
            LogPrintf("Reconstructed key is invalid.\n");
            continue;
        };
        
        CPubKey cpkT = ckey.GetPubKey(true);
        
        if (!cpkT.IsValid())
        {
            LogPrintf("cpkT is invalid.\n");
            continue;
        };

        if (cpkT != pubKey)
        {
            LogPrintf("Error: Generated secret does not match.\n");
            if (fDebug)
            {
                LogPrintf("cpkT   %s\n", HexStr(cpkT).c_str());
                LogPrintf("pubKey %s\n", HexStr(pubKey).c_str());
            };
            continue;
        };
        
        if (fDebug)
        {
            CKeyID keyID = cpkT.GetID();
            CBitcoinAddress coinAddress(keyID);
            LogPrintf("Adding secret to key %s.\n", coinAddress.ToString().c_str());
        };

        if (!AddKeyPubKey(ckey, cpkT))
        {
            LogPrintf("AddKeyPubKey failed.\n");
            continue;
        };

        if (!CWalletDB(strWalletFile).EraseStealthKeyMeta(ckid))
            LogPrintf("EraseStealthKeyMeta failed for %s\n", addr.ToString().c_str());
    };
    return true;
}

bool CWallet::UpdateStealthAddress(std::string &addr, std::string &label, bool addIfNotExist)
{
    if (fDebug)
        LogPrintf("UpdateStealthAddress %s\n", addr.c_str());


    CStealthAddress sxAddr;

    if (!sxAddr.SetEncoded(addr))
        return false;

    std::set<CStealthAddress>::iterator it;
    it = stealthAddresses.find(sxAddr);

    ChangeType nMode = CT_UPDATED;
    CStealthAddress sxFound;
    if (it == stealthAddresses.end())
    {
        if (addIfNotExist)
        {
            sxFound = sxAddr;
            sxFound.label = label;
            stealthAddresses.insert(sxFound);
            nMode = CT_NEW;
        } else
        {
            LogPrintf("UpdateStealthAddress %s, not in set\n", addr.c_str());
            return false;
        };
    } else
    {
        sxFound = const_cast<CStealthAddress&>(*it);

        if (sxFound.label == label)
        {
            // no change
            return true;
        };

        it->label = label; // update in .stealthAddresses

        if (sxFound.scan_secret.size() == ec_secret_size)
        {
            // -- read from db to keep encryption
            CStealthAddress sxOwned;

            if (!CWalletDB(strWalletFile).ReadStealthAddress(sxFound))
            {
                LogPrintf("UpdateStealthAddress: error - sxFound not in db.\n");
                return false;
            };
        };
    };

    sxFound.label = label;

    if (!CWalletDB(strWalletFile).WriteStealthAddress(sxFound))
    {
        LogPrintf("UpdateStealthAddress(%s) Write to db failed.\n", addr.c_str());
        return false;
    };

    bool fOwned = sxFound.scan_secret.size() == ec_secret_size;
    NotifyAddressBookChanged(this, sxFound, sxFound.label, fOwned, nMode);

    return true;
}

bool CWallet::CreateStealthTransaction(CScript scriptPubKey, int64_t nValue, std::vector<uint8_t>& P, std::vector<uint8_t>& narr, std::string& sNarr, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, const CCoinControl* coinControl)
{
    vector< pair<CScript, int64_t> > vecSend;
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    CScript scriptP = CScript() << OP_RETURN << P;
    if (narr.size() > 0)
        scriptP = scriptP << OP_RETURN << narr;

    vecSend.push_back(make_pair(scriptP, 0));

    // -- shuffle inputs, change output won't mix enough as it must be not fully random for plantext narrations
    std::random_shuffle(vecSend.begin(), vecSend.end());

    int nChangePos;
    bool rv = CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet, nChangePos, coinControl);

    // -- the change txn is inserted in a random pos, check here to match narr to output
    if (rv && narr.size() > 0)
    {
        for (unsigned int k = 0; k < wtxNew.vout.size(); ++k)
        {
            if (wtxNew.vout[k].scriptPubKey != scriptPubKey
                || wtxNew.vout[k].nValue != nValue)
                continue;

            char key[64];
            if (snprintf(key, sizeof(key), "n_%u", k) < 1)
            {
                LogPrintf("CreateStealthTransaction(): Error creating narration key.");
                break;
            };
            wtxNew.mapValue[key] = sNarr;
            break;
        };
    };

    return rv;
}

string CWallet::SendStealthMoney(CScript scriptPubKey, int64_t nValue, std::vector<uint8_t>& P, std::vector<uint8_t>& narr, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee)
{
    CReserveKey reservekey(this);
    int64_t nFeeRequired;

    if (IsLocked())
    {
        string strError = _("Error: Wallet locked, unable to create transaction  ");
        LogPrintf("SendStealthMoney() : %s", strError.c_str());
        return strError;
    };

    if (fWalletUnlockStakingOnly)
    {
        string strError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        LogPrintf("SendStealthMoney() : %s", strError.c_str());
        return strError;
    };

    if (!CreateStealthTransaction(scriptPubKey, nValue, P, narr, sNarr, wtxNew, reservekey, nFeeRequired))
    {
        string strError;
        if (nValue + nFeeRequired > GetBalance())
            strError = strprintf(_("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds  "), FormatMoney(nFeeRequired).c_str());
        else
            strError = _("Error: Transaction creation failed  ");
        LogPrintf("SendStealthMoney() : %s", strError.c_str());
        return strError;
    };

    if (fAskFee && !uiInterface.ThreadSafeAskFee(nFeeRequired, _("Sending...")))
        return "ABORTED";

    if (!CommitTransaction(wtxNew, reservekey))
        return _("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    return "";
}

bool CWallet::SendStealthMoneyToDestination(CStealthAddress& sxAddress, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee)
{
    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };
    if (nValue + nTransactionFee > GetBalance())
    {
        sError = "Insufficient funds";
        return false;
    };
    
    ec_secret ephem_secret;
    ec_secret secretShared;
    ec_point pkSendTo;
    ec_point ephem_pubkey;

    if (GenerateRandomSecret(ephem_secret) != 0)
    {
        sError = "GenerateRandomSecret failed.";
        return false;
    };

    if (StealthSecret(ephem_secret, sxAddress.scan_pubkey, sxAddress.spend_pubkey, secretShared, pkSendTo) != 0)
    {
        sError = "Could not generate receiving public key.";
        return false;
    };

    CPubKey cpkTo(pkSendTo);
    if (!cpkTo.IsValid())
    {
        sError = "Invalid public key generated.";
        return false;
    };

    CKeyID ckidTo = cpkTo.GetID();

    CBitcoinAddress addrTo(ckidTo);

    if (SecretToPublicKey(ephem_secret, ephem_pubkey) != 0)
    {
        sError = "Could not generate ephem public key.";
        return false;
    };

    if (fDebug)
    {
        LogPrintf("Stealth send to generated pubkey %u: %s\n", pkSendTo.size(), HexStr(pkSendTo).c_str());
        LogPrintf("hash %s\n", addrTo.ToString().c_str());
        LogPrintf("ephem_pubkey %u: %s\n", ephem_pubkey.size(), HexStr(ephem_pubkey).c_str());
    };

    std::vector<unsigned char> vchNarr;
    if (sNarr.length() > 0)
    {
        SecMsgCrypter crypter;
        crypter.SetKey(&secretShared.e[0], &ephem_pubkey[0]);

        if (!crypter.Encrypt((uint8_t*)&sNarr[0], sNarr.length(), vchNarr))
        {
            sError = "Narration encryption failed.";
            return false;
        };

        if (vchNarr.size() > MAX_STEALTH_NARRATION_SIZE)
        {
            sError = "Encrypted narration is too long.";
            return false;
        };
    };

    // -- Parse Bitcoin address
    CScript scriptPubKey;
    scriptPubKey.SetDestination(addrTo.Get());

    if ((sError = SendStealthMoney(scriptPubKey, nValue, ephem_pubkey, vchNarr, sNarr, wtxNew, fAskFee)) != "")
        return false;


    return true;
}

bool CWallet::FindStealthTransactions(const CTransaction& tx, mapValue_t& mapNarr)
{
    if (fDebug)
        LogPrintf("FindStealthTransactions() tx: %s\n", tx.GetHash().GetHex().c_str());

    mapNarr.clear();

    LOCK(cs_wallet);
    ec_secret sSpendR;
    ec_secret sSpend;
    ec_secret sScan;
    ec_secret sShared;

    ec_point pkExtracted;

    std::vector<uint8_t> vchEphemPK;
    std::vector<uint8_t> vchDataB;
    std::vector<uint8_t> vchENarr;
    opcodetype opCode;
    char cbuf[256];

    int32_t nOutputIdOuter = -1;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        nOutputIdOuter++;
        // -- for each OP_RETURN need to check all other valid outputs

        // -- skip scan anon outputs
        if (tx.nVersion == ANON_TXN_VERSION
            && txout.IsAnonOutput())
            continue;
        
        CScript::const_iterator itTxA = txout.scriptPubKey.begin();

        if (!txout.scriptPubKey.GetOp(itTxA, opCode, vchEphemPK)
            || opCode != OP_RETURN)
            continue;
        else
        if (!txout.scriptPubKey.GetOp(itTxA, opCode, vchEphemPK)
            || vchEphemPK.size() != 33)
        {
            // -- look for plaintext narrations
            if (vchEphemPK.size() > 1
                && vchEphemPK[0] == 'n'
                && vchEphemPK[1] == 'p')
            {
                if (txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && opCode == OP_RETURN
                    && txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && vchENarr.size() > 0)
                {
                    std::string sNarr = std::string(vchENarr.begin(), vchENarr.end());

                    snprintf(cbuf, sizeof(cbuf), "n_%d", nOutputIdOuter-1); // plaintext narration always matches preceding value output
                    mapNarr[cbuf] = sNarr;
                } else
                {
                    LogPrintf("Warning: FindStealthTransactions() tx: %s, Could not extract plaintext narration.\n", tx.GetHash().GetHex().c_str());
                };
            };
            continue;
        };

        int32_t nOutputId = -1;
        nStealth++;
        BOOST_FOREACH(const CTxOut& txoutB, tx.vout)
        {
            nOutputId++;

            // -- skip anon outputs
            if (tx.nVersion == ANON_TXN_VERSION
                && txout.IsAnonOutput())
                continue;

            if (&txoutB == &txout)
                continue;

            bool txnMatch = false; // only 1 txn will match an ephem pk

            CTxDestination address;
            if (!ExtractDestination(txoutB.scriptPubKey, address))
                continue;

            if (address.type() != typeid(CKeyID))
                continue;

            CKeyID ckidMatch = boost::get<CKeyID>(address);

            if (HaveKey(ckidMatch)) // no point checking if already have key
                continue;

            std::set<CStealthAddress>::iterator it;
            for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
            {
                if (it->scan_secret.size() != ec_secret_size)
                    continue; // stealth address is not owned

                memcpy(&sScan.e[0], &it->scan_secret[0], ec_secret_size);

                if (StealthSecret(sScan, vchEphemPK, it->spend_pubkey, sShared, pkExtracted) != 0)
                {
                    LogPrintf("StealthSecret failed.\n");
                    continue;
                };

                CPubKey cpkE(pkExtracted);

                if (!cpkE.IsValid())
                    continue;
                CKeyID ckidE = cpkE.GetID();

                if (ckidMatch != ckidE)
                    continue;

                if (fDebug)
                    LogPrintf("Found stealth txn to address %s\n", it->Encoded().c_str());

                if (IsLocked())
                {
                    if (fDebug)
                        LogPrintf("Wallet locked, adding key without secret.\n");

                    // -- add key without secret
                    std::vector<uint8_t> vchEmpty;
                    AddCryptedKey(cpkE, vchEmpty);
                    CKeyID keyId = cpkE.GetID();
                    CBitcoinAddress coinAddress(keyId);
                    std::string sLabel = it->Encoded();
                    SetAddressBookName(keyId, sLabel);

                    CPubKey cpkEphem(vchEphemPK);
                    CPubKey cpkScan(it->scan_pubkey);
                    CStealthKeyMetadata lockedSkMeta(cpkEphem, cpkScan);

                    if (!CWalletDB(strWalletFile).WriteStealthKeyMeta(keyId, lockedSkMeta))
                        LogPrintf("WriteStealthKeyMeta failed for %s\n", coinAddress.ToString().c_str());

                    mapStealthKeyMeta[keyId] = lockedSkMeta;
                    nFoundStealth++;
                } else
                {
                    if (it->spend_secret.size() != ec_secret_size)
                        continue;
                    memcpy(&sSpend.e[0], &it->spend_secret[0], ec_secret_size);


                    if (StealthSharedToSecretSpend(sShared, sSpend, sSpendR) != 0)
                    {
                        LogPrintf("StealthSharedToSecretSpend() failed.\n");
                        continue;
                    };

                    ec_point pkTestSpendR;
                    if (SecretToPublicKey(sSpendR, pkTestSpendR) != 0)
                    {
                        LogPrintf("SecretToPublicKey() failed.\n");
                        continue;
                    };

                    CKey ckey;
                    ckey.Set(&sSpendR.e[0], true);

                    if (!ckey.IsValid())
                    {
                        LogPrintf("Reconstructed key is invalid.\n");
                        continue;
                    };

                    CPubKey cpkT = ckey.GetPubKey();
                    if (!cpkT.IsValid())
                    {
                        LogPrintf("cpkT is invalid.\n");
                        continue;
                    };

                    CKeyID keyID = cpkT.GetID();
                    if (fDebug)
                    {
                        CBitcoinAddress coinAddress(keyID);
                        LogPrintf("Adding key %s.\n", coinAddress.ToString().c_str());
                    };

                    if (!AddKeyPubKey(ckey, cpkT))
                    {
                        LogPrintf("AddKeyPubKey failed.\n");
                        continue;
                    };

                    std::string sLabel = it->Encoded();
                    SetAddressBookName(keyID, sLabel);
                    nFoundStealth++;
                };

                if (txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && opCode == OP_RETURN
                    && txout.scriptPubKey.GetOp(itTxA, opCode, vchENarr)
                    && vchENarr.size() > 0)
                {
                    SecMsgCrypter crypter;
                    crypter.SetKey(&sShared.e[0], &vchEphemPK[0]);
                    std::vector<uint8_t> vchNarr;
                    if (!crypter.Decrypt(&vchENarr[0], vchENarr.size(), vchNarr))
                    {
                        LogPrintf("Decrypt narration failed.\n");
                        continue;
                    };
                    std::string sNarr = std::string(vchNarr.begin(), vchNarr.end());

                    snprintf(cbuf, sizeof(cbuf), "n_%d", nOutputId);
                    mapNarr[cbuf] = sNarr;
                };

                txnMatch = true;
                break;
            };
            if (txnMatch)
                break;
        };
    };

    return true;
};

static int GetBlockHeightFromHash(const uint256& blockHash)
{
    if (blockHash == 0)
        return 0;

    if (nNodeMode == NT_FULL)
    {
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(blockHash);
        if (mi == mapBlockIndex.end())
            return 0;
        return mi->second->nHeight;
    } else
    {
        std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(blockHash);
        if (mi == mapBlockThinIndex.end()
            && !fThinFullIndex
            && pindexRear)
        {
            CTxDB txdb("r");
            CDiskBlockThinIndex diskindex;
            if (txdb.ReadBlockThinIndex(blockHash, diskindex)
                || diskindex.hashNext != 0)
            {
                return diskindex.nHeight;
            };
        } else
        {
            return mi->second->nHeight;
        };
    };

    return 0;
}

bool CWallet::UpdateAnonTransaction(CTxDB* ptxdb, const CTransaction& tx, const uint256& blockHash)
{
    uint256 txnHash = tx.GetHash();
    if (fDebugRingSig)
        LogPrintf("UpdateAnonTransaction() tx: %s\n", txnHash.GetHex().c_str());
    
    // -- update txns not received in a block

    //LOCK2(cs_main, cs_wallet);

    int nNewHeight = GetBlockHeightFromHash(blockHash);
    
    CKeyImageSpent spentKeyImage;
    for (uint32_t i = 0; i < tx.vin.size(); ++i)
    {
        const CTxIn& txin = tx.vin[i];
        
        if (!txin.IsAnonInput())
            continue;
        
        const CScript &s = txin.scriptSig;
        
        std::vector<uint8_t> vchImage;
        txin.ExtractKeyImage(vchImage);
        
        // -- get nCoinValue by reading first ring element
        CPubKey pkRingCoin;
        CAnonOutput ao;
        CTxIndex txindex;
        const unsigned char* pPubkeys = &s[2];
        pkRingCoin = CPubKey(&pPubkeys[0 * ec_compressed_size], ec_compressed_size);
        if (!ptxdb->ReadAnonOutput(pkRingCoin, ao))
        {
            LogPrintf("UpdateAnonTransaction(): Error input %u AnonOutput %s not found.\n", i, HexStr(pkRingCoin).c_str());
            return false;
        };
        
        int64_t nCoinValue = ao.nValue;
        
        
        spentKeyImage.txnHash = txnHash;
        spentKeyImage.inputNo = i;
        spentKeyImage.nValue = nCoinValue;
        
        if (!ptxdb->WriteKeyImage(vchImage, spentKeyImage))
        {
            LogPrintf("UpdateAnonTransaction(): Error input %d WriteKeyImage failed %s .\n", i, HexStr(vchImage).c_str());
            return false;
        };
        
    };

    for (uint32_t i = 0; i < tx.vout.size(); ++i)
    {
        const CTxOut& txout = tx.vout[i];

        if (!txout.IsAnonOutput())
            continue;

        const CScript &s = txout.scriptPubKey;

        CPubKey pkCoin    = CPubKey(&s[2+1], ec_compressed_size);
        CAnonOutput ao;
        if (!ptxdb->ReadAnonOutput(pkCoin, ao))
        {
            LogPrintf("ReadAnonOutput %d failed.\n", i);
            return false;
        };

        ao.nBlockHeight = nNewHeight;

        if (!ptxdb->WriteAnonOutput(pkCoin, ao))
        {
            LogPrintf("ReadAnonOutput %d failed.\n", i);
            return false;
        };

        mapAnonOutputStats[ao.nValue].updateDepth(nNewHeight, ao.nValue);
    };
    
    return true;
};


bool CWallet::UndoAnonTransaction(const CTransaction& tx)
{
    if (fDebugRingSig)
        LogPrintf("UndoAnonTransaction() tx: %s\n", tx.GetHash().GetHex().c_str());
    // -- undo transaction - used if block is unlinked / txn didn't commit

    LOCK2(cs_main, cs_wallet);

    uint256 txnHash = tx.GetHash();

    CWalletDB walletdb(strWalletFile, "cr+");
    CTxDB txdb("cr+");

    for (unsigned int i = 0; i < tx.vin.size(); ++i)
    {
        const CTxIn& txin = tx.vin[i];

        if (!txin.IsAnonInput())
            continue;
        
        ec_point vchImage;
        txin.ExtractKeyImage(vchImage);

        CKeyImageSpent spentKeyImage;

        bool fInMempool;
        if (!GetKeyImage(&txdb, vchImage, spentKeyImage, fInMempool))
        {
            if (fDebugRingSig)
                LogPrintf("Error: keyImage for input %d not found.\n", i);
            continue;
        };

        // Possible?
        if (spentKeyImage.txnHash != txnHash)
        {
            LogPrintf("Error: spentKeyImage for %s does not match txn %s.\n", HexStr(vchImage).c_str(), txnHash.ToString().c_str());
            continue;
        };

        if (!txdb.EraseKeyImage(vchImage))
        {
            LogPrintf("EraseKeyImage %d failed.\n", i);
            continue;
        };

        mapAnonOutputStats[spentKeyImage.nValue].decSpends(spentKeyImage.nValue);


        COwnedAnonOutput oao;
        if (walletdb.ReadOwnedAnonOutput(vchImage, oao))
        {
            if (fDebugRingSig)
                LogPrintf("UndoAnonTransaction(): input %d keyimage %s found in wallet (owned).\n", i, HexStr(vchImage).c_str());

            std::map<uint256, CWalletTx>::iterator mi = mapWallet.find(oao.outpoint.hash);
            if (mi == mapWallet.end())
            {
                LogPrintf("UndoAnonTransaction(): Error input %d prev txn not in mapwallet %s .\n", i, oao.outpoint.hash.ToString().c_str());
                return false;
            };

            CWalletTx& inTx = (*mi).second;
            if (oao.outpoint.n >= inTx.vout.size())
            {
                LogPrintf("UndoAnonTransaction(): bad wtx %s\n", oao.outpoint.hash.ToString().c_str());
                return false;
            } else
            if (inTx.IsSpent(oao.outpoint.n))
            {
                LogPrintf("UndoAnonTransaction(): found spent coin %s\n", oao.outpoint.hash.ToString().c_str());


                inTx.MarkUnspent(oao.outpoint.n);
                if (!walletdb.WriteTx(oao.outpoint.hash, inTx))
                {
                    LogPrintf("UndoAnonTransaction(): input %d WriteTx failed %s.\n", i, HexStr(vchImage).c_str());
                    return false;
                };
                inTx.MarkDirty(); // recalc balances
                NotifyTransactionChanged(this, oao.outpoint.hash, CT_UPDATED);
            };

            oao.fSpent = false;
            if (!walletdb.WriteOwnedAnonOutput(vchImage, oao))
            {
                LogPrintf("UndoAnonTransaction(): input %d WriteOwnedAnonOutput failed %s.\n", i, HexStr(vchImage).c_str());
                return false;
            };
        };
    };


    for (uint32_t i = 0; i < tx.vout.size(); ++i)
    {
        const CTxOut& txout = tx.vout[i];

        if (!txout.IsAnonOutput())
            continue;

        const CScript &s = txout.scriptPubKey;

        CPubKey pkCoin    = CPubKey(&s[2+1], ec_compressed_size);
        CKeyID  ckCoinId  = pkCoin.GetID();

        CAnonOutput ao;
        if (!txdb.ReadAnonOutput(pkCoin, ao)) // read only to update mapAnonOutputStats
        {
            LogPrintf("ReadAnonOutput(): %u failed.\n", i);
            return false;
        };

        mapAnonOutputStats[ao.nValue].decExists(ao.nValue);

        if (!txdb.EraseAnonOutput(pkCoin))
        {
            LogPrintf("EraseAnonOutput(): %u failed.\n", i);
            continue;
        };

        // -- only in db if owned
        walletdb.EraseLockedAnonOutput(ckCoinId);

        std::vector<uint8_t> vchImage;

        if (!walletdb.ReadOwnedAnonOutputLink(pkCoin, vchImage))
        {
            LogPrintf("ReadOwnedAnonOutputLink(): %u failed - output wasn't owned.\n", i);
            continue;
        };

        if (!walletdb.EraseOwnedAnonOutput(vchImage))
        {
            LogPrintf("EraseOwnedAnonOutput(): %u failed.\n", i);
            continue;
        };

        if (!walletdb.EraseOwnedAnonOutputLink(pkCoin))
        {
            LogPrintf("EraseOwnedAnonOutputLink(): %u failed.\n", i);
            continue;
        };
    };


    if (!walletdb.EraseTx(txnHash))
    {
        LogPrintf("UndoAnonTransaction() EraseTx %s failed.\n", txnHash.ToString().c_str());
        return false;
    };

    mapWallet.erase(txnHash);

    return true;
};

bool CWallet::ProcessAnonTransaction(CWalletDB* pwdb, CTxDB* ptxdb, const CTransaction& tx, const uint256& blockHash, bool& fIsMine, mapValue_t& mapNarr, std::vector<std::map<uint256, CWalletTx>::iterator>& vUpdatedTxns)
{
    uint256 txnHash = tx.GetHash();
    
    if (fDebugRingSig)
        LogPrintf("ProcessAnonTransaction() tx: %s\n", txnHash.GetHex().c_str());
    
    if (nNodeMode != NT_FULL)
    {
        LogPrintf("ProcessAnonTransaction() skipped - must run in full mode.\n");
        return true;
    };
    
    // -- must hold cs_main and cs_wallet lock
    //    txdb and walletdb must be in a transaction (no commit if fail)
    
    for (uint32_t i = 0; i < tx.vin.size(); ++i)
    {
        const CTxIn& txin = tx.vin[i];
        
        if (!txin.IsAnonInput())
            continue;
        
        const CScript &s = txin.scriptSig;
        
        std::vector<uint8_t> vchImage;
        txin.ExtractKeyImage(vchImage);
        
        CKeyImageSpent spentKeyImage;
        
        bool fInMempool;
        if (GetKeyImage(ptxdb, vchImage, spentKeyImage, fInMempool))
        {
            if (spentKeyImage.txnHash == txnHash
                && spentKeyImage.inputNo == i)
            {
                if (fDebugRingSig)
                    LogPrintf("found matching spent key image - txn has been processed before\n");
                return UpdateAnonTransaction(ptxdb, tx, blockHash);
            };
            
            if (TxnHashInSystem(ptxdb, spentKeyImage.txnHash))
            {
                LogPrintf("ProcessAnonTransaction(): Error input %d keyimage %s already spent.\n", i, HexStr(vchImage).c_str());
                return false;
            };
            
            if (fDebugRingSig)
                LogPrintf("Input %d keyimage %s matches unknown txn %s, continuing.\n", i, HexStr(vchImage).c_str(), spentKeyImage.txnHash.ToString().c_str());
            
            // -- keyimage is in db, but invalid as does not point to a known transaction
            //    could be an old mempool keyimage
            //    continue
        };
        
        
        COwnedAnonOutput oao;
        if (pwdb->ReadOwnedAnonOutput(vchImage, oao))
        {
            if (fDebugRingSig)
                LogPrintf("ProcessAnonTransaction(): input %d keyimage %s found in wallet (owned).\n", i, HexStr(vchImage).c_str());
            
            std::map<uint256, CWalletTx>::iterator mi = mapWallet.find(oao.outpoint.hash);
            if (mi == mapWallet.end())
            {
                LogPrintf("ProcessAnonTransaction(): Error input %d prev txn not in mapwallet %s .\n", i, oao.outpoint.hash.ToString().c_str());
                return false;
            };
            
            CWalletTx& inTx = (*mi).second;
            if (oao.outpoint.n >= inTx.vout.size())
            {
                LogPrintf("ProcessAnonTransaction(): bad wtx %s\n", oao.outpoint.hash.ToString().c_str());
                return false;
            } else
            if (!inTx.IsSpent(oao.outpoint.n))
            {
                LogPrintf("ProcessAnonTransaction(): found spent coin %s\n", oao.outpoint.hash.ToString().c_str());
                
                inTx.MarkSpent(oao.outpoint.n);
                if (!pwdb->WriteTx(oao.outpoint.hash, inTx))
                {
                    LogPrintf("ProcessAnonTransaction(): input %d WriteTx failed %s.\n", i, HexStr(vchImage).c_str());
                    return false;
                };
                
                inTx.MarkDirty();           // recalc balances
                vUpdatedTxns.push_back(mi); // notify updates outside db txn
            };
            
            oao.fSpent = true;
            if (!pwdb->WriteOwnedAnonOutput(vchImage, oao))
            {
                LogPrintf("ProcessAnonTransaction(): input %d WriteOwnedAnonOutput failed %s.\n", i, HexStr(vchImage).c_str());
                return false;
            };
        };
        
        int nRingSize = txin.ExtractRingSize();
        if (nRingSize < (int)MIN_RING_SIZE
            || nRingSize > (int)MAX_RING_SIZE)
        {
            LogPrintf("ProcessAnonTransaction(): Error input %d ringsize %d not in range [%d, %d].\n", i, nRingSize, MIN_RING_SIZE, MAX_RING_SIZE);
            return false;
        };
        
        if (s.size() < 2 + (ec_compressed_size + ec_secret_size + ec_secret_size) * nRingSize)
        {
            LogPrintf("ProcessAnonTransaction(): Error input %d scriptSig too small.\n", i);
            return false;
        };
        
        int64_t nCoinValue = -1;
        
        CPubKey pkRingCoin;
        CAnonOutput ao;
        CTxIndex txindex;
        const unsigned char* pPubkeys = &s[2];
        for (uint32_t ri = 0; ri < (uint32_t)nRingSize; ++ri)
        {
            pkRingCoin = CPubKey(&pPubkeys[ri * ec_compressed_size], ec_compressed_size);
            if (!ptxdb->ReadAnonOutput(pkRingCoin, ao))
            {
                LogPrintf("ProcessAnonTransaction(): Error input %u AnonOutput %s not found.\n", i, HexStr(pkRingCoin).c_str());
                return false;
            };
            
            if (nCoinValue == -1)
            {
                nCoinValue = ao.nValue;
            } else
            if (nCoinValue != ao.nValue)
            {
                LogPrintf("ProcessAnonTransaction(): Error input %u ring amount mismatch %d, %d.\n", i, nCoinValue, ao.nValue);
                return false;
            };
            
            if (ao.nBlockHeight == 0
                || nBestHeight - ao.nBlockHeight < MIN_ANON_SPEND_DEPTH)
            {
                LogPrintf("ProcessAnonTransaction(): Error input %u ring coin %u depth < MIN_ANON_SPEND_DEPTH.\n", i, ri);
                return false;
            };
            
            // -- ring sig validation is done in CTransaction::CheckAnonInputs()
        };
        
        spentKeyImage.txnHash = txnHash;
        spentKeyImage.inputNo = i;
        spentKeyImage.nValue = nCoinValue;
        
        if (blockHash != 0)
        {
            if (!ptxdb->WriteKeyImage(vchImage, spentKeyImage))
            {
                LogPrintf("ProcessAnonTransaction(): Error input %d WriteKeyImage failed %s .\n", i, HexStr(vchImage).c_str());
                return false;
            };
        } else
        {
            // -- add keyImage to mempool, will be added to txdb in UpdateAnonTransaction 
            mempool.insertKeyImage(vchImage, spentKeyImage);
        };
        
        mapAnonOutputStats[spentKeyImage.nValue].incSpends(spentKeyImage.nValue);
    };
    
    ec_secret sSpendR;
    ec_secret sSpend;
    ec_secret sScan;
    ec_secret sShared;
    
    ec_point pkExtracted;
    
    std::vector<uint8_t> vchEphemPK;
    std::vector<uint8_t> vchDataB;
    std::vector<uint8_t> vchENarr;
    
    std::vector<std::vector<uint8_t> > vPrevMatch;
    char cbuf[256];
    
    try { vchEphemPK.resize(ec_compressed_size); } catch (std::exception& e)
    {
        LogPrintf("Error: vchEphemPK.resize threw: %s.\n", e.what());
        return false;
    };
    
    int nBlockHeight = GetBlockHeightFromHash(blockHash);
    
    for (uint32_t i = 0; i < tx.vout.size(); ++i)
    {
        const CTxOut& txout = tx.vout[i];
        
        if (!txout.IsAnonOutput())
            continue;
        
        const CScript &s = txout.scriptPubKey;
        
        CPubKey pkCoin    = CPubKey(&s[2+1], ec_compressed_size);
        CKeyID  ckCoinId  = pkCoin.GetID();
        
        COutPoint outpoint = COutPoint(tx.GetHash(), i);
        
        // -- add all anon outputs to txdb
        CAnonOutput ao;
        
        if (ptxdb->ReadAnonOutput(pkCoin, ao)) // check if exists
        {
            if (blockHash != 0)
            {
                if (fDebugRingSig)
                    LogPrintf("Found existing anon output - assuming txn has been processed before.\n");
                return UpdateAnonTransaction(ptxdb, tx, blockHash);
            };
            LogPrintf("Error: Found duplicate anon output.\n");
            return false;
        };
        
        ao = CAnonOutput(outpoint, txout.nValue, nBlockHeight, 0);
        
        if (!ptxdb->WriteAnonOutput(pkCoin, ao))
        {
            LogPrintf("WriteKeyImage failed.\n");
            continue;
        };
        
        mapAnonOutputStats[txout.nValue].addCoin(nBlockHeight, txout.nValue);
        
        memcpy(&vchEphemPK[0], &s[2+ec_compressed_size+2], ec_compressed_size);
        
        bool fOwnOutput = false;
        CPubKey cpkE;
        std::set<CStealthAddress>::iterator it;
        for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
        {
            if (it->scan_secret.size() != ec_secret_size)
                continue; // stealth address is not owned

            memcpy(&sScan.e[0], &it->scan_secret[0], ec_secret_size);

            if (StealthSecret(sScan, vchEphemPK, it->spend_pubkey, sShared, pkExtracted) != 0)
            {
                LogPrintf("StealthSecret failed.\n");
                continue;
            };

            cpkE = CPubKey(pkExtracted);

            if (!cpkE.IsValid()
                || cpkE != pkCoin)
                continue;
            fOwnOutput = true;
            break;
        };

        if (!fOwnOutput)
            continue;

        if (fDebugRingSig)
            LogPrintf("anon output match tx, no %s, %u\n", txnHash.GetHex().c_str(), i);

        fIsMine = true; // mark tx to be added to wallet

        int lenENarr = 0;
        if (s.size() > MIN_ANON_OUT_SIZE)
            lenENarr = s[2+ec_compressed_size+1 + ec_compressed_size+1];

        if (lenENarr > 0)
        {
            if (fDebugRingSig)
                LogPrintf("Processing encrypted narration of %d bytes\n", lenENarr);
            
            try { vchENarr.resize(lenENarr); } catch (std::exception& e)
            {
                LogPrintf("Error: vchENarr.resize threw: %s.\n", e.what());
                continue;
            };
            memcpy(&vchENarr[0], &s[2+ec_compressed_size+1+ec_compressed_size+2], lenENarr);

            SecMsgCrypter crypter;
            crypter.SetKey(&sShared.e[0], &vchEphemPK[0]);
            std::vector<uint8_t> vchNarr;
            if (!crypter.Decrypt(&vchENarr[0], vchENarr.size(), vchNarr))
            {
                LogPrintf("Decrypt narration failed.\n");
                continue;
            };
            std::string sNarr = std::string(vchNarr.begin(), vchNarr.end());

            snprintf(cbuf, sizeof(cbuf), "n_%u", i);
            mapNarr[cbuf] = sNarr;
        };

        if (IsLocked())
        {
            std::vector<uint8_t> vchEmpty;
            CWalletDB *pwalletdbEncryptionOld = pwalletdbEncryption;
            pwalletdbEncryption = pwdb; // HACK, pass pdb to AddCryptedKey
            AddCryptedKey(cpkE, vchEmpty);
            pwalletdbEncryption = pwalletdbEncryptionOld;

            if (fDebugRingSig)
                LogPrintf("Wallet locked, adding key without secret.\n");

            std::string sSxAddr = it->Encoded();
            std::string sLabel = std::string("ao ") + sSxAddr.substr(0, 16) + "...";
            SetAddressBookName(ckCoinId, sLabel, pwdb, false);

            CPubKey cpkEphem(vchEphemPK);
            CPubKey cpkScan(it->scan_pubkey);
            CLockedAnonOutput lockedAo(cpkEphem, cpkScan, COutPoint(txnHash, i));
            if (!pwdb->WriteLockedAnonOutput(ckCoinId, lockedAo))
            {
                CBitcoinAddress coinAddress(ckCoinId);
                LogPrintf("WriteLockedAnonOutput failed for %s\n", coinAddress.ToString().c_str());
            };
        } else
        {
            if (it->spend_secret.size() != ec_secret_size)
                continue;
            
            memcpy(&sSpend.e[0], &it->spend_secret[0], ec_secret_size);
            
            if (StealthSharedToSecretSpend(sShared, sSpend, sSpendR) != 0)
            {
                LogPrintf("StealthSharedToSecretSpend() failed.\n");
                continue;
            };


            ec_point pkTestSpendR;
            if (SecretToPublicKey(sSpendR, pkTestSpendR) != 0)
            {
                LogPrintf("SecretToPublicKey() failed.\n");
                continue;
            };

            CKey ckey;
            ckey.Set(&sSpendR.e[0], true);

            if (!ckey.IsValid())
            {
                LogPrintf("Reconstructed key is invalid.\n");
                continue;
            };

            CPubKey cpkT = ckey.GetPubKey();
            if (!cpkT.IsValid()
                || cpkT != pkCoin)
            {
                LogPrintf("cpkT is invalid.\n");
                continue;
            };

            if (fDebugRingSig)
            {
                CBitcoinAddress coinAddress(ckCoinId);
                LogPrintf("Adding key %s.\n", coinAddress.ToString().c_str());
            };

            if (!AddKeyInDBTxn(pwdb, ckey))
            {
                LogPrintf("AddKeyInDBTxn failed.\n");
                continue;
            };

            // TODO: groupings?
            std::string sSxAddr = it->Encoded();
            std::string sLabel = std::string("ao ") + sSxAddr.substr(0, 16) + "...";
            SetAddressBookName(ckCoinId, sLabel, pwdb, false);
            
            
            // -- store keyImage
            ec_point pkImage;
            if (generateKeyImage(pkTestSpendR, sSpendR, pkImage) != 0)
            {
                LogPrintf("generateKeyImage() failed.\n");
                continue;
            };
            
            bool fSpentAOut = false;
            bool fInMemPool;
            CKeyImageSpent kis;
            if (GetKeyImage(ptxdb, pkImage, kis, fInMemPool)
                && !fInMemPool) // shouldn't be possible for kis to be in mempool here
            {
                fSpentAOut = true;
            };
            
            COwnedAnonOutput oao(outpoint, fSpentAOut);
            
            if (!pwdb->WriteOwnedAnonOutput(pkImage, oao)
                || !pwdb->WriteOwnedAnonOutputLink(pkCoin, pkImage))
            {
                LogPrintf("WriteOwnedAnonOutput() failed.\n");
                continue;
            };
            
            if (fDebugRingSig)
                LogPrintf("Adding anon output to wallet: %s.\n", HexStr(pkImage).c_str());
        };
    };
    
    
    return true;
};

bool CWallet::GetAnonChangeAddress(CStealthAddress& sxAddress)
{
    // return owned stealth address to send anon change to.
    // TODO: make an option

    std::set<CStealthAddress>::iterator it;
    for (it = stealthAddresses.begin(); it != stealthAddresses.end(); ++it)
    {
        if (it->scan_secret.size() < 1)
            continue; // stealth address is not owned

        sxAddress = *it;
        return true;
    };

    return false;
};

bool CWallet::CreateStealthOutput(CStealthAddress* sxAddress, int64_t nValue, std::string& sNarr, std::vector<std::pair<CScript, int64_t> >& vecSend, std::map<int, std::string>& mapNarr, std::string& sError)
{
    if (fDebugRingSig)
        LogPrintf("CreateAnonOutputs()\n");

    if (!sxAddress)
    {
        sError = "!sxAddress, todo.";
        return false;
    };

    ec_secret ephem_secret;
    ec_secret secretShared;
    ec_point pkSendTo;
    ec_point ephem_pubkey;

    if (GenerateRandomSecret(ephem_secret) != 0)
    {
        sError = "GenerateRandomSecret failed.";
        return false;
    };

    if (StealthSecret(ephem_secret, sxAddress->scan_pubkey, sxAddress->spend_pubkey, secretShared, pkSendTo) != 0)
    {
        sError = "Could not generate receiving public key.";
        return false;
    };

    CPubKey cpkTo(pkSendTo);
    if (!cpkTo.IsValid())
    {
        sError = "Invalid public key generated.";
        return false;
    };

    CKeyID ckidTo = cpkTo.GetID();

    CBitcoinAddress addrTo(ckidTo);

    if (SecretToPublicKey(ephem_secret, ephem_pubkey) != 0)
    {
        sError = "Could not generate ephem public key.";
        return false;
    };

    if (fDebug)
    {
        LogPrintf("CreateStealthOutput() to generated pubkey %u: %s\n", pkSendTo.size(), HexStr(pkSendTo).c_str());
        LogPrintf("hash %s\n", addrTo.ToString().c_str());
        LogPrintf("ephem_pubkey %u: %s\n", ephem_pubkey.size(), HexStr(ephem_pubkey).c_str());
    };

    std::vector<unsigned char> vchENarr;
    if (sNarr.length() > 0)
    {
        SecMsgCrypter crypter;
        crypter.SetKey(&secretShared.e[0], &ephem_pubkey[0]);

        if (!crypter.Encrypt((uint8_t*)&sNarr[0], sNarr.length(), vchENarr))
        {
            sError = "Narration encryption failed.";
            return false;
        };

        if (vchENarr.size() > MAX_STEALTH_NARRATION_SIZE)
        {
            sError = "Encrypted narration is too long.";
            return false;
        };
    };


    CScript scriptPubKey;
    scriptPubKey.SetDestination(addrTo.Get());

    vecSend.push_back(make_pair(scriptPubKey, nValue));

    CScript scriptP = CScript() << OP_RETURN << ephem_pubkey;
    if (vchENarr.size() > 0)
        scriptP = scriptP << OP_RETURN << vchENarr;

    vecSend.push_back(make_pair(scriptP, 0));

    // TODO: shuffle change later?
    if (vchENarr.size() > 0)
    {
        for (unsigned int k = 0; k < vecSend.size(); ++k)
        {
            if (vecSend[k].first != scriptPubKey
                || vecSend[k].second != nValue)
                continue;

            mapNarr[k] = sNarr;
            break;
        };
    };

    return true;
};

bool CWallet::CreateAnonOutputs(CStealthAddress* sxAddress, int64_t nValue, std::string& sNarr, std::vector<std::pair<CScript, int64_t> >& vecSend, CScript& scriptNarration)
{
    if (fDebugRingSig)
        LogPrintf("CreateAnonOutputs()\n");

    ec_secret scEphem;
    ec_secret scShared;
    ec_point  pkSendTo;
    ec_point  pkEphem;

    CPubKey   cpkTo;

    // -- output scripts OP_RETURN ANON_TOKEN pkTo R enarr
    //    Each outputs split from the amount must go to a unique pk, or the key image would be the same
    //    Only the first output of the group carries the enarr (if present)


    std::vector<int64_t> vOutAmounts;
    if (splitAmount(nValue, vOutAmounts) != 0)
    {
        LogPrintf("splitAmount() failed.\n");
        return false;
    };

    for (uint32_t i = 0; i < vOutAmounts.size(); ++i)
    {
        if (GenerateRandomSecret(scEphem) != 0)
        {
            LogPrintf("GenerateRandomSecret failed.\n");
            return false;
        };

        if (sxAddress) // NULL for test only
        {
            if (StealthSecret(scEphem, sxAddress->scan_pubkey, sxAddress->spend_pubkey, scShared, pkSendTo) != 0)
            {
                LogPrintf("Could not generate receiving public key.\n");
                return false;
            };

            cpkTo = CPubKey(pkSendTo);
            if (!cpkTo.IsValid())
            {
                LogPrintf("Invalid public key generated.\n");
                return false;
            };

            if (SecretToPublicKey(scEphem, pkEphem) != 0)
            {
                LogPrintf("Could not generate ephem public key.\n");
                return false;
            };
        };

        CScript scriptSendTo;
        scriptSendTo.push_back(OP_RETURN);
        scriptSendTo.push_back(OP_ANON_MARKER);
        scriptSendTo << cpkTo;
        scriptSendTo << pkEphem;

        if (i == 0 && sNarr.length() > 0)
        {
            std::vector<unsigned char> vchNarr;
            SecMsgCrypter crypter;
            crypter.SetKey(&scShared.e[0], &pkEphem[0]);

            if (!crypter.Encrypt((uint8_t*)&sNarr[0], sNarr.length(), vchNarr))
            {
                LogPrintf("Narration encryption failed.\n");
                return false;
            };

            if (vchNarr.size() > MAX_STEALTH_NARRATION_SIZE)
            {
                LogPrintf("Encrypted narration is too long.\n");
                return false;
            };
            scriptSendTo << vchNarr;
            scriptNarration = scriptSendTo;
        };

        if (fDebug)
        {
            CKeyID ckidTo = cpkTo.GetID();
            CBitcoinAddress addrTo(ckidTo);

            LogPrintf("CreateAnonOutput to generated pubkey %u: %s\n", pkSendTo.size(), HexStr(pkSendTo).c_str());
            if (!sxAddress)
                LogPrintf("Test Mode\n");
            LogPrintf("hash %s\n", addrTo.ToString().c_str());
            LogPrintf("ephemeral pubkey %u: %s\n", pkEphem.size(), HexStr(pkEphem).c_str());

            LogPrintf("scriptPubKey %s\n", scriptSendTo.ToString().c_str());
        };
        vecSend.push_back(make_pair(scriptSendTo, vOutAmounts[i]));
    };

    // TODO: will this be optimised away?
    memset(&scShared.e[0], 0, ec_secret_size);

    return true;
};

static bool checkCombinations(int64_t nReq, int m, std::vector<COwnedAnonOutput*>& vData, std::vector<int>& v)
{
    // -- m of n combinations, check smallest coins first
    
    if (fDebugRingSig)
        LogPrintf("checkCombinations() %d, %u\n", m, vData.size());
    
    int n = vData.size();
    
    try { v.resize(m); } catch (std::exception& e)
    {
        LogPrintf("Error: checkCombinations() v.resize(%d) threw: %s.\n", m, e.what());
        return false;
    };
    
    
    int64_t nCount = 0;
     
    if (m > n) // ERROR
    {
        LogPrintf("Error: checkCombinations() m > n\n");
        return false;
    };
    
    int i, l, startL = 0;
    
    // -- pick better start point
    //    lAvailableCoins is sorted, if coin i * m < nReq, no combinations of lesser coins will be < either
    for (l = m; l <= n; ++l)
    {
        if (vData[l-1]->nValue * m < nReq)
            continue;
        startL = l;
        break;
    };
    
    if (fDebugRingSig)
        LogPrintf("Starting at level %d\n", startL);
    
    if (startL == 0)
    {
        LogPrintf("checkCombinations() No possible combinations.\n");
        return false;
    };
    
    
    for (l = startL; l <= n; ++l)
    {
        for (i = 0; i < m; ++i)
            v[i] = (m - i)-1;
        v[0] = l-1;
        
        // -- m must be > 2 to use coarse seeking
        bool fSeekFine = m > 2 ? false : true;
        
        // -- coarse
        while(!fSeekFine && v[1] < v[0]-1)
        {
            for (i = 1; i < m; ++i)
                v[i] = v[i]+1;
            
            int64_t nTotal = 0;
            
            for (i = 0; i < m; ++i)
                nTotal += vData[v[i]]->nValue;
            
            nCount++;
            
            if (nTotal == nReq)
            {
                if (fDebugRingSig)
                {
                    LogPrintf("Found match of total %d, in %d tries\n", nTotal, nCount);
                    for (i = m; i--;) LogPrintf("%d%c", v[i], i ? ' ': '\n');
                };
                return true;
            };
            if (nTotal > nReq)
            {
                for (i = 1; i < m; ++i) // rewind
                    v[i] = v[i]-1;
                
                if (fDebugRingSig)
                {
                    LogPrintf("Found coarse match of total %d, in %d tries\n", nTotal, nCount);
                    for (i = m; i--;) LogPrintf("%d%c", v[i], i ? ' ': '\n');
                };
                fSeekFine = true;
            };
        };
        
        if (!fSeekFine)
            continue;
        
        // -- fine
        i = m-1;
        for (;;)
        {
            if (v[0] == l-1) // otherwise get duplicate combinations
            {
                int64_t nTotal = 0;
                
                for (i = 0; i < m; ++i)
                    nTotal += vData[v[i]]->nValue;
                
                nCount++;
                
                if (nTotal >= nReq)
                {
                    if (fDebugRingSig)
                    {
                        LogPrintf("Found match of total %d, in %d tries\n", nTotal, nCount);
                        for (i = m; i--;) LogPrintf("%d%c", v[i], i ? ' ': '\n');
                    };
                    return true;
                };
                
                if (fDebugRingSig && !(nCount % 500))
                {
                    LogPrintf("checkCombinations() nCount: %d - l: %d, n: %d, m: %d, i: %d, nReq: %d, v[0]: %d, nTotal: %d \n", nCount, l, n, m, i, nReq, v[0], nTotal);
                    for (i = m; i--;) LogPrintf("%d%c", v[i], i ? ' ': '\n');
                };
            };
            
            for (i = 0; v[i] >= l - i;) // 0 is largest element
            {
                if (++i >= m)
                    goto EndInner;
            };
            
            // -- fill the set with the next values
            for (v[i]++; i; i--)
                v[i-1] = v[i] + 1;
        };
        EndInner:
        if (i+1 > n)
            break;
    };
    
    return false;
}

int CWallet::PickAnonInputs(int64_t nValue, int64_t& nFee, int nRingSize, CWalletTx& wtxNew, int nOutputs, int nSizeOutputs, int& nExpectChangeOuts, std::list<COwnedAnonOutput>& lAvailableCoins, std::vector<COwnedAnonOutput*>& vPickedCoins, vector<pair<CScript, int64_t> >& vecChange, bool fTest, std::string& sError)
{
    if (fDebugRingSig)
        LogPrintf("PickAnonInputs(), ChangeOuts %d\n", nExpectChangeOuts);
    // - choose the smallest coin that can cover the amount + fee
    //   or least no. of smallest coins


    int64_t nAmountCheck = 0;

    std::vector<COwnedAnonOutput*> vData;
    try { vData.resize(lAvailableCoins.size()); } catch (std::exception& e)
    {
        LogPrintf("Error: PickAnonInputs() vData.resize threw: %s.\n", e.what());
        return false;
    };

    uint32_t vi = 0;
    for (std::list<COwnedAnonOutput>::iterator it = lAvailableCoins.begin(); it != lAvailableCoins.end(); ++it)
    {
        vData[vi++] = &(*it);
        nAmountCheck += it->nValue;
    };

    // -- sigSize:  2byte marker + (cpubkey + sigc + sigr) * nRingSize;
    uint32_t nByteSizePerInCoin = (sizeof(COutPoint) + sizeof(unsigned int)) // CTxIn
        + GetSizeOfCompactSize(2 + (33 + 32 + 32) * nRingSize)
        + 2 + (33 + 32 + 32) * nRingSize;

    // -- repeat until all levels are tried (1 coin, 2 coins, 3 coins etc)
    for (uint32_t i = 0; i < lAvailableCoins.size(); ++i)
    {
        if (fDebugRingSig)
            LogPrintf("Input loop %u\n", i);

        uint32_t nTotalBytes = (4 + 4 + 4) // Ctx: nVersion, nTime, nLockTime
            + GetSizeOfCompactSize(nOutputs + nExpectChangeOuts)
            + nSizeOutputs
            + (GetSizeOfCompactSize(MIN_ANON_OUT_SIZE) + MIN_ANON_OUT_SIZE + sizeof(int64_t)) * nExpectChangeOuts
            + GetSizeOfCompactSize((i+1))
            + nByteSizePerInCoin * (i+1);

        nFee = wtxNew.GetMinFee(0, GMF_ANON, nTotalBytes);

        if (nValue + nFee > nAmountCheck)
        {
            sError = "Not enough coins with requested ring size.";
            return 3;
        };

        vPickedCoins.clear();
        vecChange.clear();

        std::vector<int> v;
        if (checkCombinations(nValue + nFee, i+1, vData, v))
        {
            if (fDebugRingSig)
            {
                LogPrintf("Found combination %u, ", i+1);
                for (int ic = v.size(); ic--;)
                    LogPrintf("%d%c", v[ic], ic ? ' ': '\n');

                LogPrintf("nTotalBytes %u\n", nTotalBytes);
                LogPrintf("nFee %d\n", nFee);
            };

            int64_t nTotalIn = 0;
            vPickedCoins.resize(v.size());
            for (uint32_t ic = 0; ic < v.size(); ++ic)
            {
                vPickedCoins[ic] = vData[v[ic]];
                nTotalIn += vPickedCoins[ic]->nValue;
            };

            int64_t nChange = nTotalIn - (nValue + nFee);

            CStealthAddress sxChange;
            if (!GetAnonChangeAddress(sxChange))
            {
                sError = "GetAnonChangeAddress() change failed.";
                return 3;
            };

            std::string sNone;
            sNone.clear();
            CScript scriptNone;
            if (!CreateAnonOutputs(fTest ? NULL : &sxChange, nChange, sNone, vecChange, scriptNone))
            {
                sError = "CreateAnonOutputs() change failed.";
                return 3;
            };


            // -- get nTotalBytes again, using actual no. of change outputs
            uint32_t nTotalBytes = (4 + 4 + 4) // Ctx: nVersion, nTime, nLockTime
                + GetSizeOfCompactSize(nOutputs + vecChange.size())
                + nSizeOutputs
                + (GetSizeOfCompactSize(MIN_ANON_OUT_SIZE) + MIN_ANON_OUT_SIZE + sizeof(int64_t)) * vecChange.size()
                + GetSizeOfCompactSize((i+1))
                + nByteSizePerInCoin * (i+1);

            int64_t nTestFee = wtxNew.GetMinFee(0, GMF_ANON, nTotalBytes);

            if (nTestFee > nFee)
            {
                if (fDebugRingSig)
                    LogPrintf("Try again - nTestFee > nFee %d, %d, nTotalBytes %u\n", nTestFee, nFee, nTotalBytes);
                nExpectChangeOuts = vecChange.size();
                return 2; // up changeOutSize
            };

            nFee = nTestFee;
            return 1; // found
        };

    };

    return 0; // not found
};

int CWallet::GetTxnPreImage(CTransaction& txn, uint256& hash)
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << txn.nVersion;
    ss << txn.nTime;
    for (uint32_t i = 0; i < txn.vin.size(); ++i)
    {
        const CTxIn& txin = txn.vin[i];
        ss << txin.prevout; // keyimage only
        
        int ringSize = txin.ExtractRingSize();
        
        // TODO: is it neccessary to include the ring members in the hash?
        if (txin.scriptSig.size() < 2 + ringSize * ec_compressed_size)
        {
            LogPrintf("scriptSig is too small, input %u, ring size %d.\n", i, ringSize);
            return 1;
        };
        ss.write((const char*)&txin.scriptSig[2], ringSize * ec_compressed_size);
    };

    for (uint32_t i = 0; i < txn.vout.size(); ++i)
        ss << txn.vout[i];
    ss << txn.nLockTime;

    hash = ss.GetHash();

    return 0;
};

int CWallet::PickHidingOutputs(int64_t nValue, int nRingSize, CPubKey& pkCoin, int skip, uint8_t* p)
{
    if (fDebug)
        LogPrintf("PickHidingOutputs() %d, %d\n", nValue, nRingSize);

    // TODO: process multiple inputs in 1 db loop?

    // -- offset skip is pre filled with the real coin

    LOCK(cs_main);
    CTxDB txdb("r");

    leveldb::DB* pdb = txdb.GetInstance();
    if (!pdb)
        throw runtime_error("CWallet::PickHidingOutputs() : cannot get leveldb instance");

    leveldb::Iterator *iterator = pdb->NewIterator(leveldb::ReadOptions());

    std::vector<CPubKey> vHideKeys;

    // Seek to start key.
    CPubKey pkZero;
    pkZero.SetZero();

    CDataStream ssStartKey(SER_DISK, CLIENT_VERSION);
    ssStartKey << make_pair(string("ao"), pkZero);
    iterator->Seek(ssStartKey.str());

    CPubKey pkAo;
    CAnonOutput anonOutput;
    while (iterator->Valid())
    {
        // Unpack keys and values.
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;
        
        if (strType != "ao")
            break;
        
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());
        
        
        ssKey >> pkAo;
        
        if (pkAo != pkCoin
            && pkAo.IsValid())
        {
            ssValue >> anonOutput;
            
            if ((anonOutput.nBlockHeight > 0 && nBestHeight - anonOutput.nBlockHeight >= MIN_ANON_SPEND_DEPTH)
                && anonOutput.nValue == nValue
                && anonOutput.nCompromised == 0)
                try { vHideKeys.push_back(pkAo); } catch (std::exception& e)
                {
                    LogPrintf("Error: PickHidingOutputs() vHideKeys.push_back threw: %s.\n", e.what());
                    return 1;
                };

        };

        iterator->Next();
    };

    delete iterator;

    if ((int)vHideKeys.size() < nRingSize-1)
    {
        LogPrintf("Not enough keys found.\n");
        return 1;
    };

    for (int i = 0; i < nRingSize; ++i)
    {
        if (i == skip)
            continue;

        if (vHideKeys.size() < 1)
        {
            LogPrintf("vHideKeys.size() < 1\n");
            return 1;
        };

        uint32_t pick = GetRand(vHideKeys.size());

        memcpy(p + i * 33, vHideKeys[pick].begin(), 33);

        vHideKeys.erase(vHideKeys.begin()+pick);
    };


    return 0;
};

bool CWallet::AreOutputsUnique(CWalletTx& wtxNew)
{
    LOCK(cs_main);
    CTxDB txdb;

    for (uint32_t i = 0; i < wtxNew.vout.size(); ++i)
    {
        const CTxOut& txout = wtxNew.vout[i];

        if (txout.IsAnonOutput())
            continue;

        const CScript &s = txout.scriptPubKey;

        CPubKey pkCoin = CPubKey(&s[2+1], ec_compressed_size);
        CAnonOutput ao;
        
        if (txdb.ReadAnonOutput(pkCoin, ao))
        {
            //LogPrintf("AreOutputsUnique() pk %s is not unique.\n", pkCoin);
            return false;
        };
    };

    return true;
};

bool CWallet::AddAnonInputs(int64_t nTotalOut, int nRingSize, std::vector<std::pair<CScript, int64_t> >&vecSend, std::vector<std::pair<CScript, int64_t> >&vecChange, CWalletTx& wtxNew, int64_t& nFeeRequired, bool fTestOnly, std::string& sError)
{
    if (fDebugRingSig)
        LogPrintf("AddAnonInputs() %d, %d\n", nTotalOut, nRingSize);

    std::list<COwnedAnonOutput> lAvailableCoins;
    if (ListUnspentAnonOutputs(lAvailableCoins, true) != 0)
    {
        sError = "ListUnspentAnonOutputs() failed";
        return false;
    };

    std::map<int64_t, int> mOutputCounts;
    for (std::list<COwnedAnonOutput>::iterator it = lAvailableCoins.begin(); it != lAvailableCoins.end(); ++it)
        mOutputCounts[it->nValue] = 0;

    if (CountAnonOutputs(mOutputCounts, true) != 0)
    {
        sError = "CountAnonOutputs() failed";
        return false;
    };

    if (fDebugRingSig)
    {
        for (std::map<int64_t, int>::iterator it = mOutputCounts.begin(); it != mOutputCounts.end(); ++it)
            LogPrintf("mOutputCounts %ld %d\n", it->first, it->second);
    };

    int64_t nAmountCheck = 0;
    // -- remove coins that don't have enough same value anonoutputs in the system for the ring size
    std::list<COwnedAnonOutput>::iterator it = lAvailableCoins.begin();
    while (it != lAvailableCoins.end())
    {
        std::map<int64_t, int>::iterator mi = mOutputCounts.find(it->nValue);
        if (mi == mOutputCounts.end()
            || mi->second < nRingSize)
        {
            // -- not enough coins of same value, drop coin
            lAvailableCoins.erase(it++);
            continue;
        };

        nAmountCheck += it->nValue;
        ++it;
    };

    if (fDebugRingSig)
        LogPrintf("%u coins available with ring size %d, total %d\n", lAvailableCoins.size(), nRingSize, nAmountCheck);

    // -- estimate fee

    uint32_t nSizeOutputs = 0;
    for (uint32_t i = 0; i < vecSend.size(); ++i) // need to sum due to narration
        nSizeOutputs += GetSizeOfCompactSize(vecSend[i].first.size()) + vecSend[i].first.size() + sizeof(int64_t); // CTxOut

    bool fFound = false;
    int64_t nFee;
    int nExpectChangeOuts = 5;
    std::string sPickError;
    std::vector<COwnedAnonOutput*> vPickedCoins;
    for (int k = 0; k < 50; ++k) // safety
    {
        // -- nExpectChangeOuts is raised if needed (rv == 2)
        int rv = PickAnonInputs(nTotalOut, nFee, nRingSize, wtxNew, vecSend.size(), nSizeOutputs, nExpectChangeOuts, lAvailableCoins, vPickedCoins, vecChange, false, sPickError);
        if (rv == 0)
            break;
        if (rv == 3)
        {
            sError = sPickError;
            return false;
        };
        if (rv == 1)
        {
            fFound = true;
            break;
        };
    };

    if (!fFound)
    {
        sError = "No combination of coins matches amount and ring size.";
        return false;
    };

    nFeeRequired = nFee; // set in PickAnonInputs()

    // -- need hash of tx without signatures
    std::vector<int> vCoinOffsets;
    uint32_t ii = 0;
    wtxNew.vin.resize(vPickedCoins.size());
    vCoinOffsets.resize(vPickedCoins.size());
    for (std::vector<COwnedAnonOutput*>::iterator it = vPickedCoins.begin(); it != vPickedCoins.end(); ++it)
    {
        CTxIn& txin = wtxNew.vin[ii];
        if (fDebugRingSig)
            LogPrintf("pickedCoin %s %d\n", HexStr((*it)->vchImage).c_str(), (*it)->nValue);

        // -- overload prevout to hold keyImage
        memcpy(txin.prevout.hash.begin(), &(*it)->vchImage[0], ec_secret_size);

        txin.prevout.n = 0 | (((*it)->vchImage[32]) & 0xFF) | (int32_t)(((int16_t) nRingSize) << 16);

        // -- size for full signature, signature is added later after hash
        try { txin.scriptSig.resize(2 + (ec_compressed_size + ec_secret_size + ec_secret_size) * nRingSize); } catch (std::exception& e)
        {
            LogPrintf("Error: SendAnonToAnon() txin.scriptSig.resize threw: %s.\n", e.what());
            sError = "resize failed.\n";
            return false;
        };

        txin.scriptSig[0] = OP_RETURN;
        txin.scriptSig[1] = OP_ANON_MARKER;

        if (fTestOnly)
            continue;

        int nCoinOutId = (*it)->outpoint.n;
        map<uint256, CWalletTx>::iterator mi = mapWallet.find((*it)->outpoint.hash);
        if (mi == mapWallet.end()
            || mi->second.nVersion != ANON_TXN_VERSION
            || (int)mi->second.vout.size() < nCoinOutId)
        {
            LogPrintf("Error: SendAnonToAnon() picked coin not in wallet, %s version %d.\n", (*it)->outpoint.hash.ToString().c_str(), (*mi).second.nVersion);
            sError = "picked coin not in wallet.\n";
            return false;
        };

        CWalletTx& wtxAnonCoin = mi->second;

        const CTxOut& txout = wtxAnonCoin.vout[nCoinOutId];
        const CScript &s = txout.scriptPubKey;

        if (!txout.IsAnonOutput())
        {
            sError = "picked coin not an anon output.\n";
            return false;
        };
        
        CPubKey pkCoin = CPubKey(&s[2+1], ec_compressed_size);
        
        if (!pkCoin.IsValid())
        {
            sError = "pkCoin is invalid.\n";
            return false;
        };
        
        vCoinOffsets[ii] = GetRand(nRingSize);
        
        memcpy(&txin.scriptSig[2 + vCoinOffsets[ii] * ec_compressed_size], pkCoin.begin(), ec_compressed_size);
        
        if (PickHidingOutputs((*it)->nValue, nRingSize, pkCoin, vCoinOffsets[ii], &txin.scriptSig[2]) != 0)
        {
            sError = "PickHidingOutputs() failed.\n";
            return false;
        };
        ii++;
    };
    
    for (uint32_t i = 0; i < vecSend.size(); ++i)
        wtxNew.vout.push_back(CTxOut(vecSend[i].second, vecSend[i].first));
    for (uint32_t i = 0; i < vecChange.size(); ++i)
        wtxNew.vout.push_back(CTxOut(vecChange[i].second, vecChange[i].first));
    
    if (fTestOnly)
        return true;
    
    uint256 preimage;
    if (GetTxnPreImage(wtxNew, preimage) != 0)
    {
        sError = "GetPreImage() failed.\n";
        return false;
    };
    
    // TODO: Does it lower security to use the same preimage for each input?
    //  cryptonote seems to do so too

    for (uint32_t i = 0; i < wtxNew.vin.size(); ++i)
    {
        CTxIn& txin = wtxNew.vin[i];
        
        // Test
        std::vector<uint8_t> vchImageTest;
        txin.ExtractKeyImage(vchImageTest);
        
        int nTestRingSize = txin.ExtractRingSize();
        if (nTestRingSize != nRingSize)
        {
            sError = "nRingSize embed error.";
            return false;
        };

        if (txin.scriptSig.size() < 2 + (ec_compressed_size + ec_secret_size + ec_secret_size) * nRingSize)
        {
            sError = "Error: scriptSig too small.";
            return false;
        };
        
        
        int nSecretOffset = vCoinOffsets[i];
        
        // -- get secret
        CPubKey pkCoin = CPubKey(&txin.scriptSig[2 + ec_compressed_size * nSecretOffset], ec_compressed_size);
        CKeyID pkId = pkCoin.GetID();
        
        CKey key;
        if (!GetKey(pkId, key))
        {
            sError = "Error: don't have key for output.";
            return false;
        };
        
        ec_secret ecSecret;
        if (key.size() != ec_secret_size)
        {
            sError = "Error: key.size() != ec_secret_size.";
            return false;
        };

        memcpy(&ecSecret.e[0], key.begin(), key.size());

        unsigned char* pPubkeys = &txin.scriptSig[2];
        unsigned char* pSigc    = &txin.scriptSig[2 + ec_compressed_size * nRingSize];
        unsigned char* pSigr    = &txin.scriptSig[2 + (ec_compressed_size + ec_secret_size) * nRingSize];
        if (generateRingSignature(vchImageTest, preimage, nRingSize, nSecretOffset, ecSecret, pPubkeys, pSigc, pSigr) != 0)
        {
            sError = "Error: generateRingSignature() failed.";
            return false;
        };

        memset(&ecSecret.e[0], 0, ec_secret_size); // optimised away?

        // -- test verify
        if (verifyRingSignature(vchImageTest, preimage, nRingSize, pPubkeys, pSigc, pSigr) != 0)
        {
            sError = "Error: verifyRingSignature() failed.";
            return false;
        };
    };

    // -- check if new coins already exist (in case random is broken ?)
    if (!AreOutputsUnique(wtxNew))
    {
        sError = "Error: Anon outputs are not unique - is random working!.";
        return false;
    };

    return true;
};

bool CWallet::SendGameunitsToAnon(CStealthAddress& sxAddress, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee)
{
    if (fDebugRingSig)
        LogPrintf("SendGameunitsToAnon()\n");

    if (IsLocked())
    {
        sError = _("Error: Wallet locked, unable to create transaction.");
        return false;
    };

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    if (fWalletUnlockStakingOnly)
    {
        sError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        return false;
    };

    if (nBestHeight < GetNumBlocksOfPeers()-1)
    {
        sError = _("Error: Block chain must be fully synced first.");
        return false;
    };

    if (vNodes.empty())
    {
        sError = _("Error: Gameunits is not connected!");
        return false;
    };


    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetBalance())
    {
        sError = "Insufficient funds";
        return false;
    };

    wtxNew.nVersion = ANON_TXN_VERSION;

    CScript scriptNarration; // needed to match output id of narr
    std::vector<std::pair<CScript, int64_t> > vecSend;

    if (!CreateAnonOutputs(&sxAddress, nValue, sNarr, vecSend, scriptNarration))
    {
        sError = "CreateAnonOutputs() failed.";
        return false;
    };


    // -- shuffle outputs
    std::random_shuffle(vecSend.begin(), vecSend.end());

    CReserveKey reservekey(this);
    int64_t nFeeRequired;
    int nChangePos;
    if (!CreateTransaction(vecSend, wtxNew, reservekey, nFeeRequired, nChangePos, NULL))
    {
        sError = "CreateTransaction() failed.";
        return false;
    };

    if (scriptNarration.size() > 0)
    {
        for (uint32_t k = 0; k < wtxNew.vout.size(); ++k)
        {
            if (wtxNew.vout[k].scriptPubKey != scriptNarration)
                continue;
            char key[64];
            if (snprintf(key, sizeof(key), "n_%u", k) < 1)
            {
                sError = "Error creating narration key.";
                return false;
            };
            wtxNew.mapValue[key] = sNarr;
            break;
        };
    };

    if (fAskFee && !uiInterface.ThreadSafeAskFee(nFeeRequired, _("Sending...")))
    {
        sError = "ABORTED";
        return false;
    };

    // -- check if new coins already exist (in case random is broken ?)
    if (!AreOutputsUnique(wtxNew))
    {
        sError = "Error: Anon outputs are not unique - is random working!.";
        return false;
    };


    if (!CommitTransaction(wtxNew, reservekey))
    {
        sError = "Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
        UndoAnonTransaction(wtxNew);
        return false;
    };


    return true;
};

bool CWallet::SendAnonToAnon(CStealthAddress& sxAddress, int64_t nValue, int nRingSize, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee)
{
    if (fDebugRingSig)
        LogPrintf("SendAnonToAnon()\n");

    if (IsLocked())
    {
        sError = _("Error: Wallet locked, unable to create transaction.");
        return false;
    };

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    if (fWalletUnlockStakingOnly)
    {
        sError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        return false;
    };

    if (nBestHeight < GetNumBlocksOfPeers()-1)
    {
        sError = _("Error: Block chain must be fully synced first.");
        return false;
    };

    if (vNodes.empty())
    {
        sError = _("Error: Gameunits is not connected!");
        return false;
    };

    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetGameunitsXBalance())
    {
        sError = "Insufficient GAMEUNITSX funds";
        return false;
    };

    wtxNew.nVersion = ANON_TXN_VERSION;

    CScript scriptNarration; // needed to match output id of narr
    std::vector<std::pair<CScript, int64_t> > vecSend;
    std::vector<std::pair<CScript, int64_t> > vecChange;


    if (!CreateAnonOutputs(&sxAddress, nValue, sNarr, vecSend, scriptNarration))
    {
        sError = "CreateAnonOutputs() failed.";
        return false;
    };



    // -- shuffle outputs (any point?)
    //std::random_shuffle(vecSend.begin(), vecSend.end());

    int64_t nFeeRequired;
    std::string sError2;
    if (!AddAnonInputs(nValue, nRingSize, vecSend, vecChange, wtxNew, nFeeRequired, false, sError2))
    {
        LogPrintf("SendAnonToAnon() AddAnonInputs failed %s.\n", sError2.c_str());
        sError = "AddAnonInputs() failed : " + sError2;
        return false;
    };

    if (scriptNarration.size() > 0)
    {
        for (uint32_t k = 0; k < wtxNew.vout.size(); ++k)
        {
            if (wtxNew.vout[k].scriptPubKey != scriptNarration)
                continue;
            char key[64];
            if (snprintf(key, sizeof(key), "n_%u", k) < 1)
            {
                sError = "Error creating narration key.";
                return false;
            };
            wtxNew.mapValue[key] = sNarr;
            break;
        };
    };

    CReserveKey reservekey(this); // not used

    if (!CommitTransaction(wtxNew, reservekey))
    {
        sError = "Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
        UndoAnonTransaction(wtxNew);
        return false;
    };

    return true;
};

bool CWallet::SendAnonToGameunits(CStealthAddress& sxAddress, int64_t nValue, int nRingSize, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee)
{
    if (fDebug)
        LogPrintf("SendAnonToGameunits()\n");

    if (IsLocked())
    {
        sError = _("Error: Wallet locked, unable to create transaction.");
        return false;
    };

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    if (fWalletUnlockStakingOnly)
    {
        sError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        return false;
    };

    if (nBestHeight < GetNumBlocksOfPeers()-1)
    {
        sError = _("Error: Block chain must be fully synced first.");
        return false;
    };

    if (vNodes.empty())
    {
        sError = _("Error: Gameunits is not connected!");
        return false;
    };

    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetGameunitsXBalance())
    {
        sError = "Insufficient GAMEUNITSX funds";
        return false;
    };

    wtxNew.nVersion = ANON_TXN_VERSION;

    std::vector<std::pair<CScript, int64_t> > vecSend;
    std::vector<std::pair<CScript, int64_t> > vecChange;
    std::map<int, std::string> mapStealthNarr;
    if (!CreateStealthOutput(&sxAddress, nValue, sNarr, vecSend, mapStealthNarr, sError))
    {
        LogPrintf("SendCoinsAnon() CreateStealthOutput failed %s.\n", sError.c_str());
        return false;
    };
    std::map<int, std::string>::iterator itN;
    for (itN = mapStealthNarr.begin(); itN != mapStealthNarr.end(); ++itN)
    {
        int pos = itN->first;
        char key[64];
        if (snprintf(key, sizeof(key), "n_%u", pos) < 1)
        {
            LogPrintf("SendCoinsAnon(): Error creating narration key.");
            continue;
        };
        wtxNew.mapValue[key] = itN->second;
    };

    // -- get anon inputs

    int64_t nFeeRequired;
    std::string sError2;
    if (!AddAnonInputs(nValue, nRingSize, vecSend, vecChange, wtxNew, nFeeRequired, false, sError2))
    {
        LogPrintf("SendAnonToAnon() AddAnonInputs failed %s.\n", sError2.c_str());
        sError = "AddAnonInputs() failed: " + sError2;
        return false;
    };

    CReserveKey reservekey(this); // not used

    if (!CommitTransaction(wtxNew, reservekey))
    {
        sError = "Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
        UndoAnonTransaction(wtxNew);
        return false;
    };

    return true;
};


bool CWallet::ExpandLockedAnonOutput(CWalletDB* pwdb, CKeyID& ckeyId, CLockedAnonOutput& lao)
{
    if (fDebugRingSig)
    {
        CBitcoinAddress addrTo(ckeyId);
        LogPrintf("ExpandLockedAnonOutput() %s\n", addrTo.ToString().c_str());
    };
    
    AssertLockHeld(cs_main);
    AssertLockHeld(cs_wallet);
    
    CStealthAddress sxFind;
    sxFind.SetScanPubKey(lao.pkScan);
    
    
    std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxFind);
    if (si == stealthAddresses.end())
    {
        LogPrintf("ExpandLockedAnonOutput - No stealth key found.\n");
        return false;
    };
    
    ec_secret sSpendR;
    ec_secret sSpend;
    ec_secret sScan;
    
    if (si->spend_secret.size() != ec_secret_size
        || si->scan_secret.size() != ec_secret_size)
    {
        LogPrintf("Stealth address has no secret.\n");
        return false;
    };
    
    memcpy(&sScan.e[0], &si->scan_secret[0], ec_secret_size);
    memcpy(&sSpend.e[0], &si->spend_secret[0], ec_secret_size);
    
    ec_point pkEphem;
    pkEphem.resize(lao.pkEphem.size());
    memcpy(&pkEphem[0], lao.pkEphem.begin(), lao.pkEphem.size());
    
    if (StealthSecretSpend(sScan, pkEphem, sSpend, sSpendR) != 0)
    {
        LogPrintf("StealthSecretSpend() failed.\n");
        return false;
    };
    
    ec_point pkTestSpendR;
    if (SecretToPublicKey(sSpendR, pkTestSpendR) != 0)
    {
        LogPrintf("SecretToPublicKey() failed.\n");
        return false;
    };
    
    
    CKey ckey;
    ckey.Set(&sSpendR.e[0], true);
    if (!ckey.IsValid())
    {
        LogPrintf("Reconstructed key is invalid.\n");
        return false;
    };
    
    CPubKey pkCoin = ckey.GetPubKey(true);
    if (!pkCoin.IsValid())
    {
        LogPrintf("pkCoin is invalid.\n");
        return false;
    };
    
    CKeyID keyIDTest = pkCoin.GetID();
    if (keyIDTest != ckeyId)
    {
        LogPrintf("Error: Generated secret does not match.\n");
        if (fDebugRingSig)
        {
            LogPrintf("test   %s\n", keyIDTest.ToString().c_str());
            LogPrintf("gen    %s\n", ckeyId.ToString().c_str());
        };
        return false;
    };
    
    if (fDebugRingSig)
    {
        CBitcoinAddress coinAddress(keyIDTest);
        LogPrintf("Adding secret to key %s.\n", coinAddress.ToString().c_str());
    };
    
    if (!AddKeyInDBTxn(pwdb, ckey))
    {
        LogPrintf("AddKeyInDBTxn failed.\n");
        return false;
    };
    
    // -- store keyimage
    ec_point pkImage;
    if (generateKeyImage(pkTestSpendR, sSpendR, pkImage) != 0)
    {
        LogPrintf("generateKeyImage() failed.\n");
        return false;
    };
    
    bool fSpentAOut = false;
    
    {
        // -- check if this output is already spent
        CTxDB txdb;
        
        CKeyImageSpent kis;
        
        bool fInMemPool;
        if (GetKeyImage(&txdb, pkImage, kis, fInMemPool)
            && !fInMemPool) // shouldn't be possible for kis to be in mempool here
        {
            fSpentAOut = true;
            std::map<uint256, CWalletTx>::iterator mi = mapWallet.find(lao.outpoint.hash);
            if (mi != mapWallet.end())
            {
                CWalletTx& wtx = (*mi).second;
                wtx.MarkSpent(lao.outpoint.n);
                
                if (!pwdb->WriteTx(lao.outpoint.hash, wtx))
                {
                    LogPrintf("ExpandLockedAnonOutput(): WriteTx %s failed.\n", wtx.ToString().c_str());
                    return false;
                };
                
                wtx.MarkDirty();
            };
        };
    }
    
    COwnedAnonOutput oao(lao.outpoint, fSpentAOut);
    if (!pwdb->WriteOwnedAnonOutput(pkImage, oao)
        || !pwdb->WriteOwnedAnonOutputLink(pkCoin, pkImage))
    {
        LogPrintf("WriteOwnedAnonOutput() failed.\n");
        return false;
    };
    
    if (fDebugRingSig)
        LogPrintf("Adding anon output to wallet: %s.\n", HexStr(pkImage).c_str());
    
    return true;
};

bool CWallet::ProcessLockedAnonOutputs()
{
    if (fDebugRingSig)
        LogPrintf("ProcessLockedAnonOutputs()\n");
    
    // -- process owned anon outputs received when wallet was locked.
    
    AssertLockHeld(cs_main);
    AssertLockHeld(cs_wallet);
    
    CWalletDB walletdb(strWalletFile, "cr+");
    walletdb.TxnBegin();
    Dbc* pcursor = walletdb.GetTxnCursor();

    if (!pcursor)
        throw runtime_error("CWallet::ProcessLockedAnonOutputs() : cannot create DB cursor");
    unsigned int fFlags = DB_SET_RANGE;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << std::string("lao");
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        } else
        if (ret != 0)
        {
            pcursor->close();
            throw runtime_error("CWallet::ProcessLockedAnonOutputs() : error scanning DB");
        };

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "lao")
            break;
        CLockedAnonOutput lockedAnonOutput;
        CKeyID ckeyId;
        ssKey >> ckeyId;
        ssValue >> lockedAnonOutput;
        
        if (ExpandLockedAnonOutput(&walletdb, ckeyId, lockedAnonOutput))
        {
            if ((ret = pcursor->del(0)) != 0)
               LogPrintf("Delete failed %d, %s\n", ret, db_strerror(ret));
        };
    };

    pcursor->close();
    
    walletdb.TxnCommit();

    return true;
};

bool CWallet::EstimateAnonFee(int64_t nValue, int nRingSize, std::string& sNarr, CWalletTx& wtxNew, int64_t& nFeeRet, std::string& sError)
{
    if (fDebugRingSig)
        LogPrintf("EstimateAnonFee()\n");

    if (nNodeMode != NT_FULL)
    {
        sError = _("Error: Must be in full mode.");
        return false;
    };

    nFeeRet = 0;

    // -- Check amount
    if (nValue <= 0)
    {
        sError = "Invalid amount";
        return false;
    };

    if (nValue + nTransactionFee > GetGameunitsXBalance())
    {
        sError = "Insufficient GAMEUNITS funds";
        return false;
    };

    CScript scriptNarration; // needed to match output id of narr
    std::vector<std::pair<CScript, int64_t> > vecSend;
    std::vector<std::pair<CScript, int64_t> > vecChange;

    if (!CreateAnonOutputs(NULL, nValue, sNarr, vecSend, scriptNarration))
    {
        sError = "CreateAnonOutputs() failed.";
        return false;
    };

    int64_t nFeeRequired;
    if (!AddAnonInputs(nValue, nRingSize, vecSend, vecChange, wtxNew, nFeeRequired, true, sError))
    {
        LogPrintf("EstimateAnonFee() AddAnonInputs failed %s.\n", sError.c_str());
        sError = "AddAnonInputs() failed.";
        return false;
    };

    nFeeRet = nFeeRequired;

    return true;
};

int CWallet::ListUnspentAnonOutputs(std::list<COwnedAnonOutput>& lUAnonOutputs, bool fMatureOnly)
{
    CWalletDB walletdb(strWalletFile, "r");

    Dbc* pcursor = walletdb.GetAtCursor();
    if (!pcursor)
        throw runtime_error("CWallet::ListUnspentAnonOutputs() : cannot create DB cursor");
    unsigned int fFlags = DB_SET_RANGE;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << std::string("oao");
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        } else
        if (ret != 0)
        {
            pcursor->close();
            throw runtime_error("CWallet::ListUnspentAnonOutputs() : error scanning DB");
        };

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "oao")
            break;
        COwnedAnonOutput oao;
        ssKey >> oao.vchImage;

        ssValue >> oao;

        if (oao.fSpent)
            continue;

        std::map<uint256, CWalletTx>::iterator mi = mapWallet.find(oao.outpoint.hash);
        if (mi == mapWallet.end()
            || mi->second.nVersion != ANON_TXN_VERSION
            || mi->second.vout.size() <= oao.outpoint.n
            || mi->second.IsSpent(oao.outpoint.n))
            continue;

        // -- txn must be in MIN_ANON_SPEND_DEPTH deep in the blockchain to be spent
        if (fMatureOnly
            && mi->second.GetDepthInMainChain() < MIN_ANON_SPEND_DEPTH)
        {
            continue;
        };

        // TODO: check ReadAnonOutput?

        oao.nValue = mi->second.vout[oao.outpoint.n].nValue;


        // -- insert by nValue asc
        bool fInserted = false;
        for (std::list<COwnedAnonOutput>::iterator it = lUAnonOutputs.begin(); it != lUAnonOutputs.end(); ++it)
        {
            if (oao.nValue > it->nValue)
                continue;
            lUAnonOutputs.insert(it, oao);
            fInserted = true;
            break;
        };
        if (!fInserted)
            lUAnonOutputs.push_back(oao);
    };

    pcursor->close();
    return 0;
};

int CWallet::CountAnonOutputs(std::map<int64_t, int>& mOutputCounts, bool fMatureOnly)
{
    LOCK(cs_main);
    CTxDB txdb("r");

    leveldb::DB* pdb = txdb.GetInstance();
    if (!pdb)
        throw runtime_error("CWallet::CountAnonOutputs() : cannot get leveldb instance");

    leveldb::Iterator *iterator = pdb->NewIterator(leveldb::ReadOptions());


    // Seek to start key.
    CPubKey pkZero;
    pkZero.SetZero();

    CDataStream ssStartKey(SER_DISK, CLIENT_VERSION);
    ssStartKey << make_pair(string("ao"), pkZero);
    iterator->Seek(ssStartKey.str());


    while (iterator->Valid())
    {
        // Unpack keys and values.
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;

        if (strType != "ao")
            break;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());

        CAnonOutput anonOutput;
        ssValue >> anonOutput;

        if (!fMatureOnly
            || (anonOutput.nBlockHeight > 0 && nBestHeight - anonOutput.nBlockHeight >= MIN_ANON_SPEND_DEPTH))
        {
            std::map<int64_t, int>::iterator mi = mOutputCounts.find(anonOutput.nValue);
            if (mi != mOutputCounts.end())
                mi->second++;
        };

        iterator->Next();
    };

    delete iterator;

    return 0;
};

int CWallet::CountAllAnonOutputs(std::list<CAnonOutputCount>& lOutputCounts, bool fMatureOnly)
{
    if (fDebugRingSig)
        LogPrintf("CountAllAnonOutputs()\n");

    // TODO: there are few enough possible coin values to preinitialise a vector with all of them

    LOCK(cs_main);
    CTxDB txdb("r");

    leveldb::DB* pdb = txdb.GetInstance();
    if (!pdb)
        throw runtime_error("CWallet::CountAnonOutputs() : cannot get leveldb instance");

    leveldb::Iterator *iterator = pdb->NewIterator(leveldb::ReadOptions());


    // Seek to start key.
    CPubKey pkZero;
    pkZero.SetZero();

    CDataStream ssStartKey(SER_DISK, CLIENT_VERSION);
    ssStartKey << make_pair(string("ao"), pkZero);
    iterator->Seek(ssStartKey.str());


    while (iterator->Valid())
    {
        // Unpack keys and values.
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;

        if (strType != "ao")
            break;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());

        CAnonOutput ao;
        ssValue >> ao;

        int nHeight = ao.nBlockHeight > 0 ? nBestHeight - ao.nBlockHeight : 0;


        if (fMatureOnly
            && nHeight < MIN_ANON_SPEND_DEPTH)
        {
            // -- skip
        } else
        {
            // -- insert by nValue asc
            bool fProcessed = false;
            for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
            {
                if (ao.nValue == it->nValue)
                {
                    it->nExists++;
                    if (it->nLeastDepth > nHeight)
                        it->nLeastDepth = nHeight;
                    fProcessed = true;
                    break;
                };
                if (ao.nValue > it->nValue)
                    continue;
                lOutputCounts.insert(it, CAnonOutputCount(ao.nValue, 1, 0, 0, nHeight));
                fProcessed = true;
                break;
            };
            if (!fProcessed)
                lOutputCounts.push_back(CAnonOutputCount(ao.nValue, 1, 0, 0, nHeight));
        };

        iterator->Next();
    };

    delete iterator;


    // -- count spends

    iterator = pdb->NewIterator(leveldb::ReadOptions());
    ssStartKey.clear();
    ssStartKey << make_pair(string("ki"), pkZero);
    iterator->Seek(ssStartKey.str());

    while (iterator->Valid())
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());
        string strType;
        ssKey >> strType;

        if (strType != "ki")
            break;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.write(iterator->value().data(), iterator->value().size());

        CKeyImageSpent kis;
        ssValue >> kis;


        bool fProcessed = false;
        for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
        {
            if (kis.nValue != it->nValue)
                continue;
            it->nSpends++;
            fProcessed = true;
            break;
        };
        if (!fProcessed)
            LogPrintf("WARNING: CountAllAnonOutputs found keyimage without matching anon output value.\n");

        iterator->Next();
    };

    delete iterator;

    return 0;
};

int CWallet::CountOwnedAnonOutputs(std::map<int64_t, int>& mOwnedOutputCounts, bool fMatureOnly)
{
    if (fDebugRingSig)
        LogPrintf("CountOwnedAnonOutputs()\n");

    CWalletDB walletdb(strWalletFile, "r");

    Dbc* pcursor = walletdb.GetAtCursor();
    if (!pcursor)
        throw runtime_error("CWallet::CountOwnedAnonOutputs() : cannot create DB cursor");
    unsigned int fFlags = DB_SET_RANGE;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << std::string("oao");
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        } else
        if (ret != 0)
        {
            pcursor->close();
            throw runtime_error("CWallet::CountOwnedAnonOutputs() : error scanning DB");
        };

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "oao")
            break;
        COwnedAnonOutput oao;
        ssKey >> oao.vchImage;

        ssValue >> oao;

        if (oao.fSpent)
            continue;

        std::map<uint256, CWalletTx>::iterator mi = mapWallet.find(oao.outpoint.hash);
        if (mi == mapWallet.end()
            || mi->second.nVersion != ANON_TXN_VERSION
            || mi->second.vout.size() <= oao.outpoint.n
            || mi->second.IsSpent(oao.outpoint.n))
            continue;

        //LogPrintf("[rem] mi->second.GetDepthInMainChain() %d \n", mi->second.GetDepthInMainChain());
        //LogPrintf("[rem] mi->second.hashBlock %s \n", mi->second.hashBlock.ToString().c_str());
        // -- txn must be in MIN_ANON_SPEND_DEPTH deep in the blockchain to be spent
        
        {
            LOCK(cs_main);
            if (fMatureOnly
                && mi->second.GetDepthInMainChain() < MIN_ANON_SPEND_DEPTH)
            {
                continue;
            };
        }

        // TODO: check ReadAnonOutput?

        oao.nValue = mi->second.vout[oao.outpoint.n].nValue;

        mOwnedOutputCounts[oao.nValue]++;
    };

    pcursor->close();
    return 0;
};

bool CWallet::EraseAllAnonData()
{
    LogPrintf("EraseAllAnonData()\n");

    LOCK2(cs_main, cs_wallet);
    CWalletDB walletdb(strWalletFile, "cr+");
    CTxDB txdb("cr+");

    string strType;
    txdb.TxnBegin();
    leveldb::DB* pdb = txdb.GetInstance();
    if (!pdb)
        throw runtime_error("EraseAllAnonData() : cannot get leveldb instance");

    leveldb::Iterator *iterator = pdb->NewIterator(leveldb::ReadOptions());

    iterator->SeekToFirst();
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;
    while (iterator->Valid())
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.write(iterator->key().data(), iterator->key().size());

        ssKey >> strType;

        if (strType == "ao"
            || strType == "ki")
        {
            LogPrintf("Erasing from txdb %s\n", strType.c_str());
            leveldb::Status s = pdb->Delete(writeOptions, iterator->key());

            if (!s.ok())
                LogPrintf("EraseAllAnonData() erase failed: %s\n", s.ToString().c_str());
        };

        iterator->Next();
    };

    delete iterator;
    txdb.TxnCommit();


    walletdb.TxnBegin();
    Dbc* pcursor = walletdb.GetTxnCursor();

    if (!pcursor)
        throw runtime_error("EraseAllAnonData() : cannot create DB cursor");
    unsigned int fFlags = DB_NEXT;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = walletdb.ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
        {
            break;
        } else
        if (ret != 0)
        {
            pcursor->close();
            throw runtime_error("EraseAllAnonData() : error scanning DB");
        };

        ssKey >> strType;
        if (strType == "lao"
            || strType == "oao"
            || strType == "oal")
        {
            LogPrintf("Erasing from walletdb %s\n", strType.c_str());
            //continue;
            if ((ret = pcursor->del(0)) != 0)
               LogPrintf("Delete failed %d, %s\n", ret, db_strerror(ret));
        };
    };

    pcursor->close();

    walletdb.TxnCommit();


    return true;
};

bool CWallet::CacheAnonStats()
{
    if (fDebugRingSig)
        LogPrintf("CacheAnonStats()\n");

    mapAnonOutputStats.clear();

    std::list<CAnonOutputCount> lOutputCounts;
    if (CountAllAnonOutputs(lOutputCounts, false) != 0)
    {
        LogPrintf("Error: CountAllAnonOutputs() failed.\n");
    } else
    {
        for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
            mapAnonOutputStats[it->nValue].set(
                it->nValue, it->nExists, it->nSpends, it->nOwned,
                it->nLeastDepth < 1 ? 0 : nBestHeight - it->nLeastDepth); // mapAnonOutputStats stores height in chain instead of depth
    };

    return true;
};

bool CWallet::InitBloomFilter()
{
    if (fDebug)
        LogPrintf("Initialising bloom filter, max elements %d.\n", nBloomFilterElements);

    LOCK(cs_wallet);

    if (pBloomFilter)
        return error("Bloom filter already created.");

    char nFlags = BLOOM_UPDATE_ALL;

    if (nLocalRequirements & THIN_STEALTH)
        nFlags |= BLOOM_ACCEPT_STEALTH;
    pBloomFilter = new CBloomFilter(nBloomFilterElements, 0.001, GetRandUInt32(), nFlags);

    if (!pBloomFilter)
        return error("Bloom filter new failed.");

    if (!pBloomFilter->IsWithinSizeConstraints())
    {
        delete pBloomFilter;
        pBloomFilter = NULL;
        return error("Bloom filter is too large.");
    };

    std::string sAnonPrefix("ao ");

    // TODO: don't load addresses created from receiving stealth txns
    // TODO: exclude change addresses of spent outputs
    std::set<CKeyID> setKeys;
    GetKeys(setKeys);

    uint32_t nKeysAdded = 0;

    // -- need to add change addresses too
    BOOST_FOREACH(const CKeyID &keyId, setKeys)
    {
        // -- don't add keys generated for aonon outputs (marked with label prefix "ao ")
        std::map<CTxDestination, std::string>::iterator mi(mapAddressBook.find(keyId));
        if (mi != mapAddressBook.end() && mi->second.compare(0, sAnonPrefix.length(), sAnonPrefix) == 0)
        {
            if (fDebugRingSig)
            {
                CBitcoinAddress coinAddress(keyId);
                LogPrintf("InitBloomFilter() - ignoring key for anon output %s.\n", coinAddress.ToString().c_str());
            };
            continue;
        };

        pBloomFilter->UpdateEmptyFull();
        if (pBloomFilter->IsFull())
        {
            // TODO: try resize?
            LogPrintf("Error: InitBloomFilter() - Filter is full.\n");
            continue; // continue so more messages show in log
        };

        if (fDebug)
        {
            CBitcoinAddress coinAddress(keyId);
            if (coinAddress.IsValid())
                LogPrintf("Adding key: %s.\n", coinAddress.ToString().c_str());
        };

        vector<unsigned char> vData(keyId.begin(), keyId.end());

        pBloomFilter->insert(vData);

        nKeysAdded++;
    };

    setKeys.clear();


    // - load unspent outputs
    uint32_t nOutPointsAdded = 0;

    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        CWalletTx& wtx = (*it).second;

        // -- add unspent outputs to bloom filters
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            if (wtx.nVersion == ANON_TXN_VERSION
                && txin.IsAnonInput())
                continue;

            std::map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi == mapWallet.end())
                continue;

            CWalletTx& wtxPrev = (*mi).second;

            if (txin.prevout.n >= wtxPrev.vout.size())
            {
                LogPrintf("InitBloomFilter(): bad wtx %s\n", wtxPrev.GetHash().ToString().c_str());
            } else
            //if (!wtxPrev.IsSpent(txin.prevout.n) && IsMine(wtxPrev.vout[txin.prevout.n]))
            if (!wtxPrev.IsSpent(txin.prevout.n))
            {
                CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
                stream << txin.prevout;
                std::vector<unsigned char> vData(stream.begin(), stream.end());
                pBloomFilter->insert(vData);
                nOutPointsAdded++;
            };

        };
    };

    if (fDebug)
    {
        LogPrintf("Added %u key%s, %u outpoint%s to filter.\n",
            nKeysAdded, nKeysAdded != 1 ? "s": "",
            nOutPointsAdded, nOutPointsAdded != 1 ? "s": "");
        LogPrintf("Filter bytes: %u.\n", pBloomFilter->GetSize());
    };

    return true;
};






// NovaCoin: get current stake weight posv2
uint64_t CWallet::GetStakeWeight() const
{
    // Choose coins to use
    int64_t nBalance = GetBalance();

    if (nBalance <= nReserveBalance)
        return false;

    vector<const CWalletTx*> vwtxPrev;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    int64_t nValueIn = 0;


    if (!SelectCoinsForStaking(nBalance - nReserveBalance, GetTime(), setCoins, nValueIn)
      || setCoins.empty())
        return false;

    uint64_t nWeight = 0;
    int64_t nCurrentTime = GetTime();
    
    CTxDB txdb("r");
    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        {
            LOCK2(cs_main, cs_wallet);
            if (nNodeMode == NT_THIN)
            {
                // -- check txn is in chain
                std::map<uint256, CBlockThinIndex*>::iterator mi (mapBlockThinIndex.find(pcoin.first->hashBlock));
                if (mi == mapBlockThinIndex.end())
                {
                    if (fThinFullIndex
                        || !pindexRear)
                        continue;

                    CDiskBlockThinIndex diskindex;
                    if (!txdb.ReadBlockThinIndex(pcoin.first->hashBlock, diskindex)
                        || diskindex.hashNext == 0)
                        continue;
                };
            } else
            {
                CTxIndex txindex;
                if (!txdb.ReadTxIndex(pcoin.first->GetHash(), txindex))
                    continue;
            }
        }

        if (nCurrentTime - pcoin.first->nTime > nStakeMinAge)
            nWeight += pcoin.first->vout[pcoin.second].nValue;
    };

    return nWeight;
}

bool CWallet::CreateCoinStake(const CKeyStore& keystore, unsigned int nBits, int64_t nSearchInterval, int64_t nFees, CTransaction& txNew, CKey& key)
{
    CBlockIndex* pindexPrev = pindexBest;
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    txNew.vin.clear();
    txNew.vout.clear();

    // Mark coin stake transaction
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));
    
    // Choose coins to use
    int64_t nBalance = GetBalance();
    
    if (nBalance <= nReserveBalance)
        return false;
    
    vector<const CWalletTx*> vwtxPrev;
    
    set<pair<const CWalletTx*,unsigned int> > setCoins;
    int64_t nValueIn = 0;

    // Select coins with suitable depth
    if (!SelectCoinsForStaking(nBalance - nReserveBalance, txNew.nTime, setCoins, nValueIn))
        return false;
    
    if (setCoins.empty())
        return false;
    
    int64_t nCredit = 0;
    CScript scriptPubKeyKernel;
    CTxDB txdb("r");
    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        boost::this_thread::interruption_point();
        static int nMaxStakeSearchInterval = 60;
        
        bool fKernelFound = false;
        for (unsigned int n=0; n<min(nSearchInterval,(int64_t)nMaxStakeSearchInterval) && !fKernelFound && pindexPrev == pindexBest; n++)
        {
            boost::this_thread::interruption_point();
            // Search backward in time from the given txNew timestamp
            // Search nSearchInterval seconds back up to nMaxStakeSearchInterval
            COutPoint prevoutStake = COutPoint(pcoin.first->GetHash(), pcoin.second);

            int64_t nBlockTime;
            if (CheckKernel(pindexPrev, nBits, txNew.nTime - n, prevoutStake, &nBlockTime))
            {
                // Found a kernel
                if (fDebugPoS)
                    LogPrintf("CreateCoinStake : kernel found\n");

                vector<valtype> vSolutions;
                txnouttype whichType;
                CScript scriptPubKeyOut;
                scriptPubKeyKernel = pcoin.first->vout[pcoin.second].scriptPubKey;

                if (!Solver(scriptPubKeyKernel, whichType, vSolutions))
                {
                    if (fDebugPoS)
                        LogPrintf("CreateCoinStake : failed to parse kernel\n");
                    break;
                };

                if (fDebugPoS)
                    LogPrintf("CreateCoinStake : parsed kernel type=%d\n", whichType);

                if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH)
                {
                    if (fDebugPoS)
                        LogPrintf("CreateCoinStake : no support for kernel type=%d\n", whichType);
                    break;  // only support pay to public key and pay to address
                };

                if (whichType == TX_PUBKEYHASH) // pay to address type
                {
                    // convert to pay to public key type
                    if (!keystore.GetKey(uint160(vSolutions[0]), key))
                    {
                        if (fDebugPoS)
                            LogPrintf("CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
                        break;  // unable to find corresponding public key
                    };
                    scriptPubKeyOut << key.GetPubKey() << OP_CHECKSIG;
                };

                if (whichType == TX_PUBKEY)
                {
                    valtype& vchPubKey = vSolutions[0];
                    if (!keystore.GetKey(Hash160(vchPubKey), key))
                    {
                        if (fDebugPoS)
                            LogPrintf("CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
                        break;  // unable to find corresponding public key
                    };

                    if (key.GetPubKey() != vchPubKey)
                    {
                        if (fDebugPoS)
                            LogPrintf("CreateCoinStake : invalid key for kernel type=%d\n", whichType);
                        break; // keys mismatch
                    };

                    scriptPubKeyOut = scriptPubKeyKernel;
                };

                txNew.nTime -= n;
                txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
                nCredit += pcoin.first->vout[pcoin.second].nValue;
                vwtxPrev.push_back(pcoin.first);
                txNew.vout.push_back(CTxOut(0, scriptPubKeyOut));

                if (GetWeight(nBlockTime, (int64_t)txNew.nTime) < nStakeSplitAge)
                    txNew.vout.push_back(CTxOut(0, scriptPubKeyOut)); //split stake

                if (fDebugPoS)
                    LogPrintf("CreateCoinStake : added kernel type=%d\n", whichType);
                fKernelFound = true;
                break;
            };
        };

        if (fKernelFound)
            break; // if kernel is found stop searching
    }

    if (nCredit == 0 || nCredit > nBalance - nReserveBalance)
        return false;

    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        // Attempt to add more inputs
        // Only add coins of the same key/address as kernel
        if (txNew.vout.size() == 2 && ((pcoin.first->vout[pcoin.second].scriptPubKey == scriptPubKeyKernel || pcoin.first->vout[pcoin.second].scriptPubKey == txNew.vout[1].scriptPubKey))
            && pcoin.first->GetHash() != txNew.vin[0].prevout.hash)
        {
            int64_t nTimeWeight = GetWeight((int64_t)pcoin.first->nTime, (int64_t)txNew.nTime);

            // Stop adding more inputs if already too many inputs
            if (txNew.vin.size() >= 100)
                break;
            // Stop adding more inputs if value is already pretty significant
            if (nCredit >= nStakeCombineThreshold)
                break;
            // Stop adding inputs if reached reserve limit
            if (nCredit + pcoin.first->vout[pcoin.second].nValue > nBalance - nReserveBalance)
                break;
            // Do not add additional significant input
            if (pcoin.first->vout[pcoin.second].nValue >= nStakeCombineThreshold)
                continue;
            // Do not add input that is still too young
            if (nTimeWeight < nStakeMinAge)
                continue;

            txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
            nCredit += pcoin.first->vout[pcoin.second].nValue;
            vwtxPrev.push_back(pcoin.first);
        };
    };

    // Calculate coin age reward
    {
        uint64_t nCoinAge;
        CTxDB txdb("r");
        if (!txNew.GetCoinAge(txdb, nCoinAge))
            return error("CreateCoinStake : failed to calculate coin age");

        int64_t nReward = Params().GetProofOfStakeReward(nCoinAge, nFees);
        if (nReward <= 0)
            return false;

        nCredit += nReward;
    }

    // Set output amount
    if (txNew.vout.size() == 3)
    {
        txNew.vout[1].nValue = (nCredit / 2 / CENT) * CENT;
        txNew.vout[2].nValue = nCredit - txNew.vout[1].nValue;
    } else
    {
        txNew.vout[1].nValue = nCredit;
    };

    // Sign
    int nIn = 0;
    BOOST_FOREACH(const CWalletTx* pcoin, vwtxPrev)
    {
        if (!SignSignature(*this, *pcoin, txNew, nIn++))
            return error("CreateCoinStake : failed to sign coinstake");
    };

    // Limit size
    unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
    if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
        return error("CreateCoinStake : exceeded coinstake size limit");

    // Successfully generated coinstake
    return true;
}


// Call after CreateTransaction unless you want to abort
bool CWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey)
{
    
    if (!wtxNew.CheckTransaction())
    {
        LogPrintf("CommitTransaction: CheckTransaction() failed %s\n", wtxNew.GetHash().ToString().c_str());
        return false;
    };
    
    mapValue_t mapNarr;
    FindStealthTransactions(wtxNew, mapNarr);

    bool fIsMine = false;
    if (wtxNew.nVersion == ANON_TXN_VERSION)
    {
        LOCK2(cs_main, cs_wallet);
        CWalletDB walletdb(strWalletFile, "cr+");
        CTxDB txdb("cr+");

        walletdb.TxnBegin();
        txdb.TxnBegin();
        std::vector<std::map<uint256, CWalletTx>::iterator> vUpdatedTxns;
        if (!ProcessAnonTransaction(&walletdb, &txdb, wtxNew, wtxNew.hashBlock, fIsMine, mapNarr, vUpdatedTxns))
        {
            LogPrintf("CommitTransaction: ProcessAnonTransaction() failed %s\n", wtxNew.GetHash().ToString().c_str());
            walletdb.TxnAbort();
            txdb.TxnAbort();
            return false;
        } else
        {
            walletdb.TxnCommit();
            txdb.TxnCommit();
            for (std::vector<std::map<uint256, CWalletTx>::iterator>::iterator it = vUpdatedTxns.begin();
                it != vUpdatedTxns.end(); ++it)
                NotifyTransactionChanged(this, (*it)->first, CT_UPDATED);
        };
    };

    if (!mapNarr.empty())
    {
        BOOST_FOREACH(const PAIRTYPE(string,string)& item, mapNarr)
            wtxNew.mapValue[item.first] = item.second;
    };

    {
        LOCK2(cs_main, cs_wallet);
        LogPrintf("CommitTransaction:\n%s", wtxNew.ToString().c_str());

        {
            // This is only to keep the database open to defeat the auto-flush for the
            // duration of this scope.  This is the only place where this optimization
            // maybe makes sense; please don't do it anywhere else.
            CWalletDB* pwalletdb = fFileBacked ? new CWalletDB(strWalletFile,"r") : NULL;

            // Take key pair from key pool so it won't be used again
            reservekey.KeepKey();

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            uint256 hash = wtxNew.GetHash();
            AddToWallet(wtxNew, hash);

            // Mark old coins as spent
            set<CWalletTx*> setCoins;
            BOOST_FOREACH(const CTxIn& txin, wtxNew.vin)
            {
                if (wtxNew.nVersion == ANON_TXN_VERSION
                    && txin.IsAnonInput())
                    continue;
                CWalletTx &coin = mapWallet[txin.prevout.hash];
                coin.BindWallet(this);
                coin.MarkSpent(txin.prevout.n);
                coin.WriteToDisk();
                NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
            };

            if (fFileBacked)
                delete pwalletdb;
        }

        // Track how many getdata requests our transaction gets
        mapRequestCount[wtxNew.GetHash()] = 0;

        // Broadcast
        if (!wtxNew.AcceptToMemoryPool())
        {
            // This must not fail. The transaction has already been signed and recorded.
            LogPrintf("CommitTransaction() : Error: Transaction not valid\n");
            return false;
        };
        wtxNew.RelayWalletTransaction();

        // -- look for new change addresses to add to the bloom filters
        if (pBloomFilter)
        {
            BOOST_FOREACH(CTxOut txout, wtxNew.vout)
            {
                if (wtxNew.nVersion == ANON_TXN_VERSION
                    && txout.IsAnonOutput())
                    continue;

                if (IsChange(txout))
                {
                    CTxDestination txoutAddr;
                    if (!ExtractDestination(txout.scriptPubKey, txoutAddr))
                        continue;
                    AddKeyToMerkleFilters(txoutAddr);
                };
            };
        }
    }
    return true;
}




string CWallet::SendMoney(CScript scriptPubKey, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee)
{
    CReserveKey reservekey(this);
    int64_t nFeeRequired;

    if (IsLocked())
    {
        string strError = _("Error: Wallet locked, unable to create transaction  ");
        LogPrintf("SendMoney() : %s", strError.c_str());
        return strError;
    };

    if (fWalletUnlockStakingOnly)
    {
        string strError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        LogPrintf("SendMoney() : %s", strError.c_str());
        return strError;
    };

    if (!CreateTransaction(scriptPubKey, nValue, sNarr, wtxNew, reservekey, nFeeRequired))
    {
        string strError;
        if (nValue + nFeeRequired > GetBalance())
            strError = strprintf(_("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds  "), FormatMoney(nFeeRequired).c_str());
        else
            strError = _("Error: Transaction creation failed  ");
        LogPrintf("SendMoney() : %s", strError.c_str());
        return strError;
    };

    if (fAskFee && !uiInterface.ThreadSafeAskFee(nFeeRequired, _("Sending...")))
        return "ABORTED";

    if (!CommitTransaction(wtxNew, reservekey))
        return _("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    return "";
}



string CWallet::SendMoneyToDestination(const CTxDestination& address, int64_t nValue, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee)
{
    // Check amount
    if (nValue <= 0)
        return _("Invalid amount");
    if (nValue + nTransactionFee > GetBalance())
        return _("Insufficient funds");

    if (sNarr.length() > 24)
        return _("Narration must be 24 characters or less.");

    // Parse Bitcoin address
    CScript scriptPubKey;
    scriptPubKey.SetDestination(address);

    return SendMoney(scriptPubKey, nValue, sNarr, wtxNew, fAskFee);
}




DBErrors CWallet::LoadWallet(bool& fFirstRunRet)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    fFirstRunRet = false;
    DBErrors nLoadWalletRet = CWalletDB(strWalletFile,"cr+").LoadWallet(this);
    if (nLoadWalletRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // the requires a new key.
        };
    };

    if (nLoadWalletRet != DB_LOAD_OK)
        return nLoadWalletRet;
    fFirstRunRet = !vchDefaultKey.IsValid();
    return DB_LOAD_OK;
}


bool CWallet::SetAddressBookName(const CTxDestination& address, const string& strName, CWalletDB* pwdb, bool fAddKeyToMerkleFilters)
{
    bool fOwned;
    ChangeType nMode;

    {
        LOCK(cs_wallet); // mapAddressBook
        std::map<CTxDestination, std::string>::iterator mi = mapAddressBook.find(address);
        nMode = (mi == mapAddressBook.end()) ? CT_NEW : CT_UPDATED;
        fOwned = ::IsMine(*this, address);

        mapAddressBook[address] = strName;
    }

    // -- fAddKeyToMerkleFilters is always false when adding keys for anonoutputs
    if (fOwned
        && fAddKeyToMerkleFilters)
    {
        const CBitcoinAddress& caddress = address;
        SecureMsgWalletKeyChanged(caddress.ToString(), strName, nMode);
    };

    if (nMode == CT_NEW
        && fAddKeyToMerkleFilters)
    {
        AddKeyToMerkleFilters(address);
    };

    NotifyAddressBookChanged(this, address, strName, fOwned, nMode);

    if (!fFileBacked)
        return false;

    if (!pwdb)
        return CWalletDB(strWalletFile).WriteName(CBitcoinAddress(address).ToString(), strName);
    return pwdb->WriteName(CBitcoinAddress(address).ToString(), strName);
}

bool CWallet::DelAddressBookName(const CTxDestination& address)
{
    if (address.type() == typeid(CStealthAddress))
    {
        const CStealthAddress& sxAddr = boost::get<CStealthAddress>(address);
        bool fOwned; // must check on copy from wallet

        {
            LOCK(cs_wallet);

            std::set<CStealthAddress>::iterator si = stealthAddresses.find(sxAddr);
            if (si == stealthAddresses.end())
            {
                LogPrintf("Error: DelAddressBookName() stealth address not found in wallet.\n");
                return false;
            };

            fOwned = si->scan_secret.size() < 32 ? false : true;

            if (stealthAddresses.erase(sxAddr) < 1
                || !CWalletDB(strWalletFile).EraseStealthAddress(sxAddr))
            {
                LogPrintf("Error: DelAddressBookName() remove stealthAddresses failed.\n");
                return false;
            };
        }

        NotifyAddressBookChanged(this, address, "", fOwned, CT_DELETED);
        return true;
    };


    {
        LOCK(cs_wallet); // mapAddressBook

        mapAddressBook.erase(address);
    }

    bool fOwned = ::IsMine(*this, address);
    string sName = "";

    if (fOwned)
    {
        const CBitcoinAddress& caddress = address;
        SecureMsgWalletKeyChanged(caddress.ToString(), sName, CT_DELETED);
    };

    NotifyAddressBookChanged(this, address, "", fOwned, CT_DELETED);

    if (!fFileBacked)
        return false;
    return CWalletDB(strWalletFile).EraseName(CBitcoinAddress(address).ToString());
}


void CWallet::PrintWallet(const CBlock& block)
{
    {
        LOCK(cs_wallet);
        if (block.IsProofOfWork() && mapWallet.count(block.vtx[0].GetHash()))
        {
            CWalletTx& wtx = mapWallet[block.vtx[0].GetHash()];
            LogPrintf("    mine:  %d  %d  %d", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
        };

        if (block.IsProofOfStake() && mapWallet.count(block.vtx[1].GetHash()))
        {
            CWalletTx& wtx = mapWallet[block.vtx[1].GetHash()];
            LogPrintf("    stake: %d  %d  %d", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
        };

    }
    LogPrintf("\n");
}

bool CWallet::GetTransaction(const uint256 &hashTx, CWalletTx& wtx)
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
        {
            wtx = (*mi).second;
            return true;
        };
    }
    return false;
}

bool CWallet::SetDefaultKey(const CPubKey &vchPubKey)
{
    if (fFileBacked)
    {
        if (!CWalletDB(strWalletFile).WriteDefaultKey(vchPubKey))
            return false;
    };
    vchDefaultKey = vchPubKey;
    return true;
}

bool GetWalletFile(CWallet* pwallet, string &strWalletFileOut)
{
    if (!pwallet->fFileBacked)
        return false;
    strWalletFileOut = pwallet->strWalletFile;
    return true;
}

//
// Mark old keypool keys as used,
// and generate all new keys
//
bool CWallet::NewKeyPool()
{
    {
        LOCK(cs_wallet);
        CWalletDB walletdb(strWalletFile);
        BOOST_FOREACH(int64_t nIndex, setKeyPool)
            walletdb.ErasePool(nIndex);
        setKeyPool.clear();

        if (IsLocked())
            return false;

        int64_t nKeys = max(GetArg("-keypool", 100), (int64_t)0);
        for (int i = 0; i < nKeys; i++)
        {
            int64_t nIndex = i+1;
            walletdb.WritePool(nIndex, CKeyPool(GenerateNewKey()));
            setKeyPool.insert(nIndex);
        };
        LogPrintf("CWallet::NewKeyPool wrote %d new keys\n", nKeys);
    }
    return true;
}

bool CWallet::TopUpKeyPool(unsigned int nSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked())
            return false;

        CWalletDB walletdb(strWalletFile);

        // Top up key pool
        unsigned int nTargetSize;
        if (nSize > 0)
            nTargetSize = nSize;
        else
            nTargetSize = max(GetArg("-keypool", 100), (int64_t)0);

        while (setKeyPool.size() < (nTargetSize + 1))
        {
            int64_t nEnd = 1;
            if (!setKeyPool.empty())
                nEnd = *(--setKeyPool.end()) + 1;
            if (!walletdb.WritePool(nEnd, CKeyPool(GenerateNewKey())))
                throw runtime_error("TopUpKeyPool() : writing generated key failed");
            setKeyPool.insert(nEnd);
            LogPrintf("keypool added key %d, size=%u\n", nEnd, setKeyPool.size());
        };
    }
    return true;
}

void CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_wallet);

        if (!IsLocked())
            TopUpKeyPool();

        // Get the oldest key
        if(setKeyPool.empty())
            return;

        CWalletDB walletdb(strWalletFile);

        nIndex = *(setKeyPool.begin());
        setKeyPool.erase(setKeyPool.begin());
        if (!walletdb.ReadPool(nIndex, keypool))
            throw runtime_error("ReserveKeyFromKeyPool() : read failed");
        if (!HaveKey(keypool.vchPubKey.GetID()))
            throw runtime_error("ReserveKeyFromKeyPool() : unknown key in key pool");
        assert(keypool.vchPubKey.IsValid());
        if (fDebug && GetBoolArg("-printkeypool"))
            LogPrintf("keypool reserve %d\n", nIndex);
    }
}

int64_t CWallet::AddReserveKey(const CKeyPool& keypool)
{
    {
        LOCK2(cs_main, cs_wallet);
        CWalletDB walletdb(strWalletFile);

        int64_t nIndex = 1 + *(--setKeyPool.end());
        if (!walletdb.WritePool(nIndex, keypool))
            throw runtime_error("AddReserveKey() : writing added key failed");
        setKeyPool.insert(nIndex);
        return nIndex;
    }
    return -1;
}

void CWallet::KeepKey(int64_t nIndex)
{
    // Remove from key pool
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.ErasePool(nIndex);
    };

    if(fDebug)
        LogPrintf("keypool keep %d\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex)
{
    // Return to key pool
    {
        LOCK(cs_wallet);
        setKeyPool.insert(nIndex);
    }
    if(fDebug)
        LogPrintf("keypool return %d\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey& result, bool fAllowReuse)
{
    int64_t nIndex = 0;
    CKeyPool keypool;

    {
        LOCK(cs_wallet);
        ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex == -1)
        {
            if (fAllowReuse && vchDefaultKey.IsValid())
            {
                result = vchDefaultKey;
                return true;
            };

            if (IsLocked())
                return false;

            result = GenerateNewKey();
            return true;
        };
        KeepKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

int64_t CWallet::GetOldestKeyPoolTime()
{
    int64_t nIndex = 0;
    CKeyPool keypool;
    ReserveKeyFromKeyPool(nIndex, keypool);
    if (nIndex == -1)
        return GetTime();
    ReturnKey(nIndex);
    return keypool.nTime;
}

std::map<CTxDestination, int64_t> CWallet::GetAddressBalances()
{
    map<CTxDestination, int64_t> balances;

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
        {
            CWalletTx *pcoin = &walletEntry.second;

            if (!pcoin->IsFinal() || !pcoin->IsTrusted())
                continue;

            if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < (pcoin->IsFromMe() ? 0 : 1))
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(pcoin->vout[i]))
                    continue;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, addr))
                    continue;

                int64_t n = pcoin->IsSpent(i) ? 0 : pcoin->vout[i].nValue;

                if (!balances.count(addr))
                    balances[addr] = 0;
                balances[addr] += n;
            }
        }
    }

    return balances;
}

set< set<CTxDestination> > CWallet::GetAddressGroupings()
{
    AssertLockHeld(cs_wallet); // mapWallet
    set< set<CTxDestination> > groupings;
    set<CTxDestination> grouping;

    BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
    {
        CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->vin.size() > 0 && IsMine(pcoin->vin[0]))
        {
            // group all input addresses with each other
            BOOST_FOREACH(CTxIn txin, pcoin->vin)
            {
                CTxDestination address;
                if(!ExtractDestination(mapWallet[txin.prevout.hash].vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
            }

            // group change with input addresses
            BOOST_FOREACH(CTxOut txout, pcoin->vout)
            {
                if (IsChange(txout))
                {
                    CWalletTx tx = mapWallet[pcoin->vin[0].prevout.hash];
                    CTxDestination txoutAddr;
                    if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                        continue;
                    grouping.insert(txoutAddr);
                };
            };
            groupings.insert(grouping);
            grouping.clear();
        };

        // group lone addrs by themselves
        for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            if (IsMine(pcoin->vout[i]))
            {
                CTxDestination address;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            };
    };

    set< set<CTxDestination>* > uniqueGroupings; // a set of pointers to groups of addresses
    map< CTxDestination, set<CTxDestination>* > setmap;  // map addresses to the unique group containing it
    BOOST_FOREACH(set<CTxDestination> grouping, groupings)
    {
        // make a set of all the groups hit by this new group
        set< set<CTxDestination>* > hits;
        map< CTxDestination, set<CTxDestination>* >::iterator it;
        BOOST_FOREACH(CTxDestination address, grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);

        // merge all hit groups into a new single group and delete old groups
        set<CTxDestination>* merged = new set<CTxDestination>(grouping);
        BOOST_FOREACH(set<CTxDestination>* hit, hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        };
        uniqueGroupings.insert(merged);

        // update setmap
        BOOST_FOREACH(CTxDestination element, *merged)
            setmap[element] = merged;
    };

    set< set<CTxDestination> > ret;
    BOOST_FOREACH(set<CTxDestination>* uniqueGrouping, uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    };

    return ret;
}

// ppcoin: check 'spent' consistency between wallet and txindex
// ppcoin: fix wallet spent state according to txindex
void CWallet::FixSpentCoins(int& nMismatchFound, int64_t& nBalanceInQuestion, bool fCheckOnly)
{
    if (nNodeMode != NT_FULL)
    {
        LogPrintf("FixSpentCoins must be run in full mode.\n");
        return;
    };

    nMismatchFound = 0;
    nBalanceInQuestion = 0;

    LOCK(cs_wallet);
    vector<CWalletTx*> vCoins;
    vCoins.reserve(mapWallet.size());
    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        vCoins.push_back(&(*it).second);

    CTxDB txdb("r");
    BOOST_FOREACH(CWalletTx* pcoin, vCoins)
    {
        // Find the corresponding transaction index
        CTxIndex txindex;
        if (!txdb.ReadTxIndex(pcoin->GetHash(), txindex))
            continue;
        for (unsigned int n=0; n < pcoin->vout.size(); n++)
        {
            if (IsMine(pcoin->vout[n]) && pcoin->IsSpent(n) && (txindex.vSpent.size() <= n || txindex.vSpent[n].IsNull()))
            {
                LogPrintf("FixSpentCoins found lost coin %s GAMEUNITS %s[%d], %s\n",
                    FormatMoney(pcoin->vout[n].nValue).c_str(), pcoin->GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                nMismatchFound++;
                nBalanceInQuestion += pcoin->vout[n].nValue;
                if (!fCheckOnly)
                {
                    pcoin->MarkUnspent(n);
                    pcoin->WriteToDisk();
                };
            } else
            if (IsMine(pcoin->vout[n]) && !pcoin->IsSpent(n) && (txindex.vSpent.size() > n && !txindex.vSpent[n].IsNull()))
            {
                LogPrintf("FixSpentCoins found spent coin %s GAMEUNITS %s[%d], %s\n",
                    FormatMoney(pcoin->vout[n].nValue).c_str(), pcoin->GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                nMismatchFound++;
                nBalanceInQuestion += pcoin->vout[n].nValue;
                if (!fCheckOnly)
                {
                    pcoin->MarkSpent(n);
                    pcoin->WriteToDisk();
                };
            };
        };
    };
}

// ppcoin: disable transaction (only for coinstake)
void CWallet::DisableTransaction(const CTransaction &tx)
{
    if (!tx.IsCoinStake() || !IsFromMe(tx))
        return; // only disconnecting coinstake requires marking input unspent

    LOCK(cs_wallet);
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        if (tx.nVersion == ANON_TXN_VERSION
            && txin.IsAnonInput())
            continue;
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size() && IsMine(prev.vout[txin.prevout.n]))
            {
                prev.MarkUnspent(txin.prevout.n);
                prev.WriteToDisk();
            };
        };
    };
}

bool CReserveKey::GetReservedKey(CPubKey& pubkey)
{
    if (nIndex == -1)
    {
        CKeyPool keypool;
        pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex != -1)
        {
            vchPubKey = keypool.vchPubKey;
        } else
        {
            if (pwallet->vchDefaultKey.IsValid())
            {
                LogPrintf("CReserveKey::GetReservedKey(): Warning: Using default key instead of a new key, top up your keypool!");
                vchPubKey = pwallet->vchDefaultKey;
            } else
                return false;
        };
    };

    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

void CReserveKey::KeepKey()
{
    if (nIndex != -1)
        pwallet->KeepKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey()
{
    if (nIndex != -1)
        pwallet->ReturnKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CWallet::GetAllReserveKeys(set<CKeyID>& setAddress) const
{
    setAddress.clear();

    CWalletDB walletdb(strWalletFile);

    LOCK2(cs_main, cs_wallet);
    BOOST_FOREACH(const int64_t& id, setKeyPool)
    {
        CKeyPool keypool;
        if (!walletdb.ReadPool(id, keypool))
            throw runtime_error("GetAllReserveKeyHashes() : read failed");
        assert(keypool.vchPubKey.IsValid());
        CKeyID keyID = keypool.vchPubKey.GetID();
        if (!HaveKey(keyID))
            throw runtime_error("GetAllReserveKeyHashes() : unknown key in key pool");
        setAddress.insert(keyID);
    };
}

void CWallet::UpdatedTransaction(const uint256 &hashTx)
{
    {
        LOCK(cs_wallet);
        // Only notify UI if this transaction is in this wallet
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
            NotifyTransactionChanged(this, hashTx, CT_UPDATED);
    }
}

void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    mapKeyBirth.clear();

    // get birth times for keys with metadata
    for (std::map<CKeyID, CKeyMetadata>::const_iterator it = mapKeyMetadata.begin(); it != mapKeyMetadata.end(); it++)
        if (it->second.nCreateTime)
            mapKeyBirth[it->first] = it->second.nCreateTime;

    // map in which we'll infer heights of other keys
    CBlockIndex *pindexMax = FindBlockByHeight(std::max(0, nBestHeight - 144)); // the tip can be reorganised; use a 144-block safety margin
    std::map<CKeyID, CBlockIndex*> mapKeyFirstBlock;
    std::set<CKeyID> setKeys;
    GetKeys(setKeys);

    BOOST_FOREACH(const CKeyID &keyid, setKeys)
    {
        if (mapKeyBirth.count(keyid) == 0)
            mapKeyFirstBlock[keyid] = pindexMax;
    };

    setKeys.clear();

    // if there are no such keys, we're done
    if (mapKeyFirstBlock.empty())
        return;

    // find first block that affects those keys, if there are any left
    std::vector<CKeyID> vAffected;
    for (std::map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); it++)
    {
        // iterate over all wallet transactions...
        const CWalletTx &wtx = (*it).second;
        std::map<uint256, CBlockIndex*>::const_iterator blit = mapBlockIndex.find(wtx.hashBlock);
        if (blit != mapBlockIndex.end() && blit->second->IsInMainChain())
        {
            // ... which are already in a block
            int nHeight = blit->second->nHeight;
            BOOST_FOREACH(const CTxOut &txout, wtx.vout)
            {
                // iterate over all their outputs
                ::ExtractAffectedKeys(*this, txout.scriptPubKey, vAffected);
                BOOST_FOREACH(const CKeyID &keyid, vAffected)
                {
                    // ... and all their affected keys
                    std::map<CKeyID, CBlockIndex*>::iterator rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && nHeight < rit->second->nHeight)
                        rit->second = blit->second;
                };
                vAffected.clear();
            };
        };
    };

    // Extract block timestamps for those keys
    for (std::map<CKeyID, CBlockIndex*>::const_iterator it = mapKeyFirstBlock.begin(); it != mapKeyFirstBlock.end(); it++)
        mapKeyBirth[it->first] = it->second->nTime - 7200; // block times can be 2h off
}

