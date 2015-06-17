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
    void Bitcoin_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteHashBitcreditClaimTip(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteTotalClaimedCoins(CLevelDBBatch &batch, const int64_t &totalClaimedCoins);
    void Bitcoin_BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Claim_CCoins &coins);
    void Claim_BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Claim_CCoins &coins);

public:
	static const unsigned char BITCOIN_COIN_KEY;
	static const unsigned char CLAIM_COIN_KEY;
	static const unsigned char BITCOIN_BEST_CHAIN_KEY;
	static const unsigned char CLAIM_BEST_CHAIN_KEY;
	static const unsigned char CLAIM_BITCREDIT_CLAIM_TIP_KEY;
	static const unsigned char CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY;

	//This breaks encapsulation, yes
    CLevelDBWrapper db;
    Bitcoin_CCoinsViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool Bitcoin_GetCoins(const uint256 &txid, Claim_CCoins &coins);
    bool Claim_GetCoins(const uint256 &txid, Claim_CCoins &coins);
    bool Bitcoin_SetCoins(const uint256 &txid, const Claim_CCoins &coins);
    bool Claim_SetCoins(const uint256 &txid, const Claim_CCoins &coins);
    bool Bitcoin_HaveCoins(const uint256 &txid);
    bool Claim_HaveCoins(const uint256 &txid);
    uint256 Bitcoin_GetBestBlock();
    uint256 Claim_GetBestBlock();
    bool Bitcoin_SetBestBlock(const uint256 &hashBlock);
    bool Claim_SetBestBlock(const uint256 &hashBlock);
    uint256 Claim_GetBitcreditClaimTip();
    bool Claim_SetBitcreditClaimTip(const uint256 &hashBlock);
    int64_t Claim_GetTotalClaimedCoins();
    bool Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins);
    bool Bitcoin_BatchWrite(const std::map<uint256, Claim_CCoins> &mapCoins, const uint256 &hashBlock);
    bool Claim_BatchWrite(const std::map<uint256, Claim_CCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins);
    bool All_BatchWrite(const std::map<uint256, Claim_CCoins> &bitcoin_mapCoins, const uint256 &bitcoin_hashBlock, const std::map<uint256, Claim_CCoins> &claim_mapCoins, const uint256 &claim_hashBlock, const uint256 &claim_hashBitcreditClaimTip, const int64_t &claim_totalClaimedCoins);
    bool Bitcoin_GetStats(Claim_CCoinsStats &stats);
    bool Claim_GetStats(Claim_CCoinsStats &stats);
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
    bool WriteBlockTxHashesWithInputs(const Bitcoin_CDiskBlockIndex& blockindex, const std::vector<pair<uint256, std::vector<COutPoint> > > &vTxHashesWithInputs);
    bool BatchWriteBlockTxHashesWithInputs(std::vector<Bitcoin_CDiskBlockIndex>& vblockindexes, const std::vector<std::vector<pair<uint256, std::vector<COutPoint> > > > &vTxHashesWithInputs);
    bool ReadBlockTxHashesWithInputs(const uint256 &blockHash, std::vector<pair<uint256, std::vector<COutPoint> > > &vTxHashesWithInputs);
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
