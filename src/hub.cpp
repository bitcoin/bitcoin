#include "hub.h"
#include "main.h"

CHub* phub;

class CHubCallbackCommitBlock : public CHubCallback
{
private:
    CBlock block;
public:
    CHubCallbackCommitBlock(const CBlock &blockIn) : block(blockIn) {}
    void Signal(CHubSignalTable& sigtable) { LOCK(sigtable.cs_sigCommitBlock); printf("CHubCallbackCommitBlock: New Block Committed: %s\n", block.GetHash().ToString().substr(0,20).c_str()); sigtable.sigCommitBlock(block); }
};

class CHubCallbackCommitAlert : public CHubCallback
{
private:
    CAlert alert;
public:
    CHubCallbackCommitAlert(const CAlert &alertIn) : alert(alertIn) {}
    void Signal(CHubSignalTable& sigtable) { LOCK(sigtable.cs_sigCommitAlert); sigtable.sigCommitAlert(alert); }
};

class CHubCallbackRemoveAlert : public CHubCallback
{
private:
    CAlert alert;
public:
    CHubCallbackRemoveAlert(const CAlert &alertIn) : alert(alertIn) {}
    void Signal(CHubSignalTable& sigtable) { LOCK(sigtable.cs_sigRemoveAlert); sigtable.sigRemoveAlert(alert); }
};

class CHubCallbackAskForBlocks : public CHubCallback
{
private:
    uint256 hashEnd, hashOrig;
public:
    CHubCallbackAskForBlocks(uint256 hashEndIn, uint256 hashOrigIn) : hashEnd(hashEndIn), hashOrig(hashOrigIn) {}
    void Signal(CHubSignalTable& sigtable) { LOCK(sigtable.cs_sigAskForBlocks); sigtable.sigAskForBlocks(hashEnd, hashOrig); }
};

class CHubCallbackCommitTransactionToMemoryPool : public CHubCallback
{
private:
    CTransaction tx;
public:
    CHubCallbackCommitTransactionToMemoryPool(const CTransaction &txIn) : tx(txIn) {}
    void Signal(CHubSignalTable& sigtable) { LOCK(sigtable.cs_sigCommitTransactionToMemoryPool); sigtable.sigCommitTransactionToMemoryPool(tx); }
};

class CHubCallbackDoS : public CHubCallback
{
private:
    CNode* pNode;
    int nDoS;
public:
    CHubCallbackDoS(CNode* pNodeIn, const int nDoSIn) { pNode = pNodeIn; nDoS = nDoSIn; if(pNode) pNode->AddRef(); }
    void Signal(CHubSignalTable& sigtable) { if (pNode) pNode->Misbehaving(nDoS); }
    ~CHubCallbackDoS() { if(pNode) pNode->Release(); }
};

void CHub::SubmitCallbackCommitBlock(const CBlock &block)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CHubCallbackCommitBlock(block));
    sem_callbacks.post();
}

void CHub::SubmitCallbackCommitAlert(const CAlert &alert)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CHubCallbackCommitAlert(alert));
    sem_callbacks.post();
}

void CHub::SubmitCallbackRemoveAlert(const CAlert &alert)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CHubCallbackRemoveAlert(alert));
    sem_callbacks.post();
}

void CHub::AskForBlocks(const uint256 hashEnd, const uint256 hashOriginator)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CHubCallbackAskForBlocks(hashEnd, hashOriginator));
    sem_callbacks.post();
}

void CHub::SubmitCallbackCommitTransactionToMemoryPool(const CTransaction &tx)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CHubCallbackCommitTransactionToMemoryPool(tx));
    sem_callbacks.post();
}

bool CHub::ConnectToBlockStore(CBlockStore* pblockstoreIn)
{
    if (pblockstore)
        return false;
    pblockstore = pblockstoreIn;

    pblockstore->RegisterCommitBlock(boost::bind(&CHub::SubmitCallbackCommitBlock, this, _1));

    pblockstore->RegisterAskForBlocks(boost::bind(&CHub::AskForBlocks, this, _1, _2));

    pblockstore->RegisterDoSHandler(boost::bind(&CHub::SubmitCallbackDoS, this, _1, _2));

    return true;
}

void CHub::SubmitCallbackDoS(CNode* pNode, const int nDoS)
{
    LOCK(cs_callbacks);
        queueCallbacks.push(new CHubCallbackDoS(pNode, nDoS));
    sem_callbacks.post();
}

void CHub::ProcessCallbacks()
{
    {
        LOCK(cs_callbacks);
        if (fProcessCallbacks)
            nCallbackThreads++;
        else
            return;
    }

    loop
    {
        CHubCallback *pcallback = NULL;
        sem_callbacks.wait();
        if (fProcessCallbacks)
        {
            LOCK(cs_callbacks);
            assert(queueCallbacks.size() > 0);
            pcallback = queueCallbacks.front();
            queueCallbacks.pop();
        }
        else
        {
            LOCK(cs_callbacks);
            nCallbackThreads--;
            return;
        }

        pcallback->Signal(sigtable);
        delete pcallback;
    }
}

void CHub::StopProcessCallbacks()
{
    {
        LOCK(cs_callbacks);
        fProcessCallbacks = false;
        for (int i = 0; i < nCallbackThreads; i++)
            sem_callbacks.post();
    }
    while (nCallbackThreads > 0)
        Sleep(20);
}

void ProcessCallbacks(void* parg)
{
    ((CHub*)parg)->ProcessCallbacks();
}

CHub::CHub() : sem_callbacks(0), fProcessCallbacks(true), nCallbackThreads(0), pblockstore(NULL)
{
    for (int i = 0; i < GetArg("-callbackconcurrency", 1); i++)
        if (!CreateThread(::ProcessCallbacks, this))
            throw std::runtime_error("Couldn't create callback threads");
}



void CHubListener::RegisterWithHub(CHub* phub)
{
    phub->RegisterCommitBlock(boost::bind(&CHubListener::HandleCommitBlock, this, _1));

    phub->RegisterCommitTransactionToMemoryPool(boost::bind(&CHubListener::HandleCommitTransactionToMemoryPool, this, _1));

    phub->RegisterCommitAlert(boost::bind(&CHubListener::HandleCommitAlert, this, _1));
    phub->RegisterRemoveAlert(boost::bind(&CHubListener::HandleRemoveAlert, this, _1));

    phub->RegisterAskForBlocks(boost::bind(&CHubListener::HandleAskForBlocks, this, _1, _2));
}

void CHubListener::DeregisterFromHub()
{
    // TODO: Allow deregistration from CHub callbacks
}
