// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqpublishnotifier.h>
#include <zmq/zmqutil.h>

#include <zmq.h>

#include <validation.h>
#include <util/system.h>
// SYSCOIN
#include <governance/governancevote.h>
#include <governance/governanceobject.h>
// SYSCOIN
CZMQNotificationInterface::CZMQNotificationInterface() : pcontext(nullptr), pcontextsub(nullptr)
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
    factories["pubhashtx"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionNotifier>;
    factories["pubrawblock"] = CZMQAbstractNotifier::Create<CZMQPublishRawBlockNotifier>;
    factories["pubrawtx"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionNotifier>;
    // SYSCOIN
    factories["pubrawmempooltx"] = CZMQAbstractNotifier::Create<CZMQPublishRawMempoolTransactionNotifier>;
    factories["pubhashgovernancevote"] = CZMQAbstractNotifier::Create<CZMQPublishHashGovernanceVoteNotifier>;
    factories["pubhashgovernanceobject"] = CZMQAbstractNotifier::Create<CZMQPublishHashGovernanceObjectNotifier>;
    factories["pubrawgovernancevote"] = CZMQAbstractNotifier::Create<CZMQPublishRawGovernanceVoteNotifier>;
    factories["pubrawgovernanceobject"] = CZMQAbstractNotifier::Create<CZMQPublishRawGovernanceObjectNotifier>;
    factories["pubsequence"] = CZMQAbstractNotifier::Create<CZMQPublishSequenceNotifier>;
    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;

    const std::string &strPub = gArgs.GetArg("-zmqpubnevm", "");
    if(!strPub.empty()) {
        std::string pubCmd = "pubnevmblock";
        CZMQNotifierFactory factory = CZMQAbstractNotifier::Create<CZMQPublishNEVMBlockNotifier>;
        std::string arg("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier = factory();
        notifier->SetAddressSub(strPub);
        notifier->SetType(pubCmd);
        notifier->SetAddress(strPub);
        notifier->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier));

        pubCmd = "pubnevmconnect";
        CZMQNotifierFactory factory1 = CZMQAbstractNotifier::Create<CZMQPublishNEVMBlockConnectNotifier>;
        arg = std::string("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier1 = factory1();
        notifier1->SetAddressSub(strPub);
        notifier1->SetType(pubCmd);
        notifier1->SetAddress(strPub);
        notifier1->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier1));

        pubCmd = "pubnevmdisconnect";
        CZMQNotifierFactory factory2 = CZMQAbstractNotifier::Create<CZMQPublishNEVMBlockDisconnectNotifier>;
        arg = std::string("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier2 = factory2();
        notifier2->SetAddressSub(strPub);
        notifier2->SetType(pubCmd);
        notifier2->SetAddress(strPub);
        notifier2->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier2));

        pubCmd = "pubnevmcomms";
        CZMQNotifierFactory factory3 = CZMQAbstractNotifier::Create<CZMQPublishNEVMCommsNotifier>;
        arg = std::string("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier3 = factory3();
        notifier3->SetAddressSub(strPub);
        notifier3->SetType(pubCmd);
        notifier3->SetAddress(strPub);
        notifier3->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier3));
    }
    
    
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
    // SYSCOIN
    assert(!pcontext && !pcontextsub);

    pcontext = zmq_ctx_new();
    pcontextsub = zmq_ctx_new();

    if (!pcontext || !pcontextsub)
    {
        zmqError("Unable to initialize context");
        return false;
    }

    for (auto& notifier : notifiers) {
        if (notifier->Initialize(pcontext, pcontextsub)) {
            LogPrint(BCLog::ZMQ, "zmq: Notifier %s ready (address = %s, subscriber address = %s)\n", notifier->GetType(), notifier->GetAddress(), notifier->GetAddressSub());
        } else {
            LogPrint(BCLog::ZMQ, "zmq: Notifier %s failed (address = %s, subscriber address = %s)\n", notifier->GetType(), notifier->GetAddress(), notifier->GetAddressSub());
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
            LogPrint(BCLog::ZMQ, "zmq: Shutdown notifier %s at %s, subscriber: %s\n", notifier->GetType(), notifier->GetAddress(), notifier->GetAddressSub());
            notifier->Shutdown();
        }
        zmq_ctx_term(pcontext);

        pcontext = nullptr;
    }
    // SYSCOIN
    LogPrint(BCLog::ZMQ, "zmq: Shutdown subscription interface\n");
    if (pcontextsub)
    {
        zmq_ctx_term(pcontextsub);
        pcontextsub = nullptr;
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
} // anonymous namespace
// SYSCOIN
void CZMQNotificationInterface::NotifyNEVMComms(bool bConnect)
{
    TryForEachAndRemoveFailed(notifiers, [&bConnect](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyNEVMComms(bConnect);
    });
}
void CZMQNotificationInterface::NotifyNEVMBlockConnect(const CNEVMBlock &evmBlock, BlockValidationState &state, const uint256& nBlockHash)
{
    TryForEachAndRemoveFailed(notifiers, [&evmBlock, &nBlockHash, &state](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyNEVMBlockConnect(evmBlock, state, nBlockHash);
    });
}
void CZMQNotificationInterface::NotifyNEVMBlockDisconnect(const CNEVMBlock &evmBlock, BlockValidationState &state, const uint256& nBlockHash)
{
    TryForEachAndRemoveFailed(notifiers, [&evmBlock, &nBlockHash, &state](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyNEVMBlockDisconnect(evmBlock, state, nBlockHash);
    });
}
void CZMQNotificationInterface::NotifyGetNEVMBlock(CNEVMBlock &evmBlock, BlockValidationState &state)
{
    TryForEachAndRemoveFailed(notifiers, [&evmBlock, &state](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGetNEVMBlock(evmBlock, state);
    });
}
void CZMQNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;
    TryForEachAndRemoveFailed(notifiers, [pindexNew](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlock(pindexNew);
    });
}

void CZMQNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, uint64_t mempool_sequence)
{
    const CTransaction& tx = *ptx;

    TryForEachAndRemoveFailed(notifiers, [&tx, mempool_sequence](CZMQAbstractNotifier* notifier) {
        // SYSCOIN
        return notifier->NotifyTransaction(tx) && notifier->NotifyTransactionAcceptance(tx, mempool_sequence) && (mempool_sequence > 0 || notifier->NotifyTransactionMempool(tx));
    });
}

void CZMQNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason, uint64_t mempool_sequence)
{
    // Called for all non-block inclusion reasons
    const CTransaction& tx = *ptx;

    TryForEachAndRemoveFailed(notifiers, [&tx, mempool_sequence](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyTransactionRemoval(tx, mempool_sequence);
    });
}

void CZMQNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected)
{
    for (const CTransactionRef& ptx : pblock->vtx) {
        const CTransaction& tx = *ptx;
        TryForEachAndRemoveFailed(notifiers, [&tx](CZMQAbstractNotifier* notifier) {
            return notifier->NotifyTransaction(tx);
        });
    }

    // Next we notify BlockConnect listeners for *all* blocks
    TryForEachAndRemoveFailed(notifiers, [pindexConnected](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlockConnect(pindexConnected);
    });
}

void CZMQNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    for (const CTransactionRef& ptx : pblock->vtx) {
        const CTransaction& tx = *ptx;
        TryForEachAndRemoveFailed(notifiers, [&tx](CZMQAbstractNotifier* notifier) {
            return notifier->NotifyTransaction(tx);
        });
    }

    // Next we notify BlockDisconnect listeners for *all* blocks
    TryForEachAndRemoveFailed(notifiers, [pindexDisconnected](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlockDisconnect(pindexDisconnected);
    });
}
// SYSCOIN
void CZMQNotificationInterface::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote> &vote)
{
    TryForEachAndRemoveFailed(notifiers, [&vote](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGovernanceVote(vote);
    });
}

void CZMQNotificationInterface::NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject> &object)
{
    TryForEachAndRemoveFailed(notifiers, [&object](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGovernanceObject(object);
    });
}
CZMQNotificationInterface* g_zmq_notification_interface = nullptr;
