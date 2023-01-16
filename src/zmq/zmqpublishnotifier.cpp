// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqpublishnotifier.h>

#include <chain.h>
#include <chainparams.h>
#include <crypto/common.h>
#include <kernel/cs_main.h>
#include <logging.h>
#include <netaddress.h>
#include <netbase.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <version.h>
#include <zmq/zmqutil.h>

#include <zmq.h>

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Consensus {
struct Params;
}

using node::ReadBlockFromDisk;
extern int RPCSerializationFlags();
static std::multimap<std::string, CZMQAbstractPublishNotifier*> mapPublishNotifiers;
static bool bFirstTime = true;
static const char *MSG_HASHBLOCK = "hashblock";
static const char *MSG_HASHTX    = "hashtx";
static const char *MSG_RAWBLOCK  = "rawblock";
static const char *MSG_RAWTX     = "rawtx";
// SYSCOIN
static const char *MSG_NEVMBLOCKCONNECT  = "nevmconnect";
static const char *MSG_NEVMCOMMS  = "nevmcomms";
static const char *MSG_NEVMBLOCKDISCONNECT  = "nevmdisconnect";
static const char *MSG_NEVMBLOCK  = "nevmblock";
static const char *MSG_NEVMBLOCKINFO  = "nevmblockinfo";
static const char *MSG_RAWMEMPOOLTX  = "rawmempooltx";
static const char *MSG_HASHGVOTE     = "hashgovernancevote";
static const char *MSG_HASHGOBJ      = "hashgovernanceobject";
static const char *MSG_SEQUENCE  = "sequence";
RecursiveMutex cs_nevm;
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
static bool IsZMQAddressIPV6(const std::string &zmq_address)
{
    const std::string tcp_prefix = "tcp://";
    const size_t tcp_index = zmq_address.rfind(tcp_prefix);
    const size_t colon_index = zmq_address.rfind(":");
    if (tcp_index == 0 && colon_index != std::string::npos) {
        const std::string ip = zmq_address.substr(tcp_prefix.length(), colon_index - tcp_prefix.length());
        CNetAddr addr;
        LookupHost(ip, addr, false);
        if (addr.IsIPv6()) return true;
    }
    return false;
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
    // timed out
    if (rc == -1) {
        zmq_msg_close (&part);
        return -1;
    }
    int size = zmq_msg_size (&part);
    char* buf = static_cast<char*>(zmq_msg_data(&part));
    parts.emplace_back(std::string(buf, size));
    /* Determine if more message parts are to follow */
    rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
    assert (rc == 0);
    zmq_msg_close (&part); } while (more);
    return 0;
}

