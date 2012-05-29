#include "hub.h"
#include "main.h"

CHub* phub;

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

CHub::CHub() : sem_callbacks(0), fProcessCallbacks(true), nCallbackThreads(0)
{
    for (int i = 0; i < GetArg("-callbackconcurrency", 1); i++)
        if (!CreateThread(::ProcessCallbacks, this))
            throw std::runtime_error("Couldn't create callback threads");
}
