// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_txdb.h"

#include "bitcoin_core.h"
#include "uint256.h"

#include <stdint.h>

using namespace std;

const unsigned char Bitcoin_CCoinsViewDB::COIN_KEY = 'c';
const unsigned char Bitcoin_CCoinsViewDB::BEST_CHAIN_KEY = 'B';

void Bitcoin_CCoinsViewDB::BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const Bitcoin_CCoins &coins) {
    if (coins.IsPruned())
        batch.Erase(make_pair(COIN_KEY, hash));
    else
        batch.Write(make_pair(COIN_KEY, hash), coins);
}
void Bitcoin_CCoinsViewDB::BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(BEST_CHAIN_KEY, hash);
}

bool Bitcoin_CCoinsViewDB::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) {
    return db.Read(make_pair(COIN_KEY, txid), coins);
}

bool Bitcoin_CCoinsViewDB::SetCoins(const uint256 &txid, const Bitcoin_CCoins &coins) {
    CLevelDBBatch batch;
    BatchWriteCoins(batch, txid, coins);
    return db.WriteBatch(batch);
}

bool Bitcoin_CCoinsViewDB::HaveCoins(const uint256 &txid) {
    return db.Exists(make_pair(COIN_KEY, txid));
}

uint256 Bitcoin_CCoinsViewDB::GetBestBlock() {
    uint256 hashBestChain;
    if (!db.Read(BEST_CHAIN_KEY, hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool Bitcoin_CCoinsViewDB::SetBestBlock(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    BatchWriteHashBestChain(batch, hashBlock);
    return db.WriteBatch(batch);
}

bool Bitcoin_CCoinsViewDB::BatchWrite(const std::map<uint256, Bitcoin_CCoins> &mapCoins, const uint256 &hashBlock) {
    LogPrint("coindb", "Committing %u changed transactions to coin database...\n", (unsigned int)mapCoins.size());

    CLevelDBBatch batch;
    for (std::map<uint256, Bitcoin_CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        BatchWriteCoins(batch, it->first, it->second);
    if (hashBlock != uint256(0))
        BatchWriteHashBestChain(batch, hashBlock);

    return db.WriteBatch(batch);
}

bool Bitcoin_CCoinsViewDB::GetStats(Bitcoin_CCoinsStats &stats) {
    leveldb::Iterator *pcursor = db.NewIterator();
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, BITCOIN_PROTOCOL_VERSION);
    stats.hashBlock = GetBestBlock();
    ss << stats.hashBlock;
    int64_t nTotalAmount = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, Bitcoin_Params().ClientVersion());
            char chType;
            ssKey >> chType;
            if (chType == COIN_KEY) {
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
    stats.nHeight = bitcoin_mapBlockIndex.find(GetBestBlock())->second->nHeight;
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
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
                pindexNew->nUndoPosClaim       = diskindex.nUndoPosClaim;
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
