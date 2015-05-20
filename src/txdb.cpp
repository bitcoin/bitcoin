// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "core.h"
#include "bitcoin_core.h"
#include "uint256.h"

#include <stdint.h>

using namespace std;

const unsigned char Credits_CCoinsViewDB::CREDITS_COIN_KEY = 'c';
const unsigned char Credits_CCoinsViewDB::CREDITS_BEST_CHAIN_KEY = 'B';

const unsigned char Credits_CCoinsViewDB::CLAIM_COIN_KEY = 'd';
const unsigned char Credits_CCoinsViewDB::CLAIM_BEST_CHAIN_KEY = 'C';
const unsigned char Credits_CCoinsViewDB::CLAIM_BITCREDIT_CLAIM_TIP_KEY = 'R';
const unsigned char Credits_CCoinsViewDB::CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY = 'T';

void Credits_CCoinsViewDB::Credits_BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Credits_CCoins &coins) {
    if (coins.IsPruned())
        batch.Erase(make_pair(CREDITS_COIN_KEY, hash));
    else
        batch.Write(make_pair(CREDITS_COIN_KEY, hash), coins);
}
void Credits_CCoinsViewDB::Claim_BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Claim_CCoins &coins) {
    if (coins.IsPruned())
    	batch.Erase(make_pair(CLAIM_COIN_KEY, hash));
    else
        batch.Write(make_pair(CLAIM_COIN_KEY, hash), coins);
}
void Credits_CCoinsViewDB::Credits_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(CREDITS_BEST_CHAIN_KEY, hash);
}
void Credits_CCoinsViewDB::Claim_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(CLAIM_BEST_CHAIN_KEY, hash);
}
void Credits_CCoinsViewDB::Claim_BatchWriteHashBitcreditClaimTip(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(CLAIM_BITCREDIT_CLAIM_TIP_KEY, hash);
}
void Credits_CCoinsViewDB::Claim_BatchWriteTotalClaimedCoins(CLevelDBBatch &batch, const int64_t &totalClaimedCoins) {
    batch.Write(CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY, totalClaimedCoins);
}

bool Credits_CCoinsViewDB::Credits_GetCoins(const uint256 &txid, Credits_CCoins &coins) {
    return db.Read(make_pair(CREDITS_COIN_KEY, txid), coins);
}
bool Credits_CCoinsViewDB::Claim_GetCoins(const uint256 &txid, Claim_CCoins &coins) {
    return db.Read(make_pair(CLAIM_COIN_KEY, txid), coins);
}

bool Credits_CCoinsViewDB::Credits_SetCoins(const uint256 &txid, const Credits_CCoins &coins) {
    CLevelDBBatch batch;
    Credits_BatchWriteCoins(batch, txid, coins);
    return db.WriteBatch(batch);
}
bool Credits_CCoinsViewDB::Claim_SetCoins(const uint256 &txid, const Claim_CCoins &coins) {
    CLevelDBBatch batch;
    Claim_BatchWriteCoins(batch, txid, coins);
    return db.WriteBatch(batch);
}

bool Credits_CCoinsViewDB::Credits_HaveCoins(const uint256 &txid) {
    return db.Exists(make_pair(CREDITS_COIN_KEY, txid));
}
bool Credits_CCoinsViewDB::Claim_HaveCoins(const uint256 &txid) {
    return db.Exists(make_pair(CLAIM_COIN_KEY, txid));
}

