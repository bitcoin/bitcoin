#ifndef BITCOIN_HUB_H
#define BITCOIN_HUB_H

// This API is considered stable ONLY for existing bitcoin codebases,
// any futher uses are not yet supported.
// This API is subject to change dramatically overnight, do not
// depend on it for anything.

#include <boost/signals2/signal.hpp>
#include <queue>

#include "sync.h"

class CBlock;

class CHubSignalTable
{
public:
};

class CHubCallback
{
public:
    virtual ~CHubCallback() {};
    virtual void Signal(CHubSignalTable& sigtable) =0;
};

class CHub
{
private:
    CHubSignalTable sigtable;

    CCriticalSection cs_callbacks;
    std::queue<CHubCallback*> queueCallbacks;
    CSemaphore sem_callbacks;

    bool fProcessCallbacks;
    int nCallbackThreads;

public:
//Util methods
    // Loops to process callbacks (do not call manually, automatically started in the constructor)
        void ProcessCallbacks();
    // Stop callback processing threads 
    void StopProcessCallbacks();

    CHub();
    ~CHub()  { StopProcessCallbacks(); }
};

extern CHub* phub;

#endif
