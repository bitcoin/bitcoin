// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_TXDB_BDB_H
#define BITCOIN_TXDB_BDB_H

#include "db.h"

/** Access to the transaction database (coins.dat) */
class CCoinsDB : public CDB
{
public:
    CCoinsDB(const char* pszMode="r+") : CDB("coins.dat", pszMode) { }
private:
    CCoinsDB(const CCoinsDB&);
    void operator=(const CCoinsDB&);
public:
    bool ReadCoins(uint256 hash, CCoins &coins);
    bool WriteCoins(uint256 hash, const CCoins& coins);
    bool HaveCoins(uint256 hash);
    bool ReadHashBestChain(uint256& hashBestChain);
    bool WriteHashBestChain(uint256 hashBestChain);
};

/** CCoinsView backed by a CCoinsDB */
class CCoinsViewDB : public CCoinsView
{
protected:
    CCoinsDB db;
public:
    CCoinsViewDB();

    bool GetCoins(uint256 txid, CCoins &coins);
    bool SetCoins(uint256 txid, const CCoins &coins);
    bool HaveCoins(uint256 txid);
    CBlockIndex *GetBestBlock();
    bool SetBestBlock(CBlockIndex *pindex);
    bool BatchWrite(const std::map<uint256, CCoins> &mapCoins, CBlockIndex *pindex);
};

/** Access to the block database (blktree.dat) */
class CBlockTreeDB : public CDB
{
public:
    CBlockTreeDB(const char* pszMode="r+") : CDB("blktree.dat", pszMode) { }
private:
    CBlockTreeDB(const CBlockTreeDB&);
    void operator=(const CBlockTreeDB&);
public:
    bool WriteBlockIndex(const CDiskBlockIndex& blockindex);
    bool ReadBestInvalidWork(CBigNum& bnBestInvalidWork);
    bool WriteBestInvalidWork(CBigNum bnBestInvalidWork);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &fileinfo);
    bool WriteBlockFileInfo(int nFile, const CBlockFileInfo &fileinfo);
    bool ReadLastBlockFile(int &nFile);
    bool WriteLastBlockFile(int nFile);
    bool LoadBlockIndexGuts();
};

#endif // BITCOIN_TXDB_BDB_H
