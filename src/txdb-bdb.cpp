// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb-bdb.h"

bool CCoinsDB::HaveCoins(uint256 hash) {
    assert(!fClient);
    return Exists(make_pair('c', hash));
}

bool CCoinsDB::ReadCoins(uint256 hash, CCoins &coins) {
    assert(!fClient);
    return Read(make_pair('c', hash), coins);
}

bool CCoinsDB::WriteCoins(uint256 hash, const CCoins &coins) {
    assert(!fClient);
    if (coins.IsPruned())
        return Erase(make_pair('c', hash));
    else
        return Write(make_pair('c', hash), coins);
}

bool CCoinsDB::ReadHashBestChain(uint256& hashBestChain)
{
    return Read('B', hashBestChain);
}

bool CCoinsDB::WriteHashBestChain(uint256 hashBestChain)
{
    return Write('B', hashBestChain);
}

bool CBlockTreeDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(make_pair('b', blockindex.GetBlockHash()), blockindex);
}

bool CBlockTreeDB::ReadBestInvalidWork(CBigNum& bnBestInvalidWork)
{
    return Read('I', bnBestInvalidWork);
}

bool CBlockTreeDB::WriteBestInvalidWork(CBigNum bnBestInvalidWork)
{
    return Write('I', bnBestInvalidWork);
}

bool CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo &info) {
    return Write(make_pair('f', nFile), info);
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(make_pair('f', nFile), info);
}

bool CBlockTreeDB::WriteLastBlockFile(int nFile) {
    return Write('l', nFile);
}

bool CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read('l', nFile);
}

CCoinsViewDB::CCoinsViewDB() : db("cr+") {}
bool CCoinsViewDB::GetCoins(uint256 txid, CCoins &coins) { return db.ReadCoins(txid, coins); }
bool CCoinsViewDB::SetCoins(uint256 txid, const CCoins &coins) { return db.WriteCoins(txid, coins); }
bool CCoinsViewDB::HaveCoins(uint256 txid) { return db.HaveCoins(txid); }
CBlockIndex *CCoinsViewDB::GetBestBlock() {
    uint256 hashBestChain;
    if (!db.ReadHashBestChain(hashBestChain))
        return NULL;
    std::map<uint256, CBlockIndex*>::iterator it = mapBlockIndex.find(hashBestChain);
    if (it == mapBlockIndex.end())
        return NULL;
    return it->second;
}
bool CCoinsViewDB::SetBestBlock(CBlockIndex *pindex) { return db.WriteHashBestChain(pindex->GetBlockHash()); }
bool CCoinsViewDB::BatchWrite(const std::map<uint256, CCoins> &mapCoins, CBlockIndex *pindex) {
    printf("Committing %u changed transactions to coin database...\n", (unsigned int)mapCoins.size());

    if (!db.TxnBegin())
        return false;
    bool fOk = true;
    for (std::map<uint256, CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++) {
        fOk = db.WriteCoins(it->first, it->second);
        if (!fOk)
            break;
    }
    if (fOk)
        fOk = db.WriteHashBestChain(pindex->GetBlockHash());

    if (!fOk)
        db.TxnAbort();
    else
        fOk = db.TxnCommit();

    return fOk;
}


bool CBlockTreeDB::LoadBlockIndexGuts()
{
    // Get database cursor
    Dbc* pcursor = GetCursor();
    if (!pcursor)
        return false;

    // Load mapBlockIndex
    unsigned int fFlags = DB_SET_RANGE;
    loop
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << make_pair('b', uint256(0));
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
            break;
        else if (ret != 0)
            return false;

        // Unserialize

        try {
        char chType;
        ssKey >> chType;
        if (chType == 'b' && !fRequestShutdown)
        {
            CDiskBlockIndex diskindex;
            ssValue >> diskindex;

            // Construct block index object
            CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
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

            // Watch for genesis block
            if (pindexGenesisBlock == NULL && diskindex.GetBlockHash() == hashGenesisBlock)
                pindexGenesisBlock = pindexNew;

            if (!pindexNew->CheckIndex())
                return error("LoadBlockIndex() : CheckIndex failed: %s", pindexNew->ToString().c_str());
        }
        else
        {
            break; // if shutdown requested or finished loading block index
        }
        }    // try
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    pcursor->close();

    return true;
}
