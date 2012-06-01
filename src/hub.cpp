#include "hub.h"
#include "main.h"

CHub* phub;

class CHubCallbackCommitBlock : public CHubCallback
{
private:
    CBlock block;
public:
    CHubCallbackCommitBlock(const CBlock &blockIn) : block(blockIn) {}
    void Signal(CHubSignalTable& sigtable) { LOCK(sigtable.cs_sigCommitBlock); sigtable.sigCommitBlock(block); }
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



void CHubListener::RegisterWithHub(CHub* phub)
{
    phub->RegisterCommitBlock(boost::bind(&CHubListener::HandleCommitBlock, this, _1));

    phub->RegisterCommitAlert(boost::bind(&CHubListener::HandleCommitAlert, this, _1));
    phub->RegisterRemoveAlert(boost::bind(&CHubListener::HandleRemoveAlert, this, _1));

    phub->RegisterAskForBlocks(boost::bind(&CHubListener::HandleAskForBlocks, this, _1, _2));
}

void CHubListener::DeregisterFromHub()
{
    // TODO: Allow deregistration from CHub callbacks
}
