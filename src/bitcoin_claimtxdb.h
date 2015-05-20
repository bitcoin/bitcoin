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
	static const unsigned char CLAIM_COIN_KEY;
	static const unsigned char CLAIM_PRUNED_COIN_KEY;
	static const unsigned char CLAIM_BEST_CHAIN_KEY;
	static const unsigned char CLAIM_BITCREDIT_CLAIM_TIP_KEY;
	static const unsigned char CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY;

    void Claim_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteHashBitcreditClaimTip(CLevelDBBatch &batch, const uint256 &hash);
    void Claim_BatchWriteTotalClaimedCoins(CLevelDBBatch &batch, const int64_t &totalClaimedCoins);
    void Claim_BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Bitcoin_CClaimCoins &coins);
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

    bool Claim_GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins);
    bool Claim_SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins);
    bool Claim_HaveCoins(const uint256 &txid);
    uint256 Claim_GetBestBlock();
    bool Claim_SetBestBlock(const uint256 &hashBlock);
    uint256 Claim_GetBitcreditClaimTip();
    bool Claim_SetBitcreditClaimTip(const uint256 &hashBlock);
    int64_t Claim_GetTotalClaimedCoins();
    bool Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins);
    bool Claim_BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins);
    bool Claim_GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore);
    bool Claim_GetStats(Bitcoin_CClaimCoinsStats &stats);
};

#endif // BITCOIN_CLAIMTXDB_LEVELDB_H
