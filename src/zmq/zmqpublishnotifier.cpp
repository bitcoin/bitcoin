// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqpublishnotifier.h>

#include <chain.h>
#include <chainparams.h>
#include <node/blockstorage.h>
#include <rpc/server.h>
#include <streams.h>
#include <util/system.h>
#include <validation.h> // For cs_main
#include <zmq/zmqutil.h>

#include <zmq.h>

#include <cstdarg>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
// SYSCOIN
#include <governance/governancevote.h>
#include <governance/governanceobject.h>
#include <core_io.h>
static std::multimap<std::string, CZMQAbstractPublishNotifier*> mapPublishNotifiers;

static const char *MSG_HASHBLOCK = "hashblock";
static const char *MSG_HASHTX    = "hashtx";
static const char *MSG_RAWBLOCK  = "rawblock";
static const char *MSG_RAWTX     = "rawtx";
// SYSCOIN
static const char *MSG_NEVMBLOCKCONNECT  = "nevmblockconnect";
static const char *MSG_NEVMBLOCKDISCONNECT  = "nevmblockdisconnect";
static const char *MSG_NEVMBLOCK  = "nevmblock";
static const char *MSG_RAWMEMPOOLTX  = "rawmempooltx";
static const char *MSG_HASHGVOTE     = "hashgovernancevote";
static const char *MSG_HASHGOBJ      = "hashgovernanceobject";
static const char *MSG_RAWGVOTE      = "rawgovernancevote";
static const char *MSG_RAWGOBJ       = "rawgovernanceobject";
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

// Internal function to receive multipart message
static int zmq_receive_multipart(void *socket, std::vector<std::string>& parts)
{
    int64_t more;
    size_t more_size = sizeof (more);
    do {
    /* Create an empty ØMQ message to hold the message part */
    zmq_msg_t part;
    int rc = zmq_msg_init (&part);
    assert (rc == 0);
    /* Block until a message is available to be received from socket */
    rc = zmq_msg_recv (&part, socket, 0);
    assert (rc != -1);
    int size = zmq_msg_size (&part);
    char* buf = static_cast<char*>(zmq_msg_data(&part));
    parts.emplace_back(std::string(buf, size));
    /* Determine if more message parts are to follow */
    rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
    assert (rc == 0);
    zmq_msg_close (&part); } while (more);
    return 0;
}
// SYSCOIN
bool CZMQAbstractPublishNotifier::Initialize(void *pcontext, void *pcontextsub)
{
    assert(!psocket && !psocketsub);

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
            zmqError("Failed to bind address for publisher");
            zmq_close(psocket);
            return false;
        }
        // SYSCOIN
        psocketsub = zmq_socket(pcontextsub, ZMQ_SUB);
        if (!psocketsub)
        {
            zmqError("Failed to create socket");
            return false;
        }

        rc = zmq_setsockopt(psocketsub, ZMQ_TCP_KEEPALIVE, &so_keepalive_option, sizeof(so_keepalive_option));
        if (rc != 0) {
            zmqError("Failed to set SO_KEEPALIVE");
            zmq_close(psocketsub);
            return false;
        }

        rc = zmq_connect(psocketsub, address.c_str());
        if (rc != 0)
        {
            zmqError("Failed to bind address for subscriber");
            zmq_close(psocketsub);
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
        psocketsub = i->second->psocketsub;
        mapPublishNotifiers.insert(std::make_pair(address, this));

        return true;
    }
}

void CZMQAbstractPublishNotifier::Shutdown()
{
    // Early return if Initialize was not called
    if (!psocket && !psocketsub) return;

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
        // SYSCOIN
        linger = 0;
        zmq_setsockopt(psocketsub, ZMQ_LINGER, &linger, sizeof(linger));
        zmq_close(psocketsub);
    }

    psocket = nullptr;
    // SYSCOIN
    psocketsub = nullptr;
}

