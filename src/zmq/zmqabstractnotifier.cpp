// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqabstractnotifier.h>

#include <cassert>

const int CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM;

CZMQAbstractNotifier::~CZMQAbstractNotifier()
{
    assert(!psocket);
}

bool CZMQAbstractNotifier::NotifyBlock(const CBlockIndex * /*CBlockIndex*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyChainLock(const CBlockIndex * /*CBlockIndex*/, const std::shared_ptr<const llmq::CChainLockSig> & /*clsig*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransaction(const CTransaction &/*transaction*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransactionLock(const CTransactionRef &/*transaction*/, const std::shared_ptr<const llmq::CInstantSendLock>& /*islock*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote> & /*vote*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyGovernanceObject(const std::shared_ptr<const Governance::Object> & /*object*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef & /*currentTx*/, const CTransactionRef & /*previousTx*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig> & /*sig*/)
{
    return true;
}
