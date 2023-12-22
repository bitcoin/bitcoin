// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqpublishnotifier.h>
#include <zmq/zmqutil.h>

#include <zmq.h>

#include <validation.h>
#include <util/system.h>

CZMQNotificationInterface::CZMQNotificationInterface() : pcontext(nullptr)
{
}

CZMQNotificationInterface::~CZMQNotificationInterface()
{
    Shutdown();
}

std::list<const CZMQAbstractNotifier*> CZMQNotificationInterface::GetActiveNotifiers() const
{
    std::list<const CZMQAbstractNotifier*> result;
    for (const auto& n : notifiers) {
        result.push_back(n.get());
    }
    return result;
}

CZMQNotificationInterface* CZMQNotificationInterface::Create()
{
    std::map<std::string, CZMQNotifierFactory> factories;
    factories["pubhashblock"] = CZMQAbstractNotifier::Create<CZMQPublishHashBlockNotifier>;
    factories["pubhashchainlock"] = CZMQAbstractNotifier::Create<CZMQPublishHashChainLockNotifier>;
    factories["pubhashtx"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionNotifier>;
    factories["pubhashtxlock"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionLockNotifier>;
    factories["pubhashgovernancevote"] = CZMQAbstractNotifier::Create<CZMQPublishHashGovernanceVoteNotifier>;
    factories["pubhashgovernanceobject"] = CZMQAbstractNotifier::Create<CZMQPublishHashGovernanceObjectNotifier>;
    factories["pubhashinstantsenddoublespend"] = CZMQAbstractNotifier::Create<CZMQPublishHashInstantSendDoubleSpendNotifier>;
    factories["pubhashrecoveredsig"] = CZMQAbstractNotifier::Create<CZMQPublishHashRecoveredSigNotifier>;
    factories["pubrawblock"] = CZMQAbstractNotifier::Create<CZMQPublishRawBlockNotifier>;
    factories["pubrawchainlock"] = CZMQAbstractNotifier::Create<CZMQPublishRawChainLockNotifier>;
    factories["pubrawchainlocksig"] = CZMQAbstractNotifier::Create<CZMQPublishRawChainLockSigNotifier>;
    factories["pubrawtx"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionNotifier>;
    factories["pubrawtxlock"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionLockNotifier>;
    factories["pubrawtxlocksig"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionLockSigNotifier>;
    factories["pubrawgovernancevote"] = CZMQAbstractNotifier::Create<CZMQPublishRawGovernanceVoteNotifier>;
    factories["pubrawgovernanceobject"] = CZMQAbstractNotifier::Create<CZMQPublishRawGovernanceObjectNotifier>;
    factories["pubrawinstantsenddoublespend"] = CZMQAbstractNotifier::Create<CZMQPublishRawInstantSendDoubleSpendNotifier>;
    factories["pubrawrecoveredsig"] = CZMQAbstractNotifier::Create<CZMQPublishRawRecoveredSigNotifier>;

    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
    for (const auto& entry : factories)
    {
        std::string arg("-zmq" + entry.first);
        const auto& factory = entry.second;
        for (const std::string& address : gArgs.GetArgs(arg)) {
            std::unique_ptr<CZMQAbstractNotifier> notifier = factory();
            notifier->SetType(entry.first);
            notifier->SetAddress(address);
            notifier->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
            notifiers.push_back(std::move(notifier));
        }
    }

    if (!notifiers.empty())
    {
        std::unique_ptr<CZMQNotificationInterface> notificationInterface(new CZMQNotificationInterface());
        notificationInterface->notifiers = std::move(notifiers);

        if (notificationInterface->Initialize()) {
            return notificationInterface.release();
        }
    }

    return nullptr;
}

// Called at startup to conditionally set up ZMQ socket(s)
bool CZMQNotificationInterface::Initialize()
{
    int major = 0, minor = 0, patch = 0;
    zmq_version(&major, &minor, &patch);
    LogPrint(BCLog::ZMQ, "zmq: version %d.%d.%d\n", major, minor, patch);

    LogPrint(BCLog::ZMQ, "zmq: Initialize notification interface\n");
    assert(!pcontext);

    pcontext = zmq_ctx_new();

    if (!pcontext)
    {
        zmqError("Unable to initialize context");
        return false;
    }

    for (auto& notifier : notifiers) {
        if (notifier->Initialize(pcontext)) {
            LogPrint(BCLog::ZMQ, "zmq: Notifier %s ready (address = %s)\n", notifier->GetType(), notifier->GetAddress());
        } else {
            LogPrint(BCLog::ZMQ, "zmq: Notifier %s failed (address = %s)\n", notifier->GetType(), notifier->GetAddress());
            return false;
        }
    }

    return true;
}

// Called during shutdown sequence
void CZMQNotificationInterface::Shutdown()
{
    LogPrint(BCLog::ZMQ, "zmq: Shutdown notification interface\n");
    if (pcontext)
    {
        for (auto& notifier : notifiers) {
            LogPrint(BCLog::ZMQ, "zmq: Shutdown notifier %s at %s\n", notifier->GetType(), notifier->GetAddress());
            notifier->Shutdown();
        }
        zmq_ctx_term(pcontext);

        pcontext = nullptr;
    }
}

namespace {
template <typename Function>
void TryForEachAndRemoveFailed(std::list<std::unique_ptr<CZMQAbstractNotifier>>& notifiers, const Function& func)
{
    for (auto i = notifiers.begin(); i != notifiers.end(); ) {
        CZMQAbstractNotifier* notifier = i->get();
        if (func(notifier)) {
            ++i;
        } else {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}
}

void CZMQNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    TryForEachAndRemoveFailed(notifiers, [pindexNew](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlock(pindexNew);
    });
}

void CZMQNotificationInterface::NotifyChainLock(const CBlockIndex *pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    TryForEachAndRemoveFailed(notifiers, [pindex, &clsig](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyChainLock(pindex, clsig);
    });
}

void CZMQNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime)
{
    // Used by BlockConnected and BlockDisconnected as well, because they're
    // all the same external callback.
    const CTransaction& tx = *ptx;

    TryForEachAndRemoveFailed(notifiers, [&tx](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyTransaction(tx);
    });
}

void CZMQNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected)
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

void CZMQNotificationInterface::NotifyTransactionLock(const CTransactionRef& tx, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    TryForEachAndRemoveFailed(notifiers, [&tx, &islock](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyTransactionLock(tx, islock);
    });
}

void CZMQNotificationInterface::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote> &vote)
{
    TryForEachAndRemoveFailed(notifiers, [&vote](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGovernanceVote(vote);
    });
}

void CZMQNotificationInterface::NotifyGovernanceObject(const std::shared_ptr<const Governance::Object> &object)
{
    TryForEachAndRemoveFailed(notifiers, [&object](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGovernanceObject(object);
    });
}

void CZMQNotificationInterface::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx)
{
    TryForEachAndRemoveFailed(notifiers, [&currentTx, &previousTx](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyInstantSendDoubleSpendAttempt(currentTx, previousTx);
    });
}

void CZMQNotificationInterface::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig)
{
    TryForEachAndRemoveFailed(notifiers, [&sig](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyRecoveredSig(sig);
    });
}

CZMQNotificationInterface* g_zmq_notification_interface = nullptr;
