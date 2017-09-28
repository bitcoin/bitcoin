// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "init.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "rpc/server.h"
#include "sync.h"
#include "util.h"
#include "utilstrencodings.h"
#include "validationinterface.h"

#include <univalue.h>

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>

#include <deque>
#include <exception>
#include <stdint.h>

static const char* MSG_HASHBLOCK = "hashblock";
static const char* MSG_HASHTX = "hashtx";

/* keep the max queue size large becase we don't
   auto-register for notification on startup */
static const size_t MAX_QUEUE_SIZE = 1024 * 1024;
static const int DEFAULT_POLL_TIMEOUT = 30;

enum class NotificationType {
    Unknown,
    Block,
    Tx
};

typedef std::pair<size_t, size_t> queueRange_t;
typedef std::string clientUUID_t;

// class that represents a notification
class NotificationEntry
{
public:
    NotificationType m_type;
    int32_t m_sequence_number;
    UniValue m_notification;
};

class NotificationQueue
{
public:
    std::deque<NotificationEntry> m_queue;
    std::map<NotificationType, int32_t> m_map_sequence_numbers;
    std::set<NotificationType> m_registered_notification_types;

    CCriticalSection m_cs_notification_queue;

    const std::string typeToString(NotificationType type) const
    {
        switch (type) {
        case NotificationType::Block:
            return MSG_HASHBLOCK;
            break;
        case NotificationType::Tx:
            return MSG_HASHTX;
            break;
        default:
            return "unknown";
        }
    }

    NotificationType stringToType(const std::string& strType) const
    {
        if (strType == MSG_HASHBLOCK)
            return NotificationType::Block;
        else if (strType == MSG_HASHTX)
            return NotificationType::Tx;
        else
            return NotificationType::Unknown;
    }

    // populates a json object with all notifications in the queue
    // returns a range to allow removing the elements from the queue
    // after successfull transmitting
    queueRange_t weakDequeueNotifications(UniValue& result)
    {
        size_t firstElement = 0;
        size_t elementCount = 0;

        LOCK(m_cs_notification_queue);
        for (const NotificationEntry& entry : m_queue) {
            UniValue obj = UniValue(UniValue::VOBJ);
            obj.pushKV("type", typeToString(entry.m_type));
            obj.pushKV("seq", entry.m_sequence_number);
            obj.pushKV("obj", entry.m_notification);
            result.push_back(obj);
            elementCount++;
        }
        return std::make_pair(firstElement, elementCount);
    }

    // removes notifications in the given range from the queue
    void eraseRangeFromQueue(const queueRange_t range)
    {
        LOCK(m_cs_notification_queue);
        m_queue.erase(m_queue.begin() + range.first, m_queue.begin() + range.first + range.second);
    }

    // dequeues all notifications from the queue
    void dequeueElements(UniValue& result)
    {
        queueRange_t range = weakDequeueNotifications(result);
        eraseRangeFromQueue(range);
    }

    bool elementsAvailable()
    {
        LOCK(m_cs_notification_queue);
        return m_queue.size() > 0;
    }

    void registerType(NotificationType type)
    {
        if (type == NotificationType::Unknown)
            return;

        LOCK(m_cs_notification_queue);
        m_registered_notification_types.insert(type);
    }

    void unregisterType(NotificationType type)
    {
        LOCK(m_cs_notification_queue);
        m_registered_notification_types.erase(type);
    }

    void unregisterAllTypes()
    {
        LOCK(m_cs_notification_queue);
        m_registered_notification_types.clear();
    }

    void addToQueue(NotificationEntry entry)
    {
        LOCK(m_cs_notification_queue);

        size_t queueSize = m_queue.size();
        if (queueSize > MAX_QUEUE_SIZE) {
            m_queue.pop_front();
            LogPrintf("RPC Notification limit has been reached, dropping oldest element\n");
        }
        m_map_sequence_numbers[entry.m_type]++;
        entry.m_sequence_number = m_map_sequence_numbers[entry.m_type];
        m_queue.push_back(entry);
    }

