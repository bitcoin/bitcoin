// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_txdb.h"

#include "bitcoin_core.h"
#include "uint256.h"

#include <stdint.h>

using namespace std;

const unsigned char Bitcoin_CCoinsViewDB::BITCOIN_COIN_KEY = 'c';
const unsigned char Bitcoin_CCoinsViewDB::BITCOIN_BEST_CHAIN_KEY = 'B';
const unsigned char Bitcoin_CCoinsViewDB::CLAIM_BEST_CHAIN_KEY = 'C';
const unsigned char Bitcoin_CCoinsViewDB::CLAIM_BITCREDIT_CLAIM_TIP_KEY = 'R';
const unsigned char Bitcoin_CCoinsViewDB::CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY = 'T';

void Bitcoin_CCoinsViewDB::BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Bitcoin_CCoins &coins) {
    if (coins.IsPruned())
        batch.Erase(make_pair(BITCOIN_COIN_KEY, hash));
    else
        batch.Write(make_pair(BITCOIN_COIN_KEY, hash), coins);
}
void Bitcoin_CCoinsViewDB::Bitcoin_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(BITCOIN_BEST_CHAIN_KEY, hash);
}
void Bitcoin_CCoinsViewDB::Claim_BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(CLAIM_BEST_CHAIN_KEY, hash);
}
void Bitcoin_CCoinsViewDB::Claim_BatchWriteHashCreditsClaimTip(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(CLAIM_BITCREDIT_CLAIM_TIP_KEY, hash);
}
void Bitcoin_CCoinsViewDB::Claim_BatchWriteTotalClaimedCoins(CLevelDBBatch &batch, const int64_t &totalClaimedCoins) {
    batch.Write(CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY, totalClaimedCoins);
}

bool Bitcoin_CCoinsViewDB::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) {
    return db.Read(make_pair(BITCOIN_COIN_KEY, txid), coins);
}

bool Bitcoin_CCoinsViewDB::SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) {
    CLevelDBBatch batch;
    BatchWriteCoins(batch, txid, coins);
    return db.WriteBatch(batch);
}

bool Bitcoin_CCoinsViewDB::HaveCoins(const uint256 &txid) {
    return db.Exists(make_pair(BITCOIN_COIN_KEY, txid));
}

