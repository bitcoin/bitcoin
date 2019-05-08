// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zmqabstractnotifier.h"
#include "util.h"


CZMQAbstractNotifier::~CZMQAbstractNotifier()
{
    assert(!psocket);
}

bool CZMQAbstractNotifier::NotifyBlock(const CBlockIndex * /*CBlockIndex*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyChainLock(const CBlockIndex * /*CBlockIndex*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransaction(const CTransaction &/*transaction*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransactionLock(const CTransaction &/*transaction*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyGovernanceVote(const CGovernanceVote& /*vote*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyGovernanceObject(const CGovernanceObject& /*object*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransaction& /*currentTx*/, const CTransaction& /*previousTx*/)
{
    return true;
}
