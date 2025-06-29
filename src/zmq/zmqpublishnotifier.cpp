// Copyright (c) 2015-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqpublishnotifier.h>

#include <chain.h>
#include <chainparams.h>
#include <netbase.h>
#include <node/blockstorage.h>
#include <streams.h>
#include <zmq/zmqutil.h>

#include <governance/common.h>
#include <governance/vote.h>

#include <llmq/chainlocks.h>
#include <llmq/instantsend.h>
#include <llmq/signing.h>

#include <zmq.h>

#include <cstdarg>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>

using node::ReadBlockFromDisk;

static std::multimap<std::string, CZMQAbstractPublishNotifier*> mapPublishNotifiers;

static const char *MSG_HASHBLOCK     = "hashblock";
static const char *MSG_HASHCHAINLOCK = "hashchainlock";
static const char *MSG_HASHTX        = "hashtx";
static const char *MSG_HASHTXLOCK    = "hashtxlock";
static const char *MSG_HASHGVOTE     = "hashgovernancevote";
static const char *MSG_HASHGOBJ      = "hashgovernanceobject";
static const char *MSG_HASHISCON     = "hashinstantsenddoublespend";
static const char *MSG_HASHRECSIG    = "hashrecoveredsig";
static const char *MSG_RAWBLOCK      = "rawblock";
static const char *MSG_RAWCHAINLOCK  = "rawchainlock";
static const char *MSG_RAWCLSIG      = "rawchainlocksig";
static const char *MSG_RAWTX         = "rawtx";
static const char *MSG_RAWTXLOCK     = "rawtxlock";
static const char *MSG_RAWTXLOCKSIG  = "rawtxlocksig";
static const char *MSG_RAWGVOTE      = "rawgovernancevote";
static const char *MSG_RAWGOBJ       = "rawgovernanceobject";
static const char *MSG_RAWISCON      = "rawinstantsenddoublespend";
static const char *MSG_RAWRECSIG     = "rawrecoveredsig";
static const char *MSG_SEQUENCE      = "sequence";

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
        const std::optional<CNetAddr> addr{LookupHost(ip, false)};
        if (addr.has_value() && addr.value().IsIPv6()) return true;
    }
    return false;
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
        LogPrint(BCLog::ZMQ, "Reusing socket for address %s\n", address);
        LogPrint(BCLog::ZMQ, "Outbound message high water mark for %s at %s is %d\n", type, address, outbound_message_high_water_mark);

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
        LogPrint(BCLog::ZMQ, "Close socket at address %s\n", address);
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
    WriteLE32(msgseq, nSequence);
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
    LogPrint(BCLog::ZMQ, "Publish hashblock %s to %s\n", hash.GetHex(), this->address);
    uint8_t data[32];
    for (unsigned int i = 0; i < 32; i++) {
        data[31 - i] = hash.begin()[i];
    }
    return SendZmqMessage(MSG_HASHBLOCK, data, 32);
}

