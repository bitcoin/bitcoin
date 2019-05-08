// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
#define BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H

#include "validationinterface.h"
#include <string>
#include <map>

class CBlockIndex;
class CZMQAbstractNotifier;

class CZMQNotificationInterface : public CValidationInterface
{
public:
    virtual ~CZMQNotificationInterface();

    static CZMQNotificationInterface* Create();

protected:
    bool Initialize();
    void Shutdown();

    // CValidationInterface
    void SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, int posInBlock) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void NotifyChainLock(const CBlockIndex *pindex) override;
    void NotifyTransactionLock(const CTransaction &tx) override;
    void NotifyGovernanceVote(const CGovernanceVote& vote) override;
    void NotifyGovernanceObject(const CGovernanceObject& object) override;
    void NotifyInstantSendDoubleSpendAttempt(const CTransaction &currentTx, const CTransaction &previousTx) override;


private:
    CZMQNotificationInterface();

    void *pcontext;
    std::list<CZMQAbstractNotifier*> notifiers;
};

#endif // BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
