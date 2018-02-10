// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqpublishnotifier.h>

#include <version.h>
#include <validation.h>
#include <streams.h>
#include <util.h>
#include <netbase.h>

void zmqError(const char *str)
{
    LogPrint(BCLog::ZMQ, "zmq: Error: %s, errno=%s\n", str, zmq_strerror(errno));
}

bool CZMQNotificationInterface::IsWhitelistedRange(const CNetAddr &addr) {
    for (const CSubNet& subnet : vWhitelistedRange) {
        if (subnet.Match(addr))
            return true;
    }
    return false;
}

void CZMQNotificationInterface::ThreadZAP()
{
    // https://rfc.zeromq.org/spec:27/ZAP/
    assert(pcontext);
    void *sock = zmq_socket(pcontext, ZMQ_REP);
    zmq_bind(sock, "inproc://zeromq.zap.01");
    zapActive = true;

    uint8_t buf[10][1024];
    size_t nb[10];
    while (zapActive)
    {
        zmq_pollitem_t poll_items[] = { sock, 0, ZMQ_POLLIN, 0 };
        int rc = zmq_poll(poll_items, 1, 500);
        if (!(rc > 0 && poll_items[0].revents & ZMQ_POLLIN))
            continue;

        size_t nParts = 0;
        int more;
        size_t size = sizeof(int);
        do {
            size_t b = nParts <= 9 ? nParts : 9; // read any extra messages into last chunk
            nb[b] = zmq_recv(sock, buf[b], sizeof(buf[b]), 0);
            zmq_getsockopt(sock, ZMQ_RCVMORE, &more, &size);
            nParts++;
        } while (more);

        if (nParts < 5) // too few parts to be valid
            continue;

        if (nb[0] != 3 || memcmp(buf[0], "1.0", 3) != 0)
            continue;

        std::string address((char*)buf[3], nb[3]);

        bool fAccept = true;
        if (vWhitelistedRange.size() > 0)
        {
            CNetAddr addr;
            if (!LookupHost(address.c_str(), addr, false))
                fAccept = false;
            else
                fAccept = IsWhitelistedRange(addr);
        };

        LogPrint(BCLog::ZMQ, "zmq: Connection request from %s %s.\n", address, fAccept ? "accepted" : "denied");

        zmq_send(sock, buf[0], nb[0], ZMQ_SNDMORE);                 // version "1.0"
        zmq_send(sock, buf[1], nb[1], ZMQ_SNDMORE);                 // request id
        zmq_send(sock, fAccept ? "200" : "400", 3, ZMQ_SNDMORE);    // status code
        zmq_send(sock, NULL, 0, ZMQ_SNDMORE);                       // status text
        zmq_send(sock, NULL, 0, ZMQ_SNDMORE);                       // user id
        zmq_send(sock, NULL, 0, 0);                                 // metadata
    };

    zmq_close(sock);
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
    factories["pubhashtx"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionNotifier>;
    factories["pubrawblock"] = CZMQAbstractNotifier::Create<CZMQPublishRawBlockNotifier>;
    factories["pubrawtx"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionNotifier>;

    factories["pubhashwtx"] = CZMQAbstractNotifier::Create<CZMQPublishHashWalletTransactionNotifier>;
    factories["pubsmsg"] = CZMQAbstractNotifier::Create<CZMQPublishSMSGNotifier>;

    for (const auto& entry : factories)
    {
        std::string arg("-zmq" + entry.first);
        if (gArgs.IsArgSet(arg))
        {
            CZMQNotifierFactory factory = entry.second;
            std::string address = gArgs.GetArg(arg, "");
            CZMQAbstractNotifier *notifier = factory();
            notifier->SetType(entry.first);
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


    for (const auto& net : gArgs.GetArgs("-whitelistzmq")) {
        CSubNet subnet;
        LookupSubNet(net.c_str(), subnet);
        if (!subnet.IsValid())
            LogPrintf("Invalid netmask specified in -whitelistzmq: '%s'\n", net);
        else
            vWhitelistedRange.push_back(subnet);
    }

    if (vWhitelistedRange.size() > 0)
    {
        zapActive = false;
        threadZAP = std::thread(&TraceThread<std::function<void()> >, "zap", std::function<void()>(std::bind(&CZMQNotificationInterface::ThreadZAP, this)));

        for (size_t nTries = 1000; nTries > 0; nTries--)
        {
            if (zapActive)
                break;
            MilliSleep(100);
        };
        if (!zapActive)
        {
            zmqError("Unable to start zap thread");
            return false;
        };
    };

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

    if (threadZAP.joinable())
    {
        zapActive = false;
        threadZAP.join();
    };

    if (pcontext)
    {
        for (std::list<CZMQAbstractNotifier*>::iterator i=notifiers.begin(); i!=notifiers.end(); ++i)
        {
            CZMQAbstractNotifier *notifier = *i;
            LogPrint(BCLog::ZMQ, "   Shutdown notifier %s at %s\n", notifier->GetType(), notifier->GetAddress());
            notifier->Shutdown();
        }
        zmq_ctx_destroy(pcontext);

        pcontext = nullptr;
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

void CZMQNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx)
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
        TransactionAddedToMempool(ptx);
    }
}

void CZMQNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock)
{
    for (const CTransactionRef& ptx : pblock->vtx) {
        // Do a normal notify for each transaction removed in block disconnection
        TransactionAddedToMempool(ptx);
    }
}

void CZMQNotificationInterface::TransactionAddedToWallet(const std::string &sWalletName, const CTransactionRef& ptx)
{
    const CTransaction& tx = *ptx;
    for (auto i = notifiers.begin(); i!=notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifyTransaction(sWalletName, tx))
        {
            i++;
        } else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
};

void CZMQNotificationInterface::NewSecureMessage(const smsg::SecureMessage *psmsg, const uint160 &hash)
{
    for (std::list<CZMQAbstractNotifier*>::iterator i = notifiers.begin(); i!=notifiers.end(); )
    {
        CZMQAbstractNotifier *notifier = *i;
        if (notifier->NotifySecureMessage(psmsg, hash))
        {
            i++;
        }
        else
        {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
};
