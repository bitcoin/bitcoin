// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqnotificationinterface.h>

#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <util/system.h>
#include <validationinterface.h>
#include <zmq/zmqabstractnotifier.h>
#include <zmq/zmqpublishnotifier.h>
#include <zmq/zmqutil.h>

#include <zmq.h>

#include <cassert>
#include <map>
#include <string>
#include <utility>
#include <vector>

CZMQNotificationInterface::CZMQNotificationInterface()
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
    factories["pubsequence"] = CZMQAbstractNotifier::Create<CZMQPublishSequenceNotifier>;
    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
    if(!fNEVMSub.empty()) {
        std::string pubCmd = "pubnevmblockinfo";
        CZMQNotifierFactory factory0 = CZMQAbstractNotifier::Create<CZMQPublishNEVMBlockInfoNotifier>;
        std::string arg("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier0 = factory0();
        notifier0->SetAddressSub(fNEVMSub);
        notifier0->SetType(pubCmd);
        notifier0->SetAddress(fNEVMSub);
        notifier0->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetIntArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier0));

        pubCmd = "pubnevmblock";
        CZMQNotifierFactory factory = CZMQAbstractNotifier::Create<CZMQPublishNEVMBlockNotifier>;
        arg = std::string("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier = factory();
        notifier->SetAddressSub(fNEVMSub);
        notifier->SetType(pubCmd);
        notifier->SetAddress(fNEVMSub);
        notifier->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetIntArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier));

        pubCmd = "pubnevmconnect";
        CZMQNotifierFactory factory1 = CZMQAbstractNotifier::Create<CZMQPublishNEVMBlockConnectNotifier>;
        arg = std::string("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier1 = factory1();
        notifier1->SetAddressSub(fNEVMSub);
        notifier1->SetType(pubCmd);
        notifier1->SetAddress(fNEVMSub);
        notifier1->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetIntArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier1));

        pubCmd = "pubnevmdisconnect";
        CZMQNotifierFactory factory2 = CZMQAbstractNotifier::Create<CZMQPublishNEVMBlockDisconnectNotifier>;
        arg = std::string("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier2 = factory2();
        notifier2->SetAddressSub(fNEVMSub);
        notifier2->SetType(pubCmd);
        notifier2->SetAddress(fNEVMSub);
        notifier2->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetIntArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
        notifiers.push_back(std::move(notifier2));

        pubCmd = "pubnevmcomms";
        CZMQNotifierFactory factory3 = CZMQAbstractNotifier::Create<CZMQPublishNEVMCommsNotifier>;
        arg = std::string("-zmq" + pubCmd);
        std::unique_ptr<CZMQAbstractNotifier> notifier3 = factory3();
        notifier3->SetAddressSub(fNEVMSub);
        notifier3->SetType(pubCmd);
        notifier3->SetAddress(fNEVMSub);
        notifier3->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetIntArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
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
            notifier->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetIntArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
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
    LogPrint(BCLog::ZMQ, "version %d.%d.%d\n", major, minor, patch);

    LogPrint(BCLog::ZMQ, "Initialize notification interface\n");
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
            LogPrint(BCLog::ZMQ, "Notifier %s ready (address = %s, subscriber address = %s)\n", notifier->GetType(), notifier->GetAddress(), notifier->GetAddressSub());
        } else {
            LogPrint(BCLog::ZMQ, "Notifier %s failed (address = %s, subscriber address = %s)\n", notifier->GetType(), notifier->GetAddress(), notifier->GetAddressSub());
            return false;
        }
    }

    return true;
}

// Called during shutdown sequence
void CZMQNotificationInterface::Shutdown()
{
    LogPrint(BCLog::ZMQ, "Shutdown notification interface\n");
    if (pcontext)
    {
        for (auto& notifier : notifiers) {
            LogPrint(BCLog::ZMQ, "Shutdown notifier %s at %s, subscriber: %s\n", notifier->GetType(), notifier->GetAddress(), notifier->GetAddressSub());
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
// SYSCOIN
template <typename Function>
void TryForEach(std::list<std::unique_ptr<CZMQAbstractNotifier>>& notifiers, const Function& func)
{
    for (auto i = notifiers.begin(); i != notifiers.end(); ) {
        CZMQAbstractNotifier* notifier = i->get();
        if (func(notifier)) {
            ++i;
        } else {
            break;
        }
    }
}
} // anonymous namespace
// SYSCOIN
void CZMQNotificationInterface::NotifyNEVMComms(const std::string& commMessage, bool &bResponse)
{
    TryForEach(notifiers, [&commMessage, &bResponse](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyNEVMComms(commMessage, bResponse);
    });
}
void CZMQNotificationInterface::NotifyNEVMBlockConnect(const CNEVMHeader &evmBlock, const CBlock& block, std::string &state, const uint256& nBlockHash, NEVMDataVec &NEVMDataVecOut, const uint32_t& nHeight, bool bSkipValidation)
{
    TryForEach(notifiers, [&evmBlock, &block, &nBlockHash, &state, &NEVMDataVecOut, &nHeight, &bSkipValidation](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyNEVMBlockConnect(evmBlock, block, state, nBlockHash, NEVMDataVecOut, nHeight, bSkipValidation);
    });
}
void CZMQNotificationInterface::NotifyNEVMBlockDisconnect(std::string &state, const uint256& nBlockHash)
{
    TryForEach(notifiers, [&nBlockHash, &state](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyNEVMBlockDisconnect(state, nBlockHash);
    });
}
void CZMQNotificationInterface::NotifyGetNEVMBlockInfo(uint64_t &nHeight, std::string &state)
{
    TryForEach(notifiers, [&nHeight, &state](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGetNEVMBlockInfo(nHeight, state);
    });
}

void CZMQNotificationInterface::NotifyGetNEVMBlock(CNEVMBlock &evmBlock, std::string &state)
{
    TryForEach(notifiers, [&evmBlock, &state](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGetNEVMBlock(evmBlock, state);
    });
}
void CZMQNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, ChainstateManager& chainman, bool fInitialDownload)
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
void CZMQNotificationInterface::NotifyGovernanceVote(const uint256 &vote)
{
    TryForEachAndRemoveFailed(notifiers, [&vote](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGovernanceVote(vote);
    });
}

void CZMQNotificationInterface::NotifyGovernanceObject(const uint256 &object)
{
    TryForEachAndRemoveFailed(notifiers, [&object](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyGovernanceObject(object);
    });
}
CZMQNotificationInterface* g_zmq_notification_interface = nullptr;