uint256 Bitcoin_CCoinsViewDB::Bitcoin_GetBestBlock() {
    uint256 hashBestChain;
    if (!db.Read(BITCOIN_BEST_CHAIN_KEY, hashBestChain))
        return uint256(0);
    return hashBestChain;
}
uint256 Bitcoin_CCoinsViewDB::Claim_GetBestBlock() {
    uint256 hashBestChain;
    if (!db.Read(CLAIM_BEST_CHAIN_KEY, hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool Bitcoin_CCoinsViewDB::Bitcoin_SetBestBlock(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    Bitcoin_BatchWriteHashBestChain(batch, hashBlock);
    return db.WriteBatch(batch);
}
bool Bitcoin_CCoinsViewDB::Claim_SetBestBlock(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    Claim_BatchWriteHashBestChain(batch, hashBlock);
    return db.WriteBatch(batch);
}

uint256 Bitcoin_CCoinsViewDB::Claim_GetCreditsClaimTip() {
    uint256 hash;
    if (!db.Read(CLAIM_BITCREDIT_CLAIM_TIP_KEY, hash))
        return uint256(0);
    return hash;
}
bool Bitcoin_CCoinsViewDB::Claim_SetCreditsClaimTip(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    Claim_BatchWriteHashCreditsClaimTip(batch, hashBlock);
    return db.WriteBatch(batch);
}

int64_t Bitcoin_CCoinsViewDB::Claim_GetTotalClaimedCoins() {
	int64_t totalClaimedCoins;
    if (!db.Read(CLAIM_BITCREDIT_TOTAL_CLAIMED_COINS_KEY, totalClaimedCoins))
        return int64_t(0);
    return totalClaimedCoins;
}
bool Bitcoin_CCoinsViewDB::Claim_SetTotalClaimedCoins(const int64_t &totalClaimedCoins) {
    CLevelDBBatch batch;
    Claim_BatchWriteTotalClaimedCoins(batch, totalClaimedCoins);
    return db.WriteBatch(batch);
}

bool Bitcoin_CCoinsViewDB::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &bitcoin_hashBlock, const uint256 &claim_hashBlock, const uint256 &claim_hashCreditsClaimTip, const int64_t &claim_totalClaimedCoins) {
    LogPrint("coindb", "(All batch write) Committing %u changed transactions to coin database...\n", (unsigned int)mapCoins.size());

    CLevelDBBatch batch;
    for (std::map<uint256, Bitcoin_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
    	BatchWriteCoins(batch, it->first, it->second);

    if (bitcoin_hashBlock != uint256(0))
    	Bitcoin_BatchWriteHashBestChain(batch, bitcoin_hashBlock);

    if (claim_hashBlock != uint256(0))
    	Claim_BatchWriteHashBestChain(batch, claim_hashBlock);
    if (claim_hashCreditsClaimTip != uint256(0))
    	Claim_BatchWriteHashCreditsClaimTip(batch, claim_hashCreditsClaimTip);
    if (claim_totalClaimedCoins != int64_t(0))
    	Claim_BatchWriteTotalClaimedCoins(batch, claim_totalClaimedCoins);

    return db.WriteBatch(batch);
}

bool Bitcoin_CCoinsViewDB::GetStats(Bitcoin_CCoinsStats &stats) {
    leveldb::Iterator *pcursor = db.NewIterator();
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, BITCOIN_PROTOCOL_VERSION);
    stats.bitcoin_hashBlock = Bitcoin_GetBestBlock();
    ss << stats.bitcoin_hashBlock;
    stats.claim_hashBlock = Claim_GetBestBlock();
    ss << stats.claim_hashBlock;
    stats.hashCreditsClaimTip = Claim_GetCreditsClaimTip();
    ss << stats.hashCreditsClaimTip;
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
                Bitcoin_CCoins coins;
                ssValue >> coins;
                uint256 txhash;
                ssKey >> txhash;
                ss << txhash;
                ss << VARINT(coins.nVersion);
                ss << (coins.fCoinBase ? 'c' : 'n');
                ss << VARINT(coins.nHeight);
                for (unsigned int i=0; i<coins.vout.size(); i++) {
                    const Bitcoin_CTxOut &out = coins.vout[i];
                    if (!out.IsNull()) {
                        ss << VARINT(i+1);
                        ss << out;

                        if(!out.nValueOriginalHasBeenSpent) {
                        	stats.nTransactionOutputsOriginal++;
                        	nTotalAmountOriginal += out.nValueOriginal;
                        }

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

Bitcoin_CCoinsViewDB::Bitcoin_CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "bitcoin_chainstate", nCacheSize, fMemory, fWipe) { }

//-----------------------------------------------


Bitcoin_CBlockTreeDB::Bitcoin_CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "bitcoin_blocks" / "index", nCacheSize, fMemory, fWipe) {
}

bool Bitcoin_CBlockTreeDB::WriteBlockIndex(const Bitcoin_CDiskBlockIndex& blockindex)
{
    return Write(make_pair('b', blockindex.GetBlockHash()), blockindex);
}
bool Bitcoin_CBlockTreeDB::BatchWriteBlockIndex(std::vector<Bitcoin_CDiskBlockIndex> &vblockindexes)
{
    CLevelDBBatch batch;
    for (unsigned int i = 0; i < vblockindexes.size(); i++) {
    	Bitcoin_CDiskBlockIndex &blockindex = vblockindexes[i];
    	batch.Write(make_pair('b', blockindex.GetBlockHash()), blockindex);
    }
    return WriteBatch(batch);
}
bool Bitcoin_CBlockTreeDB::WriteBlockTxHashesWithInputs(const Bitcoin_CDiskBlockIndex& blockindex, const std::vector<pair<uint256, std::vector<COutPoint> > > &vTxHashesWithInputs)
{
    return Write(make_pair('h', blockindex.GetBlockHash()), vTxHashesWithInputs);
}
bool Bitcoin_CBlockTreeDB::BatchWriteBlockTxHashesWithInputs(std::vector<Bitcoin_CDiskBlockIndex>& vblockindexes, const std::vector<std::vector<pair<uint256, std::vector<COutPoint> > > > &vvTxHashesWithInputs)
{
    CLevelDBBatch batch;
    for (unsigned int i = 0; i < vblockindexes.size(); i++) {
    	Bitcoin_CDiskBlockIndex &blockindex = vblockindexes[i];
    	batch.Write(make_pair('h', blockindex.GetBlockHash()), vvTxHashesWithInputs[i]);
    }
    return WriteBatch(batch);
}
bool Bitcoin_CBlockTreeDB::ReadBlockTxHashesWithInputs(const uint256 &blockHash, std::vector<pair<uint256, std::vector<COutPoint> > > &vTxHashesWithInputs) {
    return Read(make_pair('h', blockHash), vTxHashesWithInputs);
}

bool Bitcoin_CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo &info) {
    return Write(make_pair('f', nFile), info);
}

bool Bitcoin_CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(make_pair('f', nFile), info);
}

bool Bitcoin_CBlockTreeDB::WriteLastBlockFile(int nFile) {
    return Write('l', nFile);
}

bool Bitcoin_CBlockTreeDB::WriteTrimToTime(int nTrimToTime) {
    return Write('T', nTrimToTime);
}

bool Bitcoin_CBlockTreeDB::ReadTrimToTime(int &nTrimToTime) {
    return Read('T', nTrimToTime);
}

bool Bitcoin_CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write('R', '1');
    else
        return Erase('R');
}

