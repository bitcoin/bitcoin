// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqpublishnotifier.h>

#include <chain.h>
#include <chainparams.h>
#include <node/blockstorage.h>
#include <streams.h>
#include <validation.h>
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
#include <string>
#include <utility>

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
    assert(psocket);

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
    LogPrint(BCLog::ZMQ, "zmq: Publish hashblock %s to %s\n", hash.GetHex(), this->address);
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHBLOCK, data, 32);
}

bool CZMQPublishHashChainLockNotifier::NotifyChainLock(const CBlockIndex *pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashchainlock %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHCHAINLOCK, data, 32);
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

bool CZMQPublishHashTransactionLockNotifier::NotifyTransactionLock(const CTransactionRef& transaction, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    uint256 hash = transaction->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashtxlock %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHTXLOCK, data, 32);
}

bool CZMQPublishHashGovernanceVoteNotifier::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote)
{
    uint256 hash = vote->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashgovernancevote %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGVOTE, data, 32);
}

bool CZMQPublishHashGovernanceObjectNotifier::NotifyGovernanceObject(const std::shared_ptr<const Governance::Object>& object)
{
    uint256 hash = object->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashgovernanceobject %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendZmqMessage(MSG_HASHGOBJ, data, 32);
}

bool CZMQPublishHashInstantSendDoubleSpendNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx)
{
    uint256 currentHash = currentTx->GetHash(), previousHash = previousTx->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish hashinstantsenddoublespend %s conflicts against %s\n", currentHash.ToString(), previousHash.ToString());
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
    LogPrint(BCLog::ZMQ, "zmq: Publish hashrecoveredsig %s\n", sig->getMsgHash().ToString());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = sig->getMsgHash().begin()[i];
    return SendZmqMessage(MSG_HASHRECSIG, data, 32);
}

bool CZMQPublishRawBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish rawblock %s to %s\n", pindex->GetBlockHash().GetHex(), this->address);

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

    return SendZmqMessage(MSG_RAWBLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawChainLockNotifier::NotifyChainLock(const CBlockIndex *pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish rawchainlock %s\n", pindex->GetBlockHash().GetHex());

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
    LogPrint(BCLog::ZMQ, "zmq: Publish rawchainlocksig %s\n", pindex->GetBlockHash().GetHex());

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
    LogPrint(BCLog::ZMQ, "zmq: Publish rawtx %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << transaction;
    return SendZmqMessage(MSG_RAWTX, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawTransactionLockNotifier::NotifyTransactionLock(const CTransactionRef& transaction, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    uint256 hash = transaction->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawtxlock %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *transaction;
    return SendZmqMessage(MSG_RAWTXLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawTransactionLockSigNotifier::NotifyTransactionLock(const CTransactionRef& transaction, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    uint256 hash = transaction->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawtxlocksig %s to %s\n", hash.GetHex(), this->address);
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *transaction;
    ss << *islock;
    return SendZmqMessage(MSG_RAWTXLOCKSIG, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawGovernanceVoteNotifier::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote)
{
    uint256 nHash = vote->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawgovernanceobject: hash = %s to %s, vote = %d\n", nHash.ToString(), this->address, vote->ToString());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *vote;
    return SendZmqMessage(MSG_RAWGVOTE, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawGovernanceObjectNotifier::NotifyGovernanceObject(const std::shared_ptr<const Governance::Object>& govobj)
{
    uint256 nHash = govobj->GetHash();
    LogPrint(BCLog::ZMQ, "zmq: Publish rawgovernanceobject: hash = %s to %s, type = %d\n", nHash.ToString(), this->address, ToUnderlying(govobj->type));
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *govobj;
    return SendZmqMessage(MSG_RAWGOBJ, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawInstantSendDoubleSpendNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish rawinstantsenddoublespend %s conflicts with %s\n", currentTx->GetHash().ToString(), previousTx->GetHash().ToString());
    CDataStream ssCurrent(SER_NETWORK, PROTOCOL_VERSION), ssPrevious(SER_NETWORK, PROTOCOL_VERSION);
    ssCurrent << *currentTx;
    ssPrevious << *previousTx;
    return SendZmqMessage(MSG_RAWISCON, &(*ssCurrent.begin()), ssCurrent.size())
        && SendZmqMessage(MSG_RAWISCON, &(*ssPrevious.begin()), ssPrevious.size());
}

bool CZMQPublishRawRecoveredSigNotifier::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig)
{
    LogPrint(BCLog::ZMQ, "zmq: Publish rawrecoveredsig %s\n", sig->getMsgHash().ToString());

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *sig;

    return SendZmqMessage(MSG_RAWRECSIG, &(*ss.begin()), ss.size());
}

