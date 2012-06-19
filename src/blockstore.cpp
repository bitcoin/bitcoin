#include "blockstore.h"
#include "util.h"
#include "net.h"
#include "main.h"

void CBlockStore::SubmitCallbackFinishEmitBlock(CBlock& block, CNode* pNodeDoS)
{
    unsigned int nQueueSize;
    {
        LOCK(cs_callbacks);
        nQueueSize = queueFinishEmitBlockCallbacks.size();
    }
    while (nQueueSize >= GetArg("-blockbuffersize", 20) && fProcessCallbacks)
    {
        Sleep(20);
        LOCK(cs_callbacks);
        nQueueSize = queueFinishEmitBlockCallbacks.size();
    }

    if (pNodeDoS) pNodeDoS->AddRef();

    LOCK(cs_callbacks);
    queueFinishEmitBlockCallbacks.push(std::make_pair(new CBlock(block), pNodeDoS));
    sem_callbacks.post();
}

void CBlockStore::StopProcessCallbacks()
{
    {
        LOCK(cs_callbacks);
        fProcessCallbacks = false;
        sem_callbacks.post();
    }
    while (fProcessingCallbacks)
        Sleep(20);
}

void CBlockStore::ProcessCallbacks()
{
    {
        LOCK(cs_callbacks);
        if (!fProcessCallbacks)
            return;
        fProcessingCallbacks = true;
    }

    loop
    {
        std::pair<CBlock*, CNode*> callback;
        sem_callbacks.wait();
        if (fProcessCallbacks)
        {
            LOCK(cs_callbacks);
            assert(queueFinishEmitBlockCallbacks.size() > 0);
            callback = queueFinishEmitBlockCallbacks.front();
            queueFinishEmitBlockCallbacks.pop();
        }
        else
        {
            fProcessingCallbacks = false;
            return;
        }

        FinishEmitBlock(*(callback.first), callback.second);
        delete callback.first;
        if (callback.second) callback.second->Release();
    }
}

void CBlockStoreProcessCallbacks(void* parg)
{
    ((CBlockStore*)parg)->ProcessCallbacks();
}

CBlockStore::CBlockStore() : sem_callbacks(0), fProcessCallbacks(true), fProcessingCallbacks(false)
{
    if (!CreateThread(CBlockStoreProcessCallbacks, this))
        throw std::runtime_error("Couldn't create callback threads");
}
