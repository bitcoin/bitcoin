// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIN_CLAIMTXDB_LEVELDB_H
#define BITCOIN_BITCOIN_CLAIMTXDB_LEVELDB_H

#include "leveldbwrapper.h"
#include "bitcoin_main.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class Bitcoin_CClaimCoins;
class uint256;

// -dbcache default (MiB)
static const int64_t bitcoin_claim_nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t bitcoin_claim_nMaxDbCache = sizeof(void*) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t bitcoin_claim_nMinDbCache = 4;

/** Bitcoin_CClaimCoinsView backed by the LevelDB coin database (chainstate/) */
class Bitcoin_CClaimCoinsViewDB : public Bitcoin_CClaimCoinsView {
private:
	static const unsigned char COIN_KEY;
	static const unsigned char PRUNED_COIN_KEY;
	static const unsigned char BEST_CHAIN_KEY;
	static const unsigned char BITCREDIT_CLAIM_TIP_KEY;
	static const unsigned char BITCREDIT_TOTAL_CLAIMED_COINS_KEY;

    void BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void BatchWriteHashBitcreditClaimTip(CLevelDBBatch &batch, const uint256 &hash);
    void BatchWriteTotalClaimedCoins(CLevelDBBatch &batch, const int64_t &totalClaimedCoins);
    void BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Bitcoin_CClaimCoins &coins);
protected:
    CLevelDBWrapper db;
    bool trackPruned;
    bool deleteOnDestroy;

    boost::filesystem::path path;
    //Holds temporary iteration state over db
	leveldb::Iterator *ptmpcursor;
public:
    Bitcoin_CClaimCoinsViewDB(const boost::filesystem::path &path, size_t nCacheSize, bool trackPruned = true, bool deleteOnDestroy = false, bool fMemory = false, bool fWipe = false);
   ~Bitcoin_CClaimCoinsViewDB();

    bool GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins);
    bool SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins);
    bool HaveCoins(const uint256 &txid);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    uint256 GetBitcreditClaimTip();
    bool SetBitcreditClaimTip(const uint256 &hashBlock);
    int64_t GetTotalClaimedCoins();
    bool SetTotalClaimedCoins(const int64_t &totalClaimedCoins);
    bool BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins);
    bool GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore);
    bool GetStats(Bitcoin_CClaimCoinsStats &stats);
};

#endif // BITCOIN_CLAIMTXDB_LEVELDB_H
