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
    WAITABLE_LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackAskForBlocks(hashEnd, hashOriginator));
    NOTIFY(condHaveCallbacks);
}

void CBlockStore::Relayed(const uint256 hash)
{
    WAITABLE_LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackRelayed(hash));
    NOTIFY(condHaveCallbacks);
}

void CBlockStore::TransactionReplaced(const uint256 hash)
{
    WAITABLE_LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackTransactionReplaced(hash));
    NOTIFY(condHaveCallbacks);
}

void CBlockStore::SubmitCallbackCommitTransactionToMemoryPool(const CTransaction &tx)
{
    WAITABLE_LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackCommitTransactionToMemoryPool(tx));
    NOTIFY(condHaveCallbacks);
}

void CBlockStore::SubmitCallbackCommitBlock(const CBlock &block)
{
    WAITABLE_LOCK(cs_callbacks);
        queueCallbacks.push(new CBlockStoreCallbackCommitBlock(block));
    NOTIFY(condHaveCallbacks);
}

void ProcessCallbacks(void* parg)
{
    ((CBlockStore*)parg)->ProcessCallbacks();
}

void CBlockStore::ProcessCallbacks()
{
    {
        WAITABLE_LOCK(cs_callbacks);
        fProcessCallbacks = true;
    }

    loop
    {
        CBlockStoreCallback *pcallback = NULL;
        {
            WAITABLE_LOCK(cs_callbacks);
            WAIT(condHaveCallbacks, !fProcessCallbacks || queueCallbacks.size()>0);
            if (fProcessCallbacks)
            {
                pcallback = queueCallbacks.front();
                queueCallbacks.pop();
            }
        }

        if (!fProcessCallbacks)
            return;

        pcallback->Signal(sigtable);
        delete pcallback;
    }
}

void CBlockStore::StopProcessCallbacks()
{
    {
        WAITABLE_LOCK(cs_callbacks);
        fProcessCallbacks = false;
        NOTIFY_ALL(condHaveCallbacks);
    }
}
