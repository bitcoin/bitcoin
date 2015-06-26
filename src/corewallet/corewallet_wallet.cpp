// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_wallet.h"

#include "base58.h"
#include "eccryptoverify.h"
#include "random.h"
#include "timedata.h"
#include "util.h"
#include "utilstrencodings.h"

#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

namespace CoreWallet {

bool Wallet::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return CCryptoKeyStore::AddKeyPubKey(key, pubkey);
}

bool Wallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    
    return walletCacheDB->WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
}

bool Wallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    LOCK(cs_coreWallet);
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool Wallet::SetAddressBook(const CTxDestination& address, const std::string& strPurpose)
{
    LOCK(cs_coreWallet);
    std::map<CTxDestination, CAddressBookMetadata> ::iterator mi = mapAddressBook.find(address);
    if(mi == mapAddressBook.end())
        mapAddressBook[address] = CAddressBookMetadata();
    
    mapAddressBook[address].purpose = strPurpose;
    
    return walletCacheDB->Write(make_pair(std::string("adrmeta"), CBitcoinAddress(address).ToString()), mapAddressBook[address]);
}



const unsigned int HD_MAX_DEPTH = 20;

bool Wallet::HDSetChainPath(const std::string& chainPathIn, bool generateMaster, CKeyingMaterial& vSeed, HDChainID& chainId, bool overwrite)
{
    LOCK(cs_coreWallet);

    if (IsLocked())
        return false;

    if (chainPathIn[0] != 'm')
        throw std::runtime_error("CoreWallet::SetHDChainPath(): Non masterkey chainpaths are not allowed.");

    if (chainPathIn.find_first_of("c", 1) == std::string::npos)
        throw std::runtime_error("CoreWallet::SetHDChainPath(): 'c' (internal/external chain selection) is requires in the given chainpath.");

    if (chainPathIn.find_first_not_of("0123456789'/mch", 0) != std::string::npos)
        throw std::runtime_error("CoreWallet::SetHDChainPath(): Invalid chainpath.");

    std::string newChainPath = chainPathIn;
    boost::to_lower(newChainPath);
    boost::erase_all(newChainPath, " ");
    boost::replace_all(newChainPath, "h", "'"); //support h instead of ' to allow easy JSON input over cmd line
    if (newChainPath.size() > 0 && *newChainPath.rbegin() == '/')
        newChainPath.resize(newChainPath.size() - 1);

    std::vector<std::string> pathFragments;
    boost::split(pathFragments, newChainPath, boost::is_any_of("/"));

    if (pathFragments.size() > HD_MAX_DEPTH)
        throw std::runtime_error("CoreWallet::SetHDChainPath(): Max chain depth ("+itostr(HD_MAX_DEPTH)+") exceeded!");

    int64_t nCreationTime = GetTime();
    CHDChain newChain(nCreationTime);
    newChain.chainPath = newChainPath;
    CExtKey parentKey;
    BOOST_FOREACH(std::string fragment, pathFragments)
    {
        bool harden = false;
        if (*fragment.rbegin() == '\'')
        {
            harden = true;
            fragment = fragment.substr(0,fragment.size()-1);
        }

        if (fragment == "m")
        {
            //generate a master key seed
            //currently seed size is fixed to 256bit
            assert(vSeed.size() == 32);
            if (generateMaster)
            {
                RandAddSeedPerfmon();
                do {
                    GetRandBytes(&vSeed[0], vSeed.size());
                } while (!eccrypto::Check(&vSeed[0]));
            }

            CExtKey bip32MasterKey;
            bip32MasterKey.SetMaster(&vSeed[0], vSeed.size());

            CBitcoinExtKey b58key;
            b58key.SetKey(bip32MasterKey);
            chainId = bip32MasterKey.key.GetPubKey().GetHash();

            //only one chain per master seed is allowed
            CHDChain possibleChain;
            if (GetChain(chainId, possibleChain) && possibleChain.IsValid())
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Only one chain per masterseed is allowed.");

            if (!AddMasterSeed(chainId, vSeed))
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Could not store master seed.");

            //keep the master pubkeyhash for chain identifying
            newChain.chainHash = chainId;

            if (IsCrypted())
            {
                std::vector<unsigned char> vchCryptedSecret;
                GetCryptedMasterSeed(chainId, vchCryptedSecret);

                if (!walletPrivateDB->WriteHDCryptedMasterSeed(chainId, vchCryptedSecret))
                    throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing hdmasterseed failed!");
            }
            else
            {
                if (!walletPrivateDB->WriteHDMasterSeed(chainId, vSeed))
                    throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing cryted hdmasterseed failed!");
            }

            //set active hd chain
            activeHDChain = chainId;
            if (!walletCacheDB->WriteHDAchiveChain(chainId))
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing active hd chain failed!");

            parentKey = bip32MasterKey;
        }
        else if (fragment == "c")
        {
            harden = false; //external / internal chain root keys can not be hardened
            CExtPubKey parentExtPubKey = parentKey.Neuter();
            parentExtPubKey.Derive(newChain.externalPubKey, 0);
            parentExtPubKey.Derive(newChain.internalPubKey, 1);

            AddChain(newChain);

            if (!walletCacheDB->WriteHDChain(newChain))
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing new chain failed!");
        }
        else
        {
            CExtKey childKey;
            int32_t nIndex;
            if (!ParseInt32(fragment,&nIndex))
                return false;
            parentKey.Derive(childKey, (harden ? 0x80000000 : 0)+nIndex);
            parentKey = childKey;
        }
    }

    return true;
}