bool Bitcoin_CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists('R');
    return true;
}

bool Bitcoin_CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read('l', nFile);
}

bool Bitcoin_CBlockTreeDB::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return Read(make_pair('t', txid), pos);
}

bool Bitcoin_CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >&vect) {
    CLevelDBBatch batch;
    for (std::vector<std::pair<uint256,CDiskTxPos> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair('t', it->first), it->second);
    return WriteBatch(batch);
}

bool Bitcoin_CBlockTreeDB::WriteFlag(const std::string &name, bool fValue) {
    return Write(std::make_pair('F', name), fValue ? '1' : '0');
}

bool Bitcoin_CBlockTreeDB::ReadFlag(const std::string &name, bool &fValue) {
    char ch;
    if (!Read(std::make_pair('F', name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

Bitcoin_CBlockIndex * Bitcoin_CBlockTreeDB::InsertBlockIndex(uint256 hash)
{
    if (hash == 0)
        return NULL;

    // Return existing
    map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(hash);
    if (mi != bitcoin_mapBlockIndex.end())
        return (*mi).second;

    // Create new
    Bitcoin_CBlockIndex* pindexNew = new Bitcoin_CBlockIndex();
    if (!pindexNew)
        throw runtime_error("LoadBlockIndex() : new CBlockIndex failed");
    mi = bitcoin_mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);

    return pindexNew;
}

bool Bitcoin_CBlockTreeDB::LoadBlockIndexGuts()
{
    leveldb::Iterator *pcursor = NewIterator();

    CDataStream ssKeySet(SER_DISK, Bitcoin_Params().ClientVersion());
    ssKeySet << make_pair('b', uint256(0));
    pcursor->Seek(ssKeySet.str());

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, Bitcoin_Params().ClientVersion());
            char chType;
            ssKey >> chType;
            if (chType == 'b') {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, Bitcoin_Params().ClientVersion());
                Bitcoin_CDiskBlockIndex diskindex;
                ssValue >> diskindex;

                // Construct block index object
                Bitcoin_CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev          = InsertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight        = diskindex.nHeight;
                pindexNew->nFile          = diskindex.nFile;
                pindexNew->nDataPos       = diskindex.nDataPos;
                pindexNew->nUndoPos       = diskindex.nUndoPos;
                pindexNew->nVersion       = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime          = diskindex.nTime;
                pindexNew->nBits          = diskindex.nBits;
                pindexNew->nNonce         = diskindex.nNonce;
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
