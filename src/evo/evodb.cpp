// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/evodb.h>

std::unique_ptr<CEvoDB> evoDb;

CEvoDBScopedCommitter::CEvoDBScopedCommitter(CEvoDB &_evoDB) :
    evoDB(_evoDB)
{
}

CEvoDBScopedCommitter::~CEvoDBScopedCommitter()
{
    if (!didCommitOrRollback)
        Rollback();
}

void CEvoDBScopedCommitter::Commit()
{
    assert(!didCommitOrRollback);
    didCommitOrRollback = true;
    evoDB.CommitCurTransaction();
}

void CEvoDBScopedCommitter::Rollback()
{
    assert(!didCommitOrRollback);
    didCommitOrRollback = true;
    evoDB.RollbackCurTransaction();
}

CEvoDB::CEvoDB(size_t nCacheSize, bool fMemory, bool fWipe) :
    db(fMemory ? "" : (gArgs.GetDataDirNet() / "evodb"), nCacheSize, fMemory, fWipe),
    rootBatch(db),
    rootDBTransaction(db, rootBatch),
    curDBTransaction(rootDBTransaction, rootDBTransaction)
{
}

void CEvoDB::CommitCurTransaction()
{
    LOCK(cs);
    curDBTransaction.Commit();
}

void CEvoDB::RollbackCurTransaction()
{
    LOCK(cs);
    curDBTransaction.Clear();
}

bool CEvoDB::CommitRootTransaction()
{
    LOCK(cs);
    assert(curDBTransaction.IsClean());
    rootDBTransaction.Commit();
    bool ret = db.WriteBatch(rootBatch);
    rootBatch.Clear();
    return ret;
}

bool CEvoDB::VerifyBestBlock(const uint256& hash)
{
    // Make sure evodb is consistent.
    // If we already have best block hash saved, the previous block should match it.
    uint256 hashBestBlock;
    if (!Read(EVODB_BEST_BLOCK, hashBestBlock)) {
        return false;
    }
    return hashBestBlock == hash;
}

void CEvoDB::WriteBestBlock(const uint256& hash)
{
    Write(EVODB_BEST_BLOCK, hash);
}