    /* checks if a certain notification type is registered */
    bool isTypeRegistered(NotificationType type)
    {
        LOCK(m_cs_notification_queue);
        return (m_registered_notification_types.find(type) != m_registered_notification_types.end());
    }
};

class NotificationQueueManager : public CValidationInterface
{
public:
    CCriticalSection m_cs_queue_manager;
    std::map<clientUUID_t, NotificationQueue*> m_map_sequence_numbers;

    NotificationQueue* getQueue(const clientUUID_t& clientid)
    {
        LOCK(m_cs_queue_manager);
        return m_map_sequence_numbers[clientid];
    }

    NotificationQueue* addQueue(const clientUUID_t& clientid)
    {
        LOCK(m_cs_queue_manager);
        m_map_sequence_numbers[clientid] = new NotificationQueue();
        return m_map_sequence_numbers[clientid];
    }

    void NotifyTransaction(const CTransactionRef& ptx)
    {
        LOCK(m_cs_queue_manager);
        for (auto& queueEntry : m_map_sequence_numbers) {
            if (!queueEntry.second->isTypeRegistered(NotificationType::Tx)) continue;

            NotificationEntry entry;
            entry.m_type = NotificationType::Tx;
            entry.m_notification.setStr(ptx->GetHash().GetHex());
            queueEntry.second->addToQueue(entry);
        }
    }

    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected, const std::vector<CTransactionRef>& vtxConflicted) override
    {
        for (const CTransactionRef& ptx : pblock->vtx) {
            // Do a normal notify for each transaction added in the block
            NotifyTransaction(ptx);
        }
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock) override
    {
        for (const CTransactionRef& ptx : pblock->vtx) {
            // Do a normal notify for each transaction removed in block disconnection
            NotifyTransaction(ptx);
        }
    }

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override
    {
        LOCK(m_cs_queue_manager);
        BOOST_FOREACH (NotificationQueue* queue, m_map_sequence_numbers | boost::adaptors::map_values) {
            if (!queue->isTypeRegistered(NotificationType::Block)) continue;

            NotificationEntry entry;
            entry.m_type = NotificationType::Block;
            entry.m_notification.setStr(pindexNew->GetBlockHash().GetHex());
            queue->addToQueue(entry);
        }
    }

    void TransactionAddedToMempool(const CTransactionRef& ptx) override
    {
        NotifyTransaction(ptx);
    }
};

CCriticalSection cs_queueManagerSharedInstance;
static NotificationQueueManager* queueManagerSharedInstance = NULL;

NotificationQueue* getQueue(const std::string& clientID, bool createIfNotExists)
{
    LOCK(cs_queueManagerSharedInstance);
    NotificationQueue* clientQueue = queueManagerSharedInstance->getQueue(clientID);
    if (!clientQueue && !createIfNotExists)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Client UUID not found.");
    if (!clientQueue)
        clientQueue = queueManagerSharedInstance->addQueue(clientID);
    return clientQueue;
}

UniValue setregisterednotifications(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "setregisterednotifications <uuid> [<type>, <type>, ...]\n"
            "Register for rpc notification(s).\n"
            "Notifications can be polled by calling pollnotifications."
            "The client UUID must be unique per client application and will define the used queue."
            "\nArguments:\n"
            "1. \"uuid\"         (string, required) The client uuid\n"
            "2. \"type\"         (string, required) The notification type to register for (\"hashblock\", \"hashtx\")\n"
            "\nExamples:\n"
            "\nRegister for block and transaction notifications\n" +
            HelpExampleCli("setregisterednotifications", "\"[\"hashblock\", \"hashtx\"]\" \"") +
            "register for transaction and block signals\n");

    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR)(UniValue::VARR), true);

    NotificationQueue* clientQueue = getQueue(request.params[0].get_str(), true);

    /* remove all current registered types */
    clientQueue->unregisterAllTypes();

    UniValue types = request.params[1].get_array();
    BOOST_FOREACH (const UniValue& newType, types.getValues()) {
        if (!newType.isStr())
            continue;

        NotificationType type = clientQueue->stringToType(newType.get_str());
        if (type == NotificationType::Unknown) {
            /* don't register only for a subset of the requested notifications */
            clientQueue->unregisterAllTypes();
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Notification type not found");
        }

        clientQueue->registerType(type);
    }

    return NullUniValue;
}

