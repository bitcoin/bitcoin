// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
#define SYSCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H

#include <zmq/zmqabstractnotifier.h>

#include <cstddef>
#include <cstdint>

class CBlockIndex;
class CTransaction;
// SYSCOIN
class CNEVMBlock;
class CNEVMHeader;
class CBlock;
class uint256;
class CNEVMData;
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
    bool NotifyNEVMCommsCommon(const std::string& commMessage, bool &bResponse);
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
    bool NotifyNEVMComms(const std::string& commMessage, bool &bResponse) override;
};
class CZMQPublishNEVMBlockInfoNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGetNEVMBlockInfo(uint64_t &nHeight, std::string &state) override;
};
class CZMQPublishNEVMBlockNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGetNEVMBlock(CNEVMBlock &evmBlock, std::string &state) override;
};

class CZMQPublishNEVMBlockConnectNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyNEVMBlockConnect(const CNEVMHeader &evmBlock, const CBlock& block, std::string &state, const uint256& nBlockHash, NEVMDataVec &NEVMDataVecOut, const uint32_t& nHeight, bool bSkipValidation) override;
};
class CZMQPublishNEVMBlockDisconnectNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyNEVMBlockDisconnect(std::string &state, const uint256& nBlockHash) override;
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
    bool NotifyGovernanceVote(const uint256 &vote) override;
};

class CZMQPublishHashGovernanceObjectNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyGovernanceObject(const uint256 &object) override;
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
