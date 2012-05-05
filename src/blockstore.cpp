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
}

void CBlockStore::Relayed(const uint256 hash)
{
    LOCK(cs_callbacks);
    queueCallbacks.push(new CBlockStoreCallbackRelayed(hash));
}

void CBlockStore::TransactionReplaced(const uint256 hash)
{
    LOCK(cs_callbacks);
    queueCallbacks.push(new CBlockStoreCallbackTransactionReplaced(hash));
}

void CBlockStore::SubmitCallbackCommitTransactionToMemoryPool(const CTransaction &tx)
{
    LOCK(cs_callbacks);
    queueCallbacks.push(new CBlockStoreCallbackCommitTransactionToMemoryPool(tx));
}

void CBlockStore::SubmitCallbackCommitBlock(const CBlock &block)
{
    LOCK(cs_callbacks);
    queueCallbacks.push(new CBlockStoreCallbackCommitBlock(block));
}

void ProcessCallbacks(void* parg)
{
    ((CBlockStore*)parg)->ProcessCallbacks();
}

void CBlockStore::ProcessCallbacks()
{
    loop
    {
        CBlockStoreCallback *pcallback = NULL;
        {
            LOCK(cs_callbacks);
            if (!queueCallbacks.empty())
            {
                pcallback = queueCallbacks.front();
                queueCallbacks.pop();
            }
        }

        if (pcallback)
        {
            pcallback->Signal(sigtable);
            delete pcallback;
        }
        else
            Sleep(100);

        if (fShutdown)
            return;
    }
}
