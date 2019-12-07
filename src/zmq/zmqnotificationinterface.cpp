// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zmqnotificationinterface.h"
#include "zmqpublishnotifier.h"

#include "version.h"
#include "validation.h"
#include "streams.h"
#include "util.h"

void zmqError(const char *str)
{
    LogPrint(BCLog::ZMQ, "zmq: Error: %s, errno=%s\n", str, zmq_strerror(errno));
}

CZMQNotificationInterface::CZMQNotificationInterface() : pcontext(nullptr)
{
}

CZMQNotificationInterface::~CZMQNotificationInterface()
{
    Shutdown();

    for (std::list<CZMQAbstractNotifier*>::iterator i=notifiers.begin(); i!=notifiers.end(); ++i)
    {
        delete *i;
    }
}

CZMQNotificationInterface* CZMQNotificationInterface::Create()
{
    CZMQNotificationInterface* notificationInterface = nullptr;
    std::map<std::string, CZMQNotifierFactory> factories;
    std::list<CZMQAbstractNotifier*> notifiers;

    factories["pubhashblock"] = CZMQAbstractNotifier::Create<CZMQPublishHashBlockNotifier>;
    factories["pubhashchainlock"] = CZMQAbstractNotifier::Create<CZMQPublishHashChainLockNotifier>;
    factories["pubhashtx"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionNotifier>;
    factories["pubhashtxlock"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionLockNotifier>;
    factories["pubhashgovernancevote"] = CZMQAbstractNotifier::Create<CZMQPublishHashGovernanceVoteNotifier>;
    factories["pubhashgovernanceobject"] = CZMQAbstractNotifier::Create<CZMQPublishHashGovernanceObjectNotifier>;
    factories["pubhashinstantsenddoublespend"] = CZMQAbstractNotifier::Create<CZMQPublishHashInstantSendDoubleSpendNotifier>;
    factories["pubrawblock"] = CZMQAbstractNotifier::Create<CZMQPublishRawBlockNotifier>;
    factories["pubrawchainlock"] = CZMQAbstractNotifier::Create<CZMQPublishRawChainLockNotifier>;
    factories["pubrawchainlocksig"] = CZMQAbstractNotifier::Create<CZMQPublishRawChainLockSigNotifier>;
    factories["pubrawtx"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionNotifier>;
    factories["pubrawtxlock"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionLockNotifier>;
    factories["pubrawtxlocksig"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionLockSigNotifier>;
    factories["pubrawgovernancevote"] = CZMQAbstractNotifier::Create<CZMQPublishRawGovernanceVoteNotifier>;
    factories["pubrawgovernanceobject"] = CZMQAbstractNotifier::Create<CZMQPublishRawGovernanceObjectNotifier>;
    factories["pubrawinstantsenddoublespend"] = CZMQAbstractNotifier::Create<CZMQPublishRawInstantSendDoubleSpendNotifier>;

    for (std::map<std::string, CZMQNotifierFactory>::const_iterator i=factories.begin(); i!=factories.end(); ++i)
    {
        std::string arg("-zmq" + i->first);
        if (gArgs.IsArgSet(arg))
        {
            CZMQNotifierFactory factory = i->second;
            std::string address = gArgs.GetArg(arg, "");
            CZMQAbstractNotifier *notifier = factory();
            notifier->SetType(i->first);
            notifier->SetAddress(address);
            notifiers.push_back(notifier);
        }
    }

    if (!notifiers.empty())
    {
        notificationInterface = new CZMQNotificationInterface();
        notificationInterface->notifiers = notifiers;

        if (!notificationInterface->Initialize())
        {
            delete notificationInterface;
            notificationInterface = nullptr;
        }
    }

    return notificationInterface;
}

// Called at startup to conditionally set up ZMQ socket(s)
bool CZMQNotificationInterface::Initialize()
{
    LogPrint(BCLog::ZMQ, "zmq: Initialize notification interface\n");
    assert(!pcontext);

    pcontext = zmq_init(1);

    if (!pcontext)
    {
        zmqError("Unable to initialize context");
        return false;
    }

    std::list<CZMQAbstractNotifier*>::iterator i=notifiers.begin();
    for (; i!=notifiers.end(); ++i)
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->Initialize(pcontext))
        {
            LogPrint(BCLog::ZMQ, "  Notifier %s ready (address = %s)\n", notifier->GetType(), notifier->GetAddress());
        }
        else
        {
            LogPrint(BCLog::ZMQ, "  Notifier %s failed (address = %s)\n", notifier->GetType(), notifier->GetAddress());
            break;
        }
    }

    if (i!=notifiers.end())
    {
        return false;
    }

    return true;
}

// Called during shutdown sequence
void CZMQNotificationInterface::Shutdown()
{
    LogPrint(BCLog::ZMQ, "zmq: Shutdown notification interface\n");
    if (pcontext)
    {
        for (std::list<CZMQAbstractNotifier*>::iterator i=notifiers.begin(); i!=notifiers.end(); ++i)
        {
            CZMQAbstractNotifier *notifier = *i;
            LogPrint(BCLog::ZMQ, "   Shutdown notifier %s at %s\n", notifier->GetType(), notifier->GetAddress());
            notifier->Shutdown();
        }
        zmq_ctx_destroy(pcontext);

        pcontext = 0;
    }
}

void CZMQNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    for (std::list<CZMQAbstractNotifier*>::iterator i = notifiers.begin(); i!=notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifyBlock(pindexNew))
        {
            i++;
        }
        else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}

void CZMQNotificationInterface::NotifyChainLock(const CBlockIndex *pindex, const llmq::CChainLockSig& clsig)
{
    for (std::list<CZMQAbstractNotifier*>::iterator i = notifiers.begin(); i!=notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifyChainLock(pindex, clsig))
        {
            i++;
        }
        else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}

void CZMQNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime)
{
    // Used by BlockConnected and BlockDisconnected as well, because they're
    // all the same external callback.
    const CTransaction& tx = *ptx;

    for (std::list<CZMQAbstractNotifier*>::iterator i = notifiers.begin(); i!=notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifyTransaction(tx))
        {
            i++;
        }
        else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}

void CZMQNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected, const std::vector<CTransactionRef>& vtxConflicted)
{
    for (const CTransactionRef& ptx : pblock->vtx) {
        // Do a normal notify for each transaction added in the block
        TransactionAddedToMempool(ptx, 0);
    }
}

void CZMQNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    for (const CTransactionRef& ptx : pblock->vtx) {
        // Do a normal notify for each transaction removed in block disconnection
        TransactionAddedToMempool(ptx, 0);
    }
}

void CZMQNotificationInterface::NotifyTransactionLock(const CTransaction &tx, const llmq::CInstantSendLock& islock)
{
    for (std::list<CZMQAbstractNotifier*>::iterator i = notifiers.begin(); i!=notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifyTransactionLock(tx, islock))
        {
            i++;
        }
        else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}

void CZMQNotificationInterface::NotifyGovernanceVote(const CGovernanceVote &vote)
{
    for (std::list<CZMQAbstractNotifier*>::iterator i = notifiers.begin(); i != notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifyGovernanceVote(vote))
        {
            i++;
        }
        else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}

void CZMQNotificationInterface::NotifyGovernanceObject(const CGovernanceObject &object)
{
    for (std::list<CZMQAbstractNotifier*>::iterator i = notifiers.begin(); i != notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifyGovernanceObject(object))
        {
            i++;
        }
        else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}

void CZMQNotificationInterface::NotifyInstantSendDoubleSpendAttempt(const CTransaction &currentTx, const CTransaction &previousTx)
{
    for (auto it = notifiers.begin(); it != notifiers.end();) {
        CZMQAbstractNotifier *notifier = *it;
        if (notifier->NotifyInstantSendDoubleSpendAttempt(currentTx, previousTx)) {
            ++it;
        } else {
            notifier->Shutdown();
            it = notifiers.erase(it);
        }
    }
}
