// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXDB_LEVELDB_H
#define BITCOIN_TXDB_LEVELDB_H

#include "leveldbwrapper.h"
#include "main.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class Bitcredit_CCoins;
class uint256;

// -dbcache default (MiB)
static const int64_t bitcredit_nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t bitcredit_nMaxDbCache = sizeof(void*) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t bitcredit_nMinDbCache = 4;

/** CCoinsView backed by the LevelDB coin database (chainstate/) */
class Bitcredit_CCoinsViewDB : public Bitcredit_CCoinsView {
private:
	static const unsigned char COIN_KEY;
	static const unsigned char BEST_CHAIN_KEY;

    void BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Bitcredit_CCoins &coins);
protected:
    CLevelDBWrapper db;
public:
    Bitcredit_CCoinsViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetCoins(const uint256 &txid, Bitcredit_CCoins &coins);
    bool SetCoins(const uint256 &txid, const Bitcredit_CCoins &coins);
    bool HaveCoins(const uint256 &txid);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(const std::map<uint256, Bitcredit_CCoins> &mapCoins, const uint256 &hashBlock);
    bool GetStats(Bitcredit_CCoinsStats &stats);
};

/** Access to the block database (blocks/index/) */
class Credits_CBlockTreeDB : public CLevelDBWrapper
{
public:
    Credits_CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
private:
	static const unsigned char BLOCKINDEX_KEY;
	static const unsigned char REINDEX_KEY;
	static const unsigned char FILE_KEY;
	static const unsigned char FLAG_KEY;
	static const unsigned char LAST_BLOCK_KEY;
	static const unsigned char TX_KEY;
	static const unsigned char ONE;
	static const unsigned char ZERO;

    Credits_CBlockTreeDB(const Credits_CBlockTreeDB&);
    void operator=(const Credits_CBlockTreeDB&);
public:
    bool WriteBlockIndex(const Credits_CDiskBlockIndex& blockindex);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &fileinfo);
    bool WriteBlockFileInfo(int nFile, const CBlockFileInfo &fileinfo);
    bool ReadLastBlockFile(int &nFile);
    bool WriteLastBlockFile(int nFile);
    bool WriteReindexing(bool fReindex);
    bool ReadReindexing(bool &fReindex);
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> > &list);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool LoadBlockIndexGuts();
};

#endif // BITCOIN_TXDB_LEVELDB_H
