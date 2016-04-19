// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "zmqpublishnotifier.h"
#include "main.h"
#include "util.h"
#include "crypto/common.h"

static std::multimap<std::string, CZMQAbstractPublishNotifier*> mapPublishNotifiers;

static const char *MSG_HASHBLOCK = "hashblock";
static const char *MSG_HASHTX    = "hashtx";
static const char *MSG_RAWBLOCK  = "rawblock";
static const char *MSG_RAWTX     = "rawtx";
static const char *MSG_MEMPOOLADDED = "mempooladded";
static const char *MSG_MEMPOOLREMOVED = "mempoolremoved";

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

CZMQValidationPublishNotifier::CZMQValidationPublishNotifier()
{
    RegisterValidationInterface(this);
}

CZMQValidationPublishNotifier::~CZMQValidationPublishNotifier()
{
    UnregisterValidationInterface(this);
}

/** Write 256-bit hash to buffer reversed format */
static void ZMQWriteHash(unsigned char *ptr, const uint256 &hash)
{
    for (unsigned int i = 0; i < 32; i++)
        ptr[31 - i] = hash.begin()[i];
}

void CZMQPublishHashBlockNotifier::UpdatedBlockTip(const CBlockIndex *pindex)
{
    uint256 hash = pindex->GetBlockHash();
    LogPrint("zmq", "zmq: Publish hashblock %s\n", hash.GetHex());
    uint8_t data[32];
    ZMQWriteHash(data, hash);
    SendMessage(MSG_HASHBLOCK, data, 32);
}

void CZMQPublishHashTransactionNotifier::SyncTransaction(const CTransaction& transaction, const CBlockIndex * /*pindex*/, const CBlock* /*pblock*/)
{
    uint256 hash = transaction.GetHash();
    LogPrint("zmq", "zmq: Publish hashtx %s\n", hash.GetHex());
    uint8_t data[32];
    ZMQWriteHash(data, hash);
    SendMessage(MSG_HASHTX, data, 32);
}

void CZMQPublishRawBlockNotifier::UpdatedBlockTip(const CBlockIndex *pindex)
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
            return;
        }

        ss << block;
    }

    SendMessage(MSG_RAWBLOCK, &(*ss.begin()), ss.size());
}

void CZMQPublishRawTransactionNotifier::SyncTransaction(const CTransaction& transaction, const CBlockIndex * /*pindex*/, const CBlock* /*pblock*/)
{
    uint256 hash = transaction.GetHash();
    LogPrint("zmq", "zmq: Publish rawtx %s\n", hash.GetHex());
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << transaction;
    SendMessage(MSG_RAWTX, &(*ss.begin()), ss.size());
}

CZMQPublishMempoolNotifier::CZMQPublishMempoolNotifier(CTxMemPool *mempoolIn):
    mempool(mempoolIn)
{
    mempool->NotifyEntryAdded.connect(boost::bind(&CZMQPublishMempoolNotifier::NotifyEntryAdded, this, _1));
    mempool->NotifyEntryRemoved.connect(boost::bind(&CZMQPublishMempoolNotifier::NotifyEntryRemoved, this, _1, _2));
}

CZMQPublishMempoolNotifier::~CZMQPublishMempoolNotifier()
{
    mempool->NotifyEntryAdded.disconnect(boost::bind(&CZMQPublishMempoolNotifier::NotifyEntryAdded, this, _1));
    mempool->NotifyEntryRemoved.disconnect(boost::bind(&CZMQPublishMempoolNotifier::NotifyEntryRemoved, this, _1, _2));
}

void CZMQPublishMempoolNotifier::NotifyEntryAdded(const CTxMemPoolEntry &entry)
{
    LogPrint("zmq", "zmq: mempool entry added: %s fee=%i size=%i\n", entry.GetTx().GetHash().ToString(),
            entry.GetFee(), entry.GetTxSize());
    uint8_t data[32 + 8 + 4];
    ZMQWriteHash(&data[0], entry.GetTx().GetHash());
    WriteLE64(&data[32], entry.GetFee());
    WriteLE32(&data[40], entry.GetTxSize());
    SendMessage(MSG_MEMPOOLADDED, data, sizeof(data));
}

/**
 * Removal reason on ZMQ notification protocol.
 * Keep this up to date with the documentation in doc/zmq.md
 */
enum class ZMQMempoolRemovalReason: uint8_t {
    UNKNOWN = 0,   //! Manually removed or unknown reason
    EXPIRY = 1,    //! Expired from mempool
    SIZELIMIT = 2, //! Removed in size limiting
    REORG = 3,     //! Removed for reorganization
    BLOCK = 4,     //! Removed for block
    REPLACED = 5   //! Removed for conflict (replaced)
};

ZMQMempoolRemovalReason MapRemovalReasonToZMQ(MemPoolRemovalReason reason)
{
    switch(reason)
    {
    case MemPoolRemovalReason::UNKNOWN: return ZMQMempoolRemovalReason::UNKNOWN;
    case MemPoolRemovalReason::EXPIRY: return ZMQMempoolRemovalReason::EXPIRY;
    case MemPoolRemovalReason::SIZELIMIT: return ZMQMempoolRemovalReason::SIZELIMIT;
    case MemPoolRemovalReason::REORG: return ZMQMempoolRemovalReason::REORG;
    case MemPoolRemovalReason::BLOCK: return ZMQMempoolRemovalReason::BLOCK;
    case MemPoolRemovalReason::REPLACED: return ZMQMempoolRemovalReason::REPLACED;
    // No default clause: we want a warning here if not all values are covered.
    }
    return ZMQMempoolRemovalReason::UNKNOWN;
}

void CZMQPublishMempoolNotifier::NotifyEntryRemoved(const CTxMemPoolEntry &entry, MemPoolRemovalReason reason)
{
    LogPrint("zmq", "zmq: mempool entry removed: %s, reason %i\n", entry.GetTx().GetHash().ToString(), (int)reason);
    uint8_t data[32 + 1];
    ZMQWriteHash(&data[0], entry.GetTx().GetHash());
    data[32] = static_cast<uint8_t>(MapRemovalReasonToZMQ(reason));
    SendMessage(MSG_MEMPOOLREMOVED, data, sizeof(data));
}

