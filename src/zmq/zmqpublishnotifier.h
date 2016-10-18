// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
#define BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H

#include "zmqabstractnotifier.h"
#include "validationinterface.h"
#include "txmempool.h"

class CBlockIndex;

class CZMQAbstractPublishNotifier : public CZMQAbstractNotifier
{
private:
    uint32_t nSequence; //!< upcounting per message sequence number

public:

    /* send zmq multipart message
       parts:
          * command
          * data
          * message sequence number
    */
    bool SendMessage(const char *command, const void* data, size_t size);

    bool Initialize(void *pcontext);
    void Shutdown();
};

//! Publish notifier that listens to validation events
class CZMQValidationPublishNotifier : public CZMQAbstractPublishNotifier, public CValidationInterface
{
public:
    CZMQValidationPublishNotifier();
    ~CZMQValidationPublishNotifier();

    // Child classes should override one or more of these from CValidationInterface:
    // void SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, const CBlock* pblock);
    // void UpdatedBlockTip(const CBlockIndex *pindex);
};

class CZMQPublishHashBlockNotifier : public CZMQValidationPublishNotifier
{
public:
    void UpdatedBlockTip(const CBlockIndex *pindex);
};

class CZMQPublishHashTransactionNotifier : public CZMQValidationPublishNotifier
{
public:
    void SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, const CBlock* pblock);
};

class CZMQPublishRawBlockNotifier : public CZMQValidationPublishNotifier
{
public:
    void UpdatedBlockTip(const CBlockIndex *pindex);
};

class CZMQPublishRawTransactionNotifier : public CZMQValidationPublishNotifier
{
public:
    void SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, const CBlock* pblock);
};

class CZMQPublishMempoolNotifier : public CZMQAbstractPublishNotifier
{
public:
    CZMQPublishMempoolNotifier(CTxMemPool *mempoolIn);
    ~CZMQPublishMempoolNotifier();

    void NotifyEntryAdded(const CTxMemPoolEntry &entry);
    void NotifyEntryRemoved(const CTxMemPoolEntry &entry, MemPoolRemovalReason reason);

private:
    CTxMemPool *mempool;
};

#endif // BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