UniValue getregisterednotifications(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getregisterednotifications <uuid>\n"
            "\nReturns the currently registered RPC notification types for the given uuid.\n"
            "\nArguments:\n"
            "1. \"uuid\"         (string, required) The client uuid\n"
            "\nResult:\n"
            "\"[\"\n"
            "\"  \"<signal>\"             (string) The registered signal\n"
            "\"  ,...\n"
            "\"]\"\n"
            "\nExamples:\n"
            "\nCreate a transaction\n" +
            HelpExampleCli("getregisterednotifications", "") +
            "Get the registered notification types\n" + HelpExampleRpc("getregisterednotifications", ""));

    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR), true);
    NotificationQueue* clientQueue = getQueue(request.params[0].get_str(), false);

    UniValue result = UniValue(UniValue::VARR);
    BOOST_FOREACH (NotificationType type, clientQueue->m_registered_notification_types)
        result.push_back(clientQueue->typeToString(type));

    return result;
}

UniValue pollnotifications(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "pollnotifications <uuid> <timeout>\n"
            "\nLong poll function to get all available notifications for a given uuid (see setregisterednotifications for how to register for notifications).\n"
            "The RPC thread will idle for the via <timeout> defined amount of seconds and/or will immediately response if new notifications are available\n"
            "Arguments:\n"
            "1. \"uuid\"         (string, required) The client uuid\n"
            "2. \"timeout\"      (numeric, optional) The timeout \n"
            "\nResult:\n"
            "\"[ notification, ... ]\"             (object) The notification object\n"
            "\nExamples:\n"
            "\nPoll notifications for client a8098c1a...\n" +
            HelpExampleCli("pollnotifications", "\"a8098c1a-f86e-11da-bd1a-00112444be1e\" 500") +
            "Long poll notification (max. 500 seconds)\n" + HelpExampleRpc("pollnotifications", "\"a8098c1a-f86e-11da-bd1a-00112444be1e\" 500"));

    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR)(UniValue::VNUM), false);
    NotificationQueue* clientQueue = getQueue(request.params[0].get_str(), false);

    int64_t timeOut = DEFAULT_POLL_TIMEOUT;
    if (request.params.size() == 2)
        timeOut = request.params[1].get_int64();

    int64_t startTime = GetTime();

    UniValue result = UniValue(UniValue::VARR);
    // allow long polling
    while (!ShutdownRequested()) {
        if (clientQueue->elementsAvailable()) {
            clientQueue->dequeueElements(result);
            break;
        }
        if (startTime + timeOut + (500 / 1000.0) < GetTime())
            break;
        MilliSleep(500);
    }

    return result;
}

static const CRPCCommand commands[] =
{ //  category              name                           actor (function)             argNames
  //  --------------------- ----------------------------   ---------------------------  ----------
    { "notification",       "setregisterednotifications",  &setregisterednotifications, {"uuid","type"} },
    { "notification",       "getregisterednotifications",  &getregisterednotifications, {"uuid"} },
    { "notification",       "pollnotifications",           &pollnotifications,          {"uuid","timeout"} },

};

void RegisterLongPollNotificationsRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++) {
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
    }
}

void RegisterLongPollNotificationsInterface()
{
    LOCK(cs_queueManagerSharedInstance);
    if (!queueManagerSharedInstance) {
        queueManagerSharedInstance = new NotificationQueueManager();
        RegisterValidationInterface(queueManagerSharedInstance);
    }
}