uint256 Credits_CCoinsViewDB::Credits_GetBestBlock() {
    uint256 hashBestChain;
    if (!db.Read(CREDITS_BEST_CHAIN_KEY, hashBestChain))
        return uint256(0);
    return hashBestChain;
}
uint256 Credits_CCoinsViewDB::Claim_GetBestBlock() {
    uint256 hashBestChain;
    if (!db.Read(CLAIM_BEST_CHAIN_KEY, hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool Credits_CCoinsViewDB::Credits_SetBestBlock(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    Credits_BatchWriteHashBestChain(batch, hashBlock);
    return db.WriteBatch(batch);
}
bool Credits_CCoinsViewDB::Claim_SetBestBlock(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    Claim_BatchWriteHashBestChain(batch, hashBlock);
    return db.WriteBatch(batch);
}

uint256 Credits_CCoinsViewDB::Claim_GetBitcreditClaimTip() {
    uint256 hash;
    if (!db.Read(CLAIM_BITCREDIT_CLAIM_TIP_KEY, hash))
        return uint256(0);
    return hash;
}
bool Credits_CCoinsViewDB::Claim_SetBitcreditClaimTip(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    Claim_BatchWriteHashBitcreditClaimTip(batch, hashBlock);
    return db.WriteBatch(batch);
}

int64_t Credits_CCoinsViewDB::Claim_GetTotalClaimedCoins() {
	int64_t totalClaimedCoins;
    if (!db.Read(CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY, totalClaimedCoins))
        return int64_t(0);
    return totalClaimedCoins;
}
bool Credits_CCoinsViewDB::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) {
    CLevelDBBatch batch;
    Claim_BatchWriteTotalClaimedCoins(batch, totalClaimedCoins);
    return db.WriteBatch(batch);
}

bool Credits_CCoinsViewDB::Credits_BatchWrite(const std::map<uint256, Credits_CCoins> &mapCoins, const uint256 &hashBlock) {
    LogPrint("coindb", "Committing %u changed transactions to coin database...\n", (unsigned int)mapCoins.size());

    CLevelDBBatch batch;
    for (std::map<uint256, Credits_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	Credits_BatchWriteCoins(batch, it->first, it->second);
    if (hashBlock != uint256(0))
    	Credits_BatchWriteHashBestChain(batch, hashBlock);

    return db.WriteBatch(batch);
}
bool Credits_CCoinsViewDB::Claim_BatchWrite(const std::map<uint256, Claim_CCoins> &mapCoins, const uint256 &hashBlock, const uint256 &hashBitcreditClaimTip, const int64_t &totalClaimedCoins) {
    LogPrint("coindb", "Committing %u changed transactions to coin database...\n", (unsigned int)mapCoins.size());

    CLevelDBBatch batch;
    for (std::map<uint256, Claim_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	Claim_BatchWriteCoins(batch, it->first, it->second);
    if (hashBlock != uint256(0))
    	Claim_BatchWriteHashBestChain(batch, hashBlock);
    if (hashBitcreditClaimTip != uint256(0))
    	Claim_BatchWriteHashBitcreditClaimTip(batch, hashBitcreditClaimTip);
    if (totalClaimedCoins != int64_t(0))
    	Claim_BatchWriteTotalClaimedCoins(batch, totalClaimedCoins);

    return db.WriteBatch(batch);
}

bool Credits_CCoinsViewDB::Credits_GetStats(Credits_CCoinsStats &stats) {
    leveldb::Iterator *pcursor = db.NewIterator();
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, CREDITS_PROTOCOL_VERSION);
    stats.hashBlock = Credits_GetBestBlock();
    ss << stats.hashBlock;
    int64_t nTotalAmount = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CREDITS_CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            if (chType == 'c') {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CREDITS_CLIENT_VERSION);
                Credits_CCoins coins;
                ssValue >> coins;
                uint256 txhash;
                ssKey >> txhash;
                ss << txhash;
                ss << VARINT(coins.nMetaData);
                ss << VARINT(coins.nVersion);
                ss << (coins.fCoinBase ? 'c' : 'n');
                ss << VARINT(coins.nHeight);
                stats.nTransactions++;
                for (unsigned int i=0; i<coins.vout.size(); i++) {
                    const CTxOut &out = coins.vout[i];
                    if (!out.IsNull()) {
                        stats.nTransactionOutputs++;
                        ss << VARINT(i+1);
                        ss << out;
                        nTotalAmount += out.nValue;
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
    stats.nHeight = credits_mapBlockIndex.find(Credits_GetBestBlock())->second->nHeight;
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    return true;
}

bool Credits_CCoinsViewDB::Claim_GetStats(Claim_CCoinsStats &stats) {
    leveldb::Iterator *pcursor = db.NewIterator();
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, BITCOIN_PROTOCOL_VERSION);
    stats.hashBlock = Claim_GetBestBlock();
    ss << stats.hashBlock;
    stats.hashBitcreditClaimTip = Claim_GetBitcreditClaimTip();
    ss << stats.hashBitcreditClaimTip;
    stats.totalClaimedCoins = Claim_GetTotalClaimedCoins();
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
                Claim_CCoins coins;
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
    stats.nHeight = bitcoin_mapBlockIndex.find(Claim_GetBestBlock())->second->nHeight;
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmountOriginal = nTotalAmountOriginal;
    stats.nTotalAmountClaimable = nTotalAmountClaimable;
    return true;
}

Credits_CCoinsViewDB::Credits_CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "credits_chainstate", nCacheSize, fMemory, fWipe) { }

//-----------------------------------------------

const unsigned char Credits_CBlockTreeDB::BLOCKINDEX_KEY = 'b';
const unsigned char Credits_CBlockTreeDB::REINDEX_KEY = 'R';
const unsigned char Credits_CBlockTreeDB::FILE_KEY = 'f';
const unsigned char Credits_CBlockTreeDB::FLAG_KEY = 'F';
const unsigned char Credits_CBlockTreeDB::LAST_BLOCK_KEY = 'l';
const unsigned char Credits_CBlockTreeDB::TX_KEY = 't';
const unsigned char Credits_CBlockTreeDB::ONE = '1';
const unsigned char Credits_CBlockTreeDB::ZERO = '0';

Credits_CBlockTreeDB::Credits_CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "credits_blocks" / "index", nCacheSize, fMemory, fWipe) {
}

bool Credits_CBlockTreeDB::WriteBlockIndex(const Credits_CDiskBlockIndex& blockindex)
{
    return Write(make_pair(BLOCKINDEX_KEY, blockindex.GetBlockHash()), blockindex);
}

bool Credits_CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo &info) {
    return Write(make_pair(FILE_KEY, nFile), info);
}

bool Credits_CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(make_pair(FILE_KEY, nFile), info);
}