bool CZMQAbstractPublishNotifier::SendZmqMessage(const char *command, const void* data, size_t size)
{
    assert(psocket);

    /* send three parts, command & data & a LE 4byte sequence number */
    unsigned char msgseq[sizeof(uint32_t)];
    WriteLE32(msgseq, nSequence);
    int rc = zmq_send_multipart(psocket, command, strlen(command), data, size, msgseq, (size_t)sizeof(uint32_t), nullptr);
    if (rc == -1)
        return false;

    /* increment memory only sequence number after sending */
    nSequence++;

    return true;
}

// SYSCOIN
bool CZMQAbstractPublishNotifier::ReceiveZmqMessage(std::vector<std::string>& parts)
{
    assert(psocketsub);
    int rc = zmq_receive_multipart(psocketsub, parts);
    if (rc == -1)
        return false;
    return true;
}
bool CZMQPublishEVMNotifier::NotifyEVMBlockConnect(const CNEVMBlock &evmBlock, const uint256& nSYSBlockHash, const bool bWaitForResponse)
{
    std::vector<std::string> parts;
    uint256 hash = evmBlock.nBlockHash;
    LogPrint(BCLog::ZMQ, "zmq: Publish evm block connect %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << evmBlock << nSYSBlockHash;
    if(!SendZmqMessage(MSG_NEVMBLOCKCONNECT, &(*ss.begin()), ss.size()))
        return false;
    if(bWaitForResponse) {
        if(ReceiveZmqMessage(parts)) {
            if(parts.size() != 2) {
                LogPrint(BCLog::ZMQ, "zmq: Publish evm block connect wrong number of parts in multipart message %d: \n", parts.size());
                return false;    
            }
            if(parts[0] != MSG_NEVMBLOCKCONNECT) {
                LogPrint(BCLog::ZMQ, "zmq: Publish evm block connect wrong command: %s\n", parts[0]);
                return false;
            }
            if(parts[1] != "connected") {
                LogPrint(BCLog::ZMQ, "zmq: Publish evm block connect send invalid data: %s\n", parts[1]);
                return false;
            }
            return true;
        }
    } else {
        return true;
    }
    LogPrint(BCLog::ZMQ, "zmq: Publish evm block connect could not receive message\n");
    return false;
}
bool CZMQPublishEVMNotifier::NotifyEVMBlockDisconnect(const CNEVMBlock &evmBlock, const uint256& nSYSBlockHash, const bool bWaitForResponse)
{
    // disconnect shouldn't include the block data
    if(!evmBlock.vchNEVMBlockData.empty()) {
        return false;
    }
    std::vector<std::string> parts;
    uint256 hash = evmBlock.nBlockHash;
    LogPrint(BCLog::ZMQ, "zmq: Publish evm block disconnect %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << evmBlock << nSYSBlockHash;
    if(!SendZmqMessage(MSG_NEVMBLOCKDISCONNECT, &(*ss.begin()), ss.size()))
        return false;
    if(bWaitForResponse) {
        if(ReceiveZmqMessage(parts)) {
            if(parts.size() != 2) {
                LogPrint(BCLog::ZMQ, "zmq: Publish evm block disconnect wrong number of parts in multipart message %d: \n", parts.size());
                return false;    
            }
            if(parts[0] != MSG_NEVMBLOCKDISCONNECT) {
                LogPrint(BCLog::ZMQ, "zmq: Publish evm block disconnect wrong command: %s\n", parts[0]);
                return false;
            }
            if(parts[1] != "disconnected") {
                LogPrint(BCLog::ZMQ, "zmq: Publish evm block disconnect send invalid data: %s\n", parts[1]);
                return false;
            }
            return true;
        }
    } else {
        return true;
    }
    return true;
}
bool CZMQPublishEVMNotifier::NotifyGetNEVMBlock(CNEVMBlock &evmBlock)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish evmblock\n");
    char data[1];
    if(!SendZmqMessage(MSG_NEVMBLOCK, data, 1))
        return false;
    
    std::vector<std::string> parts;
    if(ReceiveZmqMessage(parts)) {
        if(parts.size() != 2) {
            LogPrint(BCLog::ZMQ, "zmq: Publish evm block wrong number of parts in multipart message %d: \n", parts.size());
            return false;    
        }
        if(parts[0] != MSG_NEVMBLOCK) {
            LogPrint(BCLog::ZMQ, "zmq: Publish evm block wrong command: %s\n", parts[0]);
            return false;
        }
        const std::vector<unsigned char> evmData{parts[1].begin(), parts[1].end()};
        // SYSCOIN
        CDataStream ss(evmData, SER_NETWORK, PROTOCOL_VERSION);
        try {
            ss >> evmBlock;
        } catch (const std::exception&) {
            return false;
        }
        if(evmBlock.nBlockHash.IsNull()) {
            LogPrint(BCLog::ZMQ, "zmq: Publish evm block could not parse block hash in response\n");
            return false;
        }

        if(evmBlock.vchTxRoot.size() != 32) {
            LogPrint(BCLog::ZMQ, "zmq: Publish evm block wrong tx root size %d\n", evmBlock.vchTxRoot.size());
            return false;
        }

        if(evmBlock.vchReceiptRoot.size() != 32) {
            LogPrint(BCLog::ZMQ, "zmq: Publish evm block wrong receipt root size %d\n", evmBlock.vchTxRoot.size());
            return false;
        }
        if(evmBlock.vchNEVMBlockData.empty()) {
            LogPrint(BCLog::ZMQ, "zmq: Publish evm block empty block datae\n");
            return false;
        }
        return true;
    }
    return false;
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
// SYSCOIN
bool CZMQPublishHashGovernanceVoteNotifier::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote)
{
    const uint256 &hash = vote->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashgovernancevote %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGVOTE, data, 32);
}