bool CZMQPublishHashChainLockNotifier::NotifyChainLock(const CBlockIndex *pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "Publish hashchainlock %s to %s\n", hash.GetHex(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHCHAINLOCK, data, 32);
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

bool CZMQPublishHashTransactionLockNotifier::NotifyTransactionLock(const CTransactionRef& transaction, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    uint256 hash = transaction->GetHash();
    LogPrint(BCLog::ZMQ, "Publish hashtxlock %s to %s\n", hash.GetHex(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHTXLOCK, data, 32);
}

bool CZMQPublishHashGovernanceVoteNotifier::NotifyGovernanceVote(const CDeterministicMNList& tip_mn_list, const std::shared_ptr<const CGovernanceVote>& vote)
{
    uint256 hash = vote->GetHash();
    LogPrint(BCLog::ZMQ, "Publish hashgovernancevote %s to %s\n", hash.GetHex(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGVOTE, data, 32);
}

bool CZMQPublishHashGovernanceObjectNotifier::NotifyGovernanceObject(const std::shared_ptr<const Governance::Object>& object)
{
    uint256 hash = object->GetHash();
    LogPrint(BCLog::ZMQ, "Publish hashgovernanceobject %s to %s\n", hash.GetHex(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGOBJ, data, 32);
}

bool CZMQPublishHashInstantSendDoubleSpendNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx)
{
    uint256 currentHash = currentTx->GetHash(), previousHash = previousTx->GetHash();
    LogPrint(BCLog::ZMQ, "Publish hashinstantsenddoublespend %s conflicts against %s to %s\n", currentHash.ToString(), previousHash.ToString(), this->address);
    char dataCurrentHash[32], dataPreviousHash[32];
    for (unsigned int i = 0; i < 32; i++) {
        dataCurrentHash[31 - i] = currentHash.begin()[i];
        dataPreviousHash[31 - i] = previousHash.begin()[i];
    }
    return SendZmqMessage(MSG_HASHISCON, dataCurrentHash, 32)
        && SendZmqMessage(MSG_HASHISCON, dataPreviousHash, 32);
}

bool CZMQPublishHashRecoveredSigNotifier::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig> &sig)
{
    LogPrint(BCLog::ZMQ, "Publish hashrecoveredsig %s to %s\n", sig->getMsgHash().ToString(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = sig->getMsgHash().begin()[i];
    return SendZmqMessage(MSG_HASHRECSIG, data, 32);
}

bool CZMQPublishRawBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    LogPrint(BCLog::ZMQ, "Publish rawblock %s to %s\n", pindex->GetBlockHash().GetHex(), this->address);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    CBlock block;
    if(!ReadBlockFromDisk(block, pindex, consensusParams)) {
        zmqError("Can't read block from disk");
        return false;
    }

    ss << block;

    return SendZmqMessage(MSG_RAWBLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawChainLockNotifier::NotifyChainLock(const CBlockIndex *pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    LogPrint(BCLog::ZMQ, "Publish rawchainlock %s to %s\n", pindex->GetBlockHash().GetHex(), this->address);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
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

    return SendZmqMessage(MSG_RAWCHAINLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawChainLockSigNotifier::NotifyChainLock(const CBlockIndex *pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    LogPrint(BCLog::ZMQ, "Publish rawchainlocksig %s to %s\n", pindex->GetBlockHash().GetHex(), this->address);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    {
        LOCK(cs_main);
        CBlock block;
        if(!ReadBlockFromDisk(block, pindex, consensusParams))
        {
            zmqError("Can't read block from disk");
            return false;
        }

        ss << block;
        ss << *clsig;
    }

    return SendZmqMessage(MSG_RAWCLSIG, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawTransactionNotifier::NotifyTransaction(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint(BCLog::ZMQ, "Publish rawtx %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << transaction;
    return SendZmqMessage(MSG_RAWTX, &(*ss.begin()), ss.size());
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

bool CZMQPublishRawTransactionLockNotifier::NotifyTransactionLock(const CTransactionRef& transaction, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    uint256 hash = transaction->GetHash();
    LogPrint(BCLog::ZMQ, "Publish rawtxlock %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *transaction;
    return SendZmqMessage(MSG_RAWTXLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawTransactionLockSigNotifier::NotifyTransactionLock(const CTransactionRef& transaction, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    uint256 hash = transaction->GetHash();
    LogPrint(BCLog::ZMQ, "Publish rawtxlocksig %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *transaction;
    ss << *islock;
    return SendZmqMessage(MSG_RAWTXLOCKSIG, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawGovernanceVoteNotifier::NotifyGovernanceVote(const CDeterministicMNList& tip_mn_list, const std::shared_ptr<const CGovernanceVote>& vote)
{
    uint256 nHash = vote->GetHash();
    LogPrint(BCLog::ZMQ, "Publish rawgovernanceobject %s with vote %d to %s\n", nHash.ToString(), vote->ToString(tip_mn_list), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *vote;
    return SendZmqMessage(MSG_RAWGVOTE, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawGovernanceObjectNotifier::NotifyGovernanceObject(const std::shared_ptr<const Governance::Object>& govobj)
{
    uint256 nHash = govobj->GetHash();
    LogPrint(BCLog::ZMQ, "Publish rawgovernanceobject %s with type %d to %s\n", nHash.ToString(), ToUnderlying(govobj->type), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *govobj;
    return SendZmqMessage(MSG_RAWGOBJ, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawInstantSendDoubleSpendNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx)
{
    LogPrint(BCLog::ZMQ, "Publish rawinstantsenddoublespend %s conflicts with %s to %s\n", currentTx->GetHash().ToString(), previousTx->GetHash().ToString(), this->address);
    CDataStream ssCurrent(SER_NETWORK, PROTOCOL_VERSION), ssPrevious(SER_NETWORK, PROTOCOL_VERSION);
    ssCurrent << *currentTx;
    ssPrevious << *previousTx;
    return SendZmqMessage(MSG_RAWISCON, &(*ssCurrent.begin()), ssCurrent.size())
        && SendZmqMessage(MSG_RAWISCON, &(*ssPrevious.begin()), ssPrevious.size());
}

bool CZMQPublishRawRecoveredSigNotifier::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig)
{
    LogPrint(BCLog::ZMQ, "Publish rawrecoveredsig %s to %s\n", sig->getMsgHash().ToString(), this->address);

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *sig;

    return SendZmqMessage(MSG_RAWRECSIG, &(*ss.begin()), ss.size());
}