bool Credits_CBlockTreeDB::WriteLastBlockFile(int nFile) {
    return Write(LAST_BLOCK_KEY, nFile);
}

bool Credits_CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read(LAST_BLOCK_KEY, nFile);
}

bool Credits_CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write(REINDEX_KEY, ONE);
    else
        return Erase(REINDEX_KEY);
}

bool Credits_CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists(REINDEX_KEY);
    return true;
}

bool Credits_CBlockTreeDB::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return Read(make_pair(TX_KEY, txid), pos);
}

bool Credits_CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >&vect) {
    CLevelDBBatch batch;
    for (std::vector<std::pair<uint256,CDiskTxPos> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair(TX_KEY, it->first), it->second);
    return WriteBatch(batch);
}

bool Credits_CBlockTreeDB::WriteFlag(const std::string &name, bool fValue) {
    return Write(std::make_pair(FLAG_KEY, name), fValue ? ONE : ZERO);
}

bool Credits_CBlockTreeDB::ReadFlag(const std::string &name, bool &fValue) {
    char ch;
    if (!Read(std::make_pair(FLAG_KEY, name), ch))
        return false;
    fValue = ch == ONE;
    return true;
}

bool Credits_CBlockTreeDB::LoadBlockIndexGuts()
{
    leveldb::Iterator *pcursor = NewIterator();

    CDataStream ssKeySet(SER_DISK, CREDITS_CLIENT_VERSION);
    ssKeySet << make_pair(BLOCKINDEX_KEY, uint256(0));
    pcursor->Seek(ssKeySet.str());

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CREDITS_CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            if (chType == BLOCKINDEX_KEY) {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CREDITS_CLIENT_VERSION);
                Credits_CDiskBlockIndex diskindex;
                ssValue >> diskindex;

                // Construct block index object
                Credits_CBlockIndex* pindexNew = Bitcredit_InsertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev          = Bitcredit_InsertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight        = diskindex.nHeight;
                pindexNew->nFile          = diskindex.nFile;
                pindexNew->nDataPos       = diskindex.nDataPos;
                pindexNew->nUndoPos       = diskindex.nUndoPos;
                pindexNew->nVersion       = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->hashLinkedBitcoinBlock         = diskindex.hashLinkedBitcoinBlock;
                pindexNew->hashSigMerkleRoot        = diskindex.hashSigMerkleRoot;
                pindexNew->nTime          = diskindex.nTime;
                pindexNew->nBits          = diskindex.nBits;
                pindexNew->nNonce         = diskindex.nNonce;
                pindexNew->nTotalMonetaryBase         = diskindex.nTotalMonetaryBase;
                pindexNew->nTotalDepositBase         = diskindex.nTotalDepositBase;
                pindexNew->nDepositAmount         = diskindex.nDepositAmount;
                pindexNew->nStatus        = diskindex.nStatus;
                pindexNew->nTx            = diskindex.nTx;

                if (!pindexNew->CheckIndex())
                    return error("LoadBlockIndex() : CheckIndex failed: %s", pindexNew->ToString());

                pcursor->Next();
            } else {
                break; // if shutdown requested or finished loading block index
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}
