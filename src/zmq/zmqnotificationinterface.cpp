// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zmqnotificationinterface.h"
#include "zmqpublishnotifier.h"

#include "version.h"
#include "main.h"
#include "streams.h"
#include "util.h"

void zmqError(const char *str)
{
    LogPrint("zmq", "zmq: Error: %s, errno=%s\n", str, zmq_strerror(errno));
}

CZMQNotificationInterface::CZMQNotificationInterface() : pcontext(NULL)
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

/** Conditionally add notifier, if the right arguments are provided */
static void addNotifier(const std::map<std::string, std::string> &args, std::list<CZMQAbstractNotifier*> &notifiers, const std::string &name, CZMQAbstractNotifier* notifier)
{
    std::map<std::string, std::string>::const_iterator j = args.find("-zmq" + name);
    if (j!=args.end())
    {
        std::string address = j->second;
        notifier->SetType(name);
        notifier->SetAddress(address);
        notifiers.push_back(notifier);
    } else {
        delete notifier;
    }
}

CZMQNotificationInterface* CZMQNotificationInterface::CreateWithArguments(const std::map<std::string, std::string> &args, CTxMemPool *mempool)
{
    CZMQNotificationInterface* notificationInterface = NULL;
    std::list<CZMQAbstractNotifier*> notifiers;

    addNotifier(args, notifiers, "pubhashblock", new CZMQPublishHashBlockNotifier());
    addNotifier(args, notifiers, "pubhashtx", new CZMQPublishHashTransactionNotifier());
    addNotifier(args, notifiers, "pubrawblock", new CZMQPublishRawBlockNotifier());
    addNotifier(args, notifiers, "pubrawtx", new CZMQPublishRawTransactionNotifier());
    if (mempool)
        addNotifier(args, notifiers, "pubmempool", new CZMQPublishMempoolNotifier(mempool));

    if (!notifiers.empty())
    {
        notificationInterface = new CZMQNotificationInterface();
        notificationInterface->notifiers = notifiers;

        if (!notificationInterface->Initialize())
        {
            delete notificationInterface;
            notificationInterface = NULL;
        }
    }

    return notificationInterface;
}

// Called at startup to conditionally set up ZMQ socket(s)
bool CZMQNotificationInterface::Initialize()
{
    LogPrint("zmq", "zmq: Initialize notification interface\n");
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
            LogPrint("zmq", "  Notifier %s ready (address = %s)\n", notifier->GetType(), notifier->GetAddress());
        }
        else
        {
            LogPrint("zmq", "  Notifier %s failed (address = %s)\n", notifier->GetType(), notifier->GetAddress());
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
    LogPrint("zmq", "zmq: Shutdown notification interface\n");
    if (pcontext)
    {
        for (std::list<CZMQAbstractNotifier*>::iterator i=notifiers.begin(); i!=notifiers.end(); ++i)
        {
            CZMQAbstractNotifier *notifier = *i;
            LogPrint("zmq", "   Shutdown notifier %s at %s\n", notifier->GetType(), notifier->GetAddress());
            notifier->Shutdown();
        }
        zmq_ctx_destroy(pcontext);

        pcontext = 0;
    }
}
