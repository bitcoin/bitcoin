// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "streams.h"
#include "zmqpublishnotifier.h"
#include "validation.h"
#include "util.h"

static std::multimap<std::string, CZMQAbstractPublishNotifier*> mapPublishNotifiers;

static const char *MSG_HASHBLOCK     = "hashblock";
static const char *MSG_HASHCHAINLOCK = "hashchainlock";
static const char *MSG_HASHTX        = "hashtx";
static const char *MSG_HASHTXLOCK    = "hashtxlock";
static const char *MSG_HASHGVOTE     = "hashgovernancevote";
static const char *MSG_HASHGOBJ      = "hashgovernanceobject";
static const char *MSG_HASHISCON     = "hashinstantsenddoublespend";
static const char *MSG_RAWBLOCK      = "rawblock";
static const char *MSG_RAWCHAINLOCK  = "rawchainlock";
static const char *MSG_RAWTX         = "rawtx";
static const char *MSG_RAWTXLOCK     = "rawtxlock";
static const char *MSG_RAWGVOTE      = "rawgovernancevote";
static const char *MSG_RAWGOBJ       = "rawgovernanceobject";
static const char *MSG_RAWISCON      = "rawinstantsenddoublespend";

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
            return -1;
        }

        zmq_msg_close(&msg);

        if (!data)
            break;

        size = va_arg(args, size_t);
    }
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

        int rc = zmq_bind(psocket, address.c_str());
        if (rc!=0)
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
        LogPrint("zmq", "zmq: Reusing socket for address %s\n", address);

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
        LogPrint("zmq", "Close socket at address %s\n", address);
        int linger = 0;
        zmq_setsockopt(psocket, ZMQ_LINGER, &linger, sizeof(linger));
        zmq_close(psocket);
    }

    psocket = 0;
}

bool CZMQAbstractPublishNotifier::SendMessage(const char *command, const void* data, size_t size)
{
    assert(psocket);

    /* send three parts, command & data & a LE 4byte sequence number */
    unsigned char msgseq[sizeof(uint32_t)];
    WriteLE32(&msgseq[0], nSequence);
    int rc = zmq_send_multipart(psocket, command, strlen(command), data, size, msgseq, (size_t)sizeof(uint32_t), (void*)0);
    if (rc == -1)
        return false;

    /* increment memory only sequence number after sending */
    nSequence++;

    return true;
}

bool CZMQPublishHashBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint("zmq", "zmq: Publish hashblock %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendMessage(MSG_HASHBLOCK, data, 32);
}

bool CZMQPublishHashChainLockNotifier::NotifyChainLock(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint("zmq", "zmq: Publish hashchainlock %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendMessage(MSG_HASHCHAINLOCK, data, 32);
}

bool CZMQPublishHashTransactionNotifier::NotifyTransaction(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint("zmq", "zmq: Publish hashtx %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendMessage(MSG_HASHTX, data, 32);
}

bool CZMQPublishHashTransactionLockNotifier::NotifyTransactionLock(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint("zmq", "zmq: Publish hashtxlock %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendMessage(MSG_HASHTXLOCK, data, 32);
}

bool CZMQPublishHashGovernanceVoteNotifier::NotifyGovernanceVote(const CGovernanceVote &vote)
{
    uint256 hash = vote.GetHash();
    LogPrint("zmq", "zmq: Publish hashgovernancevote %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendMessage(MSG_HASHGVOTE, data, 32);
}

bool CZMQPublishHashGovernanceObjectNotifier::NotifyGovernanceObject(const CGovernanceObject &object)
{
    uint256 hash = object.GetHash();
    LogPrint("zmq", "zmq: Publish hashgovernanceobject %s\n", hash.GetHex());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    return SendMessage(MSG_HASHGOBJ, data, 32);
}

bool CZMQPublishHashInstantSendDoubleSpendNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransaction &currentTx, const CTransaction &previousTx)
{
    uint256 currentHash = currentTx.GetHash(), previousHash = previousTx.GetHash();
    LogPrint("zmq", "zmq: Publish hashinstantsenddoublespend %s conflicts against %s\n", currentHash.ToString(), previousHash.ToString());
    char dataCurrentHash[32], dataPreviousHash[32];
    for (unsigned int i = 0; i < 32; i++) {
        dataCurrentHash[31 - i] = currentHash.begin()[i];
        dataPreviousHash[31 - i] = previousHash.begin()[i];
    }
    return SendMessage(MSG_HASHISCON, dataCurrentHash, 32)
        && SendMessage(MSG_HASHISCON, dataPreviousHash, 32);
}


bool CZMQPublishRawBlockNotifier::NotifyBlock(const CBlockIndex *pindex)
{
    LogPrint("zmq", "zmq: Publish rawblock %s\n", pindex->GetBlockHash().GetHex());

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

    return SendMessage(MSG_RAWBLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawChainLockNotifier::NotifyChainLock(const CBlockIndex *pindex)
{
    LogPrint("zmq", "zmq: Publish rawchainlock %s\n", pindex->GetBlockHash().GetHex());

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

    return SendMessage(MSG_RAWCHAINLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawTransactionNotifier::NotifyTransaction(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint("zmq", "zmq: Publish rawtx %s\n", hash.GetHex());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << transaction;
    return SendMessage(MSG_RAWTX, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawTransactionLockNotifier::NotifyTransactionLock(const CTransaction &transaction)
{
    uint256 hash = transaction.GetHash();
    LogPrint("zmq", "zmq: Publish rawtxlock %s\n", hash.GetHex());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << transaction;
    return SendMessage(MSG_RAWTXLOCK, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawGovernanceVoteNotifier::NotifyGovernanceVote(const CGovernanceVote &vote)
{
    uint256 nHash = vote.GetHash();
    LogPrint("gobject", "gobject: Publish rawgovernanceobject: hash = %s, vote = %d\n", nHash.ToString(), vote.ToString());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << vote;
    return SendMessage(MSG_RAWGVOTE, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawGovernanceObjectNotifier::NotifyGovernanceObject(const CGovernanceObject &govobj)
{
    uint256 nHash = govobj.GetHash();
    LogPrint("gobject", "gobject: Publish rawgovernanceobject: hash = %s, type = %d\n", nHash.ToString(), govobj.GetObjectType());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << govobj;
    return SendMessage(MSG_RAWGOBJ, &(*ss.begin()), ss.size());
}

bool CZMQPublishRawInstantSendDoubleSpendNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransaction &currentTx, const CTransaction &previousTx)
{
    LogPrint("zmq", "zmq: Publish rawinstantsenddoublespend %s conflicts with %s\n", currentTx.GetHash().ToString(), previousTx.GetHash().ToString());
    CDataStream ssCurrent(SER_NETWORK, PROTOCOL_VERSION), ssPrevious(SER_NETWORK, PROTOCOL_VERSION);
    ssCurrent << currentTx;
    ssPrevious << previousTx;
    return SendMessage(MSG_RAWISCON, &(*ssCurrent.begin()), ssCurrent.size())
        && SendMessage(MSG_RAWISCON, &(*ssPrevious.begin()), ssPrevious.size());
}
