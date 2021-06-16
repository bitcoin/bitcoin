// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqabstractnotifier.h>

#include <cassert>

const int CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM;

CZMQAbstractNotifier::~CZMQAbstractNotifier()
{
    assert(!psocket);
}
// SYSCOIN
bool CZMQAbstractNotifier::NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& /*object*/)
{
    return true;
}
bool CZMQAbstractNotifier::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& /*vote*/)
{
    return true;
}
bool CZMQAbstractNotifier::NotifyEVMBlockConnect(const CNEVMBlock &evmBlock)
{
    return true;
}
bool CZMQAbstractNotifier::NotifyEVMBlockDisconnect(const CNEVMBlock &evmBlock)
{
    return true;
}
bool CZMQAbstractNotifier::NotifyGetNEVMBlock(CNEVMBlock &evmBlock)
{
    return true;
}
bool CZMQAbstractNotifier::NotifyTransactionMempool(const CTransaction &/*transaction*/)
{
    return true;
}
bool CZMQAbstractNotifier::NotifyBlock(const CBlockIndex * /*CBlockIndex*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransaction(const CTransaction &/*transaction*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyBlockConnect(const CBlockIndex * /*CBlockIndex*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyBlockDisconnect(const CBlockIndex * /*CBlockIndex*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransactionAcceptance(const CTransaction &/*transaction*/, uint64_t mempool_sequence)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransactionRemoval(const CTransaction &/*transaction*/, uint64_t mempool_sequence)
{
    return true;
}
