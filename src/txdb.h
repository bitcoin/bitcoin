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

class Credits_CCoins;
class uint256;

// -dbcache default (MiB)
static const int64_t bitcredit_nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t bitcredit_nMaxDbCache = sizeof(void*) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t bitcredit_nMinDbCache = 4;

/** CCoinsView backed by the LevelDB coin database (chainstate/) */
class Credits_CCoinsViewDB : public Credits_CCoinsView {
private:
	static const unsigned char CREDITS_COIN_KEY;
	static const unsigned char CLAIM_COIN_KEY;
	static const unsigned char CREDITS_BEST_CHAIN_KEY;
	static const unsigned char CLAIM_BEST_CHAIN_KEY;
	static const unsigned char CLAIM_BITCREDIT_CLAIM_TIP_KEY;
	static const unsigned char CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY;

    void Credits_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteHashBitcreditClaimTip(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteTotalClaimedCoins(CLevelDBBatch &batch, const int64_t &totalClaimedCoins);
    void Credits_BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Credits_CCoins &coins);
    void Claim_BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Claim_CCoins &coins);

protected:
    CLevelDBWrapper db;
public:
    Credits_CCoinsViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins);
    bool Claim_GetCoins(const uint256 &txid, Claim_CCoins &coins);
    bool Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins);
    bool Claim_SetCoins(const uint256 &txid, const Claim_CCoins &coins);
    bool Credits_HaveCoins(const uint256 &txid);
    bool Claim_HaveCoins(const uint256 &txid);
    uint256 Credits_GetBestBlock();
    uint256 Claim_GetBestBlock();
    bool Credits_SetBestBlock(const uint256 &hashBlock);
    bool Claim_SetBestBlock(const uint256 &hashBlock);
    uint256 Claim_GetBitcreditClaimTip();
    bool Claim_SetBitcreditClaimTip(const uint256 &hashBlock);
    int64_t Claim_GetTotalClaimedCoins();
    bool Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins);
    bool Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock);
    bool Claim_BatchWrite(const std::map<uint256, Claim_CCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins);
    bool Credits_GetStats(Credits_CCoinsStats &stats);
    bool Claim_GetStats(Claim_CCoinsStats &stats);
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
