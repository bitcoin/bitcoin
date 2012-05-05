#include "blockstore.h"
#include "main.h"

class CBlockStoreCallbackCommitBlock : public CBlockStoreCallback
{
private:
    CBlock block;
public:
    CBlockStoreCallbackCommitBlock(const CBlock &blockIn) : block(blockIn) {}
    void Signal(const CBlockStoreSignalTable& sigtable) { sigtable.sigCommitBlock(block); }
};

class CBlockStoreCallbackAskForBlocks : public CBlockStoreCallback
{
private:
    uint256 hashEnd, hashOrig;
public:
    CBlockStoreCallbackAskForBlocks(uint256 hashEndIn, uint256 hashOrigIn) : hashEnd(hashEndIn), hashOrig(hashOrigIn) {}
    void Signal(const CBlockStoreSignalTable& sigtable) { sigtable.sigAskForBlocks(hashEnd, hashOrig); }
};

class CBlockStoreCallbackRelayed : public CBlockStoreCallback
{
private:
    uint256 hash;
public:
    CBlockStoreCallbackRelayed(uint256 hashIn) : hash(hashIn) {}
    void Signal(const CBlockStoreSignalTable& sigtable) { sigtable.sigRelayed(hash); }
};

class CBlockStoreCallbackTransactionReplaced : public CBlockStoreCallback
{
private:
    uint256 hash;
public:
    CBlockStoreCallbackTransactionReplaced(uint256 hashIn) : hash(hashIn) {}
    void Signal(const CBlockStoreSignalTable& sigtable) { sigtable.sigTransactionReplaced(hash); }
};

class CBlockStoreCallbackCommitTransactionToMemoryPool : public CBlockStoreCallback
{
private:
    CTransaction tx;
public:
    CBlockStoreCallbackCommitTransactionToMemoryPool(const CTransaction &txIn) : tx(txIn) {}
    void Signal(const CBlockStoreSignalTable& sigtable) { sigtable.sigCommitTransactionToMemoryPool(tx); }
};

void CBlockStore::AskForBlocks(const uint256 hashEnd, const uint256 hashOriginator)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackAskForBlocks(hashEnd, hashOriginator));
    sem_callbacks.post();
}

void CBlockStore::Relayed(const uint256 hash)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackRelayed(hash));
    sem_callbacks.post();
}

void CBlockStore::TransactionReplaced(const uint256 hash)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackTransactionReplaced(hash));
    sem_callbacks.post();
}

void CBlockStore::SubmitCallbackCommitTransactionToMemoryPool(const CTransaction &tx)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackCommitTransactionToMemoryPool(tx));
    sem_callbacks.post();
}

void CBlockStore::SubmitCallbackCommitBlock(const CBlock &block)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackCommitBlock(block));
    sem_callbacks.post();
}

void ProcessCallbacks(void* parg)
{
    ((CBlockStore*)parg)->ProcessCallbacks();
}

void CBlockStore::ProcessCallbacks()
{
    {
        LOCK(cs_callbacks);
        fProcessCallbacks = true;
    }

    loop
    {
        CBlockStoreCallback *pcallback = NULL;
        sem_callbacks.wait();
        if (fProcessCallbacks)
        {
            LOCK(cs_callbacks);
            assert(queueCallbacks.size() > 0);
            pcallback = queueCallbacks.front();
            queueCallbacks.pop();
        }
        else
            return;

        pcallback->Signal(sigtable);
        delete pcallback;
    }
}

void CBlockStore::StopProcessCallbacks()
{
    {
        LOCK(cs_callbacks);
        fProcessCallbacks = false;
        //TODO: This needs to happen n times where n is the number of callback threads
        sem_callbacks.post();
    }
}
