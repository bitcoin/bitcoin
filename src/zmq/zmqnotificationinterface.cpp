// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqnotificationinterface.h>

#include <common/args.h>
#include <kernel/chain.h>
#include <kernel/mempool_entry.h>
#include <logging.h>
#include <netbase.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
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

std::unique_ptr<CZMQNotificationInterface> CZMQNotificationInterface::Create(std::function<bool(std::vector<uint8_t>&, const CBlockIndex&)> get_block_by_index)
{
    std::map<std::string, CZMQNotifierFactory> factories;
    factories["pubhashblock"] = CZMQAbstractNotifier::Create<CZMQPublishHashBlockNotifier>;
    factories["pubhashtx"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionNotifier>;
    factories["pubrawblock"] = [&get_block_by_index]() -> std::unique_ptr<CZMQAbstractNotifier> {
        return std::make_unique<CZMQPublishRawBlockNotifier>(get_block_by_index);
    };
    factories["pubrawtx"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionNotifier>;
    factories["pubsequence"] = CZMQAbstractNotifier::Create<CZMQPublishSequenceNotifier>;

    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
    for (const auto& entry : factories)
    {
        std::string arg("-zmq" + entry.first);
        const auto& factory = entry.second;
        for (std::string& address : gArgs.GetArgs(arg)) {
            // libzmq uses prefix "ipc://" for UNIX domain sockets
            if (address.substr(0, ADDR_PREFIX_UNIX.length()) == ADDR_PREFIX_UNIX) {
                address.replace(0, ADDR_PREFIX_UNIX.length(), ADDR_PREFIX_IPC);
            }

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
            return notificationInterface;
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
    assert(!pcontext);

    pcontext = zmq_ctx_new();

    if (!pcontext)
    {
        zmqError("Unable to initialize context");
        return false;
    }

    for (auto& notifier : notifiers) {
        if (notifier->Initialize(pcontext)) {
            LogPrint(BCLog::ZMQ, "Notifier %s ready (address = %s)\n", notifier->GetType(), notifier->GetAddress());
        } else {
            LogPrint(BCLog::ZMQ, "Notifier %s failed (address = %s)\n", notifier->GetType(), notifier->GetAddress());
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
            LogPrint(BCLog::ZMQ, "Shutdown notifier %s at %s\n", notifier->GetType(), notifier->GetAddress());
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

} // anonymous namespace

void CZMQNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    TryForEachAndRemoveFailed(notifiers, [pindexNew](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlock(pindexNew);
    });
}

void CZMQNotificationInterface::TransactionAddedToMempool(const NewMempoolTransactionInfo& ptx, uint64_t mempool_sequence)
{
    const CTransaction& tx = *(ptx.info.m_tx);

    TryForEachAndRemoveFailed(notifiers, [&tx, mempool_sequence](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyTransaction(tx) && notifier->NotifyTransactionAcceptance(tx, mempool_sequence);
    });
}

void CZMQNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx, const MemPoolRemovalReason& reason, uint64_t mempool_sequence)
{
    // Called for all non-block inclusion reasons
    const CTransaction& tx = *ptx;

    TryForEachAndRemoveFailed(notifiers, [&tx, mempool_sequence](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyTransactionRemoval(tx, mempool_sequence);
    });
}

void CZMQNotificationInterface::BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected)
{
    if (role == ChainstateRole::BACKGROUND) {
        return;
    }
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

std::unique_ptr<CZMQNotificationInterface> g_zmq_notification_interface;