bool Wallet::HDGetChildPubKeyAtIndex(const HDChainID& chainIDIn, CPubKey &pubKeyOut, std::string& newKeysChainpath, unsigned int nIndex, bool internal)
{
    AssertLockHeld(cs_coreWallet);

    //check possible null chainId and load current active chain
    CHDChain chain;
    HDChainID chainID = chainIDIn;
    if (chainID.IsNull())
        chainID = activeHDChain;

    if (!GetChain(chainID, chain) || !chain.IsValid())
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Selected chain is not vailid!");

    CHDPubKey newHdPubKey;
    if (!DeriveHDPubKeyAtIndex(chainID, newHdPubKey, nIndex, internal))
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Deriving child key faild!");

    // Create new metadata
    int64_t nCreationTime = GetTime();
    mapKeyMetadata[newHdPubKey.pubkey.GetID()] = CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;

    if (!LoadHDPubKey(newHdPubKey))
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Add key to keystore failed!");

    if (!walletCacheDB->WriteHDPubKey(newHdPubKey, mapKeyMetadata[newHdPubKey.pubkey.GetID()]))
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Writing pubkey failed!");

    pubKeyOut = newHdPubKey.pubkey;

    newKeysChainpath = chain.chainPath;
    boost::replace_all(newKeysChainpath, "c", itostr(internal)); //replace the chain switch index
    newKeysChainpath += "/"+itostr(nIndex);

    return true;
}

bool Wallet::HDGetNextChildPubKey(const HDChainID& chainIDIn, CPubKey &pubKeyOut, std::string& newKeysChainpath, bool internal)
{
    AssertLockHeld(cs_coreWallet);

    CHDChain chain;
    HDChainID chainID = chainIDIn;
    if (chainID.IsNull())
        chainID = activeHDChain;

    if (!GetChain(chainID, chain) || !chain.IsValid())
        throw std::runtime_error("CoreWallet::HDGetNextChildPubKey(): Selected chain is not vailid!");

    unsigned int nextIndex = GetNextChildIndex(chainID, internal);
    if (!HDGetChildPubKeyAtIndex(chainID, pubKeyOut, newKeysChainpath, nextIndex, internal))
        return false;

    return true;
}

bool Wallet::EncryptHDSeeds(CKeyingMaterial& vMasterKeyIn)
{
    EncryptSeeds();

    std::vector<HDChainID> chainIds;
    GetAvailableChainIDs(chainIds);

    BOOST_FOREACH(HDChainID& chainId, chainIds)
    {
        std::vector<unsigned char> vchCryptedSecret;
        if (!GetCryptedMasterSeed(chainId, vchCryptedSecret))
            throw std::runtime_error("CoreWallet::EncryptHDSeeds(): Encrypting seeds failed!");
        
        if (!walletPrivateDB->WriteHDCryptedMasterSeed(chainId, vchCryptedSecret))
            throw std::runtime_error("CoreWallet::EncryptHDSeeds(): Writing hdmasterseed failed!");
    }
    
    return true;
}

bool Wallet::HDSetActiveChainID(const HDChainID& chainID, bool check)
{
    LOCK(cs_coreWallet);
    CHDChain chainOut;
    
    //do the chainID check optional because we need a way of setting the chainID before the acctual chain is loaded into the memory
    //this is becuase of the way we load the chains from the wallet.dat
    if (check && !GetChain(chainID, chainOut)) //TODO: implement FindChain() to avoild copy of CHDChain
        return false;
    
    activeHDChain = chainID;
    return true;
}

bool Wallet::HDGetActiveChainID(HDChainID& chainID)
{
    LOCK(cs_coreWallet);
    chainID = activeHDChain;
    return true;
}

}; //end namespace