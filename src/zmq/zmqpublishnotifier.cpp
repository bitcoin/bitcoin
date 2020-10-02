// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqpublishnotifier.h>

#include <chain.h>
#include <chainparams.h>
#include <rpc/server.h>
#include <streams.h>
#include <util/system.h>
#include <validation.h>
#include <zmq/zmqutil.h>

#include <zmq.h>

#include <cstdarg>
#include <cstddef>
#include <map>
#include <string>
#include <utility>

static std::multimap<std::string, CZMQAbstractPublishNotifier*> mapPublishNotifiers;

static const char *MSG_HASHBLOCK = "hashblock";
static const char *MSG_HASHTX    = "hashtx";
static const char *MSG_RAWBLOCK  = "rawblock";
static const char *MSG_RAWTX     = "rawtx";
static const char *MSG_SEQUENCE  = "sequence";

// Internal function to send multipart message
static int zmq_send_multipart(void *sock, const void* data, size_t size, ...)
{
    va_list args;
    va_start(args, size);

    while (1)
    {
        zmq_msg_t msg;

        int rc = zmq_msg_init_size(&msg, size);
        if (rc != 0)
        {
            zmqError("Unable to initialize ZMQ msg");
            va_end(args);
            return -1;
        }

        void *buf = zmq_msg_data(&msg);
        memcpy(buf, data, size);

        data = va_arg(args, const void*);

        rc = zmq_msg_send(&msg, sock, data ? ZMQ_SNDMORE : 0);
        if (rc == -1)
        {
            zmqError("Unable to send ZMQ msg");
            zmq_msg_close(&msg);
            va_end(args);
            return -1;
        }

        zmq_msg_close(&msg);

        if (!data)
            break;

        size = va_arg(args, size_t);
    }
    va_end(args);
    return 0;
}

bool CZMQAbstractPublishNotifier::Initialize(void *pcontext)
{
    assert(!psocket);

    // check if address is being used by other publish notifier
    std::multimap<std::string, CZMQAbstractPublishNotifier*>::iterator i = mapPublishNotifiers.find(address);

    if (i==mapPublishNotifiers.end())
    {
        psocket = zmq_socket(pcontext, ZMQ_PUB);
        if (!psocket)
        {
            zmqError("Failed to create socket");
            return false;
        }

        LogPrint(BCLog::ZMQ, "zmq: Outbound message high water mark for %s at %s is %d\n", type, address, outbound_message_high_water_mark);

        int rc = zmq_setsockopt(psocket, ZMQ_SNDHWM, &outbound_message_high_water_mark, sizeof(outbound_message_high_water_mark));
        if (rc != 0)
        {
            zmqError("Failed to set outbound message high water mark");
            zmq_close(psocket);
            return false;
        }

        const int so_keepalive_option {1};
        rc = zmq_setsockopt(psocket, ZMQ_TCP_KEEPALIVE, &so_keepalive_option, sizeof(so_keepalive_option));
        if (rc != 0) {
            zmqError("Failed to set SO_KEEPALIVE");
            zmq_close(psocket);
            return false;
        }

        rc = zmq_bind(psocket, address.c_str());
        if (rc != 0)
        {
            zmqError("Failed to bind address");
            zmq_close(psocket);
            return false;
        }

        // register this notifier for the address, so it can be reused for other publish notifier
        mapPublishNotifiers.insert(std::make_pair(address, this));
        return true;
    }
    else
    {
        LogPrint(BCLog::ZMQ, "zmq: Reusing socket for address %s\n", address);
        LogPrint(BCLog::ZMQ, "zmq: Outbound message high water mark for %s at %s is %d\n", type, address, outbound_message_high_water_mark);

        psocket = i->second->psocket;
        mapPublishNotifiers.insert(std::make_pair(address, this));

        return true;
    }
}

void CZMQAbstractPublishNotifier::Shutdown()
{
    // Early return if Initialize was not called
    if (!psocket) return;

    int count = mapPublishNotifiers.count(address);

    // remove this notifier from the list of publishers using this address
    typedef std::multimap<std::string, CZMQAbstractPublishNotifier*>::iterator iterator;
    std::pair<iterator, iterator> iterpair = mapPublishNotifiers.equal_range(address);

    for (iterator it = iterpair.first; it != iterpair.second; ++it)
    {
        if (it->second==this)
        {
            mapPublishNotifiers.erase(it);
            break;
        }
    }

    if (count == 1)
    {
        LogPrint(BCLog::ZMQ, "zmq: Close socket at address %s\n", address);
        int linger = 0;
        zmq_setsockopt(psocket, ZMQ_LINGER, &linger, sizeof(linger));
        zmq_close(psocket);
    }

    psocket = nullptr;
}

