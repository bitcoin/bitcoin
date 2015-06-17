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
  
bool Wallet::GenerateBip32Structure(const std::string& chainPathIn, unsigned char (&vch)[32], bool useSeed)
{
    //take the given chainpath and extend it with /0 for external keys and /1 for internal
    //example: input chainpath "m/44'/0'/0'/" will result in m/44'/0'/0'/0/<nChild> for external and m/44'/0'/0'/1/<nChild> for internal keypair generation
    //disk cache the internal and external parent pubkey for faster pubkey generation
    if (!walletCacheDB->Write(std::string("chainpath"), chainPathIn, true)) //for easy serialization store the unsigned char[32] as hex string. //TODO: use 32byte binary ser.
        throw std::runtime_error("CoreWallet::GenerateBip32Structure(): write chainpath failed!");

    strChainPath = chainPathIn;
    std::string chainPath = chainPathIn;
    boost::to_lower(chainPath);
    boost::erase_all(chainPath, " ");
    if (chainPath.size() > 0 && chainPath.back() == '/')
        chainPath.resize(chainPath.size() - 1);
    
    std::vector<std::string> pathFragments;
    boost::split(pathFragments, chainPath, boost::is_any_of("/"));
    
    
    CExtKey parentKey;
    
    BOOST_FOREACH(std::string fragment, pathFragments)
    {
        bool harden = false;
        if (fragment.back() == '\'')
            harden = true;

        if (fragment == "m")
        {
            // lets generate a master key seed
            //currently seed size is fixed to 256bit
            if (!useSeed)
            {
                RandAddSeedPerfmon();
                do {
                    GetRandBytes(vch, sizeof(vch));
                } while (!eccrypto::Check(vch));
            }

            CExtKey bip32MasterKey;
            bip32MasterKey.SetMaster(vch, sizeof(vch));

            CBitcoinExtKey b58key;
            b58key.SetKey(bip32MasterKey);
            LogPrintf("key: %s", b58key.ToString());

            strMasterseedHex = HexStr(vch, vch+sizeof(vch));
            uint32_t seedNum = 0;
            if (!walletPrivateDB->Write(std::make_pair(std::string("masterseed"),seedNum), strMasterseedHex, true)) //for easy serialization store the unsigned char[32] as hex string. //TODO: use 32byte binary ser.
                throw std::runtime_error("CoreWallet::GenerateBip32Structure(): Writing masterkeyid failed!");

            parentKey = bip32MasterKey;
        }
        else if (fragment == "k")
        {
            harden = false;
        }
        else if (fragment == "c")
        {
            harden = false;
            CExtPubKey parentExtPubKey = parentKey.Neuter();
            parentExtPubKey.Derive(externalPubKey, 0);
            parentExtPubKey.Derive(internalPubKey, 1);

            uint32_t keyRingNum = 0;
            if (!walletPrivateDB->Write(std::make_pair(std::string("externalpubkey"),keyRingNum), externalPubKey, true))
                throw std::runtime_error("CoreWallet::GenerateBip32Structure(): Writing masterkeyid failed!");

            if (!walletPrivateDB->Write(std::make_pair(std::string("internalpubkey"),keyRingNum), internalPubKey, true))
                throw std::runtime_error("CoreWallet::GenerateBip32Structure(): Writing masterkeyid failed!");
        }
        else
        {
            CExtKey childKey;
            int nIndex = atoi(fragment.c_str());
            parentKey.Derive(childKey, (harden ? 0x80000000 : 0)+nIndex);
            parentKey = childKey;
        }
    }

    return true;
}
    
CPubKey Wallet::GenerateNewKey(int indexIn)
{
    unsigned int useIndex = 0;
    if (index >= 0)
        useIndex = indexIn;
    
    CExtPubKey newExtPubKey;
    internalPubKey.Derive(newExtPubKey, useIndex);
    
    int64_t nCreationTime = GetTime();
    
    CKeyMetadata meta(nCreationTime);
    meta.keyidParent = internalPubKey.pubkey.GetID();
    meta.nDepth = newExtPubKey.nDepth;
    meta.nDerivationIndex = newExtPubKey.nChild;
    mapKeyMetadata[newExtPubKey.pubkey.GetID()] = meta;
    
    return newExtPubKey.pubkey;
}

CPubKey Wallet::GetNextUnusedKey(unsigned int& usedIndex)
{

//
//            internalPubKey = internalKey.Neuter();
//            externalPubKey = externalKey.Neuter();
//
//            CExtPubKey childPubKey = childKey.Neuter();
//            int64_t nCreationTime = GetTime();
//
//            CKeyMetadata metaInternal(nCreationTime);
//            metaInternal.keyidParent = childPubKey.pubkey.GetID();
//            metaInternal.nDepth = internalPubKey.nDepth;
//            metaInternal.nDerivationIndex = internalPubKey.nChild;
//            mapKeyMetadata[internalPubKey.pubkey.GetID()] = metaInternal;
//
//            CKeyMetadata metaExternal(nCreationTime);
//            metaExternal.keyidParent = childPubKey.pubkey.GetID();
//            metaExternal.nDepth = internalPubKey.nDepth;
//            metaExternal.nDerivationIndex = internalPubKey.nChild;
//            mapKeyMetadata[externalPubKey.pubkey.GetID()] = metaExternal;


    usedIndex = internalNextPos;
    CPubKey pubKey = GetKeyAtIndex(internalNextPos++);
    return pubKey;
}

CPubKey Wallet::GetKeyAtIndex(unsigned int index, bool internal)
{
    LOCK(cs_coreWallet);

    CExtPubKey childKey;
    externalPubKey.Derive(childKey,index);

    int64_t nCreationTime = GetTime();
    CKeyMetadata meta(nCreationTime);
    meta.keyidParent = externalPubKey.pubkey.GetID();
    meta.nDepth = childKey.nDepth;
    meta.nDerivationIndex = childKey.nChild;
    mapKeyMetadata[childKey.pubkey.GetID()] = meta;

    if (!walletCacheDB->Write(std::make_pair(std::string("extpubkey"),childKey.pubkey.GetID()), childKey, true))
        throw std::runtime_error("CoreWallet::GenerateBip32Structure(): Writing masterkeyid failed!");

    externalUsedKeysCache.insert(childKey.nDepth);

    return childKey.pubkey;
}

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

bool Wallet::HDGetChildPubKeyAtIndex(const HDChainID& chainID, CPubKey &pubKeyOut, unsigned int nIndex, bool internal)
{
    AssertLockHeld(cs_coreWallet);

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
    if (!HDGetChildPubKeyAtIndex(chainID, pubKeyOut, nextIndex, internal))
        return false;

    newKeysChainpath = chain.chainPath;
    boost::replace_all(newKeysChainpath, "c", itostr(internal)); //replace the chain switch index
    newKeysChainpath += "/"+itostr(nextIndex);

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