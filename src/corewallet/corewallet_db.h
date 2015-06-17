// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLET_COREWALLET_DB_H
#define BITCOIN_COREWALLET_COREWALLET_DB_H

#include "corewallet/corewallet_basics.h"
#include "corewallet/logdb.h"
#include "key.h"
#include "hdkeystore.h"

namespace CoreWallet
{
    
class Wallet;

// FileDB: a wallet file-database based on logdb
class FileDB : public CLogDB
{
public:
    FileDB(const std::string& strFilename, const char* pszMode = "r+", bool fFlushOnClose = true) : CLogDB( (GetDataDir() / strFilename).string(), false) { }

    bool LoadWallet(Wallet* pCoreWallet);
    
    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CoreWallet::CKeyMetadata &keyMeta);
    bool WriteHDMasterSeed(const uint256& hash, const CKeyingMaterial& masterSeed);
    bool WriteHDCryptedMasterSeed(const uint256& hash, const std::vector<unsigned char>& vchCryptedSecret);
    bool EraseHDMasterSeed(const uint256& hash);
    bool WriteHDChain(const CHDChain& chain);
    bool WriteHDPubKey(const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta);
    bool WriteHDAchiveChain(const uint256& hash);
};
    
}; // end namespace CoreWallet

#endif // BITCOIN_COREWALLET_COREWALLET_DB_H