bool CZMQAbstractPublishNotifier::SendZmqMessage(const char *command, const void* data, size_t size)
{
    assert(psocket);

    /* send three parts, command & data & a LE 4byte sequence number */
    unsigned char msgseq[sizeof(uint32_t)];
    WriteLE32(&msgseq[0], nSequence);
    int rc = zmq_send_multipart(psocket, command, strlen(command), data, size, msgseq, (size_t)sizeof(uint32_t), nullptr);
    if (rc == -1)
        return false;

    /* increment memory only sequence number after sending */
    nSequence++;

    return true;
}

bool CZMQPublishHashBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashblock %s to %s\n", hash.GetHex(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHBLOCK, data, 32);
}

bool CZMQPublishHashTransactionNotifier::NotifyTransaction(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashtx %s to %s\n", hash.GetHex(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHTX, data, 32);
}

bool CZMQPublishRawBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish rawblock %s to %s\n", pindex->GetBlockHash().GetHex(), this->address);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
    {
        LOCK(cs_main);
        CBlock block;
        if(!ReadBlockFromDisk(block, pindex, consensusParams))
        {
            zmqError("Can't read block from disk");
            return false;
        }

        ss << block;
    }

    return SendZmqMessage(MSG_RAWBLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawTransactionNotifier::NotifyTransaction(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawtx %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
    ss << transaction;
    return SendZmqMessage(MSG_RAWTX, &(*ss.begin()), ss.size());
}


// TODO: Dedup this code to take label char, log string
bool CZMQPublishSequenceNotifier::NotifyBlockConnect(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish sequence block connect %s to %s\n", hash.GetHex(), this->address);
    char data[sizeof(uint256)+1];
    for (unsigned int i = 0; i < sizeof(uint256); i++)
        data[sizeof(uint256) - 1 - i] = hash.begin()[i];
    data[sizeof(data) - 1] = 'C'; // Block (C)onnect
    return SendZmqMessage(MSG_SEQUENCE, data, sizeof(data));
}

bool CZMQPublishSequenceNotifier::NotifyBlockDisconnect(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish sequence block disconnect %s to %s\n", hash.GetHex(), this->address);
    char data[sizeof(uint256)+1];
    for (unsigned int i = 0; i < sizeof(uint256); i++)
        data[sizeof(uint256) - 1 - i] = hash.begin()[i];
    data[sizeof(data) - 1] = 'D'; // Block (D)isconnect
    return SendZmqMessage(MSG_SEQUENCE, data, sizeof(data));
}

bool CZMQPublishSequenceNotifier::NotifyTransactionAcceptance(const CTransaction &transaction, uint64_t mempool_sequence)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashtx mempool acceptance %s to %s\n", hash.GetHex(), this->address);
    unsigned char data[sizeof(uint256)+sizeof(mempool_sequence)+1];
    for (unsigned int i = 0; i < sizeof(uint256); i++)
        data[sizeof(uint256) - 1 - i] = hash.begin()[i];
    data[sizeof(uint256)] = 'A'; // Mempool (A)cceptance
    WriteLE64(data+sizeof(uint256)+1, mempool_sequence);
    return SendZmqMessage(MSG_SEQUENCE, data, sizeof(data));
}

bool CZMQPublishSequenceNotifier::NotifyTransactionRemoval(const CTransaction &transaction, uint64_t mempool_sequence)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashtx mempool removal %s to %s\n", hash.GetHex(), this->address);
    unsigned char data[sizeof(uint256)+sizeof(mempool_sequence)+1];
    for (unsigned int i = 0; i < sizeof(uint256); i++)
        data[sizeof(uint256) - 1 - i] = hash.begin()[i];
    data[sizeof(uint256)] = 'R'; // Mempool (R)emoval
    WriteLE64(data+sizeof(uint256)+1, mempool_sequence);
    return SendZmqMessage(MSG_SEQUENCE, data, sizeof(data));
}