bool CZMQPublishHashGovernanceObjectNotifier::NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& object)
{
    const uint256 &hash = object->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashgovernanceobject %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGOBJ, data, 32);
}
bool CZMQPublishRawGovernanceVoteNotifier::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote)
{
    uint256 nHash = vote->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawgovernanceobject: hash = %s, vote = %d\n", nHash.ToString(), vote->ToString());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << vote;
    return SendZmqMessage(MSG_RAWGVOTE, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawGovernanceObjectNotifier::NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& govobj)
{
    uint256 nHash = govobj->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawgovernanceobject: hash = %s, type = %d\n", nHash.ToString(), govobj->GetObjectType());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << govobj;
    return SendZmqMessage(MSG_RAWGOBJ, &(*ss.begin()), ss.size());
}
bool CZMQPublishRawMempoolTransactionNotifier::NotifyTransactionMempool(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawmempooltx %s\n", hash.GetHex());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
    ss << transaction;
    return SendZmqMessage(MSG_RAWMEMPOOLTX, &(*ss.begin()), ss.size());
}

// Helper function to send a 'sequence' topic message with the following structure:
//    <32-byte hash> | <1-byte label> | <8-byte LE sequence> (optional)
static bool SendSequenceMsg(CZMQAbstractPublishNotifier& notifier, uint256 hash, char label, std::optional<uint64_t> sequence = {})
{
    unsigned char data[sizeof(hash) + sizeof(label) + sizeof(uint64_t)];
    for (unsigned int i = 0; i < sizeof(hash); ++i) {
        data[sizeof(hash) - 1 - i] = hash.begin()[i];
    }
    data[sizeof(hash)] = label;
    if (sequence) WriteLE64(data + sizeof(hash) + sizeof(label), *sequence);
    return notifier.SendZmqMessage(MSG_SEQUENCE, data, sequence ? sizeof(data) : sizeof(hash) + sizeof(label));
}

bool CZMQPublishSequenceNotifier::NotifyBlockConnect(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish sequence block connect %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Block (C)onnect */ 'C');
}

bool CZMQPublishSequenceNotifier::NotifyBlockDisconnect(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish sequence block disconnect %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Block (D)isconnect */ 'D');
}

bool CZMQPublishSequenceNotifier::NotifyTransactionAcceptance(const CTransaction &transaction, uint64_t mempool_sequence)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashtx mempool acceptance %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Mempool (A)cceptance */ 'A', mempool_sequence);
}

bool CZMQPublishSequenceNotifier::NotifyTransactionRemoval(const CTransaction &transaction, uint64_t mempool_sequence)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashtx mempool removal %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Mempool (R)emoval */ 'R', mempool_sequence);
}
