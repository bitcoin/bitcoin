// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_claimtxdb.h"

#include "bitcoin_core.h"
#include "uint256.h"

#include <stdint.h>

using namespace std;

const unsigned char Bitcoin_CClaimCoinsViewDB::COIN_KEY = 'c';
const unsigned char Bitcoin_CClaimCoinsViewDB::PRUNED_COIN_KEY = 'p';
const unsigned char Bitcoin_CClaimCoinsViewDB::BEST_CHAIN_KEY = 'B';
const unsigned char Bitcoin_CClaimCoinsViewDB::BITCREDIT_CLAIM_TIP_KEY = 'R';
const unsigned char Bitcoin_CClaimCoinsViewDB::BITCREDIT_TOTAL_CLAIMED_COINS_KEY = 'T';

void Bitcoin_CClaimCoinsViewDB::BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Bitcoin_CClaimCoins &coins) {
    if (coins.IsPruned())
    	if(trackPruned)
            batch.Write(make_pair(PRUNED_COIN_KEY, hash), PRUNED_COIN_KEY);
    	else
    		batch.Erase(make_pair(COIN_KEY, hash));
    else
        batch.Write(make_pair(COIN_KEY, hash), coins);
}
void Bitcoin_CClaimCoinsViewDB::BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(BEST_CHAIN_KEY, hash);
}
void Bitcoin_CClaimCoinsViewDB::BatchWriteHashBitcreditClaimTip(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(BITCREDIT_CLAIM_TIP_KEY, hash);
}
void Bitcoin_CClaimCoinsViewDB::BatchWriteTotalClaimedCoins(CLevelDBBatch &batch, const int64_t &totalClaimedCoins) {
    batch.Write(BITCREDIT_TOTAL_CLAIMED_COINS_KEY, totalClaimedCoins);
}

bool Bitcoin_CClaimCoinsViewDB::GetCoins(const uint256 &txid, Bitcoin_CClaimCoins &coins) {
    return db.Read(make_pair(COIN_KEY, txid), coins);
}

bool Bitcoin_CClaimCoinsViewDB::SetCoins(const uint256 &txid, const Bitcoin_CClaimCoins &coins) {
    CLevelDBBatch batch;
    BatchWriteCoins(batch, txid, coins);
    return db.WriteBatch(batch);
}

bool Bitcoin_CClaimCoinsViewDB::HaveCoins(const uint256 &txid) {
    return db.Exists(make_pair(COIN_KEY, txid));
}

