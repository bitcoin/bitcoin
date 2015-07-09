// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"

#include <stdint.h>

#include "corewallet/corewallet_db.h"
#include "corewallet/corewallet_wallet.h"
#include "corewallet/corewallet_wtx.h"

namespace CoreWallet
{
    
bool FileDB::WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta)
{
    if (!Write(std::make_pair(kvs_keymetadata_key, vchPubKey), keyMeta, false))
        return false;
    
    // hash pubkey/privkey to accelerate wallet load
    std::vector<unsigned char> vchKey;
    vchKey.reserve(vchPubKey.size() + vchPrivKey.size());
    vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
    vchKey.insert(vchKey.end(), vchPrivKey.begin(), vchPrivKey.end());
    
    return Write(std::make_pair(kvs_key_key, vchPubKey), std::make_pair(vchPrivKey, Hash(vchKey.begin(), vchKey.end())), false);
}

bool FileDB::WriteHDMasterSeed(const uint256& hash, const CKeyingMaterial& masterSeed)
{
    return Write(std::make_pair(kvs_hd_master_seed_key, hash), masterSeed);
}

bool FileDB::WriteHDCryptedMasterSeed(const uint256& hash, const std::vector<unsigned char>& vchCryptedSecret)
{
    if (!Write(std::make_pair(kvs_hd_encrypted_master_seed_key, hash), vchCryptedSecret))
        return false;

    Erase(std::make_pair(kvs_hd_master_seed_key, hash));
    Erase(std::make_pair(kvs_hd_master_seed_key, hash));

    return true;
}

bool FileDB::EraseHDMasterSeed(const uint256& hash)
{
    return Erase(std::make_pair(kvs_hd_master_seed_key, hash));
}

bool FileDB::WriteHDChain(const CHDChain &chain)
{
    return Write(std::make_pair(kvs_hd_hdchain_key, chain.chainHash), chain);
}

bool FileDB::WriteHDPubKey(const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta)
{
    if (!Write(std::make_pair(kvs_keymetadata_key, hdPubKey.pubkey),
               keyMeta))
        return false;

    return Write(std::make_pair(kvs_hdpubkey_key, hdPubKey.pubkey), hdPubKey);
}

bool FileDB::WriteHDAchiveChain(const uint256& hash)
{
    return Write(kvs_hdactivechain_key, hash);
}

bool FileDB::WriteTx(uint256 hash, const WalletTx& wtx)
{
    return Write(std::make_pair(kvs_wtx_key, hash), wtx);
}

bool FileDB::EraseTx(uint256 hash)
{
    return Erase(std::make_pair(kvs_wtx_key, hash));
}

bool ReadKeyValue(Wallet* pCoreWallet, CDataStream& ssKey, CDataStream& ssValue, std::string& strType, std::string& strErr)
{
    try {
        ssKey >> strType;
        if (strType == kvs_key_key)
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
            
            ssValue >> pkey;
            ssValue >> hash;

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
            if (!pCoreWallet->InMemAddKey(key, vchPubKey))
            {
                strErr = "Error reading wallet database: LoadKey failed";
                return false;
            }
        }
        else if (strType == kvs_keymetadata_key)
        {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            
            pCoreWallet->InMemAddKeyMetadata(vchPubKey, keyMeta);
            
            // find earliest key creation time, as wallet birthday
            if (!pCoreWallet->nTimeFirstKey ||
                (keyMeta.nCreateTime < pCoreWallet->nTimeFirstKey))
                pCoreWallet->nTimeFirstKey = keyMeta.nCreateTime;
        }
        else if (strType == kvs_address_book_metadata_key)
        {
            std::string strAddress;
            CAddressBookMetadata metadata;
            ssKey >> strAddress;
            ssValue >> metadata;
            pCoreWallet->mapAddressBook[CBitcoinAddress(strAddress).Get()] = metadata;
        }
        else if (strType == kvs_hd_master_seed_key)
        {
            uint256 masterPubKeyHash;
            CKeyingMaterial masterSeed;
            ssKey >> masterPubKeyHash;
            ssValue >> masterSeed;
            pCoreWallet->AddMasterSeed(masterPubKeyHash, masterSeed);
        }
        else if (strType == kvs_hd_encrypted_master_seed_key)
        {
            uint256 masterPubKeyHash;
            std::vector<unsigned char> vchCryptedSecret;
            ssKey >> masterPubKeyHash;
            ssValue >> vchCryptedSecret;
            pCoreWallet->AddCryptedMasterSeed(masterPubKeyHash, vchCryptedSecret);
        }
        else if (strType == kvs_hdactivechain_key)
        {
            HDChainID chainID;
            ssValue >> chainID;
            pCoreWallet->HDSetActiveChainID(chainID, false); //don't check if the chain exists because this record could come in before the CHDChain object itself
        }
        else if (strType == kvs_hdpubkey_key)
        {
            CHDPubKey hdPubKey;
            ssValue >> hdPubKey;
            if (!hdPubKey.IsValid())
            {
                strErr = "Error reading wallet database: CHDPubKey corrupt";
                return false;
            }
            if (!pCoreWallet->LoadHDPubKey(hdPubKey))
            {
                strErr = "Error reading wallet database: LoadHDPubKey failed";
                return false;
            }
        }
        else if (strType == kvs_hd_hdchain_key)
        {
            CHDChain chain;
            ssValue >> chain;
            if (!pCoreWallet->AddChain(chain))
            {
                strErr = "Error reading wallet database: AddChain failed";
                return false;
            }
        }
        else if (strType == kvs_wtx_key)
        {
            uint256 hash;
            ssKey >> hash;
            WalletTx wtx;
            ssValue >> wtx;
            pCoreWallet->AddToWallet(wtx, true);
        }



    } catch (...)
    {
        return false;
    }
    return true;
}

bool FileDB::LoadWallet(Wallet* pCoreWallet)
{
    if(!Load())
        return false;
    
    bool fNoncriticalErrors = false;
    bool result = true;
    
    bool fAutoTransaction = TxnBegin();
    
    try {
        LOCK(pCoreWallet->cs_coreWallet);
        for (FileDB::const_iterator it = begin(); it != end(); it++)
        {
            // Read next record
            CDataStream ssKey((*it).first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue((*it).second, SER_DISK, CLIENT_VERSION);
            
            // Try to be tolerant of single corrupt records:
            std::string strType, strErr;
            if (!ReadKeyValue(pCoreWallet, ssKey, ssValue, strType, strErr))
            {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (strType == kvs_key_key)
                    result = false;
                else
                {
                    // Leave other errors alone, if we try to fix them we might make things worse.
                    fNoncriticalErrors = true; // ... but do warn the user there is something wrong.
                    if (strType == kvs_wtx_key)
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
        result = false;
    }
    
    if (fNoncriticalErrors && result == true)
        result = false;
    
    // Any wallet corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != true)
        return result;
    
    if (fAutoTransaction)
        TxnCommit();
    
    return result;
}

}; //end namespace