bool CZMQAbstractPublishNotifier::Initialize(void *pcontext, void *pcontextsub)
{
    assert(!psocket && !psocketsub);

    // check if address is being used by other publish notifier
    std::multimap<std::string, CZMQAbstractPublishNotifier*>::iterator i = mapPublishNotifiers.find(address);
    if (i==mapPublishNotifiers.end())
    {
        if(!addresssub.empty()) {
            psocketsub = zmq_socket(pcontextsub, ZMQ_REQ);
            if (!psocketsub)
            {
                zmqError("Failed to create socket");
                return false;
            }
            int rc = zmq_connect(psocketsub, addresssub.c_str());
            if (rc != 0)
            {
                zmqError("Failed to bind address for subscriber");
                zmq_close(psocketsub);
                return false;
            }
            int timeout = 60000;
            rc = zmq_setsockopt(psocketsub, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
            if (rc != 0) {
                zmqError("Failed to set ZMQ_SNDTIMEO");
                zmq_close(psocketsub);
                return false;
            }
            LogPrint(BCLog::ZMQ, "REQ subscribed on address %s\n", addresssub);
        } else {
            psocket = zmq_socket(pcontext, ZMQ_PUB);
            if (!psocket)
            {
                zmqError("Failed to create socket");
                return false;
            }

            LogPrint(BCLog::ZMQ, "Outbound message high water mark for %s at %s is %d\n", type, address, outbound_message_high_water_mark);

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
            // On some systems (e.g. OpenBSD) the ZMQ_IPV6 must not be enabled, if the address to bind isn't IPv6
            const int enable_ipv6 { IsZMQAddressIPV6(address) ? 1 : 0};
            rc = zmq_setsockopt(psocket, ZMQ_IPV6, &enable_ipv6, sizeof(enable_ipv6));
            if (rc != 0) {
                zmqError("Failed to set ZMQ_IPV6");
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
        }
        // register this notifier for the address, so it can be reused for other publish notifier
        mapPublishNotifiers.insert(std::make_pair(address, this));
        return true;
    }
    else
    {
        LogPrint(BCLog::ZMQ, "Reusing socket for address %s, subscriber %s\n", address, addresssub);
        LogPrint(BCLog::ZMQ, "Outbound message high water mark for %s at %s is %d\n", type, address, outbound_message_high_water_mark);

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
        LogPrint(BCLog::ZMQ, "Close socket at address %s\n", address);
        if(psocket) {
            int linger = 0;
            zmq_setsockopt(psocket, ZMQ_LINGER, &linger, sizeof(linger));
            zmq_close(psocket);
        }
    }
    // SYSCOIN
    if(psocketsub) {
        int linger = 0;
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
bool CZMQAbstractPublishNotifier::SendZmqMessageNEVM(const char *command, const void* data, size_t size)
{
    assert(psocketsub);

    int rc = zmq_send_multipart(psocketsub, command, strlen(command), data, size, nullptr);
    if (rc == -1)
        return false;

    return true;
}

// SYSCOIN
bool CZMQAbstractPublishNotifier::ReceiveZmqMessage(std::vector<std::string>& parts)
{
    if(!psocketsub) {
        return false;
    }
    int rc = zmq_receive_multipart(psocketsub, parts);
    if (rc == -1)
        return false;
    return true;
}
bool CZMQPublishNEVMCommsNotifier::NotifyNEVMComms(const std::string &commMessage, bool &bResponse)
{
    LOCK(cs_nevm);
    bResponse = false;
    if(psocketsub) {
        int timeout = 150000;
        int rc = zmq_setsockopt(psocketsub, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        if (rc != 0) {
            zmqError("Failed to set ZMQ_RCVTIMEO");
            zmq_close(psocketsub);
            return false;
        }
    }
    std::vector<std::string> parts;
    LogPrint(BCLog::ZMQ, "zmq: Publish nevm communication %s, subscriber %s\n", commMessage, this->addresssub);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << commMessage;
    if(!SendZmqMessageNEVM(MSG_NEVMCOMMS, &(*ss.begin()), ss.size())) {
        LogPrint(BCLog::SYS, "NotifyNEVMComms: nevm-connect-not-sent: %s\n", commMessage);
        return false;
    }
    if(commMessage != "disconnect") {
        if(ReceiveZmqMessage(parts)) {
            if(parts.size() != 2) {
                LogPrint(BCLog::SYS, "NotifyNEVMComms: nevm-response-invalid-parts\n");
                return false;  
            }
            if(parts[0] != MSG_NEVMCOMMS) {
                LogPrint(BCLog::SYS, "NotifyNEVMComms: nevm-response-wrong-command\n");
                return false;
            }
            if(parts[1] != "ack") {
                LogPrint(BCLog::SYS, "NotifyNEVMComms: nevm-comms-response-invalid-data\n");
                return false;
            }
            bResponse = true;
        } else {
            LogPrint(BCLog::SYS, "NotifyNEVMComms: nevm-response-not-found: %s\n", commMessage);
            return false;
        }
    } else {
        bResponse = true;
    }
    return true;
}
bool CZMQPublishNEVMBlockConnectNotifier::NotifyNEVMBlockConnect(const CNEVMHeader &evmBlock, const CBlock& block, std::string &state, const uint256& nSYSBlockHash, NEVMDataVec &NEVMDataVecOut, const uint32_t& nHeight, bool bSkipValidation)
{
    LOCK(cs_nevm);
    state = "";
    if(bFirstTime) {
        bFirstTime = false;
        bool bResponse = false;
        NotifyNEVMComms("status", bResponse);
        if(!bResponse) {
            state = "nevm-not-connected";
            return false;
        }
    }
    if(psocketsub) {
        int timeout = 150000;
        int rc = zmq_setsockopt(psocketsub, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        if (rc != 0) {
            zmqError("Failed to set ZMQ_RCVTIMEO");
            zmq_close(psocketsub);
            state = "ZMQ_RCVTIMEO";
            return false;
        }
    }
    std::vector<std::string> parts;
    uint256 hash = evmBlock.nBlockHash;
    LogPrint(BCLog::ZMQ, "zmq: Publish nevm block connect %s to %s, subscriber %s\n", hash.GetHex(), this->address, this->addresssub);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << evmBlock << block.vchNEVMBlockData << nSYSBlockHash << NEVMDataVecOut;
    if(!SendZmqMessageNEVM(MSG_NEVMBLOCKCONNECT, &(*ss.begin()), ss.size())) {
        state = "nevm-connect-not-sent";
        return false;
    }
    if(ReceiveZmqMessage(parts)) {
        if(parts.size() != 2) {
            state = "nevm-response-invalid-parts";
            return false;
        }
        if(parts[0] != MSG_NEVMBLOCKCONNECT) {
            state = "nevm-response-wrong-command";
            return false;
        }
        if(!bSkipValidation && parts[1] != "connected") {
            LogPrint(BCLog::SYS, "NotifyNEVMBlockConnect: %s\n", parts[1]);
            state = "nevm-connect-response-invalid-data";
            return false;
        }
    } else if (!bSkipValidation) {
        state = "nevm-response-not-found";
        return false;
    }

    return true;
}
bool CZMQPublishNEVMBlockDisconnectNotifier::NotifyNEVMBlockDisconnect(std::string &state, const uint256& nSYSBlockHash)
{
    LOCK(cs_nevm);
    if(bFirstTime) {
        bFirstTime = false;
        bool bResponse = false;
        NotifyNEVMComms("status", bResponse);
        if(!bResponse) {
            state = "nevm-not-connected";
            return false;
        }
    }
    if(psocketsub) {
        int timeout = 30000;
        int rc = zmq_setsockopt(psocketsub, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        if (rc != 0) {
            zmqError("Failed to set ZMQ_RCVTIMEO");
            zmq_close(psocketsub);
            state = "ZMQ_RCVTIMEO";
            return false;
        }
    }
    std::vector<std::string> parts;
    LogPrint(BCLog::ZMQ, "zmq: Publish nevm block disconnect %s to %s, subscriber %s\n", nSYSBlockHash.GetHex(), this->address, this->addresssub);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << nSYSBlockHash;
    if(!SendZmqMessageNEVM(MSG_NEVMBLOCKDISCONNECT, &(*ss.begin()), ss.size())) {
        state = "nevm-disconnect-not-sent";
        return false;
    }
    if(ReceiveZmqMessage(parts)) {
        if(parts.size() != 2) {
            state = "nevm-response-invalid-parts";
            return false;   
        }
        if(parts[0] != MSG_NEVMBLOCKDISCONNECT) {
            state = "nevm-response-wrong-command";
            return false;
        }
        if(parts[1] != "disconnected") {
            state = "nevm-disconnect-response-invalid-data";
            return false;
        }
        return true;
    } else {
        state = "nevm-response-not-found";
        return false;
    }
    return true;
}
bool CZMQPublishNEVMBlockInfoNotifier::NotifyGetNEVMBlockInfo(uint64_t &nHeight, std::string &state)
{
    LOCK(cs_nevm);
    if(bFirstTime) {
        bFirstTime = false;
        bool bResponse = false;
        NotifyNEVMComms("status", bResponse);
        if(!bResponse) {
            state = "nevm-not-connected";
            return false;
        }
    }
    LogPrint(BCLog::ZMQ, "zmq: Publish nevm block info to %s, subscriber %s\n", this->address, this->addresssub);
    if(!SendZmqMessageNEVM(MSG_NEVMBLOCKINFO, MSG_NEVMBLOCKINFO, strlen(MSG_NEVMBLOCKINFO))) {
        state = "nevm-header-not-sent";
        return false;
    }
    std::vector<std::string> parts;
    if(ReceiveZmqMessage(parts)) {
        if(parts.size() != 2) {
            state = "nevm-response-invalid-parts";
            return false;
        }
        if(parts[0] != MSG_NEVMBLOCKINFO) {
            state = "nevm-response-wrong-command";
            return false;
        }
        if(!ParseUInt64(parts[1], &nHeight)) {
            state = "nevm-response-unserialize";
            return false;
        }
            
    } else {
        state = "nevm-response-not-found";
        return false;
    }
    return true;
}

bool CZMQPublishNEVMBlockNotifier::NotifyGetNEVMBlock(CNEVMBlock &evmBlock, std::string &state)
{
    LOCK(cs_nevm);
    if(bFirstTime) {
        bFirstTime = false;
        bool bResponse = false;
        NotifyNEVMComms("status", bResponse);
        if(!bResponse) {
            state = "nevm-not-connected";
            return false;
        }
    }
    LogPrint(BCLog::ZMQ, "zmq: Publish nevm block to %s, subscriber %s\n", this->address, this->addresssub);
    if(!SendZmqMessageNEVM(MSG_NEVMBLOCK, MSG_NEVMBLOCK, strlen(MSG_NEVMBLOCK))) {
        state = "nevm-header-not-sent";
        return false;
    }
    std::vector<std::string> parts;
    if(ReceiveZmqMessage(parts)) {
        if(parts.size() != 2) {
            state = "nevm-response-invalid-parts";
            return false;
        }
        if(parts[0] != MSG_NEVMBLOCK) {
            state = "nevm-response-wrong-command";
            return false;
        }
        const std::vector<unsigned char> evmData{parts[1].begin(), parts[1].end()};
        // SYSCOIN
        CDataStream ss(evmData, SER_NETWORK, PROTOCOL_VERSION);
        try {
            ss >> evmBlock;
        } catch (const std::exception& e) {
            state = "nevm-response-unserialize";
            return false;
        }
        if(evmBlock.nBlockHash.IsNull()) {
            state = "nevm-response-parse-hash";
            return false;
        }
        if(evmBlock.nTxRoot.IsNull()) {
            state = "nevm-response-invalid-txroot";
            return false;
        }
        if(evmBlock.nReceiptRoot.IsNull()) {
            state = "nevm-response-invalid-receiptroot";
            return false;
        }
        if(evmBlock.vchNEVMBlockData.empty()) {
            state = "nevm-response-empty-data";
            return false;
        }
    } else {
        state = "nevm-response-not-found";
        return false;
    }
    return true;
}
bool CZMQPublishHashBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "Publish hashblock %s to %s\n", hash.GetHex(), this->address);
    uint8_t data[32];
    for (unsigned int i = 0; i < 32; i++) {
        data[31 - i] = hash.begin()[i];
    }
    return SendZmqMessage(MSG_HASHBLOCK, data, 32);
}

bool CZMQPublishHashTransactionNotifier::NotifyTransaction(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "Publish hashtx %s to %s\n", hash.GetHex(), this->address);
    uint8_t data[32];
    for (unsigned int i = 0; i < 32; i++) {
        data[31 - i] = hash.begin()[i];
    }
    return SendZmqMessage(MSG_HASHTX, data, 32);
}

bool CZMQPublishRawBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    LogPrint(BCLog::ZMQ, "Publish rawblock %s to %s\n", pindex->GetBlockHash().GetHex(), this->address);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
    CBlock block;
    if (!ReadBlockFromDisk(block, pindex, consensusParams)) {
        zmqError("Can't read block from disk");
        return false;
    }

    ss << block;

    return SendZmqMessage(MSG_RAWBLOCK, &(*ss.begin()), ss.size());
}
bool CZMQPublishRawTransactionNotifier::NotifyTransaction(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "Publish rawtx %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
    ss << transaction;
    return SendZmqMessage(MSG_RAWTX, &(*ss.begin()), ss.size());
}
// SYSCOIN
bool CZMQPublishHashGovernanceVoteNotifier::NotifyGovernanceVote(const uint256& hash)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish hashgovernancevote %s\n", hash.GetHex());
    uint8_t data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGVOTE, data, 32);
}

bool CZMQPublishHashGovernanceObjectNotifier::NotifyGovernanceObject(const uint256& hash)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish hashgovernanceobject %s\n", hash.GetHex());
    uint8_t data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGOBJ, data, 32);
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
    LogPrint(BCLog::ZMQ, "Publish sequence block connect %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Block (C)onnect */ 'C');
}

bool CZMQPublishSequenceNotifier::NotifyBlockDisconnect(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "Publish sequence block disconnect %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Block (D)isconnect */ 'D');
}

bool CZMQPublishSequenceNotifier::NotifyTransactionAcceptance(const CTransaction &transaction, uint64_t mempool_sequence)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "Publish hashtx mempool acceptance %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Mempool (A)cceptance */ 'A', mempool_sequence);
}

bool CZMQPublishSequenceNotifier::NotifyTransactionRemoval(const CTransaction &transaction, uint64_t mempool_sequence)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "Publish hashtx mempool removal %s to %s\n", hash.GetHex(), this->address);
    return SendSequenceMsg(*this, hash, /* Mempool (R)emoval */ 'R', mempool_sequence);
}
