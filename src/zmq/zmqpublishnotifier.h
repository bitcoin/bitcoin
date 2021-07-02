// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
#define SYSCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H

#include <zmq/zmqabstractnotifier.h>
#include <vector>
class CBlockIndex;
// SYSCOIN
class CNEVMBlock;
class uint256;
class CZMQAbstractPublishNotifier : public CZMQAbstractNotifier
{
private:
    uint32_t nSequence {0U}; //!< upcounting per message sequence number

public:

    /* send zmq multipart message
       parts:
          * command
          * data
          * message sequence number
    */
    bool SendZmqMessage(const char *command, const void* data, size_t size);
    // SYSCOIN
    bool SendZmqMessageNEVM(const char *command, const void* data, size_t size);
    /* receive zmq message
       parts:
          * command
          * data
    */
    bool ReceiveZmqMessage(std::vector<std::string>& parts);
    bool Initialize(void *pcontext, void *pcontextsub) override;
    void Shutdown() override;
};
// SYSCOIN
class CZMQPublishNEVMCommsNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyNEVMComms(bool bConnect) override;
};
class CZMQPublishNEVMBlockNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGetNEVMBlock(CNEVMBlock &evmBlock, BlockValidationState &state) override;
};
class CZMQPublishNEVMBlockConnectNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyNEVMBlockConnect(const CNEVMBlock &evmBlock, BlockValidationState &state, const uint256& nBlockHash) override;
};
class CZMQPublishNEVMBlockDisconnectNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyNEVMBlockDisconnect(const CNEVMBlock &evmBlock, BlockValidationState &state, const uint256& nBlockHash) override;
};
class CZMQPublishHashBlockNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyBlock(const CBlockIndex *pindex) override;
};

class CZMQPublishHashTransactionNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyTransaction(const CTransaction &transaction) override;
};

class CZMQPublishRawBlockNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyBlock(const CBlockIndex *pindex) override;
};

class CZMQPublishRawTransactionNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyTransaction(const CTransaction &transaction) override;
};
// SYSCOIN
class CZMQPublishRawMempoolTransactionNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyTransactionMempool(const CTransaction &transaction) override;
};
class CZMQPublishHashGovernanceVoteNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote> &vote) override;
};

class CZMQPublishHashGovernanceObjectNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject> &object) override;
};
class CZMQPublishRawGovernanceVoteNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote) override;
};

class CZMQPublishRawGovernanceObjectNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& object) override;
};
class CZMQPublishSequenceNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyBlockConnect(const CBlockIndex *pindex) override;
    bool NotifyBlockDisconnect(const CBlockIndex *pindex) override;
    bool NotifyTransactionAcceptance(const CTransaction &transaction, uint64_t mempool_sequence) override;
    bool NotifyTransactionRemoval(const CTransaction &transaction, uint64_t mempool_sequence) override;
};

#endif // SYSCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
