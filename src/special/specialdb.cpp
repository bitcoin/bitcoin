// Copyright (c) 2018 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <special/specialdb.h>

std::unique_ptr<CSpecialDB> pspecialdb;

CSpecialDB::CSpecialDB(size_t nCacheSize, bool fMemory, bool fWipe) :
    db(fMemory ? "" : (GetDataDir() / "specialdb"), nCacheSize, fMemory, fWipe),
    rootBatch(db),
    rootDBTransaction(db, rootBatch),
    curDBTransaction(rootDBTransaction, rootDBTransaction)
{
}

bool CSpecialDB::CommitRootTransaction()
{
    assert(curDBTransaction.IsClean());
    rootDBTransaction.Commit();
    bool ret = db.WriteBatch(rootBatch);
    rootBatch.Clear();
    return ret;
}

bool CSpecialDB::VerifyBestBlock(const uint256& hash)
{
    // Make sure specialdb is consistent.
    // If we already have best block hash saved, the previous block should match it.
    uint256 hashBestBlock;
    bool fHasBestBlock = Read("b_b", hashBestBlock);
    uint256 hashBlockIndex = fHasBestBlock ? hash : uint256();
    assert(hashBestBlock == hashBlockIndex);

    return fHasBestBlock;
}

void CSpecialDB::WriteBestBlock(const uint256& hash)
{
    Write("b_b", hash);
}
