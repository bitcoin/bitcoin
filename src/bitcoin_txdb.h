// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIN_TXDB_LEVELDB_H
#define BITCOIN_BITCOIN_TXDB_LEVELDB_H

#include "leveldbwrapper.h"
#include "bitcoin_main.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class Bitcoin_CCoins;
class uint256;

// -dbcache default (MiB)
static const int64_t bitcoin_nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t bitcoin_nMaxDbCache = sizeof(void*) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t bitcoin_nMinDbCache = 4;

/** Bitcoin_CCoinsView backed by the LevelDB coin database (chainstate/) */
class Bitcoin_CCoinsViewDB : public Bitcoin_CCoinsView {
private:
    void BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Bitcoin_CCoins &coins);
public:
	static const unsigned char COIN_KEY;
	static const unsigned char BEST_CHAIN_KEY;
    //This breaks encapsulation, yes
    CLevelDBWrapper db;
    Bitcoin_CCoinsViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetCoins(const uint256 &txid, Bitcoin_CCoins &coins);
    bool SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins);
    bool HaveCoins(const uint256 &txid);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlock);
    bool GetStats(Bitcoin_CCoinsStats &stats);
};

/** Access to the block database (blocks/index/) */
class Bitcoin_CBlockTreeDB : public CLevelDBWrapper
{
public:
    Bitcoin_CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
private:
    Bitcoin_CBlockTreeDB(const Bitcoin_CBlockTreeDB&);
    void operator=(const Bitcoin_CBlockTreeDB&);
public:
    bool WriteBlockIndex(const Bitcoin_CDiskBlockIndex& blockindex);
    bool BatchWriteBlockIndex(std::vector<Bitcoin_CDiskBlockIndex>& vblockindexes);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &fileinfo);
    bool WriteBlockFileInfo(int nFile, const CBlockFileInfo &fileinfo);
    bool ReadLastBlockFile(int &nFile);
    bool WriteLastBlockFile(int nFile);
    bool ReadTrimToTime(int &nTrimToTime);
    bool WriteTrimToTime(int nTrimToTime);
    bool WriteReindexing(bool fReindex);
    bool ReadReindexing(bool &fReindex);
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> > &list);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool LoadBlockIndexGuts();
    /** Create a new block index entry for a given block hash */
    Bitcoin_CBlockIndex * InsertBlockIndex(uint256 hash);
};

#endif // BITCOIN_TXDB_LEVELDB_H