uint256 Bitcoin_CClaimCoinsViewDB::GetBestBlock() {
    uint256 hashBestChain;
    if (!db.Read(BEST_CHAIN_KEY, hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool Bitcoin_CClaimCoinsViewDB::SetBestBlock(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    BatchWriteHashBestChain(batch, hashBlock);
    return db.WriteBatch(batch);
}

uint256 Bitcoin_CClaimCoinsViewDB::GetBitcreditClaimTip() {
    uint256 hash;
    if (!db.Read(BITCREDIT_CLAIM_TIP_KEY, hash))
        return uint256(0);
    return hash;
}
bool Bitcoin_CClaimCoinsViewDB::SetBitcreditClaimTip(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    BatchWriteHashBitcreditClaimTip(batch, hashBlock);
    return db.WriteBatch(batch);
}

int64_t Bitcoin_CClaimCoinsViewDB::GetTotalClaimedCoins() {
	int64_t totalClaimedCoins;
    if (!db.Read(BITCREDIT_TOTAL_CLAIMED_COINS_KEY, totalClaimedCoins))
        return int64_t(0);
    return totalClaimedCoins;
}
bool Bitcoin_CClaimCoinsViewDB::SetTotalClaimedCoins(const int64_t &totalClaimedCoins) {
    CLevelDBBatch batch;
    BatchWriteTotalClaimedCoins(batch, totalClaimedCoins);
    return db.WriteBatch(batch);
}

bool Bitcoin_CClaimCoinsViewDB::BatchWrite(const std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) {
    LogPrint("coindb", "Committing %u changed transactions to coin database %s...\n", (unsigned int)mapCoins.size(), path.string());

    CLevelDBBatch batch;
    for (std::map<uint256, Bitcoin_CClaimCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        BatchWriteCoins(batch, it->first, it->second);
    if (hashBlock != uint256(0))
        BatchWriteHashBestChain(batch, hashBlock);
    if (hashBitcreditClaimTip != uint256(0))
        BatchWriteHashBitcreditClaimTip(batch, hashBitcreditClaimTip);
    if (totalClaimedCoins != int64_t(0))
        BatchWriteTotalClaimedCoins(batch, totalClaimedCoins);

    return db.WriteBatch(batch);
}

bool Bitcoin_CClaimCoinsViewDB::GetCoinSlice(std::map<uint256, Bitcoin_CClaimCoins> &mapCoins, const int& size, const bool& firstInvocation, bool& fMore) {
    LogPrint("coindb", "Getting coin slice from claim coin database %s...\n", path.string());

	//Iterate over tmp db and move everything over to base
    if(firstInvocation) {
		assert_with_stacktrace(!ptmpcursor, "Invocation attempt of Bitcoin_CClaimCoinsViewDB::GetCoinSlice() without pointer being reset");

    	ptmpcursor = db.NewIterator();
    	ptmpcursor->SeekToFirst();
    }

	fMore = true;

	int count = 0;
	while (ptmpcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
			leveldb::Slice slKey = ptmpcursor->key();
			CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, Bitcoin_Params().ClientVersion());
			char chType;
			ssKey >> chType;
			if (chType == COIN_KEY) {
				leveldb::Slice slValue = ptmpcursor->value();
				CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, Bitcoin_Params().ClientVersion());
				Bitcoin_CClaimCoins coins;
				ssValue >> coins;
				uint256 txhash;
				ssKey >> txhash;

				mapCoins[txhash] = coins;
				count++;
			} else if (chType == PRUNED_COIN_KEY){
				uint256 txhash;
				ssKey >> txhash;

				mapCoins[txhash] = Bitcoin_CClaimCoins();
				count++;
			}
			ptmpcursor->Next();

			if(count == size) {
				return true;
			}
		} catch (std::exception &e) {
			return error("%s : Deserialize or I/O error - %s", __func__, e.what());
		}
	}

	fMore = false;

	delete ptmpcursor;
	ptmpcursor = NULL;
	return true;
}

bool Bitcoin_CClaimCoinsViewDB::GetStats(Bitcoin_CClaimCoinsStats &stats) {
    leveldb::Iterator *pcursor = db.NewIterator();
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, BITCOIN_PROTOCOL_VERSION);
    stats.hashBlock = GetBestBlock();
    ss << stats.hashBlock;
    stats.hashBitcreditClaimTip = GetBitcreditClaimTip();
    ss << stats.hashBitcreditClaimTip;
    stats.totalClaimedCoins = GetTotalClaimedCoins();
    ss << stats.totalClaimedCoins;
    int64_t nTotalAmountOriginal = 0;
    int64_t nTotalAmountClaimable = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, Bitcoin_Params().ClientVersion());
            char chType;
            ssKey >> chType;
            if (chType == 'c') {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, Bitcoin_Params().ClientVersion());
                Bitcoin_CClaimCoins coins;
                ssValue >> coins;
                uint256 txhash;
                ssKey >> txhash;
                ss << txhash;
                ss << VARINT(coins.nVersion);
                ss << (coins.fCoinBase ? 'c' : 'n');
                ss << VARINT(coins.nHeight);
                for (unsigned int i=0; i<coins.vout.size(); i++) {
                    const CTxOutClaim &out = coins.vout[i];
                    if (!out.IsNull()) {
                        ss << VARINT(i+1);
                        ss << out;

                        stats.nTransactionOutputsOriginal++;
                        nTotalAmountOriginal += out.nValueOriginal;

						if (out.nValueClaimable > 0) {
							stats.nTransactionOutputsClaimable++;
							nTotalAmountClaimable += out.nValueClaimable;
						}
                    }
                }
                stats.nSerializedSize += 32 + slValue.size();
                ss << VARINT(0);
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;
    stats.nHeight = bitcoin_mapBlockIndex.find(GetBestBlock())->second->nHeight;
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmountOriginal = nTotalAmountOriginal;
    stats.nTotalAmountClaimable = nTotalAmountClaimable;
    return true;
}

Bitcoin_CClaimCoinsViewDB::Bitcoin_CClaimCoinsViewDB(const boost::filesystem::path &pathIn, size_t nCacheSize, bool trackPrunedIn, bool deleteOnDestroyIn, bool fMemory, bool fWipe) : db(pathIn, nCacheSize, fMemory, fWipe), trackPruned(trackPrunedIn), deleteOnDestroy(deleteOnDestroyIn), path(pathIn), ptmpcursor(NULL){ }
Bitcoin_CClaimCoinsViewDB::~Bitcoin_CClaimCoinsViewDB() {
	if(deleteOnDestroy) {
        db.DestroyDB();
		TryRemoveDirectory(path);
	}
}
