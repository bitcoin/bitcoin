// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_ZMQ_ZMQABSTRACTNOTIFIER_H
#define SYSCOIN_ZMQ_ZMQABSTRACTNOTIFIER_H


#include <memory>
#include <string>

class CBlockIndex;
class CTransaction;
class CZMQAbstractNotifier;
// SYSCOIN
class CGovernanceObject;
class CGovernanceVote;
class CNEVMBlock;
using CZMQNotifierFactory = std::unique_ptr<CZMQAbstractNotifier> (*)();

class CZMQAbstractNotifier
{
public:
    static const int DEFAULT_ZMQ_SNDHWM {1000};
    // SYSCOIN
    CZMQAbstractNotifier() : psocket(nullptr), psocketsub(nullptr), outbound_message_high_water_mark(DEFAULT_ZMQ_SNDHWM) { }
    virtual ~CZMQAbstractNotifier();

    template <typename T>
    static std::unique_ptr<CZMQAbstractNotifier> Create()
    {
        return std::make_unique<T>();
    }

    std::string GetType() const { return type; }
    void SetType(const std::string &t) { type = t; }
    std::string GetAddress() const { return address; }
    void SetAddress(const std::string &a) { address = a; }
    int GetOutboundMessageHighWaterMark() const { return outbound_message_high_water_mark; }
    void SetOutboundMessageHighWaterMark(const int sndhwm) {
        if (sndhwm >= 0) {
            outbound_message_high_water_mark = sndhwm;
        }
    }
    // SYSCOIN
    virtual bool Initialize(void *pcontext, void *pcontextsub) = 0;
    virtual void Shutdown() = 0;

    // Notifies of ConnectTip result, i.e., new active tip only
    virtual bool NotifyBlock(const CBlockIndex *pindex);
    // Notifies of every block connection
    virtual bool NotifyBlockConnect(const CBlockIndex *pindex);
    // Notifies of every block disconnection
    virtual bool NotifyBlockDisconnect(const CBlockIndex *pindex);
    // Notifies of every mempool acceptance
    virtual bool NotifyTransactionAcceptance(const CTransaction &transaction, uint64_t mempool_sequence);
    // Notifies of every mempool removal, except inclusion in blocks
    virtual bool NotifyTransactionRemoval(const CTransaction &transaction, uint64_t mempool_sequence);
    // Notifies of transactions added to mempool or appearing in blocks
    virtual bool NotifyTransaction(const CTransaction &transaction);
    // SYSCOIN
    virtual bool NotifyTransactionMempool(const CTransaction &transaction);
    virtual bool NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote);
    virtual bool NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& object);
    virtual bool NotifyEVMBlockConnect(const CNEVMBlock &evmBlock);
    virtual bool NotifyEVMBlockDisconnect(const CNEVMBlock &evmBlock);
    virtual bool NotifyGetNEVMBlock(CNEVMBlock &evmBlock);

protected:
    void *psocket;
    // SYSCOIN
    void *psocketsub;
    std::string type;
    std::string address;
    int outbound_message_high_water_mark; // aka SNDHWM
};

#endif // SYSCOIN_ZMQ_ZMQABSTRACTNOTIFIER_H